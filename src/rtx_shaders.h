#pragma once
// RTX SHADERS Part 1: G-Buffer, SSAO, SSAO Blur (OpenGL 3.3 core)

const char* rtxGbufVS = R"(#version 330
layout(location=0) in vec3 p;
layout(location=1) in vec2 t;
layout(location=2) in vec3 n;
out vec2 uv; out vec3 fragPos; out vec3 fragNorm; out vec3 viewTS;
uniform mat4 M, V, P; uniform vec3 vp;
void main() {
    fragPos = vec3(M * vec4(p, 1.0));
    fragNorm = mat3(transpose(inverse(M))) * n;
    uv = t;
    vec3 N = normalize(fragNorm);
    vec3 T = normalize(cross(N, vec3(0.0, 1.0, 0.1)));
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));
    viewTS = TBN * normalize(vp - fragPos);
    gl_Position = P * V * M * vec4(p, 1.0);
})";

const char* rtxGbufFS = R"(#version 330
layout(location=0) out vec4 gPosition;
layout(location=1) out vec4 gNormal;
layout(location=2) out vec4 gAlbedo;
in vec2 uv; in vec3 fragPos; in vec3 fragNorm; in vec3 viewTS;
uniform sampler2D tex; uniform sampler2D normalMap; uniform sampler2D materialMap;
uniform vec3 vp; uniform int surfaceType;
vec2 parallaxOffset(vec2 tc, vec3 vDir) {
    float height = texture(tex, tc).a;
    float scale = 0.025;
    float dist = length(vp - fragPos);
    float lod = 1.0 - smoothstep(8.0, 15.0, dist);
    if (lod < 0.01) return tc;
    return tc + vDir.xy * ((height - 0.5) * scale * lod);
}
void main() {
    vec3 V = normalize(viewTS);
    vec2 tc = parallaxOffset(uv, V);
    vec3 albedo = texture(tex, tc).rgb;
    vec3 N = normalize(fragNorm);
    vec3 nmapNormal = texture(normalMap, tc).rgb * 2.0 - 1.0;
    vec3 T = normalize(cross(N, vec3(0.0, 1.0, 0.1)));
    vec3 B = cross(N, T);
    vec3 perturbedN = normalize(mat3(T, B, N) * nmapNormal);
    vec2 matSample = texture(materialMap, tc).rg;
    float roughness = matSample.r; float metallic = matSample.g;
    if (surfaceType == 1) { roughness *= 0.85; metallic = max(metallic, 0.02); }
    else if (surfaceType == 2) { roughness = max(roughness, 0.8); }
    float depth = length(fragPos - vp);
    gPosition = vec4(fragPos, depth);
    gNormal = vec4(perturbedN, roughness);
    gAlbedo = vec4(albedo, metallic);
})";

// SSAO fullscreen quad vertex shader (shared by all post-process passes)
const char* rtxSsaoVS = R"(#version 330
layout(location=0) in vec2 p; layout(location=1) in vec2 t;
out vec2 uv; void main() { uv = t; gl_Position = vec4(p, 0.0, 1.0); })";

const char* rtxSsaoFS = R"(#version 330
out float fragAO; in vec2 uv;
uniform sampler2D gPosition; uniform sampler2D gNormal; uniform sampler2D noiseTex;
uniform mat4 proj; uniform mat4 view;
uniform int sampleCount; uniform float radius; uniform float bias;
uniform float intensity; uniform vec3 noiseScale;
uniform vec3 kernel[32]; uniform vec3 viewPos;
void main() {
    vec4 posData = texture(gPosition, uv);
    vec3 fragPos = posData.xyz; float depth = posData.w;
    if (depth <= 0.0 || depth > 50.0) { fragAO = 1.0; return; }
    vec3 normal = normalize(texture(gNormal, uv).xyz);
    vec3 randomVec = normalize(texture(noiseTex, uv * noiseScale.xy).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    float occlusion = 0.0; int count = min(sampleCount, 32);
    for (int i = 0; i < count; i++) {
        vec3 samplePos = fragPos + TBN * kernel[i] * radius;
        vec4 offset = proj * view * vec4(samplePos, 1.0);
        offset.xy /= offset.w; offset.xy = offset.xy * 0.5 + 0.5;
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) continue;
        float sampleDepth = texture(gPosition, offset.xy).w;
        if (sampleDepth <= 0.0) continue;
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(depth - sampleDepth), 0.001));
        if (sampleDepth < length(samplePos - viewPos) - bias) occlusion += rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(count)) * intensity;
    fragAO = clamp(occlusion, 0.0, 1.0);
})";

const char* rtxSsaoBlurFS = R"(#version 330
out float fragAO; in vec2 uv;
uniform sampler2D aoTex; uniform sampler2D gPosition;
uniform vec3 texelSize; uniform int horizontal;
void main() {
    float centerDepth = texture(gPosition, uv).w;
    if (centerDepth <= 0.0) { fragAO = texture(aoTex, uv).r; return; }
    float result = 0.0; float totalWeight = 0.0;
    for (int i = -2; i <= 2; i++) {
        vec2 offset = (horizontal == 1) ? vec2(float(i) * texelSize.x, 0.0) : vec2(0.0, float(i) * texelSize.y); // texelSize.z unused
        float sampleAO = texture(aoTex, uv + offset).r;
        float sampleDepth = texture(gPosition, uv + offset).w;
        float weight = exp(-abs(centerDepth - sampleDepth) * 8.0) * exp(-float(i * i) / 4.0);
        result += sampleAO * weight; totalWeight += weight;
    }
    fragAO = result / max(totalWeight, 0.001);
})";

#include "rtx_shaders_fx.h"
