#pragma once
// Part of limen_narrator.h: decisions, the drill road, roaming, the tick.

inline LmNarCtx lmNarContext(int j, int i, bool structural) {
    LmNarCtx c;
    c.j = j; c.i = i;
    c.r = lmSlabRect(j, i);
    c.w = c.r.x1 - c.r.x0 - 1; c.h = c.r.z1 - c.r.z0 - 1;
    c.roadAxis = -1; c.roadCoord = 0; c.roadInside = false;
    c.road = lmNarRoadAt(c.r, 9, c.roadAxis, c.roadCoord, c.roadInside);
    c.structural = structural;
    c.naturalVault = lmSlabTiers(j, i) > 1;
    c.wingNatural = lmWingAt(j, i);
    c.haloPin = -2;
    c.entryX = c.entryZ = -1;
    if (gNar.planEntry && !c.road) {
        c.entryX = (short)lmClampInt(gNar.planPX - c.r.x0 - 1, 0, c.w - 1);
        c.entryZ = (short)lmClampInt(gNar.planPZ - c.r.z0 - 1, 0, c.h - 1);
    }
    return c;
}

// Decides one slab. `structural` as in lmNarFree. Neighbouring natural vaults of a new
// megastructure are flattened so that it stands alone (only when they may still change).
inline int lmNarDecide(int j, int i, int role, bool structural, int lastMajorCls) {
    LmNarSlab& rec = lmNarRec(j, i);
    rec.decided = true;             // until something is laid here nothing is to be retried (see below)
    if (role == LNR_NONE) return LNR_NONE;
    long long key = lmSlabKey(j, i);
    int pin = -2;
    LmBeat old; bool hadOld = false;
    { const LmBeat* o = lmBeatAt(j, i); if (o) { pin = o->haloPin; old = *o; hadOld = true; } }
    size_t before = gNar.placed.size();
    // A natural vault that was flattened to let a megastructure next door stand alone stays flat
    // under whatever is laid here now. Dropping that pin with the old beat let the vault grow
    // back without any neighbour being told: their seams, cells and meshes kept the flat slab,
    // and the vault's hall and bridges hung in mid-air against them.
    const int oldForm = hadOld ? (int)old.megaForm : LB_FORM_KEEP;
    const bool oldKnot = hadOld && (old.force & LBF_ESCHER) != 0u;
    if (oldForm == LB_FORM_FLAT) { LmBeat base; base.megaForm = LB_FORM_FLAT; base.haloPin = (signed char)pin; gLmBeats[key] = base; }
    else gLmBeats.erase(key);
    LmNarCtx c = lmNarContext(j, i, structural);
    c.haloPin = pin;
    c.baseForm = oldForm == LB_FORM_FLAT ? LB_FORM_FLAT : LB_FORM_KEEP;
    LmBeat b;
    int got = lmNarCompose(c, role, b, lastMajorCls);
    // the seam contract: whatever this slab opens onto must still be walkable once it is built
    if (got != LNR_NONE) {
        gLmBeats[key] = b;
        if (!lmNarOpeningsHold(lmNarTrial(j, i))) {
            // taken back: nothing of it was ever shown
            for (size_t k = before; k < gNar.placed.size(); k++)
                if (gNar.levelPlaced[gNar.placed[k].feat] > 0) gNar.levelPlaced[gNar.placed[k].feat]--;
            gNar.placed.resize(before < gNar.placed.size() ? before : gNar.placed.size());
            got = LNR_NONE;
        }
    }
    // The table goes back to what the world stands built from. When nothing fitted that is the
    // end of it; otherwise lmNarSetBeat below has to read the neighbours' pins (haloPin) off the
    // old world: with the trial's beat still in the table a seen neighbour was pinned to the
    // halo of the megastructure it had never been built with, and changed on its next rebuild.
    if (hadOld) gLmBeats[key] = old;
    else gLmBeats.erase(key);
    if (got == LNR_NONE) {
        lmNarDropAround(j, i);
        lmNarDropSlab(key);
        return got;
    }
    // quiet things this slab held before are superseded by the new beat (none of them was seen)
    for (size_t k = 0; k < before && k < gNar.placed.size(); k++) {
        LmNarPlaced& p = gNar.placed[k];
        if (p.slab != key || p.major || p.witnessed || p.gone) continue;
        p.gone = true;
        if (gNar.levelPlaced[p.feat] > 0) gNar.levelPlaced[p.feat]--;
    }
    // a quiet outcome leaves the slab open: a big thing may still be laid here while it is unseen
    rec.decided = (got != LNR_MINOR);
    rec.minored = true;
    // (shape is what shows outside the slab: compared with what stood here, not with the bare genes)
    bool changesShape = (int)b.megaForm != oldForm || ((b.force & LBF_ESCHER) != 0u) != oldKnot;
    lmNarSetBeat(j, i, &b, changesShape);
    if (b.megaForm >= 0) {
        long long nb[16];
        int n = lmNarNeighbours(j, i, nb, 16);
        for (int k = 0; k < n; k++) {
            int nj = (int)(nb[k] >> 32), ni = (int)(nb[k] & 0xFFFFFFFFll);
            if (lmSlabTiers(nj, ni) <= 1) continue;
            const LmBeat* have = lmBeatAt(nj, ni);
            if (have && have->megaForm >= 0) continue;
            if (gNar.adaptive && !lmNarFree(nj, ni, true)) continue;
            LmBeat flat = have ? *have : LmBeat();
            flat.megaForm = LB_FORM_FLAT;
            lmNarSetBeat(nj, ni, &flat, true);
        }
    }
    return got;
}

// ---------------------------------------------------------------- the road
struct LmRoadSlab { int j, i; int enter, leave; };    // steps along the road (cells from its start)

inline int lmNarRoadSlabs(LmRoadSlab* out, int cap, int maxSteps = 720) {
    const LmNarrator& N = gNar;
    int n = 0;
    if (!N.hasRoad) return 0;
    for (int t = 0; t < maxSteps && n <= cap; t++) {
        LimenRect r = lmSlabAt(N.roadX + N.roadDX * t, N.roadZ + N.roadDZ * t);
        if (n > 0 && out[n - 1].j == r.j && out[n - 1].i == r.i) { out[n - 1].leave = t; continue; }
        if (n == cap) break;
        out[n++] = {r.j, r.i, t, t};
    }
    return n;
}

// Lays the level out along the rig's lane. Called once, when the lane becomes known and before
// anybody has seen anything: every slab is still free.
inline void lmNarPlanRoad() {
    LmNarrator& N = gNar;
    if (!N.enabled || !N.hasRoad) return;
    LmRoadSlab rs[12];
    int n = lmNarRoadSlabs(rs, 12);
    if (n == 0) return;
    // Three ways to open a level, never the same twice running; the first floor is the tutorial
    // and keeps its arrival hall as it is.
    static const int ARC[3][3] = {{LNR_MEGA, LNR_STRANGE, LNR_MINOR}, {LNR_MINOR, LNR_MEGA, LNR_STRANGE}, {LNR_STRANGE, LNR_MEGA, LNR_MINOR}};
    int arc = lmMod(N.level, 3);
    int first = 0;
    if (N.arcPin >= 0 && N.arcPin < 3) arc = N.arcPin;
    if (N.level == 0) { first = 1; arc = 0; lmNarRec(rs[0].j, rs[0].i).decided = true; }
    else if (rs[0].leave - rs[0].enter < 9 && n > 1) first = 1;       // the rig lands at the very edge: the stage is the next slab
    for (int k = 0; k < first; k++) lmNarRec(rs[k].j, rs[k].i).decided = true;
    N.kioskSlabs = 0;
    if (N.level != 0) {
        for (int k = 0; k < n && N.kioskSlabs < 2; k++)
            if (rs[k].enter <= 10 && rs[k].leave >= 5) N.kioskSlab[N.kioskSlabs++] = lmSlabKey(rs[k].j, rs[k].i);
    }
    int lastMajor = -1, prevRole = LNR_NONE;
    for (int k = first; k < n; k++) {
        int role = ARC[arc][(k - first) % 3];
        if (role == LNR_MEGA && prevRole == LNR_MEGA) role = LNR_STRANGE;
        int got = lmNarDecide(rs[k].j, rs[k].i, role, true, lastMajor);
        if (got == LNR_MEGA) lastMajor = LNC_MEGA; else if (got == LNR_STRANGE) lastMajor = LNC_STRANGE;
        prevRole = got;
    }
    // What stands beside the road: quiet things on the side that faces it, and one louder one
    // near the start if the road itself opens quietly.
    bool loudSide = (ARC[arc][0] == LNR_MINOR || first > 0);
    int nSide = 0;
    for (int k = 0; k < n && k < 5; k++) {
        for (int t = rs[k].enter; t <= rs[k].leave; t += 4) {
            for (int side = -1; side <= 1; side += 2) {
                int wx = N.roadX + N.roadDX * t + N.roadDZ * side * 9, wz = N.roadZ + N.roadDZ * t - N.roadDX * side * 9;
                LimenRect r = lmSlabAt(wx, wz);
                LmNarSlab& rec = lmNarRec(r.j, r.i);
                if (rec.decided) continue;
                const LmBeat* have = lmBeatAt(r.j, r.i);
                if (have && have->megaForm != LB_FORM_KEEP) { rec.decided = true; continue; }   // flattened neighbour of a megastructure
                int role = (loudSide && k <= 1 && nSide == 0) ? LNR_STRANGE : LNR_MINOR;
                lmNarDecide(r.j, r.i, role, false, lastMajor);
                nSide++;
            }
        }
    }
    N.dirty.clear();    // the caller rebuilds the whole world after this
    lmNarLog("road planned: %d slabs", n);
}

// ---------------------------------------------------------------- level, tick
inline void lmNarBeginLevel() {
    LmNarrator& N = gNar;
    N.seed = gLimen.seed; N.level = gLimen.level; N.ready = true;
    N.beatsGen = gLmBeatsGen;
    N.hasRoad = false; N.hasPlayer = false;
    N.slabs.clear(); N.placed.clear(); N.dirty.clear(); N.touched.clear();
    memset(N.levelPlaced, 0, sizeof(N.levelPlaced));
    N.levelWitnessed = 0;
    N.time = 0.0f; N.sinceMajor = 0.0f; N.sinceAny = 0.0f; N.rest = 0.0f; N.nextPlan = 2.0f; N.nextSave = 20.0f;
    N.vx = N.vz = N.speed = 0.0f;
    N.lookX = N.lookZ = 1e9f; N.lookYaw = 0.0f; N.lookT = -1.0f;
    N.freshBig = 0; N.lastFresh = 0.0f; N.lullMax = 0.0f;
    N.quirkCounted = false;
    N.mem.levels++; N.mem.dirty = true;
    lmNarSave();
}
// The world was reset under the narrator (new seed or level, or the beats were wiped): start over.
inline bool lmNarSync() {
    LmNarrator& N = gNar;
    if (N.ready && N.seed == gLimen.seed && N.level == gLimen.level && N.beatsGen == gLmBeatsGen) return false;
    lmNarBeginLevel();
    return true;
}
inline void lmNarSetRoad(int x, int z, int dx, int dz) {
    LmNarrator& N = gNar;
    N.hasRoad = true; N.roadX = x; N.roadZ = z; N.roadDX = dx; N.roadDZ = dz;
}
inline bool lmNarRoadIs(int x, int z, int dx, int dz) {
    const LmNarrator& N = gNar;
    return N.hasRoad && N.roadX == x && N.roadZ == z && N.roadDX == dx && N.roadDZ == dz;
}

// The door the player will most likely come through: the opening in the slab's wall on the side
// that faces them, nearest to where they are heading. Returns its across-coordinate (world
// cells) on the given axis, or `fallback` when that side has no opening in reach.
inline int lmNarFacingDoor(const LimenRect& r, int axis, int dir, int want, int fallback) {
    int best = fallback, bestD = 1 << 30;
    if (axis == 0) {
        int wx = dir > 0 ? r.x0 : r.x1;                    // west wall is the slab's own, east wall its neighbour's
        LimenRect owner = dir > 0 ? r : lmSlabRect(r.j, r.i + 1);
        for (int wz = r.z0 + 1; wz < r.z1; wz++) {
            if (!lmWallCellOpen(owner, wx, wz)) continue;
            int d = abs(wz - want);
            if (d < bestD) { bestD = d; best = wz; }
        }
    } else {
        int wz = dir > 0 ? r.z0 : r.z1;
        for (int wx = r.x0 + 1; wx < r.x1; wx++) {
            LimenRect owner = dir > 0 ? r : lmSlabAt(wx, wz);
            if (wx == owner.x0 || !lmWallCellOpen(owner, wx, wz)) continue;
            int d = abs(wx - want);
            if (d < bestD) { bestD = d; best = wx; }
        }
    }
    return best;
}

// Takes back something big that was prepared in a slab the player never saw (another one was
// found first): the slab returns to what the genes make of it and may be used again later.
inline bool lmNarRevert(long long key) {
    LmNarrator& N = gNar;
    int j = (int)(key >> 32), i = (int)(key & 0xFFFFFFFFll);
    if (lmNarSlabSeen(j, i)) return false;
    const LmBeat* bt = lmBeatAt(j, i);
    if (!bt) return false;
    bool shape = bt->megaForm != LB_FORM_KEEP || (bt->force & LBF_ESCHER) != 0u;
    if (!lmNarFree(j, i, shape)) return false;
    for (LmNarPlaced& p : N.placed) {
        if (p.slab != key || p.witnessed || p.gone) continue;
        p.gone = true;
        if (N.levelPlaced[p.feat] > 0) N.levelPlaced[p.feat]--;
    }
    lmNarSetBeat(j, i, nullptr, shape);
    lmNarRec(j, i).decided = false;
    lmNarRec(j, i).minored = false;
    return true;
}

// Off the road (or with no road at all) nobody knows which way the player will turn, and
// nobody has to: what was not seen is not fixed. The narrator keeps every unseen slab on the
// edge of what the player has seen, on the side they are heading, prepared. When something big
// is due, each of those slabs holds one (laid out around the door the player would come
// through), so whichever way the player goes, it is there. The moment one is found, the others
// are taken back, the player gets a breather, and for that while the edge is prepared with quiet
// anomalies only.
inline void lmNarPlanRoam(float cs) {
    LmNarrator& N = gNar;
    if (!N.hasPlayer) return;
    int pcx = (int)floorf(N.px / cs), pcz = (int)floorf(N.pz / cs);
    LimenRect here = lmSlabAt(pcx, pcz);
    float hx = sinf(N.yaw), hz = cosf(N.yaw);
    if (N.speed > 0.6f) { float l = sqrtf(N.vx * N.vx + N.vz * N.vz); if (l > 1e-3f) { hx = N.vx / l; hz = N.vz / l; } }
    bool sky = lmHasQuirk(gLimenDNA, LQ_SKY);

    // 0. something big was found: the spares go back
    if (N.revert) {
        N.revert = false;
        std::vector<long long> spare;
        for (const LmNarPlaced& p : N.placed) {
            if (!p.major || p.witnessed || p.gone || !p.roam) continue;
            bool dup = false;
            for (long long k : spare) if (k == p.slab) dup = true;
            if (!dup) spare.push_back(p.slab);
        }
        int n = 0;
        for (long long k : spare) if (lmNarRevert(k)) n++;
        if (n > 0) lmNarLog("%d spare%s taken back", n, n == 1 ? "" : "s");
    }

    // A big thing owed: every unseen slab ahead may take one (the first found settles it).
    // Otherwise only slabs with nothing yet get a quiet thing, and those stay open to a big one.
    // Spares are only worth their rebuild where the player will walk next: a few, and the nearest.
    // A spare the player has walked away from is no use where it lies and keeps the thing it holds
    // from being offered again: it goes back (one a turn), or, if its slab has been seen meanwhile,
    // simply stays there as part of the world.
    int spares = 0;
    {
        LmNarPlaced* stale = nullptr; float staleD = 0.0f;
        for (LmNarPlaced& p : N.placed) {
            if (!p.major || !p.roam || p.witnessed || p.gone) continue;
            LimenRect r = lmSlabRect((int)(p.slab >> 32), (int)(p.slab & 0xFFFFFFFFll));
            float dx = (float)(lmClampInt(pcx, r.x0, r.x1 - 1) - pcx), dz = (float)(lmClampInt(pcz, r.z0, r.z1 - 1) - pcz);
            float d = sqrtf(dx * dx + dz * dz);
            float mx = (float)(r.x0 + r.x1) * 0.5f - (float)pcx, mz = (float)(r.z0 + r.z1) * 0.5f - (float)pcz;
            float ml = sqrtf(mx * mx + mz * mz);
            float al = ml > 1.0f ? (mx * hx + mz * hz) / ml : 1.0f;
            bool away = (d > 26.0f || (d > 9.0f && al < -0.2f));
            // (a glance over the shoulder is not a change of mind: only one left behind for a while)
            if (!away) p.staleT = -1.0f; else if (p.staleT < 0.0f) p.staleT = N.time;
            if (!away || N.time - p.staleT < 12.0f) { spares++; continue; }
            if (!stale || d > staleD) { stale = &p; staleD = d; }
        }
        if (stale) {
            long long key = stale->slab;
            if (lmNarRevert(key)) lmNarLog("a spare left behind taken back");
            else for (LmNarPlaced& p : N.placed) if (p.slab == key) p.roam = false;
        }
    }
    const int keepSpares = 4;
    bool due = N.urge || (N.rest <= 0.0f && spares < keepSpares);
    // (quiet things only where the player is about to walk: ~65 m, not across the level)
    const float reachNow = due ? 20.0f : 13.0f;

    struct Cand { int j, i; float dist, cost; bool freeStruct, natural; };
    Cand cand[64]; int n = 0;
    for (int gz = -3; gz <= 3; gz++) {
        for (int gx = -3; gx <= 3; gx++) {
            LimenRect r = lmSlabAt(pcx + gx * gLimen.colPitch, pcz + gz * gLimen.rowPitch);
            if (r.j == here.j && r.i == here.i) continue;
            bool dup = false;
            for (int k = 0; k < n; k++) if (cand[k].j == r.j && cand[k].i == r.i) dup = true;
            if (dup || n >= 64) continue;
            // nearest point of the slab to the player
            int nx = lmClampInt(pcx, r.x0, r.x1 - 1), nz = lmClampInt(pcz, r.z0, r.z1 - 1);
            float dx = (float)(nx - pcx), dz = (float)(nz - pcz);
            float d = sqrtf(dx * dx + dz * dz);
            if (d > reachNow) continue;
            // its middle has to lie on the side the player is heading, unless it is close enough to turn to
            float mx = (float)(r.x0 + r.x1) * 0.5f - (float)pcx, mz = (float)(r.z0 + r.z1) * 0.5f - (float)pcz;
            float ml = sqrtf(mx * mx + mz * mz);
            float al = ml > 1.0f ? (mx * hx + mz * hz) / ml : 1.0f;
            if (al < 0.15f && d > 8.0f) continue;
            const LmNarSlab* rec = lmNarRecPeek(r.j, r.i);
            if (lmNarSlabSeen(r.j, r.i) || (rec && rec->decided)) continue;
            if (!due && rec && rec->minored) continue;      // quiet already; nothing big is owed yet
            if (!lmNarFree(r.j, r.i, false)) continue;
            Cand c; c.j = r.j; c.i = r.i; c.dist = d;
            c.cost = d + (1.0f - al) * 6.0f;      // the one the player is walking at, not merely the nearest
            c.freeStruct = lmNarFree(r.j, r.i, true);
            // a megastructure the narrator itself put next door stays alone
            if (c.freeStruct) {
                long long nb[16];
                int nn = lmNarNeighbours(r.j, r.i, nb, 16);
                for (int k = 0; k < nn; k++) {
                    const LmBeat* nbt = lmBeatAt((int)(nb[k] >> 32), (int)(nb[k] & 0xFFFFFFFFll));
                    if (nbt && nbt->megaForm >= 0) c.freeStruct = false;
                }
            }
            c.natural = lmSlabTiers(r.j, r.i) > 1;
            // on a roofless level a slab that grows a vault is seen from afar: only beyond the far plane
            if (sky && c.freeStruct && d < 66.0f) c.freeStruct = false;
            cand[n++] = c;
        }
    }
    if (n == 0) return;
    int best = 0;
    for (int k = 1; k < n; k++) if (cand[k].cost < cand[best].cost) best = k;
    const Cand& c = cand[best];

    int role = LNR_MINOR;
    if (c.natural) {
        // A megastructure the genes put there. If the player is owed that very form it stays
        // (and counts as the thing ahead); a stale one is recast into the form owed most.
        const LimenSlab& t = lmNarTrial(c.j, c.i);
        int nf = lmFeatOfMega(t.megaForm, t.megaSub);
        // (and one that holds a knot the player is owed is not recast either)
        bool knot = !t.dark && lmSlabHasEscherKnotAt(c.j, c.i);
        if (nf < 0 || lmNarNovelty(nf) >= 1.6f || !c.freeStruct || (knot && lmNarNovelty(LN_ESCHER) >= 1.6f)) {
            lmNarRec(c.j, c.i).decided = true;
            if (nf >= 0) { N.placed.push_back({lmSlabKey(c.j, c.i), (unsigned char)nf, true, false, N.time}); N.levelPlaced[nf]++; }
            if (knot) { N.placed.push_back({lmSlabKey(c.j, c.i), (unsigned char)LN_ESCHER, true, false, N.time}); N.levelPlaced[LN_ESCHER]++; }
            return;
        }
        role = LNR_MEGA;
    } else if (due) {
        // megastructures and strangeness take turns, spare after spare
        bool wantMega = N.roamMegaNext;
        if (N.wish >= 0 && N.wish < LN_QUIRK_FIRST) wantMega = lmFeatInfo(N.wish).cls == LNC_MEGA;
        role = (wantMega && c.freeStruct) ? LNR_MEGA : LNR_STRANGE;
    }
    // The way the player would come is this slab's road: megastructures open towards the door
    // on that side, quiet things stand along the line from it.
    LimenRect r = lmSlabRect(c.j, c.i);
    bool keepRoad = N.hasRoad;
    int kx = N.roadX, kz = N.roadZ, kdx = N.roadDX, kdz = N.roadDZ;
    int ax = 0, co = 0; bool in = false;
    if (!(N.hasRoad && lmNarRoadAt(r, 9, ax, co, in))) {
        int ox = pcx < r.x0 ? -1 : (pcx >= r.x1 ? 1 : 0), oz = pcz < r.z0 ? -1 : (pcz >= r.z1 ? 1 : 0);
        int axis = (ox != 0 && oz != 0) ? (fabsf(hx) >= fabsf(hz) ? 0 : 1) : (ox != 0 ? 0 : 1);
        int dir = axis == 0 ? (ox != 0 ? -ox : (hx >= 0 ? 1 : -1)) : (oz != 0 ? -oz : (hz >= 0 ? 1 : -1));
        int lo = (axis == 0 ? r.z0 : r.x0) + 3, hi = (axis == 0 ? r.z1 : r.x1) - 3;
        int want = lmClampInt(axis == 0 ? pcz : pcx, lo, hi > lo ? hi : lo);
        int across = lmNarFacingDoor(r, axis, dir, want, want);
        N.hasRoad = true;
        N.roadX = axis == 0 ? (dir > 0 ? r.x0 - 1 : r.x1 + 1) : across;
        N.roadZ = axis == 0 ? across : (dir > 0 ? r.z0 - 1 : r.z1 + 1);
        N.roadDX = axis == 0 ? dir : 0; N.roadDZ = axis == 1 ? dir : 0;
    }
    size_t before = N.placed.size();
    N.planEntry = true; N.planPX = pcx; N.planPZ = pcz;      // the doorway the player comes through (off the road)
    // (strangeness may change the slab's shape too: a knot raises the vault it hangs in)
    int got = lmNarDecide(c.j, c.i, role, c.freeStruct && (role == LNR_MEGA || role == LNR_STRANGE), N.roamMegaNext ? LNC_STRANGE : LNC_MEGA);
    N.planEntry = false;
    N.hasRoad = keepRoad; N.roadX = kx; N.roadZ = kz; N.roadDX = kdx; N.roadDZ = kdz;
    for (size_t k = before; k < N.placed.size(); k++) N.placed[k].roam = true;
    if (got == LNR_NONE) return;
    N.rewrites++;
    if (got == LNR_MEGA || got == LNR_STRANGE) {
        N.urge = false;
        N.roamMegaNext = (got != LNR_MEGA);
        const LmNarPlaced* what = nullptr;
        for (size_t k = before; k < N.placed.size(); k++) if (N.placed[k].major) { what = &N.placed[k]; break; }
        lmNarLog("ahead: %s, %dm", what ? lmFeatInfo(what->feat).name : "?", (int)(c.dist * cs));
    }
}

// Once a frame (or as often as the caller likes). Returns how many slabs were rewritten; their
// keys are in gNar.dirty until the caller clears it.
inline int lmNarTick(float dt, float ex, float ez, float yaw, float cs, float halfFov = 1.22f) {
    LmNarrator& N = gNar;
    lmNarSync();
    if (dt > 0.5f) dt = 0.5f;
    N.time += dt; N.sinceMajor += dt; N.sinceAny += dt;
    N.rest = N.rest > dt ? N.rest - dt : 0.0f;
    if (N.hasPlayer && dt > 1e-4f) {
        float mx = ex - N.px, mz = ez - N.pz;
        float sp = sqrtf(mx * mx + mz * mz) / dt;
        if (sp < 45.0f) {                         // a jump further than that is a teleport
            float k = 1.0f - expf(-dt / 2.5f);
            N.vx += (mx / dt - N.vx) * k; N.vz += (mz / dt - N.vz) * k;
            N.speed += (sp - N.speed) * k;
        } else { N.vx = N.vz = N.speed = 0.0f; }
    }
    N.px = ex; N.pz = ez; N.yaw = yaw; N.hasPlayer = true;
    int pcx = (int)floorf(ex / cs), pcz = (int)floorf(ez / cs);
    LimenRect here = lmSlabAt(pcx, pcz);
    if (pcx != here.x0 && pcz != here.z0) {
        LmNarSlab& rec = lmNarRec(here.j, here.i);
        rec.entered = true; rec.inside += dt;
    }
    float dyaw = fabsf(yaw - N.lookYaw);
    if (N.time - N.lookT >= 0.12f || dyaw > 0.12f || fabsf(ex - N.lookX) + fabsf(ez - N.lookZ) > 1.5f) {
        lmNarLook(ex, ez, yaw, halfFov, cs);
        lmNarWitnessTouched(ex, ez, cs);
        N.lookT = N.time; N.lookYaw = yaw; N.lookX = ex; N.lookZ = ez;
    }
    if (!N.quirkCounted && N.time > 20.0f) {
        N.quirkCounted = true;
        for (int q = 1; q < LM_QUIRK_COUNT; q++) if (lmHasQuirk(gLimenDNA, q)) lmNarWitness(LN_QUIRK_FIRST + q - 1, here.j, here.i);
    }
    if (N.enabled && N.adaptive && N.rewrite && N.time >= N.nextPlan) {
        N.nextPlan = N.time + 0.5f;
        lmNarPlanRoam(cs);
    }
    if (N.time >= N.nextSave) { N.nextSave = N.time + 20.0f; lmNarSave(); }
    return (int)N.dirty.size();
}

// ---------------------------------------------------------------- for the debug menu and the tool
inline const char* lmNarRoleName(int role) {
    return role == LNR_MEGA ? "megastructure" : (role == LNR_STRANGE ? "strangeness" : (role == LNR_MINOR ? "quiet" : "-"));
}
// What is waiting to be found on this level, nearest first is up to the caller.
inline int lmNarPending(const LmNarPlaced** out, int cap) {
    int n = 0;
    for (const LmNarPlaced& p : gNar.placed) if (!p.witnessed && n < cap) out[n++] = &p;
    return n;
}
