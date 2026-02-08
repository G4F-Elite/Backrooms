#pragma once

// Health/Sanity/Stamina bars moved from menu.h

inline void drawHealthBar(float hp) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char b[21];
    int f = (int)(hp / 5);
    for (int i = 0; i < 20; i++) b[i] = (i < f) ? '|' : ' ';
    b[20] = 0;
    
    float r = (hp < 30) ? 0.9f : 0.7f;
    float g = (hp > 50) ? 0.7f : 0.3f;
    drawText("HP:", -0.95f, -0.85f, 1.7f, r, g, 0.2f, 0.8f);
    drawText(b, -0.80f, -0.85f, 1.7f, r, g, 0.2f, 0.7f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawSanityBar(float sn) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char b[21];
    int f = (int)(sn / 5);
    for (int i = 0; i < 20; i++) b[i] = (i < f) ? '~' : ' ';
    b[20] = 0;
    
    float p = (sn < 30) ? 0.6f + 0.3f * sinf((float)rand() / 1000.0f) : 1.0f;
    drawText("SN:", -0.95f, -0.92f, 1.7f, 0.4f*p, 0.3f*p, 0.7f*p, 0.8f);
    drawText(b, -0.80f, -0.92f, 1.7f, 0.4f*p, 0.3f*p, 0.7f*p, 0.7f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawStaminaBar(float st) {
    if (st >= 99.0f) return;
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char b[21];
    int f = (int)(st / 5);
    for (int i = 0; i < 20; i++) b[i] = (i < f) ? '>' : ' ';
    b[20] = 0;
    
    float yc = (st < 20) ? 0.5f + 0.3f * sinf((float)rand() / 500.0f) : 0.7f;
    drawText("ST:", -0.95f, -0.78f, 1.7f, 0.3f, 0.6f*yc, 0.3f, 0.8f);
    drawText(b, -0.80f, -0.78f, 1.7f, 0.3f, 0.6f*yc, 0.3f, 0.7f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawFlashlightBattery(float battery, bool isOn) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char b[11];
    int f = (int)(battery / 10);
    for (int i = 0; i < 10; i++) b[i] = (i < f) ? '#' : '.';
    b[10] = 0;
    
    float r = isOn ? 0.9f : 0.4f;
    float g = isOn ? 0.85f : 0.4f;
    drawText("FL:", 0.52f, -0.92f, 1.7f, r, g, 0.3f, 0.8f);
    drawText(b, 0.66f, -0.92f, 1.7f, r, g, 0.3f, 0.7f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawDamageOverlay(float fl, float hp) {
    if (fl < 0.01f && hp > 50) return;
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float lo = (100.0f - hp) / 100.0f * 0.3f;
    float a = fl + lo;
    if (a > 0.8f) a = 0.8f;
    
    drawText("                                                  ", -1.0f, -1.0f, 50.0f, 0.6f, 0.0f, 0.0f, a * 0.4f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawNoteCounter(int count) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char b[16];
    snprintf(b, 16, "NOTES: %d/5", count);
    drawText(b, -0.95f, 0.9f, 1.65f, 0.6f, 0.55f, 0.35f, 0.7f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawPhaseIndicator(int phase) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    const char* pn[] = {"EXPLORATION", "TENSION", "PURSUIT", "ESCAPE"};
    if (phase >= 0 && phase < 4) {
        drawText("PHASE:", 0.49f, 0.85f, 1.25f, 0.45f, 0.4f, 0.28f, 0.55f);
        drawText(pn[phase], 0.60f, 0.82f, 1.35f, 0.5f, 0.45f, 0.3f, 0.58f);
    }
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawInteractPrompt() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawText("[E] READ NOTE", -0.15f, -0.4f, 1.8f, 0.8f, 0.75f, 0.5f, 0.8f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawHallucinationEffect(float intensity) {
    if (intensity < 0.05f) return;
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float a = intensity * 0.3f;
    if (a > 0.4f) a = 0.4f;
    
    drawText("                                        ", -1.0f, -1.0f, 50.0f, 0.3f, 0.0f, 0.4f, a);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}