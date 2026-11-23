#pragma once
// Part of limen_genome.h: gene fields, wings, mega form of a slab, palettes.

// What a quirk does to the rest of the DNA (materials, lamps, fog, themes), so that the level
// built around it belongs to it. The shader, prop and sound sides read d.quirk* directly.
inline void lmApplyQuirkToDNA(LimenLevelDNA& d, int q, int var, u32 seed, int level, bool isMain) {
    unsigned char* cA = isMain ? d.quirkColA : d.quirk2ColA;
    unsigned char* cB = isMain ? d.quirkColB : d.quirk2ColB;
    u32 h = lmLvlHash(seed, level, 910u + (u32)q);
    auto setRGB = [](unsigned char* c, int r, int g, int b) { c[0] = (unsigned char)r; c[1] = (unsigned char)g; c[2] = (unsigned char)b; };
    auto boostTheme = [&](int t, int w) {
        if (!isMain) w = w * 2 / 3;
        if ((int)d.themeWeight[t] < w) d.themeWeight[t] = (unsigned char)w;
        d.domThemeMask |= (1u << t);
    };
    auto fogFrom = [&](const unsigned char* c, int pct, int lo) {
        for (int k = 0; k < 3; k++) d.fogRGB[k] = (unsigned char)lmClampInt((int)c[k] * pct / 100, lo, 170);
    };
    setRGB(cA, 128, 128, 128); setRGB(cB, 128, 128, 128);
    switch (q) {
    case LQ_FLOOD: {
        // Body colour of the water: pool aqua ... stagnant olive ... tea-brown ... ink.
        int kind = (int)(h % 8u);
        if (kind < 4)      lmHueSatValToRGB(168 + (int)((h >> 4) % 34u), 150 + (int)((h >> 10) % 60u), 150 + (int)((h >> 16) % 60u), cA);
        else if (kind < 6) lmHueSatValToRGB(70 + (int)((h >> 4) % 40u), 120 + (int)((h >> 10) % 50u), 84 + (int)((h >> 16) % 40u), cA);
        else if (kind < 7) lmHueSatValToRGB(26 + (int)((h >> 4) % 14u), 150, 96 + (int)((h >> 16) % 30u), cA);
        else               setRGB(cA, 10, 14, 20);
        setRGB(cB, kind < 4 ? 40 : (kind < 7 ? 150 : 235), 0, 0);   // murk
        d.floorSurf.sheen = (unsigned char)lmClampInt((int)d.floorSurf.sheen + 90, 170, 250);
        d.wallSurf.grime  = (unsigned char)lmClamp255((int)d.wallSurf.grime + 50);
        d.fogDensity      = (unsigned char)lmClamp255((int)d.fogDensity + 18);
        boostTheme(5, 215);                                   // pool
        break;
    }
    case LQ_SKY: {
        int kind = var % LM_SKY_KINDS;
        d.ceilGridStrength = 0;
        d.corniceSize = 0;
        if (kind == LM_SKY_OVERCAST) {
            int v = 196 + (int)(h % 30u);
            setRGB(cA, v - 14, v - 10, v - 4); setRGB(cB, v + 14, v + 14, v + 12);
        } else if (kind == LM_SKY_CLOUDS) {
            lmHueSatValToRGB(206 + (int)(h % 12u), 150 + (int)((h >> 6) % 40u), 226, cA);
            lmHueSatValToRGB(200 + (int)(h % 12u), 60, 244, cB);
        } else if (kind == LM_SKY_NIGHT) {
            setRGB(cA, 3, 5, 14); setRGB(cB, 14 + (int)(h % 10u), 18 + (int)((h >> 4) % 10u), 36 + (int)((h >> 8) % 14u));
        } else if (kind == LM_SKY_DUSK) {
            lmHueSatValToRGB(232 + (int)(h % 20u), 150, 96, cA);
            lmHueSatValToRGB(14 + (int)((h >> 6) % 22u), 170, 250, cB);
        } else {
            setRGB(cA, 6, 6, 8); setRGB(cB, 22, 21, 19);
        }
        if (kind == LM_SKY_OVERCAST || kind == LM_SKY_CLOUDS) {
            d.lightDensity   = (unsigned char)lmClampInt((int)d.lightDensity / 4, 6, 40);
            d.darkSlabChance = 0;
            d.ambientGain    = 176;
            d.lampGain       = 104;
            d.lightRGB[0] = 255; d.lightRGB[1] = 250; d.lightRGB[2] = 238;
            fogFrom(cB, 60, 40);
            d.fogDensity = (unsigned char)lmClampInt((int)d.fogDensity - 30, 20, 120);
        } else if (kind == LM_SKY_DUSK) {
            d.ambientGain = 120; d.lampGain = 128;
            d.lightRGB[0] = 255; d.lightRGB[1] = 206; d.lightRGB[2] = 150;
            fogFrom(cB, 46, 20);
        } else {
            d.ambientGain = (unsigned char)(kind == LM_SKY_NIGHT ? 84 : 72);
            fogFrom(cB, 100, 8);
        }
        break;
    }
    case LQ_PARTY:
        d.lightRGB[0] = 255; d.lightRGB[1] = 236; d.lightRGB[2] = 236;
        d.ambientGain = (unsigned char)lmClampInt((int)d.ambientGain, 96, 128);
        d.lampGain    = 156;
        d.lightDensity = (unsigned char)lmClampInt((int)d.lightDensity + 40, 150, 255);
        d.darkSlabChance = 0;
        d.propDensity = (unsigned char)lmClampInt((int)d.propDensity + 40, 150, 240);
        d.monoFloodTendency = (unsigned char)lmClampInt((int)d.monoFloodTendency + 60, 150, 255);
        boostTheme(9, 235);                                   // playroom
        boostTheme(8, 150);                                   // mall
        break;
    case LQ_BLACKOUT: {
        static const unsigned char phos[4][3] = {{110, 255, 150}, {120, 240, 255}, {255, 190, 80}, {235, 255, 120}};
        const unsigned char* p = phos[h % 4u];
        setRGB(cA, p[0], p[1], p[2]);
        d.ambientGain = 12; d.lampGain = 118;
        d.lightDensity = (unsigned char)(4 + (int)((h >> 4) % 22u));
        d.darkSlabChance = 90;                                // nine slabs in ten keep a single lamp
        d.fogRGB[0] = d.fogRGB[1] = d.fogRGB[2] = 3;
        d.fogDensity = (unsigned char)lmClampInt((int)d.fogDensity + 20, 90, 200);
        d.megaFogDensity = (unsigned char)lmClampInt((int)d.megaFogDensity + 40, 120, 230);
        break;
    }
    case LQ_GREYBOX: {
        // the "wall" colour of the blockout: mostly the familiar orange
        int hue = ((h % 5u) < 3u) ? 26 + (int)((h >> 4) % 8u) : (int)((h >> 4) % 360u);
        lmHueSatValToRGB(hue, 178, 226, cA);
        setRGB(cB, 138, 138, 140);
        d.lightRGB[0] = 255; d.lightRGB[1] = 252; d.lightRGB[2] = 246;
        d.ambientGain = 186; d.lampGain = 136;
        d.darkSlabChance = 0;
        d.fogRGB[0] = 52; d.fogRGB[1] = 53; d.fogRGB[2] = 56;
        d.fogDensity = (unsigned char)lmClampInt((int)d.fogDensity - 30, 20, 110);
        d.dadoHeight = 0;
        break;
    }
    case LQ_DRUNK:
        d.deliriumDrive  = 255;
        d.deliriumThresh = (unsigned char)(62 + (int)(h % 30u));
        d.deliriumPeriod = (unsigned char)lmClampInt((int)d.deliriumPeriod - 3, 10, 22);
        break;
    case LQ_WARP:
        break;
    case LQ_GIANT:
        d.propDensity = (unsigned char)lmClampInt((int)d.propDensity + 20, 110, 230);
        d.roomFurnish = (unsigned char)lmClampInt((int)d.roomFurnish + 30, 120, 245);
        break;
    case LQ_LAVA: {
        int kind = var % LM_LAVA_KINDS;
        if (kind == LM_LAVA_MAGMA)      setRGB(cA, 255, 92 + (int)(h % 40u), 16);
        else if (kind == LM_LAVA_TOXIC) lmHueSatValToRGB(84 + (int)(h % 80u), 210, 255, cA);
        else                            lmHueSatValToRGB((int)(h % 360u), 10 + (int)((h >> 9) % 40u), 255, cA);
        d.lightDensity = (unsigned char)(kind == LM_LAVA_PANEL ? 0 : 10 + (int)((h >> 4) % 22u));
        d.darkSlabChance = (unsigned char)(kind == LM_LAVA_PANEL ? 100 : 94);
        d.ambientGain = (unsigned char)(kind == LM_LAVA_PANEL ? 74 : 60);
        if (kind == LM_LAVA_PANEL) {
            lmHueSatValToRGB(0, 0, 214, d.floorSurf.rgb);
            lmHueSatValToRGB(0, 0, 196, d.floorSurf.rgb2);
            d.floorSurf.sheen = 230;
            d.floorSeamStrength = 0;
            lmScaleSurfRGB(d.ceilSurf, 46);
            fogFrom(cA, 22, 14);
        } else {
            setRGB(d.floorSurf.rgb, 44, 40, 38); setRGB(d.floorSurf.rgb2, 30, 27, 26);
            d.floorSurf.sheen = 40; d.floorSurf.grain = 230;
            d.floorSeamStrength = 0;
            lmScaleSurfRGB(d.wallSurf, 62); lmScaleSurfRGB(d.dadoSurf, 62); lmScaleSurfRGB(d.ceilSurf, 50);
            for (int p = 0; p < LM_MEGA_POLES; p++) lmScaleSurfRGB(d.megaPoles[p], 66);
            fogFrom(cA, 12, 6);
            d.fogDensity = (unsigned char)lmClampInt((int)d.fogDensity + 24, 80, 190);
        }
        break;
    }
    case LQ_MONO: {
        if ((h % 6u) == 0u) setRGB(cA, 238, 238, 236);       // the white level
        else {
            int hue = (int)((h >> 3) % 360u);
            if (hue >= 245 && hue <= 334) hue = (hue + 120) % 360;
            lmHueSatValToRGB(hue, 170 + (int)((h >> 12) % 50u), 226, cA);
        }
        d.lightRGB[0] = 255; d.lightRGB[1] = 250; d.lightRGB[2] = 244;
        d.ambientGain = (unsigned char)lmClampInt((int)d.ambientGain + 20, 130, 170);
        d.darkSlabChance = 0;
        fogFrom(cA, 24, 12);
        d.monoFloodTendency = 255;
        d.dadoHeight = 0;
        break;
    }
    case LQ_NEON: {
        int hue = (int)(h % 360u);
        if (hue >= 245 && hue <= 334) hue = 186;             // cyan instead of debug magenta
        int hue2 = (hue + 120 + (int)((h >> 9) % 110u)) % 360;
        if (hue2 >= 245 && hue2 <= 334) hue2 = 338 + (int)((h >> 17) % 18u);
        lmHueSatValToRGB(hue, 215, 255, cA);
        lmHueSatValToRGB(hue2, 215, 255, cB);
        d.ambientGain = 34; d.lampGain = 110;
        d.lightDensity = (unsigned char)(22 + (int)((h >> 4) % 30u));
        d.darkSlabChance = 70;
        for (int k = 0; k < 3; k++) d.lightRGB[k] = (unsigned char)(((int)cA[k] + 255) / 2);
        lmScaleSurfRGB(d.wallSurf, 40); lmScaleSurfRGB(d.dadoSurf, 40); lmScaleSurfRGB(d.floorSurf, 34); lmScaleSurfRGB(d.ceilSurf, 30);
        for (int p = 0; p < LM_MEGA_POLES; p++) lmScaleSurfRGB(d.megaPoles[p], 50);
        d.floorSurf.sheen = 235;
        d.floorSeamStrength = 0;
        d.ceilGridStrength = 0;
        fogFrom(cB, 9, 4);
        break;
    }
    case LQ_RETRO:
        d.fogDensity = (unsigned char)lmClampInt((int)d.fogDensity + 36, 110, 210);   // short draw distance
        break;
    default: break;
    }
}

inline LmSurfDNA lmDeriveDadoSurf(const LimenLevelDNA& d, int hueGene = 128) {
    LmSurfDNA s = d.dadoSurf;
    int nudge = (hueGene - 128) / 16;
    for (int c = 0; c < 3; c++) s.rgb[c] = (unsigned char)lmClampInt((int)s.rgb[c] + nudge, 36, 210);
    return s;
}

inline LimenLevelDNA gLimenDNA = lmMakeLevelDNA(1u, 0);

inline void lmApplyLevelDNA(u32 seed, int level) {
    gLimen.seed = seed;
    gLimen.level = level;
    gLimenDNA = lmMakeLevelDNA(seed, level, gLmQuirkForce);
    gLimen.rowPitch = gLimenDNA.rowPitch;
    gLimen.colPitch = gLimenDNA.colPitch;
    gLimen.dnaReady = true;
}
inline bool gLimenBootDNA = (lmApplyLevelDNA(1u, 0), true);

inline int lmShapeGene(int v) {
    v = lmClamp255(v);
    int s = v * v * (765 - 2 * v) / (255 * 255);
    return lmClamp255(128 + (s - 128) * 7 / 5);
}

inline int lmGeneFieldWithDNA(const LimenLevelDNA& dna, int j, int i, int gene, u32 extraSalt = 0u) {
    int period = dna.genePeriod[gene];
    u32 salt = 0x4700u + (u32)gene + extraSalt;
    int smooth = 0;
    if (period <= 1) {
        smooth = (int)(lmHash((u32)j, (u32)i, 1u, salt) & 255u);
    } else {
        int cj = lmFloorDiv(j, period), ci = lmFloorDiv(i, period);
        int fj = lmMod(j, period), fi = lmMod(i, period);
        int v00 = (int)(lmHash((u32)cj, (u32)ci, 1u, salt) & 255u);
        int v01 = (int)(lmHash((u32)cj, (u32)(ci + 1), 1u, salt) & 255u);
        int v10 = (int)(lmHash((u32)(cj + 1), (u32)ci, 1u, salt) & 255u);
        int v11 = (int)(lmHash((u32)(cj + 1), (u32)(ci + 1), 1u, salt) & 255u);
        int top = v00 * (period - fi) + v01 * fi;
        int bot = v10 * (period - fi) + v11 * fi;
        smooth = (top * (period - fj) + bot * fj) / (period * period);
    }
    int own = (int)(lmHash((u32)j, (u32)i, 2u, salt) & 255u);
    int smW = dna.smoothMix[gene];
    int shaped = lmShapeGene((smooth * smW + own * (5 - smW)) / 5);
    int mod = 128 + (shaped - 128) * (int)dna.geneGain[gene] / 128 + (int)dna.geneBias[gene];
    return lmClamp255(mod);
}

inline int lmGeneField(int j, int i, int gene) {
    return lmGeneFieldWithDNA(gLimenDNA, j, i, gene, 0u);
}

inline int lmStrangeness(int j, int i) {
    int rings = gLimenDNA.originCalmRings;
    if (rings <= 0) return 255;
    int d = lmDepth(j, i);
    if (rings == 1) return d == 0 ? 95 : 255;
    return d == 0 ? 0 : (d == 1 ? 150 : 255);
}

inline int lmGene(int j, int i, int gene) {
    int v = lmGeneField(j, i, gene);
    if (gene == LG_HEIGHT || gene == LG_SINK || gene == LG_SHELL || gene == LG_GRAFT || gene == LG_DECAY)
        v = v * lmStrangeness(j, i) / 255;
    if (const LmBeat* bt = lmBeatAt(j, i)) v = lmClamp255(v + (int)bt->geneAdd[gene]);
    return v;
}

inline LimenGenome lmGenome(int j, int i) {
    LimenGenome g;
    for (int k = 0; k < LG_COUNT; k++) g.g[k] = (unsigned char)lmGene(j, i, k);
    return g;
}

inline LimenGenome lmForeignGenome(const LimenLevelDNA& fdna, int j, int i, u32 salt) {
    LimenGenome g;
    for (int k = 0; k < LG_COUNT; k++) g.g[k] = (unsigned char)lmGeneFieldWithDNA(fdna, j, i, k, salt);
    return g;
}

// Operator-family affinities. No slab draws a region kind from a list. Each slab carries a continuous
// 0..255 affinity to five operator families, computed from its own genome (a product of two genes, so
// a family needs a conjunction of traits). limen_grammar.h weights its operators by these affinities
// and limen_build.h reads threshold memberships for room treatments. LM_WING_* is only the argmax
// label of that vector, kept for narration and debug output.
enum LmWingKind {
    LM_WING_PARTITIONS = 0, // thin screens & labyrinths: tight grain, low storeys
    LM_WING_CORRIDORS  = 1, // compact cell rows along axial corridors: many cells, orderly
    LM_WING_ENFILADE   = 2, // telescoping room chains: deep shells with many cells
    LM_WING_CONCOURSE  = 3, // open aisles & skylight wells: coarse grain, tall storeys
    LM_WING_SERVICE    = 4, // low, regular, tunnel-like plant spaces: low storeys, orderly
    LM_WING_COUNT      = 5
};

inline const char* lmWingName(int w) {
    static const char* names[LM_WING_COUNT] = {"partitions", "corridors", "enfilade", "concourse", "service"};
    return (w >= 0 && w < LM_WING_COUNT) ? names[w] : "?";
}

inline int lmWingAffinityOf(const unsigned char* g, int k) {
    switch (k) {
    case LM_WING_PARTITIONS: return (255 - g[LG_GRAIN]) * (255 - g[LG_HEIGHT]) / 255;
    case LM_WING_CORRIDORS:  return g[LG_CELLS] * g[LG_ORDER] / 255;
    case LM_WING_ENFILADE:   return g[LG_SHELL] * g[LG_CELLS] / 255;
    case LM_WING_CONCOURSE:  return g[LG_GRAIN] * g[LG_HEIGHT] / 255;
    case LM_WING_SERVICE:    return (255 - g[LG_HEIGHT]) * (255 - g[LG_ORDER]) / 255;
    default: return 0;
    }
}

// A narrator beat may ask for one family; it adds to that family's affinity (never a hard override).
inline int lmBeatWingOf(int j, int i) {
    const LmBeat* bt = lmBeatAt(j, i);
    return (bt && bt->wing >= 0 && bt->wing < LM_WING_COUNT) ? (int)bt->wing : -1;
}

inline int lmWingAffinityAt(const unsigned char* g, int j, int i, int k) {
    int a = lmWingAffinityOf(g, k) + (lmBeatWingOf(j, i) == k ? 120 : 0);
    return a > 255 ? 255 : a;
}

// Descriptive label only: argmax over the families (used by narration, props and debug output).
inline int lmWingAt(int j, int i) {
    LimenGenome gm = lmGenome(j, i);
    int best = 0, bestA = -1;
    for (int k = 0; k < LM_WING_COUNT; k++) {
        int a = lmWingAffinityAt(gm.g, j, i, k);
        if (a > bestA) { bestA = a; best = k; }
    }
    return best;
}

// Single strict functional prop theme index (0..LM_THEME_COUNT-1) shared across an entire wing district.
inline int lmWingThemeIdx(int j, int i) {
    int dj = lmFloorDiv(j + (lmMod(i, 3) == 1 ? 1 : 0), 2);
    int di = lmFloorDiv(i + (lmMod(j, 3) == 2 ? 1 : 0), 2);
    u32 h = lmHash((u32)dj, (u32)di, 2u, 0x575448u);
    // Strongly service-like genomes prefer maintenance/lab/warehouse infrastructure if weighted or available
    if (lmWingAffinityAt(lmGenome(j, i).g, j, i, LM_WING_SERVICE) >= 128 && (h % 100u) < 62u) {
        if (gLimenDNA.themeWeight[2] >= 12) return 2; // maint
    }
    int tot = 0;
    for (int t = 0; t < LM_THEME_COUNT; t++) tot += (int)gLimenDNA.themeWeight[t];
    if (tot <= 0) return 0;
    int roll = (int)(h % (u32)tot);
    for (int t = 0; t < LM_THEME_COUNT; t++) {
        roll -= (int)gLimenDNA.themeWeight[t];
        if (roll < 0) return t;
    }
    return 0;
}

inline int lmSlabMegaFormFromGenome(int j, int i, const LimenGenome& gm) {
    if (const LmBeat* bt = lmBeatAt(j, i)) { if (bt->megaForm >= 0 && bt->megaForm < LM_MEGA_FORM_COUNT) return bt->megaForm; }
    const unsigned char* g = gm.g;
    int aff[LM_MEGA_FORM_COUNT] = {
        56 + (int)g[LG_SINK] / 3 + (int)g[LG_ABYSS] / 4,
        44 + (int)g[LG_MASS] / 3 + (int)g[LG_ORDER] / 4 + (int)g[LG_SINK] / 5,
        58 + (int)g[LG_ABYSS] / 2 + (int)g[LG_SPAN] * 2 / 5 + (255 - (int)g[LG_GRAIN]) / 5,
        56 + (int)g[LG_LATTICE] * 3 / 5 + (255 - (int)g[LG_CELLS]) / 3,
        58 + (int)g[LG_ABYSS] * 2 / 5 + (int)g[LG_ORDER] * 2 / 5 + (int)g[LG_SHELL] / 4,
        54 + (int)g[LG_SHELL] * 3 / 5 + (int)g[LG_MASS] / 3,
        58 + (int)g[LG_CELLS] / 2 + (int)g[LG_LATTICE] * 2 / 5 + (int)g[LG_LIGHT] / 4,
        60 + (int)g[LG_SPAN] * 3 / 5 + (int)g[LG_USE] * 2 / 5 + (int)g[LG_ORDER] / 5,
        58 + (int)g[LG_MASS] * 2 / 5 + (255 - (int)g[LG_USE]) / 3 + (int)g[LG_ABYSS] / 5,
        58 + (255 - (int)g[LG_CELLS]) * 2 / 5 + (255 - (int)g[LG_ORDER]) / 3 + (int)g[LG_GRAFT] / 4,
        58 + (int)g[LG_SHELL] / 3 + (int)g[LG_SINK] / 3 + (255 - (int)g[LG_LIGHT]) / 3,
        60 + (int)g[LG_ABYSS] * 2 / 5 + (int)g[LG_SPAN] / 3 + (255 - (int)g[LG_MASS]) / 3
    };
    long long w[LM_MEGA_FORM_COUNT], total = 0;
    for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) {
        w[f] = (long long)gLimenDNA.megaFormWeight[f] * (long long)aff[f];
        total += w[f];
    }
    if (total <= 0) return MF_CLASSIC;
    u32 h = lmHash((u32)j, (u32)i, 11u, 0x4D46u);
    long long roll = (long long)(((unsigned long long)h * (unsigned long long)total) >> 32);
    for (int f = 0; f < LM_MEGA_FORM_COUNT; f++) {
        roll -= w[f];
        if (roll < 0) return f;
    }
    return LM_MEGA_FORM_COUNT - 1;
}

inline int lmSlabMegaForm(int j, int i) {
    return lmSlabMegaFormFromGenome(j, i, lmGenome(j, i));
}

// Continuous non-manmade drive of a vault slab (0..255). Acoustics and surfaces read this drive
// directly, so a level's character comes from genes, not from a list of named megastructures.
inline int lmSlabRawPhenomOf(int tiers, int j, int i, const LimenGenome& gm) {
    if (tiers <= 1) return 0;
    const unsigned char* g = gm.g;
    u32 hr = lmHash((u32)j, (u32)i, 21u, 0x524157u);
    int raw = ((255 - g[LG_USE]) * 3 + g[LG_GRAFT] * 2 + g[LG_ABYSS] + (255 - g[LG_ORDER])) / 7;
    raw += (int)(hr % 61u) - 30;
    if (tiers < 5 && raw > 135) raw = 135;
    return lmClamp255(raw);
}

// Continuous upper-sky exposure of a slab (0..255): 0 sealed lid, 1..119 pierced roof, 120..255 roofless.
inline int lmSlabSkyDriveOf(int tiers, int j, int i, const LimenGenome& gm, int raw) {
    if (lmHasQuirk(gLimenDNA, LQ_SKY)) return tiers <= 1 ? 200 : 210;
    if (tiers <= 1) return 0;
    const unsigned char* g = gm.g;
    u32 hs = lmHash((u32)j, (u32)i, 23u, 0x534B59u);
    int score = (raw * 2 + g[LG_ABYSS] + (255 - g[LG_SHELL]) + g[LG_LIGHT] / 2) / 7;
    score += (int)(hs % 75u) - 42;
    if (tiers < 6 && score >= 120) score = 96;
    if (score < 48) return 0;
    return lmClamp255((score - 48) * 255 / 170);
}

// Pure O(1) World Attractor Core: places rare, non-adjacent monumental focal slabs (1 per coarse
// super-block, always cushioned by human-scale buffer slabs so megastructures feel earned, never wallpaper).
inline bool lmIsAttractorCore(int j, int i) {
    if (lmStrangeness(j, i) < 120) return false;
    int sp = gLimenDNA.spacePole;
    int bjSpan = (gLimen.level == 0) ? 3 : ((sp == 0) ? 2 : ((sp == 1 || sp == 5 || sp == 7) ? 4 : 3));
    int biSpan = (gLimen.level == 0) ? 4 : ((sp == 0) ? 3 : ((sp == 1 || sp == 5 || sp == 7) ? 4 : 3));
    int bj = lmFloorDiv(j, bjSpan), bi = lmFloorDiv(i, biSpan);
    u32 h = lmHash((u32)bj, (u32)bi, 17u, 0x415454u);
    // Keep 1-slab margin on the +J/+I edges of each super-block so two cores NEVER touch!
    int freeJ = bjSpan > 2 ? (bjSpan - 1) : 1;
    int freeI = biSpan > 2 ? (biSpan - 1) : 1;
    int cj = bj * bjSpan + (int)(h % (u32)freeJ);
    int ci = bi * biSpan + (int)((h >> 8) % (u32)freeI);
    if (bjSpan == 2 && ((bj + bi) & 1) != 0 && j == cj) {
        // On dense 'vaults' pole (2x3), stagger alternate rows so cores still don't merge into endless sheets
        cj = bj * bjSpan + 1;
    }
    return j == cj && i == ci;
}

inline int lmSlabTiers(int j, int i) {
    const LmBeat* bt = lmBeatAt(j, i);
    int pinned = bt ? (int)bt->megaForm : LB_FORM_KEEP;
    if (pinned == LB_FORM_FLAT) return 1;
    if (pinned < 0 && lmStrangeness(j, i) < 100) return 1;
    int v = lmGene(j, i, LG_HEIGHT);
    int thr = gLimenDNA.vaultThreshold;
    int tiers = 1;
    int maxS = gLimenDNA.maxStoreys;
    bool isCore = lmIsAttractorCore(j, i);
    if (pinned >= 0) {
        // the narrator's megastructure: as tall as it asked, or a hashed 5..8 storeys before the form's own minimum
        tiers = bt->tiers ? (int)bt->tiers : 5 + (int)(lmHash((u32)j, (u32)i, 3u, 0x5449u) % 4u);
    } else if (isCore) {
        u32 hc = lmHash((u32)j, (u32)i, 3u, 0x5449u);
        tiers = 4 + (int)(hc % (u32)(maxS > 4 ? (maxS - 3) : 2));
        if (tiers > maxS && maxS >= 4) tiers = maxS;
    } else if (gLimen.level != 0 && v >= thr) {
        // Secondary gene-driven vault only when not crowding an attractor core
        if (!lmIsAttractorCore(j - 1, i) && !lmIsAttractorCore(j + 1, i) &&
            !lmIsAttractorCore(j, i - 1) && !lmIsAttractorCore(j, i + 1)) {
            int span = 256 - thr;
            if (span < 1) span = 1;
            int base = 2 + (v - thr) * (maxS - 1) / span;
            int jitter = (int)(lmHash((u32)j, (u32)i, 5u, 0x5449u) % 2u);
            tiers = base + jitter;
            if (tiers > maxS) tiers = maxS;
        }
    }
    if (tiers > 1) {
        int mf = lmSlabMegaForm(j, i);
        u32 hv = lmHash((u32)j, (u32)i, 13u, 0x544Au);
        if (mf == MF_CANYON || mf == MF_CELLBLOCK || mf == MF_SILO) {
            int minVert = 7 + (int)(hv % 5u); // 7..11 storeys (31.5..49.5 m!)
            if (tiers < minVert) tiers = minVert;
        } else if (mf == MF_MONOLITH || mf == MF_GROTTO) {
            int minVert = 8 + (int)(hv % 5u); // 8..12 storeys (36..54 m towering non-manmade scale!)
            if (tiers < minVert) tiers = minVert;
        } else if (mf == MF_SPIRES || mf == MF_TENDRILS) {
            int minVert = 8 + (int)(hv % 4u); // 8..11 storeys (36..49.5 m endless sky/mist!)
            if (tiers < minVert) tiers = minVert;
        } else if (mf == MF_HANGING || mf == MF_ZIGGURAT) {
            int minVert = 6 + (int)(hv % 4u); // 6..9 storeys (27..40.5 m!)
            if (tiers < minVert) tiers = minVert;
        } else if (mf == MF_HYPOSTYLE || mf == MF_PLATFORM) {
            int minVert = 4 + (int)(hv % 3u); // 4..6 storeys
            if (tiers < minVert) tiers = minVert;
        }
        if (tiers > 12) tiers = 12;
    }
    return tiers;
}

// Returns 0..3 (0=-X/West, 1=+X/East, 2=-Z/North, 3=+Z/South) if non-vault slab (j, i) directly
// borders a monumental vault slab — driving Foreshadowing Halos (compression throats & portal warnings)!
inline int lmSlabHaloDir(int j, int i) {
    if (const LmBeat* bt = lmBeatAt(j, i)) { if (bt->haloPin >= -1 && bt->haloPin <= 3) return bt->haloPin; }
    if (lmSlabTiers(j, i) > 1) return -1;
    if (lmSlabTiers(j, i - 1) > 1) return 0; // West neighbour is a vault
    if (lmSlabTiers(j, i + 1) > 1) return 1; // East neighbour is a vault
    if (lmSlabTiers(j - 1, i) > 1) return 2; // North row neighbour
    if (lmSlabTiers(j + 1, i) > 1) return 3; // South row neighbour
    return -1;
}

inline int lmPaletteOf(int hueGene) { return hueGene < 85 ? 0 : (hueGene < 170 ? 1 : 2); }

// Smooth 2D value noise in [0..255] at integer cell (wx, wz) with period P.
inline int lmSmoothCellNoise255(int wx, int wz, int period, u32 salt) {
    if (period <= 1) return (int)(lmHash((u32)wx, (u32)wz, 1u, salt) & 255u);
    int cx = lmFloorDiv(wx, period), cz = lmFloorDiv(wz, period);
    int fx = lmMod(wx, period),      fz = lmMod(wz, period);
    // Smoothstep Hermite weights in [0..256]
    int ux = (fx * 256) / period, uz = (fz * 256) / period;
    ux = (ux * ux * (768 - 2 * ux)) >> 16;
    uz = (uz * uz * (768 - 2 * uz)) >> 16;
    int v00 = (int)(lmHash((u32)cx,       (u32)cz,       1u, salt) & 255u);
    int v10 = (int)(lmHash((u32)(cx + 1), (u32)cz,       1u, salt) & 255u);
    int v01 = (int)(lmHash((u32)cx,       (u32)(cz + 1), 1u, salt) & 255u);
    int v11 = (int)(lmHash((u32)(cx + 1), (u32)(cz + 1), 1u, salt) & 255u);
    int top = (v00 * (256 - ux) + v10 * ux) >> 8;
    int bot = (v01 * (256 - ux) + v11 * ux) >> 8;
    return (top * (256 - uz) + bot * uz) >> 8;
}

// Continuous world-space architectural delirium ("пьянка") in [0..255]:
//   0..28   = sober (~60..88% of world depending on |level|)
//   29..114 = tipsy transition zone
//   115..255 = intense delirious pocket
inline int lmDeliriumCell255(int wx, int wz) {
    int p1 = (int)gLimenDNA.deliriumPeriod;
    int p2 = p1 >= 6 ? (p1 / 2) : 3;
    int n1 = lmSmoothCellNoise255(wx, wz, p1, 0x444C1u);
    int n2 = lmSmoothCellNoise255(wx + 19, wz - 11, p2, 0x444C2u);
    // Broaden tail of 2-octave sum around 128 so pockets peak cleanly up to ~250
    int mix = (n1 * 3 + n2 * 2) / 5;
    int raw = lmClamp255(128 + (mix - 128) * 195 / 100);
    int thr = (int)gLimenDNA.deliriumThresh;
    if (raw <= thr) return gLmPockets.empty() ? 0 : lmBeatPocketAt(wx, wz);
    int span = 255 - thr;
    if (span < 1) span = 1;
    int lin = (raw - thr) * 255 / span;
    int shaped = (lin * 95 + (lin * lin * 160) / 255) / 255;
    if (gLimenDNA.originCalmRings > 0) {
        LimenRect r = lmSlabAt(wx, wz);
        shaped = shaped * lmStrangeness(r.j, r.i) / 255;
    }
    if (!gLmPockets.empty()) { int pk = lmBeatPocketAt(wx, wz); if (pk > shaped) shaped = pk; }
    return lmClamp255(shaped);
}

inline float lmDeliriumAt(int wx, int wz) {
    return (float)lmDeliriumCell255(wx, wz) / 255.0f;
}
