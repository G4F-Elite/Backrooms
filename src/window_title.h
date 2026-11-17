#pragma once

inline void updateWindowTitleForLevel(){
    static int lastTitleLevel = -999;
    if(lastTitleLevel == gCurrentLevel) return;
    char levelTitle[96];
    std::snprintf(levelTitle, 96, "Backrooms: Seed Round - B%d", gCurrentLevel + 1);
    glfwSetWindowTitle(gWin, levelTitle);
    lastTitleLevel = gCurrentLevel;
}
