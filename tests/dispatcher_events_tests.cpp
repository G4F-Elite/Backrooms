#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include "../src/dispatcher_events.h"

struct EventRun {
    DispatcherEventObserver observer;
    DispatcherEventFrame state;
    std::vector<std::string> emitted;

    void step(float dt = 0.25f) {
        emitted.clear();
        observer.update(state, dt, [&](const char* id) { emitted.emplace_back(id); });
    }
    bool has(const char* id) const {
        return std::find(emitted.begin(), emitted.end(), id) != emitted.end();
    }
};

static void testMonsterVisibility() {
    const Vec3 eye(0, 1, 0), front(0, 0, 1), target(0, 1, 8);
    auto open = [](Vec3) { return true; };
    auto blocked = [](Vec3) { return false; };
    assert(dispatcherCanIdentifyMonster(true, true, eye, front, target, open));
    assert(!dispatcherCanIdentifyMonster(false, true, eye, front, target, open));
    assert(!dispatcherCanIdentifyMonster(true, false, eye, front, target, open));
    assert(!dispatcherCanIdentifyMonster(true, true, eye, front, target, blocked));
    assert(!dispatcherCanIdentifyMonster(true, true, eye, front, eye, open));
    assert(!dispatcherCanIdentifyMonster(true, true, eye, front, Vec3(0, 1, 15), open));
    assert(!dispatcherCanIdentifyMonster(true, true, eye, front, Vec3(0, 1, -3), open));
    assert(!dispatcherCanIdentifyMonster(true, true, eye, Vec3(0, 1, 0), target, open));
}

static void testTutorialAndRealSightings() {
    EventRun run;
    run.step();
    assert(run.has("tutorial_garage"));
    run.step();
    assert(run.emitted.empty());
    run.state.stage = HEIST_TUT_HAUL;
    run.step();
    assert(run.has("tutorial_haul"));
    run.state.stage = HEIST_TUT_CORE;
    run.step();
    assert(run.has("tutorial_core"));
    run.state.stage = HEIST_ACCIDENT;
    run.step();
    assert(run.has("accident"));
    run.state.stage = HEIST_RUN;
    run.state.monsters = 7;
    run.state.landmarks = (1u << dispatcherLandmarkIds.size()) - 1u;
    run.step();
    assert(run.has("run_briefing"));
    for (auto id : dispatcherMonsterIds) assert(run.has(id));
    for (auto id : dispatcherLandmarkIds) assert(run.has(id));
    run.step();
    assert(run.emitted.empty());
    run.state.monsters = 0;
    run.step();
    run.state.monsters = 7;
    run.step();
    for (auto id : dispatcherMonsterIds) {
        assert(run.has(id));
        run.observer.markSpoken(id);
    }
    run.state.monsters = 0;
    run.step();
    run.state.monsters = 7;
    run.step();
    assert(run.emitted.empty());
}

static void testVisibleMaskFollowsSightAndIndependentAcknowledgments() {
    EventRun run;
    std::vector<Entity> entities(2);
    const Vec3 eye(0, 1, 0), forward(0, 0, 1);
    for (Entity& entity : entities) {
        entity.active = entity.visible = true;
        entity.pos = Vec3(0, 0, 6);
    }
    entities[0].type = ENTITY_STALKER;
    entities[1].type = ENTITY_SHADOW;
    auto scan = [&](bool clear) {
        run.state.monsters = dispatcherVisibleMonsters(entities, eye, forward, [=](Vec3) { return clear; });
        run.step();
    };
    scan(true);
    assert(run.observer.previous.monsters == 5);
    assert(run.has("first_stalker") && run.has("first_shadow"));
    run.observer.markSpoken("first_shadow");
    run.observer.markSpoken("run_briefing");
    scan(true);
    assert(run.observer.previous.monsters == 5 && run.emitted.empty());
    scan(false);
    assert(run.observer.previous.monsters == 0);
    scan(true);
    assert(run.emitted == std::vector<std::string>{"first_stalker"});
    entities[0].visible = false;
    scan(true);
    assert(run.observer.previous.monsters == 4);
}

static void testLandmarkOnFirstFrameOfFloor() {
    EventRun run;
    run.state.landmarks = 1;
    run.step();
    assert(run.has("first_wall_door"));
    run.step();
    assert(run.emitted.empty());
    ++run.state.level;
    run.state.landmarks = 3;
    run.step();
    assert(run.has("first_wall_door") && run.has("first_ceiling_stair"));
    run.step();
    assert(run.emitted.empty());
}

static void testHeistEventsAndTransitions() {
    EventRun run;
    run.state.stage = HEIST_RUN;
    run.state.quotaNeed = 10;
    run.step();
    assert(run.emitted == std::vector<std::string>{"run_briefing"});
    run.state.clocks = 1;
    run.state.carried = 1;
    run.step();
    assert(run.emitted == std::vector<std::string>{"pickup_clock"});
    run.state.carried = HEIST_CARRY_SLOTS;
    run.state.lampBlinking = true;
    run.state.broken = true;
    run.state.breaks++;
    run.state.studied = 2;
    run.step();
    assert(run.has("cargo_full") && run.has("low_flashlight") && run.has("rig_fault"));
    assert(run.has("wall_broken") && run.has("study_complete"));
    run.state.quota = 4;
    run.state.broken = false;
    run.state.recovering = run.state.inCabin = true;
    run.step();
    assert(run.has("first_deposit") && run.has("rig_repaired") && run.has("cabin_recovery"));
    run.state.quota = 10;
    run.step();
    assert(run.has("quota_ready") && !run.has("first_deposit"));
    run.step();
    assert(run.emitted.empty());
    run.state.stage = HEIST_DRILLING;
    run.step();
    assert(run.has("drill_started"));
    run.state.broken = true;
    run.step();
    assert(run.emitted.empty());
    run.state.broken = false;
    run.step();
    assert(run.emitted.empty());
    run.state.level++;
    run.state.stage = HEIST_RUN;
    run.state.breaks++;
    run.state.broken = true;
    run.step();
    assert(run.emitted == std::vector<std::string>{"floor_descent"});
}

static void testFloorSystemsFromActualTransitions() {
    EventRun run;
    run.state.stage = HEIST_RUN;
    run.step();
    run.state.blackout = run.state.chargerReady = run.state.reserveSpent = true;
    run.state.scannerHot = run.state.farFromRig = true;
    run.step();
    assert(run.has("floor_blackout") && run.has("charger_ready") && run.has("reserve_spent"));
    assert(run.has("scanner_overheat") && run.has("far_from_rig"));
    run.step();
    assert(run.emitted.empty());
    run.state.blackout = run.state.chargerReady = run.state.reserveSpent = false;
    run.state.scannerHot = run.state.farFromRig = false;
    run.step();
    assert(run.emitted == (std::vector<std::string>{"scanner_cooled", "lights_return"}));
    run.state.stage = HEIST_ACCIDENT;
    run.state.blackout = true;
    run.step();
    assert(run.has("accident") && !run.has("floor_blackout"));
    ++run.state.level;
    run.state.stage = HEIST_RUN;
    run.state.reserveSpent = run.state.chargerReady = true;
    run.state.blackout = false;
    run.step();
    assert(run.emitted.empty());
}

static void testSpecificPickupConsumesFirstLoadEvenIfRadioDropsIt() {
    EventRun run;
    run.state.stage = HEIST_RUN;
    run.step();
    run.state.clocks = run.state.carried = 1;
    std::vector<std::string> rejected;
    run.observer.update(run.state, 0.25f, [&](const char* id) { rejected.emplace_back(id); return false; });
    assert(rejected == std::vector<std::string>{"pickup_clock"});
    ++run.state.seed;
    run.state.clocks = run.state.carried = 0;
    run.step();
    run.state.carried = 1;
    run.step();
    assert(run.emitted.empty());

    EventRun restored;
    restored.state.stage = HEIST_RUN;
    restored.state.clocks = restored.state.carried = 1;
    restored.step();
    assert(!restored.has("pickup_clock") && !restored.has("first_load"));
    restored.state.clocks = restored.state.carried = 0;
    restored.step();
    restored.state.clocks = restored.state.carried = 1;
    restored.step();
    assert(restored.emitted.empty());
}

static void testOrdinaryFirstLoadThenClock() {
    EventRun run;
    run.step();
    run.state.carried = 1;
    run.step();
    assert(run.emitted == std::vector<std::string>{"first_load"});
    run.state.carried = 2;
    run.state.clocks = 1;
    run.step();
    assert(run.emitted == std::vector<std::string>{"pickup_clock"});
}

static void testRoutesNeedDistinctSectionsAndResetBetweenFloors() {
    EventRun run;
    run.state.stage = HEIST_RUN;
    run.state.slab = -3;
    run.step();
    for (int key : {1, -3, 1, -3, 1, 2}) {
        run.state.slab = key;
        run.step();
        assert(!run.has("route_return"));
    }
    run.state.slab = -3;
    run.step();
    assert(run.has("route_return"));
    run.step();
    assert(!run.has("route_return"));
    ++run.state.seed;
    run.state.slab = 1;
    run.step();
    assert(!run.has("route_return"));
    for (int key = 0; key < 20; ++key) { run.state.slab = key; run.step(); }
    run.state.slab = 0;
    run.step();
    assert(!run.has("route_return")); // a route from much earlier has expired
}

static void testWalkingInDarkAndStoppingAfterAWalk() {
    EventRun run;
    run.state.stage = HEIST_RUN;
    run.state.slab = 3;
    run.state.walking = true;
    run.state.lampOn = false;
    run.step();
    for (int i = 0; i < 35; ++i) { run.step(); assert(!run.has("dark_walk")); }
    run.state.lampOn = true;
    run.step();
    run.state.lampOn = false;
    for (int i = 0; i < 47; ++i) { run.step(); assert(!run.has("dark_walk")); }
    run.step();
    assert(run.has("dark_walk"));
    run.step();
    assert(!run.has("dark_walk"));
    run.state.walking = false;
    run.state.footStill = true;
    for (int i = 0; i < 19; ++i) { run.step(); assert(!run.has("pause_to_listen")); }
    run.step();
    assert(run.has("pause_to_listen"));
    run.step();
    assert(!run.has("pause_to_listen"));
    run.state.slab.reset(); // a stopped rig deck or a spectator never counts as a stop on foot
    for (int i = 0; i < 60; ++i) { run.step(); assert(run.emitted.empty()); }
}

static void testRunBriefingAcrossWorldChanges() {
    EventRun run;
    run.step();
    assert(run.has("tutorial_garage"));
    ++run.state.level;
    ++run.state.seed;
    run.state.stage = HEIST_RUN;
    run.step();
    assert(run.emitted == std::vector<std::string>{"run_briefing"});
    ++run.state.seed;
    run.step();
    assert(run.emitted.empty());
    ++run.state.level;
    run.step();
    assert(run.emitted.empty());
}

static void testWalkingRequiresQuietPlayerMovement() {
    EventRun run;
    run.state.stage = HEIST_RUN;
    run.step();
    for (int i = 0; i < 150; ++i) { run.step(); assert(run.emitted.empty()); }
    run.state.walking = true;
    for (int i = 0; i < 138; ++i) { run.step(); assert(run.emitted.empty()); }
    run.state.witnessed = 64;
    run.step();
    assert(run.emitted.empty());
    run.step(40.0f);
    assert(run.emitted.empty());
    for (int i = 0; i < 139; ++i) { run.step(); assert(run.emitted.empty()); }
    run.step();
    assert(run.has("long_walk"));
    run.step();
    assert(run.emitted.empty());
}

int main() {
    testMonsterVisibility();
    testVisibleMaskFollowsSightAndIndependentAcknowledgments();
    testLandmarkOnFirstFrameOfFloor();
    testTutorialAndRealSightings();
    testHeistEventsAndTransitions();
    testRunBriefingAcrossWorldChanges();
    testFloorSystemsFromActualTransitions();
    testSpecificPickupConsumesFirstLoadEvenIfRadioDropsIt();
    testOrdinaryFirstLoadThenClock();
    testRoutesNeedDistinctSectionsAndResetBetweenFloors();
    testWalkingInDarkAndStoppingAfterAWalk();
    testWalkingRequiresQuietPlayerMovement();
}
