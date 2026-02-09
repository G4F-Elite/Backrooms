#pragma once
#include <glad/glad.h>
#include <vector>
#include "math.h"
#include "geometry.h"

struct RenderData {
    GLuint wallVAO, wallVBO, floorVAO, floorVBO, ceilVAO, ceilVBO;
    GLuint lightVAO, lightVBO, pillarVAO, pillarVBO;
    GLuint quadVAO, quadVBO, fbo, fboTex, fboDepthTex;
    int wallVC, floorVC, ceilVC, lightVC, pillarVC;
};

inline void setupVAO(GLuint& vao, GLuint& vbo, std::vector<float>& d, bool hasNorm) {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, d.size() * 4, d.data(), GL_STATIC_DRAW);
    
    if (hasNorm) {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, (void*)12);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, (void*)20);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
    } else {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*)12);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }
}

inline void initFBO(GLuint& fbo, GLuint& fboTex, GLuint& depthTex, int W, int H) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &fboTex);
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);

    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, W, H, 0, 0x1902, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void initQuad(GLuint& vao, GLuint& vbo) {
    float qv[] = {-1, -1, 0, 0, 1, -1, 1, 0, 1, 1, 1, 1, -1, -1, 0, 0, 1, 1, 1, 1, -1, 1, 0, 1};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(qv), qv, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}
