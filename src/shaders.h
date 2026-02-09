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
uniform sampler2D tex; uniform vec3 vp; uniform float tm;
uniform int nl; uniform vec3 lp[16]; uniform float danger;
uniform int flashOn; uniform vec3 flashDir;
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
 
 vec3 tc = texture(tex, texCoord).rgb;
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
  vec3 toFrag = normalize(fp - vp);
  float spotAngle = dot(toFrag, flashDir);
  float dist = length(fp - vp);
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
out vec2 uv; uniform mat4 M,V,P;
void main(){ uv=t; gl_Position=P*V*M*vec4(p,1); })";

const char* lightFS=R"(#version 330
out vec4 F; in vec2 uv;
uniform sampler2D tex; uniform float inten,tm,fade;
float hash(float n){ return fract(sin(n)*43758.5453); }
void main(){
 vec3 c = texture(tex,uv).rgb * inten;
 float flick = 0.97 + sin(tm*25.0)*0.015 + hash(floor(tm*7.0))*0.015;
 // Apply fade for smooth light sprite transitions
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
uniform sampler2D depthTex;

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
float rtxSSAO(vec2 tc, int ns){
 float z0 = linZ(tc);
 float rad = 0.03 * (1.0 + 2.5 / max(z0, 0.5));
 float ao = 0.0;
 float rot = rnd(tc * 7.13 + vec2(tm * 0.37, tm * 0.61)) * 6.283;
 for(int i = 0; i < ns; i++){
  float a = float(i) * 6.283 / float(ns) + rot;
  float r = rad * (0.25 + 0.75 * rnd(tc * float(i+1) * 0.73 + vec2(tm * 0.53)));
  vec2 sc = tc + vec2(cos(a), sin(a)) * r;
  float diff = z0 - linZ(sc);
  if(diff > 0.02 && diff < 2.0) ao += (1.0 - diff / 2.0);
 }
 return clamp(1.0 - ao / float(ns) * 4.0, 0.0, 1.0);
}
vec3 rtxGI(vec2 tc, int ns){
 float z0 = linZ(tc);
 float rad = 0.04 * (1.0 + 2.0 / max(z0, 0.5));
 vec3 gi = vec3(0.0); float tw = 0.001;
 float rot = rnd(tc * 11.3 + vec2(tm * 0.41)) * 6.283;
 for(int i = 0; i < ns; i++){
  float a = float(i) * 6.283 / float(ns) + rot;
  float r = rad * (0.3 + 0.7 * rnd(tc * float(i+1) * 0.53 + vec2(tm * 0.29)));
  vec2 sc = tc + vec2(cos(a), sin(a)) * r;
  vec3 sC = texture(tex, sc).rgb;
  float depthDiff = abs(z0 - linZ(sc));
  float w = max(0.0, 1.0 - depthDiff * 3.0);
  float br = min(dot(sC, vec3(0.3, 0.59, 0.11)), 0.5);
  w *= br * 2.5;
  gi += sC * w; tw += w;
 }
 return gi / tw;
}
vec3 rtxRays(vec2 tc, int ns){
 vec3 rays = vec3(0.0);
 float rot = rnd(tc * 5.7 + vec2(tm * 0.17)) * 6.283;
 for(int i = 0; i < ns; i++){
  float a = float(i) * 6.283 / float(ns) + rot;
  float r = 0.015 + 0.07 * float(i) / float(ns);
  vec2 sc = clamp(tc + vec2(cos(a), sin(a)) * r, 0.001, 0.999);
  vec3 s = texture(tex, sc).rgb;
  float lum = dot(s, vec3(0.3, 0.59, 0.11));
  if(lum > 0.12){
   float dw = smoothstep(12.0, 1.0, linZ(sc));
   float decay = 1.0 - float(i) / float(ns);
   rays += s * (lum - 0.12) * decay * dw * 2.5;
  }
 }
 return rays / float(ns);
}
vec3 rtxBloom(vec2 tc, int rad){
 vec3 bl = vec3(0.0); float tw = 0.0;
 float sx = texelX * 2.5, sy = texelY * 2.5;
 for(int x = -rad; x <= rad; x++) for(int y = -rad; y <= rad; y++){
  vec3 s = texture(tex, tc + vec2(float(x)*sx, float(y)*sy)).rgb;
  float lum = dot(s, vec3(0.3, 0.59, 0.11));
  float w = smoothstep(0.15, 0.45, lum) * exp(-length(vec2(x, y)) * 0.35);
  bl += s * w; tw += 1.0;
 }
 return bl / max(tw, 1.0);
}

void main(){
 if(inten < 0.02) {
  vec3 c0 = resolveSample(uv);
  if(rtxA>0){ int n=rtxA<=1?6:(rtxA<=2?10:(rtxA<=3?14:20)); c0*=rtxSSAO(uv,n); }
  if(rtxG>0){ int n=rtxG<=1?6:(rtxG<=2?10:16); c0+=rtxGI(uv,n)*0.18; }
  if(rtxR>0){ c0+=rtxRays(uv,24)*0.45; }
  if(rtxB>0){ c0+=rtxBloom(uv,3)*0.45; }
  F = vec4(c0, 1.0);
 return;
}

// Chromatic aberration - asymmetric for realistic lens feel
float ab = 0.0018 * inten;
float abV = 0.0008 * inten; // slight vertical component
float r = resolveSample(uv + vec2(ab, abV * 0.5)).r;
float g = resolveSample(uv).g;
float b = resolveSample(uv - vec2(ab, abV * 0.3)).b;
vec3 c = vec3(r,g,b);
 if(frameGen == 1 && taaValid > 0.5){
  vec3 prev = texture(histTex, uv).rgb;
  c = mix(c, prev, clamp(frameGenBlend, 0.0, 0.6));
 }
 
 // Film grain - organic noise instead of sharp static
 float grainTime = tm * 0.7;
 float grain1 = rnd(uv * 0.8 + vec2(grainTime, grainTime * 0.73));
 float grain2 = rnd(uv * 1.3 + vec2(grainTime * 1.1, grainTime * 0.5));
 float filmGrain = (grain1 * 0.6 + grain2 * 0.4 - 0.5) * 0.028 * inten;
 // Film grain is stronger in darker areas (like real film)
 float pixelLum = dot(c, vec3(0.3, 0.6, 0.1));
 float grainStrength = mix(1.5, 0.5, smoothstep(0.0, 0.4, pixelLum));
 c += vec3(filmGrain) * grainStrength;
 
 // Subtle scanlines - less aggressive, more like CRT phosphor
 float scanline = sin(uv.y * 400.0 + tm * 3.0) * 0.5 + 0.5;
 scanline = pow(scanline, 3.0) * 0.012 * inten;
 c -= vec3(scanline);
 
 // Horizontal distortion glitch (very rare, subtle)
 if(inten > 0.45 && rnd(vec2(tm * 0.06, floor(uv.y * 40))) > 0.992) {
  float of = (rnd(vec2(tm, floor(uv.y * 40))) - 0.5) * 0.012 * inten;
  c = resolveSample(uv + vec2(of, 0));
 }
 
 // Vignette - natural camera lens falloff
 vec2 vigUV = uv - 0.5;
 float vigDist = dot(vigUV, vigUV); // squared distance for natural falloff
 float vig = 1.0 - vigDist * 0.8;
 vig = smoothstep(0.15, 1.0, vig);
 // Slightly warm the vignette edges
 c.r *= mix(vig, 1.0, 0.05);
 c.g *= vig;
 c.b *= mix(vig, 1.0, -0.03);
 
 // Subtle warm light leak from edge (like old camera)
 float leakAngle = atan(vigUV.y, vigUV.x);
 float leak = sin(leakAngle * 1.5 + tm * 0.05) * 0.5 + 0.5;
 float leakMask = smoothstep(0.35, 0.55, length(vigUV)) * leak * 0.015 * inten;
 c += vec3(leakMask * 1.2, leakMask * 0.9, leakMask * 0.4);
 
 // Ghost frame (very subtle double image with slight color shift)
 vec3 ghost = resolveSample(uv - vec2(0.003, 0.0015));
 c = mix(c, ghost * vec3(1.02, 0.98, 0.96), 0.025 * inten);
 
 // Cinematic color grading - warm midtones, cool shadows
 float lumC = dot(c, vec3(0.3, 0.6, 0.1));
 // Lift shadows slightly warm
 c += vec3(0.012, 0.008, 0.002) * (1.0 - smoothstep(0.0, 0.2, lumC)) * inten;
 // Warm midtones
 c.r += 0.008 * smoothstep(0.1, 0.3, lumC) * (1.0 - smoothstep(0.3, 0.6, lumC)) * inten;
 // Slight green in highlights (fluorescent light feel)
 c.g += 0.005 * smoothstep(0.4, 0.8, lumC) * inten;
 
 // Subtle contrast enhancement
 c = mix(vec3(lumC), c, 1.05);

 if(rtxA>0){ int n=rtxA<=1?6:(rtxA<=2?10:(rtxA<=3?14:20)); c*=rtxSSAO(uv,n); }
 if(rtxG>0){ int n=rtxG<=1?6:(rtxG<=2?10:16); c+=rtxGI(uv,n)*0.18; }
 if(rtxR>0){ c+=rtxRays(uv,24)*0.45; }
 if(rtxB>0){ c+=rtxBloom(uv,3)*0.45; }

 F = vec4(c, 1);
})";