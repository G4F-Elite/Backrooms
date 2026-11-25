// Present a calibration image through the real VHS path on a hidden GL context.
// One half measures temporal grain; one-pixel vertical bars measure horizontal detail.
#define LIMEN_SHOTS
#define main vhs_test_unused_client_main
#include "../src/game_core.h"
#undef main
#include <array>
#include <cassert>
#include <cmath>

namespace {
constexpr int width=640, height=384;
using Image = std::vector<unsigned char>;
using ReadPixels = void (APIENTRY*)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
PostTarget display;

Image calibrationImage() {
    Image rgba(4*width*height);
    for (int y=0; y<height; ++y) for (int x=0; x<width; ++x) {
        size_t pos=4*(y*width+x);
        unsigned char v = x<width/2 ? 104 : (x%2==0 ? 180 : 64);
        rgba[pos]=rgba[pos+1]=rgba[pos+2]=v;
        rgba[pos+3]=255;
    }
    return rgba;
}

Image readDisplay() {
    static auto read=reinterpret_cast<ReadPixels>(glfwGetProcAddress("glReadPixels"));
    assert(read);
    Image rgba(width*height*4);
    read(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    return rgba;
}

Image present(float level, float time, float rows=height/3.f) {
    static const PostProgram tape(vhsShader);
    static const GLint scanRows=glGetUniformLocation(vhsShader,"tapeRows");
    assert(scanRows>=0);
    tape.begin(display.framebuffer,width,height,fboTex);
    glUniform1f(tape.tm,time);
    glUniform1f(tape.inten,level);
    glUniform1f(scanRows,rows);
    glUniform3f(tape.pixel,1.f/width,1.f/height,0.f);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    return readDisplay();
}

double luma(const Image& image, int x, int y) {
    size_t i=4*(y*width+x);
    return .299*image[i]+.587*image[i+1]+.114*image[i+2];
}

struct Metrics { double grainRms=0, detailContrast=0; };

Metrics measure(float level, float rows=height/3.f) {
    Metrics result;
    constexpr int frames=8;
    Image last;
    int grains=0, pairs=0;
    for (int frame=0; frame<frames; ++frame) {
        const Image current=present(level, 1.5f+frame/24.f,rows);
        for (int y=height/4; y<height*3/4; ++y) {
            // Remove the row mean before measuring changes so global flicker and
            // scanlines alone cannot satisfy the grain test.
            if (!last.empty()) {
                double rowSum=0, rowSquares=0;
                const int lo=width/8, hi=width*3/8;
                for(int x=lo; x<hi; ++x){
                    double d=luma(current,x,y)-luma(last,x,y);
                    rowSum+=d;
                    rowSquares+=d*d;
                }
                result.grainRms+=rowSquares-rowSum*rowSum/(hi-lo);
                grains+=hi-lo;
            }
            for(int x=width*5/8; x<width*7/8; x+=2){
                result.detailContrast+=luma(current,x,y)-luma(current,x+1,y);
                ++pairs;
            }
        }
        last=current;
    }
    result.grainRms=std::sqrt(result.grainRms/grains);
    result.detailContrast=std::abs(result.detailContrast/pairs);
    return result;
}
}

int main() {
    assert(glfwInit());
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    auto* window=glfwCreateWindow(64,64,"tape response test",nullptr,nullptr);
    assert(window);
    glfwMakeContextCurrent(window);
    assert(gladLoadGL());
    initQuad(quadVAO,quadVBO);
    vhsShader=mkShader(vhsVS,vhsFS);
    settings.upscalerMode=UPSCALER_MODE_OFF;
    settings.aaMode=AA_MODE_OFF;
    settings.ssaoQuality=0; // the tape calibration image must reach VHS unchanged
    windowResize(nullptr,width,height);
    display.prepare(width,height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,fboTex);
    const Image source=calibrationImage();
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,source.data());
    assert(present(0.f,0.f)==source && present(0.f,107.4f)==source);
    const Metrics off=measure(0.f), half=measure(.5f), full=measure(1.f);
    std::printf("VHS 0/50/100%%, grain RMS %.2f / %.2f / %.2f; bar contrast %.2f / %.2f / %.2f\n",
                off.grainRms,half.grainRms,full.grainRms,off.detailContrast,half.detailContrast,full.detailContrast);
    assert(off.grainRms==0 && off.detailContrast>115);
    assert(half.grainRms>0.5 && full.grainRms>half.grainRms*1.15);
    assert(half.detailContrast<off.detailContrast && full.detailContrast<0.9*half.detailContrast);
    const Metrics native=measure(1.f,height);
    assert(native.grainRms>0.5 && native.detailContrast>full.detailContrast);

    // Check the game's actual settings-to-uniform binding, using the manually
    // specified shader results as references for the same source and field.
    postFxCaptureScene(Mat4::persp(1.f,float(width)/height,.1f,320.f),Mat4(),nullptr,0);
    vhsTime=3.5f;
    const Image lowRaster=present(1.f,vhsTime,240);
    const Image nativeRaster=present(1.f,vhsTime,height);
    assert(lowRaster!=nativeRaster);
    for(int preset : {VHS_RES_240,VHS_RES_NATIVE}) {
        settings.vhsResolution=preset;
        presentScene(display.framebuffer,1.f,0.f);
        assert(readDisplay()==(preset==VHS_RES_240 ? lowRaster : nativeRaster));
        presentScene(display.framebuffer,0.f,0.f);
        assert(readDisplay()==source);
    }
    glfwTerminate();
}
