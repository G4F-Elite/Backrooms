#pragma once

#include <cstdio>

inline void loadArchiveMetaProgress(int& points, int& tier, bool& perkQuiet, bool& perkFast, bool& perkEcho) {
    FILE* f = std::fopen("save_meta.dat", "rb");
    if (!f) return;
    int p = 0, t = 0, q = 0, h = 0, e = 0;
    if (std::fread(&p, sizeof(int), 1, f) == 1 &&
        std::fread(&t, sizeof(int), 1, f) == 1 &&
        std::fread(&q, sizeof(int), 1, f) == 1 &&
        std::fread(&h, sizeof(int), 1, f) == 1 &&
        std::fread(&e, sizeof(int), 1, f) == 1) {
        points = p;
        tier = t;
        perkQuiet = (q != 0);
        perkFast = (h != 0);
        perkEcho = (e != 0);
    }
    std::fclose(f);
}

inline void saveArchiveMetaProgress(int points, int tier, bool perkQuiet, bool perkFast, bool perkEcho) {
    FILE* f = std::fopen("save_meta.dat", "wb");
    if (!f) return;
    int q = perkQuiet ? 1 : 0, h = perkFast ? 1 : 0, e = perkEcho ? 1 : 0;
    std::fwrite(&points, sizeof(int), 1, f);
    std::fwrite(&tier, sizeof(int), 1, f);
    std::fwrite(&q, sizeof(int), 1, f);
    std::fwrite(&h, sizeof(int), 1, f);
    std::fwrite(&e, sizeof(int), 1, f);
    std::fclose(f);
}

inline int gCurrentLevel = 0;
inline int gCompletedLevels = 0;

inline bool isLevelZero(int level) {
    return level <= 0;
}

inline bool isParkingLevel(int level) {
    return level >= 1;
}

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
    if (level == 0) {
        std::snprintf(out, outSize, "LEVEL 1 - YELLOW ROOMS");
        return;
    }
    if (level == 1) {
        std::snprintf(out, outSize, "LEVEL 2 - PARKING");
        return;
    }
    std::snprintf(out, outSize, "LEVEL %d - VOID SHIFT", level + 1);
}
