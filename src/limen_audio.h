#pragma once
// LIMEN audio: joins the world-driven sound generator (limen_sound.h) to the game's unmodified
// procedural engine (`gAudioEngine`).
//   game thread   lmUpdateSpatialAudio(): measures the place the player stands in (LmSoundScene)
//   audio thread  fillAudio(): tunes the game's reverb and light hum from that scene, replaces the
//                 game's single "carpet" footstep with material footsteps, and mixes the generated
//                 lamps / room tone / draught / plant / drone / drips / creaks / echoes on top.
#include <atomic>
#include <mutex>
#include "limen_sound.h"

struct LmAudioBridge {
    std::mutex lock;
    LmSoundScene shared;            // guarded by lock
    unsigned stamp = 0;             // guarded by lock
    // game thread only
    LmSoundScene probe;
    float probeX = 1e9f, probeZ = 1e9f;
    int probeAge = 0;
    unsigned probeEpoch = 0;
    u32 probeSeed = 0; int probeLevel = 0;
    // audio thread only
    LmSoundSynth synth;
    unsigned seen = 0;
    float curFeedback = 0.84f, curDamp = 0.44f, curHum = 1.0f, curBed = 1.0f, dead = 0.0f;
    float mixBuf[4096];
};

inline LmAudioBridge gLmAudio;

// Called once per frame on the game thread with the player's position and view yaw.
inline void lmUpdateSpatialAudio(float px, float pz, float yaw, float cs) {
    LmAudioBridge& b = gLmAudio;
    // The full probe (rays, lamp search) only when the listener has actually moved, the world
    // changed (a lamp died behind the player's back, a new level), or a moment has passed.
    float mx = px - b.probeX, mz = pz - b.probeZ;
    bool moved = mx * mx + mz * mz > 0.5f * 0.5f;
    bool changed = b.probeEpoch != gLmSiteEpoch || b.probeSeed != gLimen.seed || b.probeLevel != gLimen.level;
    if (moved || changed || ++b.probeAge >= 20 || !b.probe.valid) {
        lmProbeSound(px, pz, cs, b.probe);
        b.probeX = px; b.probeZ = pz; b.probeAge = 0;
        b.probeEpoch = gLmSiteEpoch; b.probeSeed = gLimen.seed; b.probeLevel = gLimen.level;
    }
    b.probe.px = px; b.probe.pz = pz; b.probe.yaw = yaw;
    std::lock_guard<std::mutex> guard(b.lock);
    b.shared = b.probe;
    b.stamp++;
}

inline void fillAudio(short* buf, int frames) {
    LmAudioBridge& b = gLmAudio;
    LmSoundSynth& synth = b.synth;
    // Never wait for the game thread: if it is writing right now, the next buffer gets the scene.
    if (b.lock.try_lock()) {
        if (b.stamp != b.seen) { b.seen = b.stamp; synth.setScene(b.shared); }
        b.lock.unlock();
    }
    const LmSoundScene& sc = synth.sc;
    bool live = sc.valid && synth.stale < 0.6f;

    // The game's own reverb follows the measured reverberation time; its hum becomes the diffuse
    // bed of the far lamp lattice (the near tubes are voiced one by one by the generator).
    if (live) {
        const float glide = 0.08f;
        float fb = powf(10.0f, -3.0f * 0.0305f / sc.rt60);
        fb = fb < 0.66f ? 0.66f : (fb > 0.962f ? 0.962f : fb);
        b.curFeedback += (fb - b.curFeedback) * glide;
        b.curDamp += ((0.58f - 0.44f * lsClamp(1.2f - 4.0f * sc.absorb, 0.0f, 1.0f)) - b.curDamp) * glide;
        b.curHum += (sc.lampField * 0.6f - b.curHum) * glide;
        b.curBed += (sc.bed - b.curBed) * glide;
        gAudioEngine.reverb.feedback = b.curFeedback;
        gAudioEngine.reverb.damp = b.curDamp;
    }

    // Footsteps belong to the floor the generator laid: take the trigger before the game's synth does.
    if (sndState.stepTrig) {
        sndState.stepTrig = false;
        synth.startStep(sndState.stepPitch, gAudioEngine.director.params.value[AP_SPRINT]);
    }

    float savedHum = sndState.humVol, savedAmb = sndState.ambienceVol;
    sndState.humVol = savedHum * b.curHum;
    // The game's place-blind room bed steps back; the generated one carries the place.
    sndState.ambienceVol = savedAmb * 0.6f * b.curBed;
    legacyFillAudio(buf, frames);
    sndState.humVol = savedHum;
    sndState.ambienceVol = savedAmb;

    // Same sliders, same gameplay routing (crouch / sprint step level, panic ducking) as the game's buses.
    const float* d = gAudioEngine.director.params.dest;
    b.dead += ((sndState.deathMode ? 1.0f : 0.0f) - b.dead) * 0.05f;
    float master = gAudioEngine.vol.master;
    LsMix mix;
    mix.amb = master * clamp01Audio(savedAmb) * d[AD_AMBIENCE_GAIN] * (1.0f - 0.85f * b.dead);
    mix.sfx = master * gAudioEngine.vol.bus[BUS_SFX] * d[AD_STEP] * (1.0f - 0.88f * b.dead);
    mix.hum = savedHum / 0.15f * d[AD_HUM];

    while (frames > 0) {
        int n = frames > 2048 ? 2048 : frames;
        memset(b.mixBuf, 0, sizeof(float) * 2 * (size_t)n);
        synth.render(b.mixBuf, n, mix);
        for (int i = 0; i < n * 2; i++) {
            float x = (float)buf[i] * (1.0f / 32767.0f) + b.mixBuf[i];
            // The game's limiter has already run; keep the sum inside full scale without a hard edge.
            float a = fabsf(x);
            if (a > 0.80f) { a = 0.80f + 0.20f * tanhf((a - 0.80f) * 5.0f); x = x < 0.0f ? -a : a; }
            buf[i] = (short)(x * 32767.0f);
        }
        buf += n * 2;
        frames -= n;
    }
}
