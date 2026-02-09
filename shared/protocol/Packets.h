#pragma once

#include <cstdint>
#include <cstring>

#include "Protocol.h"

namespace backrooms::protocol {

constexpr int kHeaderSize = 7;  // protocol(u16) + type(u8) + sequence(u32)
constexpr int kPlayerNameLen = 32;
constexpr int kServerNameLen = 32;

struct HandshakeHello {
    std::uint32_t nonce;
    char playerName[kPlayerNameLen];
};

struct HandshakeWelcome {
    std::uint8_t accepted;
    std::uint8_t playerId;
    std::uint16_t tickRateHz;
};

struct Heartbeat {
    std::uint16_t gamePort;
    std::uint8_t currentPlayers;
    std::uint8_t maxPlayers;
    std::uint8_t flags;
    char serverName[kServerNameLen];
};

struct ServerListQuery {
    std::uint8_t reserved;
};

struct ServerListEntry {
    std::uint32_t ipv4HostOrder;
    std::uint16_t gamePort;
    std::uint8_t currentPlayers;
    std::uint8_t maxPlayers;
    std::uint8_t flags;
    char serverName[kServerNameLen];
};

struct ServerListResponse {
    std::uint8_t count;
    ServerListEntry entries[kMaxServerListEntries];
};

inline void writeU16(std::uint8_t* out, int& off, std::uint16_t v) {
    out[off++] = (std::uint8_t)((v >> 8) & 0xFF);
    out[off++] = (std::uint8_t)(v & 0xFF);
}

inline void writeU32(std::uint8_t* out, int& off, std::uint32_t v) {
    out[off++] = (std::uint8_t)((v >> 24) & 0xFF);
    out[off++] = (std::uint8_t)((v >> 16) & 0xFF);
    out[off++] = (std::uint8_t)((v >> 8) & 0xFF);
    out[off++] = (std::uint8_t)(v & 0xFF);
}

inline bool readU16(const std::uint8_t* in, int len, int& off, std::uint16_t& v) {
    if (off + 2 > len) return false;
    v = (std::uint16_t)((in[off] << 8) | in[off + 1]);
    off += 2;
    return true;
}

inline bool readU32(const std::uint8_t* in, int len, int& off, std::uint32_t& v) {
    if (off + 4 > len) return false;
    v = ((std::uint32_t)in[off] << 24) |
        ((std::uint32_t)in[off + 1] << 16) |
        ((std::uint32_t)in[off + 2] << 8) |
        (std::uint32_t)in[off + 3];
    off += 4;
    return true;
}

inline bool encodeHeader(std::uint8_t* out, int outLen, MessageType type, std::uint32_t sequence) {
    if (!out || outLen < kHeaderSize) return false;
    int off = 0;
    writeU16(out, off, kProtocolVersion);
    out[off++] = (std::uint8_t)type;
    writeU32(out, off, sequence);
    return true;
}

inline bool decodeHeader(const std::uint8_t* in, int inLen, MessageHeader& header) {
    if (!in || inLen < kHeaderSize) return false;
    int off = 0;
    if (!readU16(in, inLen, off, header.protocolVersion)) return false;
    header.type = in[off++];
    if (!readU32(in, inLen, off, header.sequence)) return false;
    return true;
}

inline bool encodeHandshakeHello(std::uint8_t* out, int outLen, std::uint32_t seq, const HandshakeHello& msg, int& written) {
    written = 0;
    if (!encodeHeader(out, outLen, MessageType::HandshakeHello, seq)) return false;
    int off = kHeaderSize;
    if (off + 4 + kPlayerNameLen > outLen) return false;
    writeU32(out, off, msg.nonce);
    std::memcpy(out + off, msg.playerName, kPlayerNameLen);
    off += kPlayerNameLen;
    written = off;
    return true;
}

inline bool decodeHandshakeHello(const std::uint8_t* in, int inLen, HandshakeHello& msg) {
    if (!in || inLen < kHeaderSize + 4 + kPlayerNameLen) return false;
    MessageHeader h{};
    if (!decodeHeader(in, inLen, h)) return false;
    if (h.protocolVersion != kProtocolVersion || h.type != (std::uint8_t)MessageType::HandshakeHello) return false;
    int off = kHeaderSize;
    if (!readU32(in, inLen, off, msg.nonce)) return false;
    std::memcpy(msg.playerName, in + off, kPlayerNameLen);
    msg.playerName[kPlayerNameLen - 1] = '\0';
    return true;
}

inline bool encodeHandshakeWelcome(std::uint8_t* out, int outLen, std::uint32_t seq, const HandshakeWelcome& msg, int& written) {
    written = 0;
    if (!encodeHeader(out, outLen, MessageType::HandshakeWelcome, seq)) return false;
    int off = kHeaderSize;
    if (off + 4 > outLen) return false;
    out[off++] = msg.accepted;
    out[off++] = msg.playerId;
    writeU16(out, off, msg.tickRateHz);
    written = off;
    return true;
}

inline bool decodeHandshakeWelcome(const std::uint8_t* in, int inLen, HandshakeWelcome& msg) {
    if (!in || inLen < kHeaderSize + 4) return false;
    MessageHeader h{};
    if (!decodeHeader(in, inLen, h)) return false;
    if (h.protocolVersion != kProtocolVersion || h.type != (std::uint8_t)MessageType::HandshakeWelcome) return false;
    int off = kHeaderSize;
    msg.accepted = in[off++];
    msg.playerId = in[off++];
    return readU16(in, inLen, off, msg.tickRateHz);
}

inline bool encodeHeartbeat(std::uint8_t* out, int outLen, std::uint32_t seq, const Heartbeat& msg, int& written) {
    written = 0;
    if (!encodeHeader(out, outLen, MessageType::Heartbeat, seq)) return false;
    int off = kHeaderSize;
    if (off + 2 + 3 + kServerNameLen > outLen) return false;
    writeU16(out, off, msg.gamePort);
    out[off++] = msg.currentPlayers;
    out[off++] = msg.maxPlayers;
    out[off++] = msg.flags;
    std::memcpy(out + off, msg.serverName, kServerNameLen);
    off += kServerNameLen;
    written = off;
    return true;
}

inline bool decodeHeartbeat(const std::uint8_t* in, int inLen, Heartbeat& msg) {
    if (!in || inLen < kHeaderSize + 2 + 3 + kServerNameLen) return false;
    MessageHeader h{};
    if (!decodeHeader(in, inLen, h)) return false;
    if (h.protocolVersion != kProtocolVersion || h.type != (std::uint8_t)MessageType::Heartbeat) return false;
    int off = kHeaderSize;
    if (!readU16(in, inLen, off, msg.gamePort)) return false;
    msg.currentPlayers = in[off++];
    msg.maxPlayers = in[off++];
    msg.flags = in[off++];
    std::memcpy(msg.serverName, in + off, kServerNameLen);
    msg.serverName[kServerNameLen - 1] = '\0';
    return true;
}

inline bool encodeServerListQuery(std::uint8_t* out, int outLen, std::uint32_t seq, int& written) {
    written = 0;
    if (!encodeHeader(out, outLen, MessageType::ServerListQuery, seq)) return false;
    int off = kHeaderSize;
    if (off + 1 > outLen) return false;
    out[off++] = 0;
    written = off;
    return true;
}

inline bool decodeServerListQuery(const std::uint8_t* in, int inLen) {
    if (!in || inLen < kHeaderSize + 1) return false;
    MessageHeader h{};
    if (!decodeHeader(in, inLen, h)) return false;
    return h.protocolVersion == kProtocolVersion && h.type == (std::uint8_t)MessageType::ServerListQuery;
}

inline bool encodeServerListResponse(std::uint8_t* out, int outLen, std::uint32_t seq, const ServerListResponse& msg, int& written) {
    written = 0;
    if (!encodeHeader(out, outLen, MessageType::ServerListResponse, seq)) return false;
    int off = kHeaderSize;
    std::uint8_t count = msg.count > kMaxServerListEntries ? kMaxServerListEntries : msg.count;
    out[off++] = count;
    for (std::uint8_t i = 0; i < count; i++) {
        if (off + 4 + 2 + 3 + kServerNameLen > outLen) return false;
        writeU32(out, off, msg.entries[i].ipv4HostOrder);
        writeU16(out, off, msg.entries[i].gamePort);
        out[off++] = msg.entries[i].currentPlayers;
        out[off++] = msg.entries[i].maxPlayers;
        out[off++] = msg.entries[i].flags;
        std::memcpy(out + off, msg.entries[i].serverName, kServerNameLen);
        off += kServerNameLen;
    }
    written = off;
    return true;
}

inline bool decodeServerListResponse(const std::uint8_t* in, int inLen, ServerListResponse& msg) {
    if (!in || inLen < kHeaderSize + 1) return false;
    MessageHeader h{};
    if (!decodeHeader(in, inLen, h)) return false;
    if (h.protocolVersion != kProtocolVersion || h.type != (std::uint8_t)MessageType::ServerListResponse) return false;
    int off = kHeaderSize;
    msg.count = in[off++];
    if (msg.count > kMaxServerListEntries) return false;
    for (std::uint8_t i = 0; i < msg.count; i++) {
        if (off + 4 + 2 + 3 + kServerNameLen > inLen) return false;
        if (!readU32(in, inLen, off, msg.entries[i].ipv4HostOrder)) return false;
        if (!readU16(in, inLen, off, msg.entries[i].gamePort)) return false;
        msg.entries[i].currentPlayers = in[off++];
        msg.entries[i].maxPlayers = in[off++];
        msg.entries[i].flags = in[off++];
        std::memcpy(msg.entries[i].serverName, in + off, kServerNameLen);
        msg.entries[i].serverName[kServerNameLen - 1] = '\0';
        off += kServerNameLen;
    }
    return true;
}

}  // namespace backrooms::protocol
