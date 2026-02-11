#pragma once
// Player model for multiplayer - animated humanoid with flashlight and interpolation
#include <vector>
#include <glad/glad.h>
#include "math.h"
#include "interpolation.h"
#include "net_types.h"

// Multiplayer player models are drawn with the main shader, so we must ensure
// the correct texture is bound (otherwise the last-held item texture bleeds onto players).
extern GLuint playerTex;

const float PLAYER_COLORS[4][3] = {
    {0.1f, 0.9f, 0.2f},
    {0.2f, 0.6f, 1.0f},
    {1.0f, 0.5f, 0.1f},
    {0.9f, 0.2f, 0.8f}
};

inline void addColorBox(std::vector<float>& v, float x, float y, float z,
    float sx, float sy, float sz, float r, float g, float b) {
    (void)r; (void)g; (void)b;
    float hx = sx / 2, hy = sy / 2, hz = sz / 2;
    float faces[6][48] = {
        {x-hx,y-hy,z+hz,0,0,0,0,1, x+hx,y-hy,z+hz,1,0,0,0,1, x+hx,y+hy,z+hz,1,1,0,0,1,
         x-hx,y-hy,z+hz,0,0,0,0,1, x+hx,y+hy,z+hz,1,1,0,0,1, x-hx,y+hy,z+hz,0,1,0,0,1},
        {x+hx,y-hy,z-hz,0,0,0,0,-1, x-hx,y-hy,z-hz,1,0,0,0,-1, x-hx,y+hy,z-hz,1,1,0,0,-1,
         x+hx,y-hy,z-hz,0,0,0,0,-1, x-hx,y+hy,z-hz,1,1,0,0,-1, x+hx,y+hy,z-hz,0,1,0,0,-1},
        {x-hx,y-hy,z-hz,0,0,-1,0,0, x-hx,y-hy,z+hz,1,0,-1,0,0, x-hx,y+hy,z+hz,1,1,-1,0,0,
         x-hx,y-hy,z-hz,0,0,-1,0,0, x-hx,y+hy,z+hz,1,1,-1,0,0, x-hx,y+hy,z-hz,0,1,-1,0,0},
        {x+hx,y-hy,z+hz,0,0,1,0,0, x+hx,y-hy,z-hz,1,0,1,0,0, x+hx,y+hy,z-hz,1,1,1,0,0,
         x+hx,y-hy,z+hz,0,0,1,0,0, x+hx,y+hy,z-hz,1,1,1,0,0, x+hx,y+hy,z+hz,0,1,1,0,0},
        {x-hx,y+hy,z+hz,0,0,0,1,0, x+hx,y+hy,z+hz,1,0,0,1,0, x+hx,y+hy,z-hz,1,1,0,1,0,
         x-hx,y+hy,z+hz,0,0,0,1,0, x+hx,y+hy,z-hz,1,1,0,1,0, x-hx,y+hy,z-hz,0,1,0,1,0},
        {x-hx,y-hy,z-hz,0,0,0,-1,0, x+hx,y-hy,z-hz,1,0,0,-1,0, x+hx,y-hy,z+hz,1,1,0,-1,0,
         x-hx,y-hy,z-hz,0,0,0,-1,0, x+hx,y-hy,z+hz,1,1,0,-1,0, x-hx,y-hy,z+hz,0,1,0,-1,0}
    };
    for (int f = 0; f < 6; f++) for (int i = 0; i < 48; i++) v.push_back(faces[f][i]);
}

inline void buildPlayerModel(std::vector<float>& v, int colorId) {
    float r = PLAYER_COLORS[colorId][0];
    float g = PLAYER_COLORS[colorId][1];
    float b = PLAYER_COLORS[colorId][2];
    addColorBox(v, 0, 1.0f, 0, 0.45f, 0.55f, 0.22f, r, g, b);
    addColorBox(v, 0.41f, 0.80f, 0.16f, 0.20f, 0.06f, 0.06f, 0.08f, 0.08f, 0.09f);
    addColorBox(v, 0.49f, 0.80f, 0.16f, 0.08f, 0.07f, 0.07f, 0.14f, 0.14f, 0.15f);
    addColorBox(v, 0.54f, 0.80f, 0.16f, 0.03f, 0.05f, 0.05f, 0.92f, 0.92f, 0.78f);
    addColorBox(v, 0.37f, 0.72f, 0.16f, 0.05f, 0.16f, 0.05f, 0.09f, 0.09f, 0.10f);
    addColorBox(v, 0, 1.5f, 0, 0.28f, 0.28f, 0.28f, 0.85f, 0.75f, 0.65f);
    addColorBox(v, 0, 1.67f, 0, 0.30f, 0.08f, 0.30f, 0.15f, 0.1f, 0.05f);
    addColorBox(v, -0.06f, 1.52f, 0.14f, 0.05f, 0.03f, 0.02f, 0.1f, 0.1f, 0.1f);
    addColorBox(v, 0.06f, 1.52f, 0.14f, 0.05f, 0.03f, 0.02f, 0.1f, 0.1f, 0.1f);
    addColorBox(v, -0.1f, 0.35f, 0, 0.14f, 0.65f, 0.14f, 0.15f, 0.2f, 0.35f);
    addColorBox(v, 0.1f, 0.35f, 0, 0.14f, 0.65f, 0.14f, 0.15f, 0.2f, 0.35f);
    addColorBox(v, -0.1f, 0.05f, 0.02f, 0.14f, 0.1f, 0.18f, 0.1f, 0.1f, 0.1f);
    addColorBox(v, 0.1f, 0.05f, 0.02f, 0.14f, 0.1f, 0.18f, 0.1f, 0.1f, 0.1f);
    addColorBox(v, -0.32f, 1.0f, 0, 0.1f, 0.45f, 0.1f, r*0.8f, g*0.8f, b*0.8f);
    addColorBox(v, 0.32f, 1.0f, 0, 0.1f, 0.45f, 0.1f, r*0.8f, g*0.8f, b*0.8f);
    addColorBox(v, -0.32f, 0.72f, 0, 0.08f, 0.1f, 0.08f, 0.85f, 0.75f, 0.65f);
    addColorBox(v, 0.32f, 0.72f, 0, 0.08f, 0.1f, 0.08f, 0.85f, 0.75f, 0.65f);
    addColorBox(v, 0, 1.0f, -0.18f, 0.3f, 0.4f, 0.12f, 0.2f, 0.15f, 0.1f);
}

inline GLuint playerVAOs[MAX_PLAYERS] = {0};
inline GLuint playerVBOs[MAX_PLAYERS] = {0};
inline int playerVCs[MAX_PLAYERS] = {0};
inline float playerAnimPhase[MAX_PLAYERS] = {0};
inline Vec3 playerLastPos[MAX_PLAYERS];
inline Vec3 playerRenderPos[MAX_PLAYERS];
inline float playerRenderYaw[MAX_PLAYERS] = {0};
inline float playerRenderPitch[MAX_PLAYERS] = {0};
inline bool playerInterpReady[MAX_PLAYERS] = {false};

inline void initPlayerModels() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        std::vector<float> v;
        buildPlayerModel(v, i);
        playerVCs[i] = (int)v.size() / 8;
        glGenVertexArrays(1, &playerVAOs[i]);
        glGenBuffers(1, &playerVBOs[i]);
        glBindVertexArray(playerVAOs[i]);
        glBindBuffer(GL_ARRAY_BUFFER, playerVBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, v.size() * 4, v.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, (void*)12);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, (void*)20);
        glEnableVertexAttribArray(2);
        playerLastPos[i] = Vec3(0, 0, 0);
        playerRenderPos[i] = Vec3(0, 0, 0);
        playerRenderYaw[i] = 0.0f;
        playerRenderPitch[i] = 0.0f;
        playerInterpReady[i] = false;
    }
}

inline void resetPlayerInterpolation() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        playerLastPos[i] = Vec3(0, 0, 0);
        playerRenderPos[i] = Vec3(0, 0, 0);
        playerRenderYaw[i] = 0.0f;
        playerRenderPitch[i] = 0.0f;
        playerAnimPhase[i] = 0.0f;
        playerInterpReady[i] = false;
    }
}

inline void updatePlayerInterpolation(int myId, float dt) {
    float alpha = clamp01(dt * 10.0f);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (i == myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) {
            playerInterpReady[i] = false;
            continue;
        }
        Vec3 targetPos = netMgr.players[i].pos;
        float targetYaw = netMgr.players[i].yaw;
        float targetPitch = netMgr.players[i].pitch;
        if (!playerInterpReady[i]) {
            playerRenderPos[i] = targetPos;
            playerLastPos[i] = targetPos;
            playerRenderYaw[i] = targetYaw;
            playerRenderPitch[i] = targetPitch;
            playerInterpReady[i] = true;
            continue;
        }
        playerRenderPos[i] = playerRenderPos[i] + (targetPos - playerRenderPos[i]) * alpha;
        playerRenderYaw[i] = lerpAngle(playerRenderYaw[i], targetYaw, alpha);
        playerRenderPitch[i] = lerpAngle(playerRenderPitch[i], targetPitch, alpha);
    }
}

inline int gatherRemoteFlashlights(int myId, float outPos[12], float outDir[12]) {
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS && count < 4; i++) {
        if (i == myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
        if (!netMgr.players[i].flashlightOn || !playerInterpReady[i]) continue;
        Vec3 pos = playerRenderPos[i];
        float yaw = playerRenderYaw[i];
        float pitch = playerRenderPitch[i];
        Vec3 dir(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch));
        outPos[count * 3 + 0] = pos.x;
        outPos[count * 3 + 1] = pos.y;
        outPos[count * 3 + 2] = pos.z;
        outDir[count * 3 + 0] = dir.x;
        outDir[count * 3 + 1] = dir.y;
        outDir[count * 3 + 2] = dir.z;
        count++;
    }
    return count;
}

inline void renderPlayers(GLuint shader, Mat4& proj, Mat4& view, int myId) {
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "P"), 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(glGetUniformLocation(shader, "V"), 1, GL_FALSE, view.m);

    // Ensure a stable texture for players.
    glBindTexture(GL_TEXTURE_2D, playerTex);
    GLint tintLoc = glGetUniformLocation(shader, "modelTint");

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (i == myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
        if (!playerInterpReady[i]) continue;

        if(tintLoc >= 0) glUniform3f(tintLoc, PLAYER_COLORS[i][0], PLAYER_COLORS[i][1], PLAYER_COLORS[i][2]);
        Vec3 pos = playerRenderPos[i];
        float yaw = playerRenderYaw[i];
        Vec3 delta = pos - playerLastPos[i];
        delta.y = 0;
        float moved = sqrtf(delta.x * delta.x + delta.z * delta.z);
        if (moved > 0.01f) playerAnimPhase[i] += moved * 8.0f;
        playerLastPos[i] = pos;
        float bobY = sinf(playerAnimPhase[i]) * 0.02f;
        float px = pos.x;
        float py = pos.y - 1.7f + bobY;
        float pz = pos.z;
        float c = cosf(yaw);
        float s = sinf(yaw);
        Mat4 model;
        model.m[0] = c;    model.m[4] = 0;  model.m[8] = s;   model.m[12] = px;
        model.m[1] = 0;    model.m[5] = 1;  model.m[9] = 0;   model.m[13] = py;
        model.m[2] = -s;   model.m[6] = 0;  model.m[10] = c;  model.m[14] = pz;
        model.m[3] = 0;    model.m[7] = 0;  model.m[11] = 0;  model.m[15] = 1;
        glUniformMatrix4fv(glGetUniformLocation(shader, "M"), 1, GL_FALSE, model.m);
        glBindVertexArray(playerVAOs[i]);
        glDrawArrays(GL_TRIANGLES, 0, playerVCs[i]);
    }

    // Restore default tint for subsequent draws.
    if(tintLoc >= 0) glUniform3f(tintLoc, 1.0f, 1.0f, 1.0f);
}
