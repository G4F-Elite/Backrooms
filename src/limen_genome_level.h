#pragma once
// Part of limen_genome.h: deriving the DNA of a level.

// Returns the level's ceilGridStrength (the suspended T-bar grid is a mesh, not a texture).
inline int lmApplyCeilPole(LmSurfDNA& s, int pole, u32 seed, int level, int hue, int sat, int val) {
    LmPoleRng R{seed, level, 980u};
    lmSurfSecondPigment(s, hue, sat, val - R(24, 40));
    switch (pole) {
    case 0: // acoustic tile in a T-bar grid
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(R(2, 4));
        s.seamDepth = (unsigned char)R(30, 70); s.stripe = (unsigned char)R(0, 50);
        s.grain = (unsigned char)R(130, 190);   s.speckle = (unsigned char)R(50, 110);
        s.sheen = (unsigned char)R(0, 60);
        return R(150, 250);
    case 1: // flat plaster
        s.seamDepth = (unsigned char)R(0, 16); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(50, 110);   s.sheen = (unsigned char)R(10, 90);
        return 0;
    case 2: // raw slab
        s.seamFreqX = lmSeamFreqForRepeats(R(1, 2)); s.seamFreqY = lmSeamFreqForRepeats(R(1, 3));
        s.seamDepth = (unsigned char)R(110, 170); s.stripe = (unsigned char)R(0, 50);
        s.grain = (unsigned char)R(170, 230);     s.sheen = (unsigned char)R(0, 60);
        s.grime = (unsigned char)lmClamp255((int)s.grime + R(40, 90));
        return 0;
    case 3: // slats
        s.seamFreqX = lmSeamFreqForRepeats(R(13, 16)); s.seamFreqY = 0;
        s.plank = (unsigned char)R(170, 240);
        s.seamDepth = (unsigned char)R(130, 180); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(60, 120);      s.sheen = (unsigned char)R(30, 110);
        return 0;
    default: // pressed coffers under a sparse metal grid
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(R(3, 5));
        s.seamDepth = (unsigned char)R(175, 235); s.stripe = (unsigned char)R(0, 50);
        s.grain = (unsigned char)R(30, 80);       s.sheen = (unsigned char)R(140, 220);
        return R(90, 140);
    }
}

inline void lmApplyQuirkToDNA(LimenLevelDNA& d, int q, int var, u32 seed, int level, bool isMain);

inline LimenLevelDNA lmMakeLevelDNA(u32 seed, int level, int forceQuirk = -1) {
    LimenLevelDNA d{};
    u32 absL = (level < 0) ? (0u - (u32)level) : (u32)level;
    int sat3 = (absL >= 3u) ? 300 : (int)(absL * 100u);
    // Level 0 is the one familiar place (yellow wallpaper, damp carpet, calm start). Everything
    // else is its own level at full strength from the first step out of it.
    bool classicLvl = (absL == 0u);
    d.originCalmRings = classicLvl ? 2 : 0;
    int amp = classicLvl ? 38 : (absL == 1u ? 88 : 100);
    int classicBlend = classicLvl ? 88 : 0;

    d.wallPole  = (signed char)lmLvlPick(seed, level, 900u, LM_WALL_POLES, 1);
    d.floorPole = (signed char)lmLvlPick(seed, level, 901u, LM_FLOOR_POLES, 0);
    d.ceilPole  = (signed char)lmLvlPick(seed, level, 902u, LM_CEIL_POLES, 0);
    d.lightPole = (signed char)lmLvlPick(seed, level, 903u, LM_LIGHT_POLES, 0);
    d.spacePole = classicLvl ? (signed char)-1 : (signed char)lmLvlPick(seed, level, 904u, LM_SPACE_POLES);

    // 1. Permute 16 genes and push 3..5 of them into extreme regimes (up to +-90)
    int perm[LG_COUNT];
    for (int k = 0; k < LG_COUNT; k++) perm[k] = k;
    for (int k = LG_COUNT - 1; k > 0; k--) {
        int sw = (int)(lmLvlHash(seed, level, 10u + (u32)k) % (u32)(k + 1));
        int tmp = perm[k]; perm[k] = perm[sw]; perm[sw] = tmp;
    }
    int nExtreme = 3 + (int)(lmLvlHash(seed, level, 1u) % 3u);
    for (int idx = 0; idx < LG_COUNT; idx++) {
        int g = perm[idx];
        u32 hg = lmLvlHash(seed, level, 30u + (u32)g);
        int bias = 0, gain = 128;
        if (idx < nExtreme) {
            int sgn = ((hg >> 7) & 1u) ? 1 : -1;
            int mag = 68 + (int)(hg % 23u);
            bias = sgn * (mag * amp / 100);
            gain = 76 + (int)((hg >> 9) % 34u);
        } else {
            int rawB = (int)(hg % 89u) - 44;
            bias = rawB * amp / 100;
            gain = 96 + (int)((hg >> 9) % 125u);
        }
        d.geneBias[g]   = (signed char)lmClampInt(bias, -90, 90);
        d.geneGain[g]   = (unsigned char)lmClampInt(gain, 72, 224);
        d.genePeriod[g] = (unsigned char)(1 + (int)((hg >> 18) % 7u));
        d.smoothMix[g]  = (unsigned char)(1 + (int)((hg >> 24) % 4u));
    }
    // The kind of space: one or two genes the plan reads first are driven to their limit, so
    // the next level is a different sort of building, not the same one with other numbers.
    if (d.spacePole >= 0) {
        static const signed char poleGenes[LM_SPACE_POLES][3][2] = {
            {{LG_HEIGHT, 1},  {LG_SPAN, 1},    {-1, 0}},          // vaults
            {{LG_HEIGHT, -1}, {LG_CELLS, 1},   {LG_GRAIN, -1}},   // warren of low rooms
            {{LG_GRAIN, 1},   {LG_CELLS, -1},  {-1, 0}},          // open halls
            {{LG_LATTICE, 1}, {LG_CELLS, -1},  {LG_ORDER, 1}},    // pillar fields
            {{LG_SINK, 1},    {LG_SPAN, 1},    {-1, 0}},          // drops and bridges
            {{LG_GRAIN, -1},  {LG_HEIGHT, -1}, {-1, 0}},          // closets
            {{LG_SHELL, 1},   {LG_GRAIN, 1},   {-1, 0}},          // buildings inside buildings
            {{LG_MASS, 1},    {LG_LATTICE, -1},{LG_EDGE, -1}}     // solid mass, sealed slabs
        };
        for (int q = 0; q < 3; q++) {
            int g = poleGenes[d.spacePole][q][0];
            if (g < 0) continue;
            u32 hq = lmLvlHash(seed, level, 50u + (u32)q);
            int mag = (q == 0 ? 74 : 56) + (int)(hq % 17u);
            d.geneBias[g] = (signed char)lmClampInt(poleGenes[d.spacePole][q][1] * mag * amp / 100, -90, 90);
            d.geneGain[g] = (unsigned char)(76 + (int)((hq >> 9) % 34u));
        }
    }
    if (d.geneBias[LG_MASS] > 66) d.geneBias[LG_MASS] = 66;
    if (d.geneBias[LG_MASS] > 42 && d.geneBias[LG_CELLS] < -50) d.geneBias[LG_CELLS] = -45;

    // 2. Structural & spatial scales
    u32 hs0 = lmLvlHash(seed, level, 60u);
    u32 hs1 = lmLvlHash(seed, level, 61u);
    int grainPush = d.geneBias[LG_GRAIN] / 12;
    d.rowPitch = lmClampInt(22 + (int)(hs0 % 17u) + grainPush, 20, 40);
    d.colPitch = lmClampInt(22 + (int)((hs0 >> 8) % 17u) + grainPush, 20, 40);

    int vBase = classicLvl ? 236
              : (196 - (int)d.geneBias[LG_HEIGHT] * 3 / 5
                     - (d.geneBias[LG_SINK] > 45 ? d.geneBias[LG_SINK] / 4 : 0)
                     + (((int)((hs0 >> 16) % 37u) - 18) * amp / 100));
    if (d.spacePole == 0) vBase -= 28; // 'vaults' pole: ~26..34% monumental vaults, never 100% hangar soup
    d.vaultThreshold = lmClampInt(vBase, 148, 244);

    int stBase = 4 + (int)(hs1 % 8u) + d.geneBias[LG_HEIGHT] / 20;
    d.maxStoreys = lmClampInt(stBase, 2, 12);
    d.transitPermille = lmClampInt(460 + d.geneBias[LG_SPAN] * 3 + ((int)((hs1 >> 8) % 241u) - 120), 140, 860);
    d.throughEdgeThresh = lmClampInt(315 - d.geneBias[LG_EDGE] + ((int)((hs1 >> 17) % 71u) - 35), 210, 420);
    d.pillarLatticeThresh = lmClampInt(104 - d.geneBias[LG_LATTICE] / 2 + ((int)((hs1 >> 24) % 41u) - 20), 52, 180);

    // 3. Harmonized everyday liminal palette (heavy-tail muted S, single-accent cap, floor < wall < ceil)
    u32 hl0 = lmLvlHash(seed, level, 80u);
    u32 hl1 = lmLvlHash(seed, level, 81u);
    u32 hp0 = lmLvlHash(seed, level, 200u);
    u32 hp1 = lmLvlHash(seed, level, 210u);
    u32 hp2 = lmLvlHash(seed, level, 220u);
    u32 hp3 = lmLvlHash(seed, level, 230u);

    // Lamp spectrum poles: {hue, hue span, sat, sat span, ambient fill, lamp output,
    // fog luminance %, fog density shift, lamp density shift}
    static const short lightPoles[LM_LIGHT_POLES][9] = {
        { 36, 13,  22, 34, 128, 128, 100,   0,   0},   // warm phosphor tube
        {196, 20,  16, 30, 162, 150, 125, -15,  25},   // cool office daylight
        {114, 40,  30, 36, 100, 118,  90,  15, -10},   // aged greenish ballast
        { 24, 11, 100, 50,  80, 128,  70,  10, -35},   // sodium amber, deep shadow
        {205, 21,  70, 40,  86, 114,  80,  25, -25},   // mercury blue murk
        { 40, 20,   4, 12, 168, 142, 330,  40,  35},   // overlit, white haze
        {  0, 11, 105, 40,  82, 132,  60,   5, -28}    // red safelight
    };
    const short* lpole = lightPoles[d.lightPole];
    d.ambientGain = (unsigned char)lmClampInt(128 + (lpole[4] - 128) * amp / 100 + (int)((hl0 >> 24) % 17u) - 8, 60, 210);
    d.lampGain    = (unsigned char)lmClampInt(128 + (lpole[5] - 128) * amp / 100, 100, 160);

    int lDens = 136 + (int)d.geneBias[LG_LIGHT] * 5 / 4 + (((int)(hl0 % 61u) - 30) * amp / 100) + lpole[8] * amp / 100;
    d.lightDensity = (unsigned char)lmClamp255(lDens);
    int darkBase = (d.lightDensity < 85 ? (85 - d.lightDensity) / 2 : 0) +
                   (d.geneBias[LG_DECAY] > 30 ? d.geneBias[LG_DECAY] / 6 : 0) +
                   (int)((hl0 >> 8) % 8u) * amp / 100;
    d.darkSlabChance = (unsigned char)lmClampInt(darkBase, 0, 52);

    // The pigment wheel is walked in golden-ratio steps from level 0's cream, so the next level
    // always lands in another family of colour (warm <-> cool), whatever the seed.
    int hueStep = (lmLvlHash(seed, 0, 90u) & 1u) ? 618 : 382;
    int basePm  = 150 + (int)lmMod((int)(((long long)level * hueStep) % 1000LL), 1000) + (int)((hl0 >> 14) % 141u) - 70;
    int baseHue = lmArchHueFromPermille(basePm);
    bool analogous = ((hp3 & 7u) < 5u); // 62.5% analogous family, 37.5% neutral + single accent
    int anaStep = ((hp3 >> 4) & 1u) ? (14 + (int)((hp3 >> 5) % 28u)) : -(14 + (int)((hp3 >> 5) % 28u));
    int accHue  = lmArchHueFromPermille(basePm + 210 + (int)((hp3 >> 12) % 520u));

    int wallSat  = lmHeavyTailSat255(hp0);
    int floorSat = lmHeavyTailSat255(hp1);
    int ceilSat  = 6 + (int)(hp2 % 22u); // S = 0.02..0.11 light neutral mineral ceiling

    int wallHue  = baseHue;
    int floorHue = analogous ? ((baseHue + anaStep + 360) % 360) : accHue;
    int ceilHue  = ((hp2 >> 8) & 1u) ? wallHue : 44;

    // Enforce strict harmony: at most ONE surface (usually floor carpet/linoleum, rarely wall) may exceed S=0.33
    bool hasStrongAccent = (wallSat > 86 || floorSat > 86 || !analogous);
    if (hasStrongAccent) {
        bool accentOnFloor = ((hp3 >> 20) % 10u) < 7u; // 70% floor accent + neutral wall, 30% painted wall + neutral floor
        if (accentOnFloor) {
            floorSat = (floorSat > wallSat) ? floorSat : wallSat;
            wallSat  = 12 + (int)((hp0 >> 18) % 32u); // calm plaster/concrete neutral wall (S = 0.05..0.17)
        } else {
            wallSat  = lmClampInt((wallSat > floorSat) ? wallSat : floorSat, 20, 126); // painted wall capped <= 0.49
            floorHue = (wallHue + anaStep / 2 + 360) % 360;
            floorSat = 14 + (int)((hp1 >> 18) % 34u); // dark neutral linoleum/terrazzo floor (S = 0.05..0.19)
        }
    } else if (wallSat > 68 && floorSat > 68) {
        // Even in muted analogous mode, avoid two medium-chroma surfaces competing at once
        if ((hp3 >> 24) & 1u) wallSat = 22 + (wallSat % 36);
        else floorSat = 24 + (floorSat % 36);
    }

    int wallVal  = 148 + (int)((hp0 >> 20) % 68u);                          // V = 0.58..0.84

    // Materials that bring their own pigment: fired clay, timber, cement, bare steel.
    u32 hm0 = lmLvlHash(seed, level, 890u);
    u32 hm1 = lmLvlHash(seed, level, 891u);
    u32 hm2 = lmLvlHash(seed, level, 892u);
    if (d.wallPole == 4 && (hm0 % 3u) != 0u) {          // red/brown brick (else painted over)
        wallHue = 8 + (int)((hm0 >> 4) % 17u); wallSat = 96 + (int)((hm0 >> 9) % 44u); wallVal = 104 + (int)((hm0 >> 16) % 46u);
    } else if (d.wallPole == 5) {
        if (wallSat > 26) wallSat = 26;
        wallVal = 118 + (int)((hm0 >> 16) % 54u);
    } else if (d.wallPole == 6) {
        wallHue = 18 + (int)((hm0 >> 4) % 17u); wallSat = 92 + (int)((hm0 >> 9) % 48u); wallVal = 92 + (int)((hm0 >> 16) % 60u);
    } else if (d.wallPole == 7) {
        if (wallSat > 44) wallSat = 44;
        wallVal = 120 + (int)((hm0 >> 16) % 60u);
    }
    if (wallSat > 86 && floorSat > 60) floorSat = 24 + (floorSat % 36);

    int floorDrop = 32 + (int)((hp1 >> 20) % 44u);
    int floorVal = lmClampInt(wallVal - floorDrop, 76, 164);                // V = 0.30..0.64, darker than a light wall
    if (wallVal < 136) floorVal = ((hm1 >> 28) & 1u) ? (150 + (int)((hm1 >> 20) % 36u)) : lmClampInt(wallVal - 26, 62, 120);
    if (d.floorPole == 4) {                              // timber
        floorHue = 18 + (int)((hm1 >> 4) % 17u); floorSat = 84 + (int)((hm1 >> 9) % 46u); floorVal = 80 + (int)((hm1 >> 16) % 56u);
        if (wallSat > 70) wallSat = 34 + (wallSat % 36);
    } else if (d.floorPole == 5) {
        if (floorSat > 26) floorSat = 26;
        floorVal = 96 + (int)((hm1 >> 16) % 54u);
    }

    int ceilVal  = lmClampInt(wallVal + 14 + (int)((hp2 >> 16) % 28u), 186, 228); // V = 0.73..0.89 light ceiling
    if (d.ceilPole == 2) {                               // a raw slab hangs dark overhead
        if (ceilSat > 20) ceilSat = 20;
        ceilVal = 112 + (int)((hm2 >> 16) % 48u);
    } else if (d.ceilPole == 3 && (hm2 & 1u)) {          // timber slats (else painted)
        ceilHue = 20 + (int)((hm2 >> 4) % 15u); ceilSat = 70 + (int)((hm2 >> 9) % 40u); ceilVal = 106 + (int)((hm2 >> 16) % 54u);
    }

    d.wallSurf  = lmBuildSurfFromHSV(seed, level, 200u, wallHue,  wallSat,  wallVal,  d.geneBias[LG_ORDER], d.geneBias[LG_DECAY], classicBlend, 194, 180, 124);
    d.floorSurf = lmBuildSurfFromHSV(seed, level, 210u, floorHue, floorSat, floorVal, d.geneBias[LG_ORDER], d.geneBias[LG_DECAY], classicBlend, 138, 124, 96);
    d.ceilSurf  = lmBuildSurfFromHSV(seed, level, 220u, ceilHue,  ceilSat,  ceilVal,  d.geneBias[LG_ORDER], d.geneBias[LG_DECAY], classicBlend, 214, 208, 190);
    {
        // the second pigment follows the pigment actually on the surface (level 0 is blended)
        float fh, fs, fv;
        lmRGBToHSV01(d.wallSurf.rgb, fh, fs, fv);
        lmApplyWallPole(d.wallSurf, d.wallPole, seed, level, (int)fh, (int)(fs * 255.0f), (int)(fv * 255.0f));
        lmRGBToHSV01(d.floorSurf.rgb, fh, fs, fv);
        d.floorSeamStrength = (unsigned char)lmApplyFloorPole(d.floorSurf, d.floorPole, seed, level, (int)fh, (int)(fs * 255.0f), (int)(fv * 255.0f));
        lmRGBToHSV01(d.ceilSurf.rgb, fh, fs, fv);
        d.ceilGridStrength = (unsigned char)lmApplyCeilPole(d.ceilSurf, d.ceilPole, seed, level, (int)fh, (int)(fs * 255.0f), (int)(fv * 255.0f));
    }

    for (int p = 0; p < LM_MEGA_POLES; p++) {
        u32 mpH = lmLvlHash(seed, level, 240u + (u32)p);
        int pHue = wallHue, pSat = 24, pVal = 152;
        if (p == 0) {
            // Pole 0: architectural coffered concrete / dressed stone
            pHue = (wallHue + (int)(mpH % 19u) - 9 + 360) % 360;
            pSat = lmClampInt(wallSat / 2 + 12 + (int)((mpH >> 8) % 24u), 14, 58);
            pVal = lmClampInt(142 + (int)((mpH >> 16) % 44u), 132, 194);
        } else if (p == 1) {
            // Pole 1: warm brick / terracotta / ochre-panel masonry
            pHue = lmArchHueFromPermille(basePm + 260 + (int)(mpH % 220u));
            pSat = lmClampInt(46 + (int)((mpH >> 8) % 54u), 42, 102);
            pVal = lmClampInt(126 + (int)((mpH >> 16) % 52u), 118, 184);
        } else {
            // Pole 2: glazed institutional ceramic tile / painted steel / slate
            pHue = lmArchHueFromPermille(basePm + 540 + (int)(mpH % 240u));
            pSat = lmClampInt(42 + (int)((mpH >> 8) % 52u), 38, 96);
            pVal = lmClampInt(138 + (int)((mpH >> 16) % 54u), 128, 198);
        }
        LmSurfDNA mp = lmBuildSurfFromHSV(seed, level, 250u + (u32)(p * 4), pHue, pSat, pVal,
                                          d.geneBias[LG_ORDER], d.geneBias[LG_DECAY], classicBlend / 2, 186, 174, 128);
        if (p == 0) {
            mp.seamFreqX = mp.seamFreqY = (unsigned char)(48 + (mpH % 48u));
            mp.seamDepth = (unsigned char)lmClampInt((int)mp.seamDepth + 55, 145, 225);
        } else if (p == 1) {
            mp.seamFreqX = (unsigned char)(140 + (mpH % 70u));
            mp.seamFreqY = (unsigned char)(200 + ((mpH >> 6) % 50u));
            mp.stripe    = (unsigned char)lmClampInt((int)mp.stripe + 60, 125, 240);
            mp.seamDepth = (unsigned char)lmClampInt((int)mp.seamDepth + 65, 165, 238);
        } else {
            mp.seamFreqX = mp.seamFreqY = (unsigned char)(175 + (mpH % 68u));
            mp.seamDepth = (unsigned char)lmClampInt((int)mp.seamDepth + 70, 170, 240);
            mp.sheen     = (unsigned char)lmClampInt((int)mp.sheen + 85, 160, 245);
        }
        d.megaPoles[p] = mp;
    }

    // Harmonized fluorescent / institutional discharge tube spectrum (warm / cool / aged greenish)
    int lightHue = (lpole[0] + (int)((hl1 >> 4) % (u32)lpole[1])) % 360;
    int lightSat = lpole[2] + (int)((hl1 >> 9) % (u32)lpole[3]);
    lmHueSatValToRGB(lightHue, lightSat, 255, d.lightRGB);
    if (classicBlend > 0) {
        d.lightRGB[0] = (unsigned char)((d.lightRGB[0] * (100 - classicBlend) + 255 * classicBlend) / 100);
        d.lightRGB[1] = (unsigned char)((d.lightRGB[1] * (100 - classicBlend) + 240 * classicBlend) / 100);
        d.lightRGB[2] = (unsigned char)((d.lightRGB[2] * (100 - classicBlend) + 206 * classicBlend) / 100);
    }

    int fogScale = (18 + (int)d.lightDensity / 8) * lpole[6] / 100;
    for (int c = 0; c < 3; c++) {
        int mixC = ((int)d.wallSurf.rgb[c] * 42 + (int)d.floorSurf.rgb[c] * 18 + (int)d.lightRGB[c] * 40) / 100;
        int fv = (mixC * fogScale) / 255 + 8;
        if (classicBlend > 0) {
            const int cFog[3] = {42, 36, 22};
            fv = (fv * (100 - classicBlend) + cFog[c] * classicBlend) / 100;
        }
        d.fogRGB[c] = (unsigned char)lmClampInt(fv, 8, 150);
    }
    d.fogDensity     = (unsigned char)lmClamp255(84 + ((int)(lmLvlHash(seed, level, 82u) % 135u) - 55 + lpole[7]) * amp / 100);
    d.megaFogDensity = (unsigned char)lmClamp255(72 + ((int)(lmLvlHash(seed, level, 83u) % 140u) - 55) * amp / 100);

    d.reverbBias = (signed char)lmClampInt((d.geneBias[LG_HEIGHT] / 4) + ((int)d.wallSurf.sheen - 128) / 8, -35, 35);
    d.humGain    = (unsigned char)lmClampInt(45 + (int)d.lightDensity * 3 / 5 + (int)(lmLvlHash(seed, level, 84u) % 45u), 15, 220);

    // 4. Continuous prop theme weights (2..3 clear peaks + contrasting alien graft weights)
    int tperm[LM_THEME_COUNT];
    for (int t = 0; t < LM_THEME_COUNT; t++) tperm[t] = t;
    for (int t = LM_THEME_COUNT - 1; t > 0; t--) {
        int sw = (int)(lmLvlHash(seed, level, 300u + (u32)t) % (u32)(t + 1));
        int tmp = tperm[t]; tperm[t] = tperm[sw]; tperm[sw] = tmp;
    }
    u32 ht = lmLvlHash(seed, level, 320u);
    int nPeaks = 2 + (int)(ht & 1u);
    d.domThemeMask = 0u;
    d.graftThemeMask = 0u;
    for (int rank = 0; rank < LM_THEME_COUNT; rank++) {
        int t = tperm[rank];
        u32 hr = lmLvlHash(seed, level, 330u + (u32)rank);
        int w = 2 + (int)(hr % 6u);
        if (rank == 0)                     w = 200 + (int)(hr % 56u);
        else if (rank == 1)                w = 125 + (int)(hr % 68u);
        else if (rank == 2 && nPeaks == 3) w = 78 + (int)(hr % 58u);
        else if (rank <= nPeaks + 1)       w = 16 + (int)(hr % 26u);
        d.themeWeight[t] = (unsigned char)lmClampInt(w, 1, 255);
        if (rank < nPeaks) d.domThemeMask |= (1u << t);

        int gw = 2 + (int)((hr >> 8) % 5u);
        if (rank == LM_THEME_COUNT - 1)      { gw = 195 + (int)((hr >> 8) % 60u); d.graftThemeMask |= (1u << t); }
        else if (rank == LM_THEME_COUNT - 2) { gw = 120 + (int)((hr >> 8) % 65u); d.graftThemeMask |= (1u << t); }
        d.graftWeight[t] = (unsigned char)lmClampInt(gw, 1, 255);
    }

    u32 hp = lmLvlHash(seed, level, 360u);
    int pDens = 120 + d.geneBias[LG_USE] * 3 / 5 + d.geneBias[LG_CELLS] / 3 + ((int)(hp % 81u) - 40);
    d.propDensity = (unsigned char)lmClampInt(pDens, 28, 240);
    d.roomFurnish = (unsigned char)lmClampInt(135 + d.geneBias[LG_ORDER] / 3 + ((int)((hp >> 8) % 81u) - 40), 45, 245);
    d.poiChance   = (unsigned char)lmClampInt(42 + d.geneBias[LG_USE] / 4 + ((int)((hp >> 16) % 37u) - 18), 12, 85);

    // 5. Continuous Stage-2 liminal proclivities
    unsigned char* tendencies[8] = {
        &d.lackTendency, &d.faceWallTendency, &d.twinMutTendency, &d.monoFloodTendency,
        &d.waitVoidTendency, &d.xenograftTendency, &d.deadThreshTendency, &d.phaseShiftTendency
    };
    int aperm[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int k = 7; k > 0; k--) {
        int sw = (int)(lmLvlHash(seed, level, 400u + (u32)k) % (u32)(k + 1));
        int tmp = aperm[k]; aperm[k] = aperm[sw]; aperm[sw] = tmp;
    }
    int uncannyScale = 54 + sat3 * 46 / 300;
    for (int rank = 0; rank < 8; rank++) {
        u32 ha = lmLvlHash(seed, level, 420u + (u32)rank);
        int raw = (rank < 2) ? (185 + (int)(ha % 70u))
                : (rank < 5 ? (95 + (int)(ha % 80u)) : (34 + (int)(ha % 55u)));
        *tendencies[aperm[rank]] = (unsigned char)lmClamp255(raw * uncannyScale / 100);
    }

    // 6. Continuous architectural trim, lower-wall dado, ceiling grid & fixture rhythm
    u32 hd0 = lmLvlHash(seed, level, 500u);
    u32 hd1 = lmLvlHash(seed, level, 501u);
    u32 hd2 = lmLvlHash(seed, level, 502u);
    // A two-tone wall is a decision, not a default: about every other level has no dado at all
    // (and boards, brick and raw concrete rarely take one).
    bool bareWall = (d.wallPole >= 4 && d.wallPole <= 6);
    bool noDado   = !classicLvl && ((lmLvlHash(seed, level, 503u) % 100u) < (bareWall ? 80u : 42u));
    d.dadoHeight = noDado ? (unsigned char)0
                 : (unsigned char)lmClampInt(74 + (int)(hd0 % 168u) + d.geneBias[LG_USE] / 5 + d.geneBias[LG_MASS] / 6, 74, 255);
    int toneMag  = 26 + (int)((hd0 >> 8) % 38u);
    bool darkerDado = (wallVal > 170) ? (((hd0 >> 16) % 6u) != 0u) : (((hd0 >> 16) % 3u) != 0u);
    d.dadoToneDelta = (signed char)lmClampInt(darkerDado ? -toneMag : (toneMag * 3 / 4), -68, 48);
    d.dadoHueShift  = (unsigned char)(hd0 >> 24);
    int dadoHue = ((hd0 >> 19) & 1u) ? ((wallHue + ((int)(d.dadoHueShift % 29u) - 14) + 360) % 360) : floorHue;
    int dadoSat = (wallSat <= 48 && floorSat <= 48 && ((hd0 >> 21) % 5u) == 0u)
                ? lmClampInt(42 + (int)((hd0 >> 24) % 44u), 36, 86) // occasional muted oil-paint half-wall on neutral room
                : lmClampInt((wallSat * 2 + floorSat) / 3 + 6, 12, 78);
    int dadoVal = lmClampInt(wallVal + (int)d.dadoToneDelta, 72, 198);
    d.dadoSurf  = lmBuildSurfFromHSV(seed, level, 510u, dadoHue, dadoSat, dadoVal,
                                     d.geneBias[LG_ORDER], d.geneBias[LG_DECAY], classicBlend, 158, 140, 92);
    d.dadoSurf.seamFreqX = (unsigned char)lmClampInt(96 + ((int)d.wallSurf.seamFreqX / 2), 84, 240);
    d.dadoSurf.seamFreqY = (unsigned char)lmClampInt(42 + ((int)d.wallSurf.seamFreqY / 3), 36, 160);
    d.dadoSurf.seamDepth = (unsigned char)lmClampInt((int)d.wallSurf.seamDepth + 58, 88, 232);
    d.dadoSurf.sheen     = (unsigned char)lmClampInt((int)d.wallSurf.sheen + 36, 36, 235);
    d.baseboardSize     = (unsigned char)lmClampInt(64 + (int)(hd1 % 165u) + d.geneBias[LG_MASS] / 4, 40, 245);
    d.corniceSize       = (unsigned char)lmClamp255(28 + (int)((hd1 >> 8) % 205u) + d.geneBias[LG_ORDER] / 4);
    d.ceilGridStep      = (unsigned char)(hd1 >> 24);
    d.trimPresence      = (unsigned char)lmClampInt(62 + (int)((hd2 >> 8) % 176u) + d.geneBias[LG_MASS] / 4, 45, 250);
    d.fixtureRhythm     = (unsigned char)lmClampInt(54 + (int)((hd2 >> 16) % 180u) + d.geneBias[LG_USE] / 3, 35, 245);

    // 7. Continuous megastructure form proclivities (steered by level hash + extreme gene biases)
    int mperm[LM_MEGA_FORM_COUNT];
    for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) mperm[f] = f;
    for (int f = LM_MEGA_FORM_COUNT - 1; f > 0; f--) {
        int sw = (int)(lmLvlHash(seed, level, 600u + (u32)f) % (u32)(f + 1));
        int tmp = mperm[f]; mperm[f] = mperm[sw]; mperm[sw] = tmp;
    }
    int geneNudge[LM_MEGA_FORM_COUNT] = {
        (int)d.geneBias[LG_SINK] / 4 + (int)d.geneBias[LG_ABYSS] / 5,
        (int)d.geneBias[LG_MASS] / 4 + (int)d.geneBias[LG_ORDER] / 4 + (int)d.geneBias[LG_SINK] / 5,
        (int)d.geneBias[LG_ABYSS] / 3 + (int)d.geneBias[LG_SPAN] / 4,
        (int)d.geneBias[LG_LATTICE] / 3 - (int)d.geneBias[LG_CELLS] / 5,
        (int)d.geneBias[LG_ABYSS] / 4 + (int)d.geneBias[LG_ORDER] / 4 + (int)d.geneBias[LG_SHELL] / 5,
        (int)d.geneBias[LG_SHELL] / 3 + (int)d.geneBias[LG_MASS] / 4,
        (int)d.geneBias[LG_CELLS] / 3 + (int)d.geneBias[LG_LATTICE] / 4 + (int)d.geneBias[LG_LIGHT] / 5,
        (int)d.geneBias[LG_SPAN] / 3 + (int)d.geneBias[LG_USE] / 4,
        (int)d.geneBias[LG_MASS] / 3 + (int)d.geneBias[LG_ABYSS] / 4 - (int)d.geneBias[LG_USE] / 4,
        (int)d.geneBias[LG_GRAFT] / 3 - (int)d.geneBias[LG_CELLS] / 4 - (int)d.geneBias[LG_ORDER] / 5,
        (int)d.geneBias[LG_SINK] / 3 + (int)d.geneBias[LG_SHELL] / 4 - (int)d.geneBias[LG_LIGHT] / 4,
        (int)d.geneBias[LG_ABYSS] / 3 + (int)d.geneBias[LG_SPAN] / 4 - (int)d.geneBias[LG_MASS] / 4
    };
    int nMegaPeaks = 2 + (int)(lmLvlHash(seed, level, 620u) % 2u);
    d.domMegaMask = 0u;
    for (int rank = 0; rank < LM_MEGA_FORM_COUNT; rank++) {
        int f = mperm[rank];
        u32 hm = lmLvlHash(seed, level, 630u + (u32)rank);
        int w = 18 + (int)(hm % 22u);
        if (rank == 0)                          w = 195 + (int)(hm % 56u);
        else if (rank == 1)                     w = 140 + (int)(hm % 56u);
        else if (rank == 2 && nMegaPeaks == 3)  w = 96  + (int)(hm % 46u);
        else if (rank <= nMegaPeaks + 2)        w = 48  + (int)(hm % 40u);
        if (f == MF_CLASSIC && rank == 0) w = w * 3 / 4; // never let classic vault monopolize a level
        d.megaFormWeight[f] = (unsigned char)lmClampInt(w + geneNudge[f] / 2, 26, 255);
    }
    int top1 = 0, top2 = 1;
    for (int f = 1; f < LM_MEGA_FORM_COUNT; f++) {
        if (d.megaFormWeight[f] > d.megaFormWeight[top1]) { top2 = top1; top1 = f; }
        else if (f != top1 && d.megaFormWeight[f] > d.megaFormWeight[top2]) { top2 = f; }
    }
    d.domMegaMask = (1u << top1) | (1u << top2);

    // 8. Continuous architectural delirium ("пьянка"): rare on L0, more frequent pockets as |level| grows
    u32 hdel = lmLvlHash(seed, level, 700u);
    int depthScale = 0; // 0..100 monotonic log-like ramp in |level|
    if (absL == 0u)      depthScale = 8;
    else if (absL <= 2u) depthScale = 22 + (int)absL * 10;
    else if (absL <= 15u) depthScale = 44 + (int)(absL - 2u) * 2;
    else if (absL <= 120u) depthScale = 70 + (int)(absL - 15u) / 5;
    else depthScale = 94 + (int)((absL > 10000u ? 10000u : absL) / 1600u);
    if (depthScale > 100) depthScale = 100;

    int delJitter = (int)(hdel % 35u) - 14 - d.geneBias[LG_ORDER] / 7 + d.geneBias[LG_DECAY] / 7;
    d.deliriumDrive  = (unsigned char)lmClamp255(16 + depthScale * 210 / 100 + delJitter);
    // Higher drive lowers pocket threshold from ~188 (~2% pockets on L0) down to ~134 (~20% pockets on deep levels)
    d.deliriumThresh = (unsigned char)lmClampInt(192 - (int)d.deliriumDrive * 62 / 255, 130, 192);
    d.deliriumPeriod = (unsigned char)(13 + (int)((hdel >> 8) % 13u));

    // 9. Quirks. Level 0 is pinned to none; the first deal (levels 0..12, and -13..-1) is pure,
    // further out about half of the levels carry a second quirk on top.
    d.quirk = (signed char)lmLvlPick(seed, level, 905u, LM_QUIRK_COUNT, LQ_NONE);
    d.quirkVar  = (unsigned char)(lmLvlHash(seed, level, 906u) >> 11);
    if (forceQuirk >= 0 && forceQuirk < LM_QUIRK_COUNT) {
        d.quirk = (signed char)forceQuirk;
        if (gLmQuirkVarForce >= 0) d.quirkVar = (unsigned char)gLmQuirkVarForce;
    }
    d.quirkVar2 = (unsigned char)(lmLvlHash(seed, level, 907u) >> 11);
    d.quirk2 = LQ_NONE;
    if (absL >= (u32)LM_QUIRK_COUNT && d.quirk != LQ_NONE && (lmLvlHash(seed, level, 908u) % 100u) < 55u) {
        int q2 = lmLvlPick(seed, level, 909u, LM_QUIRK_COUNT, LQ_NONE);
        // These repaint the whole picture and would cancel each other; the rest stack.
        auto look = [](int q) { return q == LQ_GREYBOX || q == LQ_MONO || q == LQ_NEON || q == LQ_BLACKOUT || q == LQ_LAVA; };
        auto dark = [](int q) { return q == LQ_NEON || q == LQ_BLACKOUT || q == LQ_LAVA; };
        bool clash = (q2 == d.quirk) || (look(q2) && look(d.quirk)) ||
                     (q2 == LQ_SKY && dark(d.quirk)) || (d.quirk == LQ_SKY && dark(q2));
        if (q2 != LQ_NONE && !clash) d.quirk2 = (signed char)q2;
    }
    if (d.quirk != LQ_NONE)  lmApplyQuirkToDNA(d, d.quirk, d.quirkVar, seed, level, true);
    if (d.quirk2 != LQ_NONE) lmApplyQuirkToDNA(d, d.quirk2, d.quirkVar2, seed, level, false);
    return d;
}

inline void lmScaleSurfRGB(LmSurfDNA& s, int pct) {
    for (int c = 0; c < 3; c++) {
        s.rgb[c]  = (unsigned char)lmClamp255((int)s.rgb[c] * pct / 100);
        s.rgb2[c] = (unsigned char)lmClamp255((int)s.rgb2[c] * pct / 100);
    }
}
