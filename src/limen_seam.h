#pragma once
// LIMEN seam contracts. What is open in the wall line between two slabs depends only
// on the seam segment and on the genes of the two slabs (cheap pure functions), so both
// neighbours agree on it without reading each other's interiors.
#include "limen_genome.h"

const int LM_MAX_PORTS = 5;

// Standard port distribution along a seam interval [a, b).
inline int lmPortList(int a, int b, u32 k1, u32 k2, u32 salt, int* out) {
    int len = b - a;
    if (len < 1) return 0;
    int n = 1 + len / 14;
    if (n > LM_MAX_PORTS - 1) n = LM_MAX_PORTS - 1;
    for (int k = 0; k < n; k++) {
        int lo = a + len * k / n, hi = a + len * (k + 1) / n;
        out[k] = lo + (int)(lmHash(k1, k2, (u32)k, salt) % (u32)(hi - lo));
    }
    return n;
}

inline int lmAppendUniquePort(int* out, int n, int v) {
    for (int k = 0; k < n; k++) if (out[k] == v) return n;
    if (n < LM_MAX_PORTS) out[n++] = v;
    return n;
}

// Row-wide east-west transit Z coordinate: depends only on row j, so every slab in row j
// shares an aligned port along one straight multi-slab avenue.
inline int lmRowTransitZ(int j, int z0, int z1) {
    if (z1 - z0 < 12) return -999999;
    if ((int)(lmHash((u32)j, 0u, 0u, 0x5452u) % 1000u) >= gLimenDNA.transitPermille) return -999999;
    return z0 + 3 + (int)(lmHash((u32)j, 1u, 0u, 0x5452u) % (u32)(z1 - z0 - 5));
}

// Meridian north-south transit X coordinate: depends only on coarse world X band, so
// consecutive shifted rows align along 250-450 m north-south corridors/viaducts.
inline int lmBandTransitX(int a, int b) {
    if (b - a < 8) return -999999;
    int band = lmFloorDiv((a + b) / 2, 48);
    if ((int)(lmHash((u32)band, 0u, 0u, 0x5458u) % 1000u) >= gLimenDNA.transitPermille) return -999999;
    int tx = band * 48 + 8 + (int)(lmHash((u32)band, 1u, 0u, 0x5458u) % 32u);
    return (tx >= a && tx < b) ? tx : -999999;
}

// Two slabs that both want to be open toward their neighbours and stand under the same
// kind of ceiling share a "through" colonnade seam, so space can bleed across 300+ m
// without a hard partition wall.
inline bool lmSeamIsThrough(int jA, int iA, int jB, int iB) {
    if ((lmSlabTiers(jA, iA) > 1) != (lmSlabTiers(jB, iB) > 1)) return false;
    return lmGene(jA, iA, LG_EDGE) + lmGene(jB, iB, LG_EDGE) > gLimenDNA.throughEdgeThresh;
}

inline bool lmIsPier(int coord) { return lmMod(coord, 6) >= 2; }

inline int lmWestPorts(const LimenRect& s, int* outZ) {
    int n = lmPortList(s.z0 + 1, s.z1, (u32)s.j, (u32)s.i, 0x5645u, outZ);
    int tz = lmRowTransitZ(s.j, s.z0, s.z1);
    if (tz > s.z0 && tz < s.z1) n = lmAppendUniquePort(outZ, n, tz);
    return n;
}

inline int lmNorthPorts(const LimenRect& s, const LimenRect& nb, int* outX) {
    int a = (s.x0 > nb.x0 ? s.x0 : nb.x0) + 1;
    int b = (s.x1 < nb.x1 ? s.x1 : nb.x1);
    int n = lmPortList(a, b, (u32)s.j, (u32)a, 0x484Fu, outX);
    int tx = lmBandTransitX(a, b);
    if (tx >= a && tx < b) n = lmAppendUniquePort(outX, n, tx);
    return n;
}

// Is the wall-line cell (wx, wz) owned by slab s an opening?
inline bool lmWallCellOpen(const LimenRect& s, int wx, int wz) {
    if (wx == s.x0 && wz == s.z0) return false;
    int ports[LM_MAX_PORTS];
    if (wx == s.x0) {
        if (lmSeamIsThrough(s.j, s.i, s.j, s.i - 1) && !lmIsPier(wz)) return true;
        int n = lmWestPorts(s, ports);
        for (int k = 0; k < n; k++) if (ports[k] == wz) return true;
        return false;
    }
    LimenRect nb = lmSlabRect(s.j - 1, lmColOf(s.j - 1, wx));
    if (wx == nb.x0) return false;
    if (lmSeamIsThrough(s.j, s.i, nb.j, nb.i) && !lmIsPier(wx)) return true;
    int n = lmNorthPorts(s, nb, ports);
    for (int k = 0; k < n; k++) if (ports[k] == wx) return true;
    return false;
}
