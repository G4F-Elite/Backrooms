#pragma once
// Dev self-test: plays the heist loop from a script and saves screenshots.
//   set BACKROOMS_SELFTEST=1 && build\backrooms.exe
// The window stays hidden, audio is muted, results go to build/selftest.log and
// build/selftest_*.bmp. Does nothing unless the environment variable is set.

extern "C" {
__declspec(dllimport) void __stdcall glReadPixels(int x, int y, int w, int h, unsigned int format, unsigned int type, void* data);
__declspec(dllimport) void __stdcall glPixelStorei(unsigned int name, int value);
}

inline FILE* gSelftestLog = nullptr;
inline int gSelftestFails = 0;

inline void selftestInit() {
    const char* env = std::getenv("BACKROOMS_SELFTEST");
    gSelftest = env && env[0] == '1';
    if (!gSelftest) return;
    gSelftestLog = std::fopen("build/selftest.log", "w");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // BACKROOMS_SELFTEST_SIZE=1920x1080 runs the script (and its screenshots) at another window size.
    const char* size = std::getenv("BACKROOMS_SELFTEST_SIZE");
    int sizeW = 0, sizeH = 0;
    if (size && std::sscanf(size, "%dx%d", &sizeW, &sizeH) == 2 && sizeW >= 640 && sizeH >= 360) {
        winW = sizeW;
        winH = sizeH;
    }
    // BACKROOMS_SELFTEST_FX=<ssao><gi><rays><bloom><denoise><aa>, e.g. 421110, turns post effects on for the screenshots.
    const char* fx = std::getenv("BACKROOMS_SELFTEST_FX");
    size_t fxLen = fx ? std::strlen(fx) : 0;
    auto fxDigit = [&](size_t i) { return i < fxLen ? fx[i] - '0' : 0; };
    settings.ssaoQuality = fxDigit(0);
    settings.giQuality = fxDigit(1);
    settings.godRays = fxDigit(2) > 0;
    settings.bloom = fxDigit(3) > 0;
    settings.rtxDenoise = fxDigit(4) > 0;
    settings.aaMode = fxDigit(5);
    // BACKROOMS_SELFTEST_VHS=0..100 overrides the tape effect strength; 0 gives clean screenshots.
    if (const char* vhs = std::getenv("BACKROOMS_SELFTEST_VHS")) settings.vhsIntensity = std::clamp(std::atoi(vhs) / 100.0f, 0.0f, 1.0f);
}

inline void selftestLog(const char* what, bool ok) {
    if (!ok) gSelftestFails++;
    if (!gSelftestLog) return;
    std::fprintf(gSelftestLog, "%s %s | level %d stage %d quota %d/%d cash %d carry %d items %d breaks %d\n",
                 ok ? "ok  " : "FAIL", what, gCurrentLevel, heist.stage, heist.quotaHave, heist.quotaNeed, heist.cash,
                 carryCount(heistMyCarry()), (int)worldItems.size(), heist.breakCount);
    std::fflush(gSelftestLog);
}

inline void selftestShot(const char* name) {
    std::vector<unsigned char> px((size_t)winW * winH * 3);
    glPixelStorei(0x0D05, 1);
    glReadPixels(0, 0, winW, winH, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    char path[96];
    std::snprintf(path, sizeof(path), "build/selftest_%s.bmp", name);
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    int row = (winW * 3 + 3) & ~3, size = 54 + row * winH;
    unsigned char hd[54] = {'B', 'M'};
    hd[2] = (unsigned char)size; hd[3] = (unsigned char)(size >> 8); hd[4] = (unsigned char)(size >> 16);
    hd[10] = 54; hd[14] = 40;
    hd[18] = (unsigned char)winW; hd[19] = (unsigned char)(winW >> 8);
    hd[22] = (unsigned char)winH; hd[23] = (unsigned char)(winH >> 8);
    hd[26] = 1; hd[28] = 24;
    std::fwrite(hd, 1, 54, f);
    std::vector<unsigned char> line((size_t)row, 0);
    for (int y = 0; y < winH; y++) {
        for (int x = 0; x < winW; x++) {
            line[x * 3] = px[((size_t)y * winW + x) * 3 + 2];
            line[x * 3 + 1] = px[((size_t)y * winW + x) * 3 + 1];
            line[x * 3 + 2] = px[((size_t)y * winW + x) * 3];
        }
        std::fwrite(line.data(), 1, (size_t)row, f);
    }
    std::fclose(f);
}

inline void selftestLookAt(const Vec3& from, const Vec3& target) {
    cam.pos = Vec3(from.x, PH, from.z);
    Vec3 d = target - from;
    cam.yaw = atan2f(d.x, d.z);
    cam.pitch = -0.12f;
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    updateMapContent(playerChunkX, playerChunkZ);
    buildGeom();
}

inline bool selftestPickNearest() {
    Vec3 target;
    if (!heistScannerTarget(target)) return false;
    cam.pos = Vec3(target.x + 0.6f, PH, target.z);
    HeistPrompt p = heistFindPrompt();
    return p.itemId >= 0 && heistHostPick(heistMe(), p.itemId);
}

inline void selftestStartRun(bool skipTutorial) {
    gCurrentLevel = 0;
    gHeistSkipTutorial = skipTutorial;
    genWorld();
    buildGeom();
    gameState = STATE_GAME;
    settings.masterVol = 0.0f;
}

#include "dev_selftest_ui.h"

// One scripted step every few frames, so each change gets rendered before its screenshot.
inline void selftestFrame() {
    if (!gSelftest) return;
    static int frame = 0;
    static int step = 0;
    static Vec3 legsFrom;
    static float wait = 0.0f;  // seconds to let the game run before the next step
    if (wait > 0.0f) { wait -= dTime; return; }
    if (++frame % 12 != 0) return;
    Vec3 base = heistCellCenter(heist.baseX, heist.baseZ);
    switch (step++) {
    case 0: selftestMenuShots(); selftestStartRun(false); selftestLog("tutorial starts in the garage", heist.stage == HEIST_TUT_GARAGE && heistPosInGarage(cam.pos)); break;
    case 1: selftestShot("01_garage"); selftestPauseShot(0); cam.yaw = 3.1415927f; break;
    case 2: selftestShot("02_whiteboard"); selftestPauseShot(1); selftestLookAt(heist.padPos + Vec3(-3.0f, 0, 2.0f), heist.padPos); break;
    case 3: selftestShot("03_pallet"); selftestPauseShot(2); selftestLookAt(base + Vec3(0, 0, 3.0f), heist.portalPos); break;
    case 4: selftestShot("04_portal_from_backrooms"); selftestLog("leaving the garage starts the haul", heist.stage == HEIST_TUT_HAUL); break;
    case 5: case 7: case 9: {
        if (step == 10) heistBreakWall(heist.baseX - 1, heist.baseZ - 1);
        bool picked = selftestPickNearest();
        selftestLog("lesson loot picked", picked && carryCount(heistMyCarry()) == 1);
        if (step == 8) selftestShot("05_dark_lesson");
        break;
    }
    case 6: case 8: case 10:
        cam.pos = Vec3(heist.padPos.x, PH, heist.padPos.z + 1.0f);
        selftestLog("delivered to the pallet", heistHostAction(heistMe(), HEIST_ACT_DEPOSIT));
        break;
    case 11: selftestLog("core appears after three deliveries", heist.stage == HEIST_TUT_CORE && heist.tutDelivered == 3); selftestLookAt(heist.corePos + Vec3(2.5f, 0, 0), heist.corePos); break;
    case 12: selftestShot("06_core"); selftestLog("core picked", selftestPickNearest() && heistCarriesCore(heistMe())); break;
    case 13: cam.pos = Vec3(heist.portalPos.x, PH, heist.portalPos.z + 2.0f); cam.yaw = 3.1415927f; break;
    case 14: selftestShot("07_accident"); selftestLog("core in the doorway breaks the portal", heist.stage == HEIST_ACCIDENT && getCellWorld(heist.baseX, heist.baseZ - 3) == 1); break;
    case 15: heist.stageTimer = HEIST_ACCIDENT_SECONDS; gHeistFlash = 0.0f; lightsOutTimer = 0.001f; break;
    case 16: selftestLookAt(base + Vec3(0, 0, 1.0f), heist.portalPos); break;
    case 17: selftestShot("08_wreck"); selftestLog("run starts, loot is scattered", heist.stage == HEIST_RUN && worldItems.size() >= 10); selftestLookAt(rigWorld(0.9f, -7.2f), rigWorld(0.0f, -1.0f)); break;
    case 18: selftestShot("09_drill"); activeDeviceSlot = 2; break;
    case 19: {
        selftestShot("10_scanner");
        activeDeviceSlot = 1;
        int guard = 0;
        while (heist.quotaHave < heist.quotaNeed && guard++ < 20) {
            if (!selftestPickNearest()) break;
            Vec3 hopper = rigWorld(-0.3f, -0.6f);
            cam.pos = Vec3(hopper.x, PH, hopper.z);
            heistHostAction(heistMe(), HEIST_ACT_DEPOSIT);
        }
        selftestLog("drill fed until ready", heistDrillReady());
        selftestLookAt(rigWorld(0.2f, -1.4f), rigWorld(-0.1f, 0.2f));
        break;
    }
    case 20: selftestShot("11_drill_ready"); selftestLog("drill started", heistHostAction(heistMe(), HEIST_ACT_START_DRILL)); break;
    case 21: heist.stageTimer = HEIST_DRILL_SECONDS; break;
    case 22: selftestLog("drill took the squad one floor down", gCurrentLevel == 1 && heist.stage == HEIST_RUN && heist.hasKiosk); gHeistFlash = 0.0f; selftestLookAt(heist.kioskPos + Vec3(2.2f, 0, 2.4f), heist.kioskPos); break;
    case 23: for (int i = 0; i < 8 && carryCount(heistMyCarry()) == 0; i++) selftestPickNearest(); cam.pos = Vec3(heist.kioskPos.x + 1.6f, PH, heist.kioskPos.z + 1.8f); break;
    case 24: selftestShot("12_kiosk"); selftestLog("loot sold for cash", heistHostAction(heistMe(), HEIST_ACT_SELL) && heist.cash > 0); break;
    case 25: selftestLog("battery bought", heistHostBuy(heistMe(), SHOP_BATTERY)); heist.cash = extractionPrice(1); break;
    case 26: selftestShot("13_can_extract"); selftestLog("extraction bought", heistHostBuy(heistMe(), SHOP_EXTRACT)); break;
    case 27: selftestLog("run complete", playerEscaped && gRunComplete); break;
    case 28: selftestShot("14_report"); gameState = STATE_GUIDE; break;
    case 29: selftestShot("15_guide"); selftestStartRun(true); selftestLog("veterans start at the drill", heist.stage == HEIST_RUN && !heistPosInGarage(cam.pos)); break;
    case 30: selftestShot("16_skip_start"); isPlayerDead = true; break;
    case 31: selftestShot("17_death"); isPlayerDead = false; gCurrentLevel = 4; genWorld(); buildGeom(); break;
    case 32: {
        int cracks = 0;
        for (int dx = -20; dx <= 20; dx++) for (int dz = -20; dz <= 20; dz++) cracks += isCrackedWall(heist.baseX + dx, heist.baseZ + dz) ? 1 : 0;
        selftestLog("deep floor: themed props, loot, kiosk, cracked walls", heist.hasKiosk && worldItems.size() >= 12 && cracks > 0 && !mapProps.empty());
        selftestLookAt(rigWorld(0.9f, -7.2f), rigWorld(0.0f, -1.0f));
        break;
    }
    case 33: selftestShot("18_deep_floor"); selftestLog("squad arrives in the break room, the rig crawls", rigTravelled() > 0.01f && rigMoving()); break;
    case 34:
        heist.rigFault = RIG_FAULT_BELT;
        heist.rigFaultSecs = 0.0f;
        selftestLookAt(rigWorld(-2.6f, 2.9f), rigWorld(-1.3f, 0.6f));
        break;
    case 35: {
        selftestShot("19_rig_broken");
        selftestLog("a breakdown stops the rig and shrinks the haven", !rigMoving() && rigHavenNow() == RIG_HAVEN_RESERVE_RADIUS);
        Vec3 cover = rigWorld(-1.85f, 0.85f);
        cam.pos = Vec3(cover.x, PH, cover.z);
        selftestLog("fixed at the engine cover", heistFindPrompt().kind == HEIST_PROMPT_REPAIR && rigHostRepair(heistMe()) && rigMoving());
        break;
    }
    case 36:
        entityMgr.spawnEntity(ENTITY_STALKER, rigWorld(0.0f, -6.2f), nullptr, 0, 0);
        selftestLookAt(rigWorld(0.25f, -3.4f), rigWorld(-0.2f, 0.0f));
        break;
    case 37: {
        selftestShot("20_break_room");
        bool keptOut = !entityMgr.entities.empty() && gRigSheltered && gRigInCabin;
        for (const auto& e : entityMgr.entities) {
            if (entityMgr.inHaven(e.pos) && (e.mood != MOOD_RETREAT || e.state == ENT_ATTACKING)) keptOut = false;
        }
        selftestLog("the haven turns monsters away", keptOut);
        heist.rigCharge = 1.0f;
        Vec3 charger = rigWorld(0.6f, -0.6f);
        cam.pos = Vec3(charger.x, PH, charger.z);
        int before = invBattery;
        selftestLog("charger hands out a battery", heistFindPrompt().kind == HEIST_PROMPT_CHARGER && rigHostTakeBattery(heistMe()) && invBattery == before + 1);
        break;
    }
    case 38: {
        // fast-forward five minutes of crawling: the lane has to be open, loot has to keep coming
        entityMgr.entities.clear();
        int walls = 0;
        for (int i = 0; i < 360; i++) {
            heist.drillPos.x += (float)rigDirX(heist.rigDir) * 0.25f;
            heist.drillPos.z += (float)rigDirZ(heist.rigDir) * 0.25f;
            cam.pos = Vec3(heist.drillPos.x, PH, heist.drillPos.z);
            updateVisibleChunks(cam.pos.x, cam.pos.z);
            rigCarveLane();
            walls += gRigCrunch;
            gRigCrunch = 0;
            rigHostLayLoot();
        }
        bool open = true;
        for (float lz = RIG_CABIN_BACK; lz <= RIG_NOSE; lz += 0.5f) {
            Vec3 at = rigWorld(0.0f, lz);
            if (collideWorld(at.x, at.z, RIG_HALF_W)) open = false;
        }
        int ahead = 0, alive = 0;
        for (const auto& it : worldItems) {
            if (!it.active) continue;
            alive++;
            if (nearPoint2D(it.pos, heist.drillPos, 60.0f)) ahead++;
        }
        if (gSelftestLog) std::fprintf(gSelftestLog, "     rig: %.0f m, %d walls eaten, %d items within 60 m, %d total\n", rigTravelled(), walls, ahead, alive);
        selftestLog("after 90 m the lane is open and there is loot around the rig", open && walls > 0 && ahead >= 4 && alive <= MAX_SYNC_ITEMS);
        selftestLookAt(rigWorld(1.6f, -10.5f), rigWorld(0.0f, -1.0f));
        break;
    }
    case 39: selftestShot("21_tunnel"); cam.pitch = -1.0f; gLegsScriptFwd = 1.0f; legsFrom = cam.pos; wait = 1.2f; break;
    case 40: {
        // the player is the body of the leg simulation: it walks down the lane, looking at its feet
        selftestShot("22_legs_walk");
        const LegSim& sim = gLegs.sim;
        float walked = sqrtf((cam.pos.x - legsFrom.x) * (cam.pos.x - legsFrom.x) + (cam.pos.z - legsFrom.z) * (cam.pos.z - legsFrom.z));
        float eyeY = gLegs.eye.y;
        if (gSelftestLog) std::fprintf(gSelftestLog, "     legs: walked %.2f m in %d steps, %.2f m/s, eyes at %.2f m\n", walked, sim.stepsTaken, sim.speed, eyeY);
        selftestLog("the player walks on simulated legs", gLegs.live && walked > 0.2f && walked < 20.0f && sim.stepsTaken > 0 && eyeY > 1.3f && eyeY < 1.8f);
        gLegsScriptFwd = 0.0f;
        wait = 1.5f;
        break;
    }
    case 41: selftestShot("23_legs_stand"); selftestLog("and comes to a stand", gLegs.sim.speed < 0.2f && !gLegs.sim.walking); break;
    default:
        if (gSelftestLog) std::fprintf(gSelftestLog, "done, %d failure(s)\n", gSelftestFails);
        if (gSelftestLog) std::fclose(gSelftestLog);
        glfwSetWindowShouldClose(gWin, 1);
        break;
    }
}
