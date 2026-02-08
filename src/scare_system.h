#pragma once

enum ScarePhase {
    SCARE_PHASE_INTRO = 0,
    SCARE_PHASE_EXPLORATION = 1,
    SCARE_PHASE_SURVIVAL = 2,
    SCARE_PHASE_DESPERATION = 3
};

struct ScareSystemState {
    bool storyTriggered[12];
    float randomTimer;
    float cooldown;
};

inline void resetScareSystemState(ScareSystemState& state) {
    for (int i = 0; i < 12; i++) state.storyTriggered[i] = false;
    state.randomTimer = 15.0f;
    state.cooldown = 0.0f;
}

inline bool isStoryScareNote(int noteId) {
    return noteId == 2 || noteId == 5 || noteId == 8 || noteId == 11;
}

inline float randomScareInterval(int phase) {
    if (phase == SCARE_PHASE_EXPLORATION) return 20.0f;
    if (phase == SCARE_PHASE_SURVIVAL) return 15.0f;
    if (phase == SCARE_PHASE_DESPERATION) return 10.0f;
    return 999.0f;
}

inline int randomScareChancePercent(int phase, float sanity) {
    if (phase == SCARE_PHASE_INTRO) return 0;
    int baseChance = 10;
    if (phase == SCARE_PHASE_SURVIVAL) baseChance = 18;
    else if (phase == SCARE_PHASE_DESPERATION) baseChance = 25;
    int sanityBonus = 0;
    if (sanity < 70.0f) sanityBonus += 6;
    if (sanity < 45.0f) sanityBonus += 8;
    if (sanity < 25.0f) sanityBonus += 10;
    int chance = baseChance + sanityBonus;
    if (chance > 55) chance = 55;
    return chance;
}

inline bool tryTriggerStoryScare(ScareSystemState& state, int noteId) {
    if (noteId < 0 || noteId >= 12) return false;
    if (!isStoryScareNote(noteId)) return false;
    if (state.storyTriggered[noteId]) return false;
    if (state.cooldown > 0.0f) return false;
    state.storyTriggered[noteId] = true;
    state.cooldown = 7.5f;
    return true;
}

inline bool tryTriggerRandomScare(ScareSystemState& state, float dt, int phase, float sanity, int rollPercent) {
    if (state.cooldown > 0.0f) {
        state.cooldown -= dt;
        if (state.cooldown < 0.0f) state.cooldown = 0.0f;
    }
    if (phase == SCARE_PHASE_INTRO) return false;
    state.randomTimer -= dt;
    if (state.randomTimer > 0.0f) return false;
    state.randomTimer = randomScareInterval(phase);
    if (state.cooldown > 0.0f) return false;
    if (rollPercent < 0) rollPercent = 0;
    if (rollPercent > 99) rollPercent = 99;
    if (rollPercent >= randomScareChancePercent(phase, sanity)) return false;
    state.cooldown = 9.0f;
    return true;
}
