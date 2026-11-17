#pragma once
#include "decor_parts.h"

extern const float CS, WH;

// ============================================================================
// PLAYROOM PROPS
// Party rooms, soft play corners and arcade nooks; everything set up for a
// celebration nobody came to. Themes: see prop_catalog.h.
// ============================================================================

// Trestle table laid for a birthday: cake, paper hats, cups.
inline void buildPartyTableModel(DecorBuilder& b) {
    const float top = 0.73f;
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.72f * (float)side, 0.00f, 0.0f, 0.04f, 0.70f, 0.60f, DECOR_MAT_METAL);
        b.box(0.0f, 0.45f, 0.385f * (float)side, 1.82f, 0.25f, 0.01f, DECOR_MAT_WHITE);
        b.box(0.905f * (float)side, 0.45f, 0.0f, 0.01f, 0.25f, 0.76f, DECOR_MAT_WHITE);
    }
    b.box(0.0f, 0.70f, 0.0f, 1.82f, 0.03f, 0.78f, DECOR_MAT_WHITE);
    // cake with candles on a tray
    b.cylinder(0.0f, top, 0.0f, 0.17f, 0.012f, DECOR_MAT_METAL, 10);
    b.cylinder(0.0f, top + 0.012f, 0.0f, 0.13f, 0.09f, DECOR_MAT_CEILTILE, 10);
    b.cylinder(0.0f, top + 0.102f, 0.0f, 0.13f, 0.012f, DECOR_MAT_ORANGE, 10);
    for (int i = 0; i < 3; i++) {
        float a = 2.0944f * (float)i;
        b.cylinder(sinf(a) * 0.08f, top + 0.114f, cosf(a) * 0.08f, 0.008f, 0.05f, DECOR_MAT_DRUM, 4);
    }
    // places: plate, cup, hat
    const int hatMat[4] = {DECOR_MAT_ORANGE, DECOR_MAT_DRUM, DECOR_MAT_CAUTION, DECOR_MAT_DRUM};
    for (int i = 0; i < 4; i++) {
        float x = -0.66f + 0.44f * (float)i;
        float z = (i & 1) ? -0.22f : 0.22f;
        b.cylinder(x, top, z, 0.09f, 0.006f, DECOR_MAT_CARDBOARD, 6);
        b.cylinder(x + (i < 2 ? -0.13f : 0.13f), top, z * 0.6f, 0.03f, 0.08f, hatMat[(i + 1) % 4], 6);
        if (i != 2) decorCone(b, x, top + 0.006f, z, 0.06f, 0.0f, 0.17f, hatMat[i], 8);
    }
    DecorBuilder fallen = b.sub(0.52f, 0.22f, 0.9f);
    decorRod(fallen, Vec3(0.0f, top + 0.03f, -0.04f), Vec3(0.0f, top + 0.03f, 0.04f), 0.03f, DECOR_MAT_ORANGE, 6);
}

// Balloons on strings tied to a weight; one has already gone soft.
inline void buildBalloonBunchModel(DecorBuilder& b) {
    const float at[4][3] = {
        {-0.13f, 1.78f, 0.05f}, {0.12f, 1.98f, -0.07f}, {0.02f, 2.24f, 0.11f}, {-0.03f, 1.52f, -0.13f}
    };
    const int mat[4] = {DECOR_MAT_ORANGE, DECOR_MAT_DRUM, DECOR_MAT_WHITE, DECOR_MAT_CAUTION};
    b.box(0.0f, 0.00f, 0.0f, 0.16f, 0.07f, 0.16f, DECOR_MAT_DARK_METAL);
    b.cylinder(0.0f, 0.07f, 0.0f, 0.02f, 0.02f, DECOR_MAT_METAL, 6);
    for (int i = 0; i < 4; i++) {
        float r = (i == 3) ? 0.12f : 0.16f;
        Vec3 knot(at[i][0], at[i][1] - r - 0.03f, at[i][2]);
        decorRod(b, Vec3(0.0f, 0.09f, 0.0f), knot, 0.004f, DECOR_MAT_WHITE, 4);
        decorCone(b, knot.x, knot.y, knot.z, 0.012f, 0.03f, 0.04f, mat[i], 6);
        decorBall(b, at[i][0], at[i][1], at[i][2], r, mat[i]);
    }
    b.cylinder(0.19f, 0.00f, 0.16f, 0.07f, 0.012f, DECOR_MAT_ORANGE, 8);
}

// Padded pit, still full of balls.
inline void buildBallPitModel(DecorBuilder& b) {
    const int ballMat[4] = {DECOR_MAT_ORANGE, DECOR_MAT_DRUM, DECOR_MAT_WHITE, DECOR_MAT_DRUM};
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.0f, 0.00f, 0.71f * (float)side, 1.60f, 0.46f, 0.18f, DECOR_MAT_DRUM);
        b.box(0.71f * (float)side, 0.00f, 0.0f, 0.18f, 0.46f, 1.24f, DECOR_MAT_ORANGE);
    }
    b.box(0.0f, 0.00f, 0.0f, 1.24f, 0.20f, 1.24f, DECOR_MAT_RUBBER);
    for (int ix = 0; ix < 4; ix++) {
        for (int iz = 0; iz < 4; iz++) {
            int roll = ix * 7 + iz * 11 + ix * iz * 3;
            float x = -0.45f + 0.30f * (float)ix + 0.02f * (float)((roll % 3) - 1);
            float z = -0.45f + 0.30f * (float)iz + 0.02f * (float)(((roll / 3) % 3) - 1);
            float y = 0.27f + 0.03f * (float)(roll % 4);
            if (roll % 4 == 3) continue;
            decorBall(b, x, y, z, 0.15f, ballMat[roll % 4]);
        }
    }
    // two that got out
    decorBall(b, 0.30f, 0.57f, 0.68f, 0.11f, DECOR_MAT_ORANGE);
    decorBall(b, -0.68f, 0.57f, -0.20f, 0.11f, DECOR_MAT_WHITE);
}

// Upright arcade machine, attract screen still running.
inline void buildArcadeCabinetModel(DecorBuilder& b) {
    b.box(0.0f, 0.00f, -0.05f, 0.64f, 0.85f, 0.60f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.85f, 0.0f, 0.64f, 0.08f, 0.70f, DECOR_MAT_RUBBER);
    b.box(0.0f, 0.93f, -0.10f, 0.64f, 0.57f, 0.50f, DECOR_MAT_DARK_METAL);
    b.panel(0.0f, 1.00f, 0.153f, 0.50f, 0.42f, 0.006f, DECOR_MAT_SCREEN);
    b.box(0.0f, 1.50f, -0.085f, 0.64f, 0.25f, 0.53f, DECOR_MAT_ORANGE);
    b.panel(0.0f, 1.54f, 0.183f, 0.52f, 0.15f, 0.006f, DECOR_MAT_CAUTION);
    // stick, buttons, coin door
    b.cylinder(-0.14f, 0.93f, 0.25f, 0.012f, 0.07f, DECOR_MAT_METAL, 6);
    decorBall(b, -0.14f, 1.02f, 0.25f, 0.03f, DECOR_MAT_ORANGE, 6);
    for (int i = 0; i < 3; i++) {
        b.cylinder(0.04f + 0.08f * (float)i, 0.93f, 0.25f, 0.022f, 0.012f, (i == 1) ? DECOR_MAT_WHITE : DECOR_MAT_DRUM, 6);
    }
    b.box(0.0f, 0.30f, 0.253f, 0.26f, 0.30f, 0.006f, DECOR_MAT_METAL);
    b.box(-0.05f, 0.44f, 0.258f, 0.03f, 0.06f, 0.006f, DECOR_MAT_ORANGE);
    b.box(0.05f, 0.44f, 0.258f, 0.03f, 0.06f, 0.006f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.00f, 0.253f, 0.64f, 0.12f, 0.006f, DECOR_MAT_DRUM);
}

// Indoor slide: climb the rungs at the back, come down at the front (+Z).
inline void buildPlaySlideModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.31f * (float)side;
        b.box(x, 0.00f, -0.95f, 0.06f, 1.50f, 0.06f, DECOR_MAT_DRUM);
        b.box(x, 0.00f, -0.45f, 0.06f, 1.50f, 0.06f, DECOR_MAT_DRUM);
        b.box(x, 1.00f, -0.70f, 0.03f, 0.40f, 0.44f, DECOR_MAT_WHITE);
        decorBeam(b, Vec3(0.295f * (float)side, 1.03f, -0.42f), Vec3(0.295f * (float)side, 0.15f, 0.90f), 0.03f, 0.14f, DECOR_MAT_DRUM);
    }
    b.box(0.0f, 0.95f, -0.70f, 0.56f, 0.05f, 0.56f, DECOR_MAT_ORANGE);
    for (int i = 1; i <= 3; i++) {
        b.box(0.0f, 0.24f * (float)i - 0.02f, -0.95f, 0.56f, 0.04f, 0.06f, DECOR_MAT_METAL);
    }
    decorBeam(b, Vec3(0.0f, 0.975f, -0.42f), Vec3(0.0f, 0.095f, 0.90f), 0.56f, 0.04f, DECOR_MAT_ORANGE);
    b.box(0.0f, 0.00f, 0.90f, 0.62f, 0.08f, 0.16f, DECOR_MAT_DRUM);
}

// Foam play shapes in a heap.
inline void buildSoftBlocksModel(DecorBuilder& b) {
    b.box(-0.24f, 0.00f, -0.16f, 0.40f, 0.40f, 0.40f, DECOR_MAT_DRUM);
    DecorBuilder top = b.sub(-0.20f, -0.14f, 0.5f);
    top.box(0.0f, 0.40f, 0.0f, 0.28f, 0.28f, 0.28f, DECOR_MAT_ORANGE);
    b.taper(0.26f, 0.00f, -0.20f, 0.40f, 0.40f, 0.40f, 0.02f, 0.30f, DECOR_MAT_WHITE);
    decorRod(b, Vec3(0.08f, 0.14f, 0.24f), Vec3(0.50f, 0.14f, 0.24f), 0.14f, DECOR_MAT_CAUTION, 10);
    DecorBuilder flat = b.sub(-0.28f, 0.26f, -0.3f);
    flat.box(0.0f, 0.00f, 0.0f, 0.44f, 0.14f, 0.22f, DECOR_MAT_ORANGE);
}

// Wrapped box with a ribbon tied both ways and a bow on top.
inline void buildGiftBoxModel(DecorBuilder& b, float y, float sx, float sy, float sz, int mat, int ribbon) {
    b.box(0.0f, y, 0.0f, sx, sy, sz, mat);
    b.box(0.0f, y, 0.0f, sx * 0.16f, sy + 0.004f, sz + 0.008f, ribbon);
    b.box(0.0f, y, 0.0f, sx + 0.008f, sy + 0.004f, sz * 0.16f, ribbon);
    b.box(0.0f, y + sy + 0.004f, 0.0f, sx * 0.30f, 0.03f, sz * 0.22f, ribbon);
}

inline void buildGiftPileModel(DecorBuilder& b) {
    DecorBuilder big = b.sub(-0.14f, -0.20f, 0.15f);
    buildGiftBoxModel(big, 0.00f, 0.50f, 0.34f, 0.44f, DECOR_MAT_DRUM, DECOR_MAT_WHITE);
    DecorBuilder top = big.sub(0.02f, 0.02f, 0.6f);
    buildGiftBoxModel(top, 0.344f, 0.28f, 0.20f, 0.26f, DECOR_MAT_ORANGE, DECOR_MAT_DRUM);
    DecorBuilder side = b.sub(0.32f, -0.14f, -0.35f);
    buildGiftBoxModel(side, 0.00f, 0.26f, 0.26f, 0.26f, DECOR_MAT_WHITE, DECOR_MAT_ORANGE);
    DecorBuilder small = b.sub(0.06f, 0.28f, 0.8f);
    buildGiftBoxModel(small, 0.00f, 0.30f, 0.10f, 0.20f, DECOR_MAT_CAUTION, DECOR_MAT_RUBBER);
}

// String of pennants sagging between two ceiling cords.
inline void buildBuntingModel(DecorBuilder& b) {
    const int flagMat[4] = {DECOR_MAT_ORANGE, DECOR_MAT_DRUM, DECOR_MAT_WHITE, DECOR_MAT_CAUTION};
    const float yEnd = 3.50f;
    const float sag = 0.22f;
    for (int side = -1; side <= 1; side += 2) {
        b.box(1.05f * (float)side, yEnd, 0.0f, 0.012f, WH - yEnd, 0.012f, DECOR_MAT_WHITE);
        decorRod(b, Vec3(1.05f * (float)side, yEnd, 0.0f), Vec3(0.0f, yEnd - sag, 0.0f), 0.005f, DECOR_MAT_WHITE, 4);
    }
    for (int i = 0; i < 9; i++) {
        float x = -0.88f + 0.22f * (float)i;
        float y = yEnd - sag * (1.0f - fabsf(x) / 1.05f) - 0.004f;
        b.taper(x, y - 0.24f, 0.0f, 0.0f, 0.006f, 0.18f, 0.006f, 0.24f, flagMat[i % 4]);
    }
}
