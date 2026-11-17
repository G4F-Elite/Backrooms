#pragma once
// Heist state: everything about the loot run that the host owns and mirrors to the squad,
// plus the local-only feedback (toasts, flashes, hold progress).

#include "heist_rules.h"
#include "heist_net.h"
#include "rig_rules.h"
#include "hud_icon_ids.h"

struct HeistState {
    int stage = HEIST_TUT_GARAGE;
    int tutDelivered = 0;
    int quotaHave = 0;
    int quotaNeed = 0;
    int cash = 0;
    int studiedMask = 0;      // bit per EntityType: field notes already sold on this floor
    bool hasGarage = false;   // floor 0 only: the home side of the portal
    bool hasKiosk = false;    // floors below: the rival crew's stall
    int baseX = 0, baseZ = 0; // squad arrival cell; layouts hang off it
    Vec3 drillPos;            // origin of the rig; it moves (see rig_state.h)
    Vec3 rigStart;            // centre of the cell the rig arrived in
    int rigDir = 0;           // heading, fixed for the whole floor
    int rigFault = RIG_OK;
    float rigFaultSecs = 0.0f;   // how long it has been broken
    float rigCharge = 0.0f;      // battery charger, 0..1
    float rigNextFault = 60.0f;  // host: engine seconds until the next breakdown
    int rigLootStep = 0;         // host: lane step loot was last laid out for
    Vec3 kioskPos;
    Vec3 portalPos;           // centre of the portal cell
    Vec3 padPos;              // delivery pallet in the garage
    Vec3 lessonPos[HEIST_TUTORIAL_LOOT];
    Vec3 corePos;
    float stageTimer = 0.0f;  // seconds in the current stage
    signed char carry[MAX_PLAYERS][HEIST_CARRY_SLOTS];
    int breakCount = 0;
    signed char breakDX[HEIST_MAX_BREAKS];
    signed char breakDZ[HEIST_MAX_BREAKS];
};

inline HeistState heist;

// ---- local feedback -------------------------------------------------------------------
struct HeistToast {
    int icon;
    int amount;     // 0 = icon only
    float ttl;
    bool good;
};
inline constexpr int HEIST_TOAST_MAX = 5;
inline constexpr float HEIST_TOAST_SECONDS = 2.6f;
inline HeistToast gHeistToasts[HEIST_TOAST_MAX] = {};
inline float gHeistFlash = 0.0f;        // white-out after the accident / while the drill bites
inline float gHeistHold = 0.0f;         // 0..1 progress of the current hold action
inline int gHeistHoldKind = 0;          // which prompt the hold belongs to
inline float gHeistStudy = 0.0f;        // 0..1 monster study progress
inline int gHeistStudyType = 0;
inline int gHeistAppliedBreaks = 0;     // how many entries of heist.break* this peer has carved
inline bool gHeistShopOpen = false;     // standing at the kiosk

inline void heistToast(int icon, int amount, bool good) {
    for (int i = HEIST_TOAST_MAX - 1; i > 0; i--) gHeistToasts[i] = gHeistToasts[i - 1];
    gHeistToasts[0] = {icon, amount, HEIST_TOAST_SECONDS, good};
}

inline int heistMe() {
    return multiState == MULTI_IN_GAME ? netMgr.myId : 0;
}

inline bool heistIsAuthority() {
    return multiState != MULTI_IN_GAME || netMgr.isHost;
}

inline signed char* heistMyCarry() {
    return heist.carry[heistMe()];
}

inline int heistSquadSize() {
    if (multiState != MULTI_IN_GAME) return 1;
    int n = 0;
    for (int p = 0; p < MAX_PLAYERS; p++) if (netMgr.players[p].active) n++;
    return n < 1 ? 1 : n;
}

inline Vec3 heistCellCenter(int wx, int wz) {
    return Vec3(((float)wx + 0.5f) * CS, 0.0f, ((float)wz + 0.5f) * CS);
}

// Monsters, wall shifts and floor events only start once the portal is gone.
inline bool heistDangerActive() {
    return !heistStageIsSafe(heist.stage) && heist.stage != HEIST_ACCIDENT;
}

inline bool heistDrillReady() {
    return heist.stage == HEIST_RUN && heist.quotaHave >= heist.quotaNeed;
}

inline void captureHeistNet(NetHeistState& out) {
    out.stage = (unsigned char)heist.stage;
    out.tutDelivered = (unsigned char)heist.tutDelivered;
    out.studiedMask = (unsigned char)heist.studiedMask;
    out.breakCount = (unsigned char)heist.breakCount;
    out.quotaHave = (short)heist.quotaHave;
    out.quotaNeed = (short)heist.quotaNeed;
    out.cash = heist.cash;
    out.drillPos = heist.drillPos;
    out.rigStart = heist.rigStart;
    out.rigDir = (unsigned char)heist.rigDir;
    out.rigFault = (unsigned char)heist.rigFault;
    out.rigFaultSecs = (unsigned char)(heist.rigFaultSecs > 250.0f ? 250.0f : heist.rigFaultSecs);
    out.rigCharge = (unsigned char)(heist.rigCharge * 255.0f);
    out.kioskPos = heist.kioskPos;
    for (int i = 0; i < HEIST_TUTORIAL_LOOT; i++) out.lessonPos[i] = heist.lessonPos[i];
    out.corePos = heist.corePos;
    memcpy(out.carry, heist.carry, sizeof(out.carry));
    memcpy(out.breakDX, heist.breakDX, sizeof(out.breakDX));
    memcpy(out.breakDZ, heist.breakDZ, sizeof(out.breakDZ));
}

#include "rig_state.h"
