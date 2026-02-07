#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <glad/glad.h>
#include "entity_types.h"
#include "entity_model.h"

class EntityManager {
public:
    std::vector<Entity> entities;
    GLuint entityVAO, entityVBO, entityTex;
    int entityVC;
    float spawnTimer, maxEntities, dangerLevel;
    
    EntityManager() : entityVAO(0), entityVBO(0), entityTex(0), entityVC(0), 
                      spawnTimer(0), maxEntities(3), dangerLevel(0) {}
    
    void init() {
        std::vector<float> verts; buildStalkerModel(verts); entityVC = (int)verts.size() / 8;
        glGenVertexArrays(1, &entityVAO); glGenBuffers(1, &entityVBO);
        glBindVertexArray(entityVAO); glBindBuffer(GL_ARRAY_BUFFER, entityVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,32,(void*)12); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,32,(void*)20); glEnableVertexAttribArray(2);
        unsigned char* td = new unsigned char[64*64*4]; genEntityTexture(td, 64, 64);
        glGenTextures(1, &entityTex); glBindTexture(GL_TEXTURE_2D, entityTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,td); delete[] td;
    }
    
    void spawnEntity(EntityType type, Vec3 pos, int maze[][30], int mzw, int mzh) {
        Entity e; e.type = type; e.pos = pos; e.pos.y = 0; e.active = true; e.state = ENT_ROAMING;
        if(type==ENTITY_STALKER) { e.speed=1.5f; e.detectionRange=20.0f; e.attackRange=1.0f; }
        else if(type==ENTITY_CRAWLER) { e.speed=5.0f; e.detectionRange=15.0f; e.attackRange=1.5f; e.pos.y=-0.8f; }
        else if(type==ENTITY_SHADOW) { e.speed=0.5f; e.detectionRange=8.0f; e.attackRange=2.0f; }
        entities.push_back(e);
    }
    
    void update(float dt, Vec3 pPos, float pYaw, int maze[][30], int mzw, int mzh, float cs) {
        dangerLevel = 0;
        for(auto& e : entities) {
            if(!e.active) continue;
            Vec3 toP = pPos - e.pos; toP.y = 0;
            float dist = sqrtf(toP.x*toP.x + toP.z*toP.z);
            Vec3 look(sinf(pYaw), 0, cosf(pYaw)), toE = e.pos - pPos; toE.y = 0;
            float toEL = sqrtf(toE.x*toE.x + toE.z*toE.z);
            if(toEL > 0.01f) { toE.x /= toEL; toE.z /= toEL; }
            bool looking = (look.x*toE.x + look.z*toE.z) > 0.85f && dist < e.detectionRange;
            if(dist < e.detectionRange) { float prox = 1.0f - dist/e.detectionRange; if(prox > dangerLevel) dangerLevel = prox; }
            
            if(e.type == ENTITY_STALKER) updateStalker(e, dt, pPos, dist, looking, maze, mzw, mzh, cs);
            else if(e.type == ENTITY_CRAWLER) updateCrawler(e, dt, pPos, dist, maze, mzw, mzh, cs);
            else if(e.type == ENTITY_SHADOW) updateShadow(e, dt, pPos, dist, looking);
            // Check for attacking state for shadow/crawler
            if(e.type == ENTITY_SHADOW && dist < e.attackRange && !looking) e.state = ENT_ATTACKING;
            if(e.type == ENTITY_CRAWLER && dist < e.attackRange) e.state = ENT_ATTACKING;
            
            e.animPhase += dt * 3.0f; if(e.animPhase > 6.283f) e.animPhase -= 6.283f;
            e.flickerTimer += dt;
            e.visible = (dist < 5.0f) ? ((int)(e.flickerTimer*15)%3) != 0 : true;
        }
        entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity& e) { return !e.active; }), entities.end());
    }
    
    void updateStalker(Entity& e, float dt, Vec3 pPos, float dist, bool looking, int maze[][30], int mzw, int mzh, float cs) {
        e.stateTimer += dt;
        if(looking && dist < e.detectionRange) { e.lastSeenTimer += dt; if(e.lastSeenTimer > 2.0f) e.state = ENT_FLEEING; }
        else { e.lastSeenTimer = 0;
            if(dist < e.detectionRange) { e.state = ENT_STALKING;
                Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x + dir.z*dir.z);
                if(len > 0.1f) { dir.x /= len; dir.z /= len;
                    float nx = e.pos.x + dir.x*e.speed*dt, nz = e.pos.z + dir.z*e.speed*dt;
                    int cx = (int)(nx/cs), cz = (int)(nz/cs);
                    if(cx>=0 && cx<mzw && cz>=0 && cz<mzh && maze[cx][cz]==0) { e.pos.x = nx; e.pos.z = nz; }
                    e.yaw = atan2f(dir.x, dir.z);
                }
            } else { e.state = ENT_ROAMING;
                if(e.stateTimer > 3.0f) { e.yaw += ((rand()%100)/50.0f-1.0f)*3.14159f; e.stateTimer = 0; }
                float nx = e.pos.x + sinf(e.yaw)*e.speed*0.3f*dt, nz = e.pos.z + cosf(e.yaw)*e.speed*0.3f*dt;
                int cx = (int)(nx/cs), cz = (int)(nz/cs);
                if(cx>=0 && cx<mzw && cz>=0 && cz<mzh && maze[cx][cz]==0) { e.pos.x = nx; e.pos.z = nz; }
                else e.yaw += 1.5708f;
            }
        }
        if(e.state == ENT_FLEEING) { Vec3 dir = e.pos - pPos; dir.y = 0; float len = sqrtf(dir.x*dir.x+dir.z*dir.z);
            if(len > 0.1f) { dir.x/=len; dir.z/=len; e.pos.x += dir.x*e.speed*3.0f*dt; e.pos.z += dir.z*e.speed*3.0f*dt; }
            if(e.stateTimer > 5.0f || dist > 25.0f) e.active = false; }
        if(dist < e.attackRange && e.state == ENT_STALKING) e.state = ENT_ATTACKING;
    }
    
    void updateCrawler(Entity& e, float dt, Vec3 pPos, float dist, int maze[][30], int mzw, int mzh, float cs) {
        e.stateTimer += dt; e.pos.y = -0.8f; // Stay low
        if(dist < e.detectionRange) { e.state = ENT_CHASING;
            Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x + dir.z*dir.z);
            if(len > 0.1f) { dir.x /= len; dir.z /= len;
                float nx = e.pos.x + dir.x*e.speed*dt, nz = e.pos.z + dir.z*e.speed*dt;
                int cx = (int)(nx/cs), cz = (int)(nz/cs);
                if(cx>=0 && cx<mzw && cz>=0 && cz<mzh && maze[cx][cz]==0) { e.pos.x = nx; e.pos.z = nz; }
                e.yaw = atan2f(dir.x, dir.z); }
            if(dist < e.attackRange) e.state = ENT_ATTACKING;
        } else { e.state = ENT_ROAMING;
            if(e.stateTimer > 2.0f) { e.yaw += ((rand()%100)/50.0f-1.0f)*3.14159f; e.stateTimer = 0; }
            float nx = e.pos.x + sinf(e.yaw)*e.speed*0.2f*dt, nz = e.pos.z + cosf(e.yaw)*e.speed*0.2f*dt;
            int cx = (int)(nx/cs), cz = (int)(nz/cs);
            if(cx>=0 && cx<mzw && cz>=0 && cz<mzh && maze[cx][cz]==0) { e.pos.x = nx; e.pos.z = nz; }
            else e.yaw += 1.5708f; }
    }
    
    void updateShadow(Entity& e, float dt, Vec3 pPos, float dist, bool looking) {
        e.visible = looking ? false : (dist < e.detectionRange);
        if(!looking && dist < e.detectionRange && dist > e.attackRange) {
            Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x+dir.z*dir.z);
            if(len > 0.1f) { dir.x/=len; dir.z/=len; e.pos.x += dir.x*e.speed*dt; e.pos.z += dir.z*e.speed*dt; e.yaw = atan2f(dir.x,dir.z); }
        }
    }
    
    void render(GLuint shader, Mat4& proj, Mat4& view) {
        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader,"P"),1,GL_FALSE,proj.m);
        glUniformMatrix4fv(glGetUniformLocation(shader,"V"),1,GL_FALSE,view.m);
        glBindTexture(GL_TEXTURE_2D, entityTex); glBindVertexArray(entityVAO);
        for(auto& e : entities) {
            if(!e.active || !e.visible) continue;
            Mat4 model; float c=cosf(e.yaw), s=sinf(e.yaw);
            model.m[0]=c; model.m[2]=s; model.m[8]=-s; model.m[10]=c;
            model.m[12]=e.pos.x+sinf(e.animPhase)*0.02f; model.m[13]=e.pos.y; model.m[14]=e.pos.z;
            glUniformMatrix4fv(glGetUniformLocation(shader,"M"),1,GL_FALSE,model.m);
            glDrawArrays(GL_TRIANGLES, 0, entityVC);
        }
    }
    
    bool checkPlayerAttack(Vec3 pPos) {
        for(auto& e : entities) { if(!e.active) continue;
            Vec3 d = e.pos - pPos; d.y = 0; float dist = sqrtf(d.x*d.x + d.z*d.z);
            if(dist < e.attackRange && e.state == ENT_ATTACKING) return true; }
        return false;
    }
    
    void reset() { entities.clear(); spawnTimer = 0; dangerLevel = 0; }
};

#endif