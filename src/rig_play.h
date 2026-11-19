#pragma once
// Rig, local side. Runs on every peer each frame: moves the rig along its lane (clients
// predict, the host corrects), carries whoever stands on it, keeps bodies out of the hull,
// tells the monsters where the haven is, and turns its moods into things you see and hear.
// Included from heist_play.h.
#include "player_legs.h"

inline constexpr float RIG_CABIN_SANITY_PER_SEC = 5.0f;  // sitting in the break room
inline constexpr float RIG_HAVEN_SANITY_PER_SEC = 2.4f;  // standing in its light
inline constexpr float RIG_CABIN_HEAL_PER_SEC = 1.2f;
inline constexpr float RIG_CABIN_HEAL_CAP = 80.0f;
inline constexpr float RIG_CABIN_LIGHT_PER_SEC = 3.0f;   // flashlight on the cabin socket
inline constexpr float RIG_RADAR_RANGE = 34.0f;
inline constexpr float RIG_ENGINE_HEAR_RANGE = 36.0f;

inline bool gRigSheltered = false; // local player is inside the haven
inline bool gRigInCabin = false;   // local player is in the break room
inline bool gRigAtRadar = false;   // ...and the radar has power

inline bool rigLocalPlayerOnFoot() {
    return !gCoopDead && !isPlayerDead && !playerFalling && !debugTools.flyMode;
}

inline void rigHurtByAuger() {
    if (gDownedGraceTimer > 0.0f) return;
    playerHealth -= RIG_AUGER_DPS * dTime;
    if (damageFlash < 0.35f) damageFlash = 0.35f;
    if (camShake < 0.25f) camShake = 0.25f;
    // thrown clear, the way the rig is going
    gKnockbackVel = Vec3((float)rigDirX(heist.rigDir), 0.0f, (float)rigDirZ(heist.rigDir)) * 5.0f;
    if (playerHealth > 0.0f) return;
    if (playerDowned) {
        killLocalPlayer("CAUSE: STOOD IN FRONT OF THE DRILL");
        return;
    }
    downLocalPlayer("DOWNED: STOOD IN FRONT OF THE DRILL");
    playerHealth = 15.0f;
    setEchoStatus("DOWNED: THE POINTY END IS THE FRONT");
}

// The rig moved by `delta` since the last frame. Whoever stood on the floor plate goes with
// it; whoever is in the way of the hull is shouldered aside.
inline void rigMoveLocalPlayer(const Vec3& delta) {
    if (!rigLocalPlayerOnFoot()) return;
    RigFrame before;
    before.x = gRigPrevPos.x;
    before.z = gRigPrevPos.z;
    before.dir = heist.rigDir;
    float lx = 0.0f, lz = 0.0f;
    rigToLocal(before, cam.pos.x, cam.pos.z, lx, lz);
    if (rigOnBoard(lx, lz)) {
        cam.pos.x += delta.x;
        cam.pos.z += delta.z;
        legsCarry(delta); // the feet stand on the deck
    }

    RigPoint p = rigLocal(cam.pos);
    int hit = rigPushOut(p.x, p.z, PR);
    if (hit < 0) return;
    Vec3 to = rigWorld(p.x, p.z);
    if (!collideWorld(to.x, to.z, PR)) {
        cam.pos.x = to.x;
        cam.pos.z = to.z;
    }
    if (hit == RIG_BOX_AUGER && rigMoving()) rigHurtByAuger();
}

// Crawl. The host is the one that counts; clients run the same clock between snapshots.
inline void rigAdvance() {
    bool sameRig = gRigPrevDir == heist.rigDir && nearPoint2D(gRigPrevPos, heist.drillPos, 2.0f);
    if (rigMoving()) {
        heist.drillPos.x += (float)rigDirX(heist.rigDir) * RIG_SPEED * dTime;
        heist.drillPos.z += (float)rigDirZ(heist.rigDir) * RIG_SPEED * dTime;
    }
    if (!heistIsAuthority() && heist.rigFault != RIG_OK) heist.rigFaultSecs += dTime;
    // a new floor or a correction from the host is a jump, not a ride
    if (sameRig) rigMoveLocalPlayer(heist.drillPos - gRigPrevPos);
    gRigPrevPos = heist.drillPos;
    gRigPrevDir = heist.rigDir;
    rigCarveLane(); // the mesh follows through the heist model signature
}

// The break room is where the crew gets its breath back.
inline void rigApplyComfort() {
    bool alive = !gCoopDead && !isPlayerDead;
    gRigInCabin = alive && rigPosInCabin(cam.pos);
    gRigSheltered = alive && rigPosInHaven(cam.pos);
    gRigAtRadar = gRigInCabin && rigLampLit() && heist.stage == HEIST_RUN;
    if (!gRigSheltered) return;
    playerSanity += (gRigInCabin ? RIG_CABIN_SANITY_PER_SEC : RIG_HAVEN_SANITY_PER_SEC) * dTime;
    if (playerSanity > 100.0f) playerSanity = 100.0f;
    if (!gRigInCabin || !rigEngineOn()) return;
    if (!playerDowned && playerHealth < RIG_CABIN_HEAL_CAP) playerHealth += RIG_CABIN_HEAL_PER_SEC * dTime;
    if (!flashlightOn && flashlightBattery < 100.0f) flashlightBattery += RIG_CABIN_LIGHT_PER_SEC * dTime;
}

// In the haven the floor stops getting at you: no jump scares, no growling from down the
// hall, and the noise meter runs down instead of up.
inline void rigCalmSenses() {
    gScaresMuted = gRigSheltered;
    if (!gRigSheltered) return;
    sndState.monsterProximity = 0.0f;
    sndState.monsterMenace = 0.0f;
    sndState.monsterAlert = 0.0f;
    sndState.monsterCrowd = 0.0f;
    sndState.monsterCall = 0;
    if (!heistIsAuthority()) return;
    attentionLevel -= 8.0f * dTime;
    if (attentionLevel < 0.0f) attentionLevel = 0.0f;
}

inline float rigLoudnessHere() {
    Vec3 d = heist.drillPos - cam.pos;
    return squadSoundFalloff(sqrtf(d.x * d.x + d.z * d.z) * (SQUAD_SOUND_RANGE / RIG_ENGINE_HEAR_RANGE));
}

// The engine is a two-stroke somebody found: putt, putt, putt. You hear it from across the
// floor, and you hear it stop.
inline void rigPuttPutt() {
    static float beat = 0.0f;
    if (!rigEngineOn() || heist.stage == HEIST_DRILLING) return; // drilling has its own rumble
    beat -= dTime;
    if (beat > 0.0f) return;
    bool pulling = rigMoving();
    beat = pulling ? 0.42f : 0.74f;
    float vol = (pulling ? 0.26f : 0.15f) * rigLoudnessHere();
    if (gRigInCabin) vol *= 0.6f; // the walls take the edge off
    triggerToySound(TOY_SHOVE, vol, pulling ? 0.36f : 0.30f);
}

// Runs on every peer: a breakdown, a repair, the lamp dying, a backfire, a wall being eaten.
inline void rigWatchMoods() {
    static int lastFault = RIG_OK;
    static float lastSecs = 0.0f;
    static int lastLevel = -1;
    static unsigned int lastSeed = 0;
    bool fresh = lastLevel != gCurrentLevel || lastSeed != worldSeed;
    lastLevel = gCurrentLevel;
    lastSeed = worldSeed;
    float loud = rigLoudnessHere();

    if (gRigCrunch > 0) {
        gRigCrunch = 0;
        if (!fresh && loud > 0.02f) {
            triggerToySound(TOY_SHOVE, 0.9f * loud, 0.32f);
            if (camShake < 0.45f * loud) camShake = 0.45f * loud;
        }
    }
    if (!fresh && heist.rigFault != lastFault) {
        if (heist.rigFault != RIG_OK) {
            heistToast(HUD_ICON_WRENCH, 0, false);
            triggerToySound(TOY_SHOVE, 0.4f + 0.6f * loud, 0.42f);
            if (camShake < 0.5f * loud) camShake = 0.5f * loud;
        } else {
            heistToast(HUD_ICON_WRENCH, 0, true);
            triggerToySound(TOY_RESPAWN, 0.35f + 0.4f * loud, 0.8f);
        }
    }
    if (!fresh && heist.rigFault != RIG_OK && heist.rigFaultSecs >= RIG_RESERVE_SECONDS) {
        float since = heist.rigFaultSecs - RIG_RESERVE_SECONDS;
        bool justDied = lastSecs < RIG_RESERVE_SECONDS;
        bool bang = (int)(since / RIG_BACKFIRE_SECONDS) != (int)((lastSecs - RIG_RESERVE_SECONDS) / RIG_BACKFIRE_SECONDS);
        if (justDied && heistIsAuthority()) setTrapStatus("THE SPARE BATTERY IS DEAD. NOTHING ABOUT THE DRILL IS SAFE NOW");
        if (justDied || bang) {
            triggerToySound(TOY_SHOVE, 0.3f + 0.7f * loud, 0.24f);
            if (camShake < 0.35f * loud) camShake = 0.35f * loud;
        }
    }
    lastFault = heist.rigFault;
    lastSecs = heist.rigFaultSecs;
}

inline bool rigAiCollide(float x, float z, float r) {
    return coop.initialized && rigCollide(x, z, r);
}

inline void rigUpdate() {
    rigAdvance();
    entityMgr.havenPos = rigHavenPos();
    entityMgr.havenRadius = rigHavenNow();
    entityMgr.quietRadius = rigQuietNow();
    gAiExtraCollide = rigAiCollide;
    rigApplyComfort();
    rigCalmSenses();
    rigWatchMoods();
    rigPuttPutt();
}
