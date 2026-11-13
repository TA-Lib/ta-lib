#!/bin/sh
# Usage: run9.sh <cpu> [maxreps=6]
# The functions that page-fault today, ONE PROCESS PER FUNCTION AND LENGTH, so
# shipped batch meets the allocator state it meets in a program that calls
# only that function: batch, block kernel and blocked batch with faults.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$KERN_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  KERN_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"
KERN_WARM_MS=30; KERN_FAULTS=1; export KERN_WARM_MS KERN_FAULTS
"$HERE/kern_harness" spin 2
for F in SQZMOM CRSI GATOR ICHIMOKU KC KSTEXT MACDEXT STOCHRSI SMA; do
  for N in 2000 8000 20000 100000 1000000; do
    KERN_ONLY=$F "$HERE/kern_harness" time $N 512 "$REPS" | grep "^$F " | sed "s/^/points=$N /"
  done
done
