#pragma once

// Slot 3 behavior: equip a consumable (battery/plush) so it has a first-person model.
// Pressing slot 3 again uses the equipped consumable.

inline void handleConsumableSlot3Press(){
    // Mod behavior: slot 3 is always the plush toy and is always available.
    // Press once to equip, press again to use.
    if(activeDeviceSlot != 3){
        heldConsumableType = ITEM_PLUSH_TOY;
        activeDeviceSlot = 3;
        return;
    }

    // Use (even if invPlush == 0, the "always available" mode can still apply the effect).
    applyItemUse(ITEM_PLUSH_TOY);
}
