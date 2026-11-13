#!/bin/sh
# Usage: run.sh <cpu> [maxreps=6] [funcs] [lens] [alias_n=30000] [alias_offs=0]
# Times ab_old then ab_new back to back per (function, length); one line per pair.
# Then one aliased call per function and offset (output 0 at that element of
# the first input), old vs new. Offset 0 lands every output on a bar already
# consumed; an offset past the lookback (5000) overwrites bars not yet read.
# Exits 1 if any row's bits differ or any run failed.
if [ $# -lt 1 ]; then
   echo "usage: $0 <cpu> [maxreps=6] [funcs] [lens] [alias_n=30000] [alias_offs=0]" >&2
   exit 2
fi
if [ -z "$AB_PINNED" ] && command -v taskset >/dev/null 2>&1; then
   AB_PINNED=1 exec taskset -c "$1" "$0" "$@"
fi
HERE="$(cd "$(dirname "$0")" && pwd)"
MAXREPS="${2:-6}"
FUNCS="${3:-SQZMOM GATOR ICHIMOKU KC KSTEXT MACDEXT STOCHRSI CRSI SMA}"
LENS="${4:-250 2000 19999 20000 20001 100000 1000000}"
ALIAS_N="${5:-30000}"
ALIAS_OFFS="${6:-0}"

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
for f in $FUNCS; do
for ALIAS_OFF in $ALIAS_OFFS; do
   o="$("$HERE/ab_old" alias "$f" "$ALIAS_N" "$ALIAS_OFF")" || { echo "$f alias $ALIAS_N FAILED old"; failed=$((failed + 1)); continue; }
   w="$("$HERE/ab_new" alias "$f" "$ALIAS_N" "$ALIAS_OFF")" || { echo "$f alias $ALIAS_N FAILED new"; failed=$((failed + 1)); continue; }
   line="$(printf '%s\n%s\n' "$o" "$w" | awk -v off="$ALIAS_OFF" '
      { for (i = 4; i <= NF; i++) { split($i, kv, "="); v[NR, kv[1]] = kv[2] } f = $1; n = $2 }
      END {
         same = (v[1, "hash"] == v[2, "hash"] && v[1, "rc"] == v[2, "rc"] && v[1, "nb"] == v[2, "nb"])
         bits = same ? "bits=same" : sprintf("bits=DIFFER old=%s/rc%s/nb%s new=%s/rc%s/nb%s", \
            v[1, "hash"], v[1, "rc"], v[1, "nb"], v[2, "hash"], v[2, "rc"], v[2, "nb"])
         printf "%s alias %s %s%s\n", f, n, (off + 0 ? "off=" off " " : ""), bits
      }')"
   echo "$line"
   case "$line" in *bits=DIFFER*) differ=$((differ + 1)) ;; esac
done
done
if [ "$differ" -eq 0 ]; then echo "ALL BITS SAME"; else echo "BITS DIFFER in $differ rows"; fi
[ "$failed" -eq 0 ] || echo "$failed run(s) FAILED"
[ "$differ" -eq 0 ] && [ "$failed" -eq 0 ]
