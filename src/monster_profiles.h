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

inline MonsterAttackProfile monsterAttackProfile(EntityType t){
    MonsterAttackProfile p{35.0f, 15.0f, 0.15f, 0.40f, 0.78f};
    if(t == ENTITY_STALKER) p = {31.0f, 22.0f, 0.09f, 0.52f, 0.64f};
    else if(t == ENTITY_CRAWLER) p = {43.0f, 10.0f, 0.24f, 0.24f, 0.86f};
    else if(t == ENTITY_SHADOW) p = {22.0f, 30.0f, 0.06f, 0.14f, 0.58f};
    return p;
}
