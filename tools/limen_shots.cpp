// LIMEN frame grabber and geometry audit: the game's translation unit (src/game.cpp) built
// with LIMEN_SHOTS, which renames the game's main away, plus the grabber's own main.
//   g++ -std=c++17 -O2 -I"deps/include" -I"src" -o build/limen_shots.exe tools/limen_shots.cpp deps/src/glad.c -L"deps/lib" -lglfw3 ...
//   build\limen_shots.exe <seed> <frames> <level>          frames of a level into out/shots_L<level>
//   build\limen_shots.exe audit <seed> <slabs> <level> <eyes>   rays against the meshes: holes, z-fight, paper walls
//   build\limen_shots.exe at <seed> 1 <level> <x> <z> <yaw> <pitch> <out.bmp> [eyeY]
#define LIMEN_SHOTS
#include "../src/game.cpp"
#include "limen_shots.h"
