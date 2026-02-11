#pragma once

#include <cstdio>

inline constexpr int META_MAGIC = 0x56534D31;

inline void loadArchiveMetaProgress(int& points, int& tier, bool& perkQuiet, bool& perkFast, bool& perkEcho,
                                    bool& recNoise, bool& recBeacon, bool& recFlash, bool& recFix, int& markerStyle) {
    FILE* f = std::fopen("save_meta.dat", "rb");
    if (!f) return;
    int first = 0;
    if (std::fread(&first, sizeof(int), 1, f) != 1) { std::fclose(f); return; }

    if (first == META_MAGIC) {
        int t = 0, q = 0, h = 0, e = 0, rn = 0, rb = 0, rf = 0, rx = 0, ms = 0;
        if (std::fread(&points, sizeof(int), 1, f) == 1 &&
            std::fread(&t, sizeof(int), 1, f) == 1 &&
            std::fread(&q, sizeof(int), 1, f) == 1 &&
            std::fread(&h, sizeof(int), 1, f) == 1 &&
            std::fread(&e, sizeof(int), 1, f) == 1 &&
            std::fread(&rn, sizeof(int), 1, f) == 1 &&
            std::fread(&rb, sizeof(int), 1, f) == 1 &&
            std::fread(&rf, sizeof(int), 1, f) == 1 &&
            std::fread(&rx, sizeof(int), 1, f) == 1 &&
            std::fread(&ms, sizeof(int), 1, f) == 1) {
            tier = t;
            perkQuiet = (q != 0); perkFast = (h != 0); perkEcho = (e != 0);
            recNoise = (rn != 0); recBeacon = (rb != 0); recFlash = (rf != 0); recFix = (rx != 0);
            markerStyle = ms;
        }
        std::fclose(f);
        return;
    }

    int p = first, t = 0, q = 0, h = 0, e = 0;
    if (std::fread(&t, sizeof(int), 1, f) == 1 &&
        std::fread(&q, sizeof(int), 1, f) == 1 &&
        std::fread(&h, sizeof(int), 1, f) == 1 &&
        std::fread(&e, sizeof(int), 1, f) == 1) {
        points = p;
        tier = t;
        perkQuiet = (q != 0); perkFast = (h != 0); perkEcho = (e != 0);
        recNoise = tier >= 1; recBeacon = tier >= 1; recFlash = tier >= 2; recFix = tier >= 3;
        markerStyle = 0;
    }
    std::fclose(f);
}

inline void saveArchiveMetaProgress(int points, int tier, bool perkQuiet, bool perkFast, bool perkEcho,
                                    bool recNoise, bool recBeacon, bool recFlash, bool recFix, int markerStyle) {
    FILE* f = std::fopen("save_meta.dat", "wb");
    if (!f) return;
    int q = perkQuiet ? 1 : 0, h = perkFast ? 1 : 0, e = perkEcho ? 1 : 0;
    int rn = recNoise ? 1 : 0, rb = recBeacon ? 1 : 0, rf = recFlash ? 1 : 0, rx = recFix ? 1 : 0;
    std::fwrite(&META_MAGIC, sizeof(int), 1, f);
    std::fwrite(&points, sizeof(int), 1, f);
    std::fwrite(&tier, sizeof(int), 1, f);
    std::fwrite(&q, sizeof(int), 1, f);
    std::fwrite(&h, sizeof(int), 1, f);
    std::fwrite(&e, sizeof(int), 1, f);
    std::fwrite(&rn, sizeof(int), 1, f);
    std::fwrite(&rb, sizeof(int), 1, f);
    std::fwrite(&rf, sizeof(int), 1, f);
    std::fwrite(&rx, sizeof(int), 1, f);
    std::fwrite(&markerStyle, sizeof(int), 1, f);
    std::fclose(f);
}

inline void loadArchiveMetaProgress(int& points, int& tier, bool& perkQuiet, bool& perkFast, bool& perkEcho) {
    bool rn = false, rb = false, rf = false, rx = false;
    int ms = 0;
    loadArchiveMetaProgress(points, tier, perkQuiet, perkFast, perkEcho, rn, rb, rf, rx, ms);
}

inline void saveArchiveMetaProgress(int points, int tier, bool perkQuiet, bool perkFast, bool perkEcho) {
    saveArchiveMetaProgress(points, tier, perkQuiet, perkFast, perkEcho, tier >= 1, tier >= 1, tier >= 2, tier >= 3, 0);
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
