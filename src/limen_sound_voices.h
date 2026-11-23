#pragma once
// Part of limen_sound.h: oscillators, filters, voices, reverb.

// ------------------------------------------------------------------ DSP primitives
struct LsRng {
    u32 s = 0x2545F491u;
    u32 next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float)(next() >> 8) * (1.0f / 16777216.0f); }
    float bi() { return uni() * 2.0f - 1.0f; }
    float range(float a, float b) { return a + (b - a) * uni(); }
};

// Trapezoidal state-variable filter: stays stable while its frequency is being swept.
struct LsSvf {
    float ic1 = 0.0f, ic2 = 0.0f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, lpOut = 0.0f;
    void set(float hz, float q) {
        float g = tanf(3.14159265f * lsClamp(hz, 10.0f, 18000.0f) * LS_DT);
        float k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    float bp(float x) {
        float v3 = x - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1 + 1e-20f; ic2 = 2.0f * v2 - ic2 + 1e-20f;
        lpOut = v2;
        return v1;
    }
    float lp(float x) { bp(x); return lpOut; }
};

// Sine/cosine pair advanced by rotation: no sinf() per sample for the steady tones.
struct LsOsc {
    float s = 0.0f, c = 1.0f, ds = 0.0f, dc = 1.0f, hz = -1.0f;
    void set(float f) {
        if (fabsf(f - hz) < 1e-4f) return;
        hz = f;
        float w = LS_TAU * f * LS_DT;
        ds = sinf(w); dc = cosf(w);
    }
    float tick() {
        float ns = s * dc + c * ds;
        c = c * dc - s * ds;
        s = ns;
        return s;
    }
    void renorm() {
        float m = s * s + c * c;
        if (m > 1e-12f) { m = 1.0f / sqrtf(m); s *= m; c *= m; } else { s = 0.0f; c = 1.0f; }
    }
};

struct LsPink {
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float next(LsRng& r) {
        float w = r.bi();
        b0 = 0.99765f * b0 + w * 0.0990460f;
        b1 = 0.96300f * b1 + w * 0.2965164f;
        b2 = 0.57000f * b2 + w * 1.0526913f;
        return (b0 + b1 + b2 + w * 0.1848f) * 0.25f;
    }
};

// Value noise in time: a slowly wandering number in -1..1 (gusts, uneven burning).
struct LsWander {
    float a = 0.0f, b = 0.0f, t = 1.0f;
    float next(LsRng& r, float rate) {
        t += rate;
        if (t >= 1.0f) { t -= 1.0f; a = b; b = r.bi(); }
        float u = t * t * (3.0f - 2.0f * t);
        return a + (b - a) * u;
    }
};

struct LsPan {
    float l = 0.7071f, r = 0.7071f;
    void set(float p) {
        float a = (lsClamp(p, -1.0f, 1.0f) + 1.0f) * 0.78539816f;
        l = cosf(a); r = sinf(a);
    }
};

inline float lsPluck(float t, float attack, float decay) {
    if (t < 0.0f) return 0.0f;
    return (1.0f - expf(-t / attack)) * expf(-t * decay);
}

// ------------------------------------------------------------------ voices
struct LsLampVoice {
    u32 id = 0;
    int live = 0;
    float g = 0.0f, gT = 0.0f;
    LsPan pan;
    float lpC = 0.5f, lpS = 0.0f;
    float w1 = 0.0f, w2 = 0.0f, w3 = 0.0f;       // weights of mains harmonics 2, 4, 6
    float cp = 1.0f, sp = 0.0f, cq = 1.0f, sq = 0.0f, cr = 1.0f, sr = 0.0f;   // their phases
    float buzz = 0.0f;
    LsSvf bz, tk;
    bool restless = false;                        // this tube flickers / tries to strike
    float gate = 1.0f, gateT = 1.0f;
    float burst = 0.0f, nextToggle = 0.0f, tickT = -1.0f;
};

struct LsStepVoice {
    float t = -1.0f, dur = 0.3f;
    float pitch = 1.0f, weight = 1.0f, toe = 0.08f;
    float hard = 0.0f, grit = 0.0f, hollow = 0.0f, wet = 0.0f, metal = 0.0f;
    float phase = 0.0f, body = 0.0f, clickDecay = 400.0f;
    float mph[5] = {}, mhz[5] = {};
    LsSvf click, scuff, splash;
    LsPan pan;
};

struct LsDripVoice {
    float t = -1.0f, dur = 0.06f, f0 = 900.0f, sweep = 1.4f, gain = 0.0f, phase = 0.0f, wet = 1.0f;
    LsSvf tick;
    LsPan pan;
};

struct LsCreakVoice {
    int type = -1;          // 0 ping (thermal tick of a plate), 1 groan (stick-slip of a loaded beam)
    float t = 0.0f, dur = 1.0f, f = 200.0f, gain = 0.0f, brown = 0.0f, slipPh = 0.0f, slipHz = 12.0f;
    float ph[3] = {};
    LsSvf res;
    LsWander wander;
    LsPan pan;
};

struct LsEchoTap {
    int d = 4410, dT = 4410;
    float g = 0.0f, gT = 0.0f, lp = 0.0f;
    LsPan pan;
};

struct LsReverb {
    static const int LINES = 4, MAXLEN = 4096, AP = 2, PRE = 4096;
    float buf[LINES][MAXLEN] = {};
    int len[LINES] = {1447, 1811, 2239, 2671}, idx[LINES] = {};
    float lp[LINES] = {}, g[LINES] = {0.7f, 0.7f, 0.7f, 0.7f};
    float apBuf[AP][512] = {};
    int apLen[AP] = {347, 113}, apIdx[AP] = {};
    float pre[PRE] = {};
    int preIdx = 0, preLen = 600;
    float dampC = 0.5f, wetGain = 0.3f, hp = 0.0f;

    void tune(float rt60, float absorb, float meanR, float airDull) {
        float e2 = 0.0f;
        for (int i = 0; i < LINES; i++) {
            g[i] = powf(10.0f, -3.0f * (float)len[i] / (LS_SR * rt60));
            e2 += g[i] * g[i];
        }
        e2 *= 0.25f;
        dampC = lsClamp(0.80f - 1.6f * absorb - 0.25f * airDull, 0.12f, 0.85f);
        // Long tails pile up energy; hold most of that back so a vault is longer, not just louder.
        wetGain = 0.85f * powf(1.0f - e2 > 0.02f ? 1.0f - e2 : 0.02f, 0.22f);
        int target = (int)(lsClamp(2.0f * meanR / LS_SOUND_SPEED, 0.004f, 0.085f) * LS_SR);
        if (target > preLen) preLen++; else if (target < preLen) preLen--;
    }

    void tick(float in, float& outL, float& outR) {
        hp += (in - hp) * 0.012f;       // keep sub rumble out of the tail
        float x = in - hp;
        int rd = preIdx - preLen; if (rd < 0) rd += PRE;
        float delayed = pre[rd];
        pre[preIdx] = x; if (++preIdx >= PRE) preIdx = 0;
        x = delayed;
        for (int a = 0; a < AP; a++) {
            float b = apBuf[a][apIdx[a]];
            float y = b - 0.55f * x;
            apBuf[a][apIdx[a]] = x + 0.55f * y;
            if (++apIdx[a] >= apLen[a]) apIdx[a] = 0;
            x = y;
        }
        float o[LINES], sum = 0.0f;
        for (int i = 0; i < LINES; i++) {
            lp[i] += (buf[i][idx[i]] - lp[i]) * dampC;
            o[i] = lp[i] * g[i];
            sum += o[i];
        }
        sum *= 0.5f;
        for (int i = 0; i < LINES; i++) {
            buf[i][idx[i]] = x + o[i] - sum + 1e-20f;   // Householder feedback: dense, colourless
            if (++idx[i] >= len[i]) idx[i] = 0;
        }
        outL = (o[0] - o[2]) * wetGain;
        outR = (o[1] - o[3]) * wetGain;
    }
};

struct LsMix {
    float amb = 1.0f;   // master * ambience bus
    float sfx = 1.0f;   // master * effects bus * step level
    float hum = 1.0f;   // user's light hum setting relative to its default
};

// Per-layer output power, for calibration and tests.
enum LsLayer { LSL_LAMPS = 0, LSL_ROOM, LSL_DRAFT, LSL_PLANT, LSL_DRONE, LSL_EVENTS, LSL_STEPS, LSL_WET, LSL_COUNT };

inline const char* lsLayerName(int k) {
    static const char* names[LSL_COUNT] = {"lamps", "room", "draught", "plant", "drone", "events", "steps", "reverb"};
    return (k >= 0 && k < LSL_COUNT) ? names[k] : "?";
}
