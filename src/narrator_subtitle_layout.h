#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

struct NarratorSubtitleLayout {
    std::vector<std::string> lines;
    float width = 0, bottom = 0, top = 0, lineHeight = 0;
};

// The game's atlas uses a fixed advance for every ASCII glyph. Prefer a space
// for a break; identifiers longer than a row still display in full.
inline std::vector<std::string> wrapNarratorSubtitle(std::string_view text, size_t columns) {
    std::vector<std::string> rows;
    if (columns == 0) return rows;
    while (!text.empty()) {
        size_t start = text.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) break;
        text.remove_prefix(start);
        size_t end = (std::min)(columns, text.size());
        size_t newline = text.find_first_of("\r\n");
        if (newline <= end) end = newline;
        else if (end < text.size() && text[end] != ' ' && text[end] != '\t') {
            size_t space = text.substr(0, end).find_last_of(" \t");
            if (space != std::string_view::npos) end = space;
        }
        auto row = text.substr(0, end);
        row = row.substr(0, row.find_last_not_of(" \t") + 1);
        rows.emplace_back(row);
        std::replace(rows.back().begin(), rows.back().end(), '\t', ' ');
        text.remove_prefix(end);
    }
    return rows;
}

inline constexpr float NARRATOR_PAD_X = 14.0f;
inline constexpr float NARRATOR_HEADER_H = 24.0f;
inline constexpr float NARRATOR_BOTTOM_PAD = 6.0f;

// Canvas units match ui_core. Keep the lower plate between the carry strip and
// the interaction prompt, and inside the side gauges even with the scanner out.
inline NarratorSubtitleLayout layoutNarratorSubtitle(std::string_view text, float halfW, float halfH,
                                                      float advance, float cell, bool lowerHudOccupied) {
    NarratorSubtitleLayout out;
    float maxWidth = (std::min)(halfW * 0.94f, 840.0f);
    if (advance <= 0 || maxWidth < 2 * NARRATOR_PAD_X + advance || halfH <= 0) return out;
    size_t columns = static_cast<size_t>((maxWidth - 2 * NARRATOR_PAD_X) / advance);
    out.lines = wrapNarratorSubtitle(text, columns);
    if (out.lines.empty()) return out;
    size_t longest = 0;
    for (const auto& row : out.lines) longest = (std::max)(longest, row.size());
    out.width = (std::min)(maxWidth, 2 * NARRATOR_PAD_X + (std::max)(76.0f, longest * advance));
    out.lineHeight = cell + 5.0f;
    float height = NARRATOR_HEADER_H + NARRATOR_BOTTOM_PAD + out.lines.size() * out.lineHeight;
    out.bottom = -0.825f * halfH;
    out.top = out.bottom + height;
    if (lowerHudOccupied || out.top > -0.54f * halfH) {
        out.top = 0.48f * halfH;
        out.bottom = out.top - height;
    }
    return out;
}
