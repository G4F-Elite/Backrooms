#pragma once
// LIMEN core: seed, hashing, slab partition.
// Integer-only, no game dependencies: identical bytes on MinGW and Linux.
//
// LIMEN has no list of rooms, buildings or worlds. Layer order (each layer reads
// only the layers above it, never a neighbour's interior):
//   hash -> slab partition                              limen_core.h
//   -> gene fields (continuous, per slab)               limen_genome.h
//   -> seam contracts between slabs                     limen_seam.h
//   -> space grammar: operators rewrite regions         limen_grammar.h
//   -> repair: connectivity, sealing, anomaly sites     limen_build.h
//   -> lamps, pillars, world queries                    limen_world.h

typedef unsigned int u32;

struct LimenConfig {
    u32 seed;
    int level;
    int rowPitch;
    int colPitch;
    bool dnaReady;
};
inline LimenConfig gLimen = {1u, 0, 30, 36, false};

inline u32 lmLvlHash(u32 seed, int level, u32 ch) {
    u32 u = (u32)level;
    u32 h = seed * 0x9E3779B1u ^ (u * 0x85EBCA6Bu + 0x27D4EB2Fu);
    h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15;
    h ^= ch * 0xC2B2AE35u + 0x165667B1u;
    h *= 0x846CA68Bu; h ^= h >> 16;
    return h;
}

inline u32 lmHash(u32 a, u32 b, u32 c, u32 salt) {
    u32 h = gLimen.seed * 0x9E3779B1u + (u32)gLimen.level * 0x632BE5ABu + salt * 0x85EBCA6Bu;
    h ^= a + 0x7F4A7C15u; h *= 0x2C1B3C6Du; h ^= h >> 15;
    h ^= b + 0x165667B1u; h *= 0x297A2D39u; h ^= h >> 13;
    h ^= c + 0xD3A2646Cu; h *= 0x85EBCA6Bu; h ^= h >> 16;
    h *= 0xC2B2AE35u; h ^= h >> 15;
    return h;
}

// C++ division truncates toward zero; the world has negative coordinates.
inline int lmFloorDiv(int a, int b) {
    int q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

inline int lmMod(int a, int b) { return a - lmFloorDiv(a, b) * b; }

// ---------------------------------------------------------------- partition
// "Brick masonry": rows of slabs with shifted joints. Row j starts at rowZ(j),
// slab i of row j starts at colX(j, i). Finding the slab of any cell is O(1).
// A slab owns the wall line on its low sides (x0 column, z0 row); its interior
// is [x0 + 1, x1) x [z0 + 1, z1).

inline int lmRowZ(int j) {
    int p = gLimen.rowPitch;
    return j * p + (int)(lmHash((u32)j, 0u, 0u, 0x524Fu) % (u32)(p / 2));
}

inline int lmColX(int j, int i) {
    int p = gLimen.colPitch;
    return i * p + (int)(lmHash((u32)j, (u32)i, 0u, 0x434Fu) % (u32)(p / 2));
}

inline int lmRowOf(int z) {
    int j = lmFloorDiv(z, gLimen.rowPitch);
    return (z >= lmRowZ(j)) ? j : j - 1;
}

inline int lmColOf(int j, int x) {
    int i = lmFloorDiv(x, gLimen.colPitch);
    return (x >= lmColX(j, i)) ? i : i - 1;
}

struct LimenRect { int j, i, x0, z0, x1, z1; };

inline LimenRect lmSlabRect(int j, int i) {
    LimenRect r;
    r.j = j; r.i = i;
    r.x0 = lmColX(j, i); r.x1 = lmColX(j, i + 1);
    r.z0 = lmRowZ(j); r.z1 = lmRowZ(j + 1);
    return r;
}

inline LimenRect lmSlabAt(int wx, int wz) {
    int j = lmRowOf(wz);
    return lmSlabRect(j, lmColOf(j, wx));
}

// Distance from the origin slab in slabs; the origin slab is an island of norm.
inline int lmDepth(int j, int i) {
    int aj = j < 0 ? -j : j, ai = i < 0 ? -i : i;
    return aj > ai ? aj : ai;
}
