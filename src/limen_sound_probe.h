#pragma once
// LIMEN sound generator. No samples, no list of "room ambiences": everything the world
// sounds like is derived from what the generator built around the listener.
//
//   lmMakeSoundDNA   (seed, level) -> mains frequency, lamp timbre, drone chord, plant pitch
//   lmProbeSound     world + position -> LmSoundScene: measured room (12 rays -> area, volume,
//                    Sabine RT60, room modes, echo walls), floor material under the feet,
//                    nearest lamps with line-of-sight, shafts, basins, machinery field, delirium
//   LmSoundSynth     LmSoundScene -> stereo audio: per-lamp ballast hum, room tone, shaft
//                    draught, air plant, level drone, drips, structural creaks, material
//                    footsteps with wall echoes, and a reverb tuned to the measured RT60
//
// Depends on limen_world.h only (no game code): limen_tool renders it to a WAV and tests it.
// The probe runs on the game thread, the synth on the audio thread; limen_audio.h joins them.
#include <cmath>
#include <cstring>
#include "limen_world.h"

const float LS_SR = 44100.0f;
const float LS_DT = 1.0f / 44100.0f;
const float LS_TAU = 6.28318530718f;
const float LS_SOUND_SPEED = 343.0f;
const int LS_LAMPS = 4;
const int LS_RAYS = 12;
const float LS_RAY_MAX_CELLS = 28.0f;

inline float lsClamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lsLerp(float a, float b, float t) { return a + (b - a) * t; }
inline float lsSmooth(float e0, float e1, float x) {
    float t = lsClamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ------------------------------------------------------------------ level sound DNA
struct LmSoundDNA {
    u32 seed; int level; bool ready;
    float mainsHz;      // 50 or 60 Hz grid, a little off nominal
    float humTilt;      // 0 warm ballast drone .. 1 harsh tube buzz (cool light buzzes more)
    float droneHz;      // root of the building drone
    float ratioA;       // second voice: between a minor third and a tritone above the root
    float ratioB;       // third voice: a mistuned octave
    float plantHz;      // air handler fan fundamental
    float plantBias;    // how mechanical the level's themes are
    float dripBias;     // how wet the level's themes and surfaces are
    float airDull;      // fog density: thick air swallows the top end
};

inline LmSoundDNA lmMakeSoundDNA(const LimenLevelDNA& d, u32 seed, int level) {
    LmSoundDNA s;
    s.seed = seed; s.level = level; s.ready = true;
    auto u = [&](u32 ch) { return (float)(lmLvlHash(seed, level, 0x5A00u + ch) & 0xFFFFu) / 65535.0f; };
    s.mainsHz = ((lmLvlHash(seed, level, 0x5A40u) & 1u) ? 50.0f : 60.0f) + (u(1u) - 0.5f) * 0.5f;
    float cool = ((float)d.lightRGB[2] - (float)d.lightRGB[0]) / 255.0f;
    s.humTilt = lsClamp(0.45f + cool * 1.3f + (u(2u) - 0.5f) * 0.5f, 0.0f, 1.0f);
    s.droneHz = 31.0f + 30.0f * u(3u);
    s.ratioA = 1.18f + 0.34f * u(4u);
    s.ratioB = 1.95f + 0.12f * u(5u);
    s.plantHz = 46.0f + 70.0f * u(6u);
    float tw[LM_THEME_COUNT], sum = 0.0f;
    for (int t = 0; t < LM_THEME_COUNT; t++) { tw[t] = (float)d.themeWeight[t]; sum += tw[t]; }
    for (int t = 0; t < LM_THEME_COUNT; t++) tw[t] /= (sum > 0.0f ? sum : 1.0f);
    // Theme order: office parking maint warehs hotel pool hospit school mall playrm station lab kitchen
    float mech = tw[1] + tw[2] * 1.4f + tw[3] + tw[10] + tw[11] + tw[12] + 0.5f * (tw[0] + tw[6] + tw[8]);
    s.plantBias = lsClamp(0.15f + mech * 1.3f, 0.0f, 1.0f);
    float grime = ((float)d.wallSurf.grime + (float)d.floorSurf.grime) / 510.0f;
    s.dripBias = lsClamp(tw[5] * 2.6f + tw[2] + 0.6f * (tw[12] + tw[1]) + grime * 0.7f + (lmHasQuirk(d, LQ_FLOOD) ? 0.7f : 0.0f), 0.0f, 1.5f);
    s.airDull = (float)d.fogDensity / 255.0f;
    return s;
}

// ------------------------------------------------------------------ scene
struct LmSoundLamp {
    u32 id;             // 0 = empty slot; otherwise a stable identity of the lamp's cell
    float x, z;         // world position, m
    float h;            // height above the listener's ears, m
    float occl;         // 1 in line of sight .. 0.22 behind a wall
    int live;           // 1 burning, 0 dark tube that may still try to strike
};

struct LmSoundScene {
    int valid;
    float px, pz, yaw;
    // The space around the listener
    float ceil, meanR, minR, area, volume, absorb, rt60, open;
    float ray[LS_RAYS];
    float mode[3];                      // room mode frequencies, Hz
    float echoDelay[2], echoGain[2], echoDx[2], echoDz[2];
    // The floor under the feet
    float hard, grit, hollow, wet, metal;
    // Sources
    LmSoundLamp lamp[LS_LAMPS];
    float lampField;                    // diffuse hum of the lattice beyond the nearest tubes
    float unstable;                     // decay: flickering tubes, restless dead ones
    float voidGain, voidDx, voidDz, voidHz;
    float plant, plantHz, plantTilt;
    float drone, droneHz, ratioA, ratioB, warble;
    float dripRate, dripDx, dripDz, dripWet;
    float creakRate, creakSize;
    float mainsHz, humTilt, airDull;
    float bed;                          // how alive the place is: scales the generic room bed
    // Party quirk: dance music from a room that is always behind one more wall
    float party, partyDx, partyDz, partyBpm, partyKey;
};

struct LmSndCell { const LimenSlab* s; int lx, lz, tag; unsigned short flag; unsigned char zone; };

inline LmSndCell lmSndCellAt(int wx, int wz) {
    LmSndCell c;
    LimenRect r = lmSlabAt(wx, wz);
    c.s = nullptr; c.lx = c.lz = 0; c.flag = 0; c.zone = 0;
    if (wx == r.x0 || wz == r.z0) {
        c.tag = lmWallCellOpen(r, wx, wz) ? LT_CORR : LT_WALL;
        return c;
    }
    const LimenSlab& s = lmSlab(r.j, r.i);
    c.s = &s; c.lx = wx - r.x0 - 1; c.lz = wz - r.z0 - 1;
    c.tag = s.tag[c.lx][c.lz]; c.flag = s.flag[c.lx][c.lz]; c.zone = s.zone[c.lx][c.lz];
    return c;
}

inline bool lmSndSolidTag(int t) { return t == LT_WALL || t == LT_VITRINE; }

// Sound crossing from cell (ax, az) into a 4-neighbour: solid mass or an unbroken thin partition.
inline bool lmSndEdgeBlocked(int ax, int az, int bx, int bz) {
    if (lmSndSolidTag(lmTagAt(bx, bz))) return true;
    if (bx != ax) {
        unsigned short f = lmFlagAt(bx > ax ? bx : ax, az);
        if ((f & LF_THIN_W) && !(f & LF_THIN_DOOR_W)) return true;
    }
    if (bz != az) {
        unsigned short f = lmFlagAt(bx, bz > az ? bz : az);
        if ((f & LF_THIN_N) && !(f & LF_THIN_DOOR_N)) return true;
    }
    return false;
}

// Free path from (ox, oz) along (dx, dz), all in cells. Returns maxCells if nothing was hit.
inline float lmSndRay(float ox, float oz, float dx, float dz, float maxCells) {
    const float step = 0.25f;
    int cx = (int)floorf(ox), cz = (int)floorf(oz);
    for (float t = step; t <= maxCells; t += step) {
        int nx = (int)floorf(ox + dx * t), nz = (int)floorf(oz + dz * t);
        if (nx == cx && nz == cz) continue;
        bool hit;
        if (nx != cx && nz != cz) hit = lmSndEdgeBlocked(cx, cz, nx, cz) || lmSndEdgeBlocked(nx, cz, nx, nz);
        else hit = lmSndEdgeBlocked(cx, cz, nx, nz);
        if (hit) return t - step * 0.5f;
        cx = nx; cz = nz;
    }
    return maxCells;
}

inline LmSoundDNA gLmSoundDNA = {0u, 0, false, 60.0f, 0.5f, 45.0f, 1.3f, 2.0f, 80.0f, 0.5f, 0.3f, 0.3f};
inline LmSoundDNA gLmSoundXenoDNA = gLmSoundDNA;

inline const LmSoundDNA& lmSoundDNA() {
    if (!gLmSoundDNA.ready || gLmSoundDNA.seed != gLimen.seed || gLmSoundDNA.level != gLimen.level)
        gLmSoundDNA = lmMakeSoundDNA(gLimenDNA, gLimen.seed, gLimen.level);
    return gLmSoundDNA;
}

// A pure function of (seed, level, position): what this exact spot of the world sounds like.
inline void lmProbeSound(float px, float pz, float cs, LmSoundScene& sc) {
    memset(&sc, 0, sizeof(sc));
    const LmSoundDNA& sd = lmSoundDNA();
    const LimenLevelDNA& dna = gLimenDNA;
    const float WH = 4.5f;
    int wx = (int)floorf(px / cs), wz = (int)floorf(pz / cs);
    float fx = px / cs, fz = pz / cs;
    LmSndCell c = lmSndCellAt(wx, wz);
    LimenRect rect = lmSlabAt(wx, wz);
    const LimenSlab& slab = lmSlab(rect.j, rect.i);
    bool seam = c.s == nullptr;
    bool high = !seam && lmSlabIsVault(slab) && !lmIsLowShellTag(c.tag);
    bool xeno = !seam && (c.zone & LZ_XENO) != 0 && slab.xeno.active;
    float delirium = lmDeliriumAt(wx, wz);
    float decay = (float)slab.genome.g[LG_DECAY] / 255.0f;

    sc.valid = 1;
    sc.px = px; sc.pz = pz;
    sc.mainsHz = sd.mainsHz; sc.humTilt = sd.humTilt; sc.airDull = sd.airDull;
    sc.unstable = decay;

    // ---- floor material under the feet
    int pole = (slab.palette >= 0 && slab.palette < LM_MEGA_POLES) ? slab.palette : 0;
    const LmSurfDNA& floorSurf = xeno ? slab.xeno.floorSurf : (high ? dna.megaPoles[pole] : dna.floorSurf);
    const LmSurfDNA& wallSurf = xeno ? slab.xeno.wallSurf : (high ? dna.megaPoles[pole] : dna.wallSurf);
    float sheen = (float)floorSurf.sheen / 255.0f, grain = (float)floorSurf.grain / 255.0f;
    float grime = (float)floorSurf.grime / 255.0f;
    sc.hard = sheen;
    sc.grit = lsClamp(grain * (1.0f - 0.5f * sheen) + grime * 0.3f, 0.0f, 1.0f);
    sc.wet = lsClamp((grime - 0.55f) * 1.2f * sheen, 0.0f, 0.5f);
    if (c.tag == LT_BRIDGE || (c.flag & (LF_OVER_VOID | LF_OVER_BASIN))) {
        sc.hollow = (c.flag & LF_OVER_VOID) ? 1.0f : 0.7f;
        sc.metal = 0.35f + 0.5f * (1.0f - (float)slab.genome.g[LG_MASS] / 255.0f);
        if (sc.hard < 0.55f) sc.hard = 0.55f;
    }
    if (c.tag == LT_BASIN) {
        sc.hard = sc.hard < 0.85f ? 0.85f : sc.hard;
        sc.wet = lsClamp(0.3f + 0.6f * grime, 0.0f, 1.0f);
        if (c.flag & LF_STAIR_DESC) sc.hollow = 0.2f;
    }
    if (delirium > 0.2f) sc.hollow = lsClamp(sc.hollow + (delirium - 0.2f) * 0.6f, 0.0f, 1.0f);
    if (lmHasQuirk(dna, LQ_FLOOD) && c.tag != LT_BRIDGE && (c.flag & (LF_OVER_VOID | LF_OVER_BASIN)) == 0) {
        float depth = lsClamp(lmFloodLevelY(dna) / 0.5f, 0.2f, 1.0f);   // every step is in water
        if (sc.wet < 0.55f + 0.45f * depth) sc.wet = 0.55f + 0.45f * depth;
    }
    if (lmHasQuirk(dna, LQ_PARTY)) {
        // Loud where a slow field says the party is; it comes from up that field's slope.
        const u32 salt = 0x50415254u;
        float n0 = (float)lmSmoothCellNoise255(wx, wz, 11, salt);
        float gx = (float)lmSmoothCellNoise255(wx + 3, wz, 11, salt) - (float)lmSmoothCellNoise255(wx - 3, wz, 11, salt);
        float gz = (float)lmSmoothCellNoise255(wx, wz + 3, 11, salt) - (float)lmSmoothCellNoise255(wx, wz - 3, 11, salt);
        float gl = sqrtf(gx * gx + gz * gz);
        sc.party = 0.22f + 0.78f * lsSmooth(70.0f, 190.0f, n0);
        if (gl > 1.0f) { sc.partyDx = gx / gl; sc.partyDz = gz / gl; }
        u32 hp = lmLvlHash(gLimen.seed, gLimen.level, 0x5A50u);
        sc.partyBpm = 112.0f + (float)(hp % 22u);
        sc.partyKey = 41.2f * powf(2.0f, (float)((hp >> 8) % 12u) / 12.0f);
    }

    // ---- the room: 12 rays -> polar area, perimeter, Sabine reverberation time
    sc.ceil = high ? (float)slab.ceilTiers * WH : lmShellCeilHeight(c.flag, seam ? 1 : slab.ceilTiers, WH, c.zone);
    if (c.tag == LT_BASIN) sc.ceil += 2.85f;
    float sumR = 0.0f, sumR2 = 0.0f, minR = 1e9f;
    float dirX[LS_RAYS], dirZ[LS_RAYS];
    bool finite[LS_RAYS];
    for (int k = 0; k < LS_RAYS; k++) {
        float a = LS_TAU * ((float)k + 0.5f) / (float)LS_RAYS;
        dirX[k] = cosf(a); dirZ[k] = sinf(a);
        float dCells = lmSndRay(fx, fz, dirX[k], dirZ[k], LS_RAY_MAX_CELLS);
        finite[k] = dCells < LS_RAY_MAX_CELLS;
        float d = dCells * cs;
        // Narrowed corridors: the walls stand inside the cell, about 1.25 m to either side.
        if ((c.flag & LF_NARROW_X) && fabsf(dirZ[k]) > 0.3f) d = fminf(d, 1.25f / fabsf(dirZ[k]));
        if ((c.flag & LF_NARROW_Z) && fabsf(dirX[k]) > 0.3f) d = fminf(d, 1.25f / fabsf(dirX[k]));
        if (d < 0.6f) d = 0.6f;
        sc.ray[k] = d;
        sumR += d; sumR2 += d * d;
        if (d < minR) minR = d;
    }
    sc.meanR = sumR / (float)LS_RAYS;
    sc.minR = minR;
    sc.area = 3.14159265f * sumR2 / (float)LS_RAYS;
    sc.volume = sc.area * sc.ceil;
    sc.open = lsClamp(sc.meanR / 60.0f, 0.0f, 1.0f);
    float surfSheen = sheen * 0.5f + (float)wallSurf.sheen / 255.0f * 0.5f;
    sc.absorb = 0.30f - 0.25f * surfSheen + (float)dna.propDensity / 255.0f * 0.05f;
    if (c.flag & LF_CRAMPED) sc.absorb += 0.12f;   // suspended acoustic tile right overhead
    float surface = 2.0f * sc.area + LS_TAU * sc.meanR * sc.ceil;
    float sabins = surface * sc.absorb + 0.012f * sc.volume;   // surfaces + air
    float rt = 0.161f * sc.volume / sabins * (1.0f + (float)dna.reverbBias / 100.0f);
    sc.rt60 = lsClamp(rt, 0.18f, 7.5f);

    // ---- room modes: the narrow dimension, the ceiling, the long dimension
    float sorted[LS_RAYS];
    memcpy(sorted, sc.ray, sizeof(sorted));
    for (int a = 1; a < LS_RAYS; a++) {
        float v = sorted[a]; int b = a - 1;
        while (b >= 0 && sorted[b] > v) { sorted[b + 1] = sorted[b]; b--; }
        sorted[b + 1] = v;
    }
    float width = 2.0f * sorted[2];
    sc.mode[0] = lsClamp(LS_SOUND_SPEED / width, 34.0f, 420.0f);
    sc.mode[1] = lsClamp(LS_SOUND_SPEED / sc.ceil, 30.0f, 300.0f);
    sc.mode[2] = lsClamp(1.5f * LS_SOUND_SPEED / (2.0f * sc.meanR), 28.0f, 300.0f);

    // ---- discrete echoes: the farthest wall actually hit, then the vault overhead or the wall behind
    float refl = lsClamp(1.0f - 2.2f * sc.absorb, 0.15f, 0.95f);
    int far0 = -1;
    for (int k = 0; k < LS_RAYS; k++)
        if (finite[k] && sc.ray[k] >= 9.0f && (far0 < 0 || sc.ray[k] > sc.ray[far0])) far0 = k;
    if (far0 >= 0) {
        float d = sc.ray[far0];
        sc.echoDelay[0] = 2.0f * d / LS_SOUND_SPEED;
        sc.echoGain[0] = refl * 12.0f / (12.0f + d) * 0.85f;
        sc.echoDx[0] = dirX[far0]; sc.echoDz[0] = dirZ[far0];
    }
    if (sc.ceil >= 9.0f) {
        float d = sc.ceil - 1.7f;
        sc.echoDelay[1] = 2.0f * d / LS_SOUND_SPEED;
        sc.echoGain[1] = refl * 12.0f / (12.0f + d) * 0.7f;
    } else if (far0 >= 0) {
        int far1 = -1;
        for (int k = 0; k < LS_RAYS; k++) {
            if (!finite[k] || sc.ray[k] < 9.0f) continue;
            if (dirX[k] * dirX[far0] + dirZ[k] * dirZ[far0] > -0.4f) continue;
            if (far1 < 0 || sc.ray[k] > sc.ray[far1]) far1 = k;
        }
        if (far1 >= 0) {
            float d = sc.ray[far1];
            sc.echoDelay[1] = 2.0f * d / LS_SOUND_SPEED;
            sc.echoGain[1] = refl * 12.0f / (12.0f + d) * 0.6f;
            sc.echoDx[1] = dirX[far1]; sc.echoDz[1] = dirZ[far1];
        }
    }

    // ---- lamps, shafts and basins in the neighbourhood
    float lampScore[LS_LAMPS] = {};
    float nearestLive2 = 1e9f;
    float voidX = 0.0f, voidZ = 0.0f, voidSum = 0.0f, nearVoid2 = 1e9f;
    int nearVoidX = 0, nearVoidZ = 0;
    float basinX = 0.0f, basinZ = 0.0f, basinSum = 0.0f, bridgeSum = 0.0f;
    for (int dz = -6; dz <= 6; dz++) {
        for (int dx = -6; dx <= 6; dx++) {
            int tx = wx + dx, tz = wz + dz;
            float ox = ((float)tx + 0.5f - fx), oz = ((float)tz + 0.5f - fz);   // cells
            float d2 = ox * ox + oz * oz;
            int t = lmTagAt(tx, tz);
            if (t == LT_VOID) {
                float w = 1.0f / (1.0f + d2);
                voidX += ox * w; voidZ += oz * w; voidSum += w;
                if (d2 < nearVoid2) { nearVoid2 = d2; nearVoidX = tx; nearVoidZ = tz; }
            } else if (t == LT_BASIN) {
                float w = 1.0f / (1.0f + d2);
                basinX += ox * w; basinZ += oz * w; basinSum += w;
            } else if (t == LT_BRIDGE) {
                bridgeSum += 1.0f / (1.0f + d2);
            }
            if (dx < -4 || dx > 4 || dz < -4 || dz > 4) continue;
            int lamp = lmLampAt(tx, tz);
            if (lamp == 0) continue;
            float dist = sqrtf(d2);
            float occl = 1.0f;
            if (dist > 0.75f && lmSndRay(fx, fz, ox / dist, oz / dist, dist) < dist - 0.6f) occl = 0.22f;
            if (t == LT_VITRINE) occl *= 0.45f;   // behind glass
            float dm2 = d2 * cs * cs;
            if (lamp == 1 && dm2 < nearestLive2) nearestLive2 = dm2;
            float score = occl / (1.0f + dm2 / 9.0f) * (lamp == 1 ? 1.0f : 0.3f);
            int slot = -1;
            for (int k = 0; k < LS_LAMPS; k++) if (score > lampScore[k]) { slot = k; break; }
            if (slot < 0) continue;
            for (int k = LS_LAMPS - 1; k > slot; k--) { lampScore[k] = lampScore[k - 1]; sc.lamp[k] = sc.lamp[k - 1]; }
            lampScore[slot] = score;
            LmSoundLamp& L = sc.lamp[slot];
            L.id = lmHash((u32)tx, (u32)tz, 3u, 0x534E4Cu) | 1u;
            L.x = ((float)tx + 0.5f) * cs; L.z = ((float)tz + 0.5f) * cs;
            unsigned short lf = lmFlagAt(tx, tz);
            L.h = (lf & LF_CRAMPED) ? 0.9f : (lmHighAt(tx, tz) ? 3.6f : 2.7f);
            L.occl = occl;
            L.live = lamp == 1 ? 1 : 0;
        }
    }
    float humMul = (float)dna.humGain / 128.0f;
    float field = 0.03f;
    if (nearestLive2 < 900.0f) {
        float prox = 1.0f / (1.0f + sqrtf(nearestLive2) * 0.16f);
        field = high ? (0.16f + 0.42f * prox) : (0.28f + 0.98f * prox);
        if (c.flag & LF_CRAMPED) field *= 1.18f;
    }
    sc.lampField = field * humMul;

    if (voidSum > 0.0f) {
        float len = sqrtf(voidX * voidX + voidZ * voidZ);
        float g = lsClamp(voidSum / 2.5f, 0.0f, 1.0f);
        float nd = sqrtf(nearVoid2);
        if (nd > 0.75f) {
            float ox = (float)nearVoidX + 0.5f - fx, oz = (float)nearVoidZ + 0.5f - fz;
            if (lmSndRay(fx, fz, ox / nd, oz / nd, nd) < nd - 0.9f) g *= 0.2f;
        }
        sc.voidGain = g;
        if (len > 1e-4f) { sc.voidDx = voidX / len * lsClamp(len * 3.0f, 0.0f, 1.0f); sc.voidDz = voidZ / len * lsClamp(len * 3.0f, 0.0f, 1.0f); }
        // Every shaft has its own pitch: how deep the slab's abyss gene says it falls.
        sc.voidHz = 62.0f + 0.5f * (255.0f - (float)slab.genome.g[LG_ABYSS]) + (float)(lmHash((u32)rect.j, (u32)rect.i, 5u, 0x534E56u) % 40u);
    } else if (!seam && (c.zone & LZ_HALO) != 0 && slab.haloDir >= 0) {
        // Foreshadowing Halo: bleed distant subterranean draft & resonance from the adjacent megastructure!
        sc.voidGain = 0.28f;
        sc.voidDx = (slab.haloDir == 0) ? -0.8f : ((slab.haloDir == 1) ? 0.8f : 0.0f);
        sc.voidDz = (slab.haloDir == 2) ? -0.8f : ((slab.haloDir == 3) ? 0.8f : 0.0f);
        sc.voidHz = 56.0f + (float)(lmHash((u32)rect.j, (u32)rect.i, 6u, 0x534E56u) % 28u);
    }

    // ---- air plant behind the walls: a slow field you walk into and out of
    float plantField = lsSmooth(118.0f, 215.0f, (float)lmSmoothCellNoise255(wx, wz, 11, 0x534E44u));
    float mass = (float)slab.genome.g[LG_MASS] / 255.0f;
    sc.plant = plantField * (0.2f + 0.8f * sd.plantBias) * (0.5f + 0.5f * mass)
             * ((c.flag & LF_CRAMPED) ? 1.5f : 1.0f) * (high ? 0.45f : 1.0f);
    if (sc.plant > 1.2f) sc.plant = 1.2f;
    sc.plantHz = sd.plantHz;
    sc.plantTilt = (float)slab.genome.g[LG_USE] / 255.0f;

    // ---- level drone: louder away from the origin, in the dark and in drunken pockets
    float depthN = lsClamp((float)lmDepth(rect.j, rect.i) / 6.0f, 0.0f, 1.0f);
    sc.drone = lsClamp(0.16f + 0.26f * depthN + (slab.dark ? 0.32f : 0.0f) + 0.5f * delirium, 0.0f, 1.0f);
    sc.droneHz = sd.droneHz; sc.ratioA = sd.ratioA; sc.ratioB = sd.ratioB;
    sc.warble = delirium;
    if (xeno) {
        // Inside a xenograft the other level's building is heard: its drone, its plant.
        if (!gLmSoundXenoDNA.ready || gLmSoundXenoDNA.seed != gLimen.seed || gLmSoundXenoDNA.level != slab.xeno.foreignLevel)
            gLmSoundXenoDNA = lmMakeSoundDNA(lmMakeLevelDNA(gLimen.seed, slab.xeno.foreignLevel), gLimen.seed, slab.xeno.foreignLevel);
        sc.droneHz = gLmSoundXenoDNA.droneHz; sc.ratioA = gLmSoundXenoDNA.ratioA; sc.ratioB = gLmSoundXenoDNA.ratioB;
        sc.plantHz = gLmSoundXenoDNA.plantHz;
        sc.drone = lsClamp(sc.drone + 0.2f, 0.0f, 1.0f);
    }

    // ---- water: basins drip, grimy pockets drip
    float basinProx = lsClamp(basinSum / 2.5f, 0.0f, 1.0f);
    float dripPocket = lsSmooth(150.0f, 225.0f, (float)lmSmoothCellNoise255(wx, wz, 7, 0x534E57u));
    sc.dripRate = lsClamp(basinProx * 0.9f + dripPocket * sd.dripBias * (0.35f + 0.65f * decay) * 0.8f, 0.0f, 1.6f);
    sc.dripWet = lsClamp(basinProx + sc.wet + 0.4f * sd.dripBias, 0.0f, 1.0f);
    if (basinSum > 0.0f) {
        float len = sqrtf(basinX * basinX + basinZ * basinZ);
        if (len > 1e-4f) { sc.dripDx = basinX / len * lsClamp(len * 3.0f, 0.0f, 1.0f); sc.dripDz = basinZ / len * lsClamp(len * 3.0f, 0.0f, 1.0f); }
    }

    // ---- structure: tall vaults and decks tick and groan as they settle
    float tiers = high ? (float)(slab.ceilTiers > 12 ? 12 : slab.ceilTiers) / 12.0f : 0.0f;
    sc.creakRate = tiers * 0.10f + lsClamp(bridgeSum, 0.0f, 1.0f) * 0.07f + decay * 0.025f;
    sc.creakSize = lsClamp(tiers + 0.3f * sc.hollow, 0.0f, 1.0f);

    sc.bed = slab.dark ? 0.5f : (high ? 0.8f : 1.0f);

    // Non-manmade volumes: acoustic character follows the slab's continuous drives, never a named form.
    //   rawPhenom 0..55 architecture, 56..145 hybrid, 146..255 pure phenomenon (stone, mineral, organic)
    //   skyDrive  0..119 sealed or pierced roof, 120..255 roofless sky
    // Enclosed phenomenon is heavy and long-resonant; open phenomenon is airy, windy and quick to decay.
    // A mottled mineral field (smooth cell noise) varies hardness and grit across the floor.
    float ph = lsSmooth(56.0f, 146.0f, (float)slab.rawPhenom);
    float sk = lsSmooth(96.0f, 176.0f, (float)slab.skyDrive);
    if (!seam && ph > 0.0f) {
        auto lsMix = [](float a, float b, float t) { return lsLerp(a, b, lsClamp(t, 0.0f, 1.0f)); };
        float open = ph * sk;                       // open phenomenon: wind, altitude, thin air
        float mass = ph * (1.0f - sk);              // enclosed phenomenon: heavy stone, long resonance
        float mottle = (float)lmSmoothCellNoise255(wx, wz, 9, 0x4D4F54u) / 255.0f;
        sc.lampField = lsMix(sc.lampField, 0.0f, ph);
        sc.plant     = lsMix(sc.plant, 0.0f, ph);
        sc.metal     = lsMix(sc.metal, 0.0f, ph);
        sc.hard      = lsMix(lsMix(sc.hard, 0.22f + 0.64f * mottle, ph), 0.14f + 0.20f * mottle, open);
        sc.grit      = lsMix(sc.grit, 0.40f + 0.50f * (1.0f - mottle), ph);
        sc.wet       = lsMix(sc.wet, 0.12f + 0.14f * mottle, mass);
        sc.rt60      = lsClamp(sc.rt60 * (1.0f + 0.30f * mass) * (1.0f - 0.62f * open) + 0.90f * mass, 0.32f, 7.5f);
        sc.voidGain  = lsClamp(lsMix(sc.voidGain, 0.86f, open) + 0.38f * mass, 0.0f, 1.0f);
        sc.voidHz    = lsMix(sc.voidHz, 54.0f + 96.0f * sk, ph);   // cavern ~54 Hz up to altitude wind ~150 Hz
        sc.drone     = lsClamp(sc.drone * (1.0f - 0.45f * open) + 0.22f * mass, 0.0f, 1.0f);
        sc.creakRate *= lsMix(1.0f, 0.12f, mass);
        sc.dripRate  = lsClamp(sc.dripRate + 0.65f * mass * mottle, 0.0f, 1.6f);
        sc.dripWet   = lsMix(sc.dripWet, 0.85f, mass);
        sc.hollow    = lsMix(sc.hollow, 0.38f, open);
    }
}
