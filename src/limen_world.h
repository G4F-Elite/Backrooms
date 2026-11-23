#pragma once
// limen_world.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_world_cache.h"  // slab cache, story edits, lamp sites, tag/flag lookup
#include "limen_world_ground.h"  // zones, walkability, ground height, vaults, lamps and pillars
