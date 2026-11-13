#!/bin/sh
# Usage: run_kern.sh <cpu> [maxreps=6]
# One pinned pass behind a 2 s spin (no taskset, as on macOS: unpinned).
# parts (hand, step, kern, kernR per callee), then time for ref, blk, hblk
# and kblk, on a walk that stays near 100.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$PROBE_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  PROBE_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"
PROBE_REVERT=0.0005
export PROBE_REVERT
"$HERE/probe" spin 2
PROBE_ROTATE=100 "$HERE/probe" parts 2000 512 "$REPS"
"$HERE/probe" parts 100000 512 "$REPS"
"$HERE/probe" parts 100000 32 "$REPS"
for B in 32 64 512; do
  PROBE_ROTATE=100 PROBE_ONLY=ref,blk,hblk,kblk "$HERE/probe" time 2000 "$B" "$REPS" 20 0
done
for B in 32 64 512; do
  PROBE_ONLY=ref,blk,hblk,kblk "$HERE/probe" time 100000 "$B" "$REPS" 20 0
done
