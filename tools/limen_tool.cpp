// LIMEN offline tool: maps, self-tests, statistics, motif finder. No game code, no OpenGL.
//   limen_tool map    <seed> <level> <x0> <z0> <w> <h> <out.ppm> [scale]
//   limen_tool test   <seed>
//   limen_tool sweep  <seed> <n>
//   limen_tool levels <seed> <n>
//   limen_tool stats  <seed> <level>
//   limen_tool slab   <seed> <level> <j> <i>
//   limen_tool find   <seed> <level>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>
#include <algorithm>
#include "../src/limen_world.h"
#include "limen_tool_sound.h"
#include "limen_tool_narrator.h"

// Colours carry facts, not types: brightness of open floor = ceiling height, brown walls =
// low shells (amber = 9 m mezzanine), cyan = walkable dry basins, bright steps = LF_STAIR_DESC.
static void cellColour(int wx, int wz, unsigned char* rgb) {
    LimenRect rc = lmSlabAt(wx, wz);
    bool seam = (wx == rc.x0 || wz == rc.z0);
    const LimenSlab* s = seam ? nullptr : &lmSlab(rc.j, rc.i);
    int t = lmTagAt(wx, wz);
    unsigned short f = s ? s->flag[wx - rc.x0 - 1][wz - rc.z0 - 1] : 0;
    int tiers = s ? s->ceilTiers : 1;
    int r = 24, g = 24, b = 28;
    if (t == LT_WALL && (f & LF_LOW_WALL)) {
        if (f & LF_MEZZ) { r = 132; g = 98; b = 52; }
        else { r = 96; g = 78; b = 44; }
    }
    else if (t == LT_LOW) { r = 214; g = 190; b = 96; }
    else if (t == LT_ROOM) { r = 152; g = 134; b = 82; }
    else if (t == LT_CORR) { r = 198; g = 182; b = 116; if (tiers > 1) { r = 150; g = 170; b = 176; } }
    else if (t == LT_HALL) {
        if (tiers <= 1) { r = 168; g = 160; b = 128; }
        else { r = 110 + tiers * 9; g = 124 + tiers * 9; b = 132 + tiers * 9; }
    }
    else if (t == LT_VOID) { r = 6; g = 8; b = 40; }
    else if (t == LT_BASIN) {
        if (f & LF_STAIR_DESC) { r = 186; g = 232; b = 228; }
        else if (f & LF_WALK_BASIN) { r = 74; g = 132; b = 144; }
        else { r = 48; g = 86; b = 96; }
    }
    else if (t == LT_BRIDGE) { r = 236; g = 232; b = 214; }
    else if (t == LT_VITRINE) { r = 214; g = 148; b = 224; }
    if (s && (s->zone[wx - rc.x0 - 1][wz - rc.z0 - 1] & LZ_XENO) && t != LT_WALL) {
        r = (r * 2 + 120) / 3; b = (b * 2 + 210) / 3;
    }
    rgb[0] = (unsigned char)r; rgb[1] = (unsigned char)g; rgb[2] = (unsigned char)b;
}

static void writeMap(const char* path, int x0, int z0, int w, int h, int scale) {
    std::vector<unsigned char> img((size_t)w * scale * h * scale * 3);
    for (int z = 0; z < h; z++) {
        for (int x = 0; x < w; x++) {
            int wx = x0 + x, wz = z0 + z;
            unsigned char c[3];
            cellColour(wx, wz, c);
            unsigned short flg = lmFlagAt(wx, wz);
            int lamp = scale >= 3 ? lmLampAt(wx, wz) : 0;
            bool pillar = scale >= 3 && lmPillarAt(wx, wz);
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    unsigned char pr = c[0], pg = c[1], pb = c[2];
                    bool centre = sx > 0 && sy > 0 && sx < scale - 1 && sy < scale - 1;
                    // Draw 0.26 m thin partitions on cell West/North edges!
                    if (scale >= 3 && (flg & LF_THIN_W) && sx == 0 && !( (flg & LF_THIN_DOOR_W) && sy == scale / 2 )) {
                        pr = 74; pg = 62; pb = 38;
                    }
                    if (scale >= 3 && (flg & LF_THIN_N) && sy == 0 && !( (flg & LF_THIN_DOOR_N) && sx == scale / 2 )) {
                        pr = 74; pg = 62; pb = 38;
                    }
                    // Draw narrow corridor side insets
                    if (scale >= 4 && (flg & LF_NARROW_X) && (sy == 0 || sy == scale - 1)) { pr = 56; pg = 50; pb = 34; }
                    if (scale >= 4 && (flg & LF_NARROW_Z) && (sx == 0 || sx == scale - 1)) { pr = 56; pg = 50; pb = 34; }
                    if (lamp == 1 && centre) { pr = 255; pg = 250; pb = 200; }
                    if (lamp == 2 && centre) { pr = 200; pg = 40; pb = 40; }
                    if (pillar && centre) { pr = 70; pg = 70; pb = 80; }
                    size_t o = ((size_t)(z * scale + sy) * (size_t)(w * scale) + (size_t)(x * scale + sx)) * 3;
                    img[o] = pr; img[o + 1] = pg; img[o + 2] = pb;
                }
            }
        }
    }
    FILE* f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w * scale, h * scale);
    fwrite(img.data(), 1, img.size(), f);
    fclose(f);
}

static int countStranded(int x0, int z0, int w, int h) {
    std::vector<int> comp((size_t)w * h, -1);
    std::vector<int> size, touches, stack;
    for (int s = 0; s < w * h; s++) {
        if (comp[s] >= 0 || !lmWalkable(x0 + s % w, z0 + s / w)) continue;
        int id = (int)size.size();
        size.push_back(0); touches.push_back(0);
        stack.push_back(s); comp[s] = id;
        while (!stack.empty()) {
            int c = stack.back(); stack.pop_back();
            int cx = c % w, cz = c / w;
            size[id]++;
            if (cx == 0 || cz == 0 || cx == w - 1 || cz == h - 1) touches[id] = 1;
            static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
            for (int d = 0; d < 4; d++) {
                int nx = cx + dx[d], nz = cz + dz[d];
                if (nx < 0 || nz < 0 || nx >= w || nz >= h) continue;
                int n = nz * w + nx;
                if (comp[n] >= 0 || !lmWalkable(x0 + nx, z0 + nz)) continue;
                comp[n] = id; stack.push_back(n);
            }
        }
    }
    int best = 0, stranded = 0;
    for (size_t k = 0; k < size.size(); k++) if (size[k] > size[best]) best = (int)k;
    for (size_t k = 0; k < size.size(); k++) if ((int)k != best && !touches[k]) stranded += size[k];
    return stranded;
}

struct LevelReport {
    int level;
    LimenLevelDNA dna;
    double roomPct, hallPct, dropPct, vaultPct, meanHeightM, liveLampsPer100, darkPct, xenoPct;
    int deadThreshTotal, vitrineTotal;
    int totalSlabs, megaCount[LM_MEGA_FORM_COUNT];
    double delSoberPct, delTipsyPct, delPocketPct, delMean;
    int delStructs, escherCount;
};

static LevelReport measureLevel(u32 seed, int level, int rad = 4) {
    lmReset(seed + 1, level); lmReset(seed, level);
    LevelReport rep{};
    rep.level = level;
    rep.dna = gLimenDNA;
    int slabs = 0, vaults = 0, darkSlabs = 0, xenoSlabs = 0, deadThr = 0, vitrines = 0;
    long long cells = 0, roomC = 0, hallC = 0, dropC = 0, floorC = 0, liveLamps = 0;
    long long delSober = 0, delTipsy = 0, delPocket = 0, delSum = 0;
    double heightSum = 0.0;
    for (int j = -rad; j <= rad; j++) {
        for (int i = -rad; i <= rad; i++) {
            static LimenSlab s;
            lmBuildSlab(s, j, i);
            lmPlaceLampSites(s);
            slabs++;
            rep.delStructs += (s.deliriumStruct & 15) + ((s.deliriumStruct >> 4) & 15);
            if (lmSlabIsVault(s)) {
                vaults++;
                if (s.megaForm >= 0 && s.megaForm < LM_MEGA_FORM_COUNT) rep.megaCount[s.megaForm]++;
                if (!s.dark && lmSlabHasEscherKnotAt(j, i)) rep.escherCount++;
            }
            if (s.dark) darkSlabs++;
            if (s.xeno.active) xenoSlabs++;
            deadThr += s.deadThresholds;
            vitrines += s.vitrineRooms;
            for (int x = 0; x < s.w; x++) {
                for (int z = 0; z < s.h; z++) {
                    int t = s.tag[x][z];
                    unsigned short f = s.flag[x][z];
                    unsigned char zn = s.zone[x][z];
                    cells++;
                    int dv = lmDeliriumCell255(s.rect.x0 + 1 + x, s.rect.z0 + 1 + z);
                    delSum += dv;
                    if (dv <= 28) delSober++;
                    else if (dv < 115) delTipsy++;
                    else delPocket++;
                    if (t == LT_ROOM || t == LT_LOW) roomC++;
                    if (t == LT_HALL) hallC++;
                    if (lmIsPitTag(t)) dropC++;
                    if (lmIsWalkableTag(t)) {
                        floorC++;
                        double ch = (lmSlabIsVault(s) && !lmIsLowShellTag(t))
                                  ? (s.ceilTiers * 4.5)
                                  : (double)lmShellCeilHeight(f, s.ceilTiers, 4.5f, zn);
                        heightSum += ch;
                        if (lmLampLocal(s, x, z)) {
                            bool special = (x == s.specialLampX && z == s.specialLampZ);
                            bool on = s.dark ? special : !special;
                            if (on) liveLamps++;
                        }
                    }
                }
            }
        }
    }
    rep.roomPct = 100.0 * roomC / (cells ? cells : 1);
    rep.hallPct = 100.0 * hallC / (cells ? cells : 1);
    rep.dropPct = 100.0 * dropC / (cells ? cells : 1);
    rep.vaultPct = 100.0 * vaults / (slabs ? slabs : 1);
    rep.meanHeightM = heightSum / (floorC ? floorC : 1);
    rep.liveLampsPer100 = 100.0 * liveLamps / (floorC ? floorC : 1);
    rep.darkPct = 100.0 * darkSlabs / (slabs ? slabs : 1);
    rep.xenoPct = 100.0 * xenoSlabs / (slabs ? slabs : 1);
    rep.deadThreshTotal = deadThr;
    rep.vitrineTotal = vitrines;
    rep.totalSlabs = slabs;
    rep.delSoberPct  = 100.0 * delSober  / (cells ? cells : 1);
    rep.delTipsyPct  = 100.0 * delTipsy  / (cells ? cells : 1);
    rep.delPocketPct = 100.0 * delPocket / (cells ? cells : 1);
    rep.delMean      = ((double)delSum / 255.0) / (cells ? cells : 1);
    return rep;
}

static int surfStructDiff(const LmSurfDNA& a, const LmSurfDNA& b) {
    return abs((int)a.seamFreqX - (int)b.seamFreqX) + abs((int)a.seamDepth - (int)b.seamDepth) +
           abs((int)a.stripe - (int)b.stripe) + abs((int)a.grain - (int)b.grain) +
           abs((int)a.sheen - (int)b.sheen) + abs((int)a.grime - (int)b.grime);
}

static int dnaDistance(const LimenLevelDNA& a, const LimenLevelDNA& b) {
    int d = abs(a.rowPitch - b.rowPitch) * 4 + abs(a.colPitch - b.colPitch) * 4 +
            abs(a.vaultThreshold - b.vaultThreshold) * 2 + abs(a.maxStoreys - b.maxStoreys) * 8 +
            abs((int)a.lightDensity - (int)b.lightDensity) + abs((int)a.propDensity - (int)b.propDensity) +
            abs((int)a.fogDensity - (int)b.fogDensity) / 2;
    for (int k = 0; k < LG_COUNT; k++) d += abs((int)a.geneBias[k] - (int)b.geneBias[k]);
    for (int t = 0; t < LM_THEME_COUNT; t++) d += abs((int)a.themeWeight[t] - (int)b.themeWeight[t]) / 3;
    for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) d += abs((int)a.megaFormWeight[f] - (int)b.megaFormWeight[f]) / 3;
    for (int c = 0; c < 3; c++) {
        d += abs((int)a.wallSurf.rgb[c] - (int)b.wallSurf.rgb[c]) +
             abs((int)a.floorSurf.rgb[c] - (int)b.floorSurf.rgb[c]) / 2 +
             abs((int)a.lightRGB[c] - (int)b.lightRGB[c]) / 2;
    }
    d += (surfStructDiff(a.wallSurf, b.wallSurf) + surfStructDiff(a.floorSurf, b.floorSurf)) / 5;
    d += (abs((int)a.dadoHeight - (int)b.dadoHeight) + abs((int)a.ceilGridStrength - (int)b.ceilGridStrength) +
          abs((int)a.corniceSize - (int)b.corniceSize) + abs((int)a.floorSeamStrength - (int)b.floorSeamStrength)) / 3;
    return d;
}

static double metricDistance(const LevelReport& a, const LevelReport& b) {
    double themeDiff = 0.0;
    for (int t = 0; t < LM_THEME_COUNT; t++)
        themeDiff += fabs((double)a.dna.themeWeight[t] - (double)b.dna.themeWeight[t]) / 32.0;
    double toneMatDiff =
        (abs((int)a.dna.wallSurf.rgb[0] - (int)b.dna.wallSurf.rgb[0]) +
         abs((int)a.dna.wallSurf.rgb[1] - (int)b.dna.wallSurf.rgb[1]) +
         abs((int)a.dna.wallSurf.rgb[2] - (int)b.dna.wallSurf.rgb[2]) +
         abs((int)a.dna.floorSurf.rgb[0] - (int)b.dna.floorSurf.rgb[0]) +
         abs((int)a.dna.floorSurf.rgb[1] - (int)b.dna.floorSurf.rgb[1]) +
         abs((int)a.dna.floorSurf.rgb[2] - (int)b.dna.floorSurf.rgb[2])) / 24.0 +
        (surfStructDiff(a.dna.wallSurf, b.dna.wallSurf) + surfStructDiff(a.dna.floorSurf, b.dna.floorSurf)) / 64.0 +
        (abs((int)a.dna.lightRGB[0] - (int)b.dna.lightRGB[0]) +
         abs((int)a.dna.lightRGB[2] - (int)b.dna.lightRGB[2]) +
         abs((int)a.dna.fogDensity - (int)b.dna.fogDensity) +
         abs((int)a.dna.dadoHeight - (int)b.dna.dadoHeight)) / 36.0;
    return fabs(a.roomPct - b.roomPct) + fabs(a.hallPct - b.hallPct) +
           fabs(a.dropPct - b.dropPct) * 1.5 + fabs(a.vaultPct - b.vaultPct) * 0.7 +
           fabs(a.meanHeightM - b.meanHeightM) * 2.5 + fabs(a.liveLampsPer100 - b.liveLampsPer100) * 4.0 +
           themeDiff + toneMatDiff;
}

static int satBucketIdx(float s) {
    if (s < 0.08f) return 0;
    if (s < 0.20f) return 1;
    if (s < 0.35f) return 2;
    if (s < 0.50f) return 3;
    return 4;
}

static int runLevels(u32 seed, int n) {
    if (n < 2) n = 2;
    std::vector<LevelReport> reps;
    reps.reserve(n);
    int wHist[5] = {}, fHist[5] = {}, cHist[5] = {}, maxHist[5] = {};
    int multiVivid = 0, floorLighter = 0;
    double wSSum = 0.0, fSSum = 0.0, cSSum = 0.0;

    for (int lv = 0; lv < n; lv++) {
        LevelReport r = measureLevel(seed, lv, 4);
        reps.push_back(r);
        float wH = 0, wS = 0, wV = 0, fH = 0, fS = 0, fV = 0, cH = 0, cS = 0, cV = 0;
        lmRGBToHSV01(r.dna.wallSurf.rgb,  wH, wS, wV);
        lmRGBToHSV01(r.dna.floorSurf.rgb, fH, fS, fV);
        lmRGBToHSV01(r.dna.ceilSurf.rgb,  cH, cS, cV);
        wSSum += wS; fSSum += fS; cSSum += cS;
        wHist[satBucketIdx(wS)]++;
        fHist[satBucketIdx(fS)]++;
        cHist[satBucketIdx(cS)]++;
        float mxS = wS > fS ? (wS > cS ? wS : cS) : (fS > cS ? fS : cS);
        maxHist[satBucketIdx(mxS)]++;
        if ((wS > 0.35f ? 1 : 0) + (fS > 0.35f ? 1 : 0) + (cS > 0.35f ? 1 : 0) >= 2) multiVivid++;
        if (fV > wV + 0.02f) floorLighter++;

        if (n <= 24 || lv < 16 || lv == n - 1) {
            int t1 = 0, t2 = 1;
            for (int t = 0; t < LM_THEME_COUNT; t++) {
                if (r.dna.themeWeight[t] > r.dna.themeWeight[t1]) { t2 = t1; t1 = t; }
                else if (t != t1 && r.dna.themeWeight[t] > r.dna.themeWeight[t2]) { t2 = t; }
            }
            char extBuf[80] = {0};
            int pos = 0;
            for (int g = 0; g < LG_COUNT; g++) {
                if (abs((int)r.dna.geneBias[g]) >= 52 && pos < 64) {
                    pos += snprintf(extBuf + pos, sizeof(extBuf) - pos, "%s%s%+d",
                                    pos ? "," : "", lmGeneName(g), (int)r.dna.geneBias[g]);
                }
            }
            char qn[32];
            snprintf(qn, sizeof(qn), "%s%s%s", lmQuirkName(r.dna.quirk), r.dna.quirk2 != LQ_NONE ? "+" : "",
                     r.dna.quirk2 != LQ_NONE ? lmQuirkName(r.dna.quirk2) : "");
            printf("L%-3d %-15s %-8s %-8s/%-8s/%-8s %-8s | ", lv, qn, lmSpacePoleName(r.dna.spacePole), lmWallPoleName(r.dna.wallPole),
                   lmFloorPoleName(r.dna.floorPole), lmCeilPoleName(r.dna.ceilPole), lmLightPoleName(r.dna.lightPole));
            printf("L%-3d %2dx%-2d vlt=%4.0f%% rm=%4.1f%% H=%4.1fm lmp=%4.1f | %s+%s w=#%02x%02x%02x(S%.2f/V%.2f) f=#%02x%02x%02x(S%.2f/V%.2f) c=#%02x%02x%02x(S%.2f/V%.2f) [%s]\n",
                   lv, r.dna.rowPitch, r.dna.colPitch, r.vaultPct, r.roomPct, r.meanHeightM, r.liveLampsPer100,
                   lmThemeShortName(t1), lmThemeShortName(t2),
                   r.dna.wallSurf.rgb[0], r.dna.wallSurf.rgb[1], r.dna.wallSurf.rgb[2], wS, wV,
                   r.dna.floorSurf.rgb[0], r.dna.floorSurf.rgb[1], r.dna.floorSurf.rgb[2], fS, fV,
                   r.dna.ceilSurf.rgb[0], r.dna.ceilSurf.rgb[1], r.dna.ceilSurf.rgb[2], cS, cV, extBuf);
        }
    }

    static const char* bNames[5] = {"[0.00..0.08)", "[0.08..0.20)", "[0.20..0.35)", "[0.35..0.50)", "[0.50..1.00]"};
    printf("saturation histogram across %d levels (mean S: wall=%.2f floor=%.2f ceil=%.2f; multi>0.35=%d, floor>wall=%d):\n",
           n, wSSum / n, fSSum / n, cSSum / n, multiVivid, floorLighter);
    for (int b = 0; b < 5; b++) {
        printf("  %s  wall=%3d (%4.1f%%)  floor=%3d (%4.1f%%)  ceil=%3d (%4.1f%%)  maxSurf=%3d (%4.1f%%)\n",
               bNames[b],
               wHist[b], 100.0 * wHist[b] / n,
               fHist[b], 100.0 * fHist[b] / n,
               cHist[b], 100.0 * cHist[b] / n,
               maxHist[b], 100.0 * maxHist[b] / n);
    }
    long long totSlabsAll = 0, totMegaAll = 0, formAll[LM_MEGA_FORM_COUNT] = {};
    for (const auto& r : reps) {
        totSlabsAll += r.totalSlabs;
        for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) { formAll[f] += r.megaCount[f]; totMegaAll += r.megaCount[f]; }
    }
    printf("mega-form slab share across %d levels (%lld/%lld mega slabs = %.1f%%):\n ",
           n, totMegaAll, totSlabsAll, totSlabsAll ? (100.0 * totMegaAll / totSlabsAll) : 0.0);
    for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) {
        printf(" %s=%.1f%%(%.0f%%m)", lmMegaFormName(f),
               totSlabsAll ? (100.0 * formAll[f] / totSlabsAll) : 0.0,
               totMegaAll  ? (100.0 * formAll[f] / totMegaAll)  : 0.0);
    }
    printf("\n");
    double sumSober = 0.0, sumTipsy = 0.0, sumPocket = 0.0, sumDelM = 0.0;
    int sumDelStr = 0, sumEscher = 0;
    for (const auto& r : reps) {
        sumSober += r.delSoberPct; sumTipsy += r.delTipsyPct;
        sumPocket += r.delPocketPct; sumDelM += r.delMean;
        sumDelStr += r.delStructs; sumEscher += r.escherCount;
    }
    printf("delirium ('пьянка') across %d levels: mean=%.2f | sober(<0.11)=%.1f%% tipsy(0.11..0.45)=%.1f%% drunk(>=0.45)=%.1f%% | nested/spiral=%d | escher=%d/%lld(%.1f%% mega)\n",
           n, sumDelM / n, sumSober / n, sumTipsy / n, sumPocket / n, sumDelStr,
           sumEscher, totMegaAll, totMegaAll ? (100.0 * sumEscher / totMegaAll) : 0.0);

    int minDna = 1 << 30, badPairs = 0, pairs = 0;
    double minMet = 1e9;
    for (int a = 0; a < n; a++) {
        for (int b = a + 1; b < n; b++) {
            pairs++;
            int dd = dnaDistance(reps[a].dna, reps[b].dna);
            double md = metricDistance(reps[a], reps[b]);
            if (dd < minDna) minDna = dd;
            if (md < minMet) minMet = md;
            if (dd < 180 || md < 14.0) badPairs++;
        }
    }
    printf("pairwise (%d pairs): min DNA dist=%d, min metric dist=%.1f, collisions=%d %s\n",
           pairs, minDna, minMet, badPairs, badPairs == 0 ? "ok" : "FAIL");
    return badPairs;
}

static int runSweep(u32 seed, int n) {
    if (n < 1) n = 1;
    auto t0 = std::chrono::steady_clock::now();
    const int X0 = -120, Z0 = -100, W = 240, H = 200, SRAD = 3;
    int failPart = 0, failStrand = 0, failSeam = 0, failVocab = 0;
    int negCount = 0, minLv = 2147483647, maxLv = -2147483647 - 1, totalSlabs = 0;
    std::vector<int> failedLevels;

    for (int k = 0; k < n; k++) {
        u32 h = lmLvlHash(seed, k, 0x535750u);
        int level = (k == 0) ? (-2147483647 - 1)
                  : ((k % 5 == 1) ? ((int)(h % 401u) - 200) : (int)h);
        if (level < 0) negCount++;
        if (level < minLv) minLv = level;
        if (level > maxLv) maxLv = level;

        lmReset(seed, level);

        int badP = 0;
        for (int z = -160; z < 160; z += 7) {
            for (int x = -160; x < 160; x += 5) {
                LimenRect r = lmSlabAt(x, z);
                if (x < r.x0 || x >= r.x1 || z < r.z0 || z >= r.z1) badP++;
                if (r.x1 - r.x0 - 1 > LM_MAX || r.z1 - r.z0 - 1 > LM_MAX || r.x1 - r.x0 < 10 || r.z1 - r.z0 < 10) badP++;
            }
        }

        int stranded = countStranded(X0, Z0, W, H);

        int broken = 0;
        for (int z = Z0; z < Z0 + H; z++) {
            for (int x = X0; x < X0 + W; x++) {
                LimenRect r = lmSlabAt(x, z);
                if ((x != r.x0 && z != r.z0) || lmCell(x, z) != 0) continue;
                if (x == r.x0 && (!lmWalkable(x - 1, z) || !lmWalkable(x + 1, z))) broken++;
                if (z == r.z0 && x != r.x0 && (!lmWalkable(x, z - 1) || !lmWalkable(x, z + 1))) broken++;
            }
        }

        int vocab = 0, pitUnsafe = 0;
        for (int j = -SRAD; j <= SRAD; j++) {
            for (int i = -SRAD; i <= SRAD; i++) {
                const LimenSlab& s = lmSlab(j, i);
                totalSlabs++;
                for (int x = 0; x < s.w; x++) {
                    for (int z = 0; z < s.h; z++) {
                        int t = s.tag[x][z];
                        unsigned short f = s.flag[x][z];
                        int wx = s.rect.x0 + 1 + x, wz = s.rect.z0 + 1 + z;
                        int cellVal = lmCell(wx, wz);
                        if (lmIsNonWalkOpenTag(t) && cellVal != 2) pitUnsafe++;
                        if (t == LT_WALL && cellVal != 1) pitUnsafe++;
                        if (lmIsWalkableTag(t) && cellVal != 0) pitUnsafe++;

                        bool okXenoMezz = (s.zone[x][z] & LZ_XENO) != 0 && lmIsWalkableTag(t);
                        unsigned short nonVaultForbid = okXenoMezz
                            ? (LF_LOW_WALL | LF_OVER_VOID | LF_OVER_BASIN | LF_WALK_BASIN | LF_STAIR_DESC)
                            : (LF_LOW_WALL | LF_MEZZ | LF_OVER_VOID | LF_OVER_BASIN | LF_WALK_BASIN | LF_STAIR_DESC);
                        if (!lmSlabIsVault(s) && (lmIsPitTag(t) || t == LT_BRIDGE || (f & nonVaultForbid) != 0)) vocab++;
                        if ((f & LF_LOW_WALL) && t != LT_WALL) vocab++;
                        if ((f & (LF_OVER_VOID | LF_OVER_BASIN)) && t != LT_BRIDGE) vocab++;
                        if (t == LT_BRIDGE && !(f & (LF_OVER_VOID | LF_OVER_BASIN))) vocab++;
                        if ((f & (LF_WALK_BASIN | LF_STAIR_DESC)) && t != LT_BASIN) vocab++;
                        if ((f & (LF_THIN_W | LF_THIN_N | LF_NARROW_X | LF_NARROW_Z)) && !lmIsWalkableTag(t)) vocab++;
                    }
                }
            }
        }

        if (badP) failPart++;
        if (stranded) failStrand++;
        if (broken) failSeam++;
        if (vocab || pitUnsafe) failVocab++;
        if (badP || stranded || broken || vocab || pitUnsafe) failedLevels.push_back(level);
    }

    double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    int bad = (int)failedLevels.size();
    printf("sweep seed=%u n=%d (neg=%d, range=[%d..%d], slabs=%d, %.2fs): passed=%d/%d failed=%d [part=%d strand=%d seam=%d vocab=%d] %s\n",
           seed, n, negCount, minLv, maxLv, totalSlabs, sec, n - bad, n, bad,
           failPart, failStrand, failSeam, failVocab, bad == 0 ? "ok" : "FAIL");
    if (!failedLevels.empty()) {
        printf("failed levels:");
        for (size_t i = 0; i < failedLevels.size(); i++) printf(" %d", failedLevels[i]);
        printf("\n");
    }
    return bad;
}

static int runTests(u32 seed) {
    static const int kTestLevels[] = {0, 1, 2, 3, 7, 12, -3, -77, -404, 31337, 100500, 2147483647};
    const int nLv = (int)(sizeof(kTestLevels) / sizeof(kTestLevels[0]));
    int failed = 0;
    int globalFormLevels[LM_MEGA_FORM_COUNT] = {};
    int minFormsOnAnyLevel = LM_MEGA_FORM_COUNT + 1;
    std::vector<unsigned> domMasks;
    std::vector<LevelReport> reps;
    for (int li = 0; li < nLv; li++) {
        int level = kTestLevels[li];
        lmReset(seed + 1, level); lmReset(seed, level);
        // 1. partition
        int bad = 0;
        for (int z = -700; z < 700; z += 7) {
            for (int x = -700; x < 700; x += 5) {
                LimenRect r = lmSlabAt(x, z);
                if (x < r.x0 || x >= r.x1 || z < r.z0 || z >= r.z1) bad++;
                if (r.x1 - r.x0 - 1 > LM_MAX || r.z1 - r.z0 - 1 > LM_MAX || r.x1 - r.x0 < 10 || r.z1 - r.z0 < 10) bad++;
            }
        }
        printf("level %d partition: %s\n", level, bad ? "FAIL" : "ok");
        failed += bad ? 1 : 0;

        // 2. order independence + LRU eviction determinism
        const int X0 = -240, Z0 = -200, W = 480, H = 400;
        u32 a = lmWindowChecksum(X0, Z0, W, H);
        lmSlabCacheClear();
        u32 crc = 0;
        for (int z = Z0 + H - 1; z >= Z0; z--) for (int x = X0 + W - 1; x >= X0; x--) crc += (u32)lmTagAt(x, z) * (u32)(x * 31 + z);
        u32 b = lmWindowChecksum(X0, Z0, W, H);
        for (int k = 0; k < 4000; k++) lmTagAt((int)(lmHash((u32)k, 1u, 0u, 9u) % 4000u) - 2000, (int)(lmHash((u32)k, 2u, 0u, 9u) % 4000u) - 2000);
        u32 c = lmWindowChecksum(X0, Z0, W, H);
        printf("level %d order independence: %08x %08x %08x %s\n", level, a, b, c, (a == b && b == c) ? "ok" : "FAIL");
        failed += (a == b && b == c) ? 0 : 1;

        // 3. connectivity
        int stranded = countStranded(X0, Z0, W, H);
        printf("level %d stranded floor cells: %d %s\n", level, stranded, stranded == 0 ? "ok" : "FAIL");
        failed += stranded == 0 ? 0 : 1;

        // 4. seam contract
        int broken = 0;
        for (int z = Z0; z < Z0 + H; z++) {
            for (int x = X0; x < X0 + W; x++) {
                LimenRect r = lmSlabAt(x, z);
                if ((x != r.x0 && z != r.z0) || lmCell(x, z) != 0) continue;
                if (x == r.x0 && (!lmWalkable(x - 1, z) || !lmWalkable(x + 1, z))) broken++;
                if (z == r.z0 && x != r.x0 && (!lmWalkable(x, z - 1) || !lmWalkable(x, z + 1))) broken++;
            }
        }
        printf("level %d openings leading into a wall: %d %s\n", level, broken, broken == 0 ? "ok" : "FAIL");
        failed += broken == 0 ? 0 : 1;

        // 5. cell vocabulary, AI/Objective pit safety, and mega-form diversity in radius 8
        int vocab = 0, pitUnsafe = 0;
        int lvForms[LM_MEGA_FORM_COUNT] = {};
        for (int j = -8; j <= 8; j++) {
            for (int i = -8; i <= 8; i++) {
                const LimenSlab& s = lmSlab(j, i);
                if (lmSlabIsVault(s) && s.megaForm >= 0 && s.megaForm < LM_MEGA_FORM_COUNT)
                    lvForms[s.megaForm]++;
                for (int x = 0; x < s.w; x++) {
                    for (int z = 0; z < s.h; z++) {
                        int t = s.tag[x][z];
                        unsigned short f = s.flag[x][z];
                        int wx = s.rect.x0 + 1 + x, wz = s.rect.z0 + 1 + z;
                        int cellVal = lmCell(wx, wz);
                        if (lmIsNonWalkOpenTag(t) && cellVal != 2) pitUnsafe++;
                        if (t == LT_WALL && cellVal != 1) pitUnsafe++;
                        if (lmIsWalkableTag(t) && cellVal != 0) pitUnsafe++;

                        bool okXenoMezz = (s.zone[x][z] & LZ_XENO) != 0 && lmIsWalkableTag(t);
                        unsigned short nonVaultForbid = okXenoMezz
                            ? (LF_LOW_WALL | LF_OVER_VOID | LF_OVER_BASIN | LF_WALK_BASIN | LF_STAIR_DESC)
                            : (LF_LOW_WALL | LF_MEZZ | LF_OVER_VOID | LF_OVER_BASIN | LF_WALK_BASIN | LF_STAIR_DESC);
                        if (!lmSlabIsVault(s) && (lmIsPitTag(t) || t == LT_BRIDGE || (f & nonVaultForbid) != 0))
                            vocab++;
                        if ((f & LF_LOW_WALL) && t != LT_WALL) vocab++;
                        if ((f & (LF_OVER_VOID | LF_OVER_BASIN)) && t != LT_BRIDGE) vocab++;
                        if (t == LT_BRIDGE && !(f & (LF_OVER_VOID | LF_OVER_BASIN))) vocab++;
                        if ((f & (LF_WALK_BASIN | LF_STAIR_DESC)) && t != LT_BASIN) vocab++;
                        if ((f & (LF_THIN_W | LF_THIN_N | LF_NARROW_X | LF_NARROW_Z)) && !lmIsWalkableTag(t)) vocab++;
                    }
                }
            }
        }
        int nFormsHere = 0, dom1 = 0, dom2 = 1;
        for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) {
            if (lvForms[f] > 0) { nFormsHere++; globalFormLevels[f]++; }
            if (f > 0) {
                if (lvForms[f] > lvForms[dom1]) { dom2 = dom1; dom1 = f; }
                else if (f != dom1 && lvForms[f] > lvForms[dom2]) { dom2 = f; }
            }
        }
        if (nFormsHere < minFormsOnAnyLevel) minFormsOnAnyLevel = nFormsHere;
        domMasks.push_back((1u << dom1) | (1u << dom2));
        printf("level %d cell vocabulary violations: %d, pit AI/obj hazards: %d, megaForms(rad=8)=%d [dom:%s+%s] %s\n",
               level, vocab, pitUnsafe, nFormsHere, lmMegaFormName(dom1), lmMegaFormName(dom2),
               (vocab == 0 && pitUnsafe == 0 && nFormsHere >= 3) ? "ok" : "FAIL");
        failed += (vocab == 0 && pitUnsafe == 0 && nFormsHere >= 3) ? 0 : 1;
        reps.push_back(measureLevel(seed, level, 3));
    }
    int missingNewForms = 0;
    for (int f = 1; f < LM_MEGA_FORM_COUNT; f++) if (globalFormLevels[f] == 0) missingNewForms++;
    std::sort(domMasks.begin(), domMasks.end());
    int distinctDomSets = (int)(std::unique(domMasks.begin(), domMasks.end()) - domMasks.begin());
    bool megaOk = (missingNewForms == 0 && minFormsOnAnyLevel >= 3 && distinctDomSets >= 6);
    printf("mega-form coverage across %d levels: missingNew=%d, minFormsPerLevel=%d, distinctDomPairs=%d/%d %s\n",
           nLv, missingNewForms, minFormsOnAnyLevel, distinctDomSets, nLv, megaOk ? "ok" : "FAIL");
    failed += megaOk ? 0 : 1;
    int badPairs = 0, sumXeno = 0, sumDead = 0, sumVit = 0;
    for (size_t a = 0; a < reps.size(); a++) {
        if (reps[a].xenoPct > 0.0) sumXeno++;
        sumDead += reps[a].deadThreshTotal;
        sumVit  += reps[a].vitrineTotal;
        for (size_t b = a + 1; b < reps.size(); b++) {
            if (dnaDistance(reps[a].dna, reps[b].dna) < 180 || metricDistance(reps[a], reps[b]) < 14.0) badPairs++;
        }
    }
    bool anomaliesPresent = (sumXeno >= 4 && sumDead >= 25 && sumVit >= 4);
    double allSober = 0.0, allPocket = 0.0, lowPocket = 0.0, highPocket = 0.0;
    int lowCnt = 0, highCnt = 0;
    for (const auto& r : reps) {
        allSober += r.delSoberPct;
        allPocket += r.delPocketPct;
        int alv = r.level < 0 ? (r.level == (-2147483647 - 1) ? 2147483647 : -r.level) : r.level;
        if (alv <= 1) { lowPocket += r.delPocketPct; lowCnt++; }
        if (alv >= 12) { highPocket += r.delPocketPct; highCnt++; }
    }
    double meanSober  = allSober / nLv;
    double meanPocket = allPocket / nLv;
    bool deliriumOk   = (meanSober >= 52.0 && meanPocket >= 1.5 &&
                         (highCnt > 0 && lowCnt > 0 && (highPocket / highCnt) > (lowPocket / lowCnt)));
    printf("delirium distribution (sober=%.1f%%, drunkPockets=%.1f%%, |L|<=1:%.1f%% -> |L|>=12:%.1f%%): %s\n",
           meanSober, meanPocket, lowCnt ? lowPocket / lowCnt : 0.0, highCnt ? highPocket / highCnt : 0.0,
           deliriumOk ? "ok" : "FAIL");
    printf("multi-level DNA & metric distinctness (%d levels): %s\n", nLv, badPairs == 0 ? "ok" : "FAIL");
    // Consecutive levels must sit at different poles on every axis, and a run of N levels must
    // visit all N poles of an axis (checked on three seeds, across zero and far from it).
    int samePole = 0, missedPole = 0, steps = 0, sameQuirk = 0, missedQuirk = 0;
    const u32 poleSeeds[3] = {seed, seed * 2654435761u + 17u, 1u};
    const int poleStarts[4] = {-40, 0, 977, 100480};
    for (u32 ps : poleSeeds) {
        for (int st : poleStarts) {
            LimenLevelDNA prev = lmMakeLevelDNA(ps, st - 1);
            unsigned seenW = 0, seenF = 0, seenL = 0, seenQ = 0;
            for (int lv = st; lv < st + 48; lv++) {
                LimenLevelDNA cur = lmMakeLevelDNA(ps, lv);
                steps++;
                if (cur.wallPole == prev.wallPole || cur.floorPole == prev.floorPole || cur.ceilPole == prev.ceilPole ||
                    cur.lightPole == prev.lightPole || (cur.spacePole >= 0 && cur.spacePole == prev.spacePole)) samePole++;
                if (cur.quirk == prev.quirk || (cur.quirk2 != LQ_NONE && (cur.quirk2 == cur.quirk || cur.quirk == LQ_NONE))) sameQuirk++;
                seenW |= 1u << cur.wallPole; seenF |= 1u << cur.floorPole; seenL |= 1u << cur.lightPole; seenQ |= 1u << cur.quirk;
                prev = cur;
            }
            if (seenW != (1u << LM_WALL_POLES) - 1u || seenF != (1u << LM_FLOOR_POLES) - 1u || seenL != (1u << LM_LIGHT_POLES) - 1u) missedPole++;
            if (seenQ != (1u << LM_QUIRK_COUNT) - 1u) missedQuirk++;
        }
    }
    LimenLevelDNA zero = lmMakeLevelDNA(seed, 0);
    bool classicZero = (zero.wallPole == 1 && zero.floorPole == 0 && zero.ceilPole == 0 && zero.lightPole == 0 && zero.spacePole < 0);
    bool polesOk = (samePole == 0 && missedPole == 0 && classicZero);
    printf("level poles (%d level steps): repeated on the next level=%d, runs missing a pole=%d, level 0 classic=%s: %s\n",
           steps, samePole, missedPole, classicZero ? "yes" : "no", polesOk ? "ok" : "FAIL");
    failed += polesOk ? 0 : 1;
    // Quirks are dealt the same way: never the same one twice in a row, every one of them in
    // any run, none on level 0, and the first deal on either side of zero is unmixed.
    bool firstDealPure = (zero.quirk == LQ_NONE && zero.quirk2 == LQ_NONE);
    unsigned firstSeen = 0;
    for (int lv = 0; lv < LM_QUIRK_COUNT; lv++) {
        LimenLevelDNA q = lmMakeLevelDNA(seed, lv);
        firstSeen |= 1u << q.quirk;
        if (q.quirk2 != LQ_NONE) firstDealPure = false;
    }
    if (firstSeen != (1u << LM_QUIRK_COUNT) - 1u) firstDealPure = false;
    bool quirksOk = (sameQuirk == 0 && missedQuirk == 0 && firstDealPure);
    printf("level quirks (%d kinds): repeated on the next level=%d, runs missing a quirk=%d, levels 0..%d show each once=%s: %s\n",
           LM_QUIRK_COUNT, sameQuirk, missedQuirk, LM_QUIRK_COUNT - 1, firstDealPure ? "yes" : "no", quirksOk ? "ok" : "FAIL");
    failed += quirksOk ? 0 : 1;
    printf("liminal anomalies across sample (xenoLv=%d/%d, deadThresh=%d, sealedRooms=%d): %s\n",
           sumXeno, nLv, sumDead, sumVit, anomaliesPresent ? "ok" : "FAIL");
    failed += badPairs + (anomaliesPresent ? 0 : 1) + (deliriumOk ? 0 : 1);
    return failed;
}

static void runStats(u32 seed, int level) {
    lmReset(seed + 1, level); lmReset(seed, level);
    int total = 0, fallbacks = 0, vaults = 0, sites[LS_SITE_COUNT] = {0}, tiers[13] = {0};
    int megaForms[LM_MEGA_FORM_COUNT] = {0}, escherSlabs = 0;
    long long delHist[3] = {0, 0, 0}, delStructTot = 0;
    int geneQuart[LG_COUNT][4] = {{0}};
    long long cells = 0, count[LT_TAG_COUNT] = {0}, lamps = 0, nodes = 0, links = 0, sealed = 0;
    long long loops = 0, thin = 0, narrow = 0, walkBasins = 0;
    int withPit = 0, withShell = 0, withRooms = 0, through = 0, withLoops = 0;
    std::vector<u32> signatures;
    auto t0 = std::chrono::steady_clock::now();
    for (int j = -12; j <= 12; j++) {
        for (int i = -12; i <= 12; i++) {
            static LimenSlab s;
            lmBuildSlab(s, j, i);
            lmPlaceLampSites(s);
            total++;
            if (s.fallback) fallbacks++;
            if (lmSlabIsVault(s)) {
                vaults++;
                if (s.megaForm >= 0 && s.megaForm < LM_MEGA_FORM_COUNT) megaForms[s.megaForm]++;
                if (!s.dark && lmSlabHasEscherKnotAt(j, i)) escherSlabs++;
            }
            tiers[s.ceilTiers]++;
            nodes += s.nodes; links += s.links; sealed += s.sealed;
            loops += s.loops; thin += s.thinCount; narrow += s.narrowCount; walkBasins += s.walkBasins;
            if (s.loops > 0) withLoops++;
            for (int k = 0; k < LG_COUNT; k++) geneQuart[k][s.genome.g[k] / 64]++;
            for (int k = 0; k < s.siteCount; k++) sites[s.sites[k].type]++;
            bool shell = false;
            delStructTot += (s.deliriumStruct & 15) + ((s.deliriumStruct >> 4) & 15);
            for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) {
                cells++;
                count[s.tag[x][z]]++;
                if (s.flag[x][z] & LF_LOW_WALL) shell = true;
                if (lmLampLocal(s, x, z)) lamps++;
                int dv = lmDeliriumCell255(s.rect.x0 + 1 + x, s.rect.z0 + 1 + z);
                delHist[dv <= 28 ? 0 : (dv < 115 ? 1 : 2)]++;
            }
            if (lmPitShare(s) > 0) withPit++;
            if (shell) withShell++;
            if (s.share[LT_ROOM] >= 5) withRooms++;
            if (lmSeamIsThrough(j, i, j, i - 1)) through++;
            u32 sig = (u32)(s.ceilTiers > 1 ? (s.ceilTiers > 6 ? 2 : 1) : 0);
            for (int t = 0; t < LT_TAG_COUNT; t++) sig = sig * 9u + (u32)(s.share[t] / 12);
            signatures.push_back(sig);
        }
    }
    double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / total;
    std::sort(signatures.begin(), signatures.end());
    int distinct = (int)(std::unique(signatures.begin(), signatures.end()) - signatures.begin());
    long long floorCells = cells - count[LT_WALL] - count[LT_VOID] - count[LT_BASIN];
    printf("seed %u level %d: %d slabs, %.1f us per slab, %.1f grammar nodes per slab\n", seed, level, total, us, (double)nodes / total);
    printf("  distinct coarse signatures: %d of %d slabs\n", distinct, total);
    printf("  vault %.1f%%, storeys 1..12:", 100.0 * vaults / total);
    for (int t = 1; t <= 12; t++) printf(" %d", tiers[t]);
    printf("\n  mega-forms (%d vault slabs):", vaults);
    for (int f = 0; f < LM_MEGA_FORM_COUNT; f++)
        printf(" %s=%d(%.1f%%)", lmMegaFormName(f), megaForms[f], 100.0 * megaForms[f] / total);
    printf("\n  delirium ('пьянка'): sober(<0.11)=%.1f%% tipsy(0.11..0.45)=%.1f%% drunk(>=0.45)=%.1f%% | matryoshka/spiral=%lld | escher=%d/%d(%.1f%% mega)",
           100.0 * delHist[0] / cells, 100.0 * delHist[1] / cells, 100.0 * delHist[2] / cells,
           delStructTot, escherSlabs, vaults, vaults ? (100.0 * escherSlabs / vaults) : 0.0);
    printf("\n  slabs with a drop %.1f%%, low/mezz shell %.1f%%, rooms %.1f%%, loops %.1f%% (avg %.1f), through seams %.1f%%\n",
           100.0 * withPit / total, 100.0 * withShell / total, 100.0 * withRooms / total,
           100.0 * withLoops / total, (double)loops / total, 100.0 * through / total);
    printf("  cells: wall %.1f corr %.1f room %.1f hall %.1f void %.1f bridge %.1f basin %.1f low %.1f\n",
           100.0 * count[0] / cells, 100.0 * count[1] / cells, 100.0 * count[2] / cells, 100.0 * count[3] / cells,
           100.0 * count[4] / cells, 100.0 * count[5] / cells, 100.0 * count[6] / cells, 100.0 * count[7] / cells);
    printf("  human-scale & stairs: thin-partition cells %lld, narrow-corridor cells %lld, walkable basin cells %lld (%.0f%% of basin)\n",
           thin, narrow, walkBasins, count[LT_BASIN] ? (100.0 * walkBasins / count[LT_BASIN]) : 0.0);
    printf("  repair: %.2f links per slab, %.1f%% of floor sealed, fallbacks %d\n",
           (double)links / total, 100.0 * sealed / (double)(floorCells + sealed), fallbacks);
    printf("  lamps per 100 floor cells %.1f\n", 100.0 * lamps / floorCells);
    printf("  sites: door gap %d, door shift %d, dead lamp %d, dark slab %d, rail gap %d, dyn lamp %d\n",
           sites[0], sites[1], sites[2], sites[3], sites[4], sites[5]);
    printf("  gene quartiles (0-63 / 64-127 / 128-191 / 192-255):\n");
    for (int k = 0; k < LG_COUNT; k++)
        printf("    %-8s %3d %3d %3d %3d\n", lmGeneName(k), geneQuart[k][0], geneQuart[k][1], geneQuart[k][2], geneQuart[k][3]);
}

static void runSlab(u32 seed, int level, int j, int i) {
    lmReset(seed + 1, level); lmReset(seed, level);
    const LimenSlab& s = lmSlab(j, i);
    printf("slab (%d,%d) %dx%d cells, ceiling %d storeys (%s sub=%d), palette %d, %d nodes, %d links, %d loops, %d thin, %d narrow, %d walkBasin%s\n",
           j, i, s.w, s.h, s.ceilTiers, lmMegaFormName(s.megaForm), s.megaSub, s.palette, s.nodes, s.links, s.loops, s.thinCount, s.narrowCount, s.walkBasins, s.fallback ? ", FALLBACK" : "");
    for (int k = 0; k < LG_COUNT; k++) printf("  %-8s %3d\n", lmGeneName(k), s.genome.g[k]);
    static const char glyph[LT_TAG_COUNT + 1] = "#.r ~=u,v";
    for (int z = 0; z < s.h; z++) {
        for (int x = 0; x < s.w; x++) {
            char ch = glyph[s.tag[x][z]];
            if (s.tag[x][z] == LT_WALL && (s.flag[x][z] & LF_LOW_WALL)) ch = (s.flag[x][z] & LF_MEZZ) ? 'M' : '%';
            else if (s.tag[x][z] == LT_BASIN && (s.flag[x][z] & LF_STAIR_DESC)) ch = 'S';
            putchar(ch);
        }
        putchar('\n');
    }
}

static void runFind(u32 seed, int level) {
    lmReset(seed + 1, level); lmReset(seed, level);
    bool foundStair = false, foundMezz = false, foundThin = false;
    bool foundDrunk = false, foundEscher = false, foundNested = false, foundSpiral = false;
    bool foundMega[LM_MEGA_FORM_COUNT] = {};
    for (int ring = 1; ring <= 14; ring++) {
        for (int j = -ring; j <= ring; j++) {
            for (int i = -ring; i <= ring; i++) {
                if (lmDepth(j, i) != ring) continue;
                const LimenSlab s = lmSlab(j, i);
                if (!foundDrunk && s.deliriumPeak >= 155) {
                    printf("drunk_pocket:        slab (%d,%d) peak=%.2f mean=%.2f world (%.1f, %.1f)\n",
                           j, i, s.deliriumPeak / 255.0f, s.deliriumMean / 255.0f,
                           (s.rect.x0 + 1 + s.w * 0.5f) * 5.0f, (s.rect.z0 + 1 + s.h * 0.5f) * 5.0f);
                    foundDrunk = true;
                }
                if (!foundEscher && lmSlabIsVault(s) && !s.dark && lmSlabHasEscherKnotAt(j, i)) {
                    printf("drunk_escher:        slab (%d,%d) %d storeys world (%.1f, %.1f)\n",
                           j, i, s.ceilTiers,
                           (s.rect.x0 + 1 + s.w * 0.5f) * 5.0f, (s.rect.z0 + 1 + s.h * 0.5f) * 5.0f);
                    foundEscher = true;
                }
                if (!foundNested && s.nestedRings >= 2 && s.nestedX >= 0) {
                    printf("drunk_nested:        slab (%d,%d) rings=%d world (%.1f, %.1f)\n",
                           j, i, (int)s.nestedRings,
                           (s.rect.x0 + 1.5f + s.nestedX) * 5.0f, (s.rect.z0 + 1.5f + s.nestedZ) * 5.0f);
                    foundNested = true;
                }
                if (!foundSpiral && ((s.deliriumStruct >> 4) & 15) >= 2 && s.spiralX >= 0) {
                    printf("drunk_spiral:        slab (%d,%d) turns=%d world (%.1f, %.1f)\n",
                           j, i, (s.deliriumStruct >> 4) & 15,
                           (s.rect.x0 + 1.5f + s.spiralX) * 5.0f, (s.rect.z0 + 1.5f + s.spiralZ) * 5.0f);
                    foundSpiral = true;
                }
                if (lmSlabIsVault(s) && s.megaForm >= 0 && s.megaForm < LM_MEGA_FORM_COUNT && !foundMega[s.megaForm]) {
                    printf("mega_%-15s slab (%d,%d) %d storeys world (%.1f, %.1f)\n",
                           lmMegaFormName(s.megaForm), j, i, s.ceilTiers,
                           (s.rect.x0 + 1 + s.w * 0.5f) * 5.0f, (s.rect.z0 + 1 + s.h * 0.5f) * 5.0f);
                    foundMega[s.megaForm] = true;
                }
                if (!foundStair && s.walkBasins >= 8) {
                    for (int x = 0; x < s.w && !foundStair; x++) for (int z = 0; z < s.h; z++) {
                        if (s.flag[x][z] & LF_STAIR_DESC) {
                            printf("walkable_basin_stair: slab (%d,%d) cell (%d,%d) world (%.1f, %.1f)\n",
                                   j, i, s.rect.x0 + 1 + x, s.rect.z0 + 1 + z, (s.rect.x0 + 1.5f + x) * 5.0f, (s.rect.z0 + 1.5f + z) * 5.0f);
                            foundStair = true; break;
                        }
                    }
                }
                if (!foundMezz) {
                    for (int x = 0; x < s.w && !foundMezz; x++) for (int z = 0; z < s.h; z++) {
                        if (s.flag[x][z] & LF_MEZZ) {
                            printf("mezzanine_9m_shell:  slab (%d,%d) cell (%d,%d) world (%.1f, %.1f)\n",
                                   j, i, s.rect.x0 + 1 + x, s.rect.z0 + 1 + z, (s.rect.x0 + 1.5f + x) * 5.0f, (s.rect.z0 + 1.5f + z) * 5.0f);
                            foundMezz = true; break;
                        }
                    }
                }
                if (!foundThin && s.thinCount >= 16 && s.narrowCount >= 6) {
                    printf("thin_clone_rooms:    slab (%d,%d) thin=%d narrow=%d world (%.1f, %.1f)\n",
                           j, i, s.thinCount, s.narrowCount, (s.rect.x0 + s.w * 0.5f) * 5.0f, (s.rect.z0 + s.h * 0.5f) * 5.0f);
                    foundThin = true;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc >= 5 && strcmp(argv[1], "sound") == 0) {
        runSound((u32)strtoul(argv[2], 0, 10), atoi(argv[3]), argv[4], argc >= 6 ? (float)atof(argv[5]) : 8.0f);
        return 0;
    }
    if (argc >= 9 && strcmp(argv[1], "map") == 0) {
        lmReset((u32)strtoul(argv[2], 0, 10) + 1, 0);
        lmReset((u32)strtoul(argv[2], 0, 10), atoi(argv[3]));
        writeMap(argv[8], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), argc >= 10 ? atoi(argv[9]) : 4);
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "test") == 0) {
        int failed = runTests((u32)strtoul(argv[2], 0, 10));
        failed += runSoundTests((u32)strtoul(argv[2], 0, 10));
        failed += runNarratorTests((u32)strtoul(argv[2], 0, 10));
        printf(failed ? "FAILED: %d\n" : "all tests passed\n", failed);
        return failed ? 1 : 0;
    }
    if (argc >= 4 && strcmp(argv[1], "sweep") == 0) {
        return runSweep((u32)strtoul(argv[2], 0, 10), atoi(argv[3])) ? 1 : 0;
    }
    if (argc >= 4 && strcmp(argv[1], "levels") == 0) {
        return runLevels((u32)strtoul(argv[2], 0, 10), atoi(argv[3])) ? 1 : 0;
    }
    if (argc >= 4 && strcmp(argv[1], "stats") == 0) {
        runStats((u32)strtoul(argv[2], 0, 10), atoi(argv[3]));
        return 0;
    }
    if (argc >= 6 && strcmp(argv[1], "slab") == 0) {
        runSlab((u32)strtoul(argv[2], 0, 10), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "narbench") == 0) {
        runNarBench((u32)strtoul(argv[2], 0, 10), argc >= 4 ? atoi(argv[3]) : 6, argc >= 5 ? atoi(argv[4]) : 6, argc >= 6 ? (float)atof(argv[5]) : 6.0f);
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "narfit") == 0) { runNarFit((u32)strtoul(argv[2], 0, 10), atoi(argv[3])); return 0; }
    if (argc >= 4 && strcmp(argv[1], "tour") == 0) {
        runTour((u32)strtoul(argv[2], 0, 10), atoi(argv[3]), argc >= 5 ? atoi(argv[4]) : 36);
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "roams") == 0) {
        runRoams((u32)strtoul(argv[2], 0, 10), atoi(argv[3]), argc >= 5 ? (float)atof(argv[4]) : 6.0f);
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "roam") == 0) {
        runRoam((u32)strtoul(argv[2], 0, 10), atoi(argv[3]), argc >= 5 ? (float)atof(argv[4]) : 15.0f);
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "find") == 0) {
        runFind((u32)strtoul(argv[2], 0, 10), atoi(argv[3]));
        return 0;
    }
    printf("usage: limen_tool map|test|sweep|levels|stats|slab|find|tour|roam|roams|narbench|narfit ...\n");
    return 1;
}
