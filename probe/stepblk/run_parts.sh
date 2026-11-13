#!/bin/sh
# Usage: run_parts.sh <cpu> [maxreps=6]
# One pinned pass behind a 2 s spin, on a walk that stays near 100 at any
# length (PROBE_REVERT): per-callee step against batch, the variants at each
# length, then a block-length sweep. Short series are also run over 100
# rotated series, because a replayed series trains the branch predictor.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$PROBE_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  PROBE_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"
PROBE_REVERT=0.0005; export PROBE_REVERT
"$HERE/probe" spin 2
echo "== parts"
for N in 2000 20000 1000000; do "$HERE/probe" parts $N 512 $REPS; done
PROBE_ROTATE=100 "$HERE/probe" parts 2000 512 $REPS
echo "== variants"
for R in 1 100; do PROBE_ROTATE=$R PROBE_ONLY=ref,blk,hblk,iblkv "$HERE/probe" time 2000 512 $REPS; done
for N in 20000 100000 1000000; do PROBE_ONLY=ref,blk,hblk,iblkv "$HERE/probe" time $N 512 $REPS; done
for N in 100000 1000000; do PROBE_ONLY=one "$HERE/probe" time $N 512 $REPS; done
echo "== block length, 2000 bars rotated"
for B in 32 64 128 256 512 1024; do PROBE_ROTATE=100 PROBE_ONLY=ref,blk,hblk "$HERE/probe" time 2000 $B $REPS; done
echo "== block length, 100000 bars"
for B in 32 64 128 256 512 1024; do PROBE_ONLY=ref,blk,hblk "$HERE/probe" time 100000 $B $REPS; done
