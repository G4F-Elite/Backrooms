#pragma once
// LIMEN narrator ("рассказчик"): the part of the world that wants to be seen.
//
// The generator can do a great many things, and left alone it does each of them somewhere:
// the chance that one walk passes all of them is nil. The narrator closes that gap. It keeps
//   * a catalogue of everything the generator can show (megastructures, delirium structures,
//     dead thresholds, furniture rules, kinds of ordinary space, level quirks),
//   * a memory of what this player has actually witnessed, kept between levels and sessions,
//   * a picture of what the player has laid eyes on so far (`lmNarLook`: rays through the cell
//     grid, cut short by walls and fog), slab by slab and cell by cell,
//   * a guess at where the player is going: the rig's lane if there is one, else the heading,
// and writes beats (limen_beat.h) into slabs on that way: what has not been witnessed for the
// longest comes first, a megastructure is turned and moved so the road runs through its
// natural opening, quiet anomalies are put beside the road. After something big it lets the
// player breathe, the longer the newer the thing was: a repeat of what the level is full of
// is no event at all.
//
// Whatever nobody has seen is not yet fixed. A slab the player never laid eyes on (and that
// holds nothing of the game's) may be rewritten at any time; a slab that was seen is never
// touched again, and neither is a neighbour's wall towards it.
//
// Two modes. Adaptive (single player): memory, eyes and pacing as above. Script (co-op): the
// plan is a pure function of (seed, level, the rig's lane), laid down once when the level
// starts, so every peer builds the same world; "what was not seen for longest" is replaced by
// a rotation over the level number.
//
// No game code here: limen_tool drives the same narrator through simulated walks.
#include "limen_world.h"
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------- catalogue
enum LmFeat {
    LN_MEGA_CLASSIC = 0, LN_ZIG_STAIR, LN_ZIG_PYRAMID, LN_ZIG_AMPH, LN_CANYON, LN_HYPOSTYLE, LN_SILO, LN_HANGING,
    LN_CELLBLOCK, LN_PLATFORM, LN_MONOLITH, LN_SPIRES, LN_GROTTO, LN_ROOTFALL,
    LN_DRUNK, LN_NESTED, LN_SPIRAL, LN_ESCHER, LN_XENO, LN_DARK,
    LN_VITRINE, LN_WALL_DOOR, LN_CEIL_STAIR, LN_PLUG, LN_ECHO, LN_PHASE,
    LN_MONO, LN_LACK, LN_FACEWALL, LN_TWIN, LN_WAITVOID, LN_PROCESSION,
    LN_WING_PARTITIONS, LN_WING_CORRIDORS, LN_WING_ENFILADE, LN_WING_CONCOURSE, LN_WING_SERVICE,
    LN_BASIN_WALK, LN_CHASM, LN_SKYWELL,
    LN_QUIRK_FIRST,                                  // one per LmQuirk, LQ_FLOOD first
    LN_COUNT = LN_QUIRK_FIRST + LM_QUIRK_COUNT - 1
};
static_assert(LN_COUNT <= 64, "the narrator keeps feature sets in 64-bit masks");

enum { LNC_SPACE = 0, LNC_QUIET, LNC_STRANGE, LNC_MEGA, LNC_QUIRK };
enum { LNW_ANY = 0, LNW_ROOMS = (1 << LM_WING_CORRIDORS) | (1 << LM_WING_ENFILADE) | (1 << LM_WING_SERVICE),
       LNW_CLONES = (1 << LM_WING_CORRIDORS), LNW_HALLS = (1 << LM_WING_CONCOURSE) | (1 << LM_WING_ENFILADE),
       LNW_RUNS = (1 << LM_WING_CORRIDORS) | (1 << LM_WING_SERVICE) };

struct LmFeatInfo {
    const char* name;
    unsigned char cls;
    unsigned force;            // LmBeatForce bit that guarantees it (0: placed by other means)
    signed char megaForm, megaMode;
    unsigned char wings;       // LNW_*: wings of a one-storey slab it can happen in (0 = any)
    float seeDist;             // metres within which seeing its spot counts as witnessing it
};
inline const LmFeatInfo& lmFeatInfo(int f) {
    static const LmFeatInfo T[LN_QUIRK_FIRST] = {
        {"classic vault",   LNC_MEGA, 0, MF_CLASSIC,   -1, 0, 0},
        {"great stair",     LNC_MEGA, 0, MF_ZIGGURAT,   0, 0, 0},
        {"ziggurat",        LNC_MEGA, 0, MF_ZIGGURAT,   1, 0, 0},
        {"amphitheatre",    LNC_MEGA, 0, MF_ZIGGURAT,   2, 0, 0},
        {"canyon",          LNC_MEGA, 0, MF_CANYON,    -1, 0, 0},
        {"hypostyle",       LNC_MEGA, 0, MF_HYPOSTYLE, -1, 0, 0},
        {"silo",            LNC_MEGA, 0, MF_SILO,      -1, 0, 0},
        {"hanging pyramid", LNC_MEGA, 0, MF_HANGING,   -1, 0, 0},
        {"cellblock",       LNC_MEGA, 0, MF_CELLBLOCK, -1, 0, 0},
        {"platform",        LNC_MEGA, 0, MF_PLATFORM,  -1, 0, 0},
        {"monolith",        LNC_MEGA, 0, MF_MONOLITH,  -1, 0, 0},
        {"spires",          LNC_MEGA, 0, MF_SPIRES,    -1, 0, 0},
        {"grotto",          LNC_MEGA, 0, MF_GROTTO,    -1, 0, 0},
        {"rootfall",        LNC_MEGA, 0, MF_TENDRILS,  -1, 0, 0},
        {"drunk pocket",    LNC_STRANGE, 0,          -1, -1, 0, 30.0f},
        {"nested rooms",    LNC_STRANGE, LBF_NESTED, -1, -1, 0, 38.0f},
        {"spiral corridor", LNC_STRANGE, LBF_SPIRAL, -1, -1, 0, 38.0f},
        {"escher knot",     LNC_STRANGE, LBF_ESCHER, -1, -1, 0, 70.0f},
        {"xenograft",       LNC_STRANGE, LBF_XENO,   -1, -1, 0, 34.0f},
        {"dark slab",       LNC_STRANGE, LBF_DARK,   -1, -1, 0, 45.0f},
        {"vitrine",         LNC_QUIET, LBF_VITRINE,    -1, -1, 0, 26.0f},
        {"wall door",       LNC_QUIET, LBF_WALL_DOOR,  -1, -1, 0, 24.0f},
        {"ceiling stair",   LNC_QUIET, LBF_CEIL_STAIR, -1, -1, LNW_HALLS, 30.0f},
        {"plugged corridor",LNC_QUIET, LBF_PLUG,       -1, -1, 0, 26.0f},
        {"echo corridor",   LNC_QUIET, LBF_ECHO,       -1, -1, LNW_RUNS, 26.0f},
        {"phase shift",     LNC_QUIET, LBF_PHASE,      -1, -1, 0, 22.0f},
        {"mono flood",      LNC_QUIET, LBF_MONO,       -1, -1, LNW_ROOMS, 22.0f},
        {"missing host",    LNC_QUIET, LBF_LACK,       -1, -1, LNW_ROOMS, 18.0f},
        {"facing the wall", LNC_QUIET, LBF_FACEWALL,   -1, -1, LNW_ROOMS, 18.0f},
        {"uncanny twin",    LNC_QUIET, LBF_TWIN,       -1, -1, LNW_CLONES, 18.0f},
        {"waiting void",    LNC_QUIET, LBF_WAITVOID,   -1, -1, LNW_HALLS, 26.0f},
        {"procession",      LNC_QUIET, LBF_PROCESSION, -1, -1, LNW_HALLS, 26.0f},
        {"partition maze",  LNC_SPACE, 0, -1, -1, 0, 0},
        {"corridor wing",   LNC_SPACE, 0, -1, -1, 0, 0},
        {"enfilade",        LNC_SPACE, 0, -1, -1, 0, 0},
        {"concourse",       LNC_SPACE, 0, -1, -1, 0, 0},
        {"service tunnels", LNC_SPACE, 0, -1, -1, 0, 0},
        {"walkable basin",  LNC_SPACE, 0, -1, -1, 0, 30.0f},
        {"chasm bridge",    LNC_SPACE, 0, -1, -1, 0, 40.0f},
        {"skylight well",   LNC_SPACE, 0, -1, -1, 0, 30.0f},
    };
    static LmFeatInfo quirk[LM_QUIRK_COUNT];
    if (f >= LN_QUIRK_FIRST && f < LN_COUNT) {
        LmFeatInfo& q = quirk[f - LN_QUIRK_FIRST];
        q = {lmQuirkName(f - LN_QUIRK_FIRST + 1), LNC_QUIRK, 0, -1, -1, 0, 0};
        return q;
    }
    return T[(f >= 0 && f < LN_QUIRK_FIRST) ? f : 0];
}
inline unsigned long long lmFeatBit(int f) { return 1ull << f; }
inline int lmFeatOfMega(int form, int megaSub) {
    switch (form) {
        case MF_CLASSIC: return LN_MEGA_CLASSIC;
        case MF_ZIGGURAT: { int m = megaSub & 3; return m == 0 ? LN_ZIG_STAIR : (m == 1 ? LN_ZIG_PYRAMID : LN_ZIG_AMPH); }
        case MF_CANYON: return LN_CANYON;   case MF_HYPOSTYLE: return LN_HYPOSTYLE;
        case MF_SILO: return LN_SILO;       case MF_HANGING: return LN_HANGING;
        case MF_CELLBLOCK: return LN_CELLBLOCK; case MF_PLATFORM: return LN_PLATFORM;
        case MF_MONOLITH: return LN_MONOLITH;   case MF_SPIRES: return LN_SPIRES;
        case MF_GROTTO: return LN_GROTTO;       case MF_TENDRILS: return LN_ROOTFALL;
    }
    return -1;
}

// ---------------------------------------------------------------- state
struct LmNarMemory {
    unsigned count[LN_COUNT] = {};     // times witnessed, ever
    unsigned last[LN_COUNT] = {};      // value of `stamp` when last witnessed
    unsigned stamp = 0;                // grows by one with every witnessed thing
    unsigned levels = 0;               // levels started
    bool dirty = false;
};

struct LmSpot { unsigned char feat; short lx, lz; };   // lx < 0: the slab as a whole

struct LmNarSlab {
    unsigned long long seen[LM_MAX] = {};   // bit lz of word lx: the player has laid eyes on that cell
    int seenCount = 0;
    bool entered = false;      // the player has stood in it
    bool glimpsed = false;     // its bulk was in view over the walls (roofless levels)
    bool locked = false;       // never to be rewritten, whatever was seen
    bool decided = false;      // the narrator has made up its mind about this slab (a big thing, or nothing fits)
    bool minored = false;      // it holds quiet things only: a big thing may still take its place while unseen
    float inside = 0.0f;       // seconds the player has spent in it
    unsigned long long witnessed = 0;   // features of this slab already counted
    std::vector<LmSpot> spots;
    bool spotsValid = false;
    std::vector<LmSpot> notes; // furniture rules that fired here (reported by the furnisher)
};

struct LmNarPlaced { long long slab; unsigned char feat; bool major, witnessed; float t; bool gone = false, roam = false; float staleT = -1.0f; };   // gone: taken back; roam: prepared off the road

struct LmNarLogLine { float t; char text[72]; };

enum { LNR_NONE = 0, LNR_MINOR, LNR_STRANGE, LNR_MEGA };

struct LmNarrator {
    // configuration
    bool enabled = true;           // false: it still watches and remembers, but leaves the world alone
    bool adaptive = true;          // false: script mode (pure function of seed, level, road)
    bool rewrite = true;           // adaptive only: may rewrite slabs nobody has seen
    float pace = 1.0f;             // > 1: things happen more often
    int wish = -1;                 // debug: the feature to place next, wherever its class is called for
    int arcPin = -1;               // debug: 0 megastructure first, 1 quiet first, 2 strangeness first (-1: by level)
    bool urge = false;             // debug: place the next big thing now, whatever the pacing says
    std::string savePath;          // memory file ("" = keep in RAM only)
    bool (*busy)(int x0, int z0, int x1, int z1) = nullptr;   // world cells: something of the game's is in there
    // level
    u32 seed = 0; int level = 0; bool ready = false;
    bool hasRoad = false; int roadX = 0, roadZ = 0, roadDX = 0, roadDZ = 0;
    // player
    bool hasPlayer = false;
    float px = 0, pz = 0, yaw = 0, vx = 0, vz = 0, speed = 0;
    float time = 0;                // seconds in this level
    float sinceMajor = 0, sinceAny = 0, rest = 0;
    float nextPlan = 0, nextSave = 0;
    // what happened
    LmNarMemory mem;
    std::unordered_map<long long, LmNarSlab> slabs;
    std::vector<LmNarPlaced> placed;
    unsigned levelPlaced[LN_COUNT] = {};
    unsigned long long levelWitnessed = 0;
    std::vector<long long> dirty;  // slabs rewritten since the caller last looked: their meshes, cells and furniture are stale
    std::vector<long long> touched;
    LmNarLogLine log[10]; int logCount = 0;
    int rewrites = 0, placedMajors = 0;
    int freshBig = 0;              // big things come upon for the first time on this level
    float lastFresh = 0, lullMax = 0;   // when the last of those was, and the longest wait for one
    unsigned beatsGen = 0;
    float lookX = 1e9f, lookZ = 1e9f, lookYaw = 0, lookT = -1;
    bool quirkCounted = false, lastRoamMega = false;
    bool revert = false;           // something big was found: spares prepared elsewhere go back
    bool roamMegaNext = true;      // the next spare ahead on foot: megastructure (true) or strangeness
    bool planEntry = false;        // a roam decision is being made: the player's cell is the doorway it anchors to
    int planPX = 0, planPZ = 0;
    long long kioskSlab[2] = {0, 0}; int kioskSlabs = 0;   // road slabs the game puts its kiosk in (6..9 cells down the lane)
};
inline LmNarrator gNar;

inline bool gNarTrace = false;      // tools: print every narrator line as it happens
inline void lmNarLog(const char* fmt, ...) {
    LmNarLogLine line; line.t = gNar.time;
    va_list ap; va_start(ap, fmt); vsnprintf(line.text, sizeof(line.text), fmt, ap); va_end(ap);
    if (gNarTrace) printf("      [%6.1f s] %s\n", (double)line.t, line.text);
    if (gNar.logCount < 10) gNar.log[gNar.logCount++] = line;
    else { for (int k = 1; k < 10; k++) gNar.log[k - 1] = gNar.log[k]; gNar.log[9] = line; }
}

// ---------------------------------------------------------------- memory file
inline void lmNarSave() {
    if (gNar.savePath.empty() || !gNar.mem.dirty) return;
    FILE* f = fopen(gNar.savePath.c_str(), "w");
    if (!f) return;
    fprintf(f, "LIMEN-NARRATOR 1\nstamp %u\nlevels %u\n", gNar.mem.stamp, gNar.mem.levels);
    for (int k = 0; k < LN_COUNT; k++)
        if (gNar.mem.count[k]) fprintf(f, "seen %u %u %s\n", gNar.mem.count[k], gNar.mem.last[k], lmFeatInfo(k).name);
    fclose(f);
    gNar.mem.dirty = false;
}
inline void lmNarLoad() {
    gNar.mem = LmNarMemory();
    if (gNar.savePath.empty()) return;
    FILE* f = fopen(gNar.savePath.c_str(), "r");
    if (!f) return;
    char line[160];
    if (fgets(line, sizeof(line), f) && strncmp(line, "LIMEN-NARRATOR", 14) == 0) {
        while (fgets(line, sizeof(line), f)) {
            unsigned a = 0, b = 0; int off = 0;
            if (sscanf(line, "stamp %u", &a) == 1) gNar.mem.stamp = a;
            else if (sscanf(line, "levels %u", &a) == 1) gNar.mem.levels = a;
            else if (sscanf(line, "seen %u %u %n", &a, &b, &off) >= 2 && off > 0) {
                char* name = line + off;
                size_t n = strlen(name);
                while (n > 0 && (name[n - 1] == '\n' || name[n - 1] == '\r')) name[--n] = '\0';
                for (int k = 0; k < LN_COUNT; k++)
                    if (strcmp(lmFeatInfo(k).name, name) == 0) { gNar.mem.count[k] = a; gNar.mem.last[k] = b; break; }
            }
        }
    }
    fclose(f);
}
inline void lmNarForget() {
    std::string keep = gNar.savePath;
    gNar.mem = LmNarMemory();
    gNar.mem.dirty = true;
    lmNarSave();
    lmNarLog("memory wiped");
}
inline int lmNarSeenCount(int cls = -1) {
    int n = 0;
    for (int k = 0; k < LN_COUNT; k++) if (gNar.mem.count[k] && (cls < 0 || lmFeatInfo(k).cls == cls)) n++;
    return n;
}
inline int lmNarClassSize(int cls) {
    int n = 0;
    for (int k = 0; k < LN_COUNT; k++) if (lmFeatInfo(k).cls == cls) n++;
    return n;
}

// ---------------------------------------------------------------- what a slab holds
inline const LimenSlab* lmSlabPeek(int j, int i) {
    auto it = gLimenSlabs.find(lmSlabKey(j, i));
    return it == gLimenSlabs.end() ? nullptr : &it->second;
}
inline LmNarSlab& lmNarRec(int j, int i) { return gNar.slabs[lmSlabKey(j, i)]; }
inline const LmNarSlab* lmNarRecPeek(int j, int i) {
    auto it = gNar.slabs.find(lmSlabKey(j, i));
    return it == gNar.slabs.end() ? nullptr : &it->second;
}

// Everything of the catalogue that exists in a built slab, with the cell to look at.
inline void lmNarBuildSpots(const LimenSlab& s, std::vector<LmSpot>& out) {
    out.clear();
    auto add = [&](int f, int lx, int lz) { if (f >= 0) out.push_back({(unsigned char)f, (short)lx, (short)lz}); };
    if (s.fallback) return;
    if (lmSlabIsVault(s)) {
        add(lmFeatOfMega(s.megaForm, s.megaSub), -1, -1);
        if (!s.dark && lmSlabHasEscherKnotAt(s.rect.j, s.rect.i)) add(LN_ESCHER, s.w / 2, s.h / 2);
    } else if (s.wing >= 0 && s.wing < LM_WING_COUNT) add(LN_WING_PARTITIONS + s.wing, -1, -1);
    if (s.dark) add(LN_DARK, -1, -1);                 // nobody misses a slab with the lights out
    if (s.xeno.active) {                              // a cut of another level is noticed from any side of it
        int mx = (s.xeno.x0 + s.xeno.x1) / 2, mz = (s.xeno.z0 + s.xeno.z1) / 2;
        add(LN_XENO, s.xeno.x0, mz); add(LN_XENO, s.xeno.x1, mz); add(LN_XENO, mx, s.xeno.z0); add(LN_XENO, mx, s.xeno.z1);
    }
    for (int k = 0; k < s.siteCount; k++) {
        const LimenSite& q = s.sites[k];
        int f = -1;
        switch (q.type) {
            case LS_XENOGRAFT: f = LN_XENO; break;
            case LS_WALL_DOOR: f = LN_WALL_DOOR; break;
            case LS_PLUG_CORR: f = LN_PLUG; break;
            case LS_CEIL_STAIR: f = LN_CEIL_STAIR; break;
            case LS_VITRINE_WIN: f = LN_VITRINE; break;
            case LS_PHASE_SHIFT: f = LN_PHASE; break;
            default: break;
        }
        add(f, q.lx, q.lz);
    }
    if (s.nestedRings >= 2 && s.nestedX >= 0) {
        add(LN_NESTED, lmClampInt(s.nestedX, 0, s.w - 1), lmClampInt(s.nestedZ, 0, s.h - 1));
        if (s.stampX0 >= 0 && s.spiralW == 0) {       // a stamped one: its outer ring on every side
            int mx = (s.stampX0 + s.stampX1) / 2, mz = (s.stampZ0 + s.stampZ1) / 2;
            add(LN_NESTED, s.stampX0, mz); add(LN_NESTED, s.stampX1, mz); add(LN_NESTED, mx, s.stampZ0); add(LN_NESTED, mx, s.stampZ1);
        }
    }
    if (s.spiralW > 0 && s.spiralX >= 0) {
        int mx = s.spiralX + s.spiralW / 2, mz = s.spiralZ + s.spiralH / 2;
        add(LN_SPIRAL, mx, mz);
        add(LN_SPIRAL, s.spiralX, mz); add(LN_SPIRAL, s.spiralX + s.spiralW - 1, mz);
        add(LN_SPIRAL, mx, s.spiralZ); add(LN_SPIRAL, mx, s.spiralZ + s.spiralH - 1);
    }
    // (a basin has several stairs, a chasm and a well are many cells wide: a few spots each, well apart,
    // so that walking past the far end of one counts as much as walking past the near end)
    bool echo = false;
    struct Few { int n = 0; short x[5], z[5]; };
    Few stairs, chasms, wells;
    auto few = [&](Few& q, int f, int x, int z) {
        if (q.n >= 5) return;
        for (int k = 0; k < q.n; k++) if (abs(q.x[k] - x) < 5 && abs(q.z[k] - z) < 5) return;
        q.x[q.n] = (short)x; q.z[q.n] = (short)z; q.n++;
        add(f, x, z);
    };
    int drunkSpots = 0;
    for (int x = 0; x < s.w; x++) {
        for (int z = 0; z < s.h; z++) {
            unsigned char zn = s.zone[x][z];
            int t = s.tag[x][z];
            if (!echo && (zn & LZ_ECHO_CORR)) { add(LN_ECHO, x, z); echo = true; }
            if (s.flag[x][z] & LF_STAIR_DESC) few(stairs, LN_BASIN_WALK, x, z);
            if (t == LT_BRIDGE && (s.flag[x][z] & LF_OVER_VOID) && s.megaForm != MF_TENDRILS) few(chasms, LN_CHASM, x, z);
            if ((zn & LZ_TALL_WELL) && lmIsWalkableTag(t)) few(wells, LN_SKYWELL, x, z);
            if ((x % 4) == 1 && (z % 4) == 1 && drunkSpots < 14 && lmIsWalkableTag(t) && !lmSlabIsVault(s) &&
                lmDeliriumCell255(s.rect.x0 + 1 + x, s.rect.z0 + 1 + z) >= 115) { add(LN_DRUNK, x, z); drunkSpots++; }
        }
    }
}
inline unsigned long long lmNarSpotMask(const std::vector<LmSpot>& v) {
    unsigned long long m = 0;
    for (const LmSpot& q : v) m |= lmFeatBit(q.feat);
    return m;
}

// The furnisher reports rules that fired (game build only; the tool goes by what was asked).
inline void lmNarNote(int feat, int wx, int wz) {
    if (feat < 0 || feat >= LN_COUNT) return;
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return;
    LmNarSlab& rec = lmNarRec(r.j, r.i);
    int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
    for (const LmSpot& q : rec.notes) if (q.feat == feat && q.lx == lx && q.lz == lz) return;
    if (rec.notes.size() < 96) rec.notes.push_back({(unsigned char)feat, (short)lx, (short)lz});
}

// ---------------------------------------------------------------- eyes
// Has the player seen this slab at all? Seen slabs are final.
inline bool lmNarSlabSeen(int j, int i) {
    const LmNarSlab* r = lmNarRecPeek(j, i);
    return r && (r->locked || r->entered || r->glimpsed || r->seenCount > 0);
}

inline void lmNarWitness(int feat, int j, int i);

// Casts the player's field of view through the cell grid. A ray ends at a full wall, at a slab
// that is not built, or where fog has swallowed it (the shader's own densities: tens of metres
// in ordinary rooms, the whole far plane inside a vault). Low shells inside vaults and vitrine
// windows are looked over / through. On roofless levels a blocked ray still catches the bulk of
// any vault that stands beyond the wall.
inline void lmNarLook(float ex, float ez, float yaw, float halfFov, float cs, int rays = 120) {
    if (!gLimen.dnaReady) return;
    const LimenLevelDNA& D = gLimenDNA;
    bool sky = lmHasQuirk(D, LQ_SKY);
    float normD = (0.032f + (float)D.fogDensity * 0.068f / 255.0f) * (sky ? 0.45f : 1.0f);
    float megaD = (0.0046f + (float)D.megaFogDensity * 0.0076f / 255.0f) * (sky ? 0.45f : 1.0f);
    const float maxTau = 3.9f, maxDist = 318.0f;
    int ecx = (int)floorf(ex / cs), ecz = (int)floorf(ez / cs);
    long long recKey = 0; LmNarSlab* rec = nullptr;
    LimenRect cur = {0, 0, 1, 1, 0, 0}; bool haveCur = false;
    const LimenSlab* curSlab = nullptr;
    auto slabOf = [&](int wx, int wz) {
        if (!haveCur || wx < cur.x0 || wx >= cur.x1 || wz < cur.z0 || wz >= cur.z1) {
            cur = lmSlabAt(wx, wz); haveCur = true;
            curSlab = lmSlabPeek(cur.j, cur.i);
        }
    };
    auto mark = [&](int wx, int wz) {
        long long key = lmSlabKey(cur.j, cur.i);
        if (!rec || key != recKey) { rec = &gNar.slabs[key]; recKey = key; }
        int lx = wx - cur.x0 - 1, lz = wz - cur.z0 - 1;
        unsigned long long bit = 1ull << lz;
        if (rec->seen[lx] & bit) return;
        rec->seen[lx] |= bit;
        if (rec->seenCount++ == 0 || (rec->seenCount & 7) == 0) {
            bool have = false;
            for (long long k : gNar.touched) if (k == key) { have = true; break; }
            if (!have) gNar.touched.push_back(key);
        }
    };
    for (int q = 0; q < rays; q++) {
        float a = yaw + halfFov * (2.0f * ((float)q + 0.5f) / (float)rays - 1.0f);
        float dx = sinf(a), dz = cosf(a);
        int wx = ecx, wz = ecz;
        int sx = dx > 0 ? 1 : -1, sz = dz > 0 ? 1 : -1;
        float tdx = fabsf(dx) > 1e-5f ? cs / fabsf(dx) : 1e9f, tdz = fabsf(dz) > 1e-5f ? cs / fabsf(dz) : 1e9f;
        float tmx = fabsf(dx) > 1e-5f ? ((dx > 0 ? (wx + 1) * cs - ex : ex - wx * cs) / fabsf(dx)) : 1e9f;
        float tmz = fabsf(dz) > 1e-5f ? ((dz > 0 ? (wz + 1) * cs - ez : ez - wz * cs) / fabsf(dz)) : 1e9f;
        float t = 0.0f, tau = 0.0f;
        bool over = false;      // past a wall, only vault bulk counts (roofless levels)
        for (int step = 0; step < 96; step++) {
            slabOf(wx, wz);
            bool seam = (wx == cur.x0 || wz == cur.z0);
            float tNext = tmx < tmz ? tmx : tmz;
            float dens = normD;
            bool blocked = false;
            if (seam) {
                if (!over && lmTagAt(wx, wz) == LT_WALL) blocked = true;
            } else {
                if (!curSlab) break;
                int lx = wx - cur.x0 - 1, lz = wz - cur.z0 - 1;
                int tag = curSlab->tag[lx][lz];
                bool vault = lmSlabIsVault(*curSlab);
                if (vault && !lmIsLowShellTag(tag)) dens = megaD;
                if (over) {
                    if (vault) {
                        long long key = lmSlabKey(cur.j, cur.i);
                        if (!rec || key != recKey) { rec = &gNar.slabs[key]; recKey = key; }
                        rec->glimpsed = true;
                    }
                } else {
                    mark(wx, wz);
                    if (tag == LT_WALL) {
                        bool lowShell = vault && (curSlab->flag[lx][lz] & LF_LOW_WALL) != 0;
                        if (!lowShell && !lmSlabHasSite(*curSlab, lx, lz, LS_VITRINE_WIN)) blocked = true;
                    }
                }
            }
            if (blocked) { if (!sky) break; over = true; }
            tau += (tNext - t) * (over ? megaD : dens);
            t = tNext;
            if (tau > maxTau || t > maxDist) break;
            if (tmx < tmz) { tmx += tdx; wx += sx; } else { tmz += tdz; wz += sz; }
        }
    }
}
inline bool lmNarCellSeen(const LmNarSlab& rec, int lx, int lz, int w, int h) {
    for (int x = lx - 1; x <= lx + 1; x++) for (int z = lz - 1; z <= lz + 1; z++)
        if (x >= 0 && z >= 0 && x < w && z < h && (rec.seen[x] & (1ull << z))) return true;
    return false;
}

// Counts what the last look brought into view.
inline void lmNarWitnessTouched(float ex, float ez, float cs) {
    int ecx = (int)floorf(ex / cs), ecz = (int)floorf(ez / cs);
    LimenRect here = lmSlabAt(ecx, ecz);
    long long hereKey = lmSlabKey(here.j, here.i);
    bool hereListed = false;
    for (long long k : gNar.touched) if (k == hereKey) hereListed = true;
    if (!hereListed) gNar.touched.push_back(hereKey);
    for (long long key : gNar.touched) {
        int j = (int)(key >> 32), i = (int)(key & 0xFFFFFFFFll);
        const LimenSlab* s = lmSlabPeek(j, i);
        if (!s) continue;
        LmNarSlab& rec = gNar.slabs[key];
        if (!rec.spotsValid) { lmNarBuildSpots(*s, rec.spots); rec.spotsValid = true; }
        for (int pass = 0; pass < 2; pass++) {
            const std::vector<LmSpot>& list = pass == 0 ? rec.spots : rec.notes;
            for (const LmSpot& q : list) {
                if (rec.witnessed & lmFeatBit(q.feat)) continue;
                const LmFeatInfo& fi = lmFeatInfo(q.feat);
                bool got = false;
                if (q.lx < 0) {
                    if (fi.cls == LNC_MEGA) got = (rec.entered && rec.seenCount >= 30) || rec.seenCount >= 90;
                    else if (q.feat == LN_DARK) got = rec.entered && rec.inside >= 2.0f && rec.seenCount >= 12;
                    else got = rec.entered && rec.inside >= 5.0f && rec.seenCount >= 24;
                } else if (lmNarCellSeen(rec, q.lx, q.lz, s->w, s->h)) {
                    float sx = (s->rect.x0 + 1 + q.lx + 0.5f) * cs, sz = (s->rect.z0 + 1 + q.lz + 0.5f) * cs;
                    float d2 = (sx - ex) * (sx - ex) + (sz - ez) * (sz - ez);
                    got = d2 <= fi.seeDist * fi.seeDist;
                }
                if (got) { rec.witnessed |= lmFeatBit(q.feat); lmNarWitness(q.feat, j, i); }
            }
        }
    }
    gNar.touched.clear();
}
