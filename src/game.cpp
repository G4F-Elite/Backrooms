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
GLuint wallTex, floorTex, ceilTex, lightTex;
GLuint mainShader, vhsShader, lightShader;
GLuint wallVAO, wallVBO, floorVAO, floorVBO;
GLuint ceilVAO, ceilVBO, lightVAO, lightVBO;
GLuint lightOffVAO, lightOffVBO;
GLuint pillarVAO, pillarVBO;
GLuint quadVAO, quadVBO;
GLuint fbo, fboTex, rbo;
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

// Window resize callback
void windowResize(GLFWwindow*, int w, int h) {
    if (w < 100) w = 100;
    if (h < 100) h = 100;
    winW = w;
    winH = h;
    computeRenderTargetSize(winW, winH, SCENE_RENDER_SCALE, renderW, renderH);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (fboTex) glDeleteTextures(1, &fboTex);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
    initFBO(fbo, fboTex, rbo, renderW, renderH);
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
