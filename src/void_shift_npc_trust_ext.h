#pragma once

inline void initNpcTrustState() {
    cartographerTrust = 62.0f;
    dispatcherTrust = 60.0f;
    dispatcherCodeword = 100 + (int)(rng() % 900);
}

inline void updateNpcTrustState(float dt) {
    float calm = attentionLevel < 35.0f ? 0.22f : -0.14f;
    cartographerTrust += calm * dt;
    dispatcherTrust += calm * 0.7f * dt;
    if (cartographerTrust < 8.0f) cartographerTrust = 8.0f;
    if (cartographerTrust > 95.0f) cartographerTrust = 95.0f;
    if (dispatcherTrust < 8.0f) dispatcherTrust = 8.0f;
    if (dispatcherTrust > 95.0f) dispatcherTrust = 95.0f;
}

inline void applyCartographerInteractionOutcome() {
    bool deceptive = (attentionLevel > 65.0f) && ((rng() % 100) < 34);
    if (deceptive) {
        cartographerTrust -= 8.0f;
        addAttention(9.0f);
        setTrapStatus("CARTOGRAPHER ROUTE MISLEAD");
        setEchoStatus("TIP UNVERIFIED: CROSS-CHECK MARKERS");
    } else {
        cartographerTrust += 5.0f;
        setTrapStatus("CARTOGRAPHER: RELIABLE ROUTE");
        setEchoStatus("TIP VERIFIED WITH CODEWORD");
    }
    if (cartographerTrust < 8.0f) cartographerTrust = 8.0f;
    if (cartographerTrust > 95.0f) cartographerTrust = 95.0f;
}

inline void applyDispatcherInteractionOutcome() {
    int mimicBase = 22;
    if (attentionLevel > 70.0f) mimicBase += 12;
    if (dispatcherTrust < 40.0f) mimicBase += 12;
    bool deceptive = ((rng() % 100) < mimicBase);
    if (deceptive) {
        dispatcherTrust -= 9.0f;
        addAttention(10.0f);
        playerSanity -= 5.0f;
        if (playerSanity < 0.0f) playerSanity = 0.0f;
        setTrapStatus("DISPATCH CALL MIMIC DETECTED");
        char msg[96];
        std::snprintf(msg, 96, "CODEWORD MISMATCH %03d", dispatcherCodeword);
        setEchoStatus(msg);
    } else {
        dispatcherTrust += 4.0f;
        setTrapStatus("DISPATCH AUTHENTICATED");
        char msg[96];
        std::snprintf(msg, 96, "CODEWORD OK %03d", dispatcherCodeword);
        setEchoStatus(msg);
    }
    if (dispatcherTrust < 8.0f) dispatcherTrust = 8.0f;
    if (dispatcherTrust > 95.0f) dispatcherTrust = 95.0f;
}
