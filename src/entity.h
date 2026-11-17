#ifndef ENTITY_H
#define ENTITY_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <glad/glad.h>
#include "entity_types.h"
#include "entity_model.h"
#include "creature_anim.h"
#include "world.h"
#include "progression.h"
#include "ai_senses.h"
#include "ai_director.h"

// Backlog: "hearing" support for entities (crawler reacts to loud footsteps/sprinting)
// Values are set from gameplay/audio (see game_main_entry.h loop).
inline float gPlayerNoise = 0.0f; // 0..1

struct BrainContext;

class EntityManager {
public:
    std::vector<Entity> entities;
    GLuint entityVAO[4], entityVBO[4], entityTex[4];
    int entityVC[4];
    std::vector<CreaturePart> entityParts[4];
    float animClock = 0.0f; // drives idle motion; walk cycles come from travelled distance
    float spawnTimer, maxEntities, dangerLevel;
    Vec3 lastPlayerPos;
    bool hasLastPlayerPos;
    // Hunt targets (the host fills these each frame). Empty = hunt the local player.
    AiTarget targets[AI_MAX_TARGETS];
    int targetCount = 0;
    bool ignoreLocal = false; // local player is a ghost / protected while downed
    AiNoise noises[AI_MAX_NOISES];
    int noiseCount = 0;
    AiDirector director;
    DirectorOutput directorOut;
    bool puppetMode = false;   // network client: no brains, replay snapshots
    bool localGhost = false;   // local player is dead and spectating: no danger tint for them
    bool lightsOut = false;
    float attention01 = 0.0f;  // squad noise meter, 0..1
    float pendingDamage = 0.0f;
    int pendingDowns = 0;
    int squadStanding = 1, squadSize = 1;
    unsigned char nextUid = 1;
    bool bonkPending = false;  // a crawler just knocked itself out (for sound / HUD)
    Vec3 bonkPos;
    // Haven: a lit, running machine the things in the halls keep away from (the heist rig).
    // Nobody inside it can be seen or heard, and nothing walks into it. Radius 0 = no haven.
    // Around it lies a quiet ring: monsters with nobody to chase turn back there and none spawn there.
    Vec3 havenPos;
    float havenRadius = 0.0f;
    float quietRadius = 0.0f;

    void addTarget(const AiTarget& t) {
        if(targetCount >= AI_MAX_TARGETS) return;
        targets[targetCount++] = t;
    }

    void addTarget(const Vec3& pos, float yaw) {
        AiTarget t;
        t.pos = pos;
        t.yaw = yaw;
        t.id = targetCount;
        addTarget(t);
    }

    EntityManager() : spawnTimer(0), maxEntities(3), dangerLevel(0), lastPlayerPos(0,0,0), hasLastPlayerPos(false) {
        for(int i=0;i<4;i++){
            entityVAO[i] = 0;
            entityVBO[i] = 0;
            entityTex[i] = 0;
            entityVC[i] = 0;
        }
    }
    
    void init() {
        for(int ti = (int)ENTITY_STALKER; ti <= (int)ENTITY_SHADOW; ti++) {
            EntityType type = (EntityType)ti;
            CreatureMesh mesh;
            buildEntityModel(type, mesh);
            entityVC[ti] = mesh.vertexCount();
            entityParts[ti] = mesh.parts;
            glGenVertexArrays(1, &entityVAO[ti]);
            glGenBuffers(1, &entityVBO[ti]);
            glBindVertexArray(entityVAO[ti]);
            glBindBuffer(GL_ARRAY_BUFFER, entityVBO[ti]);
            glBufferData(GL_ARRAY_BUFFER, mesh.v.size() * sizeof(float), mesh.v.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,32,(void*)12); glEnableVertexAttribArray(1);
            glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,32,(void*)20); glEnableVertexAttribArray(2);

            const int texSize = 128;
            std::vector<unsigned char> td((size_t)texSize * texSize * 4);
            genEntityTextureByType(type, td.data(), texSize, texSize);
            glGenTextures(1, &entityTex[ti]);
            glBindTexture(GL_TEXTURE_2D, entityTex[ti]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,texSize,texSize,0,GL_RGBA,GL_UNSIGNED_BYTE,td.data());
        }
    }
    
    // Simulation lives in entity_update.h (brains: entity_behaviors.h).
    void spawnEntity(EntityType type, Vec3 pos, int maze[][30], int mzw, int mzh);
    bool spawnOutOfSight(EntityType type, const Vec3& around);
    void update(float dt, Vec3 localPos, float localYaw, int maze[][30], int mzw, int mzh, float cs);
    void emitNoise(const Vec3& pos, float radius, int sourceId);
    bool anyTargetSees(const Vec3& pos, float maxDist) const;
    bool pickHiddenEntrance(const Vec3& around, Vec3& out) const;
    void sendBackstage(Entity& e);

    // Network clients: entities are puppets driven by host snapshots.
    void updatePuppets(float dt, Vec3 localPos);
    void beginSnapshot();
    void applySnapshotEntity(int uid, int type, const Vec3& pos, float yaw, unsigned char packedState);
    void endSnapshot();

    void ageNoises(float dt);
    float computeMenace(bool& anyEngaged) const;
    void hintSomeMonster();
    void animateEntity(Entity& e, float dt);
    void addLocalDanger(const Entity& e, const Vec3& localPos);
    void noteMoodChange(const Entity& e, int prevMood);
    bool inHaven(const Vec3& pos) const;
    bool inQuiet(const Vec3& pos) const;
    int shelterTargets();
    void leaveHaven(Entity& e, const BrainContext& ctx, float speedMul, float patience);

    void render(GLuint shader, Mat4& proj, Mat4& view) {
        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader,"P"),1,GL_FALSE,proj.m);
        glUniformMatrix4fv(glGetUniformLocation(shader,"V"),1,GL_FALSE,view.m);
        GLint modelLoc = glGetUniformLocation(shader,"M");
        int index = 0;
        for(auto& e : entities) {
            index++;
            if(!e.active || !e.visible) continue;
            int ti = (int)e.type;
            if(ti < 1 || ti > 3 || entityVC[ti] <= 0) continue;
            // Models stand on the floor; the crawler's logical height is not a render offset.
            Vec3 base(e.pos.x, e.type == ENTITY_CRAWLER ? 0.0f : e.pos.y, e.pos.z);
            // Offset idle motion per entity so two of a kind never move in sync. Each entity runs
            // its own idle clock: it stops dead while a stalker is being watched.
            CreaturePose pose = entityPose(e, e.animTime + (float)index * 2.3f);
            const std::vector<CreaturePart>& parts = entityParts[ti];
            Mat4 mats[CREATURE_MAX_PARTS];
            creaturePartMatrices(parts, pose, creatureWorldMatrix(base, e.yaw), mats);
            glBindTexture(GL_TEXTURE_2D, entityTex[ti]);
            glBindVertexArray(entityVAO[ti]);
            for(size_t k = 0; k < parts.size() && k < (size_t)CREATURE_MAX_PARTS; k++) {
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,mats[k].m);
                glDrawArrays(GL_TRIANGLES, parts[k].first, parts[k].count);
            }
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
    
    void reset() {
        entities.clear();
        spawnTimer = 0;
        dangerLevel = 0;
        hasLastPlayerPos = false;
        havenRadius = 0.0f;
        quietRadius = 0.0f;
        targetCount = 0;
        noiseCount = 0;
        director.reset();
        directorOut = DirectorOutput();
        pendingDamage = 0.0f;
        pendingDowns = 0;
        bonkPending = false;
    }
};

#include "entity_update.h"

#endif
