#!/bin/sh
# Usage: run_time.sh <cpu> [maxreps] [variants]
# Timing matrix for the SQZMOM mechanism probe, one pass. Each process stops
# as soon as two reps agree within 1% for every variant (at most maxreps,
# default 6). The whole pass runs pinned to <cpu> behind one 2 s spin, with no
# idle gap between processes: a core that has not ramped reads several times
# slow, and consistently so. `one` runs in a process of its own: what it frees
# moves glibc's mmap threshold for every other variant in the process.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$PROBE_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  PROBE_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"; VARS="${3:-ref,blk,sblk,strm,five}"
"$HERE/probe" spin 2
for N in 100 500 2000 20000 100000; do
  PROBE_ONLY=$VARS "$HERE/probe" time $N 512 $REPS 20 0
  PROBE_ONLY=one "$HERE/probe" time $N 512 $REPS 20 0
done
for N in 20000 100000; do
  PROBE_ONLY=$VARS "$HERE/probe" time $N 512 $REPS 20 1
  PROBE_ONLY=one "$HERE/probe" time $N 512 $REPS 20 1
done
