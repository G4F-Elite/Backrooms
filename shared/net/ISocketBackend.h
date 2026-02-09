#pragma once

#include "../net/NetSocketTypes.h"

namespace backrooms::net {

class ISocketBackend {
public:
    virtual ~ISocketBackend() = default;

    virtual bool init() = 0;
    virtual bool openUdp(Port bindPort, bool nonBlocking, bool enableBroadcast) = 0;
    virtual SocketIoResult sendTo(const Endpoint& endpoint, const void* data, int size) = 0;
    virtual SocketIoResult recvFrom(Endpoint& endpoint, void* outData, int maxSize) = 0;
    virtual void close() = 0;
};

}  // namespace backrooms::net
