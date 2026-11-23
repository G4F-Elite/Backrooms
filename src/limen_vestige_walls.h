#pragma once
// LIMEN vestigial architecture, Stage-2 dead thresholds & sealed vitrines:
//   - walkable 10-step tiled staircases descending into drained LT_BASIN pools
//   - plastered-shut ghost doorway scars at LS_DOOR_GAP & room-number plaques
//   - glowing institutional EXIT lightboxes pointing over severed spans & plugged halls
//   - closed doors embedded in blank walls (LS_WALL_DOOR), rhythmic numbered corridors
//     ending in a solid concrete plug (LZ_PLUG_CORR + LS_PLUG_CORR), staircases ascending
//     straight into flat ceilings (LS_CEIL_STAIR), vertically phase-shifted doors/props
//     (LS_PHASE_SHIFT), and glazed windows into sealed lit furnished rooms (LT_VITRINE).

struct LimenAabb2D { float x0, z0, x1, z1; };
inline std::vector<LimenAabb2D> gLmVestigeColliders;

inline float lmVitrineSillY(int wx, int wz) {
    return (lmDeliriumCell255(wx, wz) >= 80) ? 0.08f : 0.96f; // floor-level window in drunken pockets!
}

inline bool lmCollideVestiges(float x, float z, float PR) {
    for (const auto& b : gLmVestigeColliders) {
        if (circleOverlapsAabb2D(x, z, PR, b.x0, b.x1, b.z0, b.z1)) return true;
    }
    return false;
}

inline void lmPushQuad(std::vector<float>& v, Vec3 a, Vec3 b, Vec3 c, Vec3 d, float tile);
inline void lmPushBox(std::vector<float>& v, float x0, float y0, float z0, float x1, float y1, float z1, float tile);
inline void lmPushWallSpan(std::vector<float>& v, float x, float y0, float z, float dx, float dz, float y1);
inline void lmPushLightQuad(std::vector<float>& lv, Vec3 a, Vec3 b, Vec3 c, Vec3 d, bool twoSided);
inline void lmPushLightBox(std::vector<float>& lv, float cx, float y0, float cz, float hx, float hy, float hz);

inline void lmBuildExitLightbox(float wx, float y, float wz, float yaw,
                                std::vector<float>& dv, std::vector<float>& lv) {
    DecorBuilder b(dv, wx, wz, yaw, 1.0f);
    b.box(0.0f, y, 0.0f, 0.92f, 0.36f, 0.12f, DECOR_MAT_DARK_METAL);
    b.box(-0.22f, y + 0.09f, 0.065f, 0.10f, 0.18f, 0.02f, DECOR_MAT_CAUTION);
    b.box( 0.02f, y + 0.09f, 0.065f, 0.10f, 0.18f, 0.02f, DECOR_MAT_CAUTION);
    b.box( 0.25f, y + 0.13f, 0.065f, 0.16f, 0.06f, 0.02f, DECOR_MAT_CAUTION);
    Vec3 c0 = b.world(-0.41f, y + 0.04f, 0.07f);
    Vec3 c1 = b.world( 0.41f, y + 0.04f, 0.07f);
    Vec3 c2 = b.world( 0.41f, y + 0.32f, 0.07f);
    Vec3 c3 = b.world(-0.41f, y + 0.32f, 0.07f);
    lmPushLightQuad(lv, c0, c1, c2, c3, true);
}

// Closed architectural door leaf + frame + push-bar mounted flush on a solid wall face.
inline void lmBuildClosedWallDoor(float cx, float yBase, float cz, float yaw, bool withPlaque, std::vector<float>& dv) {
    DecorBuilder b(dv, cx, cz, yaw, 1.0f);
    const float doorW = 1.12f, doorH = 2.22f, postW = 0.12f;
    b.box(-doorW * 0.5f - postW * 0.5f, yBase, 0.04f, postW, doorH + postW, 0.12f, DECOR_MAT_TRIM);
    b.box( doorW * 0.5f + postW * 0.5f, yBase, 0.04f, postW, doorH + postW, 0.12f, DECOR_MAT_TRIM);
    b.box(0.0f, yBase + doorH, 0.04f, doorW + postW * 2.0f, postW, 0.12f, DECOR_MAT_TRIM);
    b.panel(0.0f, yBase, 0.035f, doorW, doorH, 0.07f, DECOR_MAT_DOOR);
    b.box(0.0f, yBase + 1.02f, 0.085f, doorW * 0.72f, 0.045f, 0.04f, DECOR_MAT_METAL);
    b.box(doorW * 0.36f, yBase + 0.94f, 0.085f, 0.05f, 0.14f, 0.04f, DECOR_MAT_DARK_METAL);
    if (withPlaque) {
        b.box(doorW * 0.5f + 0.34f, yBase + 1.58f, 0.03f, 0.28f, 0.14f, 0.03f, DECOR_MAT_DARK_METAL);
        b.box(doorW * 0.5f + 0.28f, yBase + 1.62f, 0.045f, 0.045f, 0.065f, 0.015f, DECOR_MAT_WHITE);
        b.box(doorW * 0.5f + 0.34f, yBase + 1.62f, 0.045f, 0.045f, 0.065f, 0.015f, DECOR_MAT_WHITE);
        b.box(doorW * 0.5f + 0.40f, yBase + 1.62f, 0.045f, 0.045f, 0.065f, 0.015f, DECOR_MAT_WHITE);
    }
}

inline void lmBuildSlabDoorScarsAndPlates(const LimenSlab& s, std::vector<float>& dv) {
    static const int dx[4] = {0, 0, -1, 1}, dz[4] = {-1, 1, 0, 0};
    static const float faceYaw[4] = {0.0f, 3.14159265f, 1.57079633f, -1.57079633f};

    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].type != LS_DOOR_GAP) continue;
        int lx = s.sites[k].lx, lz = s.sites[k].lz;
        if (lmTagLocal(s, lx, lz) != LT_WALL) continue;
        for (int d = 0; d < 4; d++) {
            int nt = lmTagLocal(s, lx + dx[d], lz + dz[d]);
            if (nt != LT_CORR && nt != LT_LOW && nt != LT_HALL) continue;
            float cx = (s.rect.x0 + 1 + lx + 0.5f + dx[d] * 0.5f) * CS - dx[d] * 0.03f;
            float cz = (s.rect.z0 + 1 + lz + 0.5f + dz[d] * 0.5f) * CS - dz[d] * 0.03f;
            DecorBuilder b(dv, cx, cz, faceYaw[d], 1.0f);
            b.box(0.0f, 0.0f, 0.02f, 1.24f, 2.26f, 0.04f, DECOR_MAT_PLASTER);
            b.box(-0.64f, 0.0f, 0.03f, 0.08f, 2.30f, 0.05f, DECOR_MAT_TRIM);
            b.box( 0.64f, 0.0f, 0.03f, 0.08f, 2.30f, 0.05f, DECOR_MAT_TRIM);
            b.box( 0.0f, 2.28f, 0.03f, 1.36f, 0.08f, 0.05f, DECOR_MAT_TRIM);
            b.box(0.92f, 1.58f, 0.03f, 0.28f, 0.14f, 0.03f, DECOR_MAT_DARK_METAL);
            break;
        }
    }

    bool orderly = s.genome.g[LG_ORDER] >= 95;
    if (!orderly && s.genome.g[LG_DECAY] < 100) return;
    int placed = 0;
    for (int lx = 1; lx < s.w - 1 && placed < 28; lx++) {
        for (int lz = 1; lz < s.h - 1 && placed < 28; lz++) {
            int t = s.tag[lx][lz];
            if (t != LT_CORR && t != LT_LOW) continue;
            bool wX = (s.tag[lx - 1][lz] == LT_WALL && s.tag[lx + 1][lz] == LT_WALL);
            bool wZ = (s.tag[lx][lz - 1] == LT_WALL && s.tag[lx][lz + 1] == LT_WALL);
            if (wX == wZ) continue;
            bool touchesRoom = false;
            int corrSide = -1;
            for (int d = 0; d < 4; d++) {
                int nt = lmTagLocal(s, lx + dx[d], lz + dz[d]);
                if (nt == LT_ROOM) touchesRoom = true;
                if (nt == LT_CORR || nt == LT_LOW) corrSide = d;
            }
            if (!touchesRoom || corrSide < 0) continue;
            bool shifted = lmSlabHasSite(s, lx, lz, LS_DOOR_SHIFT);
            float py = shifted ? 1.12f : 1.62f;
            float cx = (s.rect.x0 + 1 + lx + 0.5f + dx[corrSide] * 0.5f) * CS + dx[corrSide] * 0.03f;
            float cz = (s.rect.z0 + 1 + lz + 0.5f + dz[corrSide] * 0.5f) * CS + dz[corrSide] * 0.03f;
            DecorBuilder b(dv, cx, cz, faceYaw[corrSide ^ 1], 1.0f);
            b.box(1.02f, py, 0.02f, 0.30f, 0.15f, 0.03f, DECOR_MAT_DARK_METAL);
            b.box(0.96f, py + 0.04f, 0.04f, 0.05f, 0.07f, 0.02f, DECOR_MAT_WHITE);
            b.box(1.03f, py + 0.04f, 0.04f, 0.05f, 0.07f, 0.02f, DECOR_MAT_WHITE);
            b.box(1.10f, py + 0.04f, 0.04f, 0.05f, 0.07f, 0.02f, DECOR_MAT_WHITE);
            placed++;
        }
    }
}

// Emits a closed solid box into wall-textured `wv` (pos3 + uv2 + norm3).
inline void lmPushWallSolidBox(std::vector<float>& wv, float x0, float y0, float z0, float x1, float y1, float z1) {
    // (a wall span faces (dz, 0, -dx): the four sides must look outward, not into the box)
    lmPushWallSpan(wv, x1, y0, z1, x0 - x1, 0.0f, y1);
    lmPushWallSpan(wv, x0, y0, z0, x1 - x0, 0.0f, y1);
    lmPushWallSpan(wv, x1, y0, z0, 0.0f, z1 - z0, y1);
    lmPushWallSpan(wv, x0, y0, z1, 0.0f, z0 - z1, y1);
    float tx = (x1 - x0) / CS, tz = (z1 - z0) / CS;
    float top[] = {
        x0,y1,z0, 0,0, 0,1,0,  x0,y1,z1, 0,tz, 0,1,0,  x1,y1,z1, tx,tz, 0,1,0,
        x0,y1,z0, 0,0, 0,1,0,  x1,y1,z1, tx,tz, 0,1,0, x1,y1,z0, tx,0, 0,1,0
    };
    wv.insert(wv.end(), top, top + 48);
}

// Builds Stage-2 dead thresholds across EVERY slab (both 1-storey and vault):
//   - LS_WALL_DOOR: closed doors flush on solid walls
//   - LZ_PLUG_CORR + LS_PLUG_CORR: rhythmic hotel doors & plaques ending at a raw concrete plug
//   - LS_CEIL_STAIR: staircases climbing straight into the flat ceiling slab
//   - LS_PHASE_SHIFT: vertically phase-shifted doors & props (+1.35 m or -0.45 m)
inline void lmBuildSlabDeadThresholds(const LimenSlab& s, std::vector<float>& wv, std::vector<float>& dv,
                                      std::vector<float>& lv, std::vector<LimenAabb2D>& outCols) {
    static const int dx[4] = {0, 0, -1, 1}, dz[4] = {-1, 1, 0, 0};
    static const float inwardYaw[4] = {0.0f, 3.14159265f, 1.57079633f, -1.57079633f};

    // 0. Foreshadowing Halo Thresholds: where a human-scale slab opens onto an adjacent monumental
    //    vault (`s.haloDir`), frame the seam mouths with heavy concrete/steel warning collars & EXIT signs!
    if (!lmSlabIsVault(s) && s.haloDir >= 0 && !s.dark) {
        int hd = s.haloDir; // 0=West, 1=East, 2=North, 3=South
        int placedHalo = 0;
        for (int step = 2; step < (hd < 2 ? s.h - 2 : s.w - 2) && placedHalo < 4; step++) {
            int lx = (hd == 0) ? 0 : ((hd == 1) ? (s.w - 1) : step);
            int lz = (hd == 2) ? 0 : ((hd == 3) ? (s.h - 1) : step);
            if (!lmIsWalkableTag(s.tag[lx][lz]) || (s.zone[lx][lz] & LZ_STORY) != 0) continue;
            int seamWX = (hd == 0) ? s.rect.x0 : ((hd == 1) ? s.rect.x1 : (s.rect.x0 + 1 + lx));
            int seamWZ = (hd == 2) ? s.rect.z0 : ((hd == 3) ? s.rect.z1 : (s.rect.z0 + 1 + lz));
            if (lmTagAt(seamWX, seamWZ) == LT_WALL) continue;
            // Skip wide open colonnade seams; frame discrete tunnel mouths!
            int perpX = (hd < 2) ? 0 : 1, perpZ = (hd < 2) ? 1 : 0;
            if (lmTagAt(seamWX - perpX, seamWZ - perpZ) != LT_WALL ||
                lmTagAt(seamWX + perpX, seamWZ + perpZ) != LT_WALL) continue;
            float cH = lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);
            float dirX = (hd == 0) ? -1.0f : ((hd == 1) ? 1.0f : 0.0f);
            float dirZ = (hd == 2) ? -1.0f : ((hd == 3) ? 1.0f : 0.0f);
            float mx = (s.rect.x0 + 1 + lx + 0.5f + dirX * 0.44f) * CS;
            float mz = (s.rect.z0 + 1 + lz + 0.5f + dirZ * 0.44f) * CS;
            float yaw = atan2f(-dirX, -dirZ);
            DecorBuilder hb(dv, mx, mz, yaw, 1.0f);
            hb.box(0.0f, cH - 0.24f, 0.0f, CS - 0.16f, 0.22f, 0.24f, DECOR_MAT_DARK_METAL);
            hb.box(0.0f, 0.008f, 0.0f, CS - 0.20f, 0.016f, 0.22f, DECOR_MAT_CAUTION);
            lmBuildExitLightbox(mx - dirX * 0.16f, cH - 0.62f, mz - dirZ * 0.16f, yaw, dv, lv);
            placedHalo++;
        }
    }

    // 1. Plugged numbered corridor (`LZ_PLUG_CORR`): doors & plaques on both walls along the spur
    for (int lx = 1; lx < s.w - 1; lx++) {
        for (int lz = 1; lz < s.h - 1; lz++) {
            if ((s.zone[lx][lz] & LZ_PLUG_CORR) == 0) continue;
            for (int d = 0; d < 4; d++) {
                int nwx = lx + dx[d], nwz = lz + dz[d];
                if (lmTagLocal(s, nwx, nwz) != LT_WALL) continue;
                if (lmSlabHasSite(s, nwx, nwz, LS_VITRINE_WIN)) continue;
                // Check if this wall is a side flank rather than the dead-end plug wall
                bool isPlugEnd = lmSlabHasSite(s, lx, lz, LS_PLUG_CORR) && !lmIsWalkableTag(lmTagLocal(s, lx - dx[d], lz - dz[d])) == false;
                (void)isPlugEnd;
                bool walkAhead = lmIsWalkableTag(lmTagLocal(s, lx + dz[d], lz + dx[d])) ||
                                 lmIsWalkableTag(lmTagLocal(s, lx - dz[d], lz - dx[d]));
                float cx = (s.rect.x0 + 1 + lx + 0.5f + dx[d] * 0.5f) * CS - dx[d] * 0.02f;
                float cz = (s.rect.z0 + 1 + lz + 0.5f + dz[d] * 0.5f) * CS - dz[d] * 0.02f;
                if (walkAhead) {
                    lmBuildClosedWallDoor(cx, 0.0f, cz, inwardYaw[d], true, dv);
                } else if (lmSlabHasSite(s, lx, lz, LS_PLUG_CORR)) {
                    // Dead-end plug bulkhead sealing the corridor abruptly!
                    float cH = lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);
                    DecorBuilder pb(dv, cx, cz, inwardYaw[d], 1.0f);
                    pb.box(0.0f, 0.0f, 0.14f, CS - 0.12f, cH, 0.28f, DECOR_MAT_PLASTER);
                    pb.box(0.0f, 0.95f, 0.30f, CS - 0.70f, 0.22f, 0.08f, DECOR_MAT_CAUTION);
                    if (!s.dark) lmBuildExitLightbox(cx - dx[d] * 0.32f, 2.55f, cz - dz[d] * 0.32f, inwardYaw[d], dv, lv);
                }
            }
        }
    }

    // 2. Site-driven anomalies: LS_WALL_DOOR, LS_CEIL_STAIR, LS_PHASE_SHIFT
    for (int k = 0; k < s.siteCount; k++) {
        const LimenSite& site = s.sites[k];
        int lx = site.lx, lz = site.lz;
        u32 sh = site.id;

        if (site.type == LS_WALL_DOOR) {
            for (int t = 0; t < 4; t++) {
                int d = (int)((sh + (u32)t) & 3u);
                int nwx = lx + dx[d], nwz = lz + dz[d];
                if (lmTagLocal(s, nwx, nwz) != LT_WALL || lmSlabHasSite(s, nwx, nwz, LS_VITRINE_WIN)) continue;
                float cx = (s.rect.x0 + 1 + lx + 0.5f + dx[d] * 0.5f) * CS - dx[d] * 0.02f;
                float cz = (s.rect.z0 + 1 + lz + 0.5f + dz[d] * 0.5f) * CS - dz[d] * 0.02f;
                float cH = lmSlabIsVault(s) ? (s.ceilTiers * WH)
                         : lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);
                int del = lmDeliriumCell255(s.rect.x0 + 1 + lx, s.rect.z0 + 1 + lz);
                if (del >= 95 && cH >= 4.2f) {
                    // High service access door on wall with industrial steel ladder rungs leading up to it!
                    float doorY = 1.65f;
                    lmBuildClosedWallDoor(cx, doorY, cz, inwardYaw[d], false, dv);
                    DecorBuilder lb(dv, cx, cz, inwardYaw[d], 1.0f);
                    lb.box(-0.34f, 0.12f, 0.09f, 0.04f, doorY - 0.10f, 0.08f, DECOR_MAT_DARK_METAL);
                    lb.box( 0.34f, 0.12f, 0.09f, 0.04f, doorY - 0.10f, 0.08f, DECOR_MAT_DARK_METAL);
                    for (float ry = 0.28f; ry < doorY - 0.08f; ry += 0.30f) {
                        lb.box(0.0f, ry, 0.11f, 0.68f, 0.032f, 0.04f, DECOR_MAT_METAL);
                    }
                } else {
                    lmBuildClosedWallDoor(cx, 0.0f, cz, inwardYaw[d], (sh & 4u) != 0u, dv);
                }
                break;
            }
        } else if (site.type == LS_CEIL_STAIR) {
            if (lx + 1 >= s.w) continue;
            int wallSide = 0;
            if (lmTagLocal(s, lx, lz - 1) == LT_WALL && lmTagLocal(s, lx + 1, lz - 1) == LT_WALL) wallSide = -1;
            else if (lmTagLocal(s, lx, lz + 1) == LT_WALL && lmTagLocal(s, lx + 1, lz + 1) == LT_WALL) wallSide = 1;
            if (wallSide == 0) continue;

            float ceilH = lmSlabIsVault(s)
                ? fminf((float)s.ceilTiers * WH, 9.0f)
                : lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);
            float wx0 = (s.rect.x0 + 1 + lx) * CS + 0.45f;
            float wzWall = (wallSide < 0) ? ((s.rect.z0 + 1 + lz) * CS) : ((s.rect.z0 + 2 + lz) * CS);
            float z0 = (wallSide < 0) ? (wzWall + 0.08f) : (wzWall - 1.48f);
            float z1 = z0 + 1.40f;
            bool wallDoorLanding = (ceilH >= 4.25f);
            float targetY = wallDoorLanding ? (ceilH - 2.38f) : (ceilH - 0.16f);
            if (targetY < 1.35f) targetY = 1.35f;
            const int steps = wallDoorLanding ? 11 : 12;
            const float flightRun = wallDoorLanding ? 4.40f : 5.10f;
            const float landingLen = wallDoorLanding ? 1.85f : 1.20f;
            const float totalRun = flightRun + landingLen;
            const float run = flightRun / (float)steps;
            const float rise = targetY / (float)steps;
            for (int st = 0; st < steps; st++) {
                float sx0 = wx0 + (float)st * run;
                float sy1 = fminf((float)(st + 1) * rise, targetY);
                lmPushWallSolidBox(wv, sx0, 0.0f, z0, sx0 + run + 0.02f, sy1, z1);
            }
            // Solid concrete top landing platform
            lmPushWallSolidBox(wv, wx0 + flightRun, 0.0f, z0, wx0 + totalRun, targetY, z1);

            float railZ = (wallSide < 0) ? (z1 - 0.07f) : (z0 + 0.07f);
            DecorBuilder rb(dv, wx0, railZ, 0.0f, 1.0f);
            rb.box(0.10f, 0.0f, 0.0f, 0.06f, 0.92f, 0.06f, DECOR_MAT_DARK_METAL);
            rb.box(flightRun, targetY, 0.0f, 0.06f, fminf(0.92f, ceilH - targetY - 0.08f), 0.06f, DECOR_MAT_DARK_METAL);
            rb.box(totalRun - 0.08f, targetY, 0.0f, 0.06f, fminf(0.92f, ceilH - targetY - 0.08f), 0.06f, DECOR_MAT_DARK_METAL);
            float railTop = targetY + fminf(0.92f, ceilH - targetY - 0.08f);
            decorBeam(rb, Vec3(0.10f, 0.92f, 0.0f), Vec3(flightRun, railTop, 0.0f), 0.05f, 0.05f, DECOR_MAT_DARK_METAL);
            decorBeam(rb, Vec3(flightRun, railTop, 0.0f), Vec3(totalRun - 0.08f, railTop, 0.0f), 0.05f, 0.05f, DECOR_MAT_DARK_METAL);

            if (wallDoorLanding) {
                // Locked high-wall service door on the landing with an eerie glowing transom slit above it!
                float doorCX = wx0 + flightRun + landingLen * 0.5f;
                float doorCZ = (wallSide < 0) ? (wzWall + 0.03f) : (wzWall - 0.03f);
                float dYaw = (wallSide < 0) ? 0.0f : 3.14159265f;
                lmBuildClosedWallDoor(doorCX, targetY, doorCZ, dYaw, true, dv);
                if (!s.dark && targetY + 2.34f < ceilH - 0.06f) {
                    float lzOff = (wallSide < 0) ? 0.09f : -0.09f;
                    lmPushLightBox(lv, doorCX, targetY + 2.28f, wzWall + lzOff, 0.48f, 0.05f, 0.03f);
                }
            } else {
                // Low-ceiling variant: locked steel ceiling maintenance hatch with a cracked glowing slit!
                float hatchCX = wx0 + flightRun + landingLen * 0.5f, hatchCZ = 0.5f * (z0 + z1);
                DecorBuilder cb(dv, hatchCX, hatchCZ, 0.0f, 1.0f);
                cb.box(0.0f, ceilH - 0.14f, 0.0f, landingLen + 0.24f, 0.12f, 1.48f, DECOR_MAT_DARK_METAL);
                cb.box(0.0f, ceilH - 0.16f, 0.0f, landingLen - 0.08f, 0.04f, 1.16f, DECOR_MAT_METAL);
                cb.box(0.0f, ceilH - 0.15f, 0.62f, landingLen, 0.05f, 0.10f, DECOR_MAT_CAUTION);
                if (!s.dark) lmPushLightBox(lv, hatchCX, ceilH - 0.18f, hatchCZ, (landingLen - 0.28f) * 0.5f, 0.025f, 0.04f);
            }
            outCols.push_back({wx0, z0, wx0 + totalRun, z1});
        } else if (site.type == LS_PHASE_SHIFT) {
            for (int t = 0; t < 4; t++) {
                int d = (int)((sh + (u32)t) & 3u);
                int nwx = lx + dx[d], nwz = lz + dz[d];
                if (lmTagLocal(s, nwx, nwz) != LT_WALL || lmSlabHasSite(s, nwx, nwz, LS_VITRINE_WIN)) continue;
                float cH = lmSlabIsVault(s) ? (s.ceilTiers * WH)
                         : lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);
                float cx = (s.rect.x0 + 1 + lx + 0.5f + dx[d] * 0.5f) * CS - dx[d] * 0.02f;
                float cz = (s.rect.z0 + 1 + lz + 0.5f + dz[d] * 0.5f) * CS - dz[d] * 0.02f;
                bool shutterMode = (sh & 8u) != 0u;
                if (shutterMode) {
                    // Industrial security rolling shutter pulled nearly shut, spilling light through an 18 cm floor gap!
                    float shH = fminf(2.26f, cH - 0.36f);
                    DecorBuilder sb(dv, cx, cz, inwardYaw[d], 1.0f);
                    sb.box(0.0f, shH + 0.02f, 0.12f, 2.36f, 0.26f, 0.24f, DECOR_MAT_DARK_METAL);
                    sb.box(-1.14f, 0.0f, 0.07f, 0.12f, shH + 0.10f, 0.14f, DECOR_MAT_DARK_METAL);
                    sb.box( 1.14f, 0.0f, 0.07f, 0.12f, shH + 0.10f, 0.14f, DECOR_MAT_DARK_METAL);
                    for (float sy = 0.22f; sy < shH; sy += 0.16f) {
                        sb.box(0.0f, sy, 0.06f, 2.18f, 0.13f, 0.06f, DECOR_MAT_METAL);
                    }
                    sb.box(0.0f, 0.18f, 0.08f, 2.20f, 0.06f, 0.09f, DECOR_MAT_CAUTION);
                    if (!s.dark) {
                        float lxLit = cx - dx[d] * 0.04f, lzLit = cz - dz[d] * 0.04f;
                        float hx = (dx[d] == 0) ? 0.96f : 0.03f, hz = (dz[d] == 0) ? 0.96f : 0.03f;
                        lmPushLightBox(lv, lxLit, 0.01f, lzLit, hx, 0.15f, hz);
                    }
                } else if (cH >= 3.15f) {
                    // Elevated bulkhead door at +0.66 m with 3 real concrete threshold steps!
                    const float stepH = 0.22f, stepD = 0.34f, stepW = 1.62f;
                    lmBuildClosedWallDoor(cx, 3.0f * stepH, cz, inwardYaw[d], true, dv);
                    for (int st = 0; st < 3; st++) {
                        float distOut = (float)(3 - st) * stepD;
                        float scx = cx - dx[d] * (distOut * 0.5f), scz = cz - dz[d] * (distOut * 0.5f);
                        float sxH = (dx[d] == 0) ? (stepW * 0.5f) : (distOut * 0.5f);
                        float szH = (dz[d] == 0) ? (stepW * 0.5f) : (distOut * 0.5f);
                        lmPushWallSolidBox(wv, scx - sxH, 0.0f, scz - szH, scx + sxH, (float)(st + 1) * stepH, scz + szH);
                    }
                    float totD = 3.0f * stepD;
                    float bcx = cx - dx[d] * (totD * 0.5f), bcz = cz - dz[d] * (totD * 0.5f);
                    float bxH = (dx[d] == 0) ? (stepW * 0.5f) : (totD * 0.5f);
                    float bzH = (dz[d] == 0) ? (stepW * 0.5f) : (totD * 0.5f);
                    outCols.push_back({bcx - bxH, bcz - bzH, bcx + bxH, bcz + bzH});
                } else {
                    lmBuildClosedWallDoor(cx, 0.0f, cz, inwardYaw[d], true, dv);
                }
                break;
            }
        }
    }
}

// Builds sealed, brightly-lit, furnished `LT_VITRINE` rooms and the glazed `LS_VITRINE_WIN`
// viewing windows looking into them! Note: the window cell `(wx, wz)` itself is `LT_WALL`
// (so players/AI cannot enter), and we extend the vitrine room right up to `8 cm` behind
// the corridor window plane so there is zero gap between the glass and the sealed room!
inline void lmBuildSlabVitrines(const LimenSlab& s, std::vector<float>& wv, std::vector<float>& fv,
                                std::vector<float>& cv, std::vector<float>& dv, std::vector<float>& lv) {
    if (s.vitrineRooms <= 0) return;
    static const int dx[4] = {0, 0, -1, 1}, dz[4] = {-1, 1, 0, 0};
    static const float faceYaw[4] = {0.0f, 3.14159265f, 1.57079633f, -1.57079633f};

    auto isVitSpace = [&](int lx, int lz) -> bool {
        if (lx < 0 || lz < 0 || lx >= s.w || lz >= s.h) return false;
        return s.tag[lx][lz] == LT_VITRINE || lmSlabHasSite(s, lx, lz, LS_VITRINE_WIN);
    };

    // 1. Emit floor, ceiling, inner walls, lamps & furniture across all LT_VITRINE cells
    for (int lx = 0; lx < s.w; lx++) {
        for (int lz = 0; lz < s.h; lz++) {
            bool isWinCell = lmSlabHasSite(s, lx, lz, LS_VITRINE_WIN);
            if (s.tag[lx][lz] != LT_VITRINE && !isWinCell) continue;
            int wx = s.rect.x0 + 1 + lx, wz = s.rect.z0 + 1 + lz;
            float px = wx * CS, pz = wz * CS;

            float fl[] = {px,0,pz,0,0,0,1,0, px,0,pz+CS,0,1,0,1,0, px+CS,0,pz+CS,1,1,0,1,0,
                          px,0,pz,0,0,0,1,0, px+CS,0,pz+CS,1,1,0,1,0, px+CS,0,pz,1,0,0,1,0};
            fv.insert(fv.end(), fl, fl + 48);
            float cl[] = {px,WH,pz,0,0,0,-1,0, px,WH,pz+CS,0,1,0,-1,0, px+CS,WH,pz+CS,1,1,0,-1,0,
                          px,WH,pz,0,0,0,-1,0, px+CS,WH,pz+CS,1,1,0,-1,0, px+CS,WH,pz,1,0,0,-1,0};
            cv.insert(cv.end(), cl, cl + 48);

            // Inner enclosing walls of the sealed room (skipping the corridor window face!)
            int winCorrDir = -1;
            if (isWinCell) {
                for (int d = 0; d < 4; d++) {
                    if (lmIsWalkableTag(lmTagLocal(s, lx + dx[d], lz + dz[d]))) { winCorrDir = d; break; }
                }
            }
            if (!isVitSpace(lx - 1, lz) && winCorrDir != 2) lmPushWallSpan(wv, px, 0.0f, pz, 0.0f, CS, WH);
            if (!isVitSpace(lx + 1, lz) && winCorrDir != 3) lmPushWallSpan(wv, px + CS, 0.0f, pz + CS, 0.0f, -CS, WH);
            if (!isVitSpace(lx, lz - 1) && winCorrDir != 0) lmPushWallSpan(wv, px + CS, 0.0f, pz, -CS, 0.0f, WH);
            if (!isVitSpace(lx, lz + 1) && winCorrDir != 1) lmPushWallSpan(wv, px, 0.0f, pz + CS, CS, 0.0f, WH);

            if (s.tag[lx][lz] == LT_VITRINE && lmLampLocal(s, lx, lz)) {
                mkLight(lv, Vec3(px + CS * 0.5f, WH - 0.02f, pz + CS * 0.5f), 1.25f, 1.25f);
            }

            // Furnish the sealed vitrine room so the player sees an eerie unreachable room scene!
            u32 vh = lmHash((u32)wx, (u32)wz, (u32)s.nodeKey[lx][lz], 0x5646u);
            unsigned themes = lmPickCellDecorTheme(s, lx, lz, 5u);
            if (isWinCell && winCorrDir >= 0) {
                // Window frame + mullions + deep sill on the corridor face, plus a focal prop right behind the glass!
                float fcx = px + CS * 0.5f + dx[winCorrDir] * (CS * 0.5f);
                float fcz = pz + CS * 0.5f + dz[winCorrDir] * (CS * 0.5f);
                DecorBuilder wb(dv, fcx, fcz, faceYaw[winCorrDir], 1.0f);
                const float winW = 3.10f, sillY = lmVitrineSillY(wx, wz), topY = 2.38f;
                wb.box(0.0f, sillY - 0.08f, 0.0f, winW + 0.34f, 0.10f, 0.36f, DECOR_MAT_TRIM);
                wb.box(0.0f, topY,          0.0f, winW + 0.34f, 0.10f, 0.32f, DECOR_MAT_DARK_METAL);
                wb.box(-winW * 0.5f - 0.06f, sillY, 0.0f, 0.12f, topY - sillY, 0.28f, DECOR_MAT_DARK_METAL);
                wb.box( winW * 0.5f + 0.06f, sillY, 0.0f, 0.12f, topY - sillY, 0.28f, DECOR_MAT_DARK_METAL);
                wb.box(-winW * 0.18f, sillY, 0.0f, 0.05f, topY - sillY, 0.10f, DECOR_MAT_METAL);
                wb.box( winW * 0.18f, sillY, 0.0f, 0.05f, topY - sillY, 0.10f, DECOR_MAT_METAL);
                // Bright ceiling lamp right inside the window bay too so the foreground props shine!
                mkLight(lv, Vec3(px + CS * 0.5f, WH - 0.02f, pz + CS * 0.5f), 1.15f, 1.15f);

                int ptype = pickThemedPropType(themes, vh, 1u << PROP_PLACE_FLOOR);
                if (ptype >= 0) {
                    MapProp pr{};
                    pr.pos = Vec3(px + CS * 0.5f - dx[winCorrDir] * 0.35f, 0.0f,
                                  pz + CS * 0.5f - dz[winCorrDir] * 0.35f);
                    pr.type = ptype;
                    pr.scale = 1.0f;
                    // Half the time facing the viewer eerily, half the time turned away!
                    pr.yaw = ((vh >> 9) & 1u) ? faceYaw[winCorrDir ^ 1] : faceYaw[winCorrDir];
                    buildMapPropModel(dv, pr);
                }
            } else {
                int ptype = pickThemedPropType(themes, vh >> 3, (1u << PROP_PLACE_FLOOR) | (1u << PROP_PLACE_WALL));
                if (ptype >= 0) {
                    MapProp pr{};
                    pr.pos = Vec3(px + CS * 0.5f, 0.0f, pz + CS * 0.5f);
                    pr.type = ptype;
                    pr.scale = 1.0f;
                    pr.yaw = (float)(vh & 3u) * 1.57079633f;
                    buildMapPropModel(dv, pr);
                }
            }
        }
    }
}

inline void lmBuildHighOrphans(const LimenSlab& s, std::vector<float>& mv, std::vector<float>& dv, std::vector<float>& lv) {
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    if (ceilY < 18.0f) return;

    u32 baseH = lmHash((u32)s.rect.j, (u32)s.rect.i, 31u, 0x5645u);
    int count = 3 + (int)(baseH % 4u);
    for (int k = 0; k < count; k++) {
        u32 h = lmHash(baseH, (u32)k, 7u, 0x4F52u);
        int side = (int)(h % 4u);
        int maxTier = s.ceilTiers - 2;
        if (maxTier < 2) maxTier = 2;
        int tier = 2 + (int)((h >> 3) % (u32)(maxTier - 1));
        float y = tier * WH;
        if (y + 3.2f > ceilY - 2.0f) y = 9.0f;
        float frac = 0.18f + 0.64f * ((float)((h >> 8) % 100u) / 99.0f);
        float wx = x0 + (x1 - x0) * frac;
        float wz = z0 + (z1 - z0) * frac;
        float yaw = 0.0f;
        if (side == 0) { wz = z0 + 0.06f; yaw = 0.0f; }
        else if (side == 1) { wz = z1 - 0.06f; yaw = 3.14159265f; }
        else if (side == 2) { wx = x0 + 0.06f; yaw = 1.57079633f; }
        else { wx = x1 - 0.06f; yaw = -1.57079633f; }

        bool isLitWindow = ((h >> 16) % 100u) < 58u;
        DecorBuilder b(dv, wx, wz, yaw, 1.0f);
        if (isLitWindow && !s.dark) {
            float ww = 3.20f + 1.40f * (float)((h >> 20) % 2u);
            float wh = 1.35f;
            b.box(0.0f, y + 1.10f, 0.05f, ww + 0.28f, wh + 0.28f, 0.18f, DECOR_MAT_DARK_METAL);
            b.box(0.0f, y + 1.18f, 0.02f, ww, wh, 0.08f, DECOR_MAT_PLASTER);
            b.box(-ww * 0.25f, y + 1.18f, 0.11f, 0.08f, wh, 0.12f, DECOR_MAT_DARK_METAL);
            b.box(ww * 0.25f, y + 1.18f, 0.11f, 0.08f, wh, 0.12f, DECOR_MAT_DARK_METAL);
            Vec3 c0 = b.world(-ww * 0.48f, y + 1.22f, 0.13f);
            Vec3 c1 = b.world(ww * 0.48f, y + 1.22f, 0.13f);
            Vec3 c2 = b.world(ww * 0.48f, y + 1.22f + wh - 0.08f, 0.13f);
            Vec3 c3 = b.world(-ww * 0.48f, y + 1.22f + wh - 0.08f, 0.13f);
            lmPushLightQuad(lv, c0, c1, c2, c3, true);
        } else {
            b.box(0.0f, y - 0.18f, 0.12f, 1.70f, 0.18f, 0.36f, DECOR_MAT_PLASTER);
            b.box(-0.62f, y, 0.06f, 0.14f, 2.25f, 0.16f, DECOR_MAT_TRIM);
            b.box(0.62f, y, 0.06f, 0.14f, 2.25f, 0.16f, DECOR_MAT_TRIM);
            b.box(0.0f, y + 2.25f, 0.06f, 1.38f, 0.14f, 0.16f, DECOR_MAT_TRIM);
            b.panel(0.0f, y, 0.05f, 1.10f, 2.24f, 0.08f, DECOR_MAT_DOOR);
        }
    }
    (void)mv;
}

inline void lmBuildStairsToNowhere(const LimenSlab& s, std::vector<float>& mv, std::vector<float>& dv,
                                   std::vector<float>& lv, std::vector<LimenAabb2D>& outCols, float tile) {
    if (s.share[LT_VOID] >= 40) return;
    u32 baseH = lmHash((u32)s.rect.j, (u32)s.rect.i, 43u, 0x5354u);
    int want = 1 + (int)(baseH % 2u);
    int placed = 0;

    for (int attempt = 0; attempt < 28 && placed < want; attempt++) {
        u32 h = lmHash(baseH, (u32)attempt, 11u, 0x5355u);
        int lx = 2 + (int)(h % (u32)(s.w - 4));
        int lz = 2 + (int)((h >> 8) % (u32)(s.h - 4));
        if (s.tag[lx][lz] != LT_HALL || s.tag[lx + 1][lz] != LT_HALL) continue;
        if (lmSlabHasSite(s, lx, lz, LS_CEIL_STAIR)) continue;
        if (lmStoryNear(s, lx, lz, 3)) continue;
        int wallSide = 0;
        if (lmTagLocal(s, lx, lz - 1) == LT_WALL && lmTagLocal(s, lx + 1, lz - 1) == LT_WALL &&
            lmTagLocal(s, lx, lz + 1) == LT_HALL && lmTagLocal(s, lx + 1, lz + 1) == LT_HALL) {
            wallSide = -1;
        } else if (lmTagLocal(s, lx, lz + 1) == LT_WALL && lmTagLocal(s, lx + 1, lz + 1) == LT_WALL &&
                   lmTagLocal(s, lx, lz - 1) == LT_HALL && lmTagLocal(s, lx + 1, lz - 1) == LT_HALL) {
            wallSide = 1;
        }
        if (wallSide == 0) continue;

        float wx0 = (s.rect.x0 + 1 + lx) * CS + 0.5f;
        float wzWall = (wallSide < 0) ? ((s.rect.z0 + 1 + lz) * CS) : ((s.rect.z0 + 2 + lz) * CS);
        float z0 = (wallSide < 0) ? (wzWall + 0.08f) : (wzWall - 1.58f);
        float z1 = z0 + 1.50f;
        const int steps = 12;
        const float run = 0.42f, rise = 0.29f;
        for (int k = 0; k < steps; k++) {
            float sx0 = wx0 + k * run;
            lmPushBox(mv, sx0, 0.0f, z0, sx0 + run + 0.02f, (k + 1) * rise, z1, tile);
        }
        float landX0 = wx0 + steps * run, landX1 = landX0 + 1.90f;
        float topY = steps * rise;
        lmPushBox(mv, landX0, 0.0f, z0, landX1, topY, z1, tile);

        float railZ = (wallSide < 0) ? (z1 - 0.08f) : (z0 + 0.08f);
        DecorBuilder rb(dv, wx0, railZ, 0.0f, 1.0f);
        rb.box(0.10f, 0.0f, 0.0f, 0.06f, 0.95f, 0.06f, DECOR_MAT_DARK_METAL);
        rb.box(steps * run + 0.10f, topY, 0.0f, 0.06f, 0.95f, 0.06f, DECOR_MAT_DARK_METAL);
        rb.box(steps * run + 1.80f, topY, 0.0f, 0.06f, 0.95f, 0.06f, DECOR_MAT_DARK_METAL);
        decorBeam(rb, Vec3(0.10f, 0.95f, 0.0f), Vec3(steps * run + 0.10f, topY + 0.95f, 0.0f), 0.06f, 0.06f, DECOR_MAT_DARK_METAL);
        decorBeam(rb, Vec3(steps * run + 0.10f, topY + 0.95f, 0.0f), Vec3(steps * run + 1.80f, topY + 0.95f, 0.0f), 0.06f, 0.06f, DECOR_MAT_DARK_METAL);

        float faceYaw = (wallSide < 0) ? 0.0f : 3.14159265f;
        float signZ = wzWall + (wallSide < 0 ? 0.06f : -0.06f);
        if ((h >> 18) & 1u) {
            DecorBuilder db(dv, landX0 + 0.95f, signZ, faceYaw, 1.0f);
            db.box(-0.62f, topY, 0.0f, 0.14f, 2.20f, 0.14f, DECOR_MAT_TRIM);
            db.box(0.62f, topY, 0.0f, 0.14f, 2.20f, 0.14f, DECOR_MAT_TRIM);
            db.box(0.0f, topY + 2.20f, 0.0f, 1.38f, 0.14f, 0.14f, DECOR_MAT_TRIM);
            if (!s.dark) lmBuildExitLightbox(landX0 + 0.95f, topY + 2.46f, signZ, faceYaw, dv, lv);
        }

        outCols.push_back({wx0, z0, landX1, z1});
        placed++;
    }
}
