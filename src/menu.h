#pragma once
#include <cstring>
#include <cmath>
#include <cstdio>
#include "settings_model.h"
#include "progression.h"

const unsigned char FONT_DATA[96][7] = {
    {0,0,0,0,0,0,0},{4,4,4,4,0,4,0},{10,10,0,0,0,0,0},{10,31,10,31,10,0,0},{4,15,20,14,5,30,4},{24,25,2,4,8,19,3},{8,20,20,8,21,18,13},{4,4,0,0,0,0,0},
    {2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{0,4,21,14,21,4,0},{0,4,4,31,4,4,0},{0,0,0,0,4,4,8},{0,0,0,31,0,0,0},{0,0,0,0,0,4,0},{1,2,2,4,8,8,16},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,6,8,16,31},{14,17,1,6,1,17,14},{2,6,10,18,31,2,2},{31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},{14,17,17,15,1,2,12},{0,4,0,0,4,0,0},{0,4,0,0,4,4,8},{2,4,8,16,8,4,2},{0,0,31,0,31,0,0},{8,4,2,1,2,4,8},{14,17,2,4,4,0,4},
    {14,17,23,21,23,16,14},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},
    {17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,17,25,21,19,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{14,17,16,14,1,17,14},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,10,10,4},{17,17,17,21,21,21,10},
    {17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,8,8,8,8,8,14},{16,8,8,4,2,2,1},{14,2,2,2,2,2,14},{4,10,17,0,0,0,0},{0,0,0,0,0,0,31},
    {8,4,0,0,0,0,0},{0,0,14,1,15,17,15},{16,16,30,17,17,17,30},{0,0,15,16,16,16,15},{1,1,15,17,17,17,15},{0,0,14,17,31,16,14},{6,8,30,8,8,8,8},{0,0,15,17,15,1,14},
    {16,16,30,17,17,17,17},{4,0,12,4,4,4,14},{2,0,2,2,2,18,12},{16,16,18,20,24,20,18},{12,4,4,4,4,4,14},{0,0,26,21,21,17,17},{0,0,30,17,17,17,17},{0,0,14,17,17,17,14},
    {0,0,30,17,30,16,16},{0,0,15,17,15,1,1},{0,0,22,25,16,16,16},{0,0,15,16,14,1,30},{8,8,30,8,8,9,6},{0,0,17,17,17,17,15},{0,0,17,17,17,10,4},{0,0,17,17,21,21,10},
    {0,0,17,10,4,10,17},{0,0,17,17,15,1,14},{0,0,31,2,4,8,31},{2,4,4,8,4,4,2},{4,4,4,4,4,4,4},{8,4,4,2,4,4,8},{0,0,8,21,2,0,0},{0,0,0,0,0,0,0}
};

inline GLuint fontTex=0, textShader=0, textVAO=0, textVBO=0;
inline GLuint overlayShader=0, overlayVAO=0, overlayVBO=0;
inline GLuint menuBgShader=0, menuFxShader=0;
// Textures are created in game_main_entry.h (declared in game.cpp)
extern GLuint wallTex, floorTex, ceilTex, lampTex;
extern char gDeathReason[80];
inline Settings settings;
inline int settingsTab = SETTINGS_TAB_VIDEO;

enum GameState { STATE_MENU, STATE_GUIDE, STATE_GAME, STATE_PAUSE, STATE_SETTINGS, STATE_SETTINGS_PAUSE, STATE_INTRO, STATE_NOTE, STATE_MULTI, STATE_MULTI_HOST, STATE_MULTI_JOIN, STATE_MULTI_WAIT };
inline GameState gameState = STATE_MENU;
inline int menuSel=0, currentWinW=1280, currentWinH=720;
inline bool guideReturnToPause = false;
inline int keybindCaptureIndex = -1;
inline float gSurvivalTime = 0;

inline const char* textVS = R"(#version 330 core
layout(location=0) in vec2 p; layout(location=1) in vec2 t; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = t; })";
inline const char* textFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform sampler2D tex; uniform vec3 col; uniform float alpha;
void main() { float a = texture(tex, uv).r; fc = vec4(col, a * alpha); })";
inline const char* overlayVS = R"(#version 330 core
layout(location=0) in vec2 p;
void main() { gl_Position = vec4(p, 0.0, 1.0); })";
inline const char* overlayFS = R"(#version 330 core
out vec4 fc;
uniform vec3 col;
uniform float alpha;
void main() { fc = vec4(col, alpha); })";
inline const char* menuFxVS = R"(#version 330 core
layout(location=0) in vec2 p; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = p * 0.5 + 0.5; })";
inline const char* menuFxFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform float tm;
float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p){
    vec2 i=floor(p), f=fract(p);
    float a=hash(i), b=hash(i+vec2(1,0)), c=hash(i+vec2(0,1)), d=hash(i+vec2(1,1));
    vec2 u=f*f*(3.0-2.0*f);
    return mix(mix(a,b,u.x),mix(c,d,u.x),u.y);
}
float fbm(vec2 p){
    float v = 0.0;
    float a = 0.5;
    for(int i=0;i<4;i++){
        v += noise(p) * a;
        p = p * 2.07 + vec2(19.3, 7.1);
        a *= 0.5;
    }
    return v;
}
void main(){
    vec2 p = uv * 2.0 - 1.0;
    float mistLo = fbm(uv * vec2(8.0, 22.0) + vec2(tm * 0.045, -tm * 0.020));
    float mistHi = fbm(uv * vec2(22.0, 72.0) + vec2(-tm * 0.110, tm * 0.065));
    float mist = mistLo * 0.62 + mistHi * 0.38;

    float sweepA = sin(tm * 0.19 + p.y * 1.9) * 0.12;
    float sweepB = cos(tm * 0.16 + p.y * 1.6) * 0.09;
    float beamA = exp(-pow(abs(p.x + sweepA), 2.0) * 64.0);
    float beamB = exp(-pow(abs(p.x - sweepB), 2.0) * 82.0);
    float beamThin = 0.55 + 0.45 * sin(tm * 0.62 + p.y * 8.0);
    float beams = (beamA * 0.62 + beamB * 0.48) * beamThin;

    float scan = 0.5 + 0.5 * sin(uv.y * 920.0 + tm * 3.3);
    scan = pow(scan, 8.0) * 0.06;
    float grain = (hash(gl_FragCoord.xy + vec2(tm * 61.0, tm * 37.0)) - 0.5) * 0.045;

    float vign = 1.0 - smoothstep(0.52, 1.18, length(p));
    vec3 col = vec3(0.10, 0.085, 0.060) * (0.24 + mist * 0.44)
             + vec3(0.95, 0.84, 0.56) * beams * 0.13
             + vec3(0.50, 0.46, 0.36) * scan;
    col += vec3(grain);
    float alpha = (0.012 + mist * 0.038 + beams * 0.040 + scan) * (0.65 + vign * 0.35);
    fc = vec4(col, alpha);
})";
inline const char* menuBgVS = R"(#version 330 core
layout(location=0) in vec2 p; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = p * 0.5 + 0.5; })";
inline const char* menuBgFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform float tm; uniform sampler2D wallT; uniform sampler2D floorT; uniform sampler2D ceilT; uniform sampler2D lampT; float h(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);} void main(){vec2 p=uv*2.0-1.0; p.x*=1.30; float time=tm*0.11; vec3 ro=vec3(0.0,0.1,-1.7); vec3 rd=normalize(vec3(p,1.35)); float yaw=sin(time*0.30)*0.04; rd.xz=mat2(cos(yaw),-sin(yaw),sin(yaw),cos(yaw))*rd.xz; float hw=2.6, hh=1.35; float tMin=1e9; int hit=0; vec3 hp=vec3(0); vec3 n=vec3(0); float tF=(-hh-ro.y)/rd.y; if(tF>0.0){vec3 q=ro+rd*tF; if(abs(q.x)<hw){tMin=tF;hit=1;hp=q;n=vec3(0,1,0);}} float tC=(hh-ro.y)/rd.y; if(tC>0.0){vec3 q=ro+rd*tC; if(abs(q.x)<hw && tC<tMin){tMin=tC;hit=2;hp=q;n=vec3(0,-1,0);}} float tL=(-hw-ro.x)/rd.x; if(tL>0.0){vec3 q=ro+rd*tL; if(abs(q.y)<hh && tL<tMin){tMin=tL;hit=3;hp=q;n=vec3(1,0,0);}} float tR=(hw-ro.x)/rd.x; if(tR>0.0){vec3 q=ro+rd*tR; if(abs(q.y)<hh && tR<tMin){tMin=tR;hit=4;hp=q;n=vec3(-1,0,0);}} vec3 col=vec3(0.22,0.20,0.14); if(hit!=0){float z=hp.z+time*3.2+0.9; vec2 q=(hit==1)?vec2(hp.x*0.35,z*0.22):(hit==2?vec2(hp.x*0.28,z*0.20):vec2(z*0.18,(hp.y+hh)*0.42)); vec2 pq=q; if(hit>=3){pq=vec2(z*0.18,(hp.y+hh)*0.42); float seam=floor(pq.x*3.2); pq.x+=seam*0.02; float ht=texture(wallT,pq).a-0.5; pq+=ht*0.03*vec2(rd.z,rd.y); } else if(hit==1){float ht=texture(floorT,pq).a-0.5; pq+=ht*0.02*vec2(rd.x,rd.z);} else if(hit==2){float ht=texture(ceilT,pq).a-0.5; pq+=ht*0.015*vec2(rd.x,rd.z);} vec3 al=(hit==1)?texture(floorT,pq).rgb:(hit==2?texture(ceilT,pq).rgb:texture(wallT,pq).rgb); if(hit>=3){al=pow(al,vec3(0.95)); float seamLine=smoothstep(0.495,0.505,fract(pq.x*3.2)); al*=1.0-0.10*seamLine; } float segF=(z+1.0)*0.55; float dz=abs(fract(segF)-0.5)/0.55; float lampX=sin(segF*2.0)*0.60; float dx=hp.x-lampX; float dy=hp.y-(hh-0.06); float strip=exp(-dz*dz*10.0); float l=(1.9*strip)/(1.0+dx*dx*0.9+dy*dy*1.8); float fall=smoothstep(0.0,0.8,hh-hp.y); l*=fall; float ambient=0.40; col=al*(ambient+l); col+=texture(lampT,vec2(hp.x*0.35,z*0.25)).rgb*l*0.34; col*=clamp(0.72+0.28*abs(n.y),0.0,1.0);} float fog=exp(-tMin*0.12); col=mix(vec3(0.10,0.10,0.10),col,fog); float vig=smoothstep(1.25,0.25,length(p)); col*=0.78+0.22*vig; col*=0.99+0.01*sin(uv.y*900.0); float drift=0.96+0.04*sin(tm*0.12+uv.y*3.0); col*=drift; vec2 duv=uv+vec2(tm*0.006,tm*0.012); float d=h(floor(duv*vec2(160.0,90.0)))-0.5; col+=d*0.03; col=col/(col+vec3(0.85)); col=clamp(col,0.0,1.0); fc=vec4(col,1.0);} )";

// UI scale: layouts are authored for a 1280x720 canvas and grow with the window.
inline float uiScale() {
    float sx = (float)currentWinW / 1280.0f, sy = (float)currentWinH / 720.0f;
    float s = sx < sy ? sx : sy;
    return s < 0.5f ? 0.5f : s;
}

// Each font texel is stored as a FONT_UPSCALE x FONT_UPSCALE block and sampled with linear
// filtering: glyph edges stay hard at any text size instead of dropping or doubling pixels.
inline constexpr int FONT_UPSCALE = 4;

inline GLuint genFontTex() {
    const int cell = 8 * FONT_UPSCALE, texW = 96 * cell, texH = cell;
    unsigned char* data = new unsigned char[texW * texH * 3];
    memset(data, 0, texW * texH * 3);
    for (int c = 0; c < 96; c++) {
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!(FONT_DATA[c][y] & (1 << (4 - x)))) continue;
                for (int sy = 0; sy < FONT_UPSCALE; sy++) {
                    for (int sx = 0; sx < FONT_UPSCALE; sx++) {
                        int px = c * cell + (x + 1) * FONT_UPSCALE + sx;
                        int py = (y + 1) * FONT_UPSCALE + sy;
                        int i = (py * texW + px) * 3;
                        data[i] = data[i + 1] = data[i + 2] = 255;
                    }
                }
            }
        }
    }
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    delete[] data;
    return t;
}

inline void initText() {
    fontTex=genFontTex(); GLuint vs=glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs,1,&textVS,0); glCompileShader(vs);
    GLuint fs=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs,1,&textFS,0); glCompileShader(fs);
    textShader=glCreateProgram(); glAttachShader(textShader,vs); glAttachShader(textShader,fs); glLinkProgram(textShader);
    glDeleteShader(vs); glDeleteShader(fs); glGenVertexArrays(1,&textVAO); glGenBuffers(1,&textVBO);
    glBindVertexArray(textVAO); glBindBuffer(GL_ARRAY_BUFFER,textVBO); glBufferData(GL_ARRAY_BUFFER,1024*24,NULL,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,16,(void*)0); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,16,(void*)8);
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);

    GLuint ovs=glCreateShader(GL_VERTEX_SHADER); glShaderSource(ovs,1,&overlayVS,0); glCompileShader(ovs);
    GLuint ofs=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(ofs,1,&overlayFS,0); glCompileShader(ofs);
    overlayShader=glCreateProgram(); glAttachShader(overlayShader,ovs); glAttachShader(overlayShader,ofs); glLinkProgram(overlayShader);
    glDeleteShader(ovs); glDeleteShader(ofs);

    GLuint bvs=glCreateShader(GL_VERTEX_SHADER); glShaderSource(bvs,1,&menuBgVS,0); glCompileShader(bvs);
    GLuint bfs=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(bfs,1,&menuBgFS,0); glCompileShader(bfs);
    menuBgShader=glCreateProgram(); glAttachShader(menuBgShader,bvs); glAttachShader(menuBgShader,bfs); glLinkProgram(menuBgShader);
    glDeleteShader(bvs); glDeleteShader(bfs);

    GLuint mvs=glCreateShader(GL_VERTEX_SHADER); glShaderSource(mvs,1,&menuFxVS,0); glCompileShader(mvs);
    GLuint mfs=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(mfs,1,&menuFxFS,0); glCompileShader(mfs);
    menuFxShader=glCreateProgram(); glAttachShader(menuFxShader,mvs); glAttachShader(menuFxShader,mfs); glLinkProgram(menuFxShader);
    glDeleteShader(mvs); glDeleteShader(mfs);

    // Bind menu background samplers to fixed texture units
    glUseProgram(menuBgShader);
    glUniform1i(glGetUniformLocation(menuBgShader,"wallT"),0);
    glUniform1i(glGetUniformLocation(menuBgShader,"floorT"),1);
    glUniform1i(glGetUniformLocation(menuBgShader,"ceilT"),2);
    glUniform1i(glGetUniformLocation(menuBgShader,"lampT"),3);

    float quad[12] = {-1.0f,-1.0f,  1.0f,-1.0f,  1.0f,1.0f,  -1.0f,-1.0f,  1.0f,1.0f,  -1.0f,1.0f};
    glGenVertexArrays(1,&overlayVAO); glGenBuffers(1,&overlayVBO);
    glBindVertexArray(overlayVAO); glBindBuffer(GL_ARRAY_BUFFER,overlayVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,8,(void*)0); glEnableVertexAttribArray(0);
}

inline void drawFullscreenOverlay(float r, float g, float b, float a) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(overlayShader);
    glUniform3f(glGetUniformLocation(overlayShader,"col"),r,g,b);
    glUniform1f(glGetUniformLocation(overlayShader,"alpha"),a);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    glDisable(GL_BLEND);
}

inline void drawOverlayRectNdc(float left, float bottom, float right, float top, float r, float g, float b, float a) {
    if (right <= left || top <= bottom) return;
    const float rectQuad[12] = {
        left,bottom, right,bottom, right,top,
        left,bottom, right,top, left,top
    };
    const float fullQuad[12] = {
        -1.0f,-1.0f, 1.0f,-1.0f, 1.0f,1.0f,
        -1.0f,-1.0f, 1.0f,1.0f, -1.0f,1.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER,overlayVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(rectQuad),rectQuad,GL_STATIC_DRAW);
    drawFullscreenOverlay(r, g, b, a);
    glBindBuffer(GL_ARRAY_BUFFER,overlayVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(fullQuad),fullQuad,GL_STATIC_DRAW);
}

inline void drawMainMenuBackdrop(float tm) {
    glUseProgram(menuBgShader);
    glUniform1f(glGetUniformLocation(menuBgShader,"tm"),tm);

    glActiveTexture(GL_TEXTURE0+0); glBindTexture(GL_TEXTURE_2D, wallTex);
    glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, floorTex);
    glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, ceilTex);
    glActiveTexture(GL_TEXTURE0+3); glBindTexture(GL_TEXTURE_2D, lampTex);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
}

// Glyph cell and advance in whole pixels, so every letter lands on the pixel grid.
inline float textCellPx(float sc) {
    float px = floorf(sc * 8.0f * uiScale() + 0.5f);
    return px < 4.0f ? 4.0f : px;
}

inline float textAdvancePx(float sc) {
    return floorf(textCellPx(sc) * 0.75f + 0.5f);
}

// `sc` 1.0 is an 8 px cell on the 1280x720 canvas; (x, y) is the bottom-left corner in NDC.
inline void drawText(const char* s, float x, float y, float sc, float r, float g, float b, float a=1.0f) {
    if (!s || !*s) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const float pxW = 2.0f / (float)currentWinW, pxH = 2.0f / (float)currentWinH;
    x = floorf((x + 1.0f) / pxW + 0.5f) * pxW - 1.0f;
    y = floorf((y + 1.0f) / pxH + 0.5f) * pxH - 1.0f;
    float v[512*24];
    int vc = 0;
    float cx = x, cy = y;
    float cw = textCellPx(sc) * pxW, ch = textCellPx(sc) * pxH, adv = textAdvancePx(sc) * pxW;
    for (const char* p = s; *p && vc < 512*24-24; p++) {
        if (*p == '\n') { cx = x; cy -= floorf(textCellPx(sc) * 1.2f + 0.5f) * pxH; continue; }
        int c = *p - 32;
        if (c < 0 || c > 95) c = 0;
        float u0 = (float)c / 96.0f, u1 = (float)(c + 1) / 96.0f;
        float t[24] = {cx,cy,u0,1, cx+cw,cy,u1,1, cx+cw,cy+ch,u1,0, cx,cy,u0,1, cx+cw,cy+ch,u1,0, cx,cy+ch,u0,0};
        if (c != 0) for (int i = 0; i < 24; i++) v[vc++] = t[i];
        cx += adv;
    }
    if (vc == 0) return;
    static GLint colLoc = -1, alphaLoc = -1;
    glUseProgram(textShader);
    if (colLoc < 0) {
        colLoc = glGetUniformLocation(textShader, "col");
        alphaLoc = glGetUniformLocation(textShader, "alpha");
    }
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, vc * sizeof(float), v, GL_STATIC_DRAW);
    glUniform3f(colLoc, r, g, b);
    glUniform1f(alphaLoc, a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glBindVertexArray(textVAO);
    glDrawArrays(GL_TRIANGLES, 0, vc / 4);
    glEnable(GL_CULL_FACE);
}

inline float textAdvanceNdc(float sc) {
    return textAdvancePx(sc) * 2.0f / (float)currentWinW;
}

inline float measureTextWidthNdc(const char* s, float sc) {
    if(!s) return 0.0f;
    int cur = 0, mx = 0;
    for(const char* p = s; *p; p++) {
        if(*p == '\n') { if(cur > mx) mx = cur; cur = 0; continue; }
        cur++;
    }
    if(cur > mx) mx = cur;
    return (float)mx * textAdvanceNdc(sc);
}

inline void drawTextCentered(const char* s, float centerX, float y, float sc, float r, float g, float b, float a=1.0f) {
    drawText(s, centerX - measureTextWidthNdc(s, sc) * 0.5f, y, sc, r, g, b, a);
}

// Corridor fly-through plus haze: the backdrop of every screen outside a run.
inline void drawMenuAtmosphere(float tm) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(menuFxShader);
    glUniform1f(glGetUniformLocation(menuFxShader,"tm"),tm);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    glDisable(GL_BLEND);
}
