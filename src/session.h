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
#include "entity_ai.h"
#include "trap_events.h"
#include "debug_tools.h"

GLuint noteVAO=0, noteVBO=0;
int noteVC=0;
GLuint deviceVAO=0, deviceVBO=0;
int deviceVC=0;
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
int activeDeviceSlot = 1; // 1 flashlight, 2 scanner, 3 battery
float scannerSignal = 0.0f;
EchoSignal echoSignal = {};
float echoSpawnTimer = 14.0f;
float echoStatusTimer = 0.0f;
char echoStatusText[96] = {};
bool storyEchoAttuned = false;
int storyEchoAttunedCount = 0;
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

inline int storyNotesRequired(){
    return 5;
}

inline bool isStoryExitReady(){
    return storyMgr.totalCollected >= storyNotesRequired() && (storyEchoAttuned || trapCorridor.resolved);
}
