#pragma once
#include "decor_builder.h"
#include "prop_models.h"

// ============================================================================
// STRUCTURE MODELS
// Doorways, exit portal, switches, pillar trim and POI sets.
// POI sets use the wall face as origin: local Z = 0 is the wall, +Z the room.
// Sizes must stay inside mapPoiSetSize() so collision matches the mesh.
// ============================================================================

// Framed doorway filling a one-cell hole in a wall. axis: see doorwayAxisAt().
inline void buildDoorwayModel(std::vector<float>& v, int wx, int wz, int axis) {
    DoorwayDims d = doorwayDims();
    float yaw = (axis == 1) ? 0.0f : 1.5707963f;
    DecorBuilder b(v, ((float)wx + 0.5f) * CS, ((float)wz + 0.5f) * CS, yaw, 1.0f);
    float half = CS * 0.5f;
    float outer = d.openingHalf + d.postW * 0.5f;
    float sideW = half - outer;
    float sideC = (half + outer) * 0.5f;
    float lintelTop = d.openingH + d.postW;

    b.box(-sideC, 0.0f, 0.0f, sideW, WH, d.fillT, DECOR_MAT_PLASTER);
    b.box(sideC, 0.0f, 0.0f, sideW, WH, d.fillT, DECOR_MAT_PLASTER);
    b.box(0.0f, lintelTop, 0.0f, outer * 2.0f, WH - lintelTop, d.fillT, DECOR_MAT_PLASTER);
    b.box(-d.openingHalf, 0.0f, 0.0f, d.postW, d.openingH, d.frameT, DECOR_MAT_TRIM);
    b.box(d.openingHalf, 0.0f, 0.0f, d.postW, d.openingH, d.frameT, DECOR_MAT_TRIM);
    b.box(0.0f, d.openingH, 0.0f, outer * 2.0f, d.postW, d.frameT, DECOR_MAT_TRIM);
}

// Door leaf + sign for the exit portal. The frame itself is a forced doorway.
inline void buildExitDoorModel(std::vector<float>& v, const Vec3& pos, bool open) {
    DoorwayDims d = doorwayDims();
    DecorBuilder b(v, pos.x, pos.z, 0.0f, 1.0f);
    float inner = d.openingHalf - d.postW * 0.5f;
    const float leafT = 0.07f;
    if (!open) {
        b.panel(0.0f, 0.0f, 0.0f, inner * 2.0f, d.openingH, leafT, DECOR_MAT_DOOR);
        b.box(0.0f, 1.00f, leafT * 0.5f + 0.02f, inner * 1.5f, 0.05f, 0.04f, DECOR_MAT_METAL);
        b.box(0.0f, 1.00f, -leafT * 0.5f - 0.02f, inner * 1.5f, 0.05f, 0.04f, DECOR_MAT_METAL);
    } else {
        // swung open around the left post, lying along the passage
        b.panel(-inner + leafT * 0.5f, 0.0f, d.frameT * 0.5f + inner, leafT, d.openingH, inner * 2.0f, DECOR_MAT_DOOR);
    }
    float signY = d.openingH + d.postW + 0.12f;
    float signZ = d.fillT * 0.5f + 0.02f;
    b.panel(0.0f, signY, signZ, 0.90f, 0.28f, 0.04f, open ? DECOR_MAT_SCREEN : DECOR_MAT_CAUTION);
    b.panel(0.0f, signY, -signZ, 0.90f, 0.28f, 0.04f, open ? DECOR_MAT_SCREEN : DECOR_MAT_CAUTION);
}

inline void buildSwitchModel(std::vector<float>& v, const Vec3& pos, bool on) {
    DecorBuilder b(v, pos.x, pos.z, 0.0f, 1.0f);
    b.box(0.0f, 0.00f, 0.0f, 0.50f, 0.08f, 0.50f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.08f, 0.0f, 0.16f, 0.92f, 0.16f, DECOR_MAT_METAL);
    b.box(0.0f, 1.00f, 0.0f, 0.36f, 0.30f, 0.20f, DECOR_MAT_CAUTION);
    b.box(on ? 0.09f : -0.09f, 1.30f, 0.0f, 0.05f, 0.16f, 0.05f, DECOR_MAT_ORANGE);
    b.panel(0.0f, 1.10f, 0.105f, 0.14f, 0.10f, 0.01f, on ? DECOR_MAT_SCREEN : DECOR_MAT_RUBBER);
}

// Sleeves around the square pillar: slightly larger than it, so they wrap it cleanly.
inline void buildPillarTrimModel(std::vector<float>& v, const Vec3& pos, bool parking) {
    DecorBuilder b(v, pos.x, pos.z, 0.0f, 1.0f);
    if (parking) {
        b.box(0.0f, 0.00f, 0.0f, PILLAR_SIZE + 0.20f, 0.15f, PILLAR_SIZE + 0.20f, DECOR_MAT_PLASTER);
        b.box(0.0f, 0.50f, 0.0f, PILLAR_SIZE + 0.03f, 0.60f, PILLAR_SIZE + 0.03f, DECOR_MAT_CAUTION);
    } else {
        b.box(0.0f, 0.00f, 0.0f, PILLAR_SIZE + 0.12f, 0.12f, PILLAR_SIZE + 0.12f, DECOR_MAT_TRIM);
        b.box(0.0f, WH - 0.10f, 0.0f, PILLAR_SIZE + 0.12f, 0.10f, PILLAR_SIZE + 0.12f, DECOR_MAT_TRIM);
    }
}

inline void buildPoiOfficeSet(DecorBuilder& b) {
    DecorBuilder desk = b.sub(-0.45f, 0.37f, 0.0f);
    buildDeskModel(desk);
    // pin board above the desk
    b.box(-0.45f, 1.20f, 0.02f, 1.10f, 0.70f, 0.03f, DECOR_MAT_TRIM);
    b.box(-0.45f, 1.24f, 0.04f, 1.02f, 0.62f, 0.01f, DECOR_MAT_CARDBOARD);
    b.panel(-0.72f, 1.42f, 0.05f, 0.21f, 0.29f, 0.006f, DECOR_MAT_WHITE);
    b.panel(-0.30f, 1.36f, 0.05f, 0.21f, 0.29f, 0.006f, DECOR_MAT_WHITE);
    // bookshelf: back board on the wall, two sides, five shelves
    const float sx = 0.85f;
    b.box(sx, 0.00f, 0.035f, 0.74f, 1.90f, 0.03f, DECOR_MAT_WOOD);
    b.box(sx - 0.385f, 0.00f, 0.19f, 0.03f, 1.90f, 0.34f, DECOR_MAT_WOOD);
    b.box(sx + 0.385f, 0.00f, 0.19f, 0.03f, 1.90f, 0.34f, DECOR_MAT_WOOD);
    const float shelfY[5] = {0.05f, 0.50f, 0.95f, 1.40f, 1.87f};
    for (int i = 0; i < 5; i++) {
        b.box(sx, shelfY[i], 0.205f, 0.74f, 0.03f, 0.31f, DECOR_MAT_WOOD);
    }
    b.box(sx - 0.14f, 0.08f, 0.20f, 0.40f, 0.30f, 0.24f, DECOR_MAT_CARDBOARD);
    b.box(sx + 0.12f, 0.53f, 0.20f, 0.34f, 0.28f, 0.22f, DECOR_MAT_DARK_METAL);
    b.box(sx - 0.20f, 0.98f, 0.20f, 0.26f, 0.30f, 0.22f, DECOR_MAT_FABRIC);
    b.box(sx + 0.10f, 0.98f, 0.20f, 0.28f, 0.24f, 0.22f, DECOR_MAT_CARDBOARD);
    b.box(sx, 1.43f, 0.20f, 0.50f, 0.22f, 0.24f, DECOR_MAT_WHITE);
}

inline void buildPoiServerSet(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.47f, 1.98f, 0.06f, 0.90f, DECOR_MAT_METAL);
    for (int i = -1; i <= 1; i++) {
        float x = 0.66f * (float)i;
        b.box(x, 0.06f, 0.47f, 0.62f, 1.94f, 0.86f, DECOR_MAT_DARK_METAL);
        b.panel(x, 0.30f, 0.905f, 0.50f, 1.50f, 0.01f, DECOR_MAT_SCREEN);
    }
    b.box(0.0f, 2.00f, 0.47f, 1.98f, 0.05f, 0.30f, DECOR_MAT_METAL);
}

inline void buildPoiStorageSet(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        b.box(1.15f * (float)sx, 0.00f, 0.06f, 0.05f, 2.00f, 0.05f, DECOR_MAT_METAL);
        b.box(1.15f * (float)sx, 0.00f, 0.54f, 0.05f, 2.00f, 0.05f, DECOR_MAT_METAL);
    }
    const float shelfY[4] = {0.15f, 0.75f, 1.35f, 1.95f};
    for (int i = 0; i < 4; i++) {
        b.box(0.0f, shelfY[i], 0.30f, 2.25f, 0.04f, 0.53f, DECOR_MAT_METAL);
    }
    b.box(-0.70f, 0.19f, 0.30f, 0.50f, 0.38f, 0.40f, DECOR_MAT_CARDBOARD);
    b.box(-0.10f, 0.19f, 0.30f, 0.45f, 0.30f, 0.42f, DECOR_MAT_CARDBOARD);
    b.box(0.65f, 0.79f, 0.30f, 0.60f, 0.40f, 0.44f, DECOR_MAT_CARDBOARD);
    b.cylinder(-0.60f, 0.79f, 0.30f, 0.16f, 0.40f, DECOR_MAT_DRUM, 10);
    b.box(0.20f, 1.39f, 0.30f, 0.50f, 0.30f, 0.40f, DECOR_MAT_WOOD);
    b.box(-0.75f, 1.39f, 0.30f, 0.40f, 0.25f, 0.36f, DECOR_MAT_CARDBOARD);
}

inline void buildPoiRestroomSet(DecorBuilder& b) {
    b.box(0.0f, 0.00f, 0.25f, 2.00f, 0.10f, 0.44f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.10f, 0.27f, 2.06f, 0.68f, 0.50f, DECOR_MAT_WOOD);
    b.box(0.0f, 0.78f, 0.29f, 2.10f, 0.08f, 0.54f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.86f, 0.03f, 2.10f, 0.20f, 0.02f, DECOR_MAT_WHITE);
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.50f * (float)side;
        b.box(x, 0.86f, 0.32f, 0.46f, 0.03f, 0.34f, DECOR_MAT_WHITE);
        b.box(x, 0.89f, 0.32f, 0.38f, 0.004f, 0.26f, DECOR_MAT_RUBBER);
        b.box(x, 0.86f, 0.09f, 0.04f, 0.20f, 0.04f, DECOR_MAT_METAL);
        b.box(x, 1.03f, 0.16f, 0.03f, 0.03f, 0.10f, DECOR_MAT_METAL);
    }
    // mirror in a frame, flat on the wall
    b.box(0.0f, 1.20f, 0.02f, 1.90f, 0.80f, 0.02f, DECOR_MAT_TRIM);
    b.box(0.0f, 1.24f, 0.035f, 1.82f, 0.72f, 0.01f, DECOR_MAT_METAL);
}

inline void buildMapPoiModel(std::vector<float>& v, const MapPoi& poi) {
    Vec3 anchor = mapPoiAnchor(poi);
    DecorBuilder b(v, anchor.x, anchor.z, mapPoiYaw(poi), 1.0f);
    if (poi.type == MAP_POI_OFFICE) buildPoiOfficeSet(b);
    else if (poi.type == MAP_POI_SERVER) buildPoiServerSet(b);
    else if (poi.type == MAP_POI_STORAGE) buildPoiStorageSet(b);
    else buildPoiRestroomSet(b);
}
