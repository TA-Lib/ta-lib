#!/usr/bin/env bash
# Runs chunk <chunk> (1..<nchunks>) of the package's functions through
# wasm_routing_bench.mjs, one Node process per function, appending JSON lines to
# <outfile>; a process that exits non-zero appends {"fail":"<FUNC>"}.
#
# Usage: [PIN="taskset -c 2"] [BENCH_ARGS="--rounds=15"] \
#        scripts/wasm_routing_run.sh <package-dir> <outfile> <chunk> <nchunks>
#
# Run it inside a measure window, e.g.
#   scripts/quiet.py measure <session> 720 --queue=900 -- scripts/wasm_routing_run.sh ...
# Functions are dealt round-robin over the sorted names, so a family of
# neighbours (CDL*) spreads over every chunk.
set -euo pipefail

if [ $# -ne 4 ]; then
  sed -n '6,7p' "$0" >&2
  exit 2
fi
pkg=$(cd "$1" && pwd)
out=$2
chunk=$3
nchunks=$4
case "$chunk$nchunks" in *[!0-9]*) echo "chunk and nchunks are integers" >&2; exit 2;; esac
if [ "$chunk" -lt 1 ] || [ "$chunk" -gt "$nchunks" ]; then
  echo "chunk must be in 1..$nchunks" >&2
  exit 2
fi
here=$(cd "$(dirname "$0")" && pwd)
bench="$here/wasm_routing_bench.mjs"
read -r -a pin <<< "${PIN:-}"
read -r -a extra <<< "${BENCH_ARGS:-}"

meta=$(node -e 'const p=require("path"),f=require("fs");const d=process.argv[1];const m=JSON.parse(f.readFileSync(p.join(d,"package.json"),"utf8"));console.log(require("url").pathToFileURL(p.join(d,m.exports["./metadata"].default)).href)' "$pkg")
mapfile -t funcs < <(node --input-type=module -e "const m = await import(process.argv[1]); for (const f of m.Functions.all().map((f) => f.name).sort()) console.log(f);" "$meta")
[ "${#funcs[@]}" -gt 0 ] || { echo "no functions listed from $pkg" >&2; exit 1; }

case "$(uname -s)" in
  Darwin) cpu=$(sysctl -n machdep.cpu.brand_string) ;;
  *) cpu=$(lscpu | sed -n 's/^Model name:[[:space:]]*//p' | head -n 1) ;;
esac
commit=$(git -C "$here" rev-parse HEAD)
git -C "$here" diff --quiet HEAD -- 2>/dev/null || commit="$commit (dirty)"
{
  echo "node=$(node --version)"
  echo "cpu=$cpu"
  echo "arch=$(uname -m)"
  echo "commit=$commit"
  echo "package=$pkg"
  echo "pin=${PIN:-}"
  echo "bench_args=${BENCH_ARGS:-}"
  echo "chunk=$chunk/$nchunks"
  echo "started=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >> "$out.env"

i=0
for f in "${funcs[@]}"; do
  if [ $((i % nchunks + 1)) -eq "$chunk" ]; then
    if ! ${pin[@]+"${pin[@]}"} node "$bench" "$pkg" "$f" ${extra[@]+"${extra[@]}"} >> "$out"; then
      printf '{"fail":"%s"}\n' "$f" >> "$out"
    fi
  fi
  i=$((i + 1))
done
echo "finished=$(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$out.env"
