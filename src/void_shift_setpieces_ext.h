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
    setTrapStatus("EVERY PHONE ON THE FLOOR IS RINGING. NOBODY PICK UP");
}

inline void triggerCorridorShiftSetpiece(float duration) {
    if (corridorShiftTimer > 0.0f) return;
    corridorShiftTimer = duration;
    corridorShiftArmed = true;
    setTrapStatus("THE WALLS ARE ABOUT TO MOVE. STAY TOGETHER");
}

inline void triggerBlackoutSetpiece(float duration) {
    if (blackoutSectorTimer > duration) return;
    blackoutSectorTimer = duration;
    setTrapStatus("TOO LOUD. SOMEBODY FLIPPED THE BREAKER");
}

inline void updateVoidShiftSetpieces(float dt) {
    if (conferenceCallTimer > 0.0f) {
        conferenceCallTimer -= dt;
        conferenceCallPulse -= dt;
        if (conferenceCallPulse <= 0.0f) {
            conferenceCallPulse = 3.0f;
            addAttention(1.5f);
            setEchoStatus("STILL RINGING. SOMETHING CAN HEAR THAT");
        }
    }

    if (corridorShiftTimer > 0.0f) {
        corridorShiftTimer -= dt;
        if (corridorShiftArmed && corridorShiftTimer <= 7.0f) {
            corridorShiftArmed = false;
            if (multiState != MULTI_IN_GAME) {
                reshuffleBehind(cam.pos.x, cam.pos.z, cam.yaw);
                reopenObjectiveCells();
                buildGeom();
            }
            setTrapStatus("THE WALLS MOVED. THE WAY BACK IS NOW A RUMOUR");
        }
    }

    if (blackoutSectorTimer > 0.0f) {
        blackoutSectorTimer -= dt;
        if (lightsOutTimer < 1.6f) lightsOutTimer = 1.6f;
    }
}
