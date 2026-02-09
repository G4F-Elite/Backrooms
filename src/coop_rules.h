#pragma once

inline bool shouldBlockCoopDoor(bool coopInitialized, bool coopDoorOpen, int multiState, int multiInGameState) {
    if (multiState != multiInGameState) return false;
    if (!coopInitialized) return false;
    if (coopDoorOpen) return false;
    return true;
}

inline bool shouldBlockStoryDoor(
    bool coopInitialized,
    bool coopDoorOpen,
    int multiState,
    int multiInGameState,
    int notesCollected,
    int notesRequired
) {
    if (!coopInitialized) return false;
    if (multiState == multiInGameState) return !coopDoorOpen;
    if (notesRequired < 1) return false;
    return notesCollected < notesRequired;
}
