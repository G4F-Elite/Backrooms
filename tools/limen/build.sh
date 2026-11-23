#!/bin/sh
# Builds the LIMEN tools into build/ (the game itself, build/backrooms.exe, is build.bat):
#   limen_tool.exe   maps, self-tests, statistics (no game code)
#   limen_shots.exe  frame grabber / geometry audit
# usage: sh tools/limen/build.sh [tool|shots ...]   (default: both)
set -e
cd "$(dirname "$0")/../.."
mkdir -p build
FLAGS="-std=c++17 -O2 -fno-strict-aliasing -Wall -Wno-unused-result -Wno-unknown-pragmas -static -static-libgcc -static-libstdc++"
LIBS="-Ldeps/lib -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32"
INC="-Ideps/include -Isrc"
what="${*:-tool shots}"
for w in $what; do
  case $w in
    tool)  g++ -std=c++17 -O2 -Wall -o build/limen_tool.exe tools/limen_tool.cpp ;;
    shots) g++ $FLAGS $INC -o build/limen_shots.exe tools/limen_shots.cpp deps/src/glad.c $LIBS ;;
    *) echo "unknown target $w"; exit 1 ;;
  esac
  echo "built $w"
done
