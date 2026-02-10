#pragma once
// NOTE: included into main translation unit (game.cpp); relies on prior includes.

struct GLFWwindow;

inline Vec3 debugCursorSpawnPos(GLFWwindow* w){
    double cx = 0.0, cy = 0.0;
    glfwGetCursorPos(w, &cx, &cy);
    float nx = (2.0f * (float)cx / (float)winW) - 1.0f;
    float ny = 1.0f - (2.0f * (float)cy / (float)winH);
    float fov = 1.2f;
    float t = tanf(fov * 0.5f);
    float asp = (float)winW / (float)winH;
    Vec3 fwd(mSin(cam.yaw) * mCos(cam.pitch), mSin(cam.pitch), mCos(cam.yaw) * mCos(cam.pitch));
    Vec3 right(mCos(cam.yaw), 0, -mSin(cam.yaw));
    Vec3 up = right.cross(fwd).norm();
    Vec3 dir = (fwd + right * (nx * asp * t) + up * (ny * t)).norm();
    if (fabsf(dir.y) > 0.001f) {
        float hitT = -cam.pos.y / dir.y;
        if (hitT > 0.5f && hitT < 80.0f) {
            Vec3 hit = cam.pos + dir * hitT;
            if (!collideWorld(hit.x, hit.z, PR)) {
                return Vec3(hit.x, 0.0f, hit.z);
            }
        }
    }
    return findSpawnPos(cam.pos, 6.0f);
}

inline void executeDebugAction(int action){
    action = clampDebugActionIndex(action);
    bool canMutateWorld = (multiState!=MULTI_IN_GAME || netMgr.isHost);
    if(action==DEBUG_ACT_TOGGLE_FLY){
        debugTools.flyMode = !debugTools.flyMode;
        setTrapStatus(debugTools.flyMode ? "DEBUG: FLY ENABLED" : "DEBUG: FLY DISABLED");
        return;
    }
    if(action==DEBUG_ACT_TOGGLE_INFINITE_STAMINA){
        debugTools.infiniteStamina = !debugTools.infiniteStamina;
        setTrapStatus(debugTools.infiniteStamina ? "DEBUG: INFINITE STAMINA ON" : "DEBUG: INFINITE STAMINA OFF");
        return;
    }
    if(action==DEBUG_ACT_TP_NOTE){
        int target = -1;
        for(auto& n:storyMgr.notes){
            if(!n.active || n.collected) continue;
            target = n.id;
            break;
        }
        if(target < 0 && lastSpawnedNote < 11){
            trySpawnNote(lastSpawnedNote + 1);
            for(auto& n:storyMgr.notes){
                if(!n.active || n.collected) continue;
                target = n.id;
                break;
            }
        }
        if(target >= 0){
            Vec3 p = storyMgr.notes[target].pos;
            cam.pos = Vec3(p.x, PH, p.z);
            updateVisibleChunks(cam.pos.x, cam.pos.z);
            updateLightsAndPillars(playerChunkX,playerChunkZ);
            updateMapContent(playerChunkX,playerChunkZ);
            buildGeom();
            setEchoStatus("DEBUG: TELEPORTED TO NOTE");
        }
        return;
    }
    if(action==DEBUG_ACT_TP_ECHO){
        if(!echoSignal.active) spawnEchoSignal();
        cam.pos = Vec3(echoSignal.pos.x, PH, echoSignal.pos.z);
        updateVisibleChunks(cam.pos.x, cam.pos.z);
        updateLightsAndPillars(playerChunkX,playerChunkZ);
        updateMapContent(playerChunkX,playerChunkZ);
        buildGeom();
        setEchoStatus("DEBUG: TELEPORTED TO ECHO");
        return;
    }
    if(action==DEBUG_ACT_TP_EXIT){
        extern void teleportToExit();
        teleportToExit();
        setEchoStatus("DEBUG: TELEPORTED TO EXIT");
        return;
    }
    if(action==DEBUG_ACT_TRIGGER_EYE){
        if(smileEvent.corridorActive){
            setTrapStatus("DEBUG: EYE EVENT ALREADY ACTIVE");
            return;
        }
        if(!smileEvent.eyeActive){
            Vec3 fwd(mSin(cam.yaw), 0.0f, mCos(cam.yaw));
            Vec3 right(mCos(cam.yaw), 0.0f, -mSin(cam.yaw));
            Vec3 cand = cam.pos + fwd * 10.0f + right * (((rng()%2)==0) ? -4.5f : 4.5f);
            int wx = (int)floorf(cand.x / CS);
            int wz = (int)floorf(cand.z / CS);
            if(getCellWorld(wx, wz) != 0){
                cand = findSpawnPos(cam.pos, 8.0f);
            }
            smileEvent.eyePos = Vec3(cand.x, PH + 0.3f, cand.z);
            smileEvent.eyeLife = 18.0f;
            smileEvent.eyeLookTime = 0.0f;
            smileEvent.eyeActive = true;
            setEchoStatus("DEBUG: EYE SPAWNED");
        }else{
            triggerSmileCorridor();
            setEchoStatus("DEBUG: EYE CORRIDOR TRIGGERED");
        }
        return;
    }
    if(!canMutateWorld){
        setTrapStatus("DEBUG ACTION: HOST ONLY");
        return;
    }
    if(debugActionSpawnsEntity(action)){
        EntityType t = debugActionEntityType(action);
        if(multiState==MULTI_IN_GAME && !netMgr.isHost){
            int reqType = REQ_DEBUG_SPAWN_STALKER;
            if(t==ENTITY_CRAWLER) reqType = REQ_DEBUG_SPAWN_CRAWLER;
            else if(t==ENTITY_SHADOW) reqType = REQ_DEBUG_SPAWN_SHADOW;
            netMgr.sendInteractRequest(reqType, 0);
            setEchoStatus("DEBUG: SPAWN REQUEST SENT");
            return;
        }
        Vec3 sp = debugCursorSpawnPos(gWin);
        entityMgr.spawnEntity(t, sp, nullptr, 0, 0);
        setEchoStatus("DEBUG: ENTITY SPAWNED");
        return;
    }
    if(action==DEBUG_ACT_FORCE_HOLES){
        spawnFloorHoleEvent(cam.pos, floorHoleCountFromRoll((int)rng()), floorHoleDurationFromRoll((int)rng()));
        buildGeom();
        return;
    }
    if(action==DEBUG_ACT_FORCE_SUPPLY){
        applyRoamEvent(ROAM_SUPPLY_CACHE, playerChunkX, playerChunkZ, 10.0f);
        return;
    }
}
