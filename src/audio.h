#pragma once
// Procedural audio: everything the player hears is synthesized here, one stereo sample at a time.
//   audio_synth.h    - oscillators, filters, noise, reverb
//   audio_params.h   - game parameters, synth controls and the table of routes between them
//   audio_cues.h     - layered one-shots: scare, hits, warnings, monster calls
//   audio_director.h - threat stages and when a warning has to fire
//   audio_ambience.h - light hum, air, far-away events
//   audio_sfx.h      - footsteps, flashlight, menu
//   audio_threat.h   - tension drone, heartbeat, chase pulse, monster beds, breath, whispers, death
//   audio_toy.h      - squad squeaks, shoves and callouts
// The game thread only writes plain values into sndState; the audio thread reads them.

#include <windows.h>
#include <mmsystem.h>
#include <xmmintrin.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include "audio_dsp.h"
#include "audio_synth.h"
#include "audio_params.h"
#include "audio_cues.h"
#include "audio_director.h"
#include "audio_ambience.h"
#include "audio_sfx.h"
#include "audio_threat.h"
#include "audio_toy.h"

// Buffers hold BUF_LEN stereo frames, i.e. BUF_LEN * AUDIO_CHANNELS interleaved samples.
const int SAMP_RATE = 44100, BUF_COUNT = 3, BUF_LEN = 1024, AUDIO_CHANNELS = 2;

struct SoundState {
    // Mix levels (settings menu)
    float masterVol = 0.7f;
    float musicVol = 0.55f;
    float ambienceVol = 0.75f;
    float sfxVol = 0.7f;
    float voiceVol = 0.65f;
    float humVol = 0.15f;
    // World state
    float dangerLevel = 0.0f;
    float sanityLevel = 1.0f;
    float flashlightOn = 0.0f;
    bool deathMode = false;
    float healthLevel = 1.0f;
    float depthLevel = 0.0f;  // 0 first floor .. 1 as deep as it gets
    float pacing = 0.0f;      // AI director intensity
    // Player movement
    float crouch = 0.0f;
    bool stepTrig = false;
    float stepPitch = 1.0f;
    float moveIntensity = 0.0f;
    float sprintIntensity = 0.0f;
    float lowStamina = 0.0f;
    // Nearest monster
    float monsterProximity = 0.0f;
    float monsterMenace = 0.0f;
    float monsterType = 0.0f;
    float monsterPan = 0.0f;
    float monsterAlert = 0.0f; // 0 wandering .. 1 charging
    float monsterCrowd = 0.0f; // 0 none .. 1 four or more in earshot
    int monsterCall = 0;       // AudioMonsterCall of that monster
    // One-shots
    float scareVol = 0.0f;
    bool hitTrig = false;      // took damage this frame
    bool uiMoveTrig = false;
    bool uiAdjustTrig = false;
    bool uiConfirmTrig = false;
    float uiMovePitch = 1.0f;
    float uiAdjustPitch = 1.0f;
    bool scannerBeepTrig = false;
    float scannerBeepPitch = 1.0f;
    float scannerBeepVol = 0.5f;
};

extern SoundState sndState;
extern std::atomic<bool> audioRunning;
extern HANDLE hEvent;

inline float clamp01Audio(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

inline void updateGameplayAudioState(float moveIntensity, float sprintIntensity, float staminaNorm, float monsterProximity, float monsterMenace, float monsterType = 0.0f) {
    sndState.moveIntensity = clamp01Audio(moveIntensity);
    sndState.sprintIntensity = clamp01Audio(sprintIntensity);
    sndState.lowStamina = 1.0f - clamp01Audio(staminaNorm);
    sndState.monsterProximity = clamp01Audio(monsterProximity);
    sndState.monsterMenace = clamp01Audio(monsterMenace);
    sndState.monsterType = clamp01Audio(monsterType);
}

// The one place where game state becomes audio parameters.
inline void audioReadGame(const SoundState& s, float* p) {
    p[AP_DANGER] = s.dangerLevel;
    p[AP_INSANITY] = 1.0f - s.sanityLevel;
    p[AP_HEALTH_LOSS] = 1.0f - s.healthLevel;
    p[AP_LOW_STAMINA] = s.lowStamina;
    p[AP_MOVE] = s.moveIntensity;
    p[AP_SPRINT] = s.sprintIntensity;
    p[AP_CROUCH] = s.crouch;
    p[AP_DARK] = 1.0f - s.flashlightOn;
    p[AP_MONSTER_NEAR] = s.monsterProximity;
    p[AP_MONSTER_MENACE] = s.monsterMenace;
    p[AP_MONSTER_ALERT] = s.monsterAlert;
    p[AP_MONSTER_CROWD] = s.monsterCrowd;
    p[AP_DEPTH] = s.depthLevel;
    p[AP_PACING] = s.pacing;
}

// Volume sliders glide too, so dragging one never zippers.
struct BusVolumes {
    float master = 0.0f, bus[BUS_COUNT] = {};

    void update(const SoundState& s) {
        const float target[BUS_COUNT] = {s.ambienceVol, s.sfxVol, s.voiceVol, s.musicVol};
        master += (clamp01Audio(s.masterVol) - master) * 0.003f;
        for (int i = 0; i < BUS_COUNT; i++) bus[i] += (clamp01Audio(target[i]) - bus[i]) * 0.003f;
    }
};

struct AudioEngine {
    AudioDirector director;
    CuePlayer cues;
    AmbienceSynth ambience;
    SfxSynth sfx;
    ThreatSynth threat;
    StereoReverb reverb;
    BusVolumes vol;
    float scannerTime = -1.0f, scannerPitch = 1.0f, scannerVol = 0.5f;
    OnePole dcL, dcR, muffleL, muffleR;
    float limiterEnv = 0.0f;
};

inline AudioEngine gAudioEngine;

// Picks up the one-shot requests the game thread left in sndState.
inline void audioConsumeTriggers(AudioEngine& e) {
    if (sndState.stepTrig) {
        sndState.stepTrig = false;
        e.sfx.startStep(sndState.stepPitch, e.director.params.value[AP_SPRINT]);
    }
    if (sndState.scareVol > 0.0f) {
        e.cues.start(CUE_SCARE, sndState.scareVol * 1.15f);
        sndState.scareVol = 0.0f;
    }
    if (sndState.hitTrig) {
        sndState.hitTrig = false;
        e.director.noteHit();
    }
    gCueQueue.drain(e.cues);
    if (sndState.uiMoveTrig) {
        sndState.uiMoveTrig = false;
        e.sfx.uiMove.start(sndState.uiMovePitch, 0.8f, 1.2f);
    }
    if (sndState.uiAdjustTrig) {
        sndState.uiAdjustTrig = false;
        e.sfx.uiAdjust.start(sndState.uiAdjustPitch, 0.85f, 1.15f);
    }
    if (sndState.uiConfirmTrig) {
        sndState.uiConfirmTrig = false;
        e.sfx.uiConfirm.start(1.0f, 1.0f, 1.0f);
    }
    if (sndState.scannerBeepTrig) {
        sndState.scannerBeepTrig = false;
        e.scannerTime = 0.0f;
        e.scannerPitch = sndState.scannerBeepPitch;
        e.scannerVol = sndState.scannerBeepVol;
    }
}

// Renders `frames` stereo frames into buf (interleaved L/R, so buf holds frames * AUDIO_CHANNELS).
inline void fillAudio(short* buf, int frames) {
    AudioEngine& e = gAudioEngine;
    // Flush denormals to zero: decaying filters and reverb tails must not stall the audio thread.
    _mm_setcsr(_mm_getcsr() | 0x8040);

    for (int i = 0; i < frames; i++) {
        e.vol.update(sndState);
        audioConsumeTriggers(e);

        // Game state -> parameters -> synth controls, plus whatever warnings the moment calls for.
        MonsterInfo monster;
        monster.type = clamp01Audio(sndState.monsterType);
        monster.pan = sndState.monsterPan;
        monster.call = sndState.monsterCall;
        audioReadGame(sndState, e.director.params.target);
        e.director.tick(e.cues, monster);
        const float* p = e.director.params.value;
        const float* d = e.director.params.dest;

        AudioFrame bus[BUS_COUNT];
        e.ambience.render(sndState.humVol * d[AD_HUM], clamp01Audio(d[AD_STRESS]), bus[BUS_AMBIENCE]);
        bus[BUS_AMBIENCE].scale(d[AD_AMBIENCE_GAIN]);

        e.sfx.renderMovement(p[AP_MOVE], p[AP_SPRINT], d[AD_STEP], bus[BUS_SFX]);
        e.sfx.renderFlashlight(sndState.flashlightOn, bus[BUS_SFX]);
        float gadgets = toyVoiceSample(AUDIO_DT);
        if (e.scannerTime >= 0.0f) {
            gadgets += scannerBeepSample(e.scannerTime, e.scannerPitch, e.scannerVol);
            e.scannerTime += AUDIO_DT;
            if (e.scannerTime > 0.22f) e.scannerTime = -1.0f;
        }
        bus[BUS_SFX].addStereo(gadgets, gadgets, 0.30f);

        e.threat.renderBreath(clamp01Audio(d[AD_BREATH]), bus[BUS_VOICE]);
        e.threat.renderInsanity(clamp01Audio(d[AD_WHISPER]), bus[BUS_VOICE]);

        MonsterBed bed;
        bed.gain = d[AD_MONSTER];
        bed.proximity = p[AP_MONSTER_NEAR];
        bed.menace = p[AP_MONSTER_MENACE];
        bed.type = monster.type;
        bed.pan = monster.pan;
        e.threat.renderDrone(clamp01Audio(d[AD_DRONE]), bus[BUS_MUSIC]);
        e.threat.renderHeart(d[AD_HEART], d[AD_HEART_RATE], bus[BUS_MUSIC]);
        e.threat.renderChase(clamp01Audio(d[AD_CHASE]), bus[BUS_MUSIC]);
        e.threat.renderMonster(bed, bus[BUS_MUSIC]);

        // Loud one-shots push the beds down for a moment, so the hit lands on silence.
        AudioFrame cue[BUS_COUNT];
        float duck = e.cues.render(cue);
        bus[BUS_AMBIENCE].scale(1.0f - duck * 0.8f);
        bus[BUS_MUSIC].scale(1.0f - duck * 0.6f);
        for (int b = 0; b < BUS_COUNT; b++) bus[b].mix(cue[b], 1.0f);

        // Dying pushes the whole world under water and leaves only the death drone.
        AudioFrame death;
        float dead = e.threat.renderDeath(sndState.deathMode, death);
        bus[BUS_AMBIENCE].scale(1.0f - dead * 0.85f);
        bus[BUS_SFX].scale(1.0f - dead * 0.88f);
        bus[BUS_VOICE].scale(1.0f - dead * 0.92f);
        bus[BUS_MUSIC].scale(1.0f - dead);
        bus[BUS_MUSIC].mix(death, 1.0f);

        // Menu blips stay audible even with effects turned down.
        float ui = e.sfx.renderUi() * (0.45f + 0.55f * e.vol.bus[BUS_SFX]);

        AudioFrame out;
        for (int b = 0; b < BUS_COUNT; b++) out.mix(bus[b], e.vol.bus[b]);
        out.addStereo(ui, ui, 0.12f);
        out.scale(e.vol.master);

        float wetL = 0.0f, wetR = 0.0f;
        e.reverb.tick(out.wet * d[AD_REVERB], wetL, wetR);
        // Badly hurt: the world is heard through a wall.
        float open = aLerp(1.0f, 0.07f, clamp01Audio(d[AD_MUFFLE]));
        float left = e.dcL.hp(e.muffleL.lp(out.l + wetL, open), 0.003f);
        float right = e.dcR.hp(e.muffleR.lp(out.r + wetR, open), 0.003f);

        float peak = fabsf(left) > fabsf(right) ? fabsf(left) : fabsf(right);
        float gain = limiterGain(peak, e.limiterEnv);
        left = fastClamp(softClip(left * gain), -1.0f, 1.0f);
        right = fastClamp(softClip(right * gain), -1.0f, 1.0f);
        buf[i * AUDIO_CHANNELS] = (short)(left * 32767.0f);
        buf[i * AUDIO_CHANNELS + 1] = (short)(right * 32767.0f);
    }
}

// The audio thread picks the request up on its next sample.
inline void triggerScare(float vol = 0.8f) { sndState.scareVol = vol; }

// Call every frame the local player takes monster damage: the first frame is the scare,
// the following ones keep the mauling going.
inline void triggerMonsterHit() { sndState.hitTrig = true; }

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

void audioThread();
