#include <cassert>
#include <cmath>
#include <atomic>
#include <iostream>
#include <vector>

#include "../src/audio.h"

SoundState sndState = {};
std::atomic<bool> audioRunning{false};
HANDLE hEvent = nullptr;

struct Take {
    std::vector<short> data;

    float rms(int channel = -1) const {
        double acc = 0.0;
        int n = 0;
        for (size_t i = 0; i < data.size(); i++) {
            if (channel >= 0 && (int)(i % AUDIO_CHANNELS) != channel) continue;
            double v = data[i] / 32767.0;
            acc += v * v;
            n++;
        }
        return n > 0 ? (float)sqrt(acc / n) : 0.0f;
    }
    float peak() const {
        int p = 0;
        for (short s : data) p = abs((int)s) > p ? abs((int)s) : p;
        return (float)p / 32767.0f;
    }
};

Take render(float seconds) {
    Take t;
    int frames = (int)(seconds * SAMP_RATE);
    t.data.resize((size_t)frames * AUDIO_CHANNELS);
    fillAudio(t.data.data(), frames);
    return t;
}

// Back to an empty, calm room, with the previous test's tails rung out.
void settle() {
    sndState = SoundState();
    render(4.0f);
}

void testSvfStaysStableUnderFastSweeps() {
    Svf f;
    AudioRng rng;
    for (int i = 0; i < 200000; i++) {
        f.set(40.0f + 9000.0f * (0.5f + 0.5f * sinf((float)i * 0.01f)), 0.5f + 9.0f * rng.uni());
        float y = f.bp(rng.bi());
        assert(std::isfinite(y));
        assert(fabsf(y) < 60.0f);
    }
}

void testSawIsBandLimitedAndBounded() {
    float phase = 0.0f;
    for (int i = 0; i < 44100; i++) {
        float y = oscSaw(phase, 110.0f + (float)i * 0.05f);
        assert(y > -1.3f && y < 1.3f);
    }
}

void testReverbRingsOutAndDies() {
    static StereoReverb reverb;
    float l = 0.0f, r = 0.0f;
    double early = 0.0, late = 0.0;
    for (int i = 0; i < SAMP_RATE * 8; i++) {
        reverb.tick(i < 64 ? 1.0f : 0.0f, l, r);
        assert(std::isfinite(l) && std::isfinite(r));
        if (i > SAMP_RATE / 10 && i < SAMP_RATE / 2) early += l * l + r * r;
        if (i > SAMP_RATE * 7) late += l * l + r * r;
    }
    assert(early > 1e-6);        // there is a tail
    assert(late < early * 1e-6); // and it is gone a few seconds later
}

void testCalmRoomIsQuietStereoAndNeverSilent() {
    settle();
    Take t = render(6.0f);
    assert(t.rms() > 0.004f);
    assert(t.rms() < 0.08f);
    assert(t.peak() < 0.6f);
    // The two sides carry different signals, but neither is missing.
    assert(t.rms(0) > 0.002f && t.rms(1) > 0.002f);
    bool differs = false;
    for (size_t i = 0; i + 1 < t.data.size(); i += 2) {
        if (abs(t.data[i] - t.data[i + 1]) > 64) differs = true;
    }
    assert(differs);
}

void testMasterVolumeZeroIsSilence() {
    settle();
    sndState.masterVol = 0.0f;
    render(5.0f);
    triggerScare(1.0f);
    Take t = render(1.0f);
    assert(t.peak() < 0.002f);
}

void testOneShotsAreConsumedAndAudible() {
    settle();
    sndState.ambienceVol = 0.0f;
    render(1.0f);
    float silent = render(0.5f).rms();

    sndState.stepTrig = true;
    Take step = render(0.3f);
    assert(!sndState.stepTrig);
    assert(step.rms() > silent + 0.01f);

    render(3.0f);
    triggerScare(0.8f);
    Take scare = render(0.5f);
    assert(sndState.scareVol == 0.0f);
    assert(scare.rms() > step.rms());
    assert(scare.peak() <= 1.0f);

    render(4.0f);
    sndState.flashlightOn = 1.0f;
    assert(render(0.1f).rms() > silent + 0.003f);

    render(2.0f);
    sndState.scannerBeepTrig = true;
    Take beep = render(0.2f);
    assert(!sndState.scannerBeepTrig);
    assert(beep.rms() > silent + 0.01f);
}

void testThreatLayersGetLouderWithDanger() {
    settle();
    sndState.ambienceVol = 0.0f;
    render(1.0f);
    float calm = render(2.0f).rms();

    sndState.dangerLevel = 0.9f;
    render(2.0f);
    float danger = render(3.0f).rms();
    assert(danger > calm + 0.01f);

    const float types[3] = {0.0f, 0.5f, 1.0f};
    for (float type : types) {
        settle();
        sndState.ambienceVol = 0.0f;
        sndState.monsterType = type;
        sndState.monsterMenace = 0.8f;
        sndState.monsterProximity = 0.7f;
        render(2.0f);
        assert(render(4.0f).rms() > 0.01f);
    }
}

void testMonsterFollowsItsSide() {
    settle();
    sndState.ambienceVol = 0.0f;
    sndState.monsterProximity = 0.9f;
    sndState.monsterMenace = 0.8f;
    sndState.monsterPan = -1.0f;
    render(3.0f);
    Take left = render(4.0f);
    assert(left.rms(0) > left.rms(1) * 1.5f);

    sndState.monsterPan = 1.0f;
    render(3.0f);
    Take right = render(4.0f);
    assert(right.rms(1) > right.rms(0) * 1.5f);
}

void testWorstCaseNeverClips() {
    settle();
    sndState.masterVol = 1.0f;
    sndState.musicVol = sndState.ambienceVol = sndState.sfxVol = sndState.voiceVol = 1.0f;
    sndState.dangerLevel = 1.0f;
    sndState.sanityLevel = 0.0f;
    updateGameplayAudioState(1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f);
    for (int i = 0; i < 12; i++) {
        sndState.stepTrig = true;
        sndState.flashlightOn = (float)(i % 2);
        if (i % 4 == 0) triggerScare(1.0f);
        triggerToySound(TOY_SHOVE, 1.0f, 0.5f);
        triggerToySound(TOY_SQUEAK, 1.0f, 1.0f);
        triggerMenuConfirmSound();
        Take t = render(0.4f);
        // tanh soft clip: loud, but never a hard-clipped square.
        int pinned = 0;
        for (short s : t.data) pinned += (s >= 32766 || s <= -32766) ? 1 : 0;
        assert(pinned == 0);
    }
    sndState.deathMode = true;
    assert(render(3.0f).rms() > 0.01f);
}

void testCurvesAndRoutes() {
    AudioCurve c = {3, {0.0f, 0.5f, 1.0f}, {1.0f, 0.5f, 0.0f}};
    assert(fabsf(c.at(-1.0f) - 1.0f) < 1e-5f);
    assert(fabsf(c.at(0.25f) - 0.75f) < 1e-5f);
    assert(fabsf(c.at(2.0f)) < 1e-5f);

    AudioParams p;
    p.route();
    assert(fabsf(p.destTarget[AD_STEP] - 1.0f) < 1e-5f);
    assert(p.destTarget[AD_HEART] == 0.0f && p.destTarget[AD_MUFFLE] == 0.0f);
    p.value[AP_CROUCH] = 1.0f;
    p.value[AP_HEALTH_LOSS] = 0.9f;
    p.value[AP_DEPTH] = 1.0f;
    p.route();
    assert(p.destTarget[AD_STEP] < 0.5f);       // crouching is quiet
    assert(p.destTarget[AD_HEART] > 0.5f);      // nearly dead: the heart is loud without any monster
    assert(p.destTarget[AD_MUFFLE] > 0.3f);
    assert(p.destTarget[AD_REVERB] > 1.4f);     // deep floors are bigger
}

void testThreatParamsRiseFastAndFallSlow() {
    AudioParams p;
    p.target[AP_MONSTER_ALERT] = 1.0f;
    for (int i = 0; i < 60; i++) p.follow(AudioDirector::CONTROL_DT); // ~45 ms
    assert(p.value[AP_MONSTER_ALERT] > 0.7f);
    p.target[AP_MONSTER_ALERT] = 0.0f;
    for (int i = 0; i < 60; i++) p.follow(AudioDirector::CONTROL_DT);
    assert(p.value[AP_MONSTER_ALERT] > 0.6f);
}

void testEveryCuePlaysAndEnds() {
    for (int cue = CUE_NONE + 1; cue < CUE_COUNT; cue++) {
        static CuePlayer player;
        player = CuePlayer();
        assert(player.start(cue));
        assert(!player.start(cue)); // cooldown
        double early = 0.0;
        for (int i = 0; i < SAMP_RATE * 6; i++) {
            AudioFrame bus[BUS_COUNT];
            float duck = player.render(bus);
            assert(duck >= 0.0f && duck <= 1.0f);
            for (int b = 0; b < BUS_COUNT; b++) {
                assert(std::isfinite(bus[b].l) && std::isfinite(bus[b].r) && fabsf(bus[b].l) < 4.0f);
                // A warning that fades in is not a warning: there must be sound in the first 30 ms.
                if (i < SAMP_RATE * 30 / 1000) early += fabsf(bus[b].l) + fabsf(bus[b].r);
            }
        }
        assert(early > 1.0);
        for (const CueVoice& v : player.voices) assert(v.def == nullptr);
    }
}

void testMonsterAlarmIsImmediate() {
    settle();
    sndState.ambienceVol = 0.0f;
    sndState.monsterProximity = 0.5f;
    sndState.monsterMenace = 0.6f;
    render(1.0f);
    float before = render(0.2f).rms();
    sndState.monsterCall = MONSTER_CALL_ALERT;
    sndState.monsterAlert = 1.0f;
    Take alarm = render(0.15f);
    assert(alarm.rms() > before * 2.0f + 0.03f);
    assert(gAudioEngine.director.stage == THREAT_HUNTED);
    // Staying alert does not fire the alarm again; calming down and coming back does.
    render(4.0f);
    assert(gAudioEngine.cues.sinceStart[CUE_ALERT_STALKER] > 3.5f);
    assert(gAudioEngine.cues.sinceStart[CUE_WARN_HUNT] > 3.5f);
}

void testBeingMauledIsOneScareThenHits() {
    settle();
    for (int frame = 0; frame < 120; frame++) { // two seconds of damage at 60 fps
        triggerMonsterHit();
        render(1.0f / 60.0f);
        assert(!sndState.hitTrig);
    }
    assert(gAudioEngine.cues.sinceStart[CUE_SCARE] > 1.8f); // not restarted every frame
    assert(gAudioEngine.cues.sinceStart[CUE_HIT] < 0.6f);   // but the hits keep landing
    assert(gAudioEngine.director.params.value[AP_HURT] > 0.9f);
    render(1.5f);
    assert(gAudioEngine.director.params.value[AP_HURT] < 0.1f);
    assert(gAudioEngine.cues.sinceStart[CUE_HIT] > 1.0f);
}

void testPostedCuesArePlayed() {
    settle();
    sndState.ambienceVol = 0.0f;
    render(0.5f);
    float silent = render(0.2f).rms();
    audioPostCue(CUE_WARN_NEAR, 1.0f, -1.0f);
    Take t = render(0.3f);
    assert(t.rms() > silent + 0.01f);
    assert(t.rms(0) > t.rms(1));
}

void testToyVoicesOverlap() {
    settle();
    const float dt = 1.0f / (float)SAMP_RATE;
    triggerToySound(TOY_RESPAWN, 1.0f, 1.0f);
    for (int i = 0; i < 2000; i++) toyVoiceSample(dt);
    // A shove on top must not cut the respawn jingle short.
    triggerToySound(TOY_SHOVE, 1.0f, 1.0f);
    int afterShove = (int)(toySoundLength(TOY_SHOVE) / dt) + 16;
    for (int i = 0; i < afterShove; i++) toyVoiceSample(dt);
    float peak = 0.0f;
    for (int i = 0; i < 2000; i++) {
        float s = fabsf(toyVoiceSample(dt));
        if (s > peak) peak = s;
    }
    assert(peak > 0.02f);
    for (int i = 0; i < SAMP_RATE; i++) toyVoiceSample(dt);
    assert(toyVoiceSample(dt) == 0.0f);
}

int main() {
    testSvfStaysStableUnderFastSweeps();
    testSawIsBandLimitedAndBounded();
    testReverbRingsOutAndDies();
    testCalmRoomIsQuietStereoAndNeverSilent();
    testMasterVolumeZeroIsSilence();
    testOneShotsAreConsumedAndAudible();
    testThreatLayersGetLouderWithDanger();
    testMonsterFollowsItsSide();
    testWorstCaseNeverClips();
    testToyVoicesOverlap();
    testCurvesAndRoutes();
    testThreatParamsRiseFastAndFallSlow();
    testEveryCuePlaysAndEnds();
    testMonsterAlarmIsImmediate();
    testBeingMauledIsOneScareThenHits();
    testPostedCuesArePlayed();
    std::cout << "All audio engine tests passed.\n";
    return 0;
}
