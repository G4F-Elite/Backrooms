#!/bin/sh
# usage: sh naudit.sh <tag> <rigCells> <levels...>   geometry audit of every megastructure with the rig's lane through it
cd "$(dirname "$0")/../../build"   # binaries and frames live in build/
tag=$1; cells=$2; shift; shift
mkdir -p out/narr
for L in "$@"; do
  for F in 0 1 2 3 4 5 6 7 8 9 10 11 12 13; do
    LM_NARRATOR=1 LM_NAR_ARC=0 LM_NAR_WISH=$F LM_STORY=$cells ./limen_shots.exe audit 1337 3 $L 50 > out/narr/audit_${tag}_L${L}_F$F.txt 2>&1
    first=$(grep -m1 "^   " out/narr/audit_${tag}_L${L}_F$F.txt | sed 's/ *slab.*//; s/^ *//')
    base=$(grep -m1 '^slab' out/narr/audit_${tag}_L${L}_F$F.txt | sed 's/: rays.*bad/ bad/')
    echo "L$L [$first] $base | $(grep '^AUDIT' out/narr/audit_${tag}_L${L}_F$F.txt | sed 's/.*slabs/slabs/')"
  done
done
