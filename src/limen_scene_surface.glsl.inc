R"(#version 330
out vec4 F; in vec2 uv; in vec3 fp,nm;
in vec3 viewTS; in vec3 fragPosTS;
uniform sampler2D tex;
uniform sampler2D envMap;
uniform vec3 envRect;
uniform vec3 vp; uniform float tm; uniform vec3 modelTint;
uniform int nl; uniform vec3 lp[16]; uniform float danger;
uniform int flashOn; uniform vec3 flashDir; uniform vec3 flashPos;
uniform int rfc; uniform vec3 rfp[4]; uniform vec3 rfd[4];
uniform float megaEnv;
uniform vec3 uLightCol;
uniform vec3 uFogCol;
uniform vec3 uFogDens;
uniform vec3 uMegaFog[3];
uniform vec3 uMegaAmb[3];
uniform vec3 uFloorJoint;
uniform vec3 uMood;
uniform vec3 uQA[8];
uniform vec3 uQB[8];
#define uQ(i) vec4(uQA[i], uQB[i].x)
uniform vec3 uCam;
uniform float uPass;
uniform vec3 uSkyTop;
uniform vec3 uSkyHor;
uniform vec3 uUnstruct;
uniform vec3 uUnstructInfo;
noperspective in vec2 uvAff;

float hash(vec3 p) {
 return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}
float lmH2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
vec3 lmHue(float h) { return clamp(abs(fract(h + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0); }
float lmVN(vec2 p) {
 vec2 i = floor(p), f = fract(p);
 f = f * f * (3.0 - 2.0 * f);
 return mix(mix(lmH2(i), lmH2(i + vec2(1.0, 0.0)), f.x), mix(lmH2(i + vec2(0.0, 1.0)), lmH2(i + vec2(1.0, 1.0)), f.x), f.y);
}
// distance to the nearest cell border of a Voronoi tiling (cracks)
float lmCrack(vec2 p) {
 vec2 ip = floor(p), fr = fract(p);
 float d1 = 8.0, d2 = 8.0;
 for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) {
  vec2 g = vec2(float(x), float(y));
  vec2 o = vec2(lmH2(ip + g), lmH2(ip + g + 17.3));
  vec2 r = g + o - fr;
  float d = dot(r, r);
  if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
 }
 return sqrt(d2) - sqrt(d1);
}
// Party: every lamp has its own colour, drifting round the wheel and pulsing at its own rate.
vec3 lmLampCol(vec2 lampXZ) {
 if (uQ(2).x < 0.5) return uLightCol;
 float h = lmH2(floor(lampXZ / 5.0));
 float beat = 0.70 + 0.30 * sin(tm * (2.2 + 3.4 * fract(h * 7.3)) + h * 40.0);
 return mix(vec3(1.0), lmHue(h + tm * 0.021 * (0.4 + h)), 0.86) * beat * 1.12;
}
// The sky seen along direction d.
vec3 lmSky(vec3 d) {
 int kind = int(uQ(5).x + 0.5) - 1;
 float up = d.y;
 vec3 col = mix(uSkyHor, uSkyTop, pow(clamp(up, 0.0, 1.0), 0.55));
 if (kind == 0) {                                   // overcast: a lid of cloud with no sun in it
  vec2 q = d.xz / max(up, 0.07) * 0.55;
  float n = lmVN(q) * 0.6 + lmVN(q * 2.7 + 3.1) * 0.4;
  col *= 0.93 + 0.10 * n * smoothstep(0.0, 0.25, up);
 } else if (kind == 1) {                            // nursery-wallpaper clouds
  vec2 q = d.xz / max(up, 0.06) * 0.85 + vec2(tm * 0.004, 0.0);
  float n = lmVN(q) * 0.55 + lmVN(q * 2.3 + 7.1) * 0.30 + lmVN(q * 5.1 + 3.3) * 0.15;
  float cl = smoothstep(0.54, 0.585, n) * smoothstep(0.03, 0.16, up);
  float lit = smoothstep(0.585, 0.74, n);
  col = mix(col, mix(vec3(0.80, 0.85, 0.93), vec3(0.985), lit), cl);
 } else if (kind == 2) {                            // night: stars and a moon that lights nothing
  vec3 g = floor(d * 170.0);
  float st = smoothstep(0.9972, 1.0, hash(g)) * smoothstep(0.02, 0.22, up);
  col += vec3(0.85, 0.88, 1.0) * st * (0.65 + 0.35 * sin(tm * 2.0 + hash(g + 3.0) * 40.0));
  float md = dot(d, normalize(vec3(-0.42, 0.52, 0.74)));
  col += vec3(0.95, 0.94, 0.86) * smoothstep(0.9988, 0.99915, md) + vec3(0.20, 0.22, 0.30) * pow(max(md, 0.0), 180.0);
 } else if (kind == 3) {                            // dusk that never ends
  vec3 sd = normalize(vec3(0.80, 0.06, 0.55));
  float sdot = max(dot(d, sd), 0.0);
  col = mix(col, uSkyHor * vec3(1.05, 0.80, 0.62), pow(sdot, 6.0) * 0.6);
  col += vec3(1.0, 0.82, 0.55) * (smoothstep(0.9990, 0.9995, sdot) * 0.9 + pow(sdot, 90.0) * 0.35);
  float bands = lmVN(vec2(atan(d.z, d.x) * 2.2, up * 26.0));
  col *= 1.0 - 0.34 * smoothstep(0.55, 0.75, bands) * smoothstep(0.30, 0.04, abs(up - 0.13));
 } else if (kind == 4) {                            // another ceiling, seventy metres up, lamps on
  if (up > 0.015) {
   float tH = 70.0 / up;
   vec2 q = uCam.xz + d.xz * tH;
   vec2 gq = abs(fract(q / 10.0) - 0.5) * 10.0;
   float lamp = (1.0 - smoothstep(1.3, 1.6, gq.x)) * (1.0 - smoothstep(0.6, 0.8, gq.y));
   float dead = step(0.12, lmH2(floor(q / 10.0)));
   float fall = exp(-tH * 0.0042);
   vec2 gs = abs(fract(q / 2.5) - 0.5);
   float seam = smoothstep(0.47, 0.5, max(gs.x, gs.y));
   col = mix(uSkyTop, uSkyHor * (1.0 - 0.35 * seam) + uLightCol * lamp * dead * 1.25, fall);
  } else col = uSkyTop;
 }
 if (up < 0.0) col = mix(col, max(uFogCol, vec3(0.02)), clamp(-up * 5.0, 0.0, 1.0));
 return col;
}
// Greybox: the developer grid, metre squares with a heavier line every five.
vec3 lmDevGrid(vec3 P, vec3 N, float dist) {
 vec3 an = abs(N);
 vec2 q; vec3 base;
 if (an.y > 0.6) { q = P.xz; base = (N.y > 0.0) ? uQ(7).rgb : uQ(7).rgb * 0.50; }
 else if (an.x > an.z) { q = P.zy; base = uQ(6).rgb; }
 else { q = P.xy; base = uQ(6).rgb; }
 vec2 c1 = floor(q);
 base *= mix(0.90, 1.0, mod(c1.x + c1.y, 2.0));
 float px = clamp(dist * 0.0016, 0.0, 0.2);
 vec2 f1 = 0.5 - abs(fract(q) - 0.5);
 float l1 = (1.0 - smoothstep(0.010 + px, 0.022 + px * 2.0, min(f1.x, f1.y))) * (1.0 - smoothstep(18.0, 40.0, dist));
 vec2 f5 = (0.5 - abs(fract(q / 5.0) - 0.5)) * 5.0;
 float l5 = 1.0 - smoothstep(0.022 + px, 0.05 + px * 2.0, min(f5.x, f5.y));
 base = mix(base, base * 0.62, l1);
 return mix(base, vec3(0.93), l5 * 0.75);
}
// Integrate a periodic stripe over the pixel footprint on each world axis.
// This preserves a thin joint's coverage even at a grazing angle or low render scale.
float lmJointCoverage(vec2 world, vec2 footprint, float spacing, float halfWidth) {
 vec2 phase = fract(world / spacing) + halfWidth / spacing;
 vec2 width = footprint / spacing;
 float duty = (2.0 * halfWidth) / spacing;
 vec2 lo = phase - 0.5 * width, hi = phase + 0.5 * width;
 vec2 area = (floor(hi) - floor(lo)) * duty
           + min(fract(hi), vec2(duty)) - min(fract(lo), vec2(duty));
 vec2 cover = clamp(area / width, 0.0, 1.0);
 return cover.x + cover.y - cover.x * cover.y;
}
vec3 palFogSmooth(float p) {
 return (p < 0.5) ? mix(uMegaFog[0], uMegaFog[1], p * 2.0) : mix(uMegaFog[1], uMegaFog[2], (p - 0.5) * 2.0);
}
vec3 palAmbSmooth(float p) {
 return (p < 0.5) ? mix(uMegaAmb[0], uMegaAmb[1], p * 2.0) : mix(uMegaAmb[1], uMegaAmb[2], (p - 0.5) * 2.0);
}
float lmSnowFlakes(vec3 d, float dist, float t) {
 if (dist < 2.2) return 0.0;
 float az = atan(d.z, d.x) * 0.15915494 + 0.5;
 float el = d.y;
 float acc = 0.0;
 for (int layer = 0; layer < 3; layer++) {
  float fl = float(layer);
  float per = 112.0 + fl * 56.0;
  vec2 p = vec2(az * per + sin(t * 0.35 + fl) * 0.4, el * per * 0.55 + t * (0.55 + fl * 0.22));
  vec2 ip = floor(p);
  ip.x = mod(ip.x, per);
  vec2 f = fract(p) - 0.5;
  float h = lmH2(ip + fl * 37.0);
  float thresh = 0.915 - 0.015 * fl;
  if (h > thresh && dist > (3.2 + fl * 2.5)) {
   vec2 j = (vec2(fract(h * 19.3), fract(h * 41.7)) - 0.5) * 0.36;
   vec2 q = floor((f - j) * 5.0 + 0.5);
   float crossS = ((abs(q.x) + abs(q.y)) <= 1.0) ? 1.0 : 0.0;
   float diagS = (abs(q.x) == abs(q.y) && abs(q.x) <= 1.0) ? 1.0 : 0.0;
   float shape = (fract(h * 73.1) > 0.45) ? crossS : diagS;
   acc = max(acc, shape * (0.72 - 0.14 * fl) * smoothstep(2.2, 11.0, dist));
  }
 }
 return acc;
}
vec3 lmUnstructSkyColor(int uf, vec3 d, float dist) {
 vec3 col = vec3(0.5);
 if (uf == 1) {
  float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);
  col = mix(vec3(0.56, 0.58, 0.60), vec3(0.75, 0.77, 0.79), smoothstep(0.35, 0.92, up));
 } else if (uf == 2) {
  col = mix(vec3(0.41, 0.46, 0.61), vec3(0.24, 0.28, 0.44), smoothstep(-0.04, 0.68, d.y));
  float n = lmVN(vec2(atan(d.z, d.x) * 2.0, d.y * 5.0));
  col *= 0.92 + 0.12 * n;
  float sn = lmSnowFlakes(d, dist, tm);
  col = mix(col, vec3(0.90, 0.92, 0.96), sn);
 } else if (uf == 3) {
  float up = clamp(d.y, 0.0, 1.0);
  col = mix(vec3(0.64, 0.78, 0.96), vec3(0.40, 0.60, 0.94), smoothstep(0.10, 0.85, up));
  float cl = smoothstep(0.56, 0.74, lmVN(d.xz / max(up + 0.25, 0.25) * 1.4));
  col = mix(col, vec3(0.88, 0.91, 0.96), cl * 0.45);
 } else if (uf == 4) {
  float up = d.y;
  vec3 zenith = vec3(0.47, 0.65, 0.92);
  vec3 horiz  = vec3(0.78, 0.85, 0.96);
  if (up > 0.0) {
   col = mix(horiz, zenith, pow(clamp(up, 0.0, 1.0), 0.62));
   vec2 q = d.xz / max(up + 0.10, 0.10) * 0.95 + vec2(tm * 0.002, 0.0);
   float n = lmVN(q) * 0.55 + lmVN(q * 2.3 + 4.1) * 0.30 + lmVN(q * 5.1 + 1.7) * 0.15;
   float cl = smoothstep(0.48, 0.66, n) * smoothstep(0.01, 0.14, up);
   vec3 sunD = normalize(vec3(0.72, 0.28, 0.45));
   float warm = pow(max(dot(d, sunD), 0.0), 2.5);
   vec3 clCol = mix(vec3(0.92, 0.91, 0.94), vec3(0.96, 0.85, 0.74), 0.35 + 0.50 * warm);
   col = mix(col, clCol, cl * 0.85);
  } else {
   col = mix(horiz, vec3(0.85, 0.89, 0.96), clamp(-up * 2.6, 0.0, 1.0));
  }
 }
 return col;
}
void main(){
 if (uPass > 0.5 && uUnstruct.y < 1.5) { F = vec4(lmSky(normalize(fp - uCam)), 1.0); return; }
 if (uUnstruct.x > 0.5 && uUnstruct.y > 1.5) {
  int uf = int(uUnstruct.x + 0.5);
  vec3 d = normalize(fp - vp);
  float distS = length(fp - vp);
  vec3 resS = lmUnstructSkyColor(uf, d, distS);
  if (uQ(5).y > 0.5) resS *= 0.06;
  if (uQ(3).w > 0.5) resS = uQ(3).rgb * (0.26 + 0.78 * dot(resS, vec3(0.30, 0.59, 0.11)));
  if (uQ(2).y > 0.5) resS = mix(vec3(0.32), resS, 0.35);
  resS *= 1.15;
  float lS = max(dot(resS, vec3(0.299, 0.587, 0.114)), 0.0001);
  if (lS > 0.55) resS *= (0.55 + (lS - 0.55) / (1.0 + (lS - 0.55) * 2.25)) / lS;
  resS = clamp(resS, 0.0, 0.945);
  if (uQ(5).z > 0.5) {
   float b = fract(dot(floor(gl_FragCoord.xy), vec2(0.5, 0.75)));
   resS = floor(resS * 31.0 + b) / 31.0;
  }
  F = vec4(resS, 1.0);
  return;
 }
 // Retro used to sample with affine (noperspective) UVs. On 5 m wall and floor quads that made
 // the texture slide with every turn of the head, so retro keeps its vertex grid, 15-bit colour
 // and fat pixels but maps textures perspective-correct (without parallax).
 vec3 N = normalize(nm);
 float dist = length(vp - fp);
 vec3 tc;
 if (uQ(2).y > 0.5) {
  tc = lmDevGrid(fp, N, dist);
 } else {
  vec4 base = texture(tex, uv);
  vec2 dx = dFdx(uv), dy = dFdy(uv);
  tc = base.rgb;
  if (uQ(5).z < 0.5 && dist < 15.0) {
   float relief = (base.a - 0.5) * 0.025 * (1.0 - smoothstep(8.0, 15.0, dist));
   tc = textureGrad(tex, uv + normalize(viewTS).xy * relief, dx, dy).rgb;
  }
 }
 tc *= modelTint;
 bool isCeil = N.y < -0.5;
 bool isFloor = N.y > 0.5;
 bool onGround = isFloor && (abs(fp.y) < 0.036 || abs(fp.y + 2.85) < 0.036);
 bool skyOn = uQ(5).x > 0.5;
 vec3 emis = vec3(0.0);

 // --- quirks that repaint the surface
 if (uQ(3).w > 0.5) tc = uQ(3).rgb * (0.26 + 0.92 * dot(tc, vec3(0.30, 0.59, 0.11)));
 if (uQ(2).x > 0.5 && onGround && dist < 34.0) {                 // confetti
  vec2 g = fp.xz * 7.0;
  vec2 c = floor(g);
  float h = lmH2(c);
  float strew = lmVN(fp.xz * 0.23);
  if (h > 0.985 - 0.16 * smoothstep(0.35, 0.75, strew)) {
   vec2 f = fract(g) - 0.5;
   float a = h * 61.0, ca = cos(a), sa = sin(a);
   f = vec2(ca * f.x - sa * f.y, sa * f.x + ca * f.y);
   if (abs(f.x) < 0.30 && abs(f.y) < 0.16) tc = mix(tc, lmHue(h * 13.7), 0.92 * (1.0 - smoothstep(20.0, 34.0, dist)));
  }
 }
 int lavaKind = int(uQ(2).w + 0.5);
 if (lavaKind > 0 && onGround) {
  vec3 gcol = uQ(6).rgb;
  if (lavaKind == 3) {                                           // luminous panel floor
   vec2 pq = fp.xz / 1.25;
   vec2 pf = abs(fract(pq) - 0.5);
   float seam = smoothstep(0.455, 0.49, max(pf.x, pf.y));
   float ph = lmH2(floor(pq));
   float on = (ph < 0.05) ? 0.05 : ((ph < 0.10) ? 0.55 + 0.45 * step(0.5, fract(tm * (3.0 + ph * 40.0) + ph * 9.0)) : 1.0);
   emis += gcol * (1.0 - seam) * on * 0.80;
   tc = mix(tc, vec3(0.16), seam);
  } else if (lavaKind == 1) {                                    // magma under a cracked crust
   float e = lmCrack(fp.xz * 0.62);
   float e2 = lmCrack(fp.xz * 1.9 + 4.7);
   float pulse = 0.72 + 0.28 * sin(tm * 0.9 + lmVN(fp.xz * 0.2) * 9.0);
   float crack = (1.0 - smoothstep(0.02, 0.10, e)) + 0.45 * (1.0 - smoothstep(0.015, 0.05, e2)) * (1.0 - smoothstep(0.2, 0.45, e));
   emis += gcol * (crack * 1.55 + exp(-e * 8.0) * 0.30) * pulse;
   emis += vec3(1.0, 0.85, 0.45) * (1.0 - smoothstep(0.0, 0.03, e)) * 0.5 * pulse;
   tc *= 0.55 + 0.45 * smoothstep(0.0, 0.5, e);
  } else {                                                       // something seeping up
   float n = lmVN(fp.xz * 0.33) * 0.65 + lmVN(fp.xz * 1.1 + 5.0) * 0.35;
   float pool = smoothstep(0.50, 0.56, n);
   float bub = pow(lmVN(fp.xz * 4.0 + vec2(0.0, tm * 0.35)), 3.0);
   emis += gcol * pool * (0.62 + 0.5 * bub) * (0.85 + 0.15 * sin(tm * 1.1 + n * 20.0));
   emis += gcol * smoothstep(0.42, 0.50, n) * (1.0 - pool) * 0.20;
   tc *= 1.0 - 0.5 * pool;
  }
 }
 if (uQ(2).z > 0.5) {                                            // neon
  float sw = 0.5 + 0.5 * sin(tm * 0.21 + (fp.x + fp.z) * 0.013);
  vec3 nA = mix(uQ(6).rgb, uQ(7).rgb, sw * 0.35), nB = mix(uQ(7).rgb, uQ(6).rgb, sw * 0.35);
  float nf = 1.0 - smoothstep(40.0, 90.0, dist) * 0.7;
  if (onGround) {
   vec2 g = (0.5 - abs(fract(fp.xz / 2.5) - 0.5)) * 2.5;
   float dl = min(g.x, g.y);
   float wpx = clamp(dist * 0.0022, 0.0, 0.3);
   emis += nA * ((1.0 - smoothstep(0.016 + wpx, 0.045 + wpx * 2.0, dl)) * 1.15 + exp(-dl * 8.0) * 0.16) * nf;
  } else if (abs(N.y) < 0.5) {
   float d1 = abs(fp.y - 1.10);
   emis += nB * ((1.0 - smoothstep(0.022, 0.050, d1)) * 1.5 + exp(-d1 * 6.0) * 0.26) * nf;
   float d2 = abs(fp.y - 4.40);
   emis += nA * ((1.0 - smoothstep(0.020, 0.045, d2)) * 1.2 + exp(-d2 * 8.0) * 0.16) * nf;
   float d3 = abs(fp.y - 0.02);
   emis += nA * exp(-d3 * 5.0) * 0.10 * nf;
  }
 }
 if (uQ(5).y > 0.5 && onGround) {                                // blackout: phosphor dashes down the middle of every bay
  vec2 f = abs(fract(fp.xz / 5.0) - 0.5);
  float wpx = clamp(dist * 0.0006, 0.0, 0.02);
  float a = (1.0 - smoothstep(0.010 + wpx, 0.016 + wpx * 2.0, f.y)) * step(0.5, fract(fp.x * 0.8));
  float b = (1.0 - smoothstep(0.010 + wpx, 0.016 + wpx * 2.0, f.x)) * step(0.5, fract(fp.z * 0.8));
  emis += uQ(6).rgb * max(a, b) * (0.34 + 0.06 * sin(tm * 1.3));
 }

 vec2 envUV = clamp((fp.xz - envRect.xy) * envRect.z, 0.004, 0.996);
 vec4 env = texture(envMap, envUV);
 float farLight = env.r;
 float cellAO   = env.g;
 float palVal   = env.b;
 float hiGeom   = clamp(max(fp.y - 4.65, -0.08 - fp.y) * 0.45, 0.0, 1.0);
 float mega     = max(max(megaEnv * 0.35, env.a), hiGeom);

 // Damp reservoir watermark & wet floor sheen inside sunken LT_BASIN pools (-2.85 m)
 if (fp.y < -0.10 && fp.y > -3.05) {
  float dampBand = 1.0 - smoothstep(-2.85, -1.55, fp.y);
  tc *= mix(1.0, 0.72, dampBand * 0.85);
 }

 // Continuous DNA-driven architectural floor expansion & tile sub-joints
 vec2 floorFootprint = max(fwidth(fp.xz), vec2(0.002));
 if (dist < 44.0 && isFloor && uUnstruct.x < 0.5 && uFloorJoint.y > 0.01 && (abs(fp.y) < 0.032 || abs(fp.y + 2.85) < 0.032)) {
  float jFade = 1.0 - smoothstep(14.0, 44.0, dist);
  float maj = lmJointCoverage(fp.xz, floorFootprint, uFloorJoint.x, 0.028);
  float mn = lmJointCoverage(fp.xz, floorFootprint, uFloorJoint.x * 0.5, 0.018) * 0.55;
  tc *= (1.0 - max(maj, mn) * uFloorJoint.y * jFade);
 }

)"
