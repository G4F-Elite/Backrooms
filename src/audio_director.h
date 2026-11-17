#pragma once
// Reads the game parameters and decides what the moment calls for: how tense it is, which
// threat stage the player is in, and when a warning has to cut through - at once, not faded in.

#include "audio_params.h"
#include "audio_cues.h"

// What the monster that matters most just did. The game maps its AI moods onto these.
enum AudioMonsterCall {
    MONSTER_CALL_NONE = 0, // wandering, retreating, stunned
    MONSTER_CALL_NOTICE,   // heard or suspects something
    MONSTER_CALL_ALERT     // found somebody: hunting, charging, enraged
};

enum ThreatStage { THREAT_CALM, THREAT_UNEASY, THREAT_STALKED, THREAT_HUNTED };

struct MonsterInfo {
    float type = 0.0f; // 0 stalker, 0.5 crawler, 1 shadow
    float pan = 0.0f;
    int call = MONSTER_CALL_NONE;
};

struct AudioDirector {
    static constexpr float CONTROL_DT = 32.0f / 44100.0f;
    AudioParams params;
    AudioRng rng;
    int stage = THREAT_CALM;
    float calmTimer = 0.0f;
    int lastCall = MONSTER_CALL_NONE;
    float callTimer = 3.0f;
    float hurtHold = 0.0f, hitTimer = 0.0f;
    bool wasHurt = false;
    int controlLeft = 0;

    AudioDirector() { rng.s = 0x2545F491u; }

    static int monsterKind(float type) { return type < 0.25f ? 0 : (type < 0.75f ? 1 : 2); }

    // The game reports damage every frame it happens; this turns that into "is being mauled".
    void noteHit() { hurtHold = 0.22f; }

    void derive() {
        float* v = params.value;
        float seen = v[AP_MONSTER_NEAR] * (0.4f + 0.6f * v[AP_MONSTER_ALERT]);
        float tension = (v[AP_DANGER] > seen ? v[AP_DANGER] : seen) * 0.85f;
        v[AP_TENSION] = aClamp(tension + v[AP_PACING] * 0.15f + v[AP_MONSTER_CROWD] * 0.10f + v[AP_HURT] * 0.3f, 0.0f, 1.0f);
        v[AP_PANIC] = aClamp(v[AP_MONSTER_ALERT] * v[AP_MONSTER_NEAR] * 1.7f + v[AP_HURT] * 0.7f, 0.0f, 1.0f);
        v[AP_EXERTION] = aClamp(v[AP_LOW_STAMINA] * 0.82f + v[AP_SPRINT] * 0.35f + v[AP_MONSTER_NEAR] * 0.22f + v[AP_HURT] * 0.3f, 0.0f, 1.0f);
        v[AP_WHISPER] = aClamp((v[AP_INSANITY] - 0.82f) / 0.18f, 0.0f, 1.0f);
    }

    // Stages go up the moment the numbers say so and come down only after things stay quiet.
    void updateStage(CuePlayer& cues, const MonsterInfo& m) {
        const float* v = params.value;
        int want = THREAT_CALM;
        if (v[AP_PANIC] > 0.5f) want = THREAT_HUNTED;
        else if (v[AP_TENSION] > 0.42f) want = THREAT_STALKED;
        else if (v[AP_TENSION] > 0.15f) want = THREAT_UNEASY;
        if (want > stage) {
            int from = stage;
            stage = want;
            calmTimer = 0.0f;
            if (stage == THREAT_HUNTED) cues.start(CUE_WARN_HUNT, 1.0f, m.pan * 0.3f);
            else if (stage == THREAT_STALKED && from < THREAT_STALKED) cues.start(CUE_WARN_NEAR, 0.9f, m.pan * 0.5f);
        } else if (want < stage) {
            calmTimer += CONTROL_DT;
            if (calmTimer > 2.5f) {
                stage--;
                calmTimer = 0.0f;
            }
        } else {
            calmTimer = 0.0f;
        }
    }

    // The monster's own voice: an alarm when its mind changes, and calls that come faster as it closes in.
    void updateMonster(CuePlayer& cues, const MonsterInfo& m) {
        float close = params.value[AP_MONSTER_NEAR];
        float alert = params.value[AP_MONSTER_ALERT];
        int kind = monsterKind(m.type);
        float pan = m.pan * 0.85f;
        float away = (1.0f - close) * 0.9f;
        float level = 0.35f + 0.75f * close;

        if (m.call != lastCall) {
            bool louder = m.call > lastCall;
            lastCall = m.call;
            if (louder && close > 0.03f) {
                if (m.call == MONSTER_CALL_ALERT) cues.start(CUE_ALERT_STALKER + kind, level * 1.1f, pan, 1.0f, away * 0.6f);
                else cues.start(CUE_NOTICE, level, pan, kind == 0 ? 0.6f : (kind == 1 ? 1.5f : 1.0f), away);
                callTimer = rng.range(1.5f, 2.5f);
            }
        }
        if (close < 0.06f) {
            if (callTimer < 1.0f) callTimer = 1.0f;
            return;
        }
        callTimer -= CONTROL_DT;
        if (callTimer > 0.0f) return;
        callTimer = aLerp(6.5f, 1.6f, close) * (1.0f - 0.35f * alert) * rng.range(0.7f, 1.3f);
        cues.start(CUE_CALL_STALKER + kind, level * (0.8f + 0.3f * alert), pan, rng.range(0.9f, 1.1f), away);
    }

    void updateHurt(CuePlayer& cues) {
        bool hurt = hurtHold > 0.0f;
        hurtHold -= CONTROL_DT;
        params.target[AP_HURT] = hurt ? 1.0f : 0.0f;
        if (hurt && !wasHurt) {
            cues.start(CUE_SCARE, 0.9f);
            hitTimer = 0.45f;
        }
        wasHurt = hurt;
        if (!hurt) return;
        hitTimer -= CONTROL_DT;
        if (hitTimer > 0.0f) return;
        hitTimer = rng.range(0.30f, 0.48f);
        cues.start(CUE_HIT, rng.range(0.75f, 1.0f), rng.range(-0.5f, 0.5f), rng.range(0.85f, 1.2f));
    }

    // Once per sample; the decisions themselves run at control rate.
    void tick(CuePlayer& cues, const MonsterInfo& m) {
        if (--controlLeft <= 0) {
            controlLeft = 32;
            updateHurt(cues);
            params.follow(CONTROL_DT);
            derive();
            updateStage(cues, m);
            updateMonster(cues, m);
            params.route();
        }
        params.glide();
    }
};
