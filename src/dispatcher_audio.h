#pragma once

#include "dispatcher_opus.h"
#include "audio_synth.h"
#include <cstdio>
#include <mutex>

namespace dispatcher_audio {

class Player {
    std::mutex lock;
    Clip current;
    double position = 0;
    bool paused = false;
    StereoReverb room;
    int tailFrames = 0;
    int decodeError = 0;
public:
    void replace(Clip next) {
        std::lock_guard<std::mutex> guard(lock);
        std::swap(current, next); // dispose of the previous decoder on the caller after unlocking
        position = 0;
        room = {};
        room.feedback = 0.63f;
        room.damp = 0.57f;
        tailFrames = 0;
        if (!current.frames) paused = false;
    }
    void setPaused(bool value) {
        std::lock_guard<std::mutex> guard(lock);
        paused = value;
    }
    bool busy() {
        std::unique_lock<std::mutex> guard(lock);
        const bool playing = position < current.frames;
        const int error = decodeError;
        decodeError = 0;
        guard.unlock();
        if (error) std::fprintf(stderr, "Reading dispatcher Opus packet failed (%d)\n", error);
        return playing;
    }
    void mix(float* stereo, int frames, unsigned outputRate, float gain) {
        std::unique_lock<std::mutex> guard(lock, std::try_to_lock);
        if (!guard || paused) return;
        const size_t totalFrames = current.frames;
        if (position >= totalFrames && !tailFrames) return;
        const double step = static_cast<double>(current.rate) / outputRate;
        for (int i = 0; i < frames; ++i) {
            float dry[2] = {};
            if (position < totalFrames) {
                const size_t first = static_cast<size_t>(position);
                const size_t next = first + 1 < totalFrames ? first + 1 : first;
                if (next >= current.firstFrame + current.bufferedFrames) {
                    decodeError = current.bufferThrough(next);
                    if (decodeError) { position = totalFrames; tailFrames = 0; break; }
                }
                const float fraction = static_cast<float>(position - first);
                for (unsigned channel = 0; channel < current.channels; ++channel) {
                    const float a = current.pcm[(first-current.firstFrame) * current.channels + channel];
                    const float b = current.pcm[(next-current.firstFrame) * current.channels + channel];
                    dry[channel] = (a + (b - a) * fraction) / 32768.f;
                }
                if (current.channels == 1) dry[1] = dry[0];
                position += step;
                if (position >= totalFrames) tailFrames = static_cast<int>(outputRate * 0.75f);
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
        dispatcher_audio::player.replace(dispatcher_audio::openClip(bytes));
        return true;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Dispatcher voice (%s): %s\n", clipId ? clipId : "<null>", error.what());
        return false;
    }
}

inline void dispatcherStopVoice() { dispatcher_audio::player.replace({}); }
inline void dispatcherPauseVoice(bool paused) { dispatcher_audio::player.setPaused(paused); }
inline bool dispatcherVoiceBusy() { return dispatcher_audio::player.busy(); }
