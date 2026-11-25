#pragma once
inline constexpr char vhsFS[] = R"(#version 330
out vec4 F; in vec2 uv;
uniform sampler2D tex;
uniform float tm, inten, deathFx, uLmPix, tapeRows;
uniform vec3 outputPixel;
// Integer noise keeps successive tape fields distinct even on long sessions.
float tapeNoise(uvec2 cell, uint field){
 uint n = cell.x * 0x9e3779b9u + cell.y * 0x85ebca6bu + field * 0xc2b2ae35u;
 n ^= n >> 16u; n *= 0x7feb352du; n ^= n >> 15u;
 return float(n >> 8u) / 16777216.0;
}
// Tape faults run along the scanline, so the noise is smooth along x and independent per line.
float lineNoise(float x, uint line, uint field){
 float i = floor(x), f = x - i;
 uint c = uint(int(i) + 65536);
 return mix(tapeNoise(uvec2(c, line), field), tapeNoise(uvec2(c + 1u, line), field), f * f * (3.0 - 2.0 * f));
}
vec3 screenSample(vec2 tc){
 if (uLmPix > 1.0) {
  vec2 grid = vec2(uLmPix * outputPixel.y / outputPixel.x, uLmPix);
  tc = (floor(tc * grid) + 0.5) / grid;
 }
 return texture(tex, tc).rgb;
}
vec3 deathPost(vec2 tc, vec3 base){
 float d = clamp(deathFx, 0.0, 1.0); if(d <= 0.001) return base;
 vec2 dir = tc - 0.5; float len2 = dot(dir, dir); vec2 nDir = len2 > 0.000001 ? normalize(dir) : vec2(1.0, 0.0);
 float blur = (0.002 + sqrt(len2) * 0.010) * d;
 vec3 b0 = screenSample(tc + nDir * blur) * 0.24 + screenSample(tc - nDir * blur) * 0.24 + screenSample(tc + nDir * blur * 2.2) * 0.11 + screenSample(tc - nDir * blur * 2.2) * 0.11 + base * 0.30;
 float ca = (0.001 + sqrt(len2) * 0.006) * d; vec3 caC = vec3(screenSample(tc + nDir * ca).r, b0.g, screenSample(tc - nDir * ca).b);
 float fog = smoothstep(0.15, 0.9, sqrt(len2)) * d * 0.45; return mix(mix(base, caC, 0.75 * d), vec3(dot(caC, vec3(0.299,0.587,0.114))), 0.22 * d) * (1.0 - fog);
}
void main(){
 if(inten <= 0.0) {
  vec3 c0 = screenSample(uv);
  c0 = deathPost(uv, c0);
  F = vec4(c0, 1.0);
 return;
 }

 // The tape has discrete scanlines but is continuous along each of them, so the picture is
 // never cut into square cells: rows are quantised, columns are only band-limited.
 float strength = min(inten, 1.0);
 float soft = smoothstep(0.0, 0.3, strength);
 vec2 grid = vec2(tapeRows * outputPixel.y / outputPixel.x, tapeRows);
 float px = 1.0 / grid.x;
 uint field = uint(floor(tm * 60.0));
 float line = floor(uv.y * tapeRows);
 uint row = uint(line);
 float rowPos = uv.y * tapeRows - 0.5, rowBase = floor(rowPos);
 vec2 tc = vec2(uv.x, mix(uv.y, (rowBase + 0.5 + smoothstep(0.2, 0.8, rowPos - rowBase)) / tapeRows, soft));

 // Time base error: neighbouring lines drift together, they do not jump independently.
 float shift = ((lineNoise(line / 90.0 + tm * 0.9, 7u, 0u) - 0.5) * 1.3
              + (lineNoise(line / 7.0, 11u, field) - 0.5) * 0.45) * inten;
 // Head switching bends the last few lines of every field.
 float headSwitch = 1.0 - smoothstep(0.0, 0.010 + 0.012 * strength, uv.y);
 headSwitch *= headSwitch * soft;
 shift += headSwitch * (12.0 + 7.0 * tapeNoise(uvec2(2u, row), field));
 // Tracking slips are events: a torn band crosses the picture now and then, more often under stress.
 float slotTime = tm / 2.7, phase = fract(slotTime);
 uint slot = uint(floor(slotTime));
 float slip = step(1.0 - clamp((inten - 0.2) * 0.9, 0.0, 0.85), tapeNoise(uvec2(3u, 0u), slot))
            * smoothstep(0.0, 0.04, phase) * (1.0 - smoothstep(0.22, 0.34, phase));
 float bandOffset = uv.y - fract(tapeNoise(uvec2(4u, 0u), slot) + phase * 0.8);
 float band = slip * exp(-bandOffset * bandOffset / (0.0012 + 0.0022 * strength));
 shift += band * ((lineNoise(line / 3.0, 5u, field) - 0.5) * 16.0 + 5.0) * strength;
 tc.x -= shift * px;
 float fault = clamp(headSwitch + band, 0.0, 1.0);

 // One pass over the line: luma keeps about 1.5 tape samples of detail, colour-under chroma is
 // several times wider and lags behind, which gives a soft wash instead of split colour edges.
 const vec3 lumaWeights = vec3(0.299, 0.587, 0.114);
 float tap = px * soft * (0.6 + 0.3 * strength);
 float lumaSum = 0.0, lumaNorm = 0.0, wideSum = 0.0, wideNorm = 0.0, chromaNorm = 0.0;
 vec3 chroma = vec3(0.0);
 for (int k = -4; k <= 4; ++k) {
  float o = float(k), lag = o + 1.0;
  vec3 s = screenSample(tc + vec2(o * tap, 0.0));
  float y = dot(s, lumaWeights);
  float wLuma = exp(-o * o * 0.62) * (k < 0 ? 1.2 : 1.0);
  float wWide = exp(-o * o * 0.14), wChroma = exp(-lag * lag * 0.10);
  lumaSum += y * wLuma; lumaNorm += wLuma;
  wideSum += y * wWide; wideNorm += wWide;
  chroma += (s - y) * wChroma; chromaNorm += wChroma;
 }
 float luma = lumaSum / lumaNorm;
 // The deck's edge enhancer leaves a faint halo next to hard edges.
 luma += (0.30 + 0.30 * strength) * soft * (luma - wideSum / wideNorm);
 chroma *= (1.0 - 0.16 * strength) * (1.0 - 0.85 * band) / chromaNorm;

 // Luma noise is streaky and stronger in the shadows. Chroma noise is slow coloured blotches.
 float along = uv.x * grid.x + 37.0 * tapeNoise(uvec2(9u, row), 0u);
 float dark = 1.0 - smoothstep(0.05, 0.6, luma);
 float streak = lineNoise(along / 3.2, row, field) * 0.65 + lineNoise(along / 1.3, row + 4096u, field) * 0.35 - 0.5;
 luma += streak * (0.045 + 0.05 * dark) * inten * (1.0 + 4.0 * fault);
 vec2 hue = vec2(lineNoise(along / 13.0, row / 2u + 8192u, field), lineNoise(along / 13.0, row / 2u + 12288u, field)) - 0.5;
 hue *= (0.035 + 0.06 * dark) * inten;
 chroma += vec3(hue.x, -(0.299 * hue.x + 0.114 * hue.y) / 0.587, hue.y);

 // Dropouts: the head loses the tape for a moment and leaves a short white comet on one line.
 float dropTail = (uv.x - tapeNoise(uvec2(7u, row), field)) / ((5.0 + 40.0 * tapeNoise(uvec2(8u, row), field)) * px);
 float drop = step(tapeNoise(uvec2(6u, row), field), (0.0005 + 0.02 * band) * inten)
            * smoothstep(0.0, 0.1, dropTail) * exp(-2.5 * max(dropTail, 0.0));
 luma = mix(luma, 0.92, 0.85 * drop);
 chroma *= 1.0 - drop;

 // Tape cannot hold true black or full saturation.
 luma = luma * (1.0 - 0.05 * strength) + 0.02 * strength;
 vec3 c = max(vec3(luma) + chroma, 0.0);
 // The shifted lines of a fault run out of picture at the frame edge.
 c *= mix(1.0, step(0.0, tc.x) * step(tc.x, 1.0), fault);
 // Fade the line pattern as the signal approaches the display's Nyquist limit.
 float lineVisibility = 1.0 - smoothstep(0.55, 1.0, tapeRows * outputPixel.y);
 float scan = 0.5 - 0.5 * cos(uv.y * tapeRows * 6.2831853);
 c *= 1.0 - 0.085 * scan * lineVisibility * strength;
 vec2 fromCentre = uv - 0.5;
 c *= 1.0 - 0.32 * dot(fromCentre, fromCentre) * strength;
 F = vec4(deathPost(uv, c), 1.0);
})";
