#pragma once

// Slot 3 behavior: equip the plush toy. It is always available and restores
// morale only while it is held in hands (see updateSanity). There is no "use".

inline void handleConsumableSlot3Press(){
    if(activeDeviceSlot != 3){
        heldConsumableType = ITEM_PLUSH_TOY;
        activeDeviceSlot = 3;
    }
}
