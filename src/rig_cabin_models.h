#pragma once
// Rig meshes, the inside: break room furniture and everything a hand can use. Rig space, see
// rig_rules.h. The game has no words on its machines, so every station says what it is for
// with a shape: a striped arrow into the hopper mouth, a drill bit over the gauge, an arrow
// plate over the lever, a battery drawn on the charger, a bolt on the fuse box.
// State is shown on the machine itself: gauge lamps, the latch on the lever, a battery
// waiting in the charger, the fuse box hanging open.

#include "decor_builder.h"
#include "decor_parts.h"
#include "rig_rules.h"

struct RigLook {
    int lamps = 0;        // gauge lamps lit (out of gaugeLamps)
    int gaugeLamps = 5;
    bool ready = false;   // quota met, lever armed
    bool pulled = false;  // lever down, the drill is going through the floor
    int fault = RIG_OK;
    bool power = true;    // engine or reserve battery
    bool charged = false; // a battery is waiting in the charger
};

inline int rigLookSignature(const RigLook& l) {
    int sig = l.lamps;
    sig = sig * 2 + (l.ready ? 1 : 0);
    sig = sig * 2 + (l.pulled ? 1 : 0);
    sig = sig * RIG_FAULT_COUNT + l.fault;
    sig = sig * 2 + (l.power ? 1 : 0);
    sig = sig * 2 + (l.charged ? 1 : 0);
    return sig;
}

// The stations on the bulkhead: the hopper in the middle, the gauge it fills on the charger
// side, the lever on the couch side (well away from the charger, so the prompts never mix).
inline constexpr float RIG_LEVER_X = RIG_HOPPER.x - 0.78f;
inline constexpr float RIG_GAUGE_X = RIG_HOPPER.x + 0.78f;

// Flat arrow pointing down, drawn on a wall that faces -Z. `size` = its height.
inline void rigArrowDown(DecorBuilder& b, float x, float yTip, float wallZ, float size, int mat) {
    const float t = 0.014f;
    for (int i = 0; i < 4; i++) {
        float width = size * (0.14f + 0.16f * (float)i);
        b.box(x, yTip + size * 0.12f * (float)i, wallZ - t * 0.5f, width, size * 0.12f, t, mat);
    }
    b.box(x, yTip + size * 0.48f, wallZ - t * 0.5f, size * 0.20f, size * 0.52f, t, mat);
}

// A carton, the way loot is drawn everywhere else, on a wall that faces -Z.
inline void rigCartonIcon(DecorBuilder& b, float x, float y0, float wallZ, float size) {
    b.box(x, y0, wallZ - 0.010f, size, size * 0.80f, 0.020f, DECOR_MAT_CARDBOARD);
    b.box(x, y0 + size * 0.52f, wallZ - 0.024f, size * 1.04f, size * 0.10f, 0.008f, DECOR_MAT_RUBBER);
    b.box(x, y0 + size * 0.20f, wallZ - 0.024f, size * 0.30f, size * 0.16f, 0.008f, DECOR_MAT_WHITE);
}

// Couch, radar desk, charger and fuse box.
inline void buildRigFurniture(DecorBuilder& b, const RigLook& look) {
    const float h = RIG_CABIN_H;
    const float wall = RIG_HALF_W - RIG_WALL_T; // inner face of a side wall
    int lit = look.power ? DECOR_MAT_WHITE : DECOR_MAT_RUBBER;

    b.box(0.0f, h - 0.09f, RIG_HAVEN_Z, 0.60f, 0.06f, 0.30f, lit);
    b.box(0.0f, h - 0.05f, RIG_HAVEN_Z, 0.70f, 0.02f, 0.40f, DECOR_MAT_METAL);
    for (int i = 0; i < 8; i++) {
        // fairy lights along the window wall; somebody brought them from home
        float y = 2.16f - 0.06f * fabsf(sinf((float)i * 1.57f));
        int mat = !look.power ? DECOR_MAT_RUBBER : ((i & 1) ? DECOR_MAT_WHITE : DECOR_MAT_CAUTION);
        b.box(-wall + 0.03f, y, RIG_CABIN_BACK + 0.40f + (float)i * 0.48f, 0.05f, 0.05f, 0.05f, mat);
    }
    b.box(0.05f, 0.04f, -2.20f, 0.74f, 0.012f, 1.90f, DECOR_MAT_FABRIC); // rug

    // couch under the window, with the company plush on it
    b.box(-0.90f, 0.04f, -2.40f, 0.56f, 0.40f, 1.80f, DECOR_MAT_FABRIC);
    b.box(-1.13f, 0.44f, -2.40f, 0.12f, 0.46f, 1.80f, DECOR_MAT_FABRIC);
    b.box(-0.90f, 0.44f, -3.25f, 0.56f, 0.16f, 0.10f, DECOR_MAT_FABRIC);
    b.box(-0.90f, 0.44f, -1.55f, 0.56f, 0.16f, 0.10f, DECOR_MAT_FABRIC);
    b.box(-0.92f, 0.44f, -1.95f, 0.22f, 0.22f, 0.18f, DECOR_MAT_PLASTER);
    decorBall(b, -0.92f, 0.76f, -1.95f, 0.11f, DECOR_MAT_PLASTER, 6);
    b.box(-0.88f, 0.44f, -2.80f, 0.40f, 0.05f, 0.40f, DECOR_MAT_CARDBOARD); // pizza box
    b.box(-0.92f, 0.04f, -3.55f, 0.50f, 0.46f, 0.42f, DECOR_MAT_CARDBOARD); // carton of spare parts
    b.box(-0.92f, 0.50f, -3.55f, 0.30f, 0.12f, 0.20f, DECOR_MAT_ORANGE);

    // desk with the radar: loot around the rig shows up while somebody is home
    b.box(0.96f, 0.04f, -1.90f, 0.48f, 0.70f, 1.60f, DECOR_MAT_WOOD);
    b.box(0.94f, 0.74f, -1.90f, 0.54f, 0.03f, 1.66f, DECOR_MAT_TRIM);
    b.box(1.00f, 0.77f, RIG_RADAR.z, 0.34f, 0.34f, 0.42f, DECOR_MAT_WHITE);
    b.panel(0.824f, 0.81f, RIG_RADAR.z, 0.012f, 0.26f, 0.32f, look.power ? DECOR_MAT_SCREEN : DECOR_MAT_RUBBER);
    b.box(0.80f, 0.77f, RIG_RADAR.z - 0.42f, 0.16f, 0.025f, 0.34f, DECOR_MAT_DARK_METAL); // keyboard
    b.cylinder(0.84f, 0.77f, -2.52f, 0.04f, 0.09f, DECOR_MAT_WHITE, 6);
    b.cylinder(0.98f, 0.77f, -1.32f, 0.08f, 0.17f, DECOR_MAT_METAL, 8);

    // battery charger by the bulkhead: a dock with two wells and a battery drawn on it
    const float cz = RIG_CHARGER.z;
    b.box(wall - 0.10f, 0.86f, cz, 0.20f, 0.72f, 0.62f, DECOR_MAT_DARK_METAL);
    b.box(wall - 0.27f, 0.92f, cz, 0.16f, 0.05f, 0.54f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i += 2) {
        b.cylinder(wall - 0.27f, 0.97f, cz + 0.14f * (float)i, 0.068f, 0.025f, DECOR_MAT_RUBBER, 8);
    }
    b.panel(wall - 0.206f, 1.20f, cz, 0.012f, 0.22f, 0.34f, DECOR_MAT_WHITE);
    b.box(wall - 0.216f, 1.265f, cz + 0.01f, 0.010f, 0.09f, 0.18f, DECOR_MAT_RUBBER);
    b.box(wall - 0.216f, 1.29f, cz - 0.095f, 0.010f, 0.04f, 0.03f, DECOR_MAT_RUBBER);
    b.panel(wall - 0.206f, 1.47f, cz, 0.012f, 0.07f, 0.50f, look.charged ? DECOR_MAT_SCREEN : DECOR_MAT_RUBBER);
    decorRod(b, Vec3(wall - 0.10f, 1.58f, cz + 0.20f), Vec3(wall - 0.03f, h - 0.04f, cz + 0.42f), 0.02f, DECOR_MAT_RUBBER, 5);
    if (look.charged) {
        b.cylinder(wall - 0.27f, 0.97f, cz - 0.14f, 0.055f, 0.19f, DECOR_MAT_DRUM, 8);
        b.cylinder(wall - 0.27f, 1.16f, cz - 0.14f, 0.025f, 0.025f, DECOR_MAT_METAL, 6);
    }

    // fuse box by the door: shut with a bolt on it, or hanging open with a cable out
    RigPoint fuse = rigFaultPoint(RIG_FAULT_FUSE);
    bool blown = look.fault == RIG_FAULT_FUSE;
    b.box(wall - 0.06f, 1.00f, fuse.z, 0.12f, 0.60f, 0.42f, DECOR_MAT_DARK_METAL);
    if (blown) {
        b.panel(wall - 0.126f, 1.04f, fuse.z, 0.012f, 0.52f, 0.34f, DECOR_MAT_CAUTION);
        b.box(wall - 0.29f, 1.04f, fuse.z + 0.19f, 0.34f, 0.52f, 0.012f, DECOR_MAT_METAL);
        decorRod(b, Vec3(wall - 0.13f, 1.14f, fuse.z - 0.05f), Vec3(wall - 0.36f, 0.62f, fuse.z - 0.14f), 0.02f, DECOR_MAT_ORANGE, 5);
    } else {
        b.panel(wall - 0.126f, 1.04f, fuse.z, 0.012f, 0.52f, 0.34f, DECOR_MAT_METAL);
        for (int i = 0; i < 3; i++) {
            float step = (float)(i - 1);
            b.box(wall - 0.136f, 1.24f + step * 0.11f, fuse.z + step * 0.04f, 0.008f, 0.14f, 0.05f, DECOR_MAT_WHITE);
        }
        b.box(wall - 0.14f, 1.24f, fuse.z - 0.13f, 0.02f, 0.12f, 0.025f, DECOR_MAT_DARK_METAL);
    }
}

// The hopper: the one place loot goes. An open bin at waist height with crusher rolls in
// the bottom, a striped rim, a striped arrow coming down the wall into it and a carton
// drawn above the arrow.
inline void buildRigHopper(DecorBuilder& b) {
    const float bulk = RIG_CABIN_FRONT - RIG_WALL_T; // inner face of the bulkhead
    const float hx = RIG_HOPPER.x, hz = bulk - 0.32f;
    b.box(hx, 0.04f, hz + 0.08f, 0.44f, 0.58f, 0.44f, DECOR_MAT_DARK_METAL);
    b.taper(hx, 0.60f, hz, 0.46f, 0.38f, 1.00f, 0.60f, 0.26f, DECOR_MAT_ORANGE);
    // the bin: four walls around a dark floor, two rolls waiting in it
    const float binY = 0.86f, binH = 0.20f;
    b.box(hx, binY, hz - 0.28f, 1.00f, binH, 0.04f, DECOR_MAT_ORANGE);
    b.box(hx, binY, hz + 0.28f, 1.00f, binH, 0.04f, DECOR_MAT_ORANGE);
    b.box(hx - 0.48f, binY, hz, 0.04f, binH, 0.60f, DECOR_MAT_ORANGE);
    b.box(hx + 0.48f, binY, hz, 0.04f, binH, 0.60f, DECOR_MAT_ORANGE);
    b.box(hx, binY, hz, 0.92f, 0.09f, 0.52f, DECOR_MAT_RUBBER);
    for (int i = -1; i <= 1; i += 2) {
        float rz = hz + 0.10f * (float)i;
        decorRod(b, Vec3(hx - 0.44f, binY + 0.09f, rz), Vec3(hx + 0.44f, binY + 0.09f, rz), 0.065f, DECOR_MAT_DARK_METAL, 8);
    }
    b.box(hx, 1.02f, hz - 0.30f, 1.08f, 0.07f, 0.05f, DECOR_MAT_CAUTION);
    b.box(hx - 0.515f, 1.02f, hz, 0.05f, 0.07f, 0.60f, DECOR_MAT_CAUTION);
    b.box(hx + 0.515f, 1.02f, hz, 0.05f, 0.07f, 0.60f, DECOR_MAT_CAUTION);
    b.box(hx, 1.06f, bulk - 0.02f, 1.00f, 0.16f, 0.04f, DECOR_MAT_METAL); // splash board
    // the arrow is a real slab of steel, so it reads from the doorway
    b.taper(hx, 1.28f, bulk - 0.035f, 0.04f, 0.05f, 0.52f, 0.05f, 0.26f, DECOR_MAT_CAUTION);
    b.box(hx, 1.54f, bulk - 0.035f, 0.18f, 0.22f, 0.05f, DECOR_MAT_CAUTION);
    b.panel(hx, 1.82f, bulk - 0.012f, 0.56f, 0.44f, 0.024f, DECOR_MAT_WHITE);
    rigCartonIcon(b, hx, 1.90f, bulk - 0.024f, 0.32f);
    // where to stand
    b.decal(hx, 0.048f, bulk - 0.98f, 1.00f, 0.44f, DECOR_MAT_CAUTION);
}

// On the bulkhead: the hopper, the gauge it fills and the lever that sends everybody down.
inline void buildRigControls(DecorBuilder& b, const RigLook& look) {
    const float bulk = RIG_CABIN_FRONT - RIG_WALL_T;
    const float hx = RIG_HOPPER.x, gx = RIG_GAUGE_X, lx = RIG_LEVER_X;
    buildRigHopper(b);

    // gauge: one lamp per fifth of the quota, fed by a pipe from the hopper, a drill bit on top
    const float lampY = 0.78f, lampSpan = 1.22f;
    float pitch = lampSpan / (float)(look.gaugeLamps > 0 ? look.gaugeLamps : 1);
    b.box(gx, lampY - 0.06f, bulk - 0.03f, 0.32f, lampSpan + 0.08f, 0.06f, DECOR_MAT_DARK_METAL);
    for (int i = 0; i < look.gaugeLamps; i++) {
        int mat = i < look.lamps ? DECOR_MAT_WHITE : DECOR_MAT_RUBBER;
        b.box(gx, lampY + (float)i * pitch, bulk - 0.07f, 0.22f, pitch - 0.05f, 0.02f, mat);
    }
    decorRod(b, Vec3(hx + 0.20f, 0.30f, bulk - 0.16f), Vec3(gx, 0.30f, bulk - 0.10f), 0.04f, DECOR_MAT_METAL, 6);
    decorRod(b, Vec3(gx, 0.30f, bulk - 0.10f), Vec3(gx, lampY - 0.04f, bulk - 0.06f), 0.04f, DECOR_MAT_METAL, 6);
    decorBand(b, gx, 2.04f, bulk - 0.11f, 0.0f, 0.11f, 0.17f, DECOR_MAT_METAL, 6, false, true);
    b.cylinder(gx, 2.21f, bulk - 0.11f, 0.05f, 0.07f, DECOR_MAT_DARK_METAL, 6);

    // a conduit runs from the full gauge over the hopper to the lever: fill one, pull the other
    const float runY = 2.33f, runZ = bulk - 0.03f;
    decorRod(b, Vec3(gx, 2.27f, bulk - 0.11f), Vec3(gx, runY, runZ), 0.02f, DECOR_MAT_ORANGE, 5);
    decorRod(b, Vec3(gx, runY, runZ), Vec3(lx, runY, runZ), 0.02f, DECOR_MAT_ORANGE, 5);
    decorRod(b, Vec3(lx, runY, runZ), Vec3(lx, 1.92f, runZ), 0.02f, DECOR_MAT_ORANGE, 5);

    // lever: latched and dark until the gauge is full, then the arrow plate lights up
    bool armed = look.ready || look.pulled;
    b.panel(lx, 1.52f, bulk - 0.012f, 0.34f, 0.40f, 0.024f, armed ? DECOR_MAT_WHITE : DECOR_MAT_DARK_METAL);
    rigArrowDown(b, lx, 1.57f, bulk - 0.024f, 0.30f, armed ? DECOR_MAT_RUBBER : DECOR_MAT_FABRIC);
    b.box(lx, 0.84f, bulk - 0.05f, 0.28f, 0.56f, 0.10f, DECOR_MAT_DARK_METAL);
    b.box(lx, 0.90f, bulk - 0.105f, 0.05f, 0.44f, 0.01f, DECOR_MAT_RUBBER);
    Vec3 pivot(lx, 1.12f, bulk - 0.08f);
    Vec3 knob(lx, look.pulled ? 0.82f : 1.44f, bulk - 0.36f);
    decorRod(b, pivot, knob, 0.025f, DECOR_MAT_METAL, 6);
    decorBall(b, knob.x, knob.y, knob.z, 0.07f, DECOR_MAT_ORANGE, 8);
    if (!armed) {
        b.box(lx, 1.24f, bulk - 0.24f, 0.36f, 0.05f, 0.04f, DECOR_MAT_METAL);
        b.box(lx - 0.16f, 1.24f, bulk - 0.13f, 0.04f, 0.05f, 0.24f, DECOR_MAT_METAL);
        b.box(lx + 0.16f, 1.24f, bulk - 0.13f, 0.04f, 0.05f, 0.24f, DECOR_MAT_METAL);
    }

    // the engine comes through the wall here: two pipes and a stopcock nobody touches
    const float px = RIG_HALF_W - RIG_WALL_T - 0.10f;
    b.cylinder(px, 0.04f, bulk - 0.06f, 0.035f, RIG_CABIN_H - 0.08f, DECOR_MAT_METAL, 6);
    b.cylinder(px - 0.11f, 0.04f, bulk - 0.06f, 0.035f, RIG_CABIN_H - 0.08f, DECOR_MAT_DARK_METAL, 6);
    b.box(px, 1.30f, bulk - 0.06f, 0.10f, 0.08f, 0.10f, DECOR_MAT_DARK_METAL);
    decorRod(b, Vec3(px, 1.34f, bulk - 0.11f), Vec3(px, 1.34f, bulk - 0.15f), 0.07f, DECOR_MAT_ORANGE, 6);
}
