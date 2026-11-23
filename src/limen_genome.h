#pragma once
// limen_genome.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_genome_dna.h"  // genes, mega forms, surface poles, quirks, level DNA type
#include "limen_genome_level.h"  // deriving the DNA of a level
#include "limen_genome_field.h"  // gene fields, wings, mega form of a slab, palettes
