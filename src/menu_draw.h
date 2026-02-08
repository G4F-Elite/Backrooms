#pragma once

inline void drawMenu(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawMainMenuBackdrop(tm);
    drawFullscreenOverlay(0.015f, 0.013f, 0.012f, 0.72f);
    drawFullscreenOverlay(0.17f, 0.13f, 0.08f, 0.22f + 0.05f*sinf(tm*0.9f));
    drawSpinner(-0.75f, 0.5f, tm, 2.5f);
    drawSpinner(0.75f, 0.5f, tm + 0.5f, 2.5f);
    
    float p = 0.8f + 0.05f*sinf(tm*2.0f);
    float gl = (rand()%100 < 3) ? (rand()%10 - 5)*0.003f : 0;
    drawTextCentered("THE BACKROOMS", 0.0f + gl, 0.5f, 4.0f, 0.9f, 0.85f, 0.4f, p);
    drawTextCentered("LEVEL 0", 0.0f, 0.35f, 2.5f, 0.7f, 0.65f, 0.3f, 0.8f);
    
    const char* it[] = {"START GAME", "MULTIPLAYER", "SETTINGS", "QUIT"};
    for (int i = 0; i < 4; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = 0.08f - i*0.12f;
        float baseX = -measureTextWidthNdc(it[i], 2.0f) * 0.5f;
        if (menuSel == i) drawText(">", baseX - 0.08f, y, 2.0f, 0.9f*s, 0.85f*s, 0.4f*s);
        drawText(it[i], baseX, y, 2.0f, 0.9f*s, 0.85f*s, 0.4f*s);
    }
    drawTextCentered("UP/DOWN - SELECT    ENTER - CONFIRM", 0.0f, -0.6f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawSettings(bool fp) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (fp) drawFullscreenOverlay(0.02f, 0.02f, 0.03f, 0.72f);
    drawTextCentered("SETTINGS", 0.0f, 0.55f, 3.0f, 0.9f, 0.85f, 0.4f);
    
    const char* lb[] = {"MASTER VOL", "MUSIC VOL", "AMBIENCE VOL", "SFX VOL", "VOICE VOL",
                        "VHS EFFECT", "MOUSE SENS", "UPSCALER", "RESOLUTION", 
                        "FSR SHARPNESS", "ANTI-ALIASING", "KEY BINDS", "BACK"};
    float* vl[] = {&settings.masterVol, &settings.musicVol, &settings.ambienceVol, 
                   &settings.sfxVol, &settings.voiceVol, &settings.vhsIntensity, 
                   &settings.mouseSens, nullptr, nullptr, &settings.fsrSharpness, nullptr, nullptr, nullptr};
    float mx[] = {1, 1, 1, 1, 1, 1, 0.006f, 1, 1, 1, 1, 1, 1};
    
    for (int i = 0; i < 13; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = 0.43f - i*0.09f;
        if (menuSel == i) drawText(">", -0.55f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        drawText(lb[i], -0.48f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        
        if (i == 7) {
            drawText(upscalerModeLabel(settings.upscalerMode), 0.36f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        } else if (i == 8) {
            char rb[24];
            if (clampUpscalerMode(settings.upscalerMode) == UPSCALER_MODE_OFF) snprintf(rb, 24, "NATIVE");
            else snprintf(rb, 24, "%d%%", renderScalePercentFromPreset(settings.renderScalePreset));
            drawText(rb, 0.48f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        } else if (i == 10) {
            drawText(aaModeLabel(settings.aaMode), 0.43f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        } else if (i == 11) {
            drawText("OPEN", 0.48f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        } else if (vl[i]) {
            float nv = *vl[i] / mx[i];
            if (nv > 1) nv = 1;
            drawSlider(0.1f, y, 0.45f, nv, 0.9f*s, 0.85f*s, 0.4f*s);
            char b[16];
            snprintf(b, 16, "%d%%", (int)(nv*100));
            drawText(b, 0.58f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        }
    }
    drawTextCentered("LEFT/RIGHT - ADJUST    ENTER - BACK", 0.0f, -0.55f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawPause() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawFullscreenOverlay(0.02f, 0.02f, 0.03f, 0.72f);
    drawTextCentered("PAUSED", 0.0f, 0.25f, 3.0f, 0.9f, 0.85f, 0.4f);
    
    const char* it[] = {"RESUME", "SETTINGS", "MAIN MENU", "QUIT"};
    for (int i = 0; i < 4; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = -i*0.1f;
        float baseX = -measureTextWidthNdc(it[i], 1.8f) * 0.5f;
        if (menuSel == i) drawText(">", baseX - 0.07f, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
        drawText(it[i], baseX, y, 1.8f, 0.9f*s, 0.85f*s, 0.4f*s);
    }
    drawTextCentered("ESC - RESUME", 0.0f, -0.55f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawDeath(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float fl = (rand()%100 < 15) ? 0.3f : 1.0f;
    float p = 0.7f + 0.15f*sinf(tm*3.0f);
    drawTextCentered("YOU DIED", 0.0f, 0.2f, 4.0f, 0.8f*fl, 0.1f*fl, 0.1f*fl, p);
    drawTextCentered("IT GOT YOU...", 0.0f, 0.02f, 2.0f, 0.6f, 0.15f, 0.15f, 0.7f);
    
    int m = (int)(gSurvivalTime / 60);
    int s = (int)gSurvivalTime % 60;
    char tb[32];
    snprintf(tb, 32, "SURVIVED: %d:%02d", m, s);
    drawTextCentered(tb, 0.0f, -0.12f, 2.0f, 0.7f, 0.6f, 0.3f, 0.8f);
    drawTextCentered("PRESS ENTER TO RESTART", 0.0f, -0.35f, 1.8f, 0.5f, 0.4f, 0.35f, 0.6f);
    drawTextCentered("PRESS ESC FOR MAIN MENU", 0.0f, -0.47f, 1.8f, 0.5f, 0.4f, 0.35f, 0.6f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawSurvivalTime(float t) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int m = (int)(t / 60);
    int s = (int)t % 60;
    char b[16];
    snprintf(b, 16, "%d:%02d", m, s);
    drawText(b, 0.72f, 0.9f, 2.0f, 0.78f, 0.72f, 0.48f, 0.96f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawKeybindsMenu(bool fromPause, int selected, int captureIndex) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (fromPause) drawFullscreenOverlay(0.02f, 0.02f, 0.03f, 0.72f);
    drawTextCentered("KEY BINDS", 0.0f, 0.62f, 2.6f, 0.9f, 0.85f, 0.4f);
    
    for (int i = 0; i < GAMEPLAY_BIND_COUNT; i++) {
        float s = (selected == i) ? 1.0f : 0.55f;
        float y = 0.48f - i*0.075f;
        if (selected == i) drawText(">", -0.62f, y, 1.45f, 0.92f*s, 0.86f*s, 0.42f*s);
        drawText(gameplayBindLabel(i), -0.56f, y, 1.38f, 0.9f*s, 0.85f*s, 0.4f*s);
        const char* keyName = keyNameForUi(*gameplayBindByIndex(settings.binds, i));
        if (captureIndex == i) keyName = "...";
        drawText(keyName, 0.38f, y, 1.38f, 0.82f*s, 0.9f*s, 0.72f*s, 0.95f);
    }
    
    float bs = (selected == KEYBINDS_BACK_INDEX) ? 1.0f : 0.55f;
    float by = 0.48f - GAMEPLAY_BIND_COUNT*0.075f;
    if (selected == KEYBINDS_BACK_INDEX) drawText(">", -0.62f, by, 1.45f, 0.92f*bs, 0.86f*bs, 0.42f*bs);
    drawText("BACK", -0.56f, by, 1.45f, 0.9f*bs, 0.85f*bs, 0.4f*bs);
    
    const char* hint = captureIndex >= 0 ? "PRESS ANY KEY TO REBIND" : "ENTER TO REBIND  ESC TO BACK";
    drawTextCentered(hint, 0.0f, -0.84f, 1.35f, 0.58f, 0.58f, 0.46f, 0.86f);
    if (fromPause) drawTextCentered("APPLIES IN CURRENT RUN", 0.0f, -0.92f, 1.1f, 0.5f, 0.55f, 0.45f, 0.7f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawIntro(int line, float timer, float lineTime, const char** introLines) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float alpha = 1.0f;
    if (timer < 0.5f) alpha = timer / 0.5f;
    else if (timer > lineTime - 0.5f) alpha = (lineTime - timer) / 0.5f;
    if (alpha < 0) alpha = 0;
    if (alpha > 1) alpha = 1;
    
    if (line >= 0 && line < 12) {
        const char* text = introLines[line];
        if (line == 11) drawTextCentered(text, 0.0f, 0.0f, 3.5f, 0.9f, 0.85f, 0.4f, alpha);
        else if (line != 10) drawTextCentered(text, 0.0f, 0.0f, 2.0f, 0.7f, 0.65f, 0.5f, alpha * 0.9f);
    }
    drawTextCentered("PRESS SPACE TO SKIP", 0.0f, -0.8f, 1.5f, 0.4f, 0.4f, 0.35f, 0.4f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawNote(int noteId, const char* title, const char* content) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawFullscreenOverlay(0.0f, 0.0f, 0.0f, 0.94f);
    drawFullscreenOverlay(0.10f, 0.08f, 0.05f, 0.38f);

    const char* panelShadow = "                                                  ";
    const char* panelBody = "                                              ";
    for (float yp = 0.76f; yp >= -0.74f; yp -= 0.09f) {
        drawTextCentered(panelShadow, 0.0f, yp - 0.012f, 3.05f, 0.0f, 0.0f, 0.0f, 0.82f);
        drawTextCentered(panelBody, 0.0f, yp, 3.05f, 0.91f, 0.86f, 0.72f, 1.0f);
    }

    drawTextCentered(title, 0.0f, 0.56f, 2.6f, 0.10f, 0.08f, 0.05f, 1.0f);
    drawTextCentered("________________________________", 0.0f, 0.46f, 1.5f, 0.16f, 0.12f, 0.08f, 1.0f);

    float ty = 0.30f;
    char line[64];
    int li = 0;
    for (const char* pc = content; *pc; pc++) {
        if (*pc == '\n' || li >= 50) {
            line[li] = 0;
            drawTextCentered(line, 0.0f, ty, 1.62f, 0.08f, 0.07f, 0.05f, 1.0f);
            ty -= 0.084f;
            li = 0;
        } else {
            line[li++] = *pc;
        }
    }
    if (li > 0) {
        line[li] = 0;
        drawTextCentered(line, 0.0f, ty, 1.62f, 0.08f, 0.07f, 0.05f, 1.0f);
    }
    drawTextCentered("PRESS E OR ESC TO CLOSE", 0.0f, -0.75f, 1.55f, 0.17f, 0.13f, 0.08f, 1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}