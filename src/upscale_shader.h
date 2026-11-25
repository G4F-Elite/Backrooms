#pragma once

inline constexpr char upscaleResolveFS[] = R"GLSL(#version 330
in vec2 uv;
out vec4 F;
uniform sampler2D tex, histTex, depthTex;
uniform int upscaler, aaMode;
uniform float sharpness, texelX, texelY, taaBlend, taaValid;
uniform vec3 sampleJitter, historyJitter; // projected image displacement in UV units
)GLSL"
#include "upscale_shader.inc"
R"GLSL(
void main() { F = vec4(resolveSample(uv), 1.0); }
)GLSL";
