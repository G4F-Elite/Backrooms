#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <glad/glad.h>
#include "entity_types.h"
#include "entity_model.h"
#include "world.h"
#include "progression.h"

// Backlog: "hearing" support for entities (crawler reacts to loud footsteps/sprinting)
// Values are set from gameplay/audio (see game_main_entry.h loop).
inline float gPlayerNoise = 0.0f; // 0..1

class EntityManager {
public:
    std::vector<Entity> entities;
    GLuint entityVAO[4], entityVBO[4], entityTex[4];
    int entityVC[4];
    float spawnTimer, maxEntities, dangerLevel;
    Vec3 lastPlayerPos;
    bool hasLastPlayerPos;
    
    EntityManager() : spawnTimer(0), maxEntities(3), dangerLevel(0), lastPlayerPos(0,0,0), hasLastPlayerPos(false) {
        for(int i=0;i<4;i++){
            entityVAO[i] = 0;
            entityVBO[i] = 0;
            entityTex[i] = 0;
            entityVC[i] = 0;
        }
    }
    
    void init() {
        auto buildForType = [&](EntityType type, void (*builder)(std::vector<float>&)) {
            std::vector<float> verts;
            builder(verts);
            int ti = (int)type;
            entityVC[ti] = (int)verts.size() / 8;
            glGenVertexArrays(1, &entityVAO[ti]);
            glGenBuffers(1, &entityVBO[ti]);
            glBindVertexArray(entityVAO[ti]);
            glBindBuffer(GL_ARRAY_BUFFER, entityVBO[ti]);
            glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,32,(void*)12); glEnableVertexAttribArray(1);
            glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,32,(void*)20); glEnableVertexAttribArray(2);

            unsigned char* td = new unsigned char[64 * 64 * 4];
            genEntityTextureByType(type, td, 64, 64);
            glGenTextures(1, &entityTex[ti]);
            glBindTexture(GL_TEXTURE_2D, entityTex[ti]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,td);
            delete[] td;
        };

        buildForType(ENTITY_STALKER, buildStalkerModel);
        buildForType(ENTITY_CRAWLER, buildCrawlerModel);
        buildForType(ENTITY_SHADOW, buildShadowModel);
    }
    
    void spawnEntity(EntityType type, Vec3 pos, int maze[][30], int mzw, int mzh) {
        (void)maze; (void)mzw; (void)mzh;
        Entity e; e.type = type; e.pos = pos; e.pos.y = 0; e.active = true; e.state = ENT_ROAMING;
        if(type==ENTITY_STALKER) { e.speed=1.7f; e.detectionRange=22.0f; e.attackRange=1.15f; }
        else if(type==ENTITY_CRAWLER) { e.speed=3.4f; e.detectionRange=17.0f; e.attackRange=1.35f; e.pos.y=0.58f; }
        else if(type==ENTITY_SHADOW) { e.speed=1.15f; e.detectionRange=12.0f; e.attackRange=1.8f; }

        if(isParkingLevel(gCurrentLevel)){
            if(type==ENTITY_STALKER){ e.speed *= 0.92f; e.detectionRange += 4.0f; }
            if(type==ENTITY_CRAWLER){ e.speed *= 1.18f; e.detectionRange += 2.5f; }
            if(type==ENTITY_SHADOW){ e.attackRange += 0.4f; e.detectionRange += 1.2f; }
        }else{
            if(type==ENTITY_STALKER){ e.attackRange += 0.2f; }
            if(type==ENTITY_CRAWLER){ e.speed *= 1.06f; }
            if(type==ENTITY_SHADOW){ e.speed *= 0.92f; }
        }
        e.behaviorMode = BEHAVIOR_DEFAULT;
        e.behaviorTimer = 0.8f + (float)(rand()%120) * 0.01f;
        e.stateTimer = 0.0f;
        entities.push_back(e);
    }
    
    void update(float dt, Vec3 pPos, float pYaw, int maze[][30], int mzw, int mzh, float cs) {
        (void)maze; (void)mzw; (void)mzh; (void)cs;
        dangerLevel = 0;
        float playerMove = 0.0f;
        if(hasLastPlayerPos){
            Vec3 pm = pPos - lastPlayerPos;
            pm.y = 0.0f;
            playerMove = pm.len();
        }
        hasLastPlayerPos = true;
        lastPlayerPos = pPos;
        for(auto& e : entities) {
            if(!e.active) continue;
            EntityState prevState = e.state;
            Vec3 toP = pPos - e.pos; toP.y = 0;
            float dist = sqrtf(toP.x*toP.x + toP.z*toP.z);

            // Crawler hearing: expands detection range based on player noise.
            float effectiveDetect = e.detectionRange;
            if(e.type == ENTITY_CRAWLER){
                float noise = gPlayerNoise;
                if(noise < 0.0f) noise = 0.0f;
                if(noise > 1.0f) noise = 1.0f;
                effectiveDetect = e.detectionRange + noise * 10.0f; // up to +10m
            }
            Vec3 look(mSin(pYaw), 0, mCos(pYaw)), toE = e.pos - pPos; toE.y = 0;
            float toEL = sqrtf(toE.x*toE.x + toE.z*toE.z);
            if(toEL > 0.01f) { toE.x /= toEL; toE.z /= toEL; }
            bool looking = (look.x*toE.x + look.z*toE.z) > 0.85f && dist < effectiveDetect;
            if(dist < effectiveDetect + 5.0f) {
                float prox = 1.0f - dist/(effectiveDetect + 5.0f);
                if(prox > dangerLevel) dangerLevel = prox;
            }

            e.behaviorTimer -= dt;
            if(e.behaviorTimer <= 0.0f){
                e.behaviorTimer = 0.9f + (float)(rand()%190) / 100.0f;
                if(e.type == ENTITY_STALKER){
                    if(looking && dist < 11.0f) e.behaviorMode = BEHAVIOR_SNEAK;
                    else if(!looking && dist > 7.0f && dist < 18.0f) e.behaviorMode = BEHAVIOR_FLANK;
                    else if(dist < 7.0f) e.behaviorMode = BEHAVIOR_RUSH;
                    else e.behaviorMode = BEHAVIOR_DEFAULT;
                }else if(e.type == ENTITY_CRAWLER){
                    if(dist < 5.8f || (dist < 9.0f && playerMove > 0.035f)) e.behaviorMode = BEHAVIOR_RUSH;
                    else if(playerMove > 0.02f && dist < 13.0f) e.behaviorMode = BEHAVIOR_FLANK;
                    else e.behaviorMode = BEHAVIOR_DEFAULT;
                }else if(e.type == ENTITY_SHADOW){
                    if(looking) e.behaviorMode = BEHAVIOR_SNEAK;
                    else if(dist < 8.5f) e.behaviorMode = BEHAVIOR_RUSH;
                    else e.behaviorMode = BEHAVIOR_FLANK;
                }
            }
            
            if(e.type == ENTITY_STALKER) updateStalker(e, dt, pPos, dist, looking, maze, mzw, mzh, cs);
            else if(e.type == ENTITY_CRAWLER) updateCrawler(e, dt, pPos, dist, effectiveDetect, maze, mzw, mzh, cs);
            else if(e.type == ENTITY_SHADOW) updateShadow(e, dt, pPos, dist, looking, pYaw);

            if(isParkingLevel(gCurrentLevel) && e.type == ENTITY_CRAWLER && dist < 10.0f && (rand()%1000) < 18){
                e.behaviorMode = BEHAVIOR_RUSH;
            }
            if(isLevelZero(gCurrentLevel) && e.type == ENTITY_SHADOW && looking && dist < 13.0f && (rand()%1000) < 14){
                e.behaviorMode = BEHAVIOR_FLANK;
            }
            // Check for attacking state for shadow/crawler
            if(e.type == ENTITY_SHADOW && dist < e.attackRange && !looking) e.state = ENT_ATTACKING;
            if(e.type == ENTITY_CRAWLER && dist < e.attackRange) e.state = ENT_ATTACKING;
            if(e.state != prevState) e.stateTimer = 0.0f;
            
            e.animPhase += dt * 3.0f; if(e.animPhase > 6.283f) e.animPhase -= 6.283f;
            e.flickerTimer += dt;
            if(e.type != ENTITY_SHADOW) e.visible = true;
            else e.visible = (dist < 6.0f) ? ((int)(e.flickerTimer*15)%4) != 0 : true;
        }
        entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity& e) { return !e.active; }), entities.end());
    }
    
    void updateStalker(Entity& e, float dt, Vec3 pPos, float dist, bool looking, int maze[][30], int mzw, int mzh, float cs) {
        (void)maze; (void)mzw; (void)mzh; (void)cs;
        e.stateTimer += dt;
        float speedMul = 1.0f;
        if(e.behaviorMode == BEHAVIOR_SNEAK) speedMul = 0.64f;
        else if(e.behaviorMode == BEHAVIOR_RUSH) speedMul = 1.72f;
        else if(e.behaviorMode == BEHAVIOR_FLANK) speedMul = 1.12f;
        if(looking && dist < e.detectionRange) {
            e.state = ENT_STALKING;
            e.lastSeenTimer += dt;
            Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x + dir.z*dir.z);
            if(len > 0.1f) {
                dir.x /= len; dir.z /= len;
                float sideSign = ((int)(e.animPhase * 2.2f) & 1) ? -1.0f : 1.0f;
                float sx = dir.z * sideSign;
                float sz = -dir.x * sideSign;
                float keep = (dist < 5.5f) ? -0.55f : 0.15f;
                float nx = e.pos.x + (sx * 0.75f + dir.x * keep) * e.speed * speedMul * dt;
                float nz = e.pos.z + (sz * 0.75f + dir.z * keep) * e.speed * speedMul * dt;
                if(!collideWorld(nx,nz,0.32f)) { e.pos.x = nx; e.pos.z = nz; }
                e.yaw = atan2f(sx, sz);
            }
            if(e.lastSeenTimer > 3.2f) e.state = ENT_FLEEING;
        }
        else { e.lastSeenTimer = 0;
            if(dist < e.detectionRange) { e.state = ENT_STALKING;
                Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x + dir.z*dir.z);
                if(len > 0.1f) { dir.x /= len; dir.z /= len;
                    if(e.behaviorMode == BEHAVIOR_FLANK){
                        float ox = dir.x;
                        dir.x = dir.z;
                        dir.z = -ox;
                    }
                    float nx = e.pos.x + dir.x*e.speed*speedMul*dt, nz = e.pos.z + dir.z*e.speed*speedMul*dt;
                    if(!collideWorld(nx,nz,0.32f)) { e.pos.x = nx; e.pos.z = nz; }
                    else {
                        float sx = e.pos.x + dir.z*e.speed*0.65f*speedMul*dt;
                        float sz = e.pos.z - dir.x*e.speed*0.65f*speedMul*dt;
                        if(!collideWorld(sx,sz,0.32f)) { e.pos.x = sx; e.pos.z = sz; }
                    }
                    e.yaw = atan2f(dir.x, dir.z);
                }
            } else { e.state = ENT_ROAMING;
                if(e.stateTimer > 2.3f) { e.yaw += ((rand()%100)/50.0f-1.0f)*2.2f; e.stateTimer = 0; }
                float nx = e.pos.x + mSin(e.yaw)*e.speed*0.3f*speedMul*dt, nz = e.pos.z + mCos(e.yaw)*e.speed*0.3f*speedMul*dt;
                if(!collideWorld(nx,nz,0.32f)) { e.pos.x = nx; e.pos.z = nz; }
                else e.yaw += 1.2f;
            }
        }
        if(!looking && dist > 10.0f && dist < 22.0f && (rand()%1000) < 10){
            Vec3 dir = pPos - e.pos;
            dir.y = 0.0f;
            float len = dir.len();
            if(len > 0.1f){
                dir = dir * (1.0f / len);
                float nx = pPos.x - dir.x * (3.0f + (float)(rand()%4));
                float nz = pPos.z - dir.z * (3.0f + (float)(rand()%4));
                if(!collideWorld(nx, nz, 0.32f)){
                    e.pos.x = nx;
                    e.pos.z = nz;
                    e.behaviorMode = BEHAVIOR_RUSH;
                }
            }
        }
        if(e.state == ENT_FLEEING) { Vec3 dir = e.pos - pPos; dir.y = 0; float len = sqrtf(dir.x*dir.x+dir.z*dir.z);
            if(len > 0.1f) {
                dir.x/=len; dir.z/=len;
                float nx = e.pos.x + dir.x*e.speed*2.2f*dt, nz = e.pos.z + dir.z*e.speed*2.2f*dt;
                if(!collideWorld(nx,nz,0.32f)) { e.pos.x = nx; e.pos.z = nz; }
            }
            if(e.stateTimer > 5.0f || dist > 25.0f) e.active = false; }
        if(dist < e.attackRange && e.state == ENT_STALKING) e.state = ENT_ATTACKING;
    }
    
    void updateCrawler(Entity& e, float dt, Vec3 pPos, float dist, float effectiveDetect, int maze[][30], int mzw, int mzh, float cs) {
        (void)maze; (void)mzw; (void)mzh; (void)cs;
        e.stateTimer += dt; e.pos.y = 0.58f; // Keep body above floor
        float speedMul = 1.0f;
        if(e.behaviorMode == BEHAVIOR_RUSH) speedMul = 1.70f;
        else if(e.behaviorMode == BEHAVIOR_FLANK) speedMul = 1.15f;
        if(dist < 9.0f && e.stateTimer > 1.2f && e.behaviorMode != BEHAVIOR_RUSH){
            e.state = ENT_IDLE;
            if(e.stateTimer > 2.1f) {
                e.behaviorMode = BEHAVIOR_RUSH;
                e.stateTimer = 0.0f;
            }
        }
        if(dist < effectiveDetect) { e.state = ENT_CHASING;
            Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x + dir.z*dir.z);
            if(len > 0.1f) { dir.x /= len; dir.z /= len;
                float burstMul = 1.0f;
                if(e.behaviorMode == BEHAVIOR_RUSH){
                    if(e.stateTimer < 0.55f) burstMul = 2.25f;
                    else if(e.stateTimer < 1.10f) burstMul = 1.55f;
                }
                float moveX = dir.x;
                float moveZ = dir.z;
                if(e.behaviorMode == BEHAVIOR_FLANK){
                    float zig = sinf(e.animPhase * 2.4f) * 0.55f;
                    moveX = dir.x + (-dir.z) * zig;
                    moveZ = dir.z + dir.x * zig;
                    float mLen = sqrtf(moveX*moveX + moveZ*moveZ);
                    if(mLen > 0.1f){ moveX /= mLen; moveZ /= mLen; }
                }
                float nx = e.pos.x + moveX*e.speed*speedMul*burstMul*dt, nz = e.pos.z + moveZ*e.speed*speedMul*burstMul*dt;
                if(!collideWorld(nx,nz,0.34f)) { e.pos.x = nx; e.pos.z = nz; }
                else {
                    float bx = e.pos.x - moveZ*e.speed*0.7f*speedMul*dt;
                    float bz = e.pos.z + moveX*e.speed*0.7f*speedMul*dt;
                    if(!collideWorld(bx,bz,0.34f)) { e.pos.x = bx; e.pos.z = bz; }
                }
                e.yaw = atan2f(moveX, moveZ); }
            if(e.behaviorMode == BEHAVIOR_RUSH && e.stateTimer > 1.35f){
                e.behaviorMode = BEHAVIOR_DEFAULT;
                e.stateTimer = 0.0f;
            }
            if(dist < e.attackRange) e.state = ENT_ATTACKING;
        } else { e.state = ENT_ROAMING;
            if(e.stateTimer > 1.8f) { e.yaw += ((rand()%100)/50.0f-1.0f)*2.6f; e.stateTimer = 0; }
            float nx = e.pos.x + mSin(e.yaw)*e.speed*0.2f*speedMul*dt, nz = e.pos.z + mCos(e.yaw)*e.speed*0.2f*speedMul*dt;
            if(!collideWorld(nx,nz,0.34f)) { e.pos.x = nx; e.pos.z = nz; }
            else e.yaw += 1.3f; }
    }
    
    void updateShadow(Entity& e, float dt, Vec3 pPos, float dist, bool looking, float pYaw) {
        if(looking && dist < 13.0f){
            e.lastSeenTimer += dt;
            if(e.lastSeenTimer > 0.8f){
                for(int i=0;i<8;i++){
                    float ang = ((float)(rand()%628) / 100.0f);
                    float r = 6.5f + (float)(rand()%7);
                    float nx = pPos.x + mSin(ang) * r;
                    float nz = pPos.z + mCos(ang) * r;
                    if(!collideWorld(nx,nz,0.32f)){
                        e.pos.x = nx; e.pos.z = nz;
                        e.visible = false;
                        break;
                    }
                }
                e.lastSeenTimer = 0.0f;
            }
        }else{
            e.lastSeenTimer = 0.0f;
        }
        if(!looking && dist < e.detectionRange + 8.0f && (rand()%1000) < 9){
            float jitter = ((float)(rand()%628) / 100.0f);
            Vec3 backDir(-mSin(pYaw), 0.0f, -mCos(pYaw));
            Vec3 sideDir(mCos(pYaw), 0.0f, -mSin(pYaw));
            float backDist = 2.8f + (float)(rand()%4);
            float sideDist = (mSin(jitter) > 0.0f ? 1.0f : -1.0f) * (0.9f + (float)(rand()%10) * 0.1f);
            float nx = pPos.x + backDir.x * backDist + sideDir.x * sideDist;
            float nz = pPos.z + backDir.z * backDist + sideDir.z * sideDist;
            if(!collideWorld(nx,nz,0.32f)){
                e.pos.x = nx;
                e.pos.z = nz;
                e.visible = false;
            }
        }
        e.visible = !looking && (dist < e.detectionRange + 8.0f);
        if(!looking && dist < e.detectionRange + 3.0f && dist > e.attackRange) {
            Vec3 dir = pPos - e.pos; dir.y = 0; float len = sqrtf(dir.x*dir.x+dir.z*dir.z);
            if(len > 0.1f) {
                dir.x/=len; dir.z/=len;
                float moveMul = (e.behaviorMode == BEHAVIOR_SNEAK) ? 0.72f : 1.75f;
                if(e.behaviorMode == BEHAVIOR_RUSH){
                    moveMul = 2.25f;
                }
                if(e.behaviorMode == BEHAVIOR_FLANK || dist < 6.5f){
                    float ox = dir.x;
                    dir.x = dir.z;
                    dir.z = -ox;
                    if((rand()%100) < 45){ dir.x = -dir.x; dir.z = -dir.z; }
                    moveMul = (dist < 6.5f) ? 1.65f : 1.35f;
                }
                float nx = e.pos.x + dir.x*e.speed*moveMul*dt;
                float nz = e.pos.z + dir.z*e.speed*moveMul*dt;
                if(!collideWorld(nx,nz,0.32f)){ e.pos.x = nx; e.pos.z = nz; }
                e.yaw = atan2f(dir.x,dir.z);
            }
        }
        if(dist < 11.0f && !looking && (rand()%1000) < 12){
            for(int i=0;i<8;i++){
                float ang = ((float)(rand()%628) / 100.0f);
                float r = 2.4f + (float)(rand()%4);
                float nx = pPos.x + mSin(ang) * r;
                float nz = pPos.z + mCos(ang) * r;
                if(!collideWorld(nx,nz,0.32f)){
                    e.pos.x = nx;
                    e.pos.z = nz;
                    break;
                }
            }
        }
        if(dist > 32.0f) e.active = false;
    }
    
    void render(GLuint shader, Mat4& proj, Mat4& view) {
        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader,"P"),1,GL_FALSE,proj.m);
        glUniformMatrix4fv(glGetUniformLocation(shader,"V"),1,GL_FALSE,view.m);
        for(auto& e : entities) {
            if(!e.active || !e.visible) continue;
            Mat4 model; float c=mCos(e.yaw), s=mSin(e.yaw);
            model.m[0]=c; model.m[2]=s; model.m[8]=-s; model.m[10]=c;
            float bobAmp = 0.02f;
            float yOffset = e.pos.y;
            if(e.type==ENTITY_CRAWLER){
                bobAmp = 0.03f;
                model.m[0] = c * 0.95f;
                model.m[2] = s * 0.95f;
                model.m[8] = -s * 1.15f;
                model.m[10] = c * 1.15f;
            }else if(e.type==ENTITY_SHADOW){
                bobAmp = 0.06f;
                yOffset += 0.08f * mSin(e.animPhase * 0.7f);
            }
            model.m[12]=e.pos.x + mSin(e.animPhase) * bobAmp;
            model.m[13]=yOffset;
            model.m[14]=e.pos.z;
            glUniformMatrix4fv(glGetUniformLocation(shader,"M"),1,GL_FALSE,model.m);
            int ti = (int)e.type;
            if(ti < 0 || ti > 3 || entityVC[ti] <= 0) continue;
            glBindTexture(GL_TEXTURE_2D, entityTex[ti]);
            glBindVertexArray(entityVAO[ti]);
            glDrawArrays(GL_TRIANGLES, 0, entityVC[ti]);
        }
    }
    
    EntityType getAttackingType(Vec3 pPos) {
        float bestDist = 1e9f;
        EntityType best = ENTITY_NONE;
        for(auto& e : entities) {
            if(!e.active || e.state != ENT_ATTACKING) continue;
            Vec3 d = e.pos - pPos; d.y = 0;
            float dist = sqrtf(d.x*d.x + d.z*d.z);
            if(dist >= e.attackRange || dist >= bestDist) continue;
            bestDist = dist;
            best = e.type;
        }
        return best;
    }

    bool checkPlayerAttack(Vec3 pPos) {
        return getAttackingType(pPos) != ENTITY_NONE;
    }
    
    void reset() { entities.clear(); spawnTimer = 0; dangerLevel = 0; hasLastPlayerPos = false; }
};

#endif
