#pragma once
// The rig: the crew's drill on crawler tracks with a break room bolted to its back. It crawls through
// the floor in a straight line, chewing through whatever wall is in the way, and it is the
// only place down here that is safe - for as long as it runs.
// Pure numbers and rig-local geometry (no engine dependencies, covered by tests).
// Rig space: metres, origin on the floor under the gearbox, +Z = direction of travel, +X = right.

#include <cmath>

inline constexpr float RIG_SPEED = 0.30f;         // m/s, slower than a crouch
inline constexpr float RIG_HALF_W = 1.30f;        // hull half width (a 5 m cell leaves 1.2 m each side)
inline constexpr float RIG_TRACK_W = 0.20f;       // crawler tracks stick out this far past the hull
inline constexpr float RIG_TRACK_R = 0.29f;       // sprocket radius: the track is twice this tall
inline constexpr float RIG_TRACK_BACK = -3.50f;   // rear axle
inline constexpr float RIG_TRACK_FRONT = 1.18f;   // front axle
inline constexpr float RIG_CABIN_BACK = -3.90f;   // rear wall, with the doorway
inline constexpr float RIG_CABIN_FRONT = 0.20f;   // bulkhead between the break room and the engine
inline constexpr float RIG_ENGINE_FRONT = 1.50f;
inline constexpr float RIG_AUGER_BASE = 1.72f;
inline constexpr float RIG_NOSE = 3.05f;          // auger tip
inline constexpr float RIG_AUGER_RADIUS = 0.72f;
inline constexpr float RIG_AUGER_HEIGHT = 1.25f;  // axis above the floor
inline constexpr float RIG_WALL_T = 0.10f;
inline constexpr float RIG_DOOR_HALF = 0.65f;
inline constexpr float RIG_DOOR_H = 2.05f;
inline constexpr float RIG_CABIN_H = 2.45f;
inline constexpr float RIG_CARVE_LEAD = 1.2f;     // walls give way this far ahead of the auger tip

// The haven: while the engine runs nothing from the halls comes this close to the break room.
inline constexpr float RIG_HAVEN_Z = -1.85f;      // centre of the break room
inline constexpr float RIG_HAVEN_RADIUS = 14.0f;
inline constexpr float RIG_QUIET_RADIUS = 27.0f;        // nothing loiters or turns up this close to a running rig
inline constexpr float RIG_HAVEN_RESERVE_RADIUS = 2.6f; // on the reserve battery: the room only
inline constexpr float RIG_RESERVE_SECONDS = 25.0f;     // how long the lamp lasts after a breakdown
inline constexpr float RIG_BACKFIRE_SECONDS = 7.0f;     // a dead rig bangs this often...
inline constexpr float RIG_BACKFIRE_NOISE = 28.0f;      // ...and this far
inline constexpr float RIG_REPAIR_SECONDS = 2.2f;
inline constexpr float RIG_CHARGE_SECONDS = 70.0f;      // engine time per battery from the charger
inline constexpr float RIG_REACH = 1.45f;               // how close a hand has to be to a panel
inline constexpr float RIG_AUGER_DPS = 34.0f;           // do not stand in front of it

// What is wrong with it this time. Each fault is fixed at its own panel.
enum RigFault {
    RIG_OK = 0,
    RIG_FAULT_FUSE = 1,   // fuse box inside, by the door
    RIG_FAULT_BELT = 2,   // engine cover outside, left
    RIG_FAULT_VALVE = 3,  // engine cover outside, right
    RIG_FAULT_COUNT = 4
};

// Things a hand can reach on the rig.
enum RigSpot {
    RIG_SPOT_NONE = 0,
    RIG_SPOT_HOPPER,   // loot goes in, the start lever is next to it
    RIG_SPOT_CHARGER,
    RIG_SPOT_FAULT
};

struct RigPoint { float x, z; };

inline constexpr RigPoint RIG_HOPPER = {-0.15f, 0.05f}; // straight ahead from the door
inline constexpr RigPoint RIG_CHARGER = {1.12f, -0.55f};
inline constexpr RigPoint RIG_RADAR = {1.00f, -1.90f};

inline RigPoint rigFaultPoint(int fault) {
    if (fault == RIG_FAULT_FUSE) return {1.15f, -3.35f};
    if (fault == RIG_FAULT_BELT) return {-RIG_HALF_W, 0.85f};
    if (fault == RIG_FAULT_VALVE) return {RIG_HALF_W, 0.85f};
    return {0.0f, 0.0f};
}

inline bool rigFaultIsOutside(int fault) {
    return fault == RIG_FAULT_BELT || fault == RIG_FAULT_VALVE;
}

// ---- heading ---------------------------------------------------------------------------
// dir: 0 = +X, 1 = -X, 2 = +Z, 3 = -Z (same order as the wall sides elsewhere).
inline int rigDirX(int dir) { return dir == 0 ? 1 : (dir == 1 ? -1 : 0); }
inline int rigDirZ(int dir) { return dir == 2 ? 1 : (dir == 3 ? -1 : 0); }

inline float rigYaw(int dir) {
    return atan2f((float)rigDirX(dir), (float)rigDirZ(dir));
}

struct RigFrame {
    float x = 0.0f, z = 0.0f; // world position of the rig origin
    int dir = 0;
};

// Exact for the four headings: forward = (dx, dz), right = (dz, -dx), the same basis the
// renderer and DecorBuilder use for that yaw.
inline void rigToWorld(const RigFrame& f, float lx, float lz, float& wx, float& wz) {
    float dx = (float)rigDirX(f.dir), dz = (float)rigDirZ(f.dir);
    wx = f.x + lx * dz + lz * dx;
    wz = f.z - lx * dx + lz * dz;
}

inline void rigToLocal(const RigFrame& f, float wx, float wz, float& lx, float& lz) {
    float dx = (float)rigDirX(f.dir), dz = (float)rigDirZ(f.dir);
    float rx = wx - f.x, rz = wz - f.z;
    lx = rx * dz - rz * dx;
    lz = rx * dx + rz * dz;
}

// ---- hull --------------------------------------------------------------------------------
// exits: sides a body may be pushed out through (1 = -X, 2 = +X, 4 = back, 8 = front).
// Furniture never shoves anybody into the wall behind it, the auger never into the engine.
struct RigBox { float x0, x1, z0, z1; int exits; };

inline constexpr int RIG_BOX_COUNT = 8;
inline constexpr int RIG_BOX_AUGER = 5;

inline RigBox rigBox(int i) {
    const float w = RIG_HALF_W, t = RIG_WALL_T, o = RIG_HALF_W + RIG_TRACK_W;
    static const RigBox boxes[RIG_BOX_COUNT] = {
        {-o, -w + t, RIG_CABIN_BACK, RIG_CABIN_FRONT, 3},                      // left wall + track
        {w - t, o, RIG_CABIN_BACK, RIG_CABIN_FRONT, 3},                        // right wall + track
        {-w, -RIG_DOOR_HALF, RIG_CABIN_BACK, RIG_CABIN_BACK + t, 12},          // rear wall, left of the door
        {RIG_DOOR_HALF, w, RIG_CABIN_BACK, RIG_CABIN_BACK + t, 12},            // rear wall, right of the door
        {-o, o, RIG_CABIN_FRONT - t, RIG_ENGINE_FRONT, 15},                    // bulkhead + engine
        {-RIG_AUGER_RADIUS, RIG_AUGER_RADIUS, RIG_ENGINE_FRONT, RIG_NOSE, 11}, // auger
        {-w + t, -0.62f, RIG_CABIN_BACK + t, -1.50f, 10},                      // couch + the carton behind it
        {0.72f, w - t, -2.70f, -1.10f, 13}                                     // desk with the radar
    };
    return boxes[i];
}

// Index of the hull part a body of radius r overlaps, -1 when it is clear.
inline int rigHitBox(float lx, float lz, float r) {
    for (int i = 0; i < RIG_BOX_COUNT; i++) {
        RigBox b = rigBox(i);
        if (lx > b.x0 - r && lx < b.x1 + r && lz > b.z0 - r && lz < b.z1 + r) return i;
    }
    return -1;
}

// Moves a body out of the hull along the shortest way. Returns the part that pushed it
// (-1 = nothing did). The rig is slow, so one pass per frame is enough.
inline int rigPushOut(float& lx, float& lz, float r) {
    int hit = -1;
    for (int pass = 0; pass < 4; pass++) {
        int i = rigHitBox(lx, lz, r);
        if (i < 0) break;
        if (hit < 0) hit = i;
        RigBox b = rigBox(i);
        float toLeft = lx - (b.x0 - r), toRight = (b.x1 + r) - lx;
        float toBack = lz - (b.z0 - r), toFront = (b.z1 + r) - lz;
        const float way[4] = {toLeft, toRight, toBack, toFront};
        int side = -1;
        for (int s = 0; s < 4; s++) {
            if ((b.exits & (1 << s)) == 0) continue;
            if (side < 0 || way[s] < way[side]) side = s;
        }
        const float skin = 0.002f;
        if (side == 0) lx = b.x0 - r - skin;
        else if (side == 1) lx = b.x1 + r + skin;
        else if (side == 2) lz = b.z0 - r - skin;
        else lz = b.z1 + r + skin;
    }
    return hit;
}

// Standing in the break room (walls excluded).
inline bool rigInCabin(float lx, float lz) {
    return fabsf(lx) < RIG_HALF_W - RIG_WALL_T && lz > RIG_CABIN_BACK && lz < RIG_CABIN_FRONT - RIG_WALL_T;
}

// On the floor plate, doorway included: whoever stands here rides along.
inline bool rigOnBoard(float lx, float lz) {
    return fabsf(lx) < RIG_HALF_W && lz > RIG_CABIN_BACK - 0.35f && lz < RIG_CABIN_FRONT;
}

// Where squad slot N stands when the rig arrives on a floor: two rows in the aisle.
inline RigPoint rigSeat(int slot) {
    if (slot < 0) slot = 0;
    return {(slot & 1) ? 0.32f : -0.28f, -1.10f - 0.95f * (float)((slot >> 1) & 1)};
}

inline float rigPointDist(float lx, float lz, RigPoint p) {
    float dx = lx - p.x, dz = lz - p.z;
    return sqrtf(dx * dx + dz * dz);
}

// Indoor panels need you indoors and the engine covers need you outside: nobody fixes a
// belt through a wall. `slack` widens the reach (the host sees remote players a little late).
inline bool rigCanFix(float lx, float lz, int fault, float slack) {
    if (fault == RIG_OK || rigInCabin(lx, lz) == rigFaultIsOutside(fault)) return false;
    return rigPointDist(lx, lz, rigFaultPoint(fault)) < RIG_REACH + slack;
}

// What the hand at (lx, lz) can use right now.
inline int rigSpotAt(float lx, float lz, int fault, bool chargerReady) {
    bool inside = rigInCabin(lx, lz);
    if (rigCanFix(lx, lz, fault, 0.0f)) return RIG_SPOT_FAULT;
    if (!inside) return RIG_SPOT_NONE;
    float toHopper = rigPointDist(lx, lz, RIG_HOPPER);
    float toCharger = rigPointDist(lx, lz, RIG_CHARGER);
    if (chargerReady && toCharger < RIG_REACH && toCharger < toHopper) return RIG_SPOT_CHARGER;
    if (toHopper < RIG_REACH) return RIG_SPOT_HOPPER;
    return RIG_SPOT_NONE;
}

// ---- lane --------------------------------------------------------------------------------
// The rig runs along one row of cells. Steps count cells from the one it started in.
inline int rigStepAt(float travelled, float localZ, float cs) {
    return (int)floorf((travelled + localZ) / cs + 0.5f);
}

inline constexpr int RIG_LANE_BACK_STEP = -1; // the break room starts out one cell behind

// Last cell that has to be open for the auger.
inline int rigFrontStep(float travelled, float cs) {
    return rigStepAt(travelled, RIG_NOSE + RIG_CARVE_LEAD, cs);
}

// ---- breakdowns --------------------------------------------------------------------------
// Engine seconds until the next fault: it holds together for about a minute up top and
// less and less the deeper it goes. roll01 spreads it by +-30%.
inline float rigFaultDelay(int level, float roll01) {
    if (level < 0) level = 0;
    float base = 78.0f - 7.0f * (float)level;
    if (base < 36.0f) base = 36.0f;
    return base * (0.7f + 0.6f * roll01);
}

inline int rigRollFault(unsigned roll) {
    return RIG_FAULT_FUSE + (int)(roll % (unsigned)(RIG_FAULT_COUNT - 1));
}

inline bool rigOnReserve(int fault, float faultSeconds) {
    return fault != RIG_OK && faultSeconds < RIG_RESERVE_SECONDS;
}

// 1 = full reserve battery, 0 = dark.
inline float rigReserveLeft(int fault, float faultSeconds) {
    if (fault == RIG_OK) return 1.0f;
    float left = 1.0f - faultSeconds / RIG_RESERVE_SECONDS;
    return left < 0.0f ? 0.0f : left;
}

// Wider ring around the haven: monsters with nobody to chase turn around here, none enter
// the floor here, no pits open here. Only a running rig has it.
inline float rigQuietRadius(int fault) {
    return fault == RIG_OK ? RIG_QUIET_RADIUS : 0.0f;
}

inline float rigHavenRadius(int fault, float faultSeconds) {
    if (fault == RIG_OK) return RIG_HAVEN_RADIUS;
    return rigOnReserve(fault, faultSeconds) ? RIG_HAVEN_RESERVE_RADIUS : 0.0f;
}
