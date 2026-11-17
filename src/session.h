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
extern GLuint decorTex;

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
GLuint medSprayVAO=0, medSprayVBO=0;
int medSprayVC=0;

// Selected when activeDeviceSlot == 3
int heldConsumableType = ITEM_PLUSH_TOY;
bool playerModelsInit = false;

void buildGeom();

enum InteractRequestType {
    REQ_TOGGLE_SWITCH = 1,
    REQ_PICK_ITEM = 2,
    REQ_DEBUG_SPAWN_STALKER = 3,
    REQ_DEBUG_SPAWN_CRAWLER = 4,
    REQ_DEBUG_SPAWN_SHADOW = 5,
    REQ_HEIST_ACTION = 6,   // target = HeistAction (deposit, sell, drop, start the drill...)
    REQ_EXIT_LEVEL = 7,
    REQ_HEIST_BREAK = 8,    // target = side of the player's cell (0..3)
    REQ_HEIST_BUY = 9,      // target = ShopItem
    REQ_HEIST_STUDY = 10    // target = EntityType
};

enum RoamEventType {
    ROAM_NONE = 0,
    ROAM_LIGHTS_OUT = 1,
    ROAM_GEOM_SHIFT = 2,
    ROAM_FALSE_DOOR = 3,
    ROAM_FLOOR_HOLES = 4,
    ROAM_SUPPLY_CACHE = 5
};

// Where the squad regroups on a floor: the drill. (Kept as `coop.doorPos` for the teleport helpers.)
struct CoopObjectives {
    Vec3 doorPos;
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
float squadCalloutTimer = 0.0f;
char squadCalloutText[96] = {};
float conferenceCallTimer = 0.0f;
float conferenceCallPulse = 0.0f;
float corridorShiftTimer = 0.0f;
bool corridorShiftArmed = false;
float blackoutSectorTimer = 0.0f;
float paperclipSwarmTimer = 0.0f;
float oilSlipTimer = 0.0f;
float headlightExposureTimer = 0.0f;

inline void setSquadCallout(const char* txt) {
    if (!txt) return;
    std::snprintf(squadCalloutText, sizeof(squadCalloutText), "%s", txt);
    squadCalloutTimer = 3.5f;
}
float attentionLevel = 0.0f;
float attentionEventCooldown = 0.0f;

int archivePoints = 0;
int archiveTier = 0;
bool perkQuietSteps = false;
bool perkFastHands = false;
bool perkEchoBuffer = false;

TrapCorridorState trapCorridor = {};
DebugToolsState debugTools = {};
SmileEventState smileEvent = {false, Vec3(0,0,0), 0.0f, 0.0f, 28.0f, false, Vec3(0,0,0), Vec3(0,0,0), 0.0f};
float anomalyBlur = 0.0f;
float trapStatusTimer = 0.0f;
char trapStatusText[96] = {};
extern int nearbyWorldItemId;
extern int nearbyWorldItemType;

// Set while the local player stands somewhere safe (the rig's haven): no jump scares there.
inline bool gScaresMuted = false;

inline void triggerLocalScare(float flash, float shake, float sanityDamage){
    if(gScaresMuted) return;
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
    coop.doorPos = basePos;
    coop.initialized = true;
}

inline bool nearPoint2D(const Vec3& a, const Vec3& b, float r){
    Vec3 d = a - b; d.y = 0;
    return sqrtf(d.x*d.x + d.z*d.z) < r;
}

inline float gRenderViewYaw = 0.0f;
inline float gRenderViewPitch = 0.0f;

inline bool hasLabelLineOfSight(const Vec3& worldPos){
    Vec3 d = worldPos - cam.pos;
    d.y = 0.0f;
    float dist = d.len();
    if(dist <= 0.6f) return true;
    float inv = 1.0f / dist;
    Vec3 dir(d.x * inv, 0.0f, d.z * inv);
    float t = 0.6f;
    while(t < dist - 0.35f){
        Vec3 p = cam.pos + dir * t;
        if(collideWorld(p.x, p.z, 0.08f)) return false;
        t += 0.35f;
    }
    return true;
}

inline bool projectToScreen(const Vec3& worldPos, float& sx, float& sy){
    Vec3 d = worldPos - cam.pos;
    float cy = mCos(gRenderViewYaw), syaw = mSin(gRenderViewYaw);
    float cp = mCos(gRenderViewPitch), sp = mSin(gRenderViewPitch);

    // Screen right is forward x up = (-cos yaw, 0, sin yaw); (cos, 0, -sin) is the strafe-left axis.
    float cx = d.z * syaw - d.x * cy;
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

// Squad-wide status line: the host mirrors it to every client (see captureVoidShiftState).
inline int gStatusSeq = 0;

inline void setTrapStatus(const char* msg){
    snprintf(trapStatusText, sizeof(trapStatusText), "%s", msg);
    trapStatusTimer = 4.0f;
    gStatusSeq++;
}

inline void tickStatusTimers(float dt){
    if(trapStatusTimer > 0.0f) trapStatusTimer -= dt;
    if(echoStatusTimer > 0.0f) echoStatusTimer -= dt;
}

// Co-op run flow. A dead co-op player stays in the session as a ghost until the next level.
inline bool gCoopDead = false;
inline bool gRunComplete = false;
inline bool gHostExitRequested = false;

// Shift report counters restart whenever a run (re)starts from the first level.
inline void resetShiftStats(){
    for(int i=0;i<MAX_PLAYERS;i++) netMgr.squadStats[i] = {};
    netMgr.fxQueueCount = 0;
}

inline void resetCoopRunState(){
    gCoopDead = false;
    gRunComplete = false;
    gHostExitRequested = false;
    netMgr.localFlags = 0;
    netMgr.localReviveTarget = 0;
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


inline void triggerConferenceCallSetpiece(float duration);
inline void triggerCorridorShiftSetpiece(float duration);
inline void triggerBlackoutSetpiece(float duration);
inline void initVoidShiftSetpieces();
inline void updateVoidShiftSetpieces(float dt);
inline void reopenObjectiveCells();
inline bool isFallCell(int wx, int wz);

#include "heist_state.h"
#include "void_shift_runtime.h"
#include "void_shift_setpieces_ext.h"
#include "objective_layout.h"
#include "heist_sets.h"

inline bool isStoryExitReady(){
    return heistDrillReady();
}
