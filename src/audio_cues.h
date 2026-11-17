#pragma once
// Layered one-shots. A cue is a short list of parts - a crack, a body, a voice, a tail - and each
// part is one row of numbers for one of five small generators. Starting a cue starts all its parts
// with a little random detune, so no two hits sound the same. Loud cues duck the beds under them.

#include <atomic>
#include "audio_synth.h"

enum CueLayerKind {
    LAYER_NOISE,  // filtered noise: cracks, hiss, rumble.  shape: 0 low-pass, 1 band-pass, 2 high-pass
    LAYER_THUMP,  // sine that drops in pitch: impacts.     shape: overdrive
    LAYER_METAL,  // struck/scraped resonators.             shape: how long it is excited, seconds
    LAYER_THROAT, // voiced source through formants.        q: mouth 0 closed..1 open, shape: roughness
    LAYER_CLICKS  // train of bony clicks, f = clicks/s.    q: click pitch in kHz, shape: irregularity
};

struct CueLayer {
    int kind;
    float delay, attack, hold, release; // seconds; release is the time to fade to nothing
    float f0, f1, glide;                // Hz at the start, Hz it settles on, seconds to get there
    float q, shape;
    float gain, pan, send, vary;        // vary: random pitch offset, as a fraction
};

enum AudioBus { BUS_AMBIENCE, BUS_SFX, BUS_VOICE, BUS_MUSIC, BUS_COUNT };

enum AudioCue {
    CUE_NONE = 0,
    CUE_SCARE,         // jump scare / first hit
    CUE_HIT,           // being mauled
    CUE_WARN_NEAR,     // stinger: something is close
    CUE_WARN_HUNT,     // stinger: it is after you
    CUE_NOTICE,        // a monster heard something
    CUE_CALL_STALKER,  // idle calls, heard from far away
    CUE_CALL_CRAWLER,
    CUE_CALL_SHADOW,
    CUE_ALERT_STALKER, // it found somebody
    CUE_ALERT_CRAWLER,
    CUE_ALERT_SHADOW,
    CUE_PLUSH,         // items in the player's hands
    CUE_BATTERY,
    CUE_SPRAY,
    CUE_COUNT
};

struct CueDef {
    const char* name;
    const CueLayer* layers;
    int count;
    int bus;
    float duck;     // how hard the beds are pushed down while it plays
    float cooldown; // seconds before it may start again
};

#define CUE_ROWS(a) a, (int)(sizeof(a) / sizeof(a[0]))

inline const CueDef& cueDef(int cue) {
    //   kind        delay  attack  hold   rel    f0     f1    glide   q     shape  gain   pan   send  vary
    static const CueLayer scare[] = {
        {LAYER_NOISE,  0.0f,  0.0005f, 0.012f, 0.16f, 9000,  700,  0.030f, 0.7f, 0.0f,  1.00f, 0.0f,  0.5f, 0.10f},
        {LAYER_THUMP,  0.0f,  0.001f,  0.04f,  0.80f, 130,   36,   0.050f, 0.0f, 0.9f,  0.80f, 0.0f,  0.2f, 0.05f},
        {LAYER_METAL,  0.0f,  0.0005f, 0.30f,  2.60f, 137,   137,  0.0f,   420.0f, 0.006f, 0.50f, -0.3f, 1.0f, 0.08f},
        {LAYER_METAL,  0.01f, 0.010f,  0.55f,  1.10f, 1160,  1090, 0.60f,  110.0f, 0.55f, 0.20f, 0.4f,  0.8f, 0.10f},
        {LAYER_THROAT, 0.0f,  0.004f,  0.50f,  0.90f, 560,   400,  0.50f,  0.95f, 0.85f, 0.42f, -0.4f, 0.7f, 0.06f},
        {LAYER_THROAT, 0.012f, 0.004f, 0.42f,  0.80f, 790,   545,  0.45f,  1.0f, 0.95f, 0.30f, 0.4f,  0.7f, 0.06f},
        {LAYER_NOISE,  0.0f,  0.020f,  0.70f,  2.60f, 95,    60,   1.0f,   0.8f, 0.0f,  0.40f, 0.0f,  0.5f, 0.0f},
    };
    static const CueLayer hit[] = {
        {LAYER_THUMP,  0.0f,  0.001f,  0.015f, 0.22f, 170,   52,   0.030f, 0.0f, 1.0f,  0.75f, 0.0f,  0.15f, 0.12f},
        {LAYER_NOISE,  0.0f,  0.0005f, 0.010f, 0.10f, 900,   500,  0.03f,  1.6f, 1.0f,  0.55f, 0.0f,  0.2f, 0.25f},
        {LAYER_THROAT, 0.02f, 0.003f,  0.10f,  0.22f, 150,   88,   0.12f,  0.45f, 1.0f, 0.36f, 0.2f,  0.3f, 0.15f},
    };
    static const CueLayer warnNear[] = {
        {LAYER_METAL,  0.0f,  0.0005f, 0.25f,  2.40f, 98,    98,   0.0f,   380.0f, 0.004f, 0.55f, 0.0f, 0.9f, 0.04f},
        {LAYER_THUMP,  0.0f,  0.001f,  0.03f,  0.55f, 82,    44,   0.06f,  0.0f, 0.5f,  0.55f, 0.0f,  0.2f, 0.0f},
        {LAYER_NOISE,  0.0f,  0.002f,  0.16f,  0.70f, 3900,  5200, 0.30f,  14.0f, 1.0f, 0.32f, 0.5f,  0.8f, 0.08f},
        {LAYER_NOISE,  0.03f, 0.002f,  0.12f,  0.60f, 4150,  5500, 0.30f,  14.0f, 1.0f, 0.26f, -0.5f, 0.8f, 0.08f},
    };
    static const CueLayer warnHunt[] = {
        {LAYER_NOISE,  0.0f,  0.0005f, 0.010f, 0.12f, 7000,  900,  0.03f,  0.7f, 0.0f,  0.60f, 0.0f,  0.4f, 0.1f},
        {LAYER_THUMP,  0.0f,  0.001f,  0.03f,  0.50f, 110,   40,   0.04f,  0.0f, 0.9f,  0.75f, 0.0f,  0.2f, 0.0f},
        {LAYER_THUMP,  0.17f, 0.001f,  0.03f,  0.70f, 100,   36,   0.05f,  0.0f, 0.9f,  0.80f, 0.0f,  0.2f, 0.0f},
        {LAYER_THROAT, 0.17f, 0.004f,  0.55f,  1.30f, 58,    55,   0.5f,   0.30f, 0.55f, 0.50f, -0.4f, 0.5f, 0.0f},
        {LAYER_THROAT, 0.17f, 0.004f,  0.55f,  1.30f, 61.6f, 58.3f, 0.5f,  0.35f, 0.55f, 0.50f, 0.4f,  0.5f, 0.0f},
        {LAYER_METAL,  0.17f, 0.004f,  0.35f,  0.90f, 640,   610,  0.5f,   90.0f, 0.35f, 0.16f, 0.0f,  0.9f, 0.08f},
    };
    static const CueLayer notice[] = {
        {LAYER_THROAT, 0.0f,  0.004f,  0.10f,  0.22f, 150,   235,  0.10f,  0.45f, 0.7f, 0.50f, 0.0f,  0.5f, 0.12f},
        {LAYER_CLICKS, 0.05f, 0.001f,  0.22f,  0.10f, 22,    9,    0.15f,  2.3f, 0.5f,  0.50f, 0.0f,  0.5f, 0.15f},
    };
    static const CueLayer callStalker[] = {
        {LAYER_THROAT, 0.0f,  0.005f,  0.40f,  0.70f, 64,    47,   0.30f,  0.25f, 1.0f, 0.60f, 0.0f,  0.4f, 0.12f},
        {LAYER_NOISE,  0.0f,  0.008f,  0.30f,  0.60f, 620,   380,  0.40f,  2.5f, 1.0f,  0.50f, 0.0f,  0.5f, 0.15f},
        {LAYER_THUMP,  0.0f,  0.002f,  0.02f,  0.30f, 70,    42,   0.05f,  0.0f, 0.4f,  0.35f, 0.0f,  0.2f, 0.1f},
    };
    static const CueLayer callCrawler[] = {
        {LAYER_CLICKS, 0.0f,  0.001f,  0.50f,  0.15f, 36,    11,   0.30f,  2.4f, 0.6f,  0.70f, 0.0f,  0.4f, 0.15f},
        {LAYER_NOISE,  0.0f,  0.004f,  0.25f,  0.35f, 260,   170,  0.20f,  2.0f, 1.0f,  0.85f, 0.0f,  0.3f, 0.2f},
        {LAYER_THROAT, 0.30f, 0.003f,  0.07f,  0.14f, 520,   390,  0.08f,  0.7f, 0.9f,  0.22f, 0.0f,  0.5f, 0.2f},
    };
    static const CueLayer callShadow[] = {
        {LAYER_NOISE,  0.0f,  0.004f,  0.22f,  0.55f, 520,   1900, 0.22f,  5.0f, 1.0f,  0.80f, 0.0f,  0.8f, 0.15f},
        {LAYER_THROAT, 0.05f, 0.010f,  0.45f,  1.00f, 196,   185,  0.6f,   0.75f, 0.15f, 0.30f, -0.3f, 0.9f, 0.02f},
        {LAYER_THROAT, 0.05f, 0.010f,  0.45f,  1.00f, 207.6f, 196, 0.6f,   0.80f, 0.15f, 0.30f, 0.3f,  0.9f, 0.02f},
        {LAYER_THUMP,  0.0f,  0.004f,  0.10f,  0.60f, 46,    38,   0.2f,   0.0f, 0.2f,  0.40f, 0.0f,  0.2f, 0.0f},
    };
    static const CueLayer alertStalker[] = {
        {LAYER_THROAT, 0.0f,  0.003f,  0.65f,  0.80f, 104,   58,   0.45f,  0.80f, 1.0f, 0.62f, -0.2f, 0.5f, 0.08f},
        {LAYER_THROAT, 0.0f,  0.003f,  0.55f,  0.70f, 152,   85,   0.40f,  0.95f, 1.0f, 0.40f, 0.2f,  0.5f, 0.08f},
        {LAYER_THUMP,  0.0f,  0.001f,  0.03f,  0.50f, 95,    40,   0.05f,  0.0f, 0.8f,  0.60f, 0.0f,  0.2f, 0.0f},
        {LAYER_NOISE,  0.0f,  0.002f,  0.45f,  0.60f, 1700,  1100, 0.50f,  1.5f, 1.0f,  0.40f, 0.0f,  0.5f, 0.1f},
    };
    static const CueLayer alertCrawler[] = {
        {LAYER_THROAT, 0.0f,  0.002f,  0.42f,  0.35f, 410,   930,  0.30f,  0.95f, 0.75f, 0.50f, 0.0f, 0.6f, 0.08f},
        {LAYER_THROAT, 0.0f,  0.002f,  0.38f,  0.35f, 585,   1310, 0.33f,  1.0f, 0.85f, 0.30f, 0.3f,  0.6f, 0.08f},
        {LAYER_CLICKS, 0.0f,  0.001f,  0.70f,  0.20f, 30,    64,   0.50f,  2.6f, 0.4f,  0.60f, -0.2f, 0.4f, 0.1f},
        {LAYER_METAL,  0.0f,  0.004f,  0.30f,  0.60f, 2050,  2300, 0.4f,   50.0f, 0.30f, 0.12f, 0.0f, 0.8f, 0.1f},
    };
    static const CueLayer alertShadow[] = {
        {LAYER_THROAT, 0.0f,  0.003f,  0.85f,  1.50f, 233,   220,  0.8f,   1.0f, 0.5f,  0.36f, -0.5f, 0.9f, 0.02f},
        {LAYER_THROAT, 0.0f,  0.003f,  0.85f,  1.50f, 311,   330,  0.8f,   1.0f, 0.5f,  0.30f, 0.5f,  0.9f, 0.02f},
        {LAYER_THROAT, 0.02f, 0.003f,  0.80f,  1.40f, 466,   440,  0.8f,   0.9f, 0.6f,  0.24f, 0.0f,  0.9f, 0.02f},
        {LAYER_NOISE,  0.0f,  0.002f,  0.30f,  0.80f, 1800,  6500, 0.35f,  1.2f, 2.0f,  0.22f, 0.0f,  0.8f, 0.1f},
        {LAYER_THUMP,  0.0f,  0.002f,  0.08f,  0.90f, 62,    31,   0.15f,  0.0f, 0.7f,  0.65f, 0.0f,  0.2f, 0.0f},
    };
    // Squeezed: air squeals out of the squeaker, then wheezes back in, with stuffing rustling around it.
    static const CueLayer plush[] = {
        {LAYER_THUMP,  0.0f,  0.008f,  0.10f,  0.10f, 980,   1560, 0.05f,  0.0f, 0.5f,  0.42f, 0.0f,  0.25f, 0.06f},
        {LAYER_THUMP,  0.0f,  0.008f,  0.10f,  0.10f, 1960,  3120, 0.05f,  0.0f, 0.2f,  0.14f, 0.0f,  0.25f, 0.06f},
        {LAYER_THUMP,  0.24f, 0.015f,  0.12f,  0.14f, 1380,  860,  0.09f,  0.0f, 0.5f,  0.30f, 0.0f,  0.25f, 0.06f},
        {LAYER_NOISE,  0.24f, 0.020f,  0.10f,  0.16f, 2600,  1500, 0.10f,  2.5f, 1.0f,  0.30f, 0.0f,  0.2f, 0.1f},
        {LAYER_NOISE,  0.0f,  0.004f,  0.06f,  0.22f, 700,   350,  0.08f,  0.8f, 0.0f,  0.10f, 0.0f,  0.2f, 0.2f},
    };
    // Cap off, cell in, cap on - and the lamp whining back up.
    static const CueLayer battery[] = {
        {LAYER_METAL,  0.0f,  0.0005f, 0.02f,  0.10f, 2300,  2300, 0.0f,   25.0f, 0.002f, 0.07f, 0.1f, 0.25f, 0.1f},
        {LAYER_THUMP,  0.10f, 0.001f,  0.01f,  0.09f, 240,   120,  0.02f,  0.0f, 0.6f,  0.40f, 0.1f,  0.2f, 0.1f},
        {LAYER_METAL,  0.20f, 0.0005f, 0.02f,  0.12f, 1700,  1700, 0.0f,   30.0f, 0.002f, 0.08f, 0.1f, 0.25f, 0.1f},
        {LAYER_THUMP,  0.26f, 0.030f,  0.25f,  0.30f, 700,   2400, 0.22f,  0.0f, 0.0f,  0.07f, 0.1f,  0.3f, 0.0f},
    };
    static const CueLayer spray[] = {
        {LAYER_THUMP,  0.0f,  0.001f,  0.01f,  0.05f, 900,   500,  0.01f,  0.0f, 0.5f,  0.25f, 0.0f,  0.2f, 0.1f},
        {LAYER_NOISE,  0.02f, 0.010f,  0.45f,  0.18f, 5200,  4300, 0.40f,  0.9f, 2.0f,  0.30f, 0.0f,  0.25f, 0.1f},
        {LAYER_NOISE,  0.02f, 0.010f,  0.45f,  0.18f, 2800,  2300, 0.40f,  6.0f, 1.0f,  0.45f, 0.0f,  0.25f, 0.1f},
    };
    static const CueDef table[CUE_COUNT] = {
        {"none", nullptr, 0, BUS_SFX, 0.0f, 0.0f},
        {"scare", CUE_ROWS(scare), BUS_SFX, 0.85f, 1.2f},
        {"hit", CUE_ROWS(hit), BUS_SFX, 0.5f, 0.2f},
        {"warn_near", CUE_ROWS(warnNear), BUS_MUSIC, 0.4f, 6.0f},
        {"warn_hunt", CUE_ROWS(warnHunt), BUS_MUSIC, 0.65f, 5.0f},
        {"notice", CUE_ROWS(notice), BUS_SFX, 0.2f, 1.5f},
        {"call_stalker", CUE_ROWS(callStalker), BUS_SFX, 0.0f, 0.6f},
        {"call_crawler", CUE_ROWS(callCrawler), BUS_SFX, 0.0f, 0.6f},
        {"call_shadow", CUE_ROWS(callShadow), BUS_SFX, 0.0f, 0.6f},
        {"alert_stalker", CUE_ROWS(alertStalker), BUS_SFX, 0.55f, 2.5f},
        {"alert_crawler", CUE_ROWS(alertCrawler), BUS_SFX, 0.55f, 2.5f},
        {"alert_shadow", CUE_ROWS(alertShadow), BUS_SFX, 0.55f, 2.5f},
        {"plush", CUE_ROWS(plush), BUS_SFX, 0.0f, 0.3f},
        {"battery", CUE_ROWS(battery), BUS_SFX, 0.0f, 0.3f},
        {"spray", CUE_ROWS(spray), BUS_SFX, 0.0f, 0.3f},
    };
    return table[cue > CUE_NONE && cue < CUE_COUNT ? cue : CUE_NONE];
}

// One playing part of a cue.
struct CueVoice {
    const CueLayer* def = nullptr;
    int bus = BUS_SFX;
    float t = 0.0f, gain = 1.0f, pitch = 1.0f, send = 0.0f, duck = 0.0f;
    float phase[2] = {}, wobble = 0.0f, clickT = -1.0f, clickAmp = 0.0f, freq = 0.0f, norm = 1.0f;
    int control = 0;
    Svf filter[6];
    PanGain pan;
};

struct CuePlayer {
    static const int VOICES = 32;
    CueVoice voices[VOICES];
    AudioRng rng;
    float sinceStart[CUE_COUNT];
    float duckEnv = 0.0f;

    CuePlayer() {
        rng.s = 0xC0FFEE11u;
        for (float& s : sinceStart) s = 1000.0f;
    }

    // gain/pan/pitch place this one instance; farSend adds reverb for things that happen far away.
    bool start(int cue, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, float farSend = 0.0f) {
        const CueDef& def = cueDef(cue);
        if (def.count == 0 || gain <= 0.001f || sinceStart[cue] < def.cooldown) return false;
        sinceStart[cue] = 0.0f;
        for (int i = 0; i < def.count; i++) {
            // Take a free voice, or the one that has been ringing the longest.
            CueVoice* v = &voices[0];
            for (CueVoice& c : voices) {
                if (!c.def) { v = &c; break; }
                if (c.t > v->t) v = &c;
            }
            const CueLayer& l = def.layers[i];
            *v = CueVoice();
            v->def = &l;
            v->bus = def.bus;
            v->t = -l.delay;
            v->gain = gain * l.gain;
            v->pitch = pitch * (1.0f + l.vary * rng.bi());
            v->send = l.send + farSend;
            v->duck = def.duck * aClamp(gain, 0.0f, 1.0f);
            v->pan.set(pan + l.pan);
            v->phase[0] = l.kind == LAYER_THUMP ? 0.0f : rng.uni();
        }
        return true;
    }

    static float envelope(const CueLayer& l, float t) {
        if (t < l.attack) return t / l.attack;
        float r = (t - l.attack - l.hold) / l.release;
        if (r <= 0.0f) return 1.0f;
        return r >= 1.0f ? 0.0f : expf(-5.0f * r) * (1.0f - r);
    }

    float renderVoice(CueVoice& v) {
        const CueLayer& l = *v.def;
        float t = v.t;
        bool control = (v.control++ & 15) == 0;
        if (control) {
            v.freq = (l.glide > 0.0f ? l.f1 + (l.f0 - l.f1) * expf(-t / l.glide) : l.f0) * v.pitch;
            v.wobble += (rng.bi() - v.wobble) * 0.2f;
        }
        float f = v.freq;
        switch (l.kind) {
        case LAYER_NOISE: {
            if (control) {
                v.filter[0].set(f, l.q);
                // Narrow or low bands hold little of the noise's energy: make up for it.
                v.norm = l.shape < 0.5f ? aClamp(sqrtf(6000.0f / f), 1.0f, 9.0f) : (l.shape < 1.5f ? 1.0f / sqrtf(l.q) : 1.0f);
            }
            v.filter[0].tick(rng.bi());
            Svf& s = v.filter[0];
            return (l.shape < 0.5f ? s.low : (l.shape < 1.5f ? s.band : s.high)) * v.norm;
        }
        case LAYER_THUMP:
            return tanhf(oscSin(v.phase[0], f) * (1.0f + l.shape * 4.0f));
        case LAYER_METAL: {
            // Partials of a struck plate: not a chord, not a harmonic series.
            static const float ratio[6] = {1.0f, 1.58f, 2.17f, 2.93f, 3.71f, 4.86f};
            static const float level[6] = {1.0f, 0.8f, 0.65f, 0.5f, 0.4f, 0.3f};
            if (control) for (int i = 0; i < 6; i++) v.filter[i].set(f * ratio[i], l.q * (1.0f + 0.25f * (float)i));
            if (control) v.norm = 0.06f / sqrtf(l.shape);
            float strike = rng.bi() * expf(-t / l.shape);
            float sum = 0.0f;
            for (int i = 0; i < 6; i++) sum += v.filter[i].bp(strike) * level[i];
            return sum * v.norm;
        }
        case LAYER_THROAT: {
            // Vocal folds that do not close cleanly: jitter, a growling subharmonic, breath.
            float rough = l.shape, open = l.q;
            float folds = oscSaw(v.phase[0], f * (1.0f + rough * 0.08f * v.wobble));
            float sub = 0.5f + 0.5f * oscSin(v.phase[1], f * 0.5f);
            float src = folds * (1.0f - rough * 0.6f * sub) + rng.bi() * (0.12f + rough * 0.45f);
            if (control) {
                float stretch = 1.0f + 0.12f * v.wobble;
                v.filter[0].set(aLerp(330.0f, 980.0f, open) * stretch, 5.0f);
                v.filter[1].set(aLerp(880.0f, 1750.0f, open), 6.0f);
                v.filter[2].set(aLerp(2350.0f, 3400.0f, open) * (2.0f - stretch), 7.0f);
            }
            float mouth = v.filter[0].bp(src) * 0.30f + v.filter[1].bp(src) * 0.16f + v.filter[2].bp(src) * 0.09f;
            float flutter = 1.0f - rough * 0.35f * (0.5f + 0.5f * sinf(t * 200.0f + v.wobble * 3.0f));
            return tanhf((mouth + src * 0.15f) * (1.2f + rough * 2.2f)) * flutter * 0.8f;
        }
        default: {
            v.phase[0] += f * AUDIO_DT * (1.0f + l.shape * v.wobble);
            if (v.phase[0] >= 1.0f) {
                v.phase[0] -= 1.0f;
                v.clickT = 0.0f;
                v.clickAmp = rng.range(0.5f, 1.0f);
                v.filter[0].set(l.q * 1000.0f * v.pitch * rng.range(0.8f, 1.25f), 9.0f);
            }
            if (v.clickT < 0.0f) return 0.0f;
            float click = v.filter[0].bp(rng.bi() * expf(-v.clickT * 1300.0f)) * v.clickAmp * 1.1f;
            v.clickT += AUDIO_DT;
            if (v.clickT > 0.03f) v.clickT = -1.0f;
            return click;
        }
        }
    }

    // Adds every playing part to its bus and returns 0..1: how far the beds should duck.
    float render(AudioFrame* buses) {
        for (float& s : sinceStart) s += AUDIO_DT;
        float duck = 0.0f;
        for (CueVoice& v : voices) {
            if (!v.def) continue;
            v.t += AUDIO_DT;
            if (v.t < 0.0f) continue;
            const CueLayer& l = *v.def;
            float env = envelope(l, v.t);
            if (v.t >= l.attack + l.hold + l.release) {
                v.def = nullptr;
                continue;
            }
            buses[v.bus].add(renderVoice(v) * env * v.gain, v.pan, v.send);
            float push = v.duck * (v.t < l.attack + l.hold ? 1.0f : env);
            if (push > duck) duck = push;
        }
        duckEnv += (duck - duckEnv) * (duck > duckEnv ? 0.02f : 0.00008f);
        return duckEnv;
    }
};

// Cues posted from the game thread wait here for the audio thread.
struct CueRequest {
    std::atomic<int> cue{CUE_NONE};
    float gain = 1.0f, pan = 0.0f, pitch = 1.0f;
};

struct CueQueue {
    static const int SIZE = 8;
    CueRequest slots[SIZE];
    std::atomic<unsigned> writeIndex{0};
    std::atomic<int> waiting{0};

    void drain(CuePlayer& player) {
        if (waiting.load() <= 0) return;
        for (CueRequest& r : slots) {
            int cue = r.cue.exchange(CUE_NONE);
            if (cue == CUE_NONE) continue;
            waiting.fetch_sub(1);
            player.start(cue, r.gain, r.pan, r.pitch);
        }
    }
};

inline CueQueue gCueQueue;

inline void audioPostCue(int cue, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) {
    if (cue <= CUE_NONE || cue >= CUE_COUNT) return;
    CueRequest& r = gCueQueue.slots[gCueQueue.writeIndex.fetch_add(1) % CueQueue::SIZE];
    r.gain = gain;
    r.pan = pan;
    r.pitch = pitch;
    if (r.cue.exchange(cue) == CUE_NONE) gCueQueue.waiting.fetch_add(1);
}
