#pragma once
// Building blocks of the procedural audio engine: noise, oscillators, filters, panning, reverb.
// Everything here is plain math on floats, so tests and tools can include it without windows.h.

#include <cmath>
#include <cstdint>

const float AUDIO_SR = 44100.0f;
const float AUDIO_DT = 1.0f / AUDIO_SR;
const float AUDIO_TAU = 6.2831853f;

// The audio thread owns its own generator: rand() is neither fast nor fine-grained enough for noise.
struct AudioRng {
    uint32_t s = 0x9E3779B9u;

    uint32_t bits() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    // [0, 1)
    float uni() { return (float)(bits() >> 8) * (1.0f / 16777216.0f); }
    // [-1, 1)
    float bi() { return uni() * 2.0f - 1.0f; }
    float range(float lo, float hi) { return lo + (hi - lo) * uni(); }
};

inline float aClamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float aLerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// One-pole smoothing coefficient for a cutoff in Hz (also used to de-zipper gameplay parameters).
inline float aCoeff(float hz) {
    return 1.0f - expf(-AUDIO_TAU * hz * AUDIO_DT);
}

// Attack ramp followed by an exponential decay.
inline float aPluck(float t, float attack, float decay) {
    if (t < 0.0f) return 0.0f;
    if (t < attack) return t / attack;
    return expf(-(t - attack) * decay);
}

// Smooth 0..1..0 window over [0, dur] with separate fade-in and fade-out times.
inline float aSwell(float t, float dur, float fadeIn, float fadeOut) {
    if (t <= 0.0f || t >= dur) return 0.0f;
    float a = t < fadeIn ? t / fadeIn : 1.0f;
    float left = dur - t;
    float b = left < fadeOut ? left / fadeOut : 1.0f;
    return a * a * (3.0f - 2.0f * a) * b * b * (3.0f - 2.0f * b);
}

inline float oscSin(float& phase, float hz) {
    phase += hz * AUDIO_DT;
    if (phase >= 1.0f) phase -= 1.0f;
    return sinf(AUDIO_TAU * phase);
}

inline float polyBlep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Band-limited sawtooth: rich in harmonics without the aliasing fizz of a naive ramp.
inline float oscSaw(float& phase, float hz) {
    float dt = hz * AUDIO_DT;
    phase += dt;
    if (phase >= 1.0f) phase -= 1.0f;
    return 2.0f * phase - 1.0f - polyBlep(phase, dt);
}

struct OnePole {
    float z = 0.0f;

    float lp(float x, float coeff) {
        z += coeff * (x - z);
        return z;
    }
    float hp(float x, float coeff) { return x - lp(x, coeff); }
};

// Trapezoidal state-variable filter: stable under fast cutoff modulation.
struct Svf {
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, k = 1.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
    float low = 0.0f, band = 0.0f, high = 0.0f;

    void set(float hz, float q) {
        hz = aClamp(hz, 15.0f, AUDIO_SR * 0.45f);
        float g = tanf(3.14159265f * hz * AUDIO_DT);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void tick(float x) {
        float v3 = x - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        low = v2;
        band = v1;
        high = x - k * v1 - v2;
    }
    float lp(float x) {
        tick(x);
        return low;
    }
    // Band-pass with a peak gain of Q: noise keeps its loudness when the band gets narrow.
    float bp(float x) {
        tick(x);
        return band;
    }
    float hp(float x) {
        tick(x);
        return high;
    }
    void reset() { ic1 = ic2 = 0.0f; }
};

// Pink noise (Paul Kellet's economy filter), roughly -1..1.
struct PinkNoise {
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    float next(AudioRng& rng) {
        float white = rng.bi();
        b0 = 0.99765f * b0 + white * 0.0990460f;
        b1 = 0.96300f * b1 + white * 0.2965164f;
        b2 = 0.57000f * b2 + white * 1.0526913f;
        return (b0 + b1 + b2 + white * 0.1848f) * 0.22f;
    }
};

// Smooth random wander in -1..1: a new target every `hold` samples, glided to with `coeff`.
struct SlowRandom {
    float value = 0.0f, target = 0.0f;
    int left = 0;

    float next(AudioRng& rng, int hold, float coeff) {
        if (--left <= 0) {
            left = hold;
            target = rng.bi();
        }
        value += (target - value) * coeff;
        return value;
    }
};

// Equal-power stereo placement, pan in -1 (left) .. 1 (right).
struct PanGain {
    float l = 0.7071f, r = 0.7071f;

    void set(float pan) {
        float a = (aClamp(pan, -1.0f, 1.0f) + 1.0f) * 0.7853982f;
        l = cosf(a);
        r = sinf(a);
    }
};

// One output sample: dry stereo plus a mono send into the shared room reverb.
struct AudioFrame {
    float l = 0.0f, r = 0.0f, wet = 0.0f;

    void add(float x, const PanGain& pan, float send) {
        l += x * pan.l;
        r += x * pan.r;
        wet += x * send;
    }
    void addStereo(float xl, float xr, float send) {
        l += xl;
        r += xr;
        wet += (xl + xr) * 0.5f * send;
    }
    void scale(float g) {
        l *= g;
        r *= g;
        wet *= g;
    }
    void mix(const AudioFrame& o, float g) {
        l += o.l * g;
        r += o.r * g;
        wet += o.wet * g;
    }
};

// ============================================================================
// ROOM REVERB
// Freeverb topology (8 damped combs + 4 allpasses per side). It is what turns dry beeps
// into sounds that happen somewhere inside a huge, carpeted, empty office floor.
// ============================================================================

struct ReverbComb {
    float buf[1700] = {};
    int len = 1700, idx = 0;
    float store = 0.0f;

    float tick(float x, float feedback, float damp) {
        float y = buf[idx];
        store = y + (store - y) * damp;
        buf[idx] = x + store * feedback;
        if (++idx >= len) idx = 0;
        return y;
    }
};

struct ReverbAllpass {
    float buf[600] = {};
    int len = 600, idx = 0;

    float tick(float x) {
        float b = buf[idx];
        buf[idx] = x + b * 0.5f;
        if (++idx >= len) idx = 0;
        return b - x;
    }
};

struct StereoReverb {
    static const int COMBS = 8, ALLPASSES = 4, PREDELAY = 1100;
    ReverbComb combL[COMBS], combR[COMBS];
    ReverbAllpass apL[ALLPASSES], apR[ALLPASSES];
    float pre[PREDELAY] = {};
    int preIdx = 0;
    OnePole inLow, inHigh;
    float feedback = 0.86f;
    float damp = 0.42f;

    StereoReverb() {
        static const int combLen[COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        static const int apLen[ALLPASSES] = {556, 441, 341, 225};
        const int spread = 23;
        for (int i = 0; i < COMBS; i++) {
            combL[i].len = combLen[i];
            combR[i].len = combLen[i] + spread;
        }
        for (int i = 0; i < ALLPASSES; i++) {
            apL[i].len = apLen[i];
            apR[i].len = apLen[i] + spread;
        }
    }

    void tick(float in, float& outL, float& outR) {
        // Keep sub rumble and harsh highs out of the tail, then delay it behind the dry sound.
        float x = inHigh.lp(in, 0.52f);
        x = inLow.hp(x, 0.012f);
        float delayed = pre[preIdx];
        pre[preIdx] = x;
        if (++preIdx >= PREDELAY) preIdx = 0;
        // The tiny offset keeps a dying tail from turning into slow denormal numbers.
        float feed = delayed * 0.03f + 1e-18f;

        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < COMBS; i++) {
            l += combL[i].tick(feed, feedback, damp);
            r += combR[i].tick(feed, feedback, damp);
        }
        for (int i = 0; i < ALLPASSES; i++) {
            l = apL[i].tick(l);
            r = apR[i].tick(r);
        }
        outL = l;
        outR = r;
    }
};
