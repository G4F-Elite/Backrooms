#pragma once

inline bool shouldBlockCoopDoor(bool coopInitialized, bool coopDoorOpen, int multiState, int multiInGameState) {
    if (multiState != multiInGameState) return false;
    if (!coopInitialized) return false;
    if (coopDoorOpen) return false;
    return true;
}
