#pragma once
#include "decor_parts.h"

// ============================================================================
// CLUTTER PROPS
// Loose things people left behind in most themes: dead plants, bookcases,
// mop buckets, queue posts, stacked chairs. Themes: see prop_catalog.h.
// ============================================================================

// Terracotta pot with a plant that dried out long ago.
inline void buildDeadPlantModel(DecorBuilder& b) {
    decorCone(b, 0.0f, 0.00f, 0.0f, 0.15f, 0.21f, 0.34f, DECOR_MAT_ORANGE, 10);
    b.cylinder(0.0f, 0.34f, 0.0f, 0.23f, 0.04f, DECOR_MAT_ORANGE, 10);
    b.cylinder(0.0f, 0.38f, 0.0f, 0.19f, 0.004f, DECOR_MAT_TRIM, 10);
    const float tip[5][3] = {
        {-0.22f, 1.05f, 0.06f}, {0.18f, 1.22f, -0.10f}, {0.04f, 1.38f, 0.16f},
        {-0.08f, 0.92f, -0.22f}, {0.22f, 0.86f, 0.14f}
    };
    for (int i = 0; i < 5; i++) {
        Vec3 root(tip[i][0] * 0.2f, 0.384f, tip[i][2] * 0.2f);
        Vec3 top(tip[i][0], tip[i][1], tip[i][2]);
        decorBeam(b, root, top, 0.014f, 0.014f, DECOR_MAT_TRIM);
        Vec3 droop(tip[i][0] * 1.35f, tip[i][1] - 0.12f, tip[i][2] * 1.35f);
        decorBeam(b, top, droop, 0.06f, 0.005f, DECOR_MAT_CARDBOARD);
    }
    DecorBuilder leafA = b.sub(0.20f, -0.22f, 0.7f);
    leafA.box(0.0f, 0.00f, 0.0f, 0.10f, 0.004f, 0.05f, DECOR_MAT_CARDBOARD);
    DecorBuilder leafB = b.sub(-0.24f, 0.16f, -0.4f);
    leafB.box(0.0f, 0.00f, 0.0f, 0.09f, 0.004f, 0.05f, DECOR_MAT_CARDBOARD);
}

// Tall bookcase, shelves partly emptied.
inline void buildBookcaseModel(DecorBuilder& b) {
    const int bookMat[6] = {DECOR_MAT_TRIM, DECOR_MAT_DRUM, DECOR_MAT_FABRIC, DECOR_MAT_ORANGE, DECOR_MAT_WHITE, DECOR_MAT_DARK_METAL};
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.485f * (float)side, 0.00f, 0.0f, 0.03f, 2.00f, 0.32f, DECOR_MAT_WOOD);
    }
    b.box(0.0f, 0.00f, -0.01f, 0.94f, 0.06f, 0.30f, DECOR_MAT_TRIM);
    b.box(0.0f, 0.06f, -0.15f, 0.94f, 1.90f, 0.02f, DECOR_MAT_TRIM);
    b.box(0.0f, 1.96f, 0.01f, 0.94f, 0.04f, 0.30f, DECOR_MAT_WOOD);
    unsigned seed = 11u;
    for (int shelf = 0; shelf < 5; shelf++) {
        float y = 0.06f + 0.38f * (float)shelf;
        if (shelf > 0) {
            b.box(0.0f, y, 0.01f, 0.94f, 0.025f, 0.30f, DECOR_MAT_WOOD);
            y += 0.025f;
        }
        float x = -0.46f;
        while (x < 0.36f) {
            seed = seed * 1103515245u + 12345u;
            unsigned roll = seed >> 16;
            float w = 0.045f + 0.015f * (float)(roll % 4u);
            float h = 0.22f + 0.02f * (float)((roll >> 3) % 5u);
            // every few books there is a gap where some were taken
            if ((roll >> 6) % 6u == 0u || (shelf == 4 && x > -0.10f)) {
                x += 0.14f;
                continue;
            }
            b.box(x + w * 0.5f, y, -0.03f, w, h, 0.20f, bookMat[(roll >> 9) % 6u]);
            x += w;
        }
    }
    b.box(0.22f, 1.605f, -0.02f, 0.32f, 0.20f, 0.22f, DECOR_MAT_CARDBOARD);
}

// Mop bucket on casters with its wringer, mop left standing in it.
inline void buildJanitorCartModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.16f * (float)sx, 0.04f, 0.27f * (float)sz, 0.04f, 0.03f);
        }
    }
    b.taper(0.0f, 0.08f, 0.0f, 0.36f, 0.60f, 0.42f, 0.66f, 0.34f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.42f, 0.0f, 0.44f, 0.02f, 0.68f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.44f, 0.14f, 0.36f, 0.004f, 0.34f, DECOR_MAT_PUDDLE);
    // wringer with its lever
    b.box(0.0f, 0.44f, -0.19f, 0.38f, 0.16f, 0.24f, DECOR_MAT_DARK_METAL);
    decorBeam(b, Vec3(0.17f, 0.60f, -0.19f), Vec3(0.19f, 1.00f, -0.30f), 0.03f, 0.03f, DECOR_MAT_METAL);
    b.box(0.19f, 1.00f, -0.30f, 0.05f, 0.10f, 0.05f, DECOR_MAT_RUBBER);
    // mop
    b.cylinder(-0.04f, 0.444f, 0.14f, 0.09f, 0.05f, DECOR_MAT_CEILTILE, 8);
    decorRod(b, Vec3(-0.04f, 0.494f, 0.14f), Vec3(0.06f, 1.50f, -0.24f), 0.014f, DECOR_MAT_WOOD, 6);
}

// Two queue posts joined by a retractable belt.
inline void buildStanchionsModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.70f * (float)side;
        b.cylinder(x, 0.00f, 0.0f, 0.16f, 0.03f, DECOR_MAT_DARK_METAL, 10);
        b.cylinder(x, 0.03f, 0.0f, 0.025f, 0.87f, DECOR_MAT_METAL, 8);
        b.cylinder(x, 0.90f, 0.0f, 0.045f, 0.08f, DECOR_MAT_DARK_METAL, 8);
    }
    b.box(0.0f, 0.91f, 0.0f, 1.31f, 0.05f, 0.006f, DECOR_MAT_DRUM);
}

// Five plastic chairs stacked on each other, each one sitting a little further forward.
inline void buildStackedChairsModel(DecorBuilder& b) {
    for (int i = 0; i < 5; i++) {
        float y = 0.44f + 0.10f * (float)i;
        float z = -0.07f + 0.035f * (float)i;
        float legBottom = (i == 0) ? 0.0f : y - 0.10f;
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                b.box(0.205f * (float)sx, legBottom, z + 0.17f * (float)sz, 0.03f, y + 0.03f - legBottom, 0.03f, DECOR_MAT_METAL);
            }
        }
        b.box(0.0f, y, z, 0.38f, 0.03f, 0.40f, DECOR_MAT_ORANGE);
        b.box(0.0f, y + 0.03f, z - 0.185f, 0.38f, 0.36f, 0.03f, DECOR_MAT_ORANGE);
    }
}

// Ceiling tiles that came down, with a piece of the grid.
inline void buildFallenTilesModel(DecorBuilder& b) {
    DecorBuilder whole = b.sub(-0.30f, -0.18f, 0.3f);
    whole.box(0.0f, 0.00f, 0.0f, 0.60f, 0.02f, 0.60f, DECOR_MAT_CEILTILE);
    DecorBuilder half = b.sub(0.36f, 0.24f, -0.5f);
    half.box(0.0f, 0.00f, 0.0f, 0.60f, 0.02f, 0.32f, DECOR_MAT_CEILTILE);
    DecorBuilder corner = b.sub(0.34f, -0.42f, 1.0f);
    corner.box(0.0f, 0.00f, 0.0f, 0.26f, 0.02f, 0.22f, DECOR_MAT_CEILTILE);
    DecorBuilder bar = b.sub(-0.14f, 0.50f, 0.15f);
    bar.box(0.0f, 0.00f, 0.0f, 0.90f, 0.025f, 0.03f, DECOR_MAT_METAL);
    b.box(0.02f, 0.00f, 0.02f, 0.10f, 0.03f, 0.08f, DECOR_MAT_CEILTILE);
    b.box(-0.52f, 0.00f, 0.30f, 0.07f, 0.02f, 0.09f, DECOR_MAT_CEILTILE);
    b.box(0.10f, 0.00f, -0.56f, 0.08f, 0.02f, 0.06f, DECOR_MAT_CEILTILE);
}

// Three gas bottles chained to a wall rail.
inline void buildGasCylindersModel(DecorBuilder& b) {
    const int bottleMat[3] = {DECOR_MAT_DRUM, DECOR_MAT_ORANGE, DECOR_MAT_WHITE};
    const float cz = -0.015f;
    b.box(0.0f, 0.00f, 0.0f, 0.84f, 0.02f, 0.30f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.95f, -0.14f, 0.84f, 0.08f, 0.02f, DECOR_MAT_DARK_METAL);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.40f * (float)side, 0.02f, -0.14f, 0.04f, 0.93f, 0.02f, DECOR_MAT_DARK_METAL);
        b.box(0.415f * (float)side, 0.975f, -0.015f, 0.01f, 0.03f, 0.23f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.975f, 0.105f, 0.84f, 0.03f, 0.01f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i++) {
        float x = 0.26f * (float)i;
        int mat = bottleMat[i + 1];
        b.cylinder(x, 0.02f, cz, 0.115f, 1.25f, mat, 8);
        decorCone(b, x, 1.27f, cz, 0.115f, 0.05f, 0.10f, mat, 8);
        b.cylinder(x, 1.37f, cz, 0.04f, 0.05f, DECOR_MAT_METAL, 6);
        b.cylinder(x, 1.42f, cz, 0.06f, 0.10f, DECOR_MAT_DARK_METAL, 6);
    }
}

// Newspaper rack: three trays of yellowed papers under a header.
inline void buildNewsstandModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.31f * (float)side, 0.00f, 0.0f, 0.02f, 1.30f, 0.36f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.10f, -0.17f, 0.60f, 1.20f, 0.02f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.30f, -0.08f, 0.64f, 0.16f, 0.20f, DECOR_MAT_ORANGE);
    b.panel(0.0f, 1.33f, 0.022f, 0.54f, 0.10f, 0.004f, DECOR_MAT_WHITE);
    for (int i = 0; i < 3; i++) {
        float y = 0.20f + 0.35f * (float)i;
        b.box(0.0f, y, 0.01f, 0.60f, 0.02f, 0.34f, DECOR_MAT_DARK_METAL);
        b.box(0.0f, y + 0.02f, 0.17f, 0.60f, 0.08f, 0.02f, DECOR_MAT_DARK_METAL);
        if (i != 1) b.box(-0.15f, y + 0.02f, 0.0f, 0.26f, 0.03f + 0.03f * (float)i, 0.30f, DECOR_MAT_WHITE);
        b.box(0.15f, y + 0.02f, 0.0f, 0.26f, 0.09f - 0.03f * (float)i, 0.30f, DECOR_MAT_CEILTILE);
    }
}
