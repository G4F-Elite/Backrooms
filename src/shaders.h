#pragma once

const char* mainVS=R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t; layout(location=2) in vec3 n;
out vec2 uv; out vec3 fp,nm;
uniform mat4 M,V,P;
void main(){ fp=vec3(M*vec4(p,1)); nm=mat3(transpose(inverse(M)))*n; uv=t; gl_Position=P*V*M*vec4(p,1); })";

const char* mainFS=R"(#version 330
out vec4 F; in vec2 uv; in vec3 fp,nm;
uniform sampler2D tex; uniform vec3 vp; uniform float tm;
uniform int nl; uniform vec3 lp[16]; uniform float danger;
uniform int flashOn; uniform vec3 flashDir;
uniform int rfc; uniform vec3 rfp[4]; uniform vec3 rfd[4];

void main(){
 vec3 tc = texture(tex,uv).rgb;
 vec3 N = normalize(nm);
 bool isCeil = N.y < -0.5;
 bool isFloor = N.y > 0.5;
 if(isCeil) tc *= vec3(1.02, 1.0, 0.95);
 float ambVal = 0.06 * (1.0 - danger * 0.3);
 vec3 res = vec3(ambVal) * tc;
 
 for(int i = 0; i < nl && i < 16; i++) {
  vec3 toLight = lp[i] - fp;
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
  float baseFlick = sin(tm*20.0 + float(i)*2.1) * 0.015;
  float panicFlick = sin(tm*30.0 + float(i)*3.7) * 0.05 * danger;
  float fl = 1.0 + baseFlick + panicFlick;
  vec3 lightColor = vec3(1.0, 0.92, 0.75);
  res += df * lightColor * fl * tc * att * 0.5;
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
 
 // FOG - moderate density to hide distant LOD
 float dist = length(vp - fp);
 float fog = clamp(1.0 - dist * 0.038, 0.0, 1.0);
 vec3 fogColor = vec3(0.06, 0.05, 0.04);
 res = mix(fogColor, res, fog);
 
 res *= vec3(1.0, 0.97, 0.9);
 
 // Danger red tint in environment
 if(danger > 0.3) {
  res.r += danger * 0.08;
  res.gb *= (1.0 - danger * 0.15);
 }
 
 if(danger > 0.0) {
  float gray = dot(res, vec3(0.3, 0.6, 0.1));
  res = mix(res, vec3(gray), danger * 0.2);
 }
 
 F = vec4(clamp(res, 0.0, 1.0), 1.0);
})";

const char* lightVS=R"(#version 330
layout(location=0) in vec3 p; layout(location=1) in vec2 t;
out vec2 uv; uniform mat4 M,V,P;
void main(){ uv=t; gl_Position=P*V*M*vec4(p,1); })";

const char* lightFS=R"(#version 330
out vec4 F; in vec2 uv;
uniform sampler2D tex; uniform float inten,tm;
float hash(float n){ return fract(sin(n)*43758.5453); }
void main(){
 vec3 c = texture(tex,uv).rgb * inten;
 float flick = 0.97 + sin(tm*25.0)*0.015 + hash(floor(tm*7.0))*0.015;
 F = vec4(c * flick, 1);
})";

const char* vhsVS=R"(#version 330
layout(location=0) in vec2 p; layout(location=1) in vec2 t;
out vec2 uv; void main(){ uv=t; gl_Position=vec4(p,0,1); })";

const char* vhsFS=R"(#version 330
out vec4 F; in vec2 uv;
uniform sampler2D tex; uniform float tm,inten;
uniform int upscaler;
uniform float sharpness;
uniform float texelX;
uniform float texelY;

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
 vec3 rcas = c - lap * (0.22 + sharpness * 0.58) * adaptive;
 return clamp(rcas, 0.0, 1.0);
}

vec3 upscaledSample(vec2 tc){
 if(upscaler == 1) return fsr1Sample(tc);
 return texture(tex, tc).rgb;
}

void main(){
 if(inten < 0.2) {
  F = vec4(upscaledSample(uv), 1.0);
  return;
 }
 
 // Chromatic aberration
 float ab = 0.0015 * inten;
 float r = upscaledSample(uv + vec2(ab,0)).r;
 float g = upscaledSample(uv).g;
 float b = upscaledSample(uv - vec2(ab,0)).b;
 vec3 c = vec3(r,g,b);
 
 // Scanlines
 c -= sin(uv.y * 600.0 + tm * 6.0) * 0.016 * inten;
 
 // Light static noise
 c += (rnd(uv + vec2(tm,0)) - 0.5) * 0.035 * inten;
 
 // Horizontal distortion glitch (rare)
 if(inten > 0.45 && rnd(vec2(tm * 0.08, floor(uv.y * 60))) > 0.985) {
  float of = (rnd(vec2(tm, floor(uv.y * 60))) - 0.5) * 0.018 * inten;
  c = upscaledSample(uv + vec2(of, 0));
 }
 
 // Vignette - stronger to hide screen edges
 float vig = 1.0 - length(uv - 0.5) * 0.45;
 vig = smoothstep(0.3, 1.0, vig);
 c *= vig;
 
 // Subtle dust particles overlay
 float dust = noise(uv * 200.0 + vec2(tm * 3.0, tm * 2.0));
 dust = smoothstep(0.75, 0.95, dust) * 0.08 * inten;
 c += dust;
 
 // Subtle light leak / haze at center
 float haze = 1.0 - length(uv - 0.5) * 1.5;
 haze = max(haze, 0.0) * 0.03 * inten;
 c += vec3(haze * 1.1, haze, haze * 0.9);
 
 // Ghost frame (very subtle double image)
 c = mix(c, upscaledSample(uv - vec2(0.003, 0.001)), 0.03 * inten);
 
 // Warm tint
 c.g += 0.01 * inten;
 
 F = vec4(c, 1);
})";

