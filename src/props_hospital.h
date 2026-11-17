#pragma once
#include "decor_parts.h"

// ============================================================================
// HOSPITAL PROPS
// Wards, corridors and waiting areas. Themes: see prop_catalog.h.
// ============================================================================

inline void buildGurneyModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.28f * (float)sx, 0.07f, 0.80f * (float)sz, 0.07f, 0.04f);
            b.box(0.28f * (float)sx, 0.18f, 0.80f * (float)sz, 0.04f, 0.50f, 0.04f, DECOR_MAT_METAL);
        }
        // side rail
        float x = 0.32f * (float)sx;
        b.box(x, 0.72f, -0.38f, 0.03f, 0.22f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.72f, 0.58f, 0.03f, 0.22f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.94f, 0.10f, 0.03f, 0.03f, 1.00f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.14f, 0.0f, 0.60f, 0.04f, 1.70f, DECOR_MAT_METAL);
    b.box(0.0f, 0.68f, 0.0f, 0.67f, 0.04f, 1.90f, DECOR_MAT_METAL);
    b.box(0.0f, 0.72f, 0.0f, 0.60f, 0.10f, 1.86f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.82f, 0.30f, 0.60f, 0.02f, 1.10f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.82f, -0.72f, 0.44f, 0.07f, 0.30f, DECOR_MAT_WHITE);
}

inline void buildIvStandModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.22f * (float)side, 0.00f, 0.0f, 0.05f, 0.05f, 0.05f, DECOR_MAT_RUBBER);
        b.box(0.0f, 0.00f, 0.22f * (float)side, 0.05f, 0.05f, 0.05f, DECOR_MAT_RUBBER);
    }
    b.box(0.0f, 0.05f, 0.0f, 0.50f, 0.03f, 0.05f, DECOR_MAT_METAL);
    b.box(0.0f, 0.05f, 0.0f, 0.05f, 0.03f, 0.50f, DECOR_MAT_METAL);
    b.cylinder(0.0f, 0.08f, 0.0f, 0.015f, 1.72f, DECOR_MAT_METAL, 6);
    b.box(0.0f, 1.80f, 0.0f, 0.30f, 0.02f, 0.02f, DECOR_MAT_METAL);
    b.box(0.11f, 1.56f, 0.0f, 0.10f, 0.24f, 0.03f, DECOR_MAT_WHITE);
    b.box(-0.11f, 1.62f, 0.0f, 0.08f, 0.18f, 0.03f, DECOR_MAT_DRUM);
}

// Supply cabinet: steel-faced upper doors, plain lower doors.
inline void buildMedCabinetModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.20f;
    b.box(0.0f, 0.00f, cz, 0.86f, 0.08f, 0.36f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, cz, 0.90f, 1.70f, 0.40f, DECOR_MAT_WHITE);
    b.box(-0.20f, 1.78f, cz, 0.30f, 0.16f, 0.26f, DECOR_MAT_CARDBOARD);
    for (int side = -1; side <= 1; side += 2) {
        b.panel(0.22f * (float)side, 0.98f, f + 0.003f, 0.40f, 0.74f, 0.006f, DECOR_MAT_METAL);
        b.box(0.05f * (float)side, 0.56f, f + 0.010f, 0.02f, 0.12f, 0.014f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.12f, f + 0.002f, 0.012f, 0.78f, 0.004f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.90f, f + 0.002f, 0.86f, 0.012f, 0.004f, DECOR_MAT_DARK_METAL);
}

inline void buildPrivacyScreenPanel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.29f * (float)side;
        b.box(x, 0.00f, 0.0f, 0.04f, 0.02f, 0.24f, DECOR_MAT_RUBBER);
        b.box(x, 0.02f, 0.0f, 0.02f, 1.68f, 0.02f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.30f, 0.0f, 0.56f, 0.02f, 0.02f, DECOR_MAT_METAL);
    b.box(0.0f, 1.68f, 0.0f, 0.56f, 0.02f, 0.02f, DECOR_MAT_METAL);
    b.box(0.0f, 0.32f, 0.0f, 0.56f, 1.36f, 0.008f, DECOR_MAT_FABRIC);
}

// Three-leaf folding screen, the outer leaves angled towards the front.
inline void buildPrivacyScreenModel(DecorBuilder& root) {
    DecorBuilder mid = root.sub(0.0f, -0.139f, 0.0f);
    DecorBuilder left = mid.sub(-0.563f, 0.144f, 0.5f);
    DecorBuilder right = mid.sub(0.563f, 0.144f, -0.5f);
    buildPrivacyScreenPanel(mid);
    buildPrivacyScreenPanel(left);
    buildPrivacyScreenPanel(right);
}

inline void buildWheelchairModel(DecorBuilder& root) {
    DecorBuilder b = root.sub(0.0f, -0.025f, 0.0f);
    decorRod(b, Vec3(-0.27f, 0.28f, -0.10f), Vec3(0.27f, 0.28f, -0.10f), 0.015f, DECOR_MAT_METAL, 6);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.235f * (float)side;
        decorWheel(b, 0.28f * (float)side, 0.28f, -0.10f, 0.28f, 0.03f);
        decorWheel(b, x, 0.07f, 0.34f, 0.07f, 0.03f);
        b.box(x, 0.14f, 0.30f, 0.03f, 0.28f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.42f, 0.02f, 0.03f, 0.04f, 0.59f, DECOR_MAT_METAL);
        b.box(x, 0.46f, -0.20f, 0.03f, 0.46f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.46f, 0.16f, 0.03f, 0.18f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.64f, 0.02f, 0.04f, 0.03f, 0.31f, DECOR_MAT_RUBBER);
        b.box(x, 0.89f, -0.265f, 0.03f, 0.03f, 0.10f, DECOR_MAT_RUBBER);
        b.box(0.135f * (float)side, 0.14f, 0.37f, 0.17f, 0.02f, 0.12f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.46f, 0.02f, 0.44f, 0.04f, 0.42f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.56f, -0.20f, 0.44f, 0.30f, 0.02f, DECOR_MAT_FABRIC);
}

// Row of three linked seats on a beam.
inline void buildWaitingSeatsModel(DecorBuilder& b) {
    b.box(-0.70f, 0.00f, 0.0f, 0.05f, 0.36f, 0.50f, DECOR_MAT_DARK_METAL);
    b.box(0.70f, 0.00f, 0.0f, 0.05f, 0.36f, 0.50f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.36f, 0.03f, 1.45f, 0.05f, 0.10f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i++) {
        float x = 0.52f * (float)i;
        b.box(x, 0.41f, 0.03f, 0.46f, 0.04f, 0.44f, DECOR_MAT_DRUM);
        b.box(x, 0.45f, -0.21f, 0.46f, 0.36f, 0.04f, DECOR_MAT_DRUM);
    }
}
