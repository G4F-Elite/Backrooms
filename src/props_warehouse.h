#pragma once
#include "decor_parts.h"

// ============================================================================
// WAREHOUSE PROPS
// Racking, pallets and handling gear. Themes: see prop_catalog.h.
// ============================================================================

// Two-level pallet rack. Stays below PROP_OVERHEAD_Y so pipes may run above it.
inline void buildPalletRackModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(1.16f * (float)sx, 0.00f, 0.41f * (float)sz, 0.08f, 2.55f, 0.08f, DECOR_MAT_DRUM);
        }
    }
    const float beamY[2] = {1.10f, 2.10f};
    for (int i = 0; i < 2; i++) {
        b.box(0.0f, beamY[i], -0.41f, 2.24f, 0.10f, 0.05f, DECOR_MAT_ORANGE);
        b.box(0.0f, beamY[i], 0.41f, 2.24f, 0.10f, 0.05f, DECOR_MAT_ORANGE);
        b.box(0.0f, beamY[i] + 0.10f, 0.0f, 2.24f, 0.03f, 0.80f, DECOR_MAT_WOOD);
    }
    // floor level: two loaded pallets
    decorPallet(b, -0.55f, 0.0f, 0.0f);
    decorPallet(b, 0.55f, 0.0f, 0.0f);
    b.box(-0.55f, DECOR_PALLET_H, 0.0f, 0.90f, 0.70f, 0.72f, DECOR_MAT_CARDBOARD);
    b.cylinder(0.31f, DECOR_PALLET_H, 0.0f, 0.22f, 0.70f, DECOR_MAT_DRUM, 10);
    b.cylinder(0.78f, DECOR_PALLET_H, 0.05f, 0.22f, 0.70f, DECOR_MAT_METAL, 10);
    // first and second deck
    b.box(-0.60f, 1.23f, 0.0f, 0.80f, 0.50f, 0.70f, DECOR_MAT_CARDBOARD);
    b.box(0.45f, 1.23f, 0.0f, 1.00f, 0.36f, 0.66f, DECOR_MAT_WOOD);
    b.box(-0.30f, 2.23f, 0.0f, 0.70f, 0.30f, 0.60f, DECOR_MAT_CARDBOARD);
    b.box(0.60f, 2.23f, 0.0f, 0.50f, 0.26f, 0.50f, DECOR_MAT_CARDBOARD);
}

// Shrink-wrapped load strapped to a pallet.
inline void buildWrappedPalletModel(DecorBuilder& b) {
    decorPallet(b, 0.0f, 0.0f, 0.0f);
    b.box(0.0f, DECOR_PALLET_H, 0.0f, 0.96f, 1.20f, 0.76f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.25f * (float)side, DECOR_PALLET_H, 0.0f, 0.05f, 1.205f, 0.77f, DECOR_MAT_RUBBER);
    }
    b.panel(0.0f, 0.80f, 0.383f, 0.24f, 0.18f, 0.006f, DECOR_MAT_CAUTION);
}

inline void buildLongCrateModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.60f * (float)side, 0.00f, 0.0f, 0.10f, 0.06f, 0.60f, DECOR_MAT_TRIM);
        b.box(0.76f * (float)side, 0.06f, 0.0f, 0.08f, 0.44f, 0.60f, DECOR_MAT_TRIM);
    }
    b.box(0.0f, 0.06f, 0.0f, 1.44f, 0.44f, 0.56f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.50f, 0.0f, 1.60f, 0.04f, 0.60f, DECOR_MAT_TRIM);
    DecorBuilder top = b.sub(-0.30f, 0.02f, 0.35f);
    top.box(0.0f, 0.54f, 0.0f, 0.44f, 0.34f, 0.40f, DECOR_MAT_WOOD);
    top.box(0.0f, 0.88f, 0.0f, 0.46f, 0.03f, 0.42f, DECOR_MAT_TRIM);
}

// Hand pallet truck: forks to the front, pump and tiller at the back.
inline void buildPalletJackModel(DecorBuilder& root) {
    DecorBuilder b = root.sub(0.0f, -0.04f, 0.0f);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.19f * (float)side;
        b.box(x, 0.00f, 0.70f, 0.10f, 0.04f, 0.08f, DECOR_MAT_RUBBER);
        b.box(x, 0.04f, 0.25f, 0.16f, 0.05f, 1.10f, DECOR_MAT_ORANGE);
        decorWheel(b, 0.10f * (float)side, 0.09f, -0.63f, 0.09f, 0.06f);
    }
    b.box(0.0f, 0.04f, -0.42f, 0.54f, 0.30f, 0.24f, DECOR_MAT_ORANGE);
    b.cylinder(0.0f, 0.34f, -0.42f, 0.06f, 0.26f, DECOR_MAT_METAL, 8);
    decorBeam(b, Vec3(0.0f, 0.60f, -0.42f), Vec3(0.0f, 1.14f, -0.64f), 0.04f, 0.04f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.12f, -0.65f, 0.36f, 0.04f, 0.04f, DECOR_MAT_RUBBER);
}

// Empty pallets stacked a little out of square.
inline void buildPalletPileModel(DecorBuilder& b) {
    const float twist[5] = {0.00f, 0.05f, -0.04f, 0.07f, -0.02f};
    const float shift[5] = {0.00f, 0.02f, -0.01f, 0.01f, -0.02f};
    for (int i = 0; i < 5; i++) {
        DecorBuilder layer = b.sub(shift[i], -shift[i], twist[i]);
        decorPallet(layer, 0.0f, DECOR_PALLET_H * (float)i, 0.0f);
    }
}
