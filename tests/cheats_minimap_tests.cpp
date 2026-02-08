#include <cassert>
#include <cstring>
#include <iostream>

#include "../src/cheats.h"
#include "../src/minimap.h"

int sampleWalls(int wx, int wz) {
    return (wx == 3 && wz == -2) ? 1 : 0;
}

void testCheatCodeCompleteSequence() {
    int progress = 0;
    const char* seq = "MINIMAP";
    bool toggled = false;
    for (int i = 0; seq[i]; i++) {
        toggled = pushMinimapCheatChar(progress, seq[i]);
    }
    assert(toggled);
    assert(progress == 0);
}

void testCheatCodeOverlapRecovery() {
    int progress = 0;
    const char* seq = "MIMINIMAP";
    bool toggled = false;
    for (int i = 0; seq[i]; i++) {
        toggled = pushMinimapCheatChar(progress, seq[i]);
    }
    assert(toggled);
    assert(progress == 0);
}

void testCheatCodeWrongSymbolResets() {
    int progress = 0;
    pushMinimapCheatChar(progress, 'M');
    pushMinimapCheatChar(progress, 'I');
    bool toggled = pushMinimapCheatChar(progress, 'X');
    assert(!toggled);
    assert(progress == 0);
}

void testMinimapRowsBuild() {
    char rows[MINIMAP_DIAMETER][MINIMAP_DIAMETER + 1];
    buildMinimapRows(rows, 0, 0, sampleWalls);

    int center = MINIMAP_RADIUS;
    assert(rows[center][center] == 'P');

    int wallRow = center + 2;
    int wallCol = center + 3;
    assert(rows[wallRow][wallCol] == '#');
}

void testMinimapNullSampler() {
    char g = minimapGlyphForCell(1, 1, 0, 0, nullptr);
    assert(g == '?');
}

int main() {
    testCheatCodeCompleteSequence();
    testCheatCodeOverlapRecovery();
    testCheatCodeWrongSymbolResets();
    testMinimapRowsBuild();
    testMinimapNullSampler();
    std::cout << "All cheat/minimap tests passed.\n";
    return 0;
}
