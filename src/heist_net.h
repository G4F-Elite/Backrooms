#pragma once
// Wire form of the heist state (host -> clients, inside NetVoidShiftState).

#include "math.h"
#include "heist_rules.h"

inline constexpr int HEIST_NET_PLAYERS = 4; // == MAX_PLAYERS (net_types.h)

struct NetHeistState {
    unsigned char stage;
    unsigned char tutDelivered;
    unsigned char studiedMask;
    unsigned char breakCount;
    unsigned char rigDir;
    unsigned char rigFault;
    unsigned char rigFaultSecs;   // capped, whole seconds
    unsigned char rigCharge;      // charger progress, 0..255
    short quotaHave;
    short quotaNeed;
    int cash;
    Vec3 drillPos;                // the rig, wherever it has crawled to
    Vec3 rigStart;
    Vec3 kioskPos;
    Vec3 lessonPos[HEIST_TUTORIAL_LOOT];
    Vec3 corePos;
    signed char carry[HEIST_NET_PLAYERS][HEIST_CARRY_SLOTS];
    signed char breakDX[HEIST_MAX_BREAKS];
    signed char breakDZ[HEIST_MAX_BREAKS];
};
