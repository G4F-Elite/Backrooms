#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/squad_fx.h"
#include "../src/audio_toy.h"

void testShoveHitsOnlyCloseTargetsInFront() {
    Vec3 me(10.0f, 1.7f, 10.0f);
    float yaw = 0.0f; // facing +Z
    assert(isShoveHit(me, yaw, Vec3(10.0f, 1.7f, 11.5f)));
    assert(isShoveHit(me, yaw, Vec3(10.8f, 1.7f, 11.5f)));
    assert(!isShoveHit(me, yaw, Vec3(10.0f, 1.7f, 8.5f)));  // behind
    assert(!isShoveHit(me, yaw, Vec3(11.8f, 1.7f, 10.0f))); // straight to the side
    assert(!isShoveHit(me, yaw, Vec3(10.0f, 1.7f, 10.0f + SHOVE_RANGE + 0.5f)));
    // Turning around flips the result.
    assert(isShoveHit(me, 3.14159265f, Vec3(10.0f, 1.7f, 8.5f)));
}

void testKnockbackTravelsAFixedDistanceAndStops() {
    Vec3 vel(SHOVE_SPEED, 0.0f, 0.0f);
    Vec3 pos(0.0f, 0.0f, 0.0f);
    int frames = 0;
    while ((vel.x != 0.0f || vel.z != 0.0f) && frames < 2000) {
        Vec3 d = knockbackStep(vel, 1.0f / 120.0f);
        assert(d.x >= 0.0f && d.z == 0.0f && d.y == 0.0f);
        pos += d;
        frames++;
    }
    assert(frames < 2000);
    float expected = SHOVE_SPEED / KNOCKBACK_DAMPING;
    assert(fabsf(pos.x - expected) < 0.2f);
    // Enough to clear half a 5m cell, not enough to cross a room.
    assert(pos.x > 2.0f && pos.x < 4.0f);
}

void testKnockbackDistanceIsFrameRateIndependent() {
    float totals[2] = {0.0f, 0.0f};
    const float steps[2] = {1.0f / 30.0f, 1.0f / 240.0f};
    for (int i = 0; i < 2; i++) {
        Vec3 vel(0.0f, 0.0f, SHOVE_SPEED);
        for (int f = 0; f < 4000 && vel.z != 0.0f; f++) totals[i] += knockbackStep(vel, steps[i]).z;
    }
    assert(fabsf(totals[0] - totals[1]) < 0.35f);
}

void testSoundFalloff() {
    assert(squadSoundFalloff(0.0f) == 1.0f);
    assert(squadSoundFalloff(SQUAD_SOUND_RANGE) == 0.0f);
    assert(squadSoundFalloff(SQUAD_SOUND_RANGE * 2.0f) == 0.0f);
    float prev = 1.0f;
    for (float d = 1.0f; d < SQUAD_SOUND_RANGE; d += 1.0f) {
        float v = squadSoundFalloff(d);
        assert(v <= prev && v >= 0.0f);
        prev = v;
    }
}

void testAwardsPickHighestCounterPerCategory() {
    ShiftStats stats[4] = {};
    bool active[4] = {true, true, true, false};
    stats[0].work = 2;
    stats[1].work = 5;
    stats[1].downs = 1;
    stats[2].downs = 4;
    stats[2].squeaks = 30;
    stats[3].squeaks = 999; // left the session: must not win anything
    ShiftAward awards[SHIFT_AWARD_CATEGORIES];
    int count = pickShiftAwards(stats, active, 4, awards);
    assert(count == 3);
    assert(awards[0].playerId == 1 && awards[0].value == 5);  // work
    assert(awards[1].playerId == 2 && awards[1].value == 4);  // downs
    assert(awards[2].playerId == 2 && awards[2].value == 30); // squeaks
    for (int i = 0; i < count; i++) assert(awards[i].title && awards[i].title[0] && awards[i].unit[0]);
}

void testAwardsSkipEmptyCategoriesAndBreakTiesByLowestId() {
    ShiftStats stats[4] = {};
    bool active[4] = {true, true, true, true};
    ShiftAward awards[SHIFT_AWARD_CATEGORIES];
    assert(pickShiftAwards(stats, active, 4, awards) == 0);

    stats[3].shoves = 7;
    stats[1].shoves = 7;
    assert(pickShiftAwards(stats, active, 4, awards) == 1);
    assert(awards[0].playerId == 1 && awards[0].value == 7);
}

void testStatCounterSaturates() {
    unsigned short v = 65530;
    bumpStat(v, 3);
    assert(v == 65533);
    bumpStat(v, 100);
    assert(v == 65535);
}

void testPacketRoundTrip() {
    char buf[SQUAD_FX_PACKET_LEN];
    ShiftStats none = {};
    SquadFxEvent shove = {2, SQUAD_FX_SHOVE, 1, Vec3(0.6f, 0.0f, -0.8f)};
    encodeSquadFxPacket(buf, 30, shove, none);
    assert(buf[0] == 30);
    SquadFxEvent out = {};
    ShiftStats outStats = {};
    assert(decodeSquadFxPacket(buf, SQUAD_FX_PACKET_LEN, out, outStats));
    assert(out.from == 2 && out.kind == SQUAD_FX_SHOVE && out.arg == 1);
    assert(out.v.x == 0.6f && out.v.z == -0.8f);

    ShiftStats mine = {3, 1, 4, 1, 5, 926};
    SquadFxEvent statsFx = {1, SQUAD_FX_STATS, 0, Vec3(0.0f, 0.0f, 0.0f)};
    encodeSquadFxPacket(buf, 30, statsFx, mine);
    assert(decodeSquadFxPacket(buf, SQUAD_FX_PACKET_LEN, out, outStats));
    assert(out.kind == SQUAD_FX_STATS);
    assert(outStats.work == 3 && outStats.revives == 1 && outStats.downs == 4);
    assert(outStats.shoves == 1 && outStats.squeaks == 5 && outStats.meters == 926);
}

void testPacketRejectsGarbage() {
    char buf[SQUAD_FX_PACKET_LEN];
    ShiftStats none = {};
    SquadFxEvent out = {};
    ShiftStats outStats = {};
    SquadFxEvent ok = {0, SQUAD_FX_SQUEAK, 0, Vec3(1.0f, 0.0f, 0.0f)};
    encodeSquadFxPacket(buf, 30, ok, none);
    assert(!decodeSquadFxPacket(buf, SQUAD_FX_PACKET_LEN - 1, out, outStats));
    assert(!decodeSquadFxPacket(nullptr, SQUAD_FX_PACKET_LEN, out, outStats));
    buf[1] = (char)SQUAD_MAX_PLAYERS; // sender id out of range
    assert(!decodeSquadFxPacket(buf, SQUAD_FX_PACKET_LEN, out, outStats));
    buf[1] = 0;
    buf[2] = 0; // unknown kind
    assert(!decodeSquadFxPacket(buf, SQUAD_FX_PACKET_LEN, out, outStats));
    buf[2] = (char)(SQUAD_FX_STATS + 1);
    assert(!decodeSquadFxPacket(buf, SQUAD_FX_PACKET_LEN, out, outStats));
}

void testCalloutLabels() {
    for (int i = 0; i < SQUAD_CALLOUT_COUNT; i++) assert(squadCalloutLabel(i)[0] != '\0');
    assert(squadCalloutLabel(-1)[0] == '\0');
    assert(squadCalloutLabel(SQUAD_CALLOUT_COUNT)[0] == '\0');
}

void testToyVoicePlaysThenFallsSilent() {
    const float dt = 1.0f / 44100.0f;
    for (int kind = TOY_SQUEAK; kind <= TOY_RESPAWN; kind++) {
        triggerToySound(kind, 1.0f, 1.0f);
        float peak = 0.0f;
        int samples = (int)(toySoundLength(kind) / dt) + 8;
        for (int i = 0; i < samples; i++) {
            float s = toyVoiceSample(dt);
            assert(std::isfinite(s));
            if (fabsf(s) > peak) peak = fabsf(s);
        }
        assert(peak > 0.05f);  // audible
        assert(peak < 1.3f);   // never louder than the mix headroom
        for (int i = 0; i < 64; i++) assert(toyVoiceSample(dt) == 0.0f);
    }
}

void testToyVoiceIgnoresInaudibleTriggers() {
    const float dt = 1.0f / 44100.0f;
    triggerToySound(TOY_SQUEAK, 0.0f);
    for (int i = 0; i < 64; i++) assert(toyVoiceSample(dt) == 0.0f);
}

void testToyVoiceVolumeScales() {
    const float dt = 1.0f / 44100.0f;
    float peaks[2] = {0.0f, 0.0f};
    const float vols[2] = {1.0f, 0.25f};
    for (int k = 0; k < 2; k++) {
        triggerToySound(TOY_SQUEAK, vols[k], 1.0f);
        for (int i = 0; i < 20000; i++) {
            float s = fabsf(toyVoiceSample(dt));
            if (s > peaks[k]) peaks[k] = s;
        }
    }
    assert(peaks[1] < peaks[0] * 0.4f);
}

int main() {
    testShoveHitsOnlyCloseTargetsInFront();
    testKnockbackTravelsAFixedDistanceAndStops();
    testKnockbackDistanceIsFrameRateIndependent();
    testSoundFalloff();
    testAwardsPickHighestCounterPerCategory();
    testAwardsSkipEmptyCategoriesAndBreakTiesByLowestId();
    testStatCounterSaturates();
    testPacketRoundTrip();
    testPacketRejectsGarbage();
    testCalloutLabels();
    testToyVoicePlaysThenFallsSilent();
    testToyVoiceIgnoresInaudibleTriggers();
    testToyVoiceVolumeScales();
    std::cout << "All squad fx tests passed.\n";
    return 0;
}
