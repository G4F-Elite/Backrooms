#pragma once
#include <cstring>
#include <cmath>
#include <cstdio>
#include "upscaler_settings.h"

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
struct Settings {
    float masterVol=0.7f;
    float musicVol=0.55f;
    float ambienceVol=0.75f;
    float sfxVol=0.7f;
    float voiceVol=0.65f;
    float vhsIntensity=0.65f;
    float mouseSens=0.002f;
    int upscalerMode=UPSCALER_MODE_OFF;
    int renderScalePreset=RENDER_SCALE_PRESET_DEFAULT;
    float fsrSharpness=0.35f;
};
inline Settings settings;
enum GameState { STATE_MENU, STATE_GAME, STATE_PAUSE, STATE_SETTINGS, STATE_SETTINGS_PAUSE, STATE_INTRO, STATE_NOTE, STATE_MULTI, STATE_MULTI_HOST, STATE_MULTI_JOIN, STATE_MULTI_WAIT };
inline GameState gameState = STATE_MENU;
inline int menuSel=0, currentWinW=1280, currentWinH=720;
inline float gSurvivalTime = 0;

inline const char* textVS = R"(#version 330 core
layout(location=0) in vec2 p; layout(location=1) in vec2 t; out vec2 uv;
void main() { gl_Position = vec4(p, 0.0, 1.0); uv = t; })";
inline const char* textFS = R"(#version 330 core
in vec2 uv; out vec4 fc; uniform sampler2D tex; uniform vec3 col; uniform float alpha;
void main() { float a = texture(tex, uv).r; fc = vec4(col, a * alpha); })";

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
}

inline void drawText(const char* s, float x, float y, float sc, float r, float g, float b, float a=1.0f) {
    float v[512*24]; int vc=0; float cx=x,cy=y,cw=sc*8.0f/(float)currentWinW*2.0f,ch=sc*8.0f/(float)currentWinH*2.0f;
    for(const char*p=s;*p && vc<512*24-24;p++) { if(*p=='\n'){cx=x;cy-=ch*1.2f;continue;} int c=*p-32; if(c<0||c>95)c=0;
        float u0=c*8.0f/(96.0f*8.0f),u1=(c+1)*8.0f/(96.0f*8.0f);
        float t[24]={cx,cy,u0,1,cx+cw,cy,u1,1,cx+cw,cy+ch,u1,0,cx,cy,u0,1,cx+cw,cy+ch,u1,0,cx,cy+ch,u0,0};
        for(int i=0;i<24;i++) v[vc++]=t[i]; cx+=cw*0.75f;
    }
    glBindBuffer(GL_ARRAY_BUFFER,textVBO); glBufferData(GL_ARRAY_BUFFER,vc*4,v,GL_STATIC_DRAW);
    glUseProgram(textShader); glUniform3f(glGetUniformLocation(textShader,"col"),r,g,b);
    glUniform1f(glGetUniformLocation(textShader,"alpha"),a);
    glBindTexture(GL_TEXTURE_2D,fontTex); glBindVertexArray(textVAO); glDrawArrays(GL_TRIANGLES,0,vc/4);
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

inline void drawSlider(float x,float y,float w,float val,float r,float g,float b) {
    char bar[21]; int f=(int)(val*20); for(int i=0;i<20;i++)bar[i]=(i<f)?'=':'-'; bar[20]=0;
    drawText("[",x,y,1.8f,r*0.5f,g*0.5f,b*0.5f); drawText(bar,x+0.02f,y,1.8f,r,g,b); drawText("]",x+w-0.02f,y,1.8f,r*0.5f,g*0.5f,b*0.5f);
}

inline void drawMenu(float tm) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float p=0.8f+0.05f*sinf(tm*2.0f), gl=(rand()%100<3)?(rand()%10-5)*0.003f:0;
    drawTextCentered("THE BACKROOMS",0.0f+gl,0.5f,4.0f,0.9f,0.85f,0.4f,p);
    drawTextCentered("LEVEL 0",0.0f,0.35f,2.5f,0.7f,0.65f,0.3f,0.8f);
    const char* it[]={"START GAME","MULTIPLAYER","SETTINGS","QUIT"};
    for(int i=0;i<4;i++){
        float s=(menuSel==i)?1.0f:0.5f; float y=0.08f-i*0.12f;
        float baseX = -measureTextWidthNdc(it[i], 2.0f) * 0.5f;
        if(menuSel==i)drawText(">", baseX - 0.08f, y, 2.0f, 0.9f*s,0.85f*s,0.4f*s);
        drawText(it[i], baseX, y, 2.0f,0.9f*s,0.85f*s,0.4f*s);
    }
    drawTextCentered("UP/DOWN - SELECT    ENTER - CONFIRM",0.0f,-0.6f,1.5f,0.5f,0.5f,0.4f,0.6f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawSettings(bool fp) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawTextCentered("SETTINGS",0.0f,0.55f,3.0f,0.9f,0.85f,0.4f);
    const char* lb[]={"MASTER VOL","MUSIC VOL","AMBIENCE VOL","SFX VOL","VOICE VOL","VHS EFFECT","MOUSE SENS","UPSCALER","RESOLUTION","FSR SHARPNESS","BACK"};
    float*vl[]={&settings.masterVol,&settings.musicVol,&settings.ambienceVol,&settings.sfxVol,&settings.voiceVol,&settings.vhsIntensity,&settings.mouseSens,nullptr,nullptr,&settings.fsrSharpness,nullptr};
    float mx[]={1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,0.006f,1.0f,1.0f,1.0f,1.0f};
    for(int i=0;i<11;i++){
        float s=(menuSel==i)?1.0f:0.5f,y=0.43f-i*0.09f;
        if(menuSel==i)drawText(">",-0.55f,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
        drawText(lb[i],-0.48f,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
        if(i==7){
            drawText(upscalerModeLabel(settings.upscalerMode),0.36f,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
        }else if(i==8){
            int scalePercent = renderScalePercentFromPreset(settings.renderScalePreset);
            char rb[24]; snprintf(rb,24,"%d%%",scalePercent);
            drawText(rb,0.48f,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
        }else if(vl[i]){
            float nv=*vl[i]/mx[i]; if(nv>1.0f)nv=1.0f;
            drawSlider(0.1f,y,0.45f,nv,0.9f*s,0.85f*s,0.4f*s);
            char b[16]; snprintf(b,16,"%d%%",(int)(nv*100)); drawText(b,0.58f,y,1.8f,0.9f*s,0.85f*s,0.4f*s);
        }
    }
    drawTextCentered("LEFT/RIGHT - ADJUST    ENTER - BACK",0.0f,-0.55f,1.5f,0.5f,0.5f,0.4f,0.6f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawPause() {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawTextCentered("PAUSED",0.0f,0.25f,3.0f,0.9f,0.85f,0.4f);
    const char* it[]={"RESUME","SETTINGS","MAIN MENU","QUIT"};
    for(int i=0;i<4;i++){
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
    float fl=(rand()%100<15)?0.3f:1.0f, p=0.7f+0.15f*sinf(tm*3.0f);
    drawTextCentered("YOU DIED",0.0f,0.2f,4.0f,0.8f*fl,0.1f*fl,0.1f*fl,p);
    drawTextCentered("IT GOT YOU...",0.0f,0.02f,2.0f,0.6f,0.15f,0.15f,0.7f);
    int m=(int)(gSurvivalTime/60),s=(int)gSurvivalTime%60;
    char tb[32]; snprintf(tb,32,"SURVIVED: %d:%02d",m,s);
    drawTextCentered(tb,0.0f,-0.12f,2.0f,0.7f,0.6f,0.3f,0.8f);
    drawTextCentered("PRESS ENTER TO RESTART",0.0f,-0.35f,1.8f,0.5f,0.4f,0.35f,0.6f);
    drawTextCentered("PRESS ESC FOR MAIN MENU",0.0f,-0.47f,1.8f,0.5f,0.4f,0.35f,0.6f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawSurvivalTime(float t) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    int m=(int)(t/60),s=(int)t%60; char b[16]; snprintf(b,16,"%d:%02d",m,s);
    drawText(b,0.72f,0.9f,2.0f,0.78f,0.72f,0.48f,0.96f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawHealthBar(float hp) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    char b[21]; int f=(int)(hp/5); for(int i=0;i<20;i++)b[i]=(i<f)?'|':' '; b[20]=0;
    float r=(hp<30)?0.9f:0.7f, g=(hp>50)?0.7f:0.3f;
    drawText("HP:",-0.95f,-0.85f,1.7f,r,g,0.2f,0.8f); drawText(b,-0.80f,-0.85f,1.7f,r,g,0.2f,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawSanityBar(float sn) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    char b[21]; int f=(int)(sn/5); for(int i=0;i<20;i++)b[i]=(i<f)?'~':' '; b[20]=0;
    float p=(sn<30)?0.6f+0.3f*sinf((float)rand()/1000.0f):1.0f;
    drawText("SN:",-0.95f,-0.92f,1.7f,0.4f*p,0.3f*p,0.7f*p,0.8f); drawText(b,-0.80f,-0.92f,1.7f,0.4f*p,0.3f*p,0.7f*p,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawDamageOverlay(float fl,float hp) {
    if(fl<0.01f && hp>50)return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float lo=(100.0f-hp)/100.0f*0.3f, a=fl+lo; if(a>0.8f)a=0.8f;
    drawText("                                                  ",-1.0f,-1.0f,50.0f,0.6f,0.0f,0.0f,a*0.4f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawStaminaBar(float st) {
    if(st >= 99.0f) return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    char b[21]; int f=(int)(st/5); for(int i=0;i<20;i++)b[i]=(i<f)?'>':' '; b[20]=0;
    float yc=(st<20)?0.5f+0.3f*sinf((float)rand()/500.0f):0.7f;
    drawText("ST:",-0.95f,-0.78f,1.7f,0.3f,0.6f*yc,0.3f,0.8f); drawText(b,-0.80f,-0.78f,1.7f,0.3f,0.6f*yc,0.3f,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawFlashlightBattery(float battery, bool isOn) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    char b[11]; int f=(int)(battery/10); for(int i=0;i<10;i++)b[i]=(i<f)?'#':'.'; b[10]=0;
    float r=isOn?0.9f:0.4f, g=isOn?0.85f:0.4f;
    drawText("FL:",0.52f,-0.92f,1.7f,r,g,0.3f,0.8f); drawText(b,0.66f,-0.92f,1.7f,r,g,0.3f,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawNoteCounter(int count) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    char b[16]; snprintf(b,16,"NOTES: %d/5",count);
    drawText(b,-0.95f,0.9f,1.65f,0.6f,0.55f,0.35f,0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawPhaseIndicator(int phase) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const char* pn[]={"EXPLORATION","TENSION","PURSUIT","ESCAPE"};
    if(phase>=0&&phase<4){
        drawText("PHASE:",0.49f,0.85f,1.25f,0.45f,0.4f,0.28f,0.55f);
        drawText(pn[phase],0.60f,0.82f,1.35f,0.5f,0.45f,0.3f,0.58f);
    }
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawInteractPrompt() {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawText("[E] READ NOTE",-0.15f,-0.4f,1.8f,0.8f,0.75f,0.5f,0.8f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawHallucinationEffect(float intensity) {
    if(intensity<0.05f)return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float a=intensity*0.3f; if(a>0.4f)a=0.4f;
    drawText("                                        ",-1.0f,-1.0f,50.0f,0.3f,0.0f,0.4f,a);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawIntro(int line, float timer, float lineTime, const char** introLines) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
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
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawNote(int noteId, const char* title, const char* content) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const char* backFill = "                                                  ";
    for(float by = 0.96f; by >= -0.96f; by -= 0.08f) {
        drawTextCentered(backFill, 0.0f, by, 3.4f, 0.07f, 0.05f, 0.03f, 0.90f);
    }
    drawTextCentered(backFill, 0.0f, 0.88f, 3.4f, 0.22f, 0.18f, 0.12f, 0.22f);

    const char* panelShadow = "                                                  ";
    const char* panelBody = "                                              ";
    for(float yp = 0.76f; yp >= -0.74f; yp -= 0.09f) {
        drawTextCentered(panelShadow, 0.0f, yp - 0.01f, 3.0f, 0.06f, 0.04f, 0.02f, 0.55f);
        drawTextCentered(panelBody, 0.0f, yp, 3.0f, 0.84f, 0.79f, 0.64f, 0.97f);
    }

    drawTextCentered(title, 0.0f, 0.56f, 2.5f, 0.3f, 0.25f, 0.15f, 1.0f);
    drawTextCentered("________________________________", 0.0f, 0.46f, 1.5f, 0.4f, 0.35f, 0.2f, 0.92f);

    float ty = 0.30f;
    char line[64];
    int li = 0;
    for(const char* p = content; *p; p++) {
        if(*p == '\n' || li >= 50) {
            line[li] = 0;
            drawTextCentered(line, 0.0f, ty, 1.5f, 0.25f, 0.2f, 0.1f, 1.0f);
            ty -= 0.08f;
            li = 0;
        } else {
            line[li++] = *p;
        }
    }
    if(li > 0) {
        line[li] = 0;
        drawTextCentered(line, 0.0f, ty, 1.5f, 0.25f, 0.2f, 0.1f, 1.0f);
    }
    drawTextCentered("PRESS E OR ESC TO CLOSE", 0.0f, -0.75f, 1.5f, 0.45f, 0.4f, 0.24f, 0.96f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
