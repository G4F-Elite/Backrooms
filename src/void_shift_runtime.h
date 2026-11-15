#pragma once

#include <cstdio>

inline void clampVoidShiftValues() {
    if (resonatorBattery < 0.0f) resonatorBattery = 0.0f;
    if (resonatorBattery > 100.0f) resonatorBattery = 100.0f;
    if (attentionLevel < 0.0f) attentionLevel = 0.0f;
    if (attentionLevel > 100.0f) attentionLevel = 100.0f;
}

inline void addAttention(float amount) {
    attentionLevel += amount;
    clampVoidShiftValues();
}

inline void awardArchivePoints(int amount, const char* reason) {
    if (amount <= 0) return;
    archivePoints += amount;
    int newTier = archivePoints / 120;
    if (newTier > archiveTier) {
        archiveTier = newTier;
        if (archiveTier >= 1) perkQuietSteps = true;
        if (archiveTier >= 2) perkFastHands = true;
        if (archiveTier >= 3) perkEchoBuffer = true;
        setEchoStatus("ARCHIVE TIER UNLOCKED");
    } else if (reason) {
        setEchoStatus(reason);
    }
    saveArchiveMetaProgress(archivePoints, archiveTier, perkQuietSteps, perkFastHands, perkEchoBuffer);
}

inline const char* sideContractName(int type) {
    if (type == SIDE_COLLECT_BADGES) return "COLLECT BADGES";
    if (type == SIDE_SCAN_WALLS) return "SCAN WALL PATTERNS";
    if (type == SIDE_STABILIZE_RIFTS) return "SEAL RESONANCE RIFTS";
    if (type == SIDE_RESCUE_SURVIVOR) return "RESCUE LOST SURVIVOR";
    return "NO SIDE CONTRACT";
}

inline void initSideContractForLevel() {
    sideContractProgress = 0;
    sideContractCompleted = false;
    int roll = (int)(rng() % 4);
    sideContractType = roll == 0 ? SIDE_COLLECT_BADGES : (roll == 1 ? SIDE_SCAN_WALLS : (roll == 2 ? SIDE_STABILIZE_RIFTS : SIDE_RESCUE_SURVIVOR));
    if (sideContractType == SIDE_STABILIZE_RIFTS && !isLevelZero(gCurrentLevel)) sideContractType = SIDE_COLLECT_BADGES;
    if (sideContractType == SIDE_RESCUE_SURVIVOR && !npcLostSurvivorActive) sideContractType = SIDE_SCAN_WALLS;
    // Wall scans are logged by squad pings, which only exist in co-op.
    if (sideContractType == SIDE_SCAN_WALLS && multiState != MULTI_IN_GAME) sideContractType = SIDE_COLLECT_BADGES;
    sideContractTarget = sideContractType == SIDE_COLLECT_BADGES ? 6 : (sideContractType == SIDE_SCAN_WALLS ? 3 : (sideContractType == SIDE_STABILIZE_RIFTS ? 2 : 1));
}

inline void progressSideContract(int amount, const char* statusMsg) {
    if (sideContractType == SIDE_NONE || sideContractCompleted || amount <= 0) return;
    sideContractProgress += amount;
    if (sideContractProgress >= sideContractTarget) {
        sideContractProgress = sideContractTarget;
        sideContractCompleted = true;
        awardArchivePoints(35, "SIDE CONTRACT COMPLETE");
        addAttention(-8.0f);
    } else if (statusMsg) {
        setTrapStatus(statusMsg);
    }
}

inline void initVoidShiftNpcSpots(const Vec3& spawnPos, const Vec3& exitDoorPos) {
    npcLostSurvivorPos = Vec3(spawnPos.x + CS * 8.0f, 0.0f, spawnPos.z - CS * 3.0f);
    if (!isLevelZero(gCurrentLevel)) {
        npcLostSurvivorPos = Vec3(spawnPos.x - CS * 7.0f, 0.0f, spawnPos.z + CS * 5.0f);
    }
    npcLostSurvivorActive = ((rng() % 100) < 45);
    npcLostSurvivorEscorted = false;
    (void)exitDoorPos;
}

inline void resetVoidShiftState(const Vec3& spawnPos, const Vec3& exitDoorPos) {
    resonatorMode = RESONATOR_SCAN;
    resonatorBattery = 100.0f;
    attentionLevel = 0.0f;
    attentionEventCooldown = 0.0f;
    echoRecording = false;
    echoPlayback = false;
    echoRecordTimer = 0.0f;
    echoTrack.clear();
    echoPlaybackIndex = 0;
    echoGhostPos = spawnPos;
    echoGhostActive = false;

    for (int i = 0; i < 3; i++) level1NodeDone[i] = false;
    initLevel1PuzzleStages();
    level1HoldActive = false;
    level1HoldTimer = 90.0f;
    level1ContractComplete = false;
    level1Nodes[0] = Vec3(spawnPos.x + CS * 4.0f, 0.0f, spawnPos.z + CS * 1.0f);
    level1Nodes[1] = Vec3(spawnPos.x - CS * 3.0f, 0.0f, spawnPos.z + CS * 5.0f);
    level1Nodes[2] = Vec3(spawnPos.x + CS * 2.0f, 0.0f, spawnPos.z - CS * 4.0f);

    level2BatteryInstalled = false;
    level2BatteryStage = 0;
    level2FuseCount = 0;
    level2AccessReady = false;
    level2FusePanelPowered = false;
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
    level2PowerNode = Vec3(spawnPos.x + CS * 5.0f, 0.0f, spawnPos.z + CS * 1.0f);

    initVoidShiftNpcSpots(spawnPos, exitDoorPos);
    initSideContractForLevel(); initLevel2PuzzleStages(); initVoidShiftSetpieces();
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

inline void notifyVoidShiftItemPickup(int itemType) {
    if (sideContractCompleted) return;
    if (sideContractType == SIDE_COLLECT_BADGES && itemType == ITEM_BATTERY) {
        progressSideContract(1, "SIDE: BADGE CACHE FOUND");
    }
}

inline void notifyVoidShiftPingUsed() {
    if (sideContractCompleted) return;
    if (sideContractType == SIDE_SCAN_WALLS) progressSideContract(1, "SIDE: WALL PATTERN LOGGED");
}

inline void notifyVoidShiftRiftSealed() {
    if (sideContractCompleted) return;
    if (sideContractType == SIDE_STABILIZE_RIFTS) progressSideContract(1, "SIDE: RIFT SEALED");
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

inline bool getVoidShiftResonanceTarget(Vec3& outPos) {
    if (npcLostSurvivorActive && !npcLostSurvivorEscorted) {
        outPos = npcLostSurvivorPos;
        return true;
    }
    if (isLevelZero(gCurrentLevel)) {
        if (level1HoldActive || level1ContractComplete) {
            outPos = coop.doorPos;
            return true;
        }
        for (int i = 0; i < 3; i++) {
            if (level1NodeDone[i]) continue;
            outPos = level1Nodes[i];
            return true;
        }
        return false;
    }

    if (!level2BatteryInstalled) {
        outPos = level2BatteryNode;
        return true;
    }
    for (int i = 0; i < 3; i++) {
        if (level2FuseDone[i]) continue;
        outPos = level2FuseNodes[i];
        return true;
    }
    if (!level2AccessReady) {
        outPos = level2AccessNode;
        return true;
    }
    if (!level2HoldActive && !level2ContractComplete) {
        outPos = level2LiftNode;
        return true;
    }
    outPos = level2LiftNode;
    return true;
}

inline void buildVoidShiftInteractPrompt(const Vec3& playerPos, char* out, int outSize) {
    if (!out || outSize < 2) return;
    out[0] = '\0';

    if (npcLostSurvivorActive && !npcLostSurvivorEscorted && nearPoint2D(playerPos, npcLostSurvivorPos, 2.2f)) {
        std::snprintf(out, outSize, "[E] ESCORT LOST SURVIVOR");
        return;
    }

    if (isLevelZero(gCurrentLevel)) {
        for (int i = 0; i < 3; i++) {
            if (level1NodeDone[i]) continue;
            if (nearPoint2D(playerPos, level1Nodes[i], 2.3f)) {
                buildLevel1NodeActionPrompt(i, out, outSize);
                return;
            }
        }
        if (level1HoldActive && nearPoint2D(playerPos, coop.doorPos, 2.5f)) {
            std::snprintf(out, outSize, "[E] EXIT TO PARKING");
            return;
        }
        return;
    }

    if (buildLevel2ActionPrompt(playerPos, out, outSize)) return;
    for (int i = 0; i < 3; i++) {
        if (level2FuseDone[i]) continue;
        if (nearPoint2D(playerPos, level2FuseNodes[i], 2.4f)) {
            if (!level2FusePanelPowered) {
                std::snprintf(out, outSize, "[E] FUSE LOCKED: POWER PANEL FIRST");
                return;
            }
            std::snprintf(out, outSize, "[E] INSTALL LIFT FUSE");
            return;
        }
    }
    if (!level2AccessReady && nearPoint2D(playerPos, level2AccessNode, 2.4f)) {
        std::snprintf(out, outSize, "[E] AUTHORIZE SECURITY ACCESS");
        return;
    }
    if (!level2HoldActive && !level2ContractComplete && level2BatteryInstalled && level2FuseCount >= 3 && level2AccessReady && nearPoint2D(playerPos, level2LiftNode, 2.6f)) {
        std::snprintf(out, outSize, "[E] START LIFT HOLD");
    }
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
        std::snprintf(out, outSize, "CONTRACT L2: BAT %d(%d) FUSE %d/3 PWR %d ACCESS %d", level2BatteryInstalled ? 1 : 0, level2BatteryStage, level2FuseCount, level2FusePanelPowered ? 1 : 0, level2AccessReady ? 1 : 0);
        return;
    }
    if (level2HoldActive) {
        std::snprintf(out, outSize, "CONTRACT L2: HOLD LIFT %.0fs", level2HoldTimer);
        return;
    }
    std::snprintf(out, outSize, "CONTRACT L2 COMPLETE: LIFT READY");
}

inline void buildVoidShiftSupportLine(char* out, int outSize) {
    if (!out || outSize < 2) return;
    if (sideContractType != SIDE_NONE && !sideContractCompleted) {
        std::snprintf(out, outSize, "SIDE: %s %d/%d", sideContractName(sideContractType), sideContractProgress, sideContractTarget);
        return;
    }
    if (sideContractCompleted) {
        std::snprintf(out, outSize, "SIDE COMPLETE +35  ARCHIVE %d T%d", archivePoints, archiveTier);
        return;
    }
    std::snprintf(out, outSize, "ARCHIVE %d T%d", archivePoints, archiveTier);
}

inline bool tryHandleVoidShiftInteract(const Vec3& playerPos) {
    if (npcLostSurvivorActive && !npcLostSurvivorEscorted && nearPoint2D(playerPos, npcLostSurvivorPos, 2.2f)) {
        npcLostSurvivorEscorted = true;
        npcLostSurvivorActive = false;
        awardArchivePoints(30, "SURVIVOR EVACUATED");
        if (sideContractType == SIDE_RESCUE_SURVIVOR) {
            progressSideContract(sideContractTarget, "SIDE: SURVIVOR SAFE");
        }
        return true;
    }

    if (isLevelZero(gCurrentLevel)) {
        if (level1ContractComplete) return false;
        for (int i = 0; i < 3; i++) {
            if (level1NodeDone[i]) continue;
            if (!nearPoint2D(playerPos, level1Nodes[i], 2.3f)) continue;
            if (!processLevel1NodeStage(i)) return false;
            if (!level1NodeDone[i]) return true;
            addAttention(15.0f);
            if (sideContractType == SIDE_STABILIZE_RIFTS) progressSideContract(1, "SIDE: RIFT SEALED");
            if (level1DoneCount() >= 3) {
                level1HoldActive = true;
                level1HoldTimer = 90.0f;
                setTrapStatus("FINAL PHASE: HOLD 90s");
            }
            return true;
        }
        return false;
    }

    if (processLevel2Step(playerPos)) return true;
    for (int i = 0; i < 3; i++) {
        if (level2FuseDone[i]) continue;
        if (!nearPoint2D(playerPos, level2FuseNodes[i], 2.4f)) continue;
        if (!level2FusePanelPowered) {
            setTrapStatus("FUSE PANEL UNPOWERED");
            return true;
        }
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

    float sprintAttention = perkQuietSteps ? 0.75f : 1.1f;
    if (sprinting) addAttention(dt * sprintAttention);
    if (flashlightActive) addAttention(dt * 0.35f);
    if (!echoPlayback) addAttention(-dt * 1.4f);
    if (perkFastHands && !sprinting) addAttention(-dt * 0.3f);

    if (echoRecording) {
        echoRecordTimer += dt;
        float echoDrain = perkEchoBuffer ? 1.0f : 1.5f;
        resonatorBattery -= dt * echoDrain;
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

    updateLevel1SyncSwitchProgress(dt); updateVoidShiftSetpieces(dt);

    updateVoidShiftEnemyEffects(dt);

    if (npcLostSurvivorActive && !npcLostSurvivorEscorted) {
        Vec3 toDoor = coop.doorPos - npcLostSurvivorPos;
        toDoor.y = 0.0f;
        float dist = toDoor.len();
        if (dist > 0.3f) {
            float inv = 1.0f / dist;
            npcLostSurvivorPos.x += toDoor.x * inv * dt * 1.3f;
            npcLostSurvivorPos.z += toDoor.z * inv * dt * 1.3f;
        }
        if (nearPoint2D(npcLostSurvivorPos, coop.doorPos, 2.3f)) {
            npcLostSurvivorEscorted = true;
            npcLostSurvivorActive = false;
            awardArchivePoints(30, "SURVIVOR SAFE IN BREAKROOM");
        }
    }

    updateVoidShiftHoldPhases(dt);

    if (attentionEventCooldown <= 0.0f) {
        if (attentionLevel >= 90.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "CRITICAL: WANDERER HUNT ACTIVE" : "CRITICAL: TOWMAN TRACKING ACTIVE");
            if (multiState != MULTI_IN_GAME) {
                Vec3 ep = findSpawnPos(cam.pos, 12.0f);
                entityMgr.spawnEntity(ENTITY_SHADOW, ep, nullptr, 0, 0);
            }
            attentionEventCooldown = 12.0f;
        } else if (attentionLevel >= 75.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "HIGH: LAMP-HUSHER SWARM" : "HIGH: HEADLIGHT SWARM");
            if (isLevelZero(gCurrentLevel)) { lightsOutTimer = 18.0f; triggerBlackoutSetpiece(10.0f); }
            else { falseDoorTimer = 14.0f; triggerCorridorShiftSetpiece(12.0f); }
            if (multiState != MULTI_IN_GAME) {
                Vec3 ep = findSpawnPos(cam.pos, 10.0f);
                entityMgr.spawnEntity(ENTITY_CRAWLER, ep, nullptr, 0, 0);
            }
            attentionEventCooldown = 10.0f;
        } else if (attentionLevel >= 50.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "ATTENTION RISING: PHONE MIMIC RISK" : "ATTENTION RISING: DRONE ALERT");
            if (isLevelZero(gCurrentLevel)) { falseDoorTimer = 10.0f; triggerConferenceCallSetpiece(14.0f); }
            attentionEventCooldown = 8.0f;
        } else if (attentionLevel >= 25.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "ATTENTION: PAPERCLIP NOISE" : "ATTENTION: OIL TRACKS DETECTED");
            attentionEventCooldown = 6.0f;
        }
    }

    clampVoidShiftValues();
}
