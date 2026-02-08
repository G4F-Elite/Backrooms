#include <cassert>
#include <cstring>
#include <iostream>

#include "../src/player_name.h"

void testDefaultName() {
    char out[PLAYER_NAME_BUF_LEN];
    sanitizePlayerName(nullptr, out);
    assert(strcmp(out, "Player") == 0);
    sanitizePlayerName("", out);
    assert(strcmp(out, "Player") == 0);
}

void testAllowedChars() {
    char out[PLAYER_NAME_BUF_LEN];
    sanitizePlayerName("Abc_19-Z x", out);
    assert(strcmp(out, "Abc_19-Z x") == 0);
}

void testDropsInvalidChars() {
    char out[PLAYER_NAME_BUF_LEN];
    sanitizePlayerName("A!@#B$%^C", out);
    assert(strcmp(out, "ABC") == 0);
}

void testTrimRightSpaces() {
    char out[PLAYER_NAME_BUF_LEN];
    sanitizePlayerName("Name   ", out);
    assert(strcmp(out, "Name") == 0);
}

void testLengthLimit() {
    char out[PLAYER_NAME_BUF_LEN];
    sanitizePlayerName("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789", out);
    assert((int)strlen(out) == PLAYER_NAME_BUF_LEN - 1);
}

int main() {
    testDefaultName();
    testAllowedChars();
    testDropsInvalidChars();
    testTrimRightSpaces();
    testLengthLimit();
    std::cout << "All player name tests passed.\n";
    return 0;
}
