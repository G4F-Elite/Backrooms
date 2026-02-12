#pragma once
#include <cstring>
#include <cmath>
#include <cstdio>
#include "upscaler_settings.h"
#include "keybinds.h"
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
struct Settings {
    float masterVol=0.7f;
    float musicVol=0.55f;
    float ambienceVol=0.75f;
    float sfxVol=0.7f;
    float voiceVol=0.65f;
    float vhsIntensity=0.58f;
    float mouseSens=0.002f;
    int upscalerMode=UPSCALER_MODE_FSR10;
    int renderScalePreset=RENDER_SCALE_PRESET_DEFAULT;
    float fsrSharpness=0.35f;
    int aaMode=AA_MODE_TAA;
    bool fastMath=false;
    int ssaoQuality=0; int giQuality=0; bool godRays=false; bool bloom=false;
    bool rtxDenoise=true;
    float rtxDenoiseStrength=0.65f;
    int frameGenMode=FRAME_GEN_MODE_OFF;
    bool vsync=true;
    bool debugMode=false;
    GameplayBinds binds = {};
};
inline Settings settings;
enum SettingsTab {
    SETTINGS_TAB_VIDEO = 0,
    SETTINGS_TAB_EFFECTS = 1,
    SETTINGS_TAB_AUDIO = 2,
    SETTINGS_TAB_BINDS = 3
};
inline int settingsTab = SETTINGS_TAB_VIDEO;

inline int settingsItemsForTab(int tab) { if(tab==SETTINGS_TAB_AUDIO) return 7; if(tab==SETTINGS_TAB_EFFECTS) return 8; if(tab==SETTINGS_TAB_BINDS) return 3; return 12; }
inline int settingsBindsIndexForTab(int tab) { return (tab == SETTINGS_TAB_BINDS) ? 1 : -1; }
inline int settingsBackIndexForTab(int tab) { if(tab==SETTINGS_TAB_AUDIO) return 6; if(tab==SETTINGS_TAB_EFFECTS) return 7; if(tab==SETTINGS_TAB_BINDS) return 2; return 11; }

inline int clampSettingsSelection(int tab, int idx) {
    int cnt = settingsItemsForTab(tab);
    if (cnt <= 0) return 0;
    if (idx < 0) return cnt - 1;
    if (idx >= cnt) return 0;
    return idx;
}

enum GameState { STATE_MENU, STATE_GUIDE, STATE_GAME, STATE_PAUSE, STATE_SETTINGS, STATE_SETTINGS_PAUSE, STATE_KEYBINDS, STATE_KEYBINDS_PAUSE, STATE_INTRO, STATE_NOTE, STATE_MULTI, STATE_MULTI_HOST, STATE_MULTI_JOIN, STATE_MULTI_WAIT };
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
void main(){
    vec2 p = uv * 2.0 - 1.0;
    float n0 = noise(uv * vec2(5.0, 22.0) + vec2(tm * 0.08, tm * 0.03));
    float n1 = noise(uv * vec2(11.0, 44.0) + vec2(-tm * 0.12, tm * 0.05));
    float fog = n0 * 0.65 + n1 * 0.35;
    float shaftA = exp(-pow(abs(p.x + sin(tm * 0.25 + p.y * 2.5) * 0.35), 2.0) * 14.0);
    float shaftB = exp(-pow(abs(p.x - cos(tm * 0.21 + p.y * 2.0) * 0.30), 2.0) * 18.0);
    float shaftPulse = 0.6 + 0.4 * sin(tm * 0.6 + p.y * 4.0);
    float shaft = (shaftA * 0.65 + shaftB * 0.45) * shaftPulse;
    float vign = 1.0 - smoothstep(0.55, 1.20, length(p));
    vec3 col = vec3(0.16, 0.14, 0.10) * fog + vec3(0.48, 0.42, 0.30) * shaft * 0.28;
    float alpha = (0.035 + fog * 0.050 + shaft * 0.08) * (0.55 + vign * 0.45);
    fc = vec4(col, alpha);
})";
inline const char* menuBgVS = R"(#version 330 core
layout(location=0) in vec2 p; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = p * 0.5 + 0.5; })";
inline const char* menuBgFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform float tm; uniform sampler2D wallT; uniform sampler2D floorT; uniform sampler2D ceilT; uniform sampler2D lampT; float h(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);} void main(){vec2 p=uv*2.0-1.0; p.x*=1.30; float time=tm*0.11; vec3 ro=vec3(0.0,0.1,-1.7); vec3 rd=normalize(vec3(p,1.35)); float yaw=sin(time*0.30)*0.04; rd.xz=mat2(cos(yaw),-sin(yaw),sin(yaw),cos(yaw))*rd.xz; float hw=2.6, hh=1.35; float tMin=1e9; int hit=0; vec3 hp=vec3(0); vec3 n=vec3(0); float tF=(-hh-ro.y)/rd.y; if(tF>0.0){vec3 q=ro+rd*tF; if(abs(q.x)<hw){tMin=tF;hit=1;hp=q;n=vec3(0,1,0);}} float tC=(hh-ro.y)/rd.y; if(tC>0.0){vec3 q=ro+rd*tC; if(abs(q.x)<hw && tC<tMin){tMin=tC;hit=2;hp=q;n=vec3(0,-1,0);}} float tL=(-hw-ro.x)/rd.x; if(tL>0.0){vec3 q=ro+rd*tL; if(abs(q.y)<hh && tL<tMin){tMin=tL;hit=3;hp=q;n=vec3(1,0,0);}} float tR=(hw-ro.x)/rd.x; if(tR>0.0){vec3 q=ro+rd*tR; if(abs(q.y)<hh && tR<tMin){tMin=tR;hit=4;hp=q;n=vec3(-1,0,0);}} vec3 col=vec3(0.22,0.20,0.14); if(hit!=0){float z=hp.z+time*3.2+0.9; vec2 q=(hit==1)?vec2(hp.x*0.35,z*0.22):(hit==2?vec2(hp.x*0.28,z*0.20):vec2(z*0.18,(hp.y+hh)*0.42)); vec2 pq=q; if(hit>=3){pq=vec2(z*0.18,(hp.y+hh)*0.42); float seam=floor(pq.x*3.2); pq.x+=seam*0.02; float ht=texture(wallT,pq).a-0.5; pq+=ht*0.03*vec2(rd.z,rd.y); } else if(hit==1){float ht=texture(floorT,pq).a-0.5; pq+=ht*0.02*vec2(rd.x,rd.z);} else if(hit==2){float ht=texture(ceilT,pq).a-0.5; pq+=ht*0.015*vec2(rd.x,rd.z);} vec3 al=(hit==1)?texture(floorT,pq).rgb:(hit==2?texture(ceilT,pq).rgb:texture(wallT,pq).rgb); if(hit>=3){al=pow(al,vec3(0.95)); float seamLine=smoothstep(0.495,0.505,fract(pq.x*3.2)); al*=1.0-0.10*seamLine; } float segF=(z+1.0)*0.55; float dz=abs(fract(segF)-0.5)/0.55; float lampX=sin(segF*2.0)*0.60; float dx=hp.x-lampX; float dy=hp.y-(hh-0.06); float strip=exp(-dz*dz*10.0); float l=(1.9*strip)/(1.0+dx*dx*0.9+dy*dy*1.8); float fall=smoothstep(0.0,0.8,hh-hp.y); l*=fall; float ambient=0.40; col=al*(ambient+l); col+=texture(lampT,vec2(hp.x*0.35,z*0.25)).rgb*l*0.34; col*=clamp(0.72+0.28*abs(n.y),0.0,1.0);} float fog=exp(-tMin*0.12); col=mix(vec3(0.10,0.10,0.10),col,fog); float vig=smoothstep(1.25,0.25,length(p)); col*=0.78+0.22*vig; col*=0.99+0.01*sin(uv.y*900.0); float drift=0.96+0.04*sin(tm*0.12+uv.y*3.0); col*=drift; vec2 duv=uv+vec2(tm*0.006,tm*0.012); float d=h(floor(duv*vec2(160.0,90.0)))-0.5; col+=d*0.03; col=col/(col+vec3(0.85)); col=clamp(col,0.0,1.0); fc=vec4(col,1.0);} )";

inline GLuint genFontTex() {
    unsigned char* data = new unsigned char[96*8*8*3]; memset(data,0,96*8*8*3);
    for(int c=0;c<96;c++) for(int y=0;y<7;y++) for(int x=0;x<5;x++)
        if(FONT_DATA[c][y]&(1<<(4-x))) { int i=(c*8+(y+1)*96*8+x+1)*3; data[i]=data[i+1]=data[i+2]=255; }
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,96*8,8,0,GL_RGB,GL_UNSIGNED_BYTE,data);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); delete[] data; return t;
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
    glUseProgram(overlayShader);
    glUniform3f(glGetUniformLocation(overlayShader,"col"),r,g,b);
    glUniform1f(glGetUniformLocation(overlayShader,"alpha"),a);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
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

inline void drawText(const char* s, float x, float y, float sc, float r, float g, float b, float a=1.0f) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float v[512*24]; int vc=0; float cx=x,cy=y,cw=sc*8.0f/(float)currentWinW*2.0f,ch=sc*8.0f/(float)currentWinH*2.0f;
    for(const char*p=s;*p && vc<512*24-24;p++) { if(*p=='\n'){cx=x;cy-=ch*1.2f;continue;} int c=*p-32; if(c<0||c>95)c=0;
        float u0=c*8.0f/(96.0f*8.0f),u1=(c+1)*8.0f/(96.0f*8.0f);
        float t[24]={cx,cy,u0,1,cx+cw,cy,u1,1,cx+cw,cy+ch,u1,0,cx,cy,u0,1,cx+cw,cy+ch,u1,0,cx,cy+ch,u0,0};
        for(int i=0;i<24;i++) v[vc++]=t[i];
        cx+=cw*0.75f;
    }
    glBindBuffer(GL_ARRAY_BUFFER,textVBO); glBufferData(GL_ARRAY_BUFFER,vc*sizeof(float),v,GL_STATIC_DRAW);
    glUseProgram(textShader); glUniform3f(glGetUniformLocation(textShader,"col"),r,g,b);
    glUniform1f(glGetUniformLocation(textShader,"alpha"),a);
    glBindTexture(GL_TEXTURE_2D,fontTex); glBindVertexArray(textVAO); glDrawArrays(GL_TRIANGLES,0,vc/4);
    glEnable(GL_CULL_FACE);
}

inline float textAdvanceNdc(float sc) {
    float cw = sc * 8.0f / (float)currentWinW * 2.0f;
    return cw * 0.75f;
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

inline bool sliderInputActive = false;
inline int sliderInputTab = -1;
inline int sliderInputItem = -1;
inline char sliderInputBuf[8] = {};
inline int sliderInputLen = 0;

inline void drawSlider(float x,float y,float w,float val,float r,float g,float b) {
    (void)w;
    const int slots = 30;
    int filled = (int)(val * (float)slots + 0.5f);
    if(filled < 0) filled = 0;
    if(filled > slots) filled = slots;

    char base[slots + 1];
    for(int i=0;i<slots;i++) base[i]='-';
    base[slots]=0;
    drawText(base,x,y,1.55f,r*0.25f,g*0.25f,b*0.25f,0.85f);

    if(filled > 0){
        char fill[slots + 1];
        for(int i=0;i<filled;i++) fill[i]='=';
        fill[filled]=0;
        drawText(fill,x,y,1.55f,r,g,b,0.95f);
    }

    int knobIndex = filled > 0 ? (filled - 1) : 0;
    float knobX = x + textAdvanceNdc(1.55f) * (float)knobIndex;
    drawText("|",knobX,y,1.55f,0.98f,0.96f,0.88f,0.98f);
}

inline void drawMenuAtmosphere(float tm) {
    glUseProgram(menuFxShader);
    glUniform1f(glGetUniformLocation(menuFxShader,"tm"),tm);
    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    float drift = 0.5f + 0.5f * sinf(tm * 0.28f);
    drawOverlayRectNdc(-1.0f,-1.0f,1.0f,-0.88f,0.03f,0.03f,0.03f,0.07f + drift * 0.02f);
    drawOverlayRectNdc(-1.0f,0.88f,1.0f,1.0f,0.03f,0.03f,0.03f,0.07f + (1.0f - drift) * 0.02f);
}

inline void drawMenu(float tm) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawMainMenuBackdrop(tm);
    drawMenuAtmosphere(tm);
    // Keep corridor visible while adding warm grade
    drawFullscreenOverlay(0.02f,0.02f,0.02f,0.16f);
    drawFullscreenOverlay(0.17f,0.13f,0.08f,0.05f);
    float p=0.82f+0.06f*sinf(tm*2.0f);
    float gl = sinf(tm * 0.7f) * 0.0016f;
    drawTextCentered("BACKROOMS: VOID SHIFT",0.0f+gl,0.5f,3.4f,0.9f,0.85f,0.4f,p);
    char levelBuf[32];
    buildLevelLabel(gCurrentLevel, levelBuf, 32);
    drawTextCentered(levelBuf,0.0f,0.35f,2.5f,0.7f,0.65f,0.3f,0.8f);
    const char* it[]={"START CONTRACT","MULTIPLAYER","SETTINGS","GUIDE","QUIT"};
    for(int i=0;i<5;i++){
        float s=(menuSel==i)?1.0f:0.5f; float y=0.10f-i*0.11f;
        float baseX = -measureTextWidthNdc(it[i], 2.0f) * 0.5f;
        if(menuSel==i)drawText(">", baseX - 0.08f, y, 2.0f, 0.9f*s,0.85f*s,0.4f*s);
        drawText(it[i], baseX, y, 2.0f,0.9f*s,0.85f*s,0.4f*s);
    }
    drawTextCentered("UP/DOWN - SELECT    ENTER - CONFIRM",0.0f,-0.6f,1.5f,0.5f,0.5f,0.4f,0.6f);
    drawTextCentered("v1.0.0",0.0f,-0.72f,1.2f,0.64f,0.62f,0.54f,0.75f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawGuideScreen() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawFullscreenOverlay(0.02f,0.02f,0.03f,0.80f);
    drawTextCentered("VOID SHIFT GUIDE",0.0f,0.58f,3.0f,0.9f,0.85f,0.4f,0.96f);
    drawTextCentered("GOAL: COMPLETE CONTRACTS, STABILIZE REALITY, THEN EXTRACT",0.0f,0.40f,1.56f,0.82f,0.86f,0.64f,0.94f);
    drawTextCentered("RESONATOR MODES: SCAN / RECORD / PLAYBACK / PING",0.0f,0.30f,1.46f,0.72f,0.84f,0.86f,0.92f);
    drawTextCentered("ECHO RECORDING: HOLD R TO RECORD, RELEASE TO SAVE, P TO PLAY",0.0f,0.22f,1.40f,0.76f,0.74f,0.86f,0.92f);
    drawTextCentered("ATTENTION RISES FROM NOISE/LIGHT/ECHO. STAY IN CONTROL",0.0f,0.12f,1.44f,0.92f,0.66f,0.50f,0.94f);
    drawTextCentered("LEVEL 1: STABILIZE 3 NODES + HOLD 90s",0.0f,0.04f,1.42f,0.90f,0.72f,0.64f,0.92f);
    drawTextCentered("LEVEL 2: BATTERY + 3 FUSES + ACCESS + LIFT HOLD",0.0f,-0.06f,1.42f,0.82f,0.86f,0.64f,0.92f);
    drawTextCentered("NPCS: CARTOGRAPHER / DISPATCHER / LOST SURVIVOR",0.0f,-0.16f,1.34f,0.78f,0.84f,0.70f,0.90f);
    drawTextCentered("SIDE CONTRACTS GRANT ARCHIVE POINTS + PERKS",0.0f,-0.24f,1.30f,0.76f,0.84f,0.72f,0.90f);
    drawTextCentered("CONTROLS: WASD MOVE, SHIFT SPRINT, C CROUCH, E INTERACT",0.0f,-0.32f,1.36f,0.70f,0.76f,0.66f,0.90f);
    drawTextCentered("ESC OR ENTER - BACK",0.0f,-0.70f,1.52f,0.56f,0.62f,0.50f,0.82f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawSettings(bool fp) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    if(fp) drawFullscreenOverlay(0.02f,0.02f,0.03f,0.72f);
    drawTextCentered("SETTINGS",0.0f,0.55f,3.0f,0.9f,0.85f,0.4f);
    const float rightColCenterX = 0.50f;
    const bool audioTab = settingsTab == SETTINGS_TAB_AUDIO;
    const bool effectsTab = settingsTab == SETTINGS_TAB_EFFECTS;
    const int itemCount = settingsItemsForTab(settingsTab);
    const char* tabN[4]={"VIDEO","EFFECTS","AUDIO","BINDS"}; const char* tabB[4]={"[VIDEO]","[EFFECTS]","[AUDIO]","[BINDS]"};
    for(int t=0;t<4;t++){ float a=settingsTab==t?1.0f:0.52f;
        drawTextCentered(settingsTab==t?tabB[t]:tabN[t],-0.45f+0.30f*t,0.44f,1.35f,0.86f*a,0.84f*a,0.58f*a,0.96f);
    }
    for(int i=0;i<itemCount;i++){
        float s=(menuSel==i)?1.0f:0.5f;
        float y=0.33f-i*0.09f;
        if(menuSel==i)drawText(">",-0.55f,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
        if(i==0){
            drawText("CATEGORY",-0.48f,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            drawTextCentered(tabN[settingsTab],rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            continue;
        }
        if(audioTab){
            const char* lb[]={"MASTER VOL","MUSIC VOL","AMBIENCE VOL","SFX VOL","VOICE VOL","BACK"};
            float* vl[] = {&settings.masterVol,&settings.musicVol,&settings.ambienceVol,&settings.sfxVol,&settings.voiceVol,nullptr};
            float mx[] = {1.0f,1.0f,1.0f,1.0f,1.0f,1.0f};
            int ai = i - 1;
            drawText(lb[ai],-0.48f,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            if(vl[ai]){
                float nv=*vl[ai]/mx[ai]; if(nv>1.0f)nv=1.0f;
                drawSlider(-0.02f,y,0.45f,nv,0.9f*s,0.85f*s,0.4f*s);
                char b[16]; snprintf(b,16,"%d%%",(int)(nv*100));
                drawTextCentered(b,rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }
        }else if(effectsTab){
            const char* lb[]={"SSAO","GI","GOD RAYS","BLOOM","DENOISER","DENOISE STR","BACK"};
            int vi = i - 1;
            drawText(lb[vi],-0.48f,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            if(vi==0) drawTextCentered(ssaoLabel(settings.ssaoQuality),rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            else if(vi==1) drawTextCentered(giLabel(settings.giQuality),rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            else if(vi==2) drawTextCentered(settings.godRays?"ON":"OFF",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            else if(vi==3) drawTextCentered(settings.bloom?"ON":"OFF",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            else if(vi==4) drawTextCentered(settings.rtxDenoise?"ON":"OFF",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            else if(vi==5){
                float nv=settings.rtxDenoiseStrength; if(nv>1.0f)nv=1.0f; if(nv<0.0f)nv=0.0f;
                drawSlider(-0.02f,y,0.45f,nv,0.9f*s,0.85f*s,0.4f*s);
                char b[16]; snprintf(b,16,"%d%%",(int)(nv*100));
                drawTextCentered(b,rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }
        }else if(settingsTab==SETTINGS_TAB_BINDS){
            if(i==1){ drawText("OPEN BIND MENU",-0.48f,y,1.7f,0.9f*s,0.85f*s,0.4f*s); drawTextCentered("ENTER",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s); }
            else if(i==2){ drawText("BACK",-0.48f,y,1.7f,0.9f*s,0.85f*s,0.4f*s); }
        }else{
            const char* lb[]={"VHS EFFECT","MOUSE SENS","UPSCALER","RESOLUTION","FSR SHARPNESS","ANTI-ALIASING","FAST MATH","FRAME GEN","V-SYNC","DEBUG MODE","BACK"};
            int vi = i - 1;
            drawText(lb[vi],-0.48f,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            if(vi==0 || vi==1 || vi==4){
                float val = 0.0f; float maxV = 1.0f;
                if(vi==0) val = settings.vhsIntensity;
                else if(vi==1){ val = settings.mouseSens; maxV = 0.006f; }
                else val = settings.fsrSharpness;
                float nv=val/maxV; if(nv>1.0f)nv=1.0f;
                drawSlider(-0.02f,y,0.45f,nv,0.9f*s,0.85f*s,0.4f*s);
                char b[16]; snprintf(b,16,"%d%%",(int)(nv*100));
                drawTextCentered(b,rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==2){
                drawTextCentered(upscalerModeLabel(settings.upscalerMode),rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==3){
                char rb[24];
                if(clampUpscalerMode(settings.upscalerMode)==UPSCALER_MODE_OFF) snprintf(rb,24,"NATIVE");
                else { snprintf(rb,24,"%d%%",renderScalePercentFromPreset(settings.renderScalePreset)); }
                drawTextCentered(rb,rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==5){
                drawTextCentered(aaModeLabel(settings.aaMode),rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==6){
                drawTextCentered(settings.fastMath?"ON":"OFF",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==7){
                drawTextCentered(frameGenModeLabel(settings.frameGenMode),rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==8){
                drawTextCentered(settings.vsync?"ON":"OFF",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }else if(vi==9){
                drawTextCentered(settings.debugMode?"ON":"OFF",rightColCenterX,y,1.7f,0.9f*s,0.85f*s,0.4f*s);
            }
        }
    }
    // Show numeric input overlay if active
    if(sliderInputActive) {
        float iy = 0.33f - sliderInputItem * 0.09f;
        drawOverlayRectNdc(0.42f, iy - 0.02f, 0.72f, iy + 0.04f, 0.05f, 0.05f, 0.08f, 0.9f);
        char inputDisp[16];
        sliderInputBuf[sliderInputLen] = 0;
        snprintf(inputDisp, 16, "%s_%%", sliderInputBuf);
        drawText(inputDisp, 0.44f, iy, 1.9f, 0.95f, 0.9f, 0.5f, 1.0f);
        drawTextCentered("TYPE 0-100  ENTER CONFIRM  ESC CANCEL", 0.0f, -0.58f, 1.35f, 0.7f, 0.65f, 0.35f, 0.8f);
    } else {
        if(settingsTab==SETTINGS_TAB_BINDS) drawTextCentered("TAB NEXT TAB  ENTER OPEN BIND MENU  ESC BACK", 0.0f, -0.58f, 1.35f, 0.5f, 0.5f, 0.4f, 0.6f);
        else drawTextCentered("TAB NEXT TAB  L/R ADJUST  ENTER TYPE VALUE  ESC BACK", 0.0f, -0.58f, 1.35f, 0.5f, 0.5f, 0.4f, 0.6f);
    }
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawPause() {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawFullscreenOverlay(0.02f,0.02f,0.03f,0.72f);
    drawTextCentered("PAUSED",0.0f,0.25f,3.0f,0.9f,0.85f,0.4f);
    const char* it[]={"RESUME","SETTINGS","GUIDE","MAIN MENU","QUIT"};
    for(int i=0;i<5;i++){
        float s=(menuSel==i)?1.0f:0.5f,y=-i*0.1f;
        float baseX = -measureTextWidthNdc(it[i], 1.8f) * 0.5f;
        if(menuSel==i)drawText(">",baseX - 0.07f,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
        drawText(it[i],baseX,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
    }
    drawTextCentered("ESC - RESUME",0.0f,-0.55f,1.5f,0.5f,0.5f,0.4f,0.6f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawDeath(float tm) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float fl=(rand()%100<28)?0.25f:1.0f, p=0.68f+0.22f*sinf(tm*4.2f);
    float deathPulse = 0.5f + 0.5f * sinf(tm * 2.8f);
    float ch = 0.0015f + deathPulse * 0.0045f;
    drawFullscreenOverlay(0.01f,0.0f,0.0f,0.88f);
    drawOverlayRectNdc(-1.0f,-1.0f,1.0f,-0.86f,0.32f,0.02f,0.02f,0.55f);
    drawOverlayRectNdc(-1.0f,0.86f,1.0f,1.0f,0.32f,0.02f,0.02f,0.55f);
    drawOverlayRectNdc(-1.0f,-1.0f,1.0f,1.0f,0.18f,0.02f,0.02f,0.06f + deathPulse * 0.08f);
    drawTextCentered("YOU DIED",-ch,0.2f,4.2f,0.88f*fl,0.10f*fl,0.10f*fl,p*0.82f);
    drawTextCentered("YOU DIED",ch,0.2f,4.2f,0.22f*fl,0.12f*fl,0.88f*fl,p*0.72f);
    drawTextCentered("YOU DIED",0.0f,0.2f,4.2f,0.9f*fl,0.08f*fl,0.08f*fl,p);
    drawTextCentered("CONTRACT FAILED",0.0f,0.01f,2.1f,0.72f,0.12f,0.12f,0.76f);
    drawTextCentered(gDeathReason,0.0f,-0.05f,1.55f,0.78f,0.62f,0.52f,0.82f);
    int m=(int)(gSurvivalTime/60),s=(int)gSurvivalTime%60;
    char tb[32]; snprintf(tb,32,"SURVIVED: %d:%02d",m,s);
    drawTextCentered(tb,0.0f,-0.16f,2.0f,0.7f,0.6f,0.3f,0.8f);
    drawTextCentered("PRESS ENTER TO RESTART",0.0f,-0.35f,1.8f,0.5f,0.4f,0.35f,0.6f);
    drawTextCentered("PRESS ESC FOR MAIN MENU",0.0f,-0.47f,1.8f,0.5f,0.4f,0.35f,0.6f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawEscape(float tm) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float p=0.76f+0.12f*sinf(tm*2.4f);
    drawTextCentered("YOU ESCAPED",0.0f,0.2f,4.0f,0.75f,0.9f,0.75f,p);
    drawTextCentered("EXTRACTION ROUTE UNLOCKED.",0.0f,0.02f,2.0f,0.62f,0.78f,0.62f,0.84f);
    int m=(int)(gSurvivalTime/60),s=(int)gSurvivalTime%60;
    char tb[32]; snprintf(tb,32,"SURVIVED: %d:%02d",m,s);
    drawTextCentered(tb,0.0f,-0.12f,2.0f,0.7f,0.8f,0.68f,0.86f);
    drawTextCentered("PRESS ENTER TO CONTINUE",0.0f,-0.35f,1.8f,0.55f,0.64f,0.54f,0.7f);
    drawTextCentered("PRESS ESC FOR MAIN MENU",0.0f,-0.47f,1.8f,0.55f,0.64f,0.54f,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawSurvivalTime(float t) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    int m=(int)(t/60),s=(int)t%60; char b[16]; snprintf(b,16,"%d:%02d",m,s);
    drawText(b,0.72f,0.9f,2.0f,0.78f,0.72f,0.48f,0.96f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawKeybindsMenu(bool fromPause, int selected, int captureIndex) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    if(fromPause) drawFullscreenOverlay(0.02f,0.02f,0.03f,0.72f);
    drawTextCentered("KEY BINDS",0.0f,0.62f,2.6f,0.9f,0.85f,0.4f);
    for(int i=0;i<GAMEPLAY_BIND_COUNT;i++){
        float s=(selected==i)?1.0f:0.55f;
        float y=0.48f-i*0.075f;
        if(selected==i) drawText(">",-0.62f,y,1.45f,0.92f*s,0.86f*s,0.42f*s);
        drawText(gameplayBindLabel(i),-0.56f,y,1.38f,0.9f*s,0.85f*s,0.4f*s);
        const char* keyName = keyNameForUi(*gameplayBindByIndex(settings.binds, i));
        if(captureIndex==i) keyName = "...";
        drawText(keyName,0.38f,y,1.38f,0.82f*s,0.9f*s,0.72f*s,0.95f);
    }
    float bs=(selected==KEYBINDS_BACK_INDEX)?1.0f:0.55f;
    float by=0.48f-GAMEPLAY_BIND_COUNT*0.075f;
    if(selected==KEYBINDS_BACK_INDEX) drawText(">",-0.62f,by,1.45f,0.92f*bs,0.86f*bs,0.42f*bs);
    drawText("BACK",-0.56f,by,1.45f,0.9f*bs,0.85f*bs,0.4f*bs);
    drawTextCentered(captureIndex>=0?"PRESS ANY KEY TO REBIND":"ENTER TO REBIND  ESC TO BACK",0.0f,-0.84f,1.35f,0.58f,0.58f,0.46f,0.86f);
    if(fromPause) drawTextCentered("APPLIES IN CURRENT RUN",0.0f,-0.92f,1.1f,0.5f,0.55f,0.45f,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

// Split out some rarely-touched menu pages to keep this header under the 500-line limit enforced by build.bat.
#define BR_MENU_INTERNAL
#include "menu_pages.h"
#undef BR_MENU_INTERNAL
