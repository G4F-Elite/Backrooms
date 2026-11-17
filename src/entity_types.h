#ifndef ENTITY_TYPES_H
#define ENTITY_TYPES_H

#include "math.h"
#include "ai_nav.h"

enum EntityType { ENTITY_NONE = 0, ENTITY_STALKER, ENTITY_CRAWLER, ENTITY_SHADOW };
enum EntityState { ENT_IDLE, ENT_ROAMING, ENT_STALKING, ENT_CHASING, ENT_FLEEING, ENT_ATTACKING };
enum EntityBehaviorMode { BEHAVIOR_DEFAULT = 0, BEHAVIOR_SNEAK = 1, BEHAVIOR_RUSH = 2, BEHAVIOR_FLANK = 3 };

// What the monster is doing right now, in words a spectator can read. Replicated to clients
// in the upper bits of the snapshot state byte (see packEntityState).
enum EntityMood {
    MOOD_WANDER = 0,   // nothing on its mind
    MOOD_LISTEN,       // crawler: heard something, about to charge
    MOOD_INVESTIGATE,  // walking to a place it suspects
    MOOD_HUNT,         // knows where somebody is
    MOOD_FROZEN,       // stalker: somebody is looking
    MOOD_CHARGE,       // crawler: committed run
    MOOD_STUNNED,      // crawler: ran head first into something
    MOOD_LURK,         // shadow: sneaking up on a loner
    MOOD_RETREAT,      // shadow: was glanced at / monsters sent away by the director
    MOOD_ENRAGED,      // shadow: was stared at for too long
    MOOD_COUNT
};

inline unsigned char packEntityState(int state, int mood) {
    return (unsigned char)((state & 7) | ((mood & 31) << 3));
}

inline int unpackEntityState(unsigned char packed) { return packed & 7; }
inline int unpackEntityMood(unsigned char packed) { return (packed >> 3) & 31; }

struct Entity {
    Vec3 pos, velocity;
    float yaw;
    EntityType type;
    EntityState state;
    EntityBehaviorMode behaviorMode;
    float stateTimer, animPhase, health, speed, detectionRange, attackRange;
    bool active, visible;
    float lastSeenTimer, flickerTimer, behaviorTimer;

    // Brain state (host / solo). See entity_brain.h.
    unsigned char uid = 0;       // stable id, used to match snapshots on clients
    unsigned char mood = MOOD_WANDER;
    float moodTimer = 0.0f;      // seconds in the current mood
    float animTime = 0.0f;       // idle animation clock; frozen monsters do not advance it
    float awareness = 0.0f;      // 0..1 evidence that somebody is around
    float watchedTimer = 0.0f;   // seconds of being stared at
    float unseenTimer = 0.0f;    // seconds since anybody last saw it
    float patience = 0.0f;       // shadow: boldness while the squad sticks together; crawler: sniffing time left
    float chargeLeft = 0.0f;     // crawler: meters left in the committed run
    float chargeRun = 0.0f;      // crawler: meters covered in this run
    Vec3 chargeDir;
    Vec3 goal;                   // where it wants to be
    bool hasGoal = false;
    int targetId = -1;           // squad member it is after
    NavCell path[NAV_PATH_KEEP] = {};
    NavCell pathGoal = {0, 0};
    int pathLen = 0, pathIdx = 0;
    float repathTimer = 0.0f, stuckTimer = 0.0f, sideSign = 1.0f;

    // Puppet state (network clients): where the host says it is.
    Vec3 netPos;
    float netYaw = 0.0f;
    bool netSeen = false;

    Entity() : pos(0,0,0), velocity(0,0,0), yaw(0), type(ENTITY_NONE), state(ENT_IDLE), behaviorMode(BEHAVIOR_DEFAULT),
               stateTimer(0), animPhase(0), health(100), speed(2.0f), detectionRange(15.0f),
               attackRange(1.5f), active(false), visible(true), lastSeenTimer(0), flickerTimer(0), behaviorTimer(0) {}
};

#endif
