#pragma once
// LIMEN slab: the cell vocabulary every layer speaks.
#include "limen_seam.h"
#include <cstring>

const int LM_MAX = 64;

enum LimenTag {
    LT_WALL = 0,
    LT_CORR = 1,      // circulation under the slab's own ceiling
    LT_ROOM = 2,      // enclosed room (4.5 m, 2.65 m cramped, or 9.0 m mezzanine)
    LT_HALL = 3,      // open floor under the slab's own ceiling
    LT_VOID = 4,      // bottomless shaft (-56 m)
    LT_BRIDGE = 5,    // deck over a drop: viaduct, spur, basin walkway
    LT_BASIN = 6,     // drained sunken floor (-2.85 m; walkable via LF_STAIR_DESC!)
    LT_LOW = 7,       // circulation inside a nested shell
    LT_VITRINE = 8,   // intentionally sealed, lit, furnished room seen only through a window
    LT_TAG_COUNT = 9
};

enum LimenFlag : unsigned short {
    LF_LOW_WALL    = 1u << 0,   // wall of a free-standing shell (4.5 m or 9.0 m with LF_MEZZ)
    LF_OVER_VOID   = 1u << 1,   // deck cell with a bottomless shaft beneath
    LF_OVER_BASIN  = 1u << 2,   // deck cell with a dry basin beneath
    LF_THIN_W      = 1u << 3,   // thin 0.26 m drywall partition on -X edge of walkable cell
    LF_THIN_N      = 1u << 4,   // thin 0.26 m drywall partition on -Z edge of walkable cell
    LF_THIN_DOOR_W = 1u << 5,   // framed doorway in thin -X partition
    LF_THIN_DOOR_N = 1u << 6,   // framed doorway in thin -Z partition
    LF_NARROW_X    = 1u << 7,   // corridor narrowed along X (walls inset on -Z/+Z to ~2.4 m)
    LF_NARROW_Z    = 1u << 8,   // corridor narrowed along Z (walls inset on -X/+X to ~2.4 m)
    LF_STAIR_DESC  = 1u << 9,   // tiled staircase descending from y=0 rim into LT_BASIN (-2.85 m)
    LF_WALK_BASIN  = 1u << 10,  // LT_BASIN floor reachable by player via LF_STAIR_DESC
    LF_CRAMPED     = 1u << 11,  // oppressive 2.65 m service ceiling
    LF_MEZZ        = 1u << 12,  // intermediate 9.0 m two-storey pavilion / tall cut
    LF_CHAMFER     = 1u << 13   // 45-degree beveled hall corner
};

enum LimenZoneBit : unsigned char {
    LZ_XENO      = 1u << 0,     // leaked chunk from a foreign level (opXenograft)
    LZ_PLUG_CORR = 1u << 1,     // rhythmic numbered corridor ending in a solid plug
    LZ_ECHO_CORR = 1u << 2,     // deja-vu corridor repeating one 2-cell slice 3..4 times
    LZ_STORY     = 1u << 3,     // the game needs this cell (rig lane, stamped set, broken wall): kept plain
    LZ_STD_CEIL  = 1u << 4,     // 3.25 m standard institutional room/enfilade ceiling (vs 4.5 m hangar)
    LZ_TALL_WELL = 1u << 5,     // 9.00 m double-height skylight atrium well inside a 1-storey slab
    LZ_SPARSE    = 1u << 6,     // eerily stripped / vacant HobbyTown Backrooms partition bay
    LZ_HALO      = 1u << 7      // compression threshold approaching an adjacent megastructure attractor
};

const int LF_STAIR_DIR_SHIFT = 14;   // 2 bits: 0 = rim on -Z, 1 = rim on +Z, 2 = rim on -X, 3 = rim on +X

inline int lmStairRimDir(unsigned short f) { return (int)((f >> LF_STAIR_DIR_SHIFT) & 3u); }
inline unsigned short lmPackStairDir(int dir) { return (unsigned short)((dir & 3) << LF_STAIR_DIR_SHIFT); }

inline bool lmIsPitTag(int t) { return t == LT_VOID || t == LT_BASIN; }
inline bool lmIsVitrineTag(int t) { return t == LT_VITRINE; }
inline bool lmIsNonWalkOpenTag(int t) { return lmIsPitTag(t) || t == LT_VITRINE; }
inline bool lmIsWalkableTag(int t) { return t != LT_WALL && !lmIsNonWalkOpenTag(t); }
inline bool lmIsCirculationTag(int t) { return t == LT_CORR || t == LT_HALL || t == LT_BRIDGE || t == LT_LOW; }
inline bool lmIsLowShellTag(int t) { return t == LT_ROOM || t == LT_LOW || t == LT_VITRINE; }

enum LimenSiteType {
    LS_DOOR_GAP = 0,     // lack: one door is missing from a regular row, its scar remains
    LS_DOOR_SHIFT = 1,   // displacement: one door sits off the rhythm of its row
    LS_DEAD_LAMP = 2,    // lack: one lamp of a regular lattice is dark
    LS_DARK_SLAB = 3,    // the whole slab is dark except one solitary lamp
    LS_RAIL_GAP = 4,     // lack: missing parapet span along a drop
    LS_DYN_LAMP = 5,     // dynamic candidate: lamp that can die behind the player's back
    LS_XENOGRAFT = 6,    // sharp-cut foreign level intrusion (opXenograft)
    LS_WALL_DOOR = 7,    // closed door embedded directly in a solid wall
    LS_PLUG_CORR = 8,    // dead-end plugged numbered corridor
    LS_CEIL_STAIR = 9,   // staircase ascending straight into solid ceiling
    LS_VITRINE_WIN = 10, // glazed window looking into a sealed furnished room (LT_VITRINE)
    LS_PHASE_SHIFT = 11, // object/door vertically phase-shifted into floor or wall (+1.3m / -0.55m)
    LS_SITE_COUNT = 12
};

struct LimenSite { u32 id; int type; int lx, lz; };

const int LM_MAX_SITES = 28;

// Metadata for a foreign-level rectangle leaked directly into this slab (opXenograft).
struct LmXenograft {
    bool active;
    int foreignLevel;
    int x0, z0, x1, z1;        // inclusive slab-local rectangle
    int ceilMode;              // 0 = 2.65 m cramped, 1 = 4.5 m, 2 = 9.0 m tall cut
    int lampPitch;             // foreign lamp step (2..7)
    unsigned char hue;         // foreign hue (shifts envmap B channel sharply)
    unsigned char useGene;     // foreign LG_USE
    LmSurfDNA wallSurf;        // foreign wall material
    LmSurfDNA floorSurf;       // foreign floor material
    unsigned char themeWeight[LM_THEME_COUNT];
};

struct LimenSlab {
    LimenRect rect;
    int w, h;                 // interior size, cells
    LimenGenome genome;
    int ceilTiers;            // ceiling in 4.5 m storeys: 1 ordinary floor, 2..12 vault
    int rawPhenom;            // 0..255 continuous non-manmade drive: 0..55 architecture, 56..145 hybrid, 146+ phenomenon
    int skyDrive;             // 0..255 continuous upper-sky exposure: 0 sealed, 1..119 pierced roof, 120+ roofless
    int megaForm;             // -1 for 1-storey, else LmMegaForm (MF_CLASSIC .. MF_PLATFORM)
    int megaSub;              // deterministic sub-variant / axis bits for megaForm
    short megaOffX, megaOffZ; // megastructure moved off the slab's middle (cells) to stand on / beside the narrator's road
    signed char roadAxis;     // narrator's road through this slab: -1 none, 0 along X, 1 along Z
    short roadC;              // its slab-local across-coordinate (z for axis 0, x for axis 1)
    signed char focusAxis;    // where the narrator wants things to happen: the road, or the slab edge facing it
    short focusC;
    bool roadThrough;         // megaform laid out around the road: it survives the rig's lane (see lmApplyStory)
    unsigned beatForce;       // LmBeatForce bits the narrator asked of this slab
    short entryX, entryZ;     // slab-local doorway the player is approaching on foot (-1 = none)
    short stampX0, stampZ0, stampX1, stampZ1;   // rectangle of a narrator-stamped nested / spiral structure (x0 < 0: none)
    int deliriumMean;         // 0..255 mean architectural drunkenness across slab
    int deliriumPeak;         // 0..255 peak architectural drunkenness across slab
    int deliriumStruct;       // packed (spiralTurns<<4) | nestedRings
    short nestedX, nestedZ, nestedDir, nestedRings;
    short spiralX, spiralZ, spiralW, spiralH;
    int palette;              // nearest DNA megaPole index (0..2)
    int wing;                 // regional architectural wing (LmWingKind 0..4)
    int wingTheme;            // coherent primary prop theme index (0..LM_THEME_COUNT-1)
    int haloDir;              // -1 or 0..3 (-X,+X,-Z,+Z) toward adjacent megastructure attractor
    int lampPitch;            // lattice step of lamps over open floor
    bool pillars;             // open floor carries a column grid
    bool fallback;            // the grown layout was rejected; plain open floor instead
    int nodes;                // grammar nodes expanded (diagnostics)
    int links, sealed;        // repair: routes cut to join stranded parts, cells sealed solid
    int loops;                // repair: secondary loop connections ensuring non-tree routing
    int thinCount;            // cells carrying thin 0.26 m partitions
    int narrowCount;          // corridor cells narrowed to human ~2.4 m scale
    int walkBasins;           // sunken LT_BASIN cells made player-walkable via stairs
    int deadThresholds;       // count of dead doors, ceiling stairs, plugged halls & vitrine windows
    int vitrineRooms;         // count of sealed lit furnished rooms (LT_VITRINE)
    int share[LT_TAG_COUNT];  // percent of interior cells per tag, measured after repair
    unsigned char tag[LM_MAX][LM_MAX];
    unsigned short flag[LM_MAX][LM_MAX];
    unsigned char zone[LM_MAX][LM_MAX];       // LZ_XENO | LZ_PLUG_CORR | LZ_ECHO_CORR
    unsigned short nodeKey[LM_MAX][LM_MAX];   // grammar identity for identical clone-room props
    unsigned char roomRel[LM_MAX][LM_MAX];    // packed (relX<<4)|relZ inside a room bay
    LmXenograft xeno;
    LimenSite sites[LM_MAX_SITES];
    int siteCount;
    int specialLampX, specialLampZ;
    bool dark;
};

inline bool lmSlabIsVault(const LimenSlab& s) { return s.ceilTiers > 1; }

// Can megaform `form` stand in a slab of w x h cells with a road through it (roadAxis 0: along X
// at local z == c; 1: along Z at local x == c)? If so, how it has to be turned and moved for the
// road to run through its natural opening: the canyon and the tracks across the road, the
// monolith street along it, the silo's and the courtyard's own gateway on it, the pyramid and
// the great stair beside it. One answer for the grammar, the mesher and the narrator.
struct LmRoadFit {
    bool ok = false;
    int axis = -1;        // megaSub axis bit to use (-1: as rolled)
    int posEnd = -1;      // ziggurat stair: which end it climbs to (-1: as rolled)
    int off = 0;          // shift across the road, cells
};
inline LmRoadFit lmMegaRoadFit(int form, int zigMode, int w, int h, int tiers, int grain, int roadAxis, int c) {
    LmRoadFit f;
    int C = roadAxis == 0 ? h : w;          // across the road
    if (roadAxis < 0 || c < 0 || c >= C) return f;
    switch (form) {
    case MF_CLASSIC: case MF_HYPOSTYLE: case MF_HANGING: case MF_SPIRES: case MF_TENDRILS:
        f.ok = true; break;
    case MF_ZIGGURAT:
        if (zigMode >= 2) { f.ok = true; break; }                      // the road bridges the amphitheatre
        if (zigMode == 0) {                                            // the stair climbs away from the road
            int len = lmClampInt(C * 38 / 100, 6, 10);
            f.axis = roadAxis;
            if (c >= len + 5) { f.posEnd = 0; f.ok = true; }
            else if (c <= C - 1 - (len + 5)) { f.posEnd = 1; f.ok = true; }
            break;
        }
        {                                                              // the pyramid stands beside the road
            int maxP = lmClampInt(9 + tiers / 3, 9, 13);
            int P = C - 6 < maxP ? C - 6 : maxP;
            if (P < 5) break;
            int p0 = (C - P) / 2, p1 = p0 + P - 1;
            if (c < p0 - 2 || c > p1 + 2) { f.ok = true; break; }
            int offA = c + 3 - p0, offB = c - 3 - p1;
            bool okA = p1 + offA <= C - 3, okB = p0 + offB >= 2;
            if (okA && (!okB || offA <= -offB)) { f.off = offA; f.ok = true; }
            else if (okB) { f.off = offB; f.ok = true; }
        }
        break;
    case MF_CANYON:
        f.axis = 1 - roadAxis; f.ok = (w >= 12 && h >= 12); break;
    case MF_PLATFORM:
        f.axis = 1 - roadAxis; f.ok = (w >= 12 && h >= 12 && c >= 4 && c <= C - 5); break;
    case MF_MONOLITH:
        f.axis = roadAxis; f.ok = (w >= 10 && h >= 10 && c >= 4 && c <= C - 5); break;
    case MF_SILO: {
        if (w < 11 || h < 11) break;
        int maxD = lmClampInt(11 + grain / 64, 11, 15);
        int dX = w - 4 < maxD ? w - 4 : maxD, dZ = h - 4 < maxD ? h - 4 : maxD;
        int D = dX < dZ ? dX : dZ;
        int s0 = (C - D) / 2, s1 = s0 + D - 1, m = (s0 + s1) / 2;
        f.off = c - m;
        f.ok = (s0 + f.off >= 2 && s1 + f.off <= C - 3);
        break; }
    case MF_CELLBLOCK: {
        if (w < 13 || h < 13) break;
        int maxC = lmClampInt(12 + grain / 48, 12, 18);
        int cA = C - 4 < maxC ? C - 4 : maxC;
        int w0 = (C - cA) / 2 - 1, w1 = w0 + cA + 1, m = (w0 + w1) / 2;
        f.off = c - m;
        f.ok = (w0 + f.off >= 1 && w1 + f.off <= C - 2);
        break; }
    case MF_GROTTO: {
        int d = c - C / 2;
        if (w < 12 || h < 12 || d < -1 || d > 1) break;
        f.off = d == 0 ? -3 : -2 * d;       // the tree steps aside
        f.ok = true;
        break; }
    default: break;
    }
    return f;
}
inline int lmPitShare(const LimenSlab& s) { return s.share[LT_VOID] + s.share[LT_BASIN]; }

// A cell the game needs (LZ_STORY) within `reach` cells: free-standing set pieces keep away.
inline bool lmStoryNear(const LimenSlab& s, int lx, int lz, int reach) {
    for (int x = lx - reach; x <= lx + reach; x++) for (int z = lz - reach; z <= lz + reach; z++)
        if (x >= 0 && z >= 0 && x < s.w && z < s.h && (s.zone[x][z] & LZ_STORY)) return true;
    return false;
}

inline int lmTagLocal(const LimenSlab& s, int lx, int lz) {
    if (lx < 0 || lz < 0 || lx >= s.w || lz >= s.h) return LT_WALL;
    return s.tag[lx][lz];
}

inline unsigned short lmFlagLocal(const LimenSlab& s, int lx, int lz) {
    if (lx < 0 || lz < 0 || lx >= s.w || lz >= s.h) return 0;
    return s.flag[lx][lz];
}

inline unsigned char lmZoneLocal(const LimenSlab& s, int lx, int lz) {
    if (lx < 0 || lz < 0 || lx >= s.w || lz >= s.h) return 0;
    return s.zone[lx][lz];
}

// Local shell/cut ceiling height in meters:
//   2.72 m (LF_CRAMPED: oppressive Backrooms / service / corridor ceiling, > PROP_OVERHEAD_Y)
//   3.25 m (LZ_STD_CEIL: human-scale institutional room / enfilade ceiling)
//   4.50 m (WH: concourse / lobby / story datum)
//   9.00 m (LF_MEZZ or LZ_TALL_WELL: double-height skylight / pavilion)
inline float lmShellCeilHeight(unsigned short flg, int slabTiers, float wh, unsigned char zn = 0) {
    if ((zn & LZ_TALL_WELL) != 0 && slabTiers == 1) return wh * 2.0f;
    if ((flg & LF_MEZZ) != 0 && (slabTiers >= 3 || (zn & LZ_XENO) != 0)) return wh * 2.0f;
    if ((flg & LF_CRAMPED) != 0) return 2.72f;
    if ((zn & LZ_STD_CEIL) != 0 && slabTiers == 1) return 3.25f;
    return wh;
}

inline int lmPitDegreeLocal(const LimenSlab& s, int lx, int lz) {
    int n = 0;
    if (lmIsPitTag(lmTagLocal(s, lx - 1, lz))) n++;
    if (lmIsPitTag(lmTagLocal(s, lx + 1, lz))) n++;
    if (lmIsPitTag(lmTagLocal(s, lx, lz - 1))) n++;
    if (lmIsPitTag(lmTagLocal(s, lx, lz + 1))) n++;
    return n;
}

inline bool lmAddSite(LimenSlab& s, int type, int lx, int lz) {
    if (s.siteCount >= LM_MAX_SITES) return false;
    LimenSite& site = s.sites[s.siteCount++];
    site.id = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(type * 257 + s.siteCount), 0x5349u);
    site.type = type; site.lx = lx; site.lz = lz;
    return true;
}

inline bool lmSlabHasSite(const LimenSlab& s, int lx, int lz, int type) {
    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].type == type && s.sites[k].lx == lx && s.sites[k].lz == lz) return true;
    }
    return false;
}
