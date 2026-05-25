#pragma once

// Sanity economy tuning.
// The intent is: sanity slowly decays over time, and the player must interact
// with systems (Echo signals / items) to stabilize.
// Recovery kicks in when player is safe (no nearby threats).

inline float sanityPassiveDrainPerSec(float levelDangerScale) {
    // ~12-14 minutes from 100 -> 0 in perfect safety on level 0.
    // Higher levels can scale this up via levelDangerScale.
    return 0.125f * levelDangerScale;
}

inline float sanityRecoveryPerSec(bool isSafe, float currentSanity) {
    if (!isSafe || currentSanity >= 100.0f) return 0.0f;
    // Slow recovery when safe: ~0.1/sec at low sanity, tapering to 0 at full.
    // ~1000 seconds to full from 0, faster when lower.
    return 0.1f * (1.0f - currentSanity / 100.0f);
}
