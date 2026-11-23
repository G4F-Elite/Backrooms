#pragma once
// limen_tool: the sound generator without the game.
//   limen_tool sound <seed> <level> <out.wav> [seconds per place]
//       walks a listener through every kind of place the level has and writes what is heard
//   runSoundTests: probe sanity + order independence, audible contrast between places, synth health
#include <cstdio>
#include <vector>
#include <chrono>
#include <algorithm>
#include "../src/limen_sound.h"

static void sndWriteWav(const char* path, const std::vector<float>& st) {
    FILE* f = fopen(path, "wb");
    if (!f) { printf("cannot write %s\n", path); return; }
    unsigned n = (unsigned)st.size(), bytes = n * 2u, rate = 44100u, tmp;
    unsigned short u16;
    fwrite("RIFF", 1, 4, f); tmp = 36u + bytes; fwrite(&tmp, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    tmp = 16u; fwrite(&tmp, 4, 1, f); u16 = 1; fwrite(&u16, 2, 1, f); u16 = 2; fwrite(&u16, 2, 1, f);
    fwrite(&rate, 4, 1, f); tmp = rate * 4u; fwrite(&tmp, 4, 1, f); u16 = 4; fwrite(&u16, 2, 1, f); u16 = 16; fwrite(&u16, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    std::vector<short> pcm(n);
    for (unsigned i = 0; i < n; i++) {
        float v = st[i] < -1.0f ? -1.0f : (st[i] > 1.0f ? 1.0f : st[i]);
        pcm[i] = (short)(v * 32767.0f);
    }
    fwrite(pcm.data(), 2, n, f);
    fclose(f);
}

// The acoustic situations a listener standing in cell (wx, wz) is in, as a bit set.
enum { SK_ROOM = 0, SK_CRAMPED, SK_CORRIDOR, SK_HALL, SK_VAULT, SK_BRIDGE, SK_BASIN, SK_SHAFT_EDGE, SK_DARK, SK_XENO, SK_DRUNK, SK_COUNT };
static const char* kSndKindName[SK_COUNT] = {"room", "cramped", "corridor", "hall", "vault", "bridge", "basin",
                                             "shaft-edge", "dark-slab", "xenograft", "delirium"};

static unsigned sndKindsAt(int wx, int wz) {
    if (!lmPlayerWalkableAt(wx, wz)) return 0u;
    LmSndCell c = lmSndCellAt(wx, wz);
    if (!c.s) return 0u;
    const LimenSlab& s = *c.s;
    bool high = lmSlabIsVault(s) && !lmIsLowShellTag(c.tag);
    unsigned m = 0u;
    if (c.zone & LZ_XENO) m |= 1u << SK_XENO;
    if (s.dark) m |= 1u << SK_DARK;
    if (lmDeliriumCell255(wx, wz) > 140) m |= 1u << SK_DRUNK;
    if (c.tag == LT_BASIN) return m | (1u << SK_BASIN);
    if (c.tag == LT_BRIDGE) return m | (1u << SK_BRIDGE);
    if (c.flag & LF_CRAMPED) m |= 1u << SK_CRAMPED;
    else if (c.tag == LT_ROOM && !(c.flag & LF_MEZZ)) m |= 1u << SK_ROOM;
    else if (c.tag == LT_CORR && !high) m |= 1u << SK_CORRIDOR;
    else if (c.tag == LT_HALL && !high) m |= 1u << SK_HALL;
    if (high) m |= 1u << SK_VAULT;
    if (lmTagAt(wx + 1, wz) == LT_VOID || lmTagAt(wx - 1, wz) == LT_VOID ||
        lmTagAt(wx, wz + 1) == LT_VOID || lmTagAt(wx, wz - 1) == LT_VOID) m |= 1u << SK_SHAFT_EDGE;
    return m;
}

// A listener strolling in straight lines, turning where the floor ends.
struct SndWalker {
    float x, z; int dir;
    void place(int wx, int wz) { x = (wx + 0.5f) * 5.0f; z = (wz + 0.5f) * 5.0f; dir = 0; }
    float yaw() const {
        static const float y[4] = {1.5707963f, 3.1415927f, -1.5707963f, 0.0f};   // forward = (sin yaw, cos yaw)
        return y[dir & 3];
    }
    void advance(float dist) {
        static const int dx[4] = {1, 0, -1, 0}, dz[4] = {0, -1, 0, 1};
        int cx = (int)floorf(x / 5.0f), cz = (int)floorf(z / 5.0f);
        for (int tries = 0; tries < 4; tries++) {
            float nx = x + dx[dir & 3] * dist, nz = z + dz[dir & 3] * dist;
            // Keep a metre off whatever is ahead.
            int ax = (int)floorf((nx + dx[dir & 3] * 1.0f) / 5.0f), az = (int)floorf((nz + dz[dir & 3] * 1.0f) / 5.0f);
            bool sameCell = ax == cx && az == cz;
            if (sameCell || (lmPlayerWalkableAt(ax, az) && !lmSndEdgeBlocked(cx, cz, ax, az))) { x = nx; z = nz; return; }
            dir++;
        }
    }
};

static LmSoundSynth gToolSynth;

// Renders `sec` seconds of a listener who stands for a second and then walks off from (wx, wz).
static float sndRenderWalk(LmSoundSynth& synth, int wx, int wz, float sec, std::vector<float>* wav,
                           float* rmsOut, LmSoundScene* sceneOut, bool* finiteOut) {
    SndWalker w; w.place(wx, wz);
    LmSoundScene sc;
    LsMix mix; mix.amb = 0.7f * 0.75f; mix.sfx = 0.7f * 0.7f; mix.hum = 1.0f;   // the game's default sliders
    const int FRAME = 735;   // 60 fps
    std::vector<float> buf(FRAME * 2);
    float peak = 0.0f, stepClock = 0.3f;
    double sum = 0.0; long long cnt = 0; bool finite = true;
    int frames = (int)(sec * 60.0f);
    for (int f = 0; f < frames; f++) {
        bool walking = f > 60;
        if (walking) w.advance(1.6f / 60.0f);
        lmProbeSound(w.x, w.z, 5.0f, sc);
        sc.yaw = w.yaw();
        if (f == 0 && sceneOut) *sceneOut = sc;
        synth.setScene(sc);
        if (walking) {
            stepClock -= 1.0f / 60.0f;
            if (stepClock <= 0.0f) { stepClock = 0.52f; synth.startStep(0.88f + 0.3f * synth.rng.uni(), 0.0f); }
        }
        std::fill(buf.begin(), buf.end(), 0.0f);
        synth.render(buf.data(), FRAME, mix);
        for (float v : buf) {
            if (!(v == v) || fabsf(v) > 1e6f) finite = false;
            if (fabsf(v) > peak) peak = fabsf(v);
            sum += (double)v * v; cnt++;
        }
        if (wav) wav->insert(wav->end(), buf.begin(), buf.end());
    }
    if (rmsOut) *rmsOut = (float)sqrt(sum / (double)(cnt ? cnt : 1));
    if (finiteOut) *finiteOut = finite;
    return peak;
}

static bool sndSceneSane(const LmSoundScene& sc) {
    const float v[] = {sc.ceil, sc.meanR, sc.minR, sc.area, sc.volume, sc.absorb, sc.rt60, sc.open, sc.mode[0], sc.mode[1],
                       sc.mode[2], sc.echoDelay[0], sc.echoDelay[1], sc.echoGain[0], sc.echoGain[1], sc.hard, sc.grit,
                       sc.hollow, sc.wet, sc.metal, sc.lampField, sc.voidGain, sc.plant, sc.drone, sc.dripRate,
                       sc.creakRate, sc.mainsHz, sc.droneHz, sc.plantHz};
    for (float f : v) if (!(f == f) || f < 0.0f || f > 1e7f) return false;
    return sc.valid && sc.rt60 >= 0.18f && sc.rt60 <= 7.5f && sc.absorb > 0.01f && sc.absorb < 0.6f &&
           sc.hard <= 1.0f && sc.hollow <= 1.0f && sc.wet <= 1.0f && sc.echoDelay[0] < 1.4f && sc.echoDelay[1] < 1.4f &&
           sc.echoGain[0] < 1.0f && sc.echoGain[1] < 1.0f && sc.ceil >= 2.6f && sc.minR <= sc.meanR + 1e-3f;
}

static void runSound(u32 seed, int level, const char* path, float sec) {
    lmReset(seed + 1, level); lmReset(seed, level);
    const LmSoundDNA& sd = lmSoundDNA();
    printf("sound DNA level %d: mains %.2f Hz, lamp tilt %.2f, drone %.1f Hz x%.3f x%.3f, plant %.1f Hz (bias %.2f), drip bias %.2f, air dull %.2f\n",
           level, sd.mainsHz, sd.humTilt, sd.droneHz, sd.ratioA, sd.ratioB, sd.plantHz, sd.plantBias, sd.dripBias, sd.airDull);
    int spotX[SK_COUNT] = {}, spotZ[SK_COUNT] = {};
    bool have[SK_COUNT] = {};
    for (int ring = 0; ring < 260; ring++) {
        for (int k = -ring; k <= ring; k++) {
            const int cand[4][2] = {{k, -ring}, {k, ring}, {-ring, k}, {ring, k}};
            for (int q = 0; q < 4; q++) {
                unsigned m = sndKindsAt(cand[q][0], cand[q][1]);
                for (int t = 0; t < SK_COUNT; t++) {
                    if ((m >> t & 1u) && !have[t]) { have[t] = true; spotX[t] = cand[q][0]; spotZ[t] = cand[q][1]; }
                }
            }
        }
    }
    std::vector<float> wav;
    printf("%-10s %9s %5s %6s %5s %5s | %4s %4s %4s %4s | %2s %5s %5s %5s %5s %5s | %s\n", "place", "cell", "ceil", "meanR",
           "RT60", "absb", "hard", "holl", "wet", "grit", "lm", "shaft", "plant", "drone", "drip", "creak",
           "RMS x1000: lamps room draught plant drone events steps reverb | peak");
    auto t0 = std::chrono::steady_clock::now();
    double rendered = 0.0;
    for (int t = 0; t < SK_COUNT; t++) {
        if (!have[t]) { printf("%-10s (none within 1.3 km of the origin on this level)\n", kSndKindName[t]); continue; }
        LmSoundScene sc; float rms; bool finite;
        gToolSynth.resetMeters();
        float peak = sndRenderWalk(gToolSynth, spotX[t], spotZ[t], sec, &wav, &rms, &sc, &finite);
        rendered += sec;
        int lamps = 0;
        for (int k = 0; k < LS_LAMPS; k++) if (sc.lamp[k].id) lamps++;
        printf("%-10s %4d,%-4d %5.1f %6.1f %5.2f %5.2f | %4.2f %4.2f %4.2f %4.2f | %2d %5.2f %5.2f %5.2f %5.2f %5.2f |",
               kSndKindName[t], spotX[t], spotZ[t], sc.ceil, sc.meanR, sc.rt60, sc.absorb, sc.hard, sc.hollow, sc.wet, sc.grit,
               lamps, sc.voidGain, sc.plant, sc.drone, sc.dripRate, sc.creakRate);
        for (int k = 0; k < LSL_COUNT; k++) printf(" %5.1f", gToolSynth.layerRms(k) * 1000.0f);
        printf(" | %.3f%s\n", peak, finite ? "" : "  NOT FINITE");
    }
    double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    sndWriteWav(path, wav);
    printf("wrote %s: %.0f s, rendered %.1fx faster than real time (with a full probe every frame)\n",
           path, rendered, rendered / (wall > 1e-9 ? wall : 1e-9));
}

static int runSoundTests(u32 seed) {
    static const int kLv[] = {0, 7, -77, 100500};
    int probes = 0, insane = 0, unstable = 0, badAudio = 0;
    float rtMin = 1e9f, rtMax = 0.0f, hardMin = 1e9f, hardMax = 0.0f, droneMin = 1e9f, droneMax = 0.0f;
    double rtKind[SK_COUNT] = {}; int nKind[SK_COUNT] = {};
    for (int level : kLv) {
        lmReset(seed + 1, level); lmReset(seed, level);
        std::vector<std::pair<int, int>> pts;
        for (int z = -170; z <= 170; z += 9) for (int x = -170; x <= 170; x += 7) if (lmPlayerWalkableAt(x, z)) pts.push_back({x, z});
        std::vector<LmSoundScene> first(pts.size());
        for (size_t k = 0; k < pts.size(); k++) {
            lmProbeSound((pts[k].first + 0.37f) * 5.0f, (pts[k].second + 0.61f) * 5.0f, 5.0f, first[k]);
            const LmSoundScene& sc = first[k];
            probes++;
            if (!sndSceneSane(sc)) insane++;
            if (sc.rt60 < rtMin) rtMin = sc.rt60;
            if (sc.rt60 > rtMax) rtMax = sc.rt60;
            if (sc.hard < hardMin) hardMin = sc.hard;
            if (sc.hard > hardMax) hardMax = sc.hard;
            unsigned m = sndKindsAt(pts[k].first, pts[k].second);
            for (int t = 0; t < SK_COUNT; t++) if (m >> t & 1u) { rtKind[t] += sc.rt60; nKind[t]++; }
        }
        float dh = lmSoundDNA().droneHz;
        if (dh < droneMin) droneMin = dh;
        if (dh > droneMax) droneMax = dh;
        // The scene is a pure function of (seed, level, position): rebuild the world in another order.
        lmSlabCacheClear();
        for (size_t k = pts.size(); k-- > 0;) {
            LmSoundScene again;
            lmProbeSound((pts[k].first + 0.37f) * 5.0f, (pts[k].second + 0.61f) * 5.0f, 5.0f, again);
            if (memcmp(&again, &first[k], sizeof(again)) != 0) unstable++;
        }
        // The synth must stay finite, below full scale and audible wherever it is dropped.
        size_t stride = pts.size() / 5 ? pts.size() / 5 : 1;
        for (size_t k = 0; k < pts.size(); k += stride) {
            float rms; bool finite;
            float peak = sndRenderWalk(gToolSynth, pts[k].first, pts[k].second, 3.0f, nullptr, &rms, nullptr, &finite);
            if (!finite || peak > 0.9f || rms < 2e-4f) badAudio++;
        }
    }
    auto mean = [&](int t) { return nKind[t] ? rtKind[t] / nKind[t] : 0.0; };
    // The space must be audible in the numbers: closets are dry, vaults are long, levels differ.
    bool contrast = rtMax > rtMin * 4.0f && hardMax - hardMin > 0.2f && droneMax - droneMin > 3.0f &&
                    nKind[SK_VAULT] > 0 && nKind[SK_ROOM] > 0 && mean(SK_VAULT) > mean(SK_ROOM) * 1.8 &&
                    (nKind[SK_CRAMPED] == 0 || mean(SK_CRAMPED) < mean(SK_HALL));
    printf("sound probe (%d spots on 4 levels): sane %s, order-independent %s\n", probes, insane ? "FAIL" : "ok", unstable ? "FAIL" : "ok");
    printf("sound contrast (RT60 %.2f..%.2f s; mean cramped %.2f room %.2f corridor %.2f hall %.2f vault %.2f; floor hardness %.2f..%.2f): %s\n",
           rtMin, rtMax, mean(SK_CRAMPED), mean(SK_ROOM), mean(SK_CORRIDOR), mean(SK_HALL), mean(SK_VAULT), hardMin, hardMax,
           contrast ? "ok" : "FAIL");
    printf("sound synth (finite, below full scale, audible): %s\n", badAudio ? "FAIL" : "ok");
    return (insane ? 1 : 0) + (unstable ? 1 : 0) + (contrast ? 0 : 1) + (badAudio ? 1 : 0);
}
