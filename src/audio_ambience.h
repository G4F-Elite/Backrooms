#pragma once
// The room itself: fluorescent light hum, air handling rumble, and things that happen far away.

#include "audio_synth.h"

enum RoomEventType {
    ROOM_EV_PIPE = 0,
    ROOM_EV_VENT,
    ROOM_EV_SCRAPE,
    ROOM_EV_KNOCK,
    ROOM_EV_CLANG,
    ROOM_EV_FLICKER,
    ROOM_EV_STEPS,
    ROOM_EV_MOAN,
    ROOM_EV_COUNT
};

// One far-away sound. Every event gets its own spot in the stereo field and mostly reaches
// the player as reverb, which is what makes it read as "somewhere down the corridor".
struct RoomEvent {
    int type = -1;
    float t = 0.0f, dur = 0.0f;
    float a = 0.0f, b = 0.0f, c = 0.0f;
    float ph[4] = {};
    float brown = 0.0f;
    float gain = 1.0f;
    float farCoeff = 0.2f;
    int hit = -1;
    SlowRandom slow;
    PinkNoise pink;
    Svf f1, f2;
    OnePole farLp;
    PanGain pan;
};

struct AmbienceSynth {
    static const int EVENT_SLOTS = 3;
    AudioRng rng;
    // Lights
    float mains = 0.0f;
    SlowRandom lampL, lampR;
    Svf buzzL, buzzR;
    float flickerT = -1.0f, flickerDur = 0.0f, flickerGate = 1.0f, flickerSmooth = 1.0f;
    float flickerNext = 0.0f, tickT = -1.0f;
    Svf tickFilter;
    PanGain flickerPan;
    // Air
    PinkNoise pinkL, pinkR;
    Svf rumbleL, rumbleR;
    OnePole hissL, hissR;
    float droneA = 0.0f, droneB = 0.0f, droneC = 0.0f, droneLfo = 0.0f;
    // Far events
    RoomEvent events[EVENT_SLOTS];
    float clock = 0.0f, nextEvent = 4.0f;
    int lastEvent = -1;

    AmbienceSynth() {
        rng.s = 0xB5297A4Du;
        buzzL.set(3900.0f, 0.8f);
        buzzR.set(4400.0f, 0.8f);
        rumbleL.set(170.0f, 0.5f);
        rumbleR.set(195.0f, 0.5f);
        tickFilter.set(2600.0f, 2.5f);
    }

    // Ballast hum: harmonics of the 60 Hz mains with the 120 Hz magnetostriction tone on top,
    // plus the tube's gas buzz - noise that pulses twice per mains cycle.
    void renderLights(float humVol, AudioFrame& out) {
        static const int HARMONICS = 9;
        static const float mult[HARMONICS] = {1, 2, 3, 4, 6, 8, 10, 12, 14};
        static const float weight[HARMONICS] = {0.30f, 1.00f, 0.16f, 0.50f, 0.34f, 0.20f, 0.13f, 0.08f, 0.05f};
        static const float offL[HARMONICS] = {0.00f, 0.13f, 0.71f, 0.38f, 0.90f, 0.22f, 0.57f, 0.05f, 0.66f};
        // The right side runs slightly out of step with the left, more so for the upper harmonics:
        // wide enough to surround the listener, close enough to stay solid in the middle.
        static const float offR[HARMONICS] = {0.02f, 0.19f, 0.63f, 0.48f, 0.76f, 0.42f, 0.30f, 0.38f, 0.06f};

        mains += 60.0f * AUDIO_DT;
        if (mains >= 1.0f) mains -= 1.0f;
        float humL = 0.0f, humR = 0.0f;
        for (int i = 0; i < HARMONICS; i++) {
            float p = mains * mult[i];
            humL += sinf(AUDIO_TAU * (p + offL[i])) * weight[i];
            humR += sinf(AUDIO_TAU * (p + offR[i])) * weight[i];
        }

        float pulse = fabsf(sinf(AUDIO_TAU * mains));
        pulse = pulse * pulse * pulse;
        float buzzGain = 0.045f * pulse;
        humL = humL * 0.20f + buzzL.bp(rng.bi()) * buzzGain;
        humR = humR * 0.20f + buzzR.bp(rng.bi()) * buzzGain;

        // Old tubes never burn evenly.
        humL *= 0.90f + 0.10f * lampL.next(rng, 5200, 0.0004f);
        humR *= 0.90f + 0.10f * lampR.next(rng, 6900, 0.0004f);

        float tick = 0.0f;
        if (flickerT >= 0.0f) {
            if (flickerT >= flickerNext) {
                flickerGate = flickerGate > 0.5f ? rng.range(0.05f, 0.35f) : 1.0f;
                flickerNext = flickerT + rng.range(0.02f, 0.11f);
                tickT = 0.0f;
            }
            flickerT += AUDIO_DT;
            if (flickerT > flickerDur) {
                flickerT = -1.0f;
                flickerGate = 1.0f;
            }
        }
        flickerSmooth += (flickerGate - flickerSmooth) * 0.02f;
        if (tickT >= 0.0f) {
            // Starter relay / arcing tick on every stutter of the tube.
            tick = tickFilter.bp(rng.bi() * expf(-tickT * 900.0f)) * 0.10f;
            tickT += AUDIO_DT;
            if (tickT > 0.03f) tickT = -1.0f;
        }
        // Only the tube on the flickering side dims.
        float dimL = aLerp(1.0f, flickerSmooth, flickerPan.l);
        float dimR = aLerp(1.0f, flickerSmooth, flickerPan.r);
        out.addStereo(humL * dimL * humVol, humR * dimR * humVol, 0.10f);
        out.add(tick, flickerPan, 0.5f);
    }

    void renderAir(AudioFrame& out) {
        float pl = pinkL.next(rng);
        float pr = pinkR.next(rng);
        float airL = rumbleL.bp(pl) * 0.11f + pl * 0.006f + hissL.hp(rng.bi(), 0.55f) * 0.0012f;
        float airR = rumbleR.bp(pr) * 0.11f + pr * 0.006f + hissR.hp(rng.bi(), 0.55f) * 0.0012f;

        // Building drone: two low tones a hair apart, so the room slowly "breathes" as they beat.
        droneLfo += 0.05f * AUDIO_DT;
        if (droneLfo >= 1.0f) droneLfo -= 1.0f;
        float swell = 0.65f + 0.35f * sinf(AUDIO_TAU * droneLfo);
        float low = (oscSin(droneA, 49.0f) + oscSin(droneB, 49.31f)) * 0.011f + oscSin(droneC, 98.3f) * 0.009f;
        out.addStereo(airL + low * swell, airR + low * swell, 0.0f);
    }

    void startEvent(int type, float stress) {
        if (type == ROOM_EV_FLICKER) {
            if (flickerT >= 0.0f) return;
            flickerT = 0.0f;
            flickerNext = 0.0f;
            flickerDur = rng.range(0.35f, 1.1f);
            flickerPan.set(rng.range(-0.9f, 0.9f));
            return;
        }
        RoomEvent* slot = nullptr;
        for (int i = 0; i < EVENT_SLOTS; i++) {
            if (events[i].type < 0) slot = &events[i];
        }
        if (!slot) return;

        RoomEvent& e = *slot;
        e = RoomEvent();
        e.type = type;
        e.pan.set(rng.range(-0.85f, 0.85f));
        e.gain = 0.75f + stress * 0.5f;
        e.farCoeff = aCoeff(rng.range(1100.0f, 2600.0f));
        if (type == ROOM_EV_PIPE) {
            e.dur = rng.range(2.0f, 3.2f);
            e.a = rng.range(62.0f, 104.0f);
            e.f1.set(e.a * rng.range(3.5f, 5.0f), 3.0f);
        } else if (type == ROOM_EV_VENT) {
            e.dur = rng.range(2.6f, 4.2f);
            e.f1.set(rng.range(70.0f, 130.0f), 1.3f);
            e.f2.set(520.0f, 0.7f);
        } else if (type == ROOM_EV_SCRAPE) {
            e.dur = rng.range(0.9f, 1.7f);
            e.a = rng.range(380.0f, 900.0f);
        } else if (type == ROOM_EV_KNOCK) {
            e.a = rng.range(0.15f, 0.30f);
            e.b = (float)(2 + (int)(rng.uni() * 3.0f));
            e.dur = e.a * e.b + 0.4f;
            e.f1.set(rng.range(800.0f, 1400.0f), 1.6f);
        } else if (type == ROOM_EV_CLANG) {
            e.dur = 2.8f;
            e.a = rng.range(140.0f, 250.0f);
            e.f1.set(2400.0f, 0.8f);
        } else if (type == ROOM_EV_STEPS) {
            e.a = rng.range(0.42f, 0.62f);
            e.b = (float)(4 + (int)(rng.uni() * 4.0f));
            e.dur = e.a * e.b + 0.4f;
            e.ph[2] = rng.range(-0.8f, 0.8f);
            e.ph[3] = rng.range(-0.22f, 0.22f);
            e.f1.set(420.0f, 1.0f);
        } else if (type == ROOM_EV_MOAN) {
            e.dur = rng.range(2.8f, 4.4f);
            e.a = rng.range(98.0f, 150.0f);
            e.f2.set(rng.range(950.0f, 1250.0f), 8.0f);
        }
    }

    // A run of dull hits: knocking on a wall, or someone walking where nobody should be.
    float thumpSeries(RoomEvent& e, float fHi, float fLo, float decay, float clickAmt) {
        int idx = (int)(e.t / e.a);
        if (idx >= (int)e.b) return 0.0f;
        float lt = e.t - (float)idx * e.a;
        if (idx != e.hit) {
            e.hit = idx;
            e.ph[0] = 0.0f;
            e.c = rng.range(0.65f, 1.0f);
            if (e.type == ROOM_EV_STEPS) e.pan.set(e.ph[2] + e.ph[3] * (float)idx);
        }
        float body = oscSin(e.ph[0], fLo + (fHi - fLo) * expf(-lt * 30.0f)) * aPluck(lt, 0.003f, decay);
        float click = e.f1.bp(rng.bi() * expf(-lt * 240.0f)) * clickAmt;
        return (body + click) * e.c;
    }

    float renderEvent(RoomEvent& e) {
        float k = e.t / e.dur;
        float slow = e.slow.next(rng, 2600, 0.0012f);
        if (e.type == ROOM_EV_PIPE) {
            // Old plumbing under load: a low groan that sags and shudders.
            float f = e.a * (1.0f - 0.06f * k) * (1.0f + 0.015f * slow);
            float tone = oscSaw(e.ph[0], f) * 0.6f + oscSin(e.ph[1], f * 2.76f) * 0.25f + oscSin(e.ph[2], f * 0.5f) * 0.5f;
            return e.f1.lp(tone) * aSwell(e.t, e.dur, 0.35f, 1.5f) * (0.6f + 0.4f * slow) * 0.16f;
        }
        if (e.type == ROOM_EV_VENT) {
            e.brown = (e.brown + 0.03f * rng.bi()) * 0.995f;
            float air = e.f1.bp(e.brown) * 1.5f + e.f2.lp(e.pink.next(rng)) * 0.5f;
            return air * aSwell(e.t, e.dur, 0.9f, 1.6f) * 0.34f;
        }
        if (e.type == ROOM_EV_SCRAPE) {
            // Furniture dragged across a floor somewhere.
            e.f1.set(e.a * (1.0f + 0.4f * k) + 140.0f * slow, 3.0f);
            float drag = e.f1.bp(e.pink.next(rng));
            return drag * aSwell(e.t, e.dur, 0.08f, 0.5f) * (0.2f + 0.8f * fabsf(slow)) * 0.34f;
        }
        if (e.type == ROOM_EV_KNOCK) return thumpSeries(e, 150.0f, 72.0f, 24.0f, 0.5f) * 0.36f;
        if (e.type == ROOM_EV_STEPS) return thumpSeries(e, 95.0f, 52.0f, 20.0f, 0.25f) * 0.30f;
        if (e.type == ROOM_EV_CLANG) {
            // Metal door: inharmonic partials of a struck plate, each dying at its own pace.
            static const float ratio[4] = {1.0f, 2.76f, 5.40f, 8.93f};
            static const float amp[4] = {1.0f, 0.55f, 0.30f, 0.16f};
            static const float decay[4] = {2.0f, 3.2f, 5.5f, 8.5f};
            float sum = 0.0f;
            for (int i = 0; i < 4; i++) sum += oscSin(e.ph[i], e.a * ratio[i]) * amp[i] * expf(-e.t * decay[i]);
            float slam = e.f1.lp(rng.bi()) * expf(-e.t * 60.0f) * 1.2f;
            float attack = e.t < 0.004f ? e.t / 0.004f : 1.0f;
            return (sum + slam) * attack * 0.13f;
        }
        if (e.type == ROOM_EV_MOAN) {
            // Almost a voice: a sagging tone squeezed through two vowel formants.
            float bend = sinf(3.14159265f * k) * 2.5f - k * 2.0f;
            float vibrato = sinf(AUDIO_TAU * 4.6f * e.t) * 0.012f * k;
            float f = e.a * powf(2.0f, bend / 12.0f) * (1.0f + vibrato);
            float src = oscSaw(e.ph[0], f) + e.pink.next(rng) * 0.5f;
            e.f1.set(560.0f + 140.0f * slow, 7.0f);
            float voice = e.f1.bp(src) + e.f2.bp(src) * 0.45f;
            return voice * aSwell(e.t, e.dur, 1.2f, 1.6f) * 0.06f;
        }
        return 0.0f;
    }

    int pickEvent(float stress) {
        float weight[ROOM_EV_COUNT] = {20.0f, 20.0f, 10.0f, 14.0f, 11.0f, 16.0f, 4.0f, 0.0f};
        weight[ROOM_EV_STEPS] += stress * 18.0f;
        weight[ROOM_EV_KNOCK] += stress * 8.0f;
        if (stress > 0.25f) weight[ROOM_EV_MOAN] = stress * 16.0f;
        if (lastEvent >= 0) weight[lastEvent] = 0.0f;
        float total = 0.0f;
        for (int i = 0; i < ROOM_EV_COUNT; i++) total += weight[i];
        float roll = rng.uni() * total;
        for (int i = 0; i < ROOM_EV_COUNT; i++) {
            roll -= weight[i];
            if (roll < 0.0f) return i;
        }
        return ROOM_EV_PIPE;
    }

    void renderEvents(float stress, AudioFrame& out) {
        clock += AUDIO_DT;
        if (clock >= nextEvent) {
            float gap = 4.5f - stress * 2.0f;
            nextEvent = clock + gap + rng.uni() * 5.0f;
            lastEvent = pickEvent(stress);
            startEvent(lastEvent, stress);
        }
        for (int i = 0; i < EVENT_SLOTS; i++) {
            RoomEvent& e = events[i];
            if (e.type < 0) continue;
            float s = e.farLp.lp(renderEvent(e), e.farCoeff) * e.gain;
            out.add(s * 1.3f, e.pan, 3.2f);
            e.t += AUDIO_DT;
            if (e.t >= e.dur) e.type = -1;
        }
    }

    // stress: 0 calm .. 1 hunted and losing it. Makes the far events closer together and nastier.
    void render(float humVol, float stress, AudioFrame& out) {
        renderLights(humVol, out);
        renderAir(out);
        renderEvents(stress, out);
    }
};
