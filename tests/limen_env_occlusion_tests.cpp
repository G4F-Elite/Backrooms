#include <cassert>
#include "../src/limen_env_occlusion.h"

int main() {
    // Cover all four boundary strips as well as the middle of a progressive bake.
    for (int z = 0; z < 3; ++z) {
        for (int x = 0; x < 3; ++x) {
            for (int mask = 0; mask < 16; ++mask) {
                LmEnvWalls<3> field;
                field.at(x - 1, z) = (mask & 1) != 0;
                field.at(x + 1, z) = (mask & 2) != 0;
                field.at(x, z - 1) = (mask & 4) != 0;
                field.at(x, z + 1) = (mask & 8) != 0;
                for (int penalties = 0; penalties <= 2; ++penalties) {
                    float legacy = 1.0f;
                    for (int bit = 0; bit < 4; ++bit)
                        if (mask & (1 << bit)) legacy -= 0.13f;
                    for (int i = 0; i < penalties; ++i) legacy -= 0.10f;
                    if (legacy < 0.36f) legacy = 0.36f;
                    assert(field.ambient(x, z, penalties) == static_cast<unsigned char>(legacy * 255));
                }
                field.at(x, z) = true;
                assert(field.ambient(x, z, 2) == static_cast<unsigned char>(0.62f * 255));
            }
        }
    }
}
