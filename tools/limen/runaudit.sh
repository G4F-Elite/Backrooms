#!/bin/sh
# usage: sh runaudit.sh <tag> <slabs> <levels...>
cd "$(dirname "$0")/../../build"   # binaries and frames live in build/
tag=$1; n=$2; shift; shift
for L in "$@"; do ./limen_shots.exe audit 1337 $n $L 50 > out/audit_${tag}_L$L.txt 2>&1; grep "^AUDIT" out/audit_${tag}_L$L.txt; done
