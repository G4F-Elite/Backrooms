#pragma once

// Internal split from menu.h to satisfy the 500-lines-per-file rule enforced by build.bat.
// This file is intended to be included *only* from menu.h.
#ifndef BR_MENU_INTERNAL
#error "menu_pages.h must be included from menu.h"
#endif

inline void drawIntro(int line, float timer, float lineTime, const char** introLines) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    float alpha = 1.0f;
    if(timer < 0.5f) alpha = timer / 0.5f;
    else if(timer > lineTime - 0.5f) alpha = (lineTime - timer) / 0.5f;
    if(alpha < 0) alpha = 0;
    if(alpha > 1) alpha = 1;

    if(line >= 0 && line < 12) {
        const char* text = introLines[line];
        if(line == 11) drawTextCentered(text, 0.0f, 0.0f, 3.5f, 0.9f, 0.85f, 0.4f, alpha);
        else if(line != 10) drawTextCentered(text, 0.0f, 0.0f, 2.0f, 0.7f, 0.65f, 0.5f, alpha * 0.9f);
    }

    drawTextCentered("PRESS SPACE TO SKIP", 0.0f, -0.8f, 1.5f, 0.4f, 0.4f, 0.35f, 0.4f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawNote(int noteId, const char* title, const char* content) {
    (void)noteId;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    int lineCount = 1;
    int curLen = 0;
    int maxLen = 0;
    for(const char* p = content; *p; p++) {
        if(*p == '\n') {
            if(curLen > maxLen) maxLen = curLen;
            curLen = 0;
            lineCount++;
        } else {
            curLen++;
        }
    }
    if(curLen > maxLen) maxLen = curLen;
    int titleLen = (int)strlen(title);
    if(titleLen > maxLen) maxLen = titleLen;
    if(maxLen < 16) maxLen = 16;
    if(maxLen > 52) maxLen = 52;
    if(lineCount < 3) lineCount = 3;
    if(lineCount > 12) lineCount = 12;

    float width = 0.16f + (float)maxLen * 0.0085f;
    float height = 0.52f + (float)lineCount * 0.080f;
    if(width > 0.46f) width = 0.46f;
    if(height > 1.58f) height = 1.58f;
    float left = -width;
    float right = width;
    float bottom = -height * 0.52f;
    float top = height * 0.48f;

    drawOverlayRectNdc(left - 0.02f, bottom - 0.03f, right + 0.03f, top + 0.03f, 0.16f, 0.12f, 0.07f, 0.58f);
    drawOverlayRectNdc(left, bottom, right, top, 0.95f, 0.89f, 0.75f, 0.97f);
    drawOverlayRectNdc(left + 0.03f, bottom + 0.03f, right - 0.03f, top - 0.03f, 0.98f, 0.93f, 0.80f, 0.90f);
    float titleY = top - 0.18f;
    drawTextCentered(title, 0.0f, titleY, 2.4f, 0.10f, 0.08f, 0.05f, 1.0f);
    drawTextCentered("________________________________", 0.0f, titleY - 0.10f, 1.5f, 0.16f, 0.12f, 0.08f, 1.0f);

    float ty = titleY - 0.26f;
    char line[64];
    int li = 0;
    for(const char* p = content; *p; p++) {
        if(*p == '\n' || li >= 50) {
            line[li] = 0;
            drawTextCentered(line, 0.0f, ty, 1.62f, 0.08f, 0.07f, 0.05f, 1.0f);
            ty -= 0.084f;
            li = 0;
        } else {
            line[li++] = *p;
        }
    }
    if(li > 0) {
        line[li] = 0;
        drawTextCentered(line, 0.0f, ty, 1.62f, 0.08f, 0.07f, 0.05f, 1.0f);
    }

    drawTextCentered("PRESS E OR ESC TO CLOSE", 0.0f, bottom + 0.08f, 1.55f, 0.17f, 0.13f, 0.08f, 1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
