#pragma once

inline void unlockMetaRewardsFromTier() {
    if (archiveTier >= 1) { recipeNoiseLureUnlocked = true; recipeBeaconUnlocked = true; }
    if (archiveTier >= 2) recipeFlashLampUnlocked = true;
    if (archiveTier >= 3) recipeFixatorUnlocked = true;
}

inline void tryCraftNoiseLure() {
    if (!recipeNoiseLureUnlocked) { setEchoStatus("RECIPE LOCKED: NOISE LURE"); return; }
    if (invPlush < 1) { setEchoStatus("CRAFT FAIL: NEED PLUSH"); return; }
    invPlush--;
    craftedNoiseLure++;
    setEchoStatus("CRAFTED: NOISE LURE");
}

inline void tryCraftBeacon() {
    if (!recipeBeaconUnlocked) { setEchoStatus("RECIPE LOCKED: BEACON"); return; }
    if (invBattery < 1) { setEchoStatus("CRAFT FAIL: NEED BATTERY"); return; }
    invBattery--;
    craftedBeacon++;
    setEchoStatus("CRAFTED: MARKER BEACON");
}

inline void tryCraftFlashLamp() {
    if (!recipeFlashLampUnlocked) { setEchoStatus("RECIPE LOCKED: FLASH LAMP"); return; }
    if (invBattery < 1 || invPlush < 1) { setEchoStatus("CRAFT FAIL: NEED BATTERY+PLUSH"); return; }
    invBattery--; invPlush--;
    craftedFlashLamp++;
    setEchoStatus("CRAFTED: FLASH LAMP");
}

inline void tryCraftFixator() {
    if (!recipeFixatorUnlocked) { setEchoStatus("RECIPE LOCKED: FIXATOR"); return; }
    if (invBattery < 2) { setEchoStatus("CRAFT FAIL: NEED 2 BATTERIES"); return; }
    invBattery -= 2;
    craftedButtonFixator++;
    setEchoStatus("CRAFTED: BUTTON FIXATOR");
}
