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

 // The tape signal has its own raster. The game and its subtitles keep their display resolution.
 float strength = min(inten, 1.0);
 vec2 grid = vec2(tapeRows * outputPixel.y / outputPixel.x, tapeRows);
 vec2 cell = floor(uv * grid);
 vec2 centre = (cell + 0.5) / grid;
 vec2 tc = mix(uv, centre, 0.78 * strength);
 uint field = uint(floor(tm * 30.0));
 float rowNoise = tapeNoise(uvec2(0u, uint(cell.y)), field) - 0.5;
 float tracking = tapeNoise(uvec2(1u, uint(cell.y) / 3u), field);
 tc.x += (rowNoise * 0.85 + 2.5 * smoothstep(0.987, 1.0, tracking)) * inten / grid.x;

 // Luma keeps a sharper centre sample. Colour spreads sideways along the scanline.
 vec3 centreColor = screenSample(tc);
 float spread = (0.55 + 0.30 * strength) / grid.x;
 vec3 left = screenSample(tc - vec2(spread, 0.0));
 vec3 right = screenSample(tc + vec2(spread, 0.0));
 vec3 tape = centreColor * 0.50 + left * vec3(0.40, 0.25, 0.10) + right * vec3(0.10, 0.25, 0.40);
 vec3 c = mix(centreColor, tape, strength);
 float luma = dot(c, vec3(0.299, 0.587, 0.114));

 float grain = tapeNoise(uvec2(cell), field) - 0.5;
 c += grain * (0.095 + 0.045 * (1.0 - smoothstep(0.05, 0.55, luma))) * inten;
 // Fade the line pattern as the signal approaches the display's Nyquist limit.
 float lineVisibility = 1.0 - smoothstep(0.55, 1.0, tapeRows * outputPixel.y);
 float scan = 0.5 - 0.5 * cos(uv.y * tapeRows * 6.2831853);
 c *= 1.0 - 0.085 * scan * lineVisibility * strength;
 vec2 fromCentre = uv - 0.5;
 c *= 1.0 - 0.32 * dot(fromCentre, fromCentre) * strength;
 F = vec4(deathPost(uv, c), 1.0);
})";
