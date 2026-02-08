#pragma once

// Game state processing - extracted from game_loop.h for modularity

void processIntroState() {
    bool spNow = glfwGetKey(gWin, GLFW_KEY_SPACE) == GLFW_PRESS;
    if(spNow && !spacePressed) {
        storyMgr.introComplete = true;
        gameState = STATE_GAME;
    }
    spacePressed = spNow;
    storyMgr.update(dTime, survivalTime, playerSanity, rng);
    
    if(storyMgr.introComplete) {
        gameState = STATE_GAME;
        glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }
}

void processNoteState() {
    static bool wasKeyDown = true;
    bool anyKey = glfwGetKey(gWin, GLFW_KEY_SPACE) == GLFW_PRESS ||
                  glfwGetKey(gWin, GLFW_KEY_E) == GLFW_PRESS ||
                  glfwGetKey(gWin, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    
    if(!anyKey) wasKeyDown = false;
    
    if(anyKey && !wasKeyDown && storyMgr.readingNote) {
        storyMgr.closeNote();
        gameState = STATE_GAME;
        wasKeyDown = true;
    }
}

void processMenuStates() {
    menuInput(gWin);
    
    if(gameState == STATE_MULTI_WAIT && reconnectInProgress) {
        reconnectAttemptTimer -= dTime;
        if(reconnectAttemptTimer <= 0) {
            reconnectAttempts++;
            reconnectAttemptTimer = nextReconnectDelaySeconds(reconnectAttempts);
            
            if(shouldContinueReconnect(reconnectAttempts, 12) && lastSession.valid) {
                netMgr.shutdown();
                netMgr.init();
                netMgr.joinGame(lastSession.hostIP);
            } else {
                reconnectInProgress = false;
                restoreAfterReconnect = false;
                multiState = MULTI_NONE;
                gameState = STATE_MULTI;
                menuSel = 1;
            }
        }
    }
    
    if(gameState == STATE_PAUSE && enterPressed && menuSel == 2) {
        gameState = STATE_MENU;
        menuSel = 0;
        genWorld();
        buildGeom();
    }
    
    if(gameState == STATE_MULTI_WAIT && netMgr.gameStarted) {
        multiState = MULTI_IN_GAME;
        genWorld();
        buildGeom();
        
        if(restoreAfterReconnect) {
            restoreSessionSnapshot();
            reconnectInProgress = false;
            restoreAfterReconnect = false;
            gameState = STATE_GAME;
            glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        } else {
            gameState = STATE_INTRO;
        }
    }
}

void processDeadState() {
    bool eN = glfwGetKey(gWin, GLFW_KEY_ENTER) == GLFW_PRESS ||
              glfwGetKey(gWin, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool esN = glfwGetKey(gWin, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    
    if(eN && !enterPressed) {
        genWorld();
        buildGeom();
        gameState = STATE_INTRO;
    }
    
    if(esN && !escPressed) {
        gameState = STATE_MENU;
        menuSel = 0;
        glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    
    enterPressed = eN;
    escPressed = esN;
}

void updateTimers() {
    if(lightsOutTimer > 0) {
        lightsOutTimer -= dTime;
        if(lightsOutTimer <= 0) {
            lightsOutTimer = 0;
            updateLightsAndPillars(playerChunkX, playerChunkZ);
            buildGeom();
        }
    }
    
    if(falseDoorTimer > 0) {
        falseDoorTimer -= dTime;
    }
    
    if(baitEffectTimer > 0) {
        baitEffectTimer -= dTime;
    }
}

void updateChunks() {
    int targetChunkX = (int)floorf(cam.pos.x / (CS * CHUNK_SIZE));
    int targetChunkZ = (int)floorf(cam.pos.z / (CS * CHUNK_SIZE));
    
    if(targetChunkX != playerChunkX || targetChunkZ != playerChunkZ) {
        updateVisibleChunks(cam.pos.x, cam.pos.z);
    }
    
    if(playerChunkX != lastBuildChunkX || playerChunkZ != lastBuildChunkZ) {
        updateLightsAndPillars(playerChunkX, playerChunkZ);
        buildGeom();
    }
}

void updateNotes() {
    storyMgr.update(dTime, survivalTime, playerSanity, rng);
    
    if(tryTriggerRandomScare(scareState, dTime, storyMgr.getPhase(), playerSanity, (int)(rng()%100))) {
        triggerLocalScare(0.26f, 0.14f, 3.0f);
    }
    
    for(auto& n : storyMgr.notes) {
        if(n.active && !n.collected) {
            n.bobPhase += dTime * 3.0f;
        }
    }
    buildNotes(vhsTime);
    
    nearNoteId = -1;
    for(auto& n : storyMgr.notes) {
        if(!n.active || n.collected) continue;
        Vec3 d = n.pos - cam.pos;
        d.y = 0;
        if(sqrtf(d.x * d.x + d.z * d.z) < 4.0f) {
            nearNoteId = n.id;
            break;
        }
    }
    
    cleanupFarNotes();
    noteSpawnTimer -= dTime;
    
    if(noteSpawnTimer <= 0 && lastSpawnedNote < 11) {
        int nn = lastSpawnedNote + 1;
        trySpawnNote(nn);
        noteSpawnTimer = 20.0f + (rng() % 20);
    }
}

void updateEntities() {
    bool canSpawnEnt = (multiState != MULTI_IN_GAME || netMgr.isHost);
    entitySpawnTimer -= dTime;
    int maxEnt = computeEntityCap(survivalTime);
    
    if(canSpawnEnt && entitySpawnTimer <= 0 && (int)entityMgr.entities.size() < maxEnt) {
        EntityType type = chooseSpawnEntityType(survivalTime, (int)rng(), (int)rng());
        Vec3 spawnP = findSpawnPos(cam.pos, 25.0f);
        
        if(!hasEntityNearPos(entityMgr.entities, spawnP, 14.0f)) {
            entityMgr.spawnEntity(type, spawnP, nullptr, 0, 0);
        }
        entitySpawnTimer = computeEntitySpawnDelay(survivalTime, (int)rng());
    }
    
    entityMgr.update(dTime, cam.pos, cam.yaw, nullptr, 0, 0, CS);
    
    if(baitEffectTimer > 0) {
        entityMgr.dangerLevel *= 0.45f;
    }
}

void updateReshuffle() {
    reshuffleTimer -= dTime;
    float reshuffleChance = 30.0f + survivalTime * 0.1f;
    if(reshuffleChance > 80.0f) reshuffleChance = 80.0f;
    
    float reshuffleDelay = 25.0f - survivalTime * 0.03f;
    if(reshuffleDelay < 5.0f) reshuffleDelay = 5.0f;
    
    bool canReshuffle = (multiState != MULTI_IN_GAME || netMgr.isHost);
    bool frontReshuffle = survivalTime > 300 && rng() % 100 < 20;
    
    if(canReshuffle && reshuffleTimer <= 0 && rng() % 100 < (int)reshuffleChance) {
        if(frontReshuffle || reshuffleBehind(cam.pos.x, cam.pos.z, cam.yaw)) {
            buildGeom();
            if(multiState == MULTI_IN_GAME && netMgr.isHost) {
                netMgr.sendReshuffle(playerChunkX, playerChunkZ, worldSeed);
            }
        }
        reshuffleTimer = reshuffleDelay + (rng() % 10);
    } else if(reshuffleTimer <= 0) {
        reshuffleTimer = 8.0f + (rng() % 8);
    }
}

void updatePlayerStats() {
    if(entityMgr.dangerLevel > 0.1f) {
        playerSanity -= entityMgr.dangerLevel * 8.0f * dTime;
    } else {
        playerSanity += 2.0f * dTime;
    }
    
    if(playerSanity > 100) playerSanity = 100;
    if(playerSanity < 0) playerSanity = 0;
    
    if(playerSanity <= 0 && rng() % 1000 < 5) {
        isPlayerDead = true;
        playerHealth = 0;
        glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    
    if(entityMgr.checkPlayerAttack(cam.pos)) {
        playerHealth -= 35.0f * dTime;
        playerSanity -= 15.0f * dTime;
        camShake = 0.15f;
        damageFlash = 0.4f;
        
        if(multiState == MULTI_IN_GAME) {
            netMgr.sendScare(netMgr.myId);
            if(netMgr.isHost) triggerScare();
        } else {
            triggerScare();
        }
        
        if(playerHealth <= 0) {
            isPlayerDead = true;
            playerHealth = 0;
            glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    
    camShake *= 0.9f;
    damageFlash *= 0.92f;
    survivalTime += dTime;
    
    if(multiState == MULTI_IN_GAME && !netMgr.isHost) {
        captureSessionSnapshot();
    }
}

void processAliveState() {
    updateTimers();
    updateEchoSignal();
    updateTrapCorridor();
    
    gameInput(gWin);
    
    // Coop switches
    if (!coop.doorOpen) {
        if (nearPoint2D(cam.pos, coop.switches[0], 2.6f)) coop.switchOn[0] = true;
        if (nearPoint2D(cam.pos, coop.switches[1], 2.6f)) coop.switchOn[1] = true;
    }
    
    updateChunks();
    updateNotes();
    updateEntities();
    updateReshuffle();
    updatePlayerStats();
    updateMultiplayer();
}

void processGameState() {
    if(gameState == STATE_INTRO) {
        processIntroState();
    } 
    else if(gameState == STATE_NOTE) {
        processNoteState();
    } 
    else if(gameState == STATE_SETTINGS) {
        settingsInput(gWin, false);
    } 
    else if(gameState == STATE_SETTINGS_PAUSE) {
        settingsInput(gWin, true);
    } 
    else if(gameState == STATE_MENU || gameState == STATE_PAUSE ||
            gameState == STATE_MULTI || gameState == STATE_MULTI_HOST || 
            gameState == STATE_MULTI_JOIN || gameState == STATE_MULTI_WAIT) {
        processMenuStates();
    } 
    else if(gameState == STATE_GAME) {
        if(isPlayerDead) {
            processDeadState();
        } else {
            processAliveState();
        }
    }
}