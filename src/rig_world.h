#pragma once
// The rig in the world: where it stands when a floor starts, the lane it bores through the
// maze, and its hull as an obstacle. Included from heist_world.h.

inline constexpr float RIG_PI = 3.1415927f;

// Lane cells are a tunnel for a machine: no door frames in them, however they end up shaped.
inline bool rigDoorwayVeto(int wx, int wz) {
    int step = 0;
    return rigLaneStepOf(wx, wz, step) && step >= RIG_LANE_BACK_STEP;
}

inline int gRigCarvedStep = RIG_LANE_BACK_STEP - 1; // last lane step this peer has opened
inline Vec3 gRigPrevPos;                            // where the rig was a frame ago (rig_play.h)
inline int gRigPrevDir = -1;                        // -1 = it was just put down, nobody rides that
inline int gRigCrunch = 0;                          // walls eaten since the last look (local fx)

// Opens every lane cell up to the one the auger is about to enter. The lane only depends on
// the start cell, the heading and how far the rig got, so every peer carves the same tunnel.
// Returns true when the layout changed and the mesh has to be rebuilt.
inline bool rigCarveLane() {
    int front = rigFrontStep(rigTravelled(), CS);
    bool changed = false;
    while (gRigCarvedStep < front) {
        gRigCarvedStep++;
        int wx = 0, wz = 0;
        rigLaneCell(gRigCarvedStep, wx, wz);
        if (getCellWorld(wx, wz) == 1) gRigCrunch++;
        setCellPersistent(wx, wz, 0);
        changed = true;
    }
    return changed;
}

// Called once per floor, before anything else is placed. On the first floor the rig is parked
// two cells to the side of the portal, back door towards it; on every floor below it has just
// come down through the ceiling with the squad inside, pointing wherever the seed says.
inline void rigPlace() {
    heist.rigFault = RIG_OK;
    heist.rigFaultSecs = 0.0f;
    heist.rigCharge = 0.0f;
    if (heist.hasGarage) {
        heist.rigDir = 0;
        heist.rigStart = heistCellCenter(heist.baseX + 2, heist.baseZ);
    } else {
        std::mt19937 r(worldSeed ^ 0x0D2111u);
        heist.rigDir = (int)(r() % 4u);
        heist.rigStart = heistCellCenter(heist.baseX, heist.baseZ);
    }
    heist.drillPos = heist.rigStart;
    heist.rigLootStep = rigFrontStep(0.0f, CS);
    heist.rigNextFault = rigFaultDelay(gCurrentLevel, 0.5f) + 20.0f; // a quiet first minute
    gRigCarvedStep = RIG_LANE_BACK_STEP - 1;
    gRigPrevDir = -1;
    gDoorwayVeto = rigDoorwayVeto;
    rigCarveLane();
    gRigCrunch = 0;
}

// The rivals set up shop a little off the route, a few cells ahead: the rig passes them.
inline Vec3 rigKioskWant(std::mt19937& r) {
    float along = (float)(6 + (int)(r() % 4u)) * CS;
    float side = (float)(2 + (int)(r() % 2u)) * CS * ((r() & 1u) ? 1.0f : -1.0f);
    float dx = (float)rigDirX(heist.rigDir), dz = (float)rigDirZ(heist.rigDir);
    return Vec3(heist.rigStart.x + dx * along + dz * side, 0.0f, heist.rigStart.z + dz * along - dx * side);
}

// Offset of a cell from the rig's start, along the lane and across it, in cells.
inline void rigCellOffset(int wx, int wz, int& along, int& across) {
    int ox = wx - (int)floorf(heist.rigStart.x / CS), oz = wz - (int)floorf(heist.rigStart.z / CS);
    int dx = rigDirX(heist.rigDir), dz = rigDirZ(heist.rigDir);
    along = ox * dx + oz * dz;
    across = ox * dz - oz * dx;
}

// Puts the local player into the break room, facing the door.
inline void rigSeatPlayer(int slot) {
    Vec3 at = rigWorld(rigSeat(slot));
    cam.pos = Vec3(at.x, PH, at.z);
    cam.yaw = rigYaw(heist.rigDir) + RIG_PI;
    cam.pitch = 0.0f;
}

inline bool rigCollide(float x, float z, float r) {
    RigPoint p = rigLocal(Vec3(x, 0.0f, z));
    return rigHitBox(p.x, p.z, r) >= 0;
}
