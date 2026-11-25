// Exercise the actual client post-processing passes without a window on screen.
#define LIMEN_SHOTS
#define main presentation_test_unused_client_main
#include "../src/game_core.h"
#undef main
#include <cassert>

namespace {
using Image = std::vector<unsigned char>;
using ReadPixels = void (APIENTRY*)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
using GetError = GLenum (APIENTRY*)();
PostTarget display;

void resize(int w,int h) {
    windowResize(nullptr,w,h);
    display.prepare(winW,winH);
    assert(!taaHistoryValid && taaFrameIndex==0);
}
void scene(int bias) {
    Image rgba(renderW*renderH*4);
    for(int y=0;y<renderH;++y) for(int x=0;x<renderW;++x) {
        unsigned char value = static_cast<unsigned char>(48+bias+((x/3+y/5)%8)*24);
        size_t i=4*(y*renderW+x);
        rgba[i]=rgba[i+1]=rgba[i+2]=value;
        rgba[i+3]=255;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,fboTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,renderW,renderH,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    postFxCaptureScene(Mat4::persp(1.f,float(winW)/winH,.1f,320.f),Mat4(),nullptr,0);
}
Image present(float tape=0.f,float death=0.f) {
    presentScene(display.framebuffer,tape,death);
    Image pixels(winW*winH*4);
    static auto read = reinterpret_cast<ReadPixels>(glfwGetProcAddress("glReadPixels"));
    static auto error = reinterpret_cast<GetError>(glfwGetProcAddress("glGetError"));
    assert(read && error);
    read(0,0,winW,winH,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    assert(error()==0);
    return pixels;
}
void exerciseHistory() {
    settings.upscalerMode=UPSCALER_MODE_FSR20;
    settings.aaMode=AA_MODE_OFF;
    resize(159,123);
    scene(0);
    const Image fresh=present();
    assert(taaHistoryValid && taaFrameIndex==1);
    scene(5);
    const Image withHistory=present();
    assert(taaFrameIndex==2);
    ++worldSeed; // a new seed must reject the previous colour history
    const Image withoutHistory=present();
    assert(fresh!=withoutHistory && withHistory!=withoutHistory);
    scene(0);
    ++gCurrentLevel;
    assert(present()==fresh);
    initTaaTargets();
    const Image taped=present(.8f);
    assert(taped!=fresh);
    scene(5);
    assert(present()==withHistory); // tape is applied after storing clean scene colour
    settings.aaMode=AA_MODE_TAA;
    assert(present()==withoutHistory); // mode switch invalidates accumulated colour
    resize(121,133);
    scene(0);
    const Image resized=present();
    initTaaTargets();
    assert(resized==present());

    gPostFx.sceneDrawn=false;
    glClearColor(0,0,0,1);
    const Image blank=present();
    assert(!taaHistoryValid && taaFrameIndex==0);
    for(size_t i=0;i<blank.size();i+=4) assert(blank[i]==0 && blank[i+1]==0 && blank[i+2]==0);
}
void exercisePasses() {
    for(int mode : {UPSCALER_MODE_OFF, UPSCALER_MODE_NEAREST, UPSCALER_MODE_FSR10}) {
        settings.upscalerMode=mode;
        settings.aaMode=AA_MODE_OFF;
        resize(128,120);
        scene(0);
        const Image raw=present();
        assert(!taaHistoryValid);
        for(size_t i=0;i<raw.size();i+=4) assert(raw[i]>=48 && raw[i]<=216);
        assert(present(.8f)!=raw);
        assert(present(0.f,.8f)!=raw);
        gPostPixelRows=8;
        assert(present()!=raw);
        gPostPixelRows=0;
        settings.bloom=true;
        assert(present()!=raw);
        assert(present(.8f)!=raw);
        settings.bloom=false;
        assert(present()==raw);
    }
}
}

int main() {
    assert(glfwInit());
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    auto* window=glfwCreateWindow(64,64,"presentation test",nullptr,nullptr);
    assert(window);
    glfwMakeContextCurrent(window);
    assert(gladLoadGL());
    initQuad(quadVAO,quadVBO);
    vhsShader=mkShader(vhsVS,vhsFS);
    settings.ssaoQuality=0; // compare isolated history, bloom and tape passes
    exerciseHistory();
    exercisePasses();
    glfwTerminate();
}
