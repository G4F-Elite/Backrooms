#pragma once

const char* mainVS=R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t; layout(location=2) in vec3 n;
out vec2 uv; out vec3 fp,nm; out vec3 viewTS; out vec3 fragPosTS;
uniform mat4 M,V,P;
uniform vec3 vp;

void main(){
 fp=vec3(M*vec4(p,1));
 nm=mat3(transpose(inverse(M)))*n;
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
  float d = sqrt(d2);
  vec3 L = toLight / max(d, 0.001);
  float att = 1.0 / (1.0 + 0.1*d + 0.04*d*d);
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
  float fl = 1.0;
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
 
 // THICK FOG - exponential falloff to hide distant LOD transitions
 float dist = length(vp - fp);
 
 // Exponential fog - much thicker, starts closer
 float fogDensity = 0.065;
 float fog = exp(-dist * fogDensity);
 fog = clamp(fog, 0.0, 1.0);
 
 // Soft noise without block artifacts
 float fogNoise = hash(fp * 0.17);
 fog = fog * (0.99 + fogNoise * 0.01);
 
 // Darker, warmer fog color
 vec3 fogColor = vec3(0.045, 0.04, 0.035);
 
 // Distance-based color desaturation (far objects lose color)
 float desat = smoothstep(8.0, 20.0, dist);
 float gray = dot(res, vec3(0.3, 0.6, 0.1));
 res = mix(res, vec3(gray), desat * 0.4);
 
 // Distance-based darkening before fog (smooth LOD transition)
 float distDarken = smoothstep(15.0, 25.0, dist);
 res *= (1.0 - distDarken * 0.5);
 
 // Apply fog
 res = mix(fogColor, res, fog);
 
 res *= vec3(1.0, 0.97, 0.9);
 
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

void main(){
 if(inten < 0.02) {
  F = vec4(resolveSample(uv), 1.0);
 return;
}

// Chromatic aberration
float ab = 0.0015 * inten;
float r = resolveSample(uv + vec2(ab,0)).r;
float g = resolveSample(uv).g;
float b = resolveSample(uv - vec2(ab,0)).b;
vec3 c = vec3(r,g,b);
 if(frameGen == 1 && taaValid > 0.5){
  vec3 prev = texture(histTex, uv).rgb;
  c = mix(c, prev, clamp(frameGenBlend, 0.0, 0.6));
 }
 
 // Scanlines
 c -= sin(uv.y * 600.0 + tm * 6.0) * 0.016 * inten;
 
 // Light static noise
 c += (rnd(uv + vec2(tm,0)) - 0.5) * 0.035 * inten;
 
 // Horizontal distortion glitch (rare)
 if(inten > 0.45 && rnd(vec2(tm * 0.08, floor(uv.y * 60))) > 0.985) {
  float of = (rnd(vec2(tm, floor(uv.y * 60))) - 0.5) * 0.018 * inten;
  c = resolveSample(uv + vec2(of, 0));
 }
 
 // Vignette - stronger to hide screen edges
 float vig = 1.0 - length(uv - 0.5) * 0.45;
 vig = smoothstep(0.3, 1.0, vig);
 c *= vig;
 
 // Subtle light leak / haze at center
 float haze = 1.0 - length(uv - 0.5) * 1.5;
 haze = max(haze, 0.0) * 0.03 * inten;
 c += vec3(haze * 1.1, haze, haze * 0.9);
 
 // Ghost frame (very subtle double image)
 c = mix(c, resolveSample(uv - vec2(0.003, 0.001)), 0.03 * inten);
 
 // Warm tint
 c.g += 0.01 * inten;
 
 F = vec4(c, 1);
})";