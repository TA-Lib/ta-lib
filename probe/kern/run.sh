#!/bin/sh
# Usage: run.sh <cpu> [maxreps=6]
# Corpus timing, batch against block kernel, one pinned pass behind a 2 s spin.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$KERN_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  KERN_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"
KERN_WARM_MS=30; export KERN_WARM_MS
"$HERE/kern_harness" spin 2
"$HERE/kern_harness" time 2000 512 "$REPS"
"$HERE/kern_harness" time 100000 512 "$REPS"
"$HERE/kern_harness" time 2000 64 "$REPS"
