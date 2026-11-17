#pragma once
// Things the player does: footsteps, flashlight switch, menu blips.

#include "audio_synth.h"

struct FootstepVoice {
    float t = -1.0f;
    float pitch = 1.0f, weight = 1.0f;
    float phase = 0.0f;
    Svf scuff;
    OnePole body;
    PanGain pan;
};

struct UiBlip {
    float t = -1.0f;
    float pitch = 1.0f;
    float phase = 0.0f, phase2 = 0.0f;

    void start(float p, float lo, float hi) {
        t = 0.0f;
        phase = phase2 = 0.0f;
        pitch = aClamp(p, lo, hi);
    }
};

struct SfxSynth {
    AudioRng rng;
    // Footsteps
    FootstepVoice steps[2];
    int nextStep = 0;
    bool leftFoot = false;
    float gaitPhase = 0.0f;
    Svf clothFilter;
    // Flashlight
    float lastFlashlight = 0.0f;
    float clickT = -1.0f, clickTone = 1.0f, clickPhase = 0.0f;
    Svf clickFilter;
    // Menu
    UiBlip uiMove, uiAdjust, uiConfirm;
    OnePole uiClick;

    SfxSynth() {
        rng.s = 0x51ED270Bu;
        clothFilter.set(2100.0f, 0.7f);
    }

    void startStep(float pitch, float sprint) {
        FootstepVoice& v = steps[nextStep];
        nextStep = (nextStep + 1) % 2;
        leftFoot = !leftFoot;
        v.t = 0.0f;
        v.phase = 0.0f;
        v.pitch = aClamp(pitch, 0.7f, 1.45f);
        v.weight = 0.8f + sprint * 0.45f;
        v.scuff.set(rng.range(650.0f, 1150.0f) * v.pitch, 0.9f);
        v.pan.set(leftFoot ? -0.16f : 0.16f);
    }

    // Shoe on office carpet: a soft heel thud, then fibres scuffing as the foot rolls to the toe.
    float renderStep(FootstepVoice& v) {
        float t = v.t;
        float thud = oscSin(v.phase, (56.0f + 78.0f * expf(-t * 48.0f)) * v.pitch) * aPluck(t, 0.004f, 34.0f);
        float roll = aPluck(t, 0.006f, 46.0f) + aPluck(t - 0.075f, 0.012f, 40.0f) * 0.55f;
        float noise = rng.bi();
        float fibres = v.scuff.bp(noise) * 0.42f + v.body.lp(noise, 0.035f) * 1.3f;
        v.t += AUDIO_DT;
        if (v.t > 0.30f) v.t = -1.0f;
        return (thud * 0.75f + fibres * roll) * v.weight * 0.62f;
    }

    void renderMovement(float move, float sprint, float stepGain, AudioFrame& out) {
        for (int i = 0; i < 2; i++) {
            if (steps[i].t >= 0.0f) out.add(renderStep(steps[i]) * stepGain, steps[i].pan, 0.30f);
        }
        if (move > 0.02f) {
            // Clothes and gear swishing in step with the stride.
            gaitPhase += (1.25f + sprint * 0.65f) * AUDIO_DT;
            if (gaitPhase >= 1.0f) gaitPhase -= 1.0f;
            float swing = 0.5f - 0.5f * cosf(AUDIO_TAU * gaitPhase * 2.0f);
            float cloth = clothFilter.bp(rng.bi()) * (0.25f + 0.75f * swing * swing);
            cloth *= move * (0.010f + sprint * 0.012f);
            out.addStereo(cloth, cloth, 0.1f);
        }
    }

    // Slide switch: a sharp plastic tick and the latch settling right after it.
    void renderFlashlight(float flashlightOn, AudioFrame& out) {
        if (flashlightOn != lastFlashlight) {
            lastFlashlight = flashlightOn;
            clickT = 0.0f;
            clickPhase = 0.0f;
            clickTone = flashlightOn > 0.5f ? 1.12f : 0.88f;
            clickFilter.set(3100.0f * clickTone, 2.2f);
        }
        if (clickT < 0.0f) return;
        float t = clickT;
        float latch = t - 0.026f;
        float burst = expf(-t * 1500.0f) + (latch >= 0.0f ? expf(-latch * 1100.0f) * 0.6f : 0.0f);
        float tick = clickFilter.bp(rng.bi() * burst) * 0.30f;
        float shell = oscSin(clickPhase, 1650.0f * clickTone) * (aPluck(t, 0.0005f, 420.0f) + aPluck(latch, 0.0005f, 300.0f) * 0.5f);
        clickT += AUDIO_DT;
        if (clickT > 0.09f) clickT = -1.0f;
        float s = tick + shell * 0.10f;
        out.addStereo(s * 1.5f, s * 1.25f, 0.25f);
    }

    static float blipTone(UiBlip& b, float hz, float overtone) {
        float base = oscSin(b.phase, hz * b.pitch);
        return base + oscSin(b.phase2, hz * b.pitch * 2.0f) * overtone;
    }

    // Menu feedback: short, soft, slightly glassy - a tape deck's buttons rather than arcade beeps.
    float renderUi() {
        float ui = 0.0f;
        if (uiMove.t >= 0.0f) {
            float t = uiMove.t;
            ui += blipTone(uiMove, 587.3f, 0.22f) * aPluck(t, 0.003f, 42.0f) * 0.20f;
            ui += uiClick.hp(rng.bi(), 0.4f) * expf(-t * 1300.0f) * 0.07f;
            uiMove.t += AUDIO_DT;
            if (uiMove.t > 0.13f) uiMove.t = -1.0f;
        }
        if (uiAdjust.t >= 0.0f) {
            float t = uiAdjust.t;
            ui += blipTone(uiAdjust, 880.0f, 0.15f) * aPluck(t, 0.002f, 64.0f) * 0.15f;
            uiAdjust.t += AUDIO_DT;
            if (uiAdjust.t > 0.08f) uiAdjust.t = -1.0f;
        }
        if (uiConfirm.t >= 0.0f) {
            // Two notes a fifth apart.
            float t = uiConfirm.t;
            float second = t - 0.07f;
            ui += oscSin(uiConfirm.phase, 587.3f) * aPluck(t, 0.004f, 20.0f) * 0.17f;
            if (second >= 0.0f) ui += oscSin(uiConfirm.phase2, 880.0f) * aPluck(second, 0.004f, 13.0f) * 0.17f;
            uiConfirm.t += AUDIO_DT;
            if (uiConfirm.t > 0.45f) uiConfirm.t = -1.0f;
        }
        return ui;
    }
};
