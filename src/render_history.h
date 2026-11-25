#pragma once
#include "math.h"
#include "upscaler_settings.h"
#include <algorithm>
#include <stdexcept>

inline bool needsRenderHistory(int upscale, int aa) {
    return aa == AA_MODE_TAA || upscale == UPSCALER_MODE_FSR20;
}

inline void jitterSceneProjection(Mat4& projection, int frame, int w, int h) {
    // Eight centred Halton samples, in input pixel units. Geometry and its depth
    // receive the same offset before rasterization.
    static constexpr float samples[8][2] = {
        {0, -1.f/6}, {-0.25f, 1.f/6}, {0.25f, -7.f/18}, {-0.375f, -1.f/18},
        {0.125f, 5.f/18}, {-0.125f, -5.f/18}, {0.375f, 1.f/18}, {-0.4375f, 7.f/18}
    };
    projection.m[8] -= 2 * samples[frame % 8][0] / w;
    projection.m[9] -= 2 * samples[frame % 8][1] / h;
}

inline Mat4 previousClipFromCurrent(const Mat4& current, const Mat4& previous) {
    // Solve current^T * X = previous^T. Each column of X is a row of the
    // desired GPU matrix previous * inverse(current), in OpenGL layout.
    double aug[4][8];
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) {
        aug[i][j] = current.m[4*i+j];
        aug[i][j+4] = previous.m[4*i+j];
    }
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int row = col+1; row < 4; ++row)
            if (std::abs(aug[row][col]) > std::abs(aug[pivot][col])) pivot = row;
        if (std::abs(aug[pivot][col]) < 1e-12) throw std::logic_error("Singular scene view-projection");
        if (pivot != col) std::swap(aug[col], aug[pivot]);
        const double scale = aug[col][col];
        for (double& v : aug[col]) v /= scale;
        for (int row = 0; row < 4; ++row) {
            if (row == col) continue;
            const double weight = aug[row][col];
            for (int j = 0; j < 8; ++j) aug[row][j] -= weight*aug[col][j];
        }
    }
    Mat4 result;
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
        result.m[4*i+j] = static_cast<float>(aug[i][j+4]);
    return result;
}
