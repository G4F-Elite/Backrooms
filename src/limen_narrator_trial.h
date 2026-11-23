#pragma once
// Part of limen_narrator.h: rewriting unseen slabs, trials, composing a plan.

inline void lmNarWitness(int feat, int j, int i) {
    LmNarrator& N = gNar;
    const LmFeatInfo& fi = lmFeatInfo(feat);
    const bool first = N.mem.count[feat] == 0;
    const bool big = fi.cls == LNC_MEGA || fi.cls == LNC_STRANGE;
    const bool freshHere = (N.levelWitnessed & lmFeatBit(feat)) == 0;
    const float age = (float)(N.mem.stamp - N.mem.last[feat]);
    N.mem.count[feat]++;
    N.mem.last[feat] = ++N.mem.stamp;
    N.mem.dirty = true;
    N.levelWitnessed |= lmFeatBit(feat);
    // the one that was found is the one in this slab: the same thing waiting elsewhere is still a spare
    const long long key = lmSlabKey(j, i);
    bool mine = false;
    for (LmNarPlaced& p : N.placed) {
        if (p.feat != feat || p.slab != key || p.witnessed || p.gone) continue;
        p.witnessed = true; mine = true;
        if (N.levelPlaced[feat] > 0) N.levelPlaced[feat]--;                // no longer waiting: it was found
    }
    N.sinceAny = 0.0f;
    if (big) {
        // How much of an event this was. A thing never seen before is one in full; a thing known from
        // other levels less so; and the fifth pocket of a level that is drunk all over is none at all:
        // it earns no breather and does not call back what was prepared ahead.
        float impact = first ? 1.0f : (freshHere ? 0.5f + 0.5f * fminf(1.0f, age / 30.0f) : (mine ? 0.5f : 0.0f));
        if (freshHere) { N.freshBig++; N.lullMax = fmaxf(N.lullMax, N.time - N.lastFresh); N.lastFresh = N.time; }
        if (impact > 0.0f) {
            // on foot the next find comes sooner: the player is covering ground, not standing at the rig
            float walk = N.speed > 1.5f ? 0.5f : 1.0f;
            bool mega = fi.cls == LNC_MEGA;
            N.sinceMajor = 0.0f;
            N.rest = fmaxf(N.rest, (mega ? 60.0f : 35.0f) / N.pace * walk * impact);
            if (mine) N.revert = true;      // what was prepared stays where it is unless this was one of it
            N.lastRoamMega = mega; N.roamMegaNext = !mega;
        }
    }
    lmNarLog("%s %s", first ? "FIRST:" : "saw", fi.name);
}

// ---------------------------------------------------------------- what to show next
inline float lmNarHash01(u32 a, u32 b, u32 c) { return (float)(lmHash(a, b, c, 0x4E4152u) & 0xFFFFu) / 65535.0f; }

// How much the player is owed this feature. Adaptive: never witnessed comes first, then whatever
// was witnessed longest ago. Script: a rotation over the level number that every peer shares.
inline float lmNarNovelty(int f) {
    const LmNarrator& N = gNar;
    float v;
    if (!N.adaptive) {
        // each class is walked in a fixed scrambled order, five places further every level
        int n = 0, rank = 0;
        u32 mine = lmLvlHash(0x4C4D4Eu, 0, (u32)f + 77u);
        for (int k = 0; k < LN_QUIRK_FIRST; k++) {
            if (lmFeatInfo(k).cls != lmFeatInfo(f).cls) continue;
            n++;
            u32 hk = lmLvlHash(0x4C4D4Eu, 0, (u32)k + 77u);
            if (hk < mine || (hk == mine && k < f)) rank++;
        }
        int d = lmMod(rank - N.level * 5, n < 1 ? 1 : n);
        v = 5.0f * powf(0.62f, (float)d);
    } else if (N.mem.count[f] == 0) v = 5.0f;
    else {
        float age = (float)(N.mem.stamp - N.mem.last[f]);
        v = 0.3f + 1.7f * fminf(1.0f, age / 30.0f);
    }
    if (N.levelPlaced[f]) v *= 0.08f;                       // one of these is already waiting on this level
    else if (N.levelWitnessed & lmFeatBit(f)) v *= 0.2f;
    return v;
}

// The road as a slab sees it. Returns false when the road neither crosses the slab nor passes
// within `reach` cells of it. `inside`: it runs through the slab's interior.
inline bool lmNarRoadAt(const LimenRect& r, int reach, int& axis, int& coord, bool& inside) {
    const LmNarrator& N = gNar;
    if (!N.hasRoad) return false;
    axis = N.roadDX != 0 ? 0 : 1;
    coord = axis == 0 ? N.roadZ : N.roadX;
    int c0 = axis == 0 ? r.z0 : r.x0, c1 = axis == 0 ? r.z1 : r.x1;
    int a0 = axis == 0 ? r.x0 : r.z0, a1 = axis == 0 ? r.x1 : r.z1;
    int start = axis == 0 ? N.roadX : N.roadZ, dir = axis == 0 ? N.roadDX : N.roadDZ;
    if (dir > 0 ? (a1 <= start - 1) : (a0 > start + 1)) return false;     // wholly behind the start
    inside = coord > c0 && coord < c1;
    if (inside) return true;
    int d = coord <= c0 ? c0 - coord : coord - c1 + 1;
    return d <= reach;
}

// Is this slab final? Seen, or holding something of the game's.
inline bool lmNarFixed(int j, int i) {
    if (lmNarSlabSeen(j, i)) return true;
    LimenRect r = lmSlabRect(j, i);
    return gNar.busy && gNar.busy(r.x0, r.z0, r.x1, r.z1);
}

inline int lmNarNeighbours(int j, int i, long long* out, int cap);

// May this slab be rewritten? Inside its own walls: whenever it is not final. `structural`
// (vault or not, height, megaform) also shows outside it, in two ways. The wall towards a
// neighbour turns into a colonnade or back only between two vaults, so no final neighbour may
// be a vault. And a one-storey neighbour dims its lamps towards a vault next door (haloDir):
// final neighbours get that pinned to what it is now (see lmNarSetBeat), and never notice.
inline bool lmNarFree(int j, int i, bool structural) {
    if (lmNarFixed(j, i)) return false;
    if (!structural) return true;
    long long nb[16];
    int n = lmNarNeighbours(j, i, nb, 16);
    for (int k = 0; k < n; k++) {
        int nj = (int)(nb[k] >> 32), ni = (int)(nb[k] & 0xFFFFFFFFll);
        if (!lmNarFixed(nj, ni)) continue;
        if (lmSlabTiers(nj, ni) > 1) return false;
        // a final one-storey neighbour keeps its seam only while both sides agree on being vaults or not:
        // a colonnade seam (through-edge) opens or closes with the vault-ness of this slab
        if (lmGene(j, i, LG_EDGE) + lmGene(nj, ni, LG_EDGE) > gLimenDNA.throughEdgeThresh) return false;
    }
    return true;
}

// Slabs that share a wall line with (j, i): two in its own row, and every slab of the rows
// above and below that overlaps it in x.
inline int lmNarNeighbours(int j, int i, long long* out, int cap) {
    int n = 0;
    auto push = [&](int jj, int ii) {
        long long k = lmSlabKey(jj, ii);
        for (int q = 0; q < n; q++) if (out[q] == k) return;
        if (n < cap) out[n++] = k;
    };
    push(j, i - 1); push(j, i + 1);
    LimenRect me = lmSlabRect(j, i);
    for (int dj = -1; dj <= 1; dj += 2) {
        int iLo = lmColOf(j + dj, me.x0), iHi = lmColOf(j + dj, me.x1 - 1);
        for (int ii = iLo - 1; ii <= iHi + 1; ii++) push(j + dj, ii);
    }
    return n;
}

inline void lmNarDropSlab(long long key) {
    gLmStorySlabEpoch[key]++;
    if (gLimenSlabs.erase(key)) { gLimenSlabUse.erase(key); gLimenLastSlab = nullptr; }
    auto it = gNar.slabs.find(key);
    if (it != gNar.slabs.end()) { it->second.spotsValid = false; it->second.notes.clear(); }
    for (long long k : gNar.dirty) if (k == key) return;
    gNar.dirty.push_back(key);
}

// A trial build may have built neighbouring slabs (their seams, lamps and tags are read through
// the cache) with the trial's beat in place. Whatever it left behind is thrown away, so the next
// build of the slab and its neighbours is the true one.
inline void lmNarDropAround(int j, int i) {
    long long nb[16];
    int n = lmNarNeighbours(j, i, nb, 16);
    long long key = lmSlabKey(j, i);
    gLimenSlabs.erase(key); gLimenSlabUse.erase(key);
    // (a neighbour that is final keeps the build it was seen with)
    for (int k = 0; k < n; k++) {
        int nj = (int)(nb[k] >> 32), ni = (int)(nb[k] & 0xFFFFFFFFll);
        if (lmNarFixed(nj, ni)) continue;
        gLimenSlabs.erase(nb[k]); gLimenSlabUse.erase(nb[k]);
    }
    gLimenLastSlab = nullptr;
    lmBeatsRebuildPockets();
}
// A trial is over: its beat goes and what it built goes with it.
inline void lmNarEndTrial(int j, int i) {
    gLmBeats.erase(lmSlabKey(j, i));
    lmNarDropAround(j, i);
}

// Writes (or removes) the beat of a slab and throws away everything built from the old one.
// A structural change also reaches the neighbours: those that are final keep what they derive
// from this slab pinned, the others are rebuilt.
inline void lmNarSetBeat(int j, int i, const LmBeat* beat, bool structural) {
    long long key = lmSlabKey(j, i);
    if (structural && gNar.adaptive) {
        static const int hj[4] = {0, 0, 1, -1}, hi[4] = {1, -1, 0, 0};   // the four slabs whose haloDir reads this one
        for (int k = 0; k < 4; k++) {
            int bj = j + hj[k], bi = i + hi[k];
            if (!lmNarFixed(bj, bi)) continue;
            const LmBeat* have = lmBeatAt(bj, bi);
            if (have && have->haloPin != -2) continue;
            LmBeat pin = have ? *have : LmBeat();
            pin.haloPin = (signed char)lmSlabHaloDir(bj, bi);
            gLmBeats[lmSlabKey(bj, bi)] = pin;
        }
    }
    if (beat) {
        LmBeat b = *beat;
        auto old = gLmBeats.find(key);
        if (old != gLmBeats.end() && b.haloPin == -2) b.haloPin = old->second.haloPin;
        gLmBeats[key] = b;
    } else {
        // (a pin outlives the beat it rode on)
        auto old = gLmBeats.find(key);
        if (old != gLmBeats.end() && old->second.haloPin != -2) { LmBeat pin; pin.haloPin = old->second.haloPin; old->second = pin; }
        else gLmBeats.erase(key);
    }
    lmBeatsRebuildPockets();
    lmNarDropSlab(key);
    if (structural) {
        long long nb[16];
        int n = lmNarNeighbours(j, i, nb, 16);
        for (int k = 0; k < n; k++) {
            int nj = (int)(nb[k] >> 32), ni = (int)(nb[k] & 0xFFFFFFFFll);
            if (gNar.adaptive && lmNarFixed(nj, ni)) continue;
            lmNarDropSlab(nb[k]);
        }
    }
}

// Builds the slab as it would stand with the beats now in the table (nothing is cached).
inline const LimenSlab& lmNarTrial(int j, int i) {
    static LimenSlab tmp;
    lmBeatsRebuildPockets();
    lmBuildSlab(tmp, j, i);
    lmPlaceLampSites(tmp);
    lmApplyStory(tmp);          // exactly as lmSlab() builds it: the rig's lane and stamped sets come last
    return tmp;
}

// Every cell the seams promise to open (on any of the four sides) is still a floor one can walk on.
inline bool lmNarOpeningsHold(const LimenSlab& t) {
    static LimenPort ports[LM_MAX_OPENINGS * 4];
    int perSide[4] = {};
    int n = lmCollectOpenings(t.rect, ports, perSide);
    for (int k = 0; k < n; k++) {
        if (!lmIsWalkableTag(t.tag[ports[k].lx][ports[k].lz])) return false;
    }
    return true;
}

// A megastructure the rig drives through still has to be a place to work in: enough floor at
// y = 0 that can be walked to from the road, within looting distance of it.
inline int lmNarRoadReach(const LimenSlab& s, int axis, int c) {
    static unsigned char mark[LM_MAX][LM_MAX];
    static short qx[LM_MAX * LM_MAX], qz[LM_MAX * LM_MAX];
    memset(mark, 0, sizeof(mark));
    int L = axis == 0 ? s.w : s.h, head = 0, tail = 0, reach = 0;
    for (int a = 0; a < L; a++) {
        int x = axis == 0 ? a : c, z = axis == 0 ? c : a;
        mark[x][z] = 1; qx[tail] = (short)x; qz[tail] = (short)z; tail++;
    }
    static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
    while (head < tail) {
        int x = qx[head], z = qz[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h || mark[nx][nz]) continue;
            int across = (axis == 0 ? nz : nx) - c;
            if (across < -7 || across > 7 || !lmIsWalkableTag(s.tag[nx][nz])) continue;
            mark[nx][nz] = 1; qx[tail] = (short)nx; qz[tail] = (short)nz; tail++; reach++;
        }
    }
    return reach;
}

struct LmNarCtx {
    int j, i;
    LimenRect r; int w, h;
    bool road, roadInside; int roadAxis, roadCoord;   // road near / through the slab
    bool structural;                                  // megaform and height may change
    bool naturalVault;
    int wingNatural;
    int haloPin;                                      // carried over from the slab's old beat (see LmBeat)
    int baseForm = LB_FORM_KEEP;                      // LB_FORM_FLAT: the slab was flattened beside a megastructure and stays flat
    short entryX, entryZ;                             // slab-local doorway the player approaches by (-1: none)
};

inline void lmNarRemember(const LmNarCtx& c, int feat, bool major) {
    gNar.placed.push_back({lmSlabKey(c.j, c.i), (unsigned char)feat, major, false, gNar.time});
    gNar.levelPlaced[feat]++;
    if (major) gNar.placedMajors++;
    if (feat == gNar.wish) gNar.wish = -1;      // asked for once, shown once
}

inline void lmNarBeatRoad(const LmNarCtx& c, LmBeat& b) {
    if (c.road) { b.roadAxis = (signed char)c.roadAxis; b.roadCoord = c.roadCoord; }
}

// An ordinary storey is one of five kinds of space; the one the player has walked least takes
// over when the slab's own kind is stale.
// The skylight well is a concourse's own piece: a concourse is owed whenever its well is (not
// under an open sky, where it has no roof to rise through).
inline void lmNarPickWing(const LmNarCtx& c, LmBeat& b) {
    const bool well = gNar.adaptive && !lmHasQuirk(gLimenDNA, LQ_SKY) && c.w >= 12 && c.h >= 12;
    auto owed = [&](int w) {
        float v = lmNarNovelty(LN_WING_PARTITIONS + w);
        if (well && w == LM_WING_CONCOURSE) v = fmaxf(v, lmNarNovelty(LN_SKYWELL));
        return v;
    };
    float mine = owed(c.wingNatural);
    int best = -1; float bestS = mine * 1.25f;
    for (int w = 0; w < LM_WING_COUNT; w++) {
        float s = owed(w) * (1.0f + 0.15f * lmNarHash01((u32)c.j, (u32)c.i, (u32)w + 40u));
        if (w != c.wingNatural && s > bestS) { bestS = s; best = w; }
    }
    if (best >= 0) { b.wing = (signed char)best; gNar.levelPlaced[LN_WING_PARTITIONS + best]++; }
    if (well && (best >= 0 ? best : c.wingNatural) == LM_WING_CONCOURSE && gNar.levelPlaced[LN_SKYWELL] == 0) {
        // did a well come out? Then one is waiting, and the next concourse is not asked for on its account
        gLmBeats[lmSlabKey(c.j, c.i)] = b;
        std::vector<LmSpot> spots;
        lmNarBuildSpots(lmNarTrial(c.j, c.i), spots);
        lmNarEndTrial(c.j, c.i);
        if (lmNarSpotMask(spots) & lmFeatBit(LN_SKYWELL)) lmNarRemember(c, LN_SKYWELL, false);
    }
}

// Orders the features of the given classes by how much the player is owed them here.
inline int lmNarRank(const LmNarCtx& c, unsigned classMask, int* out, int lastMajorCls) {
    struct Cand { int f; float score; };
    Cand cand[LN_COUNT]; int n = 0;
    for (int f = 0; f < LN_QUIRK_FIRST; f++) {
        const LmFeatInfo& fi = lmFeatInfo(f);
        if (((classMask >> fi.cls) & 1u) == 0u) continue;
        float sc = lmNarNovelty(f);
        if (gNar.adaptive) {
            // the level's own leanings and a little chance break ties between equally owed things
            if (fi.megaForm >= 0) sc *= 0.75f + 0.5f * (float)gLimenDNA.megaFormWeight[fi.megaForm] / 255.0f;
            sc *= 0.85f + 0.3f * lmNarHash01((u32)c.j * 31u + (u32)f, (u32)c.i, (u32)gNar.level);
            // forms that fit few roads go first wherever they do fit
            if (c.roadInside) {
                if (f == LN_GROTTO) sc *= 1.6f;
                else if (f == LN_SILO || f == LN_CELLBLOCK) sc *= 1.4f;
                else if (f == LN_MONOLITH || f == LN_PLATFORM) sc *= 1.2f;
            }
            // the plain vault is where the dry basin and the chasm bridge are: owed as much as they are
            if (f == LN_MEGA_CLASSIC) sc = fmaxf(sc, 0.9f * fmaxf(lmNarNovelty(LN_BASIN_WALK), lmNarNovelty(LN_CHASM)));
        }
        if (fi.cls == lastMajorCls) sc *= 0.7f;
        if (f == gNar.wish) sc += 1000.0f;
        cand[n++] = {f, sc};
    }
    std::sort(cand, cand + n, [](const Cand& a, const Cand& b) { return a.score > b.score || (a.score == b.score && a.f < b.f); });
    for (int k = 0; k < n; k++) out[k] = cand[k].f;
    return n;
}

// Adds up to `want` quiet anomalies to a beat under construction, checking by a trial build
// that each one really came out (the spot exists); those that did not are taken back.
inline int lmNarAddMinors(const LmNarCtx& c, LmBeat& b, int want, bool vault) {
    int order[LN_COUNT];
    int n = lmNarRank(c, 1u << LNC_QUIET, order, -1);
    long long key = lmSlabKey(c.j, c.i);
    int got = 0, tries = 0;
    bool furn = false, arch = false;
    for (int k = 0; k < n && got < want && tries < 6; k++) {
        int f = order[k];
        const LmFeatInfo& fi = lmFeatInfo(f);
        bool isFurn = f >= LN_MONO;
        if (vault && (isFurn || f == LN_PLUG || f == LN_ECHO)) continue;       // these need rooms and corridors
        if (isFurn ? furn : (arch && want > 1)) continue;                    // one of each kind
        int wing = b.wing >= 0 ? b.wing : c.wingNatural;
        LmBeat trial = b;
        if (!vault && fi.wings && ((fi.wings >> wing) & 1u) == 0u) {
            if (b.wing >= 0 || (b.force & ~(unsigned)LBF_DARK) != 0u) continue;   // the wing is already spoken for
            // move the slab to the wing this needs that the player has seen least
            int bestW = -1; float bestS = -1.0f;
            for (int w = 0; w < LM_WING_COUNT; w++) {
                if (((fi.wings >> w) & 1u) == 0u) continue;
                float s = lmNarNovelty(LN_WING_PARTITIONS + w) + 0.2f * lmNarHash01((u32)c.j, (u32)c.i, (u32)w);
                if (s > bestS) { bestS = s; bestW = w; }
            }
            if (bestW < 0) continue;
            trial.wing = (signed char)bestW;
        }
        trial.force |= fi.force;
        if (f == LN_TWIN) { trial.geneAdd[LG_ORDER] = 80; trial.geneAdd[LG_CELLS] = 50; }
        else if (f == LN_MONO || f == LN_LACK || f == LN_FACEWALL) { trial.geneAdd[LG_CELLS] = 50; trial.geneAdd[LG_USE] = 40; }
        else if (f == LN_PLUG || f == LN_ECHO) trial.geneAdd[LG_ORDER] = 50;
        tries++;
        gLmBeats[key] = trial;
        const LimenSlab& t = lmNarTrial(c.j, c.i);
        bool ok = !t.fallback;
        if (ok && !isFurn) {
            std::vector<LmSpot> spots;
            lmNarBuildSpots(t, spots);
            ok = (lmNarSpotMask(spots) & lmFeatBit(f)) != 0;
            if (ok && c.road) {
                // and it has to be where the road shows it
                ok = false;
                for (const LmSpot& q : spots) {
                    if (q.feat != f) continue;
                    int across = (c.roadAxis == 0 ? t.rect.z0 + 1 + q.lz : t.rect.x0 + 1 + q.lx) - c.roadCoord;
                    if (across >= -7 && across <= 7) ok = true;
                }
            }
        } else if (ok) {
            int rooms = 0, halls = 0;
            for (int x = 0; x < t.w; x++) for (int z = 0; z < t.h; z++) {
                if (c.road) { int across = (c.roadAxis == 0 ? t.rect.z0 + 1 + z : t.rect.x0 + 1 + x) - c.roadCoord; if (across < -7 || across > 7) continue; }
                if (t.tag[x][z] == LT_ROOM) rooms++;
                if (t.tag[x][z] == LT_HALL) halls++;
            }
            ok = (f == LN_WAITVOID || f == LN_PROCESSION) ? halls >= 12 : rooms >= 6;
        }
        if (!ok) continue;
        b = trial;
        got++;
        if (isFurn) furn = true; else arch = true;
        lmNarRemember(c, f, false);
    }
    lmNarEndTrial(c.j, c.i);
    return got;
}

// Composes the beat for one slab in the given role. Returns the role that was really filled.
inline int lmNarCompose(const LmNarCtx& c, int role, LmBeat& out, int lastMajorCls) {
    LmNarrator& N = gNar;
    long long key = lmSlabKey(c.j, c.i);
    int order[LN_COUNT];
    out = LmBeat();
    out.haloPin = (signed char)c.haloPin;
    out.megaForm = (signed char)c.baseForm;
    lmNarBeatRoad(c, out);
    out.entryX = c.entryX; out.entryZ = c.entryZ;

    if (role == LNR_MEGA && c.structural) {
        int n = lmNarRank(c, 1u << LNC_MEGA, order, lastMajorCls);
        for (int k = 0; k < n; k++) {
            const LmFeatInfo& fi = lmFeatInfo(order[k]);
            // the round drum of a silo cuts through the cells beside the lane where the kiosk is set up
            if (order[k] == LN_SILO && c.roadInside && ((N.kioskSlabs > 0 && N.kioskSlab[0] == key) || (N.kioskSlabs > 1 && N.kioskSlab[1] == key))) continue;
            LmBeat b = out;
            b.megaForm = fi.megaForm; b.megaMode = fi.megaMode;
            b.roadThrough = c.roadInside;
            if (order[k] == LN_MEGA_CLASSIC) {
                // the plain vault comes with a dry basin to walk down into or with a chasm to cross
                b.geneAdd[LG_SINK] = 90;
                b.geneAdd[LG_ABYSS] = (signed char)(lmNarNovelty(LN_BASIN_WALK) >= lmNarNovelty(LN_CHASM) ? -110 : 110);
            }
            gLmBeats[key] = b;
            bool ok = true;
            unsigned long long riders = 0;       // what else came out with it
            if (c.roadInside || order[k] == LN_MEGA_CLASSIC) {
                const LimenSlab& t = lmNarTrial(c.j, c.i);
                if (c.roadInside)
                    ok = !t.fallback && t.megaForm == fi.megaForm && t.roadThrough && t.roadC >= 0 &&
                         lmFeatOfMega(t.megaForm, t.megaSub) == order[k] && lmNarRoadReach(t, t.roadAxis, t.roadC) >= 36;
                if (ok && order[k] == LN_MEGA_CLASSIC) {
                    std::vector<LmSpot> spots;
                    lmNarBuildSpots(t, spots);
                    riders = lmNarSpotMask(spots) & (lmFeatBit(LN_BASIN_WALK) | lmFeatBit(LN_CHASM));
                }
            }
            lmNarEndTrial(c.j, c.i);
            if (!ok) continue;
            out = b;
            lmNarRemember(c, order[k], true);
            for (int f = LN_BASIN_WALK; f <= LN_CHASM; f++) if (riders & lmFeatBit(f)) lmNarRemember(c, f, false);
            return LNR_MEGA;
        }
        role = LNR_STRANGE;
    } else if (role == LNR_MEGA) role = LNR_STRANGE;

    if (role == LNR_STRANGE) {
        int n = lmNarRank(c, 1u << LNC_STRANGE, order, lastMajorCls);
        for (int k = 0; k < n; k++) {
            int f = order[k];
            LmBeat b = out;
            bool vault = c.naturalVault;
            if (f == LN_ESCHER) {
                // a knot needs a tall vault of its own and no road under it
                if ((c.roadInside && !N.planEntry) || !c.structural) continue;
                int form = c.naturalVault ? lmSlabMegaForm(c.j, c.i) : -1;
                bool host = c.naturalVault && lmSlabTiers(c.j, c.i) >= 4 && form != MF_SILO && form != MF_HANGING && form != MF_ZIGGURAT && !lmIsMegaUnstruct(form);
                if (!host) {
                    // none stands here: a plain one is raised for it (on foot only; the rig's lane keeps its own megastructures)
                    if (!N.adaptive || !N.planEntry || c.baseForm == LB_FORM_FLAT || c.w < 20 || c.h < 20) continue;
                    b.megaForm = MF_CLASSIC; b.megaMode = -1;
                    vault = true;
                }
                b.force |= LBF_ESCHER;
            } else {
                if (vault) {
                    if (f != LN_DARK && !c.structural) continue;          // the rest wants an ordinary storey
                    if (f != LN_DARK) { b.megaForm = LB_FORM_FLAT; vault = false; }
                }
                if (f == LN_DRUNK) {
                    int R = (c.w < c.h ? c.w : c.h) / 2 - 1;
                    if (R < 6) continue;
                    if (R > 13) R = 13;
                    int cx = c.r.x0 + 1 + c.w / 2, cz = c.r.z0 + 1 + c.h / 2;
                    if (c.road) {
                        // the pocket sits astride the road
                        int lo = (c.roadAxis == 0 ? c.r.z0 : c.r.x0) + 1 + R / 2, hi = (c.roadAxis == 0 ? c.r.z1 : c.r.x1) - 1 - R / 2;
                        int v = lmClampInt(c.roadCoord, lo, hi > lo ? hi : lo);
                        if (c.roadAxis == 0) cz = v; else cx = v;
                    }
                    b.pocketX = cx; b.pocketZ = cz; b.pocketR = (short)R;
                    b.pocketPeak = (unsigned char)(205 + (int)(lmNarHash01((u32)c.j, (u32)c.i, 5u) * 45.0f));
                } else {
                    if ((f == LN_NESTED || f == LN_SPIRAL) && (c.w < 16 || c.h < 16)) continue;
                    if (f == LN_XENO && (c.w < 13 || c.h < 13)) continue;
                    b.force |= lmFeatInfo(f).force;
                }
            }
            gLmBeats[key] = b;
            if (b.pocketPeak) lmBeatsRebuildPockets();
            const LimenSlab& t = lmNarTrial(c.j, c.i);
            std::vector<LmSpot> spots;
            lmNarBuildSpots(t, spots);
            bool ok = !t.fallback && (lmNarSpotMask(spots) & lmFeatBit(f)) != 0;
            lmNarEndTrial(c.j, c.i);
            if (!ok) continue;
            out = b;
            lmNarRemember(c, f, true);
            if (!vault && f != LN_NESTED && f != LN_SPIRAL && f != LN_XENO) lmNarPickWing(c, out);
            if (f != LN_ESCHER) lmNarAddMinors(c, out, 1, vault);
            return LNR_STRANGE;
        }
        role = LNR_MINOR;
    }

    if (role == LNR_MINOR) {
        if (!c.naturalVault) lmNarPickWing(c, out);
        if (lmNarAddMinors(c, out, 2, c.naturalVault) > 0 || out.wing >= 0) return LNR_MINOR;
    }
    (void)N;
    return LNR_NONE;
}
