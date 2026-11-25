// EASU adapted from GPUOpen FidelityFX-FSR v1.20210629 for GLSL 330 texture fetches.
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
R"GLSL(
uniform mat4 previousFromCurrentClip;
float upscaleLuma(vec3 rgb) { return dot(rgb, vec3(0.5, 1.0, 0.5)); }

// EASU reconstructs from original texel centres. These coordinates also work
// with odd resolutions, arbitrary render scale and a one-texel source.
const ivec2 easuOffsets[12] = ivec2[12](
    ivec2(0,-1), ivec2(1,-1),
    ivec2(-1,0), ivec2(0,0), ivec2(1,0), ivec2(2,0),
    ivec2(-1,1), ivec2(0,1), ivec2(1,1), ivec2(2,1),
    ivec2(0,2), ivec2(1,2));

vec3 sourceTexel(ivec2 at, ivec2 size) {
    return texelFetch(tex, clamp(at, ivec2(0), size - 1), 0).rgb;
}

void easuDirection(inout vec2 dir, inout float strength, float weight,
                   float n, float w, float c, float e, float s) {
    vec2 slope = vec2(e - w, s - n);
    vec2 longest = max(abs(vec2(e - c, s - c)), abs(vec2(c - w, c - n)));
    vec2 continuity = clamp(abs(slope) / max(longest, vec2(1e-6)), 0.0, 1.0);
    dir += weight * slope;
    strength += weight * dot(continuity, continuity);
}

vec3 easuSample(vec2 tc) {
    ivec2 size = textureSize(tex, 0);
    vec2 pos = tc * vec2(size) - 0.5;
    ivec2 origin = ivec2(floor(pos));
    vec2 f = fract(pos);
    vec3 colors[12];
    colors[3] = sourceTexel(origin, size);
    colors[4] = sourceTexel(origin + ivec2(1,0), size);
    colors[7] = sourceTexel(origin + ivec2(0,1), size);
    colors[8] = sourceTexel(origin + ivec2(1,1), size);
    vec3 lo = min(min(colors[3], colors[4]), min(colors[7], colors[8]));
    vec3 hi = max(max(colors[3], colors[4]), max(colors[7], colors[8]));
    if (all(equal(lo, hi))) return lo;

    float luma[12];
    for (int i = 0; i < 12; ++i) {
        if (i != 3 && i != 4 && i != 7 && i != 8)
            colors[i] = sourceTexel(origin + easuOffsets[i], size);
        luma[i] = upscaleLuma(colors[i]);
    }
    vec2 dir = vec2(0.0);
    float strength = 0.0;
    vec4 weights = vec4(1.0-f.x, f.x, 1.0-f.x, f.x) * vec4(1.0-f.y, 1.0-f.y, f.y, f.y);
    easuDirection(dir, strength, weights.x, luma[0], luma[2], luma[3], luma[4], luma[7]);
    easuDirection(dir, strength, weights.y, luma[1], luma[3], luma[4], luma[5], luma[8]);
    easuDirection(dir, strength, weights.z, luma[3], luma[6], luma[7], luma[8], luma[10]);
    easuDirection(dir, strength, weights.w, luma[4], luma[7], luma[8], luma[9], luma[11]);
    float mag = dot(dir, dir);
    dir = mag < 1.0/32768.0 ? vec2(1.0,0.0) : dir * inversesqrt(mag);
    strength *= 0.5;
    strength *= strength;
    vec2 stretch = mix(vec2(1), vec2(1.0/max(abs(dir.x), abs(dir.y)), 0.5), strength);
    float lobe = mix(0.5, 0.21, strength);
    vec3 sum = vec3(0);
    float wsum = 0.0;
    for (int i = 0; i < 12; ++i) {
        vec2 offset = vec2(easuOffsets[i]) - f;
        vec2 axes = vec2(dot(offset, dir), dot(offset, vec2(-dir.y, dir.x))) * stretch;
        float d = min(dot(axes, axes), 1.0/lobe);
        float window = 0.4*d-1.0, sinc = lobe*d-1.0;
        float weight = (1.5625*window*window - 0.5625) * sinc*sinc;
        sum += colors[i] * weight;
        wsum += weight;
    }
    vec3 result = sum / wsum;
    // Edge-limited sharpening reuses the same neighbourhood; no extra reads.
    vec3 surround = weights.x*(colors[0]+colors[2]+colors[4]+colors[7])
                  + weights.y*(colors[1]+colors[3]+colors[5]+colors[8])
                  + weights.z*(colors[3]+colors[6]+colors[8]+colors[10])
                  + weights.w*(colors[4]+colors[7]+colors[9]+colors[11]);
    return clamp(result + (result - 0.25 * surround) * sharpness * 0.5, lo, hi);
}

vec3 sourceSample(vec2 tc){
    if (upscaler == 1 || upscaler == 2) return easuSample(tc);
    if (upscaler == 3) {
        ivec2 size = textureSize(tex, 0);
        return sourceTexel(ivec2(floor(tc * vec2(size))), size);
    }
    return texture(tex, tc).rgb;
}

vec3 fxaaResolve(vec2 tc, vec3 base) {
    vec2 stepUV = vec2(texelX, texelY);
    vec3 a = texture(tex, tc - stepUV).rgb;
    vec3 b = texture(tex, tc + vec2(stepUV.x,-stepUV.y)).rgb;
    vec3 c = texture(tex, tc + vec2(-stepUV.x,stepUV.y)).rgb;
    vec3 d = texture(tex, tc + stepUV).rgb;
    vec4 y = vec4(upscaleLuma(a), upscaleLuma(b), upscaleLuma(c), upscaleLuma(d));
    float edge = max(max(y.x,y.y),max(y.z,y.w)) - min(min(y.x,y.y),min(y.z,y.w));
    return mix(base, (a+b+c+d+base)*0.2, smoothstep(0.08,0.40,edge)*0.55);
}

vec3 temporalResolve(vec2 tc, vec3 cur) {
    if (taaValid < 0.5) return cur;
    float depth = texture(depthTex, tc).r;
    vec4 projected = previousFromCurrentClip * vec4(tc*2.0-1.0, depth*2.0-1.0, 1.0);
    if (projected.w <= 0.0 || abs(projected.z) > projected.w) return cur;
    // Previous VP includes its own sample phase; stored history has already
    // been resolved onto the unjittered display grid.
    vec2 prevUV = projected.xy / projected.w * 0.5 + 0.5 - historyJitter.xy;
    if (any(lessThan(prevUV, vec2(0))) || any(greaterThan(prevUV, vec2(1)))) return cur;
    vec3 hist = texture(histTex, prevUV).rgb;

    ivec2 size = textureSize(tex,0), origin = ivec2(floor(tc*vec2(size)-0.5));
    vec3 a = sourceTexel(origin, size), b = sourceTexel(origin+ivec2(1,0), size);
    vec3 c = sourceTexel(origin+ivec2(0,1), size), d = sourceTexel(origin+1, size);
    vec3 lo = min(cur, min(min(a,b), min(c,d)));
    vec3 hi = max(cur, max(max(a,b), max(c,d)));
    vec3 bounded = clamp(hist, lo, hi);
    // Camera reprojection does not track independently moving objects. Reject
    // history inconsistent with this frame rather than dragging it behind them.
    float delta = max(max(abs(hist.r-cur.r),abs(hist.g-cur.g)),abs(hist.b-cur.b));
    float trust = 1.0-smoothstep(0.04,0.22,delta);
    return mix(cur, bounded, clamp(taaBlend,0.0,0.90) * trust);
}

vec3 resolveSample(vec2 tc) {
    // The output pixel is fixed. Geometry and its matching depth moved by the
    // projection's positive jitter when rasterized into the source texture.
    tc += sampleJitter.xy;
    vec3 current = sourceSample(tc);
    if (aaMode == 1) current = fxaaResolve(tc, current);
    if (upscaler == 2 || aaMode == 2) current = temporalResolve(tc, current);
    return current;
}
)GLSL"
