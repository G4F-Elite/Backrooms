#pragma once
// What is hunting the player and what it does to them: tension drone, heartbeat,
// monster presence, breathing, whispers at low sanity, and the death drone.

#include "audio_synth.h"

struct MonsterBed {
    float gain = 0.0f;         // level of the presence bed (the mix table decides)
    float proximity = 0.0f;    // 0..1 closeness: far is echo, near is dry
    float menace = 0.0f;       // 0..1 how aggressive it is
    float type = 0.0f;         // 0 stalker, 0.5 crawler, 1 shadow
    float pan = 0.0f;          // -1..1 where it is
};

// One whispering voice: unvoiced noise through two vowel formants that jump per syllable.
struct WhisperVoice {
    Svf f1, f2;
    float freq1 = 500.0f, freq2 = 1600.0f, target1 = 500.0f, target2 = 1600.0f;
    float env = 0.0f, envTarget = 0.0f;
    float left = 0.0f;
    float pan = 0.0f, panTarget = 0.0f;

    float next(AudioRng& rng, float& outPan) {
        left -= AUDIO_DT;
        if (left <= 0.0f) {
            bool pause = rng.uni() < 0.28f;
            left = pause ? rng.range(0.12f, 0.6f) : rng.range(0.07f, 0.22f);
            envTarget = pause ? 0.0f : rng.range(0.35f, 1.0f);
            target1 = rng.range(320.0f, 820.0f);
            target2 = rng.range(1250.0f, 2700.0f);
            if (pause) panTarget = rng.range(-0.9f, 0.9f);
        }
        freq1 += (target1 - freq1) * 0.004f;
        freq2 += (target2 - freq2) * 0.004f;
        env += (envTarget - env) * 0.003f;
        pan += (panTarget - pan) * 0.0002f;
        f1.set(freq1, 6.0f);
        f2.set(freq2, 7.0f);
        float breath = rng.bi();
        outPan = pan;
        return (f1.bp(breath) * 0.6f + f2.bp(breath) * 0.4f) * env;
    }
};

struct ThreatSynth {
    AudioRng rng;
    // Tension drone
    float dronePhase[5] = {};
    float droneLfo = 0.0f, whineA = 0.0f, whineB = 0.0f, whineLfo = 0.0f;
    Svf droneL, droneR;
    // Heartbeat, chase pulse
    float heartPhase = 0.0f;
    float chasePhase = 0.0f, chaseT = 1.0f, chaseA = 0.0f, chaseB = 0.0f, chaseAccent = 1.0f;
    int chaseBeat = 0;
    Svf chaseFilter;
    // Monsters
    PanGain monsterPan;
    float monsterPanSmooth = 0.0f;
    float growlPhase = 0.0f, growlSub = 0.0f, growlBreath = 0.0f;
    SlowRandom growlJitter;
    Svf growlF1, growlF2;
    float chitterPhase = 0.0f, chitterLeft = 0.0f, chitterT = -1.0f, chitterAmp = 1.0f;
    Svf chitterFilter, dragFilter;
    PinkNoise dragNoise;
    SlowRandom dragWander;
    float shadowLfo = 0.0f, shadowTrem = 0.0f, shadowSub = 0.0f;
    float choir[3] = {};
    Svf shadowFilter;
    PinkNoise shadowNoise;
    // Player
    float breathPhase = 0.0f, breathEnv = 0.0f, breathFreq = 900.0f;
    Svf breathFilter;
    OnePole breathAir;
    WhisperVoice whispers[2];
    float ringPhase = 0.0f;
    // Death
    float deathAmount = 0.0f, deathT = -1.0f;
    float deathPhase[3] = {};
    float deathBoom = 0.0f, deathRingA = 0.0f, deathRingB = 0.0f, deathLfo = 0.0f;
    Svf deathL, deathR, deathNoise;
    bool wasDead = false;

    ThreatSynth() {
        rng.s = 0x7F4A7C15u;
        dragFilter.set(170.0f, 1.4f);
        whispers[0].panTarget = -0.6f;
        whispers[1].panTarget = 0.6f;
    }

    // Low detuned saws that open up and pick up a clashing minor ninth as danger grows.
    void renderDrone(float danger, AudioFrame& out) {
        if (danger < 0.01f) return;
        droneLfo += 0.09f * AUDIO_DT;
        if (droneLfo >= 1.0f) droneLfo -= 1.0f;
        float sway = sinf(AUDIO_TAU * droneLfo);

        // The octave keeps the drone present on speakers that cannot reproduce the 55 Hz root.
        float octave = oscSaw(dronePhase[4], 110.3f) * 0.45f;
        float a = oscSaw(dronePhase[0], 55.0f) * 0.7f + octave;
        float b = oscSaw(dronePhase[1], 55.42f) * 0.7f + octave;
        float fifth = oscSaw(dronePhase[2], 82.1f) * 0.5f;
        float ninth = oscSaw(dronePhase[3], 116.9f) * danger * 0.7f;
        float cutoff = 150.0f + danger * danger * 1000.0f + sway * 50.0f;
        droneL.set(cutoff, 1.3f);
        droneR.set(cutoff * 1.06f, 1.3f);
        float gain = danger * 0.085f;
        float left = droneL.lp(a + (fifth + ninth) * 0.7f) * gain;
        float right = droneR.lp(b + (fifth + ninth) * 0.7f) * gain;

        // Thin high cluster, like strings held too long.
        float edge = (danger - 0.45f) / 0.55f;
        if (edge > 0.0f) {
            whineLfo += 0.23f * AUDIO_DT;
            if (whineLfo >= 1.0f) whineLfo -= 1.0f;
            float amp = edge * edge * 0.010f * (0.6f + 0.4f * sinf(AUDIO_TAU * whineLfo));
            left += oscSin(whineA, 2093.0f) * amp;
            right += oscSin(whineB, 2217.5f) * amp;
        }
        out.addStereo(left, right, 0.35f);
    }

    // One heart sound: a sine that drops in pitch, phase integrated in closed form.
    static float heartThump(float t, float fHi, float fLo) {
        if (t < 0.0f) return 0.0f;
        const float glide = 34.0f;
        float cycles = fLo * t + (fHi - fLo) * (1.0f - expf(-glide * t)) / glide;
        return tanhf(sinf(AUDIO_TAU * cycles) * 1.7f) * aPluck(t, 0.007f, 24.0f);
    }

    void renderHeart(float level, float racing, AudioFrame& out) {
        level = aClamp(level, 0.0f, 1.0f);
        if (level <= 0.001f) {
            heartPhase = 0.0f;
            return;
        }
        float rate = (66.0f + aClamp(racing, 0.0f, 1.0f) * 78.0f) / 60.0f;
        heartPhase += rate * AUDIO_DT;
        if (heartPhase >= 1.0f) heartPhase -= 1.0f;
        float lub = heartThump(heartPhase / rate, 74.0f, 44.0f);
        float dub = heartThump((heartPhase - 0.31f) / rate, 92.0f, 52.0f) * 0.7f;
        float beat = (lub + dub) * level * 0.36f;
        out.addStereo(beat, beat, 0.0f);
    }

    // Being hunted: short low stabs with a hard front edge, every fourth one heavier.
    void renderChase(float level, AudioFrame& out) {
        if (level < 0.01f) {
            chasePhase = 0.999f; // the first stab lands the moment the chase starts
            chaseBeat = 0;
            return;
        }
        chasePhase += (2.6f + level * 1.1f) * AUDIO_DT;
        if (chasePhase >= 1.0f) {
            chasePhase -= 1.0f;
            chaseT = 0.0f;
            chaseAccent = (chaseBeat++ % 4 == 0) ? 1.0f : 0.62f;
        }
        float bite = aPluck(chaseT, 0.002f, 13.0f);
        chaseT += AUDIO_DT;
        float strings = oscSaw(chaseA, 55.0f) + oscSaw(chaseB, 58.4f) * 0.8f;
        chaseFilter.set(140.0f + 1500.0f * bite * chaseAccent, 1.8f);
        float stab = chaseFilter.lp(strings) * bite * chaseAccent * level * 0.20f;
        out.addStereo(stab, stab, 0.25f);
    }

    // Stalker: something large breathing through its teeth.
    float renderGrowl(float menace) {
        float jitter = growlJitter.next(rng, 700, 0.003f);
        float f = (43.0f + menace * 15.0f) * (1.0f + 0.06f * jitter);
        float src = oscSaw(growlPhase, f) * (0.7f + 0.3f * oscSin(growlSub, f * 0.5f));
        growlBreath += (0.34f + menace * 0.40f) * AUDIO_DT;
        if (growlBreath >= 1.0f) growlBreath -= 1.0f;
        // Each breath starts with a snarl and dies away, instead of swelling in politely.
        float air = aPluck(growlBreath, 0.012f, 3.2f);
        growlF1.set(260.0f + 240.0f * air, 5.0f);
        growlF2.set(880.0f + 260.0f * jitter, 6.0f);
        float throat = growlF1.bp(src) + growlF2.bp(src) * 0.4f + src * 0.25f;
        return throat * (0.30f + 0.70f * air * air) * 0.26f;
    }

    // Crawler: bursts of bony clicking and a body dragging itself over the carpet.
    float renderChitter(float menace) {
        chitterPhase += (0.50f + menace * 0.55f) * AUDIO_DT;
        if (chitterPhase >= 1.0f) chitterPhase -= 1.0f;
        float burst = sinf(AUDIO_TAU * chitterPhase);
        burst = burst > 0.0f ? burst * burst : 0.0f;
        chitterLeft -= AUDIO_DT;
        if (chitterLeft <= 0.0f && burst > 0.08f) {
            chitterLeft = rng.range(0.030f, 0.095f) / (0.75f + menace * 0.6f);
            chitterT = 0.0f;
            chitterAmp = rng.range(0.5f, 1.0f) * burst;
            chitterFilter.set(rng.range(1500.0f, 2700.0f), 9.0f);
        }
        float click = 0.0f;
        if (chitterT >= 0.0f) {
            click = chitterFilter.bp(rng.bi() * expf(-chitterT * 1400.0f)) * chitterAmp;
            chitterT += AUDIO_DT;
            if (chitterT > 0.025f) chitterT = -1.0f;
        }
        float wander = fabsf(dragWander.next(rng, 3000, 0.001f));
        float drag = dragFilter.bp(dragNoise.next(rng)) * (0.25f + 0.75f * wander);
        return click * 0.75f + drag * 0.55f;
    }

    // Shadow: no body to make a sound with - wind that is not wind, and a choir slightly out of tune.
    float renderShadow(float menace) {
        shadowLfo += 0.11f * AUDIO_DT;
        if (shadowLfo >= 1.0f) shadowLfo -= 1.0f;
        float sweep = 0.5f + 0.5f * sinf(AUDIO_TAU * shadowLfo);
        shadowFilter.set(240.0f + 330.0f * sweep, 5.0f);
        float wind = shadowFilter.bp(shadowNoise.next(rng)) * 0.24f;

        shadowTrem += (0.6f + menace * 0.9f) * AUDIO_DT;
        if (shadowTrem >= 1.0f) shadowTrem -= 1.0f;
        float pulse = 0.55f + 0.45f * sinf(AUDIO_TAU * shadowTrem);
        float voices = oscSin(choir[0], 196.0f) + oscSin(choir[1], 207.65f) + oscSin(choir[2], 293.7f) * 0.5f;
        float sub = oscSin(shadowSub, 37.0f + menace * 9.0f) * 0.22f;
        return wind + voices * pulse * 0.075f + sub * pulse;
    }

    void renderMonster(const MonsterBed& in, AudioFrame& out) {
        if (in.gain < 0.005f) return;
        float wStalker = aClamp(1.0f - in.type * 2.0f, 0.0f, 1.0f);
        float wCrawler = aClamp(1.0f - fabsf(in.type - 0.5f) * 2.0f, 0.0f, 1.0f);
        float wShadow = aClamp((in.type - 0.5f) * 2.0f, 0.0f, 1.0f);

        float voice = 0.0f;
        if (wStalker > 0.001f) voice += renderGrowl(in.menace) * wStalker;
        if (wCrawler > 0.001f) voice += renderChitter(in.menace) * wCrawler;
        if (wShadow > 0.001f) voice += renderShadow(in.menace) * wShadow;

        // Far away it is mostly echo; up close it is dry and in your ear.
        float gain = in.gain;
        monsterPanSmooth += (in.pan - monsterPanSmooth) * 0.0006f;
        monsterPan.set(monsterPanSmooth * 0.85f);
        out.add(voice * gain, monsterPan, 1.1f - in.proximity * 0.7f);
    }

    // Player's own breath: brighter, shorter inhale, then a longer, lower exhale.
    void renderBreath(float exertion, AudioFrame& out) {
        if (exertion < 0.04f && breathEnv < 0.001f) return;
        breathPhase += (0.30f + exertion * 0.95f) * AUDIO_DT;
        if (breathPhase >= 1.0f) breathPhase -= 1.0f;
        float p = breathPhase;
        float target = 0.0f, freq = breathFreq;
        if (p < 0.40f) {
            float s = sinf(3.14159265f * p / 0.40f);
            target = s * s * 0.65f;
            freq = 1500.0f;
        } else if (p > 0.48f && p < 0.97f) {
            float s = sinf(3.14159265f * (p - 0.48f) / 0.49f);
            target = s * sqrtf(s);
            freq = 780.0f;
        }
        if (exertion < 0.04f) target = 0.0f;
        breathEnv += (target - breathEnv) * 0.0015f;
        breathFreq += (freq - breathFreq) * 0.0015f;
        breathFilter.set(breathFreq, 1.1f);
        float noise = rng.bi();
        float breath = breathFilter.bp(noise) + breathAir.hp(noise, 0.35f) * 0.08f;
        breath *= breathEnv * exertion * 0.075f;
        out.addStereo(breath, breath, 0.06f);
    }

    void renderInsanity(float insanity, AudioFrame& out) {
        if (insanity < 0.001f) return;
        float ramp = insanity * insanity * (3.0f - 2.0f * insanity);
        for (int i = 0; i < 2; i++) {
            float pan = 0.0f;
            float voice = whispers[i].next(rng, pan) * ramp * 0.20f;
            PanGain g;
            g.l = 0.7071f * (1.0f - pan * 0.8f);
            g.r = 0.7071f * (1.0f + pan * 0.8f);
            out.add(voice, g, 0.8f);
        }
        // Ears ringing.
        float ring = oscSin(ringPhase, 5270.0f) * ramp * 0.006f;
        out.addStereo(ring, -ring, 0.0f);
    }

    // Returns 0..1: how far the rest of the mix should be pushed under water.
    float renderDeath(bool dead, AudioFrame& out) {
        if (dead && !wasDead) {
            deathT = 0.0f;
            deathBoom = 0.0f;
        }
        wasDead = dead;
        deathAmount += ((dead ? 1.0f : 0.0f) - deathAmount) * (dead ? 0.00006f : 0.0002f);
        if (deathAmount < 0.0005f && deathT < 0.0f) return 0.0f;

        deathLfo += 0.07f * AUDIO_DT;
        if (deathLfo >= 1.0f) deathLfo -= 1.0f;
        float cutoff = 240.0f + 80.0f * sinf(AUDIO_TAU * deathLfo);
        deathL.set(cutoff, 1.6f);
        deathR.set(cutoff * 1.1f, 1.6f);
        float fifth = oscSaw(deathPhase[2], 123.7f) * 0.4f;
        float left = deathL.lp(oscSaw(deathPhase[0], 41.2f) + fifth) * 0.15f * deathAmount;
        float right = deathR.lp(oscSaw(deathPhase[1], 41.55f) + fifth) * 0.15f * deathAmount;

        if (deathT >= 0.0f) {
            // The hit, then the ringing in the ears that slowly lets go.
            float t = deathT;
            float boom = oscSin(deathBoom, 36.0f + 60.0f * expf(-t * 9.0f)) * aPluck(t, 0.004f, 2.6f) * 0.55f;
            deathNoise.set(300.0f + 3000.0f * expf(-t * 10.0f), 0.8f);
            boom += deathNoise.lp(rng.bi()) * aPluck(t, 0.002f, 6.0f) * 0.35f;
            float ringEnv = aPluck(t, 0.05f, 0.75f) * 0.022f;
            left += boom + oscSin(deathRingA, 3890.0f) * ringEnv;
            right += boom + oscSin(deathRingB, 3902.0f) * ringEnv;
            deathT += AUDIO_DT;
            if (deathT > 7.0f) deathT = -1.0f;
        }
        out.addStereo(left, right, 0.4f);
        return deathAmount;
    }
};
