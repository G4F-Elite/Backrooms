#pragma once
#include <array>

// Bake the original sequence of float subtractions once. Reassociating them
// before truncating to RGBA8 changes a byte at certain corners.
inline constexpr auto lmEnvAmbientBytes = [] {
    std::array<std::array<unsigned char, 3>, 5> values{};
    float openness = 1.0f;
    for (auto& row : values) {
        float ao = openness;
        for (auto& value : row) {
            value = static_cast<unsigned char>((ao < 0.36f ? 0.36f : ao) * 255.0f);
            ao -= 0.10f;
        }
        openness -= 0.13f;
    }
    return values;
}();

// The padded mask includes the four neighbouring strips outside the bake. Those
// cells cast the same contact occlusion at the map edge as cells in its interior.
template<int Size>
struct LmEnvWalls {
    unsigned char cells[Size + 2][Size + 2] = {};

    unsigned char& at(int x, int z) { return cells[z + 1][x + 1]; }
    unsigned char at(int x, int z) const { return cells[z + 1][x + 1]; }

    unsigned char ambient(int x, int z, unsigned char geometryPenalties) const {
        if (at(x, z)) return static_cast<unsigned char>(0.62f * 255.0f);
        int walls = at(x - 1, z) + at(x + 1, z) + at(x, z - 1) + at(x, z + 1);
        return lmEnvAmbientBytes[walls][geometryPenalties];
    }
};
