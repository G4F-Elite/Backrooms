#pragma once

#include <cstdio>

inline int gCurrentLevel = 0;
inline int gCompletedLevels = 0;

inline int levelEntityCapBonus(int level) {
    if (level <= 0) return 0;
    return level / 2;
}

inline float levelSpawnDelayScale(int level) {
    float s = 1.0f - (float)level * 0.05f;
    if (s < 0.62f) s = 0.62f;
    return s;
}

inline float levelDangerScale(int level) {
    float s = 1.0f + (float)level * 0.12f;
    if (s > 2.1f) s = 2.1f;
    return s;
}

inline void buildLevelLabel(int level, char* out, int outSize) {
    if (!out || outSize < 2) return;
    if (level < 0) level = 0;
    std::snprintf(out, outSize, "LEVEL %d", level);
}
