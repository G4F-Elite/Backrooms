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
    if (type == SIDE_ENABLE_VENT) return "ENABLE PARKING VENT";
    if (type == SIDE_RESTORE_CAMERAS) return "RESTORE CAMERAS";
    if (type == SIDE_REPROGRAM_DRONE) return "REPROGRAM DRONE";
    return "NO SIDE CONTRACT";
}

inline void initSideContractForLevel() {
    sideContractProgress = 0;
    sideContractCompleted = false;
    if (isLevelZero(gCurrentLevel)) {
        int roll = (int)(rng() % 3);
        sideContractType = roll == 0 ? SIDE_COLLECT_BADGES : (roll == 1 ? SIDE_SCAN_WALLS : SIDE_STABILIZE_RIFTS);
        sideContractTarget = sideContractType == SIDE_COLLECT_BADGES ? 6 : (sideContractType == SIDE_SCAN_WALLS ? 3 : 2);
    } else {
        int roll = (int)(rng() % 3);
        sideContractType = roll == 0 ? SIDE_ENABLE_VENT : (roll == 1 ? SIDE_RESTORE_CAMERAS : SIDE_REPROGRAM_DRONE);
        sideContractTarget = sideContractType == SIDE_ENABLE_VENT ? 1 : (sideContractType == SIDE_RESTORE_CAMERAS ? 1 : 1);
    }
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
    npcCartographerPos = Vec3(spawnPos.x + CS * 1.0f, 0.0f, spawnPos.z + CS * 2.0f);
    npcDispatcherPhonePos = Vec3(spawnPos.x - CS * 2.0f, 0.0f, spawnPos.z + CS * 3.0f);
    npcLostSurvivorPos = Vec3(spawnPos.x + CS * 8.0f, 0.0f, spawnPos.z - CS * 3.0f);
    if (!isLevelZero(gCurrentLevel)) {
        npcCartographerPos = Vec3(spawnPos.x - CS * 2.0f, 0.0f, spawnPos.z + CS * 1.0f);
        npcDispatcherPhonePos = Vec3(spawnPos.x + CS * 3.0f, 0.0f, spawnPos.z - CS * 2.0f);
        npcLostSurvivorPos = Vec3(spawnPos.x - CS * 7.0f, 0.0f, spawnPos.z + CS * 5.0f);
    }
    npcCartographerActive = true;
    npcDispatcherActive = true;
    npcLostSurvivorActive = ((rng() % 100) < 45);
    npcLostSurvivorEscorted = false;
    dispatcherCallCooldown = 8.0f;
    (void)exitDoorPos;
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
    initLevel1PuzzleStages();
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
    level2VentDone = false;
    for (int i = 0; i < 3; i++) level2FuseDone[i] = false;
    level2BatteryNode = Vec3(spawnPos.x + CS * 6.0f, 0.0f, spawnPos.z - CS * 2.0f);
    level2FuseNodes[0] = Vec3(spawnPos.x - CS * 2.0f, 0.0f, spawnPos.z + CS * 6.0f);
    level2FuseNodes[1] = Vec3(spawnPos.x + CS * 7.0f, 0.0f, spawnPos.z + CS * 4.0f);
    level2FuseNodes[2] = Vec3(spawnPos.x - CS * 6.0f, 0.0f, spawnPos.z - CS * 1.0f);
    level2AccessNode = Vec3(spawnPos.x + CS * 1.0f, 0.0f, spawnPos.z - CS * 7.0f);
    level2VentNode = Vec3(spawnPos.x - CS * 7.0f, 0.0f, spawnPos.z + CS * 2.0f);
    level2LiftNode = exitDoorPos;

    initSideContractForLevel();
    initVoidShiftNpcSpots(spawnPos, exitDoorPos);
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

    if (npcCartographerActive && nearPoint2D(playerPos, npcCartographerPos, 2.2f)) {
        std::snprintf(out, outSize, "[E] TRADE WITH CARTOGRAPHER");
        return;
    }
    if (npcDispatcherActive && nearPoint2D(playerPos, npcDispatcherPhonePos, 2.2f)) {
        std::snprintf(out, outSize, "[E] ANSWER DISPATCH CALL");
        return;
    }
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

    if (!level2BatteryInstalled && nearPoint2D(playerPos, level2BatteryNode, 2.4f)) {
        std::snprintf(out, outSize, "[E] INSTALL TRACTION BATTERY");
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (level2FuseDone[i]) continue;
        if (nearPoint2D(playerPos, level2FuseNodes[i], 2.4f)) {
            std::snprintf(out, outSize, "[E] INSTALL LIFT FUSE");
            return;
        }
    }
    if (!level2AccessReady && nearPoint2D(playerPos, level2AccessNode, 2.4f)) {
        std::snprintf(out, outSize, "[E] AUTHORIZE SECURITY ACCESS");
        return;
    }
    if (!level2VentDone && nearPoint2D(playerPos, level2VentNode, 2.4f)) {
        std::snprintf(out, outSize, "[E] ENABLE VENTILATION");
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
        std::snprintf(out, outSize, "CONTRACT L2: BAT %d FUSE %d/3 ACCESS %d", level2BatteryInstalled ? 1 : 0, level2FuseCount, level2AccessReady ? 1 : 0);
        return;
    }
    if (level2HoldActive) {
        std::snprintf(out, outSize, "CONTRACT L2: HOLD LIFT %.0fs", level2HoldTimer);
        return;
    }
    std::snprintf(out, outSize, level2VentDone ? "CONTRACT L2 COMPLETE: LIFT READY + VENT" : "CONTRACT L2 COMPLETE: LIFT READY");
}

inline void buildVoidShiftSupportLine(char* out, int outSize) {
    if (!out || outSize < 2) return;
    if (sideContractType != SIDE_NONE && !sideContractCompleted) {
        std::snprintf(out, outSize, "SIDE: %s %d/%d", sideContractName(sideContractType), sideContractProgress, sideContractTarget);
        return;
    }
    if (sideContractCompleted) {
        std::snprintf(out, outSize, "SIDE COMPLETE: ARCHIVE +35");
        return;
    }
    std::snprintf(out, outSize, "ARCHIVE %d  TIER %d", archivePoints, archiveTier);
}

inline bool tryHandleVoidShiftInteract(const Vec3& playerPos) {
    if (npcCartographerActive && nearPoint2D(playerPos, npcCartographerPos, 2.2f)) {
        awardArchivePoints(10, "CARTOGRAPHER TRADE COMPLETE");
        if (isLevelZero(gCurrentLevel) && !sideContractCompleted && sideContractType == SIDE_SCAN_WALLS) {
            progressSideContract(1, "SIDE: CARTOGRAPHER LOGGED A PATTERN");
        }
        return true;
    }

    if (npcDispatcherActive && nearPoint2D(playerPos, npcDispatcherPhonePos, 2.2f)) {
        setTrapStatus("DISPATCHER: PRIORITY ROUTE MARKED");
        addAttention(4.0f);
        dispatcherCallCooldown = 25.0f;
        if (!isLevelZero(gCurrentLevel) && sideContractType == SIDE_RESTORE_CAMERAS) {
            progressSideContract(1, "SIDE: CAMERAS RESTORED");
        }
        return true;
    }

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
    if (!level2VentDone && nearPoint2D(playerPos, level2VentNode, 2.4f)) {
        level2VentDone = true;
        ventilationOnline = true;
        addAttention(6.0f);
        setTrapStatus("VENTILATION ONLINE");
        if (sideContractType == SIDE_ENABLE_VENT) progressSideContract(1, "SIDE: VENTILATION OBJECTIVE DONE");
        return true;
    }
    if (!level2HoldActive && !level2ContractComplete && level2BatteryInstalled && level2FuseCount >= 3 && level2AccessReady && nearPoint2D(playerPos, level2LiftNode, 2.6f)) {
        level2HoldActive = true;
        level2HoldTimer = 15.0f;
        addAttention(15.0f);
        setTrapStatus("LIFT HOLD STARTED");
        if (sideContractType == SIDE_REPROGRAM_DRONE) progressSideContract(1, "SIDE: DRONE REPROGRAMMED");
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

    if (isParkingLevel(gCurrentLevel)) {
        coLevel += dt * (ventilationOnline ? -3.2f : 2.6f);
        float coDamage = coLevel / 100.0f;
        playerStamina -= dt * coDamage * 4.0f;
        if (playerStamina < 0.0f) playerStamina = 0.0f;
        if (level2VentDone && coLevel <= 20.0f) {
            setEchoStatus("CO STABLE: ROUTE SAFER");
        }
    } else {
        coLevel = 0.0f;
    }

    if (dispatcherCallCooldown > 0.0f) {
        dispatcherCallCooldown -= dt;
        if (dispatcherCallCooldown < 0.0f) dispatcherCallCooldown = 0.0f;
    } else if (npcDispatcherActive && (rng() % 1000) < 3) {
        setEchoStatus("DISPATCHER: CHECK PHONE NODE");
        dispatcherCallCooldown = 16.0f;
    }

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
            setTrapStatus(isLevelZero(gCurrentLevel) ? "CRITICAL: WANDERER HUNT ACTIVE" : "CRITICAL: TOWMAN TRACKING ACTIVE");
            if (multiState != MULTI_IN_GAME) {
                Vec3 ep = findSpawnPos(cam.pos, 12.0f);
                entityMgr.spawnEntity(ENTITY_SHADOW, ep, nullptr, 0, 0);
            }
            attentionEventCooldown = 12.0f;
        } else if (attentionLevel >= 75.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "HIGH: LAMP-HUSHER SWARM" : "HIGH: HEADLIGHT SWARM");
            if (isLevelZero(gCurrentLevel)) lightsOutTimer = 18.0f;
            else falseDoorTimer = 14.0f;
            if (multiState != MULTI_IN_GAME) {
                Vec3 ep = findSpawnPos(cam.pos, 10.0f);
                entityMgr.spawnEntity(ENTITY_CRAWLER, ep, nullptr, 0, 0);
            }
            attentionEventCooldown = 10.0f;
        } else if (attentionLevel >= 50.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "ATTENTION RISING: PHONE MIMIC RISK" : "ATTENTION RISING: DRONE ALERT");
            if (isLevelZero(gCurrentLevel)) falseDoorTimer = 10.0f;
            attentionEventCooldown = 8.0f;
        } else if (attentionLevel >= 25.0f) {
            setTrapStatus(isLevelZero(gCurrentLevel) ? "ATTENTION: PAPERCLIP NOISE" : "ATTENTION: OIL TRACKS DETECTED");
            attentionEventCooldown = 6.0f;
        }
    }

    clampVoidShiftValues();
}
