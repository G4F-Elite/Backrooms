#pragma once
#include "decor_parts.h"

// ============================================================================
// LAB PROPS
// Laboratories and the machine rooms behind them: benches, hoods, racks.
// Themes: see prop_catalog.h.
// ============================================================================

// Equipment rack: drive shelves with status lights, blanking plates between.
inline void buildServerRackModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.42f;
    b.box(0.0f, 0.00f, cz, 0.60f, 0.06f, 0.80f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.06f, cz, 0.64f, 1.94f, 0.84f, DECOR_MAT_DARK_METAL);
    for (int i = 0; i < 8; i++) {
        float y = 0.16f + 0.22f * (float)i;
        bool blank = (i == 2 || i == 5);
        b.box(0.0f, y, f + 0.003f, 0.54f, 0.18f, 0.006f, blank ? DECOR_MAT_RUBBER : DECOR_MAT_METAL);
        if (blank) continue;
        b.panel(-0.12f, y + 0.10f, f + 0.008f, 0.22f, 0.045f, 0.004f, DECOR_MAT_SCREEN);
        b.box(0.17f, y + 0.03f, f + 0.008f, 0.12f, 0.12f, 0.004f, DECOR_MAT_DARK_METAL);
    }
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.295f * (float)side, 0.80f, f + 0.009f, 0.02f, 0.40f, 0.018f, DECOR_MAT_METAL);
    }
    // cable entry on the roof
    b.box(0.12f, 2.00f, -0.22f, 0.22f, 0.08f, 0.30f, DECOR_MAT_METAL);
    decorRod(b, Vec3(0.08f, 2.08f, -0.24f), Vec3(0.08f, 2.46f, -0.40f), 0.025f, DECOR_MAT_ORANGE, 6);
    decorRod(b, Vec3(0.17f, 2.08f, -0.24f), Vec3(0.17f, 2.46f, -0.40f), 0.02f, DECOR_MAT_RUBBER, 6);
}

// Wet bench: cupboards, sink, reagent shelf, a microscope left out.
inline void buildLabBenchModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.33f;
    const float top = 0.90f;
    b.box(0.0f, 0.00f, cz, 1.76f, 0.08f, 0.60f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, cz, 1.80f, 0.78f, 0.66f, DECOR_MAT_WHITE);
    for (int i = -1; i <= 1; i++) {
        float x = 0.59f * (float)i;
        b.box(x, 0.14f, f + 0.003f, 0.54f, 0.50f, 0.006f, DECOR_MAT_METAL);
        b.box(x, 0.68f, f + 0.003f, 0.54f, 0.13f, 0.006f, DECOR_MAT_METAL);
        b.box(x, 0.73f, f + 0.011f, 0.16f, 0.02f, 0.010f, DECOR_MAT_DARK_METAL);
        b.box(x + 0.20f, 0.44f, f + 0.011f, 0.02f, 0.12f, 0.010f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.86f, 0.0f, 1.84f, 0.04f, 0.70f, DECOR_MAT_RUBBER);
    // sink with a swan-neck tap
    b.box(0.56f, top, 0.04f, 0.44f, 0.004f, 0.36f, DECOR_MAT_METAL);
    b.box(0.56f, top + 0.004f, 0.04f, 0.36f, 0.002f, 0.28f, DECOR_MAT_DARK_METAL);
    b.cylinder(0.56f, top, -0.22f, 0.018f, 0.30f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(0.56f, top + 0.30f, -0.22f), Vec3(0.56f, top + 0.26f, -0.06f), 0.018f, DECOR_MAT_METAL, 6);
    // reagent shelf along the back
    b.box(0.0f, top, -0.34f, 1.76f, 0.56f, 0.02f, DECOR_MAT_WHITE);
    b.box(-0.30f, top + 0.36f, -0.26f, 1.16f, 0.03f, 0.14f, DECOR_MAT_WHITE);
    const int jarMat[6] = {DECOR_MAT_TRIM, DECOR_MAT_DRUM, DECOR_MAT_WHITE, DECOR_MAT_TRIM, DECOR_MAT_ORANGE, DECOR_MAT_DRUM};
    for (int i = 0; i < 6; i++) {
        float x = -0.78f + 0.17f * (float)i + (i >= 4 ? 0.12f : 0.0f);
        float h = 0.12f + 0.03f * (float)(i % 3);
        b.cylinder(x, top + 0.39f, -0.26f, 0.04f, h, jarMat[i], 6);
        b.box(x, top + 0.39f + h, -0.26f, 0.03f, 0.03f, 0.03f, DECOR_MAT_RUBBER);
    }
    // microscope
    b.box(-0.48f, top, 0.04f, 0.14f, 0.03f, 0.20f, DECOR_MAT_DARK_METAL);
    b.box(-0.48f, top + 0.03f, -0.035f, 0.04f, 0.26f, 0.05f, DECOR_MAT_DARK_METAL);
    b.box(-0.48f, top + 0.11f, 0.04f, 0.10f, 0.012f, 0.10f, DECOR_MAT_METAL);
    decorRod(b, Vec3(-0.48f, top + 0.15f, 0.06f), Vec3(-0.48f, top + 0.36f, -0.03f), 0.022f, DECOR_MAT_METAL, 6);
    // glassware
    decorCone(b, -0.08f, top, 0.10f, 0.06f, 0.018f, 0.12f, DECOR_MAT_DRUM, 8);
    b.cylinder(-0.08f, top + 0.12f, 0.10f, 0.018f, 0.05f, DECOR_MAT_DRUM, 6);
    b.cylinder(0.10f, top, 0.02f, 0.04f, 0.09f, DECOR_MAT_CEILTILE, 8);
    b.box(-0.80f, top, 0.10f, 0.12f, 0.05f, 0.30f, DECOR_MAT_WOOD);
    for (int i = 0; i < 4; i++) {
        b.box(-0.80f, top + 0.05f, 0.07f * (float)i, 0.02f, 0.09f, 0.02f, (i & 1) ? DECOR_MAT_ORANGE : DECOR_MAT_CEILTILE);
    }
}

// Fume cupboard: sash half down, extract duct on the roof.
inline void buildFumeHoodModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.39f;
    b.box(0.0f, 0.00f, cz, 1.26f, 0.08f, 0.72f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, cz, 1.30f, 0.80f, 0.78f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.31f * (float)side, 0.14f, f + 0.003f, 0.58f, 0.68f, 0.006f, DECOR_MAT_METAL);
        b.box(0.07f * (float)side, 0.50f, f + 0.010f, 0.02f, 0.12f, 0.008f, DECOR_MAT_DARK_METAL);
        b.box(0.63f * (float)side, 0.92f, 0.0f, 0.04f, 1.20f, 0.80f, DECOR_MAT_WHITE);
        b.box(0.585f * (float)side, 1.55f, 0.375f, 0.05f, 0.57f, 0.03f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.88f, 0.0f, 1.30f, 0.04f, 0.80f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.92f, -0.38f, 1.22f, 1.20f, 0.04f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 2.12f, 0.0f, 1.30f, 0.18f, 0.80f, DECOR_MAT_WHITE);
    // sash frame, baffle slots at the back
    b.box(0.0f, 1.50f, 0.375f, 1.22f, 0.05f, 0.03f, DECOR_MAT_METAL);
    b.box(0.0f, 1.55f, 0.375f, 0.03f, 0.57f, 0.03f, DECOR_MAT_METAL);
    b.box(0.0f, 1.47f, 0.385f, 0.30f, 0.03f, 0.03f, DECOR_MAT_RUBBER);
    b.box(0.0f, 1.10f, -0.357f, 1.00f, 0.04f, 0.006f, DECOR_MAT_RUBBER);
    b.box(0.0f, 1.90f, -0.357f, 1.00f, 0.04f, 0.006f, DECOR_MAT_RUBBER);
    b.panel(0.40f, 2.17f, 0.402f, 0.24f, 0.08f, 0.004f, DECOR_MAT_SCREEN);
    b.box(-0.46f, 2.17f, 0.402f, 0.10f, 0.08f, 0.004f, DECOR_MAT_ORANGE);
    b.cylinder(0.0f, 2.30f, -0.12f, 0.17f, 0.28f, DECOR_MAT_METAL, 10);
    // an experiment nobody finished
    b.box(-0.25f, 0.92f, -0.10f, 0.20f, 0.02f, 0.14f, DECOR_MAT_DARK_METAL);
    b.cylinder(-0.31f, 0.94f, -0.14f, 0.008f, 0.44f, DECOR_MAT_METAL, 4);
    b.box(-0.27f, 1.26f, -0.14f, 0.09f, 0.012f, 0.012f, DECOR_MAT_METAL);
    decorCone(b, -0.22f, 0.94f, -0.08f, 0.055f, 0.015f, 0.12f, DECOR_MAT_ORANGE, 8);
    b.cylinder(0.22f, 0.92f, 0.02f, 0.05f, 0.14f, DECOR_MAT_CEILTILE, 8);
    b.cylinder(0.36f, 0.92f, -0.12f, 0.035f, 0.20f, DECOR_MAT_TRIM, 8);
}

// Sealed column tank on a plinth with its monitor box.
inline void buildSpecimenTankModel(DecorBuilder& b) {
    b.cylinder(0.0f, 0.00f, 0.0f, 0.46f, 0.22f, DECOR_MAT_DARK_METAL, 12);
    b.cylinder(0.0f, 0.22f, 0.0f, 0.50f, 0.04f, DECOR_MAT_METAL, 12);
    b.cylinder(0.0f, 0.26f, 0.0f, 0.40f, 0.50f, DECOR_MAT_DRUM, 12);
    b.cylinder(0.0f, 0.76f, 0.0f, 0.42f, 0.04f, DECOR_MAT_METAL, 12);
    b.cylinder(0.0f, 0.80f, 0.0f, 0.40f, 0.50f, DECOR_MAT_DRUM, 12);
    b.cylinder(0.0f, 1.30f, 0.0f, 0.42f, 0.04f, DECOR_MAT_METAL, 12);
    b.cylinder(0.0f, 1.34f, 0.0f, 0.40f, 0.42f, DECOR_MAT_DRUM, 12);
    b.cylinder(0.0f, 1.76f, 0.0f, 0.50f, 0.04f, DECOR_MAT_METAL, 12);
    b.cylinder(0.0f, 1.80f, 0.0f, 0.44f, 0.20f, DECOR_MAT_DARK_METAL, 12);
    b.cylinder(0.0f, 2.00f, 0.0f, 0.14f, 0.10f, DECOR_MAT_METAL, 8);
    b.cylinder(0.0f, 0.26f, -0.45f, 0.04f, 1.50f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(-0.20f, 2.05f, 0.0f), Vec3(-0.44f, 1.82f, 0.0f), 0.03f, DECOR_MAT_RUBBER, 6);
    b.box(0.0f, 0.26f, 0.44f, 0.28f, 0.22f, 0.10f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 0.31f, 0.492f, 0.20f, 0.12f, 0.004f, DECOR_MAT_SCREEN);
}

// Tape unit: two reels behind a dark window, operator strip below.
inline void buildTapeDriveModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.34f;
    b.box(0.0f, 0.00f, cz, 0.72f, 0.08f, 0.64f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, cz, 0.76f, 1.72f, 0.68f, DECOR_MAT_WHITE);
    b.box(0.0f, 1.00f, f + 0.002f, 0.64f, 0.70f, 0.004f, DECOR_MAT_RUBBER);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.16f * (float)side;
        decorRod(b, Vec3(x, 1.50f, f + 0.004f), Vec3(x, 1.50f, f + 0.012f), 0.13f, DECOR_MAT_METAL, 12);
        decorRod(b, Vec3(x, 1.50f, f + 0.012f), Vec3(x, 1.50f, f + 0.016f), 0.10f, DECOR_MAT_TRIM, 12);
        decorRod(b, Vec3(x, 1.50f, f + 0.016f), Vec3(x, 1.50f, f + 0.020f), 0.035f, DECOR_MAT_DARK_METAL, 6);
        decorRod(b, Vec3(0.24f * (float)side, 1.16f, f + 0.004f), Vec3(0.24f * (float)side, 1.16f, f + 0.014f), 0.02f, DECOR_MAT_METAL, 6);
    }
    b.box(0.0f, 1.06f, f + 0.008f, 0.20f, 0.12f, 0.008f, DECOR_MAT_METAL);
    b.panel(0.0f, 0.80f, f + 0.003f, 0.64f, 0.12f, 0.006f, DECOR_MAT_SCREEN);
    for (int i = 0; i < 5; i++) {
        b.box(-0.24f + 0.12f * (float)i, 0.68f, f + 0.006f, 0.07f, 0.05f, 0.012f, (i & 1) ? DECOR_MAT_ORANGE : DECOR_MAT_CEILTILE);
    }
    b.box(0.0f, 0.14f, f + 0.003f, 0.64f, 0.46f, 0.006f, DECOR_MAT_METAL);
    b.panel(0.0f, 0.20f, f + 0.008f, 0.44f, 0.22f, 0.004f, DECOR_MAT_DARK_METAL);
}

// Instrument trolley: scope on top, power unit underneath.
inline void buildEquipmentCartModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            decorWheel(b, 0.33f * (float)sx, 0.05f, 0.18f * (float)sz, 0.05f, 0.03f);
            b.box(0.36f * (float)sx, 0.10f, 0.21f * (float)sz, 0.03f, 0.76f, 0.03f, DECOR_MAT_METAL);
        }
        b.box(0.40f * (float)sx, 0.92f, 0.0f, 0.02f, 0.02f, 0.44f, DECOR_MAT_RUBBER);
        b.box(0.40f * (float)sx, 0.89f, 0.21f, 0.02f, 0.03f, 0.02f, DECOR_MAT_METAL);
        b.box(0.40f * (float)sx, 0.89f, -0.21f, 0.02f, 0.03f, 0.02f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.14f, 0.0f, 0.69f, 0.025f, 0.39f, DECOR_MAT_METAL);
    b.box(0.0f, 0.86f, 0.0f, 0.82f, 0.03f, 0.48f, DECOR_MAT_METAL);
    b.box(-0.10f, 0.165f, 0.0f, 0.36f, 0.24f, 0.30f, DECOR_MAT_DARK_METAL);
    b.cylinder(0.20f, 0.165f, 0.02f, 0.10f, 0.05f, DECOR_MAT_ORANGE, 8);
    // oscilloscope and a rack of sample tubes
    b.box(-0.14f, 0.89f, 0.0f, 0.38f, 0.26f, 0.36f, DECOR_MAT_CEILTILE);
    b.panel(-0.21f, 0.93f, 0.183f, 0.18f, 0.16f, 0.006f, DECOR_MAT_SCREEN);
    b.box(-0.03f, 0.93f, 0.183f, 0.10f, 0.18f, 0.006f, DECOR_MAT_DARK_METAL);
    b.box(-0.14f, 1.15f, 0.0f, 0.10f, 0.02f, 0.04f, DECOR_MAT_RUBBER);
    b.box(0.22f, 0.89f, 0.0f, 0.20f, 0.03f, 0.32f, DECOR_MAT_WHITE);
    for (int i = 0; i < 4; i++) {
        b.cylinder(0.22f, 0.92f, -0.11f + 0.07f * (float)i, 0.014f, 0.09f, (i == 2) ? DECOR_MAT_ORANGE : DECOR_MAT_DRUM, 6);
    }
}

// Emergency shower over an eye-wash bowl, marked out on the floor.
inline void buildEyewashStationModel(DecorBuilder& b) {
    const float z = -0.19f;
    b.box(0.0f, 0.000f, 0.0f, 0.60f, 0.012f, 0.50f, DECOR_MAT_CAUTION);
    b.cylinder(0.0f, 0.012f, z, 0.03f, 2.30f, DECOR_MAT_ORANGE, 8);
    b.box(0.0f, 0.50f, z - 0.045f, 0.10f, 0.10f, 0.03f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.80f, z - 0.045f, 0.10f, 0.10f, 0.03f, DECOR_MAT_DARK_METAL);
    // bowl with two nozzles and a push paddle
    decorRod(b, Vec3(0.0f, 0.98f, z), Vec3(0.0f, 0.98f, 0.02f), 0.02f, DECOR_MAT_ORANGE, 6);
    decorCone(b, 0.0f, 1.00f, 0.02f, 0.05f, 0.16f, 0.08f, DECOR_MAT_ORANGE, 10);
    b.cylinder(-0.06f, 1.08f, 0.02f, 0.015f, 0.03f, DECOR_MAT_METAL, 6);
    b.cylinder(0.06f, 1.08f, 0.02f, 0.015f, 0.03f, DECOR_MAT_METAL, 6);
    b.box(0.105f, 0.97f, -0.10f, 0.17f, 0.02f, 0.02f, DECOR_MAT_METAL);
    b.box(0.195f, 0.92f, -0.10f, 0.01f, 0.14f, 0.10f, DECOR_MAT_CAUTION);
    // shower head on its arm, pull handle
    decorRod(b, Vec3(0.0f, 2.282f, z), Vec3(0.0f, 2.282f, 0.06f), 0.03f, DECOR_MAT_ORANGE, 6);
    decorCone(b, 0.0f, 2.14f, 0.06f, 0.13f, 0.04f, 0.112f, DECOR_MAT_ORANGE, 10);
    b.box(0.0f, 1.72f, -0.08f, 0.008f, 0.532f, 0.008f, DECOR_MAT_METAL);
    b.taper(0.0f, 1.60f, -0.08f, 0.16f, 0.012f, 0.02f, 0.012f, 0.12f, DECOR_MAT_CAUTION);
    b.panel(0.0f, 1.30f, z + 0.033f, 0.16f, 0.16f, 0.006f, DECOR_MAT_CAUTION);
}

// Rubber cable ramp across a walkway.
inline void buildCableCoverModel(DecorBuilder& b) {
    b.taper(0.0f, 0.00f, 0.0f, 1.60f, 0.34f, 1.60f, 0.14f, 0.05f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.05f, 0.0f, 1.60f, 0.004f, 0.06f, DECOR_MAT_CAUTION);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.80f * (float)side;
        decorRod(b, Vec3(x, 0.022f, 0.03f), Vec3(x + 0.16f * (float)side, 0.022f, 0.05f), 0.02f, DECOR_MAT_ORANGE, 6);
        decorRod(b, Vec3(x, 0.016f, -0.03f), Vec3(x + 0.16f * (float)side, 0.016f, -0.06f), 0.014f, DECOR_MAT_DRUM, 6);
    }
}
