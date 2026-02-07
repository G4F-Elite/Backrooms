#ifndef ENTITY_TYPES_H
#define ENTITY_TYPES_H

#include "math.h"

enum EntityType { ENTITY_NONE = 0, ENTITY_STALKER, ENTITY_CRAWLER, ENTITY_SHADOW };
enum EntityState { ENT_IDLE, ENT_ROAMING, ENT_STALKING, ENT_CHASING, ENT_FLEEING, ENT_ATTACKING };

struct Entity {
    Vec3 pos, velocity;
    float yaw;
    EntityType type;
    EntityState state;
    float stateTimer, animPhase, health, speed, detectionRange, attackRange;
    bool active, visible;
    float lastSeenTimer, flickerTimer;
    
    Entity() : pos(0,0,0), velocity(0,0,0), yaw(0), type(ENTITY_NONE), state(ENT_IDLE),
               stateTimer(0), animPhase(0), health(100), speed(2.0f), detectionRange(15.0f),
               attackRange(1.5f), active(false), visible(true), lastSeenTimer(0), flickerTimer(0) {}
};

#endif