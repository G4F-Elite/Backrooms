#pragma once

#include "entity_types.h"

struct MonsterAttackProfile {
    float healthDps;
    float sanityDps;
    float hitShake;
    float hitFlash;
    float scareVol;
};

inline float monsterTypeMixFromEntity(EntityType t){
    if(t == ENTITY_CRAWLER) return 0.5f;
    if(t == ENTITY_SHADOW) return 1.0f;
    return 0.0f;
}

// How much of a monster's attention the squad has, for the sound director.
inline float monsterAlertFromEntity(const Entity& e){
    if(e.state == ENT_ATTACKING || e.mood == MOOD_CHARGE || e.mood == MOOD_ENRAGED) return 1.0f;
    if(e.mood == MOOD_HUNT || e.state == ENT_CHASING) return 0.85f;
    if(e.mood == MOOD_LISTEN || e.mood == MOOD_LURK || e.mood == MOOD_FROZEN) return 0.55f;
    if(e.mood == MOOD_INVESTIGATE || e.state == ENT_STALKING) return 0.4f;
    return 0.0f;
}

inline int monsterCallFromAlert(float alert){
    if(alert >= 0.8f) return MONSTER_CALL_ALERT;
    return alert >= 0.35f ? MONSTER_CALL_NOTICE : MONSTER_CALL_NONE;
}

inline MonsterAttackProfile monsterAttackProfile(EntityType t){
    MonsterAttackProfile p{35.0f, 15.0f, 0.15f, 0.40f, 0.78f};
    if(t == ENTITY_STALKER) p = {31.0f, 22.0f, 0.09f, 0.52f, 0.64f};
    else if(t == ENTITY_CRAWLER) p = {43.0f, 10.0f, 0.24f, 0.24f, 0.86f};
    else if(t == ENTITY_SHADOW) p = {22.0f, 30.0f, 0.06f, 0.14f, 0.58f};
    return p;
}
