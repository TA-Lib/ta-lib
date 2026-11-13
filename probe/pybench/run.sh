#!/bin/sh
# #548 probe. run.sh <python> <wrapper-dir> <arms-root> <cpu|-> <bars|suite> [cap]
# <arms-root>/<arm>/lib holds that arm's libta-lib.so; arms run back to back per function.
# Run each invocation inside its own `quiet.py measure` window. Never lands.
set -u
PY=$1; WRAP=$2; ARMS=$3; CPU=$4; WHAT=$5; CAP=${6:-6}
HERE=$(cd "$(dirname "$0")" && pwd)
ARMLIST=${PYB_ARMS:-"dev onealloc block"}
FUNCS=${PYB_FUNCS:-"MACDEXT STOCHRSI MACDEXT_EMA BBANDS_EMA KC SQZMOM SMA"}
PIN=""
if [ "$CPU" != "-" ]; then PIN="taskset -c $CPU"; fi

one() { # arm, then pybench arguments
   arm=$1; shift
   LD_LIBRARY_PATH="$ARMS/$arm/lib" PYTHONPATH="$WRAP" $PIN "$PY" "$HERE/pybench.py" "$@" \
      --arm "$arm" --expect "$ARMS/$arm/lib" --cap "$CAP" || echo "FAILED arm=$arm $*"
}

if [ "$WHAT" = "suite" ]; then
   for pass in 1 2; do
      for arm in $ARMLIST; do
         echo "pass $pass"
         one "$arm" suite
      done
   done
else
   for f in $FUNCS; do
      for arm in $ARMLIST; do
         one "$arm" one --func "$f" --bars "$WHAT"
      done
   done
fi
