// Backrooms VHS Horror - Level 0
// Modular architecture: each .h file has single responsibility

#define _USE_MATH_DEFINES
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>
#include <thread>
#include <unordered_map>

// Windows networking first (before glad)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// OpenGL
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Game modules (in dependency order)
#include "math.h"
#include "audio.h"
#include "shaders.h"
#include "textures.h"
#include "geometry.h"
#include "render.h"
#include "perf_tuning.h"
#include "world.h"
#include "entity_types.h"
#include "entity_model.h"
#include "entity.h"
#include "entity_ai.h"
#include "story.h"
#include "menu.h"
#include "input.h"
#include "cheats.h"
#include "minimap.h"
#include "content_events.h"
#include "trap_events.h"
#include "net_types.h"
#include "net.h"
#include "player_model.h"
#include "menu_multi.h"
#include "scare_system.h"

// Constants
const float CS = 5.0f;
const float WH = 4.5f;
const float PH = 1.7f;
const float PH_CROUCH = 0.9f;
const float PR = 0.3f;

// Window
int winW = 1280, winH = 720;
int renderW = 960, renderH = 540;
GLFWwindow* gWin;

// World data
std::unordered_map<long long, Chunk> chunks;
std::vector<Light> lights;
std::vector<Vec3> pillars;
std::mt19937 rng;
int playerChunkX = 0, playerChunkZ = 0;
int lastBuildChunkX = -999, lastBuildChunkZ = -999;
unsigned int worldSeed = 0;

// Player state
struct {
    Vec3 pos;
    float yaw, pitch;
    float targetH, curH;
    bool crouch;
} cam = {{}, 0, 0, PH, PH, false};

// Game state
float dTime, lastFrame, vhsTime;
float lastX = 640, lastY = 360;
float entitySpawnTimer, playerHealth = 100, playerSanity = 100;
float camShake, damageFlash, survivalTime, reshuffleTimer;
float playerStamina = 100, staminaCooldown = 0, flashlightBattery = 100;
bool flashlightOn = false, flashlightPressed = false;
bool flashlightShutdownBlinkActive = false;
float flashlightShutdownBlinkTimer = 0.0f;
bool minimapEnabled = false;
int minimapCheatProgress = 0;
int nearbyWorldItemId = -1;
int nearbyWorldItemType = -1;
ScareSystemState scareState = {};
bool interactPressed = false, spacePressed = false;
int nearNoteId = -1, lastSpawnedNote = -1;
float noteSpawnTimer = 0;
bool firstMouse = true;
bool escPressed, enterPressed, isPlayerDead;
bool upPressed, downPressed, leftPressed, rightPressed;

// OpenGL resources
GLuint wallTex, floorTex, ceilTex, lightTex, lampTex;
GLuint mainShader, vhsShader, lightShader;
GLuint wallVAO, wallVBO, floorVAO, floorVBO;
GLuint ceilVAO, ceilVBO, lightVAO, lightVBO;
GLuint lightOffVAO, lightOffVBO;
GLuint pillarVAO, pillarVBO;
GLuint quadVAO, quadVBO;
GLuint fbo, fboTex, rbo;
GLuint taaHistoryTex = 0;
GLuint taaResolveTex = 0;
GLuint taaResolveFbo = 0;
bool taaHistoryValid = false;
int taaFrameIndex = 0;
int wallVC, floorVC, ceilVC, lightVC, lightOffVC, pillarVC;

// Audio
SoundState sndState;
std::atomic<bool> audioRunning{true};
HANDLE hEvent;
short* waveBufs[BUF_COUNT];
HWAVEOUT hWaveOut;
WAVEHDR waveHdrs[BUF_COUNT];

// Managers
EntityManager entityMgr;

// Audio thread function
void audioThread() {
    hEvent = CreateEvent(0, FALSE, FALSE, 0);
    WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, 1, SAMP_RATE, SAMP_RATE*2, 2, 16, 0};
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)hEvent, 0, CALLBACK_EVENT);
    
    for (int i = 0; i < BUF_COUNT; i++) {
        waveBufs[i] = new short[BUF_LEN];
        memset(&waveHdrs[i], 0, sizeof(WAVEHDR));
        waveHdrs[i].lpData = (LPSTR)waveBufs[i];
        waveHdrs[i].dwBufferLength = BUF_LEN * 2;
        fillAudio(waveBufs[i], BUF_LEN);
        waveOutPrepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
    }
    
    while (audioRunning) {
        WaitForSingleObject(hEvent, INFINITE);
        for (int i = 0; i < BUF_COUNT; i++) {
            if (waveHdrs[i].dwFlags & WHDR_DONE) {
                waveOutUnprepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
                fillAudio(waveBufs[i], BUF_LEN);
                waveOutPrepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
                waveOutWrite(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
            }
        }
    }
    
    waveOutReset(hWaveOut);
    for (int i = 0; i < BUF_COUNT; i++) {
        waveOutUnprepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
        delete[] waveBufs[i];
    }
    waveOutClose(hWaveOut);
    CloseHandle(hEvent);
}

void initTaaTargets() {
    if (taaResolveFbo) glDeleteFramebuffers(1, &taaResolveFbo);
    if (taaHistoryTex) glDeleteTextures(1, &taaHistoryTex);
    if (taaResolveTex) glDeleteTextures(1, &taaResolveTex);

    glGenTextures(1, &taaHistoryTex);
    glBindTexture(GL_TEXTURE_2D, taaHistoryTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winW, winH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &taaResolveTex);
    glBindTexture(GL_TEXTURE_2D, taaResolveTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winW, winH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &taaResolveFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, taaResolveFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, taaResolveTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    taaHistoryValid = false;
    taaFrameIndex = 0;
}

// Window resize callback
void windowResize(GLFWwindow*, int w, int h) {
    if (w < 100) w = 100;
    if (h < 100) h = 100;
    winW = w;
    winH = h;
    computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), renderW, renderH);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (fboTex) glDeleteTextures(1, &fboTex);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
    initFBO(fbo, fboTex, rbo, renderW, renderH);
    initTaaTargets();
}

// Shader compiler
GLuint mkShader(const char* vs, const char* fs) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, 0);
    glCompileShader(v);
    
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, 0);
    glCompileShader(f);
    
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// Include game loop (uses all above)
#include "game_loop.h"

int main() {
    std::random_device rd;
    rng.seed(rd());
    
    if (!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    gWin = glfwCreateWindow(winW, winH, "Backrooms - Level 0", NULL, NULL);
    if (!gWin) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(gWin);
    glfwSetCursorPosCallback(gWin, mouse);
    glfwSetFramebufferSizeCallback(gWin, windowResize);
    
    if (!gladLoadGL()) return -1;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    genWorld();
    wallTex = genTex(0);
    floorTex = genTex(1);
    ceilTex = genTex(2);
    lightTex = genTex(3);
    lampTex = genTex(4);
    
    mainShader = mkShader(mainVS, mainFS);
    lightShader = mkShader(lightVS, lightFS);
    vhsShader = mkShader(vhsVS, vhsFS);
    
    buildGeom();
    computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), renderW, renderH);
    initFBO(fbo, fboTex, rbo, renderW, renderH);
    initTaaTargets();
    initText();
    entityMgr.init();
    initPlayerModels();
    playerModelsInit = true;
    
    std::thread aT(audioThread);
    
    while (!glfwWindowShouldClose(gWin)) {
        float now = (float)glfwGetTime();
        dTime = now - lastFrame;
        lastFrame = now;
        vhsTime = now;
        
        int desiredRenderW = 0, desiredRenderH = 0;
        computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), desiredRenderW, desiredRenderH);
        
        if (desiredRenderW != renderW || desiredRenderH != renderH) {
            renderW = desiredRenderW;
            renderH = desiredRenderH;
            if (fbo) glDeleteFramebuffers(1, &fbo);
            if (fboTex) glDeleteTextures(1, &fboTex);
            if (rbo) glDeleteRenderbuffers(1, &rbo);
            initFBO(fbo, fboTex, rbo, renderW, renderH);
        }
        
        sndState.masterVol = settings.masterVol;
        sndState.dangerLevel = entityMgr.dangerLevel;
        sndState.musicVol = settings.musicVol;
        sndState.ambienceVol = settings.ambienceVol;
        sndState.sfxVol = settings.sfxVol;
        sndState.voiceVol = settings.voiceVol;
        sndState.sanityLevel = playerSanity / 100.0f;
        currentWinW = winW;
        currentWinH = winH;
        
        processGameState();
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, renderW, renderH);
        glClearColor(0.02f, 0.02f, 0.02f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (gameState == STATE_GAME || gameState == STATE_PAUSE || 
            gameState == STATE_SETTINGS_PAUSE || gameState == STATE_KEYBINDS_PAUSE || 
            gameState == STATE_NOTE) {
            renderScene();
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, winW, winH);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        
        glUseProgram(vhsShader);
        
        bool vhsMenu = (gameState == STATE_MENU || gameState == STATE_MULTI || 
                        gameState == STATE_MULTI_HOST || gameState == STATE_MULTI_JOIN || 
                        gameState == STATE_MULTI_WAIT || gameState == STATE_SETTINGS || 
                        gameState == STATE_KEYBINDS);
        bool vhsGameplay = (gameState == STATE_GAME || gameState == STATE_PAUSE || 
                            gameState == STATE_SETTINGS_PAUSE || gameState == STATE_KEYBINDS_PAUSE || 
                            gameState == STATE_NOTE || gameState == STATE_INTRO);
        
        float vI = 0.0f;
        if (vhsMenu) {
            vI = 0.22f + settings.vhsIntensity * 0.58f;
        } else if (vhsGameplay) {
            float sanityLoss = 1.0f - (playerSanity / 100.0f);
            if (sanityLoss < 0.0f) sanityLoss = 0.0f;
            if (sanityLoss > 1.0f) sanityLoss = 1.0f;
            float stress = entityMgr.dangerLevel * 0.65f + sanityLoss * 0.35f;
            if (stress < 0.0f) stress = 0.0f;
            if (stress > 1.0f) stress = 1.0f;
            vI = settings.vhsIntensity * (0.26f + 0.44f * stress) + anomalyBlur * 0.55f;
        }
        
        static GLint vhsTmLoc = -1, vhsIntenLoc = -1, vhsUpscalerLoc = -1;
        static GLint vhsAaModeLoc = -1, vhsSharpnessLoc = -1, vhsTexelXLoc = -1;
        static GLint vhsTexelYLoc = -1, vhsInMenuLoc = -1, vhsTaaHistLoc = -1;
        static GLint vhsTaaBlendLoc = -1, vhsTaaJitterLoc = -1, vhsTaaValidLoc = -1;
        
        if (vhsTmLoc < 0) {
            glUniform1i(glGetUniformLocation(vhsShader, "tex"), 0);
            vhsTmLoc = glGetUniformLocation(vhsShader, "tm");
            vhsIntenLoc = glGetUniformLocation(vhsShader, "inten");
            vhsUpscalerLoc = glGetUniformLocation(vhsShader, "upscaler");
            vhsAaModeLoc = glGetUniformLocation(vhsShader, "aaMode");
            vhsSharpnessLoc = glGetUniformLocation(vhsShader, "sharpness");
            vhsTexelXLoc = glGetUniformLocation(vhsShader, "texelX");
            vhsTexelYLoc = glGetUniformLocation(vhsShader, "texelY");
            vhsInMenuLoc = glGetUniformLocation(vhsShader, "inMenu");
            vhsTaaHistLoc = glGetUniformLocation(vhsShader, "histTex");
            vhsTaaBlendLoc = glGetUniformLocation(vhsShader, "taaBlend");
            vhsTaaJitterLoc = glGetUniformLocation(vhsShader, "taaJitter");
            vhsTaaValidLoc = glGetUniformLocation(vhsShader, "taaValid");
        }
        
        int inMenu = (gameState == STATE_MENU || gameState == STATE_MULTI || 
                      gameState == STATE_MULTI_HOST || gameState == STATE_MULTI_JOIN ||
                      gameState == STATE_MULTI_WAIT || gameState == STATE_SETTINGS) ? 1 : 0;
        
        static int prevAaMode = -1;
        int aaMode = clampAaMode(settings.aaMode);
        if (aaMode != prevAaMode) {
            prevAaMode = aaMode;
            taaHistoryValid = false;
            taaFrameIndex = 0;
        }
        
        float jitterX = 0.0f, jitterY = 0.0f;
        if (aaMode == AA_MODE_TAA) {
            static const float jitterSeq[8][2] = {
                {0.5f, 0.5f}, {0.75f, 0.25f}, {0.25f, 0.75f}, {0.875f, 0.625f},
                {0.375f, 0.125f}, {0.625f, 0.875f}, {0.125f, 0.375f}, {0.9375f, 0.9375f}
            };
            jitterX = jitterSeq[taaFrameIndex & 7][0] - 0.5f;
            jitterY = jitterSeq[taaFrameIndex & 7][1] - 0.5f;
        }
        
        if (aaMode == AA_MODE_TAA) {
            // TAA resolve pass
            glBindFramebuffer(GL_FRAMEBUFFER, taaResolveFbo);
            glViewport(0, 0, winW, winH);
            glClear(GL_COLOR_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fboTex);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, taaHistoryTex);
            glUniform1i(vhsTaaHistLoc, 1);
            glActiveTexture(GL_TEXTURE0);
            glUniform1f(vhsTmLoc, vhsTime);
            glUniform1f(vhsIntenLoc, vI);
            glUniform1i(vhsUpscalerLoc, clampUpscalerMode(settings.upscalerMode));
            glUniform1i(vhsAaModeLoc, AA_MODE_TAA);
            glUniform1f(vhsSharpnessLoc, clampFsrSharpness(settings.fsrSharpness));
            glUniform1f(vhsTexelXLoc, 1.0f / (float)renderW);
            glUniform1f(vhsTexelYLoc, 1.0f / (float)renderH);
            glUniform1f(vhsTaaBlendLoc, 0.88f);
            glUniform3f(vhsTaaJitterLoc, jitterX, jitterY, 0.0f);
            glUniform1f(vhsTaaValidLoc, taaHistoryValid ? 1.0f : 0.0f);
            glUniform1i(vhsInMenuLoc, inMenu);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            taaHistoryValid = true;
            taaFrameIndex = (taaFrameIndex + 1) & 7;
            
            GLuint taaTmp = taaHistoryTex;
            taaHistoryTex = taaResolveTex;
            taaResolveTex = taaTmp;
            glBindFramebuffer(GL_FRAMEBUFFER, taaResolveFbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, taaResolveTex, 0);
            
            // Final output
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, winW, winH);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, taaHistoryTex);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, taaHistoryTex);
            glUniform1i(vhsTaaHistLoc, 1);
            glActiveTexture(GL_TEXTURE0);
            glUniform1f(vhsTmLoc, vhsTime);
            glUniform1f(vhsIntenLoc, 0.0f);
            glUniform1i(vhsUpscalerLoc, UPSCALER_MODE_OFF);
            glUniform1i(vhsAaModeLoc, AA_MODE_OFF);
            glUniform1f(vhsSharpnessLoc, 0.0f);
            glUniform1f(vhsTexelXLoc, 1.0f / (float)winW);
            glUniform1f(vhsTexelYLoc, 1.0f / (float)winH);
            glUniform1f(vhsTaaBlendLoc, 0.0f);
            glUniform3f(vhsTaaJitterLoc, 0.0f, 0.0f, 0.0f);
            glUniform1f(vhsTaaValidLoc, 0.0f);
            glUniform1i(vhsInMenuLoc, inMenu);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else {
            taaHistoryValid = false;
            taaFrameIndex = 0;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fboTex);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, taaHistoryTex);
            glUniform1i(vhsTaaHistLoc, 1);
            glActiveTexture(GL_TEXTURE0);
            glUniform1f(vhsTmLoc, vhsTime);
            glUniform1f(vhsIntenLoc, vI);
            glUniform1i(vhsUpscalerLoc, clampUpscalerMode(settings.upscalerMode));
            glUniform1i(vhsAaModeLoc, aaMode);
            glUniform1f(vhsSharpnessLoc, clampFsrSharpness(settings.fsrSharpness));
            glUniform1f(vhsTexelXLoc, 1.0f / (float)renderW);
            glUniform1f(vhsTexelYLoc, 1.0f / (float)renderH);
            glUniform1f(vhsTaaBlendLoc, 0.0f);
            glUniform3f(vhsTaaJitterLoc, 0.0f, 0.0f, 0.0f);
            glUniform1f(vhsTaaValidLoc, 0.0f);
            glUniform1i(vhsInMenuLoc, inMenu);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        glEnable(GL_DEPTH_TEST);
        drawUI();
        
        glfwSwapBuffers(gWin);
        glfwPollEvents();
    }
    
    audioRunning = false;
    SetEvent(hEvent);
    aT.join();
    glfwTerminate();
    
    return 0;
}