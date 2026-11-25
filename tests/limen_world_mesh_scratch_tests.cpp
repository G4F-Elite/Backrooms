#include <cassert>
#include "../src/limen_world_mesh_scratch.h"

int main() {
    LmWorldMeshScratch scratch;
    scratch.floors.assign({1.0f, 2.0f, 3.0f});
    const std::size_t capacity = scratch.floors.capacity();
    scratch.resetAfterUpload();
    assert(scratch.floors.empty() && scratch.floors.capacity() == capacity);
    scratch.floors.push_back(-1.0f);
    assert(scratch.floors.size() == 1 && scratch.floors.front() == -1.0f);

    scratch.decor.reserve(LmWorldMeshScratch::maxBufferBytes / sizeof(float) + 1);
    scratch.resetAfterUpload();
    assert(scratch.decor.capacity() == 0);
    assert(scratch.floors.empty() && scratch.floors.capacity() == capacity);

    const std::size_t limit = LmWorldMeshScratch::maxBufferBytes / sizeof(float);
    scratch.walls.reserve(limit);
    scratch.floors.reserve(limit);
    scratch.ceilings.reserve(limit);
    scratch.lamps.push_back(7.0f);
    scratch.resetAfterUpload();
    // With three near-limit buffers, the first two have priority over the third.
    assert(scratch.walls.capacity() == limit && scratch.floors.capacity() == limit);
    assert(scratch.ceilings.capacity() == 0);
    assert(scratch.lamps.empty() && scratch.lamps.capacity() > 0);
    std::size_t bytes = (scratch.walls.capacity() + scratch.floors.capacity() +
                         scratch.lamps.capacity()) * sizeof(float);
    assert(bytes <= LmWorldMeshScratch::maxRetainedBytes);
}
