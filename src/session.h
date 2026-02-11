#pragma once

#include "reconnect_policy.h"
#include "coop_rules.h"
#include "flashlight_behavior.h"
#include "scare_system.h"
#include "cheats.h"
#include "minimap.h"
#include "minimap_bindings.h"
#include "perf_tuning.h"
#include "content_events.h"
#include "item_types.h"
#include "entity_ai.h"
#include "trap_events.h"
#include "debug_tools.h"
#include <vector>

// Provides GLuint for VAO/VBO handles used across the game.
#include <glad/glad.h>

// Textures are created in game_main_entry.h (declared in game.cpp)
extern GLuint deviceTex;
extern GLuint plushTex;
extern GLuint playerTex;

GLuint noteVAO=0, noteVBO=0;
int noteVC=0;
// First-person held items (separate models per slot/type)
GLuint flashlightVAO=0, flashlightVBO=0;
int flashlightVC=0;
GLuint scannerVAO=0, scannerVBO=0;
int scannerVC=0;
GLuint plushVAO=0, plushVBO=0;
int plushVC=0;
GLuint batteryVAO=0, batteryVBO=0;
int batteryVC=0;

// Selected when activeDeviceSlot == 3
int heldConsumableType = ITEM_PLUSH_TOY;
bool playerModelsInit = false;

void buildGeom();

enum InteractRequestType {
    REQ_TOGGLE_SWITCH = 1,
    REQ_PICK_ITEM = 2,
    REQ_DEBUG_SPAWN_STALKER = 3,
    REQ_DEBUG_SPAWN_CRAWLER = 4,
    REQ_DEBUG_SPAWN_SHADOW = 5
};

enum RoamEventType {
    ROAM_NONE = 0,
    ROAM_LIGHTS_OUT = 1,
    ROAM_GEOM_SHIFT = 2,
    ROAM_FALSE_DOOR = 3,
    ROAM_FLOOR_HOLES = 4,
    ROAM_SUPPLY_CACHE = 5
};

struct CoopObjectives {
    Vec3 switches[2];
    bool switchOn[2];
    Vec3 doorPos;
    bool doorOpen;
    bool initialized;
} coop = {};

struct WorldItem {
    int id;
    int type; // 0 battery
    Vec3 pos;
    bool active;
};

struct FloorHole {
    int wx;
    int wz;
    float ttl;
    bool active;
};

struct AbyssLocation {
    int centerX, centerZ;
    int radius;
    bool active;
};

struct SmileEventState {
    bool eyeActive;
    Vec3 eyePos;
    float eyeLookTime;
    float eyeLife;
    float nextSpawnTimer;
    bool corridorActive;
    Vec3 returnPos;
    Vec3 corridorEnd;
    float corridorTime;
};

std::vector<WorldItem> worldItems;
std::vector<FloorHole> floorHoles;
AbyssLocation abyss = {};
bool playerFalling = false;
float fallVelocity = 0.0f;
float fallTimer = 0.0f;
int nextWorldItemId = 1;
float itemSpawnTimer = 12.0f;
float lightsOutTimer = 0.0f;
float falseDoorTimer = 0.0f;
Vec3 falseDoorPos(0,0,0);
int invBattery = 0;
int invPlush = 0;
// 0 = none, 1 = flashlight, 2 = scanner, 3 = held consumable (battery/plush)
int activeDeviceSlot = 1;
float scannerSignal = 0.0f;
// Scanner advanced mechanic state (backlog: overheating/false peaks)
float scannerHeat = 0.0f;          // 0..1
bool scannerOverheated = false;
float scannerOverheatTimer = 0.0f; // seconds remaining until recovered
float scannerPhantomTimer = 0.0f;  // 0..: forces false peaks for a short time
float scannerPhantomBias = 0.0f;   // -0.35..+0.35 signal bias during phantom
EchoSignal echoSignal = {};
float echoSpawnTimer = 14.0f;
float echoStatusTimer = 0.0f;
char echoStatusText[96] = {};
bool storyEchoAttuned = false;
int storyEchoAttunedCount = 0;
enum ResonatorMode {
    RESONATOR_SCAN = 0,
    RESONATOR_RECORD = 1,
    RESONATOR_PLAYBACK = 2,
    RESONATOR_PING = 3
};
int resonatorMode = RESONATOR_SCAN;
float resonatorBattery = 100.0f;
float attentionLevel = 0.0f;
float attentionEventCooldown = 0.0f;
float coLevel = 0.0f;
bool ventilationOnline = false;

bool echoRecording = false;
bool echoPlayback = false;
float echoRecordTimer = 0.0f;
std::vector<Vec3> echoTrack;
int echoPlaybackIndex = 0;
Vec3 echoGhostPos(0, 0, 0);
bool echoGhostActive = false;

Vec3 level1Nodes[3];
bool level1NodeDone[3] = {false, false, false};
int level1NodeStage[3] = {0, 0, 0};
int level1NodeGoal[3] = {2, 2, 2};
bool level1HoldActive = false;
float level1HoldTimer = 90.0f;
bool level1ContractComplete = false;

bool level2BatteryInstalled = false;
int level2BatteryStage = 0;
int level2FuseCount = 0;
bool level2AccessReady = false;
bool level2FusePanelPowered = false;
bool level2HoldActive = false;
float level2HoldTimer = 15.0f;
bool level2ContractComplete = false;
Vec3 level2BatteryNode(0, 0, 0);
Vec3 level2FuseNodes[3];
bool level2FuseDone[3] = {false, false, false};
Vec3 level2AccessNode(0, 0, 0);
Vec3 level2VentNode(0, 0, 0);
bool level2VentDone = false;
Vec3 level2LiftNode(0, 0, 0);
Vec3 level2PowerNode(0, 0, 0);

int archivePoints = 0;
int archiveTier = 0;
bool perkQuietSteps = false;
bool perkFastHands = false;
bool perkEchoBuffer = false;

enum SideContractType {
    SIDE_NONE = 0,
    SIDE_COLLECT_BADGES = 1,
    SIDE_SCAN_WALLS = 2,
    SIDE_STABILIZE_RIFTS = 3,
    SIDE_RESCUE_SURVIVOR = 4,
    SIDE_ENABLE_VENT = 5,
    SIDE_RESTORE_CAMERAS = 6,
    SIDE_REPROGRAM_DRONE = 7
};

int sideContractType = SIDE_NONE;
int sideContractProgress = 0;
int sideContractTarget = 0;
bool sideContractCompleted = false;

Vec3 npcCartographerPos(0, 0, 0);
bool npcCartographerActive = false;
Vec3 npcDispatcherPhonePos(0, 0, 0);
bool npcDispatcherActive = false;
Vec3 npcLostSurvivorPos(0, 0, 0);
bool npcLostSurvivorActive = false;
bool npcLostSurvivorEscorted = false;
float dispatcherCallCooldown = 0.0f;

TrapCorridorState trapCorridor = {};
DebugToolsState debugTools = {};
SmileEventState smileEvent = {false, Vec3(0,0,0), 0.0f, 0.0f, 28.0f, false, Vec3(0,0,0), Vec3(0,0,0), 0.0f};
float anomalyBlur = 0.0f;
float trapStatusTimer = 0.0f;
char trapStatusText[96] = {};
extern int nearbyWorldItemId;
extern int nearbyWorldItemType;

inline void triggerLocalScare(float flash, float shake, float sanityDamage){
    if(damageFlash < flash) damageFlash = flash;
    if(camShake < shake) camShake = shake;
    playerSanity -= sanityDamage;
    if(playerSanity < 0.0f) playerSanity = 0.0f;
    triggerScare();
}

struct SessionSnapshot {
    bool valid;
    char hostIP[64];
    Vec3 camPos;
    float camYaw, camPitch;
    float health, sanity, stamina, battery;
    float survival;
    int invB;
};

SessionSnapshot lastSession = {};
bool reconnectInProgress = false;
bool restoreAfterReconnect = false;
float reconnectAttemptTimer = 0.0f;
int reconnectAttempts = 0;

inline void captureSessionSnapshot(){
    if(multiState!=MULTI_IN_GAME || netMgr.isHost) return;
    lastSession.valid = true;
    snprintf(lastSession.hostIP, sizeof(lastSession.hostIP), "%s", netMgr.hostIP);
    lastSession.camPos = cam.pos;
    lastSession.camYaw = cam.yaw;
    lastSession.camPitch = cam.pitch;
    lastSession.health = playerHealth;
    lastSession.sanity = playerSanity;
    lastSession.stamina = playerStamina;
    lastSession.battery = flashlightBattery;
    lastSession.survival = survivalTime;
    lastSession.invB = invBattery;
}

inline void restoreSessionSnapshot(){
    if(!lastSession.valid) return;
    cam.pos = lastSession.camPos;
    cam.yaw = lastSession.camYaw;
    cam.pitch = lastSession.camPitch;
    playerHealth = lastSession.health;
    playerSanity = lastSession.sanity;
    playerStamina = lastSession.stamina;
    flashlightBattery = lastSession.battery;
    survivalTime = lastSession.survival;
    invBattery = lastSession.invB;
}

inline void initCoopObjectives(const Vec3& basePos){
    auto findNearestCell = [](int targetWX, int targetWZ, int maxRadius, auto predicate, int& outWX, int& outWZ) {
        if (predicate(targetWX, targetWZ)) {
            outWX = targetWX;
            outWZ = targetWZ;
            return true;
        }
        for (int r = 1; r <= maxRadius; r++) {
            for (int dz = -r; dz <= r; dz++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (dx != -r && dx != r && dz != -r && dz != r) continue;
                    int wx = targetWX + dx;
                    int wz = targetWZ + dz;
                    if (!predicate(wx, wz)) continue;
                    outWX = wx;
                    outWZ = wz;
                    return true;
                }
            }
        }
        return false;
    };

    auto openCell = [](int wx, int wz) { return getCellWorld(wx, wz) == 0; };
    auto validDoorCell = [](int wx, int wz) {
        if (!isDoorFootprintClear(wx, wz, [](int cx, int cz) { return getCellWorld(cx, cz); })) return false;
        bool frontOpen = getCellWorld(wx, wz + 1) == 0;
        bool backOpen = getCellWorld(wx, wz - 1) == 0;
        bool leftWall = getCellWorld(wx - 1, wz) == 1;
        bool rightWall = getCellWorld(wx + 1, wz) == 1;
        return frontOpen && backOpen && leftWall && rightWall;
    };

    int baseWX = (int)floorf(basePos.x / CS);
    int baseWZ = (int)floorf(basePos.z / CS);

    int doorWX = baseWX;
    int doorWZ = baseWZ + 12;
    if (!findNearestCell(doorWX, doorWZ, 16, validDoorCell, doorWX, doorWZ)) {
        if (!findNearestCell(baseWX, baseWZ, 34, validDoorCell, doorWX, doorWZ)) {
            if (!findNearestCell(baseWX, baseWZ, 12, openCell, doorWX, doorWZ)) {
                doorWX = baseWX;
                doorWZ = baseWZ + 4;
            }
            setCellWorld(doorWX, doorWZ, 0);
            setCellWorld(doorWX, doorWZ - 1, 0);
            setCellWorld(doorWX, doorWZ + 1, 0);
            setCellWorld(doorWX - 1, doorWZ, 1);
            setCellWorld(doorWX + 1, doorWZ, 1);
        }
    }

    int sw0x = baseWX + 2;
    int sw0z = baseWZ + 1;
    int sw1x = baseWX - 2;
    int sw1z = baseWZ + 1;
    findNearestCell(sw0x, sw0z, 8, openCell, sw0x, sw0z);
    findNearestCell(sw1x, sw1z, 8, openCell, sw1x, sw1z);

    coop.switches[0] = Vec3((sw0x + 0.5f) * CS, 0, (sw0z + 0.5f) * CS);
    coop.switches[1] = Vec3((sw1x + 0.5f) * CS, 0, (sw1z + 0.5f) * CS);
    coop.switchOn[0] = false;
    coop.switchOn[1] = false;
    coop.doorPos = Vec3((doorWX + 0.5f) * CS, 0, (doorWZ + 0.5f) * CS);
    coop.doorOpen = false;
    coop.initialized = true;
}

inline bool nearPoint2D(const Vec3& a, const Vec3& b, float r){
    Vec3 d = a - b; d.y = 0;
    return sqrtf(d.x*d.x + d.z*d.z) < r;
}

inline bool projectToScreen(const Vec3& worldPos, float& sx, float& sy){
    Vec3 d = worldPos - cam.pos;
    float cy = mCos(cam.yaw), syaw = mSin(cam.yaw);
    float cp = mCos(cam.pitch), sp = mSin(cam.pitch);

    float cx = d.x * cy - d.z * syaw;
    float cz0 = d.x * syaw + d.z * cy;
    float cy2 = d.y * cp - cz0 * sp;
    float cz = d.y * sp + cz0 * cp;
    if(cz <= 0.05f) return false;

    float fov = 1.2f;
    float t = tanf(fov * 0.5f);
    float asp = (float)winW / (float)winH;
    sx = cx / (cz * asp * t);
    sy = cy2 / (cz * t);
    return sx > -1.2f && sx < 1.2f && sy > -1.2f && sy < 1.2f;
}

inline int minimapWallSampler(int wx, int wz){
    return getCellWorld(wx, wz) == 1 ? 1 : 0;
}

inline void updateMinimapCheat(GLFWwindow* w){
    static bool letterPressed[26] = {false};
    static MinimapBindingState minimapBindingState = {};

    bool mNow = glfwGetKey(w, GLFW_KEY_M) == GLFW_PRESS;
    bool f8Now = glfwGetKey(w, GLFW_KEY_F8) == GLFW_PRESS;
    if(shouldToggleMinimapFromBindings(mNow, f8Now, minimapBindingState)){
        minimapEnabled = !minimapEnabled;
    }

    for(int i=0;i<26;i++){
        bool now = glfwGetKey(w, GLFW_KEY_A + i) == GLFW_PRESS;
        if(now && !letterPressed[i]){
            char input = (char)('A' + i);
            if(pushMinimapCheatChar(minimapCheatProgress, input)){
                minimapEnabled = !minimapEnabled;
            }
        }
        letterPressed[i] = now;
    }
}

inline void setEchoStatus(const char* msg){
    snprintf(echoStatusText, sizeof(echoStatusText), "%s", msg);
    echoStatusTimer = 4.0f;
}

inline void setTrapStatus(const char* msg){
    snprintf(trapStatusText, sizeof(trapStatusText), "%s", msg);
    trapStatusTimer = 4.0f;
}

// === Viewmodel tuning (runtime) ===
// Allows adjusting first-person held item placement without rebuilding.
inline float vmHandFwd = 0.60f;
inline float vmHandSide = 0.33f;
inline float vmHandUp = -0.52f;
inline float vmFlashLensFwd = 0.52f;
inline float vmFlashLensSide = 0.02f;
inline float vmFlashLensUp = 0.09f;
// 0..1: how much camera shake affects held items (0 = stable, 1 = full shake)
inline float vmShake = 0.0f;

inline void applyPlushToyUse(){
    if(invPlush <= 0) return;
    invPlush--;
    // Slot 3 plush toy should be a strong sanity recovery tool.
    playerSanity += 40.0f;
    if(playerSanity > 100.0f) playerSanity = 100.0f;
    setEchoStatus("PLUSH TOY: YOUR MIND FEELS WHOLE AGAIN");
}

inline void initLevel1PuzzleStages();
inline bool processLevel1NodeStage(int nodeIndex);
inline void buildLevel1NodeActionPrompt(int nodeIndex, char* out, int outSize);
inline void initLevel2PuzzleStages();
inline bool processLevel2Step(const Vec3& playerPos);
inline bool buildLevel2ActionPrompt(const Vec3& playerPos, char* out, int outSize);

#include "void_shift_runtime.h"
#include "void_shift_puzzles_ext.h"
#include "void_shift_level2_ext.h"

inline int storyNotesRequired(){
    if (isParkingLevel(gCurrentLevel)) return 3;
    return 3;
}

inline bool isStoryExitReady(){
    return isVoidShiftExitReady();
}
