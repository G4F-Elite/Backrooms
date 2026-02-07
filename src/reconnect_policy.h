#pragma once

inline bool shouldContinueReconnect(int attempts, int maxAttempts) {
    return attempts <= maxAttempts;
}

inline float nextReconnectDelaySeconds(int attempts) {
    if (attempts < 3) return 1.5f;
    if (attempts < 8) return 2.0f;
    return 3.0f;
}
