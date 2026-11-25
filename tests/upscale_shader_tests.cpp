#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "../src/upscale_shader.h"
#include "../src/upscaler_settings.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
using Image = std::vector<unsigned char>;
constexpr int inputSize = 32, outputSize = 95;
using ReadPixels = void (APIENTRY*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
GLuint program;

GLuint compile(GLenum kind, const char* text) {
    GLuint shader = glCreateShader(kind);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char error[4096];
        glGetShaderInfoLog(shader, sizeof error, nullptr, error);
        std::fprintf(stderr, "%s\n", error);
    }
    assert(ok);
    return shader;
}

void init() {
    assert(glfwInit());
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(128, 128, "upscale test", nullptr, nullptr);
    assert(window);
    glfwMakeContextCurrent(window);
    assert(gladLoadGL());
    const char* vertex = R"(#version 330
out vec2 uv;
void main(){
    vec2 p=vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv=p;
    gl_Position=vec4(p*2.0-1.0,0,1);
})";
    program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, vertex));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, upscaleResolveFS));
    glLinkProgram(program);
    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    assert(ok);
    glUseProgram(program);
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glViewport(0, 0, outputSize, outputSize);
    glUniform1f(glGetUniformLocation(program, "texelX"), 1.f/inputSize);
    glUniform1f(glGetUniformLocation(program, "texelY"), 1.f/inputSize);
    glUniform1f(glGetUniformLocation(program, "sharpness"), .35f);
    glUniform1f(glGetUniformLocation(program, "taaBlend"), .85f);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    glUniform1i(glGetUniformLocation(program, "histTex"), 1);
    glUniform1i(glGetUniformLocation(program, "depthTex"), 2);
}

void bindTexture(unsigned unit, int size, const Image& values) {
    Image rgba(size * size * 4);
    for (size_t i=0; i<values.size(); ++i)
        std::fill_n(rgba.begin()+i*4, 4, values[i]);
    GLuint id;
    glGenTextures(1, &id);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
}

Image render(int mode, int aa = AA_MODE_OFF, bool history = false, float shift = 0.f, float shiftY = 0.f) {
    glUniform1i(glGetUniformLocation(program, "upscaler"), mode);
    glUniform1i(glGetUniformLocation(program, "aaMode"), aa);
    glUniform1f(glGetUniformLocation(program, "taaValid"), history ? 1.f : 0.f);
    const float transform[] = {1,0,0,0, 0,1,0,0, 0,0,1,0, shift,shiftY,0,1};
    glUniformMatrix4fv(glGetUniformLocation(program, "previousFromCurrentClip"), 1, GL_FALSE, transform);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    static auto read = reinterpret_cast<ReadPixels>(glfwGetProcAddress("glReadPixels"));
    assert(read);
    Image rgba(outputSize * outputSize * 4), output(outputSize * outputSize);
    read(0,0,outputSize,outputSize,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    for (size_t i=0; i<output.size(); ++i) output[i]=rgba[i*4];
    return output;
}

// Integrate a continuous diagonal edge over each input/output pixel.
Image diagonal(int size) {
    Image values(size*size);
    for (int y=0; y<size; ++y) for (int x=0; x<size; ++x) {
        int inside=0;
        for (int sy=0; sy<8; ++sy) for (int sx=0; sx<8; ++sx)
            inside += y+(sy+.5)/8 > (x+(sx+.5)/8)*.64 + size*.18;
        values[y*size+x] = static_cast<unsigned char>(32 + 192*inside/64);
    }
    return values;
}

double mse(const Image& output, const Image& reference) {
    double sum=0;
    for (size_t i=0; i<output.size(); ++i) {
        const double error = output[i]-reference[i];
        sum += error*error;
    }
    return sum/output.size();
}

// Render a plane whose projected brightness is linear. A translated camera
// and subpixel raster jitter move it by known amounts on each axis.
Image plane(float rasterX, float rasterY) {
    Image result(inputSize*inputSize);
    for (int y=0;y<inputSize;++y) for (int x=0;x<inputSize;++x)
        result[y*inputSize+x] = static_cast<unsigned char>(std::lround(38 + 4*(x-rasterX) + 2*(y-rasterY)));
    return result;
}

double interiorMse(const Image& a, const Image& b) {
    double sum=0;
    int count=0;
    for (int y=9;y<outputSize-9;++y) for (int x=9;x<outputSize-9;++x) {
        const int d = a[y*outputSize+x]-b[y*outputSize+x];
        sum+=d*d;
        ++count;
    }
    return sum/count;
}

void checkStableDisplayGrid() {
    const GLint cur = glGetUniformLocation(program, "sampleJitter");
    const GLint prev = glGetUniformLocation(program, "historyJitter");
    assert(cur >= 0 && prev >= 0);
    constexpr float jx=.375f, jy=-.25f;
    for (int mode : {UPSCALER_MODE_OFF, UPSCALER_MODE_FSR20}) {
        const int aa = mode == UPSCALER_MODE_OFF ? AA_MODE_TAA : AA_MODE_OFF;
        glUniform3f(cur,0,0,0);
        bindTexture(0,inputSize,plane(0,0));
        const Image stable = render(mode,aa);
        for (int cameraPixels : {0,2}) {
            // First frame has opposite jitter and sees this point cameraPixels
            // to the right. Its stored image uses unjittered output coordinates.
            bindTexture(0,inputSize,plane(cameraPixels-jx,-jy));
            glUniform3f(cur,-jx/inputSize,-jy/inputSize,0);
            const Image old = render(mode,aa);
            bindTexture(1,outputSize,old);
            bindTexture(0,inputSize,plane(jx,jy));
            glUniform3f(cur,jx/inputSize,jy/inputSize,0);
            glUniform3f(prev,-jx/inputSize,-jy/inputSize,0);
            const float dx=2*(cameraPixels-2*jx)/inputSize, dy=-4*jy/inputSize;
            const Image first = render(mode,aa);
            const Image tracked = render(mode,aa,true,dx,dy);
            glUniform3f(prev,0,0,0);
            const Image drifting = render(mode,aa,true,dx,dy);
            glUniform3f(cur,0,0,0);
            const Image uncompensated = render(mode,aa);
            assert(interiorMse(first,stable) < interiorMse(uncompensated,stable));
            assert(interiorMse(tracked,stable) < interiorMse(drifting,stable));
        }
    }
}
}

int main(){
    init();
    bindTexture(2, inputSize, Image(inputSize*inputSize, 128));
    bindTexture(1, outputSize, Image(outputSize*outputSize, 255));
    bindTexture(0, inputSize, Image(inputSize*inputSize, 86));
    for (int mode=0; mode<UPSCALER_MODE_COUNT; ++mode) {
        const auto flat = render(mode, AA_MODE_TAA, true);
        for (auto v: flat) assert(v==86);
    }

    const Image input = diagonal(inputSize), reference = diagonal(outputSize);
    bindTexture(0, inputSize, input);
    const auto nearest = render(UPSCALER_MODE_NEAREST);
    for (int y=0; y<outputSize; ++y) for (int x=0; x<outputSize; ++x) {
        const int sourceX = (2*x+1)*inputSize/(2*outputSize);
        const int sourceY = (2*y+1)*inputSize/(2*outputSize);
        // A sample exactly on a texel boundary is equidistant. Interpolated
        // vertex UVs can round to either side of that boundary.
        const int xTie = ((2*x+1)*inputSize) % (2*outputSize) == 0;
        const int yTie = ((2*y+1)*inputSize) % (2*outputSize) == 0;
        bool exactTexel = false;
        for (int dy=0; dy<=yTie; ++dy) for (int dx=0; dx<=xTie; ++dx)
            exactTexel |= nearest[y*outputSize+x] == input[(sourceY-dy)*inputSize+sourceX-dx];
        assert(exactTexel);
    }
    const auto bilinear = render(UPSCALER_MODE_OFF);
    const auto spatial = render(UPSCALER_MODE_FSR10);
    const auto temporal = render(UPSCALER_MODE_FSR20);
    assert(spatial == temporal);
    for (auto v: spatial) assert(v>=32 && v<=224);
    assert(render(UPSCALER_MODE_FSR20, AA_MODE_TAA, true, 3.f) == spatial);
    std::printf("Diagonal MSE: bilinear %.3f, nearest %.3f, EASU %.3f\n",
        mse(bilinear,reference),mse(nearest,reference),mse(spatial,reference));
    assert(mse(spatial, reference) < mse(bilinear, reference));
    Image history = spatial;
    for (auto& v: history) v = static_cast<unsigned char>(std::min(255, int(v)+6));
    bindTexture(1, outputSize, history);
    const auto accumulated = render(UPSCALER_MODE_FSR20, AA_MODE_OFF, true);
    assert(accumulated != spatial);
    assert(render(UPSCALER_MODE_FSR20, AA_MODE_TAA, true) == accumulated);
    assert(render(UPSCALER_MODE_FSR20, AA_MODE_OFF, true, 3.f) == spatial);
    // Move last frame's edge by three display pixels. The projection should
    // find matching history at its old location; identity samples the wrong edge.
    constexpr int shift = 3;
    for (int y=0; y<outputSize; ++y) for (int x=0; x<outputSize; ++x)
        history[y*outputSize+x] = spatial[y*outputSize+std::max(0, x-shift)];
    bindTexture(1, outputSize, history);
    const auto tracked = render(UPSCALER_MODE_FSR20, AA_MODE_OFF, true, 2.f*shift/outputSize);
    const auto untracked = render(UPSCALER_MODE_FSR20, AA_MODE_OFF, true);
    assert(mse(tracked, spatial) < 0.01 && mse(tracked, spatial) < mse(untracked, spatial));
    glUniform1f(glGetUniformLocation(program, "sharpness"), 0.f);
    assert(render(UPSCALER_MODE_FSR10) != spatial);
    checkStableDisplayGrid();
    glfwTerminate();
}
