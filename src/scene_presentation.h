#pragma once
#include "upscale_shader.h"
#include "scene_effects_shader.h"

struct PostTarget {
    GLuint framebuffer = 0, color = 0;
    int width = 0, height = 0;

    void prepare(int w, int h) {
        if (width == w && height == h) return;
        if (!color) {
            glGenTextures(1, &color);
            glGenFramebuffers(1, &framebuffer);
        }
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Incomplete scene post-processing framebuffer");
        width = w;
        height = h;
    }
};

struct PostProgram {
    GLuint program;
    GLint texelX, texelY, tm, inten, deathFx, pixel, rows, tapeRows;
    GLint upscale, aa, sharpness, blend, valid, reprojection;
    GLint sampleJitter, historyJitter;

    explicit PostProgram(GLuint shader) : program(shader) {
        auto uniform = [&](const char* name) { return glGetUniformLocation(program, name); };
        glUseProgram(program);
        glUniform1i(uniform("tex"), 0);
        glUniform1i(uniform("histTex"), 1);
        glUniform1i(uniform("depthTex"), 2);
        texelX = uniform("texelX"); texelY = uniform("texelY");
        tm = uniform("tm"); inten = uniform("inten"); deathFx = uniform("deathFx");
        pixel = uniform("outputPixel"); rows = uniform("uLmPix"); tapeRows = uniform("tapeRows");
        upscale = uniform("upscaler"); aa = uniform("aaMode"); sharpness = uniform("sharpness");
        blend = uniform("taaBlend"); valid = uniform("taaValid"); reprojection = uniform("previousFromCurrentClip");
        sampleJitter = uniform("sampleJitter"); historyJitter = uniform("historyJitter");
    }
    void begin(GLuint target, int w, int h, GLuint input) const {
        glBindFramebuffer(GL_FRAMEBUFFER, target);
        glViewport(0, 0, w, h);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input);
    }
};

struct ScenePrograms {
    PostProgram tape{vhsShader};
    PostProgram fx{mkShader(vhsVS, sceneEffectsFS)};
    PostProgram resolve{mkShader(vhsVS, upscaleResolveFS)};
};

inline const ScenePrograms& scenePrograms() {
    static const ScenePrograms programs;
    return programs;
}

// Keep the history free of tape distortion, film grain and death effects.
// Full-resolution reconstruction is done once, regardless of how many taps the tape effect takes.
inline void presentScene(GLuint destination, float intensity, float death) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(quadVAO);
    if (!gPostFx.sceneDrawn) {
        glBindFramebuffer(GL_FRAMEBUFFER, destination);
        glViewport(0, 0, winW, winH);
        glClear(GL_COLOR_BUFFER_BIT);
        taaHistoryValid = false;
        taaFrameIndex = 0;
        return;
    }
    const ScenePrograms& programs = scenePrograms();
    const int upscale = clampUpscalerMode(settings.upscalerMode);
    const int aa = clampAaMode(settings.aaMode);
    const bool temporal = needsRenderHistory(upscale, aa);
    const bool resolve = upscale != UPSCALER_MODE_OFF || aa != AA_MODE_OFF;
    const bool displayFx = intensity > 0.f || death > 0.001f || gPostPixelRows > 1.f;
    const bool lighting = settings.ssaoQuality || settings.giQuality || settings.bloom ||
                          (settings.godRays && gPostFx.rayCount);
    static PostTarget lit, resolved[2];
    static Mat4 previous;
    static float previousJitterX = 0, previousJitterY = 0;
    static int lastW = 0, lastH = 0, lastAa = -1, lastUpscale = -1, lastLevel = -1;
    static unsigned lastSeed = 0;
    taaHistoryValid = taaHistoryValid && temporal && lastW == renderW && lastH == renderH &&
                      aa == lastAa && upscale == lastUpscale && worldSeed == lastSeed && gCurrentLevel == lastLevel;
    lastW = renderW; lastH = renderH; lastAa = aa; lastUpscale = upscale;
    lastSeed = worldSeed; lastLevel = gCurrentLevel;
    GLuint input = fboTex;
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, fboDepthTex);
    glActiveTexture(GL_TEXTURE0);
    if (lighting) {
        const PostProgram& fx = programs.fx;
        const bool direct = !resolve && !displayFx && renderW == winW && renderH == winH;
        if (!direct) lit.prepare(renderW, renderH);
        fx.begin(direct ? destination : lit.framebuffer, renderW, renderH, input);
        glUniform1f(fx.texelX, 1.f/renderW); glUniform1f(fx.texelY, 1.f/renderH);
        postFxSetUniforms(fx.program, true);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        if (direct) return;
        input = lit.color;
    }
    if (resolve) {
        const PostProgram& filter = programs.resolve;
        const bool direct = !temporal && !displayFx;
        if (!direct) resolved[0].prepare(winW, winH);
        if (temporal) resolved[1].prepare(winW, winH);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, temporal ? resolved[1].color : 0);
        filter.begin(direct ? destination : resolved[0].framebuffer, winW, winH, input);
        glUniform1i(filter.upscale, upscale); glUniform1i(filter.aa, aa);
        glUniform1f(filter.sharpness, clampFsrSharpness(settings.fsrSharpness));
        glUniform1f(filter.texelX, 1.f/renderW); glUniform1f(filter.texelY, 1.f/renderH);
        glUniform1f(filter.blend, 0.85f); glUniform1f(filter.valid, taaHistoryValid ? 1.f : 0.f);
        glUniform3f(filter.sampleJitter, gPostFx.jitterX, gPostFx.jitterY, 0);
        glUniform3f(filter.historyJitter, previousJitterX, previousJitterY, 0);
        if (taaHistoryValid) {
            const Mat4 reproject = previousClipFromCurrent(gPostFx.viewProjection, previous);
            glUniformMatrix4fv(filter.reprojection, 1, GL_FALSE, reproject.m);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);
        if (direct) return;
        input = resolved[0].color;
        if (temporal) {
            previous = gPostFx.viewProjection;
            previousJitterX = gPostFx.jitterX; previousJitterY = gPostFx.jitterY;
            std::swap(resolved[0], resolved[1]);
            taaHistoryValid = true;
            taaFrameIndex = (taaFrameIndex + 1) % 8;
        }
    }
    const PostProgram& tape = programs.tape;
    tape.begin(destination, winW, winH, input);
    glUniform1f(tape.tm, vhsTime); glUniform1f(tape.inten, intensity); glUniform1f(tape.deathFx, death);
    glUniform1f(tape.rows, gPostPixelRows);
    glUniform1f(tape.tapeRows, static_cast<float>(vhsTapeRows(settings.vhsResolution, winH)));
    glUniform3f(tape.pixel, 1.f/winW, 1.f/winH, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
