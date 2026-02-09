#pragma once
#pragma once
#include <GLFW/glfw3.h>
#include "menu.h"
#include "audio.h"
#include "net.h"
#include "menu_multi.h"
#include "lan_discovery.h"

extern bool escPressed, upPressed, downPressed, enterPressed, leftPressed, rightPressed;
extern bool firstMouse;
extern float lastX, lastY;

inline bool isAnyKeyboardKeyDown(GLFWwindow* w) {
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_MENU; k++) {
        if (glfwGetKey(w, k) == GLFW_PRESS) return true;
    }
    return false;
}

inline int firstPressedKeyboardKey(GLFWwindow* w) {
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_MENU; k++) {
        if (glfwGetKey(w, k) == GLFW_PRESS) return k;
    }
    return -1;
}

inline void pushNicknameChar(char c) {
    char next[PLAYER_NAME_BUF_LEN + 1] = {};
    int len = (int)strlen(multiNickname);
    if (len >= PLAYER_NAME_BUF_LEN - 1) return;
    memcpy(next, multiNickname, len);
    next[len] = c;
    next[len + 1] = '\0';
    sanitizePlayerName(next, multiNickname);
}

inline void popNicknameChar() {
    int len = (int)strlen(multiNickname);
    if (len <= 0) return;
    multiNickname[len - 1] = '\0';
}

inline void handleNicknameInput(GLFWwindow* w) {
    static bool letterPressed[26] = {false};
    static bool digitPressed[10] = {false};
    static bool spacePressedNick = false;
    static bool minusPressedNick = false;
    static bool underscorePressedNick = false;
    static bool backspacePressedNick = false;

    for (int i = 0; i < 26; i++) {
        bool now = glfwGetKey(w, GLFW_KEY_A + i) == GLFW_PRESS;
        if (now && !letterPressed[i]) pushNicknameChar((char)('a' + i));
        letterPressed[i] = now;
    }
    for (int i = 0; i < 10; i++) {
        bool now = glfwGetKey(w, GLFW_KEY_0 + i) == GLFW_PRESS;
        if (now && !digitPressed[i]) pushNicknameChar((char)('0' + i));
        digitPressed[i] = now;
    }
    bool spaceNow = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceNow && !spacePressedNick) pushNicknameChar(' ');
    spacePressedNick = spaceNow;

    bool underNow = glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS &&
                    (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool minusNow = glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS;
    if (underNow && !underscorePressedNick) pushNicknameChar('_');
    else if (minusNow && !minusPressedNick) pushNicknameChar('-');
    underscorePressedNick = underNow;
    minusPressedNick = minusNow;

    bool bsNow = glfwGetKey(w, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (bsNow && !backspacePressedNick) popNicknameChar();
    backspacePressedNick = bsNow;
}

