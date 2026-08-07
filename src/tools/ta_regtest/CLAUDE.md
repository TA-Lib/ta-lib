# ta_regtest — Universal Regression Test Runner

## What This Is

ta_regtest validates TA-Lib indicator implementations. It has two modes:

1. **C reference testing** — tests the shipped C indicator implementations directly (linked in). Uses hand-written test files (`test_ma.c`, `test_rsi.c`, etc.) with known-good expected values and `doRangeTest` range sweeps.

2. **Codegen verification** — tests the generated indicator implementations (from ta_codegen) across all languages by driving JSON-RPC servers. Compares each language's output against the C reference.

## CLI Flags

| Flag | Description |
|------|-------------|
| `--function=CSV` | Substring filter — matched against the **group tag** in `DO_TEST`, not the function name. A function absent from its group's tag is unreachable by this filter (that is why the composite group is tagged `PVO,VWMA,COMPOSITE`). |
| `--codegen` | Run codegen verification after C reference tests |
| `--language=CSV` | Filter languages for codegen verification (e.g., `c,rust,java`) |
| `-p` | Profile mode |

Examples:
```bash
./ta_regtest                                           # C reference tests only
./ta_regtest --codegen                                 # C tests + all-language codegen
./ta_regtest --codegen --language=c,rust               # Codegen for C and Rust only
./ta_regtest --codegen --function=RSI,SMA              # Filter to specific functions
```

## Key Files

| File | Purpose |
|------|---------|
| `ta_regtest.c` | Main entry point. CLI flags: `--function=CSV`, `--codegen`, `--language=CSV`, `-p` |
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
        └── TaCodegenServe          (C#)
```

### Current State

A single generic callback driven by `TA_ForEachFunc` enumeration covers every indicator automatically. The callback uses ta_abstract metadata (`TA_GetFuncInfo`, `TA_GetInputParameterInfo`, `TA_GetOptInputParameterInfo`, `TA_GetOutputParameterInfo`) to build JSON-RPC requests without any per-function hand-coding. `TA_CallFunc` executes the C reference, then the callback copies the requested `outputNb` into the range-test output buffer.

The generic `doRangeTest` sweep **compares values by default** for every
function (lesson from issue #98: the TRIX partial-range mislabeling survived
two decades because this sweep used `TA_DO_NOT_COMPARE` everywhere, checking
only coherency). EMA-derived functions (DEMA, TEMA, TRIX, MACD, MACDEXT,
MACDFIX) map to `TA_FUNC_UNST_EMA` in `UNSTABLE_MAP` so the unstable-period
mechanism absorbs their legitimate trajectory dependence. Documented
exceptions that keep `TA_DO_NOT_COMPARE` (legitimate, non-converging range
dependence): running accumulations seeded at `startIdx` (AD, ADOSC, OBV, NVI,
PVI) and path-dependent state machines (SAR, SAREXT). This set is declared at
the definition site by the `path_dependent` YAML flag and surfaced through
`ta_abstract` as `TA_FUNC_FLG_PATH_DEP`, which `get_integer_tolerance()` reads
from `funcInfo->flags` (issue #127 — the same public flag a wrapper sees, no
hand-edited second list).

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
| `TA_STABLE_SKIP` | not compared | `get_integer_tolerance() == TA_DO_NOT_COMPARE` — the `TA_FUNC_FLG_PATH_DEP`-flagged set (#127): AD, ADOSC, OBV, NVI, PVI, SAR, SAREXT |

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

20 functions have a genuine unstable period that affects output (recursive /
converging — Wilder smoothing, EMA/adaptive-EMA, Hilbert IIR). Must send
`unstablePeriod` param to servers:
ADX, ATR, CMO, DX, EMA, HT_DCPERIOD, HT_DCPHASE, HT_PHASOR, HT_SINE, HT_TRENDLINE, HT_TRENDMODE, KAMA, MAMA, MINUS_DI, MINUS_DM, NATR, PLUS_DI, PLUS_DM, RSI, T3

The `TA_FuncUnstId` enum still has 24 entries: four retired ids are kept as
`TA_FUNC_UNST_UNUSED_*` (removing one would renumber the enum → ABI break).
**IMI** (#14) and **MFI** (#4) are *not* unstable — finite sliding-window
indicators (IMI recomputes its window fresh each bar → bit-exact; MFI carries a
running accumulator → ~1e-13 drift only); they carry no `unstable_period`
abstract flag and are excluded from `UNSTABLE_MAP` so their range sweeps use the
tight `TA_FUNC_UNST_NONE` tolerance rather than the loose convergence envelope.
**ADXR** and **STOCHRSI** (#129) had ids that were never read — they follow
their internal ADX/RSI instead, so `UNSTABLE_MAP` maps them to
`TA_FUNC_UNST_ADX`/`_RSI` (the DEMA→EMA pattern), keeping the converging
range tolerance and the stream K-leg coverage.

## Abstract-metadata parity — every language server vs the C library

`ta_regtest.c` opens a dedicated pipe per language and points `test_abstract.c`
at it (`test_abstract_set_server`). There is one block per language, and **the
block is the coverage**: `test_abstract_server_metadata()` and the server legs of
`test_abstract()` both short-circuit to `TA_TEST_PASS` when no pipe is set, so a
server can implement every RPC perfectly or not at all and, without a block,
every gate stays green. The C# block was added for exactly that reason — its
RPCs had none.

Each block runs two passes: the metadata getters (`TA_GetFuncInfo` +
`TA_Get{Input,OptInput,Output}ParameterInfo`) for every function, and the
dynamic-dispatch path (`abstract_call` / `abstract_get_lookback` /
`TA_FunctionDescriptionXML`) comparing output **values**. The C server is run as
the **control arm**: it answers from the very `ta_abstract` it is compared
against, so a failure there is a comparator defect, which is what makes a failure
on Rust/Java/C# meaningful.

Three properties keep the sweep from passing vacuously:

* **A missing server is a hard failure when its language was named.**
  `--language=csharp` on a box with no .NET SDK used to print a skip and exit 0.
* **`ctx.checked == 0` fails**, and on an unfiltered run so does comparing zero
  opt-level hints or zero ranges — the counts are printed *and* asserted.
* **`if( crefOpt->dataSet )` is counted.** That branch silently skips five field
  comparisons; `g_optExtendedCompared` makes "we compared all the ranges"
  distinguishable from "we looked at none".

`abstract_for_each_func` **had no caller in any language** until
`abstract_verify_for_each_func()`. It compares the server's enumeration against
`TA_ForEachFunc` as a **set** — C walks group by group while the registries are
name-sorted, so an order-sensitive compare would fail on a correct server — and
reports a function C does not have, a duplicate, an omission, or a count
mismatch. It is the only gate that can see a function *missing from* a registry,
because the per-function getters are driven by C's own enumeration. Sabotage-
proven: deleting one entry from the Java server's handler fails with
`abstract_for_each_func omits 'WILLR'` while the metadata sweep still reports 0
failures.

**The binder parameter contract.** The dynamic legs used to bind every optional
parameter at its declared default, so all four binders were exercised at one point
in their domain. `d2_param_vectors` now drives, on `inputRandomData`, a
non-default value per slot (distinct per slot — same-default siblings would
otherwise hide a transposition), the default sentinel per slot, and both bounds
out of range per slot, for integer *and* real domains. C and the server get the
same vector, so this compares two binders rather than one against its own oracle.

Two self-checks keep it honest, mirroring `--xlang-hash`'s `oorNotRejected` /
`sentNotDefault`: an out-of-range probe C *accepts* is not out of range, and a
sentinel is asserted against the all-defaults result rather than only against the
server — otherwise both tiers could be wrong together. All four counts are printed
and asserted non-zero.

Opt-level `hint` is compared too, and it was worth adding precisely because
nothing compared it: for the ~80 opt slots whose C descriptor is a predefined
`TA_DEF_UI_*`, the hint is a hand-written literal in the generator rather than a
YAML-derived value, so this is one of the few metadata checks that is not a
generator comparing against itself.

Where this runs: **one** nightly job — dev-nightly's `xlang` step, which is the
only one invoking `regtest.py --codegen` unfiltered. The other `--codegen`
jobs narrow to `rust` or `c,rust`, and `main-nightly` runs `--xlang-hash`, which
reaches `abstract_get_lookback` and no other abstract RPC. So every gate in this
section has a single point of failure in CI. That is deliberate rather than
overlooked: a second job would buy redundancy against runner flakiness, not
against defects. Worth knowing before assuming a green `main` nightly says
anything about abstract-metadata parity.

## The VARIANT gate — TA_/TA_S_ bitwise parity, no oracle (issue #137)

Every function ships twice over: `TA_<N>` and `TA_S_<N>`. `test_variants.c`
(tag `TA_S_,VARIANT`) asserts one exact contract across them, in-process, with no
server and no oracle — so a bare `./ta_regtest` covers it, which is what the
autotools dist nightly runs:

**`TA_S_` == `TA_` on widened inputs** — PR #33's contract. Feed `TA_S_` a float
array and `TA_` those same floats widened back; outputs must match bit for bit.

Dispatch comes from the generated `ta_variant_frame.h`
(`generator/src/backends/variant_frame.rs`): two uniform thunks plus a row per
function. A **header on purpose** — no source-list entry, so the CMake/autotools
lists cannot drift.

Sabotage-proven to catch what nothing caught before: `-999.0` in **guarded**
`TA_S_ADX`, and a `1e-12` drift in a `TA_S_` body (ref diff is 1e-9, float leg
1e-6 — this gate is bitwise).

**It found a live defect on first run:** `TA_S_WMA` at `optInTimePeriod == 1` did
`memmove(..., n * sizeof(double))` out of a `const float*` — wrong bits plus a
`4n`-byte over-read, through the *public guarded* API (WMA's range is
`[1,100000]`). Same shape in `TA_S_{RSI,CMO}`. Fixed in
`ta_codegen/input/{wma,rsi,cmo}/` with a forward element loop, which the
generator widens via an explicit `(double)` in the `TA_S_` bodies and which still
handles the in-place `out == in` case from #94.

The gate prints its coverage and asserts it non-zero, so it cannot pass by
silently doing nothing. The counter that matters is `nbOutputCmp`, incremented
**at the memcmp itself** — a counter bumped before the comparisons and
independently of them lets a deleted comparison leave the summary printing
byte-identical numbers while the gate checks strictly less.

## The float leg — the same contract, in Java and C# (issue #170)

`test_variants.c` covers `TA_S_` == `TA_` in-process, which is C only. The
**float leg** (`run_float_leg`, `test_codegen.c`) is the cross-language form: it
sends one function twice to the *same* server — once normally, once with
`"use_float":1` — on float-widened inputs, and requires the two to agree. That
covers the other two float surfaces: Java's `float[]` Core overloads and C#'s,
168 functions each. Rust has no single-precision surface and is the only
exclusion. Each call must come back with `"used_float":1`; a server that ignored
the flag would return its double result twice and pass while verifying nothing.

The leg runs **two parameter vectors**. The resolved defaults, and — since #170
— the **default sentinel** (`TA_INTEGER_DEFAULT` / `TA_REAL_DEFAULT` in every
optional slot, sent to *both* halves, so the property is "each tier substitutes
the same declared default" and needs no oracle). The sentinel vector is not a
refinement: it is the one that exposed the `TA_S_EMA` k-factor defect fixed in
`2e9767397`, where the float body derived `k` from the raw sentinel because its
initialiser ran before the prologue substituted it. The same defect was live in
Java's float `emaInternal` and C#'s float `Ema`, and reaching only resolved
defaults, no gate could see it there. Sabotage-proven both ways: reintroducing
it in the Java and C# float bodies fails the sentinel pass on both, and with the
sentinel pass switched off the identical sabotage passes clean.

Not asserted: `float(sentinel) == float(default)`. A body that mishandles the
sentinel either diverges from its own double tier (the pair check) or is
rejected outright (an error response where the resolved-default request
succeeded is a hard failure, not a skip), and the double tier's own
sentinel-selects-the-default contract belongs to `--xlang-hash` (#148).

Two exclusions, both counted and printed:

* **Choice-list slots on Java.** `Core` takes a real `MAType` enum, so
  `Integer.MIN_VALUE` is unrepresentable and the generated Java server dies
  constructing one (#162). That slot alone stays at its explicit default — the
  function's other parameters still ride the sentinel, which beats skipping the
  function. `codegen_lang_can_pass_enum_sentinel` is the single definition,
  shared with `--xlang-hash`.
* **Functions with no optional parameter**, where the pass would re-send the
  request just made.

The floor is **per language** and counts comparisons *that diffed output
elements*: a total would stay green while one server answered every sentinel
request with an error, and one server silently opting out is the exact shape of
the hole this closes. `eligible` (functions that reached the pass with a
sentinel-able parameter) is what the floor tests against, so a `--function=`
filter naming only parameterless functions is a legitimate zero.

`run_float_leg` snapshots and restores everything it touches in
`CodegenRangeTestParam` — the `parse_ref_baseline` fields, `optOverride[]`, the
request-shaping flags, the timing accumulators. Before #170 it was safe only
because it happened to be the last statement of `sweep_run_variant`, and the
first attempt at a second pass produced `SWEEP GUARDED MISMATCH [TA_ACCBANDS]`
(the guarded call at the swept period against a baseline left at the default).

## Transport

`codegen_pipe_call` reads responses in 256KB chunks into a per-pipe buffer that
persists across calls (a read can overrun the newline into the next response).
It used to read one byte per `read()`; at ~2MB responses that was ~800k blocking
syscalls per benchmarked function. The buffer is heap-allocated because these
structs are `main()` locals, including a `CodegenPipe[SV_MAX_PIPES]` — inline it
would not fit Windows' 1MB default stack.

The paired server-side saving is the `no_output` request flag (see the root
CLAUDE.md): callers that only want `timing_ns` suppress the output arrays. Every
correctness path omits it and still gets the values.

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
- **Coverage:** every function × 7 data shapes × 3 seeds × 3 sizes ×
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
  NaN from an unchecked `x/0`, the fix substitutes a defined neutral value; that
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

## `--xlang-hash` — cross-language BITWISE parity gate (issue #113)

An opt-in mode (`ta_regtest --xlang-hash`) that proves each **generated language
server** computes **bit-identical** outputs to the **shipped in-process C
library**, with **zero tolerance** (the sole carve-out is the transcendental
calls of Java and C# — see below). It is the strong form of the cross-language `--codegen`
check, which can only compare at `1e-6` (`CODEGEN_EPSILON`) because its
inputs/outputs cross the JSON-RPC boundary as lossy `%.15g`. `--xlang-hash`
routes around that boundary two ways — full-precision inputs (a seed both sides
regenerate, or lossless hex-of-IEEE-bits) and outputs compared by a full-precision
FNV hash — so a ~1e-10 FMA-fusion-site divergence that `1e-6` cannot see becomes a
hard failure.

Build + run everything with `scripts/build.py xlang-hash`. Both CI nightlies
(dev + main) run it as a gate (`xlang-hash` job). Needs cmake + gcc + cargo, plus
the **JDK** for the Java server and the **.NET SDK** for the managed C# server.

Architecture (see `fuzz_data.h`, the Rust port in
`ta_codegen/generator/templates/rust/fuzz.rs`, and `xlang_hash` in
`test_codegen.c`):
- **Golden = the in-process C library.** The C library is linked into
  `ta_regtest`, so there is no JSON-RPC boundary on the C side — it is called
  directly (`TA_CallFunc`) and its raw output hashed (`fuzz_hash_local`), exactly
  as `--fuzz-064` treats the current library. Each language server crosses the
  boundary and is diffed against it: **Rust**, **Java** and the managed **C#**.
  C# rides the Java-style hex transport (no `fuzz_gen` port) and takes the same
  tolerance lane. It was briefly configured as fully bitwise on the strength of
  a green local run; dev-nightly **30776189041** then produced 25 `TA_LN`
  mismatches on `ubuntu-latest` x86-64 from a commit that was bitwise-clean on
  `ubuntu-24.04-arm` and on a glibc-2.39 + .NET-10.0.10 dev box. **.NET does not
  guarantee `Math.*` reaches the platform libm.** Not a special-value problem —
  C and C# agree bit-for-bit on `0.0`/`-0.0`/negatives including the NaN payload,
  so it is a normal-value 1 ULP difference. `Math.FusedMultiplyAdd` is still
  correctly rounded, so the FMA contract is untouched; only transcendentals
  moved. Because the constant-shape skip is gated on the tolerance lane, C# now
  inherits it for HT_DCPHASE/HT_SINE as well.
- **Two transports (per-server `usesSeed` flag).**
  - **Seed (Rust).** A request with `"gen_present":1` + `(gen_shape,gen_seed,gen_n)`
    makes the server generate the OHLCV inputs from its own bit-exact `fuzz_gen`
    port (price inputs → O/H/L/C/V/OI, generic reals → real0=close, real1=volume —
    matching the driver), run the **guarded** function, and return `"out_hash"`.
  - **Hex (Java).** Java's server has no `fuzz_gen` port (#114), so the driver
    serializes its own seed-generated arrays losslessly (hex-of-IEEE-bits, the
    `codegen_write_hexbits_array` transport shared with `server_verify`) into a
    per-function `TA_<name>` request with `want_hash`. Same guarded call, same
    `out_hash`. No server-side change was needed — this reuses the #115 machinery.
  - Both take the digest of the **guarded** call — like-for-like with the golden's
    `TA_CallFunc`.
- **Transcendental tolerance (Java and C#).** Java's fdlibm differs from the C
  libm by ~1 ULP on `atan/sin/cos/exp/log/...`; .NET's `Math.*` is not
  guaranteed to reach the platform libm and empirically does not on some hosts.
  A call that reaches a transcendental therefore cannot be bit-compared against
  either. Those calls (decided **per call** — the ~20-name set OR a `*MAType`
  == `TA_MAType_MAMA`, via the shared `codegen_call_is_transcendental`) drop the
  `want_hash` and are element-compared at `CODEGEN_TRANSCENDENTAL_TOL` (1e-9)
  by the shared `codegen_compare_tol` — the identical carve-out `server_verify`
  uses. Every non-transcendental Java call, and every Rust call, stays bitwise.
  The summary reports how many calls took the tolerance path per server (the
  rest are bitwise), so the bitwise coverage is visibly non-vacuous.
  `codegen_lang_needs_transcendental_tol` is the single definition of which
  languages need it — `--xlang-hash` copies it into `XlangServer.tolTranscendental`
  and `server_verify` reads it directly, because when the two carried the rule
  as separate literals they drifted apart.
- **Input-port self-check.** Before the output gate, a `fuzz_in_hash` RPC on each
  **seed** server hashes its generated OHLCV inputs; the driver compares against
  its own in-process generation, so a `fuzz_gen`-port bug surfaces as an INPUT
  mismatch, not a fake indicator-output bug. Hex servers (Java) send the driver's
  exact arrays, so they have no port to self-check and are skipped here.
- **Unstable-period axis (#116).** The 20 functions carrying
  `TA_FUNC_FLG_UNST_PER` run the whole sweep a second time at unstable period
  `XLANG_UNST_PERIOD` (3), with the in-process golden set through
  `TA_SetUnstablePeriod` and the servers through the per-call field. 0 runs last,
  so each function leaves the servers where the next one expects them. Only
  `FUZZ_VEC_NORMAL` vectors repeat: the reject/sentinel classes assert parameter
  *validation*, which runs before any unstable-period logic. Before this the gate
  pinned `unstablePeriod: 0` everywhere and the axis was covered **only** by the
  ref differential sweep, i.e. only by the frozen `ta_ref_serve` — the last thing
  blocking its retirement.
  - **A non-zero period cannot ride the seed transport.** `abstract_call` carries
    a `funcUnstId` that no driver has ever sent, so it reads 0
    (`TA_FUNC_UNST_ADX`): the C handler would apply the period to ADX whatever
    function was called, and the Rust handler ignores the field outright. The
    per-function `TA_<name>` handler hardcodes the right id, so the unstable legs
    force the hex transport on every server, Rust included.
  - Non-vacuity is checked per function *before* the leg runs: the lookback must
    move between unstable 0 and 3. A flat lookback means the flag is lying and
    fails the run rather than banking a leg that compares nothing. A `unstCases`
    floor catches the axis going quiet wholesale.
- **Coverage:** every function × 9 shapes × 3 seeds × 3 sizes × parameter
  vectors × 3 subranges ≈ 237k comparisons **per server** (of which ~76k at a
  non-zero unstable period), ~94% with non-empty output (a non-vacuity guard
  fails the run if nothing produced output — an empty output hashes the same on
  both sides).

Scope rules (deliberate):
- **No 0.6.4, no waivers; one tolerance and two skips.** This is
  current-vs-current across languages, so — unlike `--fuzz-064` — there are none
  of the `#98`/`#107`/FMA-transition carve-outs. Every case is bitwise except
  the transcendental calls of Java and C# (1e-9, above). A non-tolerated
  mismatch is a real fusion-site / codegen divergence to fix.
- **The second skip: the choice-list default sentinel, Java only.** Every
  optional parameter gets a `TA_*_DEFAULT` vector that must resolve to the
  declared default, `enum:MAType` included. Java's `MAType` is a real enum, so
  that value is unrepresentable there and the driver never sends it (withheld
  cases are counted and printed); C, Rust and C# type the parameter as an integer
  and are held to it. These cases carry their own count and their own non-vacuity
  floor: they are a small subset of `sentCases`, so the combined total cannot show
  that the leg has stopped running.
- **The one ill-conditioning skip: HT_DCPHASE / HT_SINE on the constant shape,
  for the tolerance-lane servers (Java and C#).** These two derive their output from `atan2` of the Hilbert
  transform's in-phase/quadrature components. On `FUZZ_CONSTANT` (flat O=H=L=C,
  zero variance) those components are floating-point noise (~0), so the phase is
  `atan2(≈0,≈0)` — chaotically sensitive to the last bit of every transcendental
  step. C and Rust share the system libm and stay **bit-identical** there (Rust:
  0 mismatches on every shape); Java's fdlibm differs by ~1 ULP and this
  ill-conditioning amplifies it to whole *degrees* (~2.9° on HT_DCPHASE); C#
  inherits the skip because the gate keys on the tolerance lane. It is
  not a codegen bug — all 8 non-degenerate shapes agree within 1e-9, and `atan2`
  of a null signal is mathematically undefined — so no fixed tolerance can
  separate it from fdlibm noise. `xlang_illcond` skips exactly these two
  functions on exactly the constant shape, for the tolerance-lane servers; the
  count is reported in the summary. Rust still gates HT_DCPHASE/HT_SINE bitwise on the constant
  shape, so the C computation itself stays covered there.
- **period == 1 is in scope** (no 0.6.4 to trip on it), though the shared
  `fuzz_build_vectors` currently floors periods at 2; period-1 parity is also
  covered by the `--codegen` edge sweeps.
- Why this is expected GREEN (PR #96): every backend fuses the identical `a*b+c`
  sites via the shared `backends/fma.rs` detector and builds with
  `-ffp-contract=off`, and `fma`/`mul_add` are IEEE correctly-rounded → bit-
  identical for equal operands (Java `Math.fma` included; only its fdlibm
  transcendentals need the tolerance).

## `server_verify` — bitwise C⇄server on the hard-coded tests (issue #115)

`server_verify.c` runs during the hand-written tests (whenever `--codegen` is
active and at least one language server started). It is the **same** "in-process
C ⇄ language server, bit-for-bit, on identical inputs" operation as `--xlang-hash`
— it just feeds the hard-coded test's exact arrays instead of a seed, and shares
the driver core in `test_codegen.c` (`codegen_output_hash` /
`codegen_hash_compare` / `codegen_hash_report`, declared in `test_codegen.h`;
`--xlang-hash`'s `fuzz_hash_local` is a thin wrapper over `codegen_output_hash`).

The hard-coded tests validate **in-process C vs the expected constants** at a
legitimate tolerance. `server_verify` runs the *transitive* check: feed the same
inputs to another language and compare to what C computed — which must be
**exact** (same algorithm + same inputs ⇒ same bits). A `1e-6` re-compare there
would be strictly weaker than "C == server, then C == expected ⇒ server ==
expected", so the old `SV_EPSILON` was deleted.

- **Lossless input.** Inputs are serialized as **hex-of-IEEE-bits** strings (one
  16-hex group per double, via `to_bits`/`from_bits` — no float-parse rounding,
  no library in any language). Every server's array parser
  (`json_find_double_array` / `parse_f64_array` / `jsonDoubleArray` /
  `GetDoubleArray`) grew a "string ⇒ decode hex, else number array" branch; every
  other caller sends a number array and is unaffected.
- **Output by hash.** The request carries `want_hash:1`; each server's
  **per-function** handler (`TA_<name>`, not `abstract_call`) returns
  `out_hash` — a full-precision FNV-1a of the raw GUARDED output bytes — which the
  shared `codegen_hash_compare` diffs against the C golden's `codegen_output_hash`.
  Java/C# gained this hasher; the C per-function handler is `#ifndef
  TA_REF_SERVE`-guarded (its `fuzz_hash_*` live in `fuzz_data.h`, absent from the
  frozen `ta_ref_serve`, which `server_verify` never drives).
- **Tolerance rule.** Zero tolerance (bitwise) for **C ⇄ Rust** (Rust reaches
  the same system libm as the golden). **Java and C#** are bitwise for
  pure-arith + IEEE ops
  (incl. SQRT/CEIL/FLOOR) but get a narrow `CODEGEN_TRANSCENDENTAL_TOL`
  (1e-9, measured drift ~1e-13..1e-11) on the transcendental-using functions only
  — Java's fdlibm ≠ the C libm by ~1 ULP. The transcendental set is decided **per
  call**, not just by name: the MA-dispatch functions (MA/MAVP/BBANDS/MACDEXT/
  APO/PPO/STOCH*) route to MAMA (which uses `atan`) when a `*MAType` parameter
  selects `TA_MAType_MAMA` (7), so that call is transcendental for Java even
  though the function name is not (integer outputs like HT_TRENDMODE still match
  exactly). Rust stays bitwise even on the transcendentals. The
  tolerance constant, the per-call `codegen_call_is_transcendental` test, the
  `codegen_write_hexbits_array` input transport, and the `codegen_compare_tol`
  element-compare all live in `test_codegen.c` (declared in `test_codegen.h`) —
  **shared verbatim with the `--xlang-hash` tolerance legs** (#113), the same
  operation on a seed instead of the hard-coded arrays. Which languages need the
  tolerance is `codegen_lang_needs_transcendental_tol` — one definition read by
  both gates.
