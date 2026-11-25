#include "../src/dispatcher_audio.h"
#include <cassert>
#include <vector>

using namespace dispatcher_audio;

// 20 ms of stereo Opus with a 440 Hz tone in the left channel, encoded by libopus.
static const unsigned char stereoOpus[] = {
    0x4f, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdf, 0x76, 0x47, 0xd4, 0x01, 0x13, 0x4f, 0x70, 0x75, 0x73,
    0x48, 0x65, 0x61, 0x64, 0x01, 0x02, 0x38, 0x01, 0x80, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f,
    0x67, 0x67, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0xe9, 0x0f, 0x5f, 0xb7, 0x01, 0x2e, 0x4f, 0x70, 0x75, 0x73, 0x54,
    0x61, 0x67, 0x73, 0x06, 0x00, 0x00, 0x00, 0x66, 0x66, 0x6d, 0x70, 0x65, 0x67, 0x01, 0x00, 0x00,
    0x00, 0x14, 0x00, 0x00, 0x00, 0x65, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x72, 0x3d, 0x4c, 0x61, 0x76,
    0x63, 0x20, 0x6c, 0x69, 0x62, 0x6f, 0x70, 0x75, 0x73, 0x4f, 0x67, 0x67, 0x53, 0x00, 0x04, 0xf8,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x45,
    0x35, 0x83, 0x4c, 0x02, 0x60, 0x30, 0x7c, 0x8f, 0xb0, 0x1e, 0xf2, 0xab, 0xf2, 0x20, 0xc9, 0xa6,
    0x00, 0x00, 0x04, 0x08, 0x59, 0xc8, 0xf8, 0xfd, 0xb3, 0x27, 0xdc, 0x17, 0x01, 0x83, 0x45, 0x6c,
    0xec, 0xb2, 0x79, 0x7f, 0x8a, 0x2b, 0xc6, 0x0d, 0x8f, 0xc0, 0x89, 0x42, 0x60, 0x95, 0x1f, 0xfa,
    0x77, 0x83, 0x3f, 0x59, 0x62, 0xb4, 0x27, 0xc8, 0x57, 0x38, 0x9b, 0x60, 0x34, 0xd7, 0x5d, 0xe5,
    0x34, 0xf8, 0x74, 0xbb, 0x37, 0x32, 0x8d, 0x58, 0xaa, 0xa8, 0x5c, 0xc1, 0xc5, 0xe9, 0x52, 0x5a,
    0x18, 0xbc, 0x82, 0xb8, 0xfa, 0x89, 0x2f, 0x41, 0xf3, 0x39, 0x48, 0x22, 0x01, 0x55, 0x15, 0xb1,
    0x66, 0x97, 0xce, 0x76, 0xc4, 0x32, 0x7c, 0x0f, 0xb0, 0x20, 0xf1, 0x99, 0xfe, 0x30, 0xbc, 0xed,
    0xed, 0xc1, 0xff, 0xa8, 0xac, 0xff, 0xb1, 0xb7, 0xcd, 0x48, 0xe2, 0xcf, 0xba, 0xe3, 0xd8, 0xc3,
    0x7e, 0x52, 0xba, 0xb8, 0xdd, 0xfb, 0x5c, 0xfb, 0xec, 0xfc, 0x73, 0xb8, 0x2e, 0xc7, 0x08, 0x5e,
    0xa9, 0x5c, 0x32, 0xe2, 0x28, 0x52,
};

static void checkInvalid(const std::vector<unsigned char>& bytes) {
    try { openClip({bytes.data(), bytes.size()}); }
    catch (const std::runtime_error&) { return; }
    assert(false);
}

// A fully decoded window, for checking the resampler, room response and
// streaming output against contiguous PCM.
static Clip pcmClip(std::vector<opus_int16> samples, unsigned channels, unsigned rate) {
    Clip clip;
    clip.channels = channels;
    clip.rate = rate;
    clip.frames = clip.bufferedFrames = samples.size() / channels;
    clip.pcm = std::move(samples);
    return clip;
}

static std::vector<opus_int16> referencePcm(DispatcherClipBytes bytes, unsigned channels) {
    int error = 0;
    std::unique_ptr<OggOpusFile, decltype(&op_free)> reference(op_open_memory(bytes.data, bytes.size, &error), op_free);
    assert(reference && !error);
    std::vector<opus_int16> out;
    opus_int16 block[11520];
    for (;;) {
        const int n = channels == 1 ? op_read(reference.get(), block, 11520, nullptr)
                                    : op_read_stereo(reference.get(), block, 11520);
        assert(n >= 0);
        if (!n) break;
        out.insert(out.end(), block, block + n * channels);
    }
    return out;
}

static void checkStreaming(DispatcherClipBytes bytes, unsigned channels) {
    const auto pcm = referencePcm(bytes, channels);
    Player streamed, contiguous;
    Clip source = openClip(bytes);
    assert(source.frames == pcm.size() / channels);
    assert(source.bufferedFrames < source.frames);
    // Exercise a game thread dropping its compressed buffer immediately after preparation.
    if (source.decoder) assert(source.compressed.data() != bytes.data);
    streamed.replace(std::move(source));
    contiguous.replace(pcmClip(pcm, channels, 48000));
    const int sizes[] = {1, 509, 1024, 31, 307};
    for (int k = 0; contiguous.busy(); ++k) {
        assert(streamed.busy());
        int frames = sizes[k % 5];
        std::vector<float> a(2*frames), b(2*frames);
        streamed.mix(a.data(), frames, 44100, 0.6f);
        contiguous.mix(b.data(), frames, 44100, 0.6f);
        assert(a == b);
    }
    assert(!streamed.busy());
}

static void checkVoiceRoom() {
    constexpr unsigned rate = 44100;

    Player source;
    source.replace(pcmClip({16384}, 1, rate));
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
    source.replace(pcmClip({16384}, 1, rate));
    source.mix(direct, 1, rate, 1.f);
    source.setPaused(true);
    std::vector<float> paused(2 * rate);
    source.mix(paused.data(), rate, rate, 1.f);
    for (float value : paused) assert(value == 0);
    source.setPaused(false);
    source.mix(paused.data(), rate, rate, 1.f);
    assert(paused == uninterrupted);

    for (bool stop : {false, true}) {
        source.replace(pcmClip({16384}, 1, rate));
        source.mix(direct, 1, rate, 1.f);
        source.replace(stop ? Clip{} : pcmClip({0}, 1, rate));
        std::vector<float> output(2 * rate);
        source.mix(output.data(), rate, rate, 1.f);
        for (float value : output) assert(std::abs(value) < 1e-12f);
    }
    source.replace(pcmClip({16384}, 1, rate));
    std::vector<float> muted(2 * rate);
    source.mix(muted.data(), rate, rate, 0.f);
    source.mix(muted.data(), rate, rate, 1.f);
    for (float value : muted) assert(value == 0);
}

int main() {
    checkVoiceRoom();
    const std::vector<unsigned char> encoded(std::begin(stereoOpus), std::end(stereoOpus));
    Clip stereoClip;
    {
        auto temporary = encoded;
        stereoClip = openClip({temporary.data(), temporary.size()});
    }
    assert(stereoClip.frames == 960 && stereoClip.rate == 48000 && stereoClip.channels == 2);
    long long leftEnergy = 0, rightEnergy = 0;
    for (size_t i = 0; i < stereoClip.bufferedFrames*2; i += 2) {
        leftEnergy += std::abs(static_cast<int>(stereoClip.pcm[i]));
        rightEnergy += std::abs(static_cast<int>(stereoClip.pcm[i+1]));
    }
    assert(leftEnergy > 100000 && leftEnergy > 4 * rightEnergy);
    for (size_t cut = 0; cut < encoded.size(); ++cut)
        checkInvalid({encoded.begin(), encoded.begin() + cut});
    for (size_t offset : {0u, 28u, 121u, 170u}) {
        auto corrupt = encoded;
        corrupt[offset] ^= 0xff;
        checkInvalid(corrupt);
    }
    const auto recording = dispatcherEmbeddedClip("tutorial_garage_1");
    const Clip voice = openClip(recording);
    assert(voice.rate == 48000 && voice.channels == 1 && voice.frames > 48000);
    checkInvalid({recording.data, recording.data + recording.size - 1});
    std::vector<unsigned char> joined(recording.data, recording.data + recording.size);
    joined.insert(joined.end(), encoded.begin(), encoded.end());
    const Clip chained = openClip({joined.data(), joined.size()});
    assert(chained.channels == 2 && chained.frames == voice.frames + stereoClip.frames);
    checkStreaming(recording, 1);
    checkStreaming({joined.data(), joined.size()}, 2);
    auto broken = std::vector<unsigned char>(recording.data, recording.data+recording.size);
    broken[broken.size()/2] ^= 0x81;
    checkInvalid(broken);
    Player local;
    local.replace(pcmClip({0,16384}, 1, 16000));
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
    local.replace(pcmClip({-32768,8192}, 2, 44100));
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
    dispatcher_audio::player.replace(pcmClip({-32768,8192}, 2, 44100));
    float afterStop[2] = {};
    dispatcher_audio::player.mix(afterStop, 1, 44100, 1.f);
    assert(afterStop[0] == -1 && afterStop[1] == .25f && !dispatcherVoiceBusy());
}
