// Dev tool: renders the heist set pieces (drill, kiosk, portal, pallet, loot...) off-screen
// with the real main shader and decor atlas, one tile per model/state.
//
// Build (from repo root):
//   g++ -std=c++17 -O2 -I"deps/include" -o build/heist_preview.exe tools/heist_preview.cpp deps/src/glad.c
//       -L"deps/lib" -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
// Run:  build/heist_preview.exe [outFile]     (default build/heist_shots.bmp)
#define _USE_MATH_DEFINES
#include "preview_common.h"

#include "../src/shaders.h"
#include "../src/progression.h"
#include "../src/decor_builder.h"
#include "../src/decor_atlas.h"

const float CS = 5.0f;
const float WH = 4.5f;
#include "../src/heist_models.h"
#include "../src/rig_models.h"

// The rig is longer than a tile is wide: shown at 55%, three quarters from behind / ahead.
static void buildRigTile(std::vector<float>& v, float yaw, int lamps, bool ready, int fault, bool power) {
    DecorBuilder b(v, 0.0f, 0.0f, yaw, 0.55f);
    RigLook look;
    look.lamps = lamps;
    look.ready = ready;
    look.fault = fault;
    look.power = power;
    look.charged = ready;
    buildRigHullModel(b, look);
}

static const int TILE_COUNT = 24;

static void buildTile(int tile, std::vector<float>& v) {
    DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
    if (tile == 0) buildRigTile(v, 3.6f, 0, false, RIG_OK, true);
    else if (tile == 1) buildRigTile(v, 0.5f, 3, false, RIG_OK, true);
    else if (tile == 2) buildRigTile(v, 1.2f, 5, true, RIG_FAULT_BELT, true);
    else if (tile == 3) buildRigTile(v, -1.2f, 5, true, RIG_FAULT_VALVE, false);
    else if (tile == 4) buildKioskModel(b, false);
    else if (tile == 5) buildKioskModel(b, true);
    else if (tile == 6) buildPortalRigModel(b);
    else if (tile == 7) { DecorBuilder w(v, 0.0f, -CS * 0.5f, 0.0f, 1.0f); buildPortalWreckModel(w); }
    else if (tile == 8) buildPalletPadModel(b, 0, 3);
    else if (tile == 9) buildPalletPadModel(b, 2, 3);
    else if (tile == 10) buildPitchBoardModel(b);
    else if (tile == 11) buildCrackFaceModel(b, 12345u, false);
    else if (tile == 12) buildCrackFaceModel(b, 777u, true);
    else if (tile == 13) buildStashMesh(b);
    else if (tile >= 14 && tile < 14 + LOOT_TYPE_COUNT) buildLootMesh(b, tile - 14);
}

static const float ROOM = 6.0f;

static GLuint surfaceTexture(int r, int g, int b) {
    std::vector<unsigned char> d(64 * 64 * 4);
    for (int i = 0; i < 64 * 64; i++) {
        int n = (int)(perlin((float)(i % 64) * 0.4f, (float)(i / 64) * 0.4f, 3) * 10.0f);
        d[i * 4] = (unsigned char)(r + n);
        d[i * 4 + 1] = (unsigned char)(g + n);
        d[i * 4 + 2] = (unsigned char)(b + n);
        d[i * 4 + 3] = 128;
    }
    return makeTexture(d.data(), 64, 64);
}

int main(int argc, char** argv) {
    std::string outFile = argc > 1 ? argv[1] : "build/heist_shots.bmp";
    if (!glfwInit()) { fprintf(stderr, "glfw init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* win = glfwCreateWindow(320, 240, "objective preview", nullptr, nullptr);
    if (!win) { fprintf(stderr, "window failed\n"); return 1; }
    glfwMakeContextCurrent(win);
    if (!gladLoadGL()) { fprintf(stderr, "glad failed\n"); return 1; }

    const int TILE = 300, COLS = 6, ROWS = TILE_COUNT / COLS;
    const int W = TILE * COLS, H = TILE * ROWS;
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

    std::vector<float> floorV, wallV;
    pushQuad(floorV, Vec3(-ROOM, 0, ROOM), Vec3(ROOM, 0, ROOM), Vec3(ROOM, 0, -ROOM), Vec3(-ROOM, 0, -ROOM), Vec3(0, 1, 0));
    pushQuad(wallV, Vec3(-ROOM, 0, -ROOM), Vec3(ROOM, 0, -ROOM), Vec3(ROOM, WH, -ROOM), Vec3(-ROOM, WH, -ROOM), Vec3(0, 0, 1));
    pushQuad(wallV, Vec3(-ROOM, 0, ROOM), Vec3(-ROOM, 0, -ROOM), Vec3(-ROOM, WH, -ROOM), Vec3(-ROOM, WH, ROOM), Vec3(1, 0, 0));
    GpuMesh floorG = upload(floorV), wallG = upload(wallV);
    GLuint floorTex = surfaceTexture(128, 112, 70);
    GLuint wallTex = surfaceTexture(186, 168, 96);
    GLuint decorTex = genDecorAtlasTex();

    glViewport(0, 0, W, H);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int kind = 0; kind < ROWS; kind++) {
        for (int state = 0; state < COLS; state++) {
            std::vector<float> v;
            buildTile(kind * COLS + state, v);
            GpuMesh mesh = upload(v);
            // frame the machine itself, not the conduit running up to the ceiling
            Vec3 lo(1e9f, 1e9f, 1e9f), hi(-1e9f, -1e9f, -1e9f);
            for (size_t i = 0; i < v.size(); i += 8) {
                float y = fminf(v[i + 1], 3.3f);
                lo = Vec3(fminf(lo.x, v[i]), fminf(lo.y, y), fminf(lo.z, v[i + 2]));
                hi = Vec3(fmaxf(hi.x, v[i]), fmaxf(hi.y, y), fmaxf(hi.z, v[i + 2]));
            }
            Vec3 target = (lo + hi) * 0.5f;
            float size = fmaxf(hi.x - lo.x, fmaxf(hi.y - lo.y, hi.z - lo.z));
            float dist = 1.1f + size * 1.15f;
            const float orbit = 0.62f;
            Vec3 eye(target.x + sinf(orbit) * dist, target.y + 0.45f + dist * 0.22f, target.z + cosf(orbit) * dist);
            Vec3 fwd = (target - eye).norm();
            Mat4 proj = Mat4::persp(0.95f, 1.0f, 0.05f, 60.0f);
            Mat4 view = Mat4::look(eye, target, Vec3(0, 1, 0));
            Mat4 ident;
            glViewport(state * TILE, (ROWS - 1 - kind) * TILE, TILE, TILE);
            glUniformMatrix4fv(glGetUniformLocation(prog, "P"), 1, GL_FALSE, proj.m);
            glUniformMatrix4fv(glGetUniformLocation(prog, "V"), 1, GL_FALSE, view.m);
            glUniformMatrix4fv(glGetUniformLocation(prog, "M"), 1, GL_FALSE, ident.m);
            glUniform3f(glGetUniformLocation(prog, "vp"), eye.x, eye.y, eye.z);
            glUniform1f(glGetUniformLocation(prog, "tm"), 1.0f);
            glUniform1f(glGetUniformLocation(prog, "danger"), 0.0f);
            glUniform1i(glGetUniformLocation(prog, "nl"), 2);
            const float lp[6] = { 1.5f, WH - 0.1f, 2.0f, -2.5f, WH - 0.1f, -3.0f };
            glUniform3fv(glGetUniformLocation(prog, "lp"), 2, lp);
            glUniform1i(glGetUniformLocation(prog, "flashOn"), 1);
            glUniform3f(glGetUniformLocation(prog, "flashPos"), eye.x, eye.y, eye.z);
            glUniform3f(glGetUniformLocation(prog, "flashDir"), fwd.x, fwd.y, fwd.z);
            glUniform1i(glGetUniformLocation(prog, "rfc"), 0);
            glUniform1i(glGetUniformLocation(prog, "tex"), 0);
            glUniform3f(glGetUniformLocation(prog, "modelTint"), 1, 1, 1);

            glEnable(GL_CULL_FACE);
            glBindTexture(GL_TEXTURE_2D, floorTex);
            glBindVertexArray(floorG.vao);
            glDrawArrays(GL_TRIANGLES, 0, floorG.count);
            glBindTexture(GL_TEXTURE_2D, wallTex);
            glBindVertexArray(wallG.vao);
            glDrawArrays(GL_TRIANGLES, 0, wallG.count);
            // decor is drawn two-sided, exactly like in the game
            glDisable(GL_CULL_FACE);
            glBindTexture(GL_TEXTURE_2D, decorTex);
            glBindVertexArray(mesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, mesh.count);
        }
    }
    std::vector<unsigned char> px((size_t)W * H * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    writeBmp(outFile, W, H, px);
    printf("wrote %s (see buildTile for the order)\n", outFile.c_str());
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
