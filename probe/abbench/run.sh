#!/bin/sh
# Usage: run.sh <cpu> [maxreps=6] [funcs] [lens]
# Times ab_old then ab_new back to back per (function, length); one line per pair.
# Exits 1 if any pair's bits differ or any run failed.
if [ $# -lt 1 ]; then
   echo "usage: $0 <cpu> [maxreps=6] [funcs] [lens]" >&2
   exit 2
fi
if [ -z "$AB_PINNED" ] && command -v taskset >/dev/null 2>&1; then
   AB_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
HERE="$(cd "$(dirname "$0")" && pwd)"
MAXREPS="${2:-6}"
FUNCS="${3:-SQZMOM CRSI GATOR ICHIMOKU KC KSTEXT MACDEXT SMA}"
LENS="${4:-100 2000 100000 1000000}"

"$HERE/ab_old" spin 2
differ=0
failed=0
for f in $FUNCS; do
   for n in $LENS; do
      o="$("$HERE/ab_old" time "$f" "$n" "$MAXREPS")" || { echo "$f $n FAILED old"; failed=$((failed + 1)); continue; }
      w="$("$HERE/ab_new" time "$f" "$n" "$MAXREPS")" || { echo "$f $n FAILED new"; failed=$((failed + 1)); continue; }
      line="$(printf '%s\n%s\n' "$o" "$w" | awk '
         { for (i = 3; i <= NF; i++) { split($i, kv, "="); v[NR, kv[1]] = kv[2] } f = $1; n = $2 }
         END {
            a = v[1, "min"] + 0; b = v[2, "min"] + 0
            pct = (a > 0) ? (a - b) / a * 100 : 0
            word = (pct >= 0) ? "faster" : "slower"
            if (pct < 0) pct = -pct
            same = (v[1, "hash"] == v[2, "hash"] && v[1, "rc"] == v[2, "rc"] && v[1, "nb"] == v[2, "nb"])
            bits = same ? "bits=same" : sprintf("bits=DIFFER old=%s/rc%s/nb%s new=%s/rc%s/nb%s", \
               v[1, "hash"], v[1, "rc"], v[1, "nb"], v[2, "hash"], v[2, "rc"], v[2, "nb"])
            printf "%s %s old=%s new=%s %.1f%% %s reps=%s,%s faults=%s->%s %s\n", \
               f, n, v[1, "min"], v[2, "min"], pct, word, v[1, "reps"], v[2, "reps"], \
               v[1, "faults"], v[2, "faults"], bits
         }')"
      echo "$line"
      case "$line" in *bits=DIFFER*) differ=$((differ + 1)) ;; esac
   done
done
if [ "$differ" -eq 0 ]; then echo "ALL BITS SAME"; else echo "BITS DIFFER in $differ rows"; fi
[ "$failed" -eq 0 ] || echo "$failed run(s) FAILED"
[ "$differ" -eq 0 ] && [ "$failed" -eq 0 ]
