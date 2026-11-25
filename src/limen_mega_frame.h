#pragma once
// Part of limen_mega.h: post-build geometry, visibility, frame slice, quirk uniforms.
#include "limen_world_geometry.h"

// Fast 2D cone/distance culling for cached slab meshes (with generous guard band).
inline bool lmSlabVisibleFrom(const LimenRect& r, const Vec3& camPos, float fwdX, float fwdZ) {
    float x0 = r.x0 * CS, x1 = (r.x1 + 1) * CS;
    float z0 = r.z0 * CS, z1 = (r.z1 + 1) * CS;
    float cx = camPos.x < x0 ? x0 : (camPos.x > x1 ? x1 : camPos.x);
    float cz = camPos.z < z0 ? z0 : (camPos.z > z1 ? z1 : camPos.z);
    float dx = cx - camPos.x, dz = cz - camPos.z;
    float d2 = dx * dx + dz * dz;
    if (d2 < 3600.0f) return true;      // within 60 m: always draw
    if (d2 > 108900.0f) return false;   // beyond 330 m: culled by fog/far plane
    // Check if any corner of the slab AABB is in front of a wide -0.35 half-space
    float xs[2] = {x0, x1}, zs[2] = {z0, z1};
    for (int ix = 0; ix < 2; ix++) {
        for (int iz = 0; iz < 2; iz++) {
            float vx = xs[ix] - camPos.x, vz = zs[iz] - camPos.z;
            if (vx * fwdX + vz * fwdZ > -25.0f) return true;
        }
    }
    return false;
}

// One frame's share of the streaming work, called from the draw hook. Returns false when idle.
// At most one kind of heavy work per frame: furnish a chunk, or model props, or mesh a slab.
inline bool lmRunFrameSlice() {
    bool busy = false;
    { LM_PROF("slice:env"); lmUpdateEnvMap(playerChunkX, playerChunkZ, true); }
    if (lmEnvJobRow >= 0) busy = true;
    if (!gLmStreaming) return busy;
    { LM_PROF("slice:content"); if (lmPumpDeferredContent()) return true; }
    if (gLmChunkPropPending) {
        LM_PROF("slice:props");
        lmSyncChunkPropMeshes(LM_PROPSYNC_SLICE);
        return true;
    }
    LM_PROF("slice:slab");
    while (!gLmPrewarmSlabKeys.empty()) {
        long long skey = gLmPrewarmSlabKeys.back();
        gLmPrewarmSlabKeys.pop_back();
        auto it = gLmSlabMeshes.find(skey);
        if (it != gLmSlabMeshes.end() && it->second.builtSeed == worldSeed && it->second.builtLevel == gCurrentLevel) continue;
        lmEnsureSlabMesh((int)(skey >> 32), (int)(skey & 0xFFFFFFFFull));
        return true;
    }
    return busy;
}

extern "C" __declspec(dllimport) void __stdcall glDepthMask(unsigned char flag);

// Fills the quirk uniform block (8 vec4, see lmQuirkProjGLSL), the sky colours and the row
// count of the retro pixel grid (0 = off) from the level DNA.
inline void lmQuirkUniforms(float uq[32], float skyTop[3], float skyHor[3], float& pixRows) {
    const LimenLevelDNA& D = gLimenDNA;
    for (int k = 0; k < 32; k++) uq[k] = 0.0f;
    for (int c = 0; c < 3; c++) { skyTop[c] = 0.0f; skyHor[c] = 0.0f; }
    pixRows = 0.0f;
    auto rgb = [&](int slot, const unsigned char* c) {
        for (int k = 0; k < 3; k++) uq[slot * 4 + k] = (float)c[k] / 255.0f;
    };
    if (lmHasQuirk(D, LQ_FLOOD)) {
        uq[0] = 1.0f;
        uq[1] = lmFloodLevelY(D);
        uq[2] = (float)lmQuirkColBOf(D, LQ_FLOOD)[0] / 255.0f;
        rgb(1, lmQuirkColAOf(D, LQ_FLOOD));
    }
    if (lmHasQuirk(D, LQ_PARTY))   uq[8] = 1.0f;
    if (lmHasQuirk(D, LQ_GREYBOX)) { uq[9] = 1.0f;  rgb(6, lmQuirkColAOf(D, LQ_GREYBOX)); rgb(7, lmQuirkColBOf(D, LQ_GREYBOX)); }
    if (lmHasQuirk(D, LQ_NEON))    { uq[10] = 1.0f; rgb(6, lmQuirkColAOf(D, LQ_NEON));    rgb(7, lmQuirkColBOf(D, LQ_NEON)); }
    if (lmHasQuirk(D, LQ_LAVA))    { uq[11] = 1.0f + (float)lmLavaKind(D); rgb(6, lmQuirkColAOf(D, LQ_LAVA)); }
    if (lmHasQuirk(D, LQ_BLACKOUT)) { uq[21] = 1.0f; rgb(6, lmQuirkColAOf(D, LQ_BLACKOUT)); }
    if (lmHasQuirk(D, LQ_MONO))    { rgb(3, lmQuirkColAOf(D, LQ_MONO)); uq[15] = 1.0f; }
    if (lmHasQuirk(D, LQ_WARP)) {
        int var = lmQuirkVarOf(D, LQ_WARP);
        float t = (float)(var >> 2) / 63.0f;
        switch (lmWarpKind(D)) {
            case LM_WARP_BOWL:  uq[16] = 0.0018f + 0.0018f * t; break;
            case LM_WARP_GLOBE: uq[16] = -(0.0018f + 0.0018f * t); break;
            case LM_WARP_LEAN:
                uq[17] = 0.15f + 0.13f * t;
                uq[27] = (float)(lmLvlHash(gLimen.seed, gLimen.level, 930u) % 6283u) * 0.001f;
                break;
            default: uq[18] = ((var & 128) ? 1.0f : -1.0f) * (0.12f + 0.11f * t); break;
        }
    }
    if (lmHasQuirk(D, LQ_DRUNK)) uq[19] = 1.0f;
    if (lmHasQuirk(D, LQ_SKY)) {
        uq[20] = 1.0f + (float)lmSkyKind(D);
        for (int c = 0; c < 3; c++) {
            skyTop[c] = (float)lmQuirkColAOf(D, LQ_SKY)[c] / 255.0f;
            skyHor[c] = (float)lmQuirkColBOf(D, LQ_SKY)[c] / 255.0f;
        }
    }
    if (lmHasQuirk(D, LQ_RETRO)) {
        pixRows = 200.0f + 40.0f * (float)(lmQuirkVarOf(D, LQ_RETRO) % 3);
        uq[22] = 1.0f;
        uq[23] = pixRows * 0.5f;
    }
}
