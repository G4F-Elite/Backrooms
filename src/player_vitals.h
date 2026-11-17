#pragma once
// Player survival state: sanity and health drain, downed -> revive -> death,
// and the co-op ghost state (a dead co-op player keeps walking around until the next level).

#include "coop_flow.h"
#include "sanity_balance.h"
#include "monster_profiles.h"
#include "ai_targets.h"

inline constexpr float DOWNED_SECONDS = 60.0f;
inline constexpr float DOWNED_GRACE_SECONDS = 4.0f;
inline constexpr float REVIVE_RANGE = 2.8f;
inline constexpr float REVIVE_SECONDS = 3.0f;

inline float gSanityCollapseTimer = 0.0f;
inline float gDownedGraceTimer = 0.0f;
inline float gReviveProgress = 0.0f; // 0..1, teammates reviving the local player
inline int gReviveCandidate = -1;    // downed teammate in reach of the local player

inline void killLocalPlayer(const char* reason) {
    snprintf(gDeathReason, sizeof(gDeathReason), "%s", reason);
    playerDowned = false;
    playerDownedTimer = 0.0f;
    playerHealth = 0.0f;
    if (multiState == MULTI_IN_GAME) {
        gCoopDead = true;
        setEchoStatus("YOU ARE DEAD. BACK ON THE CLOCK AT THE NEXT DRILL LAMP");
        return;
    }
    isPlayerDead = true;
    glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

inline void downLocalPlayer(const char* reason) {
    playerDowned = true;
    playerDownedTimer = DOWNED_SECONDS;
    gDownedGraceTimer = DOWNED_GRACE_SECONDS;
    gReviveProgress = 0.0f;
    snprintf(gDeathReason, sizeof(gDeathReason), "%s", reason);
}

inline void reviveLocalPlayer(const char* status) {
    playerDowned = false;
    playerDownedTimer = 0.0f;
    gReviveProgress = 0.0f;
    gSanityCollapseTimer = 0.0f;
    playerHealth = 45.0f;
    if (playerSanity < 35.0f) playerSanity = 35.0f;
    setEchoStatus(status);
}

// Deaths raised elsewhere (falls) end a solo run but only ghost a co-op player.
inline void convertDeathToGhost() {
    if (multiState != MULTI_IN_GAME || !isPlayerDead) return;
    isPlayerDead = false;
    playerFalling = false;
    cam.pos = Vec3(netMgr.spawnPos.x, PH, netMgr.spawnPos.z);
    cam.pitch = 0.0f;
    glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
    killLocalPlayer(gDeathReason);
}

inline void updateSanity() {
    float levelDanger = levelDangerScale(gCurrentLevel);
    bool holdingPlush = activeDeviceSlot == 3 && heldConsumableType == ITEM_PLUSH_TOY;
    if (holdingPlush) playerSanity += 3.8f * dTime;
    else playerSanity -= sanityPassiveDrainPerSec(levelDanger) * dTime;

    if (entityMgr.dangerLevel > 0.1f) playerSanity -= entityMgr.dangerLevel * (8.0f * levelDanger) * dTime;
    else playerSanity += sanityRecoveryPerSec(true, playerSanity) * dTime;
    if (playerSanity > 100.0f) playerSanity = 100.0f;
    if (playerSanity < 0.0f) playerSanity = 0.0f;

    if (playerSanity > 0.0f) {
        gSanityCollapseTimer -= dTime * 1.7f;
        if (gSanityCollapseTimer < 0.0f) gSanityCollapseTimer = 0.0f;
        return;
    }
    gSanityCollapseTimer += dTime;
    if (gSanityCollapseTimer < 2.3f) return;
    if (playerDowned) {
        killLocalPlayer("CAUSE: COGNITIVE OVERLOAD");
        return;
    }
    downLocalPlayer("DOWNED: COGNITIVE OVERLOAD");
    // Leave a sliver of sanity so the downed player does not collapse again on the next frame.
    playerSanity = 12.0f;
    gSanityCollapseTimer = 0.0f;
    setEchoStatus("DOWNED: MORALE HIT ZERO. SOMEBODY HUG THIS PERSON");
}

inline void updateFloorFall() {
    int cellX = (int)floorf(cam.pos.x / CS);
    int cellZ = (int)floorf(cam.pos.z / CS);
    if (playerFalling || !isFallCell(cellX, cellZ) || rigPosInCabin(cam.pos)) return; // the rig bridges holes
    float dx = cam.pos.x - (cellX + 0.5f) * CS;
    float dz = cam.pos.z - (cellZ + 0.5f) * CS;
    if (sqrtf(dx * dx + dz * dz) >= CS * 0.35f) return;
    playerFalling = true;
    fallVelocity = 0.0f;
    fallTimer = 0.0f;
    setEchoStatus(isAbyssCell(cellX, cellZ) ? "THAT PIT WAS NOT ON THE FLOOR PLAN" : "THE FLOOR QUIT. YOU WENT WITH IT");
}

inline void updateMonsterDamage() {
    EntityType attackerType = entityMgr.getAttackingType(cam.pos);
    if (attackerType == ENTITY_NONE || gDownedGraceTimer > 0.0f) return;

    MonsterAttackProfile attack = monsterAttackProfile(attackerType);
    playerHealth -= attack.healthDps * dTime;
    entityMgr.pendingDamage += attack.healthDps * dTime; // the director watches how rough it gets
    playerSanity -= attack.sanityDps * dTime;
    if (attack.hitShake > camShake) camShake = attack.hitShake;
    if (attack.hitFlash > damageFlash) damageFlash = attack.hitFlash;
    if (multiState == MULTI_IN_GAME) netMgr.sendScare(netMgr.myId);
    triggerMonsterHit();

    if (playerHealth > 0.0f) return;
    if (playerDowned) {
        killLocalPlayer("CAUSE: HOSTILE CONTACT");
        return;
    }
    downLocalPlayer("DOWNED: HOSTILE CONTACT");
    playerHealth = 15.0f;
    setEchoStatus("DOWNED: WORKPLACE INJURY. A COWORKER CAN PICK YOU UP");
}

inline bool isTeammateRevivingMe() {
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (!isRemotePlayerStanding(p) || !netMgr.players[p].hasValidPos) continue;
        if (netMgr.players[p].reviveTarget != netMgr.myId + 1) continue;
        if (nearPoint2D(netMgr.players[p].pos, cam.pos, REVIVE_RANGE + 1.0f)) return true;
    }
    return false;
}

inline int findReviveCandidate() {
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
        if ((netMgr.players[p].flags & PLAYER_FLAG_DOWNED) == 0) continue;
        if ((netMgr.players[p].flags & PLAYER_FLAG_DEAD) != 0) continue;
        if (nearPoint2D(netMgr.players[p].pos, cam.pos, REVIVE_RANGE)) return p;
    }
    return -1;
}

inline void updateDownedAndRevive() {
    gReviveCandidate = -1;
    netMgr.localReviveTarget = 0;
    if (gDownedGraceTimer > 0.0f) gDownedGraceTimer -= dTime;

    bool beingRevived = false;
    if (multiState == MULTI_IN_GAME) {
        if (playerDowned) {
            beingRevived = isTeammateRevivingMe();
        } else if (!gCoopDead) {
            gReviveCandidate = findReviveCandidate();
            bool holdingInteract = glfwGetKey(gWin, settings.binds.interact) == GLFW_PRESS;
            if (gReviveCandidate >= 0 && holdingInteract) netMgr.localReviveTarget = (unsigned char)(gReviveCandidate + 1);
        }
    }

    if (!playerDowned) {
        gReviveProgress = 0.0f;
        return;
    }
    if (beingRevived) {
        gReviveProgress += dTime / REVIVE_SECONDS;
        if (gReviveProgress >= 1.0f) reviveLocalPlayer("A COWORKER PICKED YOU UP. YOU OWE THEM ONE");
        return;
    }
    gReviveProgress -= dTime * 0.5f;
    if (gReviveProgress < 0.0f) gReviveProgress = 0.0f;

    playerDownedTimer -= dTime;
    playerStamina -= dTime * 14.0f;
    if (playerStamina < 0.0f) playerStamina = 0.0f;
    if (playerDownedTimer <= 0.0f) killLocalPlayer("CAUSE: BLEED OUT");
}

inline void updatePlayerVitals() {
    tickStatusTimers(dTime);
    convertDeathToGhost();

    if (gCoopDead) {
        // Ghosts take no damage and see clearly; monsters ignore them (see collectEntityTargets).
        playerHealth = 0.0f;
        playerSanity = 100.0f;
        gSanityCollapseTimer = 0.0f;
        gReviveCandidate = -1;
        netMgr.localReviveTarget = 0;
    } else {
        updateSanity();
        updateFloorFall();
        if (!isPlayerDead && !gCoopDead) updateMonsterDamage();
        if (!isPlayerDead && !gCoopDead) updateDownedAndRevive();
    }

    unsigned char flags = 0;
    if (playerDowned) flags |= PLAYER_FLAG_DOWNED;
    if (gCoopDead) flags |= PLAYER_FLAG_DEAD;
    netMgr.localFlags = flags;
    updateCoopFlowHost(dTime);
}

