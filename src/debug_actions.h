#pragma once
// NOTE: included into main translation unit (game.cpp); relies on prior includes.

struct GLFWwindow;

inline const char* debugActionRuntimeLabel(int idx){
    int a = clampDebugActionIndex(idx);
    if(a==DEBUG_ACT_TP_NOTE) return isLevelZero(gCurrentLevel) ? "TP L1 OBJECTIVE" : "TP L2 OBJECTIVE";
    if(a==DEBUG_ACT_TP_ECHO) return isLevelZero(gCurrentLevel) ? "TP L1 NPC" : "TP L2 NPC";
    if(a==DEBUG_ACT_TP_EXIT) return isLevelZero(gCurrentLevel) ? "TP L1 EXIT" : "TP L2 LIFT";
    if(a==DEBUG_ACT_SKIP_LEVEL){
        if(multiState==MULTI_IN_GAME) return isLevelZero(gCurrentLevel) ? "COMPLETE L1 CONTRACT" : "COMPLETE L2 CONTRACT";
        return isLevelZero(gCurrentLevel) ? "SKIP TO LEVEL 2" : "END RUN TO MENU";
    }
    if(a==DEBUG_ACT_TRIGGER_EYE) return isLevelZero(gCurrentLevel) ? "TRIGGER EYE EVENT" : "TRIGGER TOWMAN EVENT";
    return debugActionLabel(idx);
}

inline void debugTeleportTo(const Vec3& p, const char* msg){
    Vec3 tp = Vec3(p.x, PH, p.z);
    if(collideWorld(tp.x, tp.z, 0.40f)){
        Vec3 safe = findSpawnPos(p, 4.0f);
        tp = Vec3(safe.x, PH, safe.z);
    }
    cam.pos = tp;
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    buildGeom();
    if(msg) setEchoStatus(msg);
}

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
        Vec3 target = coop.doorPos;
        if(isLevelZero(gCurrentLevel)){
            target = coop.doorPos;
            for(int i=0;i<3;i++){
                if(level1NodeDone[i]) continue;
                target = level1Nodes[i];
                break;
            }
        }else if(!level2BatteryInstalled) target = level2BatteryNode;
        else if(level2FuseCount < 3){
            for(int i=0;i<3;i++) if(!level2FuseDone[i]){ target = level2FuseNodes[i]; break; }
        }else if(!level2AccessReady) target = level2AccessNode;
        else if(!level2HoldActive && !level2ContractComplete) target = level2LiftNode;
        debugTeleportTo(target, "DEBUG: TELEPORTED TO OBJECTIVE");
        return;
    }
    if(action==DEBUG_ACT_TP_ECHO){
        Vec3 target = npcCartographerPos;
        if(isLevelZero(gCurrentLevel)){
            if(npcDispatcherActive) target = npcDispatcherPhonePos;
            if(npcLostSurvivorActive && !npcLostSurvivorEscorted) target = npcLostSurvivorPos;
        }else{
            if(!level2CameraOnline) target = level2CameraNode;
            else if(!level2DroneReprogrammed) target = level2DroneNode;
            else if(npcDispatcherActive) target = npcDispatcherPhonePos;
        }
        debugTeleportTo(target, "DEBUG: TELEPORTED TO TARGET");
        return;
    }
    if(action==DEBUG_ACT_TP_EXIT){
        extern void teleportToExit();
        teleportToExit();
        setEchoStatus("DEBUG: TELEPORTED TO EXIT");
        return;
    }
    if(action==DEBUG_ACT_SKIP_LEVEL){
        if(multiState==MULTI_IN_GAME && !netMgr.isHost){
            setTrapStatus("DEBUG: SKIP LEVEL HOST ONLY");
            return;
        }
        if(multiState==MULTI_IN_GAME){
            if(isLevelZero(gCurrentLevel)){
                for(int i=0;i<3;i++) level1NodeDone[i] = true;
                level1HoldActive = false;
                level1HoldTimer = 0.0f;
                level1ContractComplete = true;
                coop.doorOpen = true;
                setTrapStatus("DEBUG: L1 CONTRACT FORCED COMPLETE");
            }else{
                level2BatteryInstalled = true;
                level2BatteryStage = 2;
                level2FusePanelPowered = true;
                level2FuseCount = 3;
                for(int i=0;i<3;i++) level2FuseDone[i] = true;
                level2AccessReady = true;
                level2HoldActive = false;
                level2HoldTimer = 0.0f;
                level2VentDone = true;
                level2ContractComplete = true;
                coop.doorOpen = true;
                setTrapStatus("DEBUG: L2 CONTRACT FORCED COMPLETE");
            }
            return;
        }
        if(isLevelZero(gCurrentLevel)){
            gCurrentLevel = 1;
            gCompletedLevels++;
            genWorld();
            buildGeom();
            gameState = STATE_INTRO;
            setEchoStatus("DEBUG: SKIPPED TO LEVEL 2");
        }else{
            gCompletedLevels++;
            gCurrentLevel = 0;
            gameState = STATE_MENU;
            menuSel = 0;
            glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
            setEchoStatus("DEBUG: RUN COMPLETE SKIPPED");
        }
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
    if(action==DEBUG_ACT_SPAWN_MED_SPRAY){
        Vec3 sp = debugCursorSpawnPos(gWin);
        if(collideWorld(sp.x, sp.z, PR)) sp = findSpawnPos(cam.pos, 2.0f);
        WorldItem it;
        it.id = nextWorldItemId++ % 250;
        it.type = ITEM_MED_SPRAY;
        it.pos = Vec3(sp.x, 0.0f, sp.z);
        it.active = true;
        worldItems.push_back(it);
        setEchoStatus("DEBUG: MED SPRAY SPAWNED");
        return;
    }
}
