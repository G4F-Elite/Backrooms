#pragma once
#include "decor_parts.h"

// ============================================================================
// SCHOOL PROPS
// Classrooms, the gym hall and the corridors between them.
// Themes: see prop_catalog.h.
// ============================================================================

// One-piece pupil desk: seat at the back, writing top and book tray at the front.
inline void buildSchoolDeskModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.24f * (float)side;
        b.box(x, 0.00f, 0.0f, 0.03f, 0.03f, 0.88f, DECOR_MAT_DARK_METAL);
        b.box(x, 0.03f, 0.30f, 0.03f, 0.67f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.03f, -0.20f, 0.03f, 0.39f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.45f, -0.415f, 0.03f, 0.41f, 0.03f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.70f, 0.24f, 0.60f, 0.03f, 0.42f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.56f, 0.24f, 0.45f, 0.02f, 0.36f, DECOR_MAT_METAL);
    b.box(0.0f, 0.42f, -0.22f, 0.51f, 0.03f, 0.42f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.66f, -0.415f, 0.45f, 0.20f, 0.03f, DECOR_MAT_WOOD);
    b.box(0.08f, 0.58f, 0.22f, 0.20f, 0.04f, 0.26f, DECOR_MAT_DRUM);
}

// Chalkboard with a tray; the last lesson is still on it.
inline void buildBlackboardModel(DecorBuilder& b) {
    b.box(0.0f, 0.90f, -0.02f, 2.00f, 1.24f, 0.06f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.96f, 0.012f, 1.88f, 1.12f, 0.004f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.87f, 0.01f, 1.70f, 0.03f, 0.08f, DECOR_MAT_TRIM);
    b.box(0.34f, 0.90f, 0.03f, 0.08f, 0.012f, 0.012f, DECOR_MAT_WHITE);
    b.box(-0.50f, 0.90f, 0.03f, 0.13f, 0.03f, 0.04f, DECOR_MAT_FABRIC);
    // chalk lines, the bottom one trails off
    const float line[5][3] = {
        {-0.42f, 1.84f, 0.80f}, {-0.36f, 1.68f, 0.92f}, {-0.50f, 1.52f, 0.64f},
        {-0.30f, 1.36f, 1.04f}, {-0.62f, 1.20f, 0.40f}
    };
    for (int i = 0; i < 5; i++) {
        b.box(line[i][0], line[i][1], 0.0145f, line[i][2], 0.014f, 0.001f, DECOR_MAT_WHITE);
    }
    b.box(0.58f, 1.14f, 0.0145f, 0.34f, 0.30f, 0.001f, DECOR_MAT_CEILTILE);
}

// Cloakroom bench under a row of pegs, a bag and a jacket never collected.
inline void buildCoatHooksModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.62f * (float)side, 0.00f, 0.0f, 0.04f, 0.38f, 0.36f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.38f, 0.0f, 1.40f, 0.04f, 0.36f, DECOR_MAT_WOOD);
    b.box(0.0f, 1.40f, -0.165f, 1.40f, 0.14f, 0.03f, DECOR_MAT_WOOD);
    for (int i = -2; i <= 2; i++) {
        float x = 0.28f * (float)i;
        decorRod(b, Vec3(x, 1.47f, -0.15f), Vec3(x, 1.47f, -0.06f), 0.012f, DECOR_MAT_METAL, 6);
        b.box(x, 1.47f, -0.065f, 0.024f, 0.03f, 0.01f, DECOR_MAT_METAL);
    }
    // school bag on the second peg, jacket on the fourth
    b.box(-0.28f, 1.38f, -0.10f, 0.03f, 0.09f, 0.02f, DECOR_MAT_RUBBER);
    b.box(-0.28f, 1.02f, -0.08f, 0.28f, 0.36f, 0.14f, DECOR_MAT_DRUM);
    b.box(-0.28f, 1.06f, -0.005f, 0.20f, 0.16f, 0.01f, DECOR_MAT_RUBBER);
    b.box(0.28f, 0.92f, -0.11f, 0.36f, 0.54f, 0.08f, DECOR_MAT_ORANGE);
    b.box(0.28f, 1.36f, -0.068f, 0.10f, 0.10f, 0.004f, DECOR_MAT_FABRIC);
    // shoes under the bench
    b.box(0.20f, 0.00f, 0.02f, 0.09f, 0.07f, 0.24f, DECOR_MAT_RUBBER);
    b.box(0.32f, 0.00f, 0.04f, 0.09f, 0.07f, 0.24f, DECOR_MAT_RUBBER);
}

// Gym vaulting box: three stacked sections under a padded top.
inline void buildVaultingHorseModel(DecorBuilder& b) {
    float w = 1.30f, d = 0.70f, y = 0.0f;
    for (int i = 0; i < 3; i++) {
        b.taper(0.0f, y, 0.0f, w, d, w - 0.07f, d - 0.07f, 0.28f, DECOR_MAT_WOOD);
        b.box(0.0f, y + 0.28f, 0.0f, w - 0.09f, 0.02f, d - 0.09f, DECOR_MAT_TRIM);
        for (int side = -1; side <= 1; side += 2) {
            b.box(0.0f, y + 0.11f, (d * 0.5f - 0.014f) * (float)side, 0.22f, 0.05f, 0.006f, DECOR_MAT_RUBBER);
        }
        w -= 0.09f;
        d -= 0.09f;
        y += 0.30f;
    }
    b.box(0.0f, y, 0.0f, w + 0.02f, 0.10f, d + 0.02f, DECOR_MAT_FABRIC);
}

inline void buildTrophyModel(DecorBuilder& b, float x, float y, float z, float s, int mat) {
    b.box(x, y, z, 0.10f * s, 0.04f * s, 0.10f * s, DECOR_MAT_TRIM);
    b.cylinder(x, y + 0.04f * s, z, 0.015f * s, 0.07f * s, mat, 6);
    decorCone(b, x, y + 0.11f * s, z, 0.02f * s, 0.06f * s, 0.11f * s, mat, 8);
}

// Display case: cupboard below, open shelves of cups and plaques above.
inline void buildTrophyCaseModel(DecorBuilder& b) {
    const float f = 0.18f;
    b.box(0.0f, 0.00f, 0.0f, 1.20f, 0.50f, 0.36f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.05f, f + 0.002f, 0.012f, 0.40f, 0.004f, DECOR_MAT_TRIM);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.06f * (float)side, 0.28f, f + 0.006f, 0.02f, 0.08f, 0.008f, DECOR_MAT_METAL);
        b.box(0.585f * (float)side, 0.50f, 0.0f, 0.03f, 1.36f, 0.36f, DECOR_MAT_TRIM);
    }
    b.box(0.0f, 0.50f, -0.165f, 1.14f, 1.36f, 0.03f, DECOR_MAT_FABRIC);
    b.box(0.0f, 1.86f, 0.0f, 1.20f, 0.04f, 0.36f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.95f, 0.015f, 1.14f, 0.02f, 0.33f, DECOR_MAT_WOOD);
    b.box(0.0f, 1.40f, 0.015f, 1.14f, 0.02f, 0.33f, DECOR_MAT_WOOD);
    buildTrophyModel(b, -0.36f, 0.50f, 0.02f, 1.5f, DECOR_MAT_ORANGE);
    buildTrophyModel(b, -0.08f, 0.50f, 0.00f, 1.0f, DECOR_MAT_METAL);
    b.panel(0.30f, 0.50f, -0.04f, 0.30f, 0.22f, 0.02f, DECOR_MAT_WHITE);
    buildTrophyModel(b, 0.34f, 0.97f, 0.02f, 1.3f, DECOR_MAT_METAL);
    buildTrophyModel(b, 0.08f, 0.97f, 0.04f, 0.9f, DECOR_MAT_ORANGE);
    b.box(-0.30f, 0.97f, 0.0f, 0.24f, 0.16f, 0.03f, DECOR_MAT_TRIM);
    b.box(-0.30f, 1.01f, 0.016f, 0.16f, 0.08f, 0.004f, DECOR_MAT_ORANGE);
    buildTrophyModel(b, -0.10f, 1.42f, 0.02f, 1.2f, DECOR_MAT_ORANGE);
    decorBall(b, 0.30f, 1.53f, 0.02f, 0.11f, DECOR_MAT_WHITE);
}

// Television trolley: CRT strapped on top, video recorder on the middle shelf.
inline void buildAvCartModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.29f * (float)sx, 0.05f, 0.19f * (float)sz, 0.05f, 0.03f);
            b.box(0.32f * (float)sx, 0.10f, 0.22f * (float)sz, 0.03f, 1.07f, 0.03f, DECOR_MAT_DARK_METAL);
        }
    }
    b.box(0.0f, 0.10f, 0.0f, 0.61f, 0.03f, 0.41f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.62f, 0.0f, 0.61f, 0.03f, 0.41f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.17f, 0.0f, 0.70f, 0.03f, 0.50f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.65f, 0.0f, 0.42f, 0.10f, 0.32f, DECOR_MAT_METAL);
    b.box(-0.04f, 0.69f, 0.162f, 0.20f, 0.03f, 0.004f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.75f, 0.02f, 0.19f, 0.03f, 0.11f, DECOR_MAT_RUBBER);
    // television
    b.box(0.0f, 1.20f, 0.02f, 0.62f, 0.50f, 0.36f, DECOR_MAT_RUBBER);
    b.box(0.0f, 1.24f, -0.20f, 0.42f, 0.36f, 0.08f, DECOR_MAT_RUBBER);
    b.panel(-0.06f, 1.26f, 0.203f, 0.42f, 0.38f, 0.006f, DECOR_MAT_SCREEN);
    b.box(0.24f, 1.26f, 0.203f, 0.08f, 0.38f, 0.006f, DECOR_MAT_DARK_METAL);
    b.box(0.24f, 1.54f, 0.210f, 0.04f, 0.04f, 0.012f, DECOR_MAT_METAL);
    b.box(0.24f, 1.46f, 0.210f, 0.04f, 0.04f, 0.012f, DECOR_MAT_METAL);
}

// Speaker's stand with a sloped reading top and a dead microphone.
inline void buildLecternModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.0f, 0.60f, 0.05f, 0.50f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.05f, 0.04f, 0.52f, 1.00f, 0.34f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.30f, 0.212f, 0.36f, 0.50f, 0.004f, DECOR_MAT_TRIM);
    decorBeam(b, Vec3(0.0f, 1.07f, -0.23f), Vec3(0.0f, 1.21f, 0.22f), 0.60f, 0.03f, DECOR_MAT_TRIM);
    b.box(0.0f, 1.05f, 0.14f, 0.52f, 0.10f, 0.14f, DECOR_MAT_WOOD);
    decorRod(b, Vec3(0.18f, 1.20f, 0.14f), Vec3(0.14f, 1.40f, 0.02f), 0.008f, DECOR_MAT_DARK_METAL, 4);
    decorRod(b, Vec3(0.14f, 1.40f, 0.02f), Vec3(0.13f, 1.44f, -0.03f), 0.022f, DECOR_MAT_RUBBER, 6);
}
