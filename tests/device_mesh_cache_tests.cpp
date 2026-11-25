#include <cassert>
#include "../src/device_models.h"
#include "../src/zfight_guard.h"

static int uploads = 0;
static void setupVAO(unsigned& vao, unsigned& vbo, std::vector<float>& vertices, bool normals) {
    assert(normals && !vertices.empty() && vertices.size() % 8 == 0);
    vao = ++uploads;
    vbo = vao + 100;
}

#include "../src/device_mesh_cache.h"

int main() {
    using Builder = void (*)(std::vector<float>&);
    const Builder models[] = {buildFlashlightModel, buildScannerModel, buildPlushToyModel,
                              buildBatteryModel, buildMedSprayModel};
    for (Builder build : models) {
        unsigned vao = 0, vbo = 0;
        int count = 0;
        const int before = uploads;
        ensureDeviceMesh(vao, vbo, count, build, "equipment");
        assert(count > 0 && uploads == before + 1);
        const int firstCount = count;
        const unsigned firstVAO = vao, firstVBO = vbo;
        for (int chunk = 0; chunk < 4; ++chunk) {
            ensureDeviceMesh(vao, vbo, count, build, "equipment");
            assert(firstCount == count && firstVAO == vao && firstVBO == vbo);
        }
        assert(uploads == before + 1);
        vao = vbo = 0;
        ensureDeviceMesh(vao, vbo, count, build, "equipment");
        assert(count == firstCount && uploads == before + 2);
    }
}
