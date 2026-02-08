#include <cassert>
#include <iostream>

#include "../src/scare_system.h"

void testStoryScareNotes() {
    assert(!isStoryScareNote(0));
    assert(isStoryScareNote(2));
    assert(isStoryScareNote(5));
    assert(isStoryScareNote(8));
    assert(isStoryScareNote(11));
}

void testStoryScareOneShotAndCooldown() {
    ScareSystemState state{};
    resetScareSystemState(state);
    assert(tryTriggerStoryScare(state, 2));
    assert(!tryTriggerStoryScare(state, 2));
    assert(!tryTriggerStoryScare(state, 5)); // blocked by cooldown
    state.cooldown = 0.0f;
    assert(tryTriggerStoryScare(state, 5));
}

void testRandomChanceByPhaseAndSanity() {
    assert(randomScareChancePercent(SCARE_PHASE_INTRO, 100.0f) == 0);
    assert(randomScareChancePercent(SCARE_PHASE_EXPLORATION, 100.0f) == 6);
    assert(randomScareChancePercent(SCARE_PHASE_SURVIVAL, 60.0f) > randomScareChancePercent(SCARE_PHASE_EXPLORATION, 100.0f));
    assert(randomScareChancePercent(SCARE_PHASE_DESPERATION, 20.0f) <= 45);
}

void testRandomTriggerFlow() {
    ScareSystemState state{};
    resetScareSystemState(state);
    state.randomTimer = 0.0f;
    state.cooldown = 0.0f;
    assert(tryTriggerRandomScare(state, 0.1f, SCARE_PHASE_DESPERATION, 10.0f, 0));
    assert(state.cooldown > 0.0f);
    assert(!tryTriggerRandomScare(state, 0.1f, SCARE_PHASE_DESPERATION, 10.0f, 0));
    state.cooldown = 0.0f;
    state.randomTimer = 0.0f;
    assert(!tryTriggerRandomScare(state, 0.1f, SCARE_PHASE_INTRO, 10.0f, 0));
}

int main() {
    testStoryScareNotes();
    testStoryScareOneShotAndCooldown();
    testRandomChanceByPhaseAndSanity();
    testRandomTriggerFlow();
    std::cout << "All scare system tests passed.\n";
    return 0;
}
