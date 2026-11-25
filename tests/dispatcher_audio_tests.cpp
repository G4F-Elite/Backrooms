#include "../src/dispatcher_audio.h"
#include <cassert>
#include <vector>

using namespace dispatcher_audio;

static const unsigned char monoWav[] = {
    'R','I','F','F',40,0,0,0, 'W','A','V','E',
    'f','m','t',' ',16,0,0,0, 1,0,1,0, 0x80,0x3e,0,0, 0x00,0x7d,0,0, 2,0,16,0,
    'd','a','t','a',4,0,0,0, 0,0,0,64
};

static void checkInvalid(const std::vector<unsigned char>& bytes) {
    try { decode({bytes.data(), bytes.size()}); }
    catch (const std::runtime_error&) { return; }
    assert(false);
}

static void checkVoiceRoom() {
    constexpr unsigned rate = 44100;
    const unsigned char impulse[] = {0, 64};
    const unsigned char silence[] = {0, 0};
    const Clip click{impulse, 1, 1, rate};
    Player source;
    source.replace(click);
    float direct[2] = {};
    source.mix(direct, 1, rate, 1.f);
    assert(direct[0] == .5f && direct[1] == .5f && !source.busy());

    std::vector<float> uninterrupted(2 * rate);
    source.mix(uninterrupted.data(), rate, rate, 1.f);
    double energy = 0, stereoDifference = 0;
    for (size_t i = 0; i < rate; ++i) {
        const float l = uninterrupted[i * 2], r = uninterrupted[i * 2 + 1];
        energy += l * l + r * r;
        stereoDifference += std::abs(l - r);
        if (i < 1000) assert(std::abs(l) + std::abs(r) < 1e-6f);
        if (i >= rate * 3 / 4) assert(l == 0 && r == 0);
    }
    assert(energy > 1e-6 && energy < 0.05 && stereoDifference > 1e-4);

    // A pause in the gap between the impulse and its first reflection must
    // freeze the room as well as the dry clip. The resumed tail is sample-exact.
    source.replace(click);
    source.mix(direct, 1, rate, 1.f);
    source.setPaused(true);
    std::vector<float> paused(2 * rate);
    source.mix(paused.data(), rate, rate, 1.f);
    for (float value : paused) assert(value == 0);
    source.setPaused(false);
    source.mix(paused.data(), rate, rate, 1.f);
    assert(paused == uninterrupted);

    for (const Clip replacement : {Clip{}, Clip{silence, 1, 1, rate}}) {
        source.replace(click);
        source.mix(direct, 1, rate, 1.f);
        source.replace(replacement);
        std::vector<float> output(2 * rate);
        source.mix(output.data(), rate, rate, 1.f);
        for (float value : output) assert(std::abs(value) < 1e-12f);
    }
    source.replace(click);
    std::vector<float> muted(2 * rate);
    source.mix(muted.data(), rate, rate, 0.f);
    source.mix(muted.data(), rate, rate, 1.f);
    for (float value : muted) assert(value == 0);
}

int main() {
    checkVoiceRoom();
    const Clip clip = decode({monoWav, sizeof monoWav});
    assert(clip.channels == 1 && clip.rate == 16000 && clip.frames == 2);
    for (const auto offset : {0, 4, 8, 12, 16, 20, 22, 32, 34, 36, 40}) {
        std::vector<unsigned char> corrupt(std::begin(monoWav), std::end(monoWav));
        corrupt[offset] = 0xff;
        checkInvalid(corrupt);
    }
    for (size_t cut = 0; cut < sizeof monoWav; ++cut)
        checkInvalid({monoWav, monoWav + cut});
    std::vector<unsigned char> extended(std::begin(monoWav), std::end(monoWav));
    extended.insert(extended.begin() + 12, {'J','U','N','K',1,0,0,0,'x',0});
    extended[4] += 10;
    assert(decode({extended.data(), extended.size()}).frames == 2);

    Player local;
    local.replace(clip);
    float first[4] = {};
    local.mix(first, 2, 32000, 1.f);
    assert(first[0] == 0 && first[1] == 0 && first[2] == .25f && first[3] == .25f && local.busy());
    local.setPaused(true);
    float ambience[4] = {1, 2, 3, 4};
    local.mix(ambience, 2, 32000, .5f);
    assert(ambience[0] == 1 && ambience[1] == 2 && ambience[2] == 3 && ambience[3] == 4 && local.busy());
    local.setPaused(false);
    float last[6] = {};
    local.mix(last, 3, 32000, .5f);
    assert(last[0] == .25f && last[3] == .25f && last[4] == 0 && !local.busy());
    const unsigned char pair[] = {0, 128, 0, 32};
    local.replace({pair, 1, 2, 44100});
    float stereo[2] = {1.f, 0.f};
    local.mix(stereo, 1, 44100, 1.f);
    assert(stereo[0] == 0 && stereo[1] == .25f && !local.busy());

#define DISPATCH_LINE(event, id, words) assert(dispatcherPlayVoice(#id) && dispatcherVoiceBusy());
#include "../src/dispatcher_lines.inc"
#undef DISPATCH_LINE
    assert(!dispatcherPlayVoice("missing-test-clip") && dispatcherVoiceBusy());
    assert(!dispatcherPlayVoice(nullptr) && dispatcherVoiceBusy());
    dispatcherPauseVoice(true);
    dispatcherStopVoice();
    assert(!dispatcherVoiceBusy());
    dispatcher_audio::player.replace({pair, 1, 2, 44100});
    float afterStop[2] = {};
    dispatcher_audio::player.mix(afterStop, 1, 44100, 1.f);
    assert(afterStop[0] == -1 && afterStop[1] == .25f && !dispatcherVoiceBusy());
}
