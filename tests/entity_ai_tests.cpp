#include <cassert>
#include <iostream>

#include "../src/entity_ai.h"

void testEntityCapProgression() {
    assert(computeEntityCap(10.0f) == 1);
    assert(computeEntityCap(120.0f) == 2);
    assert(computeEntityCap(260.0f) == 3);
    assert(computeEntityCap(600.0f) == 4);
}

void testSpawnDelayLowerBound() {
    float d = computeEntitySpawnDelay(1000.0f, 3);
    assert(d >= 12.0f);
}

void testSpawnDelayUsesRoll() {
    float d1 = computeEntitySpawnDelay(100.0f, 1);
    float d2 = computeEntitySpawnDelay(100.0f, 8);
    assert(d2 > d1);
}

void testHasEntityNearPos() {
    std::vector<Entity> entities;
    Entity e;
    e.active = true;
    e.pos = Vec3(10, 0, 10);
    entities.push_back(e);
    assert(hasEntityNearPos(entities, Vec3(11, 0, 10), 2.0f));
    assert(!hasEntityNearPos(entities, Vec3(30, 0, 30), 2.0f));
}

void testChooseSpawnEntityType() {
    assert(chooseSpawnEntityType(50.0f, 0, 0) == ENTITY_STALKER);
    EntityType t = chooseSpawnEntityType(300.0f, 10, 90);
    assert(t == ENTITY_SHADOW);
}

int main() {
    testEntityCapProgression();
    testSpawnDelayLowerBound();
    testSpawnDelayUsesRoll();
    testHasEntityNearPos();
    testChooseSpawnEntityType();
    std::cout << "All entity AI tests passed.\n";
    return 0;
}
