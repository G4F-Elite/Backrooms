#pragma once
#include <cmath>
// LIMEN space grammar. There are no builders for kinds of places here, only eight
// operators that rewrite a rectangle of a slab:
//   spine  - drive a straight axial avenue across the region (snapping to transit ports)
//   split  - cut into bays (identical when the genome is orderly) with some separator
//   ring   - peel a band off the perimeter and keep growing inside it
//   sink   - drop the floor away, then cross it, break the crossings, raise islands
//   shell  - enclose a nested building (2.65 m, 4.5 m, or 9.0 m mezzanine) and grow inside
//   cells  - pack rooms behind doors along corridors (using 0.26 m thin partitions when light)
//   field  - leave open floor, optionally with piers, baffle screens, and beveled corners
//   mass   - leave it solid, or hollow it into a tower
// Which operator fires, with which numbers, is rolled from the region's genome; each child
// region inherits a mutated copy. Any nesting is legal. limen_build.h repairs connectivity
// and guarantees loops afterwards.
#include "limen_slab.h"

struct LgBuild {
    LimenSlab* s;
    bool vault;
    int nodes;
    int doorCounter, gapDoor, shiftDoor;
    int gapX, gapZ, shiftX, shiftZ;
};

enum { LG_W = 1, LG_E = 2, LG_N = 4, LG_S = 8 };

struct LgNode {
    int x0, z0, x1, z1;      // inclusive, slab-local
    LimenGenome g;
    u32 h;                   // identity: equal hash + equal genome + equal size = equal layout
    int depth;
    unsigned walled;         // sides already closed by a wall just outside the rectangle
    bool low;                // inside a nested shell of a vault
    bool mezz;               // inside a 9.0 m intermediate mezzanine shell
};

inline void lgGrow(LgBuild& b, const LgNode& n);

// ---------------------------------------------------------------- primitives
inline int lgR(const LgNode& n, u32 key, int mod) { return (int)(lmHash(n.h, key, 0u, 0x4C47u) % (u32)(mod < 1 ? 1 : mod)); }
inline int lgBias(int g) { return g * g / 255; }
inline int lgGrain(const LgNode& n) { return 4 + n.g.g[LG_GRAIN] / 15; }
// Operator-family affinity of a region (0..255) and its quadratic operator weight `boost * a^2 / 255^2 * 2`,
// which averages ~0.22*boost over uniform genomes: a family is rare on average but commits strongly when present.
inline int lgFamAff(const LgBuild& b, const LgNode& n, int k) { return lmWingAffinityAt(n.g.g, b.s->rect.j, b.s->rect.i, k); }
inline int lgFamW(int a, int boost) { return boost * a * a * 2 / (255 * 255); }
inline int lgFloorTag(const LgNode& n) { return n.low ? LT_ROOM : LT_HALL; }
inline int lgPathTag(const LgNode& n) { return n.low ? LT_LOW : LT_CORR; }

inline void lgSet(LgBuild& b, int x, int z, int tag) {
    LimenSlab& s = *b.s;
    if (x < 0 || z < 0 || x >= s.w || z >= s.h) return;
    s.tag[x][z] = (unsigned char)tag;
    s.flag[x][z] = 0;
    s.zone[x][z] &= LZ_XENO;
    s.nodeKey[x][z] = 0;
    s.roomRel[x][z] = 0;
}

inline void lgFill(LgBuild& b, int x0, int z0, int x1, int z1, int tag) {
    for (int x = x0; x <= x1; x++) for (int z = z0; z <= z1; z++) lgSet(b, x, z, tag);
}

// Axis frame: axis 0 maps (along, across) to (x, z), axis 1 to (z, x).
inline void lgFillA(LgBuild& b, int axis, int a0, int c0, int a1, int c1, int tag) {
    if (axis == 0) lgFill(b, a0, c0, a1, c1, tag);
    else lgFill(b, c0, a0, c1, a1, tag);
}

inline void lgOrFlagA(LgBuild& b, int axis, int a, int c, unsigned short flg) {
    LimenSlab& s = *b.s;
    int x = axis == 0 ? a : c, z = axis == 0 ? c : a;
    if (x < 0 || z < 0 || x >= s.w || z >= s.h) return;
    s.flag[x][z] |= flg;
}

// Stamps deterministic clone-room identity from LgNode::h so identical bays get identical props.
inline void lgStampRoomIdentity(LgBuild& b, int axis, int a0, int r0, int a1, int r1, u32 roomKey) {
    LimenSlab& s = *b.s;
    unsigned short nk = (unsigned short)(1u + (roomKey % 65534u));
    for (int a = a0; a <= a1; a++) {
        for (int r = r0; r <= r1; r++) {
            int x = axis == 0 ? a : r, z = axis == 0 ? r : a;
            if (x < 0 || z < 0 || x >= s.w || z >= s.h) continue;
            int ra = (a - a0) & 15, rr = (r - r0) & 15;
            s.nodeKey[x][z] = nk;
            s.roomRel[x][z] = (unsigned char)((ra << 4) | rr);
        }
    }
}

// Bevels the four corners of a rectangular hall/court so slabs are not pure AABB boxes.
inline void lgChamferCorners(LgBuild& b, int x0, int z0, int x1, int z1, int cut) {
    LimenSlab& s = *b.s;
    if (cut < 1) return;
    for (int dx = 0; dx < cut; dx++) {
        for (int dz = 0; dz < cut; dz++) {
            if (dx + dz >= cut) continue;
            bool diagEdge = (dx + dz == cut - 1);
            int xs[2] = {x0 + dx, x1 - dx}, zs[2] = {z0 + dz, z1 - dz};
            for (int ix = 0; ix < 2; ix++) {
                for (int iz = 0; iz < 2; iz++) {
                    int x = xs[ix], z = zs[iz];
                    if (x <= 0 || z <= 0 || x >= s.w - 1 || z >= s.h - 1) continue;
                    lgSet(b, x, z, LT_WALL);
                    if (diagEdge) s.flag[x][z] |= LF_CHAMFER;
                }
            }
        }
    }
}

// A deck only exists over a drop; solid ground, islands and monoliths are left alone.
inline void lgDeck(LgBuild& b, int x, int z) {
    LimenSlab& s = *b.s;
    if (x < 0 || z < 0 || x >= s.w || z >= s.h) return;
    if (s.tag[x][z] == LT_VOID) { s.tag[x][z] = LT_BRIDGE; s.flag[x][z] = LF_OVER_VOID; }
    else if (s.tag[x][z] == LT_BASIN) { s.tag[x][z] = LT_BRIDGE; s.flag[x][z] = LF_OVER_BASIN; }
}

inline void lgDeckA(LgBuild& b, int axis, int a0, int a1, int c) {
    for (int a = a0; a <= a1; a++) { if (axis == 0) lgDeck(b, a, c); else lgDeck(b, c, a); }
}

inline LgNode lgChild(const LgNode& p, int x0, int z0, int x1, int z1, u32 key, unsigned walled, int graftBoost) {
    LgNode c;
    c.x0 = x0; c.z0 = z0; c.x1 = x1; c.z1 = z1;
    c.depth = p.depth + 1; c.walled = walled; c.low = p.low; c.mezz = p.mezz;
    c.h = lmHash(p.h, key, (u32)c.depth, 0x4348u);
    int area = (x1 - x0 + 1) * (z1 - z0 + 1);
    bool graft = area >= 16 && (int)(lmHash(c.h, 1u, 0u, 0x4752u) % 2048u) < p.g.g[LG_GRAFT] * graftBoost;
    int amp = 5 + (255 - p.g.g[LG_ORDER]) / 6;
    for (int k = 0; k < LG_COUNT; k++) {
        u32 r = lmHash(c.h, (u32)k, 2u, 0x4D55u);
        if (graft) c.g.g[k] = (unsigned char)(r & 255u);
        else c.g.g[k] = (unsigned char)lmClamp255((int)p.g.g[k] + (int)(r % (u32)(2 * amp + 1)) - amp);
    }
    return c;
}

// A row of equal rooms with one door each. When `thinDividers` is true (low LG_MASS),
// rooms stand back-to-back separated by 0.26 m drywall partitions with connecting suite
// doorways instead of 5 m solid concrete blocks!
inline void lgRoomRow(LgBuild& b, const LgNode& n, int axis, int a0, int a1, int r0, int r1,
                      int door, int roomW, int doorTag) {
    int len = a1 - a0 + 1;
    if (len < 2 || r1 < r0) return;
    bool thinDividers = (n.g.g[LG_MASS] < 145) && len >= 4;
    int step = thinDividers ? roomW : (roomW + 1);
    int count = thinDividers ? (len / step) : ((len + 1) / step);
    if (count <= 0) return;
    int used = thinDividers ? (count * roomW) : (count * (roomW + 1) - 1);
    int start = a0 + (len - used) / 2;
    u32 baseRoomId = lmHash(n.h, (u32)(r1 - r0 + 1), (u32)roomW, 0x524Du);

    for (int k = 0; k < count; k++) {
        int p0 = start + k * step;
        int off = roomW / 2;
        int index = b.doorCounter++;
        if (index == b.shiftDoor) off = (off == 0) ? roomW - 1 : 0;
        lgFillA(b, axis, p0, r0, p0 + roomW - 1, r1, LT_ROOM);
        // Orderly rows share one room key across the whole row so props repeat identically!
        u32 rk = (n.g.g[LG_ORDER] >= 120) ? baseRoomId : lmHash(baseRoomId, (u32)k, 0u, 0x524Eu);
        lgStampRoomIdentity(b, axis, p0, r0, p0 + roomW - 1, r1, rk);

        if (thinDividers && k > 0) {
            unsigned short wallBit = (axis == 0) ? LF_THIN_W : LF_THIN_N;
            unsigned short doorBit = (axis == 0) ? LF_THIN_DOOR_W : LF_THIN_DOOR_N;
            int midR = (r0 + r1) / 2;
            for (int r = r0; r <= r1; r++) {
                unsigned short bits = wallBit | (r == midR ? doorBit : 0u);
                lgOrFlagA(b, axis, p0, r, bits);
            }
        }

        int dx = axis == 0 ? p0 + off : door, dz = axis == 0 ? door : p0 + off;
        if (index == b.gapDoor) { b.gapX = dx; b.gapZ = dz; continue; }
        if (index == b.shiftDoor) { b.shiftX = dx; b.shiftZ = dz; }
        lgSet(b, dx, dz, doorTag);
    }
}

// ---------------------------------------------------------------- field
inline void lgField(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    int A = W < H ? W : H;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, lgFloorTag(n));

    // Bevel corners into octagonal/chamfered halls when moderately orderly
    if (!n.low && A >= 9 && g[LG_ORDER] >= 75 && lgR(n, 19u, 256) < 115) {
        int cut = A >= 16 ? (2 + lgR(n, 18u, 2)) : 2;
        lgChamferCorners(b, n.x0, n.z0, n.x1, n.z1, cut);
    }

    // In non-vault slabs, break up bare "oatmeal" fields with baffle partitions or pier rows
    int latThresh = 24 + lgBias(g[LG_LATTICE]) + (!b.vault && A >= 8 ? 85 : 0);
    if (W < 5 || H < 5 || lgR(n, 20u, 256) >= latThresh) return;

    int mw = 1 + g[LG_MASS] * lgR(n, 21u, 4) / 256;
    int md = 1 + g[LG_MASS] * lgR(n, 22u, 6) / 256;
    if (!b.vault && mw == 1 && md == 1 && g[LG_MASS] >= 70) {
        // Baffle fin walls instead of 1x1 dots
        if (W >= H) md = 2 + (g[LG_MASS] > 160 ? 1 : 0);
        else mw = 2 + (g[LG_MASS] > 160 ? 1 : 0);
    }
    if (n.low) { if (mw > 2) mw = 2; if (md > 2) md = 2; }
    int ax = 2 + lgR(n, 23u, 4) + (255 - g[LG_MASS]) / 100;
    int az = lgR(n, 24u, 256) < g[LG_ORDER] ? ax : 2 + lgR(n, 25u, 4) + (255 - g[LG_MASS]) / 100;
    int nx = (W - 2 + ax) / (mw + ax), nz = (H - 2 + az) / (md + az);
    if (nx < 1 || nz < 1) return;
    int sx = n.x0 + (W - (nx * (mw + ax) - ax)) / 2, sz = n.z0 + (H - (nz * (md + az) - az)) / 2;
    for (int kx = 0; kx < nx; kx++) {
        for (int kz = 0; kz < nz; kz++) {
            if ((int)(lmHash(n.h, (u32)kx, (u32)kz, 0x4C41u) % 256u) < g[LG_DECAY] / 3) continue;
            int x = sx + kx * (mw + ax), z = sz + kz * (md + az);
            lgFill(b, x, z, x + mw - 1, z + md - 1, LT_WALL);
        }
    }
}

// ---------------------------------------------------------------- mass
inline void lgMass(LgBuild& b, const LgNode& n) {
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
    if (W < 4 || H < 4 || lgR(n, 30u, 3) == 0) return;
    lgFill(b, n.x0 + 1, n.z0 + 1, n.x1 - 1, n.z1 - 1, lgFloorTag(n));   // hollow tower
    int slits = 1 + lgR(n, 31u, 2);
    for (int k = 0; k < slits; k++) {
        int side = lgR(n, 32u + (u32)k, 4);
        int x = n.x0 + 1 + lgR(n, 34u + (u32)k, W - 2), z = n.z0 + 1 + lgR(n, 36u + (u32)k, H - 2);
        if (side == 0) x = n.x0; else if (side == 1) x = n.x1; else if (side == 2) z = n.z0; else z = n.z1;
        lgSet(b, x, z, lgPathTag(n));
    }
}

// ---------------------------------------------------------------- spine
// Straight axial corridor/gallery cutting end-to-end across the region, snapping to aligned
// multi-slab transit ports at root depth so long 300 m vistas pierce across slab boundaries.
inline void lgSpine(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    LimenSlab& s = *b.s;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    int axis = W >= H ? 0 : 1;
    int C = axis == 0 ? H : W;
    if (C < 7) { lgField(b, n); return; }

    int a0 = axis == 0 ? n.x0 : n.z0, a1 = axis == 0 ? n.x1 : n.z1;
    int c0 = axis == 0 ? n.z0 : n.x0, c1 = axis == 0 ? n.z1 : n.x1;
    int spineW = (b.vault && !n.low && C >= 13 && g[LG_SPAN] > 140) ? 2 : 1;
    int mid = c0 + (C - spineW) / 2;

    // At slab root, snap spine to the row/band transit coordinate if one crosses this slab!
    if (n.depth == 0) {
        if (axis == 0) {
            int tz = lmRowTransitZ(s.rect.j, s.rect.z0, s.rect.z1) - s.rect.z0 - 1;
            if (tz >= c0 + 2 && tz + spineW - 1 <= c1 - 2) mid = tz;
        } else {
            int tx = lmBandTransitX(s.rect.x0 + 1, s.rect.x1) - s.rect.x0 - 1;
            if (tx >= c0 + 2 && tx + spineW - 1 <= c1 - 2) mid = tx;
        }
    } else if (g[LG_ORDER] < 130 && C >= 10) {
        int drift = lgR(n, 91u, 3) - 1;
        if (mid + drift >= c0 + 2 && mid + drift + spineW - 1 <= c1 - 2) mid += drift;
    }

    int path = lgPathTag(n);
    bool colonnade = (g[LG_LATTICE] > 135 && g[LG_CELLS] < 130);
    int pierStep = 3 + lgR(n, 92u, 3);

    // Flank walls (or colonnade piers) on both sides of the axial spine
    int fLo = mid - 1, fHi = mid + spineW;
    for (int a = a0; a <= a1; a++) {
        bool openCol = colonnade && ((a - a0) % pierStep != 0);
        lgFillA(b, axis, a, fLo, a, fLo, openCol ? path : LT_WALL);
        lgFillA(b, axis, a, fHi, a, fHi, openCol ? path : LT_WALL);
    }
    lgFillA(b, axis, a0, mid, a1, mid + spineW - 1, path);

    // Periodic portals along walled flanks into the two side wings
    if (!colonnade) {
        int stride = 5 + lgR(n, 93u, 5);
        for (int a = a0 + 2; a <= a1 - 2; a += stride) {
            lgFillA(b, axis, a, fLo, a, fLo, path);
            int aOpp = (g[LG_ORDER] >= 110) ? a : (a + 1 <= a1 - 1 ? a + 1 : a);
            lgFillA(b, axis, aOpp, fHi, aOpp, fHi, path);
        }
    }

    unsigned sideHiBit = (axis == 0) ? LG_S : LG_E;
    unsigned sideLoBit = (axis == 0) ? LG_N : LG_W;
    if (fLo - 1 >= c0) {
        unsigned w0 = (n.walled & ~sideHiBit) | (colonnade ? 0u : sideHiBit);
        LgNode cA = (axis == 0) ? lgChild(n, a0, c0, a1, fLo - 1, 11u, w0, 1)
                                : lgChild(n, c0, a0, fLo - 1, a1, 11u, w0, 1);
        lgGrow(b, cA);
    }
    if (fHi + 1 <= c1) {
        unsigned w1 = (n.walled & ~sideLoBit) | (colonnade ? 0u : sideLoBit);
        u32 kB = (g[LG_ORDER] >= 165 && (fLo - c0) == (c1 - fHi)) ? 11u : 12u;
        LgNode cB = (axis == 0) ? lgChild(n, a0, fHi + 1, a1, c1, kB, w1, 1)
                                : lgChild(n, fHi + 1, a0, c1, a1, kB, w1, 1);
        lgGrow(b, cB);
    }
}

// ---------------------------------------------------------------- split
inline void lgSplit(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    int axis = W >= H ? 0 : 1;
    if (W * 4 < H * 5 && H * 4 < W * 5) axis = lgR(n, 40u, 2);
    else if (lgR(n, 41u, 256) < (255 - g[LG_ORDER]) / 8) axis ^= 1;
    if ((axis == 0 ? W : H) < 7) axis ^= 1;
    int L = axis == 0 ? W : H, C = axis == 0 ? H : W;
    int a0 = axis == 0 ? n.x0 : n.z0, c0 = axis == 0 ? n.z0 : n.x0, c1 = c0 + C - 1;
    bool rooms = !b.vault || n.low;

    enum { SEP_NONE = 0, SEP_WALL = 1, SEP_PATH = 2, SEP_PIERS = 3 };
    int sw[4] = {
        25 + (255 - g[LG_MASS]) / 4,
        35 + g[LG_MASS] / 3 + (rooms ? g[LG_CELLS] / 4 : 0),
        35 + (rooms ? g[LG_CELLS] / 2 : g[LG_SPAN] / 3),
        20 + g[LG_LATTICE] / 2
    };
    int stot = sw[0] + sw[1] + sw[2] + sw[3], sroll = lgR(n, 42u, stot), sep = 0;
    while (sep < 3 && sroll >= sw[sep]) sroll -= sw[sep++];
    int sepW = (sep == SEP_NONE) ? 0 : ((sep == SEP_PATH && !rooms && C >= 10 && lgR(n, 43u, 3) == 0) ? 2 : 1);

    bool regular = lgR(n, 44u, 256) < g[LG_ORDER];
    int k = 2, bay = 3;
    if (regular) {
        k = (L + sepW) / (lgGrain(n) + sepW);
        if (k < 2) k = 2;
        if (k > 6) k = 6;
        while (k > 2 && (L - (k - 1) * sepW) / k < 3) k--;
        bay = (L - (k - 1) * sepW) / k;
    } else {
        bay = L * (30 + lgR(n, 45u, 41)) / 100;
        if (bay > L - sepW - 3) bay = L - sepW - 3;
        if (bay < 3) bay = 3;
    }
    // In an orderly row every bay is the same node; decay lets one of them break the rhythm.
    int odd = (regular && lgR(n, 46u, 512) < g[LG_DECAY]) ? lgR(n, 47u, k) : -1;
    unsigned lowBit = axis == 0 ? LG_W : LG_N, highBit = axis == 0 ? LG_E : LG_S;
    int piersP = 3 + lgR(n, 48u, 4), piersSolid = 1 + g[LG_MASS] * (piersP - 2) / 256;
    int pos = a0;
    for (int q = 0; q < k; q++) {
        int size = (q == k - 1) ? (a0 + L - pos) : bay;
        if (size < 1) break;
        unsigned walled = n.walled & ~(lowBit | highBit);
        if (q == 0 ? (n.walled & lowBit) != 0 : sep == SEP_WALL) walled |= lowBit;
        if (q == k - 1 ? (n.walled & highBit) != 0 : sep == SEP_WALL) walled |= highBit;
        u32 key = regular ? (q == odd ? 900u + (u32)q : 7u) : 100u + (u32)q;
        LgNode c = axis == 0 ? lgChild(n, pos, c0, pos + size - 1, c1, key, walled, 1)
                             : lgChild(n, c0, pos, c1, pos + size - 1, key, walled, 1);
        lgGrow(b, c);
        pos += size;
        if (q == k - 1 || sepW == 0) continue;
        if (sep == SEP_PATH) {
            lgFillA(b, axis, pos, c0, pos + sepW - 1, c1, lgPathTag(n));
        } else if (sep == SEP_PIERS) {
            for (int c = c0; c <= c1; c++)
                lgFillA(b, axis, pos, c, pos, c, ((c - c0 + 1) % piersP) < piersSolid ? LT_WALL : lgFloorTag(n));
        } else {
            lgFillA(b, axis, pos, c0, pos, c1, LT_WALL);
            int nd = 1 + C / 12;
            for (int d = 0; d < nd; d++) {
                int lo = c0 + C * d / nd, hi = c0 + C * (d + 1) / nd;
                int c = lo + lgR(n, 49u + (u32)d * 8u + (regular ? 0u : (u32)q * 64u), hi - lo);
                lgFillA(b, axis, pos, c, pos, c, lgPathTag(n));
            }
        }
        pos += sepW;
    }
}

// ---------------------------------------------------------------- ring
inline void lgRing(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1, A = W < H ? W : H;
    int path = lgPathTag(n);
    bool rooms = !b.vault || n.low;
    if (rooms && A >= 11 && lgR(n, 50u, 256) < 40 + g[LG_CELLS]) {
        int d = 2 + lgR(n, 51u, 2), roomW = 2 + lgR(n, 52u, 2) + (g[LG_GRAIN] > 170 ? 1 : 0);
        int oW = (n.walled & LG_W) ? 0 : 1, oE = (n.walled & LG_E) ? 0 : 1;
        int oN = (n.walled & LG_N) ? 0 : 1, oS = (n.walled & LG_S) ? 0 : 1;
        lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
        int cx0 = n.x0 + oW + d + 1, cx1 = n.x1 - oE - d - 1, cz0 = n.z0 + oN + d + 1, cz1 = n.z1 - oS - d - 1;
        if (cx1 - cx0 >= 3 && cz1 - cz0 >= 3) {
            lgFill(b, cx0, cz0, cx1, cz0, path); lgFill(b, cx0, cz1, cx1, cz1, path);
            lgFill(b, cx0, cz0, cx0, cz1, path); lgFill(b, cx1, cz0, cx1, cz1, path);
            lgRoomRow(b, n, 0, cx0 + 1, cx1 - 1, n.z0 + oN, cz0 - 2, cz0 - 1, roomW, path);
            lgRoomRow(b, n, 0, cx0 + 1, cx1 - 1, cz1 + 2, n.z1 - oS, cz1 + 1, roomW, path);
            lgRoomRow(b, n, 1, cz0 + 1, cz1 - 1, n.x0 + oW, cx0 - 2, cx0 - 1, roomW, path);
            lgRoomRow(b, n, 1, cz0 + 1, cz1 - 1, cx1 + 2, n.x1 - oE, cx1 + 1, roomW, path);
            if (oW) lgFill(b, n.x0, cz0, cx0, cz0, path);
            if (oE) lgFill(b, cx1, cz1, n.x1, cz1, path);
            if (oN) lgFill(b, cx1, n.z0, cx1, cz0, path);
            if (oS) lgFill(b, cx0, cz1, cx0, n.z1, path);
            if (cx1 - cx0 >= 4 && cz1 - cz0 >= 4) lgGrow(b, lgChild(n, cx0 + 1, cz0 + 1, cx1 - 1, cz1 - 1, 5u, 0u, 1));
            return;
        }
    }
    int wd = (A >= 14 && lgR(n, 53u, 3) == 0) ? 2 : 1;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, path);
    if (!n.low && A >= 11 && g[LG_ORDER] >= 90 && lgR(n, 54u, 256) < 96) {
        lgChamferCorners(b, n.x0, n.z0, n.x1, n.z1, 2);
    }
    lgGrow(b, lgChild(n, n.x0 + wd, n.z0 + wd, n.x1 - wd, n.z1 - wd, 6u, 0u, 1));
}

// ---------------------------------------------------------------- sink
inline void lgSink(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1, A = W < H ? W : H;
    int rim = 1 + lgR(n, 70u, 2);
    if (A < 8) rim = 1;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_HALL);
    int px0 = n.x0 + rim, pz0 = n.z0 + rim, px1 = n.x1 - rim, pz1 = n.z1 - rim;
    int pw = px1 - px0 + 1, ph = pz1 - pz0 + 1;
    if (pw < 2 || ph < 2) return;
    // Lower 2-3 storey concourses prefer dry tiled basins; deep abysses prefer tall vaults.
    int abyssBias = (b.s->ceilTiers <= 3) ? (g[LG_ABYSS] / 2) : g[LG_ABYSS];
    int pit = lgR(n, 71u, 256) < abyssBias ? LT_VOID : LT_BASIN;
    lgFill(b, px0, pz0, px1, pz1, pit);

    bool island = false;
    int icx = 0, icz = 0;
    if (pw >= 9 && ph >= 9 && lgR(n, 72u, 256) < 40 + g[LG_SPAN] / 3 + g[LG_SHELL] / 4) {
        int iw = 3 + lgR(n, 73u, pw - 6), ih = 3 + lgR(n, 74u, ph - 6);
        if (iw > pw * 2 / 3) iw = pw * 2 / 3;
        if (ih > ph * 2 / 3) ih = ph * 2 / 3;
        if (iw > pw - 4) iw = pw - 4;
        if (ih > ph - 4) ih = ph - 4;
        int ix0 = px0 + 2 + lgR(n, 75u, pw - 3 - iw), iz0 = pz0 + 2 + lgR(n, 76u, ph - 3 - ih);
        lgGrow(b, lgChild(n, ix0, iz0, ix0 + iw - 1, iz0 + ih - 1, 3u, 0u, 2));
        island = true; icx = ix0 + iw / 2; icz = iz0 + ih / 2;
    }

    int spans = g[LG_SPAN] * (2 + (pw > ph ? pw : ph) / 6) / 256;
    if (spans > 6) spans = 6;
    if (island && spans == 0) spans = 1;
    for (int q = 0; q < spans; q++) {
        u32 sh = lmHash(n.h, (u32)q, 5u, 0x5350u);
        int axis = (int)(sh & 1u);
        int a0 = axis == 0 ? px0 : pz0, len = axis == 0 ? pw : ph;
        int c0 = axis == 0 ? pz0 : px0, cross = axis == 0 ? ph : pw;
        int c = c0 + (int)((sh >> 4) % (u32)cross);
        if (island && q == 0) c = axis == 0 ? icz : icx;
        int mode = (int)((sh >> 12) % 256u);
        bool wide = pit == LT_VOID && cross >= 8 && c + 1 < c0 + cross && ((sh >> 28) & 1u);
        if (mode < g[LG_DECAY] / 2 && !(island && q == 0)) {
            int l = 2 + (int)((sh >> 20) % (u32)(len / 2 + 1));
            if (l > len - 2) l = len - 2;
            if ((sh >> 2) & 1u) lgDeckA(b, axis, a0, a0 + l - 1, c);
            else lgDeckA(b, axis, a0 + len - l, a0 + len - 1, c);
        } else if (mode < g[LG_DECAY] / 2 + (255 - g[LG_ORDER]) / 4 && len >= 8 && cross >= 6) {
            int mid = a0 + len / 2 + (int)((sh >> 20) % 3u) - 1;
            int c2 = c + (((sh >> 24) & 1u) ? 1 : -1) * (2 + (int)((sh >> 25) % 2u));
            if (c2 < c0) c2 = c0;
            if (c2 > c0 + cross - 1) c2 = c0 + cross - 1;
            lgDeckA(b, axis, a0, mid, c);
            lgDeckA(b, axis, mid, a0 + len - 1, c2);
            for (int cc = (c < c2 ? c : c2); cc <= (c > c2 ? c : c2); cc++) lgDeckA(b, axis, mid, mid, cc);
        } else {
            lgDeckA(b, axis, a0, a0 + len - 1, c);
            if (wide) lgDeckA(b, axis, a0, a0 + len - 1, c + 1);
        }
    }

    if (lgR(n, 77u, 256) < g[LG_LATTICE] / 2) {
        int pitch = 4 + lgR(n, 78u, 4);
        for (int x = px0 + 2; x <= px1 - 2; x += pitch) {
            for (int z = pz0 + 2; z <= pz1 - 2; z += pitch) {
                if (b.s->tag[x][z] != pit || lmPitDegreeLocal(*b.s, x, z) < 4) continue;
                if ((int)(lmHash(n.h, (u32)x, (u32)z, 0x4D4Fu) % 256u) < g[LG_DECAY] / 3) continue;
                lgSet(b, x, z, LT_WALL);
            }
        }
    }
}
