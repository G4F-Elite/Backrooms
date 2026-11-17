#pragma once
// Spectator camera for dead co-op players. Instead of wandering the level alone, a ghost
// rides along with a teammate (third person orbit or their bodycam) or with a monster,
// and can still squeak from wherever it sits. The ghost's real position follows the perch,
// so the world streams in around the target and teammates see a tiny ghost on their shoulder.

#include "spectator_math.h"
#include "entity_behaviors.h"

struct SpectatorState {
    bool riding = false;    // attached to a target; false = free-walking ghost
    bool freeRoam = false;  // the player asked to walk around instead
    bool bodycam = false;   // first person through a teammate's eyes
    int kind = SPEC_PLAYER;
    int id = -1;
    float yaw = 0.0f;
    float pitch = -0.28f;
    float dist = SPECTATOR_DEFAULT_DIST;
    float distNow = SPECTATOR_DEFAULT_DIST;
    float deadTimer = 0.0f;

    // View override: the camera the scene is drawn from while riding.
    bool viewActive = false;
    Vec3 savedPos;
    float savedYaw = 0.0f, savedPitch = 0.0f;
    int savedSlot = 0;
    bool savedFlashlight = false;
};

inline SpectatorState gSpectator;

inline constexpr float SPECTATOR_AUTO_DELAY = 1.2f; // a beat to read "YOU DIED" before the cut

inline float spectatorMonsterTop(EntityType type) {
    if (type == ENTITY_CRAWLER) return 1.05f;
    if (type == ENTITY_SHADOW) return 2.05f;
    return 2.0f;
}

inline const Entity* spectatorFindMonster(int uid) {
    for (const auto& e : entityMgr.entities) {
        if (e.active && e.uid == (unsigned char)uid) return &e;
    }
    return nullptr;
}

inline bool spectatorPlayerValid(int p) {
    if (p < 0 || p >= MAX_PLAYERS || p == netMgr.myId) return false;
    if (!netMgr.players[p].active || !netMgr.players[p].hasValidPos || !playerInterpReady[p]) return false;
    return (netMgr.players[p].flags & PLAYER_FLAG_DEAD) == 0;
}

inline bool spectatorTargetValid(int kind, int id) {
    if (kind == SPEC_PLAYER) return spectatorPlayerValid(id);
    return spectatorFindMonster(id) != nullptr;
}

// Teammates first (standing before downed), then the monsters.
inline int spectatorBuildSlots(SpectatorSlot* out) {
    int n = 0;
    for (int pass = 0; pass < 2; pass++) {
        for (int p = 0; p < MAX_PLAYERS && n < SPECTATOR_MAX_SLOTS; p++) {
            if (!spectatorPlayerValid(p)) continue;
            bool downed = (netMgr.players[p].flags & PLAYER_FLAG_DOWNED) != 0;
            if (downed == (pass == 1)) out[n++] = {SPEC_PLAYER, p};
        }
    }
    for (const auto& e : entityMgr.entities) {
        if (e.active && n < SPECTATOR_MAX_SLOTS) out[n++] = {SPEC_MONSTER, (int)e.uid};
    }
    return n;
}

// Point the camera looks at, plus the direction the target itself faces.
inline bool spectatorFocus(int kind, int id, Vec3& focus, float& targetYaw, float& targetPitch) {
    if (kind == SPEC_PLAYER) {
        if (!spectatorPlayerValid(id)) return false;
        focus = playerRenderPos[id];
        targetYaw = playerRenderYaw[id];
        targetPitch = playerRenderPitch[id];
        return true;
    }
    const Entity* e = spectatorFindMonster(id);
    if (!e) return false;
    focus = Vec3(e->pos.x, spectatorMonsterTop(e->type) * 0.8f, e->pos.z);
    targetYaw = e->yaw;
    targetPitch = 0.0f;
    return true;
}

inline void spectatorAttach(int kind, int id) {
    Vec3 focus;
    float tYaw = 0.0f, tPitch = 0.0f;
    if (!spectatorFocus(kind, id, focus, tYaw, tPitch)) return;
    bool wasRiding = gSpectator.riding;
    gSpectator.riding = true;
    gSpectator.freeRoam = false;
    gSpectator.kind = kind;
    gSpectator.id = id;
    if (kind == SPEC_MONSTER) gSpectator.bodycam = false;
    if (!wasRiding) {
        // Start behind the target, looking the way it looks.
        gSpectator.yaw = tYaw;
        gSpectator.pitch = -0.28f;
        gSpectator.distNow = SPECTATOR_MIN_DIST;
    }
    flashlightOn = false;
    sndState.flashlightOn = 0.0f;
}

inline void spectatorDetach(bool toFreeRoam) {
    if (gSpectator.riding) {
        // Drop the ghost where its ride was, on its own feet again.
        Vec3 focus;
        float tYaw = 0.0f, tPitch = 0.0f;
        if (spectatorFocus(gSpectator.kind, gSpectator.id, focus, tYaw, tPitch)) cam.pos = Vec3(focus.x, PH, focus.z);
        else cam.pos.y = PH;
        cam.yaw = gSpectator.yaw;
        cam.pitch = 0.0f;
    }
    gSpectator.riding = false;
    gSpectator.freeRoam = toFreeRoam;
    gSpectator.bodycam = false;
}

inline void resetSpectator() {
    gSpectator = SpectatorState();
}

inline bool spectatorRiding() {
    return gSpectator.riding;
}

inline void spectatorCycleTarget(int dir) {
    SpectatorSlot slots[SPECTATOR_MAX_SLOTS];
    int count = spectatorBuildSlots(slots);
    int current = gSpectator.riding ? spectatorFindSlot(slots, count, gSpectator.kind, gSpectator.id) : -1;
    int next = spectatorCycle(current, count, dir);
    if (next >= 0) spectatorAttach(slots[next].kind, slots[next].id);
}

// Closest standing teammate, else anybody alive, else a monster.
inline void spectatorAttachDefault() {
    SpectatorSlot slots[SPECTATOR_MAX_SLOTS];
    int count = spectatorBuildSlots(slots);
    int best = -1;
    float bestDist = 1e9f;
    for (int i = 0; i < count; i++) {
        if (slots[i].kind != SPEC_PLAYER) continue;
        if ((netMgr.players[slots[i].id].flags & PLAYER_FLAG_DOWNED) != 0) continue;
        Vec3 d = netMgr.players[slots[i].id].pos - cam.pos;
        d.y = 0.0f;
        float dist = d.len();
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    if (best < 0 && count > 0) best = 0;
    if (best >= 0) spectatorAttach(slots[best].kind, slots[best].id);
}

// Mouse look goes to the orbit camera while riding. Returns true when it used the motion.
inline bool spectatorMouseLook(float dx, float dy, float sens) {
    if (!gSpectator.riding) return false;
    if (gSpectator.bodycam) return true; // the teammate does the looking
    gSpectator.yaw -= dx * sens;
    gSpectator.pitch = spectatorClampPitch(gSpectator.pitch + dy * sens);
    return true;
}

// Called from gameInput every gameplay frame.
inline void updateSpectator(GLFWwindow* w) {
    static bool prevPressed = false, nextPressed = false, freePressed = false, viewPressed = false;
    if (!gCoopDead || multiState != MULTI_IN_GAME) {
        if (gSpectator.riding) {
            // Back among the living (the respawn already placed us): keep facing where we looked.
            cam.yaw = gSpectator.yaw;
            cam.pitch = 0.0f;
        }
        if (gSpectator.riding || gSpectator.freeRoam || gSpectator.deadTimer > 0.0f) resetSpectator();
        return;
    }
    gSpectator.deadTimer += dTime;

    bool prevNow = glfwGetKey(w, settings.binds.left) == GLFW_PRESS || glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool nextNow = glfwGetKey(w, settings.binds.right) == GLFW_PRESS || glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    bool freeNow = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool viewNow = glfwGetKey(w, GLFW_KEY_V) == GLFW_PRESS;
    if (gSpectator.riding) {
        if (prevNow && !prevPressed) spectatorCycleTarget(-1);
        if (nextNow && !nextPressed) spectatorCycleTarget(1);
        if (viewNow && !viewPressed && gSpectator.kind == SPEC_PLAYER) gSpectator.bodycam = !gSpectator.bodycam;
        if (glfwGetKey(w, settings.binds.forward) == GLFW_PRESS) gSpectator.dist = spectatorClampDist(gSpectator.dist - 3.0f * dTime);
        if (glfwGetKey(w, settings.binds.back) == GLFW_PRESS) gSpectator.dist = spectatorClampDist(gSpectator.dist + 3.0f * dTime);
    }
    if (freeNow && !freePressed) {
        if (gSpectator.riding) spectatorDetach(true);
        else spectatorAttachDefault();
    }
    prevPressed = prevNow;
    nextPressed = nextNow;
    freePressed = freeNow;
    viewPressed = viewNow;

    if (!gSpectator.riding && !gSpectator.freeRoam && gSpectator.deadTimer >= SPECTATOR_AUTO_DELAY) spectatorAttachDefault();
    if (!gSpectator.riding) return;

    // The ride died, left or despawned: hop to the next one, or back on foot.
    if (!spectatorTargetValid(gSpectator.kind, gSpectator.id)) {
        gSpectator.riding = false;
        spectatorAttachDefault();
        if (!gSpectator.riding) {
            cam.pos.y = PH;
            return;
        }
    }

    Vec3 focus;
    float tYaw = 0.0f, tPitch = 0.0f;
    spectatorFocus(gSpectator.kind, gSpectator.id, focus, tYaw, tPitch);
    bool monster = gSpectator.kind == SPEC_MONSTER;
    float top = 0.0f;
    if (monster) {
        const Entity* e = spectatorFindMonster(gSpectator.id);
        top = e ? spectatorMonsterTop(e->type) : 2.0f;
    }
    cam.pos = spectatorPerch(focus, tYaw, monster, top);
    cam.crouch = false;

    float freeDist = spectatorFreeDistance(focus, gSpectator.yaw, gSpectator.pitch, gSpectator.dist, CS, WH, getCellWorld);
    gSpectator.distNow = spectatorSmoothDist(gSpectator.distNow, freeDist, dTime);
}

// Swap the gameplay camera for the spectator view around rendering + HUD, then put it back:
// everything that draws reads `cam`, everything that simulates keeps the perch position.
inline void spectatorBeginView() {
    gHiddenPlayerModel = -1;
    if (!gSpectator.riding || !gCoopDead) return;
    Vec3 focus;
    float tYaw = 0.0f, tPitch = 0.0f;
    if (!spectatorFocus(gSpectator.kind, gSpectator.id, focus, tYaw, tPitch)) return;

    gSpectator.viewActive = true;
    gSpectator.savedPos = cam.pos;
    gSpectator.savedYaw = cam.yaw;
    gSpectator.savedPitch = cam.pitch;
    gSpectator.savedSlot = activeDeviceSlot;
    gSpectator.savedFlashlight = flashlightOn;
    activeDeviceSlot = 0; // no viewmodel hands floating in the shot
    flashlightOn = false;

    if (gSpectator.bodycam && gSpectator.kind == SPEC_PLAYER) {
        cam.pos = focus;
        cam.yaw = tYaw;
        cam.pitch = tPitch;
        gHiddenPlayerModel = gSpectator.id;
        return;
    }
    cam.pos = spectatorOrbitPos(focus, gSpectator.yaw, gSpectator.pitch, gSpectator.distNow);
    flashlightOn = true; // ghost light: only this viewer's render gets it, so the show is watchable
    // Squeezed against a wall: do not render the inside of the teammate's head.
    if (gSpectator.kind == SPEC_PLAYER && gSpectator.distNow < 0.7f) gHiddenPlayerModel = gSpectator.id;
    cam.yaw = gSpectator.yaw;
    cam.pitch = gSpectator.pitch;
}

inline void spectatorEndView() {
    gHiddenPlayerModel = -1;
    if (!gSpectator.viewActive) return;
    gSpectator.viewActive = false;
    cam.pos = gSpectator.savedPos;
    cam.yaw = gSpectator.savedYaw;
    cam.pitch = gSpectator.savedPitch;
    activeDeviceSlot = gSpectator.savedSlot;
    flashlightOn = gSpectator.savedFlashlight;
}
