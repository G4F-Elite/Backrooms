#pragma once

// Continuous surface synthesizer: generates seamless RGBA8 + heightmap-alpha textures from
// `LmSurfDNA` without discrete material enums. Tile, wallpaper, board-formed concrete,
// smooth plaster, carpet, and acoustic panels emerge continuously from `sf`.
inline void lmUploadContinuousSurfaceTex(GLuint& texId, const LmSurfDNA& sf, int sz, float seedOfs) {
    std::vector<unsigned char> d((size_t)sz * sz * 4);
    auto clampB = [](float c) -> unsigned char {
        return (unsigned char)(c < 0.0f ? 0.0f : (c > 255.0f ? 255.0f : c));
    };

    int repX = 1 + (int)sf.seamFreqX * 15 / 255; // integer repeats [1..16] for seamless GL_REPEAT
    int repY = 1 + (int)sf.seamFreqY * 15 / 255;
    // Second-pigment patterns (surfaces without one, e.g. the vault poles, keep rgb2 = 0)
    const bool  two       = ((int)sf.rgb2[0] + (int)sf.rgb2[1] + (int)sf.rgb2[2]) > 0;
    const float checkAmt  = two ? (float)sf.checker / 255.0f : 0.0f;
    const float plankAmt  = (float)sf.plank / 255.0f;
    const float speckAmt  = two ? (float)sf.speckle / 255.0f : 0.0f;
    const float motifAmt  = two ? (float)sf.motif / 255.0f : 0.0f;
    const float stripeMix = (two && sf.stripe > 150) ? fminf((float)(sf.stripe - 150) / 60.0f, 1.0f) : 0.0f;
    if (checkAmt > 0.15f) { repX += repX & 1; repY += repY & 1; }   // parity has to wrap
    const int   motifRep  = 2 * (repX < 8 ? repX : 8);              // half-drop needs an even count
    const bool  fibreU    = repY > repX;                             // fibre runs along the long side
    const int   chip      = sz >= 512 ? sz / 256 : 1;
    const int   chipN     = sz / chip;
    const float r2R = (float)sf.rgb2[0], r2G = (float)sf.rgb2[1], r2B = (float)sf.rgb2[2];
    auto cellHash = [](int a, int b, int c) -> float {
        unsigned int h = (unsigned int)a * 0x9E3779B1u ^ ((unsigned int)b * 0x85EBCA6Bu + 0x27D4EB2Fu) ^ ((unsigned int)c * 0xC2B2AE35u);
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        return (float)(h & 0xFFFFu) / 65535.0f;
    };
    const int hashSalt = (int)(seedOfs * 97.0f);
    const float seamAmt   = sf.seamDepth > 28 ? (float)(sf.seamDepth - 28) / 227.0f : 0.0f;
    const float stripeAmt = (float)sf.stripe / 255.0f;
    const float grainAmt  = (float)sf.grain / 255.0f;
    const float grimeAmt  = (float)sf.grime / 255.0f;
    const float sheenAmt  = (float)sf.sheen / 255.0f;
    const int   stripeRep = 2 + ((int)sf.stripe / 24) * 2; // even harmonic count
    const float stagger   = (sf.bond >= 128 || (!two && sf.seamFreqX != sf.seamFreqY && sf.stripe > 110)) ? 0.5f : 0.0f;
    const float gw        = 0.014f + 0.022f * seamAmt;
    const float baseR = (float)sf.rgb[0], baseG = (float)sf.rgb[1], baseB = (float)sf.rgb[2];

    for (int y = 0; y < sz; y++) {
        float v = (float)y / (float)sz;
        float pv = v * (float)repY;
        float rowIdx = floorf(pv);
        float fv = pv - rowIdx;
        float dv = fminf(fv, 1.0f - fv);
        float bandV = cosf(v * 6.2831853f * (float)(stripeRep / 2 + 2));

        for (int x = 0; x < sz; x++) {
            float u = (float)x / (float)sz;
            float pu = u * (float)repX + (fmodf(rowIdx, 2.0f) > 0.5f ? stagger : 0.0f);
            float colIdx = floorf(pu);
            float fu = pu - colIdx;
            if (colIdx >= (float)repX) colIdx -= (float)repX;   // a shifted block wraps onto itself
            float du = fminf(fu, 1.0f - fu);

            // 1. Continuous joint/grout network + per-block tonal drift
            float edgeD = fminf(du, dv);
            float groove = (edgeD < gw) ? (1.0f - edgeD / gw) * seamAmt : 0.0f;
            float cellTone = noise2d(colIdx * 19.0f + seedOfs, rowIdx * 29.0f + seedOfs * 0.7f) * (10.0f * seamAmt);
            float blockRnd = cellHash((int)colIdx, (int)rowIdx, hashSalt);
            cellTone += (blockRnd - 0.5f) * 44.0f * plankAmt;

            // Tie holes on wide panels vs subtle diamond/woven accent on patterned surfaces
            float detail = 0.0f;
            if (repX <= 4 && seamAmt > 0.35f) {
                float cx = fminf(fabsf(fu - 0.16f), fabsf(fu - 0.84f));
                float cy = fminf(fabsf(fv - 0.24f), fabsf(fv - 0.76f));
                float dHole = sqrtf(cx * cx + cy * cy);
                if (dHole < 0.022f) detail -= (1.0f - dHole / 0.022f) * 32.0f * seamAmt;
            } else if (stripeAmt > 0.35f && seamAmt < 0.45f) {
                float dmd = 1.0f - (fabsf(fu - 0.5f) + fabsf(fv - 0.5f)) * 2.0f;
                if (dmd > 0.0f) detail += dmd * 11.0f * stripeAmt * (1.0f - seamAmt);
            }

            // 2. Periodic striping / ribbing / weave
            float wave = sinf(u * 6.2831853f * (float)stripeRep);
            float band = (wave * 9.5f + bandV * 3.5f) * stripeAmt * (1.0f - seamAmt * 0.45f) * (1.0f - stripeMix * 0.6f);

            // 3. Multi-scale aggregate / fiber grain
            float rough = perlin(x * 0.09f + seedOfs, y * 0.09f + seedOfs, 3);
            float fine  = perlin(x * 0.42f + 11.0f, y * 0.42f + 3.0f, 2);
            float texGrain = (rough * 19.0f + fine * 9.0f) * grainAmt;
            if (plankAmt > 0.02f) {
                // wood fibre: stretched along the board, periodic along it, restarted per board
                float a = fibreU ? (float)x : (float)y, c = fibreU ? (float)y : (float)x;
                float t = a / (float)sz, ofs = blockRnd * 211.0f;
                float fib = perlin((a + (float)sz) * 0.011f + ofs, c * 0.38f + ofs, 3) * (1.0f - t) +
                            perlin(a * 0.011f + ofs, c * 0.38f + ofs, 3) * t;
                texGrain = texGrain * (1.0f - 0.6f * plankAmt) + fib * 30.0f * plankAmt;
            }

            // 4. Grime blotches & vertical damp streaks
            float blotch = perlin(x * 0.016f + seedOfs, y * 0.014f + 5.0f, 3);
            float stain  = blotch > 0.25f ? (blotch - 0.25f) * -42.0f * grimeAmt : 0.0f;
            float drip   = perlin(x * 0.05f + 4.0f, y * 0.008f + seedOfs, 2);
            if (drip > 0.32f) stain -= (drip - 0.32f) * 28.0f * grimeAmt;

            // 5. Second pigment: checker blocks, wallpaper stripes, half-drop ornament, chips
            float mix2 = 0.0f, chipDark = 0.0f;
            if (checkAmt > 0.0f && (((int)colIdx + (int)rowIdx) & 1)) mix2 = checkAmt;
            if (stripeMix > 0.0f) {
                float sq = wave * 2.2f + 0.5f;
                sq = sq < 0.0f ? 0.0f : (sq > 1.0f ? 1.0f : sq);
                mix2 = fmaxf(mix2, sq * stripeMix * 0.9f);
            }
            if (motifAmt > 0.0f) {
                float mu = u * (float)motifRep;
                int mc = (int)mu;
                float mvv = v * (float)motifRep + ((mc & 1) ? 0.5f : 0.0f);
                float lu = mu - (float)mc - 0.5f, lv = mvv - floorf(mvv) - 0.5f;
                float au = fabsf(lu), av = fabsf(lv), m = 0.0f;
                auto edge = [](float d, float r0) { float e = (r0 - d) / 0.035f; return e < 0.0f ? 0.0f : (e > 1.0f ? 1.0f : e); };
                if (sf.motifKind < 85) {
                    m = edge(sqrtf(lu * lu + lv * lv), 0.19f);                               // dots
                } else if (sf.motifKind < 170) {
                    float dd = au + av;
                    m = fmaxf(edge(dd, 0.36f) - edge(dd, 0.25f), edge(dd, 0.10f));           // diamond ring + pip
                } else {
                    float pu2 = au - 0.17f, pv2 = av - 0.17f;                                 // quatrefoil
                    m = fmaxf(edge(sqrtf(pu2 * pu2 + lv * lv), 0.14f), edge(sqrtf(lu * lu + pv2 * pv2), 0.14f));
                    m = fmaxf(m - edge(sqrtf(lu * lu + lv * lv), 0.07f), 0.0f);
                }
                mix2 = fmaxf(mix2, m * motifAmt * 0.9f);
            }
            if (speckAmt > 0.0f) {
                float hc = cellHash((x / chip) % chipN, (y / chip) % chipN, hashSalt + 7);
                float pc = 0.04f + 0.20f * speckAmt;
                if (hc < pc) mix2 = fmaxf(mix2, 0.55f + 0.45f * speckAmt);
                else if (hc > 1.0f - pc * 0.5f) chipDark = 20.0f * speckAmt;
            }
            mix2 *= (1.0f - groove);

            float delta = cellTone + detail + band + texGrain - groove * 34.0f - chipDark;
            float r = baseR + (r2R - baseR) * mix2 + delta + stain * 0.85f;
            float g = baseG + (r2G - baseG) * mix2 + delta * 0.97f + stain * 0.92f;
            float b = baseB + (r2B - baseB) * mix2 + delta * 0.92f + stain * 0.96f;
            float h = 128.0f + texGrain * (0.65f - 0.40f * sheenAmt) + detail * 0.8f + band * 0.35f - groove * 34.0f;

            size_t o = ((size_t)y * sz + (size_t)x) * 4;
            d[o + 0] = clampB(r);
            d[o + 1] = clampB(g);
            d[o + 2] = clampB(b);
            d[o + 3] = clampB(h);
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
}
