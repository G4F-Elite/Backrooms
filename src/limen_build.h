#pragma once
// limen_build.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_build_repair.h"  // openings, flood, links, loops, repair, basin stairs, cell facts
#include "limen_build_grow.h"  // growing a slab, liminal excisions, measurement
#include "limen_build_slab.h"  // lmBuildSlab: the whole slab pipeline
