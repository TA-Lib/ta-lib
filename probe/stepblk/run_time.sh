#!/bin/sh
# Usage: run_time.sh <cpu> [maxreps=6] [variants] [lengths]
# One pinned pass behind a 2 s spin; each process stops when two reps agree
# within 1% for every variant. No variant here moves glibc's mmap threshold
# for the others, so they share a process.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$PROBE_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  PROBE_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"; VARS="${3:-ref,blk,iblk,iblkv,five}"; LENS="${4:-100 500 2000 20000 100000 1000000}"
"$HERE/probe" spin 2
for N in $LENS; do
  PROBE_ONLY=$VARS "$HERE/probe" time $N 512 $REPS 20 0
done
