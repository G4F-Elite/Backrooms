#pragma once

#include "dispatcher_clips.h"
#include <opusfile.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace dispatcher_audio {

// opusfile accepts interrupted internet streams. The embedded assets are complete
// files: check page framing, checksum, page sequence and EOS before queuing speech.
inline bool completeOgg(DispatcherClipBytes bytes) {
    if (!bytes.data || !bytes.size || bytes.size > LONG_MAX) return false;
    ogg_sync_state sync{};
    ogg_sync_init(&sync);
    const long size = static_cast<long>(bytes.size);
    char* data = ogg_sync_buffer(&sync, size);
    if (!data) {
        ogg_sync_clear(&sync);
        throw std::bad_alloc();
    }
    std::memcpy(data, bytes.data, bytes.size);
    ogg_sync_wrote(&sync, size);
    ogg_page page;
    bool atEnd = false, previousEnded = true;
    int serial = 0;
    long nextPage = 0;
    for (;;) {
        const long consumed = ogg_sync_pageseek(&sync, &page);
        if (consumed <= 0) break;
        if (previousEnded) {
            if (!ogg_page_bos(&page)) break;
            serial = ogg_page_serialno(&page);
            nextPage = 0;
        }
        if (serial != ogg_page_serialno(&page) || nextPage++ != ogg_page_pageno(&page)) break;
        previousEnded = ogg_page_eos(&page) != 0;
        atEnd = previousEnded && page.body + page.body_len == reinterpret_cast<unsigned char*>(data) + size;
    }
    ogg_sync_clear(&sync);
    return atEnd;
}

struct Clip {
    unsigned channels = 1, rate = 48000;
    size_t frames = 0;
    // Declare compressed storage first: the decoder borrows it until op_free.
    std::vector<unsigned char> compressed;
    std::unique_ptr<OggOpusFile, decltype(&op_free)> decoder{nullptr, op_free};
    std::vector<opus_int16> pcm;
    size_t firstFrame = 0, bufferedFrames = 0;

    // The callback reads forward. Preserve the previous packet's final frame
    // so fractional sampling straddles packets exactly as in contiguous PCM.
    int bufferThrough(size_t frame) {
        while (frame >= firstFrame + bufferedFrames) {
            if (!decoder || frame >= frames) return OP_EBADPACKET;
            const size_t carry = bufferedFrames ? 1 : 0;
            if (carry) {
                std::copy_n(pcm.data() + (bufferedFrames - 1) * channels, channels, pcm.data());
                firstFrame += bufferedFrames - 1;
            }
            opus_int16* out = pcm.data() + carry * channels;
            const int room = static_cast<int>(pcm.size() - carry * channels);
            const int got = channels == 1 ? op_read(decoder.get(), out, room, nullptr)
                                          : op_read_stereo(decoder.get(), out, room);
            if (got <= 0) return got ? got : OP_EBADPACKET;
            bufferedFrames = carry + static_cast<unsigned>(got);
            if (bufferedFrames > frames - firstFrame) return OP_EBADTIMESTAMP;
        }
        return 0;
    }
};

inline Clip openClip(DispatcherClipBytes bytes) {
    if (!completeOgg(bytes)) throw std::runtime_error("invalid, damaged or incomplete Ogg recording");
    Clip clip;
    clip.compressed.assign(bytes.data, bytes.data + bytes.size);
    int error = 0;
    clip.decoder.reset(op_open_memory(clip.compressed.data(), clip.compressed.size(), &error));
    if (!clip.decoder) throw std::runtime_error("opening Ogg Opus failed (" + std::to_string(error) + ")");
    const auto expected = op_pcm_total(clip.decoder.get(), -1);
    if (expected <= 0 || static_cast<uint64_t>(expected) > SIZE_MAX)
        throw std::runtime_error("invalid Ogg Opus duration: " + std::to_string(expected));
    clip.frames = static_cast<size_t>(expected);
    for (int i = 0; i < op_link_count(clip.decoder.get()); ++i)
        if (op_channel_count(clip.decoder.get(), i) != 1) clip.channels = 2;
    // The largest Opus packet is 120 ms at 48 kHz; one extra frame is
    // reserved for the interpolation overlap. Never resize on the audio thread.
    clip.pcm.resize((5760 + 1) * clip.channels);
    error = clip.bufferThrough(0);
    if (error) throw std::runtime_error("decoding first Opus packet failed (" + std::to_string(error) + ")");
    return clip;
}
} // namespace dispatcher_audio
