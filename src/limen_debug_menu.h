#pragma once
// LIMEN debug menu (F10, needs DEBUG MODE in the game's settings). Replaces the game's modal
// thirteen-row list without touching the game modules: game.cpp routes the game's
// `glfwGetKey` and `glfwSwapBuffers` calls through the two shims at the bottom of limen_debug_draw.h.
//
//   * not modal: the panel sits on the left, the mouse still looks and WASD still moves, so a
//     value can be tuned while flying;
//   * five sections (TAB): PLAYER, WORLD, SPAWN, VIEW, STORY;
//   * rows are switches, steppers (LEFT / RIGHT) or actions (ENTER);
//   * WORLD knows LIMEN: load any level, reroll the seed, jump to the nearest megastructure of
//     a chosen form or to the nearest anomaly site of a chosen type;
//   * VIEW > WORLD INFO: seed, level, cell tag and flags, slab, megaform, the sixteen genes;
//   * STORY is the narrator (limen_narrator.h): switch it off, change its pace, ask it for one
//     thing in particular, jump to what it has waiting, and read its journal (what this player
//     has and has not witnessed so far, what waits on this level, what it did last).
//
// Included after game.cpp: uses the game's globals and its legacy `executeDebugAction`.

struct LmDbgState {
    bool open = false;
    int tab = 0;
    int sel[5] = {0, 0, 0, 0, 0};
    int flySpeed = 0;           // index into LM_DBG_FLY_MUL
    bool god = false;
    bool infBattery = false;
    bool info = false;
    int levelPick = 0;
    int megaPick = -1;          // -1 = any form
    int sitePick = 0;
    int quirkPick = -1;         // -1 = whatever the level deals itself
    int pacePick = 1;           // index into LM_DBG_PACE
    bool journal = false;
    bool swallowUntilRelease = false;
};
inline LmDbgState gLmDbg;

const float LM_DBG_FLY_MUL[] = {1.0f, 2.0f, 4.0f, 8.0f, 16.0f};
const int LM_DBG_FLY_MUL_COUNT = 5;

const float LM_DBG_PACE[] = {0.5f, 1.0f, 2.0f, 4.0f};
const int LM_DBG_PACE_COUNT = 4;

enum { LM_DBG_TAB_PLAYER, LM_DBG_TAB_WORLD, LM_DBG_TAB_SPAWN, LM_DBG_TAB_VIEW, LM_DBG_TAB_STORY, LM_DBG_TAB_COUNT };

enum LmDbgItem {
    DI_FLY, DI_FLY_SPEED, DI_STAMINA, DI_GOD, DI_BATTERY, DI_HEAL,
    DI_LEVEL, DI_REROLL, DI_QUIRK, DI_TP_MEGA, DI_TP_SITE, DI_TP_ORIGIN, DI_TP_OBJECTIVE, DI_TP_KIOSK, DI_TP_DRILL, DI_SKIP_STAGE,
    DI_SPAWN_STALKER, DI_SPAWN_CRAWLER, DI_SPAWN_SHADOW, DI_EYE, DI_HOLES, DI_SUPPLY, DI_MED, DI_CLEAR_ENTITIES,
    DI_INFO, DI_PERF, DI_TELEMETRY, DI_MINIMAP,
    DI_NAR_ON, DI_NAR_REWRITE, DI_NAR_PACE, DI_NAR_WISH, DI_NAR_TP, DI_NAR_JOURNAL, DI_NAR_FORGET
};

struct LmDbgTabDef { const char* name; int first, count; };
const LmDbgTabDef LM_DBG_TABS[LM_DBG_TAB_COUNT] = {
    {"PLAYER", DI_FLY, 6},
    {"WORLD", DI_LEVEL, 10},
    {"SPAWN", DI_SPAWN_STALKER, 8},
    {"VIEW", DI_INFO, 4},
    {"STORY", DI_NAR_ON, 7},
};

enum { LM_DBG_ACTION, LM_DBG_SWITCH, LM_DBG_STEPPER };

inline const char* lmDbgSiteName(int t) {
    static const char* names[LS_SITE_COUNT] = {
        "DOOR GAP", "DOOR SHIFT", "DEAD LAMP", "DARK SLAB", "RAIL GAP", "DYN LAMP",
        "XENOGRAFT", "WALL DOOR", "PLUG CORRIDOR", "CEILING STAIR", "VITRINE WINDOW", "PHASE SHIFT"
    };
    return (t >= 0 && t < LS_SITE_COUNT) ? names[t] : "?";
}

inline const char* lmDbgTagName(int t) {
    static const char* names[LT_TAG_COUNT] = {"WALL", "CORR", "ROOM", "HALL", "VOID", "BRIDGE", "BASIN", "LOW", "VITRINE"};
    return (t >= 0 && t < LT_TAG_COUNT) ? names[t] : "?";
}

inline void lmDbgUpper(char* s) {
    for (; *s; s++) if (*s >= 'a' && *s <= 'z') *s = (char)(*s - 32);
}

inline bool lmDbgSolo() { return multiState != MULTI_IN_GAME; }

// ---------------------------------------------------------------- rows

inline const char* lmDbgLabel(int item) {
    switch (item) {
        case DI_FLY: return "FLY";
        case DI_FLY_SPEED: return "FLY SPEED";
        case DI_STAMINA: return "INFINITE STAMINA";
        case DI_GOD: return "GOD MODE";
        case DI_BATTERY: return "INFINITE BATTERY";
        case DI_HEAL: return "HEAL";
        case DI_LEVEL: return "LOAD LEVEL";
        case DI_REROLL: return "REROLL SEED";
        case DI_QUIRK: return "FORCE QUIRK";
        case DI_TP_MEGA: return "TP MEGA";
        case DI_TP_SITE: return "TP SITE";
        case DI_TP_ORIGIN: return "TP ORIGIN";
        case DI_TP_OBJECTIVE: return "TP NEAREST LOOT";
        case DI_TP_KIOSK: return "TP KIOSK";
        case DI_TP_DRILL: return "TP DRILL";
        case DI_SKIP_STAGE: return "SKIP STAGE";
        case DI_SPAWN_STALKER: return "STALKER";
        case DI_SPAWN_CRAWLER: return "CRAWLER";
        case DI_SPAWN_SHADOW: return "SHADOW";
        case DI_EYE: return "EYE EVENT";
        case DI_HOLES: return "FLOOR HOLES";
        case DI_SUPPLY: return "SUPPLY CACHE";
        case DI_MED: return "MED SPRAY";
        case DI_CLEAR_ENTITIES: return "REMOVE ALL ENTITIES";
        case DI_INFO: return "WORLD INFO";
        case DI_PERF: return "PERF GRAPH";
        case DI_TELEMETRY: return "HUD TELEMETRY";
        case DI_MINIMAP: return "MINIMAP";
        case DI_NAR_ON: return "NARRATOR";
        case DI_NAR_REWRITE: return "REWRITE THE UNSEEN";
        case DI_NAR_PACE: return "PACE";
        case DI_NAR_WISH: return "SHOW ME";
        case DI_NAR_TP: return "TP TO WHAT WAITS";
        case DI_NAR_JOURNAL: return "JOURNAL";
        case DI_NAR_FORGET: return "FORGET EVERYTHING";
    }
    return "";
}

inline int lmDbgKind(int item) {
    switch (item) {
        case DI_FLY: case DI_STAMINA: case DI_GOD: case DI_BATTERY:
        case DI_INFO: case DI_PERF: case DI_TELEMETRY: case DI_MINIMAP:
        case DI_NAR_ON: case DI_NAR_REWRITE: case DI_NAR_JOURNAL:
            return LM_DBG_SWITCH;
        case DI_FLY_SPEED: case DI_LEVEL: case DI_QUIRK: case DI_TP_MEGA: case DI_TP_SITE:
        case DI_NAR_PACE: case DI_NAR_WISH:
            return LM_DBG_STEPPER;
    }
    return LM_DBG_ACTION;
}

inline bool* lmDbgSwitchPtr(int item) {
    switch (item) {
        case DI_FLY: return &debugTools.flyMode;
        case DI_STAMINA: return &debugTools.infiniteStamina;
        case DI_GOD: return &gLmDbg.god;
        case DI_BATTERY: return &gLmDbg.infBattery;
        case DI_INFO: return &gLmDbg.info;
        case DI_PERF: return &gPerfDebugOverlay;
        case DI_TELEMETRY: return &gHudTelemetryVisible;
        case DI_MINIMAP: return &minimapEnabled;
        case DI_NAR_ON: return &gNar.enabled;
        case DI_NAR_REWRITE: return &gNar.rewrite;
        case DI_NAR_JOURNAL: return &gLmDbg.journal;
    }
    return nullptr;
}

inline void lmDbgStepperValue(int item, char* out, int cap) {
    out[0] = '\0';
    if (item == DI_FLY_SPEED) snprintf(out, cap, "X%d", (int)LM_DBG_FLY_MUL[gLmDbg.flySpeed]);
    else if (item == DI_LEVEL) snprintf(out, cap, gLmDbg.levelPick == gCurrentLevel ? "%d (NOW)" : "%d", gLmDbg.levelPick);
    else if (item == DI_TP_MEGA) {
        snprintf(out, cap, "%s", gLmDbg.megaPick < 0 ? "any" : lmMegaFormName(gLmDbg.megaPick));
        lmDbgUpper(out);
    } else if (item == DI_TP_SITE) snprintf(out, cap, "%s", lmDbgSiteName(gLmDbg.sitePick));
    else if (item == DI_QUIRK) {
        snprintf(out, cap, "%s%s", gLmDbg.quirkPick < 0 ? "auto" : (gLmDbg.quirkPick == 0 ? "none" : lmQuirkName(gLmDbg.quirkPick)),
                 gLmDbg.quirkPick == gLmQuirkForce ? " (NOW)" : "");
        lmDbgUpper(out);
    } else if (item == DI_NAR_PACE) snprintf(out, cap, "X%.1f", LM_DBG_PACE[gLmDbg.pacePick]);
    else if (item == DI_NAR_WISH) {
        snprintf(out, cap, "%s", gNar.wish < 0 ? "its own choice" : lmFeatInfo(gNar.wish).name);
        lmDbgUpper(out);
    }
}

// Returns false at the end of the range.
inline bool lmDbgStep(int item, int dir, bool fast) {
    if (item == DI_FLY_SPEED) {
        int v = gLmDbg.flySpeed + dir;
        if (v < 0 || v >= LM_DBG_FLY_MUL_COUNT) return false;
        gLmDbg.flySpeed = v;
    } else if (item == DI_LEVEL) {
        int v = gLmDbg.levelPick + dir * (fast ? 10 : 1);
        if (v < -999) v = -999;
        if (v > 999) v = 999;
        if (v == gLmDbg.levelPick) return false;
        gLmDbg.levelPick = v;
    } else if (item == DI_TP_MEGA) {
        int n = LM_MEGA_FORM_COUNT + 1;   // "any" sits before the first form
        gLmDbg.megaPick = (gLmDbg.megaPick + 1 + dir + n) % n - 1;
    } else if (item == DI_QUIRK) {
        int n = LM_QUIRK_COUNT + 1;       // "auto" sits before "none"
        gLmDbg.quirkPick = (gLmDbg.quirkPick + 1 + dir + n) % n - 1;
    } else if (item == DI_TP_SITE) {
        gLmDbg.sitePick = (gLmDbg.sitePick + dir + LS_SITE_COUNT) % LS_SITE_COUNT;
    } else if (item == DI_NAR_PACE) {
        int v = gLmDbg.pacePick + dir;
        if (v < 0 || v >= LM_DBG_PACE_COUNT) return false;
        gLmDbg.pacePick = v;
        gNar.pace = LM_DBG_PACE[v];
    } else if (item == DI_NAR_WISH) {
        int n = LN_QUIRK_FIRST + 1;       // "its own choice" sits before the first feature; quirks belong to levels
        gNar.wish = (gNar.wish + 1 + dir + n) % n - 1;
    } else return false;
    return true;
}

// ---------------------------------------------------------------- actions

inline void lmDbgTeleport(float x, float z, const char* msg) {
    debugTeleportTo(Vec3(x, 0.0f, z), msg);
    cam.pos.y = PH + lmGroundHeightAt(cam.pos.x, cam.pos.z, CS);
    cam.curH = PH;
}

// The walkable cell of slab (j, i) closest to its middle; floor at y = 0 is preferred to a basin.
inline bool lmDbgSlabStand(int j, int i, float& outX, float& outZ) {
    LimenRect r = lmSlabRect(j, i);
    int cx = (r.x0 + r.x1) / 2, cz = (r.z0 + r.z1) / 2;
    long long best = -1;
    for (int wz = r.z0 + 1; wz < r.z1; wz++) {
        for (int wx = r.x0 + 1; wx < r.x1; wx++) {
            if (!lmPlayerWalkableAt(wx, wz)) continue;
            long long d = (long long)(wx - cx) * (wx - cx) + (long long)(wz - cz) * (wz - cz);
            if (!lmWalkable(wx, wz)) d += 1000000;
            if (best < 0 || d < best) { best = d; outX = (wx + 0.5f) * CS; outZ = (wz + 0.5f) * CS; }
        }
    }
    return best >= 0;
}

// Megaforms follow from the genome alone, so a wide search builds no slabs.
inline void lmDbgTeleportMega() {
    const int R = 10;
    int wx = (int)floorf(cam.pos.x / CS), wz = (int)floorf(cam.pos.z / CS);
    LimenRect here = lmSlabAt(wx, wz);
    int bj = 0, bi = 0, bestForm = -1;
    long long best = -1;
    for (int dj = -R; dj <= R; dj++) {
        for (int di = -R; di <= R; di++) {
            if (dj == 0 && di == 0) continue;
            int j = here.j + dj, i = here.i + di;
            if (lmSlabTiers(j, i) <= 1) continue;
            int form = lmSlabMegaForm(j, i);
            if (gLmDbg.megaPick >= 0 && form != gLmDbg.megaPick) continue;
            LimenRect r = lmSlabRect(j, i);
            long long ddx = (r.x0 + r.x1) / 2 - wx, ddz = (r.z0 + r.z1) / 2 - wz;
            long long d = ddx * ddx + ddz * ddz;
            if (best < 0 || d < best) { best = d; bj = j; bi = i; bestForm = form; }
        }
    }
    float x = 0.0f, z = 0.0f;
    if (best < 0 || !lmDbgSlabStand(bj, bi, x, z)) { setTrapStatus("DEBUG: NO SUCH MEGASTRUCTURE NEARBY"); return; }
    char msg[80], name[24];
    snprintf(name, sizeof(name), "%s", lmMegaFormName(bestForm));
    lmDbgUpper(name);
    snprintf(msg, sizeof(msg), "DEBUG: %s, SLAB %d %d, %d STOREYS", name, bj, bi, lmSlabTiers(bj, bi));
    lmDbgTeleport(x, z, msg);
}

inline void lmDbgTeleportSite() {
    const int R = 2;   // sites exist only in built slabs: stay near what is already cached
    int wx = (int)floorf(cam.pos.x / CS), wz = (int)floorf(cam.pos.z / CS);
    LimenRect here = lmSlabAt(wx, wz);
    long long best = -1;
    int bx = 0, bz = 0;
    for (int dj = -R; dj <= R; dj++) {
        for (int di = -R; di <= R; di++) {
            const LimenSlab& s = lmSlab(here.j + dj, here.i + di);
            for (int k = 0; k < s.siteCount; k++) {
                if (s.sites[k].type != gLmDbg.sitePick) continue;
                int sx = s.rect.x0 + 1 + s.sites[k].lx, sz = s.rect.z0 + 1 + s.sites[k].lz;
                long long d = (long long)(sx - wx) * (sx - wx) + (long long)(sz - wz) * (sz - wz);
                if (d < 9) continue;   // already standing at it: go to the next one
                if (best < 0 || d < best) { best = d; bx = sx; bz = sz; }
            }
        }
    }
    if (best < 0) { setTrapStatus("DEBUG: NO SUCH SITE NEARBY"); return; }
    char msg[80];
    snprintf(msg, sizeof(msg), "DEBUG: %s AT CELL %d %d", lmDbgSiteName(gLmDbg.sitePick), bx, bz);
    lmDbgTeleport((bx + 0.5f) * CS, (bz + 0.5f) * CS, msg);
}

// Nearest thing the narrator has placed on this level that the player has not found yet.
inline void lmDbgTeleportPending() {
    const LmNarPlaced* best = nullptr;
    float bestD = 1e30f, bx = 0.0f, bz = 0.0f;
    for (const LmNarPlaced& p : gNar.placed) {
        if (p.witnessed) continue;
        if (gNar.wish >= 0 && p.feat != gNar.wish) continue;
        int j = (int)(p.slab >> 32), i = (int)(p.slab & 0xFFFFFFFFll);
        float x = 0.0f, z = 0.0f;
        if (!lmDbgSlabStand(j, i, x, z)) continue;
        float d = (x - cam.pos.x) * (x - cam.pos.x) + (z - cam.pos.z) * (z - cam.pos.z);
        if (d < bestD) { bestD = d; best = &p; bx = x; bz = z; }
    }
    if (!best) { setTrapStatus("DEBUG: NOTHING IS WAITING"); return; }
    char msg[96], name[32];
    snprintf(name, sizeof(name), "%s", lmFeatInfo(best->feat).name);
    lmDbgUpper(name);
    snprintf(msg, sizeof(msg), "DEBUG: %s, %d M AWAY", name, (int)sqrtf(bestD));
    lmDbgTeleport(bx, bz, msg);
}

inline void lmDbgLoadLevel(int level) {
    if (!lmDbgSolo()) { setTrapStatus("DEBUG: SOLO ONLY"); return; }
    gCurrentLevel = level;
    reloadLevelAndResume(nullptr);
    updateWindowTitleForLevel();
    char msg[64];
    snprintf(msg, sizeof(msg), "DEBUG: LEVEL %d, SEED %u", gCurrentLevel, worldSeed);
    setTrapStatus(msg);
}

inline void lmDbgActivate(int item) {
    switch (item) {
        case DI_HEAL:
            playerHealth = 100.0f; playerSanity = 100.0f; playerStamina = 125.0f; flashlightBattery = 100.0f;
            playerDowned = false; playerDownedTimer = 0.0f;
            setEchoStatus("DEBUG: HEALED");
            break;
        case DI_LEVEL: lmDbgLoadLevel(gLmDbg.levelPick); break;
        case DI_REROLL: lmDbgLoadLevel(gCurrentLevel); break;
        case DI_QUIRK:
            if (!lmDbgSolo()) { setTrapStatus("DEBUG: SOLO ONLY"); break; }
            gLmQuirkForce = gLmDbg.quirkPick;
            gLimen.dnaReady = false;      // the DNA and the level textures are rebuilt with the forced quirk
            lmTexBuiltValid = false;
            lmDbgLoadLevel(gCurrentLevel);
            break;
        case DI_NAR_WISH:
            // ask for it right now: ahead of the player if there is room, otherwise on the next floor
            gNar.urge = true; gNar.rest = 0.0f; gNar.nextPlan = 0.0f;
            setTrapStatus(gNar.wish < 0 ? "DEBUG: NARRATOR, SOMETHING NOW" : "DEBUG: NARRATOR, THAT ONE NEXT");
            break;
        case DI_NAR_PACE: if (!lmDbgStep(item, 1, false)) { gLmDbg.pacePick = 0; gNar.pace = LM_DBG_PACE[0]; } break;
        case DI_NAR_TP: lmDbgTeleportPending(); break;
        case DI_NAR_FORGET: lmNarForget(); setTrapStatus("DEBUG: THE NARRATOR REMEMBERS NOTHING"); break;
        case DI_TP_MEGA: lmDbgTeleportMega(); break;
        case DI_TP_SITE: lmDbgTeleportSite(); break;
        case DI_TP_ORIGIN: lmDbgTeleport(8.0f * CS, 8.0f * CS, "DEBUG: TELEPORTED TO ORIGIN"); break;
        case DI_TP_OBJECTIVE: executeDebugAction(DEBUG_ACT_TP_NOTE); break;
        case DI_TP_KIOSK: executeDebugAction(DEBUG_ACT_TP_ECHO); break;
        case DI_TP_DRILL: executeDebugAction(DEBUG_ACT_TP_EXIT); break;
        case DI_SKIP_STAGE: executeDebugAction(DEBUG_ACT_SKIP_LEVEL); break;
        case DI_SPAWN_STALKER: executeDebugAction(DEBUG_ACT_SPAWN_STALKER); break;
        case DI_SPAWN_CRAWLER: executeDebugAction(DEBUG_ACT_SPAWN_CRAWLER); break;
        case DI_SPAWN_SHADOW: executeDebugAction(DEBUG_ACT_SPAWN_SHADOW); break;
        case DI_EYE: executeDebugAction(DEBUG_ACT_TRIGGER_EYE); break;
        case DI_HOLES: executeDebugAction(DEBUG_ACT_FORCE_HOLES); break;
        case DI_SUPPLY: executeDebugAction(DEBUG_ACT_FORCE_SUPPLY); break;
        case DI_MED: executeDebugAction(DEBUG_ACT_SPAWN_MED_SPRAY); break;
        case DI_CLEAR_ENTITIES:
            if (!lmDbgSolo() && !netMgr.isHost) { setTrapStatus("DEBUG ACTION: HOST ONLY"); break; }
            entityMgr.entities.clear();
            setEchoStatus("DEBUG: ENTITIES REMOVED");
            break;
        default: break;
    }
}

// ---------------------------------------------------------------- input

inline bool lmDbgKeyDown(int key) { return glfwGetKey(gWin, key) == GLFW_PRESS; }

// Keys the menu takes for itself while it is open; everything else still reaches the game.
inline bool lmDbgOwnsKey(int key) {
    return key == GLFW_KEY_UP || key == GLFW_KEY_DOWN || key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT ||
           key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_TAB || key == GLFW_KEY_ESCAPE ||
           key == GLFW_KEY_PAGE_UP || key == GLFW_KEY_PAGE_DOWN;
}
