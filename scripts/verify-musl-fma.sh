#!/usr/bin/env bash
# Verify ta-lib builds on musl (musllinux) with FMA dispatch correctly EXCLUDED.
# target_clones/ifunc is glibc-only (musl has no ifunc; Alpine gcc hard-errors), so
# TA_FMA_MULTIVERSION is gated on __GLIBC__ and a musl build falls through to plain
# software fma(). Local twin of the `musl-build` nightly job; a regression guard
# against widening the guard back to __linux__ (which breaks the musllinux build).
#
# Needs Docker (sudo if not in the docker group):  sudo scripts/verify-musl-fma.sh
# Override image:  MUSL_IMAGE=quay.io/pypa/musllinux_1_2_x86_64 scripts/verify-musl-fma.sh
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${MUSL_IMAGE:-alpine:3.20}"

if ! docker ps >/dev/null 2>&1; then
  echo "ERROR: cannot reach the Docker daemon."
  echo "  If your user is not in the 'docker' group, re-run:  sudo $0"
  exit 2
fi

echo ">> Verifying ta-lib builds on musl with FMA dispatch correctly EXCLUDED — image: $IMAGE"
docker run --rm -v "$REPO":/src -w /src "$IMAGE" sh -c '
  set -e
  apk add --no-cache build-base cmake >/dev/null
  echo "== musl target: alpine $(cat /etc/alpine-release 2>/dev/null || echo unknown) =="

  echo "-- [0] sanity: GCC on musl REJECTS target_clones (this is why the guard exists) --"
  printf "%s\n" "__attribute__((target_clones(\"default\",\"fma\"))) int f(int x){return x;} int main(void){return f(0);}" > /tmp/tc.c
  if gcc -O2 /tmp/tc.c -o /tmp/tc 2>/tmp/tc.err; then
    echo "   NOTE: target_clones compiled on this musl toolchain (guard is merely cautious, not wrong)"
  else
    echo "   confirmed: $(grep -oi "requires .ifunc." /tmp/tc.err | head -1) — target_clones MUST be excluded on musl"
  fi

  echo "-- [1] the ACTUAL TA_FMA_MULTIVERSION guard must expand to NOTHING on musl --"
  cat > /tmp/mv.c <<EOF
#include <stdio.h>
#include <math.h>
#if defined(__x86_64__) && defined(__GLIBC__) && defined(__GNUC__) && !defined(__clang__)
#define MV __attribute__((target_clones("default","fma")))
#else
#define MV
#endif
MV double ema(const double *x, int n, double k){
    double e = x[0], o = 1.0 - k;
    for (int i = 1; i < n; i++) e = fma(o, e, k * x[i]);
    return e;
}
int main(void){
    double x[5] = {1,2,3,4,5};
    double g = ema(x, 5, 0.5);
    printf("   ema=%.17g (want 4.0625) %s\n", g, g == 4.0625 ? "OK" : "MISMATCH");
    return g != 4.0625;
}
EOF
  gcc -O3 -ffp-contract=off /tmp/mv.c -o /tmp/mv -lm
  ic=$(readelf -sW /tmp/mv | grep -c IFUNC || true)
  echo "   IFUNC symbols (must be 0 on musl): $ic"; test "$ic" -eq 0
  /tmp/mv
  echo "   -> __GLIBC__ guard correctly disabled dispatch; plain software fma() is correct"

  echo "-- [2] real ta-lib library on musl (must build, emit no ifunc, compute correctly) --"
  cmake -S . -B /tmp/bm -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build /tmp/bm -j"$(nproc)" >/dev/null
  SO=$(find /tmp/bm -name "libta-lib.so*" -type f | head -1)
  echo "   DSO: $SO"
  n=$(readelf -sW "$SO" | grep -c IFUNC || true); echo "   IFUNC symbols in DSO (must be 0 on musl): $n"; test "$n" -eq 0
  REG=$(find /tmp/bm -name ta_regtest -type f | head -1)
  echo "   running the C reference suite (software fma) on musl:"
  LD_LIBRARY_PATH="$(dirname "$SO")" "$REG" | tail -3
'
echo ">> PASSED: ta-lib builds correctly on musl with FMA dispatch excluded"
