#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <vector>
#include "entity_types.h"
#include "heist_rules.h"
#include "math.h"

template<class ClearSight>
inline bool dispatcherCanIdentifyMonster(bool active, bool visible, const Vec3& eye, const Vec3& forward,
                                        const Vec3& target, ClearSight clearSight) {
    if (!active || !visible) return false;
    const Vec3 ray = target - eye;
    const float distance = ray.len();
    // Identification uses the centre of the player's view, including camera pitch.
    return distance >= 0.15f && distance <= 14.0f &&
           forward.dot(ray) >= 0.94f * distance && clearSight(target);
}

// Facts sampled on this peer. Counts and stage belong to the squad; the carry,
// lamp, sights and landmarks belong to this particular player.
struct DispatcherEventFrame {
    unsigned seed = 0;
    int level = 0;
    int stage = HEIST_TUT_GARAGE;
    int quota = 0, quotaNeed = 0, breaks = 0, studied = 0;
    int carried = 0, clocks = 0;
    bool broken = false, lampBlinking = false, inCabin = false, recovering = false;
    bool walking = false;
    bool footStill = false, lampOn = true, farFromRig = false;
    bool scannerHot = false, blackout = false, chargerReady = false, reserveSpent = false;
    std::optional<long long> slab; // only on foot, outside the rig
    unsigned monsters = 0, landmarks = 0;
    // Any newly witnessed geography interrupts a quiet walk, including features
    // for which the dispatcher has no dialogue.
    unsigned long long witnessed = 0;
};

inline constexpr std::array<const char*, 3> dispatcherMonsterIds = {
    "first_stalker", "first_crawler", "first_shadow"
};
inline constexpr std::array<const char*, 14> dispatcherLandmarkIds = {
    "first_wall_door", "first_ceiling_stair", "first_echo_hall", "first_chasm", "first_twin",
    "first_facewall", "first_vitrine", "first_plugged_hall", "first_nested_rooms", "first_spiral",
    "first_skywell", "first_basin", "first_dark_wing", "first_xenograft"
};

template<class ClearSight>
inline unsigned dispatcherVisibleMonsters(const std::vector<Entity>& entities, const Vec3& eye,
                                          const Vec3& forward, ClearSight clearSight) {
    static_assert(ENTITY_STALKER == 1 && ENTITY_CRAWLER == 2 && ENTITY_SHADOW == 3);
    unsigned mask = 0;
    for (const Entity& e : entities) {
        if (e.type < ENTITY_STALKER || e.type > ENTITY_SHADOW) continue;
        const unsigned bit = 1u << (static_cast<unsigned>(e.type) - 1u);
        if (mask & bit) continue;
        Vec3 target = e.pos;
        target.y = e.type == ENTITY_CRAWLER ? 0.5f : e.pos.y + 1.0f;
        if (dispatcherCanIdentifyMonster(e.active, e.visible, eye, forward, target, clearSight)) mask |= bit;
    }
    return mask;
}

struct DispatcherEventObserver {
    DispatcherEventFrame previous;
    bool initialized = false;
    unsigned spokenMonsters = 0;
    bool announcedLoad = false, announcedClock = false;
    bool gaveRunBriefing = false;
    float walkingSeconds = 0;
    float approachSeconds = 0, stoppedSeconds = 0, unlitWalkSeconds = 0;
    std::array<long long, 8> recentSlabs{};
    size_t slabCount = 0;

    void markSpoken(std::string_view event) {
        for (unsigned i = 0; i < dispatcherMonsterIds.size(); ++i) {
            if (event == dispatcherMonsterIds[i]) spokenMonsters |= 1u << i;
        }
    }

    // Most-recent first. A real loop crosses at least two distinct other sections;
    // pacing back and forth across a threshold never qualifies.
    bool returnedToSlab(long long key) {
        size_t i = 0;
        while (i < slabCount && recentSlabs[i] != key) ++i;
        const bool loop = i < slabCount && i >= 2;
        if (i == slabCount && slabCount < recentSlabs.size()) ++slabCount;
        if (i == recentSlabs.size()) --i;
        for (; i > 0; --i) recentSlabs[i] = recentSlabs[i - 1];
        recentSlabs[0] = key;
        return loop;
    }

    template<class Emit>
    void observeFootsteps(const DispatcherEventFrame& now, float dt, Emit emit) {
        if (!now.slab || now.stage != HEIST_RUN) {
            slabCount = 0;
            approachSeconds = stoppedSeconds = unlitWalkSeconds = 0;
            return;
        }
        if ((!slabCount || recentSlabs[0] != *now.slab) && returnedToSlab(*now.slab)) emit("route_return");
        if (!(dt > 0 && dt <= 0.5f)) return;
        if (now.walking) {
            stoppedSeconds = 0;
            approachSeconds += dt;
        } else if (now.footStill) {
            stoppedSeconds += dt;
            if (stoppedSeconds >= 5.0f) {
                if (approachSeconds >= 6.0f) emit("pause_to_listen");
                approachSeconds = 0;
            }
        }
        if (now.walking && !now.lampOn) {
            const float before = unlitWalkSeconds;
            unlitWalkSeconds += dt;
            if (before < 12.0f && unlitWalkSeconds >= 12.0f) emit("dark_walk");
        } else unlitWalkSeconds = 0;
    }

    template<class Emit>
    void update(const DispatcherEventFrame& now, float dt, Emit emit) {
        const bool first = !initialized;
        const bool newFloor = first || now.level != previous.level || now.seed != previous.seed;
        // An existing status on join, reload or descent is a baseline, not a transaction.
        if (newFloor) {
            if (!first && now.level > previous.level && previous.stage == HEIST_DRILLING && now.stage == HEIST_RUN)
                emit("floor_descent");
            previous = now;
            previous.monsters = previous.landmarks = 0;
            // Cargo already present on reconnect or respawn belongs to an earlier pickup.
            announcedLoad |= now.carried > 0;
            announcedClock |= now.clocks > 0;
            walkingSeconds = 0;
            approachSeconds = stoppedSeconds = unlitWalkSeconds = 0;
            slabCount = 0;
            initialized = true;
            if (first && now.stage == HEIST_TUT_GARAGE) emit("tutorial_garage");
        }
        if (now.stage == HEIST_RUN && !gaveRunBriefing) {
            emit("run_briefing");
            gaveRunBriefing = true;
        }
        if (now.stage != previous.stage) {
            switch (now.stage) {
                case HEIST_TUT_HAUL: emit("tutorial_haul"); break;
                case HEIST_TUT_CORE: emit("tutorial_core"); break;
                case HEIST_ACCIDENT: emit("accident"); break;
                case HEIST_DRILLING: emit("drill_started"); break;
            }
        }
        unsigned newMonsters = now.monsters & ~previous.monsters & ~spokenMonsters;
        for (unsigned i = 0; i < dispatcherMonsterIds.size(); ++i) {
            if (newMonsters & (1u << i)) emit(dispatcherMonsterIds[i]);
        }
        unsigned newLandmarks = now.landmarks & ~previous.landmarks;
        for (unsigned i = 0; i < dispatcherLandmarkIds.size(); ++i) {
            if (newLandmarks & (1u << i)) emit(dispatcherLandmarkIds[i]);
        }
        if (!announcedClock && now.clocks > previous.clocks) {
            announcedClock = true;
            announcedLoad = true;
            emit("pickup_clock");
        } else if (!announcedLoad && now.carried > 0 && previous.carried == 0) {
            announcedLoad = true;
            emit("first_load");
        }
        if (now.carried == HEIST_CARRY_SLOTS && previous.carried < now.carried) emit("cargo_full");
        if (now.lampBlinking && !previous.lampBlinking) emit("low_flashlight");
        if (now.scannerHot != previous.scannerHot) emit(now.scannerHot ? "scanner_overheat" : "scanner_cooled");
        if (now.breaks > previous.breaks) emit("wall_broken");
        if ((now.studied & ~previous.studied) != 0) emit("study_complete");
        if (now.stage == HEIST_RUN && previous.stage == HEIST_RUN) {
            if (now.quota > previous.quota && previous.quota == 0) emit("first_deposit");
            if (now.quotaNeed > 0 && previous.quota < now.quotaNeed && now.quota >= now.quotaNeed)
                emit("quota_ready");
            if (now.broken != previous.broken) emit(now.broken ? "rig_fault" : "rig_repaired");
            if (now.inCabin && !previous.inCabin && now.recovering) emit("cabin_recovery");
            if (now.farFromRig && !previous.farFromRig) emit("far_from_rig");
            if (now.blackout != previous.blackout) emit(now.blackout ? "floor_blackout" : "lights_return");
            if (now.chargerReady && !previous.chargerReady) emit("charger_ready");
            if (now.reserveSpent && !previous.reserveSpent) emit("reserve_spent");
        }
        observeFootsteps(now, newFloor ? 0.0f : dt, emit);
        if (newMonsters || now.witnessed != previous.witnessed || now.stage != previous.stage) {
            walkingSeconds = 0;
        } else if (now.walking && !newFloor && dt > 0 && dt <= 0.5f) {
            walkingSeconds += dt;
            if (walkingSeconds >= 35.0f) {
                emit("long_walk");
                walkingSeconds = 0;
            }
        }
        previous = now;
    }
};
