#pragma once
// Procedural low-fi textures for creature/player meshes.
// The image is split into vertical material strips (see CreatureMesh::strip); every strip tiles vertically.
#include <cmath>

enum CreaturePattern {
    CREATURE_PAT_PLAIN = 0,  // flat colour with grain
    CREATURE_PAT_SKIN,       // pallid, blotchy
    CREATURE_PAT_SHIRT,      // faint pinstripe, grime towards the folds
    CREATURE_PAT_WEAVE       // coarse cloth
};

struct CreatureStripStyle {
    float r, g, b;
    float grain;
    int pattern;
};

inline float creatureHash(int x, int y, unsigned int seed) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}

// Value noise that wraps every `period` cells on both axes.
inline float creatureNoise(float x, float y, int period, unsigned int seed) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - (float)x0, fy = y - (float)y0;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    auto wrap = [period](int a) { return ((a % period) + period) % period; };
    float a = creatureHash(wrap(x0), wrap(y0), seed);
    float b = creatureHash(wrap(x0 + 1), wrap(y0), seed);
    float c = creatureHash(wrap(x0), wrap(y0 + 1), seed);
    float d = creatureHash(wrap(x0 + 1), wrap(y0 + 1), seed);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

inline void genCreatureTexture(const CreatureStripStyle* styles, int strips, unsigned char* data, int w, int h, unsigned int seed) {
    if (strips < 1) strips = 1;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int si = x * strips / w;
            if (si >= strips) si = strips - 1;
            const CreatureStripStyle& st = styles[si];
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;
            float grain = (creatureHash(x, y, seed) - 0.5f) * 2.0f * st.grain;
            float blotch = creatureNoise(u * 16.0f, v * 8.0f, 8, seed + 5u);
            float fine = creatureNoise(u * 48.0f, v * 24.0f, 24, seed + 9u);
            float mul = 1.0f;
            float r = st.r, g = st.g, b = st.b;
            if (st.pattern == CREATURE_PAT_SKIN) {
                // Uneven circulation: dull grey-violet patches under thin skin.
                mul = 0.84f + 0.22f * blotch + 0.06f * fine;
                float bruise = blotch < 0.36f ? (0.36f - blotch) / 0.36f : 0.0f;
                r -= bruise * 20.0f;
                g -= bruise * 24.0f;
                b -= bruise * 12.0f;
            } else if (st.pattern == CREATURE_PAT_SHIRT) {
                float stripe = ((x % 5) == 0) ? 0.90f : 1.0f;
                mul = stripe * (0.80f + 0.22f * blotch);
                float stain = fine * blotch;
                if (stain < 0.12f) {
                    r *= 0.74f;
                    g *= 0.70f;
                    b *= 0.62f;
                }
            } else if (st.pattern == CREATURE_PAT_WEAVE) {
                float weave = (((x + y) & 1) == 0) ? 1.0f : 0.84f;
                mul = weave * (0.82f + 0.24f * blotch);
            } else {
                mul = 0.9f + 0.2f * blotch;
            }
            r = r * mul + grain;
            g = g * mul + grain;
            b = b * mul + grain;
            if (r < 0.0f) r = 0.0f;
            if (g < 0.0f) g = 0.0f;
            if (b < 0.0f) b = 0.0f;
            if (r > 255.0f) r = 255.0f;
            if (g > 255.0f) g = 255.0f;
            if (b > 255.0f) b = 255.0f;
            int idx = (y * w + x) * 4;
            data[idx] = (unsigned char)r;
            data[idx + 1] = (unsigned char)g;
            data[idx + 2] = (unsigned char)b;
            data[idx + 3] = 255;
        }
    }
}
