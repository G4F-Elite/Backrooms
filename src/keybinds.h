#pragma once
#include <GLFW/glfw3.h>

struct GameplayBinds {
    int forward = GLFW_KEY_W;
    int back = GLFW_KEY_S;
    int left = GLFW_KEY_A;
    int right = GLFW_KEY_D;
    int sprint = GLFW_KEY_LEFT_SHIFT;
    int crouch = GLFW_KEY_LEFT_CONTROL;
    int interact = GLFW_KEY_E;
    int flashlight = GLFW_KEY_F;
    int pause = GLFW_KEY_ESCAPE;
    int item1 = GLFW_KEY_1;
    int item2 = GLFW_KEY_2;
    int item3 = GLFW_KEY_3;
};

inline constexpr int GAMEPLAY_BIND_COUNT = 12;
inline constexpr int KEYBINDS_BACK_INDEX = GAMEPLAY_BIND_COUNT;

inline const char* gameplayBindLabel(int idx) {
    static const char* labels[GAMEPLAY_BIND_COUNT] = {
        "MOVE FORWARD", "MOVE BACK", "MOVE LEFT", "MOVE RIGHT",
        "SPRINT", "CROUCH", "INTERACT", "FLASHLIGHT",
        "PAUSE", "ITEM SLOT 1", "ITEM SLOT 2", "ITEM SLOT 3"
    };
    if (idx < 0 || idx >= GAMEPLAY_BIND_COUNT) return "";
    return labels[idx];
}

inline int* gameplayBindByIndex(GameplayBinds& binds, int idx) {
    switch (idx) {
        case 0: return &binds.forward;
        case 1: return &binds.back;
        case 2: return &binds.left;
        case 3: return &binds.right;
        case 4: return &binds.sprint;
        case 5: return &binds.crouch;
        case 6: return &binds.interact;
        case 7: return &binds.flashlight;
        case 8: return &binds.pause;
        case 9: return &binds.item1;
        case 10: return &binds.item2;
        case 11: return &binds.item3;
        default: return nullptr;
    }
}

inline bool isGameplayBindIndex(int idx) {
    return idx >= 0 && idx < GAMEPLAY_BIND_COUNT;
}

inline int clampKeybindMenuIndex(int idx) {
    if (idx < 0) return KEYBINDS_BACK_INDEX;
    if (idx > KEYBINDS_BACK_INDEX) return 0;
    return idx;
}

inline const char* keyNameForUi(int key) {
    switch (key) {
        case GLFW_KEY_SPACE: return "SPACE";
        case GLFW_KEY_LEFT_SHIFT: return "L-SHIFT";
        case GLFW_KEY_RIGHT_SHIFT: return "R-SHIFT";
        case GLFW_KEY_LEFT_CONTROL: return "L-CTRL";
        case GLFW_KEY_RIGHT_CONTROL: return "R-CTRL";
        case GLFW_KEY_LEFT_ALT: return "L-ALT";
        case GLFW_KEY_RIGHT_ALT: return "R-ALT";
        case GLFW_KEY_ENTER: return "ENTER";
        case GLFW_KEY_ESCAPE: return "ESC";
        case GLFW_KEY_TAB: return "TAB";
        case GLFW_KEY_BACKSPACE: return "BACKSPACE";
        case GLFW_KEY_UP: return "UP";
        case GLFW_KEY_DOWN: return "DOWN";
        case GLFW_KEY_LEFT: return "LEFT";
        case GLFW_KEY_RIGHT: return "RIGHT";
        case GLFW_KEY_HOME: return "HOME";
        case GLFW_KEY_F1: return "F1";
        case GLFW_KEY_F2: return "F2";
        case GLFW_KEY_F3: return "F3";
        case GLFW_KEY_F4: return "F4";
        case GLFW_KEY_F5: return "F5";
        case GLFW_KEY_F6: return "F6";
        case GLFW_KEY_F7: return "F7";
        case GLFW_KEY_F8: return "F8";
        case GLFW_KEY_F9: return "F9";
        case GLFW_KEY_F10: return "F10";
        case GLFW_KEY_F11: return "F11";
        case GLFW_KEY_F12: return "F12";
        default: break;
    }
    static char buf[8];
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        buf[0] = (char)('A' + (key - GLFW_KEY_A));
        buf[1] = '\0';
        return buf;
    }
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        buf[0] = (char)('0' + (key - GLFW_KEY_0));
        buf[1] = '\0';
        return buf;
    }
    return "KEY";
}

