#pragma once

// Sanity economy tuning.
// The intent is: sanity slowly decays over time, and the player must interact
// with systems (Echo signals / items) to stabilize.

inline float sanityPassiveDrainPerSec(float levelDangerScale) {
    // ~6-7 minutes from 100 -> 0 in perfect safety on level 0.
    // Higher levels can scale this up via levelDangerScale.
    return 0.25f * levelDangerScale;
}
