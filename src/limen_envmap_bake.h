#pragma once
#include "limen_env_occlusion.h"

// Bakes the 128x128 RGBA8 world environment map around chunk (pcx, pcz).
// Blue channel carries continuous `LG_HUE / 255.0f` to smoothly blend all 3 DNA mega poles!
//
// While streaming, a map that only needs to follow the player (or a lamp event) is baked
// `LM_ENV_ROWS_PER_FRAME` rows per frame; the previous texture and origin stay in use until the
// new one is complete. `fromFrame` marks the once-per-frame call that advances such a bake.
const int LM_ENV_ROWS_PER_FRAME = 32;
inline int lmEnvJobRow = -1, lmEnvJobOX = 0, lmEnvJobOZ = 0;
inline unsigned int lmEnvJobSeed = 0, lmEnvJobEpoch = 0;
inline int lmEnvJobLevel = 0;

inline void lmUpdateEnvMap(int pcx, int pcz, bool fromFrame = false) {
    int ox = pcx * CHUNK_SIZE - (LM_ENV_SIZE - CHUNK_SIZE) / 2;
    int oz = pcz * CHUNK_SIZE - (LM_ENV_SIZE - CHUNK_SIZE) / 2;
    if (lmEnvTex != 0 && ox == lmEnvOriginX && oz == lmEnvOriginZ &&
        lmEnvBuiltSeed == worldSeed && lmEnvBuiltLevel == gCurrentLevel && lmEnvBuiltEpoch == gLmSiteEpoch) {
        lmEnvJobRow = -1;
        return;
    }
    // (a jump of more than two chunks is a teleport: the old map covers the wrong place)
    bool progressive = gLmStreaming && lmEnvTex != 0 &&
                       lmEnvBuiltSeed == worldSeed && lmEnvBuiltLevel == gCurrentLevel &&
                       abs(ox - lmEnvOriginX) <= 2 * CHUNK_SIZE && abs(oz - lmEnvOriginZ) <= 2 * CHUNK_SIZE;
    if (progressive && !fromFrame) return;
    if (lmEnvJobRow < 0 || lmEnvJobOX != ox || lmEnvJobOZ != oz || lmEnvJobSeed != worldSeed ||
        lmEnvJobLevel != gCurrentLevel || lmEnvJobEpoch != gLmSiteEpoch) {
        lmEnvJobRow = 0; lmEnvJobOX = ox; lmEnvJobOZ = oz;
        lmEnvJobSeed = worldSeed; lmEnvJobLevel = gCurrentLevel; lmEnvJobEpoch = gLmSiteEpoch;
    }
    int rowBegin = lmEnvJobRow;
    int rowEnd = progressive ? rowBegin + LM_ENV_ROWS_PER_FRAME : LM_ENV_SIZE;
    if (rowEnd > LM_ENV_SIZE) rowEnd = LM_ENV_SIZE;

    static float lightA[LM_ENV_SIZE][LM_ENV_SIZE], lightB[LM_ENV_SIZE][LM_ENV_SIZE];
    static float palA[LM_ENV_SIZE][LM_ENV_SIZE], megaA[LM_ENV_SIZE][LM_ENV_SIZE];
    static LmEnvWalls<LM_ENV_SIZE> walls;
    static unsigned char geometryPenalties[LM_ENV_SIZE][LM_ENV_SIZE];

    for (int z = rowBegin; z < rowEnd; z++) {
        int wz = oz + z;
        walls.at(-1, z) = lmTagAt(ox - 1, wz) == LT_WALL;
        walls.at(LM_ENV_SIZE, z) = lmTagAt(ox + LM_ENV_SIZE, wz) == LT_WALL;
        for (int x = 0; x < LM_ENV_SIZE; x++) {
            int wx = ox + x;
            int t = lmTagAt(wx, wz);
            walls.at(x, z) = t == LT_WALL;
            bool high = lmHighAt(wx, wz);
            LimenRect sr = lmSlabAt(wx, wz);
            const LimenSlab& s = lmSlab(sr.j, sr.i);
            int lamp = (t != LT_WALL) ? lmLampAt(wx, wz) : 0;
            int auxLamp = (lamp == 0 && high && wx != sr.x0 && wz != sr.z0)
                        ? lmAuxVaultLampLocal(s, wx - sr.x0 - 1, wz - sr.z0 - 1) : 0;
            float emit = 0.0f;
            if (lamp == 1) emit = high ? 0.78f : 0.72f;
            else if (auxLamp == 1) emit = (t == LT_VOID) ? 0.68f : 0.82f;
            else if (high && !s.dark && t != LT_WALL) {
                emit = 0.12f;
                if (s.megaForm == MF_CELLBLOCK && wx != sr.x0 && wz != sr.z0) {
                    int lx = wx - sr.x0 - 1, lz = wz - sr.z0 - 1;
                    if (lx <= 4 || lx >= s.w - 5 || lz <= 4 || lz >= s.h - 5)
                        emit = 0.24f + (float)s.genome.g[LG_LIGHT] * 0.36f / 255.0f;
                } else if (s.megaForm == MF_SILO && wx != sr.x0 && wz != sr.z0) {
                    float dx = (float)(wx - sr.x0 - 1 - s.megaOffX) - s.w * 0.5f, dz = (float)(wz - sr.z0 - 1 - s.megaOffZ) - s.h * 0.5f;
                    if (dx * dx + dz * dz < 22.0f) emit = 0.58f;
                } else if (s.megaForm == MF_ZIGGURAT && t == LT_BASIN) {
                    emit = 0.42f;
                } else if (s.megaForm == MF_CANYON && lmPitDegreeLocal(s, wx - sr.x0 - 1, wz - sr.z0 - 1) > 0) {
                    emit = 0.38f;
                } else if (s.megaForm == MF_GROTTO && wx != sr.x0 && wz != sr.z0) {
                    float dx = (float)(wx - sr.x0 - 1) - s.w * 0.5f, dz = (float)(wz - sr.z0 - 1) - s.h * 0.5f;
                    emit = (dx * dx + dz * dz < 14.0f) ? 0.72f : 0.05f;
                } else if (s.megaForm == MF_TENDRILS) {
                    emit = 0.62f;
                } else if (s.megaForm == MF_SPIRES) {
                    emit = 0.42f;
                } else if (s.megaForm == MF_MONOLITH) {
                    emit = 0.34f;
                }
            }
            lightA[z][x] = emit;

            bool inXeno = (wx != sr.x0 && wz != sr.z0) && s.xeno.active &&
                          (s.zone[wx - sr.x0 - 1][wz - sr.z0 - 1] & LZ_XENO) != 0;
            palA[z][x]  = (float)(inXeno ? s.xeno.hue : s.genome.g[LG_HUE]) / 255.0f;
            megaA[z][x] = !high ? 0.0f : (s.dark ? 0.18f : (s.ceilTiers == 2 ? 0.56f : (s.ceilTiers == 3 ? 0.78f : 1.0f)));

            unsigned char penalties = 0;
            if (t != LT_WALL) {
                unsigned short f = lmFlagAt(wx, wz);
                penalties = ((f & (LF_NARROW_X | LF_NARROW_Z)) != 0) + ((f & LF_CRAMPED) != 0);
            }
            geometryPenalties[z][x] = penalties;
        }
    }
    lmEnvJobRow = rowEnd;
    if (rowEnd < LM_ENV_SIZE) return;   // more rows next frame
    lmEnvJobRow = -1;
    lmEnvOriginX = ox; lmEnvOriginZ = oz;
    lmEnvBuiltSeed = worldSeed; lmEnvBuiltLevel = gCurrentLevel; lmEnvBuiltEpoch = gLmSiteEpoch;

    for (int x = 0; x < LM_ENV_SIZE; ++x) {
        walls.at(x, -1) = lmTagAt(ox + x, oz - 1) == LT_WALL;
        walls.at(x, LM_ENV_SIZE) = lmTagAt(ox + x, oz + LM_ENV_SIZE) == LT_WALL;
    }
    for (int pass = 0; pass < 3; pass++) {
        for (int z = 0; z < LM_ENV_SIZE; z++) {
            for (int x = 0; x < LM_ENV_SIZE; x++) {
                float s = lightA[z][x] * 0.50f +
                          lightA[z][x > 0 ? x - 1 : 0] * 0.25f +
                          lightA[z][x + 1 < LM_ENV_SIZE ? x + 1 : x] * 0.25f;
                lightB[z][x] = walls.at(x, z) ? s * 0.28f : s;
            }
        }
        for (int z = 0; z < LM_ENV_SIZE; z++) {
            int z0 = z > 0 ? z - 1 : 0, z1 = z + 1 < LM_ENV_SIZE ? z + 1 : z;
            for (int x = 0; x < LM_ENV_SIZE; x++) {
                float s = lightB[z][x] * 0.50f + lightB[z0][x] * 0.25f + lightB[z1][x] * 0.25f;
                lightA[z][x] = walls.at(x, z) ? s * 0.28f : s;
            }
        }
    }

    static unsigned char rgba[LM_ENV_SIZE * LM_ENV_SIZE * 4];
    for (int z = 0; z < LM_ENV_SIZE; z++) {
        for (int x = 0; x < LM_ENV_SIZE; x++) {
            float pSum = 0.0f, mSum = 0.0f;
            int cnt = 0;
            for (int dz = -2; dz <= 2; dz++) {
                int zz = z + dz;
                if (zz < 0 || zz >= LM_ENV_SIZE) continue;
                for (int dx = -2; dx <= 2; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= LM_ENV_SIZE) continue;
                    pSum += palA[zz][xx];
                    mSum += megaA[zz][xx];
                    cnt++;
                }
            }
            float rawL = lightA[z][x] * 2.15f;
            float lVal = rawL / (1.0f + rawL * 0.68f);
            size_t o = ((size_t)z * LM_ENV_SIZE + (size_t)x) * 4;
            rgba[o + 0] = (unsigned char)(lVal * 255.0f);
            rgba[o + 1] = walls.ambient(x, z, geometryPenalties[z][x]);
            rgba[o + 2] = (unsigned char)((pSum / (float)cnt) * 255.0f);
            rgba[o + 3] = (unsigned char)((mSum / (float)cnt) * 255.0f);
        }
    }

    glActiveTexture(GL_TEXTURE0 + 5);
    if (lmEnvTex == 0) {
        glGenTextures(1, &lmEnvTex);
        glBindTexture(GL_TEXTURE_2D, lmEnvTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glBindTexture(GL_TEXTURE_2D, lmEnvTex);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LM_ENV_SIZE, LM_ENV_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glActiveTexture(GL_TEXTURE0);
}
