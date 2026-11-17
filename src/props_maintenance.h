#pragma once
#include "decor_parts.h"

// ============================================================================
// MAINTENANCE PROPS
// Plant rooms and service tunnels: tanks, valves, generators, benches.
// Themes: see prop_catalog.h.
// ============================================================================

inline void buildBoilerTankModel(DecorBuilder& b) {
    b.cylinder(0.0f, 0.00f, 0.0f, 0.44f, 0.10f, DECOR_MAT_DARK_METAL, 12);
    b.cylinder(0.0f, 0.10f, 0.0f, 0.50f, 0.50f, DECOR_MAT_METAL, 12);
    b.cylinder(0.0f, 0.60f, 0.0f, 0.51f, 0.06f, DECOR_MAT_DARK_METAL, 12);
    b.cylinder(0.0f, 0.66f, 0.0f, 0.50f, 1.04f, DECOR_MAT_METAL, 12);
    b.cylinder(0.0f, 1.70f, 0.0f, 0.36f, 0.10f, DECOR_MAT_DARK_METAL, 12);
    b.cylinder(0.0f, 1.80f, 0.0f, 0.10f, 0.20f, DECOR_MAT_DARK_METAL, 8);
    // gauge on the front, feed pipe at the side
    b.box(0.0f, 0.86f, 0.50f, 0.16f, 0.16f, 0.04f, DECOR_MAT_WHITE);
    b.panel(0.0f, 0.88f, 0.522f, 0.12f, 0.12f, 0.004f, DECOR_MAT_SCREEN);
    b.cylinder(-0.42f, 0.00f, 0.40f, 0.05f, 0.95f, DECOR_MAT_ORANGE, 8);
    decorRod(b, Vec3(-0.42f, 0.90f, 0.40f), Vec3(-0.30f, 0.90f, 0.29f), 0.05f, DECOR_MAT_ORANGE, 8);
}

// Four risers on wall rails with a header pipe and hand wheels.
inline void buildValveManifoldModel(DecorBuilder& b) {
    const int pipeMat[4] = {DECOR_MAT_DRUM, DECOR_MAT_METAL, DECOR_MAT_WHITE, DECOR_MAT_METAL};
    b.box(0.0f, 0.60f, -0.145f, 1.40f, 0.06f, 0.03f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.70f, -0.145f, 1.40f, 0.06f, 0.03f, DECOR_MAT_DARK_METAL);
    for (int i = 0; i < 4; i++) {
        float x = -0.525f + 0.35f * (float)i;
        b.cylinder(x, 0.00f, -0.05f, 0.08f, 2.20f, pipeMat[i], 8);
        b.box(x, 1.04f, -0.05f, 0.18f, 0.12f, 0.18f, DECOR_MAT_DARK_METAL);
        b.box(x, 1.085f, 0.08f, 0.03f, 0.03f, 0.08f, DECOR_MAT_METAL);
        decorRod(b, Vec3(x, 1.10f, 0.12f), Vec3(x, 1.10f, 0.14f), 0.08f, DECOR_MAT_ORANGE, 8);
    }
    decorRod(b, Vec3(-0.70f, 2.25f, -0.05f), Vec3(0.70f, 2.25f, -0.05f), 0.07f, DECOR_MAT_METAL, 8);
}

inline void buildGeneratorModel(DecorBuilder& b) {
    const float f = 0.28f;
    b.box(0.0f, 0.00f, -0.24f, 1.10f, 0.06f, 0.08f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.00f, 0.24f, 1.10f, 0.06f, 0.08f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.06f, 0.0f, 1.00f, 0.50f, 0.56f, DECOR_MAT_ORANGE);
    b.box(-0.10f, 0.56f, 0.0f, 0.70f, 0.14f, 0.46f, DECOR_MAT_DARK_METAL);
    b.cylinder(-0.25f, 0.70f, 0.0f, 0.04f, 0.03f, DECOR_MAT_METAL, 8);
    b.cylinder(0.40f, 0.56f, 0.12f, 0.045f, 0.22f, DECOR_MAT_METAL, 8);
    b.box(0.28f, 0.18f, f + 0.003f, 0.34f, 0.28f, 0.006f, DECOR_MAT_DARK_METAL);
    b.panel(-0.22f, 0.22f, f + 0.003f, 0.30f, 0.20f, 0.006f, DECOR_MAT_SCREEN);
    b.box(-0.515f, 0.30f, 0.0f, 0.03f, 0.04f, 0.50f, DECOR_MAT_METAL);
    b.box(0.515f, 0.30f, 0.0f, 0.03f, 0.04f, 0.50f, DECOR_MAT_METAL);
}

inline void buildWorkbenchModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.74f * (float)side, 0.00f, 0.0f, 0.06f, 0.84f, 0.60f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.18f, 0.0f, 1.42f, 0.03f, 0.56f, DECOR_MAT_METAL);
    b.box(-0.30f, 0.21f, 0.0f, 0.40f, 0.28f, 0.40f, DECOR_MAT_CARDBOARD);
    b.box(0.0f, 0.84f, 0.0f, 1.60f, 0.05f, 0.64f, DECOR_MAT_WOOD);
    // tool board with a shelf
    b.box(0.0f, 0.89f, -0.30f, 1.60f, 0.90f, 0.04f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 1.40f, -0.20f, 1.20f, 0.03f, 0.16f, DECOR_MAT_WOOD);
    b.cylinder(-0.35f, 1.43f, -0.20f, 0.05f, 0.14f, DECOR_MAT_DRUM, 8);
    b.cylinder(-0.20f, 1.43f, -0.20f, 0.05f, 0.12f, DECOR_MAT_ORANGE, 8);
    b.box(0.25f, 1.43f, -0.20f, 0.26f, 0.10f, 0.12f, DECOR_MAT_CARDBOARD);
    // tool box and vise on the top
    b.box(0.40f, 0.89f, 0.02f, 0.44f, 0.20f, 0.22f, DECOR_MAT_ORANGE);
    b.box(0.40f, 1.09f, 0.02f, 0.20f, 0.03f, 0.03f, DECOR_MAT_DARK_METAL);
    b.box(-0.55f, 0.89f, 0.20f, 0.14f, 0.10f, 0.18f, DECOR_MAT_DARK_METAL);
    b.box(-0.55f, 0.99f, 0.24f, 0.10f, 0.06f, 0.06f, DECOR_MAT_METAL);
}

inline void buildToolChestModel(DecorBuilder& b) {
    const float f = 0.23f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.28f * (float)sx, 0.00f, 0.16f * (float)sz, 0.05f, 0.08f, 0.08f, DECOR_MAT_RUBBER);
        }
    }
    b.box(0.0f, 0.08f, 0.0f, 0.70f, 0.80f, 0.46f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.88f, 0.0f, 0.70f, 0.02f, 0.46f, DECOR_MAT_DARK_METAL);
    for (int i = 0; i < 5; i++) {
        float y = 0.12f + 0.15f * (float)i;
        b.box(0.0f, y, f + 0.003f, 0.64f, 0.13f, 0.006f, DECOR_MAT_DARK_METAL);
        b.box(0.0f, y + 0.09f, f + 0.012f, 0.30f, 0.02f, 0.012f, DECOR_MAT_METAL);
    }
    b.box(0.365f, 0.60f, 0.0f, 0.03f, 0.04f, 0.24f, DECOR_MAT_METAL);
    b.box(-0.365f, 0.60f, 0.0f, 0.03f, 0.04f, 0.24f, DECOR_MAT_METAL);
}

// Access hatch set into the floor.
inline void buildFloorHatchModel(DecorBuilder& b) {
    b.box(0.0f, 0.000f, 0.0f, 0.96f, 0.012f, 0.96f, DECOR_MAT_CAUTION);
    b.box(0.0f, 0.012f, 0.0f, 0.84f, 0.008f, 0.84f, DECOR_MAT_METAL);
    b.box(0.0f, 0.020f, -0.36f, 0.50f, 0.012f, 0.04f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.020f, 0.28f, 0.16f, 0.004f, 0.06f, DECOR_MAT_RUBBER);
}

// Plant-room console: desk with switch gear, monitors in the upright back section.
inline void buildControlConsoleModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, -0.05f, 1.40f, 0.72f, 0.60f, DECOR_MAT_METAL);
    b.box(0.0f, 0.72f, 0.0f, 1.40f, 0.05f, 0.70f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.77f, -0.23f, 1.40f, 0.50f, 0.24f, DECOR_MAT_METAL);
    b.panel(-0.36f, 0.84f, -0.107f, 0.56f, 0.36f, 0.006f, DECOR_MAT_SCREEN);
    b.panel(0.36f, 0.84f, -0.107f, 0.56f, 0.36f, 0.006f, DECOR_MAT_SCREEN);
    b.box(-0.40f, 0.77f, 0.12f, 0.30f, 0.012f, 0.20f, DECOR_MAT_RUBBER);
    b.box(0.30f, 0.77f, 0.12f, 0.44f, 0.02f, 0.24f, DECOR_MAT_CAUTION);
    b.cylinder(0.18f, 0.79f, 0.12f, 0.03f, 0.03f, DECOR_MAT_ORANGE, 6);
    b.cylinder(0.30f, 0.79f, 0.12f, 0.03f, 0.03f, DECOR_MAT_WHITE, 6);
    b.cylinder(0.42f, 0.79f, 0.12f, 0.03f, 0.03f, DECOR_MAT_ORANGE, 6);
}

// Motor and pump on a common skid, outlet flange pointing up.
inline void buildPumpUnitModel(DecorBuilder& b) {
    const float y = 0.28f;
    b.box(0.0f, 0.00f, 0.0f, 1.00f, 0.08f, 0.50f, DECOR_MAT_DARK_METAL);
    decorRod(b, Vec3(-0.42f, y, 0.0f), Vec3(0.05f, y, 0.0f), 0.20f, DECOR_MAT_DRUM, 10);
    b.box(-0.20f, y + 0.20f, 0.0f, 0.16f, 0.08f, 0.14f, DECOR_MAT_DARK_METAL);
    decorRod(b, Vec3(0.05f, y, 0.0f), Vec3(0.15f, y, 0.0f), 0.06f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(0.15f, y, 0.0f), Vec3(0.32f, y, 0.0f), 0.20f, DECOR_MAT_METAL, 10);
    decorRod(b, Vec3(0.32f, y, 0.0f), Vec3(0.48f, y, 0.0f), 0.08f, DECOR_MAT_METAL, 8);
    decorRod(b, Vec3(0.48f, y, 0.0f), Vec3(0.50f, y, 0.0f), 0.11f, DECOR_MAT_DARK_METAL, 8);
    b.cylinder(0.235f, y + 0.20f, 0.0f, 0.07f, 0.22f, DECOR_MAT_METAL, 8);
    b.cylinder(0.235f, y + 0.42f, 0.0f, 0.10f, 0.03f, DECOR_MAT_DARK_METAL, 8);
}
