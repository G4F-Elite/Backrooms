#pragma once
// Off-screen frame capture through the game's own scene pass and post-process shader.

inline GLuint trialOutFbo = 0, trialOutTex = 0;

// The project's trimmed glad header has no read-back calls; both are plain GL 1.1
// exports of opengl32.dll.
extern "C" {
__declspec(dllimport) void __stdcall glReadPixels(int x, int y, int w, int h, unsigned format, unsigned type, void* data);
__declspec(dllimport) void __stdcall glPixelStorei(unsigned pname, int param);
}
const unsigned TRIAL_GL_PACK_ALIGNMENT = 0x0D05;

inline bool trialInitGl() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    gWin = glfwCreateWindow(winW, winH, "scene_trial", NULL, NULL);
    if (!gWin) { glfwTerminate(); return false; }
    glfwMakeContextCurrent(gWin);
    if (!gladLoadGL()) return false;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Same resources, in the same order, as the game's main().
    wallTex = genTex(0); floorTex = genTex(1); ceilTex = genTex(2); lightTex = genTex(3);
    lampTex = genTex(4); propTex = genTex(5); deviceTex = genTex(6);
    decorTex = genDecorAtlasTex();
    plushTex = genTex(7);
    playerTex = propTex;
    mainShader = mkShader(mainVS, mainFS);
    lightShader = mkShader(lightVS, lightFS);
    vhsShader = mkShader(vhsVS, vhsFS);
    computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), renderW, renderH);
    initFBO(fbo, fboTex, fboDepthTex, renderW, renderH);
    initTaaTargets();
    entityMgr.init();

    glGenTextures(1, &trialOutTex);
    glBindTexture(GL_TEXTURE_2D, trialOutTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winW, winH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &trialOutFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, trialOutFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, trialOutTex, 0);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

// Fixed player state: nothing held, no flashlight, no threat, no shake, fixed clock.
inline void trialFreezeState() {
    activeDeviceSlot = 0;
    flashlightOn = false;
    camShake = 0.0f;
    damageFlash = 0.0f;
    playerSanity = 100.0f;
    playerHealth = 100.0f;
    isPlayerDead = false;
    entityMgr.reset();
    sndState.moveIntensity = 0.0f;
    sndState.sprintIntensity = 0.0f;
    worldItems.clear();
    noteVC = 0;
    vhsTime = 10.0f;
    dTime = 0.016f;
    gameState = STATE_GAME;
}

// The game's non-TAA post-process branch with gameplay intensity at zero stress.
// TAA is skipped: it needs frame history, and a still frame has none.
inline void trialPostPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, trialOutFbo);
    glViewport(0, 0, winW, winH);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(vhsShader);
    GLuint s = vhsShader;
    glUniform1i(glGetUniformLocation(s, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, taaHistoryTex);
    glUniform1i(glGetUniformLocation(s, "histTex"), 1);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, fboDepthTex);
    glUniform1i(glGetUniformLocation(s, "depthTex"), 2);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(s, "rtxA"), clampSsao(settings.ssaoQuality));
    glUniform1i(glGetUniformLocation(s, "rtxG"), clampGi(settings.giQuality));
    glUniform1i(glGetUniformLocation(s, "rtxR"), settings.godRays ? 1 : 0);
    glUniform1i(glGetUniformLocation(s, "rtxB"), settings.bloom ? 1 : 0);
    glUniform1i(glGetUniformLocation(s, "rtxDenoiseOn"), settings.rtxDenoise ? 1 : 0);
    glUniform1f(glGetUniformLocation(s, "rtxDenoiseStrength"), settings.rtxDenoiseStrength);
    glUniform1f(glGetUniformLocation(s, "tm"), vhsTime);
    glUniform1f(glGetUniformLocation(s, "inten"), settings.vhsIntensity * 0.62f);
    glUniform1i(glGetUniformLocation(s, "upscaler"), clampUpscalerMode(settings.upscalerMode));
    glUniform1i(glGetUniformLocation(s, "aaMode"), AA_MODE_FXAA);
    glUniform1f(glGetUniformLocation(s, "sharpness"), clampFsrSharpness(settings.fsrSharpness));
    glUniform1f(glGetUniformLocation(s, "texelX"), 1.0f / (float)renderW);
    glUniform1f(glGetUniformLocation(s, "texelY"), 1.0f / (float)renderH);
    glUniform1f(glGetUniformLocation(s, "taaBlend"), 0.0f);
    glUniform3f(glGetUniformLocation(s, "taaJitter"), 0.0f, 0.0f, 0.0f);
    glUniform1f(glGetUniformLocation(s, "taaValid"), 0.0f);
    glUniform1i(glGetUniformLocation(s, "frameGen"), 0);
    glUniform1f(glGetUniformLocation(s, "frameGenBlend"), 0.0f);
    glUniform1f(glGetUniformLocation(s, "deathFx"), 0.0f);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
}

inline bool trialWriteBmp(const char* path, const unsigned char* rgb, int w, int h) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    int row = (w * 3 + 3) & ~3;
    unsigned size = 54u + (unsigned)(row * h);
    unsigned char hd[54] = {0};
    hd[0] = 'B'; hd[1] = 'M';
    hd[2] = (unsigned char)size; hd[3] = (unsigned char)(size >> 8);
    hd[4] = (unsigned char)(size >> 16); hd[5] = (unsigned char)(size >> 24);
    hd[10] = 54; hd[14] = 40;
    hd[18] = (unsigned char)w; hd[19] = (unsigned char)(w >> 8);
    hd[22] = (unsigned char)h; hd[23] = (unsigned char)(h >> 8);
    hd[26] = 1; hd[28] = 24;
    fwrite(hd, 1, 54, f);
    std::vector<unsigned char> line((size_t)row, 0);
    for (int y = 0; y < h; y++) {
        const unsigned char* src = rgb + (size_t)y * (size_t)w * 3;
        for (int x = 0; x < w; x++) {
            line[x * 3 + 0] = src[x * 3 + 2];
            line[x * 3 + 1] = src[x * 3 + 1];
            line[x * 3 + 2] = src[x * 3 + 0];
        }
        fwrite(line.data(), 1, (size_t)row, f);
    }
    fclose(f);
    return true;
}

// Renders the current world from the current camera and saves the frame. Returns mean luma 0..255.
inline float trialShoot(const char* path) {
    for (int pass = 0; pass < 3; pass++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, renderW, renderH);
        glClearColor(0.02f, 0.02f, 0.02f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderScene();
        trialPostPass();
    }
    std::vector<unsigned char> px((size_t)winW * (size_t)winH * 3);
    glBindFramebuffer(GL_FRAMEBUFFER, trialOutFbo);
    glPixelStorei(TRIAL_GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, winW, winH, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    trialWriteBmp(path, px.data(), winW, winH);
    double sum = 0.0;
    for (size_t i = 0; i < px.size(); i += 3) sum += 0.299 * px[i] + 0.587 * px[i + 1] + 0.114 * px[i + 2];
    return (float)(sum / (double)(px.size() / 3));
}
