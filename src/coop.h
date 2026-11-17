#pragma once
#include "traps.h"
#include "coop_rules.h"
#include "map_content.h"
#include "item_types.h"

void genWorld();
inline bool heistHostPick(int pid, int itemId);
inline void heistHostHandleRequest(int pid, int type, int target);

inline void captureVoidShiftState(NetVoidShiftState& out){
    out.attentionLevel = attentionLevel;
    out.level = gCurrentLevel;
    out.seed = worldSeed;
    out.spawn = netMgr.spawnPos;
    out.runComplete = gRunComplete;
    out.statusSeq = gStatusSeq;
    snprintf(out.status, sizeof(out.status), "%s", trapStatusText);
    captureHeistNet(out.heist);
}

inline void applyVoidShiftState(const NetVoidShiftState& state){
    attentionLevel = state.attentionLevel;
    applyHeistNet(state.heist);

    static int lastHostStatusSeq = -1;
    if(state.statusSeq != lastHostStatusSeq){
        if(lastHostStatusSeq != -1 && state.status[0] != '\0'){
            snprintf(trapStatusText, sizeof(trapStatusText), "%s", state.status);
            trapStatusTimer = 4.0f;
        }
        lastHostStatusSeq = state.statusSeq;
    }
    if(state.runComplete && !gRunComplete){
        gRunComplete = true;
        playerEscaped = true;
        glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

// The host owns level flow: when its level or seed changes, clients rebuild the same world.
inline void clientFollowHostLevel(const NetVoidShiftState& state){
    if(state.level == gCurrentLevel && state.seed == worldSeed) return;
    gCurrentLevel = state.level;
    netMgr.worldSeed = state.seed;
    netMgr.spawnPos = state.spawn;
    genWorld();
    buildGeom();
}

// The old exit door is gone: floors are left through the drill.
inline bool collideCoopDoor(float x, float z, float r){
    (void)x; (void)z; (void)r;
    return false;
}

inline void hostSpawnItem(int type, const Vec3& around){
    WorldItem it;
    it.id = nextWorldItemId++ % 250;
    it.type = type;
    it.pos = findSpawnPos(around, 6.0f);
    it.active = true;
    worldItems.push_back(it);
}

// A few batteries keep turning up near the squad; everything else has to be found or bought.
inline void hostUpdateItems(){
    worldItems.erase(std::remove_if(worldItems.begin(), worldItems.end(), [](const WorldItem& it){ return !it.active; }), worldItems.end());
    itemSpawnTimer -= dTime;
    if(itemSpawnTimer > 0) return;
    itemSpawnTimer = 26.0f + (rng()%14);
    if(!heistDangerActive()) return;
    int batteries = 0;
    for(const auto& it:worldItems) if(it.type==ITEM_BATTERY) batteries++;
    if(batteries >= 2 || (int)worldItems.size() >= MAX_SYNC_ITEMS - 4) return;
    hostSpawnItem(ITEM_BATTERY, cam.pos);
}

inline void applyItemUse(int type){
    if(type==ITEM_BATTERY && invBattery>0){
        invBattery--;
        flashlightBattery += 35.0f;
        if(flashlightBattery>100.0f) flashlightBattery = 100.0f;
        setEchoStatus("BATTERY: FLASHLIGHT CHARGED");
        audioPostCue(CUE_BATTERY);
        return;
    }
    // The plush toy has no "use": it restores morale only while held (slot 3).
    if(type==ITEM_PLUSH_TOY) return;

    if(type==ITEM_MED_SPRAY){
        if(isPlayerDead) return;
        playerHealth += 40.0f;
        if(playerHealth > 100.0f) playerHealth = 100.0f;
        setEchoStatus("MED SPRAY: GOOD ENOUGH. BACK TO WORK");
        audioPostCue(CUE_SPRAY);
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
            if(pid>=0 && pid<MAX_PLAYERS) heistHostPick(pid, target);
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
        }else{
            heistHostHandleRequest(pid, type, target);
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
        clientFollowHostLevel(netMgr.voidShiftState);
        applyVoidShiftState(netMgr.voidShiftState);
    }
}

inline void applyRoamEvent(int type, int a, int b, float duration){
    if(type==ROAM_LIGHTS_OUT){
        lightsOutTimer = duration;
        for(auto& l:lights) l.on = false;
        buildGeom();
    }else if(type==ROAM_GEOM_SHIFT){
        (void)a; (void)b;
        if(multiState==MULTI_IN_GAME) return;
        reshuffleBehind(cam.pos.x, cam.pos.z, cam.yaw);
        reopenObjectiveCells();
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
        setEchoStatus("SOMEBODY LEFT SUPPLIES NEARBY. FINDERS KEEPERS");
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
    // Around the running drill the building leaves the crew alone; only the care package still comes.
    if(type!=ROAM_SUPPLY_CACHE && rigPosInQuiet(cam.pos)) return;
    applyRoamEvent(type, playerChunkX, playerChunkZ, duration);
    if(multiState==MULTI_IN_GAME && netMgr.isHost){
        netMgr.sendRoamEvent(type, playerChunkX & 0xFF, playerChunkZ & 0xFF, duration);
    }
}
