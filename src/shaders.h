#pragma once
const char* mainVS=R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t; layout(location=2) in vec3 n;
out vec2 uv; out vec3 fp,nm; out vec3 viewTS; out vec3 fragPosTS;
uniform mat4 M,V,P;
uniform vec3 vp;
void main(){
 fp=vec3(M*vec4(p,1));
 nm=mat3(M)*n;
 uv=t;
 // Compute TBN matrix for parallax
 vec3 N = normalize(nm);
 vec3 T = normalize(cross(N, vec3(0.0, 1.0, 0.1)));
 vec3 B = cross(N, T);
 mat3 TBN = transpose(mat3(T, B, N));
 viewTS = TBN * normalize(vp - fp);
 fragPosTS = TBN * fp;
 gl_Position=P*V*M*vec4(p,1);
})";
const char* mainFS=R"(#version 330
out vec4 F; in vec2 uv; in vec3 fp,nm;
in vec3 viewTS; in vec3 fragPosTS;
uniform sampler2D tex; uniform vec3 vp; uniform float tm; uniform vec3 modelTint;
uniform int nl; uniform vec3 lp[16]; uniform float danger;
uniform int flashOn; uniform vec3 flashDir; uniform vec3 flashPos;
uniform int rfc; uniform vec3 rfp[4]; uniform vec3 rfd[4];
float hash(vec3 p) {
 return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}
// Optimized single-step parallax for performance
vec2 parallaxOffset(vec2 texCoords, vec3 viewDir) {
 float height = texture(tex, texCoords).a;
 float heightScale = 0.025; // Subtle effect to avoid artifacts
 // Distance-based LOD - disable parallax at distance
 float dist = length(vp - fp);
 float lodFade = 1.0 - smoothstep(8.0, 15.0, dist);
 if(lodFade < 0.01) return texCoords;
 // Simple offset parallax (fastest method)
 float h = (height - 0.5) * heightScale * lodFade;
 vec2 offset = viewDir.xy * h;
 return texCoords + offset;
}
void main(){
 // Apply parallax offset to UV
 vec3 V = normalize(viewTS);
 vec2 texCoord = parallaxOffset(uv, V);
 vec3 tc = texture(tex, texCoord).rgb * modelTint;
 vec3 N = normalize(nm);
 bool isCeil = N.y < -0.5;
 bool isFloor = N.y > 0.5;
 if(isCeil) tc *= vec3(1.02, 1.0, 0.95);
 float ambVal = 0.06 * (1.0 - danger * 0.3);
 vec3 res = vec3(ambVal) * tc;
 for(int i = 0; i < nl && i < 16; i++) {
  vec3 toLight = lp[i] - fp;
  // Calculate distance-based fade for smooth light transitions at edges
  float lightDistFromCam = length(lp[i] - vp);
  float fade = 1.0;
  if(lightDistFromCam > 35.0) {
   fade = 1.0 - (lightDistFromCam - 35.0) / 15.0;
   fade = clamp(fade, 0.0, 1.0);
  }
  if(fade < 0.001) continue;
  float d2 = dot(toLight, toLight);
  float invD = inversesqrt(max(d2, 0.0001));
  float d = d2 * invD;
  vec3 L = toLight * invD;
  float att = 1.0 / (1.0 + 0.1*d + 0.04*d2);
  float df;
  if(isCeil) {
   float horizDist = length(vec2(toLight.x, toLight.z));
   float vertDist = abs(toLight.y);
   float spread = 1.0 / (1.0 + horizDist * 0.85);
   df = spread * 1.5;
   if(vertDist < 0.3) df *= 2.0;
   else if(vertDist < 1.0) df *= 1.5;
   df += (1.0 / (1.0 + horizDist * 0.3)) * 0.3;
  } else {
   float NdotL = max(dot(N, L), 0.0);
   df = smoothstep(0.0, 0.5, NdotL);
  }
  float baseFlick = sin(tm*20.0 + float(i)*2.1) * 0.015;
  float panicFlick = sin(tm*30.0 + float(i)*3.7) * 0.05 * danger;
  float fl = 1.0 + baseFlick + panicFlick;
  vec3 lightColor = vec3(1.0, 0.92, 0.75);
  // Apply fade factor for smooth light transitions
  res += df * lightColor * fl * tc * att * 0.5 * fade;
 }
 // FLASHLIGHT - white cone with red danger edge
 if(flashOn == 1) {
  vec3 toFrag = normalize(fp - flashPos);
  float spotAngle = dot(toFrag, flashDir);
  float dist = length(fp - flashPos);
  if(spotAngle > 0.85 && dist < 15.0) {
   float spotAtt = smoothstep(0.85, 0.95, spotAngle);
   float distAtt = 1.0 - dist / 15.0;
   float NdotL = max(dot(N, -toFrag), 0.0);
   vec3 flashColor = mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 0.3, 0.2), danger * 0.6);
   if(spotAngle < 0.9 && danger > 0.2) flashColor = vec3(0.9, 0.2, 0.1);
   res += tc * flashColor * spotAtt * distAtt * NdotL * 1.5;
  }
 }
 // REMOTE FLASHLIGHTS (other players in multiplayer)
 for(int i = 0; i < rfc && i < 4; i++) {
  vec3 toFrag = normalize(fp - rfp[i]);
  float spotAngle = dot(toFrag, rfd[i]);
  float dist = length(fp - rfp[i]);
  if(spotAngle > 0.88 && dist < 13.0) {
   float spotAtt = smoothstep(0.88, 0.96, spotAngle);
   float distAtt = 1.0 - dist / 13.0;
   float NdotL = max(dot(N, -toFrag), 0.0);
   vec3 remoteFlash = vec3(0.9, 0.95, 1.0);
   res += tc * remoteFlash * spotAtt * distAtt * NdotL * 0.95;
  }
 }
 if(isCeil) {
  float maxB = max(res.r, max(res.g, res.b));
  if(maxB > 0.1) res = max(res, tc * 0.15);
 }
 // VOLUMETRIC FOG - layered with height variation for depth
 float dist = length(vp - fp);
 // Multi-layered fog for volumetric feel
 float fogDensityBase = 0.055;
 float fogDensityHigh = 0.075;
 // Height-based fog density - thicker near floor
 float heightFog = mix(fogDensityHigh, fogDensityBase, smoothstep(0.0, 2.5, fp.y));
 float fog = exp(-dist * heightFog);
 fog = clamp(fog, 0.0, 1.0);
 // Animated volumetric noise - swirling fog
 float fogNoise1 = hash(fp * 0.13 + vec3(tm * 0.03, tm * 0.02, tm * 0.01));
 float fogNoise2 = hash(fp * 0.07 + vec3(-tm * 0.015, tm * 0.025, tm * 0.008));
 float fogNoise = mix(fogNoise1, fogNoise2, 0.5);
 fog = fog * (0.94 + fogNoise * 0.06);
 // Warm atmospheric fog color - tinted by nearby lights
 vec3 baseFogColor = vec3(0.055, 0.048, 0.038);
 // Warm light scatter in fog from nearby lights
 float lightScatter = 0.0;
 for(int i = 0; i < nl && i < 16; i++) {
  float ld = length(lp[i] - fp);
  lightScatter += 1.0 / (1.0 + ld * ld * 0.08) * 0.15;
 }
 lightScatter = min(lightScatter, 0.5);
 vec3 fogColor = baseFogColor + vec3(0.06, 0.045, 0.02) * lightScatter;
 // Distance-based color desaturation with warm shift
 float desat = smoothstep(6.0, 18.0, dist);
 float gray = dot(res, vec3(0.3, 0.6, 0.1));
 vec3 desatColor = vec3(gray * 1.05, gray * 0.98, gray * 0.88); // warm desaturation
 res = mix(res, desatColor, desat * 0.45);
 // Distance-based darkening before fog (smooth LOD transition)
 float distDarken = smoothstep(12.0, 22.0, dist);
 res *= (1.0 - distDarken * 0.55);
 // Dust particles in light beams (subtle sparkle)
 float dustParticle = hash(fp * 1.7 + vec3(tm * 0.3));
 float dustVisible = smoothstep(0.97, 1.0, dustParticle) * lightScatter * 2.0;
 float dustDist = smoothstep(12.0, 2.0, dist);
 res += vec3(0.8, 0.75, 0.5) * dustVisible * dustDist * 0.12;
 // Apply fog
 res = mix(fogColor, res, fog);
 // Atmospheric warm tint
 res *= vec3(1.02, 0.97, 0.88);
 // Subtle color grading - lift shadows warm, cool highlights
 float lum = dot(res, vec3(0.3, 0.6, 0.1));
 vec3 shadowTint = vec3(0.02, 0.015, 0.005) * (1.0 - smoothstep(0.0, 0.15, lum));
 vec3 highlightTint = vec3(-0.005, 0.0, 0.01) * smoothstep(0.3, 0.8, lum);
 res += shadowTint + highlightTint;
 // Danger red tint in environment
 if(danger > 0.3) {
  res.r += danger * 0.08;
  res.gb *= (1.0 - danger * 0.15);
 }
 if(danger > 0.0) {
  float grayD = dot(res, vec3(0.3, 0.6, 0.1));
  res = mix(res, vec3(grayD), danger * 0.2);
 }
 F = vec4(clamp(res, 0.0, 1.0), 1.0);
})";
const char* lightVS=R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t;
out vec2 uv; out vec3 wpos; uniform mat4 M,V,P;
void main(){
 uv=t;
 wpos = vec3(M*vec4(p,1));
 gl_Position=P*V*M*vec4(p,1);
})";
const char* lightFS=R"(#version 330
out vec4 F; in vec2 uv; in vec3 wpos;
uniform sampler2D tex; uniform float inten,tm,fade;
uniform float danger;
float hash(float n){ return fract(sin(n)*43758.5453); }
float hash2(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
void main(){
 vec3 c = texture(tex,uv).rgb * inten;

 // Stable per-light seed from position
 float seed = hash2(floor(wpos.xz * 0.5));

 // Base flicker: subtle CRT-ish shimmer
 float flick = 0.97
            + sin(tm*25.0 + seed*6.283)*0.015
            + hash(floor(tm*7.0) + seed*19.0)*0.015;

 // Panic flicker: becomes obvious with danger
 float panicAmt = smoothstep(0.10, 0.60, danger);
 flick += (sin(tm*(55.0 + seed*10.0) + seed*12.3) * 0.06
        +  (hash(floor(tm*18.0) + seed*53.0) - 0.5) * 0.08) * panicAmt;

 // Drop-outs (brief near-black blinks) when danger is high
 float dropAmt = smoothstep(0.35, 0.95, danger);
 float dropGate = hash(floor(tm*(2.0 + 10.0*danger)) + seed*101.0);
 float drop = step(0.988, dropGate) * dropAmt;
 flick *= (1.0 - drop * 0.85);

 flick = clamp(flick, 0.15, 1.35);
 F = vec4(c * flick * fade, fade);
})";
const char* vhsVS=R"(#version 330
layout(location=0) in vec2 p; layout(location=1) in vec2 t;
out vec2 uv; void main(){ uv=t; gl_Position=vec4(p,0,1); })";
const char* vhsFS=R"(#version 330
out vec4 F; in vec2 uv;
uniform sampler2D tex; uniform float tm,inten;
uniform int upscaler;
uniform int aaMode;
uniform float sharpness;
uniform float texelX;
uniform float texelY;
uniform sampler2D histTex;
uniform float taaBlend;
uniform vec3 taaJitter;
uniform float taaValid;
uniform int frameGen;
uniform float frameGenBlend;
uniform int rtxA,rtxG,rtxR,rtxB;
uniform int rtxDenoiseOn;
uniform float rtxDenoiseStrength;
uniform sampler2D depthTex;
uniform vec3 projInfo;
#define projT projInfo.xy
uniform int rayN;
uniform vec3 rayL[4];
uniform float deathFx;
float rnd(vec2 s){ return fract(sin(dot(s,vec2(12.9898,78.233)))*43758.5453); }
float noise(vec2 p) {
 vec2 i = floor(p);
 vec2 f = fract(p);
 f = f * f * (3.0 - 2.0 * f);
 float a = rnd(i);
 float b = rnd(i + vec2(1.0, 0.0));
 float c = rnd(i + vec2(0.0, 1.0));
 float d = rnd(i + vec2(1.0, 1.0));
 return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
vec3 fsr1Sample(vec2 tc){
 vec3 c = texture(tex, tc).rgb;
 vec3 n = texture(tex, tc + vec2(0.0, texelY)).rgb;
 vec3 s = texture(tex, tc - vec2(0.0, texelY)).rgb;
 vec3 e = texture(tex, tc + vec2(texelX, 0.0)).rgb;
 vec3 w = texture(tex, tc - vec2(texelX, 0.0)).rgb;
 vec3 mn = min(c, min(min(n, s), min(e, w)));
 vec3 mx = max(c, max(max(n, s), max(e, w)));
 vec3 lap = (n + s + e + w) - c * 4.0;
 float span = max(max(mx.r - mn.r, mx.g - mn.g), mx.b - mn.b);
 float adaptive = 1.0 / (1.0 + span * 16.0);
 vec3 rcas = c - lap * (0.28 + sharpness * 0.92) * adaptive;
 return clamp(rcas, 0.0, 1.0);
}
vec3 fsr2Sample(vec2 tc){
 vec3 c = texture(tex, tc).rgb;
 vec3 n = texture(tex, tc + vec2(0.0, texelY)).rgb;
 vec3 s = texture(tex, tc - vec2(0.0, texelY)).rgb;
 vec3 e = texture(tex, tc + vec2(texelX, 0.0)).rgb;
 vec3 w = texture(tex, tc - vec2(texelX, 0.0)).rgb;
 vec3 nw = texture(tex, tc + vec2(-texelX, texelY)).rgb;
 vec3 ne = texture(tex, tc + vec2(texelX, texelY)).rgb;
 vec3 sw = texture(tex, tc + vec2(-texelX, -texelY)).rgb;
 vec3 se = texture(tex, tc + vec2(texelX, -texelY)).rgb;
 vec3 base = (c * 0.30) + (n + s + e + w) * 0.10 + (nw + ne + sw + se) * 0.075;
 if(taaValid > 0.5){
  vec2 reproj = tc - taaJitter.xy * vec2(texelX, texelY) * 0.75;
  vec3 hist = texture(histTex, reproj).rgb;
  vec3 mn = min(c, min(min(n, s), min(e, w)));
  vec3 mx = max(c, max(max(n, s), max(e, w)));
  vec3 clampedHist = clamp(hist, mn, mx);
  float motionEdge = abs(texture(depthTex, tc).r - texture(depthTex, reproj).r);
  float stability = 1.0 - smoothstep(0.01, 0.12, motionEdge);
  base = mix(base, clampedHist, (0.35 + sharpness * 0.25) * stability);
 }
 vec3 lap = (n + s + e + w) - base * 4.0;
 vec3 rcas = base - lap * (0.24 + sharpness * 0.78);
 return clamp(rcas, 0.0, 1.0);
}
vec3 fxaaResolve(vec2 tc, vec3 base){
 vec3 nw = texture(tex, tc + vec2(-texelX, texelY)).rgb;
 vec3 ne = texture(tex, tc + vec2(texelX, texelY)).rgb;
 vec3 sw = texture(tex, tc + vec2(-texelX, -texelY)).rgb;
 vec3 se = texture(tex, tc + vec2(texelX, -texelY)).rgb;
 float lumBase = dot(base, vec3(0.299, 0.587, 0.114));
 float lumMin = min(lumBase, min(min(dot(nw,vec3(0.299,0.587,0.114)), dot(ne,vec3(0.299,0.587,0.114))), min(dot(sw,vec3(0.299,0.587,0.114)), dot(se,vec3(0.299,0.587,0.114)))));
 float lumMax = max(lumBase, max(max(dot(nw,vec3(0.299,0.587,0.114)), dot(ne,vec3(0.299,0.587,0.114))), max(dot(sw,vec3(0.299,0.587,0.114)), dot(se,vec3(0.299,0.587,0.114)))));
 float contrast = lumMax - lumMin;
 vec3 avg = (nw + ne + sw + se + base) * 0.2;
 float blend = smoothstep(0.04, 0.20, contrast) * 0.55;
 return mix(base, avg, blend);
}
vec3 sourceSample(vec2 tc){
 if(upscaler == 1){
  return fsr1Sample(tc);
 }
 if(upscaler == 2){
  return fsr2Sample(tc);
 }
 return texture(tex, tc).rgb;
}
vec3 taaResolve(vec2 tc){
 vec2 off = taaJitter.xy * vec2(texelX, texelY);
 vec3 cur = sourceSample(tc + off);
 vec3 n = sourceSample(tc + vec2(0.0, texelY));
 vec3 s = sourceSample(tc - vec2(0.0, texelY));
 vec3 e = sourceSample(tc + vec2(texelX, 0.0));
 vec3 w = sourceSample(tc - vec2(texelX, 0.0));
 vec3 mn = min(cur, min(min(n, s), min(e, w)));
 vec3 mx = max(cur, max(max(n, s), max(e, w)));
 vec3 hist = texture(histTex, tc).rgb;
 vec3 clampedHist = clamp(hist, mn, mx);
 float useHist = clamp(taaValid, 0.0, 1.0);
 return mix(cur, clampedHist, taaBlend * useHist);
}
vec3 resolveSample(vec2 tc){
 vec3 base = sourceSample(tc);
 if(aaMode == 1){
  return fxaaResolve(tc, base);
 }
 if(aaMode == 2){
  return taaResolve(tc);
 }
 return base;
}
float linZ(vec2 tc){
 float d = texture(depthTex, tc).r;
 return 20.0 / (100.1 - (d * 2.0 - 1.0) * 99.9);
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
 float fade = smoothstep(45.0, 25.0, p.z);
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
 vec3 bl = vec3(0.0);
 float tw = 0.0;
 for(int x = -4; x <= 4; x++) for(int y = -4; y <= 4; y++){
  vec2 o = vec2(float(x), float(y));
  float w = exp(-dot(o, o) * 0.16);
  vec3 s = texture(tex, tc + o * stp).rgb;
  bl += s * smoothstep(0.70, 0.97, fxLum(s)) * w;
  tw += w;
 }
 return bl / tw;
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
 return lit;
}
vec3 deathPost(vec2 tc, vec3 base){
 float d = clamp(deathFx, 0.0, 1.0); if(d <= 0.001) return base;
 vec2 dir = tc - 0.5; float len2 = dot(dir, dir); vec2 nDir = len2 > 0.000001 ? normalize(dir) : vec2(1.0, 0.0);
 float blur = (0.002 + sqrt(len2) * 0.010) * d;
 vec3 b0 = resolveSample(tc + nDir * blur) * 0.24 + resolveSample(tc - nDir * blur) * 0.24 + resolveSample(tc + nDir * blur * 2.2) * 0.11 + resolveSample(tc - nDir * blur * 2.2) * 0.11 + base * 0.30;
 float ca = (0.001 + sqrt(len2) * 0.006) * d; vec3 caC = vec3(resolveSample(tc + nDir * ca).r, b0.g, resolveSample(tc - nDir * ca).b);
 float fog = smoothstep(0.15, 0.9, sqrt(len2)) * d * 0.45; return mix(mix(base, caC, 0.75 * d), vec3(dot(caC, vec3(0.299,0.587,0.114))), 0.22 * d) * (1.0 - fog);
}
void main(){
 if(inten < 0.02) {
  vec3 c0 = applyFx(uv, resolveSample(uv));
  c0 = deathPost(uv, c0);
  F = vec4(c0, 1.0);
 return;
 }

float lineJitter = (rnd(vec2(floor(uv.y * 320.0), floor(tm * 30.0))) - 0.5) * 0.0035 * inten;
float ab = 0.0042 * inten;
float abV = 0.0018 * inten;
float r = resolveSample(uv + vec2(ab + lineJitter, abV * 0.6)).r;
float g = resolveSample(uv + vec2(lineJitter * 0.45, 0.0)).g;
float b = resolveSample(uv + vec2(-ab + lineJitter, -abV * 0.35)).b;
vec3 c = vec3(r,g,b);
 if(frameGen == 1 && taaValid > 0.5){
  vec3 prev = texture(histTex, uv).rgb;
  c = mix(c, prev, clamp(frameGenBlend, 0.0, 0.6));
 }

 float grainTime = tm * 0.7;
 float grain1 = rnd(uv * 0.8 + vec2(grainTime, grainTime * 0.73));
 float grain2 = rnd(uv * 1.3 + vec2(grainTime * 1.1, grainTime * 0.5));
 float filmGrain = (grain1 * 0.6 + grain2 * 0.4 - 0.5) * 0.060 * inten;

 float pixelLum = dot(c, vec3(0.3, 0.6, 0.1));
 float grainStrength = mix(2.0, 0.7, smoothstep(0.0, 0.45, pixelLum));
 c += vec3(filmGrain) * grainStrength;

 float scanline = sin(uv.y * 520.0 + tm * 5.5) * 0.5 + 0.5;
 scanline = pow(scanline, 2.2) * 0.026 * inten;
 c -= vec3(scanline);

 if(inten > 0.28 && rnd(vec2(tm * 0.11, floor(uv.y * 85))) > 0.985) {
  float of = (rnd(vec2(tm * 1.9, floor(uv.y * 85))) - 0.5) * 0.024 * inten;
  c = resolveSample(uv + vec2(of, 0));
 }

 vec2 vigUV = uv - 0.5;
  float vigDist = dot(vigUV, vigUV);
 float vig = 1.0 - vigDist * 0.8;
 vig = smoothstep(0.15, 1.0, vig);

 c.r *= mix(vig, 1.0, 0.05);
 c.g *= vig;
 c.b *= mix(vig, 1.0, -0.03);

 float leakAngle = atan(vigUV.y, vigUV.x);
 float leak = sin(leakAngle * 1.5 + tm * 0.05) * 0.5 + 0.5;
 float leakMask = smoothstep(0.35, 0.55, length(vigUV)) * leak * 0.015 * inten;
 c += vec3(leakMask * 1.2, leakMask * 0.9, leakMask * 0.4);

 vec3 ghost = resolveSample(uv - vec2(0.003, 0.0015));
 c = mix(c, ghost * vec3(1.03, 0.98, 0.95), 0.080 * inten);

 float lumC = dot(c, vec3(0.3, 0.6, 0.1));

 c += vec3(0.012, 0.008, 0.002) * (1.0 - smoothstep(0.0, 0.2, lumC)) * inten;

 c.r += 0.008 * smoothstep(0.1, 0.3, lumC) * (1.0 - smoothstep(0.3, 0.6, lumC)) * inten;

 c.g += 0.005 * smoothstep(0.4, 0.8, lumC) * inten;

 c = mix(vec3(lumC), c, 1.05);
 c = applyFx(uv, c);
  c = deathPost(uv, c);
  F = vec4(c, 1);
})";
