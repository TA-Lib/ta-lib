# CLAUDE.md - TA-Lib Code Generation Guide

## Architecture Overview

All indicator code is **generated** by a single generator, **`ta_codegen`**
(`ta_codegen/generator/`, Rust): it parses `ta_codegen/input/` → IR → renders
per-backend (C, Java, C#, Rust). The C backend is generated **in place** into
`src/ta_func` / `src/ta_abstract` (the shipped library); the Rust/Java/C# bindings
live under `ta_codegen/output/`. It also generates the JSON-RPC test servers, the bench
binary, `include/ta_func_unguarded.h`, the `include/ta_defs.h` FuncUnstId enum, the
shipped Java (`ta_codegen/output/java/library/.../Core.java`, `FuncUnstId.java`, `MAType.java`), and owns the
build-system source lists (CMake `LIB_SOURCES`, `src/ta_func/Makefile.am`,
`ta_func_list.txt`). It also generates the **ta-lib.org website** — one page per function
under `website/src/functions/` (from each function's `ta_codegen/input/<name>/<name>.md`)
plus a grouped `website/src/functions/index.md`, written directly into the VuePress site
source tree (`website/`) — the one generated output that lives there rather than under
`ta_codegen/output/`. (`docs/` itself now holds only hand-written dev-docs.)

> The legacy C generator `gen_code` was **removed** in the canonical cutover (v0.7.1);
> `ta_codegen` is the only generator.

**Why the C is generated in place and not symlinked** to `ta_codegen/output/c`: a whole-dir
symlink breaks autotools' per-dir libtool recursion (`make` enters the symlink's *physical*
path, so the Makefile's relative `../../libtool` fails with `Error 127`), and it would also
force a packaging dereference step. Real files in `src/` avoid both — and downstream
consumers (notably the PHP `trader` extension) glob `src/ta_func/*.c` straight out of the
released source tarball.

**Build separation (important):** the C build systems (CMake + autotools) build **only
C** — the library + the C tools (`ta_regtest`, `ta_bench`). `ta_codegen` is Rust and is
built/run with cargo via the developer script `scripts/build.py` (`ta_codegen` /
`generate` / `servers`); **CMake never invokes cargo**, so a C-only setup needs no Rust
toolchain.

The correctness baseline that all `ta_codegen` backends are verified against is the
frozen pre-cutover reference (the `reference-pre-cutover` tag, served as `ta_ref_serve`)
plus the hardcoded `ta_regtest` expected values.

See `ta_codegen/generator/CLAUDE.md` for ta_codegen internals and
`src/tools/ta_regtest/CLAUDE.md` for the test-runner spec.

### Source of Truth: ta_codegen/input/

`ta_codegen/input/` is the single source of truth for ALL generated code
(one directory per indicator).

- **YAML** = data, config, enums, IDL. Pure definitions with no logic.
  - MAType and FuncUnstId enums (`ta_codegen/input/enums.yaml`)
  - Function metadata (inputs, outputs, optional params, groups) — per-function `<name>/<name>.yaml`
  - Shared library types — RetCode, CandleSetting defaults, Compatibility — are hand-written templates the generator emits (NOT under `input/`, which is algorithms only, and not YAML); they live with the generator under `ta_codegen/generator/templates/` (e.g. `templates/rust/types.rs`, `templates/c/ta_retcode.c.template`)
- **C source files** = logic. Anything with computation.
  - Indicator implementations (`ta_codegen/input/<name>/<name>.c`)
  - Helper functions (`ta_codegen/input/helpers/`)
  - **No logic in YAML, ever.**

No hand-coded string literals for type definitions or scaffolding in the codegen.
Do not hand-edit **generated** files under `ta_codegen/output/` — they are
overwritten on the next `generate`. Note some hand-written library source now
lives under `output/` too (the Java shared types and tests under
`output/java/library/src/io/github/talib/` — `CoreBuilder`, `OutRange`,
`CandleSetting`, the `test/` suites, and `Core.java`'s scaffolding outside the
GENCODE markers); the generator preserves those and never overwrites them.

## Quick Reference Commands

```bash
# Build (from any directory in the repo; binaries land in bin/)
scripts/build.py                # C library + all C tools (CMake)
scripts/build.py ta_regtest     # Just the C test runner (CMake)
scripts/build.py ta_codegen     # Rust codegen tool (cargo)
scripts/build.py generate       # Regenerate per-function source for all backends (cargo)
scripts/build.py servers        # Generate + compile JSON-RPC language servers (cargo)

# Test
scripts/build.py test           # C reference tests only (quick)
scripts/build.py regtest        # Full pipeline: servers (cargo) + C tests + cross-language verification
scripts/build.py regtest-only   # Codegen verification only (skip C reference tests)

# ta_codegen (run from ta_codegen/generator/)
cargo run -- generate                            # Generate indicator code for all backends
cargo run -- generate --func=SMA --backend=rust  # Specific function + backend
cargo run -- generate-servers                    # Generate JSON-RPC servers
cargo run -- build                               # Compile servers into bin/
cargo run -- extract                             # Extract indicators from C source → YAML
cargo test                                       # ta_codegen's own test suite

# ta_regtest directly (from bin/)
./ta_regtest                                     # C reference tests only
./ta_regtest --codegen                           # C tests + all-language codegen verification
./ta_regtest --codegen-only                      # Codegen verification only
./ta_regtest --codegen --language=c,rust --function=RSI,SMA
```

## Cross-Language Regression Testing

`ta_regtest` is the **universal test runner** for all languages. Instead of
linking against each language's compiled code, it drives **JSON-RPC servers**
generated by `ta_codegen`:

```
ta_regtest (C)
    ↓ JSON-RPC over stdin/stdout
    ├── ta_codegen_serve_c      (C server)
    ├── ta_codegen_serve_rust   (Rust server)
    ├── TaCodegenServe.class    (Java server)
    └── TaCodegenServe          (C# server)
```

Each server exposes its language's generated indicator code, reports available
functions via `list_functions`, returns `timing_ns` with each call, and supports
`set_unstable_period` / `set_compatibility` for global state.

`codegen_pipe.c/h` handles subprocess management and JSON-RPC communication.
`test_codegen.c` has a generic callback driven by `TA_ForEachFunc` enumeration —
it covers every indicator automatically using ta_abstract function metadata,
including price inputs (OHLCV), multi-output functions (BBANDS=3, MACD=3,
STOCH=2), integer outputs (CDL* patterns), real optional params, and all 20
unstable-period functions. It produces a timing summary, cross-language
comparison table, and JSONL report.

`server_verify.c` additionally lets the hand-written ta_regtest test functions
verify each call against the language servers **bitwise** — same inputs (sent
losslessly as hex-of-IEEE-bits), same algorithm ⇒ same bits — reusing the shared
`codegen_output_hash`/`codegen_hash_compare` core with `--xlang-hash` (issue
#115; zero tolerance except a narrow Java-transcendental one). Note: it must be
registered in BOTH `CMakeLists.txt` and the autotools `Makefile.am` (the
dist-verification CI path builds with autotools — a missing entry there breaks
the nightly).
`scripts/build.py check-source-lists` verifies the two lists agree (also run
by the dev nightly regen-check job).

### `--function=CSV` Filter

The `--function` flag accepts a comma-separated list of names, substring-matched
against test group descriptions:

| Filter Value | Test Group(s) Matched |
|-------------|----------------------|
| `MATH` | MATH,VECTOR,DCPERIOD/PHASE,TRENDLINE/MODE (includes MULT) |
| `Moving Averages` | All Moving Averages (includes SMA) |
| `RSI` | RSI,CMO + STOCH,STOCHF,STOCHRSI (substring match) |
| `BBANDS` | BBANDS |
| `ADX` | ADX,ADXR,DI,DM,DX |

Without `--function`, all test groups run.

## Rust Backend

Generated Rust lives in `ta_codegen/output/rust/` — a Cargo workspace: `library/`
is the shipped `ta-lib` crate, `tools/` holds the JSON-RPC server/bench.

- TA-Lib exports a `Core` struct (`src/ta_func/types.rs`, with `RetCode`);
  indicators are methods on `Core`, one file per indicator extending it via
  `impl Core` blocks.
- The public API uses `f64` slices (`&[f64]` / `&mut [f64]`), `usize` indices,
  and `i32` optional params.
- Each indicator generates a `xxx_lookback`, a guarded `xxx` (validates params,
  pre-computes optimization values), and an `xxx_unguarded` variant (skips the
  validation prologue only). Indexing stays safe in both: the crate is
  `#![forbid(unsafe_code)]`, so violating an unguarded precondition panics —
  never undefined behavior.
- **Cross-indicator calls always use `_unguarded`** to avoid double-validation.
- Functions with extra internal params (e.g., EMA's k factor) expose them on the
  unguarded variant only; the guarded variant pre-computes them and delegates.
  If the C source defines only the guarded function, the codegen auto-generates
  the unguarded variant by stripping range checks.
- Rustdoc is generated from each function's canonical `<name>.md`
  (`backends/rust_doc.rs`), including a runnable doctest per function; crate
  docs/README/Cargo metadata come from the scaffolding in `main.rs`. Verify with
  `cargo doc --no-deps` (warning-free) and `cargo test --doc` in the crate.

## Adding or Modifying an Indicator

1. Edit the definition in `ta_codegen/input/<name>/` (C logic) and/or its YAML metadata
2. `cd ta_codegen/generator && cargo run -- generate` (optionally `--func=<NAME>`)
3. `scripts/build.py servers` to rebuild the language servers
4. `cd bin && ./ta_regtest --codegen --function=<NAME>` to verify all backends
   against the C reference
5. **Verify other languages' output is unchanged** when fixing one backend
   (`git diff` the generated files)

The `/convert-indicator` skill automates picking up and resuming this work.

## Build Configuration

### Dependencies
- CMake 3.18+
- C compiler (clang/gcc)
- Rust toolchain (`rustup`)
- For server testing: JDK (`javac` + `java`) and .NET SDK (`dotnet`)

`scripts/build.py` checks the prerequisites per target and configures CMake
automatically on first run.

## Performance Testing

```bash
# Full pipeline (builds everything, regens, tests, benchmarks)
scripts/regtest.py

# Benchmark specific indicators (trustworthy — isolated, high iterations)
cd bin && ./ta_bench --language=cref,c --function=RSI,SMA --points=100000 --iters=500

# Full benchmark (noisy — use for overview, verify outliers in isolation)
cd bin && ./ta_bench --language=cref,c --points=100000 --iters=200
```

**Gotcha:** `ta_ref_serve` is statically linked — rebuild when `libta-lib.a`
changes or benchmarks are invalid. `regtest.py` handles this automatically.

Both hand-written benches report the **spread** of their own repeated passes,
because a bare median is silent about whether the box was quiet enough for it
to mean anything — at `--iters=50` the same five functions read 0.57–0.81x, at
`--iters=200` they read 1.00x. Read the spread before the ratio. `--max-spread=N`
(percent, default 25) exits non-zero when the run is too noisy to interpret, and
`ta_bench_direct --jsonl=PATH` appends a run record for tracking over time.

`ta_bench_direct`'s ratio is `ta_bench_cg` (single TU, `-flto`) over
`libta-lib.a` (separate TUs, no LTO) — **a build-configuration difference, not
an algorithm one**, which is why binary layout alone can move it further than
the old ±10% colour band. It now colours only outside `--no-signal` (default
1.20x) and only when the row's own spread is narrower than the effect claimed.
`--reps=N` samples both arms instead of just the reference.

`ta_bench` sends `no_output:1`, so servers return timings without serialising
the output arrays — it only ever reads `timing_ns`. Without it a 100k-point run
spends ~97% of its wall clock formatting and parsing JSON nobody looks at.
Anything that needs the values (`--codegen`, `--xlang-hash`, `server_verify`)
simply omits the flag. `cref` is a frozen binary and predates it, so runs
including `cref` stay slower than C-only ones.

### The same source, six binaries

Every benchmark ratio in this tree compares two *builds*, and they are not the
same build. Measured `.text` on x86-64 gcc:

| binary | build | TU model | bytes |
|---|---|---|---|
| `libta-lib.a` | CMake Release | separate TUs, no LTO | 2,890,487 |
| `libta-lib.so` | CMake Release | separate TUs, PIC | 2,870,694 |
| `ta_codegen_serve_c` | `gcc -O3 -flto` | single TU + ta_abstract | 3,940,182 |
| `ta_bench_stream` | `gcc -O3 -flto` | single TU + streaming | 1,955,238 |
| `ta_bench_cg` | `gcc -O3 -flto` | single TU, indicators only | 1,021,939 |
| autotools `libta-lib` | libtool | separate TUs, no LTO | not built here |

3.9x between the extremes, from identical source. The generator's flags live in
one place (`COMMON_GCC_FLAGS`, `main.rs`); `-ffp-contract=off` is set by all
three build systems and is load-bearing for the FMA contract (PR #96), not a
performance knob.

Which tool measures which:

- `ta_bench_direct` — C-ref column is `libta-lib.a`, C column is `ta_bench_cg`.
  Its ratio is therefore rows 1 vs 5 above.
- `ta_bench --language=c` — `ta_codegen_serve_c` (row 3), *not* `ta_bench_cg`.
- `ta_bench --language=cref` — `ta_ref_serve`, the frozen pre-cutover source.
  Different code, not just a different build; the only cross-*version* number.
- `ta_bench_stream` — itself, both arms, which is why its speedup column is the
  one ratio here that isn't cross-configuration.

Consequences worth internalising before quoting any number: a function's ns from
`ta_bench_direct` and from `ta_bench --language=c` are not comparable; a ratio
near 1.0 in `ta_bench_direct` means single-TU + LTO bought nothing for that
function, not that the two are the same code path; and layout alone moves these
ratios further than the old ±10% colour band allowed, which is why the band is
now 1.20x and spread-gated.

### Streaming vs batch

`ta_bench_stream` answers the question streaming has to justify itself on: is
`TA_S_<N>_Update` actually cheaper than recomputing the last bar with the batch
call? Its `speedup` column is `batch_last_ns / update_ns` — above 1 means
streaming wins. Both halves are measured in one TU, one input, one layout, so
unlike `ta_bench_direct`'s ratio it is not comparing two build configurations.

```bash
cd bin && ./ta_bench_stream --points=20000 --iters=50
./ta_bench_stream --points=20000 --iters=50 --min-ratio=0.35   # exits 1 if any func is below
```

Current shape (168 functions): median ~1.6x, but **~25 stream slower than
batch** and another ~50 sit under 1.5x. Recursive/multi-stage state wins big
(`HT_TRENDLINE` ~24x, `TRIX`/`TEMA` ~16x); window-recomputers and stateless
patterns lose (`AVGDEV`, `MAVP`, `MIDPRICE`, `WILLR`, CDL*) because the handle
buys nothing and costs indirection. Those losers overlap the rolling-extremum
family — see the corpus note below.

`--min-ratio` is a cliff detector, not a quality bar: run to run the worst ratio
moves 0.42–0.50 and the worst function's *name* changes, so a threshold near 1.0
just flaps. 0.35 has headroom while still failing on a real regression.

### Benchmark input corpus

Some indicators have input-dependent cost, so which series you measure on is
part of the measurement. `src/tools/ta_bench/bench_corpus.h` holds the corpus —
one deterministic generator, shared by `ta_bench`, `ta_bench_direct` and the
generated `ta_bench_cg` / `ta_bench_stream`. Select a class with `--shape=`:

```bash
cd bin && ./ta_bench --list-shapes            # the input classes and what each reaches

# random walk (default: the historical seed-42 series) and GBM — the acceptance gate
./ta_bench --language=cref,c --function=WILLR --shape=randwalk --iters=500
./ta_bench --language=cref,c --function=WILLR --shape=gbm      --iters=500

# alternating trend/chop legs — the class rolling min/max degrades on
for s in trend-chop-0.5p trend-chop-1p trend-chop-2p trend-chop-4p; do
  ./ta_bench --language=cref,c --function=WILLR --shape=$s --period=30 --iters=500
done
```

The rolling min/max behind MIN, MAX, MINMAX, MIDPOINT, MIDPRICE, WILLR, STOCH
and STOCHF caches the window extremum and rescans the window when that extremum
is the bar dropping out of it, so its cost depends on how often that happens. On
a zero-drift walk the rate decays as ~1/sqrt(period); on a trending leg it is
set by the drift/noise ratio instead and barely moves with the period, so the
two separate further the longer the window (1.1x the rescan rate at period 14,
3x at period 200). `randwalk` alone cannot see that — issue #147.

The tail shapes are not peers: `constant` is the worst case at `2*(period-1)`
comparisons per bar, exactly twice `mono-up`/`mono-down`. Flat input pins both
extrema because the rescan compares with strict `>`/`<` and leaves the cached
index on `trailingIdx`, so the `>=`/`<=` fast-path arms never run; a monotone
ramp pins only one of the two.

`--shape` is opt-in and `randwalk` reproduces the pre-corpus series bit for bit,
so a default run costs and measures exactly what it did before. `--seed` picks
the stream; `--regime-period` the window the trend/chop regime length is relative
to (defaults to `--period` when given, else 14); `--trend-strength` the trend-leg
drift in per-bar standard deviations (default 0.5 — sweep it to see how the cost
responds to trend/noise). `--verify-corpus` checks every shape is reproducible
and produces valid OHLC, at the `--points` you pass it.

`--list-shapes` groups the classes by what they are for, and the grouping
matters. The rescan rate depends only on the *rank order* of the bars, so
`randwalk-lo`, `randwalk-hi` and `gbm` cannot move it however much they change
the magnitudes — measured within 1% of `randwalk` at period 14/30/200. They are
controls, useful for numerical-conditioning questions (deadbands, cancellation,
ratio-based indicators), not stressors. Only `trend-chop-*` varies the rescan
rate; `mono-*` and `constant` are the analytic tail.

One documented exemption in `--verify-corpus`: the walk family floors `low` at
1.0 but leaves `close` unclamped, so `low <= min(open,close)` fails on 32 bars of
`randwalk` at n=100000 (11 with a negative close). That is inherited from the
pre-corpus generator and is preserved deliberately — clamping `close` would break
the byte-for-byte reproduction of the historical seed-42 series, which matters
more on a timing-only corpus. Every other predicate holds for every shape.

The corpus is timing-only — it is never hashed and is unrelated to
`fuzz_data.h`, whose `FUZZ_*` shape list is iterated by `--fuzz-064` /
`--xlang-hash`. Keep it that way: adding a shape there changes what those gates
compare (see the note at `test_variants.c:148`).

## Project Structure

```
ta-lib/
├── bin/                      # Built executables (ta_regtest, ta_bench, ta_codegen, servers)
├── cmake-build/              # CMake build directory
├── ta_codegen/input/             # SOURCE OF TRUTH: per-indicator C logic + YAML metadata
│   ├── <name>/<name>.c       # Indicator logic
│   ├── helpers/              # Shared helper functions
│   └── types/                # Enums, RetCode, CandleSettings, etc. (YAML)
├── ta_codegen/output/        # Generated per-language products, each split library/ (shipped) + tools/ (server/bench)
│   ├── c/tools/              # C server + bench (the C library ships from src/ — the backcompat exception)
│   ├── rust/{library,tools}/ # library/ = ta-lib crate; tools/ = server/bench (a Cargo workspace)
│   ├── java/{library,tools}/ # library/ = shipped io.github.talib package + generated metadata registry; tools/ = JSON-RPC server
│   └── csharp/{library,tools}/ # library/ = shipped TALib package (src/ generated, scaffolding hand-written); tools/ = managed JSON-RPC server
├── ta_codegen/generator/         # The Rust code generator (see its CLAUDE.md)
├── src/
│   ├── ta_func/              # The shipped C library, generated in place by ta_codegen
│   └── tools/
│       └── ta_regtest/       # Universal test runner (see its CLAUDE.md)
└── scripts/                  # build.py, regtest.py, sync.py, package.py, ...
```
