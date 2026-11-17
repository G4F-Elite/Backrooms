// Dev tool: renders the rig (drill + break room) off-screen with the real main shader and
// decor atlas: walk-around views of the outside, then the inside as a player sees it from
// the doorway and at each station, in every state that changes the mesh.
//
// Build (from repo root):
//   g++ -std=c++17 -O2 -I"deps/include" -o build/rig_preview.exe tools/rig_preview.cpp deps/src/glad.c
//       -L"deps/lib" -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
// Run:  build/rig_preview.exe [outFile]     (default build/rig_shots.bmp)
#define _USE_MATH_DEFINES
#include "preview_common.h"

#include "../src/shaders.h"
#include "../src/progression.h"
#include "../src/decor_builder.h"
#include "../src/decor_atlas.h"

const float CS = 5.0f;
const float WH = 4.5f;
#include "../src/rig_models.h"

struct RigView {
    Vec3 eye, target;
    int lamps;
    bool ready, pulled;
    int fault;
    bool power, charged;
};

static const float EYE = 1.7f; // player eye height

static const RigView VIEWS[] = {
    // outside
    {Vec3(-4.2f, 2.3f, -8.2f), Vec3(0.0f, 1.2f, -1.0f), 0, false, false, RIG_OK, true, false},
    {Vec3(4.6f, 2.2f, 6.8f), Vec3(0.0f, 1.2f, 0.4f), 2, false, false, RIG_OK, true, false},
    {Vec3(-7.6f, 1.9f, -0.4f), Vec3(0.0f, 1.2f, -0.4f), 2, false, false, RIG_OK, true, false},
    {Vec3(-3.4f, EYE, 2.6f), Vec3(-1.3f, 1.0f, 0.85f), 2, false, false, RIG_FAULT_BELT, true, false},
    {Vec3(3.4f, EYE, 2.6f), Vec3(1.3f, 1.0f, 0.85f), 2, false, false, RIG_FAULT_VALVE, false, false},
    {Vec3(0.3f, EYE, -6.6f), Vec3(0.0f, 1.3f, -3.9f), 0, false, false, RIG_OK, true, false},
    // inside
    {Vec3(0.0f, EYE, -3.7f), Vec3(-0.15f, 1.25f, 0.1f), 0, false, false, RIG_OK, true, false},
    {Vec3(0.0f, EYE, -1.6f), Vec3(-0.15f, 1.30f, 0.1f), 3, false, false, RIG_OK, true, false},
    {Vec3(-0.1f, EYE, -1.6f), Vec3(-0.15f, 1.30f, 0.1f), 5, true, false, RIG_OK, true, true},
    {Vec3(-0.1f, EYE, -1.6f), Vec3(-0.15f, 1.30f, 0.1f), 5, true, true, RIG_OK, true, true},
    {Vec3(-0.5f, EYE, -1.0f), Vec3(1.2f, 1.2f, -0.55f), 3, false, false, RIG_OK, true, true},
    {Vec3(-0.2f, EYE, -2.4f), Vec3(1.2f, 1.25f, -3.35f), 3, false, false, RIG_FAULT_FUSE, true, false},
};

static const int VIEW_COUNT = (int)(sizeof(VIEWS) / sizeof(VIEWS[0]));

// The auger is built standing on its base; lay it along +Z on the gearbox, like rigAugerMatrix.
static void appendAuger(std::vector<float>& v) {
    std::vector<float> a;
    DecorBuilder b(a, 0.0f, 0.0f, 0.0f, 1.0f);
    buildRigAugerModel(b);
    for (size_t i = 0; i < a.size(); i += 8) {
        float x = a[i], y = a[i + 1], z = a[i + 2];
        float nx = a[i + 5], ny = a[i + 6], nz = a[i + 7];
        a[i] = x; a[i + 1] = RIG_AUGER_HEIGHT - z; a[i + 2] = RIG_AUGER_BASE + y;
        a[i + 5] = nx; a[i + 6] = -nz; a[i + 7] = ny;
    }
    v.insert(v.end(), a.begin(), a.end());
}

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
    std::string outFile = argc > 1 ? argv[1] : "build/rig_shots.bmp";
    if (!glfwInit()) { fprintf(stderr, "glfw init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* win = glfwCreateWindow(320, 240, "rig preview", nullptr, nullptr);
    if (!win) { fprintf(stderr, "window failed\n"); return 1; }
    glfwMakeContextCurrent(win);
    if (!gladLoadGL()) { fprintf(stderr, "glad failed\n"); return 1; }

    const int TILE_W = 640, TILE_H = 480, COLS = 3, ROWS = (VIEW_COUNT + COLS - 1) / COLS;
    const int W = TILE_W * COLS, H = TILE_H * ROWS;
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

    const float room = 14.0f;
    std::vector<float> floorV, wallV;
    pushQuad(floorV, Vec3(-room, 0, room), Vec3(room, 0, room), Vec3(room, 0, -room), Vec3(-room, 0, -room), Vec3(0, 1, 0));
    pushQuad(wallV, Vec3(-room, 0, -room), Vec3(room, 0, -room), Vec3(room, WH, -room), Vec3(-room, WH, -room), Vec3(0, 0, 1));
    pushQuad(wallV, Vec3(room, 0, room), Vec3(-room, 0, room), Vec3(-room, WH, room), Vec3(room, WH, room), Vec3(0, 0, -1));
    pushQuad(wallV, Vec3(-room, 0, room), Vec3(-room, 0, -room), Vec3(-room, WH, -room), Vec3(-room, WH, room), Vec3(1, 0, 0));
    pushQuad(wallV, Vec3(room, 0, -room), Vec3(room, 0, room), Vec3(room, WH, room), Vec3(room, WH, -room), Vec3(-1, 0, 0));
    GpuMesh floorG = upload(floorV), wallG = upload(wallV);
    GLuint floorTex = surfaceTexture(128, 112, 70);
    GLuint wallTex = surfaceTexture(186, 168, 96);
    GLuint decorTex = genDecorAtlasTex();

    glViewport(0, 0, W, H);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int i = 0; i < VIEW_COUNT; i++) {
        const RigView& view = VIEWS[i];
        RigLook look;
        look.lamps = view.lamps;
        look.ready = view.ready;
        look.pulled = view.pulled;
        look.fault = view.fault;
        look.power = view.power;
        look.charged = view.charged;
        std::vector<float> v;
        DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
        buildRigHullModel(b, look);
        appendAuger(v);
        GpuMesh mesh = upload(v);

        Vec3 fwd = (view.target - view.eye).norm();
        Mat4 proj = Mat4::persp(1.25f, (float)TILE_W / (float)TILE_H, 0.05f, 80.0f);
        Mat4 viewM = Mat4::look(view.eye, view.target, Vec3(0, 1, 0));
        Mat4 ident;
        glViewport((i % COLS) * TILE_W, (ROWS - 1 - i / COLS) * TILE_H, TILE_W, TILE_H);
        glUniformMatrix4fv(glGetUniformLocation(prog, "P"), 1, GL_FALSE, proj.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "V"), 1, GL_FALSE, viewM.m);
        glUniformMatrix4fv(glGetUniformLocation(prog, "M"), 1, GL_FALSE, ident.m);
        glUniform3f(glGetUniformLocation(prog, "vp"), view.eye.x, view.eye.y, view.eye.z);
        glUniform1f(glGetUniformLocation(prog, "tm"), 1.0f);
        glUniform1f(glGetUniformLocation(prog, "danger"), 0.0f);
        // ceiling lights of the hall around the rig, plus the break room lamp while it has power
        float lp[18] = { 4.0f, WH - 0.1f, 5.0f, -4.5f, WH - 0.1f, -1.0f, 4.0f, WH - 0.1f, -4.0f,
                         -3.0f, WH - 0.1f, -8.0f, -3.0f, WH - 0.1f, 6.0f,
                         0.0f, RIG_CABIN_H - 0.25f, RIG_HAVEN_Z };
        glUniform1i(glGetUniformLocation(prog, "nl"), view.power ? 6 : 5);
        glUniform3fv(glGetUniformLocation(prog, "lp"), 6, lp);
        glUniform1i(glGetUniformLocation(prog, "flashOn"), 1);
        glUniform3f(glGetUniformLocation(prog, "flashPos"), view.eye.x, view.eye.y, view.eye.z);
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
        // decor is drawn two-sided, exactly like in the game; the rig gets its warm lamp tint
        glDisable(GL_CULL_FACE);
        glBindTexture(GL_TEXTURE_2D, decorTex);
        if (view.power) glUniform3f(glGetUniformLocation(prog, "modelTint"), 1.14f, 1.10f, 1.00f);
        else glUniform3f(glGetUniformLocation(prog, "modelTint"), 0.50f, 0.47f, 0.47f);
        glBindVertexArray(mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh.count);
    }
    std::vector<unsigned char> px((size_t)W * H * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    writeBmp(outFile, W, H, px);
    printf("wrote %s (%d views, see VIEWS for the order)\n", outFile.c_str(), VIEW_COUNT);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
