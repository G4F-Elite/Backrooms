#pragma once
// Squad banter layer: shoves, squeaks, callouts and the end-of-shift awards.
// Pure data + rules only (no globals), so the tests can include it standalone.

#include <cmath>
#include <cstring>
#include "math.h"

enum SquadFxKind {
    SQUAD_FX_SHOVE = 1,   // arg = shoved player id, v = push direction
    SQUAD_FX_SQUEAK = 2,  // v.x = pitch
    SQUAD_FX_CALLOUT = 3, // arg = callout index
    SQUAD_FX_STATS = 4    // payload = ShiftStats of the sender
};

struct SquadFxEvent {
    unsigned char from;
    unsigned char kind;
    unsigned char arg;
    Vec3 v;
};

inline constexpr int SQUAD_FX_PACKET_LEN = 32;
inline constexpr int SQUAD_FX_QUEUE_LEN = 16;
inline constexpr int SQUAD_MAX_PLAYERS = 4;

inline constexpr float SHOVE_RANGE = 2.6f;
inline constexpr float SHOVE_FACING_DOT = 0.45f;
inline constexpr float SHOVE_SPEED = 13.0f;
inline constexpr float SHOVE_COOLDOWN = 0.7f;
inline constexpr float KNOCKBACK_DAMPING = 5.0f;
inline constexpr float SQUEAK_COOLDOWN = 0.3f;
inline constexpr float SQUAD_SOUND_RANGE = 32.0f;

inline constexpr int SQUAD_CALLOUT_COUNT = 5;

inline const char* squadCalloutLabel(int idx) {
    static const char* labels[SQUAD_CALLOUT_COUNT] = {"OVER HERE", "DANGER", "NEED BATTERY", "FOUND IT", "I AM STARING AT IT"};
    if (idx < 0 || idx >= SQUAD_CALLOUT_COUNT) return "";
    return labels[idx];
}

// Per-player counters for one run. Shorts keep the wire format small.
struct ShiftStats {
    unsigned short work;    // hauls: deliveries, sales, walls knocked out
    unsigned short revives; // teammates picked up
    unsigned short downs;   // times knocked down
    unsigned short shoves;  // teammates shoved
    unsigned short squeaks; // squeaks made
    unsigned short meters;  // distance covered
};

inline void bumpStat(unsigned short& stat, int amount = 1) {
    int next = (int)stat + amount;
    if (next > 65535) next = 65535;
    stat = (unsigned short)next;
}

// 1 at point blank, 0 at SQUAD_SOUND_RANGE and beyond.
inline float squadSoundFalloff(float dist) {
    if (dist <= 1.0f) return 1.0f;
    if (dist >= SQUAD_SOUND_RANGE) return 0.0f;
    float k = 1.0f - (dist - 1.0f) / (SQUAD_SOUND_RANGE - 1.0f);
    return k * k;
}

// The shove hits whoever stands close and roughly in front of the shover.
inline bool isShoveHit(const Vec3& from, float yaw, const Vec3& target) {
    Vec3 d = target - from;
    d.y = 0.0f;
    float dist = sqrtf(d.x * d.x + d.z * d.z);
    if (dist > SHOVE_RANGE) return false;
    if (dist < 0.05f) return true;
    float facing = (d.x * sinf(yaw) + d.z * cosf(yaw)) / dist;
    return facing >= SHOVE_FACING_DOT;
}

// Advances a decaying knockback: returns this frame's displacement and damps the velocity.
inline Vec3 knockbackStep(Vec3& vel, float dt) {
    Vec3 delta(vel.x * dt, 0.0f, vel.z * dt);
    float damp = expf(-KNOCKBACK_DAMPING * dt);
    vel.x *= damp;
    vel.z *= damp;
    vel.y = 0.0f;
    if (vel.x * vel.x + vel.z * vel.z < 0.09f) vel = Vec3(0.0f, 0.0f, 0.0f);
    return delta;
}

struct ShiftAward {
    const char* title;
    const char* unit;
    int playerId;
    int value;
    int category; // index into the counters: work, revives, downs, shoves, squeaks, meters
};

inline constexpr int SHIFT_AWARD_CATEGORIES = 6;

// One award per category, handed to the player with the highest non-zero counter.
// Ties go to the lower player id so every peer prints the same report.
inline int pickShiftAwards(const ShiftStats* stats, const bool* active, int playerCount, ShiftAward* out) {
    static const char* titles[SHIFT_AWARD_CATEGORIES] = {
        "EMPLOYEE OF THE SHIFT", "FIELD MEDIC", "FLOOR INSPECTOR", "HR VIOLATION", "NOISE COMPLAINT", "CARDIO ENJOYER"
    };
    static const char* units[SHIFT_AWARD_CATEGORIES] = {
        "HAULS", "REVIVES", "TIMES DOWNED", "SHOVES", "SQUEAKS", "METERS RUN"
    };
    int count = 0;
    for (int cat = 0; cat < SHIFT_AWARD_CATEGORIES; cat++) {
        int best = -1;
        int bestValue = 0;
        for (int p = 0; p < playerCount; p++) {
            if (!active[p]) continue;
            const ShiftStats& s = stats[p];
            const unsigned short values[SHIFT_AWARD_CATEGORIES] = {s.work, s.revives, s.downs, s.shoves, s.squeaks, s.meters};
            if ((int)values[cat] > bestValue) {
                bestValue = values[cat];
                best = p;
            }
        }
        if (best < 0) continue;
        out[count++] = {titles[cat], units[cat], best, bestValue, cat};
    }
    return count;
}

inline void encodeSquadFxPacket(char* buf, unsigned char packetType, const SquadFxEvent& fx, const ShiftStats& stats) {
    memset(buf, 0, SQUAD_FX_PACKET_LEN);
    buf[0] = (char)packetType;
    buf[1] = (char)fx.from;
    buf[2] = (char)fx.kind;
    buf[3] = (char)fx.arg;
    if (fx.kind == SQUAD_FX_STATS) memcpy(buf + 4, &stats, sizeof(ShiftStats));
    else memcpy(buf + 4, &fx.v, sizeof(Vec3));
}

inline bool decodeSquadFxPacket(const char* buf, int len, SquadFxEvent& fx, ShiftStats& stats) {
    if (!buf || len < SQUAD_FX_PACKET_LEN) return false;
    fx.from = (unsigned char)buf[1];
    fx.kind = (unsigned char)buf[2];
    fx.arg = (unsigned char)buf[3];
    fx.v = Vec3(0.0f, 0.0f, 0.0f);
    if (fx.from >= SQUAD_MAX_PLAYERS) return false;
    if (fx.kind < SQUAD_FX_SHOVE || fx.kind > SQUAD_FX_STATS) return false;
    if (fx.kind == SQUAD_FX_STATS) memcpy(&stats, buf + 4, sizeof(ShiftStats));
    else memcpy(&fx.v, buf + 4, sizeof(Vec3));
    return true;
}
