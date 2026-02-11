#pragma once

inline void initVoidShiftSetpieces() {
    conferenceCallTimer = 0.0f;
    conferenceCallPulse = 0.0f;
    corridorShiftTimer = 0.0f;
    corridorShiftArmed = false;
    blackoutSectorTimer = 0.0f;
}

inline void triggerConferenceCallSetpiece(float duration) {
    if (conferenceCallTimer > 0.0f) return;
    conferenceCallTimer = duration;
    conferenceCallPulse = 0.2f;
    setTrapStatus("SETPIECE: CONFERENCE CALL CASCADE");
}

inline void triggerCorridorShiftSetpiece(float duration) {
    if (corridorShiftTimer > 0.0f) return;
    corridorShiftTimer = duration;
    corridorShiftArmed = true;
    setTrapStatus("SETPIECE: CORRIDOR SHIFT IMMINENT");
}

inline void triggerBlackoutSetpiece(float duration) {
    if (blackoutSectorTimer > duration) return;
    blackoutSectorTimer = duration;
    setTrapStatus("SETPIECE: SECTOR BLACKOUT");
}

inline void updateVoidShiftSetpieces(float dt) {
    if (conferenceCallTimer > 0.0f) {
        conferenceCallTimer -= dt;
        conferenceCallPulse -= dt;
        if (conferenceCallPulse <= 0.0f) {
            conferenceCallPulse = 3.0f;
            addAttention(1.5f);
            setEchoStatus("PHONE RING CLUSTER DETECTED");
        }
    }

    if (corridorShiftTimer > 0.0f) {
        corridorShiftTimer -= dt;
        if (corridorShiftArmed && corridorShiftTimer <= 7.0f) {
            corridorShiftArmed = false;
            reshuffleBehind(cam.pos.x, cam.pos.z, cam.yaw);
            setTrapStatus("SETPIECE: CORRIDOR TOPOLOGY SHIFTED");
        }
    }

    if (blackoutSectorTimer > 0.0f) {
        blackoutSectorTimer -= dt;
        if (lightsOutTimer < 1.6f) lightsOutTimer = 1.6f;
    }

    if (craftedFixatorTimer > 0.0f) {
        craftedFixatorTimer -= dt;
        if (craftedFixatorTimer < 0.0f) craftedFixatorTimer = 0.0f;
    }

    if (attentionLevel >= 90.0f && craftedFlashLamp > 0) {
        craftedFlashLamp--;
        addAttention(-18.0f);
        setTrapStatus("CRAFT: FLASH LAMP STAGGER");
    }

    if (level2HoldActive && !isLevel2HoldMaintained() && craftedButtonFixator > 0 && craftedFixatorTimer <= 0.0f) {
        craftedButtonFixator--;
        craftedFixatorTimer = 6.0f;
        setTrapStatus("CRAFT: BUTTON FIXATOR LOCKED");
    }

    if (craftedFixatorTimer > 0.0f && level2HoldActive) {
        level2HoldTimer -= dt * 0.7f;
    }
}

inline void updateVoidShiftHoldPhases(float dt) {
    if (level1HoldActive && !level1ContractComplete) {
        if (isLevel1HoldMaintained()) level1HoldTimer -= dt;
        else level1HoldTimer += dt * 0.45f;
        if (level1HoldTimer > 90.0f) level1HoldTimer = 90.0f;
        if (level1HoldTimer <= 0.0f) { level1HoldTimer = 0.0f; level1HoldActive = false; level1ContractComplete = true; setTrapStatus("LEVEL 1 CONTRACT COMPLETE"); }
    }

    if (level2HoldActive && !level2ContractComplete) {
        if (isLevel2HoldMaintained()) level2HoldTimer -= dt;
        else level2HoldTimer += dt * 0.75f;
        if (level2HoldTimer > 15.0f) level2HoldTimer = 15.0f;
        if (level2HoldTimer <= 0.0f) { level2HoldTimer = 0.0f; level2HoldActive = false; level2ContractComplete = true; setTrapStatus("LEVEL 2 CONTRACT COMPLETE"); }
    }
}
