// Dev tool: renders every prop of the catalog off-screen with the real main shader and
// decor atlas, one contact sheet per theme, so new models can be reviewed without playing.
//
// Build (from repo root):
//   g++ -std=c++17 -O2 -I"deps/include" -o build/prop_preview.exe tools/prop_preview.cpp deps/src/glad.c
//       -L"deps/lib" -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
// Run:  build/prop_preview.exe [outDir] [level]     (default build/prop_shots, level 0)
//       level 0 = yellow rooms finish, 1 = parking finish (plaster follows the level)
#define _USE_MATH_DEFINES
#include <random>
#include <unordered_map>

#include "preview_common.h"

#include "../src/shaders.h"
#include "../src/world.h"
#include "../src/map_content.h"
#include "../src/prop_models.h"
#include "../src/decor_atlas.h"

const float CS = 5.0f;
const float WH = 4.5f;
std::mt19937 rng;
unsigned int worldSeed = 0;
std::unordered_map<long long, Chunk> chunks;
std::vector<Light> lights;
std::vector<Vec3> pillars;
std::vector<MapProp> mapProps;
std::vector<MapPoi> mapPois;
int playerChunkX = 0;
int playerChunkZ = 0;

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

struct PropShot {
    GpuMesh gpu;
    Vec3 target;
    float dist;
    bool lookUp;
};

// Wall props are built against the back wall of the preview room, the rest in its centre.
static PropShot buildShot(int type) {
    MapProp pr{};
    pr.type = type;
    pr.scale = 1.0f;
    pr.yaw = 0.0f;
    float hx = 0.0f, hz = 0.0f;
    mapPropBaseExtents(type, hx, hz);
    pr.pos = Vec3(0.0f, 0.0f, isMapPropWallType(type) ? (-ROOM + hz + 0.02f) : 0.0f);
    std::vector<float> v;
    buildMapPropModel(v, pr);

    Vec3 lo(1e9f, 1e9f, 1e9f), hi(-1e9f, -1e9f, -1e9f);
    for (size_t i = 0; i < v.size(); i += 8) {
        lo = Vec3(fminf(lo.x, v[i]), fminf(lo.y, v[i + 1]), fminf(lo.z, v[i + 2]));
        hi = Vec3(fmaxf(hi.x, v[i]), fmaxf(hi.y, v[i + 1]), fmaxf(hi.z, v[i + 2]));
    }
    PropShot s;
    s.gpu = upload(v);
    s.target = (lo + hi) * 0.5f;
    float size = fmaxf(hi.x - lo.x, fmaxf(hi.y - lo.y, hi.z - lo.z));
    s.dist = 1.1f + size * 1.15f;
    s.lookUp = isPropOverheadType(type);
    return s;
}

int main(int argc, char** argv) {
    std::string outDir = argc > 1 ? argv[1] : "build/prop_shots";
    gCurrentLevel = argc > 2 ? atoi(argv[2]) : 0;
    if (!glfwInit()) { fprintf(stderr, "glfw init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* win = glfwCreateWindow(320, 240, "prop preview", nullptr, nullptr);
    if (!win) { fprintf(stderr, "window failed\n"); return 1; }
    glfwMakeContextCurrent(win);
    if (!gladLoadGL()) { fprintf(stderr, "glad failed\n"); return 1; }

    // the sheet is as tall as the best-stocked theme needs
    const int TILE = 360, COLS = 6;
    int MAX_ROWS = 1;
    for (int t = 0; t < THEME_COUNT; t++) {
        int count = 0;
        for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
            if (PROP_CATALOG[type].themes & (1u << t)) count++;
        }
        if ((count + COLS - 1) / COLS > MAX_ROWS) MAX_ROWS = (count + COLS - 1) / COLS;
    }
    const int W = TILE * COLS, H = TILE * MAX_ROWS;
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
    pushQuad(wallV, Vec3(-ROOM, WH, -ROOM), Vec3(ROOM, WH, -ROOM), Vec3(ROOM, WH, ROOM), Vec3(-ROOM, WH, ROOM), Vec3(0, -1, 0));
    pushQuad(wallV, Vec3(-ROOM, 0, -ROOM), Vec3(ROOM, 0, -ROOM), Vec3(ROOM, WH, -ROOM), Vec3(-ROOM, WH, -ROOM), Vec3(0, 0, 1));
    pushQuad(wallV, Vec3(ROOM, 0, ROOM), Vec3(-ROOM, 0, ROOM), Vec3(-ROOM, WH, ROOM), Vec3(ROOM, WH, ROOM), Vec3(0, 0, -1));
    pushQuad(wallV, Vec3(-ROOM, 0, ROOM), Vec3(-ROOM, 0, -ROOM), Vec3(-ROOM, WH, -ROOM), Vec3(-ROOM, WH, ROOM), Vec3(1, 0, 0));
    pushQuad(wallV, Vec3(ROOM, 0, -ROOM), Vec3(ROOM, 0, ROOM), Vec3(ROOM, WH, ROOM), Vec3(ROOM, WH, -ROOM), Vec3(-1, 0, 0));
    GpuMesh floorG = upload(floorV), wallG = upload(wallV);
    bool parking = isParkingLevel(gCurrentLevel);
    GLuint floorTex = parking ? surfaceTexture(96, 98, 100) : surfaceTexture(128, 112, 70);
    GLuint wallTex = parking ? surfaceTexture(118, 120, 124) : surfaceTexture(186, 168, 96);
    GLuint decorTex = genDecorAtlasTex();

    std::vector<PropShot> shots(MAP_PROP_TYPE_COUNT);
    for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) shots[type] = buildShot(type);

    for (int t = 0; t < THEME_COUNT; t++) {
        unsigned theme = 1u << t;
        std::vector<int> types;
        for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
            if (PROP_CATALOG[type].themes & theme) types.push_back(type);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, W, H);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        printf("%s:\n", propThemeName(theme));
        for (size_t i = 0; i < types.size() && i < (size_t)(COLS * MAX_ROWS); i++) {
            int col = (int)i % COLS, row = (int)i / COLS;
            const PropShot& s = shots[types[i]];
            printf("  row %d col %d: %s\n", row + 1, col + 1, PROP_CATALOG[types[i]].name);
            glViewport(col * TILE, (MAX_ROWS - 1 - row) * TILE, TILE, TILE);
            const float orbit = 0.62f;
            Vec3 eye(s.target.x + sinf(orbit) * s.dist, s.lookUp ? 1.7f : s.target.y + 0.45f + s.dist * 0.22f,
                     s.target.z + cosf(orbit) * s.dist);
            Mat4 proj = Mat4::persp(0.95f, 1.0f, 0.05f, 60.0f);
            Mat4 view = Mat4::look(eye, s.target, Vec3(0, 1, 0));
            Vec3 fwd = (s.target - eye).norm();
            Mat4 ident;
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
            glBindVertexArray(s.gpu.vao);
            glDrawArrays(GL_TRIANGLES, 0, s.gpu.count);
        }
        std::vector<unsigned char> px((size_t)W * H * 3);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
        std::string path = outDir + "/" + propThemeName(theme) + ".bmp";
        writeBmp(path, W, H, px);
        printf("wrote %s\n", path.c_str());
    }
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
