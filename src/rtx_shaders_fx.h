#pragma once
// RTX SHADERS Part 2: SSR, Volumetric Light, Composition (OpenGL 3.3 core)

const char* rtxSsrFS = R"(#version 330
out vec4 fragRefl; in vec2 uv;
uniform sampler2D gPosition; uniform sampler2D gNormal; uniform sampler2D colorTex;
uniform mat4 proj; uniform mat4 view;
uniform int maxSteps; uniform float stepSize; uniform float maxDist;
uniform float thickness; uniform float fadeEdge; uniform vec3 viewPos;
void main() {
    vec4 posData = texture(gPosition, uv);
    vec3 fragPos = posData.xyz; float depth = posData.w;
    if (depth <= 0.0 || depth > 40.0) { fragRefl = vec4(0.0); return; }
    vec4 normData = texture(gNormal, uv);
    vec3 normal = normalize(normData.xyz); float roughness = normData.w;
    if (roughness > 0.7) { fragRefl = vec4(0.0); return; }
    vec3 viewDir = normalize(fragPos - viewPos);
    vec3 reflDir = reflect(viewDir, normal);
    float roughFade = 1.0 - smoothstep(0.3, 0.7, roughness);
    vec3 rayPos = fragPos; float traveled = 0.0;
    vec3 hitColor = vec3(0.0); float hitConf = 0.0; float currentStep = stepSize;
    for (int i = 0; i < 96; i++) {
        if (i >= maxSteps) break;
        rayPos += reflDir * currentStep; traveled += currentStep;
        if (traveled > maxDist) break;
        vec4 projPos = proj * view * vec4(rayPos, 1.0);
        projPos.xy /= projPos.w; vec2 screenUV = projPos.xy * 0.5 + 0.5;
        if (screenUV.x < 0.0 || screenUV.x > 1.0 || screenUV.y < 0.0 || screenUV.y > 1.0) break;
        float sampleDepth = texture(gPosition, screenUV).w;
        if (sampleDepth <= 0.0) { currentStep *= 1.05; continue; }
        float depthDiff = length(rayPos - viewPos) - sampleDepth;
        if (depthDiff > 0.0 && depthDiff < thickness) {
            hitColor = texture(colorTex, screenUV).rgb;
            vec2 edgeFade = smoothstep(vec2(0.0), vec2(fadeEdge), screenUV) *
                           (1.0 - smoothstep(vec2(1.0 - fadeEdge), vec2(1.0), screenUV));
            hitConf = edgeFade.x * edgeFade.y * (1.0 - traveled / maxDist) * roughFade;
            break;
        }
        currentStep *= 1.05;
    }
    fragRefl = vec4(hitColor, hitConf);
})";

const char* rtxVolFS = R"(#version 330
out vec4 fragScatter; in vec2 uv;
uniform sampler2D gPosition; uniform mat4 invViewProj;
uniform vec3 viewPos; uniform int lightCount; uniform vec3 lightPositions[16];
uniform int samples; uniform float density; uniform float scatterPower;
uniform float volIntensity; uniform float time;
float hash(vec3 p) { return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }
float phase(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}
void main() {
    float depth = texture(gPosition, uv).w;
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 worldFar = invViewProj * vec4(ndc, 1.0, 1.0);
    worldFar.xyz /= worldFar.w;
    vec3 rayDir = normalize(worldFar.xyz - viewPos);
    float maxDist = (depth > 0.0) ? min(depth, 30.0) : 30.0;
    float stepLen = maxDist / float(max(samples, 1));
    vec3 scatter = vec3(0.0); float transmittance = 1.0;
    float dither = hash(vec3(uv * 500.0, 0.0)) * stepLen;
    for (int s = 0; s < 48; s++) {
        if (s >= samples) break;
        float t = dither + float(s) * stepLen;
        vec3 samplePos = viewPos + rayDir * t;
        vec3 inScatter = vec3(0.0);
        for (int i = 0; i < lightCount && i < 16; i++) {
            vec3 toLight = lightPositions[i] - samplePos;
            float lightDist = length(toLight);
            vec3 lightDir = toLight / max(lightDist, 0.001);
            if (lightDist > 12.0) continue; // skip far lights to reduce wall bleed
            float att = 1.0 / (1.0 + 0.2 * lightDist + 0.08 * lightDist * lightDist);
            float ph = phase(dot(rayDir, lightDir), 0.3);
            float verticalFocus = smoothstep(0.0, 1.0, max(-lightDir.y, 0.0));
            float flick = 1.0;
            float distFade = 1.0 - smoothstep(6.0, 12.0, lightDist);
            inScatter += vec3(1.0, 0.92, 0.75) * att * ph * verticalFocus * flick * density * distFade;
        }
        transmittance *= exp(-density * 0.3 * stepLen);
        scatter += inScatter * transmittance * stepLen;
    }
    fragScatter = vec4(scatter * volIntensity, 1.0 - transmittance);
})";

const char* rtxCompVS = R"(#version 330
layout(location=0) in vec2 p; layout(location=1) in vec2 t;
out vec2 uv; void main() { uv = t; gl_Position = vec4(p, 0.0, 1.0); })";

const char* rtxCompFS = R"(#version 330
out vec4 fragColor; in vec2 uv;
uniform sampler2D gPosition; uniform sampler2D gNormal; uniform sampler2D gAlbedo;
uniform sampler2D ssaoTex; uniform sampler2D ssrTex; uniform sampler2D volTex;
uniform sampler2D forwardTex;
uniform vec3 viewPos; uniform float time; uniform float danger;
uniform int lightCount; uniform vec3 lightPositions[16];
uniform int flashOn; uniform vec3 flashDir;
uniform int remoteFc; uniform vec3 remoteFp[4]; uniform vec3 remoteFd[4];
uniform float giBounceMul; uniform int contactShadows;
void main() {
    vec4 posData = texture(gPosition, uv);
    float depth = posData.w;
    vec3 forward = texture(forwardTex, uv).rgb;
    if (depth <= 0.15) { fragColor = vec4(forward, 1.0); return; }
    vec4 reflData = texture(ssrTex, uv);
    vec3 volScatter = texture(volTex, uv).rgb;
    vec3 result = forward;
    if (reflData.a > 0.05) {
        vec4 normData = texture(gNormal, uv);
        float roughness = normData.w;
        float reflStr = (1.0 - roughness) * reflData.a * 0.2;
        bool isFloor = normData.y > 0.5;
        if (isFloor) reflStr *= 1.3;
        reflStr = min(reflStr, 0.35);
        result = mix(result, reflData.rgb, reflStr);
    }
    result += volScatter;
    fragColor = vec4(clamp(result, 0.0, 1.0), 1.0);
})";
