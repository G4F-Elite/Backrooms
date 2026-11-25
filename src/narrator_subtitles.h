#pragma once

#include "narrator_subtitle_layout.h"
#include "ui_core.h"

inline void drawNarratorSubtitle(const char* text) {
    if (!text || !*text || currentWinW <= 0 || currentWinH <= 0) return;
    static std::string previousText;
    static int previousW = 0, previousH = 0;
    static bool previousOccupied = false;
    static NarratorSubtitleLayout panel;
    const bool lowerHudOccupied = settings.debugMode && gPerfDebugOverlay;
    const float scale = uiScale();
    const float bodySize = (std::max)(2.0f, 11.0f / (8.0f * scale));
    if (previousText != text || previousW != currentWinW || previousH != currentWinH ||
        previousOccupied != lowerHudOccupied) {
        previousText = text;
        previousW = currentWinW;
        previousH = currentWinH;
        previousOccupied = lowerHudOccupied;
        panel = layoutNarratorSubtitle(text, uiHalfW(), uiHalfH(), textAdvancePx(bodySize) / scale,
                                       textCellPx(bodySize) / scale, lowerHudOccupied);
    }
    if (panel.lines.empty()) return;
    const float right = panel.width * 0.5f, left = -right;
    const float inset = left + NARRATOR_PAD_X;
    uiGradH(left, panel.bottom, right, panel.top, UI_BG, 0.93f, 0.85f);
    uiRect(left, panel.bottom, left + 2.0f, panel.top, UI_ACCENT, 0.85f);
    uiText("NARRATOR", inset, panel.top - 12.0f, 1.4f, UI_ACCENT);
    float y = panel.top - NARRATOR_HEADER_H - panel.lineHeight * 0.5f;
    for (const auto& row : panel.lines) {
        uiText(row.c_str(), inset, y, bodySize, {0.96f, 0.94f, 0.85f});
        y -= panel.lineHeight;
    }
}
