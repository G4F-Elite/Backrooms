#pragma once
#include <cstddef>
#include <vector>

struct LmWorldMeshScratch {
    std::vector<float> walls, floors, ceilings, pillars, lamps, darkLamps, decor;

    static constexpr std::size_t maxBufferBytes = 12u << 20;
    static constexpr std::size_t maxRetainedBytes = 32u << 20;

    // glBufferData has consumed the vertices. Retain ordinary shell buffers first;
    // a one-off oversized vault must not set the process's permanent high-water mark.
    void resetAfterUpload() {
        std::size_t retainedBytes = 0;
        for (auto* buffer : {&walls, &floors, &ceilings, &pillars, &lamps, &darkLamps, &decor}) {
            const std::size_t bytes = buffer->capacity() * sizeof(float);
            if (bytes > maxBufferBytes || bytes > maxRetainedBytes - retainedBytes) {
                std::vector<float>().swap(*buffer);
            } else {
                buffer->clear();
                retainedBytes += bytes;
            }
        }
    }
};
