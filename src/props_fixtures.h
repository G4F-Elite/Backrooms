#pragma once
#include "decor_parts.h"

extern const float CS, WH;

// ============================================================================
// FIXTURE PROPS
// Things that are screwed to the building and turn up in most themes: clocks,
// speakers, sprinkler mains, radiators, payphones. Themes: see prop_catalog.h.
// Same rules as props_shared.h; wall props reach exactly to -hz (the wall).
// ============================================================================

// Round station-style clock high on the wall, stopped.
inline void buildWallClockModel(DecorBuilder& b) {
    const float y = 3.05f;
    decorRod(b, Vec3(0.0f, y, -0.04f), Vec3(0.0f, y, 0.02f), 0.24f, DECOR_MAT_DARK_METAL, 12);
    decorRod(b, Vec3(0.0f, y, 0.02f), Vec3(0.0f, y, 0.026f), 0.21f, DECOR_MAT_WHITE, 12);
    for (int i = 0; i < 4; i++) {
        float a = 1.5707963f * (float)i;
        b.box(sinf(a) * 0.17f, y + cosf(a) * 0.17f - 0.015f, 0.028f, 0.03f, 0.03f, 0.004f, DECOR_MAT_RUBBER);
    }
    decorBeam(b, Vec3(0.0f, y, 0.029f), Vec3(-0.09f, y + 0.07f, 0.029f), 0.004f, 0.025f, DECOR_MAT_RUBBER);
    decorBeam(b, Vec3(0.0f, y, 0.033f), Vec3(0.05f, y - 0.15f, 0.033f), 0.004f, 0.016f, DECOR_MAT_RUBBER);
    decorRod(b, Vec3(0.0f, y, 0.026f), Vec3(0.0f, y, 0.04f), 0.02f, DECOR_MAT_DARK_METAL, 6);
}

// Public-address speaker box on a wall bracket.
inline void buildPaSpeakerModel(DecorBuilder& b) {
    b.box(0.0f, 2.96f, -0.13f, 0.10f, 0.24f, 0.02f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 3.04f, -0.09f, 0.06f, 0.06f, 0.06f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 2.84f, 0.02f, 0.40f, 0.48f, 0.16f, DECOR_MAT_METAL);
    b.panel(0.0f, 2.88f, 0.103f, 0.32f, 0.40f, 0.006f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 2.80f, 0.02f, 0.44f, 0.04f, 0.20f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 3.32f, 0.02f, 0.44f, 0.04f, 0.20f, DECOR_MAT_DARK_METAL);
}

// CRT television on a braced wall shelf.
inline void buildWallTvModel(DecorBuilder& b) {
    const float y = 2.86f;
    b.box(0.0f, y - 0.12f, -0.27f, 0.54f, 0.12f, 0.02f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, y, -0.02f, 0.60f, 0.03f, 0.48f, DECOR_MAT_DARK_METAL);
    for (int side = -1; side <= 1; side += 2) {
        decorBeam(b, Vec3(0.22f * (float)side, y - 0.09f, -0.245f), Vec3(0.22f * (float)side, y, 0.10f), 0.03f, 0.03f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, y + 0.03f, 0.02f, 0.64f, 0.50f, 0.36f, DECOR_MAT_RUBBER);
    b.box(0.0f, y + 0.07f, -0.21f, 0.44f, 0.36f, 0.10f, DECOR_MAT_RUBBER);
    b.panel(-0.06f, y + 0.09f, 0.203f, 0.44f, 0.38f, 0.006f, DECOR_MAT_SCREEN);
    b.box(0.25f, y + 0.09f, 0.203f, 0.08f, 0.38f, 0.006f, DECOR_MAT_DARK_METAL);
    b.box(0.25f, y + 0.38f, 0.210f, 0.04f, 0.04f, 0.012f, DECOR_MAT_METAL);
    b.box(0.25f, y + 0.30f, 0.210f, 0.04f, 0.04f, 0.012f, DECOR_MAT_METAL);
}

// Dead strip light that has dropped out of the grid and hangs by its cables.
inline void buildDanglingLightModel(DecorBuilder& b) {
    const Vec3 hi(-0.58f, 3.92f, 0.0f);
    const Vec3 lo(0.58f, 3.30f, 0.0f);
    Vec3 axis = (lo - hi).norm();
    Vec3 down(axis.y, -axis.x, 0.0f);
    b.box(0.0f, WH - 0.03f, 0.0f, 1.24f, 0.03f, 0.30f, DECOR_MAT_DARK_METAL);
    decorBeam(b, hi, lo, 0.28f, 0.07f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        Vec3 off = down * 0.05f + Vec3(0.0f, 0.0f, 0.07f * (float)side);
        decorRod(b, hi + axis * 0.06f + off, lo - axis * 0.06f + off, 0.018f, DECOR_MAT_CEILTILE, 6);
    }
    decorRod(b, Vec3(-0.45f, 3.90f, 0.05f), Vec3(-0.40f, WH - 0.03f, 0.05f), 0.008f, DECOR_MAT_RUBBER, 4);
    decorRod(b, Vec3(0.45f, 3.42f, -0.05f), Vec3(0.30f, WH - 0.03f, -0.05f), 0.008f, DECOR_MAT_RUBBER, 4);
    decorRod(b, Vec3(0.10f, 3.62f, 0.0f), Vec3(0.16f, 3.20f, 0.04f), 0.006f, DECOR_MAT_ORANGE, 4);
}

// Fire main under the ceiling with two sprinkler heads.
inline void buildSprinklerPipeModel(DecorBuilder& b) {
    const float y = WH - 0.30f;
    decorRod(b, Vec3(-1.20f, y, 0.0f), Vec3(1.20f, y, 0.0f), 0.045f, DECOR_MAT_ORANGE, 8);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.95f * (float)side;
        b.box(x, y + 0.045f, 0.0f, 0.02f, 0.255f, 0.02f, DECOR_MAT_DARK_METAL);
        b.box(x, y - 0.06f, 0.0f, 0.05f, 0.12f, 0.12f, DECOR_MAT_DARK_METAL);
        float hx = 0.45f * (float)side;
        decorRod(b, Vec3(hx - 0.04f, y, 0.0f), Vec3(hx + 0.04f, y, 0.0f), 0.058f, DECOR_MAT_DARK_METAL, 8);
        b.cylinder(hx, y - 0.13f, 0.0f, 0.018f, 0.09f, DECOR_MAT_METAL, 6);
        b.cylinder(hx, y - 0.14f, 0.0f, 0.045f, 0.01f, DECOR_MAT_METAL, 8);
    }
}

// Cork board with whatever was pinned to it last.
inline void buildNoticeBoardModel(DecorBuilder& b) {
    const float f = 0.01f;
    b.box(0.0f, 1.00f, -0.01f, 1.30f, 0.90f, 0.04f, DECOR_MAT_TRIM);
    b.box(0.0f, 1.05f, f + 0.002f, 1.20f, 0.80f, 0.004f, DECOR_MAT_CARDBOARD);
    const float sheet[6][4] = {
        {-0.42f, 1.48f, 0.21f, 0.29f}, {-0.14f, 1.40f, 0.21f, 0.29f}, {0.20f, 1.52f, 0.30f, 0.21f},
        {-0.40f, 1.12f, 0.30f, 0.21f}, {0.40f, 1.14f, 0.21f, 0.29f}, {0.02f, 1.10f, 0.18f, 0.18f}
    };
    const int sheetMat[6] = {DECOR_MAT_WHITE, DECOR_MAT_WHITE, DECOR_MAT_CEILTILE, DECOR_MAT_WHITE, DECOR_MAT_ORANGE, DECOR_MAT_CAUTION};
    for (int i = 0; i < 6; i++) {
        b.panel(sheet[i][0], sheet[i][1], f + 0.006f, sheet[i][2], sheet[i][3], 0.003f, sheetMat[i]);
        b.box(sheet[i][0], sheet[i][1] + sheet[i][3] - 0.03f, f + 0.010f, 0.016f, 0.016f, 0.006f, DECOR_MAT_DRUM);
    }
}

// Wall payphone in a half booth, handset left off the hook.
inline void buildPayphoneModel(DecorBuilder& b) {
    b.box(0.0f, 0.90f, -0.17f, 0.56f, 1.10f, 0.02f, DECOR_MAT_METAL);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.29f * (float)side, 0.90f, 0.0f, 0.02f, 1.10f, 0.36f, DECOR_MAT_DRUM);
    }
    b.box(0.0f, 2.00f, 0.0f, 0.60f, 0.03f, 0.36f, DECOR_MAT_DRUM);
    b.box(0.0f, 0.90f, 0.0f, 0.56f, 0.03f, 0.32f, DECOR_MAT_METAL);
    b.box(0.0f, 0.93f, -0.04f, 0.30f, 0.05f, 0.22f, DECOR_MAT_CARDBOARD);
    // phone body: coin slot, display, keypad, return tray
    b.box(0.0f, 1.20f, -0.10f, 0.30f, 0.56f, 0.12f, DECOR_MAT_DARK_METAL);
    b.panel(0.04f, 1.60f, -0.037f, 0.16f, 0.06f, 0.006f, DECOR_MAT_SCREEN);
    b.box(0.04f, 1.40f, -0.035f, 0.14f, 0.16f, 0.010f, DECOR_MAT_METAL);
    b.box(0.04f, 1.24f, -0.03f, 0.12f, 0.06f, 0.02f, DECOR_MAT_RUBBER);
    b.box(0.11f, 1.68f, -0.037f, 0.03f, 0.05f, 0.006f, DECOR_MAT_METAL);
    // cradle and the dangling handset
    b.box(-0.11f, 1.56f, -0.03f, 0.05f, 0.06f, 0.03f, DECOR_MAT_METAL);
    decorRod(b, Vec3(-0.11f, 1.22f, -0.03f), Vec3(-0.18f, 0.72f, 0.06f), 0.008f, DECOR_MAT_METAL, 4);
    b.box(-0.18f, 0.50f, 0.06f, 0.05f, 0.22f, 0.04f, DECOR_MAT_RUBBER);
    b.box(-0.18f, 0.66f, 0.09f, 0.06f, 0.06f, 0.04f, DECOR_MAT_RUBBER);
    b.box(-0.18f, 0.50f, 0.09f, 0.06f, 0.06f, 0.04f, DECOR_MAT_RUBBER);
}

// Cast-iron column radiator on two feet.
inline void buildRadiatorModel(DecorBuilder& b) {
    for (int i = 0; i < 10; i++) {
        float x = -0.45f + 0.10f * (float)i;
        b.box(x, 0.12f, 0.0f, 0.07f, 0.62f, 0.16f, DECOR_MAT_WHITE);
    }
    b.box(0.0f, 0.16f, 0.0f, 0.94f, 0.06f, 0.08f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.64f, 0.0f, 0.94f, 0.06f, 0.08f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.40f * (float)side, 0.00f, 0.0f, 0.05f, 0.12f, 0.14f, DECOR_MAT_WHITE);
        b.box(0.40f * (float)side, 0.40f, -0.085f, 0.04f, 0.04f, 0.01f, DECOR_MAT_DARK_METAL);
    }
    // feed pipe with its valve, return pipe on the other end
    b.cylinder(0.53f, 0.00f, 0.0f, 0.02f, 0.64f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(0.485f, 0.67f, 0.0f), Vec3(0.55f, 0.67f, 0.0f), 0.02f, DECOR_MAT_METAL, 6);
    b.cylinder(0.53f, 0.69f, 0.0f, 0.035f, 0.05f, DECOR_MAT_ORANGE, 8);
    b.cylinder(-0.53f, 0.00f, 0.0f, 0.02f, 0.21f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(-0.55f, 0.19f, 0.0f), Vec3(-0.485f, 0.19f, 0.0f), 0.02f, DECOR_MAT_METAL, 6);
}

// Steel drinking fountain: push bar on the front, drain grille below.
inline void buildDrinkingFountainModel(DecorBuilder& b) {
    const float f = 0.17f;
    b.box(0.0f, 0.00f, -0.01f, 0.40f, 0.06f, 0.32f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.06f, 0.0f, 0.44f, 0.82f, 0.34f, DECOR_MAT_METAL);
    b.box(0.0f, 0.88f, 0.0f, 0.48f, 0.04f, 0.38f, DECOR_MAT_METAL);
    b.box(0.0f, 0.92f, 0.01f, 0.36f, 0.004f, 0.24f, DECOR_MAT_DARK_METAL);
    b.cylinder(0.0f, 0.924f, 0.03f, 0.03f, 0.004f, DECOR_MAT_RUBBER, 8);
    b.box(0.14f, 0.92f, -0.10f, 0.05f, 0.06f, 0.05f, DECOR_MAT_METAL);
    decorRod(b, Vec3(0.14f, 0.96f, -0.10f), Vec3(0.08f, 1.00f, -0.04f), 0.012f, DECOR_MAT_METAL, 6);
    b.box(0.0f, 0.70f, f + 0.01f, 0.30f, 0.07f, 0.02f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 0.14f, f + 0.003f, 0.34f, 0.34f, 0.006f, DECOR_MAT_DARK_METAL);
}
