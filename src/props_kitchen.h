#pragma once
#include "decor_parts.h"

extern const float CS, WH;

// ============================================================================
// KITCHEN PROPS
// Commercial kitchen, serving line and the canteen / diner in front of it.
// Themes: see prop_catalog.h.
// ============================================================================

// Four-burner range with two ovens, a pot and a pan still on the hob.
inline void buildStoveRangeModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.34f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.44f * (float)sx, 0.00f, cz + 0.28f * (float)sz, 0.06f, 0.10f, 0.06f, DECOR_MAT_DARK_METAL);
            b.cylinder(0.25f * (float)sx, 0.90f, 0.14f * (float)sz, 0.11f, 0.012f, DECOR_MAT_RUBBER, 8);
        }
        float x = 0.245f * (float)sx;
        b.box(x, 0.16f, f + 0.003f, 0.44f, 0.44f, 0.006f, DECOR_MAT_DARK_METAL);
        b.box(x, 0.62f, f + 0.010f, 0.36f, 0.025f, 0.008f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.10f, cz, 1.00f, 0.76f, 0.68f, DECOR_MAT_METAL);
    b.box(0.0f, 0.70f, f + 0.003f, 0.96f, 0.12f, 0.006f, DECOR_MAT_DARK_METAL);
    for (int i = 0; i < 6; i++) {
        float x = -0.40f + 0.16f * (float)i;
        b.box(x, 0.74f, f + 0.012f, 0.04f, 0.04f, 0.012f, DECOR_MAT_RUBBER);
    }
    b.box(0.0f, 0.86f, 0.0f, 1.00f, 0.04f, 0.70f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.90f, -0.33f, 1.00f, 0.50f, 0.04f, DECOR_MAT_METAL);
    b.box(0.0f, 1.36f, -0.21f, 1.00f, 0.03f, 0.20f, DECOR_MAT_METAL);
    // stock pot at the back, frying pan at the front
    b.cylinder(-0.25f, 0.912f, -0.14f, 0.13f, 0.17f, DECOR_MAT_METAL, 10);
    b.cylinder(-0.25f, 1.082f, -0.14f, 0.135f, 0.012f, DECOR_MAT_DARK_METAL, 10);
    b.cylinder(-0.25f, 1.094f, -0.14f, 0.02f, 0.025f, DECOR_MAT_RUBBER, 6);
    b.cylinder(0.25f, 0.912f, 0.14f, 0.12f, 0.035f, DECOR_MAT_DARK_METAL, 10);
    decorBeam(b, Vec3(0.34f, 0.935f, 0.20f), Vec3(0.45f, 0.955f, 0.29f), 0.03f, 0.015f, DECOR_MAT_RUBBER);
    b.cylinder(0.30f, 1.39f, -0.21f, 0.05f, 0.12f, DECOR_MAT_CEILTILE, 8);
    b.cylinder(0.16f, 1.39f, -0.21f, 0.04f, 0.16f, DECOR_MAT_TRIM, 8);
}

// Two-door reach-in refrigerator, compressor behind the top grille.
inline void buildFridgeModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.37f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.52f * (float)sx, 0.00f, cz + 0.30f * (float)sz, 0.06f, 0.12f, 0.06f, DECOR_MAT_DARK_METAL);
        }
        b.box(0.30f * (float)sx, 0.16f, f + 0.005f, 0.57f, 1.56f, 0.010f, DECOR_MAT_METAL);
        b.box(0.07f * (float)sx, 0.80f, f + 0.018f, 0.03f, 0.40f, 0.016f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.12f, cz, 1.20f, 1.64f, 0.74f, DECOR_MAT_METAL);
    b.box(0.0f, 1.76f, cz, 1.20f, 0.22f, 0.74f, DECOR_MAT_DARK_METAL);
    b.panel(-0.10f, 1.79f, f + 0.003f, 0.90f, 0.16f, 0.006f, DECOR_MAT_DARK_METAL);
    b.panel(0.46f, 1.83f, f + 0.003f, 0.14f, 0.08f, 0.006f, DECOR_MAT_SCREEN);
    b.box(-0.30f, 1.20f, f + 0.011f, 0.21f, 0.29f, 0.002f, DECOR_MAT_WHITE);
}

// Steel prep table: sink, chopping board, pans on the undershelf.
inline void buildPrepCounterModel(DecorBuilder& b) {
    const float top = 0.90f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.74f * (float)sx, 0.00f, 0.29f * (float)sz, 0.05f, 0.86f, 0.05f, DECOR_MAT_METAL);
        }
    }
    b.box(0.0f, 0.20f, 0.0f, 1.43f, 0.03f, 0.53f, DECOR_MAT_METAL);
    b.box(0.0f, 0.86f, 0.0f, 1.60f, 0.04f, 0.70f, DECOR_MAT_METAL);
    b.box(0.0f, top, -0.33f, 1.60f, 0.14f, 0.04f, DECOR_MAT_METAL);
    b.box(-0.48f, top, 0.03f, 0.46f, 0.004f, 0.40f, DECOR_MAT_DARK_METAL);
    b.cylinder(-0.48f, top, -0.24f, 0.018f, 0.26f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(-0.48f, top + 0.26f, -0.24f), Vec3(-0.48f, top + 0.22f, -0.08f), 0.018f, DECOR_MAT_METAL, 6);
    DecorBuilder board = b.sub(0.18f, 0.04f, 0.2f);
    board.box(0.0f, top, 0.0f, 0.40f, 0.025f, 0.28f, DECOR_MAT_WOOD);
    board.box(0.06f, top + 0.025f, -0.03f, 0.05f, 0.004f, 0.18f, DECOR_MAT_METAL);
    board.box(0.06f, top + 0.025f, 0.11f, 0.03f, 0.016f, 0.10f, DECOR_MAT_RUBBER);
    b.cylinder(0.60f, top, -0.10f, 0.11f, 0.10f, DECOR_MAT_WHITE, 10);
    b.cylinder(-0.36f, 0.23f, 0.0f, 0.15f, 0.20f, DECOR_MAT_METAL, 10);
    b.cylinder(0.02f, 0.23f, 0.02f, 0.12f, 0.14f, DECOR_MAT_DARK_METAL, 10);
    b.box(0.44f, 0.23f, 0.0f, 0.34f, 0.18f, 0.30f, DECOR_MAT_CARDBOARD);
}

// Diner booth: two padded benches across a table, table end against the wall.
inline void buildDinerBoothModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float s = (float)side;
        b.box(0.56f * s, 0.00f, 0.0f, 0.44f, 0.38f, 1.10f, DECOR_MAT_TRIM);
        b.box(0.535f * s, 0.38f, 0.0f, 0.39f, 0.08f, 1.06f, DECOR_MAT_ORANGE);
        b.box(0.765f * s, 0.38f, 0.0f, 0.07f, 0.72f, 1.10f, DECOR_MAT_ORANGE);
        b.box(0.765f * s, 1.10f, 0.0f, 0.09f, 0.03f, 1.10f, DECOR_MAT_TRIM);
    }
    b.cylinder(0.0f, 0.00f, -0.04f, 0.22f, 0.03f, DECOR_MAT_DARK_METAL, 10);
    b.cylinder(0.0f, 0.03f, -0.04f, 0.05f, 0.67f, DECOR_MAT_METAL, 8);
    b.box(0.0f, 0.70f, -0.04f, 0.62f, 0.04f, 1.02f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.70f, 0.475f, 0.62f, 0.04f, 0.01f, DECOR_MAT_METAL);
    // what was left on the table
    b.box(0.0f, 0.74f, -0.44f, 0.12f, 0.14f, 0.10f, DECOR_MAT_METAL);
    b.cylinder(0.12f, 0.74f, -0.42f, 0.025f, 0.14f, DECOR_MAT_ORANGE, 6);
    b.cylinder(-0.12f, 0.74f, -0.42f, 0.025f, 0.14f, DECOR_MAT_CAUTION, 6);
    b.cylinder(-0.10f, 0.74f, 0.08f, 0.10f, 0.01f, DECOR_MAT_CEILTILE, 10);
    b.cylinder(0.14f, 0.74f, 0.20f, 0.035f, 0.08f, DECOR_MAT_CEILTILE, 8);
    DecorBuilder menu = b.sub(0.10f, -0.14f, 0.5f);
    menu.panel(0.0f, 0.74f, 0.0f, 0.21f, 0.004f, 0.29f, DECOR_MAT_WHITE);
}

// Canteen table with six stools fixed to its frame.
inline void buildCafeteriaTableModel(DecorBuilder& b) {
    const int stoolMat[3] = {DECOR_MAT_DRUM, DECOR_MAT_ORANGE, DECOR_MAT_DRUM};
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.84f * (float)side;
        b.box(x, 0.00f, 0.0f, 0.06f, 0.03f, 0.80f, DECOR_MAT_DARK_METAL);
        b.box(x, 0.03f, 0.0f, 0.05f, 0.69f, 0.05f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.60f, 0.0f, 1.63f, 0.06f, 0.06f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.72f, 0.0f, 1.80f, 0.04f, 0.70f, DECOR_MAT_CEILTILE);
    for (int i = -1; i <= 1; i++) {
        float x = 0.60f * (float)i;
        b.box(x, 0.34f, 0.0f, 0.04f, 0.26f, 0.04f, DECOR_MAT_DARK_METAL);
        b.box(x, 0.30f, 0.0f, 0.04f, 0.04f, 1.14f, DECOR_MAT_DARK_METAL);
        for (int sz = -1; sz <= 1; sz += 2) {
            b.cylinder(x, 0.34f, 0.55f * (float)sz, 0.025f, 0.08f, DECOR_MAT_METAL, 6);
            b.cylinder(x, 0.42f, 0.55f * (float)sz, 0.15f, 0.04f, stoolMat[i + 1], 8);
        }
    }
    // one tray never cleared away
    DecorBuilder tray = b.sub(-0.42f, 0.10f, 0.12f);
    tray.box(0.0f, 0.76f, 0.0f, 0.42f, 0.015f, 0.30f, DECOR_MAT_ORANGE);
    tray.cylinder(-0.06f, 0.775f, 0.0f, 0.09f, 0.01f, DECOR_MAT_WHITE, 8);
    tray.cylinder(0.13f, 0.775f, -0.06f, 0.03f, 0.08f, DECOR_MAT_WHITE, 6);
    b.cylinder(0.50f, 0.76f, -0.14f, 0.03f, 0.08f, DECOR_MAT_DRUM, 6);
}

// Tall tray trolley, a few trays still in the runners.
inline void buildTrayRackModel(DecorBuilder& b) {
    const int trayMat[3] = {DECOR_MAT_ORANGE, DECOR_MAT_CARDBOARD, DECOR_MAT_DRUM};
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.24f * (float)sx, 0.05f, 0.29f * (float)sz, 0.05f, 0.03f);
            b.box(0.26f * (float)sx, 0.10f, 0.33f * (float)sz, 0.03f, 1.58f, 0.03f, DECOR_MAT_METAL);
        }
    }
    b.box(0.0f, 0.10f, 0.0f, 0.49f, 0.03f, 0.63f, DECOR_MAT_METAL);
    b.box(0.0f, 1.65f, 0.0f, 0.49f, 0.03f, 0.63f, DECOR_MAT_METAL);
    for (int i = 0; i < 9; i++) {
        float y = 0.26f + 0.15f * (float)i;
        for (int side = -1; side <= 1; side += 2) {
            b.box(0.2375f * (float)side, y, 0.0f, 0.015f, 0.015f, 0.63f, DECOR_MAT_METAL);
        }
        if ((i * 5) % 7 >= 4) continue;
        float slide = (i == 3) ? 0.04f : 0.0f;
        b.box(0.0f, y + 0.015f, slide, 0.46f, 0.02f, 0.60f, trayMat[i % 3]);
    }
}

// Self-service counter: three wells under a guard, tray slide along the front (+Z).
inline void buildServingLineModel(DecorBuilder& b) {
    const int foodMat[3] = {DECOR_MAT_CEILTILE, DECOR_MAT_ORANGE, DECOR_MAT_TRIM};
    const float cz = -0.10f;
    b.box(0.0f, 0.00f, cz, 1.96f, 0.10f, 0.56f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.10f, cz, 2.00f, 0.76f, 0.60f, DECOR_MAT_METAL);
    b.box(0.0f, 0.86f, cz, 2.00f, 0.04f, 0.60f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i++) {
        float x = 0.62f * (float)i;
        b.box(x, 0.90f, cz, 0.52f, 0.004f, 0.42f, DECOR_MAT_DARK_METAL);
        if (i != 1) b.box(x, 0.904f, cz, 0.44f, 0.02f, 0.34f, foodMat[i + 1]);
        b.box(x, 0.30f, 0.203f, 0.50f, 0.40f, 0.006f, DECOR_MAT_DARK_METAL);
    }
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.97f * (float)side;
        b.box(x, 0.90f, 0.17f, 0.03f, 0.42f, 0.03f, DECOR_MAT_METAL);
        b.box(x, 0.90f, -0.37f, 0.03f, 0.42f, 0.03f, DECOR_MAT_METAL);
        b.box(0.80f * (float)side, 0.76f, 0.30f, 0.03f, 0.03f, 0.20f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 1.32f, cz, 2.00f, 0.02f, 0.60f, DECOR_MAT_METAL);
    decorBeam(b, Vec3(0.0f, 1.31f, 0.20f), Vec3(0.0f, 1.08f, 0.31f), 1.90f, 0.01f, DECOR_MAT_CEILTILE);
    for (int i = 0; i < 3; i++) {
        float z = 0.245f + 0.07f * (float)i;
        decorRod(b, Vec3(-1.00f, 0.805f, z), Vec3(1.00f, 0.805f, z), 0.015f, DECOR_MAT_METAL, 6);
    }
    b.box(-0.55f, 0.82f, 0.31f, 0.42f, 0.015f, 0.17f, DECOR_MAT_ORANGE);
}

// Extract canopy over a cooking line, hung from the ceiling.
inline void buildRangeHoodModel(DecorBuilder& b) {
    const float y = 2.78f;
    b.box(0.0f, y, 0.0f, 1.80f, 0.10f, 1.00f, DECOR_MAT_METAL);
    b.taper(0.0f, y + 0.10f, 0.0f, 1.80f, 1.00f, 0.90f, 0.50f, 0.40f, DECOR_MAT_METAL);
    b.box(0.0f, y + 0.50f, 0.0f, 0.50f, WH - (y + 0.50f), 0.40f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i++) {
        b.box(0.56f * (float)i, y - 0.006f, 0.0f, 0.50f, 0.006f, 0.70f, DECOR_MAT_DARK_METAL);
    }
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.86f * (float)sx, y + 0.10f, 0.46f * (float)sz, 0.02f, WH - (y + 0.10f), 0.02f, DECOR_MAT_DARK_METAL);
        }
    }
    b.box(0.84f, y - 0.05f, 0.44f, 0.08f, 0.05f, 0.08f, DECOR_MAT_DARK_METAL);
}

// Paper sack, 0.62 x 0.14 x 0.40, bulging in the middle.
inline void buildSackModel(DecorBuilder& b, float y, int mat) {
    b.taper(0.0f, y, 0.0f, 0.54f, 0.32f, 0.62f, 0.40f, 0.07f, mat);
    b.taper(0.0f, y + 0.07f, 0.0f, 0.62f, 0.40f, 0.54f, 0.32f, 0.07f, mat);
}

// Sacks of flour stacked crosswise, one of them split.
inline void buildSackPileModel(DecorBuilder& b) {
    DecorBuilder a0 = b.sub(0.0f, -0.21f, 0.0f);
    DecorBuilder a1 = b.sub(0.0f, 0.21f, 0.0f);
    DecorBuilder b0 = b.sub(-0.20f, 0.0f, 1.5707963f);
    DecorBuilder b1 = b.sub(0.20f, 0.0f, 1.5707963f);
    DecorBuilder c0 = b.sub(-0.02f, 0.02f, 0.3f);
    buildSackModel(a0, 0.00f, DECOR_MAT_CARDBOARD);
    buildSackModel(a1, 0.00f, DECOR_MAT_CEILTILE);
    buildSackModel(b0, 0.14f, DECOR_MAT_CEILTILE);
    buildSackModel(b1, 0.14f, DECOR_MAT_CARDBOARD);
    buildSackModel(c0, 0.28f, DECOR_MAT_CARDBOARD);
    b.cylinder(0.27f, 0.00f, 0.36f, 0.12f, 0.008f, DECOR_MAT_WHITE, 9);
    b.cylinder(0.34f, 0.00f, 0.22f, 0.06f, 0.005f, DECOR_MAT_WHITE, 7);
}
