#pragma once

inline void updateVoidShiftEnemyEffects(float dt) {
    if (attentionLevel >= 25.0f && isLevelZero(gCurrentLevel)) {
        paperclipSwarmTimer += dt;
        if (paperclipSwarmTimer >= 6.0f) {
            paperclipSwarmTimer = 0.0f;
            if (invBattery > 0) {
                invBattery--;
                setEchoStatus("PAPERCLIP SWARM STOLE BATTERY");
            } else if (invPlush > 0) {
                invPlush--;
                setEchoStatus("PAPERCLIP SWARM TOOK PLUSH");
            }
            addAttention(3.0f);
        }
    } else {
        paperclipSwarmTimer = 0.0f;
    }

    if (isParkingLevel(gCurrentLevel) && attentionLevel >= 25.0f) {
        oilSlipTimer += dt;
        if (oilSlipTimer >= 7.5f) {
            oilSlipTimer = 0.0f;
            playerStamina -= 22.0f;
            if (playerStamina < 0.0f) playerStamina = 0.0f;
            addAttention(4.0f);
            setTrapStatus("OIL SLIP: FOOTPRINTS EXPOSED");
        }
    } else {
        oilSlipTimer = 0.0f;
    }

    if (isParkingLevel(gCurrentLevel) && attentionLevel >= 75.0f) {
        headlightExposureTimer += dt;
        if (headlightExposureTimer >= 2.2f) {
            headlightExposureTimer = 0.0f;
            addAttention(2.5f);
            playerSanity -= 3.0f;
            if (playerSanity < 0.0f) playerSanity = 0.0f;
            setTrapStatus("HEADLIGHT SWARM TRACKING");
        }
    } else {
        headlightExposureTimer = 0.0f;
    }
}
