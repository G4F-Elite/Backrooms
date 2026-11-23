#pragma once
// Part of limen_build.h: lmBuildSlab: the whole slab pipeline.

inline void lmBuildSlab(LimenSlab& s, int j, int i) {
    s.rect = lmSlabRect(j, i);
    s.w = s.rect.x1 - s.rect.x0 - 1;
    s.h = s.rect.z1 - s.rect.z0 - 1;
    s.genome = lmGenome(j, i);
    const unsigned char* g = s.genome.g;
    s.ceilTiers = lmSlabTiers(j, i);
    s.megaForm = lmSlabIsVault(s) ? lmSlabMegaFormFromGenome(j, i, s.genome) : -1;
    s.rawPhenom = lmSlabRawPhenomOf(s.ceilTiers, j, i, s.genome);
    s.skyDrive = lmSlabSkyDriveOf(s.ceilTiers, j, i, s.genome, s.rawPhenom);
    u32 mh = lmHash((u32)j, (u32)i, 19u, 0x4D53u);
    s.megaSub = (int)(mh & 255u);
    // The narrator's beat for this slab, if any: where the road runs, what must happen here.
    const LmBeat* bt = lmBeatAt(j, i);
    s.megaOffX = s.megaOffZ = 0;
    s.roadAxis = -1; s.roadC = -1; s.roadThrough = false;
    s.focusAxis = -1; s.focusC = -1;
    if (bt && bt->roadAxis >= 0) { s.focusAxis = bt->roadAxis; s.focusC = (short)lmBeatFocusLocal(bt, s.rect); }
    s.beatForce = bt ? bt->force : 0u;
    s.stampX0 = s.stampZ0 = s.stampX1 = s.stampZ1 = -1;
    s.entryX = (bt && bt->entryX >= 0) ? bt->entryX : (short)-1;
    s.entryZ = (bt && bt->entryZ >= 0) ? bt->entryZ : (short)-1;
    {
        int rc = lmBeatRoadLocal(bt, s.rect);
        if (rc >= 0) { s.roadAxis = bt->roadAxis; s.roadC = (short)rc; }
    }
    int zModePin = (bt && bt->megaMode >= 0 && bt->megaMode <= 2) ? (int)bt->megaMode : -1;
    LmRoadFit fit;
    if (s.megaForm == MF_ZIGGURAT) {
        int rollZ = (int)(mh % 100u);
        int zMode = (rollZ < 24) ? 0 : ((rollZ < 70 || g[LG_MASS] > g[LG_SINK] + 20) ? 1 : 2);
        if (zModePin >= 0) zMode = zModePin;
        int zAxis = (s.h >= s.w) ? 0 : 1;
        if (((mh >> 6) & 3u) == 0u) zAxis ^= 1;
        int zBits = (int)((mh >> 8) & 15u);
        if (bt && bt->roadThrough && s.roadAxis >= 0) {
            fit = lmMegaRoadFit(MF_ZIGGURAT, zMode, s.w, s.h, s.ceilTiers, g[LG_GRAIN], s.roadAxis, s.roadC);
            if (fit.ok && fit.axis >= 0) zAxis = fit.axis;
            if (fit.ok && fit.posEnd >= 0) zBits = (zBits & ~1) | fit.posEnd;
        }
        s.megaSub = (zMode & 3) | (zAxis << 2) | (zBits << 3);
    } else if (s.megaForm == MF_CANYON || s.megaForm == MF_PLATFORM || s.megaForm == MF_MONOLITH) {
        int ax = (s.w >= s.h) ? 0 : 1;
        if (g[LG_ORDER] < 130 && ((mh >> 4) & 1u)) ax ^= 1;
        if (bt && bt->roadThrough && s.roadAxis >= 0) {
            fit = lmMegaRoadFit(s.megaForm, 0, s.w, s.h, s.ceilTiers, g[LG_GRAIN], s.roadAxis, s.roadC);
            if (fit.ok && fit.axis >= 0) ax = fit.axis;
        }
        s.megaSub = ax | ((int)((mh >> 5) & 63u) << 1);
    } else if (s.megaForm >= 0 && bt && bt->roadThrough && s.roadAxis >= 0) {
        fit = lmMegaRoadFit(s.megaForm, 0, s.w, s.h, s.ceilTiers, g[LG_GRAIN], s.roadAxis, s.roadC);
    }
    if (fit.ok) {
        s.roadThrough = true;
        if (s.roadAxis == 0) s.megaOffZ = (short)fit.off; else s.megaOffX = (short)fit.off;
    }
    s.palette = lmPaletteOf(g[LG_HUE]);
    s.wing = lmWingAt(j, i);
    s.wingTheme = lmWingThemeIdx(j, i);
    s.haloDir = lmSlabHaloDir(j, i);
    int effLight = (g[LG_LIGHT] * 2 + (int)gLimenDNA.lightDensity * 3) / 5;
    if (s.haloDir >= 0 || lmSlabHasFam(s, LM_WING_SERVICE)) effLight = effLight * 3 / 4; // dimmer approach & service tunnels!
    s.lampPitch = lmSlabIsVault(s)
        ? (effLight > 185 ? 3 : (effLight > 130 ? 4 : (effLight > 75 ? 5 : 7)))
        : (effLight > 155 ? 2 : (effLight > 95 ? 3 : (effLight > 45 ? 4 : 6)));
    s.pillars = !lmSlabIsVault(s) && g[LG_LATTICE] > gLimenDNA.pillarLatticeThresh;
    s.siteCount = 0;
    s.specialLampX = s.specialLampZ = -1;
    s.dark = false;
    s.fallback = false;
    s.links = s.sealed = s.loops = s.thinCount = s.narrowCount = s.walkBasins = 0;
    s.deadThresholds = s.vitrineRooms = 0;
    int delSum = 0, delCnt = 0, delMax = 0;
    for (int sx = 0; sx < s.w; sx += 3) {
        for (int sz = 0; sz < s.h; sz += 3) {
            int dv = lmDeliriumCell255(s.rect.x0 + 1 + sx, s.rect.z0 + 1 + sz);
            delSum += dv; delCnt++;
            if (dv > delMax) delMax = dv;
        }
    }
    s.deliriumMean = delCnt > 0 ? (delSum / delCnt) : 0;
    s.deliriumPeak = delMax;
    s.deliriumStruct = 0;

    static LimenPort ports[LM_MAX_OPENINGS];
    int perSide[4];
    int portCount = lmCollectOpenings(s.rect, ports, perSide);
    u32 h = lmHash((u32)j, (u32)i, 0u, 0x494Eu);

    LgBuild b;
    b.s = &s; b.vault = lmSlabIsVault(s);
    b.gapDoor = b.shiftDoor = -1;
    for (int pass = 0; pass < 2; pass++) {
        b.nodes = 0; b.doorCounter = 0;
        b.gapX = b.gapZ = b.shiftX = b.shiftZ = -1;
        s.deliriumStruct = 0;
        s.nestedX = s.nestedZ = s.nestedDir = s.nestedRings = -1;
        s.spiralX = s.spiralZ = -1; s.spiralW = s.spiralH = 0;
        lmGrowSlab(s, b, perSide);
        if (pass == 1 || b.doorCounter <= 2) break;
        if ((int)(lmHash((u32)j, (u32)i, 1u, 0x414Eu) % 100u) < g[LG_DECAY] * 60 / 255)
            b.gapDoor = (int)((h >> 8) % (u32)b.doorCounter);
        if ((int)(lmHash((u32)j, (u32)i, 2u, 0x414Eu) % 100u) < g[LG_DECAY] * 50 / 255)
            b.shiftDoor = (int)((h >> 16) % (u32)b.doorCounter);
        if (b.shiftDoor == b.gapDoor) b.shiftDoor = -1;
        if (b.gapDoor < 0 && b.shiftDoor < 0) break;
    }
    s.nodes = b.nodes;

    bool ok = lmRepair(s, ports, portCount, b.gapX, b.gapZ, h);
    if (ok) { lmMeasure(s); ok = 100 - s.share[LT_WALL] - lmPitShare(s) >= 10; }
    if (!ok) {
        memset(s.tag, LT_HALL, sizeof(s.tag));
        memset(s.flag, 0, sizeof(s.flag));
        memset(s.zone, 0, sizeof(s.zone));
        memset(s.nodeKey, 0, sizeof(s.nodeKey));
        memset(s.roomRel, 0, sizeof(s.roomRel));
        s.xeno = {};
        s.fallback = true;
        lmMeasure(s);
        return;
    }
    lmBuildBasinStairs(s, h);
    lmFinalizeCellFacts(s);
    lmApplyLiminalExcisions(s, ports, portCount, h);
    lmMeasure(s);
    if (b.gapX >= 0 && lmTagLocal(s, b.gapX, b.gapZ) == LT_WALL) lmAddSite(s, LS_DOOR_GAP, b.gapX, b.gapZ);
    if (b.shiftX >= 0 && lmIsWalkableTag(lmTagLocal(s, b.shiftX, b.shiftZ))) lmAddSite(s, LS_DOOR_SHIFT, b.shiftX, b.shiftZ);
    if (lmPitShare(s) > 0) lmPlaceRailGap(s, h);
}
