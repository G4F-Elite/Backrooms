#pragma once

#include <cstdio>

inline bool processLevel2SideTechStep(const Vec3& playerPos) {
    if (!level2CameraOnline && nearPoint2D(playerPos, level2CameraNode, 2.2f)) {
        level2CameraOnline = true;
        setTrapStatus("CAMERA NETWORK ONLINE");
        awardArchivePoints(12, "CAMERA RESTORE COMPLETE");
        if (sideContractType == SIDE_RESTORE_CAMERAS) progressSideContract(1, "SIDE: CAMERAS RESTORED");
        return true;
    }
    if (!level2DroneReprogrammed && nearPoint2D(playerPos, level2DroneNode, 2.2f)) {
        level2DroneReprogrammed = true;
        level2DroneAssistTimer = 50.0f;
        setTrapStatus("SECURITY DRONE REPROGRAMMED");
        awardArchivePoints(14, "DRONE REPROGRAM COMPLETE");
        if (sideContractType == SIDE_REPROGRAM_DRONE) progressSideContract(1, "SIDE: DRONE REPROGRAMMED");
        return true;
    }
    return false;
}

inline bool buildLevel2SideTechPrompt(const Vec3& playerPos, char* out, int outSize) {
    if (!out || outSize < 2) return false;
    if (!level2CameraOnline && nearPoint2D(playerPos, level2CameraNode, 2.2f)) {
        std::snprintf(out, outSize, "[E] RESTORE CAMERA GRID");
        return true;
    }
    if (!level2DroneReprogrammed && nearPoint2D(playerPos, level2DroneNode, 2.2f)) {
        std::snprintf(out, outSize, "[E] REPROGRAM SECURITY DRONE");
        return true;
    }
    return false;
}

inline void updateLevel2DroneAssist(float dt) {
    if (!level2DroneReprogrammed || level2DroneAssistTimer <= 0.0f) return;
    level2DroneAssistTimer -= dt;
    if (level2DroneAssistTimer < 0.0f) level2DroneAssistTimer = 0.0f;
    if (((int)(level2DroneAssistTimer * 10.0f) % 70) == 0) {
        setEchoStatus("DRONE: ROUTE MARKER ACTIVE");
        addAttention(-0.4f);
    }
}
