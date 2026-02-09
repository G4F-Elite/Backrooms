#include "PosixSocketBackend.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace backrooms::net {

PosixSocketBackend::PosixSocketBackend() : socketFd_(-1) {}

PosixSocketBackend::~PosixSocketBackend() {
    close();
}

bool PosixSocketBackend::init() {
    return true;
}

bool PosixSocketBackend::openUdp(Port bindPort, bool nonBlocking, bool enableBroadcast) {
    close();
    socketFd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) return false;

    if (enableBroadcast) {
        int on = 1;
        setsockopt(socketFd_, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    }

    if (nonBlocking) {
        int flags = fcntl(socketFd_, F_GETFL, 0);
        if (flags >= 0) fcntl(socketFd_, F_SETFL, flags | O_NONBLOCK);
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(bindPort);
    if (bind(socketFd_, (sockaddr*)&local, sizeof(local)) != 0) {
        close();
        return false;
    }
    return true;
}

SocketIoResult PosixSocketBackend::sendTo(const Endpoint& endpoint, const void* data, int size) {
    if (socketFd_ < 0 || !data || size <= 0) return {SocketStatus::Error, 0};
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(endpoint.ipv4HostOrder);
    dest.sin_port = htons(endpoint.portHostOrder);
    int sent = (int)sendto(socketFd_, data, (size_t)size, 0, (sockaddr*)&dest, sizeof(dest));
    if (sent >= 0) return {SocketStatus::Ok, sent};
    if (errno == EWOULDBLOCK || errno == EAGAIN) return {SocketStatus::WouldBlock, 0};
    return {SocketStatus::Error, 0};
}

SocketIoResult PosixSocketBackend::recvFrom(Endpoint& endpoint, void* outData, int maxSize) {
    if (socketFd_ < 0 || !outData || maxSize <= 0) return {SocketStatus::Error, 0};
    sockaddr_in from{};
    socklen_t fromLen = sizeof(from);
    int got = (int)recvfrom(socketFd_, outData, (size_t)maxSize, 0, (sockaddr*)&from, &fromLen);
    if (got >= 0) {
        endpoint.ipv4HostOrder = ntohl(from.sin_addr.s_addr);
        endpoint.portHostOrder = ntohs(from.sin_port);
        return {SocketStatus::Ok, got};
    }
    if (errno == EWOULDBLOCK || errno == EAGAIN) return {SocketStatus::WouldBlock, 0};
    return {SocketStatus::Error, 0};
}

void PosixSocketBackend::close() {
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
}

}  // namespace backrooms::net
