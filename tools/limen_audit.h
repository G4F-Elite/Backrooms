#pragma once
// Geometry audit for the frame grabber build (`limen_shots.exe audit <seed> <slabs> <level> [kinds]`).
// Stands in every place the player can stand, looks around with rays against the very triangles
// the game draws (honouring back-face culling per buffer) and reports what must never be seen:
//   BACK  the nearest surface along the ray is a culled back face: the eye looks into the inside
//         of a wall, between the two skins of a partition, or past a wall that sticks out
//   VOID  nothing is hit at all: a hole to the outside of the world
// Findings are grouped by what was looked at, with an example camera for `limen_shots.exe at`.
#include <map>
#include <string>

struct LaHit { float tVis, tBack; int backTri; };
struct LaSurf { float t; int k; bool vis; };
static bool gLaDebug = false;

struct LaGrid {
    int ox = 0, oz = 0, n = 0;                 // dense grid of n x n cells, origin cell (ox, oz)
    std::vector<std::vector<int>> cell;
    std::vector<int> big;
    std::vector<LmAuditTri> tris;

    void build(int centerX, int centerZ, int halfCells) {
        n = halfCells * 2 + 1; ox = centerX - halfCells; oz = centerZ - halfCells;
        cell.assign((size_t)n * (size_t)n, std::vector<int>());
        big.clear();
        for (int k = 0; k < (int)tris.size(); k++) {
            const float* p = tris[k].p;
            float x0 = fminf(p[0], fminf(p[3], p[6])), x1 = fmaxf(p[0], fmaxf(p[3], p[6]));
            float z0 = fminf(p[2], fminf(p[5], p[8])), z1 = fmaxf(p[2], fmaxf(p[5], p[8]));
            int cx0 = (int)floorf((x0 - 0.01f) / CS) - ox, cx1 = (int)floorf((x1 + 0.01f) / CS) - ox;
            int cz0 = (int)floorf((z0 - 0.01f) / CS) - oz, cz1 = (int)floorf((z1 + 0.01f) / CS) - oz;
            if (cx1 < 0 || cz1 < 0 || cx0 >= n || cz0 >= n) continue;
            if ((cx1 - cx0 + 1) * (cz1 - cz0 + 1) > 48) { big.push_back(k); continue; }
            if (cx0 < 0) cx0 = 0;
            if (cz0 < 0) cz0 = 0;
            if (cx1 >= n) cx1 = n - 1;
            if (cz1 >= n) cz1 = n - 1;
            for (int x = cx0; x <= cx1; x++) for (int z = cz0; z <= cz1; z++) cell[(size_t)x * n + z].push_back(k);
        }
    }

    inline void test(int k, const float* o, const float* d, LaHit& h) const {
        const LmAuditTri& t = tris[k];
        const float* p = t.p;
        float e1x = p[3]-p[0], e1y = p[4]-p[1], e1z = p[5]-p[2];
        float e2x = p[6]-p[0], e2y = p[7]-p[1], e2z = p[8]-p[2];
        float px = d[1]*e2z - d[2]*e2y, py = d[2]*e2x - d[0]*e2z, pz = d[0]*e2y - d[1]*e2x;
        float det = e1x*px + e1y*py + e1z*pz;
        if (det > -1e-7f && det < 1e-7f) return;
        float inv = 1.0f / det;
        float tx = o[0]-p[0], ty = o[1]-p[1], tz = o[2]-p[2];
        float u = (tx*px + ty*py + tz*pz) * inv;
        if (u < -1e-4f || u > 1.0001f) return;
        float qx = ty*e1z - tz*e1y, qy = tz*e1x - tx*e1z, qz = tx*e1y - ty*e1x;
        float v = (d[0]*qx + d[1]*qy + d[2]*qz) * inv;
        if (v < -1e-4f || u + v > 1.0001f) return;
        float tt = (e2x*qx + e2y*qy + e2z*qz) * inv;
        if (tt < 0.02f) return;
        if (t.twoSided || det > 0.0f) { if (tt < h.tVis) h.tVis = tt; }
        else if (tt < h.tBack) { h.tBack = tt; h.backTri = k; }
    }

    // Every surface the ray crosses between t0 and t1, nearest first.
    void collect(const float* o, const float* d, float t0, float t1, std::vector<LaSurf>& out) const {
        out.clear();
        auto one = [&](int k) {
            const float* p = tris[k].p;
            float e1x = p[3]-p[0], e1y = p[4]-p[1], e1z = p[5]-p[2], e2x = p[6]-p[0], e2y = p[7]-p[1], e2z = p[8]-p[2];
            float px = d[1]*e2z - d[2]*e2y, py = d[2]*e2x - d[0]*e2z, pz = d[0]*e2y - d[1]*e2x;
            float det = e1x*px + e1y*py + e1z*pz;
            if (det > -1e-7f && det < 1e-7f) return;
            float inv = 1.0f / det, tx = o[0]-p[0], ty = o[1]-p[1], tz = o[2]-p[2];
            float u = (tx*px + ty*py + tz*pz) * inv;
            if (u < -1e-4f || u > 1.0001f) return;
            float qx = ty*e1z - tz*e1y, qy = tz*e1x - tx*e1z, qz = tx*e1y - ty*e1x;
            float v = (d[0]*qx + d[1]*qy + d[2]*qz) * inv;
            if (v < -1e-4f || u + v > 1.0001f) return;
            float tt = (e2x*qx + e2y*qy + e2z*qz) * inv;
            if (tt < t0 || tt > t1) return;
            out.push_back({tt, k, tris[k].twoSided || det > 0.0f});
        };
        for (int k : big) one(k);
        float gx = o[0] / CS - (float)ox, gz = o[2] / CS - (float)oz;
        int cx = (int)floorf(gx), cz = (int)floorf(gz);
        int sx = d[0] > 0 ? 1 : -1, sz = d[2] > 0 ? 1 : -1;
        float adx = fabsf(d[0]), adz = fabsf(d[2]);
        float tDX = adx > 1e-6f ? CS / adx : 1e30f, tDZ = adz > 1e-6f ? CS / adz : 1e30f;
        float tMX = adx > 1e-6f ? ((d[0] > 0 ? (float)(cx + 1) - gx : gx - (float)cx) * CS / adx) : 1e30f;
        float tMZ = adz > 1e-6f ? ((d[2] > 0 ? (float)(cz + 1) - gz : gz - (float)cz) * CS / adz) : 1e30f;
        float tEnter = 0.0f;
        while (cx >= 0 && cz >= 0 && cx < n && cz < n && tEnter <= t1 + 0.5f) {
            for (int k : cell[(size_t)cx * n + cz]) one(k);
            if (tMX < tMZ) { tEnter = tMX; tMX += tDX; cx += sx; }
            else { tEnter = tMZ; tMZ += tDZ; cz += sz; }
        }
        std::sort(out.begin(), out.end(), [](const LaSurf& a, const LaSurf& b) { return a.k < b.k; });
        out.erase(std::unique(out.begin(), out.end(), [](const LaSurf& a, const LaSurf& b) { return a.k == b.k; }), out.end());
        std::sort(out.begin(), out.end(), [](const LaSurf& a, const LaSurf& b) { return a.t < b.t || (a.t == b.t && a.k < b.k); });
    }

    void normal(int k, float* nrm) const {
        const float* p = tris[k].p;
        nrm[0] = (p[4]-p[1])*(p[8]-p[2]) - (p[5]-p[2])*(p[7]-p[1]);
        nrm[1] = (p[5]-p[2])*(p[6]-p[0]) - (p[3]-p[0])*(p[8]-p[2]);
        nrm[2] = (p[3]-p[0])*(p[7]-p[1]) - (p[4]-p[1])*(p[6]-p[0]);
        float l = sqrtf(nrm[0]*nrm[0] + nrm[1]*nrm[1] + nrm[2]*nrm[2]) + 1e-12f;
        nrm[0] /= l; nrm[1] /= l; nrm[2] /= l;
        if (tris[k].flip) { nrm[0] = -nrm[0]; nrm[1] = -nrm[1]; nrm[2] = -nrm[2]; }   // lit from the other side
    }

    // What is wrong with the surface the ray sees first (it is a sound, visible one):
    //   3 ZFIGHT  a second drawn face lies in the same plane at this very spot: the two flicker
    //   4 PAPER   the surface has no thickness: under 3 cm behind it the next room begins
    int surface(const float* o, const float* d, float tVis, float maxT, int& triA, int& triB) const {
        static thread_local std::vector<LaSurf> hs;
        collect(o, d, tVis - 0.02f, tVis + 0.05f, hs);
        int a = -1;
        for (int q = 0; q < (int)hs.size(); q++) if (hs[q].vis) { a = q; break; }
        if (a < 0) return 0;
        triA = hs[a].k; triB = -1;
        const LmAuditTri& A = tris[triA];
        float nA[3]; normal(triA, nA);
        float P[3] = {o[0] + d[0] * hs[a].t, o[1] + d[1] * hs[a].t, o[2] + d[2] * hs[a].t};
        for (int q = 0; q < (int)hs.size(); q++) {
            if (q == a || !hs[q].vis) continue;
            const LmAuditTri& B = tris[hs[q].k];
            float nB[3]; normal(hs[q].k, nB);
            float dotN = nA[0]*nB[0] + nA[1]*nB[1] + nA[2]*nB[2];
            if (dotN < 0.9995f) continue;   // back to back is hidden by the solids on both sides
            float dist = fabsf(nB[0]*(P[0]-B.p[0]) + nB[1]*(P[1]-B.p[1]) + nB[2]*(P[2]-B.p[2]));
            if (dist > 0.0015f) continue;
            if (A.buf == B.buf) {   // the two halves of one quad meet along the diagonal
                int shared = 0;
                for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++)
                    if (A.p[i*3] == B.p[j*3] && A.p[i*3+1] == B.p[j*3+1] && A.p[i*3+2] == B.p[j*3+2]) shared++;
                if (shared >= 2) continue;
            }
            triB = hs[q].k;
            return 3;
        }
        // thickness: the first surface behind A that is not in A's own plane
        if (A.buf == LA_DECOR || A.buf == LA_SLAB_DECOR || A.buf == LA_LIGHT || A.buf == LA_PROP || A.buf == LA_UNSTRUCT_ACC) return 0;
        // a ceiling or sky enclosure is a sheet by design: nobody gets above or outside it
        if (A.buf == LA_CEIL || A.buf == LA_X_CEIL || A.buf == LA_VIT_CEIL || A.buf == LA_UNSTRUCT_SKY) return 0;
        float tB = hs[a].t;
        if (!A.twoSided) {
            int b = -1;
            for (int q = a + 1; q < (int)hs.size(); q++) {
                if (hs[q].vis && hs[q].t - hs[a].t < 1e-4f) continue;
                b = q; break;
            }
            if (b < 0 || hs[b].t - hs[a].t > 0.03f) return 0;
            if (hs[b].vis && !tris[hs[b].k].twoSided) return 0;   // a panel on a wall
            int bb = tris[hs[b].k].buf;
            if (bb == LA_DECOR || bb == LA_SLAB_DECOR || bb == LA_LIGHT || bb == LA_PROP) return 0;
            float nB[3]; normal(hs[b].k, nB);
            if (nA[0]*nB[0] + nA[1]*nB[1] + nA[2]*nB[2] > -0.9f) return 0;   // the ray clips a corner
            triB = hs[b].k; tB = hs[b].t;
        }
        float o2[3] = {o[0] + d[0] * (tB + 0.004f), o[1] + d[1] * (tB + 0.004f), o[2] + d[2] * (tB + 0.004f)};
        LaHit h2 = cast(o2, d, maxT);
        if (h2.tVis > 1e29f || h2.tVis > h2.tBack || h2.tVis < 0.30f) return 0;
        // A sheet only reads as paper where it ends in free air: look for such an edge by sweeping
        // the ray off the sheet; past the edge it must reach the very space that lies behind it.
        float P1[3] = {o[0] + d[0] * (tB + 0.10f), o[1] + d[1] * (tB + 0.10f), o[2] + d[2] * (tB + 0.10f)};
        float pd = nA[0]*A.p[0] + nA[1]*A.p[1] + nA[2]*A.p[2];
        float up[3] = {0.0f, 1.0f, 0.0f};
        if (fabsf(d[1]) > 0.95f) { up[0] = 1.0f; up[1] = 0.0f; }
        float rx[3] = {d[1]*up[2] - d[2]*up[1], d[2]*up[0] - d[0]*up[2], d[0]*up[1] - d[1]*up[0]};
        float rl = sqrtf(rx[0]*rx[0] + rx[1]*rx[1] + rx[2]*rx[2]); rx[0] /= rl; rx[1] /= rl; rx[2] /= rl;
        float ux[3] = {rx[1]*d[2] - rx[2]*d[1], rx[2]*d[0] - rx[0]*d[2], rx[0]*d[1] - rx[1]*d[0]};
        static const float steps[7] = {0.003f, 0.008f, 0.02f, 0.05f, 0.10f, 0.18f, 0.30f};
        for (int dir = 0; dir < 4; dir++) {
            for (int st = 0; st < 7; st++) {
                float a = steps[st] * ((dir & 1) ? -1.0f : 1.0f);
                const float* ax = (dir < 2) ? rx : ux;
                float d2[3] = {d[0] + ax[0] * a, d[1] + ax[1] * a, d[2] + ax[2] * a};
                float l2 = sqrtf(d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2]); d2[0] /= l2; d2[1] /= l2; d2[2] /= l2;
                float den = nA[0]*d2[0] + nA[1]*d2[1] + nA[2]*d2[2];
                if (fabsf(den) < 1e-4f) break;
                float tp = (pd - (nA[0]*o[0] + nA[1]*o[1] + nA[2]*o[2])) / den;
                if (tp < 0.05f) break;
                LaHit hh = cast(o, d2, tp + 1.0f);
                float first = hh.tVis < hh.tBack ? hh.tVis : hh.tBack;
                if (first < tp - 0.004f) break;            // something stands in front
                if (first < tp + 0.035f) continue;         // still on the sheet (or on what it joins)
                float t2 = tp + fminf(0.10f, (first - tp) * 0.5f);
                float P2[3] = {o[0] + d2[0] * t2, o[1] + d2[1] * t2, o[2] + d2[2] * t2};
                float sd[3] = {P2[0] - P1[0], P2[1] - P1[1], P2[2] - P1[2]};
                float sl = sqrtf(sd[0]*sd[0] + sd[1]*sd[1] + sd[2]*sd[2]);
                if (sl < 1e-4f) break;
                sd[0] /= sl; sd[1] /= sl; sd[2] /= sl;
                LaHit hs2 = cast(P1, sd, sl + 0.1f);
                float blk = hs2.tVis < hs2.tBack ? hs2.tVis : hs2.tBack;
                if (gLaDebug) printf("    sweep dir %d step %.3f: plane at %.2f, first %.2f, P1 (%.2f,%.2f,%.2f) P2 (%.2f,%.2f,%.2f) blocked at %.2f of %.2f\n",
                                     dir, a, tp, first, P1[0], P1[1], P1[2], P2[0], P2[1], P2[2], blk, sl);
                if (blk > sl) return 4;
                break;
            }
        }
        return 0;
    }

    LaHit cast(const float* o, const float* d, float maxT) const {
        LaHit h = {1e30f, 1e30f, -1};
        for (int k : big) test(k, o, d, h);
        float gx = o[0] / CS - (float)ox, gz = o[2] / CS - (float)oz;
        int cx = (int)floorf(gx), cz = (int)floorf(gz);
        int sx = d[0] > 0 ? 1 : -1, sz = d[2] > 0 ? 1 : -1;
        float adx = fabsf(d[0]), adz = fabsf(d[2]);
        float tDX = adx > 1e-6f ? CS / adx : 1e30f, tDZ = adz > 1e-6f ? CS / adz : 1e30f;
        float tMX = adx > 1e-6f ? ((d[0] > 0 ? (float)(cx + 1) - gx : gx - (float)cx) * CS / adx) : 1e30f;
        float tMZ = adz > 1e-6f ? ((d[2] > 0 ? (float)(cz + 1) - gz : gz - (float)cz) * CS / adz) : 1e30f;
        float tEnter = 0.0f;
        while (cx >= 0 && cz >= 0 && cx < n && cz < n) {
            float nearest = h.tVis < h.tBack ? h.tVis : h.tBack;
            if (tEnter > nearest + 0.5f || tEnter > maxT) break;
            const std::vector<int>& c = cell[(size_t)cx * n + cz];
            for (int k : c) test(k, o, d, h);
            if (tMX < tMZ) { tEnter = tMX; tMX += tDX; cx += sx; }
            else { tEnter = tMZ; tMZ += tDZ; cz += sz; }
        }
        return h;
    }
};

static const char* laBufName(int b) {
    static const char* n[LA_BUF_COUNT] = {"wall", "floor", "ceil", "pillar", "decor", "mega", "megaAlt", "slabWall", "dado",
                                          "slabDecor", "vitFloor", "vitCeil", "xWall", "xDado", "xFloor", "xCeil", "light", "prop",
                                          "unstructAcc", "unstructSky"};
    return (b >= 0 && b < LA_BUF_COUNT) ? n[b] : "?";
}

static std::string laCellDesc(int wx, int wz) {
    static const char* tn[LT_TAG_COUNT] = {"WALL", "CORR", "ROOM", "HALL", "VOID", "BRIDGE", "BASIN", "LOW", "VITRINE"};
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return lmWallCellOpen(r, wx, wz) ? "seamPORT" : "seamWALL";
    const LimenSlab& s = lmSlab(r.j, r.i);
    int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
    std::string d = tn[s.tag[lx][lz]];
    unsigned short f = s.flag[lx][lz];
    if (f & LF_LOW_WALL) d += "+lowwall";
    if (f & (LF_THIN_W | LF_THIN_N)) d += "+thin";
    if (f & (LF_THIN_DOOR_W | LF_THIN_DOOR_N)) d += "+tdoor";
    if (f & (LF_NARROW_X | LF_NARROW_Z)) d += "+narrow";
    if (f & LF_CRAMPED) d += "+cramped";
    if (f & LF_MEZZ) d += "+mezz";
    if (f & LF_CHAMFER) d += "+chamfer";
    if (f & LF_STAIR_DESC) d += "+stair";
    if (s.zone[lx][lz] & LZ_XENO) d += "+xeno";
    if (s.zone[lx][lz] & LZ_PLUG_CORR) d += "+plug";
    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].lx == lx && s.sites[k].lz == lz) { char b[16]; snprintf(b, sizeof(b), "+site%d", s.sites[k].type); d += b; }
    }
    if (lmDeliriumCell255(wx, wz) >= 50) d += "+drunk";
    return d;
}

struct LaFinding { long long count = 0; char example[3][300]; int nEx = 0; };

// ---- story world -------------------------------------------------------------------------
// With LM_STORY=<cells the rig has crawled> every tool mode runs in the world as the game starts
// it: garage + portal stamped on floor 0, the rig put down and its lane bored, kiosk, pit.
static int gLaStory = -1;
static std::vector<std::pair<int, int>> gLaStoryCells;   // cells the story stands on / runs through

static void lmStoryBegin(u32 seed, int level, int rigSteps) {
    gCurrentLevel = level; worldSeed = seed; lmReset(seed, level);
    chunks.clear(); lights.clear(); pillars.clear(); resetMapContent(); forcedDoorways.clear(); gCellOverrides.clear();
    g_lightStates.clear();
    rng.seed(seed); srand(seed);
    updateVisibleChunks(0, 0); updateLightsAndPillars(0, 0); updateMapContent(0, 0);
    Vec3 sp = findSafeSpawn();
    Vec3 coopBase = sp;
    cam.pos = Vec3(sp.x, PH, sp.z); cam.yaw = cam.pitch = 0;
    if (collideWorld(cam.pos.x, cam.pos.z, 0.42f)) { Vec3 safe = findSpawnPos(cam.pos, 5.0f); cam.pos = Vec3(safe.x, PH, safe.z); }
    updateVisibleChunks(cam.pos.x, cam.pos.z); updateLightsAndPillars(playerChunkX, playerChunkZ); updateMapContent(playerChunkX, playerChunkZ);
    entityMgr.reset(); storyMgr.init(); storyMgr.introComplete = true;
    trapCorridor = {};
    if (!isLevelZero(gCurrentLevel)) initTrapCorridor(sp);
    initCoopObjectives(coopBase); resetVoidShiftState(); resetCoopRunState(); resetPoiRuntime();
    worldItems.clear(); nextWorldItemId = 1;
    heistBeginLevel(coopBase);
    coop.doorPos = heist.drillPos;
    placeAbyss(coopBase);
    floorHoles.clear();
    if (rigSteps > 0) {
        if (heist.stage != HEIST_RUN) heistEnterRun();
        heist.drillPos = Vec3(heist.rigStart.x + (float)rigDirX(heist.rigDir) * rigSteps * CS, 0.0f,
                              heist.rigStart.z + (float)rigDirZ(heist.rigDir) * rigSteps * CS);
        rigCarveLane();
    }
    gLaStoryCells.clear();
    auto add = [&](int wx, int wz) { gLaStoryCells.push_back({wx, wz}); };
    forEachObjectivePos([&](const Vec3& p) { add((int)floorf(p.x / CS), (int)floorf(p.z / CS)); });
    add(heist.baseX, heist.baseZ);
    if (heist.hasGarage) {
        for (int dx = -GARAGE_HALF_W - 1; dx <= GARAGE_HALF_W + 1; dx++)
            for (int dz = GARAGE_FAR_Z - 1; dz <= GARAGE_NEAR_Z + 1; dz++) add(heist.baseX + dx, heist.baseZ + dz);
        for (int dz = PORTAL_DZ; dz <= 0; dz++) add(heist.baseX, heist.baseZ + dz);
        for (int dx = -3; dx <= -1; dx++) for (int dz = -2; dz <= 0; dz++) add(heist.baseX + dx, heist.baseZ + dz);
    }
    for (int step = RIG_LANE_BACK_STEP; step <= rigFrontStep(rigTravelled(), CS) + RIG_LANE_KEEP_CLEAR; step++) {
        int wx = 0, wz = 0; rigLaneCell(step, wx, wz); add(wx, wz);
    }
    for (auto& kv : gCellOverrides) add((int)(kv.first >> 32), (int)(kv.first & 0xFFFFFFFF));
    std::sort(gLaStoryCells.begin(), gLaStoryCells.end());
    gLaStoryCells.erase(std::unique(gLaStoryCells.begin(), gLaStoryCells.end()), gLaStoryCells.end());
}
static bool laNearStory(int wx, int wz, int reach) {
    for (auto& c : gLaStoryCells) if (abs(c.first - wx) <= reach && abs(c.second - wz) <= reach) return true;
    return false;
}

// What LIMEN's own geometry does to the cells the story needs: `limen_shots.exe story <seed> <level> <rigSteps>`.
// Returns the number of story cells in conflict.
static int lmStoryReport(bool verbose) {
    std::map<std::string, int> kinds;
    int bad = 0;
    auto note = [&](int wx, int wz, const char* what) {
        kinds[what]++;
        if (verbose) printf("   cell (%d,%d) %s: %s\n", wx, wz, laCellDesc(wx, wz).c_str(), what);
    };
    for (auto& c : gLaStoryCells) {
        int wx = c.first, wz = c.second;
        {   // only what is loaded can be judged (an unloaded cell reads as solid)
            int ccx = wx >= 0 ? wx / CHUNK_SIZE : (wx - CHUNK_SIZE + 1) / CHUNK_SIZE, ccz = wz >= 0 ? wz / CHUNK_SIZE : (wz - CHUNK_SIZE + 1) / CHUNK_SIZE;
            if (chunks.find(chunkKey(ccx, ccz)) == chunks.end()) continue;
        }
        bool open = getCellWorld(wx, wz) == 0;
        int before = 0; for (auto& kv : kinds) before += kv.second;
        LimenRect r = lmSlabAt(wx, wz);
        bool seam = (wx == r.x0 || wz == r.z0);
        int tag = lmTagAt(wx, wz);
        unsigned short f = lmFlagAt(wx, wz);
        bool vault = !seam && lmSlabIsVault(lmSlab(r.j, r.i));
        if (open) {
            if (seam && !lmWallCellOpen(r, wx, wz) && vault) note(wx, wz, "opened seam wall of a vault");
            if (!seam) {
                if (vault && tag == LT_WALL) note(wx, wz, "opened inside vault mass (slab mesh still draws the wall)");
                if (tag == LT_VITRINE) note(wx, wz, "opened inside a sealed vitrine");
                if (lmIsPitTag(tag)) note(wx, wz, "opened over a pit / basin");
                if (f & (LF_NARROW_X | LF_NARROW_Z)) { if ((lmZoneAt(wx, wz) & LZ_PLUG_CORR) == 0) note(wx, wz, "narrow-corridor wings stand in the cell"); }
                if (f & LF_CRAMPED) note(wx, wz, "2.65 m ceiling");
            }
            if (lmStoryLaneCell(wx, wz)) {
                if (gLmStory.laneDX != 0 && ((f & LF_THIN_W) || (lmFlagAt(wx + 1, wz) & LF_THIN_W) && lmStoryLaneCell(wx + 1, wz))) note(wx, wz, "thin partition across the lane");
                if (gLmStory.laneDZ != 0 && ((f & LF_THIN_N) || (lmFlagAt(wx, wz + 1) & LF_THIN_N) && lmStoryLaneCell(wx, wz + 1))) note(wx, wz, "thin partition across the lane");
            }
            if (fabsf(lmGroundHeightAt((wx + 0.5f) * CS, (wz + 0.5f) * CS, CS)) > 0.02f) note(wx, wz, "floor is not at 0");
            for (int q = 0; q < 4; q++) if (lmDrunkVertexFloorY(wx + (q & 1), wz + (q >> 1)) != 0.0f || lmDrunkVertexCeilOffset(wx + (q & 1), wz + (q >> 1)) != 0.0f) { note(wx, wz, "warped floor / ceiling"); break; }
            for (const auto& b : gLmVestigeColliders)
                if (b.x1 > wx * CS + 0.3f && b.x0 < (wx + 1) * CS - 0.3f && b.z1 > wz * CS + 0.3f && b.z0 < (wz + 1) * CS - 0.3f) {
                    if (verbose) printf("      box x %.2f..%.2f z %.2f..%.2f\n", b.x0, b.x1, b.z0, b.z1);
                    note(wx, wz, "a solid LIMEN structure stands in the cell"); break;
                }
            for (const auto& p : pillars)
                if ((int)floorf(p.x / CS) == wx && (int)floorf(p.z / CS) == wz) { note(wx, wz, "a pillar stands in the cell"); break; }
            for (const auto& pr : mapProps) {
                if (isPropOverheadType(pr.type) || propPlacement(pr.type) == PROP_PLACE_FLAT) continue;
                if ((lmZoneAt(wx, wz) & LZ_STORY) && (int)floorf(pr.pos.x / CS) == wx && (int)floorf(pr.pos.z / CS) == wz) { note(wx, wz, "furniture in the cell"); break; }
            }
        } else {
            if (!seam && vault && tag != LT_WALL && !lmIsLowShellTag(tag)) note(wx, wz, "closed by the game on open vault floor (no wall is drawn)");
            if (!seam && lmIsWalkableTag(tag) && (f & (LF_THIN_W | LF_THIN_N))) note(wx, wz, "closed cell keeps its thin partition");
        }
        int after = 0; for (auto& kv : kinds) after += kv.second;
        if (after > before) bad++;
    }
    int nVault = 0, nBridge = 0, nTunnel = 0, nSeam = 0;
    for (auto& c : gLaStoryCells) {
        LimenRect r = lmSlabAt(c.first, c.second);
        if (c.first == r.x0 || c.second == r.z0) { if (lmStoryEdit(c.first, c.second) == 0 && !lmWallCellOpen(r, c.first, c.second)) nSeam++; continue; }
        const LimenSlab& s = lmSlab(r.j, r.i);
        if (!lmSlabIsVault(s)) continue;
        nVault++;
        int t = s.tag[c.first - r.x0 - 1][c.second - r.z0 - 1];
        if (t == LT_BRIDGE) nBridge++;
        if (t == LT_LOW && lmStoryEdit(c.first, c.second) == 0) nTunnel++;
    }
    printf("STORY L%d seed %u: base (%d,%d) rig dir %d from (%d,%d), %zu story cells (%d in vaults: %d on decks, %d tunnelled; %d seam walls bored), %d in conflict\n", gCurrentLevel, worldSeed,
           heist.baseX, heist.baseZ, heist.rigDir, (int)floorf(heist.rigStart.x / CS), (int)floorf(heist.rigStart.z / CS), gLaStoryCells.size(),
           nVault, nBridge, nTunnel, nSeam, bad);
    for (auto& kv : kinds) printf("   %4d  %s\n", kv.second, kv.first.c_str());
    return bad;
}

static int lmRunGeomAudit(u32 seed, int level, int wantSlabs, int verbose) {
    auto storySlab = [&](int j, int i) {
        for (auto& c : gLaStoryCells) { LimenRect q = lmSlabAt(c.first, c.second); if (q.j == j && q.i == i) return true; }
        return false;
    };
    gLmZfHistOn = getenv("LM_ZF_HIST") != nullptr;
    std::map<std::string, LaFinding> found;
    long long rays = 0, bad = 0, eyes = 0, overlay = 0, zf = 0, paper = 0;
    int done = 0;
    LaGrid grid;
    const int NDIR = 260;   // a Fibonacci sphere of directions
    static float dirs[NDIR][3];
    for (int k = 0; k < NDIR; k++) {
        float y = 1.0f - 2.0f * ((float)k + 0.5f) / (float)NDIR;
        float rr = sqrtf(1.0f - y * y), a = (float)k * 2.39996323f;
        dirs[k][0] = cosf(a) * rr; dirs[k][1] = y; dirs[k][2] = sinf(a) * rr;
    }
    for (int ring = 0; ring <= 12 && done < wantSlabs; ring++) {
        for (int j = -ring; j <= ring && done < wantSlabs; j++) {
            for (int i = -ring; i <= ring && done < wantSlabs; i++) {
                if (lmDepth(j, i) != ring) continue;
                if (gLaStory >= 0 && !storySlab(j, i)) continue;
                const LimenSlab s = lmSlab(j, i);
                const LimenRect& r = s.rect;
                int ccx = r.x0 + 1 + s.w / 2, ccz = r.z0 + 1 + s.h / 2;
                float sx0 = (ccx + 0.5f) * CS, sz0 = (ccz + 0.5f) * CS;
                cam.pos = Vec3(sx0, PH, sz0);
                chunks.clear();
                rng.seed(seed); srand(seed);
                updateVisibleChunks(sx0, sz0);
                updateLightsAndPillars(playerChunkX, playerChunkZ);
                resetMapContent();
                updateMapContent(playerChunkX, playerChunkZ);
                buildGeom();
                grid.tris = gLmAuditWorld;
                {
                    std::vector<float> wl;   // the shared lamp panels of the 1-storey world
                    for (auto& l : lights) {
                        int lwx = (int)floorf(l.pos.x / CS), lwz = (int)floorf(l.pos.z / CS);
                        int ltag = lmTagAt(lwx, lwz);
                        if (lmHighAt(lwx, lwz) || ltag == LT_VITRINE || lmIsPitTag(ltag)) continue;   // as the mesher does
                        Vec3 lp = l.pos;   // as the mesher hangs it
                        if (lmCellCeilWarped(lwx, lwz)) lp.y += fminf(0.0f, lmCeilWarpUnderPanel(lp.x, lp.z, l.sizeX, l.sizeZ));
                        mkLight(wl, lp, l.sizeX, l.sizeZ);
                    }
                    lmAuditAppendLights(grid.tris, wl);
                }
                for (long long skey : gLmVisibleSlabKeys) {
                    auto it = gLmAuditSlab.find(skey);
                    if (it != gLmAuditSlab.end()) grid.tris.insert(grid.tris.end(), it->second.begin(), it->second.end());
                }
                {
                    std::vector<float> pm;
                    for (const auto& pr : mapProps) lmAppendPropModel(pm, pr);
                    for (const auto& poi : mapPois) lmAppendPoiModel(pm, poi);
                    { ZfMesh zm[1] = {{&pm, 8, 0, "props"}}; lmResolveZfight(zm, 1); }
                    lmAuditAppend(grid.tris, pm, LA_PROP, true);
                }
                grid.build(ccx, ccz, 150);
                // loaded-chunk box (cells), shrunk by one cell
                int bx0 = (playerChunkX - LM_VIEW_CHUNKS) * CHUNK_SIZE + 1, bx1 = (playerChunkX + LM_VIEW_CHUNKS + 1) * CHUNK_SIZE - 1;
                int bz0 = (playerChunkZ - LM_VIEW_CHUNKS) * CHUNK_SIZE + 1, bz1 = (playerChunkZ + LM_VIEW_CHUNKS + 1) * CHUNK_SIZE - 1;
                long long slabRays = 0, slabBad = 0;
                for (int wx = r.x0; wx < r.x1; wx++) {
                    for (int wz = r.z0; wz < r.z1; wz++) {
                        if (getCellWorld(wx, wz) != 0) continue;
                        if (gLaStory >= 0 && !laNearStory(wx, wz, 4)) continue;
                        u32 hc = lmHash((u32)wx, (u32)wz, 3u, 0x4155u);
                        for (int e = 0; e < 2; e++) {
                            float ex = (wx + 0.5f) * CS, ez = (wz + 0.5f) * CS;
                            if (e == 1) {
                                ex += ((float)(hc & 255u) / 255.0f - 0.5f) * 4.2f;
                                ez += ((float)((hc >> 8) & 255u) / 255.0f - 0.5f) * 4.2f;
                            }
                            if (collideWorld(ex, ez, 0.30f)) continue;
                            bool inPillar = false;
                            for (auto& p : pillars) if (fabsf(p.x - ex) < 1.0f && fabsf(p.z - ez) < 1.0f) inPillar = true;
                            if (inPillar) continue;
                            float o[3] = {ex, PH + lmGroundHeightAt(ex, ez, CS), ez};
                            eyes++;
                            // rays: the sphere + rays aimed at the nearby cell-corner lattice lines
                            static float rd[NDIR + 512][3];
                            int nr = 0;
                            for (int k = 0; k < NDIR; k++) { rd[nr][0] = dirs[k][0]; rd[nr][1] = dirs[k][1]; rd[nr][2] = dirs[k][2]; nr++; }
                            static const float hs[5] = {0.04f, 1.3f, 2.62f, 4.46f, 4.56f};
                            for (int cxx = wx - 2; cxx <= wx + 3; cxx++) for (int czz = wz - 2; czz <= wz + 3; czz++) {
                                for (int hk = 0; hk < 5; hk++) for (int q = 0; q < 2; q++) {
                                    u32 hq = lmHash((u32)cxx, (u32)czz, (u32)(hk * 2 + q + e * 16), 0x4156u);
                                    float tx = cxx * CS + ((float)(hq & 255u) / 255.0f - 0.5f) * 0.44f;
                                    float tz = czz * CS + ((float)((hq >> 8) & 255u) / 255.0f - 0.5f) * 0.44f;
                                    float tyv = hs[hk] + ((float)((hq >> 16) & 255u) / 255.0f - 0.5f) * 0.06f;
                                    float dx = tx - o[0], dy = tyv - o[1], dz = tz - o[2];
                                    float l = sqrtf(dx * dx + dy * dy + dz * dz);
                                    if (l < 0.3f) continue;
                                    rd[nr][0] = dx / l; rd[nr][1] = dy / l; rd[nr][2] = dz / l; nr++;
                                }
                            }
                            for (int k = 0; k < nr; k++) {
                                const float* d = rd[k];
                                // distance at which the ray leaves the loaded box
                                float tExit = 260.0f;
                                if (d[0] > 1e-5f) tExit = fminf(tExit, (bx1 * CS - o[0]) / d[0]);
                                if (d[0] < -1e-5f) tExit = fminf(tExit, (bx0 * CS - o[0]) / d[0]);
                                if (d[2] > 1e-5f) tExit = fminf(tExit, (bz1 * CS - o[2]) / d[2]);
                                if (d[2] < -1e-5f) tExit = fminf(tExit, (bz0 * CS - o[2]) / d[2]);
                                LaHit h = grid.cast(o, d, tExit);
                                rays++; slabRays++;
                                int kind = 0;   // 1 BACK, 2 VOID
                                float tt = 0.0f;
                                // (a back face with a surface right behind it is a thin overlay such as a
                                //  dado panel 2 cm proud of its wall, not a hole)
                                if (h.tBack < h.tVis - 0.07f && h.tBack < tExit) { kind = 1; tt = h.tBack; }
                                else if (h.tVis > 1e29f) {
                                    // Nothing hit: a hole if the ray leaves the world upward/downward or runs
                                    // through the body of a wall cell (a 260 m avenue of aligned ports is not one)
                                    float yEnd = o[1] + d[1] * tExit;
                                    if (yEnd > 70.0f || yEnd < -70.0f) { kind = 2; tt = fminf(tExit, 12.0f); }
                                    else {
                                        for (float tq = 1.0f; tq < tExit && !kind; tq += 1.0f) {
                                            float qx = o[0] + d[0] * tq, qy = o[1] + d[1] * tq, qz = o[2] + d[2] * tq;
                                            if (qy < 0.2f || qy > 4.3f) continue;
                                            int qcx = (int)floorf(qx / CS), qcz = (int)floorf(qz / CS);
                                            float fx = qx - qcx * CS, fz = qz - qcz * CS;
                                            if (fx < 0.4f || fz < 0.4f || fx > CS - 0.4f || fz > CS - 0.4f) continue;
                                            if (lmTagAt(qcx, qcz) == LT_WALL) { kind = 2; tt = tq; }
                                        }
                                    }
                                }
                                int sA = -1, sB = -1;
                                if (!kind && h.tVis < 1e29f && h.tVis < tExit) {
                                    kind = grid.surface(o, d, h.tVis, tExit, sA, sB);
                                    tt = h.tVis;
                                }
                                if (!kind) continue;
                                if (kind == 3) {
                                    static const float jy[2] = {0.0012f, 0.0f}, jp[2] = {0.0f, 0.0012f};
                                    bool ok = true;
                                    for (int q = 0; q < 2 && ok; q++) {
                                        float d2[3] = {d[0] + d[2] * jy[q], d[1] + jp[q], d[2] - d[0] * jy[q]};
                                        if (fabsf(d[1]) > 0.95f) { d2[0] = d[0] + jy[q]; d2[1] = d[1]; d2[2] = d[2] + jp[q]; }
                                        float l2 = sqrtf(d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2]);
                                        d2[0] /= l2; d2[1] /= l2; d2[2] /= l2;
                                        LaHit h2 = grid.cast(o, d2, tExit);
                                        int a2, b2;
                                        if (!(h2.tVis < 1e29f && h2.tVis <= h2.tBack + 0.07f && grid.surface(o, d2, h2.tVis, tExit, a2, b2) == kind)) ok = false;
                                    }
                                    if (!ok) continue;
                                }
                                if (kind < 3) {   // A real defect has width and height: it must survive a 0.07 degree nudge of
                                    // the ray sideways AND up/down (grazing along an edge survives only one).
                                    static const float jy[4] = {0.0012f, -0.0012f, 0.0f, 0.0f}, jp[4] = {0.0f, 0.0f, 0.0012f, -0.0012f};
                                    bool okYaw = false, okPitch = false;
                                    for (int q = 0; q < 4; q++) {
                                        float d2[3] = {d[0] + d[2] * jy[q], d[1] + jp[q], d[2] - d[0] * jy[q]};
                                        if (fabsf(d[1]) > 0.95f) { d2[0] = d[0] + jy[q]; d2[1] = d[1]; d2[2] = d[2] + jp[q]; }
                                        float l2 = sqrtf(d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2]);
                                        d2[0] /= l2; d2[1] /= l2; d2[2] /= l2;
                                        LaHit h2 = grid.cast(o, d2, tExit);
                                        bool again = (kind == 1) ? (h2.tBack < h2.tVis - 0.07f && h2.tBack < tExit)
                                                                 : (h2.tVis > 1e29f && h2.tBack > 1e29f);
                                        if (again) { if (q < 2) okYaw = true; else okPitch = true; }
                                    }
                                    if (!okYaw || !okPitch) continue;
                                }
                                // Dado panels stand 2 cm proud of their wall and have no end returns: past a
                                // convex corner the eye slips behind the panel edge. Counted apart, not a hole.
                                if (kind == 1 && (grid.tris[h.backTri].buf == LA_DADO || grid.tris[h.backTri].buf == LA_X_DADO)) {
                                    const float* p = grid.tris[h.backTri].p;
                                    float ny = (p[5]-p[2])*(p[6]-p[0]) - (p[3]-p[0])*(p[8]-p[2]);
                                    float ySpan = fmaxf(p[1], fmaxf(p[4], p[7])) - fminf(p[1], fminf(p[4], p[7]));
                                    if (fabsf(ny) < 1e-5f && ySpan < 2.0f) { overlay++; continue; }
                                }
                                if (kind == 3) zf++; else if (kind == 4) paper++; else { bad++; slabBad++; }
                                float hx = o[0] + d[0] * tt, hy = o[1] + d[1] * tt, hz = o[2] + d[2] * tt;
                                // the cell just behind the surface that was looked through
                                int hcx = (int)floorf((hx + d[0] * 0.02f) / CS), hcz = (int)floorf((hz + d[2] * 0.02f) / CS);
                                char key[400];
                                const char* buf = "-";
                                const char* face = "-";
                                std::string pairName;
                                if (kind >= 3) {
                                    float nn[3]; grid.normal(sA, nn);
                                    face = fabsf(nn[1]) > 0.9f ? "horiz" : (fabsf(nn[1]) < 0.1f ? "vert" : "slope");
                                    pairName = laBufName(grid.tris[sA].buf);
                                    if (sB >= 0) { pairName += "+"; pairName += laBufName(grid.tris[sB].buf); }
                                    buf = pairName.c_str();
                                }
                                if (kind == 1) {
                                    const float* p = grid.tris[h.backTri].p;
                                    float nx = (p[4]-p[1])*(p[8]-p[2]) - (p[5]-p[2])*(p[7]-p[1]);
                                    float ny = (p[5]-p[2])*(p[6]-p[0]) - (p[3]-p[0])*(p[8]-p[2]);
                                    float nz = (p[3]-p[0])*(p[7]-p[1]) - (p[4]-p[1])*(p[6]-p[0]);
                                    float nl = sqrtf(nx*nx + ny*ny + nz*nz) + 1e-9f;
                                    face = fabsf(ny) / nl > 0.9f ? (ny > 0 ? "up" : "down") : (fabsf(ny) / nl < 0.1f ? "vert" : "slope");
                                    buf = laBufName(grid.tris[h.backTri].buf);
                                }
                                const char* yb = hy < -0.05f ? "y<0" : (hy < 2.6f ? "y0-2.6" : (hy < 4.45f ? "y2.6-4.5" : (hy < 4.6f ? "y~4.5" : "y>4.5")));
                                std::string form = lmSlabIsVault(s) ? ("vault form " + std::to_string(s.megaForm)) : std::string("1-storey");
                                snprintf(key, sizeof(key), "%s %s/%s %s | eye %s -> at %s | %s", kind == 1 ? "BACK" : (kind == 2 ? "VOID" : (kind == 3 ? "ZFIGHT" : "PAPER")), buf, face, yb,
                                         laCellDesc(wx, wz).c_str(), laCellDesc(hcx, hcz).c_str(), form.c_str());
                                LaFinding& f = found[key];
                                f.count++;
                                if (f.nEx < 3 && (f.nEx == 0 || (f.count % 37) == 0)) {
                                    float yaw = atan2f(d[0], d[2]), pitch = asinf(d[1]);
                                    snprintf(f.example[f.nEx++], 300, "L%d slab(%d,%d) at %.2f %.2f %.4f %.4f  eyeY %.2f  hit(%.2f,%.2f,%.2f) t=%.2f cell(%d,%d)",
                                             level, j, i, o[0], o[2], yaw, pitch, o[1], hx, hy, hz, tt, hcx, hcz);
                                }
                            }
                        }
                    }
                }
                printf("slab(%d,%d) %s tiers %d form %d: rays %lld bad %lld (%.4f%%)\n", j, i, lmSlabIsVault(s) ? "vault" : "flat",
                       s.ceilTiers, s.megaForm, slabRays, slabBad, slabRays ? 100.0 * slabBad / slabRays : 0.0);
                fflush(stdout);
                done++;
            }
        }
    }
    std::vector<std::pair<long long, std::string>> order;
    for (auto& kv : found) order.push_back({kv.second.count, kv.first});
    std::sort(order.begin(), order.end(), [](const std::pair<long long, std::string>& a, const std::pair<long long, std::string>& b) { return a.first > b.first; });
    printf("\n== findings by kind (%zu kinds) ==\n", order.size());
    int shown = 0;
    for (auto& o : order) {
        if (shown++ >= (verbose > 0 ? verbose : 60)) break;
        const LaFinding& f = found[o.second];
        printf("%7lld  %s\n", f.count, o.second.c_str());
        for (int k = 0; k < f.nEx && k < 2; k++) printf("           %s\n", f.example[k]);
    }
    if (gLmZfHistOn) {
        std::vector<std::pair<long long, std::string>> zo;
        for (auto& kv : gLmZfHist) zo.push_back({kv.second.pairs, kv.first});
        std::sort(zo.begin(), zo.end(), [](const std::pair<long long, std::string>& a, const std::pair<long long, std::string>& b) { return a.first > b.first; });
        printf("\n== coplanar overlaps the z-fight guard had to cut ==\n");
        for (size_t q = 0; q < zo.size() && q < 40; q++) {
            const LmZfHist& h = gLmZfHist[zo[q].second];
            printf("%8lld pairs %9.2f m2  %s   e.g. (%.2f, %.2f, %.2f) %.0f cm2\n", h.pairs, h.area, zo[q].second.c_str(), h.ex[0], h.ex[1], h.ex[2], h.exArea * 1e4f);
        }
    }
    printf("z-fight guard: %lld runs, %lld pairs repaired, %.1f ms total\n", gLmZfRuns, gLmZfPairs, gLmZfMs);
    for (int k = 0; k < 3; k++) printf("   %s: %lld runs, %lld tris, %lld pairs, %.1f ms\n", k == 0 ? "slab" : (k == 1 ? "props" : "world"), gLmZfRunsBy[k], gLmZfTrisBy[k], gLmZfPairsBy[k], gLmZfMsBy[k]);
    printf("\nAUDIT L%d seed %u: slabs %d, eyes %lld, rays %lld, bad %lld (%.5f%%), panel-edge slips %lld, zfight %lld, paper %lld\n", level, seed, done, eyes, rays, bad,
           rays ? 100.0 * bad / rays : 0.0, overlay, zf, paper);
    return 0;
}

// `limen_shots.exe ray <seed> <n> <level> <x> <z> <yaw> <pitch> [eyeY]`: every surface one ray crosses, in order.
static int lmRunAuditRay(u32 seed, float x, float z, float yaw, float pitch, float eyeY) {
    cam.pos = Vec3(x, PH, z);
    chunks.clear();
    rng.seed(seed); srand(seed);
    updateVisibleChunks(x, z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    resetMapContent();
    updateMapContent(playerChunkX, playerChunkZ);
    buildGeom();
    std::vector<LmAuditTri> tris = gLmAuditWorld;
    {
        std::vector<float> pm;
        for (const auto& pr : mapProps) lmAppendPropModel(pm, pr);
        for (const auto& poi : mapPois) lmAppendPoiModel(pm, poi);
        lmAuditAppend(tris, pm, LA_PROP, true);
    }
    for (long long skey : gLmVisibleSlabKeys) {
        auto it = gLmAuditSlab.find(skey);
        if (it != gLmAuditSlab.end()) tris.insert(tris.end(), it->second.begin(), it->second.end());
    }
    float o[3] = {x, eyeY > -900.0f ? eyeY : PH + lmGroundHeightAt(x, z, CS), z};
    float d[3] = {sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch)};
    struct H { float t; int k; bool front; };
    std::vector<H> hits;
    for (int k = 0; k < (int)tris.size(); k++) {
        const float* p = tris[k].p;
        float e1x = p[3]-p[0], e1y = p[4]-p[1], e1z = p[5]-p[2], e2x = p[6]-p[0], e2y = p[7]-p[1], e2z = p[8]-p[2];
        float px = d[1]*e2z - d[2]*e2y, py = d[2]*e2x - d[0]*e2z, pz = d[0]*e2y - d[1]*e2x;
        float det = e1x*px + e1y*py + e1z*pz;
        if (det > -1e-7f && det < 1e-7f) continue;
        float inv = 1.0f / det, tx = o[0]-p[0], ty = o[1]-p[1], tz = o[2]-p[2];
        float u = (tx*px + ty*py + tz*pz) * inv;
        if (u < -1e-4f || u > 1.0001f) continue;
        float qx = ty*e1z - tz*e1y, qy = tz*e1x - tx*e1z, qz = tx*e1y - ty*e1x;
        float v = (d[0]*qx + d[1]*qy + d[2]*qz) * inv;
        if (v < -1e-4f || u + v > 1.0001f) continue;
        float tt = (e2x*qx + e2y*qy + e2z*qz) * inv;
        if (tt < 0.02f || tt > 300.0f) continue;
        hits.push_back({tt, k, det > 0.0f});
    }
    std::sort(hits.begin(), hits.end(), [](const H& a, const H& b) { return a.t < b.t; });
    {
        LaGrid g; g.tris = tris; g.build((int)floorf(x / CS), (int)floorf(z / CS), 150);
        LaHit h = g.cast(o, d, 250.0f);
        int a = -1, b = -1;
        gLaDebug = true;
        int kind = (h.tVis < 1e29f) ? g.surface(o, d, h.tVis, 250.0f, a, b) : 0;
        gLaDebug = false;
        printf("surface check: %s\n", kind == 3 ? "ZFIGHT" : (kind == 4 ? "PAPER" : "ok"));
    }
    printf("eye (%.2f, %.2f, %.2f) in %s\n", o[0], o[1], o[2], laCellDesc((int)floorf(x / CS), (int)floorf(z / CS)).c_str());
    for (size_t q = 0; q < hits.size() && q < 14; q++) {
        const LmAuditTri& t = tris[hits[q].k];
        float hx = o[0] + d[0] * hits[q].t, hy = o[1] + d[1] * hits[q].t, hz = o[2] + d[2] * hits[q].t;
        printf("  t=%7.2f  %-9s %s  at (%.2f, %.2f, %.2f) cell(%d,%d) %s | tri (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
               hits[q].t, laBufName(t.buf), t.twoSided ? "2-sided" : (hits[q].front ? "FRONT  " : "back   "), hx, hy, hz,
               (int)floorf(hx / CS), (int)floorf(hz / CS), laCellDesc((int)floorf((hx + d[0] * 0.05f) / CS), (int)floorf((hz + d[2] * 0.05f) / CS)).c_str(),
               t.p[0], t.p[1], t.p[2], t.p[3], t.p[4], t.p[5], t.p[6], t.p[7], t.p[8]);
    }
    if (!hits.empty()) {   // the cells around the first surface, x to the right, z downward
        float hx = o[0] + d[0] * hits[0].t, hz = o[2] + d[2] * hits[0].t;
        int ccx = (int)floorf(hx / CS), ccz = (int)floorf(hz / CS);
        static const char glyph[LT_TAG_COUNT + 1] = "#.r ~=u,v";
        for (int cz = ccz - 7; cz <= ccz + 7; cz++) {
            printf("  z=%4d  ", cz);
            for (int cx = ccx - 9; cx <= ccx + 9; cx++) {
                LimenRect r = lmSlabAt(cx, cz);
                char ch;
                if (cx == r.x0 || cz == r.z0) ch = lmWallCellOpen(r, cx, cz) ? '+' : '|';
                else {
                    ch = glyph[lmTagAt(cx, cz)];
                    if (ch == '#' && (lmFlagAt(cx, cz) & LF_LOW_WALL)) ch = '%';
                }
                {   // what the game made of the cell: O opened, X closed
                    auto ov = gCellOverrides.find(cellKey(cx, cz));
                    if (ov != gCellOverrides.end() && (ov->second != 0) != (lmCell(cx, cz) != 0)) ch = ov->second ? 'X' : 'O';
                }
                if (cx == (int)floorf(x / CS) && cz == (int)floorf(z / CS)) ch = '@';
                putchar(ch);
            }
            printf("%s", cz == ccz - 7 ? "   x from " : ""); if (cz == ccz - 7) printf("%d", ccx - 9);
            putchar('\n');
        }
    }
    return 0;
}
