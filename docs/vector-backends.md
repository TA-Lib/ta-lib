# Vector-Math Backends for ta_codegen (SIMD via platform libraries)

**Status:** 📝 **PROPOSED — investigation complete, implementation NOT STARTED (2026-07-14).**
Originates from [PR #85](https://github.com/TA-Lib/ta-lib/pull/85) ("feat: add Apple platform
support" by @ralph-e-boy), which added Apple Accelerate (vDSP/vForce) SIMD for 16 element-wise
functions. That PR predates the `ta_codegen` cutover and hand-edits files that are now generated, so
it cannot be merged as-is. This document is the committed record of the investigation and the
recommended way to deliver the capability *through the generator*, in a form that generalizes to
other vector-math libraries on other platforms. No code has changed yet.

---

## 1. Summary & recommendation

- **Do not merge PR #85 as-is.** It is obsolete in *form* (it hand-edits `src/ta_func/*.c`, which
  `ta_codegen` now generates in place), it is marked `CONFLICTING`, and — the load-bearing
  surprise — **it has no effect on `brew install ta-lib` at all**, because Homebrew builds TA-Lib
  with **autotools**, not CMake, and the PR wires Accelerate only into CMake.
- **Do rebuild the idea inside `ta_codegen`** as a small, data-driven "vector backend" abstraction:
  one central table of libraries + one emitter codepath that wraps the *already-generated* scalar
  loop in a `#if defined(TA_USE_X) … #elif … #else <scalar> #endif` cascade. Adding a new library
  on a new platform becomes **data**, not new emitter code. The input `.c` files stay clean
  standard-C loops with no macros; the `#if` noise exists only in the generated output.
- **Prove the whole machine on Linux with a portable library (SLEEF) first**, *before* touching the
  Apple-only path that cannot be built or tested on non-Apple developer machines. When the Apple
  row is added later, only its numeric results are unknown — the codegen, build wiring, tolerance
  oracle, and CI gate are already green.
- **Separate two deliverables with very different value/risk** (see §2). The iOS/macOS *build and
  packaging* support is high-value, low-risk, and can land now. The *Accelerate optimization* is
  modest-value (it only accelerates rarely-used functions — see §8) and carries a real correctness
  subtlety (§4) that must be gated by a new tolerance test.

---

## 2. What PR #85 contains, and what to do with each part

Classification of every change. **(a)** = obsolete, would be overwritten by `cargo run -- generate`;
**(b)** = valuable standalone, mergeable independently of codegen; **(c)** = needs re-implementation
inside `ta_codegen`.

| Change | Class | Belongs in | Notes |
|---|---|---|---|
| 16× `#if defined(TA_USE_ACCELERATE)…` blocks in `src/ta_func/ta_{ACOS,ADD,ASIN,ATAN,COS,COSH,EXP,LN,LOG10,MULT,SIN,SINH,SQRT,SUB,TAN,TANH}.c` | (a)+(c) | `ta_codegen` input metadata + C emitter | The only real optimization; every hand-edit here is clobbered on regen. Rebuild per §3. |
| `src/ta_func/ta_veclib.h` (new): dispatch macros + `<vecLib/…>` includes | (c) | Generator template `ta_codegen/generator/templates/c/`, emitted into `src/ta_func/` | Must be generator-emitted to stay regen-clean; must be added to `EXTRA_DIST` (see §5). |
| `ta_utility.h`: `#include "ta_veclib.h"` | (c) | Generator header wiring | Part of the same codegen change. |
| CMake `TA_USE_ACCELERATE` option + compile-def + `-framework Accelerate` | (c)-build | Hand-maintained CMake region (outside the `ta_codegen` `LIB_SOURCES` markers) | Activation half; ship with the codegen change. **Does not affect brew** (§4.1). |
| CMake `APPLE`/`IOS`/`MACOS` detection, deployment targets, `XCODE_ATTRIBUTE_*`, iOS install rules | (b) | Build system | Useful, independent of Accelerate. Rebase onto the current `BUILD_SHARED_LIBS`/`BUILD_STATIC_LIBS` target list (PR base predates #78 and would break shared-only builds). |
| CMake `status(FATAL_ERROR …)` → `message(FATAL_ERROR …)` | (b) | Build system | Real bug fix (`status()` is not a CMake command). Trivial. |
| CMake `aarch64` → `aarch64\|arm64` processor match | (b) | Build system | Correct — Apple/CMake report `arm64`. |
| CMake `ta_regtest` libm link: `m` only when `NOT WIN32 AND NOT APPLE` | (b) | Build system | Correct — Apple libm is in libSystem. |
| `include/ta_common.h`: move system `#include`s outside `extern "C"` | (b) | Hand-maintained header | Legitimate C++ interop hygiene. |
| `mach_absolute_time` `TIMER_*` abstraction in `ta_test_priv.h` + use in `ta_regtest.c`/`test_abstract.c`/`test_util.c` | (b) | Tools/tests | Hi-res portable timer + a `clockDelta<=0` fatal→graceful robustness fix. |
| `test_abstract.c`: `!= TA_SUCCESS` → `!= TA_TEST_PASS` on `testLookback()` | (b) | Tools/tests | Correctness fix (distinct enums). |
| `scripts/ios/build-ios.sh`, `build-xcframework.sh` (new) | (b) | `scripts/` | Standalone packaging tooling. |
| `cmake/README.md` pointing at `leetal/ios-cmake` | (b), caveat | Build docs | The referenced toolchain file is not actually in the diff; resolve vendoring/license provenance before relying on it. |
| `scripts/run_bench.sh` (new) | (b), drop | — | Redundant with `ta_bench`/`regtest.py`; references flags that no longer exist. Fold any A/B into `ta_bench`. |

**Takeaway:** everything except the 16 `.c` hunks + `ta_veclib.h` + the two activation lines is
class-(b) and can be cherry-picked today; the actual optimization must be rebuilt in `ta_codegen`.

---

## 3. Recommended architecture

A **data-driven** design: one central library table + one emitter codepath. Adding a library edits
the table, never the emitter; adding a function to the program adds one library-neutral token to
that function's YAML.

### 3.1 Injection point (the one place in the emitter)

The batch function body is rendered by `gen_func_inner` in
`ta_codegen/generator/src/backends/c.rs` — specifically the body-statement render loop (around
`c.rs:621-631` as of this writing), where each IR statement (including the element-wise `for`/`while`
loop parsed from `input/<name>/<name>.c`) is emitted. The optimization wraps **the scalar loop the
generator already emits**:

```
#if defined(TA_USE_ACCELERATE) && (!defined(USE_SINGLE_PRECISION_INPUT))
   ACCEL_VFORCE_1IN(vvsin);
#elif defined(TA_USE_SLEEF)
   SLEEF_1IN(Sleef_sind4_u10);
#else
   for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 )
   {
      outReal[outIdx] = sin(inReal[i]);      /* <- unchanged, from input/sin/sin.c */
   }
#endif
   *outNBElement = outIdx;                    /* tail rendered unchanged after #endif */
   *outBegIdx    = startIdx;
   return TA_SUCCESS;
```

Because the `#else` branch **is** the scalar loop generated from `input/*.c`, the fallback can never
drift from the reference algorithm, and the input `.c` file stays a plain standard-C loop with **no
macros** — the entire preprocessor apparatus lives only in generated output. A `#if` cannot be
expressed in the parsed-C statement IR, so it is injected by the emitter around the rendered body,
keyed off metadata — not parsed from the input.

**Dispatch only in the double batch variants.** Each of these functions emits several variants
(guarded / unguarded / `TA_S_*` single-precision / streaming one-value). The vector call is valid
only for the `double` batch loop; the float and streaming paths stay scalar.

### 3.2 The data (one central file: `ta_codegen/input/vector_backends.yaml`)

This one file is the whole extension surface. Two sections:

- **Capability profiles**, one per library: the `guard` macro (also the compile-define), required
  `headers`, `link` kind (`framework` / `lib` / `none`), a `cmake_detect` mode
  (`apple` / `pkg_or_find` / `always`), an optional `precision_guard`, and per-arity **calling
  convention** templates (the call shapes genuinely differ across libraries — see §7).
- **Symbol map**: abstract op → per-library symbol, e.g.
  `sin: { accelerate: vvsin, sleef: {sym: Sleef_sind4_u10, w: 4}, mkl_vml: vdSin }`. A missing
  `(op, library)` cell simply drops that library from the cascade for that function.

This is global, non-algorithmic configuration — the same category as `enums.yaml`, and it belongs
under `input/` so a fork/regen is fully reproducible.

### 3.3 Per-function opt-in (library-neutral)

Each vectorizable function's own YAML gains **one** library-neutral token, in the same spirit as the
existing `flags: [stream]` opt-in:

```yaml
# ta_codegen/input/sin/sin.yaml
vector_op: sin
# ta_codegen/input/sub/sub.yaml
vector_op: sub        # the vDSP_vsubD "B - A" operand swap lives in the profile, not here
```

Arity comes from the existing `inputs:` list, so nothing library-specific touches the function file.
Adding a new library later touches **zero** function YAMLs. Functions the contributor benchmarked as
*not* worth vectorizing (CEIL/FLOOR/DIV, and the per-window families) simply carry no `vector_op` and
are auto-excluded.

### 3.4 One emitter codepath

A single helper (`wrap_vectorized(op, arity, scalar_block, backends)`, ~40 lines) is called from the
one body-loop site. It iterates the profiles that have a symbol for `op` (in a fixed, deterministic
priority order), emits `#if`/`#elif` arms filling each profile's convention template with the
function's real identifiers, then `#else <scalar_block> #endif`. **No library is named anywhere in
Rust code** — Apple/SLEEF/MKL are rows in the table. It must also express *flag-only* backends
(OpenMP-simd / `libmvec`): those need no symbol and no call — the arm just annotates the scalar loop
with `#pragma omp simd` and the build adds a flag (§3.6).

### 3.5 Generated dispatch header

`ta_veclib.h` becomes a generated `templates/c/` asset (peer of `ta_retcode.c.template`), written
into `src/ta_func/` and `#include`d via the emitted `ta_utility.h`. It carries the dispatch macros
and the per-profile `#if defined(GUARD) #include <hdr> #endif` include blocks, derived from the
profiles. It is a **header**, so it is *not* a member of the `.c`-only `LIB_SOURCES` /
`Makefile.am` source lists (confirmed: `ta_utility.h` / `ta_memory.h` aren't either) → **no
build-list edits**. It does, however, need one `EXTRA_DIST` entry so `make dist` ships it into the
release tarball (§5).

### 3.6 Build wiring (generic, not Apple-specific)

Both `cmake_lists.rs` and `makefile_am.rs` emit a marker-delimited "vector backends" block driven by
the **same** profiles — the emitter never says "Apple". Each profile contributes, from its
`cmake_detect` + `guard` + `link` fields:

- `cmake_detect: apple` → `if(APPLE) … endif()`; `pkg_or_find` → `find_library(...)`; `always` → no
  gate.
- `link.kind: framework` → `-framework X`; `lib` → `find_library` + link; `none` → compile-option
  only (the flag-only backends).

A `..._FOUND_DEFAULT` default means "if the library is present, default its option ON" — so a Linux
box with `libsleef-dev` auto-enables SLEEF, an Apple box auto-enables Accelerate, and a bare box
falls to scalar, with no user flag and no platform knowledge in the generator.

### 3.7 What stays inert

- **Other backends unchanged.** `rust`/`java`/`dotnet` emitters never read `vector_op`, so their
  output is byte-identical. (Adding an `Option<…>` field to the IR + one `#[serde(default)]` YAML
  field is transparent to them.)
- **Regeneration oracle stays green.** The annotation + wrap logic + template copy are fully
  deterministic; a fresh `generate` reproduces identical bytes and leaves git clean — the exact
  property PR #85's hand-edited generated files violate.

---

## 4. Correctness, brew, and testing

### 4.1 Homebrew builds with autotools, not CMake

The live `homebrew-core` formula (`Formula/t/ta-lib.rb`) builds via
`autoreconf → ./configure → make install`, then installs and runs `ta_regtest` as its `test do`
check. It ships **prebuilt, per-OS/per-arch bottles**; a normal `brew install` downloads a bottle and
never compiles. Consequences:

- PR #85's CMake-only Accelerate wiring is **dead code for brew**. The auto-detection must live in
  the hand-written **`configure.ac`** (codegen does not own it):

  ```m4
  AC_CANONICAL_HOST
  case $host_os in
    darwin*)
      CPPFLAGS="$CPPFLAGS -DTA_USE_ACCELERATE"
      LIBS="$LIBS -framework Accelerate"
      ;;
  esac
  ```

  Using global `CPPFLAGS`/`LIBS` reaches the `ta_func` compiles without editing the codegen-owned
  `src/ta_func/Makefile.am`. (An `AC_DEFINE` into a config header would *not* work — the indicator
  sources don't include it.) Optionally gate on `AC_CHECK_HEADER([Accelerate/Accelerate.h])` and
  expose a `--without-accelerate` escape hatch, keeping the default auto-ON on Darwin.
- `ta_veclib.h` must be in `EXTRA_DIST` so it reaches the brew tarball (the formula's `test do`
  links the full library through `ta_regtest`).
- **Compile-time gating is correct; runtime dispatch is unnecessary.** Accelerate is a core system
  framework present on every macOS (since 10.3; vForce since 10.4), so linking always resolves, and
  bottles are already per-OS/arch. Accelerate itself does the internal CPU dispatch.
- The CMake wiring is still worth keeping for the non-brew consumers (vcpkg, dev builds, deb/msi
  packaging), placed **outside** the `# [ta_codegen begin/end LIB_SOURCES]` markers so regeneration
  preserves it.

### 4.2 The bit-exactness subtlety (the crux)

The 13 vForce transcendentals are **not bit-identical** to scalar `libm` — they are within roughly a
few ULP, not correctly-rounded (e.g. `vvcos(0.0)` returns `0.999…9`; scalar vs Accelerate diverge on
a large fraction of inputs by ~1 ULP). The 3 vDSP arithmetic ops (`ADD`/`SUB`/`MULT`) are expected
bit-exact (IEEE-754 requires correctly-rounded `+ − ×`, with no multiply-add to fuse); `SQRT` routed
through vForce is uncertain. This interacts with the repo's oracles:

- **`fuzz-064`** (the only truly bit-exact oracle — `memcmp` of doubles vs frozen 0.6.4) runs
  **ubuntu-only**, where the `#if` compiles out → it stays green. `--codegen` already tolerates
  `1e-6` → green. The macOS `test-macos` job uses `0.01` tolerance → green.
- **So no existing gate turns red — which is the problem.** "Passes `ta_regtest` ⇒ 100% backward
  compatible" would be satisfied *vacuously*: no gate ever exercises the Accelerate path, and the
  bit-exact-vs-0.6.4 contract is silently violated on Apple hardware.

**Edge cases to test on Apple hardware** (all 16 input `.c` files are bare loops with no domain
clamping): out-of-domain `ASIN`/`ACOS` (|x|>1), `LN`/`LOG10`/`SQRT` of ≤0 → NaN whose *bit pattern*
may differ between vForce and scalar libm, which a `memcmp` oracle is sensitive to. Policy: treat
NaN-vs-NaN as benign regardless of payload, or restrict dispatch to in-domain inputs.

### 4.3 The tolerance contract and a non-vacuous gate

- **Accept bounded divergence for the transcendentals**, expressed as a per-function **ULP-count**
  tolerance (not a fixed absolute — outputs span huge magnitudes), gated behind the backend's guard
  so the ubuntu bit-exact gate keeps demanding true bit-exactness and only the accelerated build
  relaxes. This is consistent with existing methodology: the `FUZZ_064_TOL[]` manifest already
  authorizes bounded, documented divergences (CCI, the LINEARREG family, TSF, IMI), and `--codegen`
  already lives at `1e-6`.
- **Keep `ADD`/`SUB`/`MULT` bit-exact** (no manifest entry). If a hardware check ever shows a
  larger-than-signed-zero diff, drop that function from the vector path rather than widen tolerance.
- **Add a real gate: a parity differential.** Compare a `TA_USE_X=ON` build against a same-machine
  `OFF` build (isolating the vector-library delta from all libm/compiler variance) under the ULP
  manifest — generalize the contributor's own A/B methodology into `ta_regtest` (e.g. an
  `--accel-parity` mode, modeled on the `fuzz_ref064` server-differential). Add a
  `test-macos-accelerate` CI job (sibling of the existing scalar-only `test-macos`) running
  `macos-latest` + `TA_USE_ACCELERATE=ON` → plain `ta_regtest` (smoke) + the parity differential.
  Keep `fuzz-064`/`--codegen` ubuntu-only; rely on the regen-clean job to prove the `#if` blocks
  come from the generator, not a hand-edit.

### 4.4 Developer workflow (write-blind for Apple, fully local for the rest)

The scalar-user guarantee — that non-Apple builds are byte-identical to today — is **100%
verifiable on Linux** (`generate` + regen-check + `fuzz-064` + `--codegen`). Only the vForce numbers
are unobservable off-Apple; those iterate through the dedicated macOS CI job. SLEEF (§6) collapses
most of that blind surface by letting the *shape* of the optimization be exercised locally.

---

## 5. Distribution / packaging checklist

- `ta_veclib.h` → generator template + emitted into `src/ta_func/` + one `EXTRA_DIST` entry (autotools
  `make dist`) and, if `scripts/package.py` enumerates headers explicitly, one manifest line.
- No change to `LIB_SOURCES` (CMake) or `SOURCES` (`Makefile.am`) — it is a header, not a compile
  unit.
- `configure.ac` Darwin case (§4.1); optional `--without-accelerate`.
- Keep the CMake vector-backend block outside the `ta_codegen` markers.

---

## 6. SLEEF-first: prove the machine on Linux

Adopt **SLEEF** as the first concrete backend and build the entire abstraction against it *before*
merging any Apple-only path:

1. **It is the only option this box and Linux CI can compile, run, and diff.** `apt install
   libsleef-dev` provides a Boost-licensed portable vector libm (x86 SSE2/AVX2/AVX512, AArch64, and
   a pure-C fallback). Accelerate cannot be built or validated off Apple at all.
2. **Clean license.** Boost-1.0 is permissive and OSI-approved, compatible with TA-Lib's BSD-3
   posture — SLEEF can be a documented optional dependency, not an untestable `#ifdef`.
3. **It forces genuine generality.** SLEEF's convention differs from Apple's in the two hardest ways
   (per-SIMD-register rather than whole-array, and *selectable* accuracy — 1-ULP `u10` vs 3.5-ULP
   `u35`). If the table + emitter express both, "calling convention is data" is proven rather than
   asserted.
4. **It builds the numeric safety net Apple needs anyway.** The ULP tolerance class and the parity
   differential can be *designed and calibrated on hardware you have*, so the Apple/MKL rows plug
   into an already-proven gate.
5. **Lowest-risk first slice.** Start with the flag-only `-fopenmp-simd` / `libmvec` (or SLEEF's
   GNU-ABI) profile: no symbol map, no call-site change — just a `#pragma omp simd` annotation and a
   compile flag. Validate the build plumbing at essentially zero risk to output, then add the
   explicit-call SLEEF-direct path (which needs a register-chunked loop wrapper with a scalar tail).

Apple Accelerate then becomes a *second* profile row, its correctness validated by the macOS CI job
against the same tolerance gate rather than blind-merged.

---

## 7. Library landscape (what the abstraction must generalize to)

| Library | Transcendentals? | Calling convention | License | Platforms | Testable on Linux/WSL? |
|---|---|---|---|---|---|
| Apple Accelerate — vForce (`vvsin`, `vvexp`, …) | Full | array, out-first: `vvsin(out, in, &n)` | Apple SDK | macOS/iOS/tvOS/watchOS | **No** |
| Apple Accelerate — vDSP (`vDSP_vaddD`, …) | Arithmetic only | array+stride; SUB/DIV swap operands | Apple SDK | same | **No** |
| Intel oneMKL — VML (`vdSin`, `vdAdd`, …) | Full | array, count-first: `vdSin(n, in, out)` | ISSL (proprietary, redistributable) | x86/x86-64 | Partial (x86-64 + proprietary) |
| SVML (`__svml_sin4`, …) | Full | per-SIMD-register / via `-fveclib` | Bundled w/ Intel toolchain | x86-64 | Partial |
| Arm Performance Libraries / Arm Optimized Routines | Full | Vector Function ABI (`_ZGV…`) | ArmPL binary: Arm EULA; **source routines: MIT OR Apache-2.0** | AArch64 | No (AArch64-only) |
| **SLEEF** (`Sleef_sind4_u10`; GNU-ABI `_ZGV…`) | Full (1-ULP + 3.5-ULP) | per-register, or `_ZGV…` via `-fveclib` | **Boost-1.0 (OSI)** | x86, AArch64, RISC-V, pure-C | **Yes** (`apt install libsleef-dev`) |
| Compiler auto-vec / OpenMP `#pragma omp simd` | via backing veclib (`libmvec`) | **no call-site change** — annotate loop | standard pragma | portable (glibc `libmvec`) | **Yes** |

Two integration families the abstraction must both express: **explicit-call** libraries (need a
symbol + a convention wrapper) and **flag-only** libraries (need only a build flag + a `#pragma omp
simd`, no symbol, no call-site injection).

---

## 8. Scope & ROI note (be honest about the payoff)

The contributor's own benchmarks (reproduced on the PR) show Accelerate wins **only when it replaces
an entire O(n) loop with a single whole-array vForce/vDSP call** — the element-wise transcendentals,
which see 1.8×–6× at ≥10k elements. It **loses** whenever there is a per-window inner loop:
`LINEARREG` (5 funcs), `STDDEV`, `CORREL`, and the `HT_*` family were all tested and *rejected*
(the O(1) call overhead inside an O(n) outer loop exceeds the SIMD savings), and `DIV`/`CEIL`/`FLOOR`
lose to the compiler's single-instruction lowering. So the direct speed payoff lands on the
**least-used** functions (SIN/COS/EXP/LN…), not the popular indicators (RSI, MACD, BBANDS).

The durable value of this work is therefore twofold and mostly *not* the raw SIMD speedup:

1. **iOS/macOS platform support** (the class-(b) build + packaging changes) — high-value, low-risk,
   landable now.
2. **A reusable, tested vector-backend framework** you can later aim at higher-value targets, on a
   foundation that has a real correctness gate.

---

## 9. Phasing

- **Phase 0 (now, independent):** cherry-pick the class-(b) build/timer/bugfix changes from PR #85
  (iOS/macOS CMake detection, `status()`→`message()`, `arm64` match, `mach_absolute_time` timer,
  `extern "C"` reorder, `TA_TEST_PASS` fix). Rebase onto the current shared/static target list. Drop
  `scripts/run_bench.sh`. Credit @ralph-e-boy.
- **Phase 1 (foundation, 100% Linux-testable):** build `vector_backends.yaml` + `wrap_vectorized` +
  generated `ta_veclib.h` + the generic CMake/autotools wiring + the ULP tolerance manifest and
  parity differential, all proven with **SLEEF** (flag-only `libmvec`/OpenMP-simd first, then
  SLEEF-direct).
- **Phase 2 (Apple, gated):** add the `accelerate` profile row + the `configure.ac` Darwin case +
  `EXTRA_DIST` + the `test-macos-accelerate` CI job. Nothing new to invent — the machine is proven.

---

## 10. The one decision that is the maintainer's to make

Everything above works technically. The judgment call is the public promise on the PR thread —
*"works the same everywhere / 100% backward compatible."* Accelerate makes `TA_SIN` (and the other 12
transcendentals) on Apple differ from scalar `libm` by ~1 ULP; strict bit-identity is impossible for
vForce.

- **Recommended:** accept a **documented, tested few-ULP contract** for those 13 functions — with the
  non-vacuous macOS parity gate (§4.3), so "passes `ta_regtest`" means something for this path.
  Precedent: `libm` already differs across platforms; `--codegen` already tolerates `1e-6`;
  `FUZZ_064_TOL[]` already authorizes bounded exceptions.
- **If instead strict bit-identity is required:** Accelerate is off the table for the transcendentals,
  and only the bit-exact-but-neutral (~1.0×) `ADD`/`SUB`/`MULT` could ship — in which case the
  optimization is not worth it, and only Phase 0 (platform support) should be taken.

---

## 11. References

**Codebase anchors** (as of 2026-07-14; verify before implementing):
- Emitter body-loop render / injection point: `ta_codegen/generator/src/backends/c.rs`
  (`gen_func_inner`, body-statement loop ~`c.rs:621-631`; include site `gen_header` ~`c.rs:184-191`).
- Build-list generators: `ta_codegen/generator/src/backends/cmake_lists.rs` (marker-delimited
  `LIB_SOURCES` splice), `makefile_am.rs` (regenerates `src/ta_func/Makefile.am`).
- Shared C templates: `ta_codegen/generator/templates/c/` (e.g. `ta_retcode.c.template`); template
  copy pattern in `main.rs`.
- Bit-exact oracle: `src/tools/ta_regtest/test_codegen.c` — `memcmp` at ~`:3182`; `FUZZ_064_TOL[]`
  manifest ~`:3101-3110`; `CODEGEN_EPSILON = 1e-6` ~`:92`.
- CI: `.github/workflows/dev-nightly-tests.yml` — `test-macos` job ~`:170` (scalar-only, macos-latest);
  `fuzz-vs-064` ~`:316` (ubuntu). `main-nightly-tests.yml` has no macOS leg.
- Brew tooling: `scripts/post-release-brew.py`, `scripts/utilities/common.py`,
  `scripts/package.py` (`package_src_tar_gz`).
- Clean input examples: `ta_codegen/input/sin/sin.c`, `input/add/add.c`, `input/sub/sub.c`.

**External:**
- PR #85: https://github.com/TA-Lib/ta-lib/pull/85
- SLEEF: https://sleef.org/ · https://github.com/shibatch/sleef · Debian `libsleef-dev`
- Apple Accelerate / vecLib: https://developer.apple.com/accelerate/ ·
  https://developer.apple.com/documentation/accelerate/veclib ·
  `vDSP_vsub` operand-swap caution: https://developer.apple.com/documentation/accelerate/1449900-vdsp_vsub
- Arm Optimized Routines (MIT OR Apache-2.0): https://github.com/ARM-software/optimized-routines
- Intel oneMKL VML reference: https://www.intel.com/content/www/us/en/docs/onemkl/developer-reference-c/
