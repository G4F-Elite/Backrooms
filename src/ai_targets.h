#pragma once
// Glue between the squad and the monster brains: who can be hunted and how loud they are,
// the noises players make on purpose, and the feedback monsters give back (rule tips,
// the crawler knocking itself out, the crawler bowling somebody over).
// Included from player_vitals.h.

#include "entity_behaviors.h"

struct SquadMotionTrack {
    Vec3 lastPos;
    float speed = 0.0f;
    bool valid = false;
};

inline SquadMotionTrack gSquadMotion[MAX_PLAYERS];
inline unsigned char gSquadPrevFlags[MAX_PLAYERS] = {};
inline bool gMonsterTipShown[4] = {};
inline float gCrawlerSlamCooldown = 0.0f;
inline Vec3 gCrawlerSlamVel(0.0f, 0.0f, 0.0f); // picked up by the squad knockback (squad_play.h)

inline constexpr float MONSTER_TIP_RANGE = 14.0f;
inline constexpr float CRAWLER_SLAM_SPEED = 9.5f;
inline constexpr float CRAWLER_SLAM_COOLDOWN = 1.6f;

inline void resetMonsterFeedback() {
    for (auto& t : gSquadMotion) t = SquadMotionTrack();
    for (auto& f : gSquadPrevFlags) f = 0;
    for (auto& s : gMonsterTipShown) s = false;
    gCrawlerSlamCooldown = 0.0f;
    gCrawlerSlamVel = Vec3(0.0f, 0.0f, 0.0f);
}

// Smoothed horizontal speed of a squad slot. The host only gets remote positions at the
// network rate, so the raw per-frame speed is spiky; the filter evens that out.
inline float trackSquadSpeed(int slot, const Vec3& pos, float dt) {
    SquadMotionTrack& tr = gSquadMotion[slot];
    if (tr.valid && dt > 1e-4f) {
        float dx = pos.x - tr.lastPos.x, dz = pos.z - tr.lastPos.z;
        float instant = sqrtf(dx * dx + dz * dz) / dt;
        if (instant > 40.0f) instant = 0.0f; // teleport / respawn, not footsteps
        float k = dt * 6.0f;
        if (k > 1.0f) k = 1.0f;
        tr.speed += (instant - tr.speed) * k;
    }
    tr.lastPos = pos;
    tr.valid = true;
    return tr.speed;
}

inline AiTarget makeLocalAiTarget() {
    int slot = (multiState == MULTI_IN_GAME) ? netMgr.myId : 0;
    AiTarget t;
    t.pos = cam.pos;
    t.yaw = cam.yaw;
    t.pitch = cam.pitch;
    t.flashlight = flashlightOn;
    t.crouched = cam.crouch;
    t.id = slot;
    t.noiseRadius = footstepNoiseRadius(trackSquadSpeed(slot, cam.pos, dTime), t.crouched, perkQuietSteps);
    return t;
}

inline AiTarget makeRemoteAiTarget(int p) {
    const auto& pl = netMgr.players[p];
    AiTarget t;
    t.pos = pl.pos;
    t.yaw = pl.yaw;
    t.pitch = pl.pitch;
    t.flashlight = pl.flashlightOn;
    t.crouched = pl.pos.y < (PH + PH_CROUCH) * 0.5f;
    t.id = p;
    t.noiseRadius = footstepNoiseRadius(trackSquadSpeed(p, pl.pos, dTime), t.crouched, false);
    return t;
}

// A deliberate sound (squeak, callout, shove, ping) made by squad slot `playerId`.
// Only the simulating peer has ears; on clients this is a no-op and the host hears the
// same event through the squad fx packet.
inline void emitSquadNoise(int playerId, float radius) {
    if (multiState != MULTI_IN_GAME || playerId == netMgr.myId) {
        entityMgr.emitNoise(cam.pos, radius, playerId);
        return;
    }
    if (playerId < 0 || playerId >= MAX_PLAYERS || !netMgr.players[playerId].hasValidPos) return;
    entityMgr.emitNoise(netMgr.players[playerId].pos, radius, playerId);
}

// Rule tips, the bonk and the crawler slam. Runs on every peer, puppets included.
inline void updateMonsterFeedback() {
    if (survivalTime < 0.5f) resetMonsterFeedback(); // fresh level: the tips may show again
    if (gCrawlerSlamCooldown > 0.0f) gCrawlerSlamCooldown -= dTime;

    if (entityMgr.bonkPending) {
        entityMgr.bonkPending = false;
        Vec3 d = entityMgr.bonkPos - cam.pos;
        d.y = 0.0f;
        float dist = d.len();
        triggerToySound(TOY_SHOVE, squadSoundFalloff(dist), 0.55f);
        if (dist < 16.0f) setEchoStatus("BONK. THE CRAWLER KNOCKED ITSELF OUT");
    }

    bool alive = !gCoopDead && !isPlayerDead;
    for (const auto& e : entityMgr.entities) {
        if (!e.active) continue;
        Vec3 d = e.pos - cam.pos;
        d.y = 0.0f;
        float dist = d.len();
        int ti = (int)e.type;

        // The first meeting with each kind explains its one rule.
        if (alive && ti > 0 && ti < 4 && !gMonsterTipShown[ti] && dist < MONSTER_TIP_RANGE &&
            navLineOfSight(cam.pos.x, cam.pos.z, e.pos.x, e.pos.z, CS, getCellWorld)) {
            gMonsterTipShown[ti] = true;
            setEchoStatus(entityRuleTip(e.type));
        }

        // A charging crawler does not stop for anybody: whoever it runs into goes flying.
        bool slam = e.type == ENTITY_CRAWLER && e.mood == MOOD_CHARGE && dist < e.attackRange;
        if (slam && alive && !playerDowned && gCrawlerSlamCooldown <= 0.0f && dist > 0.05f) {
            gCrawlerSlamCooldown = CRAWLER_SLAM_COOLDOWN;
            gCrawlerSlamVel = d * (-CRAWLER_SLAM_SPEED / dist);
            if (camShake < 0.2f) camShake = 0.2f;
            triggerToySound(TOY_SHOVE, 1.0f, 0.7f);
        }
    }
}

inline void noteSquadFlags(int slot, unsigned char flags) {
    bool wasUp = (gSquadPrevFlags[slot] & (PLAYER_FLAG_DOWNED | PLAYER_FLAG_DEAD)) == 0;
    bool isUp = (flags & (PLAYER_FLAG_DOWNED | PLAYER_FLAG_DEAD)) == 0;
    if (wasUp && !isUp) entityMgr.pendingDowns++;
    gSquadPrevFlags[slot] = flags;
}

// Called once per frame before the monsters think. Monsters hunt every standing squad
// member, not just the host; ghosts are never hunted, the downed only once nobody stands.
inline void collectEntityTargets() {
    updateMonsterFeedback();

    entityMgr.targetCount = 0;
    entityMgr.ignoreLocal = false;
    entityMgr.puppetMode = false;
    entityMgr.localGhost = gCoopDead;
    entityMgr.lightsOut = lightsOutTimer > 0.0f;
    entityMgr.attention01 = attentionLevel * 0.01f;

    bool coop = multiState == MULTI_IN_GAME;
    if (coop && !netMgr.isHost) {
        // Clients do not simulate monsters at all: they replay the host's snapshots.
        entityMgr.puppetMode = true;
        entityMgr.ignoreLocal = gCoopDead;
        return;
    }

    unsigned char localFlags = 0;
    if (playerDowned) localFlags |= PLAYER_FLAG_DOWNED;
    if (gCoopDead || isPlayerDead) localFlags |= PLAYER_FLAG_DEAD;

    if (!coop) {
        noteSquadFlags(0, localFlags);
        entityMgr.squadSize = 1;
        entityMgr.squadStanding = localFlags == 0 ? 1 : 0;
        entityMgr.ignoreLocal = isPlayerDead;
        if (!isPlayerDead) entityMgr.addTarget(makeLocalAiTarget());
        return;
    }

    int squad = 1, standing = 0;
    noteSquadFlags(netMgr.myId, localFlags);
    entityMgr.ignoreLocal = gCoopDead || playerDowned;
    if (!gCoopDead && !playerDowned) {
        entityMgr.addTarget(makeLocalAiTarget());
        standing++;
    }
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active) continue;
        squad++;
        noteSquadFlags(p, netMgr.players[p].flags);
        if (!isRemotePlayerStanding(p) || !netMgr.players[p].hasValidPos) continue;
        entityMgr.addTarget(makeRemoteAiTarget(p));
        standing++;
    }
    entityMgr.squadSize = squad;
    entityMgr.squadStanding = standing;
    if (entityMgr.targetCount > 0) return;

    // Nobody is standing: finish off the downed.
    if (playerDowned && !gCoopDead) entityMgr.addTarget(makeLocalAiTarget());
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
        if ((netMgr.players[p].flags & PLAYER_FLAG_DEAD) != 0) continue;
        entityMgr.addTarget(makeRemoteAiTarget(p));
    }
}
