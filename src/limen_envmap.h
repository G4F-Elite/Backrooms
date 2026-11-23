#pragma once
// limen_envmap.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_envmap_shaders.h"  // textures, GLSL of the world shader, far-light/AO/fog map
#include "limen_envmap_textures.h"  // per-level decor atlas, unstructured, mega and xeno textures
