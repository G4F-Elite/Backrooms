#pragma once
// limen_narrator.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_narrator_memory.h"  // features, memory, eyes: what the player has seen
#include "limen_narrator_trial.h"  // rewriting unseen slabs, trials, composing a plan
#include "limen_narrator_road.h"  // decisions, the drill road, roaming, the tick
