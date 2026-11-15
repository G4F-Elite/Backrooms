#pragma once
#include <glad/glad.h>
#include <cmath>
#include "textures.h"
#include "decor_builder.h"

// ============================================================================
// DECOR ATLAS
// One 4x4 material atlas for every built structure (props, POI sets, door
// frames, exit portal). Tiles never repeat, so the mesh builder maps faces
// into a single tile (see decor_builder.h).
// ============================================================================

struct DecorTexel {
    float r, g, b;
};

inline DecorTexel decorAtlasTexel(int mat, float u, float v, float px, float py, bool parking) {
    float fine = perlin(px * 0.55f, py * 0.55f, 3);
    float broad = perlin(px * 0.06f + 3.0f, py * 0.06f + 9.0f, 4);
    DecorTexel t = {128.0f, 128.0f, 128.0f};

    if (mat == DECOR_MAT_WOOD) {
        float plank = fmodf(v * 4.0f, 1.0f);
        float gap = (plank < 0.06f) ? -38.0f : 0.0f;
        float grain = sinf((px + perlin(py * 0.05f, px * 0.02f, 2) * 18.0f) * 0.16f) * 10.0f;
        float nail = (fabsf(u - 0.08f) < 0.012f || fabsf(u - 0.92f) < 0.012f) && fabsf(plank - 0.5f) < 0.06f ? -30.0f : 0.0f;
        t.r = 132.0f + grain + fine * 8.0f + gap + nail;
        t.g = 98.0f + grain * 0.75f + fine * 6.0f + gap + nail;
        t.b = 60.0f + grain * 0.4f + fine * 4.0f + gap * 0.8f + nail;
    } else if (mat == DECOR_MAT_CARDBOARD) {
        float flute = sinf(px * 0.9f) * 2.5f;
        float tape = fabsf(v - 0.5f) < 0.07f ? 22.0f : 0.0f;
        float stain = broad > 0.35f ? (broad - 0.35f) * -40.0f : 0.0f;
        t.r = 166.0f + flute + fine * 7.0f + tape + stain;
        t.g = 132.0f + flute + fine * 6.0f + tape * 0.9f + stain;
        t.b = 88.0f + flute * 0.6f + fine * 4.0f + tape * 0.5f + stain;
    } else if (mat == DECOR_MAT_METAL) {
        float scratch = perlin(px * 0.9f, py * 0.05f, 2) * 6.0f;
        float rust = broad > 0.5f ? (broad - 0.5f) * 70.0f : 0.0f;
        t.r = 122.0f + scratch + fine * 4.0f + rust * 0.5f;
        t.g = 130.0f + scratch + fine * 4.0f - rust * 0.25f;
        t.b = 128.0f + scratch + fine * 4.0f - rust * 0.6f;
    } else if (mat == DECOR_MAT_DARK_METAL) {
        float vent = (fmodf(v * 16.0f, 1.0f) < 0.3f && u > 0.15f && u < 0.85f) ? -14.0f : 0.0f;
        t.r = 46.0f + fine * 5.0f + vent;
        t.g = 48.0f + fine * 5.0f + vent;
        t.b = 52.0f + fine * 5.0f + vent;
    } else if (mat == DECOR_MAT_ORANGE) {
        float dirt = broad * 16.0f;
        t.r = 214.0f + fine * 6.0f - fabsf(dirt);
        t.g = 96.0f + fine * 5.0f - fabsf(dirt) * 0.7f;
        t.b = 30.0f + fine * 3.0f;
    } else if (mat == DECOR_MAT_PLASTER) {
        if (parking) {
            float rough = perlin(px * 0.2f, py * 0.2f, 4) * 14.0f;
            t.r = 116.0f + rough;
            t.g = 118.0f + rough;
            t.b = 122.0f + rough;
        } else {
            float stripe = sinf(u * 6.28318f * 4.0f) * 4.0f;
            float stain = broad > 0.3f ? (broad - 0.3f) * -30.0f : 0.0f;
            t.r = 186.0f + stripe + fine * 5.0f + stain;
            t.g = 166.0f + stripe + fine * 5.0f + stain;
            t.b = 84.0f + stripe * 0.5f + fine * 3.0f + stain * 0.6f;
        }
    } else if (mat == DECOR_MAT_RUBBER) {
        t.r = 30.0f + fine * 6.0f;
        t.g = 30.0f + fine * 6.0f;
        t.b = 32.0f + fine * 6.0f;
    } else if (mat == DECOR_MAT_WHITE) {
        float grime = broad > 0.25f ? (broad - 0.25f) * -34.0f : 0.0f;
        t.r = 214.0f + fine * 4.0f + grime;
        t.g = 212.0f + fine * 4.0f + grime;
        t.b = 202.0f + fine * 4.0f + grime * 1.2f;
    } else if (mat == DECOR_MAT_DRUM) {
        float rib = (fabsf(v - 0.33f) < 0.025f || fabsf(v - 0.66f) < 0.025f) ? 22.0f : 0.0f;
        float rust = broad > 0.4f ? (broad - 0.4f) * 120.0f : 0.0f;
        t.r = 38.0f + fine * 5.0f + rib + rust;
        t.g = 78.0f + fine * 5.0f + rib + rust * 0.3f;
        t.b = 128.0f + fine * 6.0f + rib - rust * 0.6f;
    } else if (mat == DECOR_MAT_FABRIC) {
        float weave = (sinf(px * 1.6f) + sinf(py * 1.6f)) * 3.5f;
        t.r = 92.0f + weave + fine * 5.0f + broad * 8.0f;
        t.g = 104.0f + weave + fine * 5.0f + broad * 8.0f;
        t.b = 112.0f + weave + fine * 5.0f + broad * 8.0f;
    } else if (mat == DECOR_MAT_SCREEN) {
        float cu = fmodf(u * 8.0f, 1.0f);
        float cv = fmodf(v * 8.0f, 1.0f);
        bool led = fabsf(cu - 0.5f) < 0.12f && fabsf(cv - 0.5f) < 0.12f;
        int cell = (int)(u * 8.0f) * 7 + (int)(v * 8.0f) * 13;
        t.r = led ? ((cell % 5 == 0) ? 230.0f : 40.0f) : 18.0f;
        t.g = led ? ((cell % 5 == 0) ? 150.0f : 220.0f) : 22.0f;
        t.b = led ? 60.0f : 24.0f;
    } else if (mat == DECOR_MAT_CAUTION) {
        bool dark = fmodf((u + v) * 5.0f, 1.0f) < 0.5f;
        float wear = fine * 8.0f + broad * 10.0f;
        t.r = (dark ? 34.0f : 220.0f) + wear;
        t.g = (dark ? 34.0f : 176.0f) + wear;
        t.b = (dark ? 36.0f : 28.0f) + wear * 0.5f;
    } else if (mat == DECOR_MAT_PUDDLE) {
        float ripple = perlin(px * 0.08f, py * 0.08f, 3) * 10.0f;
        t.r = 34.0f + ripple;
        t.g = 36.0f + ripple;
        t.b = 30.0f + ripple;
    } else if (mat == DECOR_MAT_CEILTILE) {
        float pit = perlin(px * 0.8f + 5.0f, py * 0.8f + 1.0f, 2) > 0.45f ? -26.0f : 0.0f;
        float stain = broad > 0.2f ? (broad - 0.2f) * -50.0f : 0.0f;
        t.r = 198.0f + pit + stain;
        t.g = 192.0f + pit + stain * 1.1f;
        t.b = 172.0f + pit + stain * 1.5f;
    } else if (mat == DECOR_MAT_DOOR) {
        bool panelU = u > 0.16f && u < 0.84f;
        bool upper = panelU && v > 0.56f && v < 0.92f;
        bool lower = panelU && v > 0.08f && v < 0.48f;
        float inset = (upper || lower) ? -14.0f : 0.0f;
        float grime = broad * 12.0f;
        t.r = 112.0f + inset + fine * 4.0f + grime;
        t.g = 118.0f + inset + fine * 4.0f + grime;
        t.b = 104.0f + inset + fine * 4.0f + grime * 0.6f;
    } else {
        // DECOR_MAT_TRIM: dark stained frame timber
        float grain = sinf((py + perlin(px * 0.05f, py * 0.02f, 2) * 14.0f) * 0.2f) * 6.0f;
        t.r = 70.0f + grain + fine * 4.0f;
        t.g = 50.0f + grain * 0.7f + fine * 3.0f;
        t.b = 34.0f + grain * 0.4f + fine * 2.0f;
    }
    return t;
}

inline GLuint genDecorAtlasTex() {
    const int tile = 256;
    const int sz = tile * DECOR_ATLAS_GRID;
    bool parking = isParkingLevel(gCurrentLevel);
    unsigned char* d = new unsigned char[sz * sz * 4];
    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            int mat = (y / tile) * DECOR_ATLAS_GRID + (x / tile);
            float px = (float)(x % tile);
            float py = (float)(y % tile);
            DecorTexel t = decorAtlasTexel(mat, px / (float)tile, py / (float)tile, px, py, parking);
            unsigned char* o = &d[(y * sz + x) * 4];
            o[0] = (unsigned char)(t.r < 0 ? 0 : (t.r > 255 ? 255 : (int)t.r));
            o[1] = (unsigned char)(t.g < 0 ? 0 : (t.g > 255 ? 255 : (int)t.g));
            o[2] = (unsigned char)(t.b < 0 ? 0 : (t.b > 255 ? 255 : (int)t.b));
            o[3] = 128; // flat height: no parallax shift across tile borders
        }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz, sz, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Stop the mip chain while a tile is still 16 px wide, so neighbours do not bleed in.
    glTexParameteri(GL_TEXTURE_2D, 0x813D, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] d;
    return tex;
}
