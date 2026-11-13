#!/bin/sh
# Usage: run.sh <cpu> [maxreps=6]
# Corpus timing: batch, block kernel and blocked batch per function, one
# pinned pass behind a 2 s spin.
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -z "$KERN_PINNED" ] && command -v taskset >/dev/null 2>&1; then
  KERN_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
REPS="${2:-6}"
KERN_WARM_MS=30; export KERN_WARM_MS
"$HERE/kern_harness" spin 2
"$HERE/kern_harness" time 2000 512 "$REPS"
"$HERE/kern_harness" time 100000 512 "$REPS"
KERN_ONLY=ADXR,ALLIGATOR,APO,BBANDS,BBW,CRSI,GATOR,ICHIMOKU,KC,KDJ,KSTEXT,MACDEXT,PERCENTB,PPO,PVO,RVIR,SQZMOM,STDDEV,STOCH,STOCHF,STOCHRSI,SMA,EMA,RSI,MACD,ATR \
  "$HERE/kern_harness" time 1000000 512 "$REPS"
