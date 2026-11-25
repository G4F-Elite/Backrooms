#pragma once

#include "dispatcher_clips.h"
#include "audio_synth.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace dispatcher_audio {

// All views refer to immutable PCM bytes in the executable. No file I/O or decoding
// allocation takes place on either the game thread or the audio thread.
struct Clip {
    const unsigned char* pcm = nullptr;
    size_t frames = 0;
    unsigned channels = 0, rate = 0;
};

inline uint32_t read32(const unsigned char* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

inline unsigned read16(const unsigned char* p) {
    return unsigned(p[0]) | (unsigned(p[1]) << 8);
}

inline Clip decode(DispatcherClipBytes bytes) {
    if (!bytes.data || bytes.size < 12 || std::memcmp(bytes.data, "RIFF", 4) || std::memcmp(bytes.data + 8, "WAVE", 4))
        throw std::runtime_error("missing RIFF/WAVE header");
    const size_t body = read32(bytes.data + 4);
    if (body < 4 || body > bytes.size - 8) throw std::runtime_error("invalid RIFF length");
    const size_t end = body + 8;
    Clip clip;
    size_t pcmLength = 0;
    for (size_t pos = 12; pos < end;) {
        if (end - pos < 8) throw std::runtime_error("truncated WAV chunk header");
        const auto* header = bytes.data + pos;
        const auto* data = header + 8;
        const size_t length = read32(header + 4);
        pos += 8;
        if (length > end - pos) throw std::runtime_error("truncated WAV chunk");
        if (std::memcmp(header, "fmt ", 4) == 0) {
            if (clip.channels || length < 16) throw std::runtime_error("invalid WAV format chunk");
            clip.channels = read16(data + 2);
            clip.rate = read32(data + 4);
            if (read16(data) != 1 || read16(data + 14) != 16 ||
                (clip.channels != 1 && clip.channels != 2) || !clip.rate || read16(data + 12) != 2 * clip.channels)
                throw std::runtime_error("expected 16-bit PCM mono or stereo");
        } else if (std::memcmp(header, "data", 4) == 0) {
            if (clip.pcm) throw std::runtime_error("duplicate WAV data chunk");
            clip.pcm = data;
            pcmLength = length;
        }
        pos += length;
        if ((length & 1u) && pos == end) throw std::runtime_error("missing WAV chunk padding");
        pos += length & 1u;
    }
    if (!clip.channels || !pcmLength || pcmLength % (2 * clip.channels)) throw std::runtime_error("incomplete PCM frames");
    clip.frames = pcmLength / (2 * clip.channels);
    return clip;
}

inline float sample(const Clip& clip, size_t frame, unsigned channel) {
    unsigned v = read16(clip.pcm + (frame * clip.channels + channel) * 2);
    return static_cast<float>(static_cast<int>(v) - ((v & 32768u) ? 65536 : 0));
}

class Player {
    std::mutex lock;
    Clip current;
    double position = 0;
    bool paused = false;
    StereoReverb room;
    int tailFrames = 0;
public:
    void replace(Clip next) {
        std::lock_guard<std::mutex> guard(lock);
        current = next;
        position = 0;
        room = {};
        room.feedback = 0.63f;
        room.damp = 0.57f;
        tailFrames = 0;
        if (!next.frames) paused = false;
    }
    void setPaused(bool value) {
        std::lock_guard<std::mutex> guard(lock);
        paused = value;
    }
    bool busy() {
        std::lock_guard<std::mutex> guard(lock);
        return position < current.frames;
    }
    void mix(float* stereo, int frames, unsigned outputRate, float gain) {
        std::unique_lock<std::mutex> guard(lock, std::try_to_lock);
        if (!guard || paused || (position >= current.frames && !tailFrames)) return;
        const double step = static_cast<double>(current.rate) / outputRate;
        for (int i = 0; i < frames; ++i) {
            float dry[2] = {};
            if (position < current.frames) {
                const size_t first = static_cast<size_t>(position);
                const size_t next = first + 1 < current.frames ? first + 1 : first;
                const float fraction = static_cast<float>(position - first);
                for (unsigned channel = 0; channel < current.channels; ++channel) {
                    const float a = sample(current, first, channel);
                    const float b = sample(current, next, channel);
                    dry[channel] = (a + (b - a) * fraction) / 32768.f;
                }
                if (current.channels == 1) dry[1] = dry[0];
                position += step;
                if (position >= current.frames) tailFrames = static_cast<int>(outputRate * 0.75f);
            } else {
                if (!tailFrames) break;
                --tailFrames;
            }
            // Keep consonants centred and dry. A quiet, delayed stereo return
            // places the speaker beyond the listener without washing out the words.
            float wetL, wetR;
            room.tick((dry[0] + dry[1]) * 0.5f, wetL, wetR);
            stereo[i * 2] += gain * (dry[0] + wetL * 0.28f);
            stereo[i * 2 + 1] += gain * (dry[1] + wetR * 0.28f);
        }
    }
};
inline Player player;
} // namespace dispatcher_audio

inline bool dispatcherPlayVoice(const char* clipId) {
    try {
        if (!clipId || !*clipId) throw std::invalid_argument("empty clip ID");
        const auto bytes = dispatcherEmbeddedClip(clipId);
        if (!bytes.data) throw std::runtime_error("clip missing from the embedded voice catalog");
        dispatcher_audio::player.replace(dispatcher_audio::decode(bytes));
        return true;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Dispatcher voice (%s): %s\n", clipId ? clipId : "<null>", error.what());
        return false;
    }
}

inline void dispatcherStopVoice() { dispatcher_audio::player.replace({}); }
inline void dispatcherPauseVoice(bool paused) { dispatcher_audio::player.setPaused(paused); }
inline bool dispatcherVoiceBusy() { return dispatcher_audio::player.busy(); }
