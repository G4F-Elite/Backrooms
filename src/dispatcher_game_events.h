#pragma once

// Included by the client after its gameplay and LIMEN observer definitions.
#include "dispatcher_events.h"

template<class Emit>
inline void pollDispatcherGameEvents(DispatcherEventObserver& observer, Emit emit) {
    DispatcherEventFrame now;
    now.seed = worldSeed;
    now.level = gCurrentLevel;
    now.stage = heist.stage;
    now.quota = heist.quotaHave;
    now.quotaNeed = heist.quotaNeed;
    now.studied = heist.studiedMask;
    now.breaks = heist.breakCount;
    now.broken = heist.rigFault != RIG_OK;
    now.reserveSpent = now.broken && !rigOnReserve(heist.rigFault, heist.rigFaultSecs);
    now.chargerReady = rigChargerReady();
    now.scannerHot = scannerOverheated;
    now.blackout = lightsOutTimer > 0;
    now.lampOn = flashlightOn;
    now.farFromRig = !nearPoint2D(cam.pos, heist.drillPos, 45.0f);
    now.lampBlinking = flashlightShutdownBlinkActive;
    now.inCabin = gRigInCabin;
    now.recovering = rigEngineOn() && !playerDowned && playerHealth < RIG_CABIN_HEAL_CAP;
    const signed char* carry = heistMyCarry();
    now.carried = carryCount(carry);
    for (int i = 0; i < HEIST_CARRY_SLOTS; ++i) now.clocks += carry[i] == LOOT_CLOCK;

    constexpr LmFeat features[] = {
        LN_WALL_DOOR, LN_CEIL_STAIR, LN_ECHO, LN_CHASM, LN_TWIN,
        LN_FACEWALL, LN_VITRINE, LN_PLUG, LN_NESTED, LN_SPIRAL,
        LN_SKYWELL, LN_BASIN_WALK, LN_DARK, LN_XENO
    };
    static_assert(sizeof(features) / sizeof(*features) == dispatcherLandmarkIds.size());
    const RigPoint local = rigLocal(cam.pos);
    if (gNar.ready && gNar.level == now.level && gNar.seed == now.seed) {
        if (rigLocalPlayerOnFoot() && !playerDowned && !rigOnBoard(local.x, local.z)) {
            const LimenRect slab = lmSlabAt((int)floorf(cam.pos.x / CS), (int)floorf(cam.pos.z / CS));
            now.slab = lmSlabKey(slab.j, slab.i);
            now.walking = gNar.speed > 1.5f;
            now.footStill = gNar.speed < 0.25f;
        }
        now.witnessed = gNar.levelWitnessed;
        for (unsigned i = 0; i < dispatcherLandmarkIds.size(); ++i) {
            if (now.witnessed & lmFeatBit(features[i])) now.landmarks |= 1u << i;
        }
    }
    const Vec3 forward(mSin(cam.yaw) * mCos(cam.pitch), mSin(cam.pitch), mCos(cam.yaw) * mCos(cam.pitch));
    if (lightsOutTimer <= 0 || (flashlightOn && !flashlightShutdownBlinkActive)) {
        now.monsters = dispatcherVisibleMonsters(entityMgr.entities, cam.pos, forward, hasLabelLineOfSight);
    }
    observer.update(now, dTime, emit);
}
