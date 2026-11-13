#!/bin/sh
# Usage: build.sh <old libta-lib.a> <new libta-lib.a> <include dir>
# Builds ab_old and ab_new next to this script, each linking one archive by path.
set -e
if [ $# -ne 3 ]; then
   echo "usage: $0 <old libta-lib.a> <new libta-lib.a> <include dir>" >&2
   exit 2
fi
HERE="$(cd "$(dirname "$0")" && pwd)"
FLAGS="-O2 -Wall -Wextra"
${CC:-cc} $FLAGS -I"$3" "$HERE/ab_bench.c" "$1" -lm -o "$HERE/ab_old"
${CC:-cc} $FLAGS -I"$3" "$HERE/ab_bench.c" "$2" -lm -o "$HERE/ab_new"
