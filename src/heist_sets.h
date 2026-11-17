#pragma once
// Heist sets: puts the heist models into the world mesh, keeps that mesh in step with the
// run (gauge lamps, pallet boxes, portal alive / wrecked, broken walls) and makes the big
// pieces solid.

#include "heist_models.h"
#include "rig_models.h"
#include "heist_world.h"
#include "prop_models.h"

struct HeistSolid {
    Vec3 pos;
    float hx, hz;
};

inline int gHeistModelSig = -1;

inline int heistDrillLamps() {
    if (heist.quotaNeed <= 0) return 0;
    int lamps = heist.quotaHave * DRILL_GAUGE_LAMPS / heist.quotaNeed;
    return lamps > DRILL_GAUGE_LAMPS ? DRILL_GAUGE_LAMPS : lamps;
}

inline bool heistCanAffordExtraction() {
    return heist.cash >= extractionPrice(heistSquadSize());
}

// Stalls and machines turn their back to a neighbouring wall.
inline float heistBackToWallYaw(const Vec3& pos) {
    int wx = heistCellX(pos), wz = heistCellZ(pos);
    if (getCellWorld(wx, wz - 1) == 1) return 0.0f;
    if (getCellWorld(wx, wz + 1) == 1) return 3.1415927f;
    if (getCellWorld(wx - 1, wz) == 1) return 1.5707963f;
    if (getCellWorld(wx + 1, wz) == 1) return -1.5707963f;
    return 0.0f;
}

inline Vec3 heistDeskPos() {
    return Vec3(((float)heist.baseX - 1.0f) * CS + 0.55f, 0.0f, ((float)(heist.baseZ + GARAGE_FAR_Z) + 1.5f) * CS);
}

inline int collectHeistSolids(HeistSolid* out) {
    int n = 0;
    if (heist.hasKiosk) {
        bool turned = fabsf(sinf(heistBackToWallYaw(heist.kioskPos))) > 0.5f;
        out[n++] = {heist.kioskPos, turned ? KIOSK_HALF_Z : KIOSK_HALF_X, turned ? KIOSK_HALF_X : KIOSK_HALF_Z};
    }
    if (heist.hasGarage && heistStageIsSafe(heist.stage)) out[n++] = {heistDeskPos(), 0.40f, 0.75f};
    return n;
}

// The garage is the only place that does not look like the Backrooms: wood floor, white
// walls, a roller door. Lined tile by tile so the atlas texture keeps its scale.
inline void buildGarageShell(std::vector<float>& v) {
    DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
    const float tile = CS * 0.25f, rowH = WH / 3.0f, gap = 0.02f;
    float x0 = (float)(heist.baseX - GARAGE_HALF_W) * CS, x1 = (float)(heist.baseX + GARAGE_HALF_W + 1) * CS;
    float z0 = (float)(heist.baseZ + GARAGE_FAR_Z) * CS, z1 = (float)(heist.baseZ + GARAGE_NEAR_Z + 1) * CS;
    float doorX0 = (float)heist.baseX * CS, doorX1 = doorX0 + CS;
    for (float x = x0; x < x1 - 0.01f; x += tile) {
        for (float z = z0; z < z1 - 0.01f; z += tile) {
            b.quad(Vec3(x, 0.012f, z), Vec3(x, 0.012f, z + tile), Vec3(x + tile, 0.012f, z + tile), Vec3(x + tile, 0.012f, z), Vec3(0, 1, 0), DECOR_MAT_WOOD);
            float cy = WH - 0.015f;
            b.quad(Vec3(x, cy, z), Vec3(x + tile, cy, z), Vec3(x + tile, cy, z + tile), Vec3(x, cy, z + tile), Vec3(0, -1, 0), DECOR_MAT_CEILTILE);
        }
    }
    for (int row = 0; row < 3; row++) {
        float y0 = (float)row * rowH, y1 = y0 + rowH;
        for (float x = x0; x < x1 - 0.01f; x += tile) {
            b.quad(Vec3(x, y0, z0 + gap), Vec3(x + tile, y0, z0 + gap), Vec3(x + tile, y1, z0 + gap), Vec3(x, y1, z0 + gap), Vec3(0, 0, 1), DECOR_MAT_WHITE);
            bool doorway = x + tile * 0.5f > doorX0 && x + tile * 0.5f < doorX1;
            if (doorway) continue;
            b.quad(Vec3(x + tile, y0, z1 - gap), Vec3(x, y0, z1 - gap), Vec3(x, y1, z1 - gap), Vec3(x + tile, y1, z1 - gap), Vec3(0, 0, -1), DECOR_MAT_WHITE);
        }
        for (float z = z0; z < z1 - 0.01f; z += tile) {
            b.quad(Vec3(x0 + gap, y0, z + tile), Vec3(x0 + gap, y0, z), Vec3(x0 + gap, y1, z), Vec3(x0 + gap, y1, z + tile), Vec3(1, 0, 0), DECOR_MAT_WHITE);
            b.quad(Vec3(x1 - gap, y0, z), Vec3(x1 - gap, y0, z + tile), Vec3(x1 - gap, y1, z + tile), Vec3(x1 - gap, y1, z), Vec3(-1, 0, 0), DECOR_MAT_WHITE);
        }
    }
    // roller door to the street, on the far side from the portal... it stays shut
    float midZ = (z0 + z1) * 0.5f;
    DecorBuilder door(v, x1 - 0.06f, midZ + CS * 0.5f, -1.5707963f, 1.0f);
    for (int i = 0; i < 9; i++) door.box(0.0f, (float)i * 0.34f, 0.0f, 4.20f, 0.31f, 0.05f, DECOR_MAT_METAL);
    door.box(0.0f, 3.06f, 0.0f, 4.50f, 0.26f, 0.20f, DECOR_MAT_DARK_METAL);
    door.decal(0.0f, 0.016f, 0.55f, 4.20f, 0.30f, DECOR_MAT_CAUTION);
}

inline void buildGarageFurniture(std::vector<float>& v) {
    float z0 = (float)(heist.baseZ + GARAGE_FAR_Z) * CS;
    float x0 = (float)(heist.baseX - GARAGE_HALF_W) * CS;
    float midX = ((float)heist.baseX + 0.5f) * CS;

    DecorBuilder board(v, midX, z0 + 0.07f, 0.0f, 1.0f);
    buildPitchBoardModel(board);

    Vec3 deskPos = heistDeskPos();
    DecorBuilder desk(v, deskPos.x, deskPos.z, 1.5707963f, 1.0f);
    buildDeskModel(desk);
    desk.panel(0.0f, 0.78f, -0.10f, 0.52f, 0.36f, 0.05f, DECOR_MAT_SCREEN);
    desk.box(0.0f, 0.74f, -0.10f, 0.16f, 0.05f, 0.14f, DECOR_MAT_DARK_METAL);
    DecorBuilder chair(v, deskPos.x + 0.85f, deskPos.z + 0.10f, -1.2f, 1.0f);
    buildChairModel(chair);

    // pizza boxes, a pallet of flattened cartons and a crate stack: day one of a startup
    DecorBuilder corner(v, x0 + 0.9f, z0 + 0.9f, 0.4f, 1.0f);
    buildBoxPalletModel(corner);
    DecorBuilder crates(v, x0 + CS * 3.0f - 0.8f, z0 + 0.8f, 0.2f, 1.0f);
    buildCrateStackModel(crates);
    DecorBuilder pizza(v, deskPos.x + 0.10f, deskPos.z - 1.30f, 0.3f, 1.0f);
    for (int i = 0; i < 3; i++) pizza.box(0.02f * (float)i, (float)i * 0.045f, 0.0f, 0.40f, 0.04f, 0.40f, DECOR_MAT_CARDBOARD);

    DecorBuilder pad(v, heist.padPos.x, heist.padPos.z, 0.0f, 1.0f);
    buildPalletPadModel(pad, heist.tutDelivered, HEIST_TUTORIAL_LOOT);
}

inline void buildCrackedWalls(std::vector<float>& v) {
    const int dx[4] = {1, -1, 0, 0};
    const int dz[4] = {0, 0, 1, -1};
    for (const auto& entry : chunks) {
        const Chunk& c = entry.second;
        if (abs(c.cx - playerChunkX) > VIEW_CHUNKS || abs(c.cz - playerChunkZ) > VIEW_CHUNKS) continue;
        for (int lx = 0; lx < CHUNK_SIZE; lx++) {
            for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                if (c.cells[lx][lz] != 1) continue;
                int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
                if (!isCrackedWall(wx, wz)) continue;
                bool glint = heistIsTutorialCrack(wx, wz) || heistCrackHasCache(wx, wz);
                for (int d = 0; d < 4; d++) {
                    if (getCellWorld(wx + dx[d], wz + dz[d]) != 0) continue;
                    Vec3 face = heistCellCenter(wx, wz);
                    face.x += (float)dx[d] * CS * 0.5f;
                    face.z += (float)dz[d] * CS * 0.5f;
                    DecorBuilder b(v, face.x, face.z, atan2f((float)dx[d], (float)dz[d]), 1.0f);
                    buildCrackFaceModel(b, hashCell(wx, wz, 0xFACEu + (unsigned)d), glint);
                }
            }
        }
    }
}

inline int heistModelSignature() {
    int sig = gRigCarvedStep & 15;
    sig = sig * 3 + (heistStageIsSafe(heist.stage) ? 0 : (heist.stage == HEIST_DRILLING ? 2 : 1));
    sig = sig * 5 + heist.tutDelivered;
    sig = sig * 2 + ((heist.hasKiosk && heistCanAffordExtraction()) ? 1 : 0);
    sig = sig * 32 + heist.breakCount;
    return sig;
}

inline void buildHeistModels(std::vector<float>& v) {
    // The rig itself moves, so it is drawn separately (rig_render.h). What stays behind is
    // the hole it came down through.
    if (!heist.hasGarage) {
        DecorBuilder hole(v, heist.rigStart.x, heist.rigStart.z, rigYaw(heist.rigDir), 1.0f);
        buildRigArrivalModel(hole, WH);
    }
    if (heist.hasKiosk) {
        DecorBuilder kiosk(v, heist.kioskPos.x, heist.kioskPos.z, heistBackToWallYaw(heist.kioskPos), 1.0f);
        buildKioskModel(kiosk, heistCanAffordExtraction());
    }
    if (heist.hasGarage) {
        DecorBuilder portal(v, heist.portalPos.x, heist.portalPos.z, 0.0f, 1.0f);
        if (heistStageIsSafe(heist.stage)) {
            buildGarageShell(v);
            buildGarageFurniture(v);
            buildPortalRigModel(portal);
        } else {
            buildPortalWreckModel(portal);
        }
    }
    buildCrackedWalls(v);
    gHeistModelSig = heistModelSignature();
}

inline void refreshHeistModels() {
    if (heistModelSignature() != gHeistModelSig) buildGeom();
}

// A piece the player already overlaps (spawned next to it) never traps them.
inline bool collideHeistSolids(float x, float z, float r) {
    if (rigCollide(x, z, r) && !rigCollide(cam.pos.x, cam.pos.z, r)) return true;
    HeistSolid solids[4];
    int n = collectHeistSolids(solids);
    for (int i = 0; i < n; i++) {
        const HeistSolid& s = solids[i];
        if (fabsf(x - s.pos.x) >= s.hx + r || fabsf(z - s.pos.z) >= s.hz + r) continue;
        if (fabsf(cam.pos.x - s.pos.x) < s.hx + r && fabsf(cam.pos.z - s.pos.z) < s.hz + r) continue;
        return true;
    }
    return false;
}

