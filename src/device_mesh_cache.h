#pragma once
#include <vector>

// These equipment models have fixed geometry. Texture or world changes do not
// invalidate their GPU buffers.
inline void ensureDeviceMesh(unsigned int& vao, unsigned int& vbo, int& count,
                             void (*build)(std::vector<float>&), const char* label) {
    if (vao && vbo) return;
    std::vector<float> vertices;
    build(vertices);
    resolveModelMesh(vertices, label);
    count = static_cast<int>(vertices.size() / 8);
    if (count > 0) setupVAO(vao, vbo, vertices, true);
}
