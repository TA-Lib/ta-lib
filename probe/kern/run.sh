#!/bin/sh
# Usage: run.sh <cpu> [maxreps=6]
# Composed functions and a few plain ones: batch, block kernel and blocked
# batch with page faults per call, one pinned pass behind a 2 s spin.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$KERN_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  KERN_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"
KERN_WARM_MS=30; KERN_FAULTS=1
KERN_ONLY=ADXR,ALLIGATOR,APO,BBANDS,BBW,CRSI,GATOR,ICHIMOKU,KC,KDJ,KSTEXT,MACDEXT,PERCENTB,PPO,PVO,RVIR,SQZMOM,STDDEV,STOCH,STOCHF,STOCHRSI,MA,SMA,EMA,RSI,MACD,ATR
export KERN_WARM_MS KERN_FAULTS KERN_ONLY
"$HERE/kern_harness" spin 2
for N in 250 2000 20000 100000 1000000; do
  "$HERE/kern_harness" time $N 512 "$REPS"
done
