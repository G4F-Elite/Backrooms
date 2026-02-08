#pragma once

const int PLAYER_NAME_BUF_LEN = 32;

inline bool isPlayerNameChar(char c) {
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c == ' ';
}

inline void sanitizePlayerName(const char* input, char outName[PLAYER_NAME_BUF_LEN]) {
    if (!input) {
        outName[0] = 'P'; outName[1] = 'l'; outName[2] = 'a'; outName[3] = 'y'; outName[4] = 'e'; outName[5] = 'r'; outName[6] = '\0';
        return;
    }
    int out = 0;
    for (int i = 0; input[i] && out < PLAYER_NAME_BUF_LEN - 1; i++) {
        if (!isPlayerNameChar(input[i])) continue;
        outName[out++] = input[i];
    }
    while (out > 0 && outName[out - 1] == ' ') out--;
    outName[out] = '\0';
    if (out == 0) {
        outName[0] = 'P'; outName[1] = 'l'; outName[2] = 'a'; outName[3] = 'y'; outName[4] = 'e'; outName[5] = 'r'; outName[6] = '\0';
    }
}
