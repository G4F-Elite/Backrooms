// Shared by the preview tools: off-screen GL helpers and a BMP writer.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// The bundled glad is trimmed to what the game uses; read-back comes straight from opengl32.
extern "C" {
__declspec(dllimport) void __stdcall glReadPixels(int x, int y, int w, int h, unsigned int format, unsigned int type, void* data);
__declspec(dllimport) void __stdcall glPixelStorei(unsigned int name, int value);
}
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif

#include "../src/math.h"

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        fprintf(stderr, "shader error: %s\n", log);
        exit(2);
    }
    return s;
}

static GLuint makeTexture(const unsigned char* data, int w, int h) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return t;
}

struct GpuMesh {
    GLuint vao = 0;
    int count = 0;
};

static GpuMesh upload(const std::vector<float>& v) {
    GpuMesh g;
    GLuint vbo = 0;
    glGenVertexArrays(1, &g.vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(g.vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, (void*)12);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, (void*)20);
    glEnableVertexAttribArray(2);
    g.count = (int)v.size() / 8;
    return g;
}

static void pushQuad(std::vector<float>& v, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n) {
    Vec3 p[6] = { a, b, c, a, c, d };
    float uv[6][2] = { {0, 0}, {4, 0}, {4, 4}, {0, 0}, {4, 4}, {0, 4} };
    for (int i = 0; i < 6; i++) {
        v.push_back(p[i].x); v.push_back(p[i].y); v.push_back(p[i].z);
        v.push_back(uv[i][0]); v.push_back(uv[i][1]);
        v.push_back(n.x); v.push_back(n.y); v.push_back(n.z);
    }
}

static void writeBmp(const std::string& path, int w, int h, const std::vector<unsigned char>& rgb) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path.c_str()); return; }
    int row = (w * 3 + 3) & ~3;
    int size = 54 + row * h;
    unsigned char hd[54] = { 'B', 'M' };
    hd[2] = (unsigned char)size; hd[3] = (unsigned char)(size >> 8); hd[4] = (unsigned char)(size >> 16); hd[5] = (unsigned char)(size >> 24);
    hd[10] = 54; hd[14] = 40;
    hd[18] = (unsigned char)w; hd[19] = (unsigned char)(w >> 8);
    hd[22] = (unsigned char)h; hd[23] = (unsigned char)(h >> 8);
    hd[26] = 1; hd[28] = 24;
    fwrite(hd, 1, 54, f);
    std::vector<unsigned char> line((size_t)row, 0);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            line[x * 3] = rgb[(y * w + x) * 3 + 2];
            line[x * 3 + 1] = rgb[(y * w + x) * 3 + 1];
            line[x * 3 + 2] = rgb[(y * w + x) * 3];
        }
        fwrite(line.data(), 1, (size_t)row, f);
    }
    fclose(f);
}
