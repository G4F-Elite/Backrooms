#pragma once
#include "traps.h"
#include "coop_rules.h"
#include "map_content.h"
#include "item_types.h"

inline bool tryHandleVoidShiftInteract(const Vec3& playerPos);

inline void captureVoidShiftState(NetVoidShiftState& out){
    out.attentionLevel = attentionLevel;
    out.coLevel = coLevel;
    out.resonatorBattery = resonatorBattery;
    out.level1HoldTimer = level1HoldTimer;
    out.level2HoldTimer = level2HoldTimer;
    out.sideContractType = sideContractType;
    out.sideContractProgress = sideContractProgress;
    out.sideContractTarget = sideContractTarget;
    out.level2BatteryStage = level2BatteryStage;
    out.level2FuseCount = level2FuseCount;
    out.sideContractCompleted = sideContractCompleted;
    for(int i=0;i<3;i++) out.level1NodeDone[i] = level1NodeDone[i];
    out.level1HoldActive = level1HoldActive;
    out.level1ContractComplete = level1ContractComplete;
    out.level2BatteryInstalled = level2BatteryInstalled;
    for(int i=0;i<3;i++) out.level2FuseDone[i] = level2FuseDone[i];
    out.level2AccessReady = level2AccessReady;
    out.level2FusePanelPowered = level2FusePanelPowered;
    out.level2HoldActive = level2HoldActive;
    out.level2ContractComplete = level2ContractComplete;
    out.level2VentDone = level2VentDone;
    out.level2CameraOnline = level2CameraOnline;
    out.level2DroneReprogrammed = level2DroneReprogrammed;
    out.ventilationOnline = ventilationOnline;
    out.npcCartographerActive = npcCartographerActive;
    out.npcDispatcherActive = npcDispatcherActive;
    out.npcLostSurvivorActive = npcLostSurvivorActive;
    out.npcLostSurvivorEscorted = npcLostSurvivorEscorted;
    out.npcCartographerPos = npcCartographerPos;
    out.npcDispatcherPhonePos = npcDispatcherPhonePos;
    out.npcLostSurvivorPos = npcLostSurvivorPos;
}

inline void applyVoidShiftState(const NetVoidShiftState& state){
    attentionLevel = state.attentionLevel;
    coLevel = state.coLevel;
    resonatorBattery = state.resonatorBattery;
    level1HoldTimer = state.level1HoldTimer;
    level2HoldTimer = state.level2HoldTimer;
    sideContractType = state.sideContractType;
    sideContractProgress = state.sideContractProgress;
    sideContractTarget = state.sideContractTarget;
    level2BatteryStage = state.level2BatteryStage;
    level2FuseCount = state.level2FuseCount;
    sideContractCompleted = state.sideContractCompleted;
    for(int i=0;i<3;i++) level1NodeDone[i] = state.level1NodeDone[i];
    level1HoldActive = state.level1HoldActive;
    level1ContractComplete = state.level1ContractComplete;
    level2BatteryInstalled = state.level2BatteryInstalled;
    for(int i=0;i<3;i++) level2FuseDone[i] = state.level2FuseDone[i];
    level2AccessReady = state.level2AccessReady;
    level2FusePanelPowered = state.level2FusePanelPowered;
    level2HoldActive = state.level2HoldActive;
    level2ContractComplete = state.level2ContractComplete;
    level2VentDone = state.level2VentDone;
    level2CameraOnline = state.level2CameraOnline;
    level2DroneReprogrammed = state.level2DroneReprogrammed;
    ventilationOnline = state.ventilationOnline;
    npcCartographerActive = state.npcCartographerActive;
    npcDispatcherActive = state.npcDispatcherActive;
    npcLostSurvivorActive = state.npcLostSurvivorActive;
    npcLostSurvivorEscorted = state.npcLostSurvivorEscorted;
    npcCartographerPos = state.npcCartographerPos;
    npcDispatcherPhonePos = state.npcDispatcherPhonePos;
    npcLostSurvivorPos = state.npcLostSurvivorPos;
}

inline void updateCoopObjectiveHost(){
    if(!coop.initialized) return;
    for(int s=0;s<2;s++) {
        bool on = nearPoint2D(cam.pos, coop.switches[s], 2.4f);
        for(int p=0;p<MAX_PLAYERS;p++){
            if(p==netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
            if(nearPoint2D(netMgr.players[p].pos, coop.switches[s], 2.4f)) { on = true; break; }
        }
        coop.switchOn[s] = on;
    }
    coop.doorOpen = coop.switchOn[0] && coop.switchOn[1];
}

inline bool collideCoopDoor(float x, float z, float r){
    if(multiState==MULTI_IN_GAME){
        const int notesRequired = storyNotesRequired();
        if(!shouldBlockStoryDoor(
            coop.initialized,
            coop.doorOpen,
            multiState,
            MULTI_IN_GAME,
            storyMgr.totalCollected,
            notesRequired
        )) return false;
    }else{
        if(!coop.initialized || isStoryExitReady()) return false;
    }
    // Match visual geometry in game_world_flow.h: frame thickness is 0.36 (half 0.18),
    // so use the same Z half-extent + player radius.
    return fabsf(x - coop.doorPos.x) < (CS * 0.6f + r) && fabsf(z - coop.doorPos.z) < (0.18f + r);
}

inline void syncCoopFromNetwork(){
    if(!netMgr.objectiveStateReceived) return;
    netMgr.objectiveStateReceived = false;
    coop.switchOn[0] = netMgr.objectiveSwitches[0];
    coop.switchOn[1] = netMgr.objectiveSwitches[1];
    coop.doorOpen = netMgr.objectiveDoorOpen;
}

inline void hostSpawnItem(int type, const Vec3& around){
    WorldItem it;
    it.id = nextWorldItemId++ % 250;
    it.type = type;
    it.pos = findSpawnPos(around, 6.0f);
    it.active = true;
    worldItems.push_back(it);
}

inline void hostUpdateItems(){
    itemSpawnTimer -= dTime;
    if(itemSpawnTimer > 0) return;
    itemSpawnTimer = 10.0f + (rng()%10);
    if((int)worldItems.size() > 10) return;
    hostSpawnItem(ITEM_BATTERY, cam.pos);
}

inline void applyItemUse(int type){
    if(type==ITEM_BATTERY && invBattery>0){
        invBattery--;
        flashlightBattery += 35.0f;
        if(flashlightBattery>100.0f) flashlightBattery = 100.0f;
        setEchoStatus("BATTERY: FLASHLIGHT CHARGED");
        return;
    }
    if(type==ITEM_PLUSH_TOY){
        // Mod: allow plush toy use even when not in inventory (slot 3 always available).
        if(invPlush <= 0){
            playerSanity += 40.0f;
            if(playerSanity > 100.0f) playerSanity = 100.0f;
            setEchoStatus("PLUSH TOY: YOUR MIND FEELS WHOLE AGAIN");
            return;
        }
        applyPlushToyUse();
        return;
    }

    if(type==ITEM_MED_SPRAY){
        if(isPlayerDead) return;
        playerHealth += 40.0f;
        if(playerHealth > 100.0f) playerHealth = 100.0f;
        setEchoStatus("MED SPRAY: WOUNDS STITCH UP");
        return;
    }
}

inline void processHostInteractRequests(){
    if(!netMgr.isHost) return;
    static float interactCooldown[MAX_PLAYERS] = {};
    for(int p=0;p<MAX_PLAYERS;p++){
        if(interactCooldown[p] > 0.0f){
            interactCooldown[p] -= dTime;
            if(interactCooldown[p] < 0.0f) interactCooldown[p] = 0.0f;
        }
    }
    for(int i=0;i<netMgr.interactRequestCount;i++){
        if(!netMgr.interactRequests[i].valid) continue;
        int pid = netMgr.interactRequests[i].playerId;
        int type = netMgr.interactRequests[i].requestType;
        int target = netMgr.interactRequests[i].targetId;
        if(type==REQ_PICK_ITEM){
            for(auto& it:worldItems){
                if(!it.active || it.id!=target) continue;
                it.active = false;
                if(pid>=0 && pid<MAX_PLAYERS){
                    if(it.type==ITEM_BATTERY) netMgr.inventoryBattery[pid]++;
                    else if(it.type==ITEM_PLUSH_TOY) netMgr.inventoryPlush[pid]++;
                }
                break;
            }
        }else if(type==REQ_TOGGLE_SWITCH){
            if(target>=0 && target<2) coop.switchOn[target] = !coop.switchOn[target];
        }else if(type==REQ_DEBUG_SPAWN_STALKER || type==REQ_DEBUG_SPAWN_CRAWLER || type==REQ_DEBUG_SPAWN_SHADOW){
            Vec3 base = cam.pos;
            if(pid>=0 && pid<MAX_PLAYERS && netMgr.players[pid].active && netMgr.players[pid].hasValidPos){
                base = netMgr.players[pid].pos;
            }
            EntityType spawnType = ENTITY_STALKER;
            if(type==REQ_DEBUG_SPAWN_CRAWLER) spawnType = ENTITY_CRAWLER;
            else if(type==REQ_DEBUG_SPAWN_SHADOW) spawnType = ENTITY_SHADOW;
            Vec3 sp = findSpawnPos(base, 6.0f);
            entityMgr.spawnEntity(spawnType, sp, nullptr, 0, 0);
        }else if(type==REQ_VOID_SHIFT_INTERACT){
            if(pid < 0 || pid >= MAX_PLAYERS) continue;
            if(interactCooldown[pid] > 0.0f) continue;
            interactCooldown[pid] = 0.25f;
            Vec3 base = cam.pos;
            if(pid>=0 && pid<MAX_PLAYERS && netMgr.players[pid].active && netMgr.players[pid].hasValidPos){
                base = netMgr.players[pid].pos;
            }
            char prompt[96];
            buildVoidShiftInteractPrompt(base, prompt, 96);
            if(prompt[0] != '\0') tryHandleVoidShiftInteract(base);
        }
    }
    netMgr.interactRequestCount = 0;
}

inline void hostSyncFeatureState(){
    NetWorldItemSnapshotEntry entries[MAX_SYNC_ITEMS];
    int count = 0;
    for(auto& it:worldItems){
        if(count>=MAX_SYNC_ITEMS) break;
        entries[count].id = it.id;
        entries[count].type = it.type;
        entries[count].pos = it.pos;
        entries[count].active = it.active;
        count++;
    }
    netMgr.sendItemSnapshot(entries, count);
    netMgr.sendObjectiveState(coop.switchOn[0], coop.switchOn[1], coop.doorOpen);
    netMgr.inventoryBattery[netMgr.myId] = invBattery;
    netMgr.inventoryPlush[netMgr.myId] = invPlush;
    netMgr.sendInventorySync();
    NetVoidShiftState state;
    captureVoidShiftState(state);
    netMgr.sendVoidShiftState(state);
}

inline void clientApplyFeatureState(){
    if(netMgr.itemSnapshotReceived){
        netMgr.itemSnapshotReceived = false;
        worldItems.clear();
        for(int i=0;i<netMgr.itemSnapshotCount;i++){
            WorldItem it;
            it.id = netMgr.itemSnapshot[i].id;
            it.type = netMgr.itemSnapshot[i].type;
            it.pos = netMgr.itemSnapshot[i].pos;
            it.active = netMgr.itemSnapshot[i].active;
            worldItems.push_back(it);
        }
    }
    if(netMgr.inventorySyncReceived){
        netMgr.inventorySyncReceived = false;
        invBattery = netMgr.inventoryBattery[netMgr.myId];
        invPlush = netMgr.inventoryPlush[netMgr.myId];
    }
    if(netMgr.voidShiftStateReceived){
        netMgr.voidShiftStateReceived = false;
        applyVoidShiftState(netMgr.voidShiftState);
    }
    syncCoopFromNetwork();
}

inline void applyRoamEvent(int type, int a, int b, float duration){
    if(type==ROAM_LIGHTS_OUT){
        lightsOutTimer = duration;
        for(auto& l:lights) l.on = false;
        buildGeom();
    }else if(type==ROAM_GEOM_SHIFT){
        (void)a; (void)b;
        reshuffleBehind(cam.pos.x, cam.pos.z, cam.yaw);
        updateMapContent(playerChunkX, playerChunkZ);
        buildGeom();
    }else if(type==ROAM_FALSE_DOOR){
        falseDoorTimer = duration;
        falseDoorPos = findSpawnPos(cam.pos, 2.0f);
    }else if(type==ROAM_FLOOR_HOLES){
        int count = floorHoleCountFromRoll(a + b + (int)rng());
        float ttl = floorHoleDurationFromRoll((int)duration + a);
        spawnFloorHoleEvent(cam.pos, count, ttl);
    }else if(type==ROAM_SUPPLY_CACHE){
        int amount = 2 + ((a + b) % 2);
        for(int i=0;i<amount;i++){
            // Mostly batteries with a chance to include plush toys.
            int itemType = ((rng()%100) < 70) ? ITEM_BATTERY : ITEM_PLUSH_TOY;
            hostSpawnItem(itemType, cam.pos);
        }
        setEchoStatus("SUPPLY CACHE SHIFTED NEARBY");
    }
}

inline void updateRoamEventsHost(){
    static float roamTimer = 18.0f;
    roamTimer -= dTime;
    if(roamTimer > 0) return;
    roamTimer = 18.0f + (rng()%15);
    int typeRoll = (int)(rng()%100);
    int type = ROAM_FALSE_DOOR;
    if(typeRoll < 12) type = ROAM_LIGHTS_OUT;
    else if(typeRoll < 32) type = ROAM_GEOM_SHIFT;
    else if(typeRoll < 58) type = ROAM_FALSE_DOOR;
    else if(typeRoll < 82) type = ROAM_FLOOR_HOLES;
    else type = ROAM_SUPPLY_CACHE;
    float duration = (type==ROAM_GEOM_SHIFT) ? 0.1f : 11.0f + (float)(rng()%7);
    applyRoamEvent(type, playerChunkX, playerChunkZ, duration);
    if(multiState==MULTI_IN_GAME && netMgr.isHost){
        netMgr.sendRoamEvent(type, playerChunkX & 0xFF, playerChunkZ & 0xFF, duration);
    }
}
