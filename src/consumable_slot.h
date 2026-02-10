#pragma once

// Slot 3 behavior: equip a consumable (battery/plush) so it has a first-person model.
// Pressing slot 3 again uses the equipped consumable.

inline void handleConsumableSlot3Press(){
    if(activeDeviceSlot != 3){
        if(invBattery > 0){
            heldConsumableType = ITEM_BATTERY;
            activeDeviceSlot = 3;
        }else if(invPlush > 0){
            heldConsumableType = ITEM_PLUSH_TOY;
            activeDeviceSlot = 3;
        }
        return;
    }

    if(heldConsumableType == ITEM_BATTERY && invBattery > 0){
        applyItemUse(ITEM_BATTERY);
    }else if(heldConsumableType == ITEM_PLUSH_TOY && invPlush > 0){
        applyItemUse(ITEM_PLUSH_TOY);
    }

    // Auto-hide if nothing left.
    if((heldConsumableType == ITEM_BATTERY && invBattery <= 0) ||
       (heldConsumableType == ITEM_PLUSH_TOY && invPlush <= 0)){
        activeDeviceSlot = 0;
    }
}
