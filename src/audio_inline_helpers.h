#pragma once

inline float carpetStep(float t) {
    float thud = sinf(t * 55.0f) * expf(-t * 35.0f) * 1.2f;
    float rustle = sinf(t * 30.0f) * expf(-t * 50.0f) * 0.4f;
    return (thud + rustle) * 0.5f;
}

inline float clamp01Audio(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

inline void updateGameplayAudioState(float moveIntensity, float sprintIntensity, float staminaNorm, float monsterProximity, float monsterMenace, float monsterType=0.0f) {
    sndState.moveIntensity = clamp01Audio(moveIntensity);
    sndState.sprintIntensity = clamp01Audio(sprintIntensity);
    sndState.lowStamina = 1.0f - clamp01Audio(staminaNorm);
    sndState.monsterProximity = clamp01Audio(monsterProximity);
    sndState.monsterMenace = clamp01Audio(monsterMenace);
    sndState.monsterType = clamp01Audio(monsterType);
}

inline void triggerScare(float vol=0.8f, float timer=0.0f) { sndState.scareVol = vol; sndState.scareTimer = timer; }
inline void triggerMenuNavigateSound() {
    static int noteStep = 0;
    const float scale[6] = {1.0f, 1.059f, 1.122f, 1.189f, 1.122f, 1.059f};
    sndState.uiMovePitch = scale[noteStep % 6];
    noteStep++;
    sndState.uiMoveTrig = true;
}
inline void triggerMenuAdjustSound() {
    static int noteStep = 0;
    const float scale[5] = {0.944f, 1.0f, 1.059f, 1.122f, 1.189f};
    sndState.uiAdjustPitch = scale[noteStep % 5];
    noteStep++;
    sndState.uiAdjustTrig = true;
}
inline void triggerMenuConfirmSound() { sndState.uiConfirmTrig = true; }
inline void triggerMenuBackSound() {
    static int noteStep = 0;
    const float scale[4] = {1.0f, 0.944f, 0.891f, 0.944f};
    sndState.uiBackPitch = scale[noteStep % 4];
    noteStep++;
    sndState.uiBackTrig = true;
}
