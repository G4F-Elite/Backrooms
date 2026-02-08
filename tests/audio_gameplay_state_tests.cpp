#include <cassert>
#include <cmath>
#include <atomic>
#include <iostream>

#include "../src/audio.h"

SoundState sndState = {};
std::atomic<bool> audioRunning{false};
HANDLE hEvent = nullptr;

float rms(const short* data, int n) {
    if (n <= 0) return 0.0f;
    double acc = 0.0;
    for (int i = 0; i < n; i++) {
        float v = (float)data[i] / 32767.0f;
        acc += (double)v * (double)v;
    }
    return (float)sqrt(acc / (double)n);
}

void testGameplayAudioStateClamps() {
    updateGameplayAudioState(2.0f, -1.0f, 2.0f, 3.0f, -5.0f);
    assert(sndState.moveIntensity == 1.0f);
    assert(sndState.sprintIntensity == 0.0f);
    assert(sndState.lowStamina == 0.0f);
    assert(sndState.monsterProximity == 1.0f);
    assert(sndState.monsterMenace == 0.0f);
}

void testGameplayCuesIncreaseSignalEnergy() {
    short base[1024] = {};
    short stressed[1024] = {};

    sndState.masterVol = 0.9f;
    sndState.ambienceVol = 0.7f;
    sndState.sfxVol = 0.8f;
    sndState.voiceVol = 0.7f;
    sndState.musicVol = 0.6f;
    sndState.dangerLevel = 0.0f;
    sndState.sanityLevel = 1.0f;
    sndState.flashlightOn = 0.0f;
    sndState.stepTrig = false;
    updateGameplayAudioState(0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    fillAudio(base, 1024);
    float baseRms = rms(base, 1024);

    sndState.dangerLevel = 0.7f;
    sndState.sanityLevel = 0.4f;
    sndState.flashlightOn = 1.0f;
    sndState.stepTrig = true;
    updateGameplayAudioState(1.0f, 1.0f, 0.2f, 0.9f, 1.0f);
    fillAudio(stressed, 1024);
    float stressedRms = rms(stressed, 1024);

    assert(stressedRms > baseRms);
}

int main() {
    testGameplayAudioStateClamps();
    testGameplayCuesIncreaseSignalEnergy();
    std::cout << "All audio gameplay state tests passed.\n";
    return 0;
}
