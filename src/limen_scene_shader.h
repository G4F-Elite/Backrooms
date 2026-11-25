#pragma once

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

inline const char* lmMainFS =
#include "limen_scene_surface.glsl.inc"
#include "limen_scene_lighting.glsl.inc"
;

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
 float pit = 1.0 - smoothstep(-52.0, -18.0, wpos.y);
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
