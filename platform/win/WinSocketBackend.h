#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../shared/net/ISocketBackend.h"

namespace backrooms::net {

class WinSocketBackend final : public ISocketBackend {
public:
    WinSocketBackend();
    ~WinSocketBackend() override;

    bool init() override;
    bool openUdp(Port bindPort, bool nonBlocking, bool enableBroadcast) override;
    SocketIoResult sendTo(const Endpoint& endpoint, const void* data, int size) override;
    SocketIoResult recvFrom(Endpoint& endpoint, void* outData, int maxSize) override;
    void close() override;

private:
    SOCKET socket_;
    bool wsaReady_;
};

}  // namespace backrooms::net
