#pragma once
#include <cstring>
#include <cmath>
#include <cstdio>
#include "upscaler_settings.h"
#include "keybinds.h"

const unsigned char FONT_DATA[96][7] = {
    {0,0,0,0,0,0,0}, {4,4,4,4,0,4,0}, {10,10,0,0,0,0,0}, {10,31,10,31,10,0,0},
    {4,15,20,14,5,30,4}, {24,25,2,4,8,19,3}, {8,20,20,8,21,18,13}, {4,4,0,0,0,0,0},
    {2,4,8,8,8,4,2}, {8,4,2,2,2,4,8}, {0,4,21,14,21,4,0}, {0,4,4,31,4,4,0},
    {0,0,0,0,4,4,8}, {0,0,0,31,0,0,0}, {0,0,0,0,0,4,0}, {1,2,2,4,8,8,16},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,6,8,16,31}, {14,17,1,6,1,17,14},
    {2,6,10,18,31,2,2}, {31,16,30,1,1,17,14}, {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}, {0,4,0,0,4,0,0}, {0,4,0,0,4,4,8},
    {2,4,8,16,8,4,2}, {0,0,31,0,31,0,0}, {8,4,2,1,2,4,8}, {14,17,2,4,4,0,4},
    {14,17,23,21,23,16,14}, {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31},
    {31,16,16,30,16,16,16}, {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,2,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,17,25,21,19,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {14,17,16,14,1,17,14}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,10,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}, {14,8,8,8,8,8,14}, {16,8,8,4,2,2,1},
    {14,2,2,2,2,2,14}, {4,10,17,0,0,0,0}, {0,0,0,0,0,0,31}, {8,4,0,0,0,0,0},
    {0,0,14,1,15,17,15}, {16,16,30,17,17,17,30}, {0,0,15,16,16,16,15},
    {1,1,15,17,17,17,15}, {0,0,14,17,31,16,14}, {6,8,30,8,8,8,8}, {0,0,15,17,15,1,14},
    {16,16,30,17,17,17,17}, {4,0,12,4,4,4,14}, {2,0,2,2,2,18,12}, {16,16,18,20,24,20,18},
    {12,4,4,4,4,4,14}, {0,0,26,21,21,17,17}, {0,0,30,17,17,17,17}, {0,0,14,17,17,17,14},
    {0,0,30,17,30,16,16}, {0,0,15,17,15,1,1}, {0,0,22,25,16,16,16}, {0,0,15,16,14,1,30},
    {8,8,30,8,8,9,6}, {0,0,17,17,17,17,15}, {0,0,17,17,17,10,4}, {0,0,17,17,21,21,10},
    {0,0,17,10,4,10,17}, {0,0,17,17,15,1,14}, {0,0,31,2,4,8,31}, {2,4,4,8,4,4,2},
    {4,4,4,4,4,4,4}, {8,4,4,2,4,4,8}, {0,0,8,21,2,0,0}, {0,0,0,0,0,0,0}
};

inline GLuint fontTex = 0, textShader = 0, textVAO = 0, textVBO = 0;
inline GLuint overlayShader = 0, overlayVAO = 0, overlayVBO = 0;
inline GLuint menuBgShader = 0;

struct Settings {
    float masterVol = 0.7f, musicVol = 0.55f, ambienceVol = 0.75f;
    float sfxVol = 0.7f, voiceVol = 0.65f;
    float vhsIntensity = 0.65f, mouseSens = 0.002f, fsrSharpness = 0.35f;
    int upscalerMode = UPSCALER_MODE_OFF;
    int renderScalePreset = RENDER_SCALE_PRESET_DEFAULT;
    int aaMode = AA_MODE_FXAA;
    GameplayBinds binds = {};
};

inline Settings settings;

enum GameState {
    STATE_MENU, STATE_GAME, STATE_PAUSE, STATE_SETTINGS, STATE_SETTINGS_PAUSE,
    STATE_KEYBINDS, STATE_KEYBINDS_PAUSE, STATE_INTRO, STATE_NOTE,
    STATE_MULTI, STATE_MULTI_HOST, STATE_MULTI_JOIN, STATE_MULTI_WAIT
};

inline GameState gameState = STATE_MENU;
inline int menuSel = 0, currentWinW = 1280, currentWinH = 720, keybindCaptureIndex = -1;
inline float gSurvivalTime = 0;

inline const char* textVS = R"(#version 330 core
layout(location=0) in vec2 p; layout(location=1) in vec2 t; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = t; })";

inline const char* textFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform sampler2D tex; uniform vec3 col; uniform float alpha;
void main() { float a = texture(tex, uv).r; fc = vec4(col, a * alpha); })";

inline const char* overlayVS = R"(#version 330 core
layout(location=0) in vec2 p; void main() { gl_Position = vec4(p, 0.0, 1.0); })";

inline const char* overlayFS = R"(#version 330 core
out vec4 fc; uniform vec3 col; uniform float alpha; void main() { fc = vec4(col, alpha); })";

inline const char* menuBgVS = R"(#version 330 core
layout(location=0) in vec2 p; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = p * 0.5 + 0.5; })";

inline const char* menuBgFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform float tm;
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
void main() {
    vec2 p = uv * 2.0 - 1.0;
    float tunnel = 1.0 - smoothstep(0.18, 1.25, abs(p.x) * (1.0 - uv.y * 0.35));
    float wave = sin((uv.y * 28.0 - tm * 1.4) + sin(uv.x * 7.0 + tm * 0.5) * 1.2);
    float scan = 0.5 + 0.5 * wave;
    float drift = 0.5 + 0.5 * sin(tm * 0.35 + uv.x * 5.0 + uv.y * 2.0);
    float dust = hash(floor(vec2(uv.x * 180.0 + tm * 5.0, uv.y * 120.0 + tm * 3.0)));
    vec3 base = mix(vec3(0.03, 0.025, 0.02), vec3(0.12, 0.10, 0.06), tunnel * 0.7);
    vec3 lines = vec3(0.18, 0.14, 0.08) * (scan * 0.35 + drift * 0.2);
    vec3 col = base + lines * tunnel;
    col += vec3(0.08, 0.06, 0.03) * smoothstep(0.82, 1.0, uv.y) * 0.25;
    col += (dust - 0.5) * 0.02;
    fc = vec4(col, 1.0);
})";

inline GLuint genFontTex() {
    unsigned char* data = new unsigned char[96*8*8*3];
    memset(data, 0, 96*8*8*3);
    for (int c = 0; c < 96; c++) {
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (FONT_DATA[c][y] & (1 << (4 - x))) {
                    int i = (c*8 + (y+1)*96*8 + x+1)*3;
                    data[i] = data[i+1] = data[i+2] = 255;
                }
            }
        }
    }
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 96*8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    delete[] data;
    return t;
}

inline void initText() {
    fontTex = genFontTex();
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &textVS, 0);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &textFS, 0);
    glCompileShader(fs);
    textShader = glCreateProgram();
    glAttachShader(textShader, vs);
    glAttachShader(textShader, fs);
    glLinkProgram(textShader);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 1024*24, NULL, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    GLuint ovs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(ovs, 1, &overlayVS, 0);
    glCompileShader(ovs);
    GLuint ofs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(ofs, 1, &overlayFS, 0);
    glCompileShader(ofs);
    overlayShader = glCreateProgram();
    glAttachShader(overlayShader, ovs);
    glAttachShader(overlayShader, ofs);
    glLinkProgram(overlayShader);
    glDeleteShader(ovs);
    glDeleteShader(ofs);

    GLuint bvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(bvs, 1, &menuBgVS, 0);
    glCompileShader(bvs);
    GLuint bfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(bfs, 1, &menuBgFS, 0);
    glCompileShader(bfs);
    menuBgShader = glCreateProgram();
    glAttachShader(menuBgShader, bvs);
    glAttachShader(menuBgShader, bfs);
    glLinkProgram(menuBgShader);
    glDeleteShader(bvs);
    glDeleteShader(bfs);

    float quad[12] = {-1,-1, 1,-1, 1,1, -1,-1, 1,1, -1,1};
    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);
    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, (void*)0);
    glEnableVertexAttribArray(0);
}

inline void drawFullscreenOverlay(float r, float g, float b, float a) {
    glUseProgram(overlayShader);
    glUniform3f(glGetUniformLocation(overlayShader, "col"), r, g, b);
    glUniform1f(glGetUniformLocation(overlayShader, "alpha"), a);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

inline void drawMainMenuBackdrop(float tm) {
    glUseProgram(menuBgShader);
    glUniform1f(glGetUniformLocation(menuBgShader, "tm"), tm);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

inline void drawText(const char* s, float x, float y, float sc, float r, float g, float b, float a = 1.0f) {
    float v[512*24];
    int vc = 0;
    float cx = x, cy = y;
    float cw = sc * 8.0f / (float)currentWinW * 2.0f;
    float ch = sc * 8.0f / (float)currentWinH * 2.0f;
    for (const char* p = s; *p && vc < 512*24 - 24; p++) {
        if (*p == '\n') { cx = x; cy -= ch * 1.2f; continue; }
        int c = *p - 32;
        if (c < 0 || c > 95) c = 0;
        float u0 = c * 8.0f / (96.0f * 8.0f);
        float u1 = (c + 1) * 8.0f / (96.0f * 8.0f);
        float t[24] = {cx, cy, u0, 1, cx+cw, cy, u1, 1, cx+cw, cy+ch, u1, 0,
                       cx, cy, u0, 1, cx+cw, cy+ch, u1, 0, cx, cy+ch, u0, 0};
        for (int i = 0; i < 24; i++) v[vc++] = t[i];
        cx += cw * 0.75f;
    }
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, vc*4, v, GL_STATIC_DRAW);
    glUseProgram(textShader);
    glUniform3f(glGetUniformLocation(textShader, "col"), r, g, b);
    glUniform1f(glGetUniformLocation(textShader, "alpha"), a);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glBindVertexArray(textVAO);
    glDrawArrays(GL_TRIANGLES, 0, vc/4);
}

inline float textAdvanceNdc(float sc) { return sc * 8.0f / (float)currentWinW * 2.0f * 0.75f; }

inline float measureTextWidthNdc(const char* s, float sc) {
    if (!s) return 0.0f;
    int cur = 0, mx = 0;
    for (const char* p = s; *p; p++) {
        if (*p == '\n') { if (cur > mx) mx = cur; cur = 0; continue; }
        cur++;
    }
    if (cur > mx) mx = cur;
    return (float)mx * textAdvanceNdc(sc);
}

inline void drawTextCentered(const char* s, float cx, float y, float sc, float r, float g, float b, float a = 1.0f) {
    drawText(s, cx - measureTextWidthNdc(s, sc) * 0.5f, y, sc, r, g, b, a);
}

inline void drawSlider(float x, float y, float w, float val, float r, float g, float b) {
    char bar[21];
    int f = (int)(val * 20);
    for (int i = 0; i < 20; i++) bar[i] = (i < f) ? '=' : '-';
    bar[20] = 0;
    drawText("[", x, y, 1.8f, r*0.5f, g*0.5f, b*0.5f);
    drawText(bar, x + 0.02f, y, 1.8f, r, g, b);
    drawText("]", x + w - 0.02f, y, 1.8f, r*0.5f, g*0.5f, b*0.5f);
}

inline void drawSpinner(float x, float y, float tm, float sc) {
    const char* fr[] = {"|", "/", "-", "\\"};
    int f = ((int)(tm * 8.0f)) % 4;
    float pulse = 0.7f + 0.3f * sinf(tm * 3.0f);
    drawText(fr[f], x, y, sc, 0.9f*pulse, 0.8f*pulse, 0.3f*pulse, 0.9f);
}

#include "menu_draw.h"