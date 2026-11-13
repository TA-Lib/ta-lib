#!/bin/sh
# Usage: build.sh [ta-lib worktree]   (default: the checkout this script is in)
# Builds ./probe and the -DPROBE_RESEED=1 mutant ./probe_mutant next to this
# script, linking the worktree's cmake-build/libta-lib.a by path, never an
# installed library.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
WT="${1:-$(cd "$HERE/../.." && pwd)}"
FLAGS="-O3 -ffp-contract=off -fno-math-errno -Wall -Wextra"
case "$(uname -m)" in
  x86_64|amd64) FLAGS="$FLAGS -mfma -falign-functions=64 -falign-loops=64" ;;
esac
${CC:-cc} $FLAGS -I"$WT/include" "$HERE/sqzmom_probe.c" "$WT/cmake-build/libta-lib.a" -lm -o "$HERE/probe"
${CC:-cc} $FLAGS -DPROBE_RESEED=1 -I"$WT/include" "$HERE/sqzmom_probe.c" "$WT/cmake-build/libta-lib.a" -lm -o "$HERE/probe_mutant"
