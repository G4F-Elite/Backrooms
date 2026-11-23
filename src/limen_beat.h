#pragma once
// LIMEN beats: what the narrator (limen_narrator.h) asks of single slabs.
//
// The generator stays a pure function, only of one more argument: (seed, level, cell, beats).
// A slab without a beat is exactly what the genes make of it. A beat can pin the megastructure
// of a slab (or forbid one), guarantee anomalies that are otherwise rolled, raise a delirium
// pocket, and tell the slab where the player's road runs so that all of it happens next to
// that road and megastructures are composed around it.
//
// Beats are data: this file knows nothing about why one was placed. With an empty table every
// query below returns "nothing asked" and the world is byte-identical to a world without a
// narrator (limen_tool test checks that).
#include "limen_core.h"
#include <unordered_map>
#include <vector>

enum LmBeatForce : unsigned {
    LBF_VITRINE    = 1u << 0,    // a sealed lit room behind a window
    LBF_WALL_DOOR  = 1u << 1,    // closed doors hung on blank walls
    LBF_CEIL_STAIR = 1u << 2,    // a staircase into the ceiling
    LBF_PLUG       = 1u << 3,    // a numbered corridor ending in a plug
    LBF_ECHO       = 1u << 4,    // deja-vu corridor
    LBF_PHASE      = 1u << 5,    // doors / objects sunk into the floor or lifted off it
    LBF_XENO       = 1u << 6,    // a rectangle of another level
    LBF_DARK       = 1u << 7,    // the whole slab dark but for one lamp
    LBF_NESTED     = 1u << 8,    // room inside a room inside a room
    LBF_SPIRAL     = 1u << 9,    // spiral corridor
    LBF_ESCHER     = 1u << 10,   // impossible stair knot (vault slabs)
    LBF_MONO       = 1u << 11,   // rooms flooded with one object
    LBF_LACK       = 1u << 12,   // furniture around a missing centrepiece
    LBF_FACEWALL   = 1u << 13,   // furniture facing the wall
    LBF_TWIN       = 1u << 14,   // one wrong detail in a row of identical rooms
    LBF_WAITVOID   = 1u << 15,   // rows of seats facing nothing
    LBF_PROCESSION = 1u << 16    // a queue of objects snaking through a hall
};

const int LB_FORM_KEEP = -2;     // megaForm: whatever the genes say
const int LB_FORM_FLAT = -1;     // megaForm: one storey, no vault here

struct LmBeat {
    signed char megaForm = LB_FORM_KEEP;   // LB_FORM_KEEP, LB_FORM_FLAT or an LmMegaForm
    signed char megaMode = -1;             // ziggurat: 0 stair, 1 pyramid, 2 amphitheatre; -1 = as rolled
    signed char wing = -1;                 // LmWingKind, -1 = as rolled
    signed char haloPin = -2;              // -2 = as derived; else the slab's haloDir is held at this value (-1..3),
                                           // so a neighbour may grow a megastructure without this slab noticing
    unsigned char tiers = 0;               // storeys of a pinned megaform, 0 = its own minimum
    unsigned force = 0;                    // LmBeatForce bits
    signed char geneAdd[16] = {};          // added to the slab's genes (never LG_EDGE / LG_HEIGHT)
    // The road: a straight line of world cells the player is expected to travel.
    // axis 0: the line runs along X at z == roadCoord; axis 1: along Z at x == roadCoord.
    signed char roadAxis = -1;
    int roadCoord = 0;
    bool roadThrough = false;              // the megaform is laid out around the road and keeps standing on it
    // On-foot approach doorway into this slab (slab-local cell, -1 = none): anomalies, xenografts,
    // nested/spiral rooms and delirium pockets anchor right inside this entrance!
    short entryX = -1, entryZ = -1;
    // A delirium pocket raised inside this slab (world cells; fades out before the slab's edge).
    unsigned char pocketPeak = 0;          // 0 = none, else 0..255 strength at the centre
    short pocketR = 0;
    int pocketX = 0, pocketZ = 0;
    unsigned feature = 0;                  // narrator bookkeeping: bitmask over its own catalogue (low 32) ...
    unsigned featureHi = 0;                // ... and the rest of it
};

inline std::unordered_map<long long, LmBeat> gLmBeats;
struct LmPocket { int x0, z0, x1, z1, cx, cz, r, peak; };   // slab interior rect (inclusive), centre, radius, peak
inline std::vector<LmPocket> gLmPockets;

inline long long lmSlabKey(int j, int i) { return (long long)(((unsigned long long)(u32)j << 32) | (unsigned long long)(u32)i); }

inline const LmBeat* lmBeatAt(int j, int i) {
    if (gLmBeats.empty()) return nullptr;
    auto it = gLmBeats.find(lmSlabKey(j, i));
    return it == gLmBeats.end() ? nullptr : &it->second;
}
inline unsigned lmBeatForce(int j, int i) { const LmBeat* b = lmBeatAt(j, i); return b ? b->force : 0u; }
inline bool lmBeatForces(int j, int i, unsigned bit) { return (lmBeatForce(j, i) & bit) != 0u; }

inline void lmBeatsRebuildPockets() {
    gLmPockets.clear();
    for (const auto& kv : gLmBeats) {
        const LmBeat& b = kv.second;
        if (b.pocketPeak == 0 || b.pocketR < 2) continue;
        int j = (int)(kv.first >> 32), i = (int)(kv.first & 0xFFFFFFFFll);
        LimenRect r = lmSlabRect(j, i);
        gLmPockets.push_back({r.x0 + 1, r.z0 + 1, r.x1 - 1, r.z1 - 1, b.pocketX, b.pocketZ, b.pocketR, b.pocketPeak});
    }
}

inline unsigned gLmBeatsGen = 0;    // bumped whenever the table is wiped: whoever filled it knows to start over
inline void lmBeatsClear() { gLmBeats.clear(); gLmPockets.clear(); gLmBeatsGen++; }

// Strength (0..255) of narrator-raised delirium at a world cell. Zero on the outer two cells of
// the slab, so seams and neighbours never notice a pocket.
inline int lmBeatPocketAt(int wx, int wz) {
    int best = 0;
    for (const LmPocket& p : gLmPockets) {
        if (wx < p.x0 || wx > p.x1 || wz < p.z0 || wz > p.z1) continue;
        int dx = wx - p.cx, dz = wz - p.cz;
        int d2 = dx * dx + dz * dz, r2 = p.r * p.r;
        if (d2 >= r2) continue;
        int edge = wx - p.x0;
        if (p.x1 - wx < edge) edge = p.x1 - wx;
        if (wz - p.z0 < edge) edge = wz - p.z0;
        if (p.z1 - wz < edge) edge = p.z1 - wz;
        if (edge < 2) continue;
        int v = p.peak * (r2 - d2) / r2;           // parabolic dome
        v = v + (p.peak - v) / 3;                  // with a fuller shoulder
        if (edge < 5) v = v * (edge - 1) / 4;
        if (v > best) best = v;
    }
    return best;
}

// Where in the slab things should happen: the road's across-coordinate clamped into the slab
// (a road passing just outside pulls them to the edge that faces it). -1 when there is no road.
inline int lmBeatFocusLocal(const LmBeat* b, const LimenRect& r) {
    if (!b || b->roadAxis < 0) return -1;
    int c = (b->roadAxis == 0) ? (b->roadCoord - r.z0 - 1) : (b->roadCoord - r.x0 - 1);
    int span = (b->roadAxis == 0) ? (r.z1 - r.z0 - 1) : (r.x1 - r.x0 - 1);
    return c < 0 ? 0 : (c >= span ? span - 1 : c);
}
// The road in slab-local terms: its across-coordinate, or -1 when it does not cross this slab.
inline int lmBeatRoadLocal(const LmBeat* b, const LimenRect& r) {
    if (!b || b->roadAxis < 0) return -1;
    int c = (b->roadAxis == 0) ? (b->roadCoord - r.z0 - 1) : (b->roadCoord - r.x0 - 1);
    int span = (b->roadAxis == 0) ? (r.z1 - r.z0 - 1) : (r.x1 - r.x0 - 1);
    return (c >= 0 && c < span) ? c : -1;
}

// A cell for an anomaly: when the player is approaching a specific doorway `(entryX, entryZ)`,
// first attempts land 2..10 cells right behind that doorway; else within a few cells of the road;
// the rest anywhere, exactly as the unsteered generator picked them.
inline void lmBeatPickCell(const LmBeat* b, const LimenRect& r, int w, int h, u32 ha, int attempt, int nearAttempts,
                           int marginLo, int marginHi, int& outX, int& outZ) {
    int spanX = w - marginLo - marginHi, spanZ = h - marginLo - marginHi;
    outX = marginLo + (int)(ha % (u32)(spanX > 0 ? spanX : 1));
    outZ = marginLo + (int)((ha >> 7) % (u32)(spanZ > 0 ? spanZ : 1));
    if (!b || attempt >= nearAttempts) return;
    int maxX = w - marginHi - 1, maxZ = h - marginHi - 1;
    if (b->entryX >= 0 && b->entryZ >= 0 && attempt < (nearAttempts * 2) / 3) {
        int rad = 10;
        int ox = (int)((ha >> 15) % (u32)(2 * rad + 1)) - rad;
        int oz = (int)((ha >> 21) % (u32)(2 * rad + 1)) - rad;
        int px = (int)b->entryX + ox, pz = (int)b->entryZ + oz;
        if (px < marginLo) px = marginLo;
        if (px > maxX) px = maxX;
        if (pz < marginLo) pz = marginLo;
        if (pz > maxZ) pz = maxZ;
        outX = px; outZ = pz;
        return;
    }
    int c = lmBeatFocusLocal(b, r);
    if (c < 0) return;
    int off = 1 + (int)((ha >> 21) % 4u);          // 1..4 cells off the road
    if ((ha >> 25) & 1u) off = -off;
    int v = c + off;
    int lim = (b->roadAxis == 0 ? h : w) - marginHi - 1;
    if (v < marginLo) v = marginLo;
    if (v > lim) v = lim;
    if (b->roadAxis == 0) outZ = v; else outX = v;
}
