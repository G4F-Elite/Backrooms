#pragma once

#include "session.h"

inline bool isSmileSilenceActive() {
    return smileEvent.corridorActive;
}

inline void buildSmileCorridorAt(int startWX, int startWZ, int len, int dirX, int dirZ) {
    for (int i = -2; i <= len + 2; i++) {
        for (int s = -2; s <= 2; s++) {
            int wx = startWX + dirX * i + dirZ * s;
            int wz = startWZ + dirZ * i - dirX * s;
            generateChunk(wx >= 0 ? wx / CHUNK_SIZE : (wx - CHUNK_SIZE + 1) / CHUNK_SIZE,
                          wz >= 0 ? wz / CHUNK_SIZE : (wz - CHUNK_SIZE + 1) / CHUNK_SIZE);
            setCellWorld(wx, wz, (abs(s) <= 1) ? 0 : 1);
        }
    }
}

inline void triggerSmileCorridor() {
    smileEvent.eyeActive = false;
    smileEvent.corridorActive = true;
    smileEvent.corridorTime = 0.0f;
    smileEvent.returnPos = cam.pos;
    int baseWX = (int)floorf(cam.pos.x / CS);
    int baseWZ = (int)floorf(cam.pos.z / CS);
    int dirX = (mSin(cam.yaw) > 0.0f) ? 1 : -1;
    int dirZ = (mCos(cam.yaw) > 0.0f) ? 1 : -1;
    int startWX = baseWX + dirX * 22;
    int startWZ = baseWZ + dirZ * 22;
    int len = 18;
    buildSmileCorridorAt(startWX, startWZ, len, dirX, dirZ);
    smileEvent.corridorEnd = Vec3((startWX + dirX * (len - 1) + 0.5f) * CS, PH, (startWZ + dirZ * (len - 1) + 0.5f) * CS);
    cam.pos = Vec3((startWX + 0.5f) * CS, PH, (startWZ + 0.5f) * CS);
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    updateMapContent(playerChunkX, playerChunkZ);
    buildGeom();
    setTrapStatus("EYES LOCKED. FOLLOW THE RED CORRIDOR.");
    triggerScare();
}

inline void updateSmileEvent() {
    if (smileEvent.corridorActive) {
        smileEvent.corridorTime += dTime;
        Vec3 d = cam.pos - smileEvent.corridorEnd;
        d.y = 0.0f;
        if (d.len() < 2.2f || smileEvent.corridorTime > 35.0f) {
            cam.pos = Vec3(smileEvent.returnPos.x, PH, smileEvent.returnPos.z);
            smileEvent.corridorActive = false;
            smileEvent.nextSpawnTimer = 34.0f + (float)(rng()%20);
            updateVisibleChunks(cam.pos.x, cam.pos.z);
            updateLightsAndPillars(playerChunkX, playerChunkZ);
            updateMapContent(playerChunkX, playerChunkZ);
            buildGeom();
            setEchoStatus("ECHO RESTORED");
            triggerScare();
        }
        return;
    }

    smileEvent.nextSpawnTimer -= dTime;
    if (!smileEvent.eyeActive && smileEvent.nextSpawnTimer <= 0.0f) {
        Vec3 fwd(mSin(cam.yaw), 0, mCos(cam.yaw));
        Vec3 right(mCos(cam.yaw), 0, -mSin(cam.yaw));
        Vec3 cand = cam.pos + fwd * (8.0f + (float)(rng()%8)) + right * (((rng()%2)==0)?-6.0f:6.0f);
        int wx = (int)floorf(cand.x / CS);
        int wz = (int)floorf(cand.z / CS);
        if (getCellWorld(wx, wz) == 0) {
            smileEvent.eyeActive = true;
            smileEvent.eyePos = Vec3(cand.x, PH + 0.3f, cand.z);
            smileEvent.eyeLife = 14.0f;
            smileEvent.eyeLookTime = 0.0f;
            setEchoStatus("EYES AT THE EDGE OF VISION");
        } else {
            smileEvent.nextSpawnTimer = 10.0f + (float)(rng()%10);
        }
    }

    if (!smileEvent.eyeActive) return;
    smileEvent.eyeLife -= dTime;
    if (smileEvent.eyeLife <= 0.0f) {
        smileEvent.eyeActive = false;
        smileEvent.nextSpawnTimer = 20.0f + (float)(rng()%20);
        return;
    }

    Vec3 toEye = smileEvent.eyePos - cam.pos;
    float dist = toEye.len();
    if (dist < 0.001f) return;
    toEye = toEye * (1.0f / dist);
    Vec3 fwd(mSin(cam.yaw) * mCos(cam.pitch), mSin(cam.pitch), mCos(cam.yaw) * mCos(cam.pitch));
    float lookDot = fwd.dot(toEye);
    if (lookDot > 0.93f && dist < 20.0f) {
        smileEvent.eyeLookTime += dTime;
        if (smileEvent.eyeLookTime > 0.35f) {
            triggerSmileCorridor();
        }
    } else {
        smileEvent.eyeLookTime -= dTime * 0.6f;
        if (smileEvent.eyeLookTime < 0.0f) smileEvent.eyeLookTime = 0.0f;
    }
}
