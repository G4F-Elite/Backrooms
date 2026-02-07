#pragma once
// Player model for multiplayer - animated humanoid with flashlight
#include <vector>
#include "math.h"

// Яркие цвета для игроков с свечением
const float PLAYER_COLORS[4][3] = {
    {0.1f, 0.9f, 0.2f},  // Яркий зелёный - Host
    {0.2f, 0.6f, 1.0f},  // Голубой
    {1.0f, 0.5f, 0.1f},  // Оранжевый
    {0.9f, 0.2f, 0.8f}   // Розовый
};

// Добавить бокс в вершины
inline void addColorBox(std::vector<float>& v, float x, float y, float z,
    float sx, float sy, float sz, float r, float g, float b) {
    float hx = sx/2, hy = sy/2, hz = sz/2;
    // 6 граней по 6 вершин, 8 floats каждая (pos3 + uv2 + norm3)
    float faces[6][48] = {
        // Front +Z
        {x-hx,y-hy,z+hz,0,0,0,0,1, x+hx,y-hy,z+hz,1,0,0,0,1, x+hx,y+hy,z+hz,1,1,0,0,1,
         x-hx,y-hy,z+hz,0,0,0,0,1, x+hx,y+hy,z+hz,1,1,0,0,1, x-hx,y+hy,z+hz,0,1,0,0,1},
        // Back -Z
        {x+hx,y-hy,z-hz,0,0,0,0,-1, x-hx,y-hy,z-hz,1,0,0,0,-1, x-hx,y+hy,z-hz,1,1,0,0,-1,
         x+hx,y-hy,z-hz,0,0,0,0,-1, x-hx,y+hy,z-hz,1,1,0,0,-1, x+hx,y+hy,z-hz,0,1,0,0,-1},
        // Left -X
        {x-hx,y-hy,z-hz,0,0,-1,0,0, x-hx,y-hy,z+hz,1,0,-1,0,0, x-hx,y+hy,z+hz,1,1,-1,0,0,
         x-hx,y-hy,z-hz,0,0,-1,0,0, x-hx,y+hy,z+hz,1,1,-1,0,0, x-hx,y+hy,z-hz,0,1,-1,0,0},
        // Right +X
        {x+hx,y-hy,z+hz,0,0,1,0,0, x+hx,y-hy,z-hz,1,0,1,0,0, x+hx,y+hy,z-hz,1,1,1,0,0,
         x+hx,y-hy,z+hz,0,0,1,0,0, x+hx,y+hy,z-hz,1,1,1,0,0, x+hx,y+hy,z+hz,0,1,1,0,0},
        // Top +Y
        {x-hx,y+hy,z+hz,0,0,0,1,0, x+hx,y+hy,z+hz,1,0,0,1,0, x+hx,y+hy,z-hz,1,1,0,1,0,
         x-hx,y+hy,z+hz,0,0,0,1,0, x+hx,y+hy,z-hz,1,1,0,1,0, x-hx,y+hy,z-hz,0,1,0,1,0},
        // Bottom -Y
        {x-hx,y-hy,z-hz,0,0,0,-1,0, x+hx,y-hy,z-hz,1,0,0,-1,0, x+hx,y-hy,z+hz,1,1,0,-1,0,
         x-hx,y-hy,z-hz,0,0,0,-1,0, x+hx,y-hy,z+hz,1,1,0,-1,0, x-hx,y-hy,z+hz,0,1,0,-1,0}
    };
    for (int f = 0; f < 6; f++)
        for (int i = 0; i < 48; i++) v.push_back(faces[f][i]);
}

// Построить модель игрока
inline void buildPlayerModel(std::vector<float>& v, int colorId) {
    float r = PLAYER_COLORS[colorId][0];
    float g = PLAYER_COLORS[colorId][1];
    float b = PLAYER_COLORS[colorId][2];
    
    // Тело (куртка с полосами)
    addColorBox(v, 0, 1.0f, 0, 0.45f, 0.55f, 0.22f, r, g, b);
    // Полоса на куртке
    addColorBox(v, 0, 0.95f, 0.12f, 0.35f, 0.08f, 0.02f, r*1.5f, g*1.5f, b*1.5f);
    
    // Голова
    addColorBox(v, 0, 1.5f, 0, 0.28f, 0.28f, 0.28f, 0.85f, 0.75f, 0.65f);
    // Волосы
    addColorBox(v, 0, 1.67f, 0, 0.30f, 0.08f, 0.30f, 0.15f, 0.1f, 0.05f);
    // Глаза (тёмные)
    addColorBox(v, -0.06f, 1.52f, 0.14f, 0.05f, 0.03f, 0.02f, 0.1f, 0.1f, 0.1f);
    addColorBox(v, 0.06f, 1.52f, 0.14f, 0.05f, 0.03f, 0.02f, 0.1f, 0.1f, 0.1f);
    
    // Ноги (джинсы)
    addColorBox(v, -0.1f, 0.35f, 0, 0.14f, 0.65f, 0.14f, 0.15f, 0.2f, 0.35f);
    addColorBox(v, 0.1f, 0.35f, 0, 0.14f, 0.65f, 0.14f, 0.15f, 0.2f, 0.35f);
    // Обувь
    addColorBox(v, -0.1f, 0.05f, 0.02f, 0.14f, 0.1f, 0.18f, 0.1f, 0.1f, 0.1f);
    addColorBox(v, 0.1f, 0.05f, 0.02f, 0.14f, 0.1f, 0.18f, 0.1f, 0.1f, 0.1f);
    
    // Руки
    addColorBox(v, -0.32f, 1.0f, 0, 0.1f, 0.45f, 0.1f, r*0.8f, g*0.8f, b*0.8f);
    addColorBox(v, 0.32f, 1.0f, 0, 0.1f, 0.45f, 0.1f, r*0.8f, g*0.8f, b*0.8f);
    // Кисти рук
    addColorBox(v, -0.32f, 0.72f, 0, 0.08f, 0.1f, 0.08f, 0.85f, 0.75f, 0.65f);
    addColorBox(v, 0.32f, 0.72f, 0, 0.08f, 0.1f, 0.08f, 0.85f, 0.75f, 0.65f);
    
    // Рюкзак
    addColorBox(v, 0, 1.0f, -0.18f, 0.3f, 0.4f, 0.12f, 0.2f, 0.15f, 0.1f);
}

// VAOs и VBOs
inline GLuint playerVAOs[MAX_PLAYERS] = {0};
inline GLuint playerVBOs[MAX_PLAYERS] = {0};
inline int playerVCs[MAX_PLAYERS] = {0};
inline float playerAnimPhase[MAX_PLAYERS] = {0};
inline Vec3 playerLastPos[MAX_PLAYERS];

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
        
        playerLastPos[i] = Vec3(0,0,0);
    }
}

// Рендер других игроков с анимацией
inline void renderPlayers(GLuint shader, Mat4& proj, Mat4& view, int myId) {
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "P"), 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(glGetUniformLocation(shader, "V"), 1, GL_FALSE, view.m);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (i == myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
        
        Vec3 pos = netMgr.players[i].pos;
        float yaw = netMgr.players[i].yaw;
        
        // Проверяем движение для анимации
        Vec3 delta = pos - playerLastPos[i];
        delta.y = 0;
        float moved = sqrtf(delta.x*delta.x + delta.z*delta.z);
        if (moved > 0.01f) {
            playerAnimPhase[i] += moved * 8.0f;
        }
        playerLastPos[i] = pos;
        
        // Покачивание при ходьбе
        float bobY = sinf(playerAnimPhase[i]) * 0.02f;
        
        // Создаём матрицу модели вручную (column-major для OpenGL)
        // Модель на y=0..1.7, позиция игрока на высоте глаз (1.7)
        float px = pos.x;
        float py = pos.y - 1.7f + bobY;
        float pz = pos.z;
        
        // Поворачиваем модель по сетевому yaw без дополнительного смещения.
        float angle = yaw;
        float c = cosf(angle);
        float s = sinf(angle);
        
        // Column-major матрица: Translation * RotationY
        // [c  0  s  px]
        // [0  1  0  py]
        // [-s 0  c  pz]
        // [0  0  0  1 ]
        Mat4 model;
        model.m[0] = c;    model.m[4] = 0;  model.m[8] = s;   model.m[12] = px;
        model.m[1] = 0;    model.m[5] = 1;  model.m[9] = 0;   model.m[13] = py;
        model.m[2] = -s;   model.m[6] = 0;  model.m[10] = c;  model.m[14] = pz;
        model.m[3] = 0;    model.m[7] = 0;  model.m[11] = 0;  model.m[15] = 1;
        
        glUniformMatrix4fv(glGetUniformLocation(shader, "M"), 1, GL_FALSE, model.m);
        
        glBindVertexArray(playerVAOs[i]);
        glDrawArrays(GL_TRIANGLES, 0, playerVCs[i]);
    }
}
