// Dev tool: renders the creature/player models off-screen with the real main shader
// and writes contact sheets (BMP) so model changes can be reviewed without playing.
//
// Build (from repo root):
//   g++ -std=c++17 -O2 -I"deps/include" -o build/model_preview.exe tools/model_preview.cpp deps/src/glad.c
//       -L"deps/lib" -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
// Run:  build/model_preview.exe [outDir]     (default build/model_shots)
#define _USE_MATH_DEFINES
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
#include "../src/shaders.h"
#include "../src/entity_types.h"
#include "../src/entity_model.h"
#include "../src/creature_anim.h"
#include "../src/net_types.h"

// player_model.h expects the game's network manager; the preview only needs its mesh/texture builders.
struct PreviewNetStub { NetPlayer players[MAX_PLAYERS]; };
static PreviewNetStub netMgr;
#include "../src/player_model.h"

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

static GLuint flatTexture(int r, int g, int b) {
    std::vector<unsigned char> d(64 * 64 * 4);
    for (int i = 0; i < 64 * 64; i++) {
        int n = (int)(creatureHash(i % 64, i / 64, 7u) * 18.0f) - 9;
        d[i * 4] = (unsigned char)(r + n);
        d[i * 4 + 1] = (unsigned char)(g + n);
        d[i * 4 + 2] = (unsigned char)(b + n);
        d[i * 4 + 3] = 255;
    }
    return makeTexture(d.data(), 64, 64);
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

struct Shot {
    float orbit;      // camera angle around the model, 0 = straight in front
    CreaturePose pose;
};

struct Subject {
    const char* name;
    CreatureMesh mesh;
    GpuMesh gpu;
    GLuint tex;
    Vec3 tint;
    float lookY, camDist, camY;
};

int main(int argc, char** argv) {
    std::string outDir = argc > 1 ? argv[1] : "build/model_shots";
    if (!glfwInit()) { fprintf(stderr, "glfw init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* win = glfwCreateWindow(320, 240, "model preview", nullptr, nullptr);
    if (!win) { fprintf(stderr, "window failed\n"); return 1; }
    glfwMakeContextCurrent(win);
    if (!gladLoadGL()) { fprintf(stderr, "glad failed\n"); return 1; }

    const int TILE_W = 420, TILE_H = 640, COLS = 4;
    const int W = TILE_W * COLS, H = TILE_H;
    GLuint fbo = 0, color = 0, depth = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, compile(GL_VERTEX_SHADER, mainVS));
    glAttachShader(prog, compile(GL_FRAGMENT_SHADER, mainFS));
    glLinkProgram(prog);
    glUseProgram(prog);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // A small yellow room: carpet, wallpaper, ceiling.
    const float R = 6.0f, CEIL = 4.5f;
    std::vector<float> floorV, wallV;
    pushQuad(floorV, Vec3(-R, 0, R), Vec3(R, 0, R), Vec3(R, 0, -R), Vec3(-R, 0, -R), Vec3(0, 1, 0));
    pushQuad(wallV, Vec3(-R, CEIL, -R), Vec3(R, CEIL, -R), Vec3(R, CEIL, R), Vec3(-R, CEIL, R), Vec3(0, -1, 0));
    pushQuad(wallV, Vec3(-R, 0, -R), Vec3(R, 0, -R), Vec3(R, CEIL, -R), Vec3(-R, CEIL, -R), Vec3(0, 0, 1));
    pushQuad(wallV, Vec3(R, 0, R), Vec3(-R, 0, R), Vec3(-R, CEIL, R), Vec3(R, CEIL, R), Vec3(0, 0, -1));
    pushQuad(wallV, Vec3(-R, 0, R), Vec3(-R, 0, -R), Vec3(-R, CEIL, -R), Vec3(-R, CEIL, R), Vec3(1, 0, 0));
    pushQuad(wallV, Vec3(R, 0, -R), Vec3(R, 0, R), Vec3(R, CEIL, R), Vec3(R, CEIL, -R), Vec3(-1, 0, 0));
    GpuMesh floorG = upload(floorV), wallG = upload(wallV);
    GLuint floorTex = flatTexture(128, 112, 70), wallTex = flatTexture(186, 168, 96);

    std::vector<Subject> subjects(4);
    const EntityType types[3] = { ENTITY_STALKER, ENTITY_CRAWLER, ENTITY_SHADOW };
    const char* names[4] = { "stalker", "crawler", "shadow", "player" };
    for (int i = 0; i < 4; i++) {
        Subject& s = subjects[i];
        s.name = names[i];
        std::vector<unsigned char> td(128 * 128 * 4);
        if (i < 3) {
            buildEntityModel(types[i], s.mesh);
            genEntityTextureByType(types[i], td.data(), 128, 128);
            s.tint = Vec3(1, 1, 1);
        } else {
            buildPlayerModel(s.mesh);
            genPlayerTexture(td.data(), 128, 128);
            s.tint = Vec3(PLAYER_COLORS[0][0], PLAYER_COLORS[0][1], PLAYER_COLORS[0][2]);
        }
        s.tex = makeTexture(td.data(), 128, 128);
        s.gpu = upload(s.mesh.v);
        s.lookY = 1.15f; s.camDist = 3.3f; s.camY = 1.6f;
        printf("%s: %d tris, %d parts\n", s.name, s.mesh.vertexCount() / 3, (int)s.mesh.parts.size());
    }
    subjects[1].lookY = 0.3f; subjects[1].camDist = 2.3f; subjects[1].camY = 1.3f;
    subjects[3].lookY = 0.95f; subjects[3].camDist = 2.7f;

    // Sheets: [0] walk cycle around the model, [1] special states, both lit by ceiling light + flashlight;
    // [2] ceiling light only (how it reads at a distance in a lit corridor).
    for (int si = 0; si < 4; si++) {
        Subject& s = subjects[si];
        for (int sheet = 0; sheet < 3; sheet++) {
            Shot shots[COLS];
            const float orbits[COLS] = { 0.0f, 0.75f, 1.5708f, 2.6f };
            for (int c = 0; c < COLS; c++) {
                float gait = (float)c * 1.1f + 0.6f;
                float t = (float)c * 3.1f;
                shots[c].orbit = orbits[c];
                if (si == 0) shots[c].pose = stalkerPose(gait, t, sheet == 1 && c >= 2 ? ENT_ATTACKING : ENT_STALKING);
                else if (si == 1) shots[c].pose = crawlerPose(gait, t, sheet == 1 ? (c >= 2 ? ENT_ATTACKING : ENT_IDLE) : ENT_CHASING);
                else if (si == 2) shots[c].pose = shadowPose(t, sheet == 1 && c >= 2 ? ENT_ATTACKING : ENT_STALKING);
                else {
                    if (sheet == 1) shots[c].pose = playerPose(gait, c == 1 ? 1.0f : 0.0f, c >= 2 ? 1.0f : 0.0f, c == 0 ? 0.5f : -0.2f, c != 3, t);
                    else shots[c].pose = playerPose(gait, 1.0f, 0.0f, 0.0f, c % 2 == 0, t);
                }
                if (sheet == 1) shots[c].orbit = (c % 2 == 0) ? 0.5f : 1.3f;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, W, H);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (int c = 0; c < COLS; c++) {
                glViewport(c * TILE_W, 0, TILE_W, TILE_H);
                float o = shots[c].orbit;
                Vec3 eye(sinf(o) * s.camDist, s.camY, cosf(o) * s.camDist);
                Vec3 target(0, s.lookY, 0);
                Mat4 proj = Mat4::persp(0.95f, (float)TILE_W / (float)TILE_H, 0.05f, 60.0f);
                Mat4 view = Mat4::look(eye, target, Vec3(0, 1, 0));
                Vec3 fwd = (target - eye).norm();
                glUniformMatrix4fv(glGetUniformLocation(prog, "P"), 1, GL_FALSE, proj.m);
                glUniformMatrix4fv(glGetUniformLocation(prog, "V"), 1, GL_FALSE, view.m);
                glUniform3f(glGetUniformLocation(prog, "vp"), eye.x, eye.y, eye.z);
                glUniform1f(glGetUniformLocation(prog, "tm"), 1.0f);
                glUniform1f(glGetUniformLocation(prog, "danger"), 0.0f);
                glUniform1i(glGetUniformLocation(prog, "nl"), 2);
                const float lp[6] = { 1.5f, CEIL - 0.1f, 2.0f, -3.0f, CEIL - 0.1f, -3.0f };
                glUniform3fv(glGetUniformLocation(prog, "lp"), 2, lp);
                glUniform1i(glGetUniformLocation(prog, "flashOn"), sheet == 2 ? 0 : 1);
                glUniform3f(glGetUniformLocation(prog, "flashPos"), eye.x, eye.y, eye.z);
                glUniform3f(glGetUniformLocation(prog, "flashDir"), fwd.x, fwd.y, fwd.z);
                glUniform1i(glGetUniformLocation(prog, "rfc"), 0);
                glUniform1i(glGetUniformLocation(prog, "tex"), 0);
                GLint tint = glGetUniformLocation(prog, "modelTint");
                GLint mLoc = glGetUniformLocation(prog, "M");

                Mat4 ident;
                glUniform3f(tint, 1, 1, 1);
                glUniformMatrix4fv(mLoc, 1, GL_FALSE, ident.m);
                glBindTexture(GL_TEXTURE_2D, floorTex);
                glBindVertexArray(floorG.vao);
                glDrawArrays(GL_TRIANGLES, 0, floorG.count);
                glBindTexture(GL_TEXTURE_2D, wallTex);
                glBindVertexArray(wallG.vao);
                glDrawArrays(GL_TRIANGLES, 0, wallG.count);

                Mat4 mats[CREATURE_MAX_PARTS];
                creaturePartMatrices(s.mesh.parts, shots[c].pose, creatureWorldMatrix(Vec3(0, 0, 0), 0.0f), mats);
                glUniform3f(tint, s.tint.x, s.tint.y, s.tint.z);
                glBindTexture(GL_TEXTURE_2D, s.tex);
                glBindVertexArray(s.gpu.vao);
                for (size_t k = 0; k < s.mesh.parts.size(); k++) {
                    glUniformMatrix4fv(mLoc, 1, GL_FALSE, mats[k].m);
                    glDrawArrays(GL_TRIANGLES, s.mesh.parts[k].first, s.mesh.parts[k].count);
                }
            }
            std::vector<unsigned char> px((size_t)W * H * 3);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
            const char* suffix[3] = { "walk", "states", "ambient" };
            std::string path = outDir + "/" + s.name + "_" + suffix[sheet] + ".bmp";
            writeBmp(path, W, H, px);
            printf("wrote %s\n", path.c_str());
        }
    }
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
