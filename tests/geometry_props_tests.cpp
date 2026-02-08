#include <cassert>
#include <iostream>
#include <vector>

#include "../src/math.h"

const float CS = 5.0f;
const float WH = 4.5f;

#include "../src/geometry.h"

void testBoxVertexCount() {
    std::vector<float> v;
    mkBox(v, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    assert(v.size() == 240);
}

void testBoxUvExtendsBeyondOneForLargeProps() {
    std::vector<float> v;
    mkBox(v, 0.0f, 0.0f, 0.0f, 9.0f, 3.0f, 8.0f);
    bool hasTiledUv = false;
    for (size_t i = 0; i + 4 < v.size(); i += 8) {
        float u = v[i + 3];
        float t = v[i + 4];
        if (u > 1.0f || t > 1.0f) {
            hasTiledUv = true;
            break;
        }
    }
    assert(hasTiledUv);
}

int main() {
    testBoxVertexCount();
    testBoxUvExtendsBeyondOneForLargeProps();
    std::cout << "All geometry props tests passed.\n";
    return 0;
}
