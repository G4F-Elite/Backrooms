// Dev tool: renders the procedural audio engine to WAV files, one per gameplay situation,
// so sound changes can be listened to (and measured) without launching the game.
//
// Build (from repo root):
//   g++ -std=c++17 -O2 -o build/audio_preview.exe tools/audio_preview.cpp
// Run:  build/audio_preview.exe [outDir] [solo]     (default outDir: build/audio_preview)
//       "solo" mutes the room ambience in every take but the room ones, to hear each sound on its own.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/audio.h"

SoundState sndState = {};
std::atomic<bool> audioRunning{false};
HANDLE hEvent = nullptr;

static std::vector<short> gTake;
static bool gSolo = false;

// Renders `seconds` of audio with the current sndState and appends it to the take.
static void run(float seconds) {
    const int chunk = 256;
    short buf[chunk * AUDIO_CHANNELS];
    int frames = (int)(seconds * SAMP_RATE);
    while (frames > 0) {
        int n = frames < chunk ? frames : chunk;
        fillAudio(buf, n);
        gTake.insert(gTake.end(), buf, buf + n * AUDIO_CHANNELS);
        frames -= n;
    }
}

static void put32(FILE* f, unsigned v) { fwrite(&v, 4, 1, f); }
static void put16(FILE* f, unsigned short v) { fwrite(&v, 2, 1, f); }

static void save(const std::string& dir, const char* name) {
    std::string path = dir + "/" + name + ".wav";
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        printf("cannot write %s\n", path.c_str());
        return;
    }
    unsigned bytes = (unsigned)(gTake.size() * sizeof(short));
    fwrite("RIFF", 1, 4, f);
    put32(f, 36 + bytes);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 1);
    put16(f, AUDIO_CHANNELS);
    put32(f, SAMP_RATE);
    put32(f, SAMP_RATE * AUDIO_CHANNELS * 2);
    put16(f, AUDIO_CHANNELS * 2);
    put16(f, 16);
    fwrite("data", 1, 4, f);
    put32(f, bytes);
    fwrite(gTake.data(), 1, bytes, f);
    fclose(f);
    printf("%-18s %5.1f s\n", name, (float)gTake.size() / AUDIO_CHANNELS / SAMP_RATE);
    gTake.clear();
}

// Resets the game state and lets the previous take ring out.
static void calm(bool room = false) {
    sndState = SoundState();
    if (gSolo && !room) sndState.ambienceVol = 0.0f;
    run(3.0f);
    gTake.clear();
}

// Walks for `seconds`, dropping a footstep every `interval` like the head bob does in game.
static void walk(float seconds, float interval, bool sprint) {
    updateGameplayAudioState(sprint ? 1.0f : 0.62f, sprint ? 1.0f : 0.0f, 1.0f - sndState.lowStamina,
                             sndState.monsterProximity, sndState.monsterMenace, sndState.monsterType);
    int step = 0;
    for (float t = 0.0f; t < seconds; t += interval) {
        sndState.stepPitch = 0.88f + 0.05f * (float)((step++ * 7) % 6) + (sprint ? 0.10f : 0.0f);
        sndState.stepTrig = true;
        run(interval);
    }
    sndState.moveIntensity = 0.0f;
    sndState.sprintIntensity = 0.0f;
}

// A whole encounter: it wanders in from the left, hears something, finds the player,
// chases, lands a few hits, loses interest and leaves to the right.
static void monsterTake(float type) {
    calm();
    sndState.monsterType = type;
    sndState.monsterMenace = 0.6f;
    for (int i = 0; i <= 60; i++) {
        float k = (float)i / 60.0f;
        float close = k < 0.6f ? k / 0.6f : 1.0f - (k - 0.6f) / 0.4f;
        sndState.monsterProximity = close * 0.95f;
        sndState.monsterPan = k * 2.0f - 1.0f;
        sndState.dangerLevel = close * 0.7f;
        bool noticed = k > 0.25f && k < 0.8f, found = k > 0.4f && k < 0.7f;
        sndState.monsterCall = found ? MONSTER_CALL_ALERT : (noticed ? MONSTER_CALL_NOTICE : MONSTER_CALL_NONE);
        sndState.monsterAlert = found ? 1.0f : (noticed ? 0.5f : 0.0f);
        bool mauled = k > 0.56f && k < 0.64f;
        sndState.healthLevel = k > 0.6f ? 0.4f : 1.0f;
        for (int f = 0; f < 18; f++) {
            if (mauled) triggerMonsterHit();
            run(0.3f / 18.0f);
        }
    }
    run(2.0f);
}

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : "build/audio_preview";
    gSolo = argc > 2 && std::string(argv[2]) == "solo";
    CreateDirectoryA("build", nullptr);
    CreateDirectoryA(dir.c_str(), nullptr);

    calm(true);
    run(40.0f);
    save(dir, "01_room");

    calm();
    walk(5.0f, 0.78f, false);
    walk(4.0f, 0.52f, true);
    run(1.0f);
    save(dir, "02_footsteps");

    calm();
    for (int i = 0; i < 4; i++) {
        sndState.flashlightOn = (i % 2 == 0) ? 1.0f : 0.0f;
        run(0.7f);
    }
    for (int i = 0; i < 6; i++) {
        triggerMenuNavigateSound();
        run(0.25f);
    }
    for (int i = 0; i < 5; i++) {
        triggerMenuAdjustSound();
        run(0.18f);
    }
    triggerMenuConfirmSound();
    run(1.0f);
    for (int i = 0; i < 8; i++) {
        float signal = (float)i / 7.0f;
        sndState.scannerBeepPitch = 0.55f + signal;
        sndState.scannerBeepVol = 0.35f + signal * 0.55f;
        sndState.scannerBeepTrig = true;
        run(0.18f + 0.6f * (1.0f - signal));
    }
    save(dir, "03_gadgets_menu");

    calm();
    for (int kind = TOY_SQUEAK; kind <= TOY_RESPAWN; kind++) {
        triggerToySound(kind, 0.9f, 1.0f);
        run(1.0f);
    }
    triggerToySound(TOY_SHOVE, 0.9f, 0.45f);
    run(0.8f);
    triggerToySound(TOY_RESPAWN, 0.45f, 1.5f);
    run(1.2f);
    save(dir, "04_toys");

    calm();
    for (int i = 0; i <= 30; i++) {
        sndState.dangerLevel = (float)i / 30.0f;
        run(0.5f);
    }
    run(4.0f);
    save(dir, "05_danger_rising");

    monsterTake(0.0f);
    save(dir, "06_stalker");
    monsterTake(0.5f);
    save(dir, "07_crawler");
    monsterTake(1.0f);
    save(dir, "08_shadow");

    calm();
    run(0.5f);
    triggerScare(0.8f);
    run(4.5f);
    triggerScare(0.8f);
    run(4.5f);
    save(dir, "09_scare");

    calm();
    sndState.sanityLevel = 0.04f;
    run(14.0f);
    save(dir, "10_insanity");

    calm();
    sndState.lowStamina = 0.9f;
    walk(5.0f, 0.52f, true);
    run(5.0f);
    save(dir, "11_out_of_breath");

    calm();
    run(1.0f);
    sndState.deathMode = true;
    run(9.0f);
    save(dir, "12_death");

    calm(true);
    sndState.dangerLevel = 0.75f;
    sndState.sanityLevel = 0.3f;
    run(40.0f);
    save(dir, "13_room_stressed");

    // Every layered cue on its own, in the order of the AudioCue enum.
    calm();
    for (int cue = CUE_NONE + 1; cue < CUE_COUNT; cue++) {
        audioPostCue(cue, 1.0f);
        run(3.5f);
    }
    save(dir, "14_cues");

    // Hurt and deep down: muffled mix, racing heart, bigger room.
    calm(true);
    sndState.depthLevel = 1.0f;
    walk(4.0f, 0.78f, false);
    sndState.healthLevel = 0.15f;
    walk(6.0f, 0.78f, false);
    sndState.crouch = 1.0f;
    walk(4.0f, 1.0f, false);
    save(dir, "15_hurt_deep_crouch");
    return 0;
}
