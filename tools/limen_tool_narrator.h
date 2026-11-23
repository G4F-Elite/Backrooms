#pragma once
// limen_tool: the narrator driven through simulated play (no game code).
//   tour <seed> <levels> [rigCells]   a crew rides the rig down <levels> floors, looting around it
//   roam <seed> <level> [minutes]     one player walks the level with no rig, changing heading now and then
//   narbench <seed> [players] [levels] [minutes]   many such players over a run of levels: the sums a change is judged by
// Both run twice, narrator off and on, and print what the player came to see. `test` uses the
// same walks to check that beats never break the world and that nothing seen ever changes.
#include "../src/limen_narrator.h"
#include <random>

struct NarSimRoad { int baseX, baseZ, startX, startZ, dx, dz; };

// Where the game would put the rig: the first lit lamp of the far corner chunk in view of the
// origin is the arrival point; the heading comes from the seed (the first floor always drives +X).
static NarSimRoad narSimRoad(u32 seed, int level) {
    NarSimRoad r = {8, 8, 8, 8, 1, 0};
    bool found = false;
    for (int cx = -3; cx <= 3 && !found; cx++) for (int cz = -3; cz <= 3 && !found; cz++)
        for (int lx = 0; lx < 16 && !found; lx++) for (int lz = 0; lz < 16 && !found; lz++) {
            int wx = cx * 16 + lx, wz = cz * 16 + lz;
            if (lmWalkable(wx, wz) && lmLampAt(wx, wz) == 1) { r.baseX = wx; r.baseZ = wz; found = true; }
        }
    r.startX = r.baseX; r.startZ = r.baseZ;
    if (level == 0) { r.startX += 2; r.dx = 1; r.dz = 0; }
    else {
        std::mt19937 g(seed ^ 0x0D2111u);
        int dir = (int)(g() % 4u);
        r.dx = dir == 0 ? 1 : (dir == 1 ? -1 : 0);
        r.dz = dir == 2 ? 1 : (dir == 3 ? -1 : 0);
    }
    return r;
}

static void narSimOpenCell(int wx, int wz) {
    gLmStory.edits[lmCellKey64(wx, wz)] = 0;
    for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) {
        LimenRect r = lmSlabAt(wx + dx, wz + dz);
        long long key = lmSlabKey(r.j, r.i);
        if (gLimenSlabs.erase(key)) { gLimenSlabUse.erase(key); gLimenLastSlab = nullptr; }
    }
}

static u32 narSlabChecksum(const LimenSlab& s) {
    u32 crc = 0x811C9DC5u;
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) {
        u32 v = (u32)s.tag[x][z] | ((u32)s.flag[x][z] << 4) | ((u32)(s.zone[x][z] & ~LZ_STORY) << 20);
        crc = (crc ^ v) * 16777619u;
    }
    return crc ^ (u32)(s.megaForm + 2) * 2654435761u ^ (u32)s.ceilTiers * 40503u;
}

struct NarSimStats {
    unsigned long long seenEver = 0;
    int levelCount[16] = {};
    int rewrites = 0, changedSeen = 0, stuck = 0, strandedCells = 0, brokenSeams = 0;
    int bigFinds = 0;        // megastructures and strangeness come upon, repeats included
    double megaSeen = 0, strangeSeen = 0, quietSeen = 0;
    int majorsPlaced = 0, majorsFound = 0, majorsBack = 0;   // big things the narrator laid down / the player came upon / taken back unseen
    int laid[LN_COUNT] = {}, got[LN_COUNT] = {};   // per kind: laid down by the narrator / of those, come upon
    int freshBig = 0; float lullMax = 0;       // big things new to the level, and the longest wait for one (s)
};

// What was seen must stay as it was: checksum every slab when it first comes into view.
struct NarSeenGuard {
    std::unordered_map<long long, u32> sum;
    void note() {
        for (const auto& kv : gNar.slabs) {
            const LmNarSlab& rec = kv.second;
            if (!(rec.entered || rec.seenCount > 0)) continue;
            if (sum.find(kv.first) != sum.end()) continue;
            int j = (int)(kv.first >> 32), i = (int)(kv.first & 0xFFFFFFFFll);
            sum[kv.first] = narSlabChecksum(lmSlab(j, i));
        }
    }
    int changed() const {
        int bad = 0;
        for (const auto& kv : sum) {
            int j = (int)(kv.first >> 32), i = (int)(kv.first & 0xFFFFFFFFll);
            if (narSlabChecksum(lmSlab(j, i)) != kv.second) bad++;
        }
        return bad;
    }
};

static int narCountStranded(int x0, int z0, int w, int h) {
    std::vector<int> comp((size_t)w * h, -1), size, touches, stack;
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
static void narDbgSeam(int x, int z) {
    LimenRect w = lmSlabAt(x - 1, z), e = lmSlabAt(x + 1, z);
    const LimenSlab& ws = lmSlab(w.j, w.i);
    int lx = x - 1 - w.x0 - 1, lz = z - w.z0 - 1;
    LimenSlab fresh; lmBuildSlab(fresh, w.j, w.i); lmPlaceLampSites(fresh);
    printf("BROKEN seam (%d,%d) W rect x %d..%d z %d..%d, lx=%d lz=%d (w=%d) built tag %d fresh-nostory tag %d | seam open by rule: %d | E rect x0 %d\n",
           x, z, w.x0, w.x1, w.z0, w.z1, lx, lz, ws.w, ws.tag[lx][lz], fresh.tag[lx][lz], (int)lmWallCellOpen(e, e.x0, z), e.x0);
    printf("   W beat force %x road %d@%d through %d | W zone %x flag %x | story lane=%d\n", lmBeatForce(w.j, w.i), ws.roadAxis, ws.roadC, (int)ws.roadThrough,
           ws.zone[lx][lz], ws.flag[lx][lz], (int)gLmStory.lane);
}
static int narCountBrokenSeams(int x0, int z0, int w, int h) {
    int broken = 0;
    for (int z = z0; z < z0 + h; z++) for (int x = x0; x < x0 + w; x++) {
        LimenRect r = lmSlabAt(x, z);
        if ((x != r.x0 && z != r.z0) || lmCell(x, z) != 0 || lmStoryEdit(x, z) >= 0) continue;
        if (x == r.x0 && (!lmWalkable(x - 1, z) || !lmWalkable(x + 1, z))) { broken++; if (getenv("NARDBG")) narDbgSeam(x, z); }
        if (z == r.z0 && x != r.x0 && (!lmWalkable(x, z - 1) || !lmWalkable(x, z + 1))) broken++;
    }
    return broken;
}

static void narPrintSeen(unsigned long long mask) {
    bool any = false;
    for (int f = 0; f < LN_QUIRK_FIRST; f++) if (mask & lmFeatBit(f)) { printf("%s%s", any ? ", " : "", lmFeatInfo(f).name); any = true; }
    if (!any) printf("-");
}

static void narSimResetWorld(u32 seed, int level, bool narratorOn, bool adaptive) {
    gLmStory = LmStory();
    gLmStorySlabEpoch.clear();
    lmReset(seed + 1, level); lmReset(seed, level);
    gNar.enabled = narratorOn; gNar.adaptive = adaptive; gNar.rewrite = true; gNar.busy = nullptr;
    lmNarSync();
}

// A crew on one floor: the rig crawls `rigCells` cells, the player loots around it.
static void narSimLevel(u32 seed, int level, int rigCells, bool narratorOn, bool adaptive, NarSimStats& st, bool print, NarSeenGuard* guard) {
    narSimResetWorld(seed, level, narratorOn, adaptive);
    NarSimRoad road = narSimRoad(seed, level);
    lmNarSetRoad(road.startX, road.startZ, road.dx, road.dz);
    if (narratorOn) { lmNarPlanRoad(); lmSlabCacheClear(); }
    // the plan must leave a sound world behind (before the rig starts chewing through it)
    {
        int ex = road.startX + road.dx * 45, ez = road.startZ + road.dz * 45;
        int x0 = (road.startX < ex ? road.startX : ex) - 70, z0 = (road.startZ < ez ? road.startZ : ez) - 70;
        int w = abs(ex - road.startX) + 140, h = abs(ez - road.startZ) + 140;
        st.strandedCells += narCountStranded(x0, z0, w, h);
        st.brokenSeams += narCountBrokenSeams(x0, z0, w, h);
    }
    gLmStory.lane = true; gLmStory.laneX = road.startX; gLmStory.laneZ = road.startZ; gLmStory.laneDX = road.dx; gLmStory.laneDZ = road.dz;
    gLmStoryEpoch++;
    lmSlabCacheClear();
    narSimOpenCell(road.startX - road.dx, road.startZ - road.dz);
    const float cs = 5.0f;
    static const int ddx[4] = {1, -1, 0, 0}, ddz[4] = {0, 0, 1, -1};
    for (int s = 0; s <= rigCells; s++) {
        int rx = road.startX + road.dx * s, rz = road.startZ + road.dz * s;
        narSimOpenCell(rx, rz);
        narSimOpenCell(rx + road.dx, rz + road.dz);      // the auger is a cell ahead of the break room
        // the player ranges up to 9 cells from the rig over whatever can be walked
        const int R = 9, W = 2 * R + 1;
        std::vector<int> dist((size_t)W * W, -1), queue;
        dist[(size_t)R * W + R] = 0; queue.push_back(R * W + R);
        std::vector<int> stops;
        for (size_t head = 0; head < queue.size(); head++) {
            int c = queue[head], cx = c % W - R, cz = c / W - R;
            if (((cx + cz) & 1) == 0 && (int)stops.size() < 48) stops.push_back(c);
            for (int d = 0; d < 4; d++) {
                int nx = cx + ddx[d], nz = cz + ddz[d];
                if (nx < -R || nz < -R || nx > R || nz > R) continue;
                int n = (nz + R) * W + nx + R;
                if (dist[n] >= 0 || !lmPlayerWalkableAt(rx + nx, rz + nz)) continue;
                dist[n] = dist[c] + 1; queue.push_back(n);
            }
        }
        float dt = (cs / 0.30f) / (float)(stops.size() * 4 + 1);
        for (int c : stops) {
            float px = (rx + c % W - R + 0.5f) * cs, pz = (rz + c / W - R + 0.5f) * cs;
            for (int q = 0; q < 4; q++) lmNarTick(dt, px, pz, (float)q * 1.5707963f, cs, 0.80f);
            gNar.dirty.clear();
            if (guard) guard->note();
        }
    }
    unsigned long long saw = gNar.levelWitnessed;
    // furniture rules fire in the game's furnisher, which the tool does not run: take the
    // narrator's word for those it placed in slabs the crew walked through
    for (const LmNarPlaced& p : gNar.placed) {
        if (p.feat < LN_MONO || p.feat > LN_PROCESSION) continue;
        auto it = gNar.slabs.find(p.slab);
        if (it != gNar.slabs.end() && it->second.entered && it->second.seenCount >= 60) saw |= lmFeatBit(p.feat);
    }
    st.seenEver |= saw;
    int nMega = 0, nStrange = 0, nQuiet = 0;
    for (int f = 0; f < LN_QUIRK_FIRST; f++) if (saw & lmFeatBit(f)) {
        int c = lmFeatInfo(f).cls;
        if (c == LNC_MEGA) nMega++; else if (c == LNC_STRANGE) nStrange++; else if (c == LNC_QUIET) nQuiet++;
    }
    st.megaSeen += nMega; st.strangeSeen += nStrange; st.quietSeen += nQuiet;
    if (level >= 0 && level < 16) st.levelCount[level] = nMega + nStrange + nQuiet;
    if (print) {
        printf("  L%-2d %s%c  ", level, road.dx ? "x" : "z", (road.dx + road.dz) > 0 ? '+' : '-');
        narPrintSeen(saw & ~(lmFeatBit(LN_WING_PARTITIONS) | lmFeatBit(LN_WING_CORRIDORS) | lmFeatBit(LN_WING_ENFILADE) |
                             lmFeatBit(LN_WING_CONCOURSE) | lmFeatBit(LN_WING_SERVICE)));
        printf("\n");
    }
    gLmStory = LmStory();
}

static int narCatalogueSize() { int n = 0; for (int f = 0; f < LN_QUIRK_FIRST; f++) n++; return n; }
static int narBits(unsigned long long m) { int n = 0; for (int f = 0; f < LN_QUIRK_FIRST; f++) if (m & lmFeatBit(f)) n++; return n; }
static int narBitsOfClass(unsigned long long m, int cls) {
    int n = 0;
    for (int f = 0; f < LN_QUIRK_FIRST; f++) if ((m & lmFeatBit(f)) && lmFeatInfo(f).cls == cls) n++;
    return n;
}

static NarSimStats narRunTour(u32 seed, int levels, int rigCells, int mode, bool print) {
    // mode 0: narrator off, 1: adaptive, 2: script (co-op)
    NarSimStats st;
    gNar.savePath.clear();
    gNar.mem = LmNarMemory();
    if (print) printf("%s:\n", mode == 0 ? "narrator off" : (mode == 1 ? "narrator on (adaptive, remembers)" : "narrator on (script, as in co-op)"));
    for (int lv = 0; lv < levels; lv++) {
        narSimLevel(seed + (u32)lv * 7919u, lv, rigCells, mode != 0, mode == 1, st, print, nullptr);
        if (print && (lv + 1) % 4 == 0)
            printf("     after %2d floors: %d of %d seen (megastructures %d/%d, strangeness %d/%d, quiet %d/%d)\n", lv + 1,
                   narBits(st.seenEver), narCatalogueSize(), narBitsOfClass(st.seenEver, LNC_MEGA), lmNarClassSize(LNC_MEGA),
                   narBitsOfClass(st.seenEver, LNC_STRANGE), lmNarClassSize(LNC_STRANGE), narBitsOfClass(st.seenEver, LNC_QUIET), lmNarClassSize(LNC_QUIET));
    }
    if (print) {
        printf("  %d floors: %d of %d seen; per floor %.1f megastructures, %.1f strange, %.1f quiet; never seen: ", levels,
               narBits(st.seenEver), narCatalogueSize(), st.megaSeen / levels, st.strangeSeen / levels, st.quietSeen / levels);
        narPrintSeen(~st.seenEver & (lmFeatBit(LN_QUIRK_FIRST) - 1));
        printf("\n  world after planning: stranded cells %d, openings into walls %d\n", st.strandedCells, st.brokenSeams);
    }
    return st;
}

// One player on foot, no rig: walks at 4 m/s towards a far point, turns now and then, and
// like anybody steps into a great hall when one shows through a doorway.
// Debug: every seen slab's cached build must equal a fresh build with the beats as they stand now.
static bool narFixedMatchesFresh(long long key, int* where) {
    int j = (int)(key >> 32), i = (int)(key & 0xFFFFFFFFll);
    const LimenSlab& c = lmSlab(j, i);
    LimenSlab f; lmBuildSlab(f, j, i); lmPlaceLampSites(f); lmApplyStory(f);
    if (c.megaForm != f.megaForm || c.megaSub != f.megaSub || c.ceilTiers != f.ceilTiers || c.haloDir != f.haloDir || c.siteCount != f.siteCount) {
        if (where) { where[0] = -1; where[1] = -1; }
        printf("   cached mega %d/%d tiers %d halo %d sites %d | fresh mega %d/%d tiers %d halo %d sites %d\n", c.megaForm, c.megaSub, c.ceilTiers, c.haloDir, c.siteCount, f.megaForm, f.megaSub, f.ceilTiers, f.haloDir, f.siteCount);
        return false;
    }
    for (int x = 0; x < c.w; x++) for (int z = 0; z < c.h; z++) if (c.tag[x][z] != f.tag[x][z] || c.flag[x][z] != f.flag[x][z] || c.zone[x][z] != f.zone[x][z]) {
        if (where) { where[0] = x; where[1] = z; }
        printf("   cached tag %d fresh %d | self tiers %d beat %d force %x | N tiers %d/%d E/W %d/%d | S/N rows %d/%d | halo %d pin %d | cached mega %d fresh %d | story lane %d edits %d\n", c.tag[x][z], f.tag[x][z], lmSlabTiers(j, i), lmBeatAt(j, i) ? (int)lmBeatAt(j, i)->megaForm : -9, lmBeatForce(j, i), lmSlabTiers(j, i - 1), lmSlabTiers(j, i + 1), lmSlabTiers(j - 1, i), lmSlabTiers(j + 1, i), lmSlabTiers(j - 1, i), lmSlabTiers(j + 1, i), lmSlabHaloDir(j, i), lmBeatAt(j, i) ? (int)lmBeatAt(j, i)->haloPin : -9, c.megaForm, f.megaForm, (int)gLmStory.lane, (int)gLmStory.edits.size());
        return false;
    }
    return true;
}

static NarSimStats narRunRoam(u32 seed, int level, float minutes, bool narratorOn, bool print, bool keepMemory = false) {
    NarSimStats st;
    gNarTrace = print && getenv("NARTRACE") != nullptr;
    gNar.savePath.clear();
    if (!keepMemory) gNar.mem = LmNarMemory();
    narSimResetWorld(seed, level, narratorOn, true);
    NarSimRoad home = narSimRoad(seed, level);
    const float cs = 5.0f;
    int cx = home.baseX, cz = home.baseZ;
    float heading = 0.6f;
    NarSeenGuard guard;
    const int R = 40, W = 2 * R + 1;
    std::vector<int> prev((size_t)W * W);
    std::vector<std::pair<int, int>> path;
    size_t pathPos = 0;
    int steps = (int)(minutes * 60.0f / 1.25f);     // a cell every 1.25 s
    std::vector<long long> lured;                   // vaults already gone to look at
    long long lure = 0; int lureTries = 0;
    static const int ddx[4] = {1, -1, 0, 0}, ddz[4] = {0, 0, 1, -1};
    float fx = (cx + 0.5f) * cs, fz = (cz + 0.5f) * cs;
    for (int s = 0; s < steps; s++) {
        if ((s % 150) == 149) heading += ((float)(lmHash((u32)s, seed, 3u, 0x524Du) % 200u) / 100.0f - 1.0f) * 1.1f;
        if (pathPos >= path.size()) {
            // breadth-first over what can be walked, towards the cell furthest along the heading
            std::fill(prev.begin(), prev.end(), -2);
            std::vector<int> queue;
            prev[(size_t)R * W + R] = -1; queue.push_back(R * W + R);
            int best = R * W + R; float bestS = -1e9f;
            int lureCell = -1;
            if (lure != 0 && (++lureTries > 5 || (gNar.slabs.count(lure) && gNar.slabs[lure].inside > 4.0f))) lure = 0;
            for (size_t head = 0; head < queue.size(); head++) {
                int c = queue[head], lx = c % W - R, lz = c / W - R;
                float sc = (float)lx * sinf(heading) + (float)lz * cosf(heading);
                if (sc > bestS) { bestS = sc; best = c; }
                if (lureCell < 0) {
                    LimenRect q = lmSlabAt(cx + lx, cz + lz);
                    if (cx + lx != q.x0 && cz + lz != q.z0) {
                        long long key = lmSlabKey(q.j, q.i);
                        const LmNarSlab* rec = lmNarRecPeek(q.j, q.i);
                        const LimenSlab* sl = lmSlabPeek(q.j, q.i);
                        bool known = false;
                        for (long long k : lured) if (k == key) known = true;
                        if (key == lure) lureCell = c;
                        else if (lure == 0 && !known && rec && sl && !rec->entered && rec->seenCount >= 8 && lmSlabIsVault(*sl)) {
                            lure = key; lureTries = 0; lured.push_back(key); lureCell = c;
                        }
                    }
                }
                for (int d = 0; d < 4; d++) {
                    int nx = lx + ddx[d], nz = lz + ddz[d];
                    if (nx < -R || nz < -R || nx > R || nz > R) continue;
                    int n = (nz + R) * W + nx + R;
                    if (prev[n] != -2 || !lmWalkable(cx + nx, cz + nz)) continue;
                    prev[n] = c; queue.push_back(n);
                }
            }
            if (lure != 0 && lureCell >= 0) best = lureCell;
            path.clear(); pathPos = 0;
            for (int c = best; c >= 0 && prev[c] != -2; c = prev[c]) { path.push_back({cx + c % W - R, cz + c / W - R}); if (prev[c] == -1) break; }
            std::reverse(path.begin(), path.end());
            if (path.size() < 3) { heading += 1.3f; st.stuck++; }
            if (path.size() > 26) path.resize(26);
            pathPos = path.size() > 1 ? 1 : path.size();
            if (pathPos >= path.size()) continue;
        }
        int nx = path[pathPos].first, nz = path[pathPos].second;
        pathPos++;
        if (!lmWalkable(nx, nz)) { path.clear(); pathPos = 0; st.stuck++; continue; }   // the way was rewritten: look again
        float tx = (nx + 0.5f) * cs, tz = (nz + 0.5f) * cs;
        float yaw = atan2f(tx - fx, tz - fz);
        for (int q = 0; q < 5; q++) {
            fx += (tx - fx) / (float)(5 - q); fz += (tz - fz) / (float)(5 - q);
            // the player glances about while walking
            float look = yaw + 0.5f * sinf((float)(s * 5 + q) * 0.35f);
            lmNarTick(0.25f, fx, fz, look, cs);
            if (getenv("NARDBG2")) {
                static int reported = 0;
                // (every slab in the cache, seen or not: meshes and chunk cells are made from all of them)
                std::vector<std::pair<long long, int>> keys;
                for (const auto& kv : gLimenSlabs) keys.push_back({kv.first, 0});
                for (const auto& kv : keys) {
                    if (reported) break;
                    int at[2] = {0, 0};
                    if (!narFixedMatchesFresh(kv.first, at)) {
                        reported = 1;
                        { int sj = (int)(kv.first >> 32), si = (int)(kv.first & 0xFFFFFFFFll); long long nb[16]; int nn = lmNarNeighbours(sj, si, nb, 16);
                          printf("   seen %d fixed %d beat %d pin %d | tiers W %d E %d N %d S %d | geometric neighbours:", (int)lmNarSlabSeen(sj, si), (int)lmNarFixed(sj, si), lmBeatAt(sj, si) ? (int)lmBeatAt(sj, si)->megaForm : -9, lmBeatAt(sj, si) ? (int)lmBeatAt(sj, si)->haloPin : -9, lmSlabTiers(sj, si - 1), lmSlabTiers(sj, si + 1), lmSlabTiers(sj - 1, si), lmSlabTiers(sj + 1, si));
                          for (int q = 0; q < nn; q++) printf(" (%d,%d)", (int)(nb[q] >> 32), (int)(nb[q] & 0xFFFFFFFFll));
                          printf(" | beats:"); for (const auto& bk : gLmBeats) printf(" (%d,%d)=%d", (int)(bk.first >> 32), (int)(bk.first & 0xFFFFFFFFll), (int)bk.second.megaForm); printf("%c", 10); }
                        printf("STALE slab j%d i%d cell %d,%d at t=%.1f (last: %s)\n", (int)(kv.first >> 32), (int)(kv.first & 0xFFFFFFFFll), at[0], at[1], gNar.time, gNar.logCount ? gNar.log[(gNar.logCount - 1) % 10].text : "-");
                        break;
                    }
                }
            }
        }
        cx = nx; cz = nz;
        if (!gNar.dirty.empty()) { st.rewrites += (int)gNar.dirty.size(); gNar.dirty.clear(); }
        guard.note();
    }
    st.seenEver = gNar.levelWitnessed;
    for (const auto& kv : gNar.slabs) for (int f = 0; f < LN_QUIRK_FIRST; f++)
        if ((kv.second.witnessed & lmFeatBit(f)) && (lmFeatInfo(f).cls == LNC_MEGA || lmFeatInfo(f).cls == LNC_STRANGE)) st.bigFinds++;
    st.changedSeen = guard.changed();
    for (const LmNarPlaced& q : gNar.placed) {
        st.laid[q.feat]++; if (q.witnessed) st.got[q.feat]++;
        if (!q.major) continue;
        st.majorsPlaced++;
        if (q.witnessed) st.majorsFound++; else if (q.gone) st.majorsBack++;
    }
    st.freshBig = gNar.freshBig;
    st.lullMax = fmaxf(gNar.lullMax, gNar.time - gNar.lastFresh);
    st.strandedCells = narCountStranded(cx - 110, cz - 110, 220, 220);
    st.brokenSeams = narCountBrokenSeams(cx - 110, cz - 110, 220, 220);
    if (print) {
        printf("%s, %.0f min on foot: saw %d of %d (megastructures %d, strangeness %d, quiet %d): ", narratorOn ? "narrator on " : "narrator off",
               minutes, narBits(st.seenEver), narCatalogueSize(), narBitsOfClass(st.seenEver, LNC_MEGA),
               narBitsOfClass(st.seenEver, LNC_STRANGE), narBitsOfClass(st.seenEver, LNC_QUIET));
        narPrintSeen(st.seenEver);
        if (narratorOn) {
            int pl[5] = {}, wi[5] = {}, entered = 0, majors = 0;
            for (const LmNarPlaced& q : gNar.placed) {
                int c = lmFeatInfo(q.feat).cls; pl[c]++; if (q.witnessed) wi[c]++;
                if (q.major) { majors++; auto it = gNar.slabs.find(q.slab); if (it != gNar.slabs.end() && it->second.entered) entered++; }
            }
            printf("\n   placed/found: megastructures %d/%d, strangeness %d/%d, quiet %d/%d; slabs with something big that were walked into: %d of %d",
                   pl[LNC_MEGA], wi[LNC_MEGA], pl[LNC_STRANGE], wi[LNC_STRANGE], pl[LNC_QUIET], wi[LNC_QUIET], entered, majors);
        }
        printf("\n   big things come upon: %d (one every %.0f s)", st.bigFinds, st.bigFinds ? minutes * 60.0f / st.bigFinds : 0.0f);
        printf("\n   slabs rewritten unseen: %d, seen slabs that changed: %d, stranded cells: %d, openings into walls: %d, dead ends walked into: %d\n",
               st.rewrites, st.changedSeen, st.strandedCells, st.brokenSeams, st.stuck);
    }
    return st;
}

static void runTour(u32 seed, int levels, int rigCells) {
    printf("tour: seed %u, %d floors, the rig crawls %d cells (%.0f min) on each\n", seed, levels, rigCells, rigCells * 5.0f / 0.30f / 60.0f);
    narRunTour(seed, levels, rigCells, 0, true);
    narRunTour(seed, levels, rigCells, 1, true);
    narRunTour(seed, levels, rigCells, 2, true);
    gNar.enabled = false;
}
static void runRoam(u32 seed, int level, float minutes) {
    printf("roam: seed %u level %d\n", seed, level);
    narRunRoam(seed, level, minutes, false, true);
    narRunRoam(seed, level, minutes, true, true);
    gNar.enabled = false;
}
// roams <seed> <levels> [minutes]: the same player walks one level after another; what does the sum look like?
static void runRoams(u32 seed, int levels, float minutes) {
    printf("roams: seed %u, %d levels, %.0f min on foot in each\n", seed, levels, minutes);
    for (int pass = 0; pass < 2; pass++) {
        unsigned long long all = 0;
        int rewrites = 0, changed = 0;
        gNar.mem = LmNarMemory();
        printf("%s:", pass ? "narrator on " : "narrator off");
        for (int lv = 1; lv <= levels; lv++) {
            NarSimStats st = narRunRoam(seed + (u32)lv * 7919u, lv, minutes, pass != 0, false, true);
            all |= st.seenEver; rewrites += st.rewrites; changed += st.changedSeen;
            printf(" %d", narBits(all));
        }
        printf(" of %d (megastructures %d/%d, strangeness %d/%d, quiet %d/%d); rewritten %d, seen slabs changed %d; never seen: ", narCatalogueSize(),
               narBitsOfClass(all, LNC_MEGA), lmNarClassSize(LNC_MEGA), narBitsOfClass(all, LNC_STRANGE), lmNarClassSize(LNC_STRANGE),
               narBitsOfClass(all, LNC_QUIET), lmNarClassSize(LNC_QUIET), rewrites, changed);
        narPrintSeen(~all & (lmFeatBit(LN_QUIRK_FIRST) - 1));
        printf("\n");
    }
    gNar.enabled = false;
}

// narbench <seed> [seeds] [levels] [minutes]: several players, each walking a run of levels with and without the
// narrator. The sums are what a change to the narrator is judged by.
static void runNarBench(u32 seed, int seeds, int levels, float minutes) {
    printf("narbench: %d players from seed %u, %d levels each, %.0f min on foot in each\n", seeds, seed, levels, minutes);
    for (int pass = 0; pass < 2; pass++) {
        int kinds = 0, mega = 0, strange = 0, quiet = 0, rewrites = 0, changed = 0, stranded = 0, broken = 0, placed = 0, found = 0, back = 0, fresh = 0, big = 0, stuck = 0;
        double lullSum = 0; float lullWorst = 0; int runs = 0;
        int sawFeat[LN_COUNT] = {}, laid[LN_COUNT] = {}, got[LN_COUNT] = {};
        for (int p = 0; p < seeds; p++) {
            unsigned long long all = 0;
            gNar.mem = LmNarMemory();
            for (int lv = 1; lv <= levels; lv++) {
                NarSimStats st = narRunRoam(seed + (u32)p * 104729u + (u32)lv * 7919u, lv, minutes, pass != 0, false, true);
                all |= st.seenEver; rewrites += st.rewrites; changed += st.changedSeen; stranded += st.strandedCells; broken += st.brokenSeams;
                placed += st.majorsPlaced; found += st.majorsFound; back += st.majorsBack; fresh += st.freshBig; big += st.bigFinds; stuck += st.stuck;
                for (int f = 0; f < LN_COUNT; f++) { laid[f] += st.laid[f]; got[f] += st.got[f]; }
                lullSum += st.lullMax; if (st.lullMax > lullWorst) lullWorst = st.lullMax; runs++;
            }
            for (int f = 0; f < LN_QUIRK_FIRST; f++) if (all & lmFeatBit(f)) sawFeat[f]++;
            kinds += narBits(all); mega += narBitsOfClass(all, LNC_MEGA); strange += narBitsOfClass(all, LNC_STRANGE); quiet += narBitsOfClass(all, LNC_QUIET);
        }
        printf("%s: kinds seen per player %.1f of %d (megastructures %.1f, strangeness %.1f, quiet %.1f)\n", pass ? "narrator on " : "narrator off",
               (double)kinds / seeds, narCatalogueSize(), (double)mega / seeds, (double)strange / seeds, (double)quiet / seeds);
        printf("   per level: %.1f big things new to it (%.1f with repeats), longest wait for one %.0f s on average, %.0f s at worst\n",
               (double)fresh / runs, (double)big / runs, lullSum / runs, (double)lullWorst);
        printf("   big things laid down %d: come upon %d, taken back unseen %d, left lying %d; slabs rewritten %d; seen slabs changed %d, stranded cells %d, openings into walls %d, dead ends %d\n",
               placed, found, back, placed - found - back, rewrites, changed, stranded, broken, stuck);
        printf("   seen by fewer than all %d players:", seeds);
        for (int f = 0; f < LN_QUIRK_FIRST; f++) if (sawFeat[f] < seeds && !(f >= LN_MONO && f <= LN_PROCESSION)) printf(" %s %d,", lmFeatInfo(f).name, sawFeat[f]);
        printf("\n");
        if (pass && getenv("NARKINDS")) {
            printf("   laid down / come upon, by kind:");
            for (int f = 0; f < LN_QUIRK_FIRST; f++) if (laid[f]) printf(" %s %d/%d,", lmFeatInfo(f).name, laid[f], got[f]);
            printf("\n");
        }
    }
    gNar.enabled = false;
}

// narfit <seed> <level>: every megastructure tried on the first slabs of the road, with the reason it was or was not taken.
static void runNarFit(u32 seed, int level) {
    narSimResetWorld(seed, level, true, true);
    NarSimRoad road = narSimRoad(seed, level);
    lmNarSetRoad(road.startX, road.startZ, road.dx, road.dz);
    LmRoadSlab rs[4];
    int n = lmNarRoadSlabs(rs, 4);
    for (int k = 0; k < n; k++) {
        LmNarCtx c = lmNarContext(rs[k].j, rs[k].i, true);
        printf("road slab %d (%d,%d) %dx%d, road axis %d at local %d, steps %d..%d\n", k, c.j, c.i, c.w, c.h, c.roadAxis,
               c.roadAxis == 0 ? c.roadCoord - c.r.z0 - 1 : c.roadCoord - c.r.x0 - 1, rs[k].enter, rs[k].leave);
        for (int f = LN_MEGA_CLASSIC; f <= LN_ROOTFALL; f++) {
            const LmFeatInfo& fi = lmFeatInfo(f);
            LmBeat b; lmNarBeatRoad(c, b);
            b.megaForm = fi.megaForm; b.megaMode = fi.megaMode; b.roadThrough = c.roadInside;
            gLmBeats[lmSlabKey(c.j, c.i)] = b;
            const LimenSlab& t = lmNarTrial(c.j, c.i);
            printf("   %-16s tiers %2d form %-9s through %d off %d,%d reach %3d %s\n", fi.name, t.ceilTiers, lmMegaFormName(t.megaForm),
                   (int)t.roadThrough, t.megaOffX, t.megaOffZ, t.roadC >= 0 ? lmNarRoadReach(t, t.roadAxis, t.roadC) : -1, t.fallback ? "FALLBACK" : "");
            gLmBeats.erase(lmSlabKey(c.j, c.i));
        }
    }
}

static u32 narBeatsHash() {
    std::vector<long long> keys;
    for (const auto& kv : gLmBeats) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());
    u32 h = 0x811C9DC5u;
    auto mix = [&](u32 v) { h = (h ^ v) * 16777619u; };
    for (long long k : keys) {
        const LmBeat& b = gLmBeats[k];
        mix((u32)k); mix((u32)(k >> 32));
        mix((u32)(b.megaForm + 8)); mix((u32)(b.megaMode + 8)); mix((u32)(b.wing + 8)); mix(b.tiers); mix(b.force);
        for (int g = 0; g < 16; g++) mix((u32)(b.geneAdd[g] + 128));
        mix((u32)(b.roadAxis + 8)); mix((u32)b.roadCoord); mix(b.roadThrough ? 1u : 0u);
        mix(b.pocketPeak); mix((u32)b.pocketR); mix((u32)b.pocketX); mix((u32)b.pocketZ);
    }
    return h;
}

// Part of `limen_tool test`.
static int runNarratorTests(u32 seed) {
    int failed = 0;
    // 1. Script mode is a pure function of (seed, level, road): the same plan whatever the
    //    narrator remembers and however often it is asked. That is what keeps co-op peers in step.
    {
        int bad = 0, floors = 0, megas = 0;
        for (int lv = 0; lv < 8; lv++) {
            u32 h[3];
            for (int pass = 0; pass < 3; pass++) {
                gNar.mem = LmNarMemory();
                if (pass == 1) for (int f = 0; f < LN_COUNT; f++) { gNar.mem.count[f] = (unsigned)(f * 7 % 5); gNar.mem.last[f] = (unsigned)(f * 13 % 40); gNar.mem.stamp = 60; }
                narSimResetWorld(seed + (u32)lv, lv, true, false);
                NarSimRoad road = narSimRoad(seed + (u32)lv, lv);
                if (pass == 2) lmSlab(3, -2);     // and whatever happened to be built before
                lmNarSetRoad(road.startX, road.startZ, road.dx, road.dz);
                lmNarPlanRoad();
                h[pass] = narBeatsHash();
            }
            if (h[0] != h[1] || h[1] != h[2]) bad++;
            floors++;
            for (const LmNarPlaced& p : gNar.placed) if (lmFeatInfo(p.feat).cls == LNC_MEGA) { megas++; break; }
        }
        printf("narrator script mode (%d floors): plans that depend on memory or build order=%d, floors with a megastructure on the road=%d: %s\n",
               floors, bad, megas, (bad == 0 && megas >= floors - 1) ? "ok" : "FAIL");
        failed += (bad == 0 && megas >= floors - 1) ? 0 : 1;
    }
    // 2. A crew riding the rig: the narrator shows more, and what it plans is a sound world.
    {
        NarSimStats off = narRunTour(seed, 10, 30, 0, false);
        NarSimStats on = narRunTour(seed, 10, 30, 1, false);
        NarSimStats sc = narRunTour(seed, 10, 30, 2, false);
        int megaOff = narBitsOfClass(off.seenEver, LNC_MEGA), megaOn = narBitsOfClass(on.seenEver, LNC_MEGA), megaSc = narBitsOfClass(sc.seenEver, LNC_MEGA);
        bool ok = on.strandedCells == 0 && on.brokenSeams == 0 && sc.strandedCells == 0 && sc.brokenSeams == 0 &&
                  narBits(on.seenEver) >= 30 && megaOn >= 8 && megaSc >= 6 && narBits(on.seenEver) >= narBits(off.seenEver) + 8;
        printf("narrator on the rig (10 floors): seen %d of %d without it, %d with it, %d in script mode; megastructures %d / %d / %d; "
               "stranded cells %d, openings into walls %d: %s\n", narBits(off.seenEver), narCatalogueSize(), narBits(on.seenEver), narBits(sc.seenEver),
               megaOff, megaOn, megaSc, on.strandedCells + sc.strandedCells, on.brokenSeams + sc.brokenSeams, ok ? "ok" : "FAIL");
        failed += ok ? 0 : 1;
    }
    // 3. A player on foot: slabs are rewritten ahead of them, never anything they have seen.
    {
        int changed = 0, stranded = 0, broken = 0, rewrites = 0, seenOn = 0, seenOff = 0, bigOn = 0, bigOff = 0;
        static const int lv[3] = {2, 5, -7};
        for (int k = 0; k < 3; k++) {
            NarSimStats off = narRunRoam(seed + (u32)k, lv[k], 8.0f, false, false);
            NarSimStats on = narRunRoam(seed + (u32)k, lv[k], 8.0f, true, false);
            changed += on.changedSeen; stranded += on.strandedCells; broken += on.brokenSeams; rewrites += on.rewrites;
            seenOn += narBits(on.seenEver); seenOff += narBits(off.seenEver);
            bigOn += on.bigFinds; bigOff += off.bigFinds;
        }
        // 24 minutes on foot: something big at least every two minutes, and more often than by chance
        bool ok = changed == 0 && stranded == 0 && broken == 0 && rewrites >= 6 && bigOn >= 12 && bigOn > bigOff;
        printf("narrator on foot (3 levels, 8 min each): slabs rewritten unseen=%d, seen slabs that changed=%d, stranded cells=%d, "
               "openings into walls=%d, big things come upon %d -> %d, kinds seen %d -> %d: %s\n", rewrites, changed, stranded, broken,
               bigOff, bigOn, seenOff, seenOn, ok ? "ok" : "FAIL");
        failed += ok ? 0 : 1;
    }
    gNar.enabled = false; gNar.mem = LmNarMemory();
    gLmStory = LmStory();
    lmReset(seed + 1, 0); lmReset(seed, 0);
    return failed;
}
