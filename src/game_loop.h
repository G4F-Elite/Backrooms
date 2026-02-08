#pragma once

#include "coop.h"

void buildGeom() {
    std::vector<float> wv, fv, cv, pv, lv, lvOff;
    int pcx = playerChunkX, pcz = playerChunkZ;
    
    for (int dcx = -VIEW_CHUNKS; dcx <= VIEW_CHUNKS; dcx++) {
        for (int dcz = -VIEW_CHUNKS; dcz <= VIEW_CHUNKS; dcz++) {
            auto it = chunks.find(chunkKey(pcx + dcx, pcz + dcz));
            if (it == chunks.end()) continue;
            
            for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                    int wx = (pcx + dcx) * CHUNK_SIZE + lx;
                    int wz = (pcz + dcz) * CHUNK_SIZE + lz;
                    if (it->second.cells[lx][lz] != 0) continue;
                    
                    float px = wx * CS, pz = wz * CS;
                    const float uvTile = 2.2f;
                    
                    float fl[] = {
                        px, 0, pz, 0, 0, 0, 1, 0,
                        px, 0, pz + CS, 0, uvTile, 0, 1, 0,
                        px + CS, 0, pz + CS, uvTile, uvTile, 0, 1, 0,
                        px, 0, pz, 0, 0, 0, 1, 0,
                        px + CS, 0, pz + CS, uvTile, uvTile, 0, 1, 0,
                        px + CS, 0, pz, uvTile, 0, 0, 1, 0
                    };
                    for (int i = 0; i < 48; i++) fv.push_back(fl[i]);
                    
                    float cl[] = {
                        px, WH, pz, 0, 0, 0, -1, 0,
                        px, WH, pz + CS, 0, uvTile, 0, -1, 0,
                        px + CS, WH, pz + CS, uvTile, uvTile, 0, -1, 0,
                        px, WH, pz, 0, 0, 0, -1, 0,
                        px + CS, WH, pz + CS, uvTile, uvTile, 0, -1, 0,
                        px + CS, WH, pz, uvTile, 0, 0, -1, 0
                    };
                    for (int i = 0; i < 48; i++) cv.push_back(cl[i]);
                    
                    if (getCellWorld(wx - 1, wz) == 1) mkWall(wv, px, pz, 0, CS, WH, CS, WH);
                    if (getCellWorld(wx + 1, wz) == 1) mkWall(wv, px + CS, pz + CS, 0, -CS, WH, CS, WH);
                    if (getCellWorld(wx, wz - 1) == 1) mkWall(wv, px + CS, pz, -CS, 0, WH, CS, WH);
                    if (getCellWorld(wx, wz + 1) == 1) mkWall(wv, px, pz + CS, CS, 0, WH, CS, WH);
                }
            }
        }
    }
    
    for (auto& p : pillars) mkPillar(pv, p.x, p.z, 0.6f, WH);
    for (auto& l : lights) {
        if (l.on) mkLight(lv, l.pos, l.sizeX, l.sizeZ);
        else mkLight(lvOff, l.pos, l.sizeX, l.sizeZ);
    }
    
    wallVC = (int)wv.size() / 8;
    floorVC = (int)fv.size() / 8;
    ceilVC = (int)cv.size() / 8;
    pillarVC = (int)pv.size() / 8;
    lightVC = (int)lv.size() / 5;
    lightOffVC = (int)lvOff.size() / 5;
    
    setupVAO(wallVAO, wallVBO, wv, true);
    setupVAO(floorVAO, floorVBO, fv, true);
    setupVAO(ceilVAO, ceilVBO, cv, true);
    setupVAO(pillarVAO, pillarVBO, pv, true);
    setupVAO(lightVAO, lightVBO, lv, false);
    
    if (!lvOff.empty()) setupVAO(lightOffVAO, lightOffVBO, lvOff, false);
    
    initQuad(quadVAO, quadVBO);
    lastBuildChunkX = pcx;
    lastBuildChunkZ = pcz;
}

void buildNotes(float tm) {
    std::vector<float> nv;
    for (auto& n : storyMgr.notes) {
        if (!n.active || n.collected) continue;
        mkNoteGlow(nv, n.pos, n.bobPhase);
    }
    noteVC = (int)nv.size() / 8;
    if (noteVC > 0) setupVAO(noteVAO, noteVBO, nv, true);
}

void trySpawnNote(int noteId) {
    if (noteId >= 12 || storyMgr.notesCollected[noteId]) return;
    if (multiState == MULTI_IN_GAME && !netMgr.isHost) return;
    
    Vec3 sp = findSpawnPos(cam.pos, 12.0f);
    Vec3 d = sp - cam.pos;
    d.y = 0;
    
    if (sqrtf(d.x * d.x + d.z * d.z) > 8.0f) {
        storyMgr.spawnNote(sp, noteId);
        lastSpawnedNote = noteId;
        if (multiState == MULTI_IN_GAME) netMgr.sendNoteSpawn(noteId, sp);
    }
}

void cleanupFarNotes() {
    for (auto& n : storyMgr.notes) {
        if (!n.active || n.collected) continue;
        Vec3 d = n.pos - cam.pos;
        d.y = 0;
        if (sqrtf(d.x * d.x + d.z * d.z) > 80.0f) {
            n.active = false;
            if (n.id == lastSpawnedNote) lastSpawnedNote = n.id - 1;
        }
    }
}

void genWorld() {
    if (multiState == MULTI_IN_GAME && !netMgr.isHost) {
        worldSeed = netMgr.worldSeed;
    } else {
        worldSeed = (unsigned int)time(nullptr);
        if (multiState == MULTI_IN_GAME) netMgr.worldSeed = worldSeed;
    }
    
    chunks.clear();
    lights.clear();
    pillars.clear();
    g_lightStates.clear();
    
    updateVisibleChunks(0, 0);
    updateLightsAndPillars(0, 0);
    Vec3 sp = findSafeSpawn();
    Vec3 coopBase = sp;
    
    if (multiState == MULTI_IN_GAME) {
        if (netMgr.isHost) netMgr.spawnPos = sp;
        else { sp = netMgr.spawnPos; coopBase = netMgr.spawnPos; }
        sp.x += netMgr.myId * 1.5f;
    }
    
    cam.pos = Vec3(sp.x, PH, sp.z);
    cam.yaw = cam.pitch = 0;
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    
    entityMgr.reset();
    storyMgr.init();
    initTrapCorridor(sp);
    resetPlayerInterpolation();
    initCoopObjectives(coopBase);
    
    worldItems.clear();
    nextWorldItemId = 1;
    invBattery = invMedkit = invBait = 0;
    clearEchoSignal();
    echoSpawnTimer = 12.0f + (float)(rng() % 8);
    echoStatusTimer = trapStatusTimer = anomalyBlur = 0.0f;
    echoStatusText[0] = trapStatusText[0] = '\0';
    lightsOutTimer = falseDoorTimer = baitEffectTimer = 0.0f;
    itemSpawnTimer = 8.0f;
    playerHealth = playerSanity = playerStamina = 100;
    flashlightBattery = 100;
    flashlightOn = isPlayerDead = flashlightShutdownBlinkActive = false;
    flashlightShutdownBlinkTimer = 0.0f;
    resetScareSystemState(scareState);
    entitySpawnTimer = 30;
    survivalTime = reshuffleTimer = 15;
    lastSpawnedNote = -1;
    noteSpawnTimer = 15.0f;
}

void teleportToPlayer() {
    if (multiState != MULTI_IN_GAME || !netMgr.hasOtherPlayersWithPos()) return;
    Vec3 tp = netMgr.getOtherPlayerPos();
    cam.pos = Vec3(tp.x + 1.0f, PH, tp.z);
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    buildGeom();
}

void mouse(GLFWwindow*, double xp, double yp) {
    if (gameState != STATE_GAME && gameState != STATE_INTRO) return;
    if (firstMouse) { lastX = (float)xp; lastY = (float)yp; firstMouse = false; }
    cam.yaw -= ((float)xp - lastX) * settings.mouseSens;
    cam.pitch += (lastY - (float)yp) * settings.mouseSens;
    if (cam.pitch > 1.4f) cam.pitch = 1.4f;
    if (cam.pitch < -1.4f) cam.pitch = -1.4f;
    lastX = (float)xp;
    lastY = (float)yp;
}

void handleItemPickup() {
    if (multiState == MULTI_IN_GAME) {
        if (netMgr.isHost) {
            for (auto& it : worldItems) {
                if (!it.active || it.id != nearbyWorldItemId) continue;
                it.active = false;
                if (it.type == 0) invBattery++;
                else if (it.type == 1) invMedkit++;
                else invBait++;
                break;
            }
        } else {
            netMgr.sendInteractRequest(REQ_PICK_ITEM, nearbyWorldItemId);
        }
    } else {
        for (auto& it : worldItems) {
            if (!it.active || it.id != nearbyWorldItemId) continue;
            it.active = false;
            if (it.type == 0) invBattery++;
            else if (it.type == 1) invMedkit++;
            else invBait++;
            break;
        }
    }
}

void handleMovement(GLFWwindow* w) {
    float spd = 4.0f * dTime;
    bool sprinting = glfwGetKey(w, settings.binds.sprint) == GLFW_PRESS && 
                     playerStamina > 0 && staminaCooldown <= 0;
    
    if (sprinting) {
        spd *= 1.6f;
        playerStamina -= 20.0f * dTime;
        if (playerStamina < 0) { playerStamina = 0; staminaCooldown = 1.5f; }
    } else {
        playerStamina += 15.0f * dTime;
        if (playerStamina > 100) playerStamina = 100;
    }
    if (staminaCooldown > 0) staminaCooldown -= dTime;
    
    if (glfwGetKey(w, settings.binds.crouch) == GLFW_PRESS) {
        cam.targetH = PH_CROUCH; cam.crouch = true; spd *= 0.5f;
    } else { cam.targetH = PH; cam.crouch = false; }
    cam.curH += (cam.targetH - cam.curH) * 10.0f * dTime;
    
    Vec3 fwd(sinf(cam.yaw), 0, cosf(cam.yaw));
    Vec3 right(cosf(cam.yaw), 0, -sinf(cam.yaw));
    Vec3 np = cam.pos;
    bool mv = false;
    
    if (glfwGetKey(w, settings.binds.forward) == GLFW_PRESS) { np = np + fwd * spd; mv = true; }
    if (glfwGetKey(w, settings.binds.back) == GLFW_PRESS) { np = np - fwd * spd; mv = true; }
    if (glfwGetKey(w, settings.binds.left) == GLFW_PRESS) { np = np + right * spd; mv = true; }
    if (glfwGetKey(w, settings.binds.right) == GLFW_PRESS) { np = np - right * spd; mv = true; }
    
    bool blockX = collideWorld(np.x, cam.pos.z, PR) || collideCoopDoor(np.x, cam.pos.z, PR);
    bool blockZ = collideWorld(cam.pos.x, np.z, PR) || collideCoopDoor(cam.pos.x, np.z, PR);
    if (falseDoorTimer > 0) {
        if (nearPoint2D(Vec3(np.x, 0, cam.pos.z), falseDoorPos, 1.0f)) blockX = true;
        if (nearPoint2D(Vec3(cam.pos.x, 0, np.z), falseDoorPos, 1.0f)) blockZ = true;
    }
    if (!blockX) cam.pos.x = np.x;
    if (!blockZ) cam.pos.z = np.z;
    
    static float bobT = 0, lastB = 0;
    if (mv) {
        bobT += dTime * (spd > 5.0f ? 12.0f : 8.0f);
        float cb = sinf(bobT);
        cam.pos.y = cam.curH + cb * 0.04f;
        if (lastB > -0.7f && cb <= -0.7f && !sndState.stepTrig) {
            sndState.stepTrig = true; sndState.footPhase = 0;
        }
        lastB = cb;
    } else { cam.pos.y = cam.curH + (cam.pos.y - cam.curH) * 0.9f; bobT = lastB = 0; }
}

void handleFlashlight() {
    if (flashlightOn) {
        if (!flashlightShutdownBlinkActive && shouldStartFlashlightShutdownBlink(flashlightBattery)) {
            flashlightShutdownBlinkActive = true; flashlightShutdownBlinkTimer = 0.0f;
        }
        if (flashlightShutdownBlinkActive) {
            flashlightShutdownBlinkTimer += dTime;
            sndState.flashlightOn = isFlashlightOnDuringShutdownBlink(flashlightShutdownBlinkTimer) ? 1.0f : 0.0f;
            flashlightBattery -= dTime * 1.67f;
            if (flashlightBattery < 0.0f) flashlightBattery = 0.0f;
            if (isFlashlightShutdownBlinkFinished(flashlightShutdownBlinkTimer)) {
                flashlightBattery = flashlightOn = flashlightShutdownBlinkActive = 0;
                flashlightShutdownBlinkTimer = sndState.flashlightOn = 0.0f;
            }
        } else {
            sndState.flashlightOn = 1.0f;
            flashlightBattery -= dTime * 1.67f;
            if (flashlightBattery <= 0) { flashlightBattery = flashlightOn = 0; sndState.flashlightOn = 0; }
        }
    } else {
        flashlightShutdownBlinkActive = false; flashlightShutdownBlinkTimer = 0.0f;
        flashlightBattery += dTime * 10.0f;
        if (flashlightBattery > 100) flashlightBattery = 100;
    }
}

void gameInput(GLFWwindow* w) {
    if (glfwGetKey(w, settings.binds.pause) == GLFW_PRESS && !escPressed) {
        gameState = STATE_PAUSE; menuSel = 0;
        glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    escPressed = glfwGetKey(w, settings.binds.pause) == GLFW_PRESS;
    updateMinimapCheat(w);
    
    bool fNow = glfwGetKey(w, settings.binds.flashlight) == GLFW_PRESS;
    if (fNow && !flashlightPressed && flashlightBattery > 5.0f) {
        flashlightOn = !flashlightOn;
        if (!flashlightOn) { flashlightShutdownBlinkActive = false; flashlightShutdownBlinkTimer = 0.0f; }
        sndState.flashlightOn = flashlightOn ? 1.0f : 0.0f;
    }
    flashlightPressed = fNow;
    
    bool eNow = glfwGetKey(w, settings.binds.interact) == GLFW_PRESS;
    nearbyWorldItemId = nearbyWorldItemType = -1;
    for (auto& it : worldItems) {
        if (!it.active) continue;
        if (nearPoint2D(cam.pos, it.pos, 2.2f)) {
            nearbyWorldItemId = it.id; nearbyWorldItemType = it.type; break;
        }
    }
    
    bool nearEchoSignal = echoSignal.active && isEchoInRange(cam.pos, echoSignal.pos, 2.5f);
    if (eNow && !interactPressed && nearNoteId >= 0) {
        if (storyMgr.checkNotePickup(cam.pos, 4.0f)) {
            if (tryTriggerStoryScare(scareState, storyMgr.currentNote)) triggerLocalScare(0.34f, 0.18f, 5.0f);
            gameState = STATE_NOTE;
            playerSanity -= 8.0f;
            if (playerSanity < 0) playerSanity = 0;
            if (multiState == MULTI_IN_GAME) netMgr.sendNoteCollect(nearNoteId);
        }
    } else if (eNow && !interactPressed && nearbyWorldItemId >= 0) {
        handleItemPickup();
    } else if (eNow && !interactPressed && nearEchoSignal) {
        resolveEchoInteraction();
    }
    interactPressed = eNow;
    
    static bool key1P = false, key2P = false, key3P = false;
    bool k1 = glfwGetKey(w, settings.binds.item1) == GLFW_PRESS;
    bool k2 = glfwGetKey(w, settings.binds.item2) == GLFW_PRESS;
    bool k3 = glfwGetKey(w, settings.binds.item3) == GLFW_PRESS;
    if (k1 && !key1P) applyItemUse(0);
    if (k2 && !key2P) applyItemUse(1);
    if (k3 && !key3P) applyItemUse(2);
    key1P = k1; key2P = k2; key3P = k3;
    
    handleMovement(w);
    handleFlashlight();
}

#include "scene_render.h"
#include "hud.h"
#include "game_state.h"