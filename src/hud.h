#pragma once
#include "menu_hud.h"
#include "coop.h"
#include "perf_overlay.h"
#include "progression.h"
#include "smile_event.h"
#include "item_types.h"
#include "ui_migration_toggle.h"
#include "ui_primitives.h"
#include "ui_menu_immersive.h"
#include "hud_inline_helpers.h"

inline void drawProvisionalContextualFeedbackSpike(const UiPrimitiveTheme& theme, UiPrimitiveTone eventTone, UiPrimitiveTone promptTone){
    if(!useT12pContextualFeedbackSpike()) return;

    float healthRatio = uiHudClamp01(playerHealth / 100.0f);
    float sanityRatio = uiHudClamp01(playerSanity / 100.0f);
    float pressure = (1.0f - healthRatio) * 0.52f + (1.0f - sanityRatio) * 0.36f;
    if(playerDowned) pressure = 1.0f;
    if(multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime())) pressure += 0.18f;
    pressure = uiHudClamp01(pressure);

    UiPrimitiveTone pressureTone = UI_PRIMITIVE_TONE_DEFAULT;
    if(playerDowned || eventTone == UI_PRIMITIVE_TONE_CRITICAL) pressureTone = UI_PRIMITIVE_TONE_CRITICAL;
    else if(promptTone == UI_PRIMITIVE_TONE_WARNING || pressure >= 0.45f) pressureTone = UI_PRIMITIVE_TONE_WARNING;

    char spikeBuf[128];
    std::snprintf(spikeBuf, sizeof(spikeBuf), "T12P CONTEXT SPIKE  PRESSURE %.0f%%", pressure * 100.0f);
    drawUiStatusIndicatorPrimitive(theme, 0.10f, -0.56f, 0.66f, -0.50f, spikeBuf, pressureTone);

    UiColorToken accent = uiPrimitiveToneColor(pressureTone);
    float accentAlpha = 0.08f + pressure * 0.12f;
    uiVRect(0.10f, -0.58f, 0.66f, -0.575f, accent, accentAlpha);
}

inline void drawImmersiveHudMeter(const UiPrimitiveTheme& theme, const char* title, float value, float maxValue, float vl, float vb, float vr, float vt){
    float ratio = (maxValue > 0.0f) ? uiHudClamp01(value / maxValue) : 0.0f;
    UiPrimitiveTone tone = uiHudToneForRatio(ratio);
    drawUiPanelPrimitive(theme, vl, vb, vr, vt, title, tone);
    UiColorToken tc = uiPrimitiveToneColor(tone);
    float bl = uiX(vl + 0.02f), br = uiX(vr - 0.02f), bb = uiY(vb + 0.02f), bt = bb + uiH(0.028f);
    uiPrimitiveRect(bl, bb, br, bt, UiColor::kOverlayWarm, 0.35f);
    uiPrimitiveRect(bl + uiW(0.002f), bb + uiH(0.002f), bl + (br - bl - uiW(0.004f)) * ratio, bt - uiH(0.002f), tc, 0.82f);
    char vb2[48]; std::snprintf(vb2, sizeof(vb2), "%.0f%%", ratio * 100.0f);
    uiPrimitiveText(vb2, bl, bt + uiH(0.006f), UiTypography::kScaleHint, theme.text, 0.92f, UI_FONT_ROLE_BODY);
}

inline void buildImmersiveHudObjectiveLines(char* primary, int primarySize, char* support, int supportSize){
    if(!primary || primarySize < 2 || !support || supportSize < 2) return;
    primary[0] = '\0';
    support[0] = '\0';

    if(multiState==MULTI_IN_GAME){
        int switchCount = (coop.switchOn[0]?1:0) + (coop.switchOn[1]?1:0);
        std::snprintf(primary, primarySize, "CO-OP: SWITCHES %d/2", switchCount);
        if(coop.doorOpen) std::snprintf(support, supportSize, "DOOR OPEN - EXIT WHEN CONTRACT READY");
        else std::snprintf(support, supportSize, "DOOR LOCKED - HOLD BOTH SWITCHES");
        return;
    }

    buildVoidShiftObjectiveLine(primary, primarySize);
    buildVoidShiftSupportLine(support, supportSize);
}

inline void buildImmersiveHudPromptLine(char* out, int outSize){
    if(!out || outSize < 2) return;
    out[0] = '\0';

    if(playerDowned){
        std::snprintf(out, outSize, "[E] USE PLUSH TO SELF-REVIVE");
        return;
    }

    if(nearbyWorldItemId>=0){
        std::snprintf(out, outSize, "%s", worldItemPickupPrompt(nearbyWorldItemType));
        return;
    }

    char actionPrompt[96];
    actionPrompt[0] = '\0';
    buildVoidShiftInteractPrompt(cam.pos, actionPrompt, 96);
    if(actionPrompt[0] != '\0'){
        std::snprintf(out, outSize, "%s", actionPrompt);
        return;
    }

    bool nearExit = nearPoint2D(cam.pos, coop.doorPos, 2.4f);
    if(nearExit){
        if(multiState==MULTI_IN_GAME){
            if(coop.doorOpen && isStoryExitReady()) std::snprintf(out, outSize, "[E] EXIT LEVEL");
            else std::snprintf(out, outSize, "OPEN DOOR + COMPLETE CONTRACT TO EXIT");
        }else{
            if(isStoryExitReady()) std::snprintf(out, outSize, "[E] EXIT LEVEL");
            else std::snprintf(out, outSize, "COMPLETE CONTRACT TO UNLOCK EXIT");
        }
        return;
    }

    std::snprintf(out, outSize, "NO CONTEXT ACTION");
}

inline void buildImmersiveHudKeyEventLine(char* out, int outSize){
    if(!out || outSize < 2) return;
    out[0] = '\0';

    if(playerDowned){
        std::snprintf(out, outSize, "CRITICAL: DOWNED %.0fs", playerDownedTimer);
        return;
    }
    if(multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime())){
        std::snprintf(out, outSize, "NETWORK UNSTABLE - RECONNECT MAY OCCUR");
        return;
    }
    if(falseDoorTimer>0.0f){
        std::snprintf(out, outSize, "EVENT: FALSE DOOR SHIFT");
        return;
    }
    if(echoStatusTimer>0.0f){
        std::snprintf(out, outSize, "%s", echoStatusText);
        return;
    }
    if(squadCalloutTimer>0.0f){
        std::snprintf(out, outSize, "%s", squadCalloutText);
        return;
    }

    std::snprintf(out, outSize, "EVENT CHANNEL CLEAR");
}

inline void drawImmersiveGameplayHudV1(){
    UiPrimitiveTheme theme = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(vhsTime);

    // Status meters — compact, tucked into corners
    drawImmersiveHudMeter(theme, "HEALTH",     playerHealth,     100.0f, -0.66f, -0.96f, -0.22f, -0.86f);
    drawImmersiveHudMeter(theme, "SANITY",     playerSanity,     100.0f, -0.66f, -0.84f, -0.22f, -0.74f);
    drawImmersiveHudMeter(theme, "STAMINA",    playerStamina,    125.0f, -0.66f, -0.72f, -0.22f, -0.62f);
    drawImmersiveHudMeter(theme, "FLASHLIGHT", flashlightBattery,100.0f,  0.22f, -0.96f,  0.66f, -0.86f);

    // Objective — top-left, compact
    char oP[128], oS[128];
    buildImmersiveHudObjectiveLines(oP, 128, oS, 128);
    drawUiPanelPrimitive(theme, -0.66f, 0.86f, -0.10f, 0.96f, "OBJECTIVE", UI_PRIMITIVE_TONE_DEFAULT);
    uiPrimitiveText(oP, uiX(-0.62f), uiY(0.90f), UiTypography::kScaleMeta, theme.text, 0.88f, UI_FONT_ROLE_META);
    uiPrimitiveText(oS, uiX(-0.62f), uiY(0.87f), UiTypography::kScaleMeta, theme.mutedText, 0.75f, UI_FONT_ROLE_META);

    // Events — top-right, compact
    char eL[128]; buildImmersiveHudKeyEventLine(eL, 128);
    UiPrimitiveTone eT = (playerDowned || (multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime()))) ? UI_PRIMITIVE_TONE_CRITICAL : UI_PRIMITIVE_TONE_WARNING;
    drawUiPanelPrimitive(theme, 0.10f, 0.86f, 0.66f, 0.96f, "EVENTS", eT);
    uiPrimitiveText(eL, uiX(0.14f), uiY(0.90f), UiTypography::kScaleMeta, theme.text, 0.88f, UI_FONT_ROLE_META);

    // Prompt — bottom-right
    char pL[128]; buildImmersiveHudPromptLine(pL, 128);
    UiPrimitiveTone pT = (pL[0] == '[') ? UI_PRIMITIVE_TONE_WARNING : UI_PRIMITIVE_TONE_DEFAULT;
    if(playerDowned) pT = UI_PRIMITIVE_TONE_CRITICAL;
    drawUiPanelPrimitive(theme, 0.10f, -0.72f, 0.66f, -0.58f, "PROMPT", pT);
    uiPrimitiveText(pL, uiX(0.14f), uiY(-0.66f), UiTypography::kScaleHint, theme.text, 0.90f, UI_FONT_ROLE_BODY);

    if(activeDeviceSlot == 2){
        UiPrimitiveTone st = scannerOverheated ? UI_PRIMITIVE_TONE_CRITICAL : (scannerHeat>0.75f ? UI_PRIMITIVE_TONE_WARNING : UI_PRIMITIVE_TONE_DEFAULT);
        drawUiPanelPrimitive(theme, 0.34f, -0.56f, 0.66f, -0.44f, "SCANNER", st);
        char sb[96];
        if(scannerOverheated) std::snprintf(sb,sizeof(sb),"OVERHEAT  SIG %.0f%%",uiHudClamp01(scannerSignal)*100.0f);
        else std::snprintf(sb,sizeof(sb),"SIG %.0f%%  HEAT %.0f%%",uiHudClamp01(scannerSignal)*100.0f,uiHudClamp01(scannerHeat)*100.0f);
        uiPrimitiveText(sb, uiX(0.38f), uiY(-0.52f), UiTypography::kScaleMeta, theme.text, 0.88f, UI_FONT_ROLE_META);
    }

    drawProvisionalContextualFeedbackSpike(theme, eT, pT);
    uiPrimitiveEndFrame();
}

inline void drawImmersiveSessionEndOverlay(bool escaped, float tm){
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, true);
    UiPrimitiveTheme theme = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    if(escaped){
        uiImmersiveDrawHeader(theme, "EXTRACTION COMPLETE", "SESSION END", false);
        drawUiModalPrimitive(theme, "YOU ESCAPED", "EXTRACTION ROUTE SECURED", UI_PRIMITIVE_TONE_DEFAULT);
    }else{
        uiImmersiveDrawHeader(theme, "CONTRACT FAILED", "SESSION END", true);
        drawUiModalPrimitive(theme, "YOU DIED", gDeathReason, UI_PRIMITIVE_TONE_CRITICAL);
    }

    int m=(int)(gSurvivalTime/60),s=(int)gSurvivalTime%60;
    char tb[64]; snprintf(tb,64,"SURVIVED %d:%02d",m,s);
    uiPrimitiveTextCentered(tb, 0.0f, uiY(-0.24f), UiTypography::kScaleHint, UiColor::kStateWarning, 0.78f, UI_FONT_ROLE_BODY);
    uiImmersiveDrawFooter(theme, "ENTER/A CONTINUE   ESC/B MAIN MENU");

    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

#if BR_UI_COMPILE_ALLOW_LEGACY || BR_UI_COMPILE_ALLOW_NEW
inline void drawUiParityPath(){
    if(gameState==STATE_MENU) drawMenu(vhsTime);
    else if(gameState==STATE_GUIDE) drawGuideScreen();
    else if(gameState==STATE_MULTI) drawMultiMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_HOST) drawHostLobbyScreen(vhsTime,netMgr.getPlayerCount());
    else if(gameState==STATE_MULTI_JOIN) drawJoinMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_WAIT) drawWaitingScreen(vhsTime);
    else if(gameState==STATE_PAUSE){
        if(multiState==MULTI_IN_GAME) drawMultiPause(netMgr.getPlayerCount());
        else drawPause();
    }else if(gameState==STATE_SETTINGS||gameState==STATE_SETTINGS_PAUSE) drawSettings(gameState==STATE_SETTINGS_PAUSE);
    else if(gameState==STATE_KEYBINDS||gameState==STATE_KEYBINDS_PAUSE) drawKeybindsMenu(gameState==STATE_KEYBINDS_PAUSE, menuSel, keybindCaptureIndex);
    else if(gameState==STATE_INTRO) drawIntro(storyMgr.introLine,storyMgr.introTimer,storyMgr.introLineTime,INTRO_LINES);
    else if(gameState==STATE_NOTE&&storyMgr.readingNote&&storyMgr.currentNote>=0)
        drawNote(storyMgr.currentNote,NOTE_TITLES[storyMgr.currentNote],NOTE_CONTENTS[storyMgr.currentNote]);
    else if(gameState==STATE_GAME){
        const bool immersiveUiPath = useNewUiMigrationPath();
        gSurvivalTime=survivalTime;
        if(playerEscaped){
            if(immersiveUiPath) drawImmersiveSessionEndOverlay(true, vhsTime);
            else drawEscape(vhsTime);
        }
        else if(isPlayerDead){
            if(immersiveUiPath) drawImmersiveSessionEndOverlay(false, vhsTime);
            else drawDeath(vhsTime);
        }
        else{
            const bool immersiveHudActive = useNewUiMigrationPath();
            drawDamageOverlay(damageFlash,playerHealth);
            drawSurvivalTime(survivalTime);

            // === DEBUG MODE: FPS/telemetry overlay ===
            if(settings.debugMode){
                char fpsBuf[48];
                snprintf(fpsBuf,48,"FPS %.0f",gPerfFpsSmoothed);
                char fgBuf[80];
                formatFrameGenPipeline(fgBuf,80,gPerfRefreshHz,gPerfFrameGenBaseCap,settings.frameGenMode,settings.vsync);
                char upBuf[96];
                formatUpscalePipeline(upBuf,96,settings.upscalerMode,renderW,renderH,winW,winH);
                if(gHudTelemetryVisible){
                    char pingBuf[48];
                    if(multiState==MULTI_IN_GAME) snprintf(pingBuf,48,"PING %.0fms",netMgr.rttMs);
                    else snprintf(pingBuf,48,"PING --");
                    char netBuf[40];
                    if(multiState==MULTI_IN_GAME) snprintf(netBuf,40,"NET %s",netMgr.connectionQualityLabel((float)glfwGetTime()));
                    else snprintf(netBuf,40,"NET --");
                    char perfRow[300];
                    snprintf(perfRow,300,"%s | %s | %s | %s | %s",fpsBuf,fgBuf,upBuf,pingBuf,netBuf);
                    drawHudText(perfRow,-0.95f,0.95f,1.20f,0.88f,0.93f,0.78f,0.98f);
                }else{
                    drawHudText("HUD HIDDEN",-0.95f,0.95f,1.00f,0.80f,0.86f,0.74f,0.95f);
                }
            }

            if(immersiveHudActive){
                drawImmersiveGameplayHudV1();
            }else{
                if(playerHealth<100)drawHealthBar(playerHealth);
                if(playerSanity<100)drawSanityBar(playerSanity);
                drawStaminaBar(playerStamina);
                if(flashlightBattery<100)drawFlashlightBattery(flashlightBattery,flashlightOn);
                if(playerDowned){
                    char downBuf[64];
                    snprintf(downBuf,64,"DOWNED %.0fs",playerDownedTimer);
                    drawHudText(downBuf,-0.20f,-0.22f,1.45f,0.96f,0.52f,0.44f,0.95f);
                    drawHudTextCentered("[E] USE PLUSH TO SELF-REVIVE",0.0f,-0.30f,1.2f,0.90f,0.72f,0.62f,0.92f);
                }
                {
                    char contractBuf[128];
                    buildVoidShiftObjectiveLine(contractBuf, 128);
                    drawHudText(contractBuf,-0.95f,0.88f,1.08f,0.88f,0.90f,0.72f,0.95f);
                    char supportBuf[128];
                    buildVoidShiftSupportLine(supportBuf, 128);
                    drawHudText(supportBuf,-0.95f,0.85f,0.98f,0.78f,0.88f,0.74f,0.90f);
                    char attBuf[96];
                    if(isParkingLevel(gCurrentLevel)){
                        snprintf(attBuf,96,"ATTENTION %.0f%%  CO %.0f%%  RESO %.0f%%",attentionLevel,coLevel,resonatorBattery);
                    }else{
                        snprintf(attBuf,96,"ATTENTION %.0f%%  RESO %.0f%%",attentionLevel,resonatorBattery);
                    }
                    drawHudText(attBuf,-0.95f,0.79f,1.02f,0.78f,0.86f,0.76f,0.92f);
                }
            }
            if(activeDeviceSlot == 2){
                float y = -0.70f;
                drawOverlayRectNdc(0.48f, y - 0.14f, 0.95f, y + 0.07f, 0.04f, 0.09f, 0.10f, 0.42f);
                drawOverlayRectNdc(0.49f, y - 0.13f, 0.94f, y + 0.06f, 0.08f, 0.18f, 0.20f, 0.18f);
                drawHudText("SCANNER",0.52f,y,1.15f,0.55f,0.82f,0.86f,0.92f);
                if(scannerOverheated){
                    drawHudText("OVERHEAT",0.52f,y-0.06f,1.05f,0.92f,0.44f,0.30f,0.92f);
                }else if(scannerHeat > 0.02f){
                    float h = scannerHeat;
                    if(h<0.0f) h=0.0f;
                    if(h>1.0f) h=1.0f;
                    drawSlider(0.66f,y-0.06f,0.30f,h,0.95f,0.62f,0.20f);
                }
                float sig = scannerSignal;
                if(sig < 0.0f) sig = 0.0f;
                if(sig > 1.0f) sig = 1.0f;
                drawSlider(0.66f,y,0.30f,sig,0.45f,0.85f,0.95f);
            }

            // === DEBUG MODE: full objective block (top-right) ===
            if(settings.debugMode){
                float blockX = 0.44f;
                float blockY = 0.90f;
                const char* phaseNames[] = {"INTRO","EXPLORATION","SURVIVAL","DESPERATION"};
                int phaseIdx = (int)storyMgr.getPhase();
                if(phaseIdx < 0) phaseIdx = 0;
                if(phaseIdx > 3) phaseIdx = 3;
                drawHudText("OBJECTIVE",blockX,blockY,1.28f,0.90f,0.94f,0.72f,0.97f);
                blockY -= 0.07f;
                if(multiState==MULTI_IN_GAME){
                    int switchCount = (coop.switchOn[0]?1:0)+(coop.switchOn[1]?1:0);
                    char objProgress[64];
                    snprintf(objProgress,64,"SWITCHES %d/2",switchCount);
                    drawHudText(objProgress,blockX,blockY,1.18f,0.86f,0.90f,0.68f,0.95f);
                    blockY -= 0.06f;
                    drawHudText(coop.doorOpen?"DOOR OPEN":"DOOR LOCKED",blockX,blockY,1.16f,0.88f,0.84f,0.62f,0.95f);
                    blockY -= 0.06f;
                    if(!coop.doorOpen) drawHudText("ACTION HOLD 2 SWITCHES",blockX,blockY,1.02f,0.82f,0.86f,0.62f,0.90f);
                    blockY -= 0.06f;
                }else{
                    char contractLine[96];
                    buildVoidShiftObjectiveLine(contractLine, 96);
                    drawHudText(contractLine,blockX,blockY,1.02f,0.86f,0.90f,0.68f,0.95f);
                    blockY -= 0.06f;
                    char sideLine[96];
                    buildVoidShiftSupportLine(sideLine, 96);
                    drawHudText(sideLine,blockX,blockY,0.98f,0.78f,0.88f,0.74f,0.91f);
                    blockY -= 0.06f;
                    drawHudText(isStoryExitReady()?"EXIT READY":"EXIT LOCKED",blockX,blockY,1.08f,0.72f,0.86f,0.90f,0.93f);
                    blockY -= 0.06f;
                    drawHudText(level2VentDone?"ACTION: CONTRACT + VENT ONLINE":"ACTION: COMPLETE CONTRACT STEPS",blockX,blockY,1.00f,0.82f,0.86f,0.62f,0.90f);
                    blockY -= 0.06f;
                }
                char phaseBuf[64];
                snprintf(phaseBuf,64,"PHASE %s",phaseNames[phaseIdx]);
                drawHudText(phaseBuf,blockX,blockY,1.14f,0.86f,0.80f,0.56f,0.94f);
                blockY -= 0.06f;
                char levelBuf[48];
                buildLevelLabel(gCurrentLevel, levelBuf, 48);
                drawHudText(levelBuf,blockX,blockY,1.06f,0.80f,0.86f,0.66f,0.93f);
                blockY -= 0.06f;
                char invBuf[64];
                snprintf(invBuf,64,"SUPPLIES B:%d",invBattery);
                drawHudText(invBuf,blockX,blockY,1.06f,0.76f,0.86f,0.70f,0.93f);
                blockY -= 0.06f;
                if(falseDoorTimer>0) {
                    drawHudText("EVENT FALSE DOOR SHIFT",blockX,blockY,1.05f,0.95f,0.45f,0.36f,0.92f);
                    blockY -= 0.05f;
                }
            }

            if(!immersiveHudActive && multiState==MULTI_IN_GAME && !coop.doorOpen){
                if(settings.debugMode){
                    if(nearPoint2D(cam.pos, coop.switches[0], 2.6f)||nearPoint2D(cam.pos, coop.switches[1], 2.6f))
                        drawHudTextCentered("HOLD SWITCH POSITION",0.0f,-0.35f,1.4f,0.75f,0.8f,0.55f,0.90f);
                }
            }
            if(!immersiveHudActive && multiState!=MULTI_IN_GAME){
                bool nearExit = nearPoint2D(cam.pos, coop.doorPos, 2.4f);
                if(nearExit && settings.debugMode){
                    if(isStoryExitReady()) drawHudTextCentered("[E] EXIT LEVEL",0.0f,-0.35f,1.4f,0.75f,0.88f,0.70f,0.95f);
                    else drawHudTextCentered("COMPLETE CONTRACT TO UNLOCK EXIT",0.0f,-0.35f,1.2f,0.88f,0.72f,0.58f,0.93f);
                }
            }else{
                if(!immersiveHudActive){
                    bool nearExit = nearPoint2D(cam.pos, coop.doorPos, 2.4f);
                    if(nearExit && settings.debugMode){
                        if(coop.doorOpen && isStoryExitReady()) drawHudTextCentered("[E] EXIT LEVEL",0.0f,-0.35f,1.4f,0.75f,0.88f,0.70f,0.95f);
                        else drawHudTextCentered("OPEN DOOR + COMPLETE CONTRACT TO EXIT",0.0f,-0.35f,1.25f,0.88f,0.72f,0.58f,0.93f);
                    }
                }
            }
            if(!immersiveHudActive && nearbyWorldItemId>=0 && settings.debugMode){
                drawHudTextCentered(worldItemPickupPrompt(nearbyWorldItemType),0.0f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.9f);
            }
            if(multiState!=MULTI_IN_GAME){
                Vec3 resonancePos(0,0,0);
                if(getVoidShiftResonanceTarget(resonancePos)){
                    Vec3 d = resonancePos - cam.pos;
                    d.y = 0;
                    float dist = d.len();
                    if(settings.debugMode){
                        char echoBuf[72];
                        snprintf(echoBuf,72,"RESONANCE %.0fm",dist);
                        drawHudText(echoBuf,-0.95f,0.50f,1.18f,0.62f,0.85f,0.86f,0.90f);
                    }
                }

                if(settings.debugMode){
                    float sx = 0.0f, sy = 0.0f;
                    Vec3 cartLbl = npcCartographerPos + Vec3(0,1.7f,0);
                    if(npcCartographerActive && hasLabelLineOfSight(cartLbl) && projectToScreen(cartLbl, sx, sy)){
                        drawHudTextCentered("CARTOGRAPHER", sx, sy, 1.0f, 0.76f, 0.90f, 0.74f, 0.92f);
                    }
                    Vec3 dispLbl = npcDispatcherPhonePos + Vec3(0,1.7f,0);
                    if(npcDispatcherActive && hasLabelLineOfSight(dispLbl) && projectToScreen(dispLbl, sx, sy)){
                        drawHudTextCentered("DISPATCH", sx, sy, 1.0f, 0.78f, 0.84f, 0.96f, 0.92f);
                    }
                    Vec3 survLbl = npcLostSurvivorPos + Vec3(0,1.7f,0);
                    if(npcLostSurvivorActive && hasLabelLineOfSight(survLbl) && projectToScreen(survLbl, sx, sy)){
                        drawHudTextCentered("LOST SURVIVOR", sx, sy, 1.0f, 0.92f, 0.78f, 0.64f, 0.92f);
                    }
                }

                if(!immersiveHudActive){
                    char actionPrompt[96];
                    buildVoidShiftInteractPrompt(cam.pos, actionPrompt, 96);
                    if(actionPrompt[0] != '\0'){
                        drawHudTextCentered(actionPrompt,0.0f,-0.43f,1.28f,0.8f,0.84f,0.62f,0.92f);
                    }
                }
            }
            if(!immersiveHudActive && settings.debugMode && multiState!=MULTI_IN_GAME && echoStatusTimer>0.0f){
                drawHudTextCentered(echoStatusText,0.0f,0.62f,1.18f,0.7f,0.86f,0.9f,0.92f);
            }
            if(minimapEnabled) drawMinimapOverlay();
            if(storyMgr.hasHallucinations())drawHallucinationEffect((50.0f-playerSanity)/50.0f);
            if(multiState==MULTI_IN_GAME)drawMultiHUD(netMgr.getPlayerCount(),netMgr.isHost);
            if(!immersiveHudActive && squadCalloutTimer > 0.0f) drawHudTextCentered(squadCalloutText,0.0f,-0.52f,1.08f,0.88f,0.84f,0.62f,0.92f);
            if(multiState==MULTI_IN_GAME){
                for(int i=0;i<MAX_PLAYERS;i++){
                    if(i==netMgr.myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
                    if(!playerInterpReady[i]) continue;
                    // Use interpolated position for stable nametag
                    Vec3 playerPos = playerRenderPos[i];
                    Vec3 wp = playerPos + Vec3(0, 2.0f, 0);
                    float sx=0, sy=0;
                    // Check distance first for optimization
                    Vec3 dd = playerPos - cam.pos;
                    float dist = dd.len();
                    if(dist > 40.0f) continue;
                    if(dist < 0.5f) continue; // Too close
                    // Project to screen using current camera
                    if(!projectToScreen(wp, sx, sy)) continue;
                    // Check line of sight from camera to player head
                    if(!hasLabelLineOfSight(wp)) continue;
                    const char* nm = netMgr.players[i].name[0] ? netMgr.players[i].name : "Player";
                    // Fade based on distance
                    float alpha = 1.0f - (dist - 20.0f) / 20.0f;
                    if(alpha > 1.0f) alpha = 1.0f;
                    if(alpha < 0.3f) alpha = 0.3f;
                    drawHudTextCentered(nm, sx, sy, 1.0f, 0.85f, 0.9f, 0.7f, alpha);
                }

                netMgr.updatePingMarkTtl(dTime);
                if(netMgr.pingMarkReceived && netMgr.pingMarkTtl > 0.0f){
                    float psx = 0.0f, psy = 0.0f;
                    Vec3 pmark = netMgr.pingMarkPos + Vec3(0, 1.5f, 0);
                    if(projectToScreen(pmark, psx, psy)){
                        float alpha = 0.35f + 0.6f * (netMgr.pingMarkTtl / 6.0f);
                        if(alpha > 0.95f) alpha = 0.95f;
                        drawHudTextCentered("PING", psx, psy, 1.22f, 0.90f, 0.85f, 0.50f, alpha);
                        drawHudTextCentered("+", psx, psy - 0.04f, 1.35f, 0.96f, 0.88f, 0.58f, alpha);
                    }
                }
            }
            // === DEBUG MODE: trap status text, floor hazards, anomaly lock, minimap state, perf graph ===
            if(settings.debugMode){
                const char* mmState = minimapEnabled ? "MINIMAP ON [M/F8]" : "MINIMAP OFF [M/F8]";
                drawHudText(mmState,-0.95f,0.84f,0.95f,0.88f,0.93f,0.78f,0.95f);
                if(gPerfDebugOverlay){
                    char graph[40];
                    buildFrameTimeGraph(
                        gPerfFrameTimeHistory,
                        PERF_GRAPH_SAMPLES,
                        gPerfFrameTimeHead,
                        32,
                        graph,
                        40
                    );
                    float avgMs = averageFrameTimeMs(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES);
                    float p95Ms = percentileFrameTimeMs(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES, 0.95f);
                    char dbgA[96];
                    char dbgB[96];
                    snprintf(dbgA,96,"DEBUG PERF [F3] FT %.2fms AVG %.2f P95 %.2f",gPerfFrameMs,avgMs,p95Ms);
                    snprintf(dbgB,96,"GRAPH %s",graph);
                    drawHudText(dbgA,0.12f,-0.74f,1.00f,0.70f,0.85f,0.92f,0.93f);
                    drawHudText(dbgB,0.12f,-0.80f,1.00f,0.72f,0.82f,0.88f,0.93f);
                }
                if(debugTools.flyMode){
                    drawHudText("DEBUG FLY: ON",0.52f,0.95f,1.10f,0.78f,0.95f,0.85f,0.98f);
                }
                if(debugTools.infiniteStamina){
                    drawHudText("DEBUG STAMINA: INF",0.52f,0.90f,1.02f,0.75f,0.92f,0.78f,0.96f);
                }
            }
            if(!immersiveHudActive && multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime())){
                drawHudTextCentered("NETWORK UNSTABLE - RECONNECTING MAY OCCUR",0.0f,0.74f,1.12f,0.95f,0.64f,0.44f,0.95f);
            }
            // === DEBUG MODE: debug tools panel (F10) ===
            if(settings.debugMode && debugTools.open){
                drawFullscreenOverlay(0.02f,0.03f,0.04f,0.62f);
                drawHudTextCentered("DEBUG TOOLS",0.0f,0.56f,1.8f,0.9f,0.95f,0.82f,0.98f);
                for(int i=0;i<DEBUG_ACTION_COUNT;i++){
                    float y = 0.47f - i*0.08f;
                    float s = (debugTools.selectedAction==i)?1.0f:0.65f;
                    const char* lbl = debugActionLabel(i);
                    if(i==DEBUG_ACT_TP_NOTE) lbl = isLevelZero(gCurrentLevel) ? "TP L1 OBJECTIVE" : "TP L2 OBJECTIVE";
                    else if(i==DEBUG_ACT_TP_ECHO) lbl = isLevelZero(gCurrentLevel) ? "TP L1 NPC" : "TP L2 NPC";
                    else if(i==DEBUG_ACT_TP_EXIT) lbl = isLevelZero(gCurrentLevel) ? "TP L1 EXIT" : "TP L2 LIFT";
                    float baseX = -measureTextWidthNdc(lbl, 1.35f) * 0.5f;
                    if(debugTools.selectedAction==i) drawHudText(">",baseX - 0.07f,y,1.4f,0.92f,0.9f,0.65f,0.95f);
                    drawHudText(lbl,baseX,y,1.35f,0.82f*s,0.88f*s,0.72f*s,0.92f);
                }
                drawHudTextCentered("F10 TOGGLE  ENTER APPLY  ESC CLOSE",0.0f,-0.80f,1.12f,0.67f,0.72f,0.78f,0.90f);
            }
        }
    }
}
#endif

#if BR_UI_COMPILE_ALLOW_LEGACY
inline void drawLegacyUI(){
    drawUiParityPath();
}
#endif

#if BR_UI_COMPILE_ALLOW_NEW
inline void drawNewUI(){
    if(drawUiImmersiveMenuLayer(vhsTime)){
        return;
    }
    drawUiParityPath();
}
#endif

inline void drawUI(){
    gUiReducedMotion = settings.reducedMotion;
#if BR_UI_COMPILE_ALLOW_LEGACY && BR_UI_COMPILE_ALLOW_NEW
    if(useNewUiMigrationPath()) drawNewUI();
    else drawLegacyUI();
#elif BR_UI_COMPILE_ALLOW_NEW
    drawNewUI();
#else
    drawLegacyUI();
#endif
}

#include "hud_multiplayer.h"
