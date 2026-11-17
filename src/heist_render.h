#pragma once
// Heist render data: one small mesh per loot kind plus the junk pile, drawn for every
// active world item (see renderScene). Built with the decor atlas, like the sets.

#include "heist_models.h"
#include "zfight_guard.h"

inline constexpr int HEIST_ITEM_MESHES = LOOT_TYPE_COUNT + 1; // loot kinds, then the junk pile

inline GLuint heistItemVAO[HEIST_ITEM_MESHES] = {};
inline GLuint heistItemVBO[HEIST_ITEM_MESHES] = {};
inline int heistItemVC[HEIST_ITEM_MESHES] = {};

inline void buildHeistItemMeshes() {
    if (heistItemVC[0] > 0) return; // the meshes never change
    for (int i = 0; i < HEIST_ITEM_MESHES; i++) {
        std::vector<float> m;
        DecorBuilder b(m, 0.0f, 0.0f, 0.0f, 1.0f);
        if (i < LOOT_TYPE_COUNT) buildLootMesh(b, i);
        else buildStashMesh(b);
        resolveModelMesh(m, "loot");
        heistItemVC[i] = (int)m.size() / 8;
        if (heistItemVC[i] > 0) setupVAO(heistItemVAO[i], heistItemVBO[i], m, true);
    }
}

// Mesh slot for a world item type, -1 for supplies (those keep their own models).
inline int heistItemMeshIndex(int itemType) {
    if (isLootItemType(itemType)) return lootFromItemType(itemType);
    if (itemType == ITEM_STASH) return LOOT_TYPE_COUNT;
    return -1;
}

// Loot spins slowly so it catches the eye; junk piles just sit there. The core pulses.
inline bool heistItemSpins(int itemType) {
    return itemType != ITEM_STASH;
}
