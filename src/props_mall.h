#pragma once
#include "decor_parts.h"

// ============================================================================
// MALL PROPS
// Shop floors, fitting rooms and the concourse outside.
// Themes: see prop_catalog.h.
// ============================================================================

// Rolling garment rail, half of the hangers already empty.
inline void buildClothesRackModel(DecorBuilder& b) {
    const int clothMat[6] = {DECOR_MAT_FABRIC, DECOR_MAT_WHITE, DECOR_MAT_ORANGE, DECOR_MAT_DRUM, DECOR_MAT_FABRIC, DECOR_MAT_TRIM};
    const float clothLen[6] = {0.74f, 0.58f, 0.92f, 0.66f, 0.80f, 0.60f};
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.65f * (float)side;
        decorWheel(b, x, 0.04f, -0.21f, 0.04f, 0.03f);
        decorWheel(b, x, 0.04f, 0.21f, 0.04f, 0.03f);
        b.box(x, 0.08f, 0.0f, 0.04f, 0.04f, 0.50f, DECOR_MAT_METAL);
        b.box(x, 0.12f, 0.0f, 0.03f, 1.46f, 0.03f, DECOR_MAT_METAL);
    }
    decorRod(b, Vec3(-0.68f, 1.58f, 0.0f), Vec3(0.68f, 1.58f, 0.0f), 0.015f, DECOR_MAT_METAL, 6);
    b.box(0.0f, 0.10f, 0.0f, 1.26f, 0.02f, 0.03f, DECOR_MAT_METAL);
    for (int i = 0; i < 6; i++) {
        float x = -0.52f + 0.13f * (float)i + (i >= 4 ? 0.40f : 0.0f);
        b.box(x, 1.50f, 0.0f, 0.012f, 0.065f, 0.012f, DECOR_MAT_METAL);
        b.box(x, 1.47f, 0.0f, 0.02f, 0.03f, 0.40f, DECOR_MAT_METAL);
        b.box(x, 1.47f - clothLen[i], 0.0f, 0.07f, clothLen[i], 0.42f, clothMat[i]);
    }
    // bare hangers pushed together
    for (int i = 0; i < 3; i++) {
        float x = 0.10f + 0.04f * (float)i;
        b.box(x, 1.50f, 0.0f, 0.012f, 0.065f, 0.012f, DECOR_MAT_METAL);
        b.box(x, 1.47f, 0.0f, 0.012f, 0.03f, 0.38f, DECOR_MAT_METAL);
    }
}

// Faceless shop dummy on a stand, one forearm missing.
inline void buildMannequinModel(DecorBuilder& b) {
    b.cylinder(0.0f, 0.00f, 0.0f, 0.22f, 0.02f, DECOR_MAT_METAL, 10);
    b.cylinder(0.0f, 0.02f, -0.08f, 0.012f, 0.90f, DECOR_MAT_METAL, 6);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.09f * (float)side;
        b.taper(x, 0.02f, 0.0f, 0.09f, 0.10f, 0.11f, 0.12f, 0.42f, DECOR_MAT_WHITE);
        b.taper(x, 0.44f, 0.0f, 0.11f, 0.12f, 0.15f, 0.16f, 0.42f, DECOR_MAT_WHITE);
    }
    b.taper(0.0f, 0.86f, 0.0f, 0.34f, 0.18f, 0.28f, 0.16f, 0.18f, DECOR_MAT_WHITE);
    b.taper(0.0f, 1.04f, 0.0f, 0.28f, 0.16f, 0.38f, 0.20f, 0.36f, DECOR_MAT_WHITE);
    b.taper(0.0f, 1.40f, 0.0f, 0.38f, 0.20f, 0.14f, 0.12f, 0.06f, DECOR_MAT_WHITE);
    b.cylinder(0.0f, 1.46f, 0.0f, 0.045f, 0.08f, DECOR_MAT_WHITE, 8);
    decorBall(b, 0.0f, 1.65f, 0.01f, 0.115f, DECOR_MAT_WHITE);
    // right arm hangs down, the left one ends at the elbow
    decorBeam(b, Vec3(0.23f, 1.40f, 0.0f), Vec3(0.27f, 1.12f, 0.0f), 0.08f, 0.08f, DECOR_MAT_WHITE);
    decorBeam(b, Vec3(0.27f, 1.12f, 0.0f), Vec3(0.28f, 0.82f, 0.05f), 0.07f, 0.07f, DECOR_MAT_WHITE);
    decorBeam(b, Vec3(-0.23f, 1.40f, 0.0f), Vec3(-0.27f, 1.12f, 0.0f), 0.08f, 0.08f, DECOR_MAT_WHITE);
    decorRod(b, Vec3(-0.27f, 1.12f, 0.0f), Vec3(-0.273f, 1.10f, 0.0f), 0.022f, DECOR_MAT_DARK_METAL, 6);
}

// Wire trolley with one forgotten box in the basket.
inline void buildShoppingCartModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.20f * (float)side;
        decorWheel(b, x, 0.05f, -0.34f, 0.05f, 0.03f);
        decorWheel(b, x, 0.05f, 0.34f, 0.05f, 0.03f);
        b.box(x, 0.10f, 0.0f, 0.025f, 0.025f, 0.76f, DECOR_MAT_METAL);
        b.box(x, 0.125f, -0.36f, 0.025f, 0.425f, 0.025f, DECOR_MAT_METAL);
        b.box(0.215f * (float)side, 0.925f, -0.40f, 0.02f, 0.04f, 0.07f, DECOR_MAT_METAL);
        // basket corner posts
        b.box(0.2225f * (float)side, 0.565f, 0.4125f, 0.015f, 0.36f, 0.015f, DECOR_MAT_METAL);
        b.box(0.2225f * (float)side, 0.565f, -0.3725f, 0.015f, 0.36f, 0.015f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.125f, 0.04f, 0.375f, 0.015f, 0.60f, DECOR_MAT_METAL);
    b.box(0.0f, 0.55f, 0.02f, 0.46f, 0.015f, 0.80f, DECOR_MAT_METAL);
    // basket walls: wire rails instead of sheets
    for (int i = 1; i <= 4; i++) {
        float y = 0.565f + 0.09f * (float)i - 0.012f;
        for (int side = -1; side <= 1; side += 2) {
            b.box(0.2225f * (float)side, y, 0.02f, 0.012f, 0.012f, 0.77f, DECOR_MAT_METAL);
        }
        b.box(0.0f, y, 0.4125f, 0.43f, 0.012f, 0.012f, DECOR_MAT_METAL);
        b.box(0.0f, y, -0.3725f, 0.43f, 0.012f, 0.012f, DECOR_MAT_METAL);
    }
    decorRod(b, Vec3(-0.23f, 0.98f, -0.42f), Vec3(0.23f, 0.98f, -0.42f), 0.016f, DECOR_MAT_ORANGE, 6);
    b.box(0.03f, 0.565f, 0.12f, 0.28f, 0.20f, 0.32f, DECOR_MAT_CARDBOARD);
}

// Sales counter with a till; customers stand at the front (+Z).
inline void buildDisplayCounterModel(DecorBuilder& b) {
    const float f = 0.30f;
    b.box(0.0f, 0.00f, 0.0f, 1.56f, 0.08f, 0.52f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.08f, 0.0f, 1.60f, 0.80f, 0.60f, DECOR_MAT_WHITE);
    for (int i = -1; i <= 1; i++) {
        b.box(0.50f * (float)i, 0.14f, f + 0.003f, 0.44f, 0.68f, 0.006f, DECOR_MAT_WOOD);
    }
    b.box(0.0f, 0.88f, 0.0f, 1.66f, 0.04f, 0.66f, DECOR_MAT_TRIM);
    // till: drawer, keys towards the cashier, customer display on a stalk
    b.box(0.45f, 0.92f, -0.02f, 0.36f, 0.10f, 0.36f, DECOR_MAT_DARK_METAL);
    b.box(0.45f, 1.02f, 0.06f, 0.30f, 0.12f, 0.16f, DECOR_MAT_DARK_METAL);
    b.box(0.45f, 1.02f, -0.11f, 0.28f, 0.012f, 0.14f, DECOR_MAT_WHITE);
    b.panel(0.45f, 1.05f, -0.023f, 0.20f, 0.07f, 0.006f, DECOR_MAT_SCREEN);
    b.cylinder(0.56f, 1.14f, 0.10f, 0.012f, 0.10f, DECOR_MAT_METAL, 6);
    b.panel(0.56f, 1.24f, 0.10f, 0.12f, 0.05f, 0.03f, DECOR_MAT_SCREEN);
    b.box(-0.45f, 0.92f, 0.02f, 0.30f, 0.04f, 0.36f, DECOR_MAT_CARDBOARD);
    b.cylinder(-0.05f, 0.92f, 0.12f, 0.05f, 0.11f, DECOR_MAT_ORANGE, 8);
}

// Free-standing floor plan board, lit on the front.
inline void buildInfoKioskModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.0f, 0.80f, 0.10f, 0.36f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.10f, 0.0f, 0.70f, 1.76f, 0.16f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.86f, 0.0f, 0.76f, 0.08f, 0.22f, DECOR_MAT_ORANGE);
    b.panel(0.0f, 0.90f, 0.083f, 0.60f, 0.86f, 0.006f, DECOR_MAT_SCREEN);
    b.panel(0.0f, 0.90f, -0.083f, 0.60f, 0.86f, 0.006f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.62f, 0.083f, 0.60f, 0.20f, 0.006f, DECOR_MAT_WHITE);
    b.box(0.18f, 0.67f, 0.087f, 0.12f, 0.10f, 0.004f, DECOR_MAT_ORANGE);
}

// Double-sided shop shelving, mostly cleared out.
inline void buildGondolaShelfModel(DecorBuilder& b) {
    const int stockMat[4] = {DECOR_MAT_CARDBOARD, DECOR_MAT_DRUM, DECOR_MAT_WHITE, DECOR_MAT_ORANGE};
    b.box(0.0f, 0.00f, 0.0f, 1.80f, 0.14f, 0.80f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.14f, 0.0f, 1.72f, 1.36f, 0.04f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.88f * (float)side, 0.14f, 0.0f, 0.04f, 1.40f, 0.12f, DECOR_MAT_METAL);
    }
    for (int side = -1; side <= 1; side += 2) {
        for (int level = 0; level < 4; level++) {
            float y = 0.14f;
            if (level > 0) {
                y = 0.16f + 0.36f * (float)level;
                b.box(0.0f, y, 0.19f * (float)side, 1.72f, 0.025f, 0.34f, DECOR_MAT_WHITE);
                b.box(0.0f, y - 0.02f, 0.3625f * (float)side, 1.72f, 0.045f, 0.005f, DECOR_MAT_ORANGE);
                y += 0.025f;
            }
            for (int slot = 0; slot < 5; slot++) {
                int roll = level * 7 + slot * 5 + (side > 0 ? 3 : 0);
                if (roll % 3 == 0) continue;
                float h = 0.14f + 0.04f * (float)(roll % 4);
                float w = 0.20f + 0.03f * (float)(roll % 3);
                b.box(-0.68f + 0.34f * (float)slot, y, 0.17f * (float)side, w, h, 0.24f, stockMat[roll % 4]);
            }
        }
    }
}

// Changing cubicle: curtain half drawn, mirror on the back wall.
inline void buildFittingBoothModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.53f * (float)side, 0.00f, 0.0f, 0.04f, 2.10f, 1.00f, DECOR_MAT_WHITE);
    }
    b.box(0.0f, 0.00f, -0.48f, 1.02f, 2.10f, 0.04f, DECOR_MAT_WHITE);
    b.panel(0.0f, 0.45f, -0.458f, 0.50f, 1.40f, 0.004f, DECOR_MAT_METAL);
    decorRod(b, Vec3(-0.51f, 2.00f, 0.46f), Vec3(0.51f, 2.00f, 0.46f), 0.015f, DECOR_MAT_METAL, 6);
    for (int i = 0; i < 5; i++) {
        float x = -0.44f + 0.13f * (float)i;
        b.box(x, 0.22f, 0.46f + ((i & 1) ? 0.015f : -0.015f), 0.13f, 1.765f, 0.03f, DECOR_MAT_FABRIC);
    }
    b.box(0.30f, 0.00f, -0.26f, 0.32f, 0.42f, 0.32f, DECOR_MAT_WOOD);
    b.box(-0.37f, 1.60f, -0.44f, 0.03f, 0.03f, 0.04f, DECOR_MAT_METAL);
    b.box(-0.37f, 1.18f, -0.445f, 0.24f, 0.42f, 0.03f, DECOR_MAT_ORANGE);
}
