#pragma once

inline constexpr int MINIMAP_RADIUS = 5;
inline constexpr int MINIMAP_DIAMETER = MINIMAP_RADIUS * 2 + 1;

using MinimapWallSampler = int(*)(int wx, int wz);

inline char minimapGlyphForCell(int wx, int wz, int playerWX, int playerWZ, MinimapWallSampler sampler) {
    if (wx == playerWX && wz == playerWZ) return 'P';
    if (!sampler) return '?';
    return sampler(wx, wz) ? 'X' : '-';
}

inline void buildMinimapRows(
    char rows[MINIMAP_DIAMETER][MINIMAP_DIAMETER + 1],
    int playerWX,
    int playerWZ,
    MinimapWallSampler sampler
) {
    for (int row = 0; row < MINIMAP_DIAMETER; row++) {
        int wz = playerWZ + (MINIMAP_RADIUS - row);
        for (int col = 0; col < MINIMAP_DIAMETER; col++) {
            int wx = playerWX + (col - MINIMAP_RADIUS);
            rows[row][col] = minimapGlyphForCell(wx, wz, playerWX, playerWZ, sampler);
        }
        rows[row][MINIMAP_DIAMETER] = '\0';
    }
}
