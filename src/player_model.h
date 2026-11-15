#pragma once
// Player model for multiplayer - animated humanoid with flashlight and interpolation
#include <vector>
#include <glad/glad.h>
#include "math.h"
#include "interpolation.h"
#include "net_types.h"
#include "creature_mesh.h"
#include "creature_texture.h"
#include "creature_anim.h"

// Teammates use their own body texture (bound in renderPlayers) so held-item
// textures never bleed onto them.
inline GLuint playerBodyTex = 0;

// Muted per-player coverall tints: enough to tell people apart, never toy-coloured.
const float PLAYER_COLORS[4][3] = {
    {0.86f, 0.82f, 0.70f},
    {0.70f, 0.78f, 0.84f},
    {0.84f, 0.72f, 0.68f},
    {0.72f, 0.82f, 0.72f}
};

// Plain worker in a coverall: same faceted language as the entities, but with sane proportions.
inline void buildPlayerModel(CreatureMesh& m) {
    m = CreatureMesh();
    m.strips = 4;
    m.vScale = 1.0f;
    const int SKIN = 0, SUIT = 1, DARK = 2, GEAR = 3;

    int root = m.beginPart(CJ_ROOT, -1, Vec3(0, 0.93f, 0));
    m.strip = SUIT;
    CreatureRing pelvis[3] = {
        {Vec3(0, 0.82f, 0.0f), 0.145f, 0.100f},
        {Vec3(0, 0.95f, 0.0f), 0.160f, 0.105f},
        {Vec3(0, 1.09f, 0.0f), 0.142f, 0.096f}
    };
    m.tube(pelvis, 3, 8);

    int spine = m.beginPart(CJ_SPINE, root, Vec3(0, 1.08f, 0));
    CreatureRing chest[5] = {
        {Vec3(0, 1.06f, 0.000f), 0.144f, 0.098f},
        {Vec3(0, 1.25f, 0.005f), 0.165f, 0.105f},
        {Vec3(0, 1.42f, 0.010f), 0.185f, 0.110f},
        {Vec3(0, 1.50f, 0.010f), 0.170f, 0.095f},
        {Vec3(0, 1.56f, 0.010f), 0.070f, 0.060f}
    };
    m.tube(chest, 5, 8);
    m.strip = DARK;
    CreatureRing belt[2] = { {Vec3(0, 1.055f, 0), 0.150f, 0.103f}, {Vec3(0, 1.095f, 0), 0.148f, 0.101f} };
    m.tube(belt, 2, 8, false, false);
    m.strip = GEAR;
    m.box(Vec3(0, 1.30f, -0.145f), 0.25f, 0.32f, 0.10f);
    m.strip = SKIN;
    m.limb(Vec3(0, 1.54f, 0.010f), Vec3(0, 1.63f, 0.015f), 0.046f, 0.044f, 6);

    m.beginPart(CJ_HEAD, spine, Vec3(0, 1.60f, 0.015f));
    m.ovoid(Vec3(0, 1.575f, 0.018f), Vec3(0, 1.815f, 0.022f), 0.084f, 0.098f, 8);
    m.strip = DARK;
    m.ovoid(Vec3(0, 1.690f, -0.016f), Vec3(0, 1.832f, 0.004f), 0.090f, 0.098f, 8, 0.4f);

    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 shoulder(fs * 0.212f, 1.44f, 0.010f);
        Vec3 elbow(fs * 0.238f, 1.16f, 0.000f);
        Vec3 wrist(fs * 0.238f, 0.915f, 0.030f);
        int arm = m.beginPart(s < 0 ? CJ_ARM_L : CJ_ARM_R, spine, shoulder);
        m.strip = SUIT;
        m.limb(shoulder, elbow, 0.052f, 0.043f, 6);
        m.beginPart(s < 0 ? CJ_FOREARM_L : CJ_FOREARM_R, arm, elbow);
        m.limb(elbow, wrist, 0.041f, 0.034f, 6);
        m.strip = DARK;
        m.ovoid(Vec3(wrist.x, 0.925f, 0.030f), Vec3(wrist.x, 0.800f, 0.042f), 0.030f, 0.040f, 6);
        if (s > 0) {
            // Flashlight held along the forearm, so raising the arm aims it.
            m.strip = GEAR;
            m.limb(Vec3(wrist.x, 0.870f, 0.075f), Vec3(wrist.x, 0.735f, 0.082f), 0.019f, 0.019f, 6);
            m.limb(Vec3(wrist.x, 0.745f, 0.082f), Vec3(wrist.x, 0.700f, 0.084f), 0.022f, 0.030f, 6);
        }
    }

    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 hip(fs * 0.085f, 0.90f, 0.0f);
        Vec3 knee(fs * 0.090f, 0.49f, 0.015f);
        Vec3 ankle(fs * 0.090f, 0.10f, -0.010f);
        int leg = m.beginPart(s < 0 ? CJ_LEG_L : CJ_LEG_R, root, hip);
        m.strip = SUIT;
        m.limb(hip, knee, 0.080f, 0.060f, 6);
        m.beginPart(s < 0 ? CJ_SHIN_L : CJ_SHIN_R, leg, knee);
        m.limb(knee, ankle, 0.057f, 0.047f, 6);
        m.strip = DARK;
        m.limb(Vec3(ankle.x, 0.20f, -0.008f), Vec3(ankle.x, 0.06f, -0.012f), 0.052f, 0.052f, 6);
        CreatureRing boot[3] = {
            {Vec3(ankle.x, 0.052f, -0.095f), 0.048f, 0.050f},
            {Vec3(ankle.x, 0.050f, 0.040f), 0.054f, 0.046f},
            {Vec3(ankle.x, 0.036f, 0.175f), 0.046f, 0.030f}
        };
        m.tube(boot, 3, 5);
    }
    m.finish();
}

inline void genPlayerTexture(unsigned char* data, int w, int h) {
    const CreatureStripStyle st[4] = {
        {176.0f, 150.0f, 128.0f, 7.0f, CREATURE_PAT_PLAIN},
        {116.0f, 116.0f, 112.0f, 8.0f, CREATURE_PAT_WEAVE},
        {34.0f, 33.0f, 34.0f, 5.0f, CREATURE_PAT_PLAIN},
        {72.0f, 70.0f, 64.0f, 6.0f, CREATURE_PAT_WEAVE}
    };
    genCreatureTexture(st, 4, data, w, h, 53u);
}

inline std::vector<CreaturePart> playerParts;
inline float playerMoveBlend[MAX_PLAYERS] = {0};
inline double playerAnimClock = 0.0;

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
        CreatureMesh mesh;
        buildPlayerModel(mesh);
        const std::vector<float>& v = mesh.v;
        if (i == 0) playerParts = mesh.parts;
        playerVCs[i] = mesh.vertexCount();
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
    const int texSize = 128;
    std::vector<unsigned char> td((size_t)texSize * texSize * 4);
    genPlayerTexture(td.data(), texSize, texSize);
    glGenTextures(1, &playerBodyTex);
    glBindTexture(GL_TEXTURE_2D, playerBodyTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, td.data());
}

inline void resetPlayerInterpolation() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        playerLastPos[i] = Vec3(0, 0, 0);
        playerRenderPos[i] = Vec3(0, 0, 0);
        playerRenderYaw[i] = 0.0f;
        playerRenderPitch[i] = 0.0f;
        playerAnimPhase[i] = 0.0f;
        playerMoveBlend[i] = 0.0f;
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
        float yaw = netMgr.players[i].yaw;
        float pitch = netMgr.players[i].pitch;
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

    glBindTexture(GL_TEXTURE_2D, playerBodyTex);
    GLint tintLoc = glGetUniformLocation(shader, "modelTint");
    GLint modelLoc = glGetUniformLocation(shader, "M");

    double now = glfwGetTime();
    float dt = (float)(now - playerAnimClock);
    playerAnimClock = now;
    if (dt < 0.0005f) dt = 0.0005f;
    if (dt > 0.1f) dt = 0.1f;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (i == myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
        if (!playerInterpReady[i]) continue;

        if(tintLoc >= 0) glUniform3f(tintLoc, PLAYER_COLORS[i][0], PLAYER_COLORS[i][1], PLAYER_COLORS[i][2]);
        Vec3 pos = playerRenderPos[i];
        Vec3 delta = pos - playerLastPos[i];
        delta.y = 0;
        float moved = sqrtf(delta.x * delta.x + delta.z * delta.z);
        if (moved > 3.0f) moved = 0.0f; // teleport / respawn, not a step
        playerLastPos[i] = pos;
        // One full stride cycle every 1.4 m keeps the feet from sliding.
        playerAnimPhase[i] += moved * (3.14159265f / 0.7f);
        if (playerAnimPhase[i] > 6283.0f) playerAnimPhase[i] -= 6283.185f;
        float target = clamp01((moved / dt) / 1.2f);
        playerMoveBlend[i] += (target - playerMoveBlend[i]) * clamp01(dt * 9.0f);

        // Eye height drops from 1.7 to 0.9 when crouching; keep the boots on the floor.
        float crouch = 0.0f;
        if (pos.y > 0.6f && pos.y < 1.7f) crouch = clamp01((1.7f - pos.y) / 0.8f);
        Vec3 base(pos.x, pos.y - 1.7f + crouch * 0.8f, pos.z);

        CreaturePose pose = playerPose(playerAnimPhase[i], playerMoveBlend[i], crouch, playerRenderPitch[i],
                                       netMgr.players[i].flashlightOn, (float)now + (float)i * 1.7f);
        Mat4 mats[CREATURE_MAX_PARTS];
        creaturePartMatrices(playerParts, pose, creatureWorldMatrix(base, playerRenderYaw[i]), mats);
        glBindVertexArray(playerVAOs[i]);
        for (size_t k = 0; k < playerParts.size() && k < (size_t)CREATURE_MAX_PARTS; k++) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, mats[k].m);
            glDrawArrays(GL_TRIANGLES, playerParts[k].first, playerParts[k].count);
        }
    }

    // Restore default tint for subsequent draws.
    if(tintLoc >= 0) glUniform3f(tintLoc, 1.0f, 1.0f, 1.0f);
}
