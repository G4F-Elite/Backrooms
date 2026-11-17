#pragma once
// Squad toy voice: short synthesized one-shots for squeaks, shoves, callouts and respawns.
// Triggered from the game thread, rendered sample by sample from the audio thread.

#include <atomic>
#include <cmath>
#include "audio_synth.h"

enum ToySound {
    TOY_NONE = 0,
    TOY_SQUEAK = 1,
    TOY_SHOVE = 2,
    TOY_CALLOUT = 3,
    TOY_RESPAWN = 4
};

// A request waiting to be picked up by the audio thread. `kind` is written last.
struct ToyRequest {
    std::atomic<int> kind{TOY_NONE};
    std::atomic<float> vol{0.0f};
    std::atomic<float> pitch{1.0f};
};

struct ToyVoice {
    int kind = TOY_NONE;
    float vol = 0.0f;
    float pitch = 1.0f;
    float time = 0.0f;
    float phase = 0.0f;
    Svf body;
};

// Several sounds can overlap (a shove landing while someone squeaks), so nothing gets cut off.
struct ToyVoiceBank {
    static const int REQUESTS = 8, VOICES = 4;
    ToyRequest requests[REQUESTS];
    std::atomic<unsigned> writeIndex{0};
    std::atomic<int> waiting{0};
    ToyVoice voices[VOICES];
    AudioRng rng;
};

inline ToyVoiceBank gToyVoices;

inline void triggerToySound(int kind, float vol, float pitch = 1.0f) {
    if (vol <= 0.01f || kind == TOY_NONE) return;
    if (vol > 1.0f) vol = 1.0f;
    ToyRequest& r = gToyVoices.requests[gToyVoices.writeIndex.fetch_add(1) % ToyVoiceBank::REQUESTS];
    r.vol = vol;
    r.pitch = pitch;
    if (r.kind.exchange(kind) == TOY_NONE) gToyVoices.waiting.fetch_add(1);
}

inline float toySoundLength(int kind) {
    if (kind == TOY_SQUEAK) return 0.30f;
    if (kind == TOY_SHOVE) return 0.22f;
    if (kind == TOY_CALLOUT) return 0.26f;
    if (kind == TOY_RESPAWN) return 0.60f;
    return 0.0f;
}

// Instant frequency (Hz) and envelope for a toy sound at time t.
inline void toySoundShape(int kind, float t, float pitch, float& freq, float& env) {
    float len = toySoundLength(kind);
    float k = len > 0.0f ? t / len : 1.0f;
    if (k > 1.0f) k = 1.0f;
    float attack = t < 0.006f ? t / 0.006f : 1.0f;
    // Every sound ends on a short fade so a voice never stops mid-wave with a click.
    float tail = len - t < 0.012f ? (len - t) / 0.012f : 1.0f;
    if (tail < 0.0f) tail = 0.0f;
    if (kind == TOY_SQUEAK) {
        // Rubber toy: fast chirp up, sag back down.
        float bend = k < 0.35f ? k / 0.35f : 1.0f - (k - 0.35f) * 0.9f;
        freq = (780.0f + 760.0f * bend) * pitch;
        env = attack * (1.0f - k) * 0.50f;
    } else if (kind == TOY_SHOVE) {
        freq = (62.0f + 110.0f * expf(-t * 26.0f)) * pitch;
        env = attack * expf(-t * 15.0f) * 0.85f * tail;
    } else if (kind == TOY_CALLOUT) {
        freq = (k < 0.5f ? 660.0f : 880.0f) * pitch;
        // Each of the two notes gets its own little pluck.
        float note = k < 0.5f ? k * 2.0f : (k - 0.5f) * 2.0f;
        float pluck = note < 0.06f ? note / 0.06f : 1.0f - (note - 0.06f) * 0.55f;
        env = pluck * (1.0f - k * 0.5f) * 0.34f * tail;
    } else if (kind == TOY_RESPAWN) {
        float pos = k * 4.0f;
        int step = (int)pos;
        if (step > 3) step = 3;
        static const float notes[4] = {523.0f, 659.0f, 784.0f, 1046.0f};
        freq = notes[step] * pitch;
        float note = pos - (float)step;
        float pluck = note < 0.08f ? note / 0.08f : 1.0f - (note - 0.08f) * 0.6f;
        env = pluck * (1.0f - k * 0.5f) * 0.32f * tail;
    } else {
        freq = 0.0f;
        env = 0.0f;
    }
}

inline void toyStartVoice(int kind, float vol, float pitch) {
    ToyVoiceBank& bank = gToyVoices;
    // Take a free voice, or the one closest to finishing.
    ToyVoice* pick = &bank.voices[0];
    float bestLeft = 1e9f;
    for (ToyVoice& v : bank.voices) {
        float left = v.kind == TOY_NONE ? -1.0f : toySoundLength(v.kind) - v.time;
        if (left < bestLeft) {
            bestLeft = left;
            pick = &v;
        }
    }
    pick->kind = kind;
    pick->vol = vol;
    pick->pitch = pitch;
    pick->time = 0.0f;
    pick->phase = 0.0f;
    pick->body.reset();
    pick->body.set(kind == TOY_SHOVE ? 900.0f * pitch : 2600.0f, kind == TOY_SHOVE ? 1.0f : 1.6f);
}

inline float toyRenderVoice(ToyVoice& v, float dt) {
    float freq = 0.0f, env = 0.0f;
    toySoundShape(v.kind, v.time, v.pitch, freq, env);
    v.phase += freq * dt;
    if (v.phase >= 1.0f) v.phase -= floorf(v.phase);
    float p = AUDIO_TAU * v.phase;
    float noise = gToyVoices.rng.bi();
    float s = 0.0f;
    if (v.kind == TOY_SQUEAK) {
        // Reedy overtones plus air leaking through the squeaker.
        s = sinf(p) + sinf(p * 2.0f) * 0.38f + sinf(p * 3.0f) * 0.16f + v.body.bp(noise) * 0.10f;
    } else if (v.kind == TOY_SHOVE) {
        // Body hit: a soft low thump and the slap of fabric on top of it.
        float slap = v.body.bp(noise) * expf(-v.time * 55.0f) * 0.55f;
        s = sinf(p) + sinf(p * 2.0f) * 0.20f + slap;
    } else {
        // Toy xylophone: a clean note with a quick bright ping at the start.
        float noteLen = v.kind == TOY_CALLOUT ? 0.13f : 0.15f;
        float ping = sinf(p * 4.0f) * expf(-fmodf(v.time, noteLen) * 60.0f) * 0.20f;
        s = sinf(p) + sinf(p * 2.0f) * 0.22f + ping;
    }
    v.time += dt;
    if (v.time >= toySoundLength(v.kind)) v.kind = TOY_NONE;
    return s * env * v.vol;
}

// Audio thread: returns the next toy voice sample (0 while idle).
inline float toyVoiceSample(float dt) {
    ToyVoiceBank& bank = gToyVoices;
    if (bank.waiting.load() > 0) {
        for (ToyRequest& r : bank.requests) {
            int kind = r.kind.exchange(TOY_NONE);
            if (kind == TOY_NONE) continue;
            bank.waiting.fetch_sub(1);
            toyStartVoice(kind, r.vol, r.pitch);
        }
    }
    float sum = 0.0f;
    for (ToyVoice& v : bank.voices) {
        if (v.kind != TOY_NONE) sum += toyRenderVoice(v, dt);
    }
    return sum;
}
