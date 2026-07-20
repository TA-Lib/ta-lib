# CLAUDE.md - TA-Lib Code Generation Guide

## Architecture Overview

All indicator code is **generated** by a single generator, **`ta_codegen`**
(`ta_codegen/generator/`, Rust): it parses `ta_codegen/input/` → IR → renders
per-backend (C, Java, .NET, Rust). The C backend is generated **in place** into
`src/ta_func` / `src/ta_abstract` (the shipped library); the Rust/Java/.NET bindings
live under `ta_codegen/output/`. It also generates the JSON-RPC test servers, the bench
binary, `include/ta_func_unguarded.h`, the `include/ta_defs.h` FuncUnstId enum, the
shipped Java (`ta_codegen/output/java/library/.../Core.java`, `CoreAnnotated.java`, `FuncUnstId.java`), and owns the
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
(161 indicator definitions).

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
lives under `output/` too (e.g. the Java `meta/` reflection layer and tests under
`output/java/library/`); the generator preserves those and never overwrites them.

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
    └── TaCodegenServe          (.NET server)
```

Each server exposes its language's generated indicator code, reports available
functions via `list_functions`, returns `timing_ns` with each call, and supports
`set_unstable_period` / `set_compatibility` for global state.

`codegen_pipe.c/h` handles subprocess management and JSON-RPC communication.
`test_codegen.c` has a generic callback driven by `TA_ForEachFunc` enumeration —
it covers all 161 indicators automatically using ta_abstract function metadata,
including price inputs (OHLCV), multi-output functions (BBANDS=3, MACD=3,
STOCH=2), integer outputs (CDL* patterns), real optional params, and all 24
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
  pre-computes optimization values), and an `xxx_unguarded` variant (no range
  checks, `get_unchecked` indexing inside an `unsafe` block).
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
Full 161-indicator benchmark runs have 10–20% variance from icache pressure;
use `--function=NAME --iters=500` for ground truth.

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
│   ├── java/{library,tools}/ # library/ = shipped Java package + hand-written meta/; tools/ = JSON-RPC server
│   └── dotnet/tools/         # .NET P/Invoke server (tools-only; no managed library)
├── ta_codegen/generator/         # The Rust code generator (see its CLAUDE.md)
├── src/
│   ├── ta_func/              # The shipped C library, generated in place by ta_codegen
│   └── tools/
│       └── ta_regtest/       # Universal test runner (see its CLAUDE.md)
└── scripts/                  # build.py, regtest.py, sync.py, package.py, ...
```
