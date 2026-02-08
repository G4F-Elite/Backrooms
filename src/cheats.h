#pragma once

inline constexpr const char MINIMAP_CHEAT_CODE[] = "MINIMAP";
inline constexpr int MINIMAP_CHEAT_CODE_LEN = 7;

inline bool pushCheatCodeChar(const char* code, int codeLen, int& progress, char inputUpper) {
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
    return pushCheatCodeChar(MINIMAP_CHEAT_CODE, MINIMAP_CHEAT_CODE_LEN, progress, inputUpper);
}
