#pragma once
// The three monsters. Each one follows a single rule the squad can read, shout about and
// exploit (the Pac-Man lesson: personality comes from simple, different targeting rules):
//
//   STALKER  moves only while nobody has it on screen. Somebody has to keep watching.
//   CRAWLER  is blind and hunts noise. It charges in a straight line and knocks itself
//            out on walls and pillars, so it can be dodged and baited.
//   SHADOW   sneaks up on whoever wandered off alone. A glance scares it away,
//            a long stare makes it furious.
//
// None of them reads player positions: they act on what they saw, heard or were hinted.

#include "entity_brain.h"

// ---------------------------------------------------------------------------
// Stalker
// ---------------------------------------------------------------------------

inline constexpr float STALKER_WATCH_RANGE = 40.0f;
inline constexpr float STALKER_HUNT_SPEED = 1.9f;  // multipliers of the base speed
inline constexpr float STALKER_CLOSE_SPEED = 2.3f;
inline constexpr float STALKER_CLOSE_DIST = 7.0f;

// Looks around and returns the index of the most obvious visible squad member (-1 = none).
inline int stalkerLook(Entity& e, const BrainContext& ctx) {
    float bestEvidence = 0.0f;
    int seen = -1;
    for (int i = 0; i < ctx.targetCount; i++) {
        const AiTarget& t = ctx.targets[i];
        float d = brainDist(e.pos, t.pos);
        if (d >= e.detectionRange || !brainHasSight(ctx, e.pos, t.pos)) continue;
        float evidence = sightEvidencePerSec(d, e.detectionRange, t.flashlight, t.crouched);
        if (evidence > bestEvidence) {
            bestEvidence = evidence;
            seen = i;
        }
    }
    e.awareness = stepAwareness(e.awareness, bestEvidence, ctx.dt);
    if (seen >= 0 && e.awareness >= AWARE_SUSPICIOUS) {
        e.goal = ctx.targets[seen].pos;
        e.hasGoal = true;
        e.targetId = ctx.targets[seen].id;
    }
    return seen;
}

inline void brainStalker(Entity& e, const BrainContext& ctx) {
    int watcher = brainWatcher(e, ctx, STALKER_WATCH_RANGE);
    if (watcher >= 0) {
        // Caught in somebody's view: not a twitch. It does remember who looked.
        brainSetMood(e, MOOD_FROZEN);
        e.state = ENT_IDLE;
        e.watchedTimer += ctx.dt;
        e.awareness = AWARE_ALERT;
        e.goal = ctx.targets[watcher].pos;
        e.hasGoal = true;
        e.targetId = ctx.targets[watcher].id;
        return;
    }
    e.watchedTimer = 0.0f;

    int seen = stalkerLook(e, ctx);
    float nearDist = 0.0f;
    int nearest = brainNearestTarget(e, ctx, nearDist);

    if (ctx.backOff) {
        if (e.mood != MOOD_RETREAT) e.hasGoal = false;
        brainSetMood(e, MOOD_RETREAT);
        e.state = ENT_FLEEING;
        if (e.awareness > AWARE_SUSPICIOUS) e.awareness = AWARE_SUSPICIOUS;
        float d = brainRetreat(e, ctx, ctx.targets[nearest].pos, e.speed * 1.6f);
        if (d > 24.0f) e.active = false; // gone backstage; the spawner brings a new one later
        return;
    }
    if (e.mood == MOOD_RETREAT) {
        e.hasGoal = false;
        brainSetMood(e, MOOD_WANDER);
    }

    bool hunting = e.awareness >= AWARE_ALERT || (e.mood == MOOD_HUNT && e.awareness >= AWARE_SUSPICIOUS);
    if (hunting && e.hasGoal) {
        brainSetMood(e, MOOD_HUNT);
        e.state = ENT_STALKING;
        float mul = nearDist < STALKER_CLOSE_DIST ? STALKER_CLOSE_SPEED : STALKER_HUNT_SPEED;
        bool arrived = brainTravel(e, ctx, e.goal, e.speed * mul * ctx.aggression, e.attackRange * 0.6f);
        if (arrived && seen < 0) {
            // Nobody where it expected them: poke around the area for a while.
            e.awareness = AWARE_SUSPICIOUS + 0.2f;
            brainSetMood(e, MOOD_INVESTIGATE);
            e.hasGoal = brainPickCell(e, ctx, 1, 4, nullptr, e.goal);
        }
    } else if (e.hasGoal && (e.awareness >= AWARE_SUSPICIOUS || (e.mood == MOOD_INVESTIGATE && e.moodTimer < 25.0f))) {
        // A hunch is walked all the way to the spot, even after the suspicion itself has faded.
        brainSetMood(e, MOOD_INVESTIGATE);
        e.state = ENT_ROAMING;
        if (brainTravel(e, ctx, e.goal, e.speed * STALKER_HUNT_SPEED, ctx.world.cs * 0.3f)) e.hasGoal = false;
    } else {
        if (e.mood != MOOD_WANDER) e.hasGoal = false;
        brainSetMood(e, MOOD_WANDER);
        e.state = ENT_ROAMING;
        brainWander(e, ctx, e.speed * 0.55f);
    }

    if (nearest >= 0 && brainCanReach(e, ctx, ctx.targets[nearest])) e.state = ENT_ATTACKING;
}

// ---------------------------------------------------------------------------
// Crawler
// ---------------------------------------------------------------------------

inline constexpr float CRAWLER_LISTEN_SECONDS = 0.55f;     // telegraph before the first charge
inline constexpr float CRAWLER_LISTEN_HOT_SECONDS = 0.28f; // between charges of the same chase
inline constexpr float CRAWLER_CHARGE_SPEED = 1.75f;       // multiplier of the base speed
inline constexpr float CRAWLER_CHARGE_MAX_SPEED = 6.2f;    // always a hair slower than a sprint
inline constexpr float CRAWLER_OVERSHOOT = 3.5f;           // runs this far past the sound
inline constexpr float CRAWLER_TURN_RATE = 1.0f;           // rad/s of steering while committed
inline constexpr float CRAWLER_BONK_MIN_RUN = 2.5f;        // shorter runs just stop at the wall
inline constexpr float CRAWLER_STUN_SECONDS = 1.8f;
inline constexpr float CRAWLER_SNIFF_SECONDS = 3.2f;

struct HeardSound {
    bool any = false;
    Vec3 pos;
    float loudness = 0.0f;
    int source = -1;
};

inline HeardSound crawlerListen(const Entity& e, const BrainContext& ctx, float hearingMul) {
    HeardSound best;
    auto consider = [&](const Vec3& pos, float loudness, int source) {
        if (loudness <= best.loudness) return;
        best.any = true;
        best.pos = pos;
        best.loudness = loudness;
        best.source = source;
    };
    float cs = ctx.world.cs;
    for (int i = 0; i < ctx.targetCount; i++) {
        const AiTarget& t = ctx.targets[i];
        consider(t.pos, heardLoudness(e.pos, t.pos, t.noiseRadius, hearingMul, cs, ctx.world.cellAt), t.id);
        // Blind, not numb: bumping into somebody gives them away however quiet they are.
        if (brainCanReach(e, ctx, t)) consider(t.pos, 1.5f, t.id);
    }
    for (int i = 0; i < ctx.noiseCount; i++) {
        const AiNoise& n = ctx.noises[i];
        consider(n.pos, heardLoudness(e.pos, n.pos, n.radius, hearingMul, cs, ctx.world.cellAt), n.sourceId);
    }
    return best;
}

inline void crawlerStartSniffing(Entity& e) {
    brainSetMood(e, MOOD_INVESTIGATE);
    e.hasGoal = false;
    e.patience = CRAWLER_SNIFF_SECONDS;
}

inline void crawlerCharge(Entity& e, const BrainContext& ctx, const HeardSound& heard) {
    float cs = ctx.world.cs;
    float speed = e.speed * CRAWLER_CHARGE_SPEED * ctx.aggression;
    if (speed > CRAWLER_CHARGE_MAX_SPEED) speed = CRAWLER_CHARGE_MAX_SPEED;
    bool sameSource = heard.any && heard.source == e.targetId;

    if (e.chargeLeft < 0.0f) {
        // The sound came from around a corner: follow the route until there is a straight run.
        if (sameSource || heard.loudness > 0.6f) e.goal = heard.pos;
        if (brainClearRun(ctx, e.pos, e.goal)) {
            float d = brainDist(e.pos, e.goal);
            if (d > 0.05f) e.chargeDir = Vec3((e.goal.x - e.pos.x) / d, 0.0f, (e.goal.z - e.pos.z) / d);
            e.chargeLeft = d + CRAWLER_OVERSHOOT;
            return;
        }
        bool arrived = brainTravel(e, ctx, e.goal, speed, cs * 0.3f);
        if (arrived || e.moodTimer > 8.0f) crawlerStartSniffing(e);
        return;
    }

    // Committed: it can only bend the run a little towards fresh noise from the same prey.
    if (sameSource) {
        float want = atan2f(heard.pos.x - e.pos.x, heard.pos.z - e.pos.z);
        float have = atan2f(e.chargeDir.x, e.chargeDir.z);
        float diff = brainWrapAngle(want - have);
        float maxTurn = CRAWLER_TURN_RATE * ctx.dt;
        if (diff > maxTurn) diff = maxTurn;
        if (diff < -maxTurn) diff = -maxTurn;
        e.chargeDir = Vec3(sinf(have + diff), 0.0f, cosf(have + diff));
    }
    float step = speed * ctx.dt;
    float nx = e.pos.x + e.chargeDir.x * step;
    float nz = e.pos.z + e.chargeDir.z * step;
    if (ctx.world.collide(nx, nz, BRAIN_BODY_RADIUS)) {
        if (e.chargeRun >= CRAWLER_BONK_MIN_RUN) {
            brainSetMood(e, MOOD_STUNNED); // head first into the wall
            e.hasGoal = false;
            return;
        }
        // Not up to speed yet: shoulder past the door post or pillar it started next to.
        brainStep(e, ctx, e.chargeDir.x, e.chargeDir.z, step);
        e.chargeLeft -= step;
        if (e.chargeLeft <= 0.0f) crawlerStartSniffing(e);
        return;
    }
    e.pos.x = nx;
    e.pos.z = nz;
    e.chargeRun += step;
    e.chargeLeft -= step;
    brainFace(e, e.chargeDir.x, e.chargeDir.z, ctx.dt * 2.0f);
    if (e.chargeLeft <= 0.0f) crawlerStartSniffing(e);
}

inline void brainCrawler(Entity& e, const BrainContext& ctx) {
    if (e.mood == MOOD_STUNNED) {
        e.state = ENT_IDLE;
        if (e.moodTimer > CRAWLER_STUN_SECONDS) {
            brainSetMood(e, MOOD_WANDER);
            e.chargeRun = 0.0f;
        }
        return;
    }

    float hearing = ctx.backOff ? 0.5f : 1.0f;
    if (e.mood == MOOD_INVESTIGATE) hearing *= 1.25f;
    HeardSound heard = crawlerListen(e, ctx, hearing);

    if (e.mood == MOOD_LISTEN) {
        e.state = ENT_IDLE;
        if (heard.any && (heard.source == e.targetId || heard.loudness > 0.6f)) {
            e.goal = heard.pos;
            e.targetId = heard.source;
        }
        brainFace(e, e.goal.x - e.pos.x, e.goal.z - e.pos.z, ctx.dt);
        float wait = e.chargeRun > 0.0f ? CRAWLER_LISTEN_HOT_SECONDS : CRAWLER_LISTEN_SECONDS;
        if (e.moodTimer >= wait) {
            brainSetMood(e, MOOD_CHARGE);
            e.chargeLeft = -1.0f;
            e.chargeRun = 0.01f;
        }
    } else if (e.mood == MOOD_CHARGE) {
        e.state = ENT_CHASING;
        crawlerCharge(e, ctx, heard);
    } else if (heard.any) {
        e.goal = heard.pos;
        e.hasGoal = true;
        e.targetId = heard.source;
        brainSetMood(e, MOOD_LISTEN);
        e.state = ENT_IDLE;
    } else if (e.mood == MOOD_INVESTIGATE) {
        e.state = ENT_STALKING;
        if (e.hasGoal) {
            if (brainTravel(e, ctx, e.goal, e.speed * 0.8f, ctx.world.cs * 0.3f)) e.hasGoal = false;
        } else {
            // Sniffing: turn on the spot, nose to the floor.
            e.yaw = brainWrapAngle(e.yaw + sinf(e.moodTimer * 3.0f) * 2.4f * ctx.dt);
            e.patience -= ctx.dt;
            if (e.patience <= 0.0f) {
                brainSetMood(e, MOOD_WANDER);
                e.chargeRun = 0.0f;
            }
        }
    } else {
        brainSetMood(e, MOOD_WANDER);
        e.state = ENT_ROAMING;
        float nearDist = 0.0f;
        int nearest = brainNearestTarget(e, ctx, nearDist);
        if (ctx.backOff && nearest >= 0 && nearDist < 14.0f) brainRetreat(e, ctx, ctx.targets[nearest].pos, e.speed * 0.6f);
        else brainWander(e, ctx, e.speed * 0.38f);
    }

    for (int i = 0; i < ctx.targetCount; i++) {
        if (brainCanReach(e, ctx, ctx.targets[i])) e.state = ENT_ATTACKING;
    }
}

// ---------------------------------------------------------------------------
// Shadow
// ---------------------------------------------------------------------------

inline constexpr float SHADOW_SIGHT_RANGE = 24.0f;    // how far away a glance still counts
inline constexpr float SHADOW_STARE_LIMIT = 2.6f;     // seconds of staring before it snaps
inline constexpr float SHADOW_ENRAGED_SECONDS = 6.5f;
inline constexpr float SHADOW_ENRAGED_SPEED = 4.4f;   // multipliers of the base speed
inline constexpr float SHADOW_RETREAT_SPEED = 3.0f;
inline constexpr float SHADOW_HIDE_SECONDS = 3.0f;    // stays away this long after a glance
inline constexpr float SHADOW_GROUP_RADIUS = 6.0f;    // a teammate this close keeps it at bay
inline constexpr float SHADOW_BOLD_AFTER = 22.0f;     // ...but only for so long
inline constexpr float SHADOW_HOVER_NEAR = 9.0f;
inline constexpr float SHADOW_HOVER_FAR = 16.0f;

// The squad member who wandered off the farthest; the current prey gets a small bonus
// so it does not flip between two players every frame.
inline int shadowPickLoner(const Entity& e, const BrainContext& ctx) {
    int best = -1;
    float bestScore = -1e9f;
    for (int i = 0; i < ctx.targetCount; i++) {
        const AiTarget& t = ctx.targets[i];
        float isolation = t.isolation > 30.0f ? 30.0f : t.isolation;
        float score = isolation - brainDist(e.pos, t.pos) * 0.15f;
        if (t.id == e.targetId) score += 4.0f;
        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

inline void shadowLurk(Entity& e, const BrainContext& ctx) {
    brainSetMood(e, MOOD_LURK);
    e.state = ENT_STALKING;
    int ti = shadowPickLoner(e, ctx);
    const AiTarget& t = ctx.targets[ti];
    e.targetId = t.id;

    bool grouped = t.isolation < SHADOW_GROUP_RADIUS;
    if (grouped) e.patience += ctx.dt;
    bool approach = !grouped || e.patience >= SHADOW_BOLD_AFTER;
    float d = brainDist(e.pos, t.pos);

    if (!approach) {
        // The squad sticks together: circle at a distance and wait for somebody to drift off.
        if (d < SHADOW_HOVER_NEAR) brainRetreat(e, ctx, t.pos, e.speed * 1.5f);
        else if (d > SHADOW_HOVER_FAR) brainTravel(e, ctx, t.pos, e.speed * 1.9f, SHADOW_HOVER_FAR - 3.0f);
        else brainFace(e, t.pos.x - e.pos.x, t.pos.z - e.pos.z, ctx.dt);
        return;
    }

    // Come up from behind when there is room there, otherwise straight at them.
    Vec3 goal = t.pos;
    float back = e.attackRange * 0.55f;
    Vec3 behind(t.pos.x - sinf(t.yaw) * back, 0.0f, t.pos.z - cosf(t.yaw) * back);
    NavCell behindCell = brainCellOf(behind, ctx.world.cs);
    if (ctx.world.cellAt(behindCell.x, behindCell.z) == 0 && brainHasSight(ctx, t.pos, behind)) goal = behind;
    float mul = d < SHADOW_HOVER_NEAR ? 1.5f : 2.1f;
    brainTravel(e, ctx, goal, e.speed * mul * ctx.aggression, 0.3f, &t);
    if (brainCanReach(e, ctx, t)) e.state = ENT_ATTACKING;
}

inline void brainShadow(Entity& e, const BrainContext& ctx) {
    float nearDist = 0.0f;
    int nearest = brainNearestTarget(e, ctx, nearDist);

    if (e.mood == MOOD_ENRAGED) {
        // No more hiding: it goes for whoever stared, looked at or not.
        e.state = ENT_CHASING;
        int ti = brainTargetById(ctx, e.targetId);
        if (ti < 0) ti = nearest;
        const AiTarget& t = ctx.targets[ti];
        brainTravel(e, ctx, t.pos, e.speed * SHADOW_ENRAGED_SPEED, e.attackRange * 0.5f);
        if (brainCanReach(e, ctx, t)) e.state = ENT_ATTACKING;
        if (e.moodTimer > SHADOW_ENRAGED_SECONDS) {
            brainSetMood(e, MOOD_RETREAT);
            e.watchedTimer = 0.0f;
            e.unseenTimer = 0.0f;
            e.hasGoal = false;
        }
        return;
    }

    int watcher = brainWatcher(e, ctx, SHADOW_SIGHT_RANGE);
    if (watcher >= 0) {
        const AiTarget& w = ctx.targets[watcher];
        e.watchedTimer += ctx.dt * (w.flashlight ? 1.6f : 1.0f); // a beam in the face is worse
        e.unseenTimer = 0.0f;
        if (e.watchedTimer >= SHADOW_STARE_LIMIT) {
            brainSetMood(e, MOOD_ENRAGED);
            e.targetId = w.id;
            e.hasGoal = false;
            return;
        }
        if (e.mood != MOOD_RETREAT) e.hasGoal = false;
        brainSetMood(e, MOOD_RETREAT);
        e.state = ENT_FLEEING;
        e.patience = 0.0f;
        brainRetreat(e, ctx, w.pos, e.speed * SHADOW_RETREAT_SPEED);
        return;
    }

    e.unseenTimer += ctx.dt;
    e.watchedTimer -= ctx.dt * 0.6f;
    if (e.watchedTimer < 0.0f) e.watchedTimer = 0.0f;

    if (ctx.backOff && e.mood != MOOD_RETREAT) {
        brainSetMood(e, MOOD_RETREAT);
        e.hasGoal = false;
    }
    if (e.mood == MOOD_RETREAT) {
        e.state = ENT_FLEEING;
        if (e.unseenTimer < SHADOW_HIDE_SECONDS || ctx.backOff) {
            float d = brainRetreat(e, ctx, ctx.targets[nearest].pos, e.speed * 2.2f);
            if (ctx.backOff && d > 22.0f) e.active = false; // melted into the dark
            return;
        }
        e.hasGoal = false;
    }
    shadowLurk(e, ctx);
}

// ---------------------------------------------------------------------------
// Shared entry points
// ---------------------------------------------------------------------------

inline void runEntityBrain(Entity& e, const BrainContext& ctx) {
    e.moodTimer += ctx.dt;
    if (ctx.targetCount <= 0) {
        // Nobody left to hunt (everyone is a ghost): just roam.
        if (e.mood != MOOD_WANDER) e.hasGoal = false;
        brainSetMood(e, MOOD_WANDER);
        e.state = ENT_ROAMING;
        brainWander(e, ctx, e.speed * 0.4f);
        return;
    }
    if (e.type == ENTITY_STALKER) brainStalker(e, ctx);
    else if (e.type == ENTITY_CRAWLER) brainCrawler(e, ctx);
    else if (e.type == ENTITY_SHADOW) brainShadow(e, ctx);
}

inline bool entityIsEngaged(const Entity& e) {
    if (e.state == ENT_ATTACKING) return true;
    return e.mood == MOOD_HUNT || e.mood == MOOD_CHARGE || e.mood == MOOD_ENRAGED || e.mood == MOOD_LISTEN;
}

// Director hint: "they are somewhere over there". The monster still has to find them itself.
inline void giveEntityHint(Entity& e, const Vec3& around) {
    if (entityIsEngaged(e) || e.mood == MOOD_FROZEN || e.mood == MOOD_STUNNED || e.mood == MOOD_RETREAT) return;
    if (e.type == ENTITY_SHADOW) return; // always knows where the loner is anyway
    e.goal = around;
    e.hasGoal = true;
    brainSetMood(e, MOOD_INVESTIGATE);
    e.patience = CRAWLER_SNIFF_SECONDS;
    if (e.awareness < AWARE_SUSPICIOUS + 0.25f) e.awareness = AWARE_SUSPICIOUS + 0.25f;
}

inline const char* entityTypeName(EntityType type) {
    if (type == ENTITY_STALKER) return "STALKER";
    if (type == ENTITY_CRAWLER) return "CRAWLER";
    if (type == ENTITY_SHADOW) return "SHADOW";
    return "THING";
}

// One-line rule shown the first time a squad member meets the monster.
inline const char* entityRuleTip(EntityType type) {
    if (type == ENTITY_STALKER) return "STALKER: IT ONLY MOVES WHEN NOBODY IS LOOKING AT IT";
    if (type == ENTITY_CRAWLER) return "CRAWLER: BLIND. IT CHARGES AT NOISE. CROUCH, OR MAKE IT HIT A WALL";
    if (type == ENTITY_SHADOW) return "SHADOW: HUNTS LONERS. A GLANCE SCARES IT. DO NOT STARE";
    return "";
}

// What the monster cam prints under the monster's name.
inline const char* entityMoodLine(EntityType type, int mood) {
    if (mood == MOOD_FROZEN) return "FROZEN: SOMEBODY IS STARING AT IT";
    if (mood == MOOD_STUNNED) return "SEEING STARS: IT RAN INTO A WALL";
    if (mood == MOOD_LISTEN) return "HEARD SOMETHING...";
    if (mood == MOOD_CHARGE) return "CHARGING AT THE NOISE";
    if (mood == MOOD_ENRAGED) return "FURIOUS: SOMEBODY STARED TOO LONG";
    if (mood == MOOD_LURK) return "SNEAKING UP ON A LONER";
    if (mood == MOOD_HUNT) return type == ENTITY_STALKER ? "CLOSING IN WHILE NOBODY LOOKS" : "HUNTING";
    if (mood == MOOD_INVESTIGATE) return type == ENTITY_CRAWLER ? "SNIFFING AROUND" : "CHECKING A HUNCH";
    if (mood == MOOD_RETREAT) return type == ENTITY_SHADOW ? "SPOTTED: SLINKING AWAY" : "BACKING OFF";
    return "WANDERING";
}
