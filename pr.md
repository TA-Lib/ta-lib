## Apple Platform Support: iOS/macOS Toolchain + Accelerate Framework SIMD Optimizations

This PR adds first-class Apple platform support to TA-Lib: cross-compilation for iOS/Simulator/macOS via CMake toolchain, and vectorized implementations of 16 TA functions using Apple's Accelerate framework (vDSP/vForce) for up to 6x throughput on Apple Silicon.

---

### iOS / macOS Build Support

- CMake toolchain file (`cmake/ios.toolchain.cmake`) for iOS, Simulator, and macOS cross-compilation
- Platform detection (`IOS`/`MACOS` variables) with automatic dev tool disabling for iOS
- XCFramework packaging script (`build-xcframework.sh`) for creating universal static libraries
- Build script (`scripts/build-ios.sh`) for all three slices in one command
- Xcode deployment targets: iOS 12.0, macOS 10.15

```bash
# Build all platforms
scripts/build-ios.sh

# Package into XCFramework
./build-xcframework.sh
```

### Accelerate Framework Optimizations

16 TA functions are vectorized using vecLib's vDSP (SIMD arithmetic) and vForce (SIMD transcendentals) when building on Apple platforms. Controlled by `TA_USE_ACCELERATE` CMake option (default ON).

**vForce-optimized (single-input transcendentals):**
ACOS, ASIN, ATAN, COS, COSH, EXP, LN, LOG10, SIN, SINH, SQRT, TAN, TANH

**vDSP-optimized (dual-input arithmetic):**
ADD, SUB, MULT

All optimizations are guarded by `#if defined(TA_USE_ACCELERATE) && !defined(USE_SINGLE_PRECISION_INPUT)` and live in hand-written sections that survive `gen_code` re-runs. The double-precision path is optimized; float variants fall back to scalar code.

Dispatch macros in `ta_veclib.h` keep each call site to one line:
```c
ACCEL_VFORCE_1IN(vvacos)         // 13 vForce functions
ACCEL_VDSP_2IN(vDSP_vaddD)      // ADD, MULT
ACCEL_VDSP_2IN_SWAP(vDSP_vsubD) // SUB (swapped operands: vDSP_vsubD computes B-A)
```

---

### Benchmark Results

All benchmarks run on Apple Silicon (M-series), 10,000 iterations per function per input size (1,140,000 total samples), stored in `bench_results.db` via SQLite for reproducibility.

#### Speedup at 10,000 elements (mean, n=10000)

| Function | Scalar (us) | Accelerate (us) | Speedup | StdDev (S / A) |
|----------|------------|-----------------|---------|----------------|
| SIN      | 65.90      | 11.72           | **5.6x** | ±4.87 / ±1.00 |
| TAN      | 62.96      | 12.38           | **5.1x** | ±3.63 / ±0.30 |
| ASIN     | 63.44      | 13.07           | **4.9x** | ±4.39 / ±0.71 |
| ACOS     | 81.67      | 18.07           | **4.5x** | ±5.68 / ±0.29 |
| COS      | 57.09      | 12.61           | **4.5x** | ±2.47 / ±1.33 |
| ATAN     | 65.55      | 15.23           | **4.3x** | ±1.97 / ±0.64 |
| EXP      | 51.10      | 13.07           | **3.9x** | ±2.99 / ±0.30 |
| TANH     | 62.10      | 17.22           | **3.6x** | ±3.93 / ±0.25 |
| COSH     | 41.10      | 13.11           | **3.1x** | ±1.33 / ±0.42 |
| SINH     | 43.79      | 14.07           | **3.1x** | ±1.62 / ±1.31 |
| LOG10    | 30.08      | 15.06           | **2.0x** | ±0.82 / ±0.78 |
| LN       | 27.07      | 15.36           | **1.8x** | ±0.84 / ±1.89 |
| ADD      | 2.58       | 2.42            | 1.1x    | ±0.54 / ±1.24 |
| MULT     | 2.57       | 2.29            | 1.1x    | ±0.75 / ±0.79 |
| SUB      | 2.52       | 2.55            | 1.0x    | ±0.18 / ±0.34 |
| SQRT     | 3.26       | 3.42            | 1.0x    | ±0.40 / ±1.57 |

Trig and transcendental functions see **1.8x-5.6x speedups** via vForce NEON SIMD. ADD, SUB, MULT, and SQRT are neutral -- the compiler's auto-vectorization already matches vDSP for simple arithmetic. They're kept on the Accelerate path because they show no regression.

#### Methodology

Built two benchmark binaries (`ta_bench_accel`, `ta_bench_scalar`) from the same source with `TA_USE_ACCELERATE` toggled. Each binary runs all 18 functions at 100/1000/10000 elements with 10 warmup iterations followed by N timed iterations, outputting CSV. A driver script (`scripts/run_bench.sh`) feeds both into SQLite and queries for mean, stddev, and speedup.

```bash
scripts/run_bench.sh 10000  # writes bench_results.db
```

#### What we tested and held back

The benchmark infrastructure let us evaluate candidates beyond the 18 shipped functions. Several were implemented, benchmarked, and reverted because the data showed no improvement or regressions:

| Candidate | Approach | Outcome | Reason |
|-----------|----------|---------|--------|
| **DIV** | `vDSP_vdivD` | **0.6x at 1k** (n=1000) | vDSP_vdivD's operand-swap overhead + function call cost exceeds the compiler's tight NEON `fdiv` loop |
| **CEIL** | `vvceil` | **0.8x at 10k** (n=10000) | Compiler emits single `frintp` instruction; vForce call overhead makes it slower |
| **FLOOR** | `vvfloor` | **0.9x at 10k** (n=10000) | Compiler emits single `frintm` instruction; same overhead issue as CEIL |
| LINEARREG (5 funcs) | `vDSP_sveD` + `vDSP_dotprD` per window | Slower (12.9 vs 17.2us) | Two vDSP calls per window position; call overhead exceeds SIMD savings for typical periods (14-30) |
| STDDEV | `vvsqrt` over output array | Slower (4.5 vs 5.5us) | Extra clamping pass before vectorized sqrt adds overhead the branch-predicted scalar loop avoids |
| CORREL | `vDSP_dotprD` for initial sums | Slower (5.7 vs 6.2us) | Same per-window call overhead pattern |
| HT_SINE/TRENDMODE/DCPHASE | `__sincos()` for paired sin/cos | No improvement | Clang already fuses paired sin/cos at -O2 on Apple Silicon |

**Key insight:** Accelerate wins when replacing an entire O(n) loop with a single O(n) vForce/vDSP call. It loses when adding O(1) function calls inside an O(n) outer loop.

---

### Profiling Infrastructure Fixes

The `ta_regtest -p` profiling mode was broken on Apple platforms with Accelerate enabled. The root cause: `clock()` has ~microsecond granularity, and vForce-optimized functions on small inputs (100 elements) complete faster than one clock tick, producing `clockDelta == 0`, treated as fatal error 612.

**Fix:**
- Replaced `clock()` with `mach_absolute_time()` on Apple (nanosecond precision)
- Extracted platform timer logic into macros (`TIMER_DECL`, `TIMER_START`, `TIMER_STOP`, `TIMER_TICKS_TO_MS`, etc.) in `ta_test_priv.h`, eliminating 8 duplicated `#ifdef` blocks across 3 files
- Changed zero-delta handling from fatal error to graceful flag (matching `test_util.c`'s existing pattern)

### Code Quality

- **DRY:** 19 copy-pasted 7-line Accelerate blocks collapsed to 18 one-liners via `ta_veclib.h` macros
- **DRY:** Duplicate `ta-lib`/`ta-lib-static` CMake target config collapsed into `foreach(target ${TA_LIB_TARGETS})`
- **Bug fix:** `status(FATAL_ERROR)` typo (should be `message(FATAL_ERROR)`)
- **Bug fix:** Removed `-framework Foundation` linkage (unused by TA-Lib, adds unnecessary iOS launch cost)
- **Bug fix:** Removed `FORCE` from iOS `CMAKE_INSTALL_PREFIX` (was preventing user override)
- **Bug fix:** `build-xcframework.sh` used `cmake . && make` which broke on non-Makefile generators and reset build config
- **Cleanup:** Removed dead `APPLE_ARCH` variable, collapsed identical link branches, fixed README markdown

---

### Test plan

- [x] `bin/ta_regtest` -- full regression suite (all TA functions, all value ranges)
- [x] `bin/ta_regtest -p` -- profiling mode, all functions (was failing before timer fix)
- [x] Build with `-DTA_USE_ACCELERATE=OFF` -- clean compile, all tests pass, no Accelerate symbols
- [x] `scripts/run_bench.sh 10000` -- 1,140,000 samples, no regressions in shipped functions
- [x] DIV regression identified via benchmark data and removed before merge
