#pragma once
// Part of limen_mega.h: meshes of hypostyle, silo, hanging, cellblock, platform.

// Form 3 (MF_HYPOSTYLE): forest of columns with modulated spacing/thickness, broken stumps,
// and ceiling-hung inverted stalactite columns stopping in mid-air above the floor.
inline void lmBuildMegaHypostyle(const LimenSlab& s, float T, std::vector<float>& mv,
                                 std::vector<float>& dv, std::vector<float>& lv, std::vector<LimenAabb2D>& outCols) {
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;

    float baseStepX = 6.0f + (float)g[LG_GRAIN] * 3.8f / 255.0f;
    float baseStepZ = (g[LG_ORDER] >= 120) ? baseStepX : (6.0f + (float)((g[LG_GRAIN] + 97) & 255) * 3.6f / 255.0f);
    // Deep waffle coffer ribs on the vault ceiling
    for (float x = x0 + baseStepX * 0.5f; x < x1 - 2.0f; x += baseStepX)
        lmPushBox(mv, x - 0.38f, ceilY - 1.6f, z0, x + 0.38f, ceilY, z1, T);
    for (float z = z0 + baseStepZ * 0.5f; z < z1 - 2.0f; z += baseStepZ)
        lmPushBox(mv, x0, ceilY - 1.6f, z - 0.38f, x1, ceilY, z + 0.38f, T);

    int kx = 0;
    for (float bx = x0 + 6.5f; bx <= x1 - 6.5f; bx += baseStepX, kx++) {
        int kz = 0;
        for (float bz = z0 + 6.5f; bz <= z1 - 6.5f; bz += baseStepZ, kz++) {
            u32 hc = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(kx * 73 + kz), 0x4859u);
            float drift = (g[LG_ORDER] < 130) ? ((float)(255 - g[LG_ORDER]) / 255.0f * 1.15f) : 0.0f;
            float cx = bx + (((float)(hc & 63u) / 31.5f) - 1.0f) * drift;
            float cz = bz + (((float)((hc >> 6) & 63u) / 31.5f) - 1.0f) * drift;
            int wx = (int)floorf(cx / CS), wz = (int)floorf(cz / CS);
            int slx = wx - s.rect.x0 - 1, slz = wz - s.rect.z0 - 1;
            if (slx < 1 || slz < 1 || slx >= s.w - 1 || slz >= s.h - 1) continue;
            if (s.tag[slx][slz] != LT_HALL) continue;
            if (s.zone[slx][slz] & LZ_STORY) continue;            // the rig's lane runs between the columns
            const bool byLane = lmStoryNear(s, slx, slz, 1);     // next to it only what hangs from the roof

            // Modulate column half-width continuously by LG_MASS + spatial wave
            float wave = 0.5f + 0.5f * sinf(kx * 0.85f + kz * 0.65f);
            float hw = 0.36f + ((float)g[LG_MASS] / 255.0f) * 0.42f + wave * 0.24f; // 0.36..1.02 m half-width
            int roll = (int)((hc >> 12) % 256u);
            int hangChance = 48 + (int)g[LG_SHELL] / 2 + (255 - (int)g[LG_ORDER]) / 4;
            int stumpChance = 28 + (int)g[LG_DECAY] * 2 / 3;

            if (roll < hangChance) {
                // Hanging stalactite column suspended from ceiling, stopping 2.85..5.8m above floor!
                float botY = 2.85f + ((float)((hc >> 20) % 100u) / 99.0f) * fminf(3.4f, ceilY * 0.22f);
                lmPushBox(mv, cx - hw * 1.25f, ceilY - 2.1f, cz - hw * 1.25f, cx + hw * 1.25f, ceilY, cz + hw * 1.25f, T);
                lmPushBox(mv, cx - hw, botY, cz - hw, cx + hw, ceilY - 1.6f, cz + hw, T);
                if (!s.dark && ((kx + kz) % 3) == 0) {
                    lmPushLightQuad(lv, Vec3(cx - hw * 0.78f, botY - 0.03f, cz - hw * 0.78f),
                                        Vec3(cx + hw * 0.78f, botY - 0.03f, cz - hw * 0.78f),
                                        Vec3(cx + hw * 0.78f, botY - 0.03f, cz + hw * 0.78f),
                                        Vec3(cx - hw * 0.78f, botY - 0.03f, cz + hw * 0.78f), true);
                }
            } else if (roll < hangChance + stumpChance) {
                // Truncated / unfinished column rising from floor and stopping mid-air!
                if (lmLampLocal(s, slx, slz) || byLane) continue;
                float topY = 2.20f + ((float)((hc >> 20) % 100u) / 99.0f) * (ceilY * 0.52f);
                lmPushBox(mv, cx - hw * 1.22f, 0.0f, cz - hw * 1.22f, cx + hw * 1.22f, 0.45f, cz + hw * 1.22f, T);
                lmPushBox(mv, cx - hw, 0.45f, cz - hw, cx + hw, topY, cz + hw, T);
                lmPushBox(mv, cx - hw * 0.65f, topY, cz - hw * 0.65f, cx + hw * 0.65f, topY + 0.65f, cz + hw * 0.65f, T);
                outCols.push_back({cx - hw * 1.18f, cz - hw * 1.18f, cx + hw * 1.18f, cz + hw * 1.18f});
            } else {
                // Full-height monumental shaft with stepped base and flared capital
                if (lmLampLocal(s, slx, slz) || byLane) continue;
                lmPushBox(mv, cx - hw * 1.26f, 0.0f, cz - hw * 1.26f, cx + hw * 1.26f, 0.55f, cz + hw * 1.26f, T);
                lmPushBox(mv, cx - hw, 0.55f, cz - hw, cx + hw, ceilY - 2.1f, cz + hw, T);
                lmPushBox(mv, cx - hw * 1.38f, ceilY - 2.1f, cz - hw * 1.38f, cx + hw * 1.38f, ceilY - 1.4f, cz + hw * 1.38f, T);
                outCols.push_back({cx - hw * 1.20f, cz - hw * 1.20f, cx + hw * 1.20f, cz + hw * 1.20f});
            }
        }
    }
}

// Form 4 (MF_SILO): enclosed 16-sided vertical tube (-48 m .. +ceilY, 7..12 storeys!) with
// tiled/ribbed liner, helical spiral ramp, ring catwalks, oculus light-shaft column & abyss glow!
inline void lmBuildMegaSilo(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav,
                            std::vector<float>& dv, std::vector<float>& lv, std::vector<LimenAabb2D>& outCols) {
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;
    int maxD = lmClampInt(11 + (int)g[LG_GRAIN] / 64, 11, 15);
    int dX = (s.w - 4 < maxD) ? (s.w - 4) : maxD;
    int dZ = (s.h - 4 < maxD) ? (s.h - 4) : maxD;
    int D  = dX < dZ ? dX : dZ;
    if (D < 7) return;
    int sx0 = (s.w - D) / 2 + s.megaOffX, sz0 = (s.h - D) / 2 + s.megaOffZ;   // (the narrator may have moved the drum onto its road)
    float cxW = (s.rect.x0 + 1 + sx0 + D * 0.5f) * CS;
    float czW = (s.rect.z0 + 1 + sz0 + D * 0.5f) * CS;
    float rOuter = (D * 0.5f - 0.18f) * CS;
    float rPit   = fmaxf(((D - 4) * 0.5f + 0.15f) * CS, 7.5f);

    const int N = 16;
    const float tau = 6.2831853f, SHELL_T = 0.36f;
    float waist = (g[LG_MASS] < 150) ? (0.13f * (float)(150 - g[LG_MASS]) / 150.0f) : 0.0f;
    auto radAtY = [&](float y) -> float {
        if (y <= 0.0f) return rOuter;
        float u = y / fmaxf(ceilY, 9.0f);
        if (u > 1.0f) u = 1.0f;
        return rOuter * (1.0f - waist * sinf(u * 3.14159265f));
    };

    // 1a. Ground-level (0..WH) octagonal/cylindrical enclosure (open ONLY at the 4 cardinal portals!)
    for (int i = 0; i < N; i++) {
        float a0 = tau * (float)i / (float)N, a1 = tau * (float)(i + 1) / (float)N;
        bool isPortal = (i == 0 || i == 15 || i == 3 || i == 4 || i == 7 || i == 8 || i == 11 || i == 12);
        Vec3 p00(cxW + sinf(a0) * rOuter, isPortal ? 3.55f : 0.0f, czW + cosf(a0) * rOuter);
        Vec3 p01(cxW + sinf(a0) * rOuter, WH,                       czW + cosf(a0) * rOuter);
        Vec3 p11(cxW + sinf(a1) * rOuter, WH,                       czW + cosf(a1) * rOuter);
        Vec3 p10(cxW + sinf(a1) * rOuter, isPortal ? 3.55f : 0.0f, czW + cosf(a1) * rOuter);
        lmPushQuad(mav, p00, p01, p11, p10, T * 0.65f);
        // The drum is a 16-gon inside a stepped disc of cells: the cell corners left outside it
        // see the shell from behind. So it is a real wall SHELL_T thick: an outer skin, a soffit
        // over each portal, a reveal at each portal jamb, and (between portals) it is as solid to
        // walk into as it looks.
        float yLo = isPortal ? 3.55f : 0.0f, rO = rOuter + SHELL_T;
        Vec3 o00(cxW + sinf(a0) * rO, yLo, czW + cosf(a0) * rO), o01(cxW + sinf(a0) * rO, WH, czW + cosf(a0) * rO);
        Vec3 o11(cxW + sinf(a1) * rO, WH, czW + cosf(a1) * rO), o10(cxW + sinf(a1) * rO, yLo, czW + cosf(a1) * rO);
        lmPushQuad(mav, o10, o11, o01, o00, T * 0.65f);
        {   // a waisted tube starts a little inside the ground ring: the ledge between them, from below
            float rT = radAtY(WH);
            lmPushQuad(mav, o01, Vec3(cxW + sinf(a0) * rT, WH, czW + cosf(a0) * rT), Vec3(cxW + sinf(a1) * rT, WH, czW + cosf(a1) * rT), o11, T * 0.65f);
        }
        if (isPortal) {
            lmPushQuad(mav, p00, p10, o10, o00, T * 0.65f);   // soffit of the portal head
            bool prevPortal = (((i + N - 1) % N) == 0 || ((i + N - 1) % N) == 15 || ((i + N - 1) % N) == 3 || ((i + N - 1) % N) == 4 ||
                               ((i + N - 1) % N) == 7 || ((i + N - 1) % N) == 8 || ((i + N - 1) % N) == 11 || ((i + N - 1) % N) == 12);
            bool nextPortal = (((i + 1) % N) == 0 || ((i + 1) % N) == 15 || ((i + 1) % N) == 3 || ((i + 1) % N) == 4 ||
                               ((i + 1) % N) == 7 || ((i + 1) % N) == 8 || ((i + 1) % N) == 11 || ((i + 1) % N) == 12);
            if (!prevPortal) lmPushQuad(mav, Vec3(p00.x, 0.0f, p00.z), Vec3(p00.x, 3.55f, p00.z), Vec3(o00.x, 3.55f, o00.z), Vec3(o00.x, 0.0f, o00.z), T * 0.65f);
            if (!nextPortal) lmPushQuad(mav, Vec3(o10.x, 0.0f, o10.z), Vec3(o10.x, 3.55f, o10.z), Vec3(p10.x, 3.55f, p10.z), Vec3(p10.x, 0.0f, p10.z), T * 0.65f);
        }
        if (!isPortal) {
            float ex = p10.x - p00.x, ez = p10.z - p00.z;
            float mrx = 0.5f * (o00.x + o10.x - p00.x - p10.x), mrz = 0.5f * (o00.z + o10.z - p00.z - p10.z);   // to mid-thickness
            int nCol = (int)ceilf(sqrtf(ex * ex + ez * ez) / 0.34f);
            for (int c = 0; c < nCol; c++) {
                float f = ((float)c + 0.5f) / (float)nCol;
                float mx = p00.x + ex * f + mrx * 0.5f, mz = p00.z + ez * f + mrz * 0.5f;
                outCols.push_back({mx - 0.24f, mz - 0.24f, mx + 0.24f, mz + 0.24f});
            }
        }
        // Deep abyss cylindrical pit liner (0 .. -48m)
        Vec3 b00(cxW + sinf(a0) * rPit, -48.0f, czW + cosf(a0) * rPit);
        Vec3 b01(cxW + sinf(a0) * rPit, 0.0f,   czW + cosf(a0) * rPit);
        Vec3 b11(cxW + sinf(a1) * rPit, 0.0f,   czW + cosf(a1) * rPit);
        Vec3 b10(cxW + sinf(a1) * rPit, -48.0f, czW + cosf(a1) * rPit);
        lmPushQuad(mav, b00, b01, b11, b10, T * 0.65f);
        // the liner is a ring wall SHELL_T thick; its rim lies just under the floor line, so it
        // only shows in the slivers between the round pit and the stepped edge of the floor cells
        float rPo = rPit + SHELL_T;
        Vec3 c00(cxW + sinf(a0) * rPo, -48.0f, czW + cosf(a0) * rPo), c01(cxW + sinf(a0) * rPo, -0.02f, czW + cosf(a0) * rPo);
        Vec3 c11(cxW + sinf(a1) * rPo, -0.02f, czW + cosf(a1) * rPo), c10(cxW + sinf(a1) * rPo, -48.0f, czW + cosf(a1) * rPo);
        lmPushQuad(mav, c10, c11, c01, c00, T * 0.65f);
        lmPushQuad(mav, Vec3(b01.x, -0.02f, b01.z), Vec3(c01.x, -0.02f, c01.z), Vec3(c11.x, -0.02f, c11.z), Vec3(b11.x, -0.02f, b11.z), T * 0.65f);
    }

    // 1b. Enclosed 16-sided tube liner from WH up to ceilY + bold protruding ring galleries!
    int vBands = lmClampInt(s.ceilTiers - 1, 5, 10);
    for (int b = 0; b < vBands; b++) {
        float y0 = WH + (ceilY - WH) * (float)b / (float)vBands;
        float y1 = WH + (ceilY - WH) * (float)(b + 1) / (float)vBands;
        float r0 = radAtY(y0), r1 = radAtY(y1);
        for (int i = 0; i < N; i++) {
            float a0 = tau * (float)i / (float)N, a1 = tau * (float)(i + 1) / (float)N;
            Vec3 p00(cxW + sinf(a0) * r0, y0, czW + cosf(a0) * r0);
            Vec3 p01(cxW + sinf(a0) * r1, y1, czW + cosf(a0) * r1);
            Vec3 p11(cxW + sinf(a1) * r1, y1, czW + cosf(a1) * r1);
            Vec3 p10(cxW + sinf(a1) * r0, y0, czW + cosf(a1) * r0);
            lmPushQuad(mav, p00, p01, p11, p10, T * 0.65f);
            {   // outer skin of the tube, SHELL_T behind the inner one
                float ro0 = r0 + SHELL_T, ro1 = r1 + SHELL_T;
                lmPushQuad(mav, Vec3(cxW + sinf(a1) * ro0, y0, czW + cosf(a1) * ro0), Vec3(cxW + sinf(a1) * ro1, y1, czW + cosf(a1) * ro1),
                                Vec3(cxW + sinf(a0) * ro1, y1, czW + cosf(a0) * ro1), Vec3(cxW + sinf(a0) * ro0, y0, czW + cosf(a0) * ro0), T * 0.65f);
            }
            float ri = r0 - 1.35f;
            Vec3 q0(cxW + sinf(a0) * ri, y0, czW + cosf(a0) * ri);
            Vec3 q1(cxW + sinf(a1) * ri, y0, czW + cosf(a1) * ri);
            lmPushQuad(mv, Vec3(q0.x, y0 - 0.36f, q0.z), Vec3(q0.x, y0 + 0.85f, q0.z),
                           Vec3(q1.x, y0 + 0.85f, q1.z), Vec3(q1.x, y0 - 0.36f, q1.z), T);
            lmPushQuad(mv, Vec3(p00.x, y0 - 0.36f, p00.z), Vec3(q0.x, y0 - 0.36f, q0.z),
                           Vec3(q1.x, y0 - 0.36f, q1.z), Vec3(p10.x, y0 - 0.36f, p10.z), T);
        }
    }
    // Cap ceiling directly on the silo top so no outer ceiling shows
    lmPushBox(mv, cxW - rOuter - 2.0f, ceilY - 0.8f, czW - rOuter - 2.0f, cxW + rOuter + 2.0f, ceilY, czW + rOuter + 2.0f, T);

    // 2. Continuous helical spiral ramp winding from -18 m up to ceilY - 3.0 m with lit inner curb!
    float turns = 3.2f + (float)(s.ceilTiers / 4);
    int segs = (int)(turns * 16.0f);
    float rampStart = -18.0f, rampEnd = ceilY - 2.8f;
    auto rampSegPresent = [&](int k) -> bool {
        if (k < 0 || k >= segs) return false;
        float y0 = rampStart + (rampEnd - rampStart) * (float)k / (float)segs;
        if (y0 > -1.2f && y0 < 3.8f) return false; // clear of y=0 ring catwalk
        return (int)(lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)k, 0x534Cu) % 460u) >= (int)g[LG_DECAY];
    };
    for (int k = 0; k < segs; k++) {
        float f0 = (float)k / (float)segs, f1 = (float)(k + 1) / (float)segs;
        float y0 = rampStart + (rampEnd - rampStart) * f0;
        float y1 = rampStart + (rampEnd - rampStart) * f1;
        if (!rampSegPresent(k)) continue;
        float a0 = f0 * turns * tau, a1 = f1 * turns * tau;
        float rw0 = (y0 < 0.0f ? rPit : radAtY(y0)) - 0.12f;
        float rw1 = (y1 < 0.0f ? rPit : radAtY(y1)) - 0.12f;
        float ri0 = rw0 - 2.55f, ri1 = rw1 - 2.55f;
        Vec3 o0(cxW + sinf(a0) * rw0, y0, czW + cosf(a0) * rw0), o1(cxW + sinf(a1) * rw1, y1, czW + cosf(a1) * rw1);
        Vec3 i0(cxW + sinf(a0) * ri0, y0, czW + cosf(a0) * ri0), i1(cxW + sinf(a1) * ri1, y1, czW + cosf(a1) * ri1);
        lmPushQuad(mv, i0, o0, o1, i1, T);
        lmPushQuad(mv, Vec3(o0.x, y0 - 0.42f, o0.z), Vec3(i0.x, y0 - 0.42f, i0.z), Vec3(i1.x, y1 - 0.42f, i1.z), Vec3(o1.x, y1 - 0.42f, o1.z), T);
        lmPushQuad(mv, Vec3(i0.x, y0 - 0.42f, i0.z), Vec3(i0.x, y0 + 1.02f, i0.z), Vec3(i1.x, y1 + 1.02f, i1.z), Vec3(i1.x, y1 - 0.42f, i1.z), T);
        // the curb is a 22 cm kerb, not a sheet: its walker-side face, its top, and the cut ends of
        // kerb and ramp slab wherever the next piece is missing
        const float kt = 0.22f;
        Vec3 j0(cxW + sinf(a0) * (ri0 + kt), y0, czW + cosf(a0) * (ri0 + kt)), j1(cxW + sinf(a1) * (ri1 + kt), y1, czW + cosf(a1) * (ri1 + kt));
        lmPushQuad(mv, Vec3(j1.x, y1, j1.z), Vec3(j1.x, y1 + 1.02f, j1.z), Vec3(j0.x, y0 + 1.02f, j0.z), Vec3(j0.x, y0, j0.z), T);
        lmPushQuad(mv, Vec3(i0.x, y0 + 1.02f, i0.z), Vec3(j0.x, y0 + 1.02f, j0.z), Vec3(j1.x, y1 + 1.02f, j1.z), Vec3(i1.x, y1 + 1.02f, i1.z), T);
        if (!rampSegPresent(k - 1)) {
            lmPushQuad(mv, Vec3(o0.x, y0 - 0.42f, o0.z), Vec3(o0.x, y0, o0.z), Vec3(i0.x, y0, i0.z), Vec3(i0.x, y0 - 0.42f, i0.z), T);
            lmPushQuad(mv, Vec3(j0.x, y0, j0.z), Vec3(j0.x, y0 + 1.02f, j0.z), Vec3(i0.x, y0 + 1.02f, i0.z), Vec3(i0.x, y0, i0.z), T);
        }
        if (!rampSegPresent(k + 1)) {
            lmPushQuad(mv, Vec3(i1.x, y1 - 0.42f, i1.z), Vec3(i1.x, y1, i1.z), Vec3(o1.x, y1, o1.z), Vec3(o1.x, y1 - 0.42f, o1.z), T);
            lmPushQuad(mv, Vec3(i1.x, y1, i1.z), Vec3(i1.x, y1 + 1.02f, i1.z), Vec3(j1.x, y1 + 1.02f, j1.z), Vec3(j1.x, y1, j1.z), T);
        }
        if (!s.dark && (k % 2) == 0) {
            // the glowing strip stands 2 cm off the kerb face
            Vec3 g0(cxW + sinf(a0) * (ri0 - 0.02f), y0, czW + cosf(a0) * (ri0 - 0.02f)), g1(cxW + sinf(a1) * (ri1 - 0.02f), y1, czW + cosf(a1) * (ri1 - 0.02f));
            lmPushLightQuad(lv, Vec3(g0.x, y0 - 0.36f, g0.z), Vec3(g0.x, y0 - 0.12f, g0.z),
                                Vec3(g1.x, y1 - 0.12f, g1.z), Vec3(g1.x, y1 - 0.36f, g1.z), true);
        }
    }

    // 3. Blazing top oculus + vertical shaft-of-light rings + deep glowing furnace core at -24 m!
    if (!s.dark) {
        float rOc = fminf(rOuter * 0.46f, 8.2f);
        for (int i = 0; i < N; i++) {
            float a0 = tau * (float)i / (float)N, a1 = tau * (float)(i + 1) / (float)N;
            lmPushLightQuad(lv, Vec3(cxW, ceilY - 0.85f, czW),
                                Vec3(cxW + sinf(a0) * rOc, ceilY - 0.85f, czW + cosf(a0) * rOc),
                                Vec3(cxW + sinf(a1) * rOc, ceilY - 0.85f, czW + cosf(a1) * rOc),
                                Vec3(cxW, ceilY - 0.85f, czW), true);
            // Glowing abyss furnace ring + cross grate at -22 m!
            float rB = rPit * 0.76f, rBi = rPit * 0.34f;
            lmPushLightQuad(lv, Vec3(cxW + sinf(a0) * rBi, -22.0f, czW + cosf(a0) * rBi),
                                Vec3(cxW + sinf(a0) * rB,  -22.0f, czW + cosf(a0) * rB),
                                Vec3(cxW + sinf(a1) * rB,  -22.0f, czW + cosf(a1) * rB),
                                Vec3(cxW + sinf(a1) * rBi, -22.0f, czW + cosf(a1) * rBi), true);
        }
        // Suspended glowing light-column rings descending down the oculus shaft!
        for (float ly = ceilY - 5.5f; ly >= 9.0f; ly -= 6.5f) {
            float rr = rOc * (0.52f + 0.36f * (ly / ceilY));
            for (int i = 0; i < 8; i++) {
                float a0 = tau * (float)i / 8.0f, a1 = tau * (float)(i + 1) / 8.0f;
                lmPushLightQuad(lv, Vec3(cxW + sinf(a0)*rr, ly, czW + cosf(a0)*rr),
                                    Vec3(cxW + sinf(a0)*rr, ly + 0.32f, czW + cosf(a0)*rr),
                                    Vec3(cxW + sinf(a1)*rr, ly + 0.32f, czW + cosf(a1)*rr),
                                    Vec3(cxW + sinf(a1)*rr, ly, czW + cosf(a1)*rr), true);
            }
        }
    }
}

// Form 5 (MF_HANGING): brightly rim-lit inverted stepped pyramid crushing down to 2.65 m in the
// hall center + prominent floating mid-air rooms with glowing windows & underside floodlights!
inline void lmBuildMegaHanging(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav,
                               std::vector<float>& dv, std::vector<float>& lv) {
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;

    float margin = (g[LG_CELLS] > 165) ? (3.0f * CS) : (1.5f * CS);
    float px0 = x0 + margin, px1 = x1 - margin, pz0 = z0 + margin, pz1 = z1 - margin;
    if (px1 - px0 >= 20.0f && pz1 - pz0 >= 20.0f) {
        int nSteps = lmClampInt(5 + s.ceilTiers / 2, 5, 9);
        const float nadirY = 2.65f;
        float topDrop = ceilY - 2.2f;
        float wTot = px1 - px0, hTot = pz1 - pz0;
        for (int k = 0; k < nSteps; k++) {
            float frac = (float)k / (float)nSteps;
            float sx0 = px0 + wTot * 0.43f * frac, sx1 = px1 - wTot * 0.43f * frac;
            float sz0 = pz0 + hTot * 0.43f * frac, sz1 = pz1 - hTot * 0.43f * frac;
            float tPrev = (k == 0) ? -0.1f : ((float)(k - 1) / (float)(nSteps - 1));
            float tCur  = (nSteps > 1) ? ((float)k / (float)(nSteps - 1)) : 1.0f;
            float yTop  = (k == 0) ? ceilY : (topDrop + (nadirY - topDrop) * tPrev + 0.08f);
            float yBot  = topDrop + (nadirY - topDrop) * tCur;
            std::vector<float>& dst = ((k & 1) == 0) ? mv : mav;
            lmPushBox(dst, sx0, yBot, sz0, sx1, yTop, sz1, T);
            if (!s.dark) {
                // Continuous glowing step-lip coves on ALL 4 sides of every hanging step!
                lmPushLightQuad(lv, Vec3(sx0, yBot + 0.04f, sz0 - 0.06f), Vec3(sx1, yBot + 0.04f, sz0 - 0.06f), Vec3(sx1, yBot + 0.24f, sz0 - 0.06f), Vec3(sx0, yBot + 0.24f, sz0 - 0.06f), true);
                lmPushLightQuad(lv, Vec3(sx0, yBot + 0.04f, sz1 + 0.06f), Vec3(sx1, yBot + 0.04f, sz1 + 0.06f), Vec3(sx1, yBot + 0.24f, sz1 + 0.06f), Vec3(sx0, yBot + 0.24f, sz1 + 0.06f), true);
                lmPushLightQuad(lv, Vec3(sx0 - 0.06f, yBot + 0.04f, sz0), Vec3(sx0 - 0.06f, yBot + 0.04f, sz1), Vec3(sx0 - 0.06f, yBot + 0.24f, sz1), Vec3(sx0 - 0.06f, yBot + 0.24f, sz0), true);
                lmPushLightQuad(lv, Vec3(sx1 + 0.06f, yBot + 0.04f, sz0), Vec3(sx1 + 0.06f, yBot + 0.04f, sz1), Vec3(sx1 + 0.06f, yBot + 0.24f, sz1), Vec3(sx1 + 0.06f, yBot + 0.24f, sz0), true);
            }
        }
    }

    // 6..10 prominent floating rooms suspended mid-hall with glowing windows & underside floodlights!
    float midX = 0.5f * (x0 + x1), midZ = 0.5f * (z0 + z1);
    float rxSpan = fmaxf((x1 - x0) * 0.34f, 14.0f), rzSpan = fmaxf((z1 - z0) * 0.34f, 14.0f);
    int nBlocks = 6 + (int)g[LG_SHELL] * 4 / 256;
    for (int b = 0; b < nBlocks; b++) {
        u32 hb = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(70 + b), 0x4847u);
        float ang = 6.2831853f * ((float)b + 0.35f) / (float)nBlocks;
        float rad = 0.62f + 0.30f * ((float)(hb % 10u) / 9.0f);
        float cx = midX + sinf(ang) * rxSpan * rad;
        float cz = midZ + cosf(ang) * rzSpan * rad;
        float bw = 6.4f + (float)((hb >> 4) % 5u) * 1.2f;
        float bd = 6.4f + (float)((hb >> 8) % 5u) * 1.2f;
        float bh = 4.2f + (float)((hb >> 12) % 3u) * 1.4f;
        if (cx - bw * 0.5f < x0 + 2.0f || cx + bw * 0.5f > x1 - 2.0f ||
            cz - bd * 0.5f < z0 + 2.0f || cz + bd * 0.5f > z1 - 2.0f) continue;
        float y0 = 4.6f + (float)(b % 3) * 3.4f + ((float)((hb >> 16) % 20u) * 0.1f);
        if (y0 + bh > ceilY - 2.5f) y0 = fmaxf(4.4f, ceilY - bh - 3.0f);
        lmPushBox(mav, cx - bw * 0.5f, y0, cz - bd * 0.5f, cx + bw * 0.5f, y0 + bh, cz + bd * 0.5f, T * 0.7f);
        lmPushBox(mv,  cx - bw * 0.52f, y0 - 0.28f, cz - bd * 0.52f, cx + bw * 0.52f, y0, cz + bd * 0.52f, T);
        if (!s.dark) {
            // Glowing windows on all 4 sides + underside floodlight panel!
            float wy0 = y0 + 1.15f, wy1 = y0 + 2.65f;
            lmPushLightQuad(lv, Vec3(cx - 1.6f, wy0, cz - bd*0.5f - 0.06f), Vec3(cx + 1.6f, wy0, cz - bd*0.5f - 0.06f), Vec3(cx + 1.6f, wy1, cz - bd*0.5f - 0.06f), Vec3(cx - 1.6f, wy1, cz - bd*0.5f - 0.06f), true);
            lmPushLightQuad(lv, Vec3(cx - 1.6f, wy0, cz + bd*0.5f + 0.06f), Vec3(cx + 1.6f, wy0, cz + bd*0.5f + 0.06f), Vec3(cx + 1.6f, wy1, cz + bd*0.5f + 0.06f), Vec3(cx - 1.6f, wy1, cz + bd*0.5f + 0.06f), true);
            lmPushLightQuad(lv, Vec3(cx - bw*0.5f - 0.06f, wy0, cz - 1.6f), Vec3(cx - bw*0.5f - 0.06f, wy0, cz + 1.6f), Vec3(cx - bw*0.5f - 0.06f, wy1, cz + 1.6f), Vec3(cx - bw*0.5f - 0.06f, wy1, cz - 1.6f), true);
            lmPushLightQuad(lv, Vec3(cx + bw*0.5f + 0.06f, wy0, cz - 1.6f), Vec3(cx + bw*0.5f + 0.06f, wy0, cz + 1.6f), Vec3(cx + bw*0.5f + 0.06f, wy1, cz + 1.6f), Vec3(cx + bw*0.5f + 0.06f, wy1, cz - 1.6f), true);
            lmPushLightQuad(lv, Vec3(cx - 1.2f, y0 - 0.31f, cz - 1.2f), Vec3(cx + 1.2f, y0 - 0.31f, cz - 1.2f), Vec3(cx + 1.2f, y0 - 0.31f, cz + 1.2f), Vec3(cx - 1.2f, y0 - 0.31f, cz + 1.2f), true);
        }
    }
}

// Form 6 (MF_CELLBLOCK): 7..12 storey apartment/archive courtyard with brick/panel facades (mav),
// large unobstructed 1.65 x 2.05 m emissive windows (LG_LIGHT + LG_ORDER rhythm) & open-rail balconies!
inline void lmBuildMegaCellblock(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav,
                                 std::vector<float>& dv, std::vector<float>& lv) {
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;
    if (s.w < 13 || s.h < 13) return;
    int maxC = lmClampInt(12 + (int)g[LG_GRAIN] / 48, 12, 18);
    int cW = (s.w - 4 < maxC) ? (s.w - 4) : maxC;
    int cH = (s.h - 4 < maxC) ? (s.h - 4) : maxC;
    int wx0 = (s.w - cW) / 2 - 1 + s.megaOffX, wx1 = wx0 + cW + 1;
    int wz0 = (s.h - cH) / 2 - 1 + s.megaOffZ, wz1 = wz0 + cH + 1;
    float cx0 = (s.rect.x0 + 1 + wx0 + 1) * CS, cx1 = (s.rect.x0 + 1 + wx1) * CS;
    float cz0 = (s.rect.z0 + 1 + wz0 + 1) * CS, cz1 = (s.rect.z0 + 1 + wz1) * CS;
    if (cx1 - cx0 < 15.0f || cz1 - cz0 < 15.0f) return;

    // Contrasting brick/panel facade backdrop on all 4 courtyard walls (WH .. ceilY)
    lmPushQuad(mav, Vec3(cx1, WH, cz0 + 0.04f), Vec3(cx1, ceilY, cz0 + 0.04f), Vec3(cx0, ceilY, cz0 + 0.04f), Vec3(cx0, WH, cz0 + 0.04f), T * 0.7f);
    lmPushQuad(mav, Vec3(cx0, WH, cz1 - 0.04f), Vec3(cx0, ceilY, cz1 - 0.04f), Vec3(cx1, ceilY, cz1 - 0.04f), Vec3(cx1, WH, cz1 - 0.04f), T * 0.7f);
    lmPushQuad(mav, Vec3(cx0 + 0.04f, WH, cz0), Vec3(cx0 + 0.04f, ceilY, cz0), Vec3(cx0 + 0.04f, ceilY, cz1), Vec3(cx0 + 0.04f, WH, cz1), T * 0.7f);
    lmPushQuad(mav, Vec3(cx1 - 0.04f, WH, cz1), Vec3(cx1 - 0.04f, ceilY, cz1), Vec3(cx1 - 0.04f, ceilY, cz0), Vec3(cx1 - 0.04f, WH, cz0), T * 0.7f);
    // An approach corridor cut through the block looks up at the facade above its own mouth from
    // behind (everywhere else that side is buried in mass). So the facade is a wall FT thick:
    // a back face and a soffit along its foot.
    const float FT = 0.34f;
    lmPushQuad(mav, Vec3(cx0, WH, cz0 + 0.04f - FT), Vec3(cx0, ceilY, cz0 + 0.04f - FT), Vec3(cx1, ceilY, cz0 + 0.04f - FT), Vec3(cx1, WH, cz0 + 0.04f - FT), T * 0.7f);
    lmPushQuad(mav, Vec3(cx1, WH, cz1 - 0.04f + FT), Vec3(cx1, ceilY, cz1 - 0.04f + FT), Vec3(cx0, ceilY, cz1 - 0.04f + FT), Vec3(cx0, WH, cz1 - 0.04f + FT), T * 0.7f);
    lmPushQuad(mav, Vec3(cx0 + 0.04f - FT, WH, cz1), Vec3(cx0 + 0.04f - FT, ceilY, cz1), Vec3(cx0 + 0.04f - FT, ceilY, cz0), Vec3(cx0 + 0.04f - FT, WH, cz0), T * 0.7f);
    lmPushQuad(mav, Vec3(cx1 - 0.04f + FT, WH, cz0), Vec3(cx1 - 0.04f + FT, ceilY, cz0), Vec3(cx1 - 0.04f + FT, ceilY, cz1), Vec3(cx1 - 0.04f + FT, WH, cz1), T * 0.7f);
    lmPushQuad(mav, Vec3(cx0, WH + 0.004f, cz0 + 0.04f - FT), Vec3(cx1, WH + 0.004f, cz0 + 0.04f - FT), Vec3(cx1, WH + 0.004f, cz0 + 0.04f), Vec3(cx0, WH + 0.004f, cz0 + 0.04f), T * 0.7f);
    lmPushQuad(mav, Vec3(cx0, WH + 0.004f, cz1 - 0.04f), Vec3(cx1, WH + 0.004f, cz1 - 0.04f), Vec3(cx1, WH + 0.004f, cz1 - 0.04f + FT), Vec3(cx0, WH + 0.004f, cz1 - 0.04f + FT), T * 0.7f);
    lmPushQuad(mav, Vec3(cx0 + 0.04f - FT, WH + 0.004f, cz0), Vec3(cx0 + 0.04f, WH + 0.004f, cz0), Vec3(cx0 + 0.04f, WH + 0.004f, cz1), Vec3(cx0 + 0.04f - FT, WH + 0.004f, cz1), T * 0.7f);
    lmPushQuad(mav, Vec3(cx1 - 0.04f, WH + 0.004f, cz0), Vec3(cx1 - 0.04f + FT, WH + 0.004f, cz0), Vec3(cx1 - 0.04f + FT, WH + 0.004f, cz1), Vec3(cx1 - 0.04f, WH + 0.004f, cz1), T * 0.7f);

    float bayW = 5.0f;
    int litChance = s.dark ? 18 : (64 + (int)g[LG_LIGHT] * 155 / 255);
    float balProj = 1.05f;

    auto buildFacadeWall = [&](int sideId, float bx, float bz, float tx, float tz, float nx, float nz, float wallLen) {
        int nBays = (int)floorf(wallLen / bayW);
        if (nBays < 2) return;
        float step = wallLen / (float)nBays;
        Vec3 Tv(tx, 0.0f, tz), Nv(nx, 0.0f, nz);

        for (int b = 0; b <= nBays; b++) {
            Vec3 p = Vec3(bx, 0.0f, bz) + Tv * ((float)b * step) + Nv * 0.24f;
            lmPushBox(mv, p.x - 0.20f, WH, p.z - 0.20f, p.x + 0.20f, ceilY, p.z + 0.20f, T);
        }

        for (int st = 1; st < s.ceilTiers; st++) {
            float y = (float)st * WH;
            u32 hRow = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(sideId * 31 + st), 0x4346u);
            // Thin floor-cornice / balcony slab + slender open top rail (so windows y+1.25..3.25 stay 100% unobstructed!)
            Vec3 b0 = Vec3(bx, 0, bz), b1 = Vec3(bx, 0, bz) + Tv * wallLen;
            Vec3 e0 = b0 + Nv * balProj, e1 = b1 + Nv * balProj;
            float xMin = fminf(fminf(b0.x, b1.x), fminf(e0.x, e1.x)), xMax = fmaxf(fmaxf(b0.x, b1.x), fmaxf(e0.x, e1.x));
            float zMin = fminf(fminf(b0.z, b1.z), fminf(e0.z, e1.z)), zMax = fmaxf(fmaxf(b0.z, b1.z), fmaxf(e0.z, e1.z));
            lmPushBox(mv, xMin, y - 0.22f, zMin, xMax, y + 0.02f, zMax, T);
            lmPushDecorQuadRaw(dv, Vec3(e0.x, y + 0.84f, e0.z), Vec3(e1.x, y + 0.84f, e1.z),
                                   Vec3(e1.x, y + 0.96f, e1.z), Vec3(e0.x, y + 0.96f, e0.z), Nv, DECOR_MAT_DARK_METAL);

            for (int b = 0; b < nBays; b++) {
                u32 hCell = lmHash(hRow, (u32)b, 3u, 0x574Eu);
                float uMid = ((float)b + 0.5f) * step;
                // Orderly genomes light entire floors/columns in architectural rhythm!
                int rhythmRoll = (g[LG_ORDER] > 145) ? (int)(((st * 37 + b * 13 + sideId * 7) * 29) & 255) : (int)(hCell & 255u);
                bool isLit = (rhythmRoll < litChance);
                // Frame + large 1.65 x 2.00 m apartment window well above balcony slab!
                Vec3 f0 = Vec3(bx, 0, bz) + Tv * (uMid - 0.94f) + Nv * 0.08f;
                Vec3 f1 = Vec3(bx, 0, bz) + Tv * (uMid + 0.94f) + Nv * 0.08f;
                lmPushDecorQuadRaw(dv, Vec3(f0.x, y + 1.12f, f0.z), Vec3(f1.x, y + 1.12f, f1.z),
                                       Vec3(f1.x, y + 3.32f, f1.z), Vec3(f0.x, y + 3.32f, f0.z), Nv,
                                       (isLit && (hCell & 8u)) ? DECOR_MAT_CAUTION : DECOR_MAT_DARK_METAL);
                Vec3 w0 = Vec3(bx, 0, bz) + Tv * (uMid - 0.80f) + Nv * 0.11f;
                Vec3 w1 = Vec3(bx, 0, bz) + Tv * (uMid + 0.80f) + Nv * 0.11f;
                if (isLit) {
                    lmPushLightQuad(lv, Vec3(w0.x, y + 1.24f, w0.z), Vec3(w1.x, y + 1.24f, w1.z),
                                        Vec3(w1.x, y + 3.20f, w1.z), Vec3(w0.x, y + 3.20f, w0.z), true);
                } else {
                    lmPushDecorQuadRaw(dv, Vec3(w0.x, y + 1.24f, w0.z), Vec3(w1.x, y + 1.24f, w1.z),
                                           Vec3(w1.x, y + 3.20f, w1.z), Vec3(w0.x, y + 3.20f, w0.z), Nv, DECOR_MAT_RUBBER);
                }
            }
        }
    };

    buildFacadeWall(0, cx0, cz0,  1,  0,  0,  1, cx1 - cx0);
    buildFacadeWall(1, cx1, cz1, -1,  0,  0, -1, cx1 - cx0);
    buildFacadeWall(2, cx0, cz1,  0, -1,  1,  0, cz1 - cz0);
    buildFacadeWall(3, cx1, cz0,  0,  1, -1,  0, cz1 - cz0);
}

// Form 7 (MF_PLATFORM): aimless transit station / dry canal — sunken rail trackways with sleepers
// & third rail running straight into sealed end-wall tunnel portals + T-pillar platform canopies.
inline void lmBuildMegaPlatform(const LimenSlab& s, float T, std::vector<float>& mv,
                                std::vector<float>& dv, std::vector<float>& lv) {
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;
    int axis = (s.megaSub & 1);
    if ((axis == 0 ? s.h : s.w) < 9) axis ^= 1;

    // Station concourse roof trusses
    int L = (axis == 0) ? s.w : s.h, C = (axis == 0) ? s.h : s.w;
    for (int a = 2; a < L - 1; a += 3) {
        float wa = ((axis == 0 ? s.rect.x0 : s.rect.z0) + 1 + a + 0.5f) * CS;
        if (axis == 0) lmPushBox(mv, wa - 0.45f, ceilY - 2.2f, z0, wa + 0.45f, ceilY, z1, T);
        else           lmPushBox(mv, x0, ceilY - 2.2f, wa - 0.45f, x1, ceilY, wa + 0.45f, T);
    }

    // Scan cross-slices to find every sunken trackway band (LT_BASIN)
    int midA = L / 2;
    int c = 0;
    const float trackBot = -1.42f;
    while (c < C) {
        int t = (axis == 0) ? s.tag[midA][c] : s.tag[c][midA];
        bool isTrench = (t == LT_BASIN) || (t == LT_BRIDGE && (((axis == 0 ? s.flag[midA][c] : s.flag[c][midA]) & LF_OVER_BASIN) != 0));
        if (!isTrench) { c++; continue; }
        int cStart = c;
        while (c < C) {
            int tc = (axis == 0) ? s.tag[midA][c] : s.tag[c][midA];
            unsigned short fc = (axis == 0) ? s.flag[midA][c] : s.flag[c][midA];
            if (tc != LT_BASIN && !(tc == LT_BRIDGE && (fc & LF_OVER_BASIN))) break;
            c++;
        }
        int cEnd = c - 1;
        // Find along-axis extent [aMin..aMax] of this track trench
        int aMin = L, aMax = -1;
        for (int a = 0; a < L; a++) {
            int ta = (axis == 0) ? s.tag[a][cStart] : s.tag[cStart][a];
            if (ta == LT_BASIN) { if (a < aMin) aMin = a; if (a > aMax) aMax = a; }
        }
        if (aMax < aMin) continue;

        float wa0 = ((axis == 0 ? s.rect.x0 : s.rect.z0) + 1 + aMin) * CS;
        float wa1 = ((axis == 0 ? s.rect.x0 : s.rect.z0) + 2 + aMax) * CS;
        float wc0 = ((axis == 0 ? s.rect.z0 : s.rect.x0) + 1 + cStart) * CS;
        float wc1 = ((axis == 0 ? s.rect.z0 : s.rect.x0) + 2 + cEnd) * CS;
        int nTracks = (cEnd - cStart + 1 >= 2) ? 2 : 1;

        for (int tr = 0; tr < nTracks; tr++) {
            float tcMid = wc0 + (wc1 - wc0) * ((float)tr + 0.5f) / (float)nTracks;
            // Sleepers every 1.4 m
            for (float a = wa0 + 0.7f; a <= wa1 - 0.7f; a += 1.4f) {
                if (axis == 0) lmPushDecorQuadRaw(dv, Vec3(a-0.16f, trackBot+0.03f, tcMid-1.05f), Vec3(a-0.16f, trackBot+0.03f, tcMid+1.05f),
                                                      Vec3(a+0.16f, trackBot+0.03f, tcMid+1.05f), Vec3(a+0.16f, trackBot+0.03f, tcMid-1.05f), Vec3(0,1,0), DECOR_MAT_DARK_METAL);
                else           lmPushDecorQuadRaw(dv, Vec3(tcMid-1.05f, trackBot+0.03f, a-0.16f), Vec3(tcMid-1.05f, trackBot+0.03f, a+0.16f),
                                                      Vec3(tcMid+1.05f, trackBot+0.03f, a+0.16f), Vec3(tcMid+1.05f, trackBot+0.03f, a-0.16f), Vec3(0,1,0), DECOR_MAT_DARK_METAL);
            }
            // Twin steel running rails + third power rail disappearing straight into both end bulkheads!
            const float railOff[3] = {-0.72f, 0.72f, 1.18f};
            for (int rl = 0; rl < 3; rl++) {
                float rc = tcMid + railOff[rl];
                int mat = (rl == 2) ? DECOR_MAT_CAUTION : DECOR_MAT_METAL;
                if (axis == 0) lmPushBox(mv, wa0, trackBot + 0.04f, rc - 0.05f, wa1, trackBot + 0.18f, rc + 0.05f, T);
                else           lmPushBox(mv, rc - 0.05f, trackBot + 0.04f, wa0, rc + 0.05f, trackBot + 0.18f, wa1, T);
                (void)mat;
            }
        }

        // Monumental recessed tunnel portals + block signals on BOTH end walls!
        float tcCenter = 0.5f * (wc0 + wc1);
        float portalW = fminf((wc1 - wc0) - 0.5f, 8.8f);
        for (int end = 0; end < 2; end++) {
            float waEnd = (end == 0) ? (wa0 + 0.06f) : (wa1 - 0.06f);
            float nDir  = (end == 0) ? 1.0f : -1.0f;
            float pYaw  = (axis == 0) ? (end == 0 ? 1.57079633f : -1.57079633f) : (end == 0 ? 0.0f : 3.14159265f);
            float px = (axis == 0) ? waEnd : tcCenter, pz = (axis == 0) ? tcCenter : waEnd;
            DecorBuilder pb(dv, px, pz, pYaw, 1.0f);
            pb.box(-portalW * 0.5f - 0.42f, trackBot, 0.25f, 0.84f, 5.80f, 0.85f, DECOR_MAT_TRIM);
            pb.box( portalW * 0.5f + 0.42f, trackBot, 0.25f, 0.84f, 5.80f, 0.85f, DECOR_MAT_TRIM);
            pb.box(0.0f, 4.10f, 0.28f, portalW + 1.72f, 0.92f, 0.92f, DECOR_MAT_TRIM);
            pb.box(0.0f, trackBot, 0.08f, portalW, 4.15f, 0.22f, DECOR_MAT_RUBBER);
            // Glowing block signal & tunnel portal number lightbox!
            pb.box(portalW * 0.5f + 0.42f, 2.15f, 0.72f, 0.36f, 0.88f, 0.18f, DECOR_MAT_DARK_METAL);
            pb.panel(portalW * 0.5f + 0.42f, 2.52f, 0.82f, 0.22f, 0.22f, 0.04f, DECOR_MAT_ORANGE);
            if (!s.dark) lmBuildExitLightbox(px + (axis == 0 ? nDir * 0.48f : 0.0f), 4.25f,
                                             pz + (axis == 0 ? 0.0f : nDir * 0.48f), pYaw, dv, lv);
        }

        // Overhead platform canopies + blank departure boards + double-sided station clocks!
        for (int side = -1; side <= 1; side += 2) {
            float rimC = (side < 0) ? (wc0 - 2.1f) : (wc1 + 2.1f);
            if (rimC < (axis == 0 ? z0 + 2.0f : x0 + 2.0f) || rimC > (axis == 0 ? z1 - 2.0f : x1 - 2.0f)) continue;
            if (axis == 0) {
                lmPushBox(mv, wa0 + 2.0f, 3.82f, rimC - 1.85f, wa1 - 2.0f, 4.16f, rimC + 1.85f, T);
                if (!s.dark) lmPushLightQuad(lv, Vec3(wa0+4.0f, 3.79f, rimC-0.32f), Vec3(wa1-4.0f, 3.79f, rimC-0.32f),
                                                 Vec3(wa1-4.0f, 3.79f, rimC+0.32f), Vec3(wa0+4.0f, 3.79f, rimC+0.32f), true);
            } else {
                lmPushBox(mv, rimC - 1.85f, 3.82f, wa0 + 2.0f, rimC + 1.85f, 4.16f, wa1 - 2.0f, T);
                if (!s.dark) lmPushLightQuad(lv, Vec3(rimC-0.32f, 3.79f, wa0+4.0f), Vec3(rimC+0.32f, 3.79f, wa0+4.0f),
                                                 Vec3(rimC+0.32f, 3.79f, wa1-4.0f), Vec3(rimC-0.32f, 3.79f, wa1-4.0f), true);
            }
            int idx = 0;
            for (float a = wa0 + 8.0f; a <= wa1 - 8.0f; a += 13.0f, idx++) {
                if ((int)(lmHash((u32)a, (u32)(side + 2), 3u, 0x5043u) % 400u) < (int)g[LG_DECAY]) continue;
                float px = (axis == 0) ? a : rimC, pz = (axis == 0) ? rimC : a;
                DecorBuilder sb(dv, px, pz, (axis == 0) ? 1.57079633f : 0.0f, 1.0f);
                if ((idx & 1) == 0) {
                    // Blank/empty departure board
                    sb.box(0.0f, 2.82f, 0.0f, 0.16f, 1.00f, 2.15f, DECOR_MAT_DARK_METAL);
                    sb.panel(0.0f, 2.86f, 0.0f, 0.20f, 0.48f, 2.02f, DECOR_MAT_SCREEN);
                } else {
                    // Station drum clock with handless/frozen dial!
                    sb.box(0.0f, 3.35f, 0.0f, 0.10f, 0.48f, 0.10f, DECOR_MAT_DARK_METAL);
                    sb.box(0.0f, 2.75f, 0.0f, 0.24f, 0.64f, 0.64f, DECOR_MAT_DARK_METAL);
                    sb.box(0.0f, 2.81f, 0.0f, 0.28f, 0.52f, 0.52f, DECOR_MAT_WHITE);
                    sb.box(0.0f, 3.04f, 0.06f, 0.31f, 0.20f, 0.04f, DECOR_MAT_DARK_METAL);
                }
            }
        }
    }
}
