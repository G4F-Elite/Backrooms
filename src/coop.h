#pragma once
#include "traps.h"
#include "coop_rules.h"
#include "map_content.h"

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
    const int notesRequired = 5;
    if(!shouldBlockStoryDoor(
        coop.initialized,
        coop.doorOpen,
        multiState,
        MULTI_IN_GAME,
        storyMgr.totalCollected,
        notesRequired
    )) return false;
    return fabsf(x - coop.doorPos.x) < (CS * 0.6f + r) && fabsf(z - coop.doorPos.z) < (CS * 0.2f + r);
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
    int type = rng()%3;
    hostSpawnItem(type, cam.pos);
}

inline void applyItemUse(int type){
    if(type==0 && invBattery>0){
        invBattery--;
        flashlightBattery += 35.0f;
        if(flashlightBattery>100.0f) flashlightBattery = 100.0f;
    }else if(type==1 && invMedkit>0){
        invMedkit--;
        playerHealth += 40.0f;
        if(playerHealth>100.0f) playerHealth = 100.0f;
    }else if(type==2 && invBait>0){
        invBait--;
        baitEffectTimer = 12.0f;
    }
}

inline void processHostInteractRequests(){
    if(!netMgr.isHost) return;
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
                    if(it.type==0) netMgr.inventoryBattery[pid]++;
                    else if(it.type==1) netMgr.inventoryMedkit[pid]++;
                    else netMgr.inventoryBait[pid]++;
                }
                break;
            }
        }else if(type==REQ_TOGGLE_SWITCH){
            if(target>=0 && target<2) coop.switchOn[target] = !coop.switchOn[target];
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
    netMgr.inventoryMedkit[netMgr.myId] = invMedkit;
    netMgr.inventoryBait[netMgr.myId] = invBait;
    netMgr.sendInventorySync();
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
        invMedkit = netMgr.inventoryMedkit[netMgr.myId];
        invBait = netMgr.inventoryBait[netMgr.myId];
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
            int itemType = (int)(rng()%3);
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
    float duration = (type==ROAM_GEOM_SHIFT) ? 0.1f : 8.0f + (float)(rng()%5);
    applyRoamEvent(type, playerChunkX, playerChunkZ, duration);
    if(multiState==MULTI_IN_GAME && netMgr.isHost){
        netMgr.sendRoamEvent(type, playerChunkX & 0xFF, playerChunkZ & 0xFF, duration);
    }
}
