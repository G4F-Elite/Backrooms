#pragma once

#include "coop.h"
#include "map_content.h"
#include "device_models.h"
inline void mkClosedBox(std::vector<float>& v, float cx, float y0, float cz, float sx, float sy, float sz) {
    float hx = sx * 0.5f;
    float hz = sz * 0.5f;
    float y1 = y0 + sy;
    const float tile = 2.4f;
    float uvX = sx / tile;
    float uvY = sy / tile;
    float uvZ = sz / tile;
    if (uvX < 0.6f) uvX = 0.6f;
    if (uvY < 0.6f) uvY = 0.6f;
    if (uvZ < 0.6f) uvZ = 0.6f;
    auto pushQuad = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, float uMax, float vMax) {
        float vv[] = {
            a.x, a.y, a.z, 0, 0, n.x, n.y, n.z,
            b.x, b.y, b.z, uMax, 0, n.x, n.y, n.z,
            c.x, c.y, c.z, uMax, vMax, n.x, n.y, n.z,
            a.x, a.y, a.z, 0, 0, n.x, n.y, n.z,
            c.x, c.y, c.z, uMax, vMax, n.x, n.y, n.z,
            d.x, d.y, d.z, 0, vMax, n.x, n.y, n.z
        };
        v.insert(v.end(), vv, vv + 48);
    };
    pushQuad(
        Vec3(cx - hx, y0, cz + hz), Vec3(cx + hx, y0, cz + hz),
        Vec3(cx + hx, y1, cz + hz), Vec3(cx - hx, y1, cz + hz),
        Vec3(0, 0, 1), uvX, uvY
    );
    pushQuad(
        Vec3(cx + hx, y0, cz - hz), Vec3(cx - hx, y0, cz - hz),
        Vec3(cx - hx, y1, cz - hz), Vec3(cx + hx, y1, cz - hz),
        Vec3(0, 0, -1), uvX, uvY
    );
    pushQuad(
        Vec3(cx + hx, y0, cz + hz), Vec3(cx + hx, y0, cz - hz),
        Vec3(cx + hx, y1, cz - hz), Vec3(cx + hx, y1, cz + hz),
        Vec3(1, 0, 0), uvZ, uvY
    );
    pushQuad(
        Vec3(cx - hx, y0, cz - hz), Vec3(cx - hx, y0, cz + hz),
        Vec3(cx - hx, y1, cz + hz), Vec3(cx - hx, y1, cz - hz),
        Vec3(-1, 0, 0), uvZ, uvY
    );
    pushQuad(
        Vec3(cx - hx, y1, cz + hz), Vec3(cx + hx, y1, cz + hz),
        Vec3(cx + hx, y1, cz - hz), Vec3(cx - hx, y1, cz - hz),
        Vec3(0, 1, 0), uvX, uvZ
    );
    pushQuad(
        Vec3(cx - hx, y0, cz - hz), Vec3(cx + hx, y0, cz - hz),
        Vec3(cx + hx, y0, cz + hz), Vec3(cx - hx, y0, cz + hz),
        Vec3(0, -1, 0), uvX, uvZ
    );
}

void buildGeom(){
    std::vector<float>wv,fv,cv,pv,lv,lvOff,dv;
    int pcx=playerChunkX,pcz=playerChunkZ;
    for(int dcx=-VIEW_CHUNKS;dcx<=VIEW_CHUNKS;dcx++){
        for(int dcz=-VIEW_CHUNKS;dcz<=VIEW_CHUNKS;dcz++){
            auto it=chunks.find(chunkKey(pcx+dcx,pcz+dcz));
            if(it==chunks.end())continue;
            for(int lx=0;lx<CHUNK_SIZE;lx++){
                for(int lz=0;lz<CHUNK_SIZE;lz++){
                    int wx=(pcx+dcx)*CHUNK_SIZE+lx,wz=(pcz+dcz)*CHUNK_SIZE+lz;
                    if(it->second.cells[lx][lz]!=0)continue;
                    float px=wx*CS,pz=wz*CS;
                    const float uvFloor = 1.0f;
                    const float uvCeil = 1.0f;
                    bool hasHole = isFloorHoleCell(wx,wz) || isAbyssCell(wx,wz);
                    if(!hasHole){
                        float fl[]={px,0,pz,0,0,0,1,0,px,0,pz+CS,0,uvFloor,0,1,0,px+CS,0,pz+CS,uvFloor,uvFloor,0,1,0,
                                   px,0,pz,0,0,0,1,0,px+CS,0,pz+CS,uvFloor,uvFloor,0,1,0,px+CS,0,pz,uvFloor,0,0,1,0};
                        for(int i=0;i<48;i++)fv.push_back(fl[i]);
                    }else{
                        const float shaftDepth = 30.0f;
                        bool leftSolid = getCellWorld(wx-1,wz)==1 || (!isFloorHoleCell(wx-1,wz) && !isAbyssCell(wx-1,wz) && getCellWorld(wx-1,wz)==0);
                        bool rightSolid = getCellWorld(wx+1,wz)==1 || (!isFloorHoleCell(wx+1,wz) && !isAbyssCell(wx+1,wz) && getCellWorld(wx+1,wz)==0);
                        bool backSolid = getCellWorld(wx,wz-1)==1 || (!isFloorHoleCell(wx,wz-1) && !isAbyssCell(wx,wz-1) && getCellWorld(wx,wz-1)==0);
                        bool frontSolid = getCellWorld(wx,wz+1)==1 || (!isFloorHoleCell(wx,wz+1) && !isAbyssCell(wx,wz+1) && getCellWorld(wx,wz+1)==0);
                        if(leftSolid) mkShaftWall(wv,px,pz,0,CS,0,shaftDepth,CS);
                        if(rightSolid) mkShaftWall(wv,px+CS,pz+CS,0,-CS,0,shaftDepth,CS);
                        if(backSolid) mkShaftWall(wv,px+CS,pz,-CS,0,0,shaftDepth,CS);
                        if(frontSolid) mkShaftWall(wv,px,pz+CS,CS,0,0,shaftDepth,CS);
                    }
                    float cl[]={px,WH,pz,0,0,0,-1,0,px,WH,pz+CS,0,uvCeil,0,-1,0,px+CS,WH,pz+CS,uvCeil,uvCeil,0,-1,0,
                               px,WH,pz,0,0,0,-1,0,px+CS,WH,pz+CS,uvCeil,uvCeil,0,-1,0,px+CS,WH,pz,uvCeil,0,0,-1,0};
                    for(int i=0;i<48;i++)cv.push_back(cl[i]);
                    bool wallL = getCellWorld(wx-1,wz)==1;
                    bool wallR = getCellWorld(wx+1,wz)==1;
                    bool wallB = getCellWorld(wx,wz-1)==1;
                    bool wallF = getCellWorld(wx,wz+1)==1;
                    if(wallL)mkWall(wv,px,pz,0,CS,WH,CS,WH);
                    if(wallR)mkWall(wv,px+CS,pz+CS,0,-CS,WH,CS,WH);
                    if(wallB)mkWall(wv,px+CS,pz,-CS,0,WH,CS,WH);
                    if(wallF)mkWall(wv,px,pz+CS,CS,0,WH,CS,WH);

                    if(!hasHole){
                        bool corridorZ = wallL && wallR && !wallB && !wallF;
                        bool corridorX = wallB && wallF && !wallL && !wallR;
                        float cxCell = px + CS * 0.5f;
                        float czCell = pz + CS * 0.5f;
                        unsigned int doorHash = (unsigned int)(wx * 73856093u) ^ (unsigned int)(wz * 19349663u) ^ (worldSeed * 83492791u);
                        bool spawnDoorway = (doorHash % 100u) < 7u;
                        if(spawnDoorway && (corridorZ || corridorX)){
                            float openingH = WH * 0.62f;
                            float postH = openingH;
                            float postW = CS * 0.06f;
                            float frameT = CS * 0.10f;
                            float openingHalf = CS * 0.23f;
                            float edgeHalf = CS * 0.49f;
                            float sideFillW = (edgeHalf - openingHalf);
                            float sideFillCenter = (edgeHalf + openingHalf) * 0.5f;
                            float topFillH = WH - openingH;
                            float wallFillT = frameT * 0.92f;
                            if(corridorZ){
                                mkBox(dv, cxCell - openingHalf, 0.0f, czCell, postW, postH, frameT);
                                mkBox(dv, cxCell + openingHalf, 0.0f, czCell, postW, postH, frameT);
                                mkBox(dv, cxCell, openingH, czCell, openingHalf * 2.0f + postW * 2.0f, CS * 0.08f, frameT);
                                mkBox(dv, cxCell - sideFillCenter, 0.0f, czCell, sideFillW, WH, wallFillT);
                                mkBox(dv, cxCell + sideFillCenter, 0.0f, czCell, sideFillW, WH, wallFillT);
                                mkBox(dv, cxCell, openingH, czCell, openingHalf * 2.0f, topFillH, wallFillT);
                            }else{
                                mkBox(dv, cxCell, 0.0f, czCell - openingHalf, frameT, postH, postW);
                                mkBox(dv, cxCell, 0.0f, czCell + openingHalf, frameT, postH, postW);
                                mkBox(dv, cxCell, openingH, czCell, frameT, CS * 0.08f, openingHalf * 2.0f + postW * 2.0f);
                                mkBox(dv, cxCell, 0.0f, czCell - sideFillCenter, wallFillT, WH, sideFillW);
                                mkBox(dv, cxCell, 0.0f, czCell + sideFillCenter, wallFillT, WH, sideFillW);
                                mkBox(dv, cxCell, openingH, czCell, wallFillT, topFillH, openingHalf * 2.0f);
                            }
                        }

                        if(isLevelZero(gCurrentLevel) && (corridorZ || corridorX) && (doorHash % 100u) < 3u){
                            float rampY0 = 0.0f;
                            float segW = CS * 0.18f;
                            for(int step = 0; step < 5; step++){
                                float sy = rampY0 + (float)step * 0.24f;
                                if(corridorZ){
                                    float sx = px + CS * (0.16f + (float)step * 0.17f);
                                    mkBox(dv, sx, sy, czCell, segW, 0.10f, CS * 0.86f);
                                }else{
                                    float sz = pz + CS * (0.16f + (float)step * 0.17f);
                                    mkBox(dv, cxCell, sy, sz, CS * 0.86f, 0.10f, segW);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for(auto&p:pillars){
        mkPillar(pv,p.x,p.z,0.6f,WH);
        if(isParkingLevel(gCurrentLevel)){
            mkBox(dv, p.x, 0.0f, p.z, 0.90f, 0.10f, 0.90f);
            mkBox(dv, p.x, 0.32f, p.z, 0.78f, 0.08f, 0.78f);
            mkBox(dv, p.x, 0.62f, p.z, 0.66f, 0.08f, 0.66f);
        }
    }
    for(auto&pr:mapProps){
        if(pr.type==MAP_PROP_CRATE_STACK){
            float b = 1.05f * pr.scale;
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, b, 0.82f * pr.scale, b);
            mkBox(dv, pr.pos.x + 0.36f, 0.0f, pr.pos.z - 0.30f, 0.70f * pr.scale, 0.62f * pr.scale, 0.70f * pr.scale);
        }else if(pr.type==MAP_PROP_CONE_CLUSTER){
            float base = 0.42f * pr.scale;
            mkBox(dv, pr.pos.x - 0.35f, 0.0f, pr.pos.z, base, 0.45f * pr.scale, base);
            mkBox(dv, pr.pos.x + 0.28f, 0.0f, pr.pos.z + 0.25f, base, 0.42f * pr.scale, base);
            mkBox(dv, pr.pos.x + 0.10f, 0.0f, pr.pos.z - 0.32f, base, 0.38f * pr.scale, base);
        }else if(pr.type==MAP_PROP_BARRIER){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.55f * pr.scale, 0.74f * pr.scale, 0.42f * pr.scale);
        }else if(pr.type==MAP_PROP_CABLE_REEL){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 0.92f * pr.scale, 0.45f * pr.scale, 0.92f * pr.scale);
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 0.30f * pr.scale, 0.78f * pr.scale, 0.30f * pr.scale);
        }else if(pr.type==MAP_PROP_PUDDLE){
            mkFloorDecal(dv, pr.pos.x, 0.02f, pr.pos.z, 1.7f * pr.scale, 1.3f * pr.scale);
        }else if(pr.type==MAP_PROP_DESK){
            mkBox(dv, pr.pos.x, 0.72f * pr.scale, pr.pos.z, 1.05f * pr.scale, 0.14f * pr.scale, 0.70f * pr.scale);
            mkBox(dv, pr.pos.x - 0.42f * pr.scale, 0.0f, pr.pos.z - 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
            mkBox(dv, pr.pos.x + 0.42f * pr.scale, 0.0f, pr.pos.z - 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
            mkBox(dv, pr.pos.x - 0.42f * pr.scale, 0.0f, pr.pos.z + 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
            mkBox(dv, pr.pos.x + 0.42f * pr.scale, 0.0f, pr.pos.z + 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
        }else if(pr.type==MAP_PROP_CHAIR){
            mkBox(dv, pr.pos.x, 0.36f * pr.scale, pr.pos.z, 0.48f * pr.scale, 0.10f * pr.scale, 0.48f * pr.scale);
            mkBox(dv, pr.pos.x, 0.46f * pr.scale, pr.pos.z - 0.18f * pr.scale, 0.48f * pr.scale, 0.42f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x - 0.18f * pr.scale, 0.0f, pr.pos.z + 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x + 0.18f * pr.scale, 0.0f, pr.pos.z + 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x - 0.18f * pr.scale, 0.0f, pr.pos.z - 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x + 0.18f * pr.scale, 0.0f, pr.pos.z - 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
        }else if(pr.type==MAP_PROP_CABINET){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 0.64f * pr.scale, 1.55f * pr.scale, 0.52f * pr.scale);
            mkBox(dv, pr.pos.x, 1.52f * pr.scale, pr.pos.z, 0.70f * pr.scale, 0.08f * pr.scale, 0.58f * pr.scale);
        }else if(pr.type==MAP_PROP_PARTITION){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.35f * pr.scale, 1.22f * pr.scale, 0.12f * pr.scale);
        }else if(pr.type==MAP_PROP_BOX_PALLET){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.22f * pr.scale, 0.34f * pr.scale, 1.02f * pr.scale);
            mkBox(dv, pr.pos.x, 0.34f * pr.scale, pr.pos.z, 1.10f * pr.scale, 0.78f * pr.scale, 0.96f * pr.scale);
            mkBox(dv, pr.pos.x, 1.12f * pr.scale, pr.pos.z, 0.88f * pr.scale, 0.50f * pr.scale, 0.82f * pr.scale);
        }else if(pr.type==MAP_PROP_DRUM_STACK){
            mkBox(dv, pr.pos.x - 0.22f * pr.scale, 0.0f, pr.pos.z, 0.48f * pr.scale, 1.12f * pr.scale, 0.48f * pr.scale);
            mkBox(dv, pr.pos.x + 0.22f * pr.scale, 0.0f, pr.pos.z, 0.48f * pr.scale, 1.06f * pr.scale, 0.48f * pr.scale);
            mkBox(dv, pr.pos.x, 1.06f * pr.scale, pr.pos.z, 0.44f * pr.scale, 0.44f * pr.scale, 0.44f * pr.scale);
        }else if(pr.type==MAP_PROP_LOCKER_BANK){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.18f * pr.scale, 1.74f * pr.scale, 0.46f * pr.scale);
            mkBox(dv, pr.pos.x - 0.30f * pr.scale, 0.0f, pr.pos.z + 0.22f * pr.scale, 0.04f * pr.scale, 1.62f * pr.scale, 0.04f * pr.scale);
            mkBox(dv, pr.pos.x + 0.30f * pr.scale, 0.0f, pr.pos.z + 0.22f * pr.scale, 0.04f * pr.scale, 1.62f * pr.scale, 0.04f * pr.scale);
        }else{
            mkBox(dv, pr.pos.x - 0.42f, 0.0f, pr.pos.z + 0.24f, 0.68f * pr.scale, 0.35f * pr.scale, 0.68f * pr.scale);
            mkBox(dv, pr.pos.x + 0.28f, 0.0f, pr.pos.z - 0.22f, 0.56f * pr.scale, 0.28f * pr.scale, 0.56f * pr.scale);
        }
    }
    for(const auto& poi:mapPois){
        if(poi.type==MAP_POI_OFFICE){
            mkBox(dv, poi.pos.x, 0.0f, poi.pos.z, 1.00f, 0.22f, 1.00f);
            mkBox(dv, poi.pos.x, 0.22f, poi.pos.z - 0.32f, 0.92f, 0.62f, 0.10f);
            mkBox(dv, poi.pos.x - 0.52f, 0.0f, poi.pos.z + 0.42f, 0.62f, 0.84f, 0.40f);
            mkBox(dv, poi.pos.x + 0.52f, 0.0f, poi.pos.z + 0.42f, 0.62f, 0.84f, 0.40f);
            mkBox(dv, poi.pos.x, 0.84f, poi.pos.z + 0.12f, 0.52f, 0.08f, 0.18f);
            mkBox(dv, poi.pos.x - 0.22f, 0.0f, poi.pos.z - 0.68f, 0.18f, 0.64f, 0.18f);
            mkBox(dv, poi.pos.x + 0.22f, 0.0f, poi.pos.z - 0.68f, 0.18f, 0.64f, 0.18f);
        }else if(poi.type==MAP_POI_SERVER){
            mkBox(dv, poi.pos.x - 0.32f, 0.0f, poi.pos.z, 0.26f, 1.45f, 0.26f);
            mkBox(dv, poi.pos.x + 0.32f, 0.0f, poi.pos.z, 0.26f, 1.45f, 0.26f);
            mkBox(dv, poi.pos.x, 1.42f, poi.pos.z, 0.74f, 0.08f, 0.32f);
        }else if(poi.type==MAP_POI_STORAGE){
            mkBox(dv, poi.pos.x, 0.0f, poi.pos.z, 1.18f, 0.42f, 1.02f);
            mkBox(dv, poi.pos.x, 0.42f, poi.pos.z, 1.04f, 0.52f, 0.88f);
        }else{
            mkBox(dv, poi.pos.x, 0.0f, poi.pos.z, 0.82f, 0.20f, 0.82f);
            mkBox(dv, poi.pos.x, 0.20f, poi.pos.z, 0.54f, 0.12f, 0.54f);
        }
    }
    if(isParkingLevel(gCurrentLevel)){
        for(int dcx=-VIEW_CHUNKS;dcx<=VIEW_CHUNKS;dcx++){
            for(int dcz=-VIEW_CHUNKS;dcz<=VIEW_CHUNKS;dcz++){
                int bx = (pcx+dcx) * CHUNK_SIZE;
                int bz = (pcz+dcz) * CHUNK_SIZE;
                for(int row=2; row<CHUNK_SIZE-2; row+=3){
                    float lineZ = ((float)bz + (float)row + 0.5f) * CS;
                    float x0 = ((float)bx + 1.5f) * CS;
                    float x1 = ((float)bx + (float)CHUNK_SIZE - 1.5f) * CS;
                    float cx = (x0 + x1) * 0.5f;
                    float sx = x1 - x0;
                    mkFloorDecal(dv, cx, 0.018f, lineZ, sx, 0.36f);
                }
            }
        }
    }
    if(coop.initialized){
        for(int s=0;s<2;s++){
            const Vec3 sp = coop.switches[s];
            mkBox(dv, sp.x, 0.0f, sp.z, 0.84f, 0.22f, 0.84f);
            mkBox(dv, sp.x, 0.22f, sp.z, 0.20f, 0.82f, 0.20f);
            float leverX = sp.x + (coop.switchOn[s] ? 0.14f : -0.14f);
            mkBox(dv, leverX, 0.88f, sp.z, 0.28f, 0.10f, 0.10f);
        }
        bool storyExitReady = isStoryExitReady();
        bool doorOpenVisual = (multiState==MULTI_IN_GAME) ? coop.doorOpen : storyExitReady;
        const Vec3 dp = coop.doorPos;
        mkBox(dv, dp.x - CS * 0.58f, 0.0f, dp.z, 0.16f, 2.82f, 0.36f);
        mkBox(dv, dp.x + CS * 0.58f, 0.0f, dp.z, 0.16f, 2.82f, 0.36f);
        mkBox(dv, dp.x, 2.72f, dp.z, CS * 1.18f, 0.14f, 0.36f);
        if(!doorOpenVisual){
            mkBox(dv, dp.x, 0.0f, dp.z, CS * 1.06f, 2.62f, 0.20f);
        }else{
            mkBox(dv, dp.x, 0.0f, dp.z + 0.18f, CS * 1.06f, 0.10f, 0.54f);
        }
    }
    for(auto&l:lights){
        if(l.on)mkLight(lv,l.pos,l.sizeX,l.sizeZ);
        else mkLight(lvOff,l.pos,l.sizeX,l.sizeZ);
    }
    {
        std::vector<float> m;
        buildFlashlightModel(m);
        flashlightVC = (int)m.size()/8;
        if(flashlightVC>0) setupVAO(flashlightVAO, flashlightVBO, m, true);
    }
    {
        std::vector<float> m;
        buildScannerModel(m);
        scannerVC = (int)m.size()/8;
        if(scannerVC>0) setupVAO(scannerVAO, scannerVBO, m, true);
    }
    {
        std::vector<float> m;
        buildPlushToyModel(m);
        plushVC = (int)m.size()/8;
        if(plushVC>0) setupVAO(plushVAO, plushVBO, m, true);
    }
    {
        std::vector<float> m;
        buildBatteryModel(m);
        batteryVC = (int)m.size()/8;
        if(batteryVC>0) setupVAO(batteryVAO, batteryVBO, m, true);
    }
    wallVC=(int)wv.size()/8;floorVC=(int)fv.size()/8;ceilVC=(int)cv.size()/8;
    pillarVC=(int)pv.size()/8;decorVC=(int)dv.size()/8;lightVC=(int)lv.size()/5;lightOffVC=(int)lvOff.size()/5;
    setupVAO(wallVAO,wallVBO,wv,true);setupVAO(floorVAO,floorVBO,fv,true);
    setupVAO(ceilVAO,ceilVBO,cv,true);setupVAO(pillarVAO,pillarVBO,pv,true);
    if(decorVC>0)setupVAO(decorVAO,decorVBO,dv,true);
    setupVAO(lightVAO,lightVBO,lv,false);
    if(!lvOff.empty())setupVAO(lightOffVAO,lightOffVBO,lvOff,false);
    initQuad(quadVAO,quadVBO);lastBuildChunkX=pcx;lastBuildChunkZ=pcz;
}
void buildNotes(float tm){
    std::vector<float>nv;
    for(auto&n:storyMgr.notes){
        if(!n.active||n.collected)continue;
        mkNoteGlow(nv,n.pos,n.bobPhase);
    }
    noteVC=(int)nv.size()/8;
    if(noteVC>0)setupVAO(noteVAO,noteVBO,nv,true);
}
void trySpawnNote(int noteId){
    if(noteId>=12||storyMgr.notesCollected[noteId])return;
    if(multiState==MULTI_IN_GAME && !netMgr.isHost) return;
    Vec3 sp=findSpawnPos(cam.pos,12.0f);
    Vec3 d=sp-cam.pos;d.y=0;
    if(sqrtf(d.x*d.x+d.z*d.z)>8.0f){
        storyMgr.spawnNote(sp,noteId);
        lastSpawnedNote=noteId;
        if(multiState==MULTI_IN_GAME) netMgr.sendNoteSpawn(noteId, sp);
    }
}
void cleanupFarNotes(){
    for(auto&n:storyMgr.notes){
        if(!n.active||n.collected)continue;
        Vec3 d=n.pos-cam.pos;d.y=0;
        if(sqrtf(d.x*d.x+d.z*d.z)>80.0f){
            n.active=false;
            if(n.id==lastSpawnedNote)lastSpawnedNote=n.id-1;
        }
    }
}

void genWorld(){
    static int lastTextureLevel = -999;
    if(lastTextureLevel != gCurrentLevel){
        if(wallTex) glDeleteTextures(1, &wallTex);
        if(floorTex) glDeleteTextures(1, &floorTex);
        if(ceilTex) glDeleteTextures(1, &ceilTex);
        if(lampTex) glDeleteTextures(1, &lampTex);
        if(propTex) glDeleteTextures(1, &propTex);
        if(deviceTex) glDeleteTextures(1, &deviceTex);
        if(plushTex) glDeleteTextures(1, &plushTex);
        wallTex=genTex(0); floorTex=genTex(1); ceilTex=genTex(2); lampTex=genTex(4); propTex=genTex(5);
        deviceTex=genTex(6); plushTex=genTex(7);
        lastTextureLevel = gCurrentLevel;
    }

    if(multiState==MULTI_IN_GAME && !netMgr.isHost){
        worldSeed=netMgr.worldSeed;
    }else{
        worldSeed=(unsigned int)time(nullptr);
        if(multiState==MULTI_IN_GAME) netMgr.worldSeed = worldSeed;
    }
    
    chunks.clear();lights.clear();pillars.clear();mapProps.clear();mapPois.clear();
    g_lightStates.clear();
    updateVisibleChunks(0,0);
    updateLightsAndPillars(0,0);
    updateMapContent(0,0);
    Vec3 sp=findSafeSpawn();
    Vec3 coopBase = sp;
    
    if(multiState==MULTI_IN_GAME){
        if(netMgr.isHost){
            netMgr.spawnPos=sp;
        }else{
            sp=netMgr.spawnPos;
            coopBase = netMgr.spawnPos;
        }
        sp.x+=netMgr.myId*1.5f;
    }
    
    cam.pos=Vec3(sp.x,PH,sp.z);cam.yaw=cam.pitch=0;
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    entityMgr.reset();storyMgr.init();
    initTrapCorridor(sp);

    // Spawn abyss location away from player spawn
    {
        int spawnWX = (int)floorf(sp.x / CS);
        int spawnWZ = (int)floorf(sp.z / CS);
        abyss.radius = 3 + (int)(rng() % 3); // radius 3-5 cells
        int attempts = 0;
        abyss.active = false;
        while(attempts < 60 && !abyss.active){
            attempts++;
            int offX = 20 + (int)(rng() % 20) - 10; // 10-30 cells away
            int offZ = 20 + (int)(rng() % 20) - 10;
            if(rng() % 2) offX = -offX;
            if(rng() % 2) offZ = -offZ;
            int cx = spawnWX + offX;
            int cz = spawnWZ + offZ;
            // Verify center and some cells around it are open
            bool valid = true;
            for(int dx = -1; dx <= 1 && valid; dx++)
                for(int dz = -1; dz <= 1 && valid; dz++)
                    if(getCellWorld(cx+dx, cz+dz) != 0) valid = false;
            if(!valid) continue;
            abyss.centerX = cx;
            abyss.centerZ = cz;
            abyss.active = true;
            // Carve out the abyss area (ensure open cells)
            for(int dx = -abyss.radius - 1; dx <= abyss.radius + 1; dx++){
                for(int dz = -abyss.radius - 1; dz <= abyss.radius + 1; dz++){
                    if(dx*dx + dz*dz <= (abyss.radius+1)*(abyss.radius+1)){
                        setCellWorld(cx+dx, cz+dz, 0);
                    }
                }
            }
        }
    }

    resetPlayerInterpolation();
    initCoopObjectives(coopBase);
    resetVoidShiftState(cam.pos, coop.doorPos);
    {
        Vec3 d = cam.pos - coop.doorPos;
        d.y = 0.0f;
        if(d.len() < 8.0f){
            Vec3 alt = findSpawnPos(coop.doorPos, 14.0f);
            cam.pos = Vec3(alt.x, PH, alt.z);
        }
    }
    worldItems.clear();
    nextWorldItemId = 1;
    invBattery = 0;
    invPlush = 0;
    clearEchoSignal();
    echoSpawnTimer = 7.0f + (float)(rng()%6);
    echoStatusTimer = 0.0f;
    echoStatusText[0] = '\0';
    storyEchoAttuned = false;
    storyEchoAttunedCount = 0;
    trapStatusTimer = 0.0f;
    trapStatusText[0] = '\0';
    smileEvent = {false, Vec3(0,0,0), 0.0f, 0.0f, 24.0f + (float)(rng()%16), false, Vec3(0,0,0), Vec3(0,0,0), 0.0f};
    anomalyBlur = 0.0f;
    lightsOutTimer = falseDoorTimer = 0.0f;
    itemSpawnTimer = 6.0f;
    playerHealth=playerSanity=100; playerStamina=125;
    flashlightBattery=100;flashlightOn=false;isPlayerDead=false;
    snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: INCIDENT UNCONFIRMED");
    playerEscaped=false;
    flashlightShutdownBlinkActive = false;
    flashlightShutdownBlinkTimer = 0.0f;
    resetScareSystemState(scareState);
    entitySpawnTimer=30;survivalTime=0;reshuffleTimer=15;
    floorHoles.clear();
    playerFalling = false;
    fallVelocity = 0.0f;
    fallTimer = 0.0f;
    abyss = {};
    lastSpawnedNote=-1;noteSpawnTimer=8.0f;
}

#include "teleport_helpers.h"
