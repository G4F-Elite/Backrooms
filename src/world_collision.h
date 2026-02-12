#pragma once
// Door frame collision - split from world.h

inline bool collideDoorFrameAtCell(float x, float z, float r, int wx, int wz) {
    if (getCellWorld(wx, wz) != 0) return false;

    bool wallL = getCellWorld(wx - 1, wz) == 1;
    bool wallR = getCellWorld(wx + 1, wz) == 1;
    bool wallB = getCellWorld(wx, wz - 1) == 1;
    bool wallF = getCellWorld(wx, wz + 1) == 1;
    bool corridorZ = wallL && wallR && !wallB && !wallF;
    bool corridorX = wallB && wallF && !wallL && !wallR;
    if (!corridorZ && !corridorX) return false;

    unsigned int doorHash = (unsigned int)(wx * 73856093u) ^ (unsigned int)(wz * 19349663u) ^ (worldSeed * 83492791u);
    if ((doorHash % 100u) >= 7u) return false;

    float px = wx * CS;
    float pz = wz * CS;
    float cxCell = px + CS * 0.5f;
    float czCell = pz + CS * 0.5f;
    float openingHalf = CS * 0.23f;
    float edgeHalf = CS * 0.49f;
    float sideFillW = edgeHalf - openingHalf;
    float sideFillCenter = (edgeHalf + openingHalf) * 0.5f;
    float postW = CS * 0.06f;
    float frameT = CS * 0.10f;
    float wallFillT = frameT * 0.92f;
    float postHalfW = postW * 0.5f;
    float frameHalfT = frameT * 0.5f;

    if (corridorZ) {
        if (circleOverlapsAabb2D(x, z, r,
            cxCell - sideFillCenter - sideFillW * 0.5f, cxCell - sideFillCenter + sideFillW * 0.5f,
            czCell - wallFillT * 0.5f, czCell + wallFillT * 0.5f)) return true;
        if (circleOverlapsAabb2D(x, z, r,
            cxCell + sideFillCenter - sideFillW * 0.5f, cxCell + sideFillCenter + sideFillW * 0.5f,
            czCell - wallFillT * 0.5f, czCell + wallFillT * 0.5f)) return true;
        if (circleOverlapsAabb2D(x, z, r,
            cxCell - openingHalf - postHalfW, cxCell - openingHalf + postHalfW,
            czCell - frameHalfT, czCell + frameHalfT)) return true;
        if (circleOverlapsAabb2D(x, z, r,
            cxCell + openingHalf - postHalfW, cxCell + openingHalf + postHalfW,
            czCell - frameHalfT, czCell + frameHalfT)) return true;
        return false;
    }

    if (circleOverlapsAabb2D(x, z, r,
        cxCell - wallFillT * 0.5f, cxCell + wallFillT * 0.5f,
        czCell - sideFillCenter - sideFillW * 0.5f, czCell - sideFillCenter + sideFillW * 0.5f)) return true;
    if (circleOverlapsAabb2D(x, z, r,
        cxCell - wallFillT * 0.5f, cxCell + wallFillT * 0.5f,
        czCell + sideFillCenter - sideFillW * 0.5f, czCell + sideFillCenter + sideFillW * 0.5f)) return true;
    if (circleOverlapsAabb2D(x, z, r,
        cxCell - frameHalfT, cxCell + frameHalfT,
        czCell - openingHalf - postHalfW, czCell - openingHalf + postHalfW)) return true;
    if (circleOverlapsAabb2D(x, z, r,
        cxCell - frameHalfT, cxCell + frameHalfT,
        czCell + openingHalf - postHalfW, czCell + openingHalf + postHalfW)) return true;
    return false;
}
