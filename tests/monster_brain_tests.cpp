// Simulation tests for the monster brains: each monster is dropped into a hand-drawn maze
// and has to behave by its rule (and only by what it can see or hear).
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../src/entity_behaviors.h"
#include "../src/ai_director.h"

static const float TCS = 5.0f;
static std::vector<const char*> gMaze;
static bool gPillarAt[32][32];

static int mazeCellAt(int x, int z) {
    if (z < 0 || z >= (int)gMaze.size()) return 1;
    const char* row = gMaze[z];
    int w = 0;
    while (row[w]) w++;
    if (x < 0 || x >= w) return 1;
    return row[x] == '#' ? 1 : 0;
}

// Same shape as collideWorld: circle against wall cells, plus 0.6 m pillars in cell centres.
static bool mazeCollide(float x, float z, float r) {
    int cx = (int)floorf(x / TCS), cz = (int)floorf(z / TCS);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int wx = cx + dx, wz = cz + dz;
            float x0 = wx * TCS, x1 = x0 + TCS, z0 = wz * TCS, z1 = z0 + TCS;
            if (mazeCellAt(wx, wz) != 0) {
                float clx = x < x0 ? x0 : (x > x1 ? x1 : x);
                float clz = z < z0 ? z0 : (z > z1 ? z1 : z);
                if ((x - clx) * (x - clx) + (z - clz) * (z - clz) < r * r) return true;
            } else if (wx >= 0 && wz >= 0 && wx < 32 && wz < 32 && gPillarAt[wx][wz]) {
                float px = (wx + 0.5f) * TCS, pz = (wz + 0.5f) * TCS;
                if (fabsf(x - px) < 0.3f + r && fabsf(z - pz) < 0.3f + r) return true;
            }
        }
    }
    return false;
}

// 'D' in the maze is a doorway with the passage running along X: a thin frame across the
// cell's middle with a 2 m opening, the same shape collideDoorFrameAtCell builds.
static bool mazeIsDoor(int x, int z) {
    if (z < 0 || z >= (int)gMaze.size()) return false;
    const char* row = gMaze[z];
    int w = 0;
    while (row[w]) w++;
    return x >= 0 && x < w && row[x] == 'D';
}

static bool mazeCollideDoors(float x, float z, float r) {
    if (mazeCollide(x, z, r)) return true;
    int cx = (int)floorf(x / TCS), cz = (int)floorf(z / TCS);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int wx = cx + dx, wz = cz + dz;
            if (!mazeIsDoor(wx, wz)) continue;
            float mx = (wx + 0.5f) * TCS, mz = (wz + 0.5f) * TCS;
            float a = z - mz, b = x - mx; // a: across the opening, b: along the passage
            for (int sideIdx = 0; sideIdx < 2; sideIdx++) {
                float a0 = sideIdx == 0 ? -TCS * 0.5f : 1.0f, a1 = sideIdx == 0 ? -1.0f : TCS * 0.5f;
                float ca = a < a0 ? a0 : (a > a1 ? a1 : a);
                float cb = b < -0.25f ? -0.25f : (b > 0.25f ? 0.25f : b);
                if ((a - ca) * (a - ca) + (b - cb) * (b - cb) < r * r) return true;
            }
        }
    }
    return false;
}

static void setMaze(std::vector<const char*> rows) {
    gMaze = rows;
    for (auto& col : gPillarAt) for (bool& p : col) p = false;
}

static Vec3 cellPos(int x, int z) {
    return Vec3((x + 0.5f) * TCS, 0.0f, (z + 0.5f) * TCS);
}

struct Sim {
    BrainContext ctx;
    AiTarget targets[AI_MAX_TARGETS];
    AiNoise noises[AI_MAX_NOISES];
    int targetCount = 0;
    int noiseCount = 0;
    float time = 0.0f;

    Sim() {
        ctx.world.cellAt = mazeCellAt;
        ctx.world.collide = mazeCollideDoors;
        ctx.world.narrow = mazeIsDoor;
        ctx.world.cs = TCS;
        ctx.dt = 1.0f / 60.0f;
    }

    int addTarget(const Vec3& pos, float yaw, float noise = 0.0f) {
        targets[targetCount].pos = Vec3(pos.x, 1.7f, pos.z);
        targets[targetCount].yaw = yaw;
        targets[targetCount].noiseRadius = noise;
        targets[targetCount].id = targetCount;
        return targetCount++;
    }

    void step(Entity& e) {
        computeIsolation(targets, targetCount);
        ctx.targets = targets;
        ctx.targetCount = targetCount;
        ctx.noises = noises;
        ctx.noiseCount = noiseCount;
        runEntityBrain(e, ctx);
        noiseCount = 0; // one-shot sounds last a frame here
        time += ctx.dt;
    }

    void run(Entity& e, float seconds) {
        int frames = (int)(seconds / ctx.dt);
        for (int i = 0; i < frames; i++) step(e);
    }

    // Runs until the predicate holds; returns false on timeout.
    template <typename Pred>
    bool runUntil(Entity& e, float seconds, Pred pred) {
        int frames = (int)(seconds / ctx.dt);
        for (int i = 0; i < frames; i++) {
            step(e);
            if (pred()) return true;
        }
        return false;
    }
};

static Entity makeEntity(EntityType type, const Vec3& pos) {
    Entity e;
    e.type = type;
    e.pos = pos;
    e.active = true;
    e.state = ENT_ROAMING;
    if (type == ENTITY_STALKER) { e.speed = 1.7f; e.detectionRange = 22.0f; e.attackRange = 1.35f; }
    if (type == ENTITY_CRAWLER) { e.speed = 3.6f; e.detectionRange = 17.0f; e.attackRange = 1.35f; }
    if (type == ENTITY_SHADOW) { e.speed = 1.06f; e.detectionRange = 12.0f; e.attackRange = 1.8f; }
    return e;
}

static float yawTowards(const Vec3& from, const Vec3& to) {
    return atan2f(to.x - from.x, to.z - from.z);
}

static const float PI_F = 3.14159265f;

// A long corridor with a side room; used by most tests.
static void corridorMaze() {
    setMaze({
        "############",
        "#..........#",
        "#.########.#",
        "#..........#",
        "############",
    });
}

void testStalkerFreezesWhileWatched() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(2, 1));
    Vec3 player = cellPos(8, 1);
    sim.addTarget(player, yawTowards(player, e.pos)); // staring right at it
    Vec3 start = e.pos;
    float startYaw = e.yaw;
    sim.run(e, 8.0f);
    assert(e.mood == MOOD_FROZEN);
    assert(brainDist(e.pos, start) < 0.001f);
    assert(e.yaw == startYaw);
    assert(e.state != ENT_ATTACKING);
}

void testStalkerClosesInWhenNobodyLooks() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(3, 1));
    Vec3 player = cellPos(7, 1); // 20 m: inside its sight range
    int t = sim.addTarget(player, yawTowards(e.pos, player)); // back turned
    bool attacked = sim.runUntil(e, 20.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(attacked);
    assert(brainDist(e.pos, player) < e.attackRange);

    // Turning around stops it dead, even at arm's length.
    sim.targets[t].yaw = yawTowards(player, e.pos);
    Vec3 at = e.pos;
    sim.run(e, 2.0f);
    assert(e.mood == MOOD_FROZEN && e.state != ENT_ATTACKING);
    assert(brainDist(e.pos, at) < 0.001f);
}

void testStalkerIsNotFrozenThroughWalls() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(5, 3));
    Vec3 player = cellPos(5, 1); // other side of the wall, "looking" at it
    sim.addTarget(player, yawTowards(player, e.pos));
    sim.run(e, 1.0f);
    assert(e.mood != MOOD_FROZEN);
}

void testStalkerInTheDarkNeedsAFlashlight() {
    corridorMaze();
    Sim sim;
    sim.ctx.lightsOut = true;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(6, 1)); // 10 m: inside flashlight reach
    Vec3 player = cellPos(8, 1);
    int t = sim.addTarget(player, yawTowards(player, e.pos));
    sim.run(e, 0.5f);
    assert(e.mood != MOOD_FROZEN); // lights are out: staring into the dark does nothing
    sim.targets[t].flashlight = true;
    sim.run(e, 0.5f);
    assert(e.mood == MOOD_FROZEN);
}

void testStalkerChecksWhereItLastSawYou() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(6, 1));
    Vec3 corner = cellPos(10, 1);
    int t = sim.addTarget(corner, yawTowards(e.pos, corner));
    bool alerted = sim.runUntil(e, 6.0f, [&]() { return e.mood == MOOD_HUNT; });
    assert(alerted);
    // The player ducks around the corner, out of its sight, and keeps looking away.
    Vec3 hide = cellPos(10, 3);
    sim.targets[t].pos = Vec3(hide.x, 1.7f, hide.z);
    sim.targets[t].yaw = 0.0f; // facing +Z, back to the corner
    sim.step(e);
    assert(brainDist(e.goal, corner) < 0.5f); // it only knows the corner, not the new spot
    bool attacked = sim.runUntil(e, 30.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(attacked); // goes to the corner, spots them from there, finishes the job
}

void testCrawlerIgnoresSilentPlayers() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(2, 1));
    sim.addTarget(cellPos(4, 3), 0.0f, 0.0f); // other corridor, standing still
    bool noticed = sim.runUntil(e, 15.0f, [&]() { return e.mood == MOOD_LISTEN || e.mood == MOOD_CHARGE; });
    assert(!noticed);
}

void testCrawlerChargesFootsteps() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(2, 1));
    Vec3 player = cellPos(3, 1);
    player.x += 4.0f; // 9 m away: inside the walking radius
    sim.addTarget(player, 0.0f, NOISE_SPRINT);
    bool listened = sim.runUntil(e, 0.2f, [&]() { return e.mood == MOOD_LISTEN; });
    assert(listened);
    // It telegraphs before it runs.
    Vec3 at = e.pos;
    sim.run(e, CRAWLER_LISTEN_SECONDS * 0.8f);
    assert(e.mood == MOOD_LISTEN && brainDist(at, e.pos) < 0.001f);
    bool hit = sim.runUntil(e, 5.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(hit);
}

void testCrawlerFollowsSqueaksAndBonksIntoWalls() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(3, 1));
    sim.addTarget(cellPos(1, 3), 0.0f, 0.0f); // silent bystander far away
    // Somebody squeaks right next to the east wall.
    Vec3 squeak = cellPos(10, 1);
    squeak.x += 1.5f;
    sim.noises[0].pos = squeak;
    sim.noises[0].radius = NOISE_SQUEAK * 2.0f;
    sim.noises[0].sourceId = 3;
    sim.noiseCount = 1;
    bool charging = sim.runUntil(e, 2.0f, [&]() { return e.mood == MOOD_CHARGE && e.chargeLeft > 0.0f; });
    assert(charging);
    bool stunned = sim.runUntil(e, 12.0f, [&]() { return e.mood == MOOD_STUNNED; });
    assert(stunned);
    assert(e.pos.x > squeak.x - 0.5f); // ran past the sound, into the wall
    Vec3 at = e.pos;
    sim.run(e, CRAWLER_STUN_SECONDS * 0.8f);
    assert(e.mood == MOOD_STUNNED && brainDist(at, e.pos) < 0.001f && e.state != ENT_ATTACKING);
    sim.run(e, CRAWLER_STUN_SECONDS);
    assert(e.mood != MOOD_STUNNED);
}

void testCrawlerCanBeDodged() {
    setMaze({
        "##########",
        "#........#",
        "#........#",
        "#........#",
        "#........#",
        "#........#",
        "##########",
    });
    Sim sim;
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(1, 3));
    Vec3 player = e.pos + Vec3(8.0f, 0.0f, 0.0f); // walking, well within earshot
    int t = sim.addTarget(player, 0.0f, NOISE_WALK);
    bool committed = sim.runUntil(e, 3.0f, [&]() { return e.mood == MOOD_CHARGE && e.chargeLeft > 0.0f; });
    assert(committed);
    // Step out of the lane and hold still: it cannot turn fast enough.
    sim.targets[t].pos = Vec3(player.x, 1.7f, player.z + 6.0f);
    sim.targets[t].noiseRadius = 0.0f;
    bool hit = sim.runUntil(e, 4.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(!hit);
    assert(e.pos.x > player.x); // went straight past
}

void testCrawlerHearsThroughRoutesNotThroughWalls() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(5, 3));
    Vec3 player = cellPos(5, 1); // 10 m away as the crow flies, behind a wall
    sim.addTarget(player, 0.0f, NOISE_WALK);
    bool heard = sim.runUntil(e, 3.0f, [&]() { return e.mood == MOOD_LISTEN; });
    assert(!heard); // walking is muffled by the wall (9 m * 0.6)
    sim.targets[0].noiseRadius = NOISE_SPRINT;
    bool hit = sim.runUntil(e, 25.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(hit); // sprinting is not, and it finds the way around
}

void testCrawlerChargeStartsEvenWhenHuggingAWall() {
    corridorMaze();
    Sim sim;
    // Nose pressed against a pillar that stands between it and the noise.
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(2, 1));
    gPillarAt[3][1] = true;
    e.pos.x = cellPos(3, 1).x - 0.3f - BRAIN_BODY_RADIUS - 0.01f;
    Vec3 player = cellPos(5, 1);
    sim.addTarget(player, 0.0f, NOISE_SPRINT);
    bool hit = sim.runUntil(e, 6.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(hit); // used to give up on the first blocked step and sniff the floor instead
}

void testMonstersUseDoorwaysThroughTheMiddle() {
    // Two rooms joined by a doorway; the monster starts in a corner, so the straight line
    // to its prey crosses the doorway at an angle and would end on the door post.
    setMaze({
        "#########",
        "#...#...#",
        "#...D...#",
        "#...#...#",
        "#########",
    });
    // Several dice rolls: without the doorway rule the crawler only got stuck some of the time.
    for (unsigned roll = 1; roll <= 6; roll++) {
        srand(roll);
        for (int type = ENTITY_STALKER; type <= ENTITY_SHADOW; type++) {
            Sim sim;
            Entity e = makeEntity((EntityType)type, cellPos(1, 1));
            e.pos.x -= 1.5f;
            e.pos.z -= 1.5f;
            Vec3 player = cellPos(7, 3);
            player.z += 1.5f;
            sim.addTarget(player, yawTowards(e.pos, player), NOISE_SPRINT); // loud, back turned
            if (type != ENTITY_CRAWLER) giveEntityHint(e, player);
            bool hit = sim.runUntil(e, 25.0f, [&]() { return e.state == ENT_ATTACKING; });
            assert(hit);
        }
    }
}

void testMonstersWalkAroundPillars() {
    setMaze({
        "#########",
        "#.......#",
        "#########",
    });
    gPillarAt[4][1] = true;
    Sim sim;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(1, 1));
    Vec3 player = cellPos(7, 1);
    sim.addTarget(player, yawTowards(e.pos, player));
    bool attacked = sim.runUntil(e, 25.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(attacked);
}

void testShadowSneaksUpAndFleesFromAGlance() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_SHADOW, cellPos(2, 1));
    Vec3 player = cellPos(7, 1);
    int t = sim.addTarget(player, yawTowards(e.pos, player)); // back turned
    bool close = sim.runUntil(e, 20.0f, [&]() { return brainDist(e.pos, player) < 6.0f; });
    assert(close && e.mood == MOOD_LURK);

    sim.targets[t].yaw = yawTowards(player, e.pos); // a quick look over the shoulder
    float before = brainDist(e.pos, player);
    sim.run(e, 1.0f);
    assert(e.mood == MOOD_RETREAT);
    assert(brainDist(e.pos, player) > before + 1.5f);

    sim.targets[t].yaw = yawTowards(e.pos, player); // look away again
    sim.run(e, 1.0f);
    assert(e.mood == MOOD_RETREAT); // it stays away for a moment...
    bool back = sim.runUntil(e, 30.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(back); // ...and then tries again
}

void testShadowSnapsWhenStaredAt() {
    setMaze({
        "##############",
        "#............#",
        "##############",
    });
    Sim sim;
    Entity e = makeEntity(ENTITY_SHADOW, cellPos(1, 1));
    Vec3 player = cellPos(6, 1);
    int t = sim.addTarget(player, yawTowards(player, e.pos));
    // Dead-end corridor: it cannot break line of sight, the player keeps staring.
    bool enraged = sim.runUntil(e, SHADOW_STARE_LIMIT + 1.0f, [&]() { return e.mood == MOOD_ENRAGED; });
    assert(enraged);
    bool hit = sim.runUntil(e, SHADOW_ENRAGED_SECONDS, [&]() { return e.state == ENT_ATTACKING; });
    assert(hit); // looking at it no longer helps
    (void)t;
}

void testShadowPicksTheLonerAndRespectsGroups() {
    setMaze({
        "################",
        "#..............#",
        "#..............#",
        "#..............#",
        "################",
    });
    Sim sim;
    Entity e = makeEntity(ENTITY_SHADOW, cellPos(7, 2));
    Vec3 a = cellPos(2, 1), b = cellPos(2, 2), loner = cellPos(13, 2);
    b.z -= 2.0f; // a and b stand 3 m apart
    sim.addTarget(a, PI_F);
    sim.addTarget(b, PI_F);
    int l = sim.addTarget(loner, 0.5f * PI_F); // facing away from the shadow (+X)
    sim.run(e, 0.5f);
    assert(e.targetId == l);
    bool hit = sim.runUntil(e, 20.0f, [&]() { return e.state == ENT_ATTACKING; });
    assert(hit);

    // The same squad standing together is left alone for a good while.
    Sim group;
    Entity g = makeEntity(ENTITY_SHADOW, cellPos(10, 2));
    group.addTarget(a, -0.5f * PI_F); // both facing -X: backs to the shadow
    group.addTarget(b, -0.5f * PI_F);
    bool attacked = group.runUntil(g, SHADOW_BOLD_AFTER * 0.8f, [&]() { return g.state == ENT_ATTACKING; });
    assert(!attacked);
    assert(brainDist(g.pos, a) > SHADOW_HOVER_NEAR - 1.5f);
    // ...but not forever.
    bool bold = group.runUntil(g, SHADOW_BOLD_AFTER, [&]() { return g.state == ENT_ATTACKING; });
    assert(bold);
}

void testDirectorBackOffSendsMonstersAway() {
    corridorMaze();
    Sim sim;
    sim.ctx.backOff = true;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(6, 1));
    Vec3 player = cellPos(8, 1);
    sim.addTarget(player, yawTowards(e.pos, player));
    float before = brainDist(e.pos, player);
    sim.run(e, 4.0f);
    assert(e.mood == MOOD_RETREAT && e.state != ENT_ATTACKING);
    assert(brainDist(e.pos, player) > before + 3.0f);
}

void testHintsPointMonstersWithoutRevealingPlayers() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_CRAWLER, cellPos(1, 3));
    sim.addTarget(cellPos(9, 1), 0.0f, 0.0f); // silent
    giveEntityHint(e, cellPos(8, 1));
    assert(e.mood == MOOD_INVESTIGATE);
    bool arrived = sim.runUntil(e, 30.0f, [&]() { return brainDist(e.pos, cellPos(8, 1)) < TCS * 0.5f; });
    assert(arrived);
    // It came to look, found nothing to hear, and gives up after sniffing.
    bool gaveUp = sim.runUntil(e, CRAWLER_SNIFF_SECONDS + 2.0f, [&]() { return e.mood == MOOD_WANDER; });
    assert(gaveUp);
}

void testNobodyToHuntMeansWandering() {
    corridorMaze();
    Sim sim;
    Entity e = makeEntity(ENTITY_STALKER, cellPos(2, 1));
    Vec3 start = e.pos;
    sim.run(e, 10.0f);
    assert(e.mood == MOOD_WANDER && e.state == ENT_ROAMING);
    assert(brainDist(e.pos, start) > 1.0f);
    assert(mazeCellAt((int)floorf(e.pos.x / TCS), (int)floorf(e.pos.z / TCS)) == 0);
}

void testDirectorCyclesThroughPeakAndRelax() {
    AiDirector d;
    DirectorInput in;
    in.dt = 0.1f;
    in.menace = 1.0f;
    in.anyEngaged = true;
    bool sawPeak = false, sawFade = false, sawRelax = false, spawnBlocked = false;
    for (int i = 0; i < 4000; i++) {
        if (d.phase != DIRECTOR_BUILD_UP) {
            in.menace = 0.0f; // the monsters were sent away
            in.anyEngaged = false;
        }
        DirectorPhase phase = d.phase; // the phase this frame's orders were issued in
        DirectorOutput out = updateDirector(d, in);
        if (phase == DIRECTOR_SUSTAIN_PEAK) {
            sawPeak = true;
            assert(!out.backOff); // the scene plays out before anybody is called off
        }
        if (phase == DIRECTOR_PEAK_FADE) {
            sawFade = true;
            assert(out.backOff);
        }
        if (phase == DIRECTOR_RELAX) {
            sawRelax = true;
            if (!out.allowSpawn) spawnBlocked = true;
            assert(!out.giveHint);
        }
        if (sawRelax && d.phase == DIRECTOR_BUILD_UP) break;
    }
    assert(sawPeak && sawFade && sawRelax && spawnBlocked);
    assert(d.phase == DIRECTOR_BUILD_UP);
}

void testDirectorHintsWhenItGetsTooQuiet() {
    AiDirector d;
    DirectorInput in;
    in.dt = 0.5f;
    int hints = 0;
    float firstHintAt = -1.0f;
    for (int i = 0; i < 400; i++) {
        DirectorOutput out = updateDirector(d, in);
        if (out.giveHint) {
            hints++;
            if (firstHintAt < 0.0f) firstHintAt = (float)(i + 1) * in.dt;
        }
    }
    assert(hints >= 2);
    assert(firstHintAt >= DIRECTOR_HINT_QUIET_SECONDS - 0.01f);

    // A noisy squad gets visitors sooner.
    AiDirector loud;
    in.attention = 1.0f;
    float loudHintAt = -1.0f;
    for (int i = 0; i < 400 && loudHintAt < 0.0f; i++) {
        if (updateDirector(loud, in).giveHint) loudHintAt = (float)(i + 1) * in.dt;
    }
    assert(loudHintAt > 0.0f && loudHintAt < firstHintAt * 0.6f);
}

void testDirectorGoesEasyOnTheLastOneStanding() {
    DirectorInput in;
    in.squadSize = 4;
    in.standing = 4;
    float full = directorAggression(in);
    in.standing = 1;
    float clutch = directorAggression(in);
    assert(clutch < full);
    in.squadSize = 1; // solo is not a clutch, it is just the game
    assert(directorAggression(in) == full);
    in.attention = 1.0f;
    assert(directorAggression(in) > full);
}

void testDirectorSpikesWhenSomebodyGoesDown() {
    AiDirector d;
    DirectorInput in;
    in.dt = 0.016f;
    updateDirector(d, in);
    float calm = d.intensity;
    in.newDowns = 1;
    updateDirector(d, in);
    assert(d.intensity >= calm + DIRECTOR_DOWN_SPIKE - 0.01f);
}

int main() {
    const char* seed = getenv("BRAIN_TEST_SEED"); // lets CI or a curious human shake the dice
    srand(seed ? (unsigned)atoi(seed) : 1234u);
    testStalkerFreezesWhileWatched();
    testStalkerClosesInWhenNobodyLooks();
    testStalkerIsNotFrozenThroughWalls();
    testStalkerInTheDarkNeedsAFlashlight();
    testStalkerChecksWhereItLastSawYou();
    testCrawlerIgnoresSilentPlayers();
    testCrawlerChargesFootsteps();
    testCrawlerFollowsSqueaksAndBonksIntoWalls();
    testCrawlerCanBeDodged();
    testCrawlerHearsThroughRoutesNotThroughWalls();
    testCrawlerChargeStartsEvenWhenHuggingAWall();
    testMonstersUseDoorwaysThroughTheMiddle();
    testMonstersWalkAroundPillars();
    testShadowSneaksUpAndFleesFromAGlance();
    testShadowSnapsWhenStaredAt();
    testShadowPicksTheLonerAndRespectsGroups();
    testDirectorBackOffSendsMonstersAway();
    testHintsPointMonstersWithoutRevealingPlayers();
    testNobodyToHuntMeansWandering();
    testDirectorCyclesThroughPeakAndRelax();
    testDirectorHintsWhenItGetsTooQuiet();
    testDirectorGoesEasyOnTheLastOneStanding();
    testDirectorSpikesWhenSomebodyGoesDown();
    std::cout << "All monster brain tests passed.\n";
    return 0;
}
