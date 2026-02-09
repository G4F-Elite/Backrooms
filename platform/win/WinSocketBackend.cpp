#include "WinSocketBackend.h"

#include <cstring>

namespace backrooms::net {

WinSocketBackend::WinSocketBackend() : socket_(INVALID_SOCKET), wsaReady_(false) {}

WinSocketBackend::~WinSocketBackend() {
    close();
}

bool WinSocketBackend::init() {
    if (wsaReady_) return true;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
    wsaReady_ = true;
    return true;
}

bool WinSocketBackend::openUdp(Port bindPort, bool nonBlocking, bool enableBroadcast) {
    if (!wsaReady_ && !init()) return false;
    close();

    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) return false;

    if (enableBroadcast) {
        BOOL on = TRUE;
        setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
    }

    if (nonBlocking) {
        u_long mode = 1;
        ioctlsocket(socket_, FIONBIO, &mode);
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(bindPort);
    if (bind(socket_, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }
    return true;
}

SocketIoResult WinSocketBackend::sendTo(const Endpoint& endpoint, const void* data, int size) {
    if (socket_ == INVALID_SOCKET || !data || size <= 0) return {SocketStatus::Error, 0};
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(endpoint.ipv4HostOrder);
    dest.sin_port = htons(endpoint.portHostOrder);
    int sent = sendto(socket_, (const char*)data, size, 0, (sockaddr*)&dest, sizeof(dest));
    if (sent >= 0) return {SocketStatus::Ok, sent};
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK) return {SocketStatus::WouldBlock, 0};
    return {SocketStatus::Error, 0};
}

SocketIoResult WinSocketBackend::recvFrom(Endpoint& endpoint, void* outData, int maxSize) {
    if (socket_ == INVALID_SOCKET || !outData || maxSize <= 0) return {SocketStatus::Error, 0};
    sockaddr_in from{};
    int fromLen = sizeof(from);
    int got = recvfrom(socket_, (char*)outData, maxSize, 0, (sockaddr*)&from, &fromLen);
    if (got >= 0) {
        endpoint.ipv4HostOrder = ntohl(from.sin_addr.s_addr);
        endpoint.portHostOrder = ntohs(from.sin_port);
        return {SocketStatus::Ok, got};
    }
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK) return {SocketStatus::WouldBlock, 0};
    return {SocketStatus::Error, 0};
}

void WinSocketBackend::close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
    if (wsaReady_) {
        WSACleanup();
        wsaReady_ = false;
    }
}

}  // namespace backrooms::net
