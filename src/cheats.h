#pragma once

inline constexpr const char MINIMAP_CHEAT_CODE[] = "MINIMAP";
inline constexpr int MINIMAP_CHEAT_CODE_LEN = 7;

inline constexpr const char NOCLIP_CHEAT_CODE[] = "NOCLIP";
inline constexpr int NOCLIP_CHEAT_CODE_LEN = 6;

inline constexpr const char GODMODE_CHEAT_CODE[] = "GODMODE";
inline constexpr int GODMODE_CHEAT_CODE_LEN = 7;

inline constexpr const char FLY_CHEAT_CODE[] = "FLYMODE";
inline constexpr int FLY_CHEAT_CODE_LEN = 7;

inline constexpr const char SPAWN_CHEAT_CODE[] = "SPAWNER";
inline constexpr int SPAWN_CHEAT_CODE_LEN = 7;

// Debug state flags
struct DebugState {
    bool noclip = false;
    bool godMode = false;
    bool flyMode = false;
    bool spawnMode = false;

    int noclipProgress = 0;
    int godProgress = 0;
    int flyProgress = 0;
    int spawnProgress = 0;
};

inline DebugState gDebug;

inline bool pushCheatCodeChar(
    const char* code, int codeLen, int& progress, char inputUpper
) {
    if (!code || codeLen <= 0) return false;
    if (inputUpper == code[progress]) {
        progress++;
        if (progress == codeLen) {
            progress = 0;
            return true;
        }
        return false;
    }
    progress = (inputUpper == code[0]) ? 1 : 0;
    return false;
}

inline bool pushMinimapCheatChar(int& progress, char inputUpper) {
    return pushCheatCodeChar(
        MINIMAP_CHEAT_CODE, MINIMAP_CHEAT_CODE_LEN, progress, inputUpper
    );
}

// Process all debug cheat codes with a single character input
inline void pushDebugCheatChar(char inputUpper) {
    if (pushCheatCodeChar(
        NOCLIP_CHEAT_CODE, NOCLIP_CHEAT_CODE_LEN,
        gDebug.noclipProgress, inputUpper
    )) {
        gDebug.noclip = !gDebug.noclip;
    }

    if (pushCheatCodeChar(
        GODMODE_CHEAT_CODE, GODMODE_CHEAT_CODE_LEN,
        gDebug.godProgress, inputUpper
    )) {
        gDebug.godMode = !gDebug.godMode;
    }

    if (pushCheatCodeChar(
        FLY_CHEAT_CODE, FLY_CHEAT_CODE_LEN,
        gDebug.flyProgress, inputUpper
    )) {
        gDebug.flyMode = !gDebug.flyMode;
    }

    if (pushCheatCodeChar(
        SPAWN_CHEAT_CODE, SPAWN_CHEAT_CODE_LEN,
        gDebug.spawnProgress, inputUpper
    )) {
        gDebug.spawnMode = !gDebug.spawnMode;
    }
}

inline void resetDebugState() {
    gDebug = DebugState{};
}
