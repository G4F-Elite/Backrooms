#pragma once
// The sound "mixing desk": game parameters on one side, synth controls on the other, and a table
// of routes in between. Every route says "this control follows that parameter along this curve".
// To make the sound react to something new: add a parameter, feed it in audioReadGame(), add a route.

#include "audio_synth.h"

// Everything is 0..1 where 0 means "nothing going on".
enum AudioParam {
    // Written by the game
    AP_DANGER,        // a monster is inside its detection range
    AP_INSANITY,      // 1 - sanity
    AP_HEALTH_LOSS,   // 1 - health
    AP_LOW_STAMINA,
    AP_MOVE,
    AP_SPRINT,
    AP_CROUCH,
    AP_DARK,          // flashlight is off
    AP_MONSTER_NEAR,  // closeness of the monster that matters most
    AP_MONSTER_MENACE,
    AP_MONSTER_ALERT, // 0 wandering .. 1 charging at somebody
    AP_MONSTER_CROWD, // how many of them are in earshot
    AP_HURT,          // being hit right now
    AP_DEPTH,         // how deep the current floor is
    AP_PACING,        // the AI director's intensity
    // Worked out by the audio director
    AP_TENSION,       // "something is wrong"
    AP_PANIC,         // "it is coming for me"
    AP_EXERTION,      // how hard the player breathes
    AP_WHISPER,       // late-stage sanity loss
    AP_COUNT
};

struct AudioParamInfo {
    const char* name;
    float rise, fall; // seconds to follow the game value up / down
};

// Threat parameters rise at once (a warning that fades in is not a warning) and let go slowly.
inline const AudioParamInfo& audioParamInfo(int p) {
    static const AudioParamInfo table[AP_COUNT] = {
        {"danger", 0.06f, 0.9f},  {"insanity", 0.5f, 0.5f},       {"health_loss", 0.05f, 0.6f},
        {"low_stamina", 0.3f, 0.6f}, {"move", 0.05f, 0.08f},      {"sprint", 0.2f, 0.3f},
        {"crouch", 0.1f, 0.1f},   {"dark", 0.3f, 0.3f},           {"monster_near", 0.08f, 0.7f},
        {"monster_menace", 0.1f, 0.6f}, {"monster_alert", 0.03f, 1.2f}, {"monster_crowd", 0.5f, 2.0f},
        {"hurt", 0.01f, 0.5f},    {"depth", 1.0f, 1.0f},          {"pacing", 1.0f, 2.0f},
        {"tension", 0.0f, 0.0f},  {"panic", 0.0f, 0.0f},          {"exertion", 0.0f, 0.0f},
        {"whisper", 0.0f, 0.0f},
    };
    return table[p];
}

// What the synths listen to.
enum AudioDest {
    AD_AMBIENCE_GAIN, // room bed level
    AD_HUM,           // fluorescent lights
    AD_STRESS,        // how often and how nasty the far-away events are
    AD_DRONE,         // tension drone level and brightness
    AD_HEART,         // heartbeat level
    AD_HEART_RATE,    // 0 resting .. 1 racing
    AD_CHASE,         // chase pulse
    AD_MONSTER,       // monster presence bed
    AD_BREATH,
    AD_WHISPER,
    AD_STEP,          // footstep level
    AD_MUFFLE,        // 0 clear .. 1 the whole mix heard through a wall
    AD_REVERB,        // room size
    AD_COUNT
};

struct AudioCurve {
    int n;
    float x[4], y[4];

    float at(float v) const {
        if (v <= x[0]) return y[0];
        for (int i = 1; i < n; i++) {
            if (v <= x[i]) return aLerp(y[i - 1], y[i], (v - x[i - 1]) / (x[i] - x[i - 1]));
        }
        return y[n - 1];
    }
};

enum AudioRouteOp { ROUTE_ADD, ROUTE_MUL, ROUTE_MAX };

struct AudioRoute {
    int dest, param, op;
    AudioCurve curve;
};

inline float audioDestBase(int d) {
    switch (d) {
    case AD_AMBIENCE_GAIN: case AD_HUM: case AD_STEP: case AD_REVERB: return 1.0f;
    default: return 0.0f;
    }
}

inline const AudioRoute* audioRoutes(int& count) {
    static const AudioRoute table[] = {
        // The room holds its breath when something hunts: the bed drops, lights sag near a monster.
        {AD_AMBIENCE_GAIN, AP_PANIC, ROUTE_MUL, {3, {0.0f, 0.5f, 1.0f}, {1.0f, 0.8f, 0.5f}}},
        {AD_AMBIENCE_GAIN, AP_DARK, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 1.12f}}},
        {AD_HUM, AP_MONSTER_NEAR, ROUTE_MUL, {3, {0.0f, 0.5f, 1.0f}, {1.0f, 0.85f, 0.4f}}},
        {AD_HUM, AP_DEPTH, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 1.3f}}},
        {AD_STRESS, AP_DANGER, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 0.6f}}},
        {AD_STRESS, AP_INSANITY, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 0.35f}}},
        {AD_STRESS, AP_DEPTH, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 0.2f}}},
        {AD_STRESS, AP_DARK, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 0.08f}}},
        // Tension: the drone comes in early, the heart later, the chase pulse only when hunted.
        {AD_DRONE, AP_TENSION, ROUTE_ADD, {4, {0.0f, 0.12f, 0.5f, 1.0f}, {0.0f, 0.25f, 0.7f, 1.0f}}},
        {AD_HEART, AP_TENSION, ROUTE_ADD, {3, {0.25f, 0.5f, 1.0f}, {0.0f, 0.7f, 1.0f}}},
        {AD_HEART, AP_HEALTH_LOSS, ROUTE_MAX, {3, {0.55f, 0.8f, 1.0f}, {0.0f, 0.6f, 0.9f}}},
        {AD_HEART_RATE, AP_TENSION, ROUTE_ADD, {2, {0.3f, 1.0f}, {0.0f, 0.6f}}},
        {AD_HEART_RATE, AP_PANIC, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 0.4f}}},
        {AD_HEART_RATE, AP_EXERTION, ROUTE_ADD, {2, {0.3f, 1.0f}, {0.0f, 0.3f}}},
        {AD_CHASE, AP_PANIC, ROUTE_ADD, {3, {0.3f, 0.5f, 1.0f}, {0.0f, 0.75f, 1.0f}}},
        // A monster is audible long before it is close, and loud when it is.
        {AD_MONSTER, AP_MONSTER_NEAR, ROUTE_ADD, {4, {0.0f, 0.08f, 0.4f, 1.0f}, {0.0f, 0.22f, 0.7f, 1.5f}}},
        {AD_MONSTER, AP_MONSTER_MENACE, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 1.7f}}},
        {AD_MONSTER, AP_MONSTER_ALERT, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 1.35f}}},
        // The player's body.
        {AD_BREATH, AP_EXERTION, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 1.0f}}},
        {AD_WHISPER, AP_WHISPER, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 1.0f}}},
        {AD_STEP, AP_CROUCH, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 0.45f}}},
        {AD_STEP, AP_SPRINT, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 1.2f}}},
        {AD_MUFFLE, AP_HEALTH_LOSS, ROUTE_ADD, {3, {0.5f, 0.8f, 1.0f}, {0.0f, 0.3f, 0.6f}}},
        {AD_MUFFLE, AP_HURT, ROUTE_ADD, {2, {0.0f, 1.0f}, {0.0f, 0.3f}}},
        // Deeper floors are bigger and emptier.
        {AD_REVERB, AP_DEPTH, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 1.6f}}},
        {AD_REVERB, AP_PANIC, ROUTE_MUL, {2, {0.0f, 1.0f}, {1.0f, 0.75f}}},
    };
    count = (int)(sizeof(table) / sizeof(table[0]));
    return table;
}

struct AudioParams {
    float target[AP_COUNT] = {}; // what the game says
    float value[AP_COUNT] = {};  // what the synths hear
    float destTarget[AD_COUNT] = {};
    float dest[AD_COUNT] = {};

    AudioParams() {
        for (int d = 0; d < AD_COUNT; d++) dest[d] = destTarget[d] = audioDestBase(d);
    }

    // Control rate: follow the game values, each at its own speed.
    void follow(float dt) {
        for (int p = 0; p < AP_TENSION; p++) {
            const AudioParamInfo& info = audioParamInfo(p);
            float goal = aClamp(target[p], 0.0f, 1.0f);
            float time = goal > value[p] ? info.rise : info.fall;
            value[p] += (goal - value[p]) * (time > dt ? dt / time : 1.0f);
        }
    }

    void route() {
        for (int d = 0; d < AD_COUNT; d++) destTarget[d] = audioDestBase(d);
        int count = 0;
        const AudioRoute* routes = audioRoutes(count);
        for (int i = 0; i < count; i++) {
            const AudioRoute& r = routes[i];
            float v = r.curve.at(value[r.param]);
            float& out = destTarget[r.dest];
            if (r.op == ROUTE_ADD) out += v;
            else if (r.op == ROUTE_MUL) out *= v;
            else if (v > out) out = v;
        }
    }

    // Sample rate: no control ever jumps.
    void glide() {
        for (int d = 0; d < AD_COUNT; d++) dest[d] += (destTarget[d] - dest[d]) * 0.004f;
    }
};
