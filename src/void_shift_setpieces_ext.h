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
}
