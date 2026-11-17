#pragma once
// EntityManager simulation: spawning, the per-frame brain tick with the pacing director,
// and the puppet mode network clients use (they replay host snapshots instead of thinking).
// Included at the end of entity.h.

#include "entity_behaviors.h"
#include "ai_director.h"

inline constexpr float ENTITY_LEASH_DIST = 48.0f;   // farther than this from everybody: goes backstage
inline constexpr int ENTITY_ENTRANCE_MIN_STEPS = 4; // hidden entrances are picked this many cells of
inline constexpr int ENTITY_ENTRANCE_MAX_STEPS = 9; // walking away from a squad member
inline constexpr float ENTITY_MENACE_RANGE = 16.0f;
inline constexpr float ENTITY_SPAWN_MIN_DIST = 25.0f;
inline constexpr float ENTITY_SPAWN_SPACING = 14.0f;
inline constexpr float ENTITY_NOISE_TTL = 0.25f;

inline bool isDoorwayCell(int wx, int wz) {
    return doorwayAxisAt(wx, wz) != 0;
}

// Solid things that are not part of the maze (the heist rig's hull).
inline bool (*gAiExtraCollide)(float, float, float) = nullptr;

inline bool aiCollideWorld(float x, float z, float r) {
    return collideWorld(x, z, r) || (gAiExtraCollide && gAiExtraCollide(x, z, r));
}

inline AiWorld gameAiWorld() {
    AiWorld w;
    w.cellAt = getCellWorld;
    w.collide = aiCollideWorld;
    w.narrow = isDoorwayCell;
    w.cs = CS;
    return w;
}

// Per-type numbers. Shared by the host spawner and by client snapshots so that
// "is it close enough to hurt me" means the same thing on every machine.
inline void applyEntityTypeStats(Entity& e) {
    if (e.type == ENTITY_STALKER) {
        e.speed = 1.7f;
        e.detectionRange = 22.0f;
        e.attackRange = 1.15f;
    } else if (e.type == ENTITY_CRAWLER) {
        e.speed = 3.4f;
        e.detectionRange = 17.0f;
        e.attackRange = 1.35f;
    } else if (e.type == ENTITY_SHADOW) {
        e.speed = 1.15f;
        e.detectionRange = 12.0f;
        e.attackRange = 1.8f;
    }

    if (isParkingLevel(gCurrentLevel)) {
        if (e.type == ENTITY_STALKER) { e.speed *= 0.92f; e.detectionRange += 4.0f; }
        if (e.type == ENTITY_CRAWLER) { e.speed *= 1.18f; e.detectionRange += 2.5f; }
        if (e.type == ENTITY_SHADOW) { e.attackRange += 0.4f; e.detectionRange += 1.2f; }
    } else {
        if (e.type == ENTITY_STALKER) e.attackRange += 0.2f;
        if (e.type == ENTITY_CRAWLER) e.speed *= 1.06f;
        if (e.type == ENTITY_SHADOW) e.speed *= 0.92f;
    }
}

inline void EntityManager::spawnEntity(EntityType type, Vec3 pos, int maze[][30], int mzw, int mzh) {
    (void)maze; (void)mzw; (void)mzh;
    Entity e;
    e.type = type;
    e.pos = pos;
    e.pos.y = (type == ENTITY_CRAWLER) ? 0.58f : 0.0f;
    e.active = true;
    e.state = ENT_ROAMING;
    applyEntityTypeStats(e);
    e.behaviorMode = BEHAVIOR_DEFAULT;
    e.uid = nextUid++;
    if (nextUid == 0) nextUid = 1;
    e.sideSign = (rand() % 2) ? 1.0f : -1.0f;
    e.yaw = brainRand01() * 6.28f;
    e.netPos = e.pos;
    e.netYaw = e.yaw;
    entities.push_back(e);
}

inline bool EntityManager::anyTargetSees(const Vec3& pos, float maxDist) const {
    for (int i = 0; i < targetCount; i++) {
        if (brainDist(targets[i].pos, pos) > maxDist) continue;
        if (navLineOfSight(targets[i].pos.x, targets[i].pos.z, pos.x, pos.z, CS, getCellWorld)) return true;
    }
    return false;
}

// A place for a monster to enter the scene: a short walk from `around` (so it can actually
// get to the squad), out of everyone's line of sight and away from the other monsters.
inline bool EntityManager::pickHiddenEntrance(const Vec3& around, Vec3& out) const {
    auto hidden = [&](int x, int z) {
        Vec3 c(navCellCenter(x, CS), 0.0f, navCellCenter(z, CS));
        if (collideWorld(c.x, c.z, 0.42f) || anyTargetSees(c, 60.0f) || inQuiet(c)) return false;
        for (const auto& e : entities) {
            if (e.active && brainDist(e.pos, c) < ENTITY_SPAWN_SPACING) return false;
        }
        return true;
    };
    NavCell from = {navCellCoord(around.x, CS), navCellCoord(around.z, CS)};
    NavCell picked = from;
    if (!navPickReachableIf(from, ENTITY_ENTRANCE_MIN_STEPS, ENTITY_ENTRANCE_MAX_STEPS, nullptr, (unsigned)rand(),
                            getCellWorld, hidden, picked)) return false;
    out = Vec3(navCellCenter(picked.x, CS), 0.0f, navCellCenter(picked.z, CS));
    return true;
}

// Monsters never pop into existence in front of somebody.
inline bool EntityManager::spawnOutOfSight(EntityType type, const Vec3& around) {
    Vec3 p;
    if (pickHiddenEntrance(around, p)) {
        spawnEntity(type, p, nullptr, 0, 0);
        return true;
    }
    // Wide open hall with no cover nearby: fall back to a far spot, still unseen.
    for (int attempt = 0; attempt < 8; attempt++) {
        p = findSpawnPos(around, ENTITY_SPAWN_MIN_DIST);
        if (getCellWorld(navCellCoord(p.x, CS), navCellCoord(p.z, CS)) != 0) continue;
        if (anyTargetSees(p, 60.0f) || inQuiet(p)) continue;
        spawnEntity(type, p, nullptr, 0, 0);
        return true;
    }
    return false;
}

// The squad walked away from it. Like the alien slipping into the vents, the monster leaves
// the stage and comes back in somewhere near them, unseen, instead of trailing far behind.
inline void EntityManager::sendBackstage(Entity& e) {
    Vec3 p;
    if (targetCount <= 0 || !pickHiddenEntrance(targets[rand() % targetCount].pos, p)) {
        e.active = false;
        return;
    }
    e.pos.x = p.x;
    e.pos.z = p.z;
    e.hasGoal = false;
    e.pathLen = 0;
    e.awareness = 0.0f;
    e.watchedTimer = 0.0f;
    e.chargeRun = 0.0f;
    brainSetMood(e, MOOD_WANDER);
}

inline void EntityManager::emitNoise(const Vec3& pos, float radius, int sourceId) {
    if (puppetMode || radius <= 0.0f) return;
    int slot = noiseCount;
    if (noiseCount < AI_MAX_NOISES) {
        noiseCount++;
    } else {
        slot = 0;
        for (int i = 1; i < AI_MAX_NOISES; i++) {
            if (noises[i].radius < noises[slot].radius) slot = i;
        }
        if (noises[slot].radius >= radius) return;
    }
    noises[slot].pos = pos;
    noises[slot].radius = radius;
    noises[slot].ttl = ENTITY_NOISE_TTL;
    noises[slot].sourceId = sourceId;
}

inline void EntityManager::ageNoises(float dt) {
    int kept = 0;
    for (int i = 0; i < noiseCount; i++) {
        noises[i].ttl -= dt;
        if (noises[i].ttl > 0.0f) noises[kept++] = noises[i];
    }
    noiseCount = kept;
}

// How hard the monsters press on the squad right now, 0..1 (the "menace gauge").
inline float EntityManager::computeMenace(bool& anyEngaged) const {
    float menace = 0.0f;
    anyEngaged = false;
    for (const auto& e : entities) {
        if (!e.active) continue;
        if (entityIsEngaged(e)) anyEngaged = true;
        for (int i = 0; i < targetCount; i++) {
            float d = brainDist(e.pos, targets[i].pos);
            if (d >= ENTITY_MENACE_RANGE) continue;
            float m = 1.0f - d / ENTITY_MENACE_RANGE;
            if (!navLineOfSight(e.pos.x, e.pos.z, targets[i].pos.x, targets[i].pos.z, CS, getCellWorld)) m *= 0.4f;
            if (e.mood == MOOD_FROZEN || e.mood == MOOD_STUNNED) m *= 0.6f;
            if (m > menace) menace = m;
        }
    }
    return menace;
}

inline void EntityManager::hintSomeMonster() {
    if (targetCount <= 0) return;
    const AiTarget& t = targets[rand() % targetCount];
    Vec3 around = t.pos;
    for (int attempt = 0; attempt < 6; attempt++) {
        Vec3 guess(t.pos.x + (brainRand01() - 0.5f) * CS * 3.0f, 0.0f, t.pos.z + (brainRand01() - 0.5f) * CS * 3.0f);
        if (getCellWorld(navCellCoord(guess.x, CS), navCellCoord(guess.z, CS)) != 0) continue;
        around = guess;
        break;
    }
    Entity* best = nullptr;
    float bestDist = 1e9f;
    for (auto& e : entities) {
        if (!e.active || e.type == ENTITY_SHADOW || entityIsEngaged(e)) continue;
        if (e.mood == MOOD_FROZEN || e.mood == MOOD_STUNNED || e.mood == MOOD_RETREAT) continue;
        float d = brainDist(e.pos, t.pos);
        if (d < bestDist) {
            bestDist = d;
            best = &e;
        }
    }
    if (best) giveEntityHint(*best, around);
}

// Animation clocks and visibility. A frozen stalker is perfectly still; that is the point.
inline void EntityManager::animateEntity(Entity& e, float dt) {
    float rate = 3.0f;
    if (e.mood == MOOD_FROZEN) rate = 0.0f;
    else if (e.mood == MOOD_CHARGE || e.mood == MOOD_ENRAGED) rate = 5.5f;
    else if (e.mood == MOOD_STUNNED) rate = 1.2f;
    e.animPhase += dt * rate;
    if (e.animPhase > 6.283f) e.animPhase -= 6.283f;
    e.animTime += dt * rate / 3.0f;
    if (e.animTime > 3600.0f) e.animTime -= 3600.0f;
    e.flickerTimer += dt;
    // A shadow that was spotted flickers out of existence while it runs.
    bool fading = e.type == ENTITY_SHADOW && e.mood == MOOD_RETREAT;
    e.visible = fading ? (((int)(e.flickerTimer * 15.0f)) % 3) == 0 : true;
}

inline void EntityManager::addLocalDanger(const Entity& e, const Vec3& localPos) {
    if (localGhost) return; // the dead ride on monsters' heads; that must not redden their view
    if (inHaven(localPos)) return; // nothing out there is a threat to somebody in the haven
    float reach = e.detectionRange + 5.0f;
    float d = brainDist(e.pos, localPos);
    if (d >= reach) return;
    float prox = 1.0f - d / reach;
    if (prox > dangerLevel) dangerLevel = prox;
}

inline void EntityManager::noteMoodChange(const Entity& e, int prevMood) {
    if (e.mood == prevMood) return;
    if (e.mood == MOOD_STUNNED) {
        bonkPending = true;
        bonkPos = e.pos;
    }
}

inline bool EntityManager::inHaven(const Vec3& pos) const {
    return havenRadius > 0.0f && brainDist(pos, havenPos) < havenRadius;
}

inline bool EntityManager::inQuiet(const Vec3& pos) const {
    return quietRadius > 0.0f && brainDist(pos, havenPos) < quietRadius;
}

// Squad members and sounds inside the haven do not exist as far as the monsters know.
// Returns how many targets were hidden.
inline int EntityManager::shelterTargets() {
    if (havenRadius <= 0.0f) return 0;
    int kept = 0, hidden = 0;
    for (int i = 0; i < targetCount; i++) {
        if (inHaven(targets[i].pos)) hidden++;
        else targets[kept++] = targets[i];
    }
    targetCount = kept;
    int heard = 0;
    for (int i = 0; i < noiseCount; i++) {
        if (!inHaven(noises[i].pos)) noises[heard++] = noises[i];
    }
    noiseCount = heard;
    return hidden;
}

// Something wandered into the haven (or the haven rolled over it): it drops whatever it was
// doing and walks out. One that cannot find the way out in `patience` seconds leaves the stage.
inline void EntityManager::leaveHaven(Entity& e, const BrainContext& ctx, float speedMul, float patience) {
    e.moodTimer += ctx.dt;
    if (e.mood != MOOD_RETREAT) e.hasGoal = false;
    brainSetMood(e, MOOD_RETREAT);
    e.state = ENT_FLEEING;
    e.awareness = 0.0f;
    e.watchedTimer = 0.0f;
    e.chargeRun = 0.0f;
    e.targetId = -1;
    brainRetreat(e, ctx, havenPos, e.speed * speedMul);
    if (e.moodTimer > patience) sendBackstage(e);
}

inline void EntityManager::update(float dt, Vec3 localPos, float localYaw, int maze[][30], int mzw, int mzh, float cs) {
    (void)maze; (void)mzw; (void)mzh; (void)cs;
    hasLastPlayerPos = true;
    lastPlayerPos = localPos;
    animClock += dt;
    if (animClock > 3600.0f) animClock -= 3600.0f;
    if (puppetMode) {
        updatePuppets(dt, localPos);
        return;
    }

    // Nobody registered targets (solo callers, tests): hunt the local player.
    if (targetCount == 0 && !ignoreLocal && !inHaven(localPos)) {
        AiTarget t;
        t.pos = localPos;
        t.yaw = localYaw;
        t.noiseRadius = gPlayerNoise > 0.6f ? NOISE_SPRINT : (gPlayerNoise > 0.1f ? NOISE_WALK : 0.0f);
        targets[targetCount++] = t;
    }
    shelterTargets();
    computeIsolation(targets, targetCount);

    DirectorInput in;
    in.dt = dt;
    bool anyEngaged = false;
    in.menace = computeMenace(anyEngaged);
    in.anyEngaged = anyEngaged;
    in.damage = pendingDamage;
    in.newDowns = pendingDowns;
    in.standing = squadStanding;
    in.squadSize = squadSize;
    in.attention = attention01;
    in.roll = (unsigned)rand();
    directorOut = updateDirector(director, in);
    pendingDamage = 0.0f;
    pendingDowns = 0;
    if (directorOut.giveHint) hintSomeMonster();

    BrainContext ctx;
    ctx.world = gameAiWorld();
    ctx.targets = targets;
    ctx.targetCount = targetCount;
    ctx.noises = noises;
    ctx.noiseCount = noiseCount;
    ctx.dt = dt;
    ctx.aggression = directorOut.aggression;
    ctx.backOff = directorOut.backOff;
    ctx.lightsOut = lightsOut;

    dangerLevel = 0.0f;
    for (auto& e : entities) {
        if (!e.active) continue;
        int prevMood = e.mood;
        EntityState prevState = e.state;
        // In the quiet ring only a monster that is actually after somebody keeps going.
        bool idle = !entityIsEngaged(e) && e.mood != MOOD_FROZEN && e.mood != MOOD_STUNNED;
        if (inHaven(e.pos)) leaveHaven(e, ctx, 2.2f, 4.0f);
        else if (idle && inQuiet(e.pos)) leaveHaven(e, ctx, 1.3f, 9.0f);
        else runEntityBrain(e, ctx);
        if (e.state != prevState) e.stateTimer = 0.0f;
        else e.stateTimer += dt;
        noteMoodChange(e, prevMood);
        animateEntity(e, dt);
        addLocalDanger(e, localPos);

        // Left behind by the whole squad: re-enter near them. Walled in by a world change: retire.
        float nearDist = 0.0f;
        bool leftBehind = brainNearestTarget(e, ctx, nearDist) >= 0 && nearDist > ENTITY_LEASH_DIST;
        if (leftBehind && !entityIsEngaged(e) && !anyTargetSees(e.pos, 80.0f)) sendBackstage(e);
        if (getCellWorld(navCellCoord(e.pos.x, CS), navCellCoord(e.pos.z, CS)) != 0) e.active = false;
    }
    entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity& e) { return !e.active; }), entities.end());
    ageNoises(dt);
}

// ---------------------------------------------------------------------------
// Network clients
// ---------------------------------------------------------------------------

inline void EntityManager::updatePuppets(float dt, Vec3 localPos) {
    dangerLevel = 0.0f;
    float a = 1.0f - expf(-14.0f * dt);
    for (auto& e : entities) {
        if (!e.active) continue;
        e.pos = e.pos + (e.netPos - e.pos) * a;
        e.yaw = brainWrapAngle(e.yaw + brainWrapAngle(e.netYaw - e.yaw) * a);
        e.moodTimer += dt;
        e.stateTimer += dt;
        animateEntity(e, dt);
        addLocalDanger(e, localPos);
    }
}

inline void EntityManager::beginSnapshot() {
    for (auto& e : entities) e.netSeen = false;
}

inline void EntityManager::applySnapshotEntity(int uid, int type, const Vec3& pos, float yaw, unsigned char packedState) {
    Entity* found = nullptr;
    for (auto& e : entities) {
        if (e.uid == (unsigned char)uid && (int)e.type == type) found = &e;
    }
    if (!found) {
        Entity fresh;
        fresh.type = (EntityType)type;
        fresh.uid = (unsigned char)uid;
        fresh.active = true;
        fresh.pos = pos;
        fresh.yaw = yaw;
        applyEntityTypeStats(fresh);
        entities.push_back(fresh);
        found = &entities.back();
    }
    Entity& e = *found;
    int prevMood = e.mood;
    EntityState prevState = e.state;
    e.netPos = pos;
    e.netYaw = yaw;
    e.netSeen = true;
    e.state = (EntityState)unpackEntityState(packedState);
    e.mood = (unsigned char)unpackEntityMood(packedState);
    if (e.mood != prevMood) e.moodTimer = 0.0f;
    if (e.state != prevState) e.stateTimer = 0.0f;
    if (brainDist(e.pos, pos) > 6.0f) e.pos = pos; // teleported or just streamed in: do not glide
    e.pos.y = pos.y;
    noteMoodChange(e, prevMood);
}

inline void EntityManager::endSnapshot() {
    entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity& e) { return !e.netSeen; }), entities.end());
}
