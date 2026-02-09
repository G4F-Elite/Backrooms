#include <cassert>
#include <cstring>
#include <iostream>

#include "../src/dedicated_client.h"

void testParseIpv4HostOrder() {
    std::uint32_t v = parseIpv4HostOrder("192.168.1.25");
    assert(((v >> 24) & 0xFF) == 192);
    assert(((v >> 16) & 0xFF) == 168);
    assert(((v >> 8) & 0xFF) == 1);
    assert((v & 0xFF) == 25);
}

void testParseFallbackOnInvalidInput() {
    std::uint32_t v = parseIpv4HostOrder("bad.ip.value");
    assert(v == ((127u << 24) | 1u));
}

void testFormatIpv4HostOrder() {
    char out[64] = {};
    formatIpv4HostOrder((10u << 24) | (0u << 16) | (4u << 8) | 7u, out, (int)sizeof(out));
    assert(std::strcmp(out, "10.0.4.7") == 0);
}

int main() {
    testParseIpv4HostOrder();
    testParseFallbackOnInvalidInput();
    testFormatIpv4HostOrder();
    std::cout << "All dedicated client tests passed.\n";
    return 0;
}
