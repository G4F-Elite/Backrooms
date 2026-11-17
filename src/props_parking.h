#pragma once
#include "decor_parts.h"

// ============================================================================
// PARKING PROPS
// Traffic furniture for garages and loading yards. Themes: see prop_catalog.h.
// DECOR_MAT_PLASTER follows the level's wall finish (concrete on parking levels).
// ============================================================================

inline void buildJerseyBarrierModel(DecorBuilder& b) {
    b.taper(0.0f, 0.00f, 0.0f, 2.00f, 0.60f, 2.00f, 0.44f, 0.18f, DECOR_MAT_PLASTER);
    b.taper(0.0f, 0.18f, 0.0f, 2.00f, 0.44f, 2.00f, 0.20f, 0.50f, DECOR_MAT_PLASTER);
    b.box(0.0f, 0.68f, 0.0f, 2.00f, 0.04f, 0.20f, DECOR_MAT_PLASTER);
    b.box(0.0f, 0.72f, 0.0f, 2.00f, 0.08f, 0.20f, DECOR_MAT_CAUTION);
    b.box(0.0f, 0.80f, 0.0f, 2.00f, 0.03f, 0.20f, DECOR_MAT_PLASTER);
}

inline void buildBollardModel(DecorBuilder& b, float x) {
    b.cylinder(x, 0.00f, 0.0f, 0.11f, 0.04f, DECOR_MAT_DARK_METAL, 8);
    b.cylinder(x, 0.04f, 0.0f, 0.08f, 0.66f, DECOR_MAT_ORANGE, 8);
    b.cylinder(x, 0.70f, 0.0f, 0.08f, 0.18f, DECOR_MAT_WHITE, 8);
    b.cylinder(x, 0.88f, 0.0f, 0.085f, 0.05f, DECOR_MAT_DARK_METAL, 8);
}

inline void buildBollardRowModel(DecorBuilder& b) {
    for (int i = -1; i <= 1; i++) buildBollardModel(b, 0.75f * (float)i);
}

// hub = dark wheel well, only needed where the top of the tire is visible
inline void buildTireModel(DecorBuilder& b, float x, float y, float z, bool hub) {
    b.cylinder(x, y, z, 0.32f, 0.22f, DECOR_MAT_RUBBER, 10);
    if (hub) b.cylinder(x, y + 0.22f, z, 0.17f, 0.004f, DECOR_MAT_DARK_METAL, 8);
}

inline void buildTireStackModel(DecorBuilder& b) {
    const float wobbleX[4] = {0.00f, 0.02f, -0.01f, 0.01f};
    const float wobbleZ[4] = {0.00f, -0.01f, 0.02f, 0.00f};
    for (int i = 0; i < 4; i++) {
        buildTireModel(b, -0.30f + wobbleX[i], 0.22f * (float)i, -0.12f + wobbleZ[i], i == 3);
    }
    buildTireModel(b, 0.30f, 0.00f, 0.14f, true);
}

inline void buildPayStationModel(DecorBuilder& b) {
    const float cz = -0.03f;
    const float f = cz + 0.16f;
    b.box(0.0f, 0.00f, cz + 0.02f, 0.54f, 0.10f, 0.36f, DECOR_MAT_CAUTION);
    b.box(0.0f, 0.10f, cz, 0.50f, 1.40f, 0.32f, DECOR_MAT_METAL);
    b.box(0.0f, 1.50f, cz + 0.03f, 0.54f, 0.06f, 0.38f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 1.10f, f + 0.003f, 0.30f, 0.22f, 0.006f, DECOR_MAT_SCREEN);
    b.box(0.0f, 0.86f, f + 0.008f, 0.20f, 0.16f, 0.016f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.70f, f + 0.006f, 0.24f, 0.04f, 0.012f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.40f, f + 0.03f, 0.26f, 0.10f, 0.06f, DECOR_MAT_DARK_METAL);
}

// Low kerb at the head of a parking bay; walked over, not around.
inline void buildWheelStopModel(DecorBuilder& b) {
    b.taper(0.0f, 0.00f, 0.0f, 1.80f, 0.18f, 1.76f, 0.10f, 0.10f, DECOR_MAT_CAUTION);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.60f * (float)side, 0.10f, 0.0f, 0.05f, 0.012f, 0.05f, DECOR_MAT_DARK_METAL);
    }
}

// Boom gate: drive cabinet on the left, arm resting on a post on the right.
inline void buildBarrierGateModel(DecorBuilder& root) {
    DecorBuilder b = root.sub(0.035f, -0.025f, 0.0f);
    b.box(-1.10f, 0.00f, 0.0f, 0.30f, 1.05f, 0.30f, DECOR_MAT_ORANGE);
    b.box(-1.10f, 1.05f, 0.0f, 0.34f, 0.04f, 0.34f, DECOR_MAT_DARK_METAL);
    b.box(-1.10f, 0.87f, 0.16f, 0.20f, 0.20f, 0.02f, DECOR_MAT_DARK_METAL);
    b.box(0.05f, 0.92f, 0.19f, 2.30f, 0.10f, 0.04f, DECOR_MAT_CAUTION);
    b.box(1.12f, 0.00f, 0.19f, 0.06f, 0.92f, 0.06f, DECOR_MAT_METAL);
    b.box(1.12f, 0.00f, 0.19f, 0.16f, 0.02f, 0.06f, DECOR_MAT_DARK_METAL);
}
