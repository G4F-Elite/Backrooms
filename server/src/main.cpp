#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "../../shared/protocol/Protocol.h"

int main() {
    using namespace backrooms::protocol;

    std::cout << "Backrooms Dedicated Server (headless)\n";
    std::cout << "Protocol: v" << kProtocolVersion << "\n";
    std::cout << "Port: " << kDefaultGamePort << "\n";
    std::cout << "Max players: " << kMaxPlayersDedicated << "\n";

    // Server skeleton tick loop (30 Hz). Authoritative simulation is added incrementally.
    constexpr auto kTick = std::chrono::milliseconds(33);
    for (;;) {
        std::this_thread::sleep_for(kTick);
    }

    return 0;
}
