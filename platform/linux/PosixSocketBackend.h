#pragma once

#include "../../shared/net/ISocketBackend.h"

namespace backrooms::net {

class PosixSocketBackend final : public ISocketBackend {
public:
    PosixSocketBackend();
    ~PosixSocketBackend() override;

    bool init() override;
    bool openUdp(Port bindPort, bool nonBlocking, bool enableBroadcast) override;
    SocketIoResult sendTo(const Endpoint& endpoint, const void* data, int size) override;
    SocketIoResult recvFrom(Endpoint& endpoint, void* outData, int maxSize) override;
    void close() override;

private:
    int socketFd_;
};

}  // namespace backrooms::net
