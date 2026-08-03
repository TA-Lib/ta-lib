#!/usr/bin/env bash
# Verify the Rust FMA runtime dispatch actually fired (issue #156).
#
# The generated crate compiles the 26 fused indicators twice — a portable
# baseline arm (libm fma() call) and a #[target_feature(enable = "fma")]
# clone (hardware vfmadd) selected at runtime — mirroring the C library's
# target_clones("default","fma"). Both arms are correctly rounded, so every
# value gate passes even if the clones silently vanish; this script is the
# perf regression guard the value gates cannot be.
#
# Asserts on the built Rust server binary:
#   1. >= MIN_VFMADD hardware vfmadd instructions (the clones exist;
#      a baseline-only build has ~2, a dispatched build ~400+)
#   2. the portable scalar fma machinery remains (compiler-builtins' fma
#      on current toolchains, or a libm fma@GLIBC import on older ones;
#      a build that lost it would SIGILL pre-Haswell CPUs)
#
# Usage: scripts/verify-rust-fma-dispatch.sh [path-to-ta_codegen_serve_rust]
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${1:-}"
if [[ -z "$BIN" ]]; then
  for cand in "$REPO/bin/ta_codegen_serve_rust" \
              "$REPO/ta_codegen/output/rust/target/release/ta_codegen_serve"; do
    [[ -x "$cand" ]] && BIN="$cand" && break
  done
fi
if [[ -z "$BIN" || ! -x "$BIN" ]]; then
  echo "ERROR: Rust server binary not found (build with: scripts/build.py servers)" >&2
  exit 2
fi

MIN_VFMADD="${MIN_VFMADD:-200}"

VFMADD_COUNT="$(objdump -d "$BIN" | grep -c 'vfmadd' || true)"
# Portable arm: compiler-builtins' statically-linked scalar fma (current
# toolchains) or a dynamic libm import (older toolchains linked glibc fma).
PORTABLE_COUNT="$(nm "$BIN" 2>/dev/null | grep -cE 'fma_fallback|(^|[[:space:]])U fma(@|$)| fma$' || true)"

echo "binary:          $BIN"
echo "vfmadd count:    $VFMADD_COUNT (min $MIN_VFMADD)"
echo "portable fma:    $PORTABLE_COUNT symbol(s) (min 1)"

FAIL=0
if (( VFMADD_COUNT < MIN_VFMADD )); then
  echo "FAIL: hardware-FMA clones missing or stripped (vfmadd $VFMADD_COUNT < $MIN_VFMADD)" >&2
  FAIL=1
fi
if (( PORTABLE_COUNT < 1 )); then
  echo "FAIL: portable scalar-fma arm missing (no compiler-builtins/libm fma symbol)" >&2
  FAIL=1
fi
if (( FAIL )); then exit 1; fi
echo "OK: FMA dispatch present — hardware clones + portable arm both in the binary"
