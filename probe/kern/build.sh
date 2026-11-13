#!/bin/sh
# Usage: build.sh [ta-lib tree]   (default: the checkout this script is in)
# Builds ./kern_harness next to this script against the tree's static library.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
TREE="${1:-$HERE/../..}"
TREE="$(cd "$TREE" && pwd)"
${CC:-cc} -O2 -Wall -Wextra -I"$TREE/include" -I"$TREE/src/ta_func" "$HERE/kern_harness.c" "$TREE/cmake-build/libta-lib.a" -lm -o "$HERE/kern_harness"
