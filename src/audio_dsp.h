#pragma once

#include "math.h"

// ============================================================================
// AUDIO DSP UTILITIES
// Uses fast math approximations when g_fastMathEnabled is true
// ============================================================================

struct AudioSafetyState {
    float humNoise = 0.0f;
    float staticNoise = 0.0f;
    float whisperNoise = 0.0f;
    float flashlightEnv = 0.0f;
    float uiMoveEnv = 0.0f;
    float uiConfirmEnv = 0.0f;
    float insaneBurst = 0.0f;
    float limiterEnv = 0.0f;
};

// ============================================================================
// FAST TANH APPROXIMATION
// For soft clipping / saturation effects
// Error: ~2% in typical audio range [-3, 3]
// ============================================================================

inline float fastTanh(float x) {
    // Pade approximation for tanh
    // tanh(x) ≈ x * (27 + x²) / (27 + 9x²) for small x
    // Extended range version using polynomial
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Even faster tanh approximation (less accurate but faster)
inline float veryFastTanh(float x) {
    // Clamp and use simple polynomial
    x = fastClamp(x, -3.0f, 3.0f);
    float x2 = x * x;
    return x * (1.0f - x2 * 0.0925926f); // Approximation
}

// ============================================================================
// FAST SIGMOID
// Common for dynamics processing
// ============================================================================

inline float fastSigmoid(float x) {
    // sigmoid(x) = 1 / (1 + exp(-x))
    // Fast approximation: x / (1 + |x|) scaled to [0, 1]
    return 0.5f + 0.5f * x / (1.0f + fastAbs(x));
}

// ============================================================================
// MIXING FUNCTIONS
// ============================================================================

inline float mixNoise(float prev, float target, float alpha) {
    return fastLerp(prev, target, alpha);
}

// Crossfade between two signals
inline float crossfade(float a, float b, float mix) {
    return fastLerp(a, b, fastClamp01(mix));
}

// Equal power crossfade (better for audio)
inline float crossfadeEqualPower(float a, float b, float mix) {
    mix = fastClamp01(mix);
    float angle = mix * MATH_HALF_PI;
    float gainA = mCos(angle);
    float gainB = mSin(angle);
    return a * gainA + b * gainB;
}

// ============================================================================
// SOFT CLIPPING / SATURATION
// ============================================================================

// Soft clip using tanh
inline float softClip(float x) {
    const float drive = 1.2f;
    return g_fastMathEnabled ? fastTanh(x * drive) : tanhf(x * drive);
}

// Harder clip with more harmonics
inline float hardSoftClip(float x, float threshold) {
    if (fastAbs(x) <= threshold) return x;
    float sign = (x > 0.0f) ? 1.0f : -1.0f;
    float excess = fastAbs(x) - threshold;
    float compressed = threshold + (1.0f - threshold) * fastTanh(excess);
    return sign * compressed;
}

// Waveshaper using polynomial (asymmetric for tube-like sound)
inline float waveShape(float x, float asymmetry) {
    float x2 = x * x;
    float x3 = x2 * x;
    return x + asymmetry * x2 - 0.5f * asymmetry * x3;
}

// ============================================================================
// DYNAMICS PROCESSING
// ============================================================================

// Apply limiter with envelope following
inline float applyLimiter(float x, float& env) {
    float absX = fastAbs(x);
    const float attack = 0.35f;
    const float release = 0.0015f;
    
    // Envelope follower
    if (absX > env) {
        env += (absX - env) * attack;
    } else {
        env += (absX - env) * release;
    }
    
    // Gain reduction
    float gain = 1.0f;
    const float ceiling = 0.92f;
    if (env > ceiling && env > 0.0001f) {
        gain = ceiling / env;
    }
    
    return x * gain;
}

// Compressor with knee
inline float compressWithKnee(float x, float threshold, float ratio, float knee, float& env) {
    float absX = fastAbs(x);
    
    // Envelope follower (fast attack, slow release)
    const float attack = 0.5f;
    const float release = 0.001f;
    if (absX > env) {
        env += (absX - env) * attack;
    } else {
        env += (absX - env) * release;
    }
    
    // Calculate gain reduction
    float db = 20.0f * mLog(env + 1e-10f) * 0.4343f; // Fast log10 approximation
    float overThreshold = db - threshold;
    
    float reduction = 0.0f;
    if (overThreshold > knee) {
        reduction = overThreshold * (1.0f - 1.0f / ratio);
    } else if (overThreshold > -knee) {
        float t = (overThreshold + knee) / (2.0f * knee);
        reduction = t * t * knee * (1.0f - 1.0f / ratio);
    }
    
    float gain = mPow(10.0f, -reduction * 0.05f);
    return x * gain;
}

// ============================================================================
// FILTERS (First Order)
// ============================================================================

// One-pole lowpass filter coefficient from frequency
inline float lowpassCoeffFromFreq(float freq, float sampleRate) {
    // fc = freq / sampleRate
    // coeff = 1 - exp(-2 * pi * fc)
    float fc = freq / sampleRate;
    if (g_fastMathEnabled) {
        // Fast approximation for exp(-x) where x is small
        float x = MATH_TAU * fc;
        return x / (1.0f + x); // Approximation
    }
    return 1.0f - expf(-MATH_TAU * fc);
}

// Apply one-pole lowpass
inline float lowpass1Pole(float input, float& state, float coeff) {
    state += coeff * (input - state);
    return state;
}

// Apply one-pole highpass
inline float highpass1Pole(float input, float& state, float coeff) {
    float lp = lowpass1Pole(input, state, coeff);
    return input - lp;
}

// ============================================================================
// AUDIO OSCILLATORS (for effects/synthesis)
// ============================================================================

// Fast sine oscillator using phase accumulator
inline float sineOsc(float& phase, float freq, float sampleRate) {
    float phaseDelta = freq / sampleRate;
    phase += phaseDelta;
    if (phase >= 1.0f) phase -= 1.0f;
    return mSin(phase * MATH_TAU);
}

// Triangle oscillator
inline float triangleOsc(float& phase, float freq, float sampleRate) {
    float phaseDelta = freq / sampleRate;
    phase += phaseDelta;
    if (phase >= 1.0f) phase -= 1.0f;
    // Triangle wave formula
    return 4.0f * fastAbs(phase - 0.5f) - 1.0f;
}

// Sawtooth oscillator
inline float sawOsc(float& phase, float freq, float sampleRate) {
    float phaseDelta = freq / sampleRate;
    phase += phaseDelta;
    if (phase >= 1.0f) phase -= 1.0f;
    return 2.0f * phase - 1.0f;
}

// Pulse/Square oscillator with variable width
inline float pulseOsc(float& phase, float freq, float sampleRate, float width) {
    float phaseDelta = freq / sampleRate;
    phase += phaseDelta;
    if (phase >= 1.0f) phase -= 1.0f;
    return (phase < width) ? 1.0f : -1.0f;
}

// ============================================================================
// MODULATION HELPERS
// ============================================================================

// LFO with multiple shapes
enum LFOShape {
    LFO_SINE = 0,
    LFO_TRIANGLE,
    LFO_SAW,
    LFO_SQUARE,
    LFO_RANDOM
};

inline float lfo(float& phase, float freq, float sampleRate, LFOShape shape) {
    float phaseDelta = freq / sampleRate;
    phase += phaseDelta;
    if (phase >= 1.0f) phase -= 1.0f;
    
    switch (shape) {
        case LFO_SINE:
            return mSin(phase * MATH_TAU);
        case LFO_TRIANGLE:
            return 4.0f * fastAbs(phase - 0.5f) - 1.0f;
        case LFO_SAW:
            return 2.0f * phase - 1.0f;
        case LFO_SQUARE:
            return (phase < 0.5f) ? 1.0f : -1.0f;
        case LFO_RANDOM:
            // Sample and hold random
            static float lastRandom = 0.0f;
            if (phaseDelta > 0.0f && phase < phaseDelta) {
                lastRandom = ((float)(rand() % 1000) / 500.0f) - 1.0f;
            }
            return lastRandom;
        default:
            return 0.0f;
    }
}

// Exponential decay envelope
inline float expDecay(float time, float decayTime) {
    return mExp(-time / decayTime);
}

// ADSR envelope state
struct ADSREnvelope {
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    State state = IDLE;
    float value = 0.0f;
    float attackTime = 0.01f;
    float decayTime = 0.1f;
    float sustainLevel = 0.7f;
    float releaseTime = 0.3f;
    
    void trigger() { state = ATTACK; }
    void release() { state = RELEASE; }
    
    float process(float sampleRate) {
        float dt = 1.0f / sampleRate;
        
        switch (state) {
            case ATTACK:
                value += dt / attackTime;
                if (value >= 1.0f) {
                    value = 1.0f;
                    state = DECAY;
                }
                break;
            case DECAY:
                value -= dt / decayTime * (1.0f - sustainLevel);
                if (value <= sustainLevel) {
                    value = sustainLevel;
                    state = SUSTAIN;
                }
                break;
            case SUSTAIN:
                // Hold at sustain level
                break;
            case RELEASE:
                value -= dt / releaseTime * sustainLevel;
                if (value <= 0.0f) {
                    value = 0.0f;
                    state = IDLE;
                }
                break;
            default:
                break;
        }
        return value;
    }
};

// ============================================================================
// NOISE GENERATORS
// ============================================================================

// White noise
inline float whiteNoise() {
    return ((float)(rand() % 32768) / 16384.0f) - 1.0f;
}

// Pink noise approximation (using filtered white noise)
inline float pinkNoise(float& b0, float& b1, float& b2) {
    float white = whiteNoise();
    b0 = 0.99765f * b0 + white * 0.0990460f;
    b1 = 0.96300f * b1 + white * 0.2965164f;
    b2 = 0.57000f * b2 + white * 1.0526913f;
    return (b0 + b1 + b2 + white * 0.1848f) * 0.25f;
}

// Brown noise (integrated white noise)
inline float brownNoise(float& prev) {
    float white = whiteNoise();
    prev = (prev + 0.02f * white) * 0.99f; // Leaky integrator
    return prev * 3.5f; // Scale up
}

// ============================================================================
// SCANNER BEEP HELPER
// ============================================================================
inline float scannerBeepSample(float t, float pitch, float vol) {
    float atk = (t < 0.01f) ? (t / 0.01f) : 1.0f;
    float env = atk * expf(-t * 10.5f);
    float f = 260.0f + pitch * 220.0f;
    return (sinf(MATH_TAU * f * t) * 0.26f + sinf(MATH_TAU * f * 2.0f * t) * 0.08f) * vol * env;
}