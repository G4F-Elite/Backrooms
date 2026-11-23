#pragma once
// limen_grammar.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_grammar_ops.h"  // grammar state and the eight base operators
#include "limen_grammar_rooms.h"  // shells, cells and the room-scale figures
#include "limen_grammar_megaforms.h"  // growing a slab and the built megastructures
#include "limen_grammar_wild.h"  // unstructured megastructures, beat stamps, xenografts
