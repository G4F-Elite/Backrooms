#pragma once
// Rig meshes, the outside, in rig space (see rig_rules.h): crawler tracks, the hull with the
// break room, the engine with its two service hatches, and the auger as a separate part so
// it can spin. The inside is in rig_cabin_models.h.
// Stripes mean "hands go here": the doorway, the hopper, and whichever hatch has to be fixed.

#include "rig_cabin_models.h"

// Crawler tracks either side of the hull, with a fender over each.
inline void buildRigTracks(DecorBuilder& b) {
    const float z0 = RIG_TRACK_BACK, z1 = RIG_TRACK_FRONT, r = RIG_TRACK_R;
    const float len = z1 - z0, mid = (z0 + z1) * 0.5f, tw = RIG_TRACK_W;
    for (int s = -1; s <= 1; s += 2) {
        float sx = (float)s;
        float in = sx * RIG_HALF_W, cx = in + sx * tw * 0.5f, out = in + sx * tw;
        b.box(cx, 0.0f, mid, tw, 0.07f, len, DECOR_MAT_RUBBER);
        b.box(cx, r * 2.0f - 0.07f, mid, tw, 0.07f, len, DECOR_MAT_RUBBER);
        decorRod(b, Vec3(in, r, z0), Vec3(out, r, z0), r, DECOR_MAT_RUBBER, 12);
        decorRod(b, Vec3(in, r, z1), Vec3(out, r, z1), r, DECOR_MAT_RUBBER, 12);
        b.box(in + sx * (tw - 0.07f) * 0.5f, 0.07f, mid, tw - 0.07f, r * 2.0f - 0.14f, len, DECOR_MAT_DARK_METAL);
        // sprockets at the ends, road wheels between them
        const int wheels = 7;
        for (int i = 0; i < wheels; i++) {
            bool end = i == 0 || i == wheels - 1;
            float z = z0 + len * (float)i / (float)(wheels - 1);
            float y = end ? r : 0.21f, wr = end ? 0.20f : 0.15f;
            decorRod(b, Vec3(out - sx * 0.08f, y, z), Vec3(out - sx * 0.012f, y, z), wr, DECOR_MAT_METAL, 8);
            decorRod(b, Vec3(out - sx * 0.03f, y, z), Vec3(out + sx * 0.004f, y, z), wr * 0.38f, DECOR_MAT_DARK_METAL, 6);
        }
        for (float z = z0; z <= z1 + 0.01f; z += 0.26f) {
            b.box(cx, r * 2.0f, z, tw + 0.02f, 0.022f, 0.10f, DECOR_MAT_DARK_METAL); // cleats
        }
        b.box(in + sx * 0.12f, r * 2.0f + 0.045f, mid, 0.26f, 0.04f, len + r * 2.0f + 0.04f, DECOR_MAT_DARK_METAL);
        b.box(cx, 0.18f, z0 - r - 0.02f, tw, r * 2.0f - 0.13f, 0.02f, DECOR_MAT_RUBBER); // mud flap
    }
}

// Walls, floor, roof and the doorway. Site-trailer orange outside, wood inside: together
// with the garage this is the only room in the game that does not look like the Backrooms.
inline void buildRigShell(DecorBuilder& b, const RigLook& look) {
    const float w = RIG_HALF_W, t = RIG_WALL_T, h = RIG_CABIN_H;
    const float back = RIG_CABIN_BACK, front = RIG_CABIN_FRONT;
    const float len = front - back, mid = (front + back) * 0.5f;
    b.box(0.0f, 0.005f, mid, (w - t) * 2.0f, 0.035f, len, DECOR_MAT_WOOD);

    // side walls, a window in each: over the couch and over the radar desk
    const float winZ0 = -2.95f, winZ1 = -1.85f, winY0 = 1.20f, winY1 = 1.85f;
    const float winMid = (winZ0 + winZ1) * 0.5f, winLen = winZ1 - winZ0;
    for (int s = -1; s <= 1; s += 2) {
        float sx = (float)s;
        auto side = [&](float y0, float y1, float z0, float z1) {
            float cz = (z0 + z1) * 0.5f;
            b.box(sx * (w - t * 0.25f), y0, cz, t * 0.5f, y1 - y0, z1 - z0, DECOR_MAT_ORANGE);
            b.box(sx * (w - t * 0.75f), y0, cz, t * 0.5f, y1 - y0, z1 - z0, DECOR_MAT_WOOD);
        };
        side(0.0f, winY0, back, front);
        side(winY1, h, back, front);
        side(winY0, winY1, back, winZ0);
        side(winY0, winY1, winZ1, front);
        float wx = sx * (w - t * 0.5f);
        b.box(wx, winY0 - 0.04f, winMid, t + 0.08f, 0.04f, winLen + 0.10f, DECOR_MAT_METAL);
        b.box(wx, winY1, winMid, t + 0.08f, 0.04f, winLen + 0.10f, DECOR_MAT_METAL);
        b.box(wx, winY0, winZ0 - 0.02f, t + 0.04f, winY1 - winY0, 0.04f, DECOR_MAT_METAL);
        b.box(wx, winY0, winZ1 + 0.02f, t + 0.04f, winY1 - winY0, 0.04f, DECOR_MAT_METAL);
        b.box(wx, winY0, winMid, 0.03f, winY1 - winY0, 0.03f, DECOR_MAT_METAL);

        // outside: a dark sill behind the track, a white band, ribs between the panels
        float skin = sx * (w + 0.006f);
        b.box(skin, 0.0f, mid, 0.012f, 0.70f, len, DECOR_MAT_DARK_METAL);
        b.box(skin, 0.84f, mid, 0.012f, 0.12f, len, DECOR_MAT_WHITE);
        static const float ribs[4] = {-3.84f, -3.12f, -1.68f, -0.74f};
        for (float rz : ribs) b.box(sx * (w + 0.02f), 0.70f, rz, 0.04f, h - 0.70f, 0.08f, DECOR_MAT_DARK_METAL);
    }

    auto rear = [&](float x0, float x1, float y0, float y1) {
        float cx = (x0 + x1) * 0.5f;
        b.box(cx, y0, back + t * 0.25f, x1 - x0, y1 - y0, t * 0.5f, DECOR_MAT_ORANGE);
        b.box(cx, y0, back + t * 0.75f, x1 - x0, y1 - y0, t * 0.5f, DECOR_MAT_WOOD);
    };
    rear(-w, -RIG_DOOR_HALF, 0.0f, h);
    rear(RIG_DOOR_HALF, w, 0.0f, h);
    rear(-RIG_DOOR_HALF, RIG_DOOR_HALF, RIG_DOOR_H, h);
    b.box(0.0f, 0.0f, front - t * 0.25f, w * 2.0f, h, t * 0.5f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.0f, front - t * 0.75f, (w - t) * 2.0f, h, t * 0.5f, DECOR_MAT_WOOD);
    b.box(0.0f, h, mid, w * 2.0f + 0.14f, 0.07f, len + 0.14f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, h - 0.03f, mid, (w - t) * 2.0f, 0.03f, len - t * 2.0f, DECOR_MAT_WHITE);

    // doorway: hazard frame, a beacon that is lit while it is safe here, a steel ramp
    const float face = back - 0.015f;
    b.box(-RIG_DOOR_HALF - 0.05f, 0.0f, face, 0.10f, RIG_DOOR_H + 0.10f, 0.03f, DECOR_MAT_CAUTION);
    b.box(RIG_DOOR_HALF + 0.05f, 0.0f, face, 0.10f, RIG_DOOR_H + 0.10f, 0.03f, DECOR_MAT_CAUTION);
    b.box(0.0f, RIG_DOOR_H, face, RIG_DOOR_HALF * 2.0f, 0.10f, 0.03f, DECOR_MAT_CAUTION);
    b.box(0.0f, RIG_DOOR_H + 0.14f, back - 0.07f, 0.40f, 0.18f, 0.14f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, RIG_DOOR_H + 0.16f, back - 0.15f, 0.32f, 0.14f, 0.03f, look.power ? DECOR_MAT_WHITE : DECOR_MAT_RUBBER);
    b.box(0.0f, 0.0f, back - 0.40f, 1.34f, 0.03f, 0.74f, DECOR_MAT_METAL);
    b.box(0.0f, 0.03f, back - 0.72f, 1.34f, 0.012f, 0.08f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.036f, back + 0.36f, 1.10f, 0.014f, 0.46f, DECOR_MAT_RUBBER); // doormat, inside
    for (int s = -1; s <= 1; s += 2) {
        float gx = (float)s * (RIG_DOOR_HALF + 0.20f);
        decorRod(b, Vec3(gx, 0.95f, back - 0.07f), Vec3(gx, 1.55f, back - 0.07f), 0.022f, DECOR_MAT_METAL, 5);
        b.box(gx, 0.95f, back - 0.035f, 0.04f, 0.04f, 0.07f, DECOR_MAT_METAL);
        b.box(gx, 1.51f, back - 0.035f, 0.04f, 0.04f, 0.07f, DECOR_MAT_METAL);
        b.box((float)s * 0.975f, 0.0f, back - 0.05f, 0.65f, 0.20f, 0.10f, DECOR_MAT_DARK_METAL); // bumper halves
    }
    // right of the door: what this machine eats. Left of the door: the way up to the roof.
    b.panel(0.99f, 1.02f, back - 0.012f, 0.40f, 0.86f, 0.024f, DECOR_MAT_WHITE);
    rigCartonIcon(b, 0.99f, 1.58f, back - 0.024f, 0.26f);
    rigArrowDown(b, 0.99f, 1.31f, back - 0.024f, 0.22f, DECOR_MAT_RUBBER);
    b.taper(0.99f, 1.08f, back - 0.034f, 0.10f, 0.02f, 0.28f, 0.02f, 0.17f, DECOR_MAT_ORANGE);
    for (int s = -1; s <= 1; s += 2) {
        b.box(-0.99f + (float)s * 0.17f, 0.24f, back - 0.07f, 0.035f, h - 0.14f, 0.035f, DECOR_MAT_METAL);
    }
    for (int i = 0; i < 7; i++) {
        b.box(-0.99f, 0.42f + (float)i * 0.32f, back - 0.07f, 0.34f, 0.03f, 0.03f, DECOR_MAT_METAL);
    }
}

// What rides on the roof: lights that look where the drill goes, the radar dish for the desk
// below, a vent, a beacon and the things nobody found a place for.
inline void buildRigRoof(DecorBuilder& b, const RigLook& look) {
    const float top = RIG_CABIN_H + 0.07f, barZ = RIG_CABIN_FRONT - 0.12f;
    int lit = look.power ? DECOR_MAT_WHITE : DECOR_MAT_RUBBER;
    b.box(0.0f, top, barZ, 2.10f, 0.10f, 0.12f, DECOR_MAT_DARK_METAL);
    for (int i = 0; i < 4; i++) {
        float x = -0.78f + (float)i * 0.52f;
        b.box(x, top + 0.10f, barZ, 0.30f, 0.20f, 0.14f, DECOR_MAT_DARK_METAL);
        b.box(x, top + 0.13f, barZ + 0.08f, 0.24f, 0.14f, 0.03f, lit);
    }
    b.box(-0.45f, top, -2.85f, 0.74f, 0.26f, 0.92f, DECOR_MAT_WHITE);
    b.box(-0.45f, top + 0.26f, -2.85f, 0.56f, 0.03f, 0.72f, DECOR_MAT_DARK_METAL);
    // radar dish, straight above the radar desk
    const float dishX = RIG_RADAR.x - 0.10f;
    b.cylinder(dishX, top, RIG_RADAR.z, 0.035f, 0.62f, DECOR_MAT_METAL, 6);
    decorBand(b, dishX, top + 0.62f, RIG_RADAR.z, 0.05f, 0.30f, 0.12f, DECOR_MAT_WHITE, 8, true, true);
    b.cylinder(dishX, top + 0.74f, RIG_RADAR.z, 0.02f, 0.16f, DECOR_MAT_ORANGE, 5);
    // rack with a crate, a rolled tarp and a spare drum
    for (int s = -1; s <= 1; s += 2) {
        float x = 0.55f + (float)s * 0.55f;
        decorRod(b, Vec3(x, top + 0.16f, -1.25f), Vec3(x, top + 0.16f, -0.20f), 0.02f, DECOR_MAT_METAL, 5);
        b.box(x, top, -1.22f, 0.04f, 0.16f, 0.04f, DECOR_MAT_METAL);
        b.box(x, top, -0.23f, 0.04f, 0.16f, 0.04f, DECOR_MAT_METAL);
    }
    b.box(0.72f, top, -0.52f, 0.50f, 0.32f, 0.46f, DECOR_MAT_CARDBOARD);
    decorRod(b, Vec3(0.12f, top + 0.11f, -1.15f), Vec3(0.94f, top + 0.11f, -1.02f), 0.11f, DECOR_MAT_FABRIC, 7);
    b.cylinder(0.26f, top, -0.50f, 0.17f, 0.42f, DECOR_MAT_DRUM, 8);
    b.cylinder(-0.80f, top, RIG_CABIN_BACK + 0.30f, 0.11f, 0.05f, DECOR_MAT_DARK_METAL, 8);
    b.cylinder(-0.80f, top + 0.05f, RIG_CABIN_BACK + 0.30f, 0.08f, 0.13f, DECOR_MAT_ORANGE, 8);
}

// A service hatch on the side of the engine. Shut it is a steel plate with a handle; when
// it is the thing that broke, the plate is on the floor and the hole is framed in stripes.
inline void buildRigHatch(DecorBuilder& b, float side, bool open) {
    const float x = side * RIG_HALF_W, z = rigFaultPoint(RIG_FAULT_BELT).z;
    const float y0 = 0.74f, size = 0.80f;
    if (!open) {
        b.panel(x + side * 0.015f, y0, z, 0.03f, size, size, DECOR_MAT_METAL);
        b.box(x + side * 0.045f, y0 + 0.34f, z + 0.28f, 0.03f, 0.12f, 0.04f, DECOR_MAT_DARK_METAL);
        b.box(x + side * 0.04f, y0 + 0.10f, z - 0.37f, 0.03f, 0.10f, 0.06f, DECOR_MAT_DARK_METAL);
        b.box(x + side * 0.04f, y0 + 0.60f, z - 0.37f, 0.03f, 0.10f, 0.06f, DECOR_MAT_DARK_METAL);
        return;
    }
    b.panel(x + side * 0.015f, y0, z, 0.03f, size, size, DECOR_MAT_CAUTION);
    b.panel(x + side * 0.020f, y0 + 0.09f, z, 0.03f, size - 0.18f, size - 0.18f, DECOR_MAT_RUBBER);
    Vec3 top(x + side * (RIG_TRACK_W + 0.03f), RIG_TRACK_R * 2.0f + 0.08f, z + 0.06f);
    Vec3 foot(x + side * (RIG_TRACK_W + 0.42f), 0.02f, z + 0.16f);
    decorBeam(b, top, foot, size, 0.03f, DECOR_MAT_METAL);
}

// The part that keeps breaking. It is a prototype: loose cables, a toolbox left on top,
// two service hatches that come off far too easily.
inline void buildRigEngine(DecorBuilder& b, const RigLook& look) {
    const float w = RIG_HALF_W, z0 = RIG_CABIN_FRONT, z1 = RIG_ENGINE_FRONT;
    const float len = z1 - z0, mid = (z0 + z1) * 0.5f, deck = 1.66f;
    int lit = look.power ? DECOR_MAT_WHITE : DECOR_MAT_RUBBER;
    b.box(0.0f, 0.10f, mid, w * 2.0f, 0.56f, len, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.66f, mid, w * 2.0f, deck - 0.66f, len, DECOR_MAT_ORANGE);
    b.box(0.0f, deck, mid - 0.12f, 1.90f, 0.28f, len - 0.30f, DECOR_MAT_DARK_METAL); // radiator cowl
    b.box(0.0f, deck + 0.28f, mid - 0.12f, 2.00f, 0.03f, len - 0.22f, DECOR_MAT_METAL);
    for (int s = -1; s <= 1; s += 2) {
        b.box((float)s * (w - 0.04f), deck, mid, 0.04f, 0.10f, len, DECOR_MAT_METAL); // deck rail
    }

    // exhaust stack with a rain cap, air filter, the toolbox, two loose cables
    const float cowl = deck + 0.31f;
    b.cylinder(0.66f, cowl, 0.52f, 0.08f, 0.86f, DECOR_MAT_DARK_METAL, 8);
    b.cylinder(0.66f, cowl, 0.52f, 0.11f, 0.10f, DECOR_MAT_METAL, 8);
    decorBeam(b, Vec3(0.56f, cowl + 0.88f, 0.52f), Vec3(0.80f, cowl + 0.96f, 0.52f), 0.22f, 0.02f, DECOR_MAT_METAL);
    b.cylinder(-0.62f, cowl, 0.55f, 0.15f, 0.20f, DECOR_MAT_METAL, 8);
    b.cylinder(-0.62f, cowl + 0.20f, 0.55f, 0.17f, 0.04f, DECOR_MAT_DARK_METAL, 8);
    b.box(-0.05f, cowl, 0.95f, 0.42f, 0.16f, 0.20f, DECOR_MAT_ORANGE);
    b.box(-0.05f, cowl + 0.16f, 0.95f, 0.16f, 0.03f, 0.04f, DECOR_MAT_DARK_METAL);
    decorRod(b, Vec3(-0.62f, cowl + 0.12f, 0.45f), Vec3(-0.55f, RIG_CABIN_H + 0.07f, 0.12f), 0.025f, DECOR_MAT_RUBBER, 5);
    decorRod(b, Vec3(0.30f, cowl, 1.10f), Vec3(0.62f, 1.10f, z1 + 0.03f), 0.025f, DECOR_MAT_ORANGE, 5);

    // front: headlights, a bumper, and the gearbox the auger hangs on
    for (int s = -1; s <= 1; s += 2) {
        float sx = (float)s;
        b.box(sx * 0.98f, 1.16f, z1 + 0.03f, 0.40f, 0.30f, 0.06f, DECOR_MAT_DARK_METAL);
        b.box(sx * 0.98f, 1.20f, z1 + 0.07f, 0.30f, 0.22f, 0.03f, lit);
        Vec3 ramTop(sx * 0.52f, cowl - 0.06f, 1.05f), ramFoot(sx * 0.36f, RIG_AUGER_HEIGHT + 0.42f, RIG_AUGER_BASE - 0.05f);
        decorRod(b, ramTop, ramFoot, 0.045f, DECOR_MAT_METAL, 6);
    }
    b.box(0.0f, 0.12f, z1 + 0.06f, (w + RIG_TRACK_W) * 2.0f, 0.26f, 0.12f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.60f, z1 + 0.025f, 1.44f, 1.42f, 0.05f, DECOR_MAT_DARK_METAL);
    Vec3 axis0(0.0f, RIG_AUGER_HEIGHT, z1 - 0.10f), axis1(0.0f, RIG_AUGER_HEIGHT, RIG_AUGER_BASE);
    decorRod(b, axis0, axis1, 0.50f, DECOR_MAT_DARK_METAL, 10);
    decorRod(b, Vec3(0.0f, RIG_AUGER_HEIGHT, z1 + 0.08f), Vec3(0.0f, RIG_AUGER_HEIGHT, z1 + 0.14f), 0.60f, DECOR_MAT_METAL, 10);

    // left hatch: the drive belt
    const float past = w + RIG_TRACK_W; // outer face of a track
    bool belt = look.fault == RIG_FAULT_BELT;
    buildRigHatch(b, -1.0f, belt);
    if (belt) {
        decorRod(b, Vec3(-w - 0.01f, 1.28f, 0.66f), Vec3(-w - 0.07f, 1.28f, 0.66f), 0.15f, DECOR_MAT_METAL, 8);
        decorRod(b, Vec3(-w - 0.01f, 0.98f, 1.06f), Vec3(-w - 0.07f, 0.98f, 1.06f), 0.09f, DECOR_MAT_METAL, 8);
        decorRod(b, Vec3(-w - 0.06f, 1.40f, 0.70f), Vec3(-past - 0.10f, 0.66f, 0.50f), 0.03f, DECOR_MAT_FABRIC, 5);
        decorRod(b, Vec3(-past - 0.10f, 0.66f, 0.50f), Vec3(-past - 0.34f, 0.03f, 0.62f), 0.03f, DECOR_MAT_FABRIC, 5);
    }
    // right hatch: the pressure valve, with its wheel on the outside
    bool valve = look.fault == RIG_FAULT_VALVE;
    buildRigHatch(b, 1.0f, valve);
    decorRod(b, Vec3(w + 0.03f, 1.14f, 0.85f), Vec3(w + 0.11f, 1.14f, 0.85f), 0.17f, DECOR_MAT_ORANGE, 8);
    decorRod(b, Vec3(w, 1.14f, 0.85f), Vec3(w + 0.14f, 1.14f, 0.85f), 0.04f, DECOR_MAT_DARK_METAL, 6);
    if (valve) {
        decorRod(b, Vec3(w + 0.04f, 0.92f, 1.10f), Vec3(past + 0.08f, 0.66f, 1.16f), 0.03f, DECOR_MAT_ORANGE, 5);
        decorRod(b, Vec3(past + 0.08f, 0.66f, 1.16f), Vec3(past + 0.30f, 0.03f, 1.24f), 0.03f, DECOR_MAT_ORANGE, 5);
    }
}

inline void buildRigHullModel(DecorBuilder& b, const RigLook& look) {
    buildRigTracks(b);
    buildRigShell(b, look);
    buildRigRoof(b, look);
    buildRigFurniture(b, look);
    buildRigControls(b, look);
    buildRigEngine(b, look);
}

// Built standing on its base (tip up); the renderer lays it along the rig's heading and spins it.
// A cone with two cutting flights wound around it, a toothed base ring and a hard tip.
inline void buildRigAugerModel(DecorBuilder& b) {
    const float len = RIG_NOSE - RIG_AUGER_BASE, tipR = 0.06f;
    b.cylinder(0.0f, 0.0f, 0.0f, RIG_AUGER_RADIUS + 0.02f, 0.10f, DECOR_MAT_METAL, 10);
    decorCone(b, 0.0f, 0.10f, 0.0f, RIG_AUGER_RADIUS, tipR, len - 0.28f, DECOR_MAT_DARK_METAL, 10);
    decorCone(b, 0.0f, len - 0.20f, 0.0f, 0.10f, 0.0f, 0.20f, DECOR_MAT_METAL, 6);
    const int steps = 22;
    const float turns = 2.25f, y0 = 0.12f, y1 = len - 0.22f;
    for (int flight = 0; flight < 2; flight++) {
        Vec3 prev;
        for (int i = 0; i <= steps; i++) {
            float k = (float)i / (float)steps;
            float y = y0 + (y1 - y0) * k;
            float a = (float)flight * 3.1415927f + k * turns * 6.2831853f;
            float r = (RIG_AUGER_RADIUS + (tipR - RIG_AUGER_RADIUS) * (y / len)) * 0.92f + 0.03f;
            Vec3 at(sinf(a) * r, y, cosf(a) * r);
            if (i > 0) decorBeam(b, prev, at, 0.20f - 0.10f * k, 0.045f, DECOR_MAT_METAL);
            prev = at;
        }
    }
    for (int i = 0; i < 10; i++) {
        float a = ((float)i + 0.5f) * 0.6283185f;
        float r = RIG_AUGER_RADIUS - 0.04f;
        b.box(sinf(a) * r, 0.10f, cosf(a) * r, 0.10f, 0.09f, 0.10f, DECOR_MAT_CAUTION);
    }
}

// What stays where the rig came down: a torn ceiling and what fell out of it.
// Origin = centre of the arrival cell, +Z = the way the rig left. ceilingH = room height.
inline void buildRigArrivalModel(DecorBuilder& b, float ceilingH) {
    b.box(0.0f, ceilingH - 0.05f, -1.60f, 2.90f, 0.04f, 5.60f, DECOR_MAT_RUBBER);
    decorRod(b, Vec3(-1.10f, ceilingH - 0.05f, -2.60f), Vec3(-1.30f, 2.40f, -2.90f), 0.03f, DECOR_MAT_ORANGE, 5);
    decorRod(b, Vec3(1.00f, ceilingH - 0.05f, -0.80f), Vec3(1.25f, 3.00f, -0.60f), 0.03f, DECOR_MAT_RUBBER, 5);
    b.box(-1.80f, 0.0f, -3.40f, 0.50f, 0.20f, 0.44f, DECOR_MAT_CEILTILE);
    b.box(1.70f, 0.0f, -1.90f, 0.40f, 0.14f, 0.52f, DECOR_MAT_CEILTILE);
    b.box(1.90f, 0.0f, -4.00f, 0.34f, 0.30f, 0.30f, DECOR_MAT_PLASTER);
    b.box(-1.90f, 0.0f, -0.90f, 0.30f, 0.10f, 0.36f, DECOR_MAT_PLASTER);
}
