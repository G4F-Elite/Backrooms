#pragma once
#include "decor_parts.h"

// ============================================================================
// HOTEL PROPS
// Guest rooms and the lobby. Themes: see prop_catalog.h.
// ============================================================================

// Single bed, headboard against the wall.
inline void buildBedModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, -1.02f, 1.04f, 1.10f, 0.06f, DECOR_MAT_TRIM);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.44f * (float)side, 0.00f, 0.95f, 0.08f, 0.10f, 0.08f, DECOR_MAT_TRIM);
        b.box(0.44f * (float)side, 0.00f, -0.90f, 0.08f, 0.10f, 0.08f, DECOR_MAT_TRIM);
    }
    b.box(0.0f, 0.10f, 0.03f, 1.00f, 0.20f, 2.04f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.30f, 0.03f, 0.96f, 0.18f, 2.00f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.48f, 0.33f, 0.98f, 0.03f, 1.40f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.48f, -0.70f, 0.60f, 0.10f, 0.36f, DECOR_MAT_WHITE);
}

inline void buildNightstandModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.20f;
    b.box(0.0f, 0.00f, cz, 0.40f, 0.06f, 0.36f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.06f, cz, 0.44f, 0.46f, 0.40f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.52f, 0.0f, 0.46f, 0.03f, 0.42f, DECOR_MAT_TRIM);
    for (int i = 0; i < 2; i++) {
        float y = 0.10f + 0.21f * (float)i;
        b.box(0.0f, y, f + 0.003f, 0.38f, 0.18f, 0.006f, DECOR_MAT_TRIM);
        b.box(0.0f, y + 0.08f, f + 0.011f, 0.04f, 0.03f, 0.010f, DECOR_MAT_METAL);
    }
    // table lamp
    b.cylinder(-0.08f, 0.55f, -0.04f, 0.06f, 0.02f, DECOR_MAT_METAL, 8);
    b.cylinder(-0.08f, 0.57f, -0.04f, 0.015f, 0.22f, DECOR_MAT_METAL, 6);
    b.taper(-0.08f, 0.79f, -0.04f, 0.22f, 0.22f, 0.14f, 0.14f, 0.18f, DECOR_MAT_WHITE);
}

inline void buildArmchairModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.32f * (float)sx, 0.00f, 0.32f * (float)sz, 0.06f, 0.08f, 0.06f, DECOR_MAT_TRIM);
        }
    }
    b.box(0.0f, 0.08f, 0.0f, 0.80f, 0.20f, 0.80f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.28f, -0.33f, 0.80f, 0.57f, 0.14f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.28f, 0.07f, 0.52f, 0.14f, 0.66f, DECOR_MAT_FABRIC);
    b.box(-0.33f, 0.28f, 0.07f, 0.14f, 0.28f, 0.66f, DECOR_MAT_FABRIC);
    b.box(0.33f, 0.28f, 0.07f, 0.14f, 0.28f, 0.66f, DECOR_MAT_FABRIC);
}

// Bellhop cart with a few cases on the deck.
inline void buildLuggageCartModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.45f * (float)sx, 0.07f, 0.22f * (float)sz, 0.07f, 0.04f);
            b.box(0.52f * (float)sx, 0.18f, 0.27f * (float)sz, 0.03f, 1.50f, 0.03f, DECOR_MAT_METAL);
        }
    }
    b.box(0.0f, 0.14f, 0.0f, 1.10f, 0.04f, 0.60f, DECOR_MAT_METAL);
    b.box(0.0f, 0.18f, 0.0f, 1.00f, 0.012f, 0.50f, DECOR_MAT_FABRIC);
    b.box(0.0f, 1.68f, -0.27f, 1.07f, 0.03f, 0.03f, DECOR_MAT_METAL);
    b.box(0.0f, 1.68f, 0.27f, 1.07f, 0.03f, 0.03f, DECOR_MAT_METAL);
    b.box(-0.52f, 1.68f, 0.0f, 0.03f, 0.03f, 0.51f, DECOR_MAT_METAL);
    b.box(0.52f, 1.68f, 0.0f, 0.03f, 0.03f, 0.51f, DECOR_MAT_METAL);
    b.box(-0.20f, 0.192f, 0.0f, 0.50f, 0.22f, 0.36f, DECOR_MAT_TRIM);
    b.box(-0.20f, 0.412f, 0.0f, 0.44f, 0.18f, 0.32f, DECOR_MAT_FABRIC);
    b.box(0.28f, 0.192f, 0.02f, 0.30f, 0.46f, 0.20f, DECOR_MAT_DARK_METAL);
}

// Counter: guests stand at the front (+Z), staff work behind it.
inline void buildReceptionDeskModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.0f, 2.16f, 0.08f, 0.62f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, 0.26f, 2.20f, 0.98f, 0.10f, DECOR_MAT_WOOD);
    b.box(0.0f, 1.06f, 0.22f, 2.20f, 0.04f, 0.26f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.20f, 0.313f, 1.80f, 0.70f, 0.006f, DECOR_MAT_WHITE);
    b.box(-1.08f, 0.08f, -0.07f, 0.04f, 0.64f, 0.56f, DECOR_MAT_WOOD);
    b.box(1.08f, 0.08f, -0.07f, 0.04f, 0.64f, 0.56f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.72f, -0.07f, 2.20f, 0.04f, 0.56f, DECOR_MAT_WOOD);
    // terminal on the work surface, bell on the counter
    b.box(-0.50f, 0.76f, -0.10f, 0.20f, 0.02f, 0.14f, DECOR_MAT_DARK_METAL);
    b.box(-0.50f, 0.78f, -0.12f, 0.36f, 0.26f, 0.04f, DECOR_MAT_DARK_METAL);
    b.panel(-0.50f, 0.80f, -0.143f, 0.30f, 0.22f, 0.006f, DECOR_MAT_SCREEN);
    b.cylinder(0.50f, 1.10f, 0.22f, 0.05f, 0.03f, DECOR_MAT_METAL, 8);
}

inline void buildWardrobeModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.30f;
    b.box(0.0f, 0.00f, cz, 1.16f, 0.08f, 0.56f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.08f, cz, 1.20f, 1.86f, 0.60f, DECOR_MAT_WOOD);
    b.box(0.0f, 1.94f, 0.0f, 1.24f, 0.06f, 0.62f, DECOR_MAT_TRIM);
    for (int side = -1; side <= 1; side += 2) {
        b.panel(0.295f * (float)side, 0.12f, f + 0.003f, 0.57f, 1.78f, 0.006f, DECOR_MAT_DOOR);
        b.box(0.06f * (float)side, 0.95f, f + 0.012f, 0.02f, 0.16f, 0.012f, DECOR_MAT_METAL);
    }
}
