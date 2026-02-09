#pragma once

#include <cstdint>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace backrooms::net {

struct UdpAddress {
    std::uint32_t ipv4HostOrder;
    std::uint16_t portHostOrder;
};

class UdpSocketLite {
public:
    UdpSocketLite() : opened_(false) {
#ifdef _WIN32
        socket_ = INVALID_SOCKET;
#else
        socket_ = -1;
#endif
    }

    ~UdpSocketLite() {
        close();
    }

    bool open(std::uint16_t bindPort, bool nonBlocking) {
        close();
#ifdef _WIN32
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
        socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_ == INVALID_SOCKET) return false;
        if (nonBlocking) {
            u_long mode = 1;
            ioctlsocket(socket_, FIONBIO, &mode);
        }
#else
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_ < 0) return false;
        if (nonBlocking) {
            int flags = fcntl(socket_, F_GETFL, 0);
            if (flags >= 0) fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
        }
#endif
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(bindPort);
        if (::bind(socket_, (sockaddr*)&local, sizeof(local)) != 0) {
            close();
            return false;
        }
        opened_ = true;
        return true;
    }

    int sendTo(const UdpAddress& to, const void* data, int len) {
        if (!opened_ || !data || len <= 0) return -1;
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = htonl(to.ipv4HostOrder);
        dest.sin_port = htons(to.portHostOrder);
        return (int)::sendto(socket_, (const char*)data, len, 0, (sockaddr*)&dest, sizeof(dest));
    }

    int recvFrom(UdpAddress& from, void* out, int maxLen) {
        if (!opened_ || !out || maxLen <= 0) return -1;
        sockaddr_in src{};
#ifdef _WIN32
        int srcLen = sizeof(src);
#else
        socklen_t srcLen = sizeof(src);
#endif
        int r = (int)::recvfrom(socket_, (char*)out, maxLen, 0, (sockaddr*)&src, &srcLen);
        if (r >= 0) {
            from.ipv4HostOrder = ntohl(src.sin_addr.s_addr);
            from.portHostOrder = ntohs(src.sin_port);
        }
        return r;
    }

    bool wouldBlock() const {
#ifdef _WIN32
        return WSAGetLastError() == WSAEWOULDBLOCK;
#else
        return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
    }

    void close() {
        if (!opened_) return;
#ifdef _WIN32
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
        WSACleanup();
#else
        if (socket_ >= 0) {
            ::close(socket_);
            socket_ = -1;
        }
#endif
        opened_ = false;
    }

private:
#ifdef _WIN32
    SOCKET socket_;
#else
    int socket_;
#endif
    bool opened_;
};

}  // namespace backrooms::net
