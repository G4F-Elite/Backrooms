#include <cassert>
#include <iostream>

#include "../src/reconnect_policy.h"

void testReconnectContinue() {
    assert(shouldContinueReconnect(1, 12));
    assert(shouldContinueReconnect(12, 12));
    assert(!shouldContinueReconnect(13, 12));
}

void testReconnectDelay() {
    assert(nextReconnectDelaySeconds(1) == 1.5f);
    assert(nextReconnectDelaySeconds(4) == 2.0f);
    assert(nextReconnectDelaySeconds(10) == 3.0f);
}

int main() {
    testReconnectContinue();
    testReconnectDelay();
    std::cout << "All reconnect policy tests passed.\n";
    return 0;
}
