#pragma once
// Pacing director (after Left 4 Dead's "adaptive dramatic pacing" and the Alien: Isolation
// menace gauge). It never makes monsters stronger; it only decides when they press and when
// they leave the squad alone, and nudges a monster towards the players when it got too quiet.

enum DirectorPhase {
    DIRECTOR_BUILD_UP = 0, // monsters hunt freely, spawns allowed
    DIRECTOR_SUSTAIN_PEAK, // intensity just peaked: let the scene play out for a few seconds
    DIRECTOR_PEAK_FADE,    // monsters are sent away until the squad calms down
    DIRECTOR_RELAX         // breather: no spawns, no hints
};

inline constexpr float DIRECTOR_PEAK_INTENSITY = 0.85f;
inline constexpr float DIRECTOR_CALM_INTENSITY = 0.40f;
inline constexpr float DIRECTOR_SUSTAIN_SECONDS = 4.0f;
inline constexpr float DIRECTOR_FADE_MAX_SECONDS = 14.0f;
inline constexpr float DIRECTOR_RELAX_MIN_SECONDS = 20.0f;
inline constexpr float DIRECTOR_RELAX_EXTRA_SECONDS = 12.0f;
inline constexpr float DIRECTOR_HINT_QUIET_SECONDS = 26.0f; // quiet time before a monster gets a hint
inline constexpr float DIRECTOR_HINT_MIN_SECONDS = 9.0f;    // same, at full attention
inline constexpr float DIRECTOR_HINT_REPEAT_SECONDS = 8.0f; // while it stays quiet, the hint is refreshed
inline constexpr float DIRECTOR_MENACE_GAIN = 0.16f;        // intensity per second at full menace
inline constexpr float DIRECTOR_DOWN_SPIKE = 0.35f;         // a squad member going down
inline constexpr float DIRECTOR_DECAY = 0.045f;             // per second, only while nobody is engaged

struct DirectorInput {
    float dt = 0.0f;
    float menace = 0.0f;      // 0..1: how hard the closest monster presses on any squad member
    float damage = 0.0f;      // health lost this frame by the local player, 0..100
    int newDowns = 0;         // squad members that went down or died since the last update
    int standing = 1;         // squad members still on their feet
    int squadSize = 1;
    float attention = 0.0f;   // 0..1, the noise meter the players fill themselves
    bool anyEngaged = false;  // a monster is chasing or attacking right now
    unsigned roll = 0;        // random bits for the relax length
};

struct DirectorOutput {
    bool allowSpawn = true;
    bool backOff = false;    // monsters that are not mid-attack should leave
    bool giveHint = false;   // point one monster at the squad's rough position (one frame)
    float aggression = 1.0f; // speed/eagerness multiplier, 0.75..1.25
};

struct AiDirector {
    DirectorPhase phase = DIRECTOR_BUILD_UP;
    float phaseTimer = 0.0f;
    float intensity = 0.0f;
    float quietTimer = 0.0f;
    float relaxLength = DIRECTOR_RELAX_MIN_SECONDS;

    void reset() {
        *this = AiDirector();
    }
};

inline void directorEnter(AiDirector& d, DirectorPhase phase) {
    d.phase = phase;
    d.phaseTimer = 0.0f;
}

inline float directorHintDelay(float attention) {
    if (attention < 0.0f) attention = 0.0f;
    if (attention > 1.0f) attention = 1.0f;
    return DIRECTOR_HINT_QUIET_SECONDS + (DIRECTOR_HINT_MIN_SECONDS - DIRECTOR_HINT_QUIET_SECONDS) * attention;
}

// The last one standing gets a slower, more forgiving hunt: clutch revives make the best stories.
inline float directorAggression(const DirectorInput& in) {
    float aggression = 1.0f + in.attention * 0.25f;
    if (in.squadSize > 1 && in.standing <= 1) aggression *= 0.78f;
    if (aggression < 0.75f) aggression = 0.75f;
    if (aggression > 1.25f) aggression = 1.25f;
    return aggression;
}

inline DirectorOutput updateDirector(AiDirector& d, const DirectorInput& in) {
    DirectorOutput out;
    out.aggression = directorAggression(in);

    d.intensity += in.menace * DIRECTOR_MENACE_GAIN * in.dt;
    d.intensity += in.damage * 0.012f;
    d.intensity += (float)in.newDowns * DIRECTOR_DOWN_SPIKE;
    if (!in.anyEngaged) d.intensity -= DIRECTOR_DECAY * in.dt;
    if (d.intensity < 0.0f) d.intensity = 0.0f;
    if (d.intensity > 1.0f) d.intensity = 1.0f;

    d.phaseTimer += in.dt;
    if (in.menace < 0.08f) d.quietTimer += in.dt;
    else d.quietTimer = 0.0f;

    if (d.phase == DIRECTOR_BUILD_UP) {
        if (d.intensity >= DIRECTOR_PEAK_INTENSITY) {
            directorEnter(d, DIRECTOR_SUSTAIN_PEAK);
        } else if (d.quietTimer >= directorHintDelay(in.attention)) {
            // Keep pointing until a monster actually shows up: a hint goes stale as the squad moves on.
            d.quietTimer = directorHintDelay(in.attention) - DIRECTOR_HINT_REPEAT_SECONDS;
            out.giveHint = true;
        }
    } else if (d.phase == DIRECTOR_SUSTAIN_PEAK) {
        if (d.phaseTimer >= DIRECTOR_SUSTAIN_SECONDS) directorEnter(d, DIRECTOR_PEAK_FADE);
    } else if (d.phase == DIRECTOR_PEAK_FADE) {
        out.backOff = true;
        out.allowSpawn = false;
        if (d.intensity <= DIRECTOR_CALM_INTENSITY || d.phaseTimer >= DIRECTOR_FADE_MAX_SECONDS) {
            d.relaxLength = DIRECTOR_RELAX_MIN_SECONDS + (float)(in.roll % 1000u) * 0.001f * DIRECTOR_RELAX_EXTRA_SECONDS;
            if (d.intensity > DIRECTOR_CALM_INTENSITY) d.intensity = DIRECTOR_CALM_INTENSITY;
            directorEnter(d, DIRECTOR_RELAX);
        }
    } else {
        // Keep the monsters away for the first stretch, then let stragglers wander back on their own.
        out.backOff = d.phaseTimer < d.relaxLength * 0.5f;
        out.allowSpawn = false;
        d.quietTimer = 0.0f;
        if (d.phaseTimer >= d.relaxLength) directorEnter(d, DIRECTOR_BUILD_UP);
    }
    return out;
}

inline const char* directorPhaseName(DirectorPhase phase) {
    if (phase == DIRECTOR_SUSTAIN_PEAK) return "PEAK";
    if (phase == DIRECTOR_PEAK_FADE) return "FADE";
    if (phase == DIRECTOR_RELAX) return "RELAX";
    return "BUILD UP";
}
