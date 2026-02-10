#pragma once

#include "math.h"

enum EchoEventType {
    ECHO_CACHE = 0,
    ECHO_RESTORE = 1,
    ECHO_BREACH = 2
};

struct EchoSignal {
    Vec3 pos;
    int type;
    bool active;
    float ttl;
};

inline int chooseEchoTypeFromRoll(int roll) {
    int norm = roll % 100;
    if (norm < 50) return ECHO_CACHE;
    if (norm < 80) return ECHO_RESTORE;
    return ECHO_BREACH;
}

inline float nextEchoSpawnDelaySeconds(int roll) {
    int norm = roll % 15;
    if (norm < 0) norm += 15;
    return 10.0f + (float)norm;
}

inline float nextNoteSpawnDelaySeconds(int roll) {
    int norm = roll % 11;
    if (norm < 0) norm += 11;
    return 12.0f + (float)norm;
}

inline int chooseCacheItemType(int roll) {
    (void)roll;
    return 0;
}

inline bool isEchoInRange(const Vec3& playerPos, const Vec3& echoPos, float range) {
    Vec3 d = echoPos - playerPos;
    d.y = 0;
    return d.len() < range;
}

inline void clampVitals(float& hp, float& sanity, float& stamina) {
    if (hp > 100.0f) hp = 100.0f;
    if (sanity > 100.0f) sanity = 100.0f;
    if (stamina > 125.0f) stamina = 125.0f;
    if (hp < 0.0f) hp = 0.0f;
    if (sanity < 0.0f) sanity = 0.0f;
    if (stamina < 0.0f) stamina = 0.0f;
}

inline void applyEchoOutcome(
    int echoType,
    int roll,
    int& invBattery,
    float& hp,
    float& sanity,
    float& stamina,
    bool& breachTriggered
) {
    breachTriggered = false;
    if (echoType == ECHO_CACHE) {
        int item = chooseCacheItemType(roll);
        if (item == 0) invBattery++;
        return;
    }
    if (echoType == ECHO_RESTORE) {
        hp += 18.0f;
        sanity += 24.0f;
        stamina += 30.0f;
        clampVitals(hp, sanity, stamina);
        return;
    }
    breachTriggered = true;
    sanity -= 14.0f;
    if (sanity < 0.0f) sanity = 0.0f;
}
