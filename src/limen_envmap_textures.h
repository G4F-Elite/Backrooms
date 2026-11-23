#pragma once
// Part of limen_envmap.h: per-level decor atlas, unstructured, mega and xeno textures.

// Rebuilds `decorTex` so plaster doorway infills, scars, and ceiling tiles match the active
// level's continuous wall/ceiling palette rather than hardcoded Level-0 or parking colors.
inline void lmRebuildLevelDecorAtlas(GLuint& texId) {
    const int tile = 256;
    const int sz = tile * DECOR_ATLAS_GRID;
    std::vector<unsigned char> d((size_t)sz * sz * 4);
    const LmSurfDNA& ws = gLimenDNA.wallSurf;
    const LmSurfDNA& cs = gLimenDNA.ceilSurf;
    bool roughPlaster = ws.grain > 135 && ws.stripe < 90;

    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            int mat = (y / tile) * DECOR_ATLAS_GRID + (x / tile);
            float px = (float)(x % tile), py = (float)(y % tile);
            DecorTexel t = decorAtlasTexel(mat, px / (float)tile, py / (float)tile, px, py, roughPlaster);
            if (mat == DECOR_MAT_PLASTER) {
                float n = perlin(px * 0.22f, py * 0.22f, 3) * (6.0f + ws.grain * 0.05f);
                t.r = (float)ws.rgb[0] + n;
                t.g = (float)ws.rgb[1] + n;
                t.b = (float)ws.rgb[2] + n;
            } else if (mat == DECOR_MAT_CEILTILE) {
                float n = perlin(px * 0.35f, py * 0.35f, 2) * 8.0f;
                float pit = perlin(px * 0.85f + 5.0f, py * 0.85f + 1.0f, 2) > 0.42f ? -14.0f : 0.0f;
                t.r = (float)cs.rgb[0] + n + pit;
                t.g = (float)cs.rgb[1] + n + pit;
                t.b = (float)cs.rgb[2] + n + pit;
            } else if (mat == DECOR_MAT_TRIM) {
                LmSurfDNA ds = lmDeriveDadoSurf(gLimenDNA, 128);
                float grain = (ws.sheen > 138)
                    ? (perlin(px * 0.75f, py * 0.08f, 2) * 5.5f)
                    : (sinf((py + perlin(px * 0.05f, py * 0.02f, 2) * 14.0f) * 0.2f) * 6.5f);
                float darken = (gLimenDNA.dadoToneDelta < 0) ? 0.64f : 0.54f;
                t.r = (float)ds.rgb[0] * darken + 14.0f + grain;
                t.g = (float)ds.rgb[1] * darken + 13.0f + grain * 0.85f;
                t.b = (float)ds.rgb[2] * darken + 12.0f + grain * 0.70f;
            }
            size_t o = ((size_t)y * sz + (size_t)x) * 4;
            d[o + 0] = (unsigned char)(t.r < 0.0f ? 0 : (t.r > 255.0f ? 255 : (int)t.r));
            d[o + 1] = (unsigned char)(t.g < 0.0f ? 0 : (t.g > 255.0f ? 255 : (int)t.g));
            d[o + 2] = (unsigned char)(t.b < 0.0f ? 0 : (t.b > 255.0f ? 255 : (int)t.b));
            d[o + 3] = 128;
        }
    }
    if (texId == 0) glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz, sz, 0, GL_RGBA, GL_UNSIGNED_BYTE, d.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, 0x813D, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
}

inline void lmEnsureUnstructTextures() {
    if (lmUnstructTex[0] != 0) return;
    auto mkTex = [](GLuint& texId, auto&& pixelFn) {
        const int sz = 256;
        std::vector<unsigned char> d((size_t)sz * sz * 4);
        auto clampB = [](float c) -> unsigned char { return (unsigned char)(c < 0.0f ? 0 : (c > 255.0f ? 255 : (int)c)); };
        for (int y = 0; y < sz; y++) {
            for (int x = 0; x < sz; x++) {
                float r = 128.0f, g = 128.0f, b = 128.0f, h = 128.0f;
                pixelFn(x, y, sz, r, g, b, h);
                size_t o = ((size_t)y * sz + (size_t)x) * 4;
                d[o + 0] = clampB(r); d[o + 1] = clampB(g); d[o + 2] = clampB(b); d[o + 3] = clampB(h);
            }
        }
        if (texId == 0) glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz, sz, 0, GL_RGBA, GL_UNSIGNED_BYTE, d.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    };
    auto chash = [](int x, int y, unsigned int salt) -> float {
        unsigned int h = (unsigned int)x * 0x9E3779B1u ^ ((unsigned int)y * 0x85EBCA6Bu + salt);
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        return (float)(h & 0xFFFFu) / 65535.0f;
    };
    // 0: Smooth-matte pale-neutral monolith concrete
    mkTex(lmUnstructTex[UTEX_MONOLITH], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        float n1 = perlin(x * 0.045f, y * 0.045f, 3) * 14.0f;
        float n2 = perlin(x * 0.16f + 7.1f, y * 0.16f + 2.3f, 2) * 6.0f;
        float seam = ((y % (sz / 4)) < 2 || (x % (sz / 2)) < 2) ? -6.0f : 0.0f;
        float v = 132.0f + n1 + n2 + seam;
        r = v * 0.99f; g = v; b = v * 1.01f; h = 128.0f + n2 * 2.0f + seam * 2.0f;
    });
    // 1: Darker charcoal monolith variant for layered depth
    mkTex(lmUnstructTex[UTEX_MONOLITH_ALT], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        float n1 = perlin(x * 0.05f + 13.0f, y * 0.05f + 5.0f, 3) * 12.0f;
        float n2 = perlin(x * 0.18f, y * 0.18f, 2) * 5.0f;
        float seam = ((y % (sz / 4)) < 2 || (x % (sz / 2)) < 2) ? -5.0f : 0.0f;
        float v = 96.0f + n1 + n2 + seam;
        r = v * 0.98f; g = v; b = v * 1.02f; h = 128.0f + n2 * 2.0f;
    });
    // 2: Pale snowy ash ground (Minecraft/pixelated snow block feel)
    mkTex(lmUnstructTex[UTEX_SNOW], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)sz;
        int bx = x / 16, by = y / 16;
        float cell = (chash(bx, by, 101u) - 0.5f) * 14.0f;
        float drift = perlin(x * 0.04f, y * 0.025f, 3) * 14.0f;
        float v = 200.0f + cell + drift;
        r = v * 0.94f; g = v * 0.97f; b = v * 1.04f; h = 128.0f + cell * 1.5f;
    });
    // 3: Dark charred basalt needles & gnarled dark bark
    mkTex(lmUnstructTex[UTEX_BASALT], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)sz;
        int bx = x / 16, by = y / 16;
        float stria = perlin(x * 0.24f, y * 0.03f, 3) * 14.0f;
        float speck = (chash(bx, by, 202u) - 0.5f) * 10.0f;
        float v = 42.0f + stria + speck;
        r = v * 0.96f; g = v * 0.94f; b = v * 1.06f; h = 128.0f + stria * 2.5f;
    });
    // 4: Dark cavern stone
    mkTex(lmUnstructTex[UTEX_CAVE_ROCK], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)sz;
        int bx = x / 8, by = y / 8;
        float strata = perlin(x * 0.06f, y * 0.14f, 3) * 18.0f;
        float cr = (chash(bx, by, 303u) - 0.5f) * 12.0f;
        float v = 48.0f + strata + cr;
        r = v * 1.02f; g = v * 0.96f; b = v * 1.04f; h = 128.0f + strata * 2.2f + cr;
    });
    // 5: Vivid crimson foliage & fallen petal moss
    mkTex(lmUnstructTex[UTEX_CRIMSON_LEAF], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)sz;
        int bx = x / 8, by = y / 8;
        float rnd = chash(bx, by, 404u);
        float cl = perlin(x * 0.14f, y * 0.14f, 3);
        r = 148.0f + rnd * 46.0f + cl * 26.0f;
        g = 18.0f + rnd * 18.0f + (cl > 0.2f ? cl * 14.0f : 0.0f);
        b = 26.0f + rnd * 20.0f;
        h = 128.0f + (rnd - 0.5f) * 48.0f + cl * 24.0f;
    });
    // 6: Coarse brown soil / earth tendrils
    mkTex(lmUnstructTex[UTEX_EARTH], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)sz;
        int bx = x / 16, by = y / 16;
        float rnd = chash(bx, by, 505u);
        float pebble = chash(x / 8, y / 8, 509u);
        float tone = (rnd - 0.5f) * 28.0f + (pebble > 0.80f ? -18.0f : (pebble < 0.16f ? 14.0f : 0.0f));
        r = 112.0f + tone; g = 77.0f + tone * 0.72f; b = 52.0f + tone * 0.50f;
        h = 128.0f + tone * 1.5f;
    });
    // 7: Warm sunlit coarse soil / terracotta-earth accent
    mkTex(lmUnstructTex[UTEX_EARTH_WARM], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)sz;
        int bx = x / 16, by = y / 16;
        float rnd = chash(bx, by, 606u);
        float tone = (rnd - 0.5f) * 26.0f;
        r = 148.0f + tone; g = 96.0f + tone * 0.68f; b = 56.0f + tone * 0.42f;
        h = 128.0f + tone * 1.5f;
    });
    // 8: Golden dry grass tufts (greenish-olive root -> sunlit pale gold tips)
    mkTex(lmUnstructTex[UTEX_DRY_GRASS], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        float v = (float)y / (float)sz;
        float blade = sinf((float)x * 0.35f + chash(x / 6, 0, 707u) * 3.14f) * 18.0f;
        r = 152.0f + v * 68.0f + blade;
        g = 148.0f + v * 48.0f + blade * 0.85f;
        b = 64.0f  + v * 28.0f + blade * 0.35f;
        h = 128.0f + blade * 1.5f;
    });
    // 9: Neutral white sky tex (modulated smoothly in mainFS by lmUnstructSkyColor)
    mkTex(lmUnstructTex[UTEX_SKY], [&](int x, int y, int sz, float& r, float& g, float& b, float& h) {
        (void)x; (void)y; (void)sz;
        r = 255.0f; g = 255.0f; b = 255.0f; h = 128.0f;
    });
}

// Lazily synthesizes wall, floor, ceiling, decor atlas, and 3 vault pole textures whenever
// `(worldSeed, gCurrentLevel)` changes. Fixes `game_world_flow.h`'s stale `lastTextureLevel`.
inline void lmEnsureLevelTextures() {
    lmReset(worldSeed, gCurrentLevel);
    lmEnsureUnstructTextures();
    if (lmTexBuiltValid && lmTexBuiltSeed == worldSeed && lmTexBuiltLevel == gCurrentLevel &&
        wallTex != 0 && floorTex != 0 && ceilTex != 0 && decorTex != 0 && lmDadoTex != 0 && lmMegaTex[0] != 0) {
        return;
    }
    float seedF = (float)((worldSeed ^ (unsigned int)(gCurrentLevel * 2654435761u)) & 1023u) * 0.37f;
    lmUploadContinuousSurfaceTex(wallTex,   gLimenDNA.wallSurf,              1024, seedF + 1.1f);
    lmUploadContinuousSurfaceTex(floorTex,  gLimenDNA.floorSurf,              512, seedF + 7.3f);
    lmUploadContinuousSurfaceTex(ceilTex,   gLimenDNA.ceilSurf,               512, seedF + 13.7f);
    lmUploadContinuousSurfaceTex(lmDadoTex, lmDeriveDadoSurf(gLimenDNA, 128), 512, seedF + 17.9f);
    for (int p = 0; p < LM_MEGA_POLES; p++) {
        lmUploadContinuousSurfaceTex(lmMegaTex[p], gLimenDNA.megaPoles[p], 512, seedF + 23.0f + p * 9.1f);
    }
    lmRebuildLevelDecorAtlas(decorTex);
    lmTexBuiltSeed = worldSeed;
    lmTexBuiltLevel = gCurrentLevel;
    lmTexBuiltValid = true;
}

inline GLuint lmEnsureMegaTex(int pole) {
    lmEnsureLevelTextures();
    if (pole < 0 || pole >= LM_MEGA_POLES) pole = 0;
    return lmMegaTex[pole];
}

// 12-slot LRU texture cache for foreign-level `LZ_XENO` slices (256x256 RGBA, <0.8 ms per slice).
struct LmXenoTexSlot {
    unsigned int seed = 0;
    int foreignLevel = -999999999;
    unsigned long long lastUsed = 0;
    GLuint wallTex = 0, floorTex = 0, ceilTex = 0, dadoTex = 0;
};
inline LmXenoTexSlot gLmXenoTexCache[12];
inline unsigned long long gLmXenoTexClock = 0;

inline void lmEnsureXenoTextures(const LmXenograft& xeno, GLuint& outWall, GLuint& outFloor, GLuint& outCeil, GLuint& outDado) {
    for (int k = 0; k < 12; k++) {
        LmXenoTexSlot& sl = gLmXenoTexCache[k];
        if (sl.wallTex != 0 && sl.seed == worldSeed && sl.foreignLevel == xeno.foreignLevel) {
            sl.lastUsed = ++gLmXenoTexClock;
            outWall = sl.wallTex;
            outFloor = sl.floorTex;
            outCeil = sl.ceilTex;
            outDado = sl.dadoTex;
            return;
        }
    }
    int best = 0;
    for (int k = 1; k < 12; k++) {
        if (gLmXenoTexCache[k].wallTex == 0) { best = k; break; }
        if (gLmXenoTexCache[k].lastUsed < gLmXenoTexCache[best].lastUsed) best = k;
    }
    LmXenoTexSlot& sl = gLmXenoTexCache[best];
    LimenLevelDNA fdna = lmMakeLevelDNA(worldSeed, xeno.foreignLevel);
    float sf = (float)((worldSeed ^ (unsigned int)(xeno.foreignLevel * 2246822519u)) & 1023u) * 0.41f;
    lmUploadContinuousSurfaceTex(sl.wallTex,  xeno.wallSurf,              256, sf + 3.7f);
    lmUploadContinuousSurfaceTex(sl.floorTex, xeno.floorSurf,             256, sf + 11.9f);
    lmUploadContinuousSurfaceTex(sl.ceilTex,  fdna.ceilSurf,              256, sf + 19.3f);
    lmUploadContinuousSurfaceTex(sl.dadoTex,  lmDeriveDadoSurf(fdna, 128),256, sf + 27.1f);
    sl.seed = worldSeed;
    sl.foreignLevel = xeno.foreignLevel;
    sl.lastUsed = ++gLmXenoTexClock;
    outWall = sl.wallTex;
    outFloor = sl.floorTex;
    outCeil = sl.ceilTex;
    outDado = sl.dadoTex;
}
