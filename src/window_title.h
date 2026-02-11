#pragma once

inline void updateWindowTitleForLevel(){
    static int lastTitleLevel = -999;
    if(lastTitleLevel == gCurrentLevel) return;
    char levelTitle[96];
    char levelName[48];
    buildLevelLabel(gCurrentLevel, levelName, 48);
    std::snprintf(levelTitle, 96, "Backrooms: Void Shift - %s", levelName);
    glfwSetWindowTitle(gWin, levelTitle);
    lastTitleLevel = gCurrentLevel;
}
