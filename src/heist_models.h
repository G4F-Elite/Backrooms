#pragma once
// Heist set pieces (the drill is in rig_models.h): the rival kiosk, the portal (alive and dead), the delivery
// pallet, cracked wall faces, loot and junk piles. Local metres, +Z is the front.
// These models carry the story, so state is shown on the object itself: gauge lamps on the
// drill, boxes stacking up on the pallet, a burnt arch where the way home used to be.

#include "decor_builder.h"
#include "decor_parts.h"
#include "heist_rules.h"

inline constexpr int DRILL_GAUGE_LAMPS = 5;
inline constexpr float KIOSK_HALF_X = 1.25f;
inline constexpr float KIOSK_HALF_Z = 0.80f;

// The rivals got here first: a market stall with a winch to the surface.
inline void buildKioskModel(DecorBuilder& b, bool canExtract) {
    b.box(0.0f, 0.00f, 0.28f, 1.90f, 0.92f, 0.56f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.92f, 0.28f, 2.04f, 0.05f, 0.66f, DECOR_MAT_TRIM);
    b.panel(0.0f, 0.22f, 0.565f, 1.50f, 0.50f, 0.012f, DECOR_MAT_CARDBOARD);
    // awning on two posts
    b.box(-0.96f, 0.0f, 0.50f, 0.07f, 2.30f, 0.07f, DECOR_MAT_METAL);
    b.box(0.96f, 0.0f, 0.50f, 0.07f, 2.30f, 0.07f, DECOR_MAT_METAL);
    decorBeam(b, Vec3(0.0f, 2.46f, -0.50f), Vec3(0.0f, 2.20f, 0.74f), 2.10f, 0.04f, DECOR_MAT_CAUTION);
    // goods on the counter: battery, med spray, plush
    b.cylinder(-0.62f, 0.97f, 0.30f, 0.06f, 0.20f, DECOR_MAT_DRUM, 8);
    b.box(-0.20f, 0.97f, 0.30f, 0.12f, 0.24f, 0.12f, DECOR_MAT_WHITE);
    b.box(-0.20f, 1.21f, 0.30f, 0.08f, 0.05f, 0.08f, DECOR_MAT_ORANGE);
    b.box(0.22f, 0.97f, 0.30f, 0.20f, 0.18f, 0.16f, DECOR_MAT_FABRIC);
    b.box(0.22f, 1.15f, 0.30f, 0.15f, 0.14f, 0.14f, DECOR_MAT_FABRIC);
    b.panel(0.66f, 0.97f, 0.30f, 0.26f, 0.20f, 0.05f, DECOR_MAT_SCREEN);
    // the shopkeeper: hooded, headlamp on, never stands up
    b.box(0.0f, 0.0f, -0.40f, 0.44f, 0.50f, 0.40f, DECOR_MAT_CARDBOARD);
    b.taper(0.0f, 0.50f, -0.40f, 0.50f, 0.32f, 0.40f, 0.26f, 0.62f, DECOR_MAT_FABRIC);
    decorBall(b, 0.0f, 1.30f, -0.38f, 0.16f, DECOR_MAT_RUBBER, 8);
    b.panel(0.0f, 1.30f, -0.225f, 0.10f, 0.07f, 0.02f, DECOR_MAT_SCREEN);
    decorBeam(b, Vec3(-0.24f, 1.02f, -0.36f), Vec3(-0.40f, 0.98f, 0.16f), 0.10f, 0.09f, DECOR_MAT_FABRIC);
    decorBeam(b, Vec3(0.24f, 1.02f, -0.36f), Vec3(0.40f, 0.98f, 0.16f), 0.10f, 0.09f, DECOR_MAT_FABRIC);
    // winch: the rope goes all the way up
    b.box(1.02f, 0.0f, -0.42f, 0.36f, 0.30f, 0.36f, DECOR_MAT_DARK_METAL);
    decorRod(b, Vec3(0.84f, 0.42f, -0.42f), Vec3(1.20f, 0.42f, -0.42f), 0.13f, DECOR_MAT_ORANGE, 8);
    b.cylinder(1.02f, 0.50f, -0.42f, 0.022f, WH - 0.50f, DECOR_MAT_CARDBOARD, 5);
    b.box(1.02f, WH - 0.05f, -0.42f, 0.90f, 0.04f, 0.90f, DECOR_MAT_RUBBER);
    b.panel(1.02f, 1.50f, -0.39f, 0.30f, 0.30f, 0.03f, canExtract ? DECOR_MAT_SCREEN : DECOR_MAT_CAUTION);
    // lantern
    b.cylinder(-0.96f, 2.30f, 0.50f, 0.09f, 0.16f, DECOR_MAT_WHITE, 6);
}

// Rig bolted around the doorway between the garage (-Z) and the Backrooms (+Z).
inline void buildPortalRigModel(DecorBuilder& b) {
    const float postX = 1.52f, top = 3.10f;
    for (int side = -1; side <= 1; side += 2) {
        float z = (float)side * 0.42f;
        b.box(-postX, 0.0f, z, 0.16f, top, 0.16f, DECOR_MAT_DARK_METAL);
        b.box(postX, 0.0f, z, 0.16f, top, 0.16f, DECOR_MAT_DARK_METAL);
        b.box(0.0f, top, z, postX * 2.0f + 0.16f, 0.16f, 0.16f, DECOR_MAT_DARK_METAL);
        b.panel(-postX + 0.10f, 0.20f, z, 0.03f, top - 0.40f, 0.10f, DECOR_MAT_SCREEN);
        b.panel(postX - 0.10f, 0.20f, z, 0.03f, top - 0.40f, 0.10f, DECOR_MAT_SCREEN);
        b.decal(0.0f, 0.012f, (float)side * 1.30f, 2.60f, 0.50f, DECOR_MAT_CAUTION);
    }
    // generator and cables on the home side
    b.box(-2.00f, 0.0f, -1.10f, 0.60f, 0.70f, 0.44f, DECOR_MAT_ORANGE);
    b.panel(-2.00f, 0.36f, -0.875f, 0.30f, 0.20f, 0.012f, DECOR_MAT_SCREEN);
    decorRod(b, Vec3(-1.80f, 0.60f, -0.98f), Vec3(-postX, 1.60f, -0.42f), 0.03f, DECOR_MAT_RUBBER, 5);
    decorRod(b, Vec3(-1.80f, 0.20f, -0.98f), Vec3(postX, 0.10f, -0.42f), 0.03f, DECOR_MAT_RUBBER, 5);
}

// What is left on the Backrooms side. Origin = centre of the (now solid) portal cell.
inline void buildPortalWreckModel(DecorBuilder& b) {
    const float face = CS * 0.5f;
    b.panel(0.0f, 0.0f, face + 0.02f, 2.50f, 2.86f, 0.04f, DECOR_MAT_RUBBER);
    b.box(-1.34f, 0.0f, face + 0.07f, 0.16f, 2.95f, 0.14f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 2.90f, face + 0.07f, 2.84f, 0.16f, 0.14f, DECOR_MAT_DARK_METAL);
    decorBeam(b, Vec3(1.30f, 0.0f, face + 0.10f), Vec3(0.20f, 1.90f, face + 0.90f), 0.16f, 0.14f, DECOR_MAT_DARK_METAL);
    decorBeam(b, Vec3(-0.90f, 0.08f, face + 0.70f), Vec3(1.20f, 0.08f, face + 1.60f), 0.16f, 0.14f, DECOR_MAT_DARK_METAL);
    b.decal(0.0f, 0.014f, face + 1.20f, 3.40f, 2.40f, DECOR_MAT_PUDDLE);
    b.box(-0.60f, 0.0f, face + 0.50f, 0.50f, 0.26f, 0.40f, DECOR_MAT_CEILTILE);
    b.box(0.50f, 0.0f, face + 0.36f, 0.36f, 0.40f, 0.30f, DECOR_MAT_PLASTER);
    b.box(0.10f, 0.0f, face + 1.10f, 0.30f, 0.16f, 0.34f, DECOR_MAT_CEILTILE);
    decorRod(b, Vec3(-1.30f, 2.60f, face + 0.14f), Vec3(-0.80f, 1.50f, face + 0.50f), 0.03f, DECOR_MAT_ORANGE, 5);
    b.panel(-0.80f, 1.44f, face + 0.50f, 0.06f, 0.06f, 0.06f, DECOR_MAT_SCREEN);
}

// Delivery pallet: every delivery stays on it, empty outlines show what is still owed.
inline void buildPalletPadModel(DecorBuilder& b, int delivered, int wanted) {
    b.decal(0.0f, 0.012f, 0.0f, 2.30f, 2.30f, DECOR_MAT_CAUTION);
    b.decal(0.0f, 0.016f, 0.0f, 1.90f, 1.90f, DECOR_MAT_RUBBER);
    decorPallet(b, 0.0f, 0.02f, 0.0f);
    for (int i = 0; i < wanted; i++) {
        float x = -0.36f + (float)i * 0.36f;
        if (i < delivered) b.box(x, 0.17f, 0.0f, 0.30f, 0.28f, 0.34f, DECOR_MAT_CARDBOARD);
        else b.decal(x, 0.176f, 0.0f, 0.28f, 0.32f, DECOR_MAT_WHITE);
    }
}

// The business plan, as drawn on the garage whiteboard: person -> door -> box -> coins.
inline void buildPitchBoardModel(DecorBuilder& b) {
    const float z = 0.035f, ink = 0.012f;
    b.panel(0.0f, 0.95f, 0.0f, 2.60f, 1.30f, 0.05f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.91f, 0.02f, 2.66f, 0.05f, 0.09f, DECOR_MAT_METAL);
    auto dot = [&](float x, float y, float w, float h, int mat) { b.box(x, y, z, w, h, ink, mat); };
    // person
    dot(-1.00f, 1.86f, 0.14f, 0.14f, DECOR_MAT_RUBBER);
    dot(-1.00f, 1.50f, 0.06f, 0.36f, DECOR_MAT_RUBBER);
    dot(-1.00f, 1.70f, 0.34f, 0.05f, DECOR_MAT_RUBBER);
    dot(-1.07f, 1.26f, 0.05f, 0.26f, DECOR_MAT_RUBBER);
    dot(-0.93f, 1.26f, 0.05f, 0.26f, DECOR_MAT_RUBBER);
    // arrows
    for (int i = 0; i < 3; i++) {
        float x = -0.66f + (float)i * 0.66f;
        dot(x, 1.58f, 0.22f, 0.04f, DECOR_MAT_RUBBER);
        dot(x + 0.09f, 1.62f, 0.05f, 0.05f, DECOR_MAT_RUBBER);
        dot(x + 0.09f, 1.53f, 0.05f, 0.05f, DECOR_MAT_RUBBER);
    }
    // door into the yellow
    dot(-0.33f, 1.26f, 0.42f, 0.70f, DECOR_MAT_PLASTER);
    dot(-0.52f, 1.26f, 0.05f, 0.74f, DECOR_MAT_ORANGE);
    dot(-0.14f, 1.26f, 0.05f, 0.74f, DECOR_MAT_ORANGE);
    dot(-0.33f, 1.96f, 0.43f, 0.05f, DECOR_MAT_ORANGE);
    // box
    dot(0.33f, 1.36f, 0.40f, 0.36f, DECOR_MAT_CARDBOARD);
    dot(0.33f, 1.53f, 0.42f, 0.04f, DECOR_MAT_RUBBER);
    // coins
    for (int i = 0; i < 4; i++) dot(0.96f, 1.30f + (float)i * 0.11f, 0.30f, 0.08f, DECOR_MAT_CAUTION);
    dot(1.16f, 1.30f, 0.22f, 0.08f, DECOR_MAT_CAUTION);
    // marker tray
    b.box(0.60f, 0.96f, 0.07f, 0.16f, 0.03f, 0.03f, DECOR_MAT_ORANGE);
}

// Cracked wall face. Origin = centre of the face on the floor, +Z points out of the wall.
inline void buildCrackFaceModel(DecorBuilder& b, unsigned seed, bool glint) {
    const float z = 0.02f;
    Vec3 p(-0.30f + (float)(seed % 7u) * 0.08f, 0.0f, z);
    for (int i = 0; i < 7; i++) {
        seed = seed * 1664525u + 1013904223u;
        float dx = ((float)((seed >> 8) % 100u) / 100.0f - 0.5f) * 0.80f;
        Vec3 q(p.x + dx, p.y + 0.36f + (float)((seed >> 16) % 30u) * 0.01f, z);
        decorBeam(b, p, q, 0.06f, 0.03f, DECOR_MAT_RUBBER);
        if (i == 2 || i == 4) {
            Vec3 tip(q.x + (i == 2 ? 0.52f : -0.56f), q.y + 0.22f, z);
            decorBeam(b, q, tip, 0.045f, 0.03f, DECOR_MAT_RUBBER);
        }
        p = q;
    }
    b.box(-0.40f, 0.0f, 0.16f, 0.34f, 0.10f, 0.24f, DECOR_MAT_CEILTILE);
    b.box(0.26f, 0.0f, 0.22f, 0.22f, 0.07f, 0.20f, DECOR_MAT_PLASTER);
    b.box(0.02f, 0.0f, 0.30f, 0.14f, 0.05f, 0.12f, DECOR_MAT_CEILTILE);
    if (glint) b.box(0.10f, 0.62f, 0.0f, 0.22f, 0.22f, 0.07f, DECOR_MAT_METAL);
}

inline void buildLootMesh(DecorBuilder& b, int loot) {
    if (loot == LOOT_LAMP_TUBE) {
        decorRod(b, Vec3(-0.46f, 0.06f, 0.0f), Vec3(0.46f, 0.06f, 0.0f), 0.045f, DECOR_MAT_WHITE, 8);
        b.box(-0.48f, 0.01f, 0.0f, 0.06f, 0.10f, 0.10f, DECOR_MAT_METAL);
        b.box(0.48f, 0.01f, 0.0f, 0.06f, 0.10f, 0.10f, DECOR_MAT_METAL);
    } else if (loot == LOOT_WALLPAPER) {
        b.cylinder(0.0f, 0.0f, 0.0f, 0.10f, 0.52f, DECOR_MAT_PLASTER, 10);
        b.cylinder(0.0f, 0.0f, 0.0f, 0.04f, 0.56f, DECOR_MAT_CARDBOARD, 6);
        b.box(0.0f, 0.20f, 0.10f, 0.20f, 0.06f, 0.012f, DECOR_MAT_WHITE);
    } else if (loot == LOOT_CLOCK) {
        b.box(0.0f, 0.0f, 0.0f, 0.40f, 0.40f, 0.07f, DECOR_MAT_TRIM);
        b.panel(0.0f, 0.03f, 0.036f, 0.34f, 0.34f, 0.01f, DECOR_MAT_WHITE);
        b.box(0.0f, 0.19f, 0.045f, 0.03f, 0.14f, 0.01f, DECOR_MAT_RUBBER);
        b.box(0.05f, 0.19f, 0.045f, 0.12f, 0.03f, 0.01f, DECOR_MAT_RUBBER);
    } else if (loot == LOOT_PHONE) {
        b.taper(0.0f, 0.0f, 0.0f, 0.34f, 0.26f, 0.26f, 0.18f, 0.11f, DECOR_MAT_ORANGE);
        b.cylinder(0.0f, 0.11f, 0.02f, 0.07f, 0.02f, DECOR_MAT_WHITE, 8);
        decorRod(b, Vec3(-0.17f, 0.17f, -0.03f), Vec3(0.17f, 0.17f, -0.03f), 0.035f, DECOR_MAT_ORANGE, 6);
        b.box(-0.17f, 0.11f, -0.03f, 0.09f, 0.08f, 0.09f, DECOR_MAT_ORANGE);
        b.box(0.17f, 0.11f, -0.03f, 0.09f, 0.08f, 0.09f, DECOR_MAT_ORANGE);
    } else if (loot == LOOT_MONITOR) {
        b.box(0.0f, 0.06f, 0.0f, 0.48f, 0.40f, 0.42f, DECOR_MAT_WHITE);
        b.panel(0.0f, 0.12f, 0.212f, 0.38f, 0.28f, 0.012f, DECOR_MAT_SCREEN);
        b.box(0.0f, 0.0f, 0.0f, 0.30f, 0.06f, 0.28f, DECOR_MAT_DARK_METAL);
    } else if (loot == LOOT_ALMOND_WATER) {
        b.cylinder(0.0f, 0.0f, 0.0f, 0.15f, 0.36f, DECOR_MAT_WHITE, 10);
        decorCone(b, 0.0f, 0.36f, 0.0f, 0.15f, 0.06f, 0.10f, DECOR_MAT_WHITE, 10);
        b.cylinder(0.0f, 0.46f, 0.0f, 0.06f, 0.05f, DECOR_MAT_DRUM, 8);
        b.box(0.0f, 0.12f, 0.15f, 0.20f, 0.14f, 0.012f, DECOR_MAT_CAUTION);
    } else if (loot == LOOT_SAFE) {
        b.box(0.0f, 0.0f, 0.0f, 0.46f, 0.46f, 0.42f, DECOR_MAT_DARK_METAL);
        b.box(0.0f, 0.04f, 0.212f, 0.38f, 0.38f, 0.012f, DECOR_MAT_METAL);
        decorRod(b, Vec3(0.0f, 0.23f, 0.21f), Vec3(0.0f, 0.23f, 0.26f), 0.06f, DECOR_MAT_CAUTION, 8);
        b.box(0.14f, 0.20f, 0.225f, 0.03f, 0.10f, 0.03f, DECOR_MAT_ORANGE);
    } else if (loot == LOOT_CORE) {
        decorBall(b, 0.0f, 0.26f, 0.0f, 0.17f, DECOR_MAT_SCREEN, 8);
        decorBand(b, 0.0f, 0.23f, 0.0f, 0.24f, 0.24f, 0.06f, DECOR_MAT_CAUTION, 8, true, true);
        for (int i = 0; i < 4; i++) {
            float a = 0.7854f + (float)i * 1.5708f;
            Vec3 foot(sinf(a) * 0.24f, 0.0f, cosf(a) * 0.24f);
            decorRod(b, foot, Vec3(foot.x * 0.5f, 0.52f, foot.z * 0.5f), 0.02f, DECOR_MAT_ORANGE, 5);
        }
        b.box(0.0f, 0.50f, 0.0f, 0.26f, 0.04f, 0.26f, DECOR_MAT_DARK_METAL);
    }
}

// Junk pile: bags, boxes, fallen tiles. Something may be under it.
inline void buildStashMesh(DecorBuilder& b) {
    decorBall(b, -0.20f, 0.20f, 0.05f, 0.27f, DECOR_MAT_RUBBER, 7);
    decorBall(b, 0.24f, 0.16f, -0.14f, 0.22f, DECOR_MAT_RUBBER, 7);
    b.box(0.16f, 0.0f, 0.26f, 0.40f, 0.26f, 0.30f, DECOR_MAT_CARDBOARD);
    b.box(-0.30f, 0.0f, -0.30f, 0.30f, 0.18f, 0.26f, DECOR_MAT_CARDBOARD);
    decorBeam(b, Vec3(-0.50f, 0.02f, 0.30f), Vec3(0.10f, 0.42f, 0.10f), 0.44f, 0.03f, DECOR_MAT_CEILTILE);
    decorBeam(b, Vec3(0.50f, 0.02f, -0.36f), Vec3(0.06f, 0.36f, -0.10f), 0.36f, 0.03f, DECOR_MAT_CEILTILE);
    b.box(0.0f, 0.0f, 0.0f, 0.90f, 0.03f, 0.80f, DECOR_MAT_FABRIC);
}
