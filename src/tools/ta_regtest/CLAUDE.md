# ta_regtest — Universal Regression Test Runner

## What This Is

ta_regtest validates TA-Lib indicator implementations. It has two modes:

1. **C reference testing** — tests the shipped C indicator implementations directly (linked in). Uses hand-written test files (`test_ma.c`, `test_rsi.c`, etc.) with known-good expected values and `doRangeTest` range sweeps.

2. **Codegen verification** — tests the generated indicator implementations (from ta_codegen) across all languages by driving JSON-RPC servers. Compares each language's output against the C reference.

## CLI Flags

| Flag | Description |
|------|-------------|
| `--function=CSV` | Substring filter — run only matching test groups |
| `--codegen` | Run codegen verification after C reference tests |
| `--codegen-only` | Run only codegen verification, skip C reference tests |
| `--language=CSV` | Filter languages for codegen verification (e.g., `c,rust,java`) |
| `-p` | Profile mode |

Examples:
```bash
./ta_regtest                                           # C reference tests only
./ta_regtest --codegen                                 # C tests + all-language codegen
./ta_regtest --codegen-only                            # Codegen only (all languages)
./ta_regtest --codegen --language=c,rust               # Codegen for C and Rust only
./ta_regtest --codegen --function=RSI,SMA              # Filter to specific functions
```

## Key Files

| File | Purpose |
|------|---------|
| `ta_regtest.c` | Main entry point. CLI flags: `--function=CSV`, `--codegen`, `--codegen-only`, `--language=CSV`, `-p` |
| `test_codegen.c` | Codegen verification: spawns servers, sends JSON-RPC, compares results |
| `test_codegen.h` | API: `test_codegen(history, languageFilter, functionFilter)` |
| `codegen_pipe.c/h` | Subprocess pipe abstraction for JSON-RPC over stdin/stdout |
| `ta_test_priv.h` | `doRangeTest()`, `checkExpectedValue()`, `RangeTestFunction` callback type |
| `ta_test_func/test_*.c` | Per-indicator C reference tests (23+ files) |

## doRangeTest — The Core Testing Primitive

`doRangeTest()` is what makes ta_regtest thorough. It calls a `RangeTestFunction` callback hundreds of times with every possible `startIdx`/`endIdx` combination, verifying:
- Output coherency across different ranges (same data regardless of range selection)
- Lookback function consistency
- Value comparison across ranges at a tolerance set by the function's
  `TA_RangeStability` class — exact / epsilon / converging / skip (see
  "Range-test tolerance is an explicit stability class" below). `doRangeTestEx`
  takes the class explicitly; the legacy `doRangeTest` derives it.

### RangeTestFunction Callback Interface

```c
typedef TA_RetCode (*RangeTestFunction)(
    TA_Integer startIdx, TA_Integer endIdx,
    TA_Real *outputBuffer,          // Write ONE output here (per outputNb)
    TA_Integer *outputBufferInt,    // For integer outputs (candlestick patterns)
    TA_Integer *outBegIdx,
    TA_Integer *outNbElement,
    TA_Integer *lookback,           // Must set this (call TA_GetLookback or function-specific)
    void *opaqueData,               // Your context struct
    unsigned int outputNb,          // Which output to write (0, 1, 2 for multi-output)
    unsigned int *isOutputInteger   // Set to 1 if output is integer
);
```

**Critical detail for generic callback**: `TA_CallFunc` fills ALL outputs at once. The callback must allocate all output buffers, call `TA_CallFunc`, then copy the requested `outputNb` into `outputBuffer`/`outputBufferInt`. Use `TA_GetLookback` on the `TA_ParamHolder` for the lookback value.

## Codegen Verification Architecture

```
ta_regtest
  ├── ta_abstract API (enumerates functions, provides TA_CallFunc for C reference)
  │
  └── codegen_pipe → server subprocess (stdin/stdout JSON-RPC)
        ├── ta_codegen_serve_c
        ├── ta_codegen_serve_rust   (Rust)
        ├── TaCodegenServe.class    (Java)
        └── TaCodegenServe          (.NET)
```

### Current State

A single generic callback driven by `TA_ForEachFunc` enumeration covers all 161 indicators automatically. The callback uses ta_abstract metadata (`TA_GetFuncInfo`, `TA_GetInputParameterInfo`, `TA_GetOptInputParameterInfo`, `TA_GetOutputParameterInfo`) to build JSON-RPC requests without any per-function hand-coding. `TA_CallFunc` executes the C reference, then the callback copies the requested `outputNb` into the range-test output buffer.

The generic `doRangeTest` sweep **compares values by default** for all 161
functions (lesson from issue #98: the TRIX partial-range mislabeling survived
two decades because this sweep used `TA_DO_NOT_COMPARE` everywhere, checking
only coherency). EMA-derived functions (DEMA, TEMA, TRIX, MACD, MACDEXT,
MACDFIX) map to `TA_FUNC_UNST_EMA` in `UNSTABLE_MAP` so the unstable-period
mechanism absorbs their legitimate trajectory dependence. Documented
exceptions that keep `TA_DO_NOT_COMPARE` (legitimate, non-converging range
dependence): running accumulations seeded at `startIdx` (AD, OBV, ADOSC) and
path-dependent state machines (SAR, SAREXT) — see `get_integer_tolerance()`.

#### Range-test tolerance is an explicit stability class, not `unstId == NONE`

The cross-range value comparison (`dataWithinReasonableRange`) picks its tolerance
from an explicit `TA_RangeStability` class (`ta_test_priv.h`), **decoupled** from
whether a function carries an unstable-period id. This exists because the old
"`unstId == NONE ? tight : loose`" inference let a *vestigial* unstable-period flag
hand a finite-window function the loose convergence tolerance and hide a real bug
(IMI #14, MFI #4). The four classes:

| Class | Tolerance | Who |
|-------|-----------|-----|
| `TA_STABLE_EXACT` | bit-exact (`==`) | fresh-recomputed finite window (IMI, price transforms, MOM/ROC, MIN/MAX/MIDPOINT/WILLR/AROON, LINEARREG/TSF/AVGDEV, vector math) |
| `TA_STABLE_EPSILON` | `1e-9` relative | running-accumulator finite window + **default** (SMA, WMA, STDDEV, CORREL, CCI, ULTOSC, MFI, …) |
| `TA_STABLE_CONVERGING` | warm-up envelope (`0.5/temp`, ignore-first-N) | recursive/IIR — anything in `UNSTABLE_MAP` |
| `TA_STABLE_SKIP` | not compared | `get_integer_tolerance() == TA_DO_NOT_COMPARE` (AD, ADOSC, OBV, SAR, SAREXT) |

`stability_class()` (`test_codegen.c`) assigns the generic-gate class per function
(explicit `exact[]` list from a source audit, `SKIP` **derived** from
`get_integer_tolerance` so it never desyncs from the integer-output skip,
`CONVERGING` from `UNSTABLE_MAP`, else `EPSILON`). `doRangeTestEx` **guards** the
invariant: `CONVERGING` must carry an unstId; `EXACT`/`EPSILON` must not (that's the
vestigial-flag trap); `SKIP` is exempt (ADOSC legitimately sweeps an internal EMA).
The legacy `doRangeTest(unstId, integerTolerance)` is a wrapper that derives the
class (never `EXACT`, a safe superset) for the hand-written per-function tests.

After all functions run, ta_regtest prints:
- A **cross-language timing comparison table** (wall-clock ns per call, speedup vs C)
- A **CLI summary** with pass/fail counts and average timing per language
- A **JSONL rolling report** (one JSON line per function per language) written to disk, tagged with git SHA

### Input Type Complexity

Functions have different input types that affect JSON-RPC serialization:

| Input Type | Example Functions | JSON Fields |
|-----------|-------------------|-------------|
| `TA_Input_Real` (single) | SMA, RSI, EMA | `"inReal": [...]` |
| `TA_Input_Real` (two) | MULT, ADD, SUB | `"inReal0": [...], "inReal1": [...]` |
| `TA_Input_Price` | STOCH, BBANDS, ADX, MACD | `"inHigh": [...], "inLow": [...], "inClose": [...]` |

For `TA_Input_Price`, the `TA_InputParameterInfo.flags` bitmask tells you which OHLCV components are needed. Map from `TA_History`:
- `TA_IN_PRICE_OPEN` → `history->open`
- `TA_IN_PRICE_HIGH` → `history->high`
- `TA_IN_PRICE_LOW` → `history->low`
- `TA_IN_PRICE_CLOSE` → `history->close`
- `TA_IN_PRICE_VOLUME` → `history->volume`
- `TA_IN_PRICE_OPENINTEREST` → `history->openInterest`

See `test_abstract.c` (lines 415-421) for the working reference.

### Output Type Complexity

| Output Type | Example Functions | JSON Fields |
|------------|-------------------|-------------|
| Single real | SMA, RSI, EMA | `"outReal": [...]` |
| Multi real | BBANDS (3), MACD (3), STOCH (2) | `"outReal": [...], "outReal1": [...], "outReal2": [...]` |
| Integer | CDL* patterns, MINMAXINDEX | `"outInteger": [...]` |

Integer outputs use exact match comparison (or tolerance via `TA_DO_NOT_COMPARE`). Real outputs use epsilon comparison (currently `1e-6`).

### Optional Parameter Types

| Type | Example | JSON Parsing |
|------|---------|-------------|
| `TA_OptInput_IntegerRange` | `optInTimePeriod` | `json_find_int` |
| `TA_OptInput_RealRange` | BBANDS `optInNbDevUp`, SAR `optInAcceleration` | `json_find_double` |
| `TA_OptInput_IntegerList` | MA `optInMAType` | `json_find_int` |

### Unstable Period Functions

22 functions have a genuine unstable period that affects output (recursive /
converging — Wilder smoothing, EMA/adaptive-EMA, Hilbert IIR). Must send
`unstablePeriod` param to servers:
ADX, ADXR, ATR, CMO, DX, EMA, HT_DCPERIOD, HT_DCPHASE, HT_PHASOR, HT_SINE, HT_TRENDLINE, HT_TRENDMODE, KAMA, MAMA, MINUS_DI, MINUS_DM, NATR, PLUS_DI, PLUS_DM, RSI, STOCHRSI, T3

The `TA_FuncUnstId` enum still has 24 entries: **IMI** and **MFI** keep their
`TA_FUNC_UNST_*` id (removing it would renumber the enum → ABI break) but are
*not* unstable — both are finite sliding-window indicators (IMI recomputes its
window fresh each bar → bit-exact; MFI carries a running accumulator → ~1e-13
drift only). They no longer carry the `unstable_period` abstract flag and are
excluded from `UNSTABLE_MAP` so their range sweeps use the tight
`TA_FUNC_UNST_NONE` tolerance rather than the loose convergence envelope.

## Buffer Sizes

- `JSON_BUF_SIZE` = 64KB in current code
- `MAX_NB_TEST_ELEMENT` = 280 (max output elements per test)
- At 20 chars/double, one 252-element array ≈ 5KB
- Functions with OHLCV inputs need 5+ arrays in request — may need larger buffers
- `test_abstract.c` uses up to 10000 bars for profiling — not needed for range tests

## Building

```bash
# C-only (standard)
cd cmake-build && cmake .. -DCMAKE_BUILD_TYPE=Release && make ta_regtest -j4
cd ../bin && ./ta_regtest

# With codegen verification
./ta_regtest --codegen --language=c,rust --function=SMA,RSI
```

## `--fuzz-064` — bit-exact differential fuzz vs released v0.6.4

An opt-in mode (`ta_regtest --fuzz-064`, never part of default/nightly `--codegen`
runs) that proves the **current shipped library is bit-identical to the last
release, v0.6.4**, function by function. It is the reusable regression oracle a
class-A optimization (e.g. the MIDPOINT/MIDPRICE cached-index rewrite, or the
EMA-cascade lockstep tranche) is validated against: run it before and after a
change — the divergence set vs 0.6.4 must not grow.

Build + run everything with `scripts/build.py fuzz-064`. Both CI nightlies
(dev + main) run it as a gate (`fuzz-vs-0.6.4` job, C-only, `fetch-depth: 0`).

Architecture (see `fuzz_data.h` + the fuzz block in `test_codegen.c`):
- **Oracle:** `bin/ta_064_serve` — the frozen v0.6.4 `libta-lib.a` (built once in
  the `../ta-lib-064` worktree @ tag `v0.6.4`) behind the current JSON-RPC
  transport, **shadow-patched at build time** by `scripts/build_064_serve.py`
  (no committed file changes). The current library is called **in-process**;
  only 0.6.4 crosses the pipe.
- **Inputs by seed:** the request carries only `(gen_shape, gen_seed, gen_n)`;
  both ends run the identical generator in `fuzz_data.h`, so inputs are
  byte-identical by construction (no array serialization, no precision to
  reconcile). `FP_CONTRACT` is forced off so the generator can't be fused into
  an FMA on one side only.
- **Outputs by hash:** the server returns a 64-bit FNV hash of the raw output
  bytes. On any mismatch the driver re-issues that one case with
  `"full_output":1` (exact `%a` hex arrays) to pinpoint the diverging element.
- **Coverage:** all 161 functions × 7 data shapes × 3 seeds × 3 sizes ×
  parameter vectors (boundary periods, MA-type lists, real-param bounds) × 3
  subranges ≈ 118k comparisons in ~17s.

Scope rules (deliberate):
- **period == 1 is out of scope** vs 0.6.4 (it rejects / has period-1 OOB bugs);
  periods are floored at 2. period-1 is validated by the *non-0.6.4*
  comparisons instead. At period ≥ 2 there are **no waivers** — anything
  non-benign is a real bug.
- **Subset tolerance is 0.6.4-only:** functions added after 0.6.4 are skipped
  via `ta_064_serve`'s `list_functions` (never failed). Any *non*-0.6.4
  comparison must instead require an exact function-set match.
- **Benign class:** a diff where every differing element is numerically equal
  (`+0.0` vs `-0.0`, from cached-index rewrites) is reported, not failed.
- **#98 exceptions:** TRIX/NATR `startIdx > lookback` cases are skipped
  (mislabeled / wrong-close output through 0.6.4, fixed in 0.8.1), plus NATR
  cases with a zero close in the output range (old code clobbered
  `outReal[0]`). Comparing these against frozen oracles would diff the bug
  fixes themselves. The fixed behavior is validated instead by the (now
  value-comparing) range tests. (IMI and MFI no longer need an unstable-period
  carve-out here: both are reclassified as stable finite-window indicators —
  no `TA_FUNC_FLG_UNST_PER`, lookback ignores the unstable period — so the ref
  sweep never runs a u&gt;0 variant for them.)
  Reported in the summary as a `skipped:` line; everything else remains
  waiver-free at period ≥ 2.
- **#112 NaN-to-neutral (`TOL_NAN_TO`):** where 0.6.4's *successful* call emitted
  NaN from an unguarded `x/0`, the fix substitutes a defined neutral value; that
  categorical `NaN(0.6.4) → finite` divergence is tolerated by the manifest.
  IMI is the first: an all-flat window (`FUZZ_CONSTANT`/`FUZZ_TIE_HEAVY`, every
  `close == open`) made `upsum+downsum == 0` → `100*(0/0)` → NaN; the guard now
  returns 50.0. The `FUZZ_064_TOL` entry `{ "IMI", TOL_NAN_TO, 50.0 }` tolerates
  a case **only** when 0.6.4 is NaN *and* current is *exactly* 50.0 — any other
  value (incl. a still-NaN regression, caught instead by `test_imi.c`) fails. The
  exact-`==0.0` guard keeps this the *sole* IMI divergence from 0.6.4; every
  `sum > 0` bar stays bit-identical. Reported as a `manifest-tolerated:` line.
- The oracle is reopened-and-retried once if it dies (latent 0.6.4 crash) so one
  bad case can't sink the run.
