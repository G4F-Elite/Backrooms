#pragma once
// Part of limen_mega.h: terrain, spires, grotto, monolith, rootfall superstructures.

// ---------------------------------------------------------------- mega-unstructures
// Phenomena rather than architecture: no rooms, doors or lamps. Each kind is drawn from three
// buffers, each with its own material (`lmUnstructSlotTex`): mv = main surface (walls, roof,
// terrain or deck), mav = secondary (grotto floor, spire stone, trunk, grass), wv = the third
// material (the grotto's crimson canopy).
inline int lmUnstructKind(int mf) { return lmIsMegaUnstruct(mf) ? mf - MF_MONOLITH + 1 : 0; }

// Terraced floor of one walkable cell: a 3x3 grid of sub-tiles at lmUnstructSubHeight, with a
// vertical riser on every side that drops to a lower walkable neighbour (walls have their own faces).
inline void lmPushTerrainCell(const LimenSlab& s, int slx, int slz, float px, float pz, std::vector<float>& fv) {
    const float sub = CS / (float)LM_TERR_SUB;
    static const int ox[4] = {1, -1, 0, 0}, oz[4] = {0, 0, 1, -1};
    for (int a = 0; a < LM_TERR_SUB; a++) {
        for (int b = 0; b < LM_TERR_SUB; b++) {
            int gx = slx * LM_TERR_SUB + a, gz = slz * LM_TERR_SUB + b;
            float h = lmUnstructSubHeight(s, gx, gz);
            float x0 = px + a * sub, x1 = x0 + sub, z0 = pz + b * sub, z1 = z0 + sub;
            lmPushQuad(fv, Vec3(x0, h, z1), Vec3(x1, h, z1), Vec3(x1, h, z0), Vec3(x0, h, z0), CS);
            for (int d = 0; d < 4; d++) {
                int ngx = gx + ox[d], ngz = gz + oz[d];
                int ncx = lmFloorDiv(ngx, LM_TERR_SUB), ncz = lmFloorDiv(ngz, LM_TERR_SUB);
                if (ncx < 0 || ncz < 0 || ncx >= s.w || ncz >= s.h || !lmIsWalkableTag(s.tag[ncx][ncz])) continue;
                float hn = lmUnstructSubHeight(s, ngx, ngz);
                if (hn >= h - 0.01f) continue;
                switch (d) {
                    case 0:  lmPushWallSpan(fv, x1, hn, z0,  0.0f,  sub, h); break;   // faces +X
                    case 1:  lmPushWallSpan(fv, x0, hn, z1,  0.0f, -sub, h); break;   // faces -X
                    case 2:  lmPushWallSpan(fv, x1, hn, z1, -sub,  0.0f, h); break;   // faces +Z
                    default: lmPushWallSpan(fv, x0, hn, z0,  sub,  0.0f, h); break;   // faces -Z
                }
            }
        }
    }
}

// MF_SPIRES: colossal forked basalt needles standing on the snow terraces (one per ~30 cells).
inline void lmBuildMegaSpires(const LimenSlab& s, float T, std::vector<float>& mav, std::vector<LimenAabb2D>& outCols) {
    const float ceilY = s.ceilTiers * WH;
    for (int slx = 1; slx < s.w - 1; slx++) {
        for (int slz = 1; slz < s.h - 1; slz++) {
            if (!lmIsWalkableTag(s.tag[slx][slz])) continue;
            u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(slx * 61 + slz), 0x5350494Cu);
            if (h % 100u >= 3u) continue;
            if (lmStoryNear(s, slx, slz, 1)) continue;            // not on the rig's lane, nor leaning into it
            float bx = (s.rect.x0 + 1 + slx + 0.2f + 0.6f * (float)((h >> 8) & 255u) / 255.0f) * CS;
            float bz = (s.rect.z0 + 1 + slz + 0.2f + 0.6f * (float)((h >> 16) & 255u) / 255.0f) * CS;
            float base = lmUnstructTerrainY(s, bx, bz, CS);
            float f = 0.55f + 0.40f * (float)((h >> 24) & 15u) / 15.0f;
            // one needle in four is a colossus that goes on up out of sight
            bool giant = ((h >> 28) & 3u) == 0u && !lmStoryNear(s, slx, slz, 1);
            float hb = giant ? 2.3f : 1.0f;
            float topY = giant ? ceilY + 14.0f : base + (ceilY - 1.2f - base) * f;
            float lx = ((float)((h >> 4) & 7u) - 3.5f) * 0.9f, lz = ((float)((h >> 1) & 7u) - 3.5f) * 0.9f;
            float tx = bx + lx, tz = bz + lz;
            lmPushShearedBox(mav, bx, base - 0.5f, bz, tx, topY, tz, hb, hb * 0.30f, T);
            // fork: two prongs leave the trunk at 62% of its height and rise to the summit
            float fy = base + (topY - base) * 0.62f;
            float fx = bx + lx * 0.62f, fz = bz + lz * 0.62f;
            float ang = (float)((h >> 12) & 7u) * 0.7853982f;
            float ox = cosf(ang) * 2.4f, oz = sinf(ang) * 2.4f;
            lmPushShearedBox(mav, fx, fy, fz, tx + ox * hb, topY * 0.985f, tz + oz * hb, 0.55f * hb, 0.12f, T);
            lmPushShearedBox(mav, fx, fy, fz, tx - ox * 0.6f * hb, topY * 0.95f, tz - oz * 0.6f * hb, 0.55f * hb, 0.12f, T);
            outCols.push_back({bx - hb, bz - hb, bx + hb, bz + hb});
        }
    }
}

// Unit float 0..1 out of a hash word.
inline float lmU01(u32 h, int shift) { return (float)((h >> shift) & 255u) / 255.0f; }

// A segment of a crooked limb (trunk, branch, root, stalactite): square in plan at both ends.
// The lower end is whichever of the two points is lower.
inline void lmPushLimb(std::vector<float>& v, Vec3 a, float ra, Vec3 b, float rb, float tile) {
    if (a.y > b.y) { Vec3 t = a; a = b; b = t; float r = ra; ra = rb; rb = r; }
    lmPushShearedBox(v, a.x, a.y, a.z, b.x, b.y, b.z, ra, rb, tile);
}

// MF_GROTTO: the cave roof. Not a lid but a dome of inverted terraces (the same 1.67 m tiles
// as the floor), low over the rim and rising in rings to the torn oculus over the clearing.
// Always at least 3.2 m above the floor under it.
const float LM_GROTTO_OPEN = 1e6f;   // "no roof here": the oculus
inline float lmGrottoCeilSub(const LimenSlab& s, int gx, int gz) {
    int gw = s.w * LM_TERR_SUB, gh = s.h * LM_TERR_SUB;
    float ceilY = s.ceilTiers * WH;
    u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)s.megaSub, 0x43454Cu);
    float p1 = lmU01(h, 0) * 6.2832f, p2 = lmU01(h, 8) * 6.2832f, p3 = lmU01(h, 16) * 6.2832f;
    float cx = (float)gw * 0.5f, cz = (float)gh * 0.5f;
    float rx = ((float)gx + 0.5f - cx) / fmaxf(cx * 0.92f, 1.0f);
    float rz = ((float)gz + 0.5f - cz) / fmaxf(cz * 0.92f, 1.0f);
    float rad = sqrtf(rx * rx + rz * rz);
    float ang = atan2f(rz, rx);
    float rOc = 0.105f + 0.034f * sinf(ang * 3.0f + p1) + 0.022f * cosf(ang * 7.0f + p2);
    if (rad < rOc) return LM_GROTTO_OPEN;
    float t = (rad - rOc) / (0.98f - rOc);
    if (t > 1.0f) t = 1.0f;
    float fold = 1.0f + 0.16f * sinf(ang * 5.0f + p3) + 0.10f * cosf((float)gx * 0.31f + (float)gz * 0.27f + p1);
    float drop = powf(t, 0.62f) * fold;
    if (drop > 1.0f) drop = 1.0f;
    const float low = 6.4f, step = 1.25f;
    float y = ceilY - (ceilY - low) * drop;
    y = low + floorf((y - low) / step + 0.5f) * step;
    float minY = lmUnstructSubHeight(s, gx, gz) + 3.2f;
    return y < minY ? minY : (y > ceilY ? ceilY : y);
}

inline void lmBuildGrottoRoof(const LimenSlab& s, float T, std::vector<float>& mv) {
    const float sub = CS / (float)LM_TERR_SUB;
    const float chimneyTop = s.ceilTiers * WH + 7.0f;
    static const int ox[4] = {1, -1, 0, 0}, oz[4] = {0, 0, 1, -1};
    int gw = s.w * LM_TERR_SUB, gh = s.h * LM_TERR_SUB;
    auto rock = [&](int gx, int gz) {   // inside the rock mass: neither roof nor riser is ever seen
        if (gx < 0 || gz < 0 || gx >= gw || gz >= gh) return true;
        return s.tag[gx / LM_TERR_SUB][gz / LM_TERR_SUB] == LT_WALL;
    };
    for (int gx = 0; gx < gw; gx++) {
        for (int gz = 0; gz < gh; gz++) {
            if (rock(gx, gz)) continue;
            float h = lmGrottoCeilSub(s, gx, gz);
            if (h >= LM_GROTTO_OPEN) continue;
            float x0 = (s.rect.x0 + 1) * CS + gx * sub, x1 = x0 + sub;
            float z0 = (s.rect.z0 + 1) * CS + gz * sub, z1 = z0 + sub;
            lmPushQuad(mv, Vec3(x0, h, z0), Vec3(x1, h, z0), Vec3(x1, h, z1), Vec3(x0, h, z1), T);
            for (int d = 0; d < 4; d++) {
                int ngx = gx + ox[d], ngz = gz + oz[d];
                if (rock(ngx, ngz)) continue;
                float hn = lmGrottoCeilSub(s, ngx, ngz);
                if (hn >= LM_GROTTO_OPEN) hn = chimneyTop;   // the throat of the oculus
                if (hn <= h + 0.01f) continue;
                switch (d) {
                    case 0:  lmPushWallSpan(mv, x1, h, z0,  0.0f,  sub, hn); break;
                    case 1:  lmPushWallSpan(mv, x0, h, z1,  0.0f, -sub, hn); break;
                    case 2:  lmPushWallSpan(mv, x1, h, z1, -sub,  0.0f, hn); break;
                    default: lmPushWallSpan(mv, x0, h, z0,  sub,  0.0f, hn); break;
                }
            }
        }
    }
}

// MF_GROTTO: dripstone. Stalactites hang from the roof terraces, stalagmites and a few
// hourglass columns stand on the floor terraces (never in the clearing, the four mouths or
// on a cell the game needs).
inline void lmBuildGrottoDripstone(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<LimenAabb2D>& outCols) {
    const float sub = CS / (float)LM_TERR_SUB;
    int gw = s.w * LM_TERR_SUB, gh = s.h * LM_TERR_SUB;
    int mx = s.w / 2, mz = s.h / 2;
    for (int gx = LM_TERR_SUB; gx < gw - LM_TERR_SUB; gx++) {
        for (int gz = LM_TERR_SUB; gz < gh - LM_TERR_SUB; gz++) {
            int slx = gx / LM_TERR_SUB, slz = gz / LM_TERR_SUB;
            if (!lmIsWalkableTag(s.tag[slx][slz])) continue;
            u32 h = lmHash((u32)s.rect.j * 131u + (u32)s.rect.i, (u32)gx, (u32)gz, 0x44524950u);
            float ceil = lmGrottoCeilSub(s, gx, gz);
            if (ceil >= LM_GROTTO_OPEN) continue;
            float flr = lmUnstructSubHeight(s, gx, gz);
            float room = ceil - flr;
            float x = (s.rect.x0 + 1) * CS + (gx + 0.25f + 0.5f * lmU01(h, 8)) * sub;
            float z = (s.rect.z0 + 1) * CS + (gz + 0.25f + 0.5f * lmU01(h, 16)) * sub;
            u32 roll = h % 1000u;
            bool keepClear = lmStoryNear(s, slx, slz, 1) || abs(slx - mx) <= 1 || abs(slz - mz) <= 1 || flr < 0.2f;
            if (roll < 26u) {                                        // stalactite
                float len = 1.0f + lmU01(h, 24) * fminf(9.0f, room - 2.9f);
                if (len > room - 2.6f) len = room - 2.6f;
                if (len < 0.5f) continue;
                float r = 0.16f + len * 0.055f;
                float lean = (lmU01(h, 4) - 0.5f) * 0.5f;
                lmPushLimb(mv, Vec3(x + lean, ceil - len, z - lean), 0.03f, Vec3(x, ceil, z), r, T);
            } else if (roll < 34u && !keepClear && room > 6.0f) {    // stalagmite
                float hg = 1.2f + lmU01(h, 24) * fminf(4.5f, room - 4.0f);
                float r = 0.34f + hg * 0.07f;
                lmPushLimb(mv, Vec3(x, flr, z), r, Vec3(x + (lmU01(h, 4) - 0.5f) * 0.4f, flr + hg, z), 0.04f, T);
                outCols.push_back({x - r * 0.8f, z - r * 0.8f, x + r * 0.8f, z + r * 0.8f});
            } else if (roll < 37u && !keepClear && room > 7.0f && room < 26.0f) {   // column
                float mid = flr + room * (0.34f + 0.2f * lmU01(h, 24));
                float r = 0.9f + 0.5f * lmU01(h, 4);
                Vec3 waist(x + (lmU01(h, 12) - 0.5f) * 0.8f, mid, z + (lmU01(h, 20) - 0.5f) * 0.8f);
                lmPushLimb(mv, Vec3(x, flr, z), r, waist, 0.30f, T);
                lmPushLimb(mv, waist, 0.30f, Vec3(x, ceil, z), r * 1.25f, T);
                outCols.push_back({x - r * 0.8f, z - r * 0.8f, x + r * 0.8f, z + r * 0.8f});
            }
        }
    }
}

// MF_GROTTO: the crimson tree of the clearing. An old crooked giant under the oculus: a bent
// trunk on buttress roots, boughs that carry blocky leaf masses, a ring of fallen leaves on
// the earth and a few leaves that stopped on their way down.
inline void lmBuildMegaGrotto(const LimenSlab& s, float T, std::vector<float>& mav, std::vector<float>& wv,
                              std::vector<LimenAabb2D>& outCols) {
    // (with the narrator's road through the clearing the tree stands a few steps to the side of it)
    float cx = (s.rect.x0 + 1 + s.w * 0.5f + s.megaOffX) * CS, cz = (s.rect.z0 + 1 + s.h * 0.5f + s.megaOffZ) * CS;
    u32 seed = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)s.megaSub, 0x54524545u);
    auto rnd = [&](int k, int shift) { return lmU01(lmHash(seed, (u32)k, 7u, 0x4C4541u), shift); };
    auto P = [&](float x, float y, float z) { return Vec3(cx + x, y, cz + z); };

    float bend = rnd(0, 0) * 6.2832f;
    float bx = cosf(bend), bz = sinf(bend);
    Vec3 t0 = P(0, 0, 0), t1 = P(bx * 0.7f, 3.6f, bz * 0.7f), t2 = P(-bx * 0.5f, 7.4f, -bz * 0.3f), t3 = P(bx * 0.4f, 10.6f, bz * 0.5f);
    lmPushLimb(mav, t0, 1.05f, t1, 0.74f, T);
    lmPushLimb(mav, t1, 0.74f, t2, 0.52f, T);
    lmPushLimb(mav, t2, 0.52f, t3, 0.30f, T);
    for (int k = 0; k < 6; k++) {                    // buttress roots
        float a = k * 1.047f + rnd(10 + k, 0) * 0.6f, len = 2.4f + rnd(10 + k, 8) * 2.2f;
        Vec3 tip = P(cosf(a) * len, 0.0f, sinf(a) * len);
        float gy = lmUnstructTerrainY(s, tip.x, tip.z, CS);
        tip.y = gy - 0.05f;
        lmPushLimb(mav, tip, 0.10f, P(cosf(a) * 0.6f, fmaxf(gy, 0.0f) + 0.9f + rnd(10 + k, 16) * 0.7f, sinf(a) * 0.6f), 0.36f, T);
    }
    auto leafMass = [&](Vec3 c, float r, int k) {
        float ry = r * (0.42f + 0.16f * rnd(k, 0));
        lmPushBox(wv, c.x - r, c.y - ry, c.z - r * 0.86f, c.x + r, c.y + ry, c.z + r * 0.86f, T);
        float r2 = r * 0.62f, ox = (rnd(k, 8) - 0.5f) * r * 1.5f, oz = (rnd(k, 16) - 0.5f) * r * 1.5f;
        lmPushBox(wv, c.x + ox - r2, c.y + ry * 0.4f, c.z + oz - r2, c.x + ox + r2, c.y + ry * 0.4f + r2 * 0.9f, c.z + oz + r2, T);
        float r3 = r * 0.5f;
        lmPushBox(wv, c.x - ox * 0.8f - r3, c.y - ry - r3 * 0.5f, c.z - oz * 0.8f - r3, c.x - ox * 0.8f + r3, c.y - ry * 0.5f, c.z - oz * 0.8f + r3, T);
    };
    const Vec3 forks[3] = {t1, t2, t3};
    for (int k = 0; k < 9; k++) {                    // boughs
        Vec3 from = forks[k % 3];
        float a = k * 2.399f + rnd(30 + k, 0) * 0.8f;
        float len = 3.6f + rnd(30 + k, 8) * 3.4f + (k % 3 == 0 ? 1.6f : 0.0f);
        float rise = 1.6f + rnd(30 + k, 16) * 2.6f;
        Vec3 elbow(from.x + cosf(a) * len * 0.55f, from.y + rise * 0.75f, from.z + sinf(a) * len * 0.55f);
        Vec3 tip(from.x + cosf(a + 0.35f) * len, from.y + rise + 0.6f, from.z + sinf(a + 0.35f) * len);
        float r0 = (k % 3 == 0) ? 0.40f : ((k % 3 == 1) ? 0.30f : 0.20f);
        lmPushLimb(mav, from, r0, elbow, r0 * 0.62f, T);
        lmPushLimb(mav, elbow, r0 * 0.62f, tip, 0.07f, T);
        leafMass(tip, 1.7f + rnd(30 + k, 24) * 1.2f, 50 + k);
        if (k % 2 == 0) leafMass(Vec3(elbow.x, elbow.y + 0.9f, elbow.z), 1.2f + rnd(40 + k, 0) * 0.6f, 70 + k);
    }
    leafMass(Vec3(t3.x, t3.y + 1.3f, t3.z), 2.6f, 90);

    for (int k = 0; k < 64; k++) {                   // fallen leaves, thicker near the trunk
        float a = rnd(100 + k, 0) * 6.2832f, r = 1.2f + powf(rnd(100 + k, 8), 1.6f) * 12.5f;
        float x = cx + cosf(a) * r, z = cz + sinf(a) * r;
        float hs = 0.16f + rnd(100 + k, 16) * 0.42f;
        float lo = 1e9f, hi = -1e9f;
        for (int c = 0; c < 4; c++) {                // a patch never bridges a terrace step
            float y = lmUnstructTerrainY(s, x + ((c & 1) ? hs : -hs), z + ((c & 2) ? hs : -hs), CS);
            if (y < lo) lo = y;
            if (y > hi) hi = y;
        }
        if (hi - lo > 0.01f) continue;
        lmPushBox(wv, x - hs, lo, z - hs * 0.8f, x + hs, lo + 0.03f, z + hs * 0.8f, T);
    }
    for (int k = 0; k < 22; k++) {                   // leaves that never landed
        float a = rnd(200 + k, 0) * 6.2832f, r = 0.8f + rnd(200 + k, 8) * 7.5f;
        float y = 2.2f + rnd(200 + k, 16) * 7.0f;
        float x = cx + cosf(a) * r, z = cz + sinf(a) * r, d = 0.09f + rnd(200 + k, 24) * 0.07f;
        lmPushHexa(wv, Vec3(x - d, y, z - d), Vec3(x + d, y + d * 0.8f, z - d), Vec3(x + d, y + d * 0.8f, z + d), Vec3(x - d, y, z + d),
                   Vec3(x - d, y + 0.02f, z - d), Vec3(x + d, y + d * 0.8f + 0.02f, z - d), Vec3(x + d, y + d * 0.8f + 0.02f, z + d), Vec3(x - d, y + 0.02f, z + d), T);
    }
    outCols.push_back({cx - 0.85f, cz - 0.85f, cx + 0.85f, cz + 0.85f});
}

// MF_MONOLITH: what the stone holds up. Slabs wedged askew between two monoliths high over
// a lane, and dark cubes that hang unsupported over the open crossings.
inline void lmBuildMegaMonolith(const LimenSlab& s, float T, std::vector<float>& mav) {
    const float ceilY = s.ceilTiers * WH;
    auto wall = [&](int x, int z) { return x >= 0 && z >= 0 && x < s.w && z < s.h && s.tag[x][z] == LT_WALL; };
    auto open = [&](int x, int z) { return x >= 0 && z >= 0 && x < s.w && z < s.h && lmIsWalkableTag(s.tag[x][z]); };
    for (int slx = 0; slx < s.w; slx++) {
        for (int slz = 0; slz < s.h; slz++) {
            if (!open(slx, slz)) continue;
            u32 h = lmHash((u32)s.rect.j * 131u + (u32)s.rect.i, (u32)slx, (u32)slz, 0x4D4F4Eu);
            float px = (s.rect.x0 + 1 + slx) * CS, pz = (s.rect.z0 + 1 + slz) * CS;
            bool laneZ = wall(slx - 1, slz) && wall(slx + 1, slz);   // lane runs along Z, stone on both X sides
            bool laneX = wall(slx, slz - 1) && wall(slx, slz + 1);
            if ((laneZ || laneX) && h % 100u < 9u) {
                float y = 8.0f + lmU01(h, 8) * fmaxf(ceilY - 18.0f, 1.0f);
                float th = 1.0f + lmU01(h, 16) * 2.8f;
                float tilt = (lmU01(h, 24) - 0.5f) * 5.0f;
                float d0 = 0.3f + lmU01(h, 4) * 1.6f, d1 = d0 + 1.2f + lmU01(h, 12) * (CS - d0 - 1.5f);
                float yA = y - tilt * 0.5f, yB = y + tilt * 0.5f;
                if (laneZ)
                    lmPushHexa(mav, Vec3(px, yA, pz + d0), Vec3(px + CS, yB, pz + d0), Vec3(px + CS, yB, pz + d1), Vec3(px, yA, pz + d1),
                               Vec3(px, yA + th, pz + d0), Vec3(px + CS, yB + th, pz + d0), Vec3(px + CS, yB + th, pz + d1), Vec3(px, yA + th, pz + d1), T);
                else
                    lmPushHexa(mav, Vec3(px + d0, yA, pz), Vec3(px + d1, yA, pz), Vec3(px + d1, yB, pz + CS), Vec3(px + d0, yB, pz + CS),
                               Vec3(px + d0, yA + th, pz), Vec3(px + d1, yA + th, pz), Vec3(px + d1, yB + th, pz + CS), Vec3(px + d0, yB + th, pz + CS), T);
                continue;
            }
            bool plaza = true;
            for (int dx = -1; dx <= 1 && plaza; dx++) for (int dz = -1; dz <= 1; dz++) if (!open(slx + dx, slz + dz)) { plaza = false; break; }
            if (plaza && h % 100u < 22u) {
                float r = 1.0f + lmU01(h, 8) * 1.1f;
                float y = 4.9f + lmU01(h, 16) * 9.0f;
                float x = px + CS * 0.5f, z = pz + CS * 0.5f;
                lmPushBox(mav, x - r, y, z - r, x + r, y + 2.0f * r, z + r, T);
            }
        }
    }
}

// MF_TENDRILS ("rootfall"): tufts of dry grass on the floating earth decks.
inline void lmBuildMegaRootfall(const LimenSlab& s, float T, std::vector<float>& mav) {
    for (int slx = 0; slx < s.w; slx++) {
        for (int slz = 0; slz < s.h; slz++) {
            if (s.tag[slx][slz] != LT_BRIDGE) continue;
            float px = (s.rect.x0 + 1 + slx) * CS, pz = (s.rect.z0 + 1 + slz) * CS;
            for (int k = 0; k < 3; k++) {
                u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(slx * 61 + slz * 7 + k), 0x52464Cu);
                if ((h % 100u) >= 45u) continue;
                float x = px + CS * (0.15f + 0.7f * (float)((h >> 8) & 255u) / 255.0f);
                float z = pz + CS * (0.15f + 0.7f * (float)((h >> 16) & 255u) / 255.0f);
                float hgt = 0.35f + 0.5f * (float)((h >> 24) & 255u) / 255.0f;
                float lean = ((float)((h >> 3) & 7u) - 3.5f) * 0.06f;
                lmPushShearedBox(mav, x, 0.0f, z, x + lean, hgt, z - lean, 0.10f, 0.02f, T);
            }
        }
    }
}

// MF_TENDRILS: what gives the place its name. Roots hang from under every earth ribbon far down
// into the void, and clods of the same earth float in the open air around them, roots and all.
inline void lmBuildRootfallRoots(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav) {
    auto root = [&](float x, float y, float z, float len, float r, u32 h) {
        float a1 = lmU01(h, 0) * 6.2832f, a2 = a1 + (lmU01(h, 8) - 0.5f) * 3.0f;
        Vec3 p0(x, y, z);
        Vec3 p1(x + cosf(a1) * len * 0.10f, y - len * 0.45f, z + sinf(a1) * len * 0.10f);
        Vec3 p2(p1.x + cosf(a2) * len * 0.12f, y - len, p1.z + sinf(a2) * len * 0.12f);
        lmPushLimb(mv, p0, r, p1, r * 0.55f, T);
        lmPushLimb(mv, p1, r * 0.55f, p2, 0.015f, T);
    };
    for (int slx = 0; slx < s.w; slx++) {
        for (int slz = 0; slz < s.h; slz++) {
            int tag = s.tag[slx][slz];
            float px = (s.rect.x0 + 1 + slx) * CS, pz = (s.rect.z0 + 1 + slz) * CS;
            if (tag == LT_BRIDGE) {
                for (int k = 0; k < 4; k++) {
                    u32 h = lmHash((u32)s.rect.j * 131u + (u32)s.rect.i, (u32)(slx * 61 + slz), (u32)k, 0x524F4Fu);
                    if (h % 100u >= 62u) continue;
                    float x = px + CS * (0.12f + 0.76f * lmU01(h, 8)), z = pz + CS * (0.12f + 0.76f * lmU01(h, 16));
                    bool core = fabsf(x - px - CS * 0.5f) < 1.0f && fabsf(z - pz - CS * 0.5f) < 1.0f;
                    float len = 2.5f + powf(lmU01(h, 24), 2.2f) * 26.0f;
                    root(x, core ? -3.1f : -1.8f, z, len, 0.07f + len * 0.006f, h * 2654435761u);
                }
            } else if (tag == LT_VOID) {
                u32 h = lmHash((u32)s.rect.j * 131u + (u32)s.rect.i, (u32)slx, (u32)slz, 0x434C4Fu);
                if (h % 100u >= 5u) continue;
                bool nearDeck = false;   // a clod never hangs where a walker's head passes
                for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) {
                    int nx = slx + dx, nz = slz + dz;
                    if (nx >= 0 && nz >= 0 && nx < s.w && nz < s.h && s.tag[nx][nz] == LT_BRIDGE) nearDeck = true;
                }
                float x = px + CS * 0.5f, z = pz + CS * 0.5f;
                float r = 0.8f + lmU01(h, 8) * 1.3f;
                float y = nearDeck ? (4.5f + lmU01(h, 16) * 16.0f) : (-16.0f + lmU01(h, 16) * 38.0f);
                lmPushBox(mv, x - r, y - 0.55f, z - r, x + r, y, z + r, T);
                lmPushBox(mv, x - r * 0.72f, y - 1.25f, z - r * 0.66f, x + r * 0.70f, y - 0.55f, z + r * 0.74f, T);
                lmPushBox(mv, x - r * 0.36f, y - 2.0f, z - r * 0.40f, x + r * 0.42f, y - 1.25f, z + r * 0.34f, T);
                for (int k = 0; k < 3; k++) {
                    u32 hk = lmHash(h, (u32)k, 3u, 0x524F4Fu);
                    root(x + (lmU01(hk, 8) - 0.5f) * r * 0.6f, y - 2.0f, z + (lmU01(hk, 16) - 0.5f) * r * 0.6f,
                         1.5f + lmU01(hk, 24) * 7.0f, 0.07f, hk);
                    float gx = x + (lmU01(hk, 0) - 0.5f) * r * 1.6f, gz = z + (lmU01(hk, 12) - 0.5f) * r * 1.6f;
                    lmPushShearedBox(mav, gx, y, gz, gx + 0.05f, y + 0.3f + 0.4f * lmU01(hk, 20), gz - 0.04f, 0.10f, 0.02f, T);
                }
            }
        }
    }
}

// Roof, feature and decor of a mega-unstructure. Monoliths keep a roof of mist-grey stone, the
// grotto a cave roof with a torn oculus, and the open kinds have no roof at all (the sky shows).
inline void lmBuildUnstructSuperstructure(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav,
                                          std::vector<float>& wv, std::vector<LimenAabb2D>& outCols) {
    switch (s.megaForm) {
        case MF_MONOLITH: lmBuildMegaMonolith(s, T, mav); break;   // no lid: the lanes are slits of pale sky
        case MF_GROTTO:
            lmBuildGrottoRoof(s, T, mv);
            lmBuildGrottoDripstone(s, T, mv, outCols);
            lmBuildMegaGrotto(s, T, mav, wv, outCols);
            break;
        case MF_SPIRES:  lmBuildMegaSpires(s, T, mav, outCols); break;
        case MF_TENDRILS: lmBuildMegaRootfall(s, T, mav); lmBuildRootfallRoots(s, T, mv, mav); break;
        default: break;
    }
}

// Which texture each buffer of a mega-unstructure is drawn with (slot 0 = mv, 1 = mav, 2 = wv).
inline GLuint lmUnstructSlotTex(int kind, int slot) {
    lmEnsureUnstructTextures();
    int t = UTEX_MONOLITH;
    switch (kind) {
        case 1: t = (slot == 1) ? UTEX_MONOLITH_ALT : UTEX_MONOLITH; break;
        case 2: t = (slot == 1) ? UTEX_BASALT : UTEX_SNOW; break;
        case 3: t = (slot == 0) ? UTEX_CAVE_ROCK : ((slot == 1) ? UTEX_EARTH : UTEX_CRIMSON_LEAF); break;
        case 4: t = (slot == 1) ? UTEX_DRY_GRASS : UTEX_EARTH; break;
        default: break;
    }
    return lmUnstructTex[t];
}

inline void lmBuildSlabSuperstructure(const LimenSlab& s, std::vector<float>& mv, std::vector<float>& mav,
                                      std::vector<float>& wv, std::vector<float>& dadoV,
                                      std::vector<float>& dv, std::vector<float>& lv, std::vector<LimenAabb2D>& outCols) {
    const float T = 5.6f + (float)(255 - gLimenDNA.megaPoles[s.palette % LM_MEGA_POLES].seamFreqX) * 2.8f / 255.0f;
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, 9u, 0x4D45u);
    bool alongZ = s.h > s.w;

    if (lmIsMegaUnstruct(s.megaForm)) { lmBuildUnstructSuperstructure(s, T, mv, mav, wv, outCols); return; }

    // Watertight vault ceiling + full-perimeter WH..ceilY upper enclosure (fixes skybox holes above low shells!)
    lmPushQuad(mv, Vec3(x0 - CS, ceilY, z0 - CS), Vec3(x1 + CS, ceilY, z0 - CS),
               Vec3(x1 + CS, ceilY, z1 + CS), Vec3(x0 - CS, ceilY, z1 + CS), T);
    lmPushQuad(mv, Vec3(x0, WH, z0), Vec3(x0, ceilY, z0), Vec3(x0, ceilY, z1), Vec3(x0, WH, z1), T);
    lmPushQuad(mv, Vec3(x1, WH, z1), Vec3(x1, ceilY, z1), Vec3(x1, ceilY, z0), Vec3(x1, WH, z0), T);
    lmPushQuad(mv, Vec3(x1, WH, z0), Vec3(x1, ceilY, z0), Vec3(x0, ceilY, z0), Vec3(x0, WH, z0), T);
    lmPushQuad(mv, Vec3(x0, WH, z1), Vec3(x0, ceilY, z1), Vec3(x1, ceilY, z1), Vec3(x1, WH, z1), T);

    lmBuildEscherStairKnot(s, T, wv, dadoV, dv, lv);

    if (s.megaForm == MF_ZIGGURAT)  { lmBuildMegaZiggurat(s, T, mv, mav, dv, lv, outCols); return; }
    if (s.megaForm == MF_CANYON)    { lmBuildMegaCanyon(s, T, mv, mav, dv, lv); return; }
    if (s.megaForm == MF_HYPOSTYLE) { lmBuildMegaHypostyle(s, T, mv, dv, lv, outCols); return; }
    if (s.megaForm == MF_SILO)      { lmBuildMegaSilo(s, T, mv, mav, dv, lv, outCols); return; }
    if (s.megaForm == MF_HANGING)   { lmBuildMegaHanging(s, T, mv, mav, dv, lv); return; }
    if (s.megaForm == MF_CELLBLOCK) { lmBuildMegaCellblock(s, T, mv, mav, dv, lv); return; }
    if (s.megaForm == MF_PLATFORM)  { lmBuildMegaPlatform(s, T, mv, dv, lv); return; }

    for (float y = 9.0f; y <= ceilY - 6.0f; y += 9.0f) {
        lmPushBox(mv, x0, y - 0.50f, z0, x1, y + 0.30f, z0 + 1.35f, T);
        lmPushBox(mv, x0, y - 0.50f, z1 - 1.35f, x1, y + 0.30f, z1, T);
        lmPushBox(mv, x0, y - 0.50f, z0, x0 + 1.35f, y + 0.30f, z1, T);
        lmPushBox(mv, x1 - 1.35f, y - 0.50f, z0, x1, y + 0.30f, z1, T);
        lmPushBox(mv, x0, y + 0.30f, z0 + 1.05f, x1, y + 1.25f, z0 + 1.35f, T);
        lmPushBox(mv, x0, y + 0.30f, z1 - 1.35f, x1, y + 1.25f, z1 - 1.05f, T);
        lmPushBox(mv, x0 + 1.05f, y + 0.30f, z0, x0 + 1.35f, y + 1.25f, z1, T);
        lmPushBox(mv, x1 - 1.35f, y + 0.30f, z0, x1 - 1.05f, y + 1.25f, z1, T);
    }

    float ribBot = ceilY - 2.6f;
    if (!alongZ) {
        float midZ = 0.5f * (z0 + z1), spanZ = (z1 - z0) * 0.26f;
        lmPushBox(mv, x0, ribBot, midZ - spanZ - 0.8f, x1, ceilY, midZ - spanZ + 0.8f, T);
        lmPushBox(mv, x0, ribBot, midZ + spanZ - 0.8f, x1, ceilY, midZ + spanZ + 0.8f, T);
        for (float x = x0 + 20.0f; x <= x1 - 15.0f; x += 20.0f) {
            lmPushBox(mv, x - 0.80f, ribBot, z0, x + 0.80f, ceilY, z1, T);
            if (!s.dark && x + 16.0f <= x1) {
                lmPushLightQuad(lv, Vec3(x + 6.5f, ceilY - 0.08f, midZ - spanZ + 2.5f),
                                Vec3(x + 8.0f, ceilY - 0.08f, midZ - spanZ + 2.5f),
                                Vec3(x + 8.0f, ceilY - 0.08f, midZ + spanZ - 2.5f),
                                Vec3(x + 6.5f, ceilY - 0.08f, midZ + spanZ - 2.5f), true);
            }
        }
    } else {
        float midX = 0.5f * (x0 + x1), spanX = (x1 - x0) * 0.26f;
        lmPushBox(mv, midX - spanX - 0.8f, ribBot, z0, midX - spanX + 0.8f, ceilY, z1, T);
        lmPushBox(mv, midX + spanX - 0.8f, ribBot, z0, midX + spanX + 0.8f, ceilY, z1, T);
        for (float z = z0 + 20.0f; z <= z1 - 15.0f; z += 20.0f) {
            lmPushBox(mv, x0, ribBot, z - 0.80f, x1, ceilY, z + 0.80f, T);
            if (!s.dark && z + 16.0f <= z1) {
                lmPushLightQuad(lv, Vec3(midX - spanX + 2.5f, ceilY - 0.08f, z + 6.5f),
                                Vec3(midX + spanX - 2.5f, ceilY - 0.08f, z + 6.5f),
                                Vec3(midX + spanX - 2.5f, ceilY - 0.08f, z + 8.0f),
                                Vec3(midX - spanX + 2.5f, ceilY - 0.08f, z + 8.0f), true);
            }
        }
    }

    auto addSkyBridge = [&](bool spanAlongX, float posFrac, float y, bool halfSevered, bool withLights) {
        if (y > ceilY - 5.5f) return;
        if (spanAlongX) {
            float cz = z0 + (z1 - z0) * posFrac;
            float bx1 = halfSevered ? (x0 + (x1 - x0) * 0.56f) : x1;
            lmPushBox(mv, x0, y - 1.5f, cz - 2.2f, bx1, y, cz + 2.2f, T);
            lmPushBox(mv, x0, y, cz - 2.2f, bx1, y + 1.0f, cz - 1.88f, T);
            lmPushBox(mv, x0, y, cz + 1.88f, bx1, y + 1.0f, cz + 2.2f, T);
            if (withLights && !s.dark) {
                for (float lx = x0 + 18.0f; lx <= bx1 - 10.0f; lx += 28.0f) {
                    lmPushLightBox(lv, lx, y + 1.0f, cz - 2.04f, 0.25f, 0.32f, 0.20f);
                    lmPushLightBox(lv, lx, y - 1.62f, cz, 0.80f, 0.10f, 0.42f);
                }
            }
        } else {
            float cx = x0 + (x1 - x0) * posFrac;
            float bz1 = halfSevered ? (z0 + (z1 - z0) * 0.56f) : z1;
            lmPushBox(mv, cx - 2.2f, y - 1.5f, z0, cx + 2.2f, y, bz1, T);
            lmPushBox(mv, cx - 2.2f, y, z0, cx - 1.88f, y + 1.0f, bz1, T);
            lmPushBox(mv, cx + 1.88f, y, z0, cx + 2.2f, y + 1.0f, bz1, T);
            if (withLights && !s.dark) {
                for (float lz = z0 + 18.0f; lz <= bz1 - 10.0f; lz += 28.0f) {
                    lmPushLightBox(lv, cx - 2.04f, y + 1.0f, lz, 0.20f, 0.32f, 0.25f);
                    lmPushLightBox(lv, cx, y - 1.62f, lz, 0.42f, 0.10f, 0.80f);
                }
            }
        }
    };

    float f1 = 0.26f + 0.09f * (float)(h % 3u);
    float f2 = 0.70f - 0.08f * (float)((h >> 3) % 3u);
    addSkyBridge(alongZ, f1, 14.0f, ((h >> 6) & 1u) != 0u, true);
    addSkyBridge(!alongZ, f2, 23.5f, ((h >> 9) & 1u) != 0u, true);
    if (s.share[LT_VOID] >= 25) {
        addSkyBridge(alongZ, 1.0f - f1, -13.5f, false, true);
        addSkyBridge(!alongZ, 1.0f - f2, -25.0f, true, false);
    }

    lmBuildHighOrphans(s, mv, dv, lv);
    lmBuildStairsToNowhere(s, mv, dv, lv, outCols, T);
    lmBuildOrphanFrames(s, dv, outCols);
}
