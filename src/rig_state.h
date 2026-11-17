#pragma once
// Rig state queries on top of the shared heist state: where the rig is, whether it is
// crawling, how much of a haven it is right now and which cells belong to its lane.
// Included at the end of heist_state.h.

inline RigFrame rigFrame() {
    RigFrame f;
    f.x = heist.drillPos.x;
    f.z = heist.drillPos.z;
    f.dir = heist.rigDir;
    return f;
}

inline Vec3 rigWorld(float lx, float lz, float y = 0.0f) {
    float wx = 0.0f, wz = 0.0f;
    rigToWorld(rigFrame(), lx, lz, wx, wz);
    return Vec3(wx, y, wz);
}

inline Vec3 rigWorld(RigPoint p, float y = 0.0f) { return rigWorld(p.x, p.z, y); }

inline RigPoint rigLocal(const Vec3& pos) {
    RigPoint p = {0.0f, 0.0f};
    rigToLocal(rigFrame(), pos.x, pos.z, p.x, p.z);
    return p;
}

inline bool rigPosInCabin(const Vec3& pos) {
    RigPoint p = rigLocal(pos);
    return rigInCabin(p.x, p.z);
}

// In the break room, within arm's reach of the hopper and the lever (host-side check).
inline bool rigPosAtHopper(const Vec3& pos) {
    RigPoint p = rigLocal(pos);
    return rigInCabin(p.x, p.z) && rigPointDist(p.x, p.z, RIG_HOPPER) < RIG_REACH + 0.4f;
}

// Metres crawled since the rig arrived on this floor.
inline float rigTravelled() {
    Vec3 d = heist.drillPos - heist.rigStart;
    return d.x * (float)rigDirX(heist.rigDir) + d.z * (float)rigDirZ(heist.rigDir);
}

// The engine only pulls while the run is on, nothing is broken and the drill is still hungry.
// Once the quota is met it parks and waits for somebody to pull the lever.
inline bool rigMoving() {
    return heist.stage == HEIST_RUN && heist.rigFault == RIG_OK && heist.quotaHave < heist.quotaNeed;
}

inline bool rigEngineOn() {
    return heist.rigFault == RIG_OK && (heist.stage == HEIST_RUN || heist.stage == HEIST_DRILLING);
}

inline float rigHavenNow() {
    if (!heistDangerActive()) return 0.0f;
    return rigHavenRadius(heist.rigFault, heist.rigFaultSecs);
}

inline Vec3 rigHavenPos() { return rigWorld(0.0f, RIG_HAVEN_Z); }

inline bool rigPosInHaven(const Vec3& pos) {
    float r = rigHavenNow();
    if (r <= 0.0f) return false;
    Vec3 d = pos - rigHavenPos();
    return d.x * d.x + d.z * d.z < r * r;
}

inline float rigQuietNow() {
    return heistDangerActive() ? rigQuietRadius(heist.rigFault) : 0.0f;
}

inline bool rigPosInQuiet(const Vec3& pos) {
    float r = rigQuietNow();
    Vec3 d = pos - rigHavenPos();
    return r > 0.0f && d.x * d.x + d.z * d.z < r * r;
}

inline bool rigLampLit() {
    if (heist.rigFault == RIG_OK) return true;
    if (!rigOnReserve(heist.rigFault, heist.rigFaultSecs)) return false;
    // the reserve battery stutters more and more as it runs down
    float left = rigReserveLeft(heist.rigFault, heist.rigFaultSecs);
    return sinf(heist.rigFaultSecs * (9.0f + 14.0f * (1.0f - left))) > -0.55f - 0.4f * left;
}

inline bool rigChargerReady() { return heist.rigCharge >= 1.0f; }

// ---- lane --------------------------------------------------------------------------------
inline void rigLaneCell(int step, int& wx, int& wz) {
    wx = (int)floorf(heist.rigStart.x / CS) + rigDirX(heist.rigDir) * step;
    wz = (int)floorf(heist.rigStart.z / CS) + rigDirZ(heist.rigDir) * step;
}

// Step of a lane cell, or false when the cell is not on the rig's row.
inline bool rigLaneStepOf(int wx, int wz, int& step) {
    int sx = (int)floorf(heist.rigStart.x / CS), sz = (int)floorf(heist.rigStart.z / CS);
    int dx = rigDirX(heist.rigDir), dz = rigDirZ(heist.rigDir);
    int ox = wx - sx, oz = wz - sz;
    if (ox * dz - oz * dx != 0) return false; // off the row
    step = ox * dx + oz * dz;
    return true;
}

inline constexpr int RIG_LANE_KEEP_CLEAR = 4; // cells ahead of the auger that props and pits stay out of

// The stretch of lane the rig stands on or is about to chew through.
inline bool rigIsLaneCell(int wx, int wz, int ahead) {
    int step = 0;
    if (!rigLaneStepOf(wx, wz, step)) return false;
    return step >= RIG_LANE_BACK_STEP && step <= rigFrontStep(rigTravelled(), CS) + ahead;
}
