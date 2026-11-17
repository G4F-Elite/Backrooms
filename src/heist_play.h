#pragma once
// Heist, local side: what the player can do right now (the prompt), key handling for
// carrying / rummaging / breaking walls / shopping / studying monsters, and the feedback
// that follows a change in the shared run (toasts, the accident, the drill going down).

#include "heist_host.h"
#include "squad_play.h"
#include "rig_play.h"

enum HeistPromptKind {
    HEIST_PROMPT_NONE = 0,
    HEIST_PROMPT_PICK_LOOT,
    HEIST_PROMPT_PICK_SUPPLY,
    HEIST_PROMPT_HANDS_FULL,
    HEIST_PROMPT_RUMMAGE,      // hold
    HEIST_PROMPT_FEED_DRILL,
    HEIST_PROMPT_DRILL_HUNGRY, // nothing to feed it with
    HEIST_PROMPT_START_DRILL,
    HEIST_PROMPT_DELIVER,
    HEIST_PROMPT_SELL,
    HEIST_PROMPT_BREAK,        // hold
    HEIST_PROMPT_REPAIR,       // hold: the rig broke down again
    HEIST_PROMPT_CHARGER       // a battery is ready in the rig
};

struct HeistPrompt {
    int kind = HEIST_PROMPT_NONE;
    int itemId = -1;
    int itemType = -1;
    int side = 0;    // wall side for HEIST_PROMPT_BREAK
    int amount = 0;  // value shown next to the icon
};

inline constexpr float HEIST_PICK_RANGE = 2.2f;
inline constexpr float HEIST_RUMMAGE_SECONDS = 1.6f;
inline constexpr float HEIST_BREAK_SECONDS = 2.4f;
inline constexpr float HEIST_BREAK_REACH = 1.7f;
inline constexpr float HEIST_STUDY_SECONDS = 3.2f;
inline constexpr float HEIST_STUDY_RANGE = 15.0f;
inline constexpr float HEIST_SCAN_MARK_RANGE = 24.0f;

inline HeistPrompt gHeistPrompt;

inline bool heistPromptIsHold(int kind) {
    return kind == HEIST_PROMPT_RUMMAGE || kind == HEIST_PROMPT_BREAK || kind == HEIST_PROMPT_REPAIR;
}

inline float heistHoldSeconds(int kind) {
    if (kind == HEIST_PROMPT_BREAK) return HEIST_BREAK_SECONDS;
    if (kind == HEIST_PROMPT_REPAIR) return RIG_REPAIR_SECONDS;
    return HEIST_RUMMAGE_SECONDS;
}

// The wall cell the player is pressed against and looking at, if it is a cracked one.
inline bool heistFacingCrack(int& side) {
    float fx = mSin(cam.yaw), fz = mCos(cam.yaw);
    side = fabsf(fx) > fabsf(fz) ? (fx > 0.0f ? 0 : 1) : (fz > 0.0f ? 2 : 3);
    static const int dx[4] = {1, -1, 0, 0};
    static const int dz[4] = {0, 0, 1, -1};
    int cx = heistCellX(cam.pos), cz = heistCellZ(cam.pos);
    if (!isCrackedWall(cx + dx[side], cz + dz[side])) return false;
    float lx = cam.pos.x - (float)cx * CS, lz = cam.pos.z - (float)cz * CS;
    float gap = side == 0 ? CS - lx : (side == 1 ? lx : (side == 2 ? CS - lz : lz));
    return gap < HEIST_BREAK_REACH;
}

inline HeistPrompt heistFindPrompt() {
    HeistPrompt p;
    signed char* carry = heistMyCarry();
    int carried = carryCount(carry);

    // The rig first: a breakdown outranks everything, then the hopper when there is something
    // to do at it, then the charger.
    RigPoint me = rigLocal(cam.pos);
    int spot = heist.stage == HEIST_RUN ? rigSpotAt(me.x, me.z, heist.rigFault, rigChargerReady()) : RIG_SPOT_NONE;
    if (spot == RIG_SPOT_FAULT) {
        p.kind = HEIST_PROMPT_REPAIR;
        return p;
    }
    if (spot == RIG_SPOT_HOPPER && (heistDrillReady() || carried > 0)) {
        p.kind = heistDrillReady() ? HEIST_PROMPT_START_DRILL : HEIST_PROMPT_FEED_DRILL;
        p.amount = carryValue(carry, gCurrentLevel);
        return p;
    }
    if (spot == RIG_SPOT_CHARGER) {
        p.kind = HEIST_PROMPT_CHARGER;
        return p;
    }

    for (const auto& it : worldItems) {
        if (!it.active || !nearPoint2D(cam.pos, it.pos, HEIST_PICK_RANGE)) continue;
        p.itemId = it.id;
        p.itemType = it.type;
        if (it.type == ITEM_STASH) p.kind = HEIST_PROMPT_RUMMAGE;
        else if (!isLootItemType(it.type)) p.kind = HEIST_PROMPT_PICK_SUPPLY;
        else if (carried >= HEIST_CARRY_SLOTS) p.kind = HEIST_PROMPT_HANDS_FULL;
        else {
            p.kind = HEIST_PROMPT_PICK_LOOT;
            p.amount = lootValueAt(lootFromItemType(it.type), gCurrentLevel);
        }
        return p;
    }

    bool facingHopper = mSin(cam.yaw) * (float)rigDirX(heist.rigDir) + mCos(cam.yaw) * (float)rigDirZ(heist.rigDir) > 0.3f;
    if (spot == RIG_SPOT_HOPPER && facingHopper) {
        p.kind = HEIST_PROMPT_DRILL_HUNGRY; // nothing to feed it with
        p.amount = heist.quotaNeed - heist.quotaHave;
        return p;
    }
    bool hauling = heist.stage == HEIST_TUT_HAUL || heist.stage == HEIST_TUT_CORE;
    if (hauling && carried > 0 && nearPoint2D(cam.pos, heist.padPos, HEIST_PAD_REACH)) {
        p.kind = HEIST_PROMPT_DELIVER;
        return p;
    }
    if (gHeistShopOpen && carried > 0) {
        p.kind = HEIST_PROMPT_SELL;
        p.amount = carryValue(carry, gCurrentLevel);
        return p;
    }
    if (heistFacingCrack(p.side)) p.kind = HEIST_PROMPT_BREAK;
    return p;
}

inline void heistRequestAction(int action) {
    if (heistIsAuthority()) heistHostAction(heistMe(), action);
    else netMgr.sendInteractRequest(REQ_HEIST_ACTION, action);
}

inline void heistRequestPick(const HeistPrompt& p) {
    if (heistIsAuthority()) {
        heistHostPick(heistMe(), p.itemId);
        return;
    }
    netMgr.sendInteractRequest(REQ_PICK_ITEM, p.itemId);
    if (p.itemType == ITEM_MED_SPRAY) applyItemUse(ITEM_MED_SPRAY);
}

inline void heistRunPrompt(const HeistPrompt& p) {
    if (p.kind == HEIST_PROMPT_PICK_LOOT || p.kind == HEIST_PROMPT_PICK_SUPPLY || p.kind == HEIST_PROMPT_RUMMAGE) {
        heistRequestPick(p);
        triggerToySound(TOY_CALLOUT, 0.35f, p.kind == HEIST_PROMPT_RUMMAGE ? 0.7f : 1.25f);
    } else if (p.kind == HEIST_PROMPT_FEED_DRILL || p.kind == HEIST_PROMPT_DELIVER) {
        heistRequestAction(HEIST_ACT_DEPOSIT);
        bumpStat(myShiftStats().work);
    } else if (p.kind == HEIST_PROMPT_START_DRILL) {
        heistRequestAction(HEIST_ACT_START_DRILL);
    } else if (p.kind == HEIST_PROMPT_SELL) {
        heistRequestAction(HEIST_ACT_SELL);
        bumpStat(myShiftStats().work);
    } else if (p.kind == HEIST_PROMPT_REPAIR) {
        heistRequestAction(HEIST_ACT_REPAIR);
        bumpStat(myShiftStats().work);
    } else if (p.kind == HEIST_PROMPT_CHARGER) {
        heistRequestAction(HEIST_ACT_BATTERY);
        triggerToySound(TOY_CALLOUT, 0.35f, 1.25f);
    } else if (p.kind == HEIST_PROMPT_BREAK) {
        if (heistIsAuthority()) heistHostBreak(heistMe(), p.side);
        else netMgr.sendInteractRequest(REQ_HEIST_BREAK, p.side);
        camShake = 0.35f;
        triggerToySound(TOY_SHOVE, 0.9f, 0.45f);
        if (multiState == MULTI_IN_GAME) emitSquadNoise(netMgr.myId, HEIST_BREAK_NOISE);
        bumpStat(myShiftStats().work);
    }
}

// Interact key: taps act at once, holds (rummage, break) fill a ring first.
inline void heistHandleInteract(bool held, bool pressedNow) {
    static int holdKind = HEIST_PROMPT_NONE;
    static float thudTimer = 0.0f;
    const HeistPrompt& p = gHeistPrompt;
    if (!heistPromptIsHold(p.kind)) {
        gHeistHold = 0.0f;
        holdKind = HEIST_PROMPT_NONE;
        if (pressedNow && p.kind != HEIST_PROMPT_NONE) heistRunPrompt(p);
        return;
    }
    if (!held || holdKind != p.kind) {
        gHeistHold = 0.0f;
        holdKind = held ? p.kind : HEIST_PROMPT_NONE;
        return;
    }
    bool breaking = p.kind == HEIST_PROMPT_BREAK;
    gHeistHold += dTime / heistHoldSeconds(p.kind);
    gHeistHoldKind = p.kind;
    thudTimer -= dTime;
    if (thudTimer <= 0.0f) {
        thudTimer = breaking ? 0.55f : 0.40f;
        if (breaking) camShake = 0.16f;
        triggerToySound(TOY_SHOVE, breaking ? 0.6f : 0.25f, breaking ? 0.6f : 1.4f);
        // Digging is loud: the blind ones come to check.
        emitSquadNoise(heistMe(), breaking ? NOISE_CALLOUT : NOISE_WALK);
    }
    if (gHeistHold < 1.0f) return;
    gHeistHold = 0.0f;
    holdKind = HEIST_PROMPT_NONE;
    heistRunPrompt(p);
}

// Kiosk: the four item keys buy instead of switching tools. Returns true while shopping.
inline bool heistHandleShopKeys(bool k1, bool k2, bool k3, bool k4) {
    if (!gHeistShopOpen) return false;
    bool keys[SHOP_ITEM_COUNT] = {k1, k2, k3, k4};
    for (int item = 0; item < SHOP_ITEM_COUNT; item++) {
        if (!keys[item]) continue;
        bool affordable = heist.cash >= shopPrice(item, heistSquadSize());
        if (!affordable) {
            triggerToySound(TOY_SHOVE, 0.3f, 1.6f);
            continue;
        }
        if (heistIsAuthority()) heistHostBuy(heistMe(), item);
        else netMgr.sendInteractRequest(REQ_HEIST_BUY, item);
    }
    return true;
}

// Scanner out + monster in view = field notes. The rivals pay for those.
inline void heistUpdateStudy() {
    int bestType = ENTITY_NONE;
    if (activeDeviceSlot == 2 && !scannerOverheated && heist.stage == HEIST_RUN) {
        Vec3 fwd(mSin(cam.yaw) * mCos(cam.pitch), mSin(cam.pitch), mCos(cam.yaw) * mCos(cam.pitch));
        float bestDist = HEIST_STUDY_RANGE;
        for (const auto& e : entityMgr.entities) {
            if (!e.active || (heist.studiedMask & (1 << (int)e.type))) continue;
            Vec3 to = e.pos + Vec3(0.0f, 1.0f, 0.0f) - cam.pos;
            float dist = to.len();
            if (dist >= bestDist || dist < 0.5f || fwd.dot(to / dist) < 0.90f) continue;
            if (!hasLabelLineOfSight(e.pos)) continue;
            bestDist = dist;
            bestType = (int)e.type;
        }
    }
    if (bestType == ENTITY_NONE || bestType != gHeistStudyType) {
        gHeistStudy = 0.0f;
        gHeistStudyType = bestType;
        return;
    }
    gHeistStudy += dTime / HEIST_STUDY_SECONDS;
    if (gHeistStudy < 1.0f) return;
    gHeistStudy = 0.0f;
    if (heistIsAuthority()) heistHostStudy(bestType);
    else netMgr.sendInteractRequest(REQ_HEIST_STUDY, bestType);
}

// Nearest thing worth walking to: the scanner beeps towards it.
inline bool heistScannerTarget(Vec3& out) {
    float best = 1e9f;
    for (const auto& it : worldItems) {
        if (!it.active || (!isLootItemType(it.type) && it.type != ITEM_STASH)) continue;
        Vec3 d = it.pos - cam.pos;
        float dist = d.x * d.x + d.z * d.z;
        if (dist >= best) continue;
        best = dist;
        out = it.pos;
    }
    return best < 1e8f;
}

inline float heistMoveScale() {
    return carrySpeedScale(carryWeight(heistMyCarry()));
}

inline void heistOnAccident() {
    gHeistFlash = 1.0f;
    camShake = 1.0f;
    damageFlash = 0.5f;
    triggerScare(1.0f);
    heistSaveTutorialSeen();
    bool nearBlast = nearPoint2D(cam.pos, heist.portalPos, CS * 1.2f);
    if (!heistPosInGarage(cam.pos) && !nearBlast) return;
    // Thrown back into the yellow, flat on the carpet.
    Vec3 land = heistCellCenter(heist.baseX, heist.baseZ - 1);
    cam.pos = Vec3(land.x + ((float)heistMe() - 1.5f) * 0.8f, PH, land.z);
    cam.yaw = 3.1415927f;
    cam.pitch = 0.25f;
    playerHealth -= 12.0f;
    if (playerHealth < 20.0f) playerHealth = 20.0f;
}

// Runs on every peer: turns changes of the shared run into things you can see and hear.
inline void heistWatchRun() {
    static int lastStage = -1, lastCash = 0, lastQuota = 0, lastDelivered = 0, lastStudied = 0, lastLevel = -1;
    static unsigned int lastSeed = 0;
    bool freshLevel = lastLevel != gCurrentLevel || lastSeed != worldSeed;
    if (freshLevel) {
        lastLevel = gCurrentLevel;
        lastSeed = worldSeed;
        lastStage = heist.stage;
        lastCash = heist.cash;
        lastQuota = heist.quotaHave;
        lastDelivered = heist.tutDelivered;
        lastStudied = heist.studiedMask;
        return;
    }
    // Nobody stays on the home side once the portal is gone (late joiners, skipped tutorials).
    if (heist.hasGarage && !heistStageIsSafe(heist.stage) && heist.stage != HEIST_ACCIDENT && heistPosInGarage(cam.pos)) {
        Vec3 at = squadSpawnSlot(heistCellCenter(heist.baseX, heist.baseZ), heistMe());
        cam.pos = Vec3(at.x, PH, at.z);
    }
    if (heist.stage != lastStage) {
        if (!heistIsAuthority()) heist.stageTimer = 0.0f;
        if (heist.stage == HEIST_ACCIDENT) heistOnAccident();
        if (heist.stage == HEIST_RUN) lastQuota = heist.quotaHave;
        lastStage = heist.stage;
    }
    if (heist.cash != lastCash) {
        heistToast(HUD_ICON_COIN, heist.cash - lastCash, heist.cash > lastCash);
        if (heist.cash > lastCash) triggerToySound(TOY_RESPAWN, 0.45f, 1.5f);
        lastCash = heist.cash;
    }
    if (heist.quotaHave > lastQuota) {
        heistToast(HUD_ICON_DRILL, heist.quotaHave - lastQuota, true);
        triggerToySound(TOY_SHOVE, 0.5f, 0.8f);
    }
    lastQuota = heist.quotaHave;
    if (heist.tutDelivered > lastDelivered) {
        heistToast(HUD_ICON_CHECK, 0, true);
        triggerToySound(TOY_RESPAWN, 0.45f, 1.2f);
    }
    lastDelivered = heist.tutDelivered;
    if (heist.studiedMask != lastStudied) heistToast(HUD_ICON_SCANNER, 0, true);
    lastStudied = heist.studiedMask;
}

inline void heistUpdateFx() {
    for (int i = 0; i < HEIST_TOAST_MAX; i++) {
        if (gHeistToasts[i].ttl > 0.0f) gHeistToasts[i].ttl -= dTime;
    }
    gHeistFlash -= dTime * 0.45f;
    if (gHeistFlash < 0.0f) gHeistFlash = 0.0f;
    if (heist.stage != HEIST_DRILLING) return;
    // The drill bites: everything shakes, then the floor is gone.
    float t = heist.stageTimer / HEIST_DRILL_SECONDS;
    if (!heistIsAuthority()) heist.stageTimer += dTime;
    if (camShake < 0.22f + t * 0.5f) camShake = 0.22f + t * 0.5f;
    if (t > 0.6f) gHeistFlash = (t - 0.6f) / 0.4f;
    static float rumble = 0.0f;
    rumble -= dTime;
    if (rumble <= 0.0f) {
        rumble = 0.22f;
        triggerToySound(TOY_SHOVE, 0.55f, 0.35f + t * 0.3f);
    }
}

// Per-frame entry point, called from the main loop while a run is being played.
inline void heistUpdate() {
    heistHostUpdate(dTime);
    rigUpdate();
    heistWatchRun();
    heistUpdateFx();
    refreshHeistModels();
    gHeistShopOpen = heist.hasKiosk && heist.stage == HEIST_RUN && !gCoopDead && !playerDowned &&
                     nearPoint2D(cam.pos, heist.kioskPos, HEIST_KIOSK_REACH);
    gHeistPrompt = (gCoopDead || playerDowned) ? HeistPrompt() : heistFindPrompt();
    heistUpdateStudy();
}
