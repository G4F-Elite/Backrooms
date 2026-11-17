#pragma once
#include "decor_builder.h"
#include "map_content.h"
#include "props_library.h"

// ============================================================================
// PROP MODELS
// Local metres, +Z = front, origin = centre of the footprint on the floor.
// Every model stays inside mapPropBaseExtents() so collision matches the mesh,
// and parts rest exactly on each other (no gaps, no interpenetrating faces).
// ============================================================================

inline void buildCrateStackModel(DecorBuilder& b) {
    b.box(-0.08f, 0.00f, -0.06f, 0.84f, 0.07f, 0.84f, DECOR_MAT_TRIM);
    b.box(-0.08f, 0.07f, -0.06f, 0.80f, 0.66f, 0.80f, DECOR_MAT_WOOD);
    b.box(-0.08f, 0.73f, -0.06f, 0.84f, 0.07f, 0.84f, DECOR_MAT_TRIM);
    DecorBuilder top = b.sub(-0.10f, -0.08f, 0.5f);
    top.box(0.0f, 0.80f, 0.0f, 0.46f, 0.42f, 0.46f, DECOR_MAT_WOOD);
    top.box(0.0f, 1.22f, 0.0f, 0.49f, 0.04f, 0.49f, DECOR_MAT_TRIM);
}

inline void buildConeModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.0f, 0.30f, 0.03f, 0.30f, DECOR_MAT_RUBBER);
    b.taper(0.0f, 0.03f, 0.0f, 0.220f, 0.220f, 0.163f, 0.163f, 0.15f, DECOR_MAT_ORANGE);
    b.taper(0.0f, 0.18f, 0.0f, 0.163f, 0.163f, 0.107f, 0.107f, 0.15f, DECOR_MAT_WHITE);
    b.taper(0.0f, 0.33f, 0.0f, 0.107f, 0.107f, 0.050f, 0.050f, 0.15f, DECOR_MAT_ORANGE);
}

inline void buildConeClusterModel(DecorBuilder& b) {
    DecorBuilder c0 = b.sub(-0.24f, -0.14f, 0.3f);
    DecorBuilder c1 = b.sub(0.22f, -0.20f, 1.1f);
    DecorBuilder c2 = b.sub(0.02f, 0.26f, 0.7f);
    buildConeModel(c0);
    buildConeModel(c1);
    buildConeModel(c2);
}

inline void buildBarrierModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.62f * (float)side;
        b.box(x, 0.00f, 0.0f, 0.10f, 0.05f, 0.46f, DECOR_MAT_RUBBER);
        b.box(x, 0.05f, 0.0f, 0.08f, 0.93f, 0.08f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.70f, 0.06f, 1.56f, 0.20f, 0.04f, DECOR_MAT_CAUTION);
    b.box(0.0f, 0.30f, 0.06f, 1.56f, 0.12f, 0.04f, DECOR_MAT_CAUTION);
}

inline void buildCableReelModel(DecorBuilder& b) {
    b.cylinder(0.0f, 0.00f, 0.0f, 0.48f, 0.06f, DECOR_MAT_WOOD, 12);
    b.cylinder(0.0f, 0.06f, 0.0f, 0.22f, 0.50f, DECOR_MAT_RUBBER, 12);
    b.cylinder(0.0f, 0.56f, 0.0f, 0.48f, 0.06f, DECOR_MAT_WOOD, 12);
    b.cylinder(0.0f, 0.62f, 0.0f, 0.07f, 0.02f, DECOR_MAT_METAL, 8);
}

inline void buildPuddleModel(DecorBuilder& b) {
    b.cylinder(-0.10f, 0.002f, 0.00f, 0.62f, 0.006f, DECOR_MAT_PUDDLE, 12);
    b.cylinder(0.42f, 0.002f, 0.18f, 0.40f, 0.009f, DECOR_MAT_PUDDLE, 10);
    b.cylinder(0.20f, 0.002f, -0.36f, 0.30f, 0.012f, DECOR_MAT_PUDDLE, 9);
}

inline void buildDebrisModel(DecorBuilder& b) {
    DecorBuilder tileA = b.sub(-0.28f, -0.10f, 0.25f);
    tileA.box(0.0f, 0.00f, 0.0f, 0.50f, 0.03f, 0.50f, DECOR_MAT_CEILTILE);
    DecorBuilder board = tileA.sub(0.0f, 0.0f, 1.0f);
    board.box(0.0f, 0.03f, 0.0f, 0.44f, 0.04f, 0.09f, DECOR_MAT_WOOD);
    DecorBuilder tileB = b.sub(0.32f, 0.12f, -0.4f);
    tileB.box(0.0f, 0.00f, 0.0f, 0.42f, 0.03f, 0.36f, DECOR_MAT_CEILTILE);
    b.box(0.05f, 0.00f, -0.36f, 0.16f, 0.10f, 0.14f, DECOR_MAT_PLASTER);
    b.box(-0.02f, 0.00f, 0.36f, 0.12f, 0.07f, 0.10f, DECOR_MAT_PLASTER);
}

inline void buildDeskModel(DecorBuilder& b) {
    b.box(0.0f, 0.72f, 0.0f, 1.40f, 0.04f, 0.70f, DECOR_MAT_WOOD);
    b.box(-0.68f, 0.00f, 0.0f, 0.04f, 0.72f, 0.66f, DECOR_MAT_WOOD);
    b.box(0.68f, 0.00f, 0.0f, 0.04f, 0.72f, 0.66f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.25f, -0.31f, 1.32f, 0.47f, 0.03f, DECOR_MAT_WOOD);
    // drawer pedestal, flush with the right panel and the underside of the top
    b.box(0.45f, 0.12f, 0.02f, 0.42f, 0.60f, 0.58f, DECOR_MAT_METAL);
    for (int i = 0; i < 3; i++) {
        float y = 0.14f + 0.19f * (float)i;
        b.box(0.45f, y, 0.32f, 0.38f, 0.17f, 0.02f, DECOR_MAT_DARK_METAL);
    }
    // CRT terminal + keyboard
    b.box(-0.20f, 0.76f, -0.08f, 0.22f, 0.03f, 0.20f, DECOR_MAT_DARK_METAL);
    b.box(-0.20f, 0.79f, -0.10f, 0.40f, 0.34f, 0.36f, DECOR_MAT_WHITE);
    b.panel(-0.20f, 0.83f, 0.085f, 0.32f, 0.26f, 0.01f, DECOR_MAT_SCREEN);
    b.box(-0.20f, 0.76f, 0.20f, 0.36f, 0.02f, 0.13f, DECOR_MAT_WHITE);
}

inline void buildChairModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.19f * (float)sx, 0.00f, 0.19f * (float)sz, 0.04f, 0.42f, 0.04f, DECOR_MAT_METAL);
        }
    }
    b.box(0.0f, 0.42f, 0.0f, 0.44f, 0.06f, 0.44f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.48f, -0.20f, 0.42f, 0.42f, 0.04f, DECOR_MAT_FABRIC);
}

inline void buildCabinetModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.0f, 0.48f, 1.32f, 0.58f, DECOR_MAT_METAL);
    for (int i = 0; i < 4; i++) {
        float y = 0.02f + 0.32f * (float)i;
        b.box(0.0f, y, 0.292f, 0.46f, 0.012f, 0.006f, DECOR_MAT_DARK_METAL);
        b.box(0.0f, y + 0.20f, 0.30f, 0.14f, 0.025f, 0.02f, DECOR_MAT_DARK_METAL);
    }
}

inline void buildPartitionModel(DecorBuilder& b) {
    b.box(-0.60f, 0.00f, 0.0f, 0.06f, 0.03f, 0.28f, DECOR_MAT_METAL);
    b.box(0.60f, 0.00f, 0.0f, 0.06f, 0.03f, 0.28f, DECOR_MAT_METAL);
    b.box(0.0f, 0.03f, 0.0f, 1.45f, 0.05f, 0.05f, DECOR_MAT_METAL);
    b.box(0.0f, 0.08f, 0.0f, 1.45f, 1.42f, 0.05f, DECOR_MAT_FABRIC);
    b.box(0.0f, 1.50f, 0.0f, 1.50f, 0.03f, 0.07f, DECOR_MAT_METAL);
    b.box(-0.7375f, 0.03f, 0.0f, 0.025f, 1.47f, 0.07f, DECOR_MAT_METAL);
    b.box(0.7375f, 0.03f, 0.0f, 0.025f, 1.47f, 0.07f, DECOR_MAT_METAL);
}

inline void buildBoxPalletModel(DecorBuilder& b) {
    for (int i = -1; i <= 1; i++) {
        b.box(0.50f * (float)i, 0.00f, 0.0f, 0.10f, 0.10f, 1.00f, DECOR_MAT_WOOD);
    }
    for (int i = -2; i <= 2; i++) {
        b.box(0.0f, 0.10f, 0.22f * (float)i, 1.20f, 0.025f, 0.12f, DECOR_MAT_WOOD);
    }
    const float deck = 0.125f;
    b.box(-0.29f, deck, -0.22f, 0.56f, 0.42f, 0.52f, DECOR_MAT_CARDBOARD);
    b.box(0.29f, deck, -0.22f, 0.56f, 0.42f, 0.52f, DECOR_MAT_CARDBOARD);
    b.box(-0.28f, deck, 0.27f, 0.50f, 0.36f, 0.40f, DECOR_MAT_CARDBOARD);
    b.box(0.30f, deck, 0.26f, 0.48f, 0.42f, 0.42f, DECOR_MAT_CARDBOARD);
    DecorBuilder top = b.sub(-0.05f, -0.20f, 0.2f);
    top.box(0.0f, deck + 0.42f, 0.0f, 0.60f, 0.36f, 0.44f, DECOR_MAT_CARDBOARD);
}

inline void buildDrumModel(DecorBuilder& b, float x, float z, int mat) {
    b.cylinder(x, 0.00f, z, 0.29f, 0.88f, mat, 12);
    b.cylinder(x + 0.12f, 0.88f, z + 0.10f, 0.035f, 0.02f, DECOR_MAT_METAL, 6);
}

inline void buildDrumStackModel(DecorBuilder& b) {
    buildDrumModel(b, -0.30f, -0.25f, DECOR_MAT_DRUM);
    buildDrumModel(b, 0.30f, -0.25f, DECOR_MAT_METAL);
    buildDrumModel(b, 0.00f, 0.27f, DECOR_MAT_DRUM);
}

inline void buildLockerBankModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, -0.01f, 1.10f, 0.08f, 0.40f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, 0.0f, 1.14f, 1.72f, 0.44f, DECOR_MAT_METAL);
    b.box(-0.19f, 0.10f, 0.222f, 0.012f, 1.68f, 0.006f, DECOR_MAT_DARK_METAL);
    b.box(0.19f, 0.10f, 0.222f, 0.012f, 1.68f, 0.006f, DECOR_MAT_DARK_METAL);
    for (int i = -1; i <= 1; i++) {
        float x = 0.38f * (float)i;
        b.box(x, 1.52f, 0.222f, 0.22f, 0.10f, 0.006f, DECOR_MAT_DARK_METAL);
        b.box(x + 0.12f, 0.95f, 0.228f, 0.03f, 0.12f, 0.016f, DECOR_MAT_DARK_METAL);
    }
}

inline void buildMapPropModel(std::vector<float>& v, const MapProp& pr) {
    DecorBuilder b(v, pr.pos.x, pr.pos.z, pr.yaw, pr.scale);
    if (pr.type == MAP_PROP_CRATE_STACK) buildCrateStackModel(b);
    else if (pr.type == MAP_PROP_CONE_CLUSTER) buildConeClusterModel(b);
    else if (pr.type == MAP_PROP_BARRIER) buildBarrierModel(b);
    else if (pr.type == MAP_PROP_CABLE_REEL) buildCableReelModel(b);
    else if (pr.type == MAP_PROP_PUDDLE) buildPuddleModel(b);
    else if (pr.type == MAP_PROP_DEBRIS) buildDebrisModel(b);
    else if (pr.type == MAP_PROP_DESK) buildDeskModel(b);
    else if (pr.type == MAP_PROP_CHAIR) buildChairModel(b);
    else if (pr.type == MAP_PROP_CABINET) buildCabinetModel(b);
    else if (pr.type == MAP_PROP_PARTITION) buildPartitionModel(b);
    else if (pr.type == MAP_PROP_BOX_PALLET) buildBoxPalletModel(b);
    else if (pr.type == MAP_PROP_DRUM_STACK) buildDrumStackModel(b);
    else if (pr.type == MAP_PROP_LOCKER_BANK) buildLockerBankModel(b);
    else buildLibraryPropModel(b, pr.type);
}
