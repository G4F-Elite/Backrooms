#pragma once
// Monster brain plumbing: the world a brain may query, collision-checked steering and
// path following. The behaviours themselves live in entity_behaviors.h.
// Everything here runs on the host (or solo); clients only replay snapshots.

#include <cmath>
#include <cstdlib>
#include "entity_types.h"
#include "ai_nav.h"
#include "ai_senses.h"

// The slice of the world a brain is allowed to see. Function pointers keep the brains
// testable on a hand-made maze.
struct AiWorld {
    int (*cellAt)(int, int) = nullptr;            // 0 = open
    bool (*collide)(float, float, float) = nullptr; // circle vs walls, door frames, pillars
    bool (*narrow)(int, int) = nullptr;           // optional: cell is a doorway, walk through its middle
    float cs = 5.0f;
};

struct BrainContext {
    AiWorld world;
    const AiTarget* targets = nullptr;
    int targetCount = 0;
    const AiNoise* noises = nullptr;
    int noiseCount = 0;
    float dt = 0.0f;
    float aggression = 1.0f; // from the director
    bool backOff = false;    // from the director: leave the squad alone
    bool lightsOut = false;
};

inline constexpr float BRAIN_BODY_RADIUS = 0.32f;
inline constexpr float BRAIN_TURN_RATE = 9.0f; // rad/s

enum BrainStep {
    STEP_FREE = 0, // moved where it wanted
    STEP_SLID,     // forward was blocked, slid along or around the obstacle
    STEP_BLOCKED   // could not move at all
};

inline float brainRand01() {
    return (float)(rand() % 1000) * 0.001f;
}

inline void brainSetMood(Entity& e, int mood) {
    if (e.mood == mood) return;
    e.mood = (unsigned char)mood;
    e.moodTimer = 0.0f;
}

inline float brainDist(const Vec3& a, const Vec3& b) {
    float dx = a.x - b.x, dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

inline float brainWrapAngle(float a) {
    while (a > 3.14159265f) a -= 6.28318531f;
    while (a < -3.14159265f) a += 6.28318531f;
    return a;
}

inline void brainFace(Entity& e, float dirX, float dirZ, float dt) {
    if (dirX * dirX + dirZ * dirZ < 1e-6f) return;
    float want = atan2f(dirX, dirZ);
    float diff = brainWrapAngle(want - e.yaw);
    float maxTurn = BRAIN_TURN_RATE * dt;
    if (diff > maxTurn) diff = maxTurn;
    if (diff < -maxTurn) diff = -maxTurn;
    e.yaw = brainWrapAngle(e.yaw + diff);
}

inline bool brainHasSight(const BrainContext& ctx, const Vec3& from, const Vec3& to) {
    return navLineOfSight(from.x, from.z, to.x, to.z, ctx.world.cs, ctx.world.cellAt);
}

inline bool brainIsNarrow(const BrainContext& ctx, int x, int z) {
    return ctx.world.narrow && ctx.world.narrow(x, z);
}

// A straight run a body can actually take: no walls, and no doorway on the way. The grid
// says a doorway cell is open, but only its middle is; cutting through one at an angle
// ends on the door post, so doorways are always taken cell centre to cell centre.
inline bool brainClearRun(const BrainContext& ctx, const Vec3& from, const Vec3& to) {
    auto solid = [&](int x, int z) {
        return (ctx.world.cellAt(x, z) != 0 || brainIsNarrow(ctx, x, z)) ? 1 : 0;
    };
    return navWalkableLine(from.x, from.z, to.x, to.z, BRAIN_BODY_RADIUS, ctx.world.cs, solid);
}

// One collision-checked step of `dist` meters. Slides along walls and steps around
// small obstacles such as pillars instead of grinding against them.
inline BrainStep brainStep(Entity& e, const BrainContext& ctx, float dirX, float dirZ, float dist) {
    float r = BRAIN_BODY_RADIUS;
    float nx = e.pos.x + dirX * dist;
    float nz = e.pos.z + dirZ * dist;
    if (!ctx.world.collide(nx, nz, r)) {
        e.pos.x = nx;
        e.pos.z = nz;
        return STEP_FREE;
    }
    if (fabsf(dirX) > 0.35f && !ctx.world.collide(nx, e.pos.z, r)) {
        e.pos.x = nx;
        return STEP_SLID;
    }
    if (fabsf(dirZ) > 0.35f && !ctx.world.collide(e.pos.x, nz, r)) {
        e.pos.z = nz;
        return STEP_SLID;
    }
    float sx = -dirZ * e.sideSign, sz = dirX * e.sideSign;
    float px = e.pos.x + sx * dist, pz = e.pos.z + sz * dist;
    if (!ctx.world.collide(px, pz, r)) {
        e.pos.x = px;
        e.pos.z = pz;
        return STEP_SLID;
    }
    e.sideSign = -e.sideSign;
    return STEP_BLOCKED;
}

inline Vec3 brainCellCenter(const NavCell& c, float cs) {
    return Vec3(navCellCenter(c.x, cs), 0.0f, navCellCenter(c.z, cs));
}

inline NavCell brainCellOf(const Vec3& p, float cs) {
    return {navCellCoord(p.x, cs), navCellCoord(p.z, cs)};
}

inline constexpr int BRAIN_SEEN_CELL_COST = 8;     // extra steps a sneaking monster pays per watched cell
inline constexpr float BRAIN_SNEAK_VIEW_RANGE = 26.0f;

// Picks the point to steer at right now: the goal itself when the way is clear,
// otherwise the next corner of an A* route (recomputed about once a second).
// With `avoidView` set the route keeps out of that squad member's sight where it can.
inline Vec3 brainAimPoint(Entity& e, const BrainContext& ctx, const Vec3& goal, const AiTarget* avoidView = nullptr) {
    float cs = ctx.world.cs;
    bool exposed = avoidView && isInViewCone(avoidView->pos, avoidView->yaw, avoidView->pitch, e.pos, BRAIN_SNEAK_VIEW_RANGE);
    bool straight = brainClearRun(ctx, e.pos, goal);
    // Sneaking: the straight line is only good once it comes from outside the prey's view.
    if (straight && avoidView && !exposed) {
        float gx = goal.x - avoidView->pos.x, gz = goal.z - avoidView->pos.z;
        float ex = e.pos.x - avoidView->pos.x, ez = e.pos.z - avoidView->pos.z;
        bool goalInFront = gx * sinf(avoidView->yaw) + gz * cosf(avoidView->yaw) > 0.5f;
        bool selfInFront = ex * sinf(avoidView->yaw) + ez * cosf(avoidView->yaw) > 0.0f;
        if (goalInFront || selfInFront) straight = false;
    }
    if (straight && !exposed) {
        e.pathLen = 0;
        return goal;
    }

    NavCell here = brainCellOf(e.pos, cs);
    NavCell there = brainCellOf(goal, cs);
    e.repathTimer -= ctx.dt;
    bool goalMoved = e.pathLen == 0 || e.pathGoal.x != there.x || e.pathGoal.z != there.z;
    bool exhausted = e.pathLen > 0 && e.pathIdx >= e.pathLen;
    if (e.repathTimer <= 0.0f || ((goalMoved || exhausted) && e.repathTimer < 0.6f)) {
        if (avoidView) {
            const AiTarget& prey = *avoidView;
            auto watched = [&](int x, int z) {
                Vec3 c(navCellCenter(x, cs), 0.0f, navCellCenter(z, cs));
                if (!isInViewCone(prey.pos, prey.yaw, prey.pitch, c, BRAIN_SNEAK_VIEW_RANGE)) return 0;
                return navLineOfSight(prey.pos.x, prey.pos.z, c.x, c.z, cs, ctx.world.cellAt) ? BRAIN_SEEN_CELL_COST : 0;
            };
            e.pathLen = navFindPathCost(here, there, ctx.world.cellAt, watched, e.path, NAV_PATH_KEEP);
        } else {
            e.pathLen = navFindPath(here, there, ctx.world.cellAt, e.path, NAV_PATH_KEEP);
        }
        e.pathIdx = 0;
        e.pathGoal = there;
        e.repathTimer = 0.9f + brainRand01() * 0.3f;
    }

    while (e.pathIdx < e.pathLen) {
        const NavCell& c = e.path[e.pathIdx];
        Vec3 wp = brainCellCenter(c, cs);
        // Ordinary cells are "reached" from well inside them; a doorway only from its middle.
        float reach = brainIsNarrow(ctx, c.x, c.z) ? 0.45f : cs * 0.38f;
        if (brainDist(e.pos, wp) > reach) break;
        e.pathIdx++;
    }
    // Cut the corner when the waypoint after the next one is already in a straight clear run.
    if (!avoidView && e.pathIdx + 1 < e.pathLen) {
        Vec3 ahead = brainCellCenter(e.path[e.pathIdx + 1], cs);
        if (brainClearRun(ctx, e.pos, ahead)) e.pathIdx++;
    }
    if (e.pathIdx < e.pathLen) return brainCellCenter(e.path[e.pathIdx], cs);
    return goal; // no route inside the search window: push towards it and slide
}

// Walks towards `goal` at `speed`. Returns true once within arriveDist.
inline bool brainTravel(Entity& e, const BrainContext& ctx, const Vec3& goal, float speed, float arriveDist,
                        const AiTarget* avoidView = nullptr) {
    if (brainDist(e.pos, goal) <= arriveDist) return true;

    Vec3 aim = brainAimPoint(e, ctx, goal, avoidView);
    float dx = aim.x - e.pos.x, dz = aim.z - e.pos.z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 1e-4f) return false;
    dx /= len;
    dz /= len;
    float want = speed * ctx.dt;
    if (want > len) want = len;

    brainStep(e, ctx, dx, dz, want);
    brainFace(e, dx, dz, ctx.dt);

    // Not getting any closer (shuffling sideways against something counts as not):
    // try the other side and ask for a fresh route.
    float gained = len - brainDist(e.pos, aim);
    if (gained < want * 0.3f) e.stuckTimer += ctx.dt;
    else if (e.stuckTimer > 0.0f) e.stuckTimer -= ctx.dt * 0.5f;
    if (e.stuckTimer > 0.8f) {
        e.stuckTimer = 0.0f;
        e.sideSign = -e.sideSign;
        e.repathTimer = 0.0f;
    }
    return false;
}

// A reachable cell a few steps away; `awayFrom` turns the pick into a retreat.
inline bool brainPickCell(const Entity& e, const BrainContext& ctx, int minSteps, int maxSteps, const Vec3* awayFrom, Vec3& out) {
    float cs = ctx.world.cs;
    NavCell here = brainCellOf(e.pos, cs);
    NavCell picked = here;
    bool ok;
    if (awayFrom) {
        NavCell threat = brainCellOf(*awayFrom, cs);
        ok = navPickReachable(here, minSteps, maxSteps, &threat, 0u, ctx.world.cellAt, picked);
    } else {
        ok = navPickReachable(here, minSteps, maxSteps, nullptr, (unsigned)rand(), ctx.world.cellAt, picked);
    }
    if (!ok) return false;
    out = brainCellCenter(picked, cs);
    return true;
}

// Slow aimless patrol between random reachable cells.
inline void brainWander(Entity& e, const BrainContext& ctx, float speed) {
    if (!e.hasGoal || e.moodTimer > 14.0f) {
        e.hasGoal = brainPickCell(e, ctx, 2, 7, nullptr, e.goal);
        e.moodTimer = 0.0f;
        if (!e.hasGoal) return;
    }
    if (brainTravel(e, ctx, e.goal, speed, ctx.world.cs * 0.3f)) e.hasGoal = false;
}

// Walks away from `threat`. Returns the distance to it.
inline float brainRetreat(Entity& e, const BrainContext& ctx, const Vec3& threat, float speed) {
    float cs = ctx.world.cs;
    bool arrived = e.hasGoal && brainDist(e.pos, e.goal) < cs * 0.4f;
    bool goalIsBad = e.hasGoal && brainDist(e.goal, threat) < brainDist(e.pos, threat);
    if (!e.hasGoal || arrived || goalIsBad) e.hasGoal = brainPickCell(e, ctx, 4, 12, &threat, e.goal);
    if (e.hasGoal) brainTravel(e, ctx, e.goal, speed, cs * 0.3f);
    return brainDist(e.pos, threat);
}

inline int brainNearestTarget(const Entity& e, const BrainContext& ctx, float& outDist) {
    int best = -1;
    outDist = 1e9f;
    for (int i = 0; i < ctx.targetCount; i++) {
        float d = brainDist(e.pos, ctx.targets[i].pos);
        if (d < outDist) {
            outDist = d;
            best = i;
        }
    }
    return best;
}

inline int brainTargetById(const BrainContext& ctx, int id) {
    for (int i = 0; i < ctx.targetCount; i++) {
        if (ctx.targets[i].id == id) return i;
    }
    return -1;
}

// Index of a squad member who has the monster on screen right now, -1 when nobody does.
// Both shoulders are tested as well as the centre: a monster peeking halfway around a
// corner is still a monster you can see.
inline int brainWatcher(const Entity& e, const BrainContext& ctx, float maxDist) {
    for (int i = 0; i < ctx.targetCount; i++) {
        const AiTarget& t = ctx.targets[i];
        if (targetSeesInLight(t, e.pos, maxDist, ctx.lightsOut, ctx.world.cs, ctx.world.cellAt)) return i;
        float dx = e.pos.x - t.pos.x, dz = e.pos.z - t.pos.z;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 0.5f) continue;
        Vec3 side(-dz / len * BRAIN_BODY_RADIUS, 0.0f, dx / len * BRAIN_BODY_RADIUS);
        if (targetSeesInLight(t, e.pos + side, maxDist, ctx.lightsOut, ctx.world.cs, ctx.world.cellAt)) return i;
        if (targetSeesInLight(t, e.pos - side, maxDist, ctx.lightsOut, ctx.world.cs, ctx.world.cellAt)) return i;
    }
    return -1;
}

inline bool brainSeenByAnyone(const Entity& e, const BrainContext& ctx, float maxDist) {
    return brainWatcher(e, ctx, maxDist) >= 0;
}

// Close enough to hurt, with nothing in between.
inline bool brainCanReach(const Entity& e, const BrainContext& ctx, const AiTarget& t) {
    if (brainDist(e.pos, t.pos) >= e.attackRange) return false;
    return brainHasSight(ctx, e.pos, t.pos);
}
