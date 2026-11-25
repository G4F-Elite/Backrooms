#pragma once
inline constexpr char sceneEffectsFS[] = R"(#version 330
out vec4 F; in vec2 uv;
uniform sampler2D tex, depthTex;
uniform int rtxA, rtxG, rtxR, rtxB, rtxDenoiseOn, rayN;
uniform float texelX, texelY, rtxDenoiseStrength, depthA;
uniform vec3 projInfo, rayL[4];
#define projT projInfo.xy
float linZ(vec2 tc){
 float d = texture(depthTex, tc).r;
 return projInfo.z / (d * 2.0 - 1.0 + depthA);
}
float fxLum(vec3 c){ return dot(c, vec3(0.3, 0.59, 0.11)); }
// View-space position (x right, y up, z forward) rebuilt from the depth buffer.
vec3 fxPos(vec2 tc){
 float z = linZ(tc);
 return vec3((tc * 2.0 - 1.0) * projT * z, z);
}
// Surface normal from depth, taking the neighbour on the same surface on each axis.
vec3 fxNormal(vec2 tc, vec3 p){
 vec3 r = fxPos(tc + vec2(texelX, 0.0)) - p;
 vec3 l = p - fxPos(tc - vec2(texelX, 0.0));
 vec3 u = fxPos(tc + vec2(0.0, texelY)) - p;
 vec3 d = p - fxPos(tc - vec2(0.0, texelY));
 vec3 dx = abs(r.z) < abs(l.z) ? r : l;
 vec3 dy = abs(u.z) < abs(d.z) ? u : d;
 return normalize(cross(dy, dx));
}
// Screen radius (uv) of a world-space radius at view depth z.
float fxScreenRadius(float worldR, float z, float maxR){
 return clamp(worldR * 0.5 / (z * projT.y), 0.003, maxR);
}
vec2 fxSpiral(int i, int ns, float rot, float rad){
 float a = float(i) * 2.399963 + rot;
 float r = sqrt((float(i) + 0.5) / float(ns)) * rad;
 return vec2(cos(a) * projT.y / projT.x, sin(a)) * r;
}
// Ambient occlusion: only geometry above the surface plane darkens it, so flat
// walls and floors stay clean and only corners, props and contact areas get shade.
float fxAO(vec2 tc, vec3 p, vec3 n, int ns, float rot){
 const float R = 0.65;
 float rad = fxScreenRadius(R, p.z, 0.10);
 float occ = 0.0;
 for(int i = 0; i < ns; i++){
  vec3 v = fxPos(tc + fxSpiral(i, ns, rot, rad)) - p;
  float d2 = dot(v, v);
  float cosT = dot(n, v) * inversesqrt(d2 + 0.00001);
  float range = clamp(1.0 - d2 / (R * R), 0.0, 1.0);
  occ += clamp((cosT - 0.18) * 1.6, 0.0, 1.0) * range;
 }
 float ao = 1.0 - occ / float(ns) * 2.6;
 float fade = (1.0 - smoothstep(25.0, 45.0, p.z));
 return mix(1.0, clamp(ao, 0.30, 1.0), fade);
}
// One light bounce from nearby visible surfaces that sit in front of this surface.
vec3 fxGI(vec2 tc, vec3 p, vec3 n, int ns, float rot){
 const float R = 1.8;
 float rad = fxScreenRadius(R, p.z, 0.22);
 vec3 gi = vec3(0.0);
 for(int i = 0; i < ns; i++){
  vec2 sc = tc + fxSpiral(i, ns, rot + 1.7, rad);
  vec3 v = fxPos(sc) - p;
  float d2 = dot(v, v);
  float cosT = max(dot(n, v) * inversesqrt(d2 + 0.00001) - 0.10, 0.0);
  float range = clamp(1.0 - d2 / (R * R), 0.0, 1.0);
  gi += texture(tex, clamp(sc, 0.001, 0.999)).rgb * cosT * range;
 }
 return gi / float(ns);
}
// Light shafts: bright lamp pixels smeared away from each visible lamp. Anything
// dark between the pixel and the lamp cuts the shaft.
vec3 fxRays(vec2 tc, float jit){
 const int NS = 14;
 vec3 acc = vec3(0.0);
 for(int l = 0; l < rayN; l++){
  vec2 d = rayL[l].xy - tc;
  float dist = length(d * vec2(projT.x / projT.y, 1.0));
  if(dist > 0.60) continue;
  float fall = 1.0 - dist / 0.60;
  vec3 s = vec3(0.0);
  for(int i = 0; i < NS; i++){
   vec2 sc = tc + d * ((float(i) + jit) / float(NS));
   vec3 col = texture(tex, sc).rgb;
   s += col * smoothstep(0.72, 0.96, fxLum(col));
  }
  acc += s / float(NS) * fall * fall * rayL[l].z;
 }
 return acc;
}
// Bloom: only really bright pixels (lamps, screens) glow, with a normalised gaussian.
vec3 fxBloom(vec2 tc){
 vec2 stp = vec2(projT.y / projT.x, 1.0) * 0.0030;
 // Normalized, two-radius tent around clipped highlights.
 vec3 glow = vec3(0.0);
 for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) {
  vec2 offset = vec2(x,y);
  float weight = (x == 0 ? 2.0 : 1.0) * (y == 0 ? 2.0 : 1.0);
  vec3 light = texture(tex, tc + offset * stp * 1.6).rgb;
  glow += weight * light * smoothstep(0.70, 0.97, fxLum(light));
 }
 for (int i = 0; i < 4; ++i) {
  vec2 offset = vec2((i & 1) == 0 ? -1.0 : 1.0, (i & 2) == 0 ? -1.0 : 1.0);
  vec3 light = texture(tex, tc + offset * stp * 3.1).rgb;
  glow += light * smoothstep(0.70, 0.97, fxLum(light));
 }
 return glow * 0.05;
}
// The denoiser averages a noisy term over its 2x2 pixel quad (each pixel uses a
// different sample rotation), which removes the grain without blurring the image.
float fxQuad(float v, vec2 q){ v -= dFdx(v) * q.x; return v - dFdy(v) * q.y; }
vec3 fxQuad(vec3 v, vec2 q){ v -= dFdx(v) * q.x; return v - dFdy(v) * q.y; }
vec3 applyFx(vec2 tc, vec3 c){
 if(rtxA <= 0 && rtxG <= 0 && rtxR <= 0 && rtxB <= 0) return c;
 float ign = fract(52.9829189 * fract(dot(floor(gl_FragCoord.xy), vec2(0.06711056, 0.00583715))));
 float ao = 1.0;
 vec3 gi = vec3(0.0);
 vec3 rays = vec3(0.0);
 if((rtxA > 0 || rtxG > 0) && texture(depthTex, tc).r < 0.99995){
  vec3 p = fxPos(tc);
  vec3 n = fxNormal(tc, p);
  float rot = ign * 6.2831853;
  if(rtxA > 0) ao = fxAO(tc, p, n, rtxA <= 1 ? 8 : (rtxA <= 2 ? 12 : (rtxA <= 3 ? 16 : 24)), rot);
  if(rtxG > 0) gi = fxGI(tc, p, n, rtxG <= 1 ? 8 : (rtxG <= 2 ? 12 : 18), rot);
 }
 if(rtxR > 0) rays = fxRays(tc, ign);
 if(rtxDenoiseOn > 0){
  vec2 q = (mod(floor(gl_FragCoord.xy), 2.0) - 0.5) * clamp(rtxDenoiseStrength, 0.0, 1.0);
  ao = fxQuad(ao, q);
  gi = fxQuad(gi, q);
  rays = fxQuad(rays, q);
 }
 vec3 lit = c * mix(1.0, ao, 1.0 - smoothstep(0.75, 1.0, fxLum(c)));
 lit += gi * (c + 0.06) * 1.5;
 lit += rays * 0.55;
 if(rtxB > 0) lit += fxBloom(tc) * 0.40;
 float luma = fxLum(lit);
 if (luma > 0.72) lit *= (0.72 + (luma - 0.72) / (1.0 + (luma - 0.72) * 3.4)) / luma;
 return lit;
}
void main(){ F = vec4(applyFx(uv, texture(tex, uv).rgb), 1.0); }
)";
