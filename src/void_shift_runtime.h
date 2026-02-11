#pragma once

#include <cstdio>

inline void clampVoidShiftValues() {
    if (resonatorBattery < 0.0f) resonatorBattery = 0.0f;
    if (resonatorBattery > 100.0f) resonatorBattery = 100.0f;
    if (attentionLevel < 0.0f) attentionLevel = 0.0f;
    if (attentionLevel > 100.0f) attentionLevel = 100.0f;
    if (coLevel < 0.0f) coLevel = 0.0f;
    if (coLevel > 100.0f) coLevel = 100.0f;
}

inline void addAttention(float amount) {
    attentionLevel += amount;
    clampVoidShiftValues();
}

inline void resetVoidShiftState(const Vec3& spawnPos, const Vec3& exitDoorPos) {
    resonatorMode = RESONATOR_SCAN;
    resonatorBattery = 100.0f;
    attentionLevel = 0.0f;
    attentionEventCooldown = 0.0f;
    coLevel = 0.0f;
    ventilationOnline = false;
    echoRecording = false;
    echoPlayback = false;
    echoRecordTimer = 0.0f;
    echoTrack.clear();
    echoPlaybackIndex = 0;
    echoGhostPos = spawnPos;
    echoGhostActive = false;

    for (int i = 0; i < 3; i++) level1NodeDone[i] = false;
    level1HoldActive = false;
    level1HoldTimer = 90.0f;
    level1ContractComplete = false;
    level1Nodes[0] = Vec3(spawnPos.x + CS * 4.0f, 0.0f, spawnPos.z + CS * 1.0f);
    level1Nodes[1] = Vec3(spawnPos.x - CS * 3.0f, 0.0f, spawnPos.z + CS * 5.0f);
    level1Nodes[2] = Vec3(spawnPos.x + CS * 2.0f, 0.0f, spawnPos.z - CS * 4.0f);

    level2BatteryInstalled = false;
    level2FuseCount = 0;
    level2AccessReady = false;
    level2HoldActive = false;
    level2HoldTimer = 15.0f;
    level2ContractComplete = false;
    for (int i = 0; i < 3; i++) level2FuseDone[i] = false;
    level2BatteryNode = Vec3(spawnPos.x + CS * 6.0f, 0.0f, spawnPos.z - CS * 2.0f);
    level2FuseNodes[0] = Vec3(spawnPos.x - CS * 2.0f, 0.0f, spawnPos.z + CS * 6.0f);
    level2FuseNodes[1] = Vec3(spawnPos.x + CS * 7.0f, 0.0f, spawnPos.z + CS * 4.0f);
    level2FuseNodes[2] = Vec3(spawnPos.x - CS * 6.0f, 0.0f, spawnPos.z - CS * 1.0f);
    level2AccessNode = Vec3(spawnPos.x + CS * 1.0f, 0.0f, spawnPos.z - CS * 7.0f);
    level2LiftNode = exitDoorPos;
}

inline void startEchoRecordingTrack() {
    if (echoPlayback) return;
    echoTrack.clear();
    echoRecording = true;
    echoRecordTimer = 0.0f;
    echoGhostActive = false;
    setEchoStatus("ECHO RECORDING START");
}

inline void stopEchoRecordingTrack() {
    if (!echoRecording) return;
    echoRecording = false;
    if (echoTrack.empty()) setEchoStatus("ECHO EMPTY");
    else setEchoStatus("ECHO RECORDING SAVED");
}

inline void startEchoPlaybackTrack() {
    if (echoRecording || echoTrack.empty()) {
        if (echoTrack.empty()) setEchoStatus("NO ECHO TRACK");
        return;
    }
    echoPlayback = true;
    echoGhostActive = true;
    echoPlaybackIndex = 0;
    echoGhostPos = echoTrack[0];
    resonatorBattery -= 12.0f;
    addAttention(10.0f);
    clampVoidShiftValues();
    setEchoStatus("ECHO PLAYBACK START");
}

inline int level1DoneCount() {
    int done = 0;
    for (int i = 0; i < 3; i++) if (level1NodeDone[i]) done++;
    return done;
}

inline bool isVoidShiftExitReady() {
    if (isLevelZero(gCurrentLevel)) return level1ContractComplete;
    return level2ContractComplete;
}

inline void buildVoidShiftObjectiveLine(char* out, int outSize) {
    if (!out || outSize < 2) return;
    if (isLevelZero(gCurrentLevel)) {
        if (!level1HoldActive && !level1ContractComplete) {
            std::snprintf(out, outSize, "CONTRACT L1: STABILIZE NODES %d/3", level1DoneCount());
            return;
        }
        if (level1HoldActive) {
            std::snprintf(out, outSize, "CONTRACT L1: HOLD STABILIZER %.0fs", level1HoldTimer);
            return;
        }
        std::snprintf(out, outSize, "CONTRACT L1 COMPLETE: EXIT OPEN");
        return;
    }

    if (!level2BatteryInstalled || level2FuseCount < 3 || !level2AccessReady) {
        std::snprintf(out, outSize, "CONTRACT L2: BAT %d FUSE %d/3 ACCESS %d", level2BatteryInstalled ? 1 : 0, level2FuseCount, level2AccessReady ? 1 : 0);
        return;
    }
    if (level2HoldActive) {
        std::snprintf(out, outSize, "CONTRACT L2: HOLD LIFT %.0fs", level2HoldTimer);
        return;
    }
    std::snprintf(out, outSize, "CONTRACT L2 COMPLETE: LIFT READY");
}

inline bool tryHandleVoidShiftInteract(const Vec3& playerPos) {
    if (isLevelZero(gCurrentLevel)) {
        if (level1ContractComplete) return false;
        for (int i = 0; i < 3; i++) {
            if (level1NodeDone[i]) continue;
            if (!nearPoint2D(playerPos, level1Nodes[i], 2.3f)) continue;
            level1NodeDone[i] = true;
            addAttention(15.0f);
            setTrapStatus("NODE STABILIZED");
            if (level1DoneCount() >= 3) {
                level1HoldActive = true;
                level1HoldTimer = 90.0f;
                setTrapStatus("FINAL PHASE: HOLD 90s");
            }
            return true;
        }
        return false;
    }

    if (!level2BatteryInstalled && nearPoint2D(playerPos, level2BatteryNode, 2.4f)) {
        level2BatteryInstalled = true;
        addAttention(12.0f);
        setTrapStatus("TRACTION BATTERY INSTALLED");
        return true;
    }
    for (int i = 0; i < 3; i++) {
        if (level2FuseDone[i]) continue;
        if (!nearPoint2D(playerPos, level2FuseNodes[i], 2.4f)) continue;
        level2FuseDone[i] = true;
        level2FuseCount++;
        addAttention(8.0f);
        setTrapStatus("LIFT FUSE INSTALLED");
        return true;
    }
    if (!level2AccessReady && nearPoint2D(playerPos, level2AccessNode, 2.4f)) {
        level2AccessReady = true;
        addAttention(10.0f);
        setTrapStatus("SECURITY ACCESS ACCEPTED");
        return true;
    }
    if (!level2HoldActive && !level2ContractComplete && level2BatteryInstalled && level2FuseCount >= 3 && level2AccessReady && nearPoint2D(playerPos, level2LiftNode, 2.6f)) {
        level2HoldActive = true;
        level2HoldTimer = 15.0f;
        addAttention(15.0f);
        setTrapStatus("LIFT HOLD STARTED");
        return true;
    }
    return false;
}

inline void updateVoidShiftSystems(float dt, bool sprinting, bool flashlightActive) {
    if (attentionEventCooldown > 0.0f) {
        attentionEventCooldown -= dt;
        if (attentionEventCooldown < 0.0f) attentionEventCooldown = 0.0f;
    }

    if (sprinting) addAttention(dt * 1.1f);
    if (flashlightActive) addAttention(dt * 0.35f);
    if (!echoPlayback) addAttention(-dt * 1.4f);

    if (echoRecording) {
        echoRecordTimer += dt;
        resonatorBattery -= dt * 1.5f;
        if ((int)echoTrack.size() == 0 || echoRecordTimer >= 0.10f) {
            echoTrack.push_back(cam.pos);
            echoRecordTimer = 0.0f;
        }
        if ((int)echoTrack.size() >= 250) stopEchoRecordingTrack();
        addAttention(dt * 0.9f);
    }

    if (echoPlayback) {
        if (echoPlaybackIndex < (int)echoTrack.size()) {
            echoGhostPos = echoTrack[echoPlaybackIndex++];
            echoGhostActive = true;
        } else {
            echoPlayback = false;
            echoGhostActive = false;
            setEchoStatus("ECHO PLAYBACK COMPLETE");
        }
    }

    if (isParkingLevel(gCurrentLevel)) {
        coLevel += dt * (ventilationOnline ? -3.2f : 2.6f);
        float coDamage = coLevel / 100.0f;
        playerStamina -= dt * coDamage * 4.0f;
        if (playerStamina < 0.0f) playerStamina = 0.0f;
    } else {
        coLevel = 0.0f;
    }

    if (level1HoldActive && !level1ContractComplete) {
        level1HoldTimer -= dt;
        if (level1HoldTimer <= 0.0f) {
            level1HoldTimer = 0.0f;
            level1HoldActive = false;
            level1ContractComplete = true;
            setTrapStatus("LEVEL 1 CONTRACT COMPLETE");
        }
    }

    if (level2HoldActive && !level2ContractComplete) {
        level2HoldTimer -= dt;
        if (level2HoldTimer <= 0.0f) {
            level2HoldTimer = 0.0f;
            level2HoldActive = false;
            level2ContractComplete = true;
            setTrapStatus("LEVEL 2 CONTRACT COMPLETE");
        }
    }

    if (attentionEventCooldown <= 0.0f) {
        if (attentionLevel >= 90.0f) {
            setTrapStatus("ATTENTION CRITICAL");
            attentionEventCooldown = 12.0f;
        } else if (attentionLevel >= 75.0f) {
            setTrapStatus("ATTENTION HIGH");
            attentionEventCooldown = 10.0f;
        } else if (attentionLevel >= 50.0f) {
            setTrapStatus("ATTENTION RISING");
            attentionEventCooldown = 8.0f;
        } else if (attentionLevel >= 25.0f) {
            setTrapStatus("ATTENTION DETECTED");
            attentionEventCooldown = 6.0f;
        }
    }

    clampVoidShiftValues();
}
