#pragma once
#include "decor_parts.h"

extern const float CS, WH;

// ============================================================================
// SHARED PROPS
// Things that turn up everywhere: bins, benches, vending machines, pipes, ducts.
// See prop_catalog.h for the themes each one is tagged with.
// Same rules as prop_models.h: local metres, +Z = front, origin = centre of the
// footprint, parts rest on each other. Wall props reach exactly to -hz (the wall).
// ============================================================================

inline void buildTrashBinModel(DecorBuilder& b) {
    b.cylinder(0.0f, 0.00f, 0.0f, 0.19f, 0.02f, DECOR_MAT_RUBBER, 10);
    b.cylinder(0.0f, 0.02f, 0.0f, 0.20f, 0.60f, DECOR_MAT_METAL, 10);
    b.cylinder(0.0f, 0.62f, 0.0f, 0.21f, 0.04f, DECOR_MAT_DARK_METAL, 10);
    b.cylinder(0.0f, 0.66f, 0.0f, 0.15f, 0.004f, DECOR_MAT_RUBBER, 10);
}

inline void buildBenchModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.60f * (float)side, 0.00f, 0.0f, 0.05f, 0.40f, 0.36f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.30f, 0.0f, 1.15f, 0.04f, 0.04f, DECOR_MAT_DARK_METAL);
    for (int i = -1; i <= 1; i++) {
        b.box(0.0f, 0.40f, 0.13f * (float)i, 1.50f, 0.04f, 0.11f, DECOR_MAT_WOOD);
    }
}

inline void buildVendingMachineModel(DecorBuilder& b) {
    const float f = 0.38f;
    b.box(0.0f, 0.00f, 0.0f, 0.86f, 0.06f, 0.70f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.06f, 0.0f, 0.90f, 1.80f, 0.76f, DECOR_MAT_DARK_METAL);
    b.panel(-0.13f, 0.56f, f + 0.003f, 0.58f, 1.22f, 0.006f, DECOR_MAT_SCREEN);
    b.box(-0.13f, 0.20f, f + 0.005f, 0.58f, 0.22f, 0.01f, DECOR_MAT_RUBBER);
    b.box(0.31f, 1.10f, f + 0.005f, 0.20f, 0.40f, 0.01f, DECOR_MAT_METAL);
    b.panel(0.31f, 1.56f, f + 0.005f, 0.20f, 0.14f, 0.01f, DECOR_MAT_WHITE);
    b.box(0.31f, 0.86f, f + 0.005f, 0.12f, 0.08f, 0.01f, DECOR_MAT_ORANGE);
}

inline void buildWaterCoolerModel(DecorBuilder& b) {
    const float cz = -0.02f;
    const float f = cz + 0.17f;
    b.box(0.0f, 0.00f, cz, 0.34f, 0.96f, 0.34f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.96f, cz, 0.30f, 0.04f, 0.30f, DECOR_MAT_METAL);
    b.cylinder(0.0f, 1.00f, cz, 0.13f, 0.34f, DECOR_MAT_DRUM, 10);
    b.cylinder(0.0f, 1.34f, cz, 0.07f, 0.06f, DECOR_MAT_DRUM, 8);
    b.box(0.0f, 0.60f, f + 0.003f, 0.24f, 0.20f, 0.006f, DECOR_MAT_DARK_METAL);
    b.box(-0.05f, 0.74f, f + 0.02f, 0.03f, 0.03f, 0.04f, DECOR_MAT_DRUM);
    b.box(0.05f, 0.74f, f + 0.02f, 0.03f, 0.03f, 0.04f, DECOR_MAT_ORANGE);
}

// Hose / extinguisher cabinet hung on the wall at chest height.
inline void buildFireCabinetModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.09f;
    b.box(0.0f, 0.90f, cz, 0.60f, 0.70f, 0.18f, DECOR_MAT_ORANGE);
    b.panel(0.0f, 0.96f, f + 0.003f, 0.48f, 0.58f, 0.006f, DECOR_MAT_WHITE);
    b.box(0.17f, 1.20f, f + 0.01f, 0.03f, 0.10f, 0.02f, DECOR_MAT_METAL);
}

inline void buildWetFloorSignModel(DecorBuilder& b) {
    b.taper(0.0f, 0.00f, 0.0f, 0.30f, 0.34f, 0.26f, 0.03f, 0.62f, DECOR_MAT_CAUTION);
    b.box(0.0f, 0.62f, 0.0f, 0.20f, 0.03f, 0.05f, DECOR_MAT_RUBBER);
}

// Three pipes on wall brackets, above head height.
inline void buildPipeRunModel(DecorBuilder& b) {
    const float y = 3.00f;
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.90f * (float)side;
        b.box(x, y, -0.18f, 0.06f, 0.72f, 0.04f, DECOR_MAT_DARK_METAL);
        b.box(x, y + 0.02f, 0.0f, 0.06f, 0.04f, 0.32f, DECOR_MAT_DARK_METAL);
        b.box(x, y + 0.40f, 0.0f, 0.06f, 0.04f, 0.32f, DECOR_MAT_DARK_METAL);
    }
    decorRod(b, Vec3(-1.20f, y + 0.13f, -0.06f), Vec3(1.20f, y + 0.13f, -0.06f), 0.07f, DECOR_MAT_DRUM, 8);
    decorRod(b, Vec3(-1.20f, y + 0.11f, 0.10f), Vec3(1.20f, y + 0.11f, 0.10f), 0.05f, DECOR_MAT_METAL, 8);
    decorRod(b, Vec3(-1.20f, y + 0.53f, 0.0f), Vec3(1.20f, y + 0.53f, 0.0f), 0.09f, DECOR_MAT_WHITE, 8);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.40f * (float)side;
        decorRod(b, Vec3(x - 0.03f, y + 0.53f, 0.0f), Vec3(x + 0.03f, y + 0.53f, 0.0f), 0.11f, DECOR_MAT_DARK_METAL, 8);
    }
}

// Sheet-metal duct hung under the ceiling.
inline void buildVentDuctModel(DecorBuilder& b) {
    const float y = WH - 0.46f;
    b.box(0.0f, y, 0.0f, 2.20f, 0.34f, 0.50f, DECOR_MAT_METAL);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.80f * (float)side, y + 0.34f, 0.0f, 0.05f, 0.12f, 0.40f, DECOR_MAT_DARK_METAL);
        b.box(0.40f * (float)side, y - 0.005f, 0.0f, 0.03f, 0.35f, 0.51f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, y - 0.01f, 0.0f, 0.60f, 0.01f, 0.36f, DECOR_MAT_DARK_METAL);
}

inline void buildElectricalCabinetModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.18f;
    b.box(0.0f, 0.00f, cz, 0.76f, 0.08f, 0.32f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, cz, 0.80f, 1.72f, 0.36f, DECOR_MAT_METAL);
    b.box(0.22f, 1.80f, cz - 0.06f, 0.20f, 0.14f, 0.20f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.10f, f + 0.003f, 0.012f, 1.68f, 0.006f, DECOR_MAT_DARK_METAL);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.20f * (float)side, 0.20f, f + 0.003f, 0.28f, 0.22f, 0.006f, DECOR_MAT_DARK_METAL);
    }
    b.panel(-0.20f, 1.30f, f + 0.003f, 0.22f, 0.22f, 0.006f, DECOR_MAT_CAUTION);
    b.panel(0.20f, 1.34f, f + 0.003f, 0.18f, 0.12f, 0.006f, DECOR_MAT_SCREEN);
    b.box(0.06f, 0.95f, f + 0.01f, 0.03f, 0.14f, 0.02f, DECOR_MAT_DARK_METAL);
}

inline void buildStepLadderModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.22f * (float)side;
        decorBeam(b, Vec3(x, 0.0f, 0.40f), Vec3(x, 1.50f, 0.02f), 0.04f, 0.07f, DECOR_MAT_ORANGE, true);
        decorBeam(b, Vec3(x * 0.9f, 0.0f, -0.40f), Vec3(x * 0.9f, 1.50f, -0.02f), 0.03f, 0.05f, DECOR_MAT_METAL, true);
    }
    for (int i = 1; i <= 4; i++) {
        float y = 0.30f * (float)i;
        float z = 0.40f - 0.38f * (y / 1.50f);
        b.box(0.0f, y, z, 0.40f, 0.03f, 0.10f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 1.50f, 0.0f, 0.50f, 0.04f, 0.20f, DECOR_MAT_ORANGE);
}

// Sack truck standing upright with two boxes on the toe plate.
inline void buildHandTruckModel(DecorBuilder& root) {
    DecorBuilder b = root.sub(0.0f, -0.025f, 0.0f);
    decorWheel(b, -0.20f, 0.10f, -0.10f, 0.10f, 0.05f);
    decorWheel(b, 0.20f, 0.10f, -0.10f, 0.10f, 0.05f);
    decorRod(b, Vec3(-0.175f, 0.10f, -0.10f), Vec3(0.175f, 0.10f, -0.10f), 0.015f, DECOR_MAT_METAL, 6);
    b.box(0.0f, 0.00f, 0.085f, 0.34f, 0.02f, 0.33f, DECOR_MAT_METAL);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.14f * (float)side, 0.02f, -0.065f, 0.03f, 1.18f, 0.03f, DECOR_MAT_ORANGE);
    }
    b.box(0.0f, 0.45f, -0.065f, 0.25f, 0.03f, 0.02f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.85f, -0.065f, 0.25f, 0.03f, 0.02f, DECOR_MAT_ORANGE);
    b.box(0.0f, 1.20f, -0.065f, 0.31f, 0.03f, 0.03f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.02f, 0.10f, 0.32f, 0.30f, 0.28f, DECOR_MAT_CARDBOARD);
    b.box(0.0f, 0.32f, 0.09f, 0.28f, 0.24f, 0.24f, DECOR_MAT_CARDBOARD);
}

inline void buildShelfUnitModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.575f * (float)sx, 0.00f, 0.195f * (float)sz, 0.05f, 1.90f, 0.05f, DECOR_MAT_METAL);
        }
    }
    const float shelfY[5] = {0.12f, 0.58f, 1.04f, 1.50f, 1.86f};
    for (int i = 0; i < 5; i++) {
        b.box(0.0f, shelfY[i], 0.0f, 1.10f, 0.03f, 0.44f, DECOR_MAT_METAL);
    }
    b.box(-0.28f, 0.15f, 0.0f, 0.42f, 0.30f, 0.34f, DECOR_MAT_CARDBOARD);
    b.box(0.26f, 0.15f, 0.0f, 0.36f, 0.24f, 0.32f, DECOR_MAT_WOOD);
    b.cylinder(-0.30f, 0.61f, 0.0f, 0.13f, 0.30f, DECOR_MAT_DRUM, 8);
    b.box(0.20f, 0.61f, 0.0f, 0.50f, 0.26f, 0.34f, DECOR_MAT_CARDBOARD);
    b.box(-0.10f, 1.07f, 0.0f, 0.60f, 0.22f, 0.36f, DECOR_MAT_WHITE);
    b.box(0.30f, 1.53f, 0.0f, 0.34f, 0.20f, 0.30f, DECOR_MAT_DARK_METAL);
}

// Direction board on two rods: lit face to the front, hazard face to the back.
inline void buildHangingSignModel(DecorBuilder& b) {
    const float y = WH - 0.86f;
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.45f * (float)side, y + 0.36f, 0.0f, 0.02f, 0.50f, 0.02f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, y, 0.0f, 1.20f, 0.36f, 0.05f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, y + 0.03f, 0.028f, 1.10f, 0.30f, 0.006f, DECOR_MAT_SCREEN);
    b.panel(0.0f, y + 0.03f, -0.028f, 1.10f, 0.30f, 0.006f, DECOR_MAT_CAUTION);
}

inline void buildFloorDrainModel(DecorBuilder& b) {
    b.box(0.0f, 0.000f, 0.0f, 0.56f, 0.008f, 0.56f, DECOR_MAT_METAL);
    b.box(0.0f, 0.008f, 0.0f, 0.46f, 0.004f, 0.46f, DECOR_MAT_DARK_METAL);
}

inline void buildCeilingFanModel(DecorBuilder& b) {
    const float y = WH - 0.52f;
    b.cylinder(0.0f, y + 0.12f, 0.0f, 0.02f, 0.40f, DECOR_MAT_METAL, 6);
    b.cylinder(0.0f, y, 0.0f, 0.11f, 0.12f, DECOR_MAT_WHITE, 8);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.37f * (float)side, y + 0.05f, 0.0f, 0.52f, 0.012f, 0.13f, DECOR_MAT_WOOD);
        b.box(0.0f, y + 0.05f, 0.37f * (float)side, 0.13f, 0.012f, 0.52f, DECOR_MAT_WOOD);
    }
}

// Lit sign box high on the wall (exit, floor number, room name).
inline void buildWallSignModel(DecorBuilder& b) {
    b.box(0.0f, 2.80f, -0.005f, 0.80f, 0.30f, 0.09f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 2.83f, 0.043f, 0.72f, 0.24f, 0.006f, DECOR_MAT_SCREEN);
}

// Cable tray on trapeze hangers under the ceiling.
inline void buildCableTrayModel(DecorBuilder& b) {
    const float y = WH - 0.38f;
    for (int sx = -1; sx <= 1; sx += 2) {
        float x = 0.80f * (float)sx;
        b.box(x, y - 0.03f, 0.0f, 0.04f, 0.03f, 0.44f, DECOR_MAT_DARK_METAL);
        b.box(x, y, -0.20f, 0.02f, 0.38f, 0.02f, DECOR_MAT_DARK_METAL);
        b.box(x, y, 0.20f, 0.02f, 0.38f, 0.02f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, y, -0.15f, 2.20f, 0.06f, 0.02f, DECOR_MAT_METAL);
    b.box(0.0f, y, 0.15f, 2.20f, 0.06f, 0.02f, DECOR_MAT_METAL);
    decorRod(b, Vec3(-1.10f, y + 0.035f, -0.07f), Vec3(1.10f, y + 0.035f, -0.07f), 0.035f, DECOR_MAT_RUBBER, 6);
    decorRod(b, Vec3(-1.10f, y + 0.030f, 0.02f), Vec3(1.10f, y + 0.030f, 0.02f), 0.030f, DECOR_MAT_ORANGE, 6);
    decorRod(b, Vec3(-1.10f, y + 0.025f, 0.09f), Vec3(1.10f, y + 0.025f, 0.09f), 0.025f, DECOR_MAT_RUBBER, 6);
}

inline void buildFloorMatModel(DecorBuilder& b) {
    b.box(0.0f, 0.000f, 0.0f, 1.40f, 0.012f, 0.90f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.012f, 0.0f, 1.24f, 0.006f, 0.74f, DECOR_MAT_FABRIC);
}

// Loose sheets scattered over the floor.
inline void buildPaperLitterModel(DecorBuilder& b) {
    const float sheet[6][3] = {
        {-0.28f, -0.20f, 0.4f}, {0.10f, -0.28f, 1.2f}, {0.30f, 0.05f, -0.5f},
        {-0.08f, 0.12f, 2.0f}, {-0.30f, 0.26f, 0.9f}, {0.22f, 0.30f, 0.1f}
    };
    for (int i = 0; i < 6; i++) {
        DecorBuilder page = b.sub(sheet[i][0], sheet[i][1], sheet[i][2]);
        page.panel(0.0f, 0.002f + 0.003f * (float)i, 0.0f, 0.21f, 0.002f, 0.29f, DECOR_MAT_WHITE);
    }
}
