#!/bin/sh
# usage: sh nshot.sh <name> <level> <rigCells> <x> <z> <yaw> <pitch> [seed] [narrator=1]   (metres, radians; frames land in out/narr/)
cd "$(dirname "$0")/../../build"   # binaries and frames live in build/
mkdir -p out/narr
LM_NARRATOR=${9:-1} LM_STORY=$3 ./limen_shots.exe at ${8:-1337} 1 $2 $4 $5 $6 $7 out/narr/$1.bmp > out/narr/$1.log 2>&1
python ../tools/limen/topng.py out/narr/$1.bmp out/narr/$1.png && rm -f out/narr/$1.bmp
