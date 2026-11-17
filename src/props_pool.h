#pragma once
#include "decor_parts.h"

// ============================================================================
// POOL PROPS
// Tiled halls, changing rooms and pool decks. Themes: see prop_catalog.h.
// ============================================================================

// Sun lounger with a raised back rest.
inline void buildLoungeChairModel(DecorBuilder& root) {
    DecorBuilder b = root.sub(0.0f, -0.02f, 0.0f);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.26f * (float)side;
        b.box(x, 0.00f, 0.85f, 0.05f, 0.26f, 0.05f, DECOR_MAT_WHITE);
        b.box(x, 0.00f, -0.30f, 0.05f, 0.26f, 0.05f, DECOR_MAT_WHITE);
        b.box(x, 0.00f, -0.82f, 0.04f, 0.64f, 0.04f, DECOR_MAT_WHITE);
    }
    b.box(0.0f, 0.26f, 0.30f, 0.60f, 0.05f, 1.30f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.31f, 0.30f, 0.54f, 0.04f, 1.24f, DECOR_MAT_FABRIC);
    decorBeam(b, Vec3(0.0f, 0.30f, -0.35f), Vec3(0.0f, 0.78f, -0.88f), 0.60f, 0.05f, DECOR_MAT_WHITE);
    decorBeam(b, Vec3(0.0f, 0.345f, -0.36f), Vec3(0.0f, 0.79f, -0.85f), 0.54f, 0.04f, DECOR_MAT_FABRIC);
}

inline void buildStartingBlockModel(DecorBuilder& b) {
    b.taper(0.0f, 0.00f, 0.0f, 0.60f, 0.70f, 0.50f, 0.60f, 0.45f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.45f, 0.0f, 0.50f, 0.03f, 0.60f, DECOR_MAT_RUBBER);
    b.box(-0.20f, 0.48f, -0.27f, 0.03f, 0.25f, 0.03f, DECOR_MAT_METAL);
    b.box(0.20f, 0.48f, -0.27f, 0.03f, 0.25f, 0.03f, DECOR_MAT_METAL);
    b.box(0.0f, 0.73f, -0.27f, 0.43f, 0.03f, 0.03f, DECOR_MAT_METAL);
}

inline void buildLifeguardChairModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.34f * (float)sx, 0.00f, 0.40f * (float)sz, 0.06f, 1.50f, 0.06f, DECOR_MAT_WHITE);
        }
        b.box(0.34f * (float)sx, 0.70f, 0.0f, 0.04f, 0.06f, 0.74f, DECOR_MAT_WHITE);
        b.box(0.37f * (float)sx, 1.55f, -0.05f, 0.05f, 0.30f, 0.70f, DECOR_MAT_WHITE);
    }
    for (int i = 1; i <= 3; i++) {
        b.box(0.0f, 0.35f * (float)i, 0.40f, 0.62f, 0.04f, 0.08f, DECOR_MAT_WHITE);
    }
    b.box(0.0f, 1.50f, 0.0f, 0.80f, 0.05f, 0.92f, DECOR_MAT_WHITE);
    b.box(0.0f, 1.55f, -0.42f, 0.69f, 0.70f, 0.05f, DECOR_MAT_WHITE);
    b.box(0.0f, 1.55f, -0.06f, 0.66f, 0.08f, 0.66f, DECOR_MAT_ORANGE);
    // life ring on the side
    decorRod(b, Vec3(-0.41f, 1.05f, 0.0f), Vec3(-0.37f, 1.05f, 0.0f), 0.20f, DECOR_MAT_ORANGE, 12);
    decorRod(b, Vec3(-0.414f, 1.05f, 0.0f), Vec3(-0.41f, 1.05f, 0.0f), 0.11f, DECOR_MAT_WHITE, 8);
}

inline void buildTowelCartModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.36f * (float)sx, 0.06f, 0.20f * (float)sz, 0.06f, 0.04f);
            b.box(0.43f * (float)sx, 0.15f, 0.255f * (float)sz, 0.04f, 0.80f, 0.04f, DECOR_MAT_METAL);
        }
    }
    b.box(0.0f, 0.12f, 0.0f, 0.90f, 0.03f, 0.55f, DECOR_MAT_METAL);
    b.box(0.0f, 0.91f, -0.255f, 0.82f, 0.04f, 0.04f, DECOR_MAT_METAL);
    b.box(0.0f, 0.91f, 0.255f, 0.82f, 0.04f, 0.04f, DECOR_MAT_METAL);
    b.box(0.0f, 0.15f, 0.0f, 0.80f, 0.74f, 0.46f, DECOR_MAT_FABRIC);
    b.box(-0.18f, 0.89f, 0.0f, 0.36f, 0.10f, 0.34f, DECOR_MAT_WHITE);
    b.box(0.20f, 0.89f, 0.02f, 0.34f, 0.16f, 0.32f, DECOR_MAT_WHITE);
    b.box(0.465f, 0.80f, 0.0f, 0.03f, 0.04f, 0.47f, DECOR_MAT_RUBBER);
}

// Tiled shower panel with a floor tray.
inline void buildShowerPostModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, -0.17f, 0.90f, 2.20f, 0.16f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.00f, 0.08f, 0.90f, 0.03f, 0.34f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.03f, 0.08f, 0.30f, 0.006f, 0.20f, DECOR_MAT_DARK_METAL);
    b.cylinder(0.0f, 0.90f, -0.07f, 0.02f, 1.17f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(0.0f, 2.05f, -0.07f), Vec3(0.0f, 2.05f, 0.14f), 0.02f, DECOR_MAT_METAL, 6);
    b.cylinder(0.0f, 1.99f, 0.14f, 0.07f, 0.04f, DECOR_MAT_METAL, 8);
    b.box(0.0f, 1.05f, -0.07f, 0.14f, 0.14f, 0.04f, DECOR_MAT_METAL);
    decorRod(b, Vec3(0.0f, 1.12f, -0.05f), Vec3(0.0f, 1.12f, -0.01f), 0.035f, DECOR_MAT_DRUM, 8);
}

// Wall rack with life rings on pegs and a shelf of kickboards.
inline void buildFloatRackModel(DecorBuilder& b) {
    b.box(-0.57f, 0.00f, -0.13f, 0.06f, 0.30f, 0.04f, DECOR_MAT_METAL);
    b.box(0.57f, 0.00f, -0.13f, 0.06f, 0.30f, 0.04f, DECOR_MAT_METAL);
    b.box(0.0f, 0.30f, -0.13f, 1.20f, 1.30f, 0.04f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.32f * (float)side;
        decorRod(b, Vec3(x, 1.25f, -0.11f), Vec3(x, 1.25f, 0.10f), 0.02f, DECOR_MAT_METAL, 6);
        decorRod(b, Vec3(x, 1.07f, -0.05f), Vec3(x, 1.07f, 0.03f), 0.20f, DECOR_MAT_ORANGE, 12);
        decorRod(b, Vec3(x, 1.07f, 0.03f), Vec3(x, 1.07f, 0.034f), 0.11f, DECOR_MAT_WHITE, 8);
    }
    b.box(0.0f, 0.42f, 0.0f, 1.10f, 0.03f, 0.22f, DECOR_MAT_METAL);
    b.box(-0.25f, 0.45f, 0.0f, 0.46f, 0.20f, 0.20f, DECOR_MAT_DRUM);
    b.box(0.28f, 0.45f, 0.0f, 0.40f, 0.12f, 0.20f, DECOR_MAT_ORANGE);
}
