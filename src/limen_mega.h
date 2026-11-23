#pragma once
// limen_mega.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_mega_prims.h"  // perf/audit counters, z-fight repair, mesh primitives
#include "limen_mega_forms.h"  // meshes of hypostyle, silo, hanging, cellblock, platform
#include "limen_mega_unstruct.h"  // terrain, spires, grotto, monolith, rootfall superstructures
#include "limen_mega_walls.h"  // slab mesh cache, thin partitions, narrow corridors, windows, moldings
#include "limen_mega_arch.h"  // architectural detail of a slab
#include "limen_mega_slab.h"  // lmBuildSlabMesh
#include "limen_mega_chunk.h"  // slab mesh upkeep, drunk doorways, chunk prop meshes, post-build hook
#include "limen_mega_frame.h"  // post-build geometry, visibility, frame slice, quirk uniforms
#include "limen_mega_draw.h"  // lights gathering and drawing the world
