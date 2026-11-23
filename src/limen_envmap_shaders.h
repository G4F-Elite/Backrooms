#pragma once
// LIMEN shaders, continuous procedural material synthesizer, lazy per-(seed, level)
// texture manager, and the 128x128 world-space environment map (`lmEnvTex`: R = unlimited
// far-lamp lightmap, G = cell AO, B = continuous LG_HUE pole blend, A = smooth vault factor).
#include "limen_vestige.h"

inline GLuint lmMegaTex[LM_MEGA_POLES] = {0, 0, 0};
inline GLuint lmDadoTex = 0;
inline GLuint lmEnvTex = 0;

enum {
    UTEX_MONOLITH = 0,
    UTEX_MONOLITH_ALT,
    UTEX_SNOW,
    UTEX_BASALT,
    UTEX_CAVE_ROCK,
    UTEX_CRIMSON_LEAF,
    UTEX_EARTH,
    UTEX_EARTH_WARM,
    UTEX_DRY_GRASS,
    UTEX_SKY,
    LM_UNSTRUCT_TEX_COUNT
};
inline GLuint lmUnstructTex[LM_UNSTRUCT_TEX_COUNT] = {};
inline int lmEnvOriginX = -999999, lmEnvOriginZ = -999999;
inline unsigned int lmEnvBuiltSeed = 0, lmEnvBuiltEpoch = 0;
inline int lmEnvBuiltLevel = -999999999;

inline unsigned int lmTexBuiltSeed = 0;
inline int lmTexBuiltLevel = -999999999;
inline bool lmTexBuiltValid = false;

const int LM_ENV_SIZE = 128;   // 128x128 cells = 640x640 m around player

// Level quirks (see LmQuirk in limen_genome.h). One block of uniforms drives all of them:
//   uQ(0) = (flood on, water surface y, murk 0..1, -)        uQ(1) = (water body rgb, -)
//   uQ(2) = (party, greybox, neon, lava kind + 1)            uQ(3) = (mono pigment rgb, mono on)
//   uQ(4) = (curvature, lean, roll, drunken sway)            uQ(5) = (sky kind + 1, blackout, retro, retro vertex grid)
//   uQ(6) = (look colour A rgb, lean heading)                uQ(7) = (look colour B rgb, -)
// uPass = 1 draws the sky backdrop (a box around the camera) instead of the world.
inline const char* lmQuirkProjGLSL = R"(
uniform vec3 uQA[8];
uniform vec3 uQB[8];
#define uQ(i) vec4(uQA[i], uQB[i].x)
uniform vec3 uCam;
uniform float uPass;
vec4 lmProject(vec3 w, mat4 V, mat4 P, float time) {
 vec4 k = uQ(4);
 vec4 v;
 if (uPass > 0.5) {
  v = vec4(mat3(V) * w, 1.0);
 } else {
  vec3 q = w;
  vec2 d = q.xz - uCam.xz;
  float rel = q.y - uCam.y;
  q.y += k.x * min(dot(d, d), 22500.0);
  q.xz += k.y * rel * vec2(cos(uQ(6).w), sin(uQ(6).w));
  if (k.w > 0.0) {
   q.x += k.w * 0.045 * rel * sin(time * 0.52 + w.z * 0.19);
   q.z += k.w * 0.045 * rel * sin(time * 0.41 + w.x * 0.17 + 1.7);
  }
  v = V * vec4(q, 1.0);
 }
 float roll = k.z + k.w * (0.045 * sin(time * 0.43) + 0.030 * sin(time * 0.71 + 1.3));
 if (roll != 0.0) {
  float cr = cos(roll), sr = sin(roll);
  v.xy = vec2(cr * v.x - sr * v.y, sr * v.x + cr * v.y);
 }
 vec4 c = P * v;
 if (uPass > 0.5) c.z = c.w * 0.99995;
 float g = uQ(5).w;
 if (g > 0.0 && c.w > 0.05 && uPass < 0.5) c.xy = floor(c.xy / c.w * g + 0.5) / g * c.w;
 return c;
}
)";

inline const char* lmMainVSBody = R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t; layout(location=2) in vec3 n;
out vec2 uv; noperspective out vec2 uvAff; out vec3 fp,nm; out vec3 viewTS; out vec3 fragPosTS;
uniform mat4 M,V,P;
uniform vec3 vp;
uniform float tm;
//QUIRK_PROJ
void main(){
 fp = (uPass > 0.5) ? uCam + p : vec3(M*vec4(p,1));
 nm = mat3(M)*n;
 uv = t; uvAff = t;
 vec3 N = normalize(nm);
 vec3 T = normalize(cross(N, vec3(0.0, 1.0, 0.1)));
 vec3 B = cross(N, T);
 mat3 TBN = transpose(mat3(T, B, N));
 viewTS = TBN * normalize(vp - fp);
 fragPosTS = TBN * fp;
 gl_Position = lmProject((uPass > 0.5) ? p : fp, V, P, tm);
})";

inline const char* lmMainFS = R"(#version 330
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
vec2 parallaxOffset(vec2 texCoords, vec3 viewDir) {
 float height = texture(tex, texCoords).a;
 float dist = length(vp - fp);
 float lodFade = 1.0 - smoothstep(8.0, 15.0, dist);
 if(lodFade < 0.01) return texCoords;
 float h = (height - 0.5) * 0.025 * lodFade;
 return texCoords + viewDir.xy * h;
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
 vec3 V = normalize(viewTS);
 vec2 texCoord = (uQ(5).z > 0.5) ? uv : parallaxOffset(uv, V);
 vec3 tc = texture(tex, texCoord).rgb * modelTint;
 vec3 N = normalize(nm);
 float dist = length(vp - fp);
 bool isCeil = N.y < -0.5;
 bool isFloor = N.y > 0.5;
 bool onGround = isFloor && (abs(fp.y) < 0.036 || abs(fp.y + 2.85) < 0.036);
 bool skyOn = uQ(5).x > 0.5;
 vec3 emis = vec3(0.0);

 // --- quirks that repaint the surface
 if (uQ(2).y > 0.5) tc = lmDevGrid(fp, N, dist) * modelTint;
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
   float crack = (1.0 - smoothstep(0.02, 0.10, e)) + 0.45 * (1.0 - smoothstep(0.015, 0.05, e2)) * smoothstep(0.45, 0.2, e);
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
 if (isFloor && uUnstruct.x < 0.5 && uFloorJoint.y > 0.01 && (abs(fp.y) < 0.032 || abs(fp.y + 2.85) < 0.032)) {
  float jFade = 1.0 - smoothstep(14.0, 44.0, dist);
  vec2 gf = abs(fract(fp.xz / uFloorJoint.x + 0.5) - 0.5);
  float dMaj = min(gf.x, gf.y) * uFloorJoint.x;
  float maj = 1.0 - smoothstep(0.012, 0.044, dMaj);
  float subStep = uFloorJoint.x * 0.5;
  vec2 gf2 = abs(fract(fp.xz / subStep + 0.5) - 0.5);
  float dMin = min(gf2.x, gf2.y) * subStep;
  float mn = (1.0 - smoothstep(0.008, 0.028, dMin)) * 0.55;
  tc *= (1.0 - max(maj, mn) * uFloorJoint.y * jFade);
 }

 // Cell Ambient Occlusion + analytic dihedral corner darkening at floor/ceiling/beam junctions
 float wallCrease = 1.0;
 if (abs(N.y) < 0.5) {
  float dFloor = min(abs(fp.y), abs(fp.y + 2.85));
  float dCeil  = abs(fp.y - 4.5);
  wallCrease = (0.76 + 0.24 * smoothstep(0.0, 0.95, dFloor)) * (0.84 + 0.16 * smoothstep(0.0, 0.70, dCeil));
 }
 float ao = mix(1.0, cellAO, 0.78) * wallCrease;

 float ambVal = 0.132 * uMood.x * (1.0 - danger * 0.3);
 vec3 baseAmb = max(vec3(ambVal * 0.84) + uFogCol * 0.48, vec3(0.118 * uMood.x)) * tc * ao;
 vec3 skyDir = normalize(vec3(0.38, 0.82, 0.42));
 float hemi = 0.38 + 0.24 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0) + 0.30 * max(dot(N, skyDir), 0.0)
            + 0.08 * max(dot(N, vec3(-0.65, 0.18, -0.72)), 0.0);
 float vertLum = (fp.y >= 0.0) ? (0.66 + 0.34 * smoothstep(0.0, 36.0, fp.y)) : max(exp(fp.y * 0.048), 0.46);
 vec3 megaAmb = tc * palAmbSmooth(palVal) * (0.56 * hemi * vertLum * mix(1.0, uMood.x, 0.6) * clamp(uMood.x * 2.2, 0.0, 1.0)) * ao;
 vec3 res = mix(baseAmb, megaAmb, mega);

 // 1. Near 16 point lights partitioned cleanly against far lightmap + crowd-normalized irradiance
 float nearDistFade = 1.0 - smoothstep(20.0, 38.0, dist);
 float quadAtt      = mix(0.042, 0.026, mega);
 float directGain   = mix(0.76, 0.94, mega) * uMood.y;
 vec3 viewDirW      = normalize(vp - fp);
 vec3 directIrr     = vec3(0.0);
 vec3 specAcc       = vec3(0.0);
 for(int i = 0; i < nl && i < 16; i++) {
  vec3 toLight = lp[i] - fp;
  float lightDistFromCam = length(lp[i] - vp);
  float fade = (1.0 - clamp((lightDistFromCam - 24.0) / 16.0, 0.0, 1.0)) * nearDistFade;
  if(fade < 0.001) continue;
  float d2 = dot(toLight, toLight);
  float invD = inversesqrt(max(d2, 0.0001));
  float d = d2 * invD;
  vec3 L = toLight * invD;
  float att = 1.0 / (1.0 + 0.11*d + quadAtt*d2);
  float df;
  if(isCeil && fp.y < 4.7) {
   float horizDist = length(vec2(toLight.x, toLight.z));
   float vertDist = abs(toLight.y);
   float spread = 1.0 / (1.0 + horizDist * 0.95);
   df = spread * 1.12 * (vertDist < 0.3 ? 1.55 : (vertDist < 1.0 ? 1.25 : 1.0));
   df += (1.0 / (1.0 + horizDist * 0.35)) * 0.20;
  } else {
   df = smoothstep(0.0, 0.52, max(dot(N, L), 0.0));
  }
  float fl = 1.0 + sin(tm*20.0 + float(i)*2.1) * 0.015 + sin(tm*30.0 + float(i)*3.7) * 0.05 * danger;
  vec3 lcol = lmLampCol(lp[i].xz);
  directIrr += lcol * (df * fl * att * directGain * fade);
  if (isFloor && uFogDens.z > 0.15) {
   vec3 H = normalize(L + viewDirW);
   float spec = pow(max(dot(N, H), 0.0), 16.0 + uFogDens.z * 28.0);
   specAcc += lcol * (spec * uFogDens.z * 0.14 * att * fade);
  }
 }
 directIrr = directIrr / (vec3(1.0) + directIrr * 0.28);
 res += tc * directIrr + specAcc / (vec3(1.0) + specAcc * 0.65);

 // 2. Unlimited 2D far-lamp lightmap (crossfaded cleanly with nearDistFade — no double counting)
 float farWeight = 1.0 - nearDistFade * 0.82;
 if (farLight > 0.005 && fp.y > -32.0) {
  float upFace = 0.60 + 0.40 * clamp(N.y * 0.6 + 0.5, 0.0, 1.0);
  float heightAtt = 1.0 - smoothstep(12.0, 42.0, abs(fp.y - 2.5));
  vec3 farCol = mix(uLightCol, palAmbSmooth(palVal), 0.32);
  res += tc * farCol * (farLight * 1.12 * uMood.y * upFace * heightAtt * ao) * farWeight;
 }
 if (N.y > 0.6 && fp.y < -2.70 && fp.y > -2.95) {
  res += uLightCol * 0.16 * farLight * (0.45 + 0.55 * ao);
 }

 if(flashOn == 1) {
  vec3 toFrag = normalize(fp - flashPos);
  float spotAngle = dot(toFrag, flashDir);
  float distF = length(fp - flashPos);
  float maxFD = mix(16.0, 36.0, mega);
  if(spotAngle > 0.85 && distF < maxFD) {
   float spotAtt = smoothstep(0.85, 0.95, spotAngle);
   float distAtt = 1.0 - distF / maxFD;
   float NdotL = max(dot(N, -toFrag), 0.0);
   vec3 flashColor = mix(vec3(1.0, 0.99, 0.92), vec3(1.0, 0.3, 0.2), danger * 0.6);
   res += tc * flashColor * spotAtt * distAtt * NdotL * 1.5;
  }
 }
 for(int i = 0; i < rfc && i < 4; i++) {
  vec3 toFrag = normalize(fp - rfp[i]);
  float spotAngle = dot(toFrag, rfd[i]);
  float distR = length(fp - rfp[i]);
  if(spotAngle > 0.88 && distR < 13.0) {
   res += tc * vec3(0.9, 0.95, 1.0) * smoothstep(0.88, 0.96, spotAngle) * (1.0 - distR / 13.0) * max(dot(N, -toFrag), 0.0) * 0.95;
  }
 }
 if(isCeil && fp.y < 4.7) {
  float maxB = max(res.r, max(res.g, res.b));
  if(maxB > 0.1) res = max(res, tc * 0.14 * ao);
 }

 // --- quirks that relight the world
 if (skyOn) {                                                    // open sky: hemisphere light, and a sun/moon without shadows
  int sk = int(uQ(5).x + 0.5) - 1;
  float open = 1.0 - 0.85 * smoothstep(0.35, 0.9, env.a);
  float kAmb = (sk <= 1) ? 0.62 : ((sk == 3) ? 0.34 : ((sk == 2) ? 0.15 : 0.06));
  vec3 skyAmb = mix(uSkyHor, uSkyTop, 0.45);
  res += tc * skyAmb * (0.32 + 0.68 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0)) * (kAmb * open) * mix(1.0, cellAO, 0.55);
  vec3 sunDir = (sk == 3) ? normalize(vec3(0.80, 0.16, 0.55)) : ((sk == 2) ? normalize(vec3(-0.42, 0.52, 0.74)) : normalize(vec3(0.42, 0.78, 0.36)));
  vec3 sunCol = (sk == 0) ? vec3(0.10) : ((sk == 1) ? vec3(0.52, 0.50, 0.44) : ((sk == 3) ? vec3(0.62, 0.34, 0.16) : ((sk == 2) ? vec3(0.07, 0.08, 0.11) : vec3(0.0))));
  res += tc * sunCol * max(dot(N, sunDir), 0.0) * open * mix(1.0, cellAO, 0.7);
 }
 if (lavaKind > 0) {                                             // the floor is the lamp
  float hgt = max(fp.y, 0.0) + max(-2.85 - fp.y, 0.0);
  if (fp.y < -0.1 && fp.y > -2.9) hgt = fp.y + 2.85;
  float face = isFloor ? (onGround ? 0.0 : 0.10) : (isCeil ? 1.0 : 0.62);
  float kUp = (lavaKind == 3) ? 1.05 : 0.60;
  float pulse = (lavaKind == 3) ? 1.0 : 0.85 + 0.15 * sin(tm * 0.9 + (fp.x + fp.z) * 0.05);
  res += tc * uQ(6).rgb * (face * kUp * pulse * exp(-hgt * 0.24) * (1.0 - 0.7 * smoothstep(0.4, 1.0, env.a)));
 }
 res += emis;

 // Flood: there is no water mesh. Every fragment below the surface is seen through it: the ray
 // is cut at the plane, the stretch below is absorbed, and the plane itself mirrors the ceiling
 // light above the hit point (from the lamp map), the lamps (glints) or the sky.
 float fogDist = dist;
 if (uQ(0).x > 0.5) {
  float wy = uQ(0).y;
  vec3 wcol = uQ(1).rgb;
  float murk = uQ(0).z;
  vec3 rd = fp - vp;
  if (vp.y > wy) {
   if (fp.y < wy) {
    float tW = (wy - vp.y) / rd.y;
    vec3 sp = vp + rd * tW;
    float sdist = length(sp - vp);
    float path = length(fp - sp);
    float rf = 1.0 - smoothstep(8.0, 46.0, sdist);
    vec2 w2 = sp.xz;
    vec2 gr = vec2(
     cos(w2.x * 1.9 + tm * 1.1) * 0.5 + cos((w2.x + w2.y) * 3.1 - tm * 1.7) * 0.3 + cos(w2.x * 7.3 + w2.y * 2.1 + tm * 2.3) * 0.14,
     cos(w2.y * 2.3 - tm * 0.9) * 0.5 + cos((w2.x + w2.y) * 3.1 - tm * 1.7) * 0.3 + cos(w2.y * 6.7 - w2.x * 1.7 - tm * 2.0) * 0.14);
    vec3 wn = normalize(vec3(-gr.x * 0.055 * rf, 1.0, -gr.y * 0.055 * rf));
    vec3 vd = normalize(-rd);
    float fres = 0.025 + 0.975 * pow(1.0 - max(dot(vd, wn), 0.0), 5.0);
    float kAbs = mix(0.36, 2.5, murk);
    vec3 trans = exp(-path * kAbs * (vec3(1.06) - wcol * 0.92));
    float ca = 0.0;
    if (N.y > 0.5) {
     vec2 cq = fp.xz * 1.3;
     float c1 = sin(cq.x * 2.1 + tm * 0.8 + sin(cq.y * 1.7 + tm * 0.6) * 1.4);
     float c2 = sin(cq.y * 2.4 - tm * 0.7 + sin(cq.x * 1.9 - tm * 0.5) * 1.4);
     ca = pow(clamp(0.5 + 0.5 * c1 * c2, 0.0, 1.0), 4.0) * (1.0 - murk) * exp(-(wy - fp.y) * 0.5);
    }
    vec3 lightHere = vec3(0.075 * uMood.x) + uLightCol * (farLight * 0.42) + directIrr * 0.40;
    if (skyOn) lightHere += mix(uSkyHor, uSkyTop, 0.5) * 0.35;
    vec3 under = res * (1.0 + ca * 1.5) * trans + wcol * lightHere * (1.0 - exp(-path * kAbs * 0.7));
    vec3 R = reflect(-vd, wn);
    vec3 refl;
    if (skyOn && env.a < 0.5) {
     refl = lmSky(normalize(vec3(R.x, max(R.y, 0.02), R.z)));
    } else {
     vec2 hit = sp.xz + R.xz * ((4.4 - wy) / max(R.y, 0.10));
     float hl = texture(envMap, clamp((hit - envRect.xy) * envRect.z, 0.004, 0.996)).r;
     refl = max(uFogCol, vec3(0.025)) * 1.1 + uLightCol * (hl * 0.50) + vec3(0.05 * uMood.x);
    }
    vec3 glint = vec3(0.0);
    for (int i = 0; i < nl && i < 16; i++) {
     vec3 Lw = lp[i] - sp;
     float ld = length(Lw);
     float s = pow(max(dot(R, Lw / ld), 0.0), 420.0);
     glint += lmLampCol(lp[i].xz) * (s * 2.6 / (1.0 + ld * 0.03));
    }
    res = mix(under, refl, fres) + glint * (0.35 + 0.65 * fres);
    fogDist = sdist;
   } else if (abs(N.y) < 0.5 && fp.y < wy + 0.24) {
    res *= mix(0.70, 1.0, smoothstep(wy, wy + 0.24, fp.y));       // tide line
   }
  } else {
   // the camera is under the surface (walking the bottom of what used to be a dry basin)
   float path = dist;
   if (fp.y > wy) path = dist * (wy - vp.y) / max(fp.y - vp.y, 0.001);
   float kAbs = mix(0.10, 0.85, murk);
   vec3 trans = exp(-path * kAbs * (vec3(1.06) - wcol * 0.92));
   vec3 lightHere = vec3(0.10 * uMood.x) + uLightCol * (farLight * 0.40);
   vec2 cq = fp.xz * 1.1;
   float ca = pow(clamp(0.5 + 0.5 * sin(cq.x * 2.1 + tm * 0.8 + sin(cq.y * 1.7 + tm * 0.6) * 1.4) * sin(cq.y * 2.4 - tm * 0.7), 0.0, 1.0), 3.0);
   res = res * (1.0 + ca * 0.9 * step(fp.y, wy) * max(N.y, 0.0)) * trans + wcol * lightHere * (1.0 - exp(-path * kAbs * 0.9));
  }
 }

 // Continuous DNA-driven world-space fog (sized so megastructures keep crisp silhouettes!)
 float normFogD = mix(uFogDens.x * 1.18, uFogDens.x * 0.84, smoothstep(0.0, 2.5, fp.y));
 float pitExtra = smoothstep(-16.0, -52.0, fp.y);
 float megaFogD = uFogDens.y + 0.024 * pitExtra;
 float fog = clamp(exp(-fogDist * mix(normFogD, megaFogD, mega) * (skyOn ? 0.45 : 1.0)), 0.0, 1.0);
 float fogNoise = mix(hash(fp * 0.11 + vec3(tm * 0.025, tm * 0.015, tm * 0.01)),
                      hash(fp * 0.05 + vec3(-tm * 0.012, tm * 0.02, tm * 0.008)), 0.5);
 fog *= (0.94 + fogNoise * 0.06);
 fog = max(fog, mega * 0.18 * (1.0 - pitExtra * 0.7));

 float lightScatter = farLight * 0.42;
 for(int i = 0; i < nl && i < 8; i++) {
  float ld = length(lp[i] - fp);
  lightScatter += 1.0 / (1.0 + ld * ld * 0.06) * 0.12;
 }
 lightScatter = min(lightScatter, 0.68);

 vec3 normFogCol = max(uFogCol, vec3(0.064, 0.060, 0.068)) + uLightCol * (0.065 * lightScatter);
 vec3 highHaze   = palFogSmooth(palVal) + uLightCol * (0.080 * lightScatter);
 vec3 abyssHaze  = max(uFogCol * 0.68, vec3(0.065, 0.068, 0.078));
 vec3 fogColor   = mix(normFogCol, mix(highHaze, abyssHaze, pitExtra), mega);

 float desat = smoothstep(6.0, 24.0, dist) * (1.0 - mega * 0.55);
 float gray = dot(res, vec3(0.3, 0.6, 0.1));
 res = mix(res, vec3(gray) * mix(vec3(1.0), normalize(uLightCol + vec3(0.35)), 0.25), desat * 0.30);
 float distDarken = smoothstep(14.0, 28.0, dist) * (1.0 - mega * 0.92);
 res *= (1.0 - distDarken * 0.18);

 if (!skyOn) res = mix(fogColor, res, fog);
 if (uUnstruct.x > 0.5) {
  int uf = int(uUnstruct.x + 0.5);
  vec3 uRes = tc * 0.25;
  vec3 uFogCol = vec3(0.5);
  float uFogDens = 0.014;
  float blk = (uQ(5).y > 0.5) ? 0.07 : 1.0;
  if (uf == 1) {
   float upN = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
   float sideN = 0.75 + 0.25 * abs(N.x) + 0.15 * abs(N.z);
   float hGrad = 0.62 + 0.38 * smoothstep(0.0, 32.0, fp.y);
   uRes = tc * vec3(0.56, 0.58, 0.61) * (0.48 + 0.52 * upN) * sideN * hGrad * ao * blk;
   uFogCol = vec3(0.67, 0.69, 0.71) * blk;
   uFogDens = 0.0135;
  } else if (uf == 2) {
   vec3 sunD = normalize(vec3(0.35, 0.85, 0.30));
   float hemiU = 0.52 + 0.38 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0) + 0.22 * max(dot(N, sunD), 0.0);
   uRes = tc * vec3(0.66, 0.71, 0.84) * hemiU * ao * blk;
   uFogCol = vec3(0.41, 0.46, 0.61) * blk;
   uFogDens = 0.0145;
  } else if (uf == 3) {
   float radC = length(fp.xz - uUnstructInfo.xz);
   float shaft = exp(-radC * radC / 145.0);
   float roofRim = smoothstep(uUnstructInfo.y - 3.0, uUnstructInfo.y + 4.0, fp.y);
   float hemiG = 0.44 + 0.56 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
   vec3 caveAmb = tc * vec3(0.34, 0.30, 0.40) * hemiG * ao;
   vec3 skyBeam = tc * vec3(0.58, 0.64, 0.76) * (0.45 + 0.55 * clamp(N.y, 0.0, 1.0)) * shaft * (0.72 + 0.28 * ao);
   vec3 rimSun  = tc * vec3(0.76, 0.74, 0.72) * (0.45 + 0.55 * clamp(dot(N, normalize(vec3(0.3, 0.7, 0.4))), 0.0, 1.0)) * roofRim;
   uRes = (caveAmb + skyBeam + rimSun) * blk;
   uFogCol = vec3(0.30, 0.28, 0.35) * blk;
   uFogDens = 0.0125;
  } else if (uf == 4) {
   vec3 sunD = normalize(vec3(0.66, 0.64, 0.40));
   float ndl = max(dot(N, sunD), 0.0);
   float upN = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
   vec3 lightT = vec3(0.36, 0.44, 0.60) * (0.55 + 0.45 * upN) + vec3(0.78, 0.64, 0.50) * ndl + vec3(0.16, 0.14, 0.12) * (1.0 - upN);
   if (uUnstruct.y > 0.5) lightT *= 1.18;
   uRes = tc * lightT * mix(0.84, 1.0, ao) * blk;
   uFogCol = mix(vec3(0.78, 0.85, 0.96), vec3(0.88, 0.84, 0.80), 0.20) * blk;
   uFogDens = 0.0075;
  }
  if (flashOn == 1) {
   vec3 toFrag = normalize(fp - flashPos);
   float spotAngle = dot(toFrag, flashDir);
   float distF = length(fp - flashPos);
   if (spotAngle > 0.85 && distF < 36.0) {
    uRes += tc * vec3(1.0, 0.99, 0.92) * smoothstep(0.85, 0.95, spotAngle) * (1.0 - distF / 36.0) * max(dot(N, -toFrag), 0.0) * 1.4;
   }
  }
  float uFog = clamp(exp(-fogDist * uFogDens), 0.0, 1.0);
  res = mix(uFogCol, uRes, uFog);
  if (uf == 2 && uQ(5).y < 0.5) {
   float sn = lmSnowFlakes(normalize(fp - vp), dist, tm);
   res = mix(res, vec3(0.90, 0.92, 0.96), sn);
  }
 }
 if(danger > 0.3) { res.r += danger * 0.08; res.gb *= (1.0 - danger * 0.15); }
 if(danger > 0.0) { float gD = dot(res, vec3(0.3, 0.6, 0.1)); res = mix(res, vec3(gD), danger * 0.2); }

 // Midtone-preserving exposure + smooth filmic highlight shoulder (prevents bleached >250 surfaces)
 res *= 1.38;
 float lumOut = max(dot(res, vec3(0.299, 0.587, 0.114)), 0.0001);
 float lumTonemapped = (lumOut <= 0.50)
  ? lumOut
  : (0.50 + (lumOut - 0.50) / (1.0 + (lumOut - 0.50) * 2.25));
 res *= lumTonemapped / lumOut;
 res = clamp(res, 0.0, 0.945);
 if (skyOn) {
  // far geometry dissolves into the very sky that stands behind it
  vec3 sd = normalize(fp - vp);
  res = mix(lmSky(normalize(vec3(sd.x, max(sd.y, 0.0) * 0.35, sd.z))), res, fog);
 }
 if (uQ(5).z > 0.5) {                                            // 15-bit colour, ordered dither
  float b = fract(dot(floor(gl_FragCoord.xy), vec2(0.5, 0.75)));
  res = floor(res * 31.0 + b) / 31.0;
 }
 F = vec4(res, 1.0);
})";

inline const char* lmLightVSBody = R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t;
out vec2 uv; out vec3 wpos; out float vDist;
uniform mat4 M,V,P;
uniform float tm;
//QUIRK_PROJ
void main(){
 uv = t;
 vec4 wp = M * vec4(p, 1.0);
 wpos = wp.xyz;
 vec4 vp4 = V * wp;
 vDist = length(vp4.xyz);
 gl_Position = lmProject(wpos, V, P, tm);
})";

inline const char* lmLightFS = R"(#version 330
out vec4 F; in vec2 uv; in vec3 wpos; in float vDist;
uniform sampler2D tex;
uniform sampler2D envMap;
uniform vec3 envRect;
uniform float inten,tm,fade;
uniform float danger;
uniform float megaEnv;
uniform vec3 uLightCol;
uniform vec3 uFogCol;
uniform vec3 uFogDens;
uniform vec3 uMegaFog[3];
uniform vec3 uQA[8];
uniform vec3 uQB[8];
#define uQ(i) vec4(uQA[i], uQB[i].x)
float hash(float n){ return fract(sin(n)*43758.5453); }
float hash2(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
vec3 lmHue(float h) { return clamp(abs(fract(h + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0); }
vec3 palFogSmooth(float p) {
 return (p < 0.5) ? mix(uMegaFog[0], uMegaFog[1], p * 2.0) : mix(uMegaFog[1], uMegaFog[2], (p - 0.5) * 2.0);
}
void main(){
 vec3 lcol = uLightCol;
 if (uQ(2).x > 0.5) {                               // party: same colour the main shader lights the room with
  float h = hash2(floor(wpos.xz / 5.0));
  float beat = 0.70 + 0.30 * sin(tm * (2.2 + 3.4 * fract(h * 7.3)) + h * 40.0);
  lcol = mix(vec3(1.0), lmHue(h + tm * 0.021 * (0.4 + h)), 0.86) * beat * 1.12;
 }
 if (uQ(3).w > 0.5) lcol = mix(lcol, uQ(3).rgb, 0.35);
 vec3 c = texture(tex,uv).rgb * lcol * inten;
 float seed = hash2(floor(wpos.xz * 0.5));
 float flick = 0.97 + sin(tm*25.0 + seed*6.283)*0.015 + hash(floor(tm*7.0) + seed*19.0)*0.015;
 float panicAmt = smoothstep(0.10, 0.60, danger);
 flick += (sin(tm*(55.0 + seed*10.0) + seed*12.3)*0.06 + (hash(floor(tm*18.0) + seed*53.0) - 0.5)*0.08) * panicAmt;
 flick = clamp(flick, 0.15, 1.35);
 vec3 glow = c * flick * fade;
 glow = glow / (vec3(1.0) + glow * 0.22);
 glow = min(glow, vec3(0.955));
 vec2 envUV = clamp((wpos.xz - envRect.xy) * envRect.z, 0.004, 0.996);
 vec4 env = texture(envMap, envUV);
 float pit = smoothstep(-18.0, -52.0, wpos.y);
 float hi  = clamp(max(wpos.y - 4.65, -0.08 - wpos.y) * 0.45, 0.0, 1.0);
 float mega = max(max(megaEnv * 0.35, env.a), hi);
 float fogD = mix(uFogDens.x * 0.85, uFogDens.y * 0.34 + 0.018 * pit, mega);
 float fogVis = max(clamp(exp(-max(vDist - 12.0, 0.0) * fogD), 0.0, 1.0), mega * 0.28);
 vec3 abyssCol = max(uFogCol * 0.58, vec3(0.052, 0.054, 0.064));
 vec3 fogCol = mix(max(uFogCol, vec3(0.052, 0.050, 0.056)), mix(palFogSmooth(env.b), abyssCol, pit), mega);
 vec3 outc = mix(fogCol, glow, fogVis);
 if (uQ(5).x > 0.5) outc = mix(glow, outc, 0.25);    // under an open sky a lamp does not fade to indoor fog
 if (uQ(5).z > 0.5) outc = floor(outc * 31.0 + fract(dot(floor(gl_FragCoord.xy), vec2(0.5, 0.75)))) / 31.0;
 F = vec4(outc, fade);
})";

inline bool lmPatchShadersOnce() {
    static std::string patchedVhs;
    patchedVhs = vhsFS;
    const std::string oldLin = "20.0 / (100.1 - (d * 2.0 - 1.0) * 99.9)";
    const std::string newLin = "64.0 / (320.1 - (d * 2.0 - 1.0) * 319.9)";
    size_t pos = patchedVhs.find(oldLin);
    if (pos != std::string::npos) patchedVhs.replace(pos, oldLin.size(), newLin);
    // Soft highlight shoulder after post-process GI + bloom + god-rays so HDR bounce never clips >250
    const std::string oldRet = "return lit;";
    const std::string newRet = R"(float lLum = max(dot(lit, vec3(0.299, 0.587, 0.114)), 0.0001);
 if (lLum > 0.72) {
  float tLum = 0.72 + (lLum - 0.72) / (1.0 + (lLum - 0.72) * 3.4);
  lit *= tLum / lLum;
 }
 return clamp(lit, 0.0, 0.976);)";
    size_t rpos = patchedVhs.find(oldRet);
    if (rpos != std::string::npos) patchedVhs.replace(rpos, oldRet.size(), newRet);
    // Retro quirk: the resolve pass reads the frame through a coarse pixel grid (uLmPix rows).
    const std::string oldSrc = "vec3 sourceSample(vec2 tc){";
    const std::string newSrc = "uniform float uLmPix;\nvec3 sourceSample(vec2 tc){\n if(uLmPix > 1.0){ vec2 g = vec2(uLmPix * texelY / max(texelX, 1e-6), uLmPix); tc = (floor(tc * g) + 0.5) / g; }";
    size_t spos = patchedVhs.find(oldSrc);
    if (spos != std::string::npos) patchedVhs.replace(spos, oldSrc.size(), newSrc);
    vhsFS = patchedVhs.c_str();
    static std::string mainVsSrc, lightVsSrc;
    auto withProj = [](const char* body) {
        std::string s = body;
        const std::string mark = "//QUIRK_PROJ";
        size_t p = s.find(mark);
        if (p != std::string::npos) s.replace(p, mark.size(), lmQuirkProjGLSL);
        return s;
    };
    mainVsSrc = withProj(lmMainVSBody);
    lightVsSrc = withProj(lmLightVSBody);
    mainVS = mainVsSrc.c_str();
    mainFS = lmMainFS;
    lightVS = lightVsSrc.c_str();
    lightFS = lmLightFS;
    return true;
}
inline bool gLmShadersReady = lmPatchShadersOnce();

// Bakes the 128x128 RGBA8 world environment map around chunk (pcx, pcz).
// Blue channel carries continuous `LG_HUE / 255.0f` to smoothly blend all 3 DNA mega poles!
//
// While streaming, a map that only needs to follow the player (or a lamp event) is baked
// `LM_ENV_ROWS_PER_FRAME` rows per frame; the previous texture and origin stay in use until the
// new one is complete. `fromFrame` marks the once-per-frame call that advances such a bake.
const int LM_ENV_ROWS_PER_FRAME = 32;
inline int lmEnvJobRow = -1, lmEnvJobOX = 0, lmEnvJobOZ = 0;
inline unsigned int lmEnvJobSeed = 0, lmEnvJobEpoch = 0;
inline int lmEnvJobLevel = 0;

inline void lmUpdateEnvMap(int pcx, int pcz, bool fromFrame = false) {
    int ox = pcx * CHUNK_SIZE - (LM_ENV_SIZE - CHUNK_SIZE) / 2;
    int oz = pcz * CHUNK_SIZE - (LM_ENV_SIZE - CHUNK_SIZE) / 2;
    if (lmEnvTex != 0 && ox == lmEnvOriginX && oz == lmEnvOriginZ &&
        lmEnvBuiltSeed == worldSeed && lmEnvBuiltLevel == gCurrentLevel && lmEnvBuiltEpoch == gLmSiteEpoch) {
        lmEnvJobRow = -1;
        return;
    }
    // (a jump of more than two chunks is a teleport: the old map covers the wrong place)
    bool progressive = gLmStreaming && lmEnvTex != 0 &&
                       lmEnvBuiltSeed == worldSeed && lmEnvBuiltLevel == gCurrentLevel &&
                       abs(ox - lmEnvOriginX) <= 2 * CHUNK_SIZE && abs(oz - lmEnvOriginZ) <= 2 * CHUNK_SIZE;
    if (progressive && !fromFrame) return;
    if (lmEnvJobRow < 0 || lmEnvJobOX != ox || lmEnvJobOZ != oz || lmEnvJobSeed != worldSeed ||
        lmEnvJobLevel != gCurrentLevel || lmEnvJobEpoch != gLmSiteEpoch) {
        lmEnvJobRow = 0; lmEnvJobOX = ox; lmEnvJobOZ = oz;
        lmEnvJobSeed = worldSeed; lmEnvJobLevel = gCurrentLevel; lmEnvJobEpoch = gLmSiteEpoch;
    }
    int rowBegin = lmEnvJobRow;
    int rowEnd = progressive ? rowBegin + LM_ENV_ROWS_PER_FRAME : LM_ENV_SIZE;
    if (rowEnd > LM_ENV_SIZE) rowEnd = LM_ENV_SIZE;

    static float lightA[LM_ENV_SIZE][LM_ENV_SIZE], lightB[LM_ENV_SIZE][LM_ENV_SIZE];
    static float palA[LM_ENV_SIZE][LM_ENV_SIZE], megaA[LM_ENV_SIZE][LM_ENV_SIZE];
    static unsigned char wallMask[LM_ENV_SIZE][LM_ENV_SIZE], aoByte[LM_ENV_SIZE][LM_ENV_SIZE];

    for (int z = rowBegin; z < rowEnd; z++) {
        int wz = oz + z;
        for (int x = 0; x < LM_ENV_SIZE; x++) {
            int wx = ox + x;
            int t = lmTagAt(wx, wz);
            wallMask[z][x] = (t == LT_WALL) ? 1 : 0;
            bool high = lmHighAt(wx, wz);
            LimenRect sr = lmSlabAt(wx, wz);
            const LimenSlab& s = lmSlab(sr.j, sr.i);
            int lamp = (t != LT_WALL) ? lmLampAt(wx, wz) : 0;
            int auxLamp = (lamp == 0 && high && wx != sr.x0 && wz != sr.z0)
                        ? lmAuxVaultLampLocal(s, wx - sr.x0 - 1, wz - sr.z0 - 1) : 0;
            float emit = 0.0f;
            if (lamp == 1) emit = high ? 0.78f : 0.72f;
            else if (auxLamp == 1) emit = (t == LT_VOID) ? 0.68f : 0.82f;
            else if (high && !s.dark && t != LT_WALL) {
                emit = 0.12f;
                if (s.megaForm == MF_CELLBLOCK && wx != sr.x0 && wz != sr.z0) {
                    int lx = wx - sr.x0 - 1, lz = wz - sr.z0 - 1;
                    if (lx <= 4 || lx >= s.w - 5 || lz <= 4 || lz >= s.h - 5)
                        emit = 0.24f + (float)s.genome.g[LG_LIGHT] * 0.36f / 255.0f;
                } else if (s.megaForm == MF_SILO && wx != sr.x0 && wz != sr.z0) {
                    float dx = (float)(wx - sr.x0 - 1 - s.megaOffX) - s.w * 0.5f, dz = (float)(wz - sr.z0 - 1 - s.megaOffZ) - s.h * 0.5f;
                    if (dx * dx + dz * dz < 22.0f) emit = 0.58f;
                } else if (s.megaForm == MF_ZIGGURAT && t == LT_BASIN) {
                    emit = 0.42f;
                } else if (s.megaForm == MF_CANYON && lmPitDegreeLocal(s, wx - sr.x0 - 1, wz - sr.z0 - 1) > 0) {
                    emit = 0.38f;
                } else if (s.megaForm == MF_GROTTO && wx != sr.x0 && wz != sr.z0) {
                    float dx = (float)(wx - sr.x0 - 1) - s.w * 0.5f, dz = (float)(wz - sr.z0 - 1) - s.h * 0.5f;
                    emit = (dx * dx + dz * dz < 14.0f) ? 0.72f : 0.05f;
                } else if (s.megaForm == MF_TENDRILS) {
                    emit = 0.62f;
                } else if (s.megaForm == MF_SPIRES) {
                    emit = 0.42f;
                } else if (s.megaForm == MF_MONOLITH) {
                    emit = 0.34f;
                }
            }
            lightA[z][x] = emit;

            bool inXeno = (wx != sr.x0 && wz != sr.z0) && s.xeno.active &&
                          (s.zone[wx - sr.x0 - 1][wz - sr.z0 - 1] & LZ_XENO) != 0;
            palA[z][x]  = (float)(inXeno ? s.xeno.hue : s.genome.g[LG_HUE]) / 255.0f;
            megaA[z][x] = !high ? 0.0f : (s.dark ? 0.18f : (s.ceilTiers == 2 ? 0.56f : (s.ceilTiers == 3 ? 0.78f : 1.0f)));

            float ao = 1.0f;
            if (t != LT_WALL) {
                if (lmTagAt(wx - 1, wz) == LT_WALL) ao -= 0.13f;
                if (lmTagAt(wx + 1, wz) == LT_WALL) ao -= 0.13f;
                if (lmTagAt(wx, wz - 1) == LT_WALL) ao -= 0.13f;
                if (lmTagAt(wx, wz + 1) == LT_WALL) ao -= 0.13f;
                unsigned short f = lmFlagAt(wx, wz);
                if (f & (LF_NARROW_X | LF_NARROW_Z)) ao -= 0.10f;
                if (f & LF_CRAMPED) ao -= 0.10f;
            } else {
                ao = 0.62f;
            }
            if (ao < 0.36f) ao = 0.36f;
            aoByte[z][x] = (unsigned char)(ao * 255.0f);
        }
    }
    lmEnvJobRow = rowEnd;
    if (rowEnd < LM_ENV_SIZE) return;   // more rows next frame
    lmEnvJobRow = -1;
    lmEnvOriginX = ox; lmEnvOriginZ = oz;
    lmEnvBuiltSeed = worldSeed; lmEnvBuiltLevel = gCurrentLevel; lmEnvBuiltEpoch = gLmSiteEpoch;

    for (int pass = 0; pass < 3; pass++) {
        for (int z = 0; z < LM_ENV_SIZE; z++) {
            for (int x = 0; x < LM_ENV_SIZE; x++) {
                float s = lightA[z][x] * 0.50f +
                          lightA[z][x > 0 ? x - 1 : 0] * 0.25f +
                          lightA[z][x + 1 < LM_ENV_SIZE ? x + 1 : x] * 0.25f;
                lightB[z][x] = wallMask[z][x] ? s * 0.28f : s;
            }
        }
        for (int z = 0; z < LM_ENV_SIZE; z++) {
            int z0 = z > 0 ? z - 1 : 0, z1 = z + 1 < LM_ENV_SIZE ? z + 1 : z;
            for (int x = 0; x < LM_ENV_SIZE; x++) {
                float s = lightB[z][x] * 0.50f + lightB[z0][x] * 0.25f + lightB[z1][x] * 0.25f;
                lightA[z][x] = wallMask[z][x] ? s * 0.28f : s;
            }
        }
    }

    static unsigned char rgba[LM_ENV_SIZE * LM_ENV_SIZE * 4];
    for (int z = 0; z < LM_ENV_SIZE; z++) {
        for (int x = 0; x < LM_ENV_SIZE; x++) {
            float pSum = 0.0f, mSum = 0.0f;
            int cnt = 0;
            for (int dz = -2; dz <= 2; dz++) {
                int zz = z + dz;
                if (zz < 0 || zz >= LM_ENV_SIZE) continue;
                for (int dx = -2; dx <= 2; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= LM_ENV_SIZE) continue;
                    pSum += palA[zz][xx];
                    mSum += megaA[zz][xx];
                    cnt++;
                }
            }
            float rawL = lightA[z][x] * 2.15f;
            float lVal = rawL / (1.0f + rawL * 0.68f);
            size_t o = ((size_t)z * LM_ENV_SIZE + (size_t)x) * 4;
            rgba[o + 0] = (unsigned char)(lVal * 255.0f);
            rgba[o + 1] = aoByte[z][x];
            rgba[o + 2] = (unsigned char)((pSum / (float)cnt) * 255.0f);
            rgba[o + 3] = (unsigned char)((mSum / (float)cnt) * 255.0f);
        }
    }

    glActiveTexture(GL_TEXTURE0 + 5);
    if (lmEnvTex == 0) {
        glGenTextures(1, &lmEnvTex);
        glBindTexture(GL_TEXTURE_2D, lmEnvTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glBindTexture(GL_TEXTURE_2D, lmEnvTex);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LM_ENV_SIZE, LM_ENV_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glActiveTexture(GL_TEXTURE0);
}

// Continuous surface synthesizer: generates seamless RGBA8 + heightmap-alpha textures from
// `LmSurfDNA` without discrete material enums. Tile, wallpaper, board-formed concrete,
// smooth plaster, carpet, and acoustic panels emerge continuously from `sf`.
inline void lmUploadContinuousSurfaceTex(GLuint& texId, const LmSurfDNA& sf, int sz, float seedOfs) {
    std::vector<unsigned char> d((size_t)sz * sz * 4);
    auto clampB = [](float c) -> unsigned char {
        return (unsigned char)(c < 0.0f ? 0.0f : (c > 255.0f ? 255.0f : c));
    };

    int repX = 1 + (int)sf.seamFreqX * 15 / 255; // integer repeats [1..16] for seamless GL_REPEAT
    int repY = 1 + (int)sf.seamFreqY * 15 / 255;
    // Second-pigment patterns (surfaces without one, e.g. the vault poles, keep rgb2 = 0)
    const bool  two       = ((int)sf.rgb2[0] + (int)sf.rgb2[1] + (int)sf.rgb2[2]) > 0;
    const float checkAmt  = two ? (float)sf.checker / 255.0f : 0.0f;
    const float plankAmt  = (float)sf.plank / 255.0f;
    const float speckAmt  = two ? (float)sf.speckle / 255.0f : 0.0f;
    const float motifAmt  = two ? (float)sf.motif / 255.0f : 0.0f;
    const float stripeMix = (two && sf.stripe > 150) ? fminf((float)(sf.stripe - 150) / 60.0f, 1.0f) : 0.0f;
    if (checkAmt > 0.15f) { repX += repX & 1; repY += repY & 1; }   // parity has to wrap
    const int   motifRep  = 2 * (repX < 8 ? repX : 8);              // half-drop needs an even count
    const bool  fibreU    = repY > repX;                             // fibre runs along the long side
    const int   chip      = sz >= 512 ? sz / 256 : 1;
    const int   chipN     = sz / chip;
    const float r2R = (float)sf.rgb2[0], r2G = (float)sf.rgb2[1], r2B = (float)sf.rgb2[2];
    auto cellHash = [](int a, int b, int c) -> float {
        unsigned int h = (unsigned int)a * 0x9E3779B1u ^ ((unsigned int)b * 0x85EBCA6Bu + 0x27D4EB2Fu) ^ ((unsigned int)c * 0xC2B2AE35u);
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        return (float)(h & 0xFFFFu) / 65535.0f;
    };
    const int hashSalt = (int)(seedOfs * 97.0f);
    const float seamAmt   = sf.seamDepth > 28 ? (float)(sf.seamDepth - 28) / 227.0f : 0.0f;
    const float stripeAmt = (float)sf.stripe / 255.0f;
    const float grainAmt  = (float)sf.grain / 255.0f;
    const float grimeAmt  = (float)sf.grime / 255.0f;
    const float sheenAmt  = (float)sf.sheen / 255.0f;
    const int   stripeRep = 2 + ((int)sf.stripe / 24) * 2; // even harmonic count
    const float stagger   = (sf.bond >= 128 || (!two && sf.seamFreqX != sf.seamFreqY && sf.stripe > 110)) ? 0.5f : 0.0f;
    const float gw        = 0.014f + 0.022f * seamAmt;
    const float baseR = (float)sf.rgb[0], baseG = (float)sf.rgb[1], baseB = (float)sf.rgb[2];

    for (int y = 0; y < sz; y++) {
        float v = (float)y / (float)sz;
        float pv = v * (float)repY;
        float rowIdx = floorf(pv);
        float fv = pv - rowIdx;
        float dv = fminf(fv, 1.0f - fv);
        float bandV = cosf(v * 6.2831853f * (float)(stripeRep / 2 + 2));

        for (int x = 0; x < sz; x++) {
            float u = (float)x / (float)sz;
            float pu = u * (float)repX + (fmodf(rowIdx, 2.0f) > 0.5f ? stagger : 0.0f);
            float colIdx = floorf(pu);
            float fu = pu - colIdx;
            if (colIdx >= (float)repX) colIdx -= (float)repX;   // a shifted block wraps onto itself
            float du = fminf(fu, 1.0f - fu);

            // 1. Continuous joint/grout network + per-block tonal drift
            float edgeD = fminf(du, dv);
            float groove = (edgeD < gw) ? (1.0f - edgeD / gw) * seamAmt : 0.0f;
            float cellTone = noise2d(colIdx * 19.0f + seedOfs, rowIdx * 29.0f + seedOfs * 0.7f) * (10.0f * seamAmt);
            float blockRnd = cellHash((int)colIdx, (int)rowIdx, hashSalt);
            cellTone += (blockRnd - 0.5f) * 44.0f * plankAmt;

            // Tie holes on wide panels vs subtle diamond/woven accent on patterned surfaces
            float detail = 0.0f;
            if (repX <= 4 && seamAmt > 0.35f) {
                float cx = fminf(fabsf(fu - 0.16f), fabsf(fu - 0.84f));
                float cy = fminf(fabsf(fv - 0.24f), fabsf(fv - 0.76f));
                float dHole = sqrtf(cx * cx + cy * cy);
                if (dHole < 0.022f) detail -= (1.0f - dHole / 0.022f) * 32.0f * seamAmt;
            } else if (stripeAmt > 0.35f && seamAmt < 0.45f) {
                float dmd = 1.0f - (fabsf(fu - 0.5f) + fabsf(fv - 0.5f)) * 2.0f;
                if (dmd > 0.0f) detail += dmd * 11.0f * stripeAmt * (1.0f - seamAmt);
            }

            // 2. Periodic striping / ribbing / weave
            float wave = sinf(u * 6.2831853f * (float)stripeRep);
            float band = (wave * 9.5f + bandV * 3.5f) * stripeAmt * (1.0f - seamAmt * 0.45f) * (1.0f - stripeMix * 0.6f);

            // 3. Multi-scale aggregate / fiber grain
            float rough = perlin(x * 0.09f + seedOfs, y * 0.09f + seedOfs, 3);
            float fine  = perlin(x * 0.42f + 11.0f, y * 0.42f + 3.0f, 2);
            float texGrain = (rough * 19.0f + fine * 9.0f) * grainAmt;
            if (plankAmt > 0.02f) {
                // wood fibre: stretched along the board, periodic along it, restarted per board
                float a = fibreU ? (float)x : (float)y, c = fibreU ? (float)y : (float)x;
                float t = a / (float)sz, ofs = blockRnd * 211.0f;
                float fib = perlin((a + (float)sz) * 0.011f + ofs, c * 0.38f + ofs, 3) * (1.0f - t) +
                            perlin(a * 0.011f + ofs, c * 0.38f + ofs, 3) * t;
                texGrain = texGrain * (1.0f - 0.6f * plankAmt) + fib * 30.0f * plankAmt;
            }

            // 4. Grime blotches & vertical damp streaks
            float blotch = perlin(x * 0.016f + seedOfs, y * 0.014f + 5.0f, 3);
            float stain  = blotch > 0.25f ? (blotch - 0.25f) * -42.0f * grimeAmt : 0.0f;
            float drip   = perlin(x * 0.05f + 4.0f, y * 0.008f + seedOfs, 2);
            if (drip > 0.32f) stain -= (drip - 0.32f) * 28.0f * grimeAmt;

            // 5. Second pigment: checker blocks, wallpaper stripes, half-drop ornament, chips
            float mix2 = 0.0f, chipDark = 0.0f;
            if (checkAmt > 0.0f && (((int)colIdx + (int)rowIdx) & 1)) mix2 = checkAmt;
            if (stripeMix > 0.0f) {
                float sq = wave * 2.2f + 0.5f;
                sq = sq < 0.0f ? 0.0f : (sq > 1.0f ? 1.0f : sq);
                mix2 = fmaxf(mix2, sq * stripeMix * 0.9f);
            }
            if (motifAmt > 0.0f) {
                float mu = u * (float)motifRep;
                int mc = (int)mu;
                float mvv = v * (float)motifRep + ((mc & 1) ? 0.5f : 0.0f);
                float lu = mu - (float)mc - 0.5f, lv = mvv - floorf(mvv) - 0.5f;
                float au = fabsf(lu), av = fabsf(lv), m = 0.0f;
                auto edge = [](float d, float r0) { float e = (r0 - d) / 0.035f; return e < 0.0f ? 0.0f : (e > 1.0f ? 1.0f : e); };
                if (sf.motifKind < 85) {
                    m = edge(sqrtf(lu * lu + lv * lv), 0.19f);                               // dots
                } else if (sf.motifKind < 170) {
                    float dd = au + av;
                    m = fmaxf(edge(dd, 0.36f) - edge(dd, 0.25f), edge(dd, 0.10f));           // diamond ring + pip
                } else {
                    float pu2 = au - 0.17f, pv2 = av - 0.17f;                                 // quatrefoil
                    m = fmaxf(edge(sqrtf(pu2 * pu2 + lv * lv), 0.14f), edge(sqrtf(lu * lu + pv2 * pv2), 0.14f));
                    m = fmaxf(m - edge(sqrtf(lu * lu + lv * lv), 0.07f), 0.0f);
                }
                mix2 = fmaxf(mix2, m * motifAmt * 0.9f);
            }
            if (speckAmt > 0.0f) {
                float hc = cellHash((x / chip) % chipN, (y / chip) % chipN, hashSalt + 7);
                float pc = 0.04f + 0.20f * speckAmt;
                if (hc < pc) mix2 = fmaxf(mix2, 0.55f + 0.45f * speckAmt);
                else if (hc > 1.0f - pc * 0.5f) chipDark = 20.0f * speckAmt;
            }
            mix2 *= (1.0f - groove);

            float delta = cellTone + detail + band + texGrain - groove * 34.0f - chipDark;
            float r = baseR + (r2R - baseR) * mix2 + delta + stain * 0.85f;
            float g = baseG + (r2G - baseG) * mix2 + delta * 0.97f + stain * 0.92f;
            float b = baseB + (r2B - baseB) * mix2 + delta * 0.92f + stain * 0.96f;
            float h = 128.0f + texGrain * (0.65f - 0.40f * sheenAmt) + detail * 0.8f + band * 0.35f - groove * 34.0f;

            size_t o = ((size_t)y * sz + (size_t)x) * 4;
            d[o + 0] = clampB(r);
            d[o + 1] = clampB(g);
            d[o + 2] = clampB(b);
            d[o + 3] = clampB(h);
        }
    }

    if (texId == 0) glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz, sz, 0, GL_RGBA, GL_UNSIGNED_BYTE, d.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
