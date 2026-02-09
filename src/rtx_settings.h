#pragma once

// ============================================================================
// RTX SETTINGS - Quality presets and configuration
// Screen-space ray tracing: SSAO, SSR, Volumetric Light, PBR Materials
// ============================================================================

inline constexpr int RTX_MODE_OFF    = 0;
inline constexpr int RTX_MODE_LOW    = 1;
inline constexpr int RTX_MODE_MEDIUM = 2;
inline constexpr int RTX_MODE_HIGH   = 3;
inline constexpr int RTX_MODE_ULTRA  = 4;
inline constexpr int RTX_MODE_COUNT  = 5;

// SSAO quality
struct RtxSsaoParams {
    int   samples;       // kernel sample count
    float radius;        // sample radius in world units
    float bias;          // depth bias to avoid self-occlusion
    float intensity;     // occlusion strength multiplier
    int   blurPasses;    // bilateral blur passes
};

// SSR quality  
struct RtxSsrParams {
    int   maxSteps;      // ray march steps
    float stepSize;      // initial step size
    float maxDist;       // max ray travel distance
    float thickness;     // depth thickness for hit test
    float fadeEdge;      // screen-edge fade distance
    float roughnessCap;  // max roughness for reflections
};

// Volumetric light quality
struct RtxVolumetricParams {
    int   samples;       // ray march steps per pixel
    float density;       // fog/scatter density
    float scatterPower;  // anisotropic scatter exponent
    float intensity;     // light shaft brightness
    int   halfRes;       // render at half resolution
};

// Combined RTX config
struct RtxConfig {
    bool             enabled;
    int              mode;
    RtxSsaoParams    ssao;
    RtxSsrParams     ssr;
    RtxVolumetricParams volumetric;
    bool             pbrMaterials;
    bool             contactShadows;
    float            giBounceMul;    // indirect light bounce multiplier
};

inline int clampRtxMode(int mode) {
    if (mode < RTX_MODE_OFF) return RTX_MODE_OFF;
    if (mode >= RTX_MODE_COUNT) return RTX_MODE_COUNT - 1;
    return mode;
}

inline int stepRtxMode(int mode, int delta) {
    return clampRtxMode(mode + delta);
}

inline bool isRtxEnabled(int mode) {
    return clampRtxMode(mode) != RTX_MODE_OFF;
}

inline const char* rtxModeLabel(int mode) {
    switch (clampRtxMode(mode)) {
        case RTX_MODE_LOW:    return "LOW";
        case RTX_MODE_MEDIUM: return "MEDIUM";
        case RTX_MODE_HIGH:   return "HIGH";
        case RTX_MODE_ULTRA:  return "ULTRA";
        default:              return "OFF";
    }
}

inline RtxConfig rtxConfigForMode(int mode) {
    RtxConfig cfg = {};
    cfg.mode = clampRtxMode(mode);
    cfg.enabled = cfg.mode != RTX_MODE_OFF;
    
    if (!cfg.enabled) return cfg;
    
    switch (cfg.mode) {
        case RTX_MODE_LOW:
            cfg.ssao = {8, 0.5f, 0.05f, 0.4f, 1};
            cfg.ssr = {24, 0.15f, 8.0f, 0.3f, 0.15f, 0.4f};
            cfg.volumetric = {16, 0.04f, 4.0f, 0.6f, 1};
            cfg.pbrMaterials = true;
            cfg.contactShadows = false;
            cfg.giBounceMul = 0.15f;
            break;
        case RTX_MODE_MEDIUM:
            cfg.ssao = {16, 0.7f, 0.05f, 0.5f, 2};
            cfg.ssr = {48, 0.1f, 12.0f, 0.25f, 0.12f, 0.55f};
            cfg.volumetric = {24, 0.05f, 5.0f, 0.75f, 1};
            cfg.pbrMaterials = true;
            cfg.contactShadows = true;
            cfg.giBounceMul = 0.22f;
            break;
        case RTX_MODE_HIGH:
            cfg.ssao = {24, 0.9f, 0.05f, 0.6f, 2};
            cfg.ssr = {64, 0.06f, 25.0f, 0.6f, 0.08f, 0.85f};
            cfg.volumetric = {32, 0.10f, 0.9f, 1.3f, 0};
            cfg.pbrMaterials = true;
            cfg.contactShadows = true;
            cfg.giBounceMul = 0.4f;
            break;
        case RTX_MODE_ULTRA:
            cfg.ssao = {32, 1.0f, 0.05f, 0.7f, 3};
            cfg.ssr = {96, 0.04f, 40.0f, 0.9f, 0.05f, 0.95f};
            cfg.volumetric = {48, 0.14f, 1.2f, 1.8f, 0};
            cfg.pbrMaterials = true;
            cfg.contactShadows = true;
            cfg.giBounceMul = 0.55f;
            break;
    }
    return cfg;
}

// Estimate RTX VRAM overhead in MB (approximate)
inline int rtxVramEstimateMB(int mode, int width, int height) {
    if (!isRtxEnabled(mode)) return 0;
    // G-buffer: 3x RGBA16F + depth = ~24 bytes/pixel
    // SSAO: 1x R8 = 1 byte/pixel  
    // SSR: 1x RGBA16F = 8 bytes/pixel
    // Volumetric: 1x RGBA16F half-res = 2 bytes/pixel
    // Total: ~35 bytes/pixel
    long long pixels = (long long)width * height;
    long long bytes = pixels * 35;
    return (int)(bytes / (1024 * 1024)) + 4; // +4 for misc buffers
}
