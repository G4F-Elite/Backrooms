#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "../../shared/protocol/Protocol.h"

int main() {
    using namespace backrooms::protocol;

    std::cout << "Backrooms Master Service\n";
    std::cout << "Protocol: v" << kProtocolVersion << "\n";
    std::cout << "Port: " << kDefaultMasterPort << "\n";

    // Registry skeleton loop. Heartbeat/server-list endpoints are added incrementally.
    constexpr auto kTick = std::chrono::milliseconds(50);
    for (;;) {
        std::this_thread::sleep_for(kTick);
    }

    return 0;
}
