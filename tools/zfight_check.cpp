// Build check: nothing in the game may z-fight (two textures in one plane flickering
// through each other). Part of build.bat: the game is not compiled while this fails.
//
// The engine repairs such faces by itself when it builds a mesh (src/zfight_guard.h).
// This tool runs the same code over every model and over sample worlds and refuses the
// build for what is left: flicker the engine cannot repair, or a guard that stopped working.
//
// Build (from repo root):
//   g++ -std=c++17 -O2 -I"deps/include" -o build/zfight_check.exe tools/zfight_check.cpp
// Run:
//   build/zfight_check.exe           pass / fail
//   build/zfight_check.exe --list    also lists what the engine has to repair, model by model
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <unordered_map>
#include <vector>

#include "../src/world.h"
#include "../src/map_content.h"
#include "../src/world_mesh.h"
#include "../src/heist_models.h"
#include "../src/rig_models.h"
#include "../src/device_models.h"

const float CS = 5.0f;
const float WH = 4.5f;
std::mt19937 rng;
unsigned int worldSeed = 0;
std::unordered_map<long long, Chunk> chunks;
std::vector<Light> lights;
std::vector<Vec3> pillars;
std::vector<MapProp> mapProps;
std::vector<MapPoi> mapPois;
int playerChunkX = 0;
int playerChunkZ = 0;

static bool gList = false;
static int gFailures = 0;
static int gChecked = 0;
static int gRepairedMeshes = 0;
static long long gRepairedPairs = 0;
static double gRepairedArea = 0.0;

static void fail(const char* what, const char* why) {
    gFailures++;
    std::printf("  FAIL: %s - %s\n", what, why);
}

// Runs the engine's repair on the meshes, then looks at them the way the depth buffer will.
static bool engineLeavesClean(const char* what, ZfMesh* meshes, int count) {
    gChecked++;
    ZfReport repaired = zfightResolve(meshes, count);
    if (!repaired.clean()) {
        gRepairedMeshes++;
        gRepairedPairs += repaired.pairs;
        gRepairedArea += repaired.area;
        if (gList) zfightPrint(stdout, what, repaired, meshes);
    }
    ZfReport left = zfightScan(meshes, count);
    if (left.clean()) return true;
    fail(what, "still flickers after the engine's repair");
    zfightPrint(stdout, what, left, meshes);
    return false;
}

static bool modelIsClean(const char* what, std::vector<float>& v) {
    ZfMesh m = {&v, 8, 0, what};
    return engineLeavesClean(what, &m, 1);
}

static double facingArea(const std::vector<float>& v, float nx, float ny, float nz) {
    double sum = 0.0;
    for (size_t i = 0; i + 24 <= v.size(); i += 24) {
        Vec3 a(v[i], v[i + 1], v[i + 2]), b(v[i + 8], v[i + 9], v[i + 10]), c(v[i + 16], v[i + 17], v[i + 18]);
        Vec3 n(v[i + 5], v[i + 6], v[i + 7]);
        if (n.x * nx + n.y * ny + n.z * nz < 0.99f) continue;
        sum += (double)(b - a).cross(c - a).len() * 0.5;
    }
    return sum;
}

// The guard itself: a check that finds nothing because it broke would pass everything.
static void checkGuard() {
    std::printf("[1/4] The guard itself...\n");
    {
        // two crates pushed half into each other: top, bottom, front and back share planes
        std::vector<float> v;
        DecorBuilder b(v, 3.0f, -7.0f, 0.6f, 1.0f);
        b.box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, DECOR_MAT_WOOD);
        b.box(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, DECOR_MAT_METAL);
        ZfMesh m = {&v, 8, 0, "crates"};
        ZfReport found = zfightScan(&m, 1);
        if (std::fabs(found.area - 2.0) > 0.01) fail("overlapping crates", "the shared area (4 x 0.5 m2) was not found");
        zfightResolve(&m, 1);
        if (!zfightScan(&m, 1).clean()) fail("overlapping crates", "not clean after the repair");
        if (std::fabs(facingArea(v, 0.0f, 1.0f, 0.0f) - 1.5) > 0.001) fail("overlapping crates", "the repair lost or doubled a piece of the top");
    }
    {
        // a crate on a crate touches it back to back: hidden, must not be reported
        std::vector<float> v;
        DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
        b.box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, DECOR_MAT_WOOD);
        b.box(0.1f, 1.0f, 0.1f, 0.5f, 0.5f, 0.5f, DECOR_MAT_METAL);
        ZfMesh m = {&v, 8, 0, "stack"};
        if (!zfightScan(&m, 1).clean()) fail("stacked crates", "faces that only touch were reported");
    }
    for (int mm = 1; mm <= 2; mm++) {
        // a decal 1 mm above the floor is the same plane for the depth buffer, 2 mm is not
        std::vector<float> floor, decal;
        mkFloorDecal(floor, 0.0f, 0.0f, 0.0f, 5.0f, 5.0f);
        mkFloorDecal(decal, 1.0f, 0.001f * (float)mm, 0.5f, 1.0f, 2.0f);
        ZfMesh layers[2] = {{&floor, 8, 0, "floor"}, {&decal, 8, 1, "decal"}};
        bool flagged = !zfightScan(layers, 2).clean();
        if (flagged != (mm == 1)) fail("decal on the floor", mm == 1 ? "a 1 mm step was not reported" : "a 2 mm step was reported");
        if (mm == 2) continue;
        zfightResolve(layers, 2);
        if (std::fabs(facingArea(floor, 0.0f, 1.0f, 0.0f) - 23.0) > 0.001) fail("decal on the floor", "the floor was not cut under the decal");
        if (std::fabs(facingArea(decal, 0.0f, 1.0f, 0.0f) - 2.0) > 0.001) fail("decal on the floor", "the decal (higher rank) was cut");
    }
}

static void checkProps() {
    for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
        // once at the origin, once far out and turned: float rounding is different there
        for (int place = 0; place < 2; place++) {
            MapProp pr{};
            pr.pos = place == 0 ? Vec3(0.0f, 0.0f, 0.0f) : Vec3(431.3f, 0.0f, -267.9f);
            pr.type = type;
            pr.scale = 1.0f;
            pr.yaw = place == 0 ? 0.0f : 2.1f;
            std::vector<float> v;
            buildMapPropModel(v, pr);
            char name[96];
            std::snprintf(name, sizeof(name), "prop %s%s", PROP_CATALOG[type].name, place == 0 ? "" : " (turned)");
            modelIsClean(name, v);
        }
    }
}

static void checkStructures() {
    for (int type = MAP_POI_OFFICE; type <= MAP_POI_RESTROOM; type++) {
        MapPoi poi{};
        poi.pos = Vec3(12.5f, 0.0f, 7.5f);
        poi.type = type;
        std::vector<float> v;
        buildMapPoiModel(v, poi);
        char name[64];
        std::snprintf(name, sizeof(name), "POI set %d", type);
        modelIsClean(name, v);
    }
    for (int axis = 1; axis <= 2; axis++) {
        std::vector<float> v;
        buildDoorwayModel(v, 3, -4, axis);
        modelIsClean("doorway", v);
    }
    for (int open = 0; open <= 1; open++) {
        std::vector<float> v;
        buildDoorwayModel(v, 0, 0, 1);
        buildExitDoorModel(v, Vec3(CS * 0.5f, 0.0f, CS * 0.5f), open != 0);
        modelIsClean(open ? "exit door (open) in its doorway" : "exit door (shut) in its doorway", v);
        std::vector<float> s;
        buildSwitchModel(s, Vec3(1.0f, 0.0f, 1.0f), open != 0);
        modelIsClean("switch", s);
    }
    for (int parking = 0; parking <= 1; parking++) {
        std::vector<float> pillar, trim;
        mkPillar(pillar, 7.5f, 2.5f, PILLAR_SIZE, WH);
        buildPillarTrimModel(trim, Vec3(7.5f, 0.0f, 2.5f), parking != 0);
        ZfMesh layers[2] = {{&pillar, 8, 1, "pillar"}, {&trim, 8, 2, "pillar trim"}};
        engineLeavesClean("pillar with its trim", layers, 2);
    }
}

template <class Build>
static void checkBuilt(const char* what, Build build) {
    std::vector<float> v;
    DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
    build(b);
    modelIsClean(what, v);
}

static void checkHeist() {
    for (int state = 0; state <= 1; state++) checkBuilt("kiosk", [&](DecorBuilder& b) { buildKioskModel(b, state != 0); });
    checkBuilt("portal rig", [](DecorBuilder& b) { buildPortalRigModel(b); });
    checkBuilt("portal wreck", [](DecorBuilder& b) { buildPortalWreckModel(b); });
    for (int n = 0; n <= 3; n++) checkBuilt("pallet pad", [&](DecorBuilder& b) { buildPalletPadModel(b, n, 3); });
    checkBuilt("pitch board", [](DecorBuilder& b) { buildPitchBoardModel(b); });
    for (unsigned seed = 0; seed < 40u; seed++) {
        checkBuilt("cracked wall face", [&](DecorBuilder& b) { buildCrackFaceModel(b, seed * 977u + 13u, (seed & 1u) != 0u); });
    }
    for (int loot = 0; loot < LOOT_TYPE_COUNT; loot++) checkBuilt("loot", [&](DecorBuilder& b) { buildLootMesh(b, loot); });
    checkBuilt("junk pile", [](DecorBuilder& b) { buildStashMesh(b); });
    checkBuilt("rig arrival hole", [](DecorBuilder& b) { buildRigArrivalModel(b, WH); });
    checkBuilt("rig auger", [](DecorBuilder& b) { buildRigAugerModel(b); });

    typedef void (*DeviceBuilder)(std::vector<float>&);
    const DeviceBuilder devices[5] = {buildFlashlightModel, buildScannerModel, buildPlushToyModel, buildBatteryModel, buildMedSprayModel};
    const char* deviceNames[5] = {"flashlight", "scanner", "plush toy", "battery", "med spray"};
    for (int i = 0; i < 5; i++) {
        std::vector<float> v;
        devices[i](v);
        modelIsClean(deviceNames[i], v);
    }
}

// The rig is drawn with its own matrix, so the engine cannot cut the floor under it:
// no face of the hull may lie in the plane of the floor or the ceiling.
static void checkRig() {
    for (int variant = 0; variant < 12; variant++) {
        RigLook look;
        look.lamps = variant % (look.gaugeLamps + 1);
        look.ready = (variant & 1) != 0;
        look.fault = variant % RIG_FAULT_COUNT;
        look.power = (variant & 2) == 0;
        look.charged = (variant & 4) != 0;
        std::vector<float> hull;
        DecorBuilder b(hull, 0.0f, 0.0f, 0.0f, 1.0f);
        buildRigHullModel(b, look);
        if (!modelIsClean("rig hull", hull)) continue;

        std::vector<float> floor, ceiling;
        mkFloorDecal(floor, 0.0f, 0.0f, 0.0f, 40.0f, 40.0f);
        mkFloorDecal(ceiling, 0.0f, WH, 0.0f, 40.0f, 40.0f);
        for (size_t i = 6; i < ceiling.size(); i += 8) ceiling[i] = -1.0f;
        ZfMesh layers[3] = {{&floor, 8, 0, "floor"}, {&ceiling, 8, 0, "ceiling"}, {&hull, 8, 1, "rig hull"}};
        gChecked++;
        ZfReport r = zfightScan(layers, 3);
        if (r.clean()) continue;
        fail("rig hull on the floor", "a moving mesh shares a plane with the level: lift that face 2 mm or more");
        zfightPrint(stdout, "rig hull on the floor", r, layers);
        break;
    }
}

static bool sameBuffers(WorldMesh& a, WorldMesh& b) {
    return a.walls == b.walls && a.floors == b.floors && a.ceilings == b.ceilings && a.pillars == b.pillars &&
           a.decor == b.decor && a.lampsOn == b.lampsOn && a.lampsOff == b.lampsOff;
}

// Worlds as the game generates and meshes them: what no single model shows - props against
// walls and each other, door frames in walls, decals on the floor, lamps in the ceiling.
static void checkWorlds() {
    std::printf("[4/4] Generated worlds...\n");
    size_t triangles = 0;
    for (int level = 0; level < 8; level++) {
        for (unsigned seed = 1; seed <= 2u; seed++) {
            chunks.clear();
            lights.clear();
            pillars.clear();
            mapProps.clear();
            mapPois.clear();
            gCurrentLevel = level;
            worldSeed = seed * 7919u + (unsigned)level * 104729u;
            int cx = (int)seed * 3 - 4, cz = 2 - (int)seed * 5;
            float px = ((float)cx + 0.5f) * CHUNK_SIZE * CS, pz = ((float)cz + 0.5f) * CHUNK_SIZE * CS;
            updateVisibleChunks(px, pz);
            updateLightsAndPillars(cx, cz);
            updateMapContent(cx, cz);

            // the second seed gets trap holes, so the shafts are covered too
            auto holeAt = [&](int wx, int wz) { return seed == 2u && hashCell(wx, wz, 0x401Eu) % 53u == 0u; };
            WorldMesh mesh;
            buildWorldBaseMesh(mesh, cx, cz, holeAt);
            buildWorldLampMesh(mesh);
            triangles += (mesh.walls.size() + mesh.floors.size() + mesh.ceilings.size() + mesh.pillars.size() + mesh.decor.size()) / 24;

            WorldMesh threaded = mesh;
            ZfMesh layers[7], layersThreaded[7];
            int n = worldMeshLayers(mesh, layers);
            worldMeshLayers(threaded, layersThreaded);
            char name[64];
            std::snprintf(name, sizeof(name), "world: level %d, seed %u", level, worldSeed);
            engineLeavesClean(name, layers, n);

            ZfConfig cfg;
            cfg.threads = 4;
            zfightResolve(layersThreaded, n, cfg);
            if (!sameBuffers(mesh, threaded)) fail(name, "the repair gives another mesh on 4 threads than on 1");
        }
    }
    gCurrentLevel = 0;
    std::printf("      16 worlds, %d thousand triangles\n", (int)(triangles / 1000));
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--list") == 0) gList = true;
    }
    std::printf("Z-FIGHT CHECK: no two faces may flicker through each other\n");
    std::printf("========================================\n");
    checkGuard();
    std::printf("[2/4] Models (props, structures, heist sets, loot, devices)...\n");
    checkProps();
    checkStructures();
    checkHeist();
    std::printf("[3/4] The rig against floor and ceiling...\n");
    checkRig();
    checkWorlds();

    std::printf("\n");
    std::printf("Checked %d meshes. The engine repairs %d of them when it builds them\n", gChecked, gRepairedMeshes);
    std::printf("(%lld face pairs, %.1f m2 that would flicker). Run with --list to see them.\n", gRepairedPairs, gRepairedArea);
    std::printf("========================================\n");
    if (gFailures > 0) {
        std::printf("BUILD REFUSED - %d z-fighting problem(s) the engine cannot repair.\n", gFailures);
        std::printf("========================================\n");
        return 1;
    }
    std::printf("No z-fighting left.\n");
    std::printf("========================================\n");
    return 0;
}
