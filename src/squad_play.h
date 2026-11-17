#pragma once
// Squad banter gameplay: shove teammates, squeak, shout callouts, come back from the dead
// when the squad makes progress, and keep the counters for the end-of-shift report.

#include "player_vitals.h"
#include "squad_fx.h"
#include "audio_toy.h"

inline Vec3 gKnockbackVel(0.0f, 0.0f, 0.0f);
inline float gShoveCooldown = 0.0f;
inline float gSqueakCooldown = 0.0f;
inline float gStatsSendTimer = 0.0f;
inline bool gSquadHintShown = false;

inline ShiftStats& myShiftStats() {
    return netMgr.squadStats[netMgr.myId];
}

inline const char* squadPlayerName(int p) {
    if (p < 0 || p >= MAX_PLAYERS || !netMgr.players[p].name[0]) return "TEAMMATE";
    return netMgr.players[p].name;
}

inline void sendSquadFxEvent(int kind, int arg, const Vec3& v) {
    if (multiState != MULTI_IN_GAME) return;
    SquadFxEvent fx = {(unsigned char)netMgr.myId, (unsigned char)kind, (unsigned char)arg, v};
    netMgr.sendSquadFx(fx, myShiftStats());
}

// Progress on this floor: gauge lamps on the drill and pallet deliveries. Any step forward
// brings ghosts back into the shift.
inline int squadObjectiveSteps() {
    return heistDrillLamps() + heist.tutDelivered;
}

inline void respawnFromGhost() {
    Vec3 at = netMgr.spawnPos;
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (!isRemotePlayerStanding(p) || !netMgr.players[p].hasValidPos) continue;
        at = netMgr.players[p].pos;
        break;
    }
    if (heist.stage == HEIST_RUN) at = rigWorld(rigSeat(netMgr.myId)); // back on the clock in the break room
    gCoopDead = false;
    playerDowned = false;
    playerHealth = 60.0f;
    playerSanity = 70.0f;
    playerStamina = 125.0f;
    gDownedGraceTimer = DOWNED_GRACE_SECONDS;
    gKnockbackVel = Vec3(0.0f, 0.0f, 0.0f);
    cam.pos = Vec3(at.x, PH, at.z);
    setEchoStatus("DRILL LAMP LIT. BREAK IS OVER, YOU ARE BACK IN");
    triggerToySound(TOY_RESPAWN, 0.8f);
}

inline void updateGhostRespawn() {
    static int baselineSteps = 0;
    static int baselineLevel = -1;
    static unsigned int baselineSeed = 0;
    int steps = squadObjectiveSteps();
    bool sameLevel = baselineLevel == gCurrentLevel && baselineSeed == worldSeed;
    bool progressed = sameLevel && steps > baselineSteps;
    baselineSteps = steps;
    baselineLevel = gCurrentLevel;
    baselineSeed = worldSeed;
    if (progressed && gCoopDead && multiState == MULTI_IN_GAME) respawnFromGhost();
}

inline int findShoveTarget() {
    int best = -1;
    float bestDist = SHOVE_RANGE + 1.0f;
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
        if ((netMgr.players[p].flags & PLAYER_FLAG_DEAD) != 0) continue;
        if (!isShoveHit(cam.pos, cam.yaw, netMgr.players[p].pos)) continue;
        Vec3 d = netMgr.players[p].pos - cam.pos;
        d.y = 0.0f;
        float dist = d.len();
        if (dist < bestDist) {
            bestDist = dist;
            best = p;
        }
    }
    return best;
}

inline void tryShove() {
    gShoveCooldown = SHOVE_COOLDOWN;
    int target = findShoveTarget();
    if (target < 0) return;
    Vec3 dir(mSin(cam.yaw), 0.0f, mCos(cam.yaw));
    sendSquadFxEvent(SQUAD_FX_SHOVE, target, dir);
    bumpStat(myShiftStats().shoves);
    emitSquadNoise(netMgr.myId, NOISE_SHOVE);
    triggerToySound(TOY_SHOVE, 0.8f);
    if (camShake < 0.04f) camShake = 0.04f;
    char buf[96];
    snprintf(buf, sizeof(buf), "YOU SHOVED %s", squadPlayerName(target));
    setEchoStatus(buf);
}

inline void doSqueak() {
    gSqueakCooldown = SQUEAK_COOLDOWN;
    float pitch = 0.85f + (float)(rng() % 45) / 100.0f;
    triggerToySound(TOY_SQUEAK, 0.9f, pitch);
    sendSquadFxEvent(SQUAD_FX_SQUEAK, 0, Vec3(pitch, 0.0f, 0.0f));
    bumpStat(myShiftStats().squeaks);
    // Noise is never free down here: anything blind comes to check, and ghosts squeak too.
    emitSquadNoise(netMgr.myId, NOISE_SQUEAK);
    if (multiState != MULTI_IN_GAME || netMgr.isHost) addAttention(1.5f);
}

inline void doCallout(int idx) {
    char buf[96];
    snprintf(buf, sizeof(buf), "YOU: %s", squadCalloutLabel(idx));
    setSquadCallout(buf);
    triggerToySound(TOY_CALLOUT, 0.5f);
    emitSquadNoise(netMgr.myId, NOISE_CALLOUT);
    sendSquadFxEvent(SQUAD_FX_CALLOUT, idx, cam.pos);
}

inline float squadSoundVolumeFrom(int p) {
    if (p < 0 || p >= MAX_PLAYERS || !netMgr.players[p].hasValidPos) return 0.0f;
    Vec3 d = netMgr.players[p].pos - cam.pos;
    d.y = 0.0f;
    return squadSoundFalloff(d.len());
}

inline void applySquadFx(const SquadFxEvent& fx) {
    float vol = squadSoundVolumeFrom(fx.from);
    char buf[96];
    if (fx.kind == SQUAD_FX_SHOVE) emitSquadNoise(fx.from, NOISE_SHOVE);
    else if (fx.kind == SQUAD_FX_SQUEAK) emitSquadNoise(fx.from, NOISE_SQUEAK);
    else if (fx.kind == SQUAD_FX_CALLOUT) emitSquadNoise(fx.from, NOISE_CALLOUT);

    if (fx.kind == SQUAD_FX_SHOVE) {
        if (fx.arg != netMgr.myId || gCoopDead) {
            triggerToySound(TOY_SHOVE, vol * 0.6f);
            return;
        }
        Vec3 dir(fx.v.x, 0.0f, fx.v.z);
        float len = dir.len();
        if (len < 0.01f) return;
        gKnockbackVel = dir * (SHOVE_SPEED / len);
        if (camShake < 0.12f) camShake = 0.12f;
        triggerToySound(TOY_SHOVE, 1.0f, 0.8f);
        snprintf(buf, sizeof(buf), "%s SHOVED YOU", squadPlayerName(fx.from));
        setEchoStatus(buf);
    } else if (fx.kind == SQUAD_FX_SQUEAK) {
        float pitch = fx.v.x < 0.5f ? 0.5f : (fx.v.x > 1.6f ? 1.6f : fx.v.x);
        triggerToySound(TOY_SQUEAK, vol * 0.9f, pitch);
        if (netMgr.isHost) addAttention(1.5f);
    } else if (fx.kind == SQUAD_FX_CALLOUT) {
        snprintf(buf, sizeof(buf), "%s: %s", squadPlayerName(fx.from), squadCalloutLabel(fx.arg));
        setSquadCallout(buf);
        triggerToySound(TOY_CALLOUT, 0.25f + vol * 0.35f);
    }
}

inline void processSquadFxQueue() {
    for (int i = 0; i < netMgr.fxQueueCount; i++) applySquadFx(netMgr.fxQueue[i]);
    netMgr.fxQueueCount = 0;
}

// Shoves move the player like normal walking does: walls, the locked exit and props stop it.
inline void applySquadKnockback() {
    if (gKnockbackVel.x == 0.0f && gKnockbackVel.z == 0.0f) return;
    Vec3 delta = knockbackStep(gKnockbackVel, dTime);
    if (playerFalling || debugTools.flyMode) return;
    float nx = cam.pos.x + delta.x;
    float nz = cam.pos.z + delta.z;
    if (!collideWorld(nx, cam.pos.z, PR) && !collideCoopDoor(nx, cam.pos.z, PR) && !collideMapProps(nx, cam.pos.z, PR) && !collideHeistSolids(nx, cam.pos.z, PR)) cam.pos.x = nx;
    else gKnockbackVel.x = 0.0f;
    if (!collideWorld(cam.pos.x, nz, PR) && !collideCoopDoor(cam.pos.x, nz, PR) && !collideMapProps(cam.pos.x, nz, PR) && !collideHeistSolids(cam.pos.x, nz, PR)) cam.pos.z = nz;
    else gKnockbackVel.z = 0.0f;
}

inline void updateShiftStats() {
    static Vec3 lastPos(0.0f, 0.0f, 0.0f);
    static float meterCarry = 0.0f;
    static bool wasDowned = false;
    static int reviveWatch = -1;
    static float reviveWatchTimer = 0.0f;

    Vec3 d = cam.pos - lastPos;
    d.y = 0.0f;
    float moved = d.len();
    lastPos = cam.pos;
    // Teleports and respawns are not legwork.
    if (moved < 2.0f && !gCoopDead) meterCarry += moved;
    if (meterCarry >= 1.0f) {
        int whole = (int)meterCarry;
        bumpStat(myShiftStats().meters, whole);
        meterCarry -= (float)whole;
    }

    if (playerDowned && !wasDowned) bumpStat(myShiftStats().downs);
    wasDowned = playerDowned;

    // A revive counts once the teammate we were working on is back on their feet.
    if (netMgr.localReviveTarget != 0) {
        reviveWatch = netMgr.localReviveTarget - 1;
        reviveWatchTimer = 1.0f;
    } else if (reviveWatch >= 0) {
        reviveWatchTimer -= dTime;
        unsigned char flags = netMgr.players[reviveWatch].flags;
        if ((flags & (PLAYER_FLAG_DOWNED | PLAYER_FLAG_DEAD)) == 0) {
            bumpStat(myShiftStats().revives);
            reviveWatch = -1;
        } else if (reviveWatchTimer <= 0.0f) {
            reviveWatch = -1;
        }
    }

    if (multiState != MULTI_IN_GAME) return;
    gStatsSendTimer -= dTime;
    if (gStatsSendTimer > 0.0f) return;
    gStatsSendTimer = 1.0f;
    sendSquadFxEvent(SQUAD_FX_STATS, 0, Vec3(0.0f, 0.0f, 0.0f));
}

// Called from gameInput every gameplay frame (ghosts included).
inline void updateSquadPlay(GLFWwindow* w) {
    static bool shovePressed = false, squeakPressed = false, calloutPressed = false;
    if (gShoveCooldown > 0.0f) gShoveCooldown -= dTime;
    if (gSqueakCooldown > 0.0f) gSqueakCooldown -= dTime;

    processSquadFxQueue();
    updateGhostRespawn();
    if (gCrawlerSlamVel.x != 0.0f || gCrawlerSlamVel.z != 0.0f) {
        // Bowled over by a charging crawler: same physics as a shove, just less friendly.
        gKnockbackVel = gCrawlerSlamVel;
        gCrawlerSlamVel = Vec3(0.0f, 0.0f, 0.0f);
    }
    applySquadKnockback();
    updateShiftStats();

    bool coopGame = multiState == MULTI_IN_GAME;
    if (coopGame && !gSquadHintShown && survivalTime > 6.0f) {
        gSquadHintShown = true;
        setEchoStatus("SQUAD: [LMB] SHOVE  [Q] SQUEAK  [G] PING  [T/Y/U/I/O] CALLOUTS");
    }

    bool shoveNow = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool canAct = !gCoopDead && !playerDowned && !playerFalling;
    if (coopGame && canAct && shoveNow && !shovePressed && gShoveCooldown <= 0.0f) tryShove();
    shovePressed = shoveNow;

    // Ghosts keep their squeak: haunting the living is half the fun of being dead.
    bool squeakNow = glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS;
    if (squeakNow && !squeakPressed && gSqueakCooldown <= 0.0f) doSqueak();
    squeakPressed = squeakNow;

    static const int calloutKeys[SQUAD_CALLOUT_COUNT] = {GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I, GLFW_KEY_O};
    int callout = -1;
    for (int i = 0; i < SQUAD_CALLOUT_COUNT; i++) {
        if (glfwGetKey(w, calloutKeys[i]) == GLFW_PRESS) callout = i;
    }
    if (callout >= 0 && !calloutPressed) doCallout(callout);
    calloutPressed = callout >= 0;
}
