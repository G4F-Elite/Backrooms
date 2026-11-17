#pragma once
#include "decor_parts.h"

// ============================================================================
// STATION PROPS
// Metro concourse and platforms: gates, ticket machines, maps.
// Themes: see prop_catalog.h.
// ============================================================================

// Fare gate: two cabinets with a tripod barrier in the lane between them.
inline void buildTurnstileModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.42f * (float)side;
        b.box(x, 0.00f, 0.0f, 0.26f, 0.95f, 0.98f, DECOR_MAT_METAL);
        b.box(x, 0.95f, 0.0f, 0.28f, 0.04f, 1.00f, DECOR_MAT_DARK_METAL);
        b.box(x, 0.06f, 0.493f, 0.22f, 0.12f, 0.006f, DECOR_MAT_DARK_METAL);
        b.panel(x, 0.66f, 0.493f, 0.12f, 0.12f, 0.006f, DECOR_MAT_SCREEN);
    }
    b.box(0.42f, 0.99f, 0.30f, 0.14f, 0.012f, 0.18f, DECOR_MAT_ORANGE);
    b.box(0.42f, 1.002f, 0.30f, 0.08f, 0.004f, 0.02f, DECOR_MAT_RUBBER);
    b.panel(0.42f, 0.99f, 0.08f, 0.14f, 0.006f, 0.14f, DECOR_MAT_SCREEN);
    // tripod: one arm across the lane, two pointing down
    const Vec3 hub(-0.20f, 0.72f, 0.0f);
    decorRod(b, Vec3(-0.29f, 0.72f, 0.0f), Vec3(-0.19f, 0.72f, 0.0f), 0.08f, DECOR_MAT_DARK_METAL, 8);
    decorRod(b, hub, Vec3(0.27f, 0.72f, 0.0f), 0.02f, DECOR_MAT_METAL, 6);
    decorRod(b, hub, Vec3(-0.02f, 0.42f, 0.30f), 0.02f, DECOR_MAT_METAL, 6);
    decorRod(b, hub, Vec3(-0.02f, 0.42f, -0.30f), 0.02f, DECOR_MAT_METAL, 6);
}

inline void buildTicketMachineModel(DecorBuilder& b) {
    const float cz = -0.01f;
    const float f = cz + 0.24f;
    b.box(0.0f, 0.00f, cz, 0.76f, 0.10f, 0.44f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.10f, cz, 0.80f, 1.70f, 0.48f, DECOR_MAT_DRUM);
    b.box(0.0f, 1.80f, cz, 0.84f, 0.12f, 0.48f, DECOR_MAT_ORANGE);
    b.panel(0.0f, 1.82f, f + 0.002f, 0.60f, 0.08f, 0.004f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.80f, f + 0.003f, 0.68f, 0.92f, 0.006f, DECOR_MAT_METAL);
    b.panel(0.0f, 0.20f, f + 0.003f, 0.60f, 0.50f, 0.006f, DECOR_MAT_WHITE);
    b.panel(-0.10f, 1.30f, f + 0.008f, 0.36f, 0.30f, 0.004f, DECOR_MAT_SCREEN);
    for (int i = 0; i < 4; i++) {
        b.box(0.22f, 1.32f + 0.07f * (float)i, f + 0.012f, 0.10f, 0.045f, 0.012f, DECOR_MAT_WHITE);
    }
    b.box(-0.10f, 1.04f, f + 0.012f, 0.16f, 0.18f, 0.012f, DECOR_MAT_DARK_METAL);
    b.box(0.22f, 1.12f, f + 0.010f, 0.05f, 0.10f, 0.008f, DECOR_MAT_DARK_METAL);
    b.box(0.22f, 1.04f, f + 0.010f, 0.10f, 0.03f, 0.008f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.85f, f + 0.008f, 0.44f, 0.11f, 0.004f, DECOR_MAT_RUBBER);
}

// Network map: coloured lines with interchange dots on a white board.
inline void buildRouteMapModel(DecorBuilder& b) {
    const float z = 0.0355f;
    b.box(0.0f, 0.90f, -0.005f, 1.60f, 1.20f, 0.07f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 0.96f, 0.032f, 1.48f, 1.08f, 0.004f, DECOR_MAT_WHITE);
    b.box(0.0f, 1.52f, z, 1.30f, 0.03f, 0.003f, DECOR_MAT_ORANGE);
    b.box(-0.28f, 1.06f, z + 0.001f, 0.03f, 0.88f, 0.003f, DECOR_MAT_DRUM);
    b.box(0.18f, 1.22f, z + 0.001f, 0.84f, 0.03f, 0.003f, DECOR_MAT_TRIM);
    decorBeam(b, Vec3(-0.62f, 1.10f, z + 0.002f), Vec3(0.56f, 1.92f, z + 0.002f), 0.003f, 0.03f, DECOR_MAT_RUBBER);
    const float stop[10][2] = {
        {-0.56f, 1.535f}, {-0.28f, 1.535f}, {0.02f, 1.535f}, {0.34f, 1.535f}, {0.60f, 1.535f},
        {-0.28f, 1.14f}, {-0.28f, 1.86f}, {-0.20f, 1.235f}, {0.56f, 1.235f}, {0.40f, 1.81f}
    };
    for (int i = 0; i < 10; i++) {
        decorRod(b, Vec3(stop[i][0], stop[i][1], 0.034f), Vec3(stop[i][0], stop[i][1], 0.040f), 0.03f, DECOR_MAT_RUBBER, 6);
        b.box(stop[i][0], stop[i][1] - 0.012f, 0.0405f, 0.024f, 0.024f, 0.001f, DECOR_MAT_WHITE);
    }
    b.box(0.50f, 1.00f, 0.035f, 0.36f, 0.14f, 0.002f, DECOR_MAT_CEILTILE);
    b.box(-0.58f, 1.00f, 0.035f, 0.20f, 0.05f, 0.002f, DECOR_MAT_ORANGE);
}

// Warning strip along a platform edge.
inline void buildTactileStripModel(DecorBuilder& b) {
    b.box(0.0f, 0.000f, 0.06f, 2.00f, 0.008f, 0.40f, DECOR_MAT_CAUTION);
    b.box(0.0f, 0.000f, -0.20f, 2.00f, 0.006f, 0.12f, DECOR_MAT_WHITE);
    // worn-through patches
    b.box(-0.52f, 0.008f, 0.08f, 0.30f, 0.001f, 0.22f, DECOR_MAT_RUBBER);
    b.box(0.38f, 0.008f, 0.02f, 0.18f, 0.001f, 0.14f, DECOR_MAT_RUBBER);
}

// Attendant's booth: counter window at the front, door on the right.
inline void buildTicketBoothModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.0f, 1.40f, 0.06f, 1.20f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.06f, 0.57f, 1.40f, 0.94f, 0.06f, DECOR_MAT_METAL);
    b.box(0.0f, 0.06f, -0.57f, 1.40f, 2.10f, 0.06f, DECOR_MAT_METAL);
    b.box(-0.67f, 0.06f, 0.0f, 0.06f, 0.94f, 1.08f, DECOR_MAT_METAL);
    b.panel(0.67f, 0.06f, 0.0f, 0.06f, 2.10f, 1.08f, DECOR_MAT_DOOR);
    b.box(0.705f, 1.00f, 0.38f, 0.01f, 0.04f, 0.12f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i++) {
        b.box(0.67f * (float)i, 1.00f, 0.57f, 0.06f, 1.16f, 0.06f, DECOR_MAT_DARK_METAL);
    }
    b.box(-0.67f, 1.00f, 0.0f, 0.06f, 0.06f, 1.08f, DECOR_MAT_DARK_METAL);
    b.box(-0.67f, 2.10f, 0.0f, 0.06f, 0.06f, 1.08f, DECOR_MAT_DARK_METAL);
    b.box(-0.32f, 1.00f, 0.62f, 0.58f, 0.04f, 0.16f, DECOR_MAT_DARK_METAL);
    b.box(-0.32f, 1.04f, 0.57f, 0.20f, 0.012f, 0.14f, DECOR_MAT_RUBBER);
    // roof with a lit sign over the window
    b.box(0.0f, 2.16f, 0.0f, 1.50f, 0.10f, 1.40f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 2.26f, 0.60f, 1.00f, 0.22f, 0.06f, DECOR_MAT_SCREEN);
    // what is left inside: a stool, a desk shelf, the cash tray
    b.cylinder(-0.25f, 0.06f, -0.05f, 0.03f, 0.50f, DECOR_MAT_METAL, 6);
    b.cylinder(-0.25f, 0.56f, -0.05f, 0.17f, 0.06f, DECOR_MAT_RUBBER, 10);
    b.box(-0.02f, 0.96f, 0.39f, 1.24f, 0.04f, 0.30f, DECOR_MAT_WOOD);
    b.box(0.30f, 1.00f, 0.38f, 0.30f, 0.06f, 0.22f, DECOR_MAT_DARK_METAL);
}

// Luggage nobody came back for.
inline void buildSuitcasesModel(DecorBuilder& b) {
    // trolley case standing upright, handle pulled out
    b.box(-0.30f, 0.00f, -0.20f, 0.44f, 0.62f, 0.20f, DECOR_MAT_DRUM);
    b.box(-0.30f, 0.20f, -0.097f, 0.36f, 0.03f, 0.006f, DECOR_MAT_RUBBER);
    b.box(-0.30f, 0.44f, -0.097f, 0.36f, 0.03f, 0.006f, DECOR_MAT_RUBBER);
    for (int side = -1; side <= 1; side += 2) {
        b.box(-0.30f + 0.10f * (float)side, 0.62f, -0.27f, 0.014f, 0.34f, 0.014f, DECOR_MAT_METAL);
    }
    b.box(-0.30f, 0.96f, -0.27f, 0.24f, 0.03f, 0.03f, DECOR_MAT_RUBBER);
    // old hard case on its side with a holdall dropped on top
    DecorBuilder flat = b.sub(0.22f, 0.06f, 0.4f);
    flat.box(0.0f, 0.00f, 0.0f, 0.50f, 0.16f, 0.34f, DECOR_MAT_TRIM);
    for (int side = -1; side <= 1; side += 2) {
        flat.box(0.14f * (float)side, 0.00f, 0.0f, 0.03f, 0.164f, 0.348f, DECOR_MAT_CARDBOARD);
    }
    flat.box(0.0f, 0.06f, 0.18f, 0.12f, 0.03f, 0.02f, DECOR_MAT_RUBBER);
    DecorBuilder bag = flat.sub(0.02f, 0.0f, 0.3f);
    decorRod(bag, Vec3(-0.19f, 0.27f, 0.0f), Vec3(0.19f, 0.27f, 0.0f), 0.105f, DECOR_MAT_FABRIC, 8);
    bag.box(0.0f, 0.36f, 0.0f, 0.20f, 0.03f, 0.03f, DECOR_MAT_RUBBER);
}
