#pragma once
#include <glad/glad.h>
#include "rtx_settings.h"

// GL constants missing from minimal glad headers
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_R16F
#define GL_R16F 0x822D
#endif
#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 0x8CE1
#endif
#ifndef GL_COLOR_ATTACHMENT2
#define GL_COLOR_ATTACHMENT2 0x8CE2
#endif

// ============================================================================
// RTX PIPELINE - G-Buffer, SSAO, SSR, Volumetric FBO management
// Deferred rendering infrastructure for screen-space ray tracing
// ============================================================================

struct RtxGBuffer {
    GLuint fbo;
    GLuint posTex;       // RGBA16F - world-space position + depth
    GLuint normTex;      // RGBA16F - world-space normal + roughness
    GLuint albedoTex;    // RGBA8   - albedo + metallic
    GLuint depthRbo;     // depth renderbuffer
    int width, height;
};

struct RtxSsaoBuffer {
    GLuint fbo;
    GLuint aoTex;        // R8 - ambient occlusion
    GLuint blurFbo;
    GLuint blurTex;      // R8 - blurred AO
    GLuint noiseTex;     // RGBA16F - random rotation vectors
    int width, height;
};

struct RtxSsrBuffer {
    GLuint fbo;
    GLuint reflTex;      // RGBA16F - reflection color + confidence
    int width, height;
};

struct RtxVolumetricBuffer {
    GLuint fbo;
    GLuint scatterTex;   // RGBA16F - scattered light
    int width, height;
};

struct RtxCompositeBuffer {
    GLuint fbo;
    GLuint resultTex;    // RGBA8 - final composited result
    int width, height;
};

struct RtxPipeline {
    RtxGBuffer        gbuf;
    RtxSsaoBuffer     ssao;
    RtxSsrBuffer      ssr;
    RtxVolumetricBuffer vol;
    RtxCompositeBuffer comp;
    RtxConfig         config;
    bool              initialized;
    
    // Shaders
    GLuint gbufShader;
    GLuint ssaoShader;
    GLuint ssaoBlurShader;
    GLuint ssrShader;
    GLuint volShader;
    GLuint compShader;
};

// ============================================================================
// G-Buffer initialization
// ============================================================================

inline void initRtxGBuffer(RtxGBuffer& gb, int w, int h) {
    gb.width = w;
    gb.height = h;
    
    glGenFramebuffers(1, &gb.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gb.fbo);
    
    // Position buffer (RGBA16F) - xyz=world pos, w=linear depth
    glGenTextures(1, &gb.posTex);
    glBindTexture(GL_TEXTURE_2D, gb.posTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gb.posTex, 0);
    
    // Normal buffer (RGBA16F) - xyz=world normal, w=roughness
    glGenTextures(1, &gb.normTex);
    glBindTexture(GL_TEXTURE_2D, gb.normTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gb.normTex, 0);
    
    // Albedo buffer (RGBA8) - rgb=albedo, a=metallic
    glGenTextures(1, &gb.albedoTex);
    glBindTexture(GL_TEXTURE_2D, gb.albedoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gb.albedoTex, 0);
    
    // Depth renderbuffer
    glGenRenderbuffers(1, &gb.depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, gb.depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gb.depthRbo);
    
    // Tell OpenGL which color attachments to use
    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    typedef void (APIENTRY *PFNGLDRAWBUFFERSPROC)(GLsizei n, const GLenum *bufs);
    static PFNGLDRAWBUFFERSPROC _glDrawBuffers = NULL;
    if (!_glDrawBuffers) _glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
    if (_glDrawBuffers) _glDrawBuffers(3, attachments);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void destroyRtxGBuffer(RtxGBuffer& gb) {
    if (gb.fbo) { glDeleteFramebuffers(1, &gb.fbo); gb.fbo = 0; }
    if (gb.posTex) { glDeleteTextures(1, &gb.posTex); gb.posTex = 0; }
    if (gb.normTex) { glDeleteTextures(1, &gb.normTex); gb.normTex = 0; }
    if (gb.albedoTex) { glDeleteTextures(1, &gb.albedoTex); gb.albedoTex = 0; }
    if (gb.depthRbo) { glDeleteRenderbuffers(1, &gb.depthRbo); gb.depthRbo = 0; }
}

// ============================================================================
// SSAO Buffer
// ============================================================================

inline void generateSsaoNoiseTex(GLuint& noiseTex) {
    // 4x4 random rotation vectors for SSAO kernel rotation
    float noise[16 * 3];
    unsigned int seed = 73856093;
    for (int i = 0; i < 16; i++) {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        float a = (float)(seed % 10000) / 10000.0f;
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        float b = (float)(seed % 10000) / 10000.0f;
        noise[i * 3 + 0] = a * 2.0f - 1.0f;
        noise[i * 3 + 1] = b * 2.0f - 1.0f;
        noise[i * 3 + 2] = 0.0f;
    }
    
    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

inline void initRtxSsaoBuffer(RtxSsaoBuffer& sb, int w, int h) {
    sb.width = w;
    sb.height = h;
    
    // Main SSAO FBO
    glGenFramebuffers(1, &sb.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sb.fbo);
    
    glGenTextures(1, &sb.aoTex);
    glBindTexture(GL_TEXTURE_2D, sb.aoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sb.aoTex, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Blur FBO
    glGenFramebuffers(1, &sb.blurFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sb.blurFbo);
    
    glGenTextures(1, &sb.blurTex);
    glBindTexture(GL_TEXTURE_2D, sb.blurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sb.blurTex, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Noise texture
    generateSsaoNoiseTex(sb.noiseTex);
}

inline void destroyRtxSsaoBuffer(RtxSsaoBuffer& sb) {
    if (sb.fbo) { glDeleteFramebuffers(1, &sb.fbo); sb.fbo = 0; }
    if (sb.aoTex) { glDeleteTextures(1, &sb.aoTex); sb.aoTex = 0; }
    if (sb.blurFbo) { glDeleteFramebuffers(1, &sb.blurFbo); sb.blurFbo = 0; }
    if (sb.blurTex) { glDeleteTextures(1, &sb.blurTex); sb.blurTex = 0; }
    if (sb.noiseTex) { glDeleteTextures(1, &sb.noiseTex); sb.noiseTex = 0; }
}

// ============================================================================
// SSR Buffer
// ============================================================================

inline void initRtxSsrBuffer(RtxSsrBuffer& sr, int w, int h) {
    sr.width = w;
    sr.height = h;
    
    glGenFramebuffers(1, &sr.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sr.fbo);
    
    glGenTextures(1, &sr.reflTex);
    glBindTexture(GL_TEXTURE_2D, sr.reflTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sr.reflTex, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void destroyRtxSsrBuffer(RtxSsrBuffer& sr) {
    if (sr.fbo) { glDeleteFramebuffers(1, &sr.fbo); sr.fbo = 0; }
    if (sr.reflTex) { glDeleteTextures(1, &sr.reflTex); sr.reflTex = 0; }
}

// ============================================================================
// Volumetric Light Buffer
// ============================================================================

inline void initRtxVolumetricBuffer(RtxVolumetricBuffer& vb, int w, int h, bool halfRes) {
    int rw = halfRes ? w / 2 : w;
    int rh = halfRes ? h / 2 : h;
    if (rw < 64) rw = 64;
    if (rh < 64) rh = 64;
    vb.width = rw;
    vb.height = rh;
    
    glGenFramebuffers(1, &vb.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, vb.fbo);
    
    glGenTextures(1, &vb.scatterTex);
    glBindTexture(GL_TEXTURE_2D, vb.scatterTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, rw, rh, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vb.scatterTex, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void destroyRtxVolumetricBuffer(RtxVolumetricBuffer& vb) {
    if (vb.fbo) { glDeleteFramebuffers(1, &vb.fbo); vb.fbo = 0; }
    if (vb.scatterTex) { glDeleteTextures(1, &vb.scatterTex); vb.scatterTex = 0; }
}

// ============================================================================
// Composite Buffer
// ============================================================================

inline void initRtxCompositeBuffer(RtxCompositeBuffer& cb, int w, int h) {
    cb.width = w;
    cb.height = h;
    
    glGenFramebuffers(1, &cb.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, cb.fbo);
    
    glGenTextures(1, &cb.resultTex);
    glBindTexture(GL_TEXTURE_2D, cb.resultTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cb.resultTex, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void destroyRtxCompositeBuffer(RtxCompositeBuffer& cb) {
    if (cb.fbo) { glDeleteFramebuffers(1, &cb.fbo); cb.fbo = 0; }
    if (cb.resultTex) { glDeleteTextures(1, &cb.resultTex); cb.resultTex = 0; }
}

// ============================================================================
// Full pipeline init/destroy
// ============================================================================

inline void initRtxPipeline(RtxPipeline& rtx, int w, int h, int mode) {
    rtx.config = rtxConfigForMode(mode);
    if (!rtx.config.enabled) {
        rtx.initialized = false;
        return;
    }
    
    initRtxGBuffer(rtx.gbuf, w, h);
    initRtxSsaoBuffer(rtx.ssao, w, h);
    initRtxSsrBuffer(rtx.ssr, w, h);
    initRtxVolumetricBuffer(rtx.vol, w, h, rtx.config.volumetric.halfRes != 0);
    initRtxCompositeBuffer(rtx.comp, w, h);
    
    rtx.initialized = true;
}

inline void destroyRtxPipeline(RtxPipeline& rtx) {
    if (!rtx.initialized) return;
    destroyRtxGBuffer(rtx.gbuf);
    destroyRtxSsaoBuffer(rtx.ssao);
    destroyRtxSsrBuffer(rtx.ssr);
    destroyRtxVolumetricBuffer(rtx.vol);
    destroyRtxCompositeBuffer(rtx.comp);
    rtx.initialized = false;
}

inline void resizeRtxPipeline(RtxPipeline& rtx, int w, int h) {
    if (!rtx.initialized) return;
    int mode = rtx.config.mode;
    destroyRtxPipeline(rtx);
    initRtxPipeline(rtx, w, h, mode);
}
