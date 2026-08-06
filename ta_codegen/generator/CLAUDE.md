# ta_codegen — TA-Lib Code Generation Tool

## What This Is

`ta_codegen` is the Rust-based code generator that replaces the old `gen_code.c` pipeline for indicator code generation. It reads YAML function definitions extracted from the C source, produces language-specific indicator implementations, and generates JSON-RPC servers for cross-language regression testing.

## Architecture

```
ta_codegen/input/                (per-indicator .c logic + YAML metadata)
       ↓
    parser                   (YAML metadata → raw serde structs → IR;
                              .c source → IR Statement/Expr directly, no raw stage)
       ↓
    ir                       (FuncDef + Statement/Expr intermediate representation)
       ↓
  ┌────┴─────────────┐
backends            server_gen / bench_gen
  ↓                      ↓
c.rs, rust_lang.rs,   JSON-RPC servers, bench binary,
java.rs               src/ta_func/ta_func_stream_private.h
ta_abstract_c.rs
  ↓
src/ta_func/*.c          (C indicator code — generated IN PLACE)
src/ta_abstract/         (ta_abstract introspection layer — generated IN PLACE)
ta_codegen/output/       (per-language products: library/ (shipped) + tools/ (server/bench))
  c/tools/               (server + bench + aggregation TUs; C library ships from src/)
  rust/library/ + rust/tools/  (ta-lib crate + server/bench — a Cargo workspace)
  java/library/ + java/tools/ + java/fragments/  (shipped Maven project + JSON-RPC
                               server + per-function method bodies: an intermediate
                               that ships nowhere, re-rendered into Core.java from
                               the IR and inlined into the server from disk)
  csharp/library/ + csharp/tools/  (shipped TALib package incl. src/metadata/ + managed JSON-RPC server)
include/ta_func.h        (generated public header)
```

**Hand-written library templates** (not indicator algorithms, not generated) live under
`ta_codegen/generator/templates/` — the generator's own assets, kept out of `input/`
(which holds only the indicator definitions) and out of `output/` (100% generated):
- `templates/rust/types.rs` — the `Core` / `RetCode` / `CoreBuilder` / `CandleSettings`
  scaffolding, copied verbatim into the Rust crate (`output/rust/library/src/ta_func/types.rs`).
- `templates/rust/scratch_election.rs` — the value gate for the scratch-buffer
  election (issue #146), copied verbatim and declared `#[cfg(test)]` in the
  generated `mod.rs`, so it never ships in a release build. Run by
  `cargo test --lib -p ta-lib`.

Both Rust templates are listed in `main.rs`'s `RUST_TEMPLATE_MODULES` (and the
test-only ones in `RUST_TEST_ONLY_MODULES`) and in `RustBackend::clean_keep`, so
`generate` copies them in and never deletes them. Adding another one means
touching all three.
- `templates/c/ta_retcode.c.template` — spliced with `src/ta_common/ta_retcode.csv`
  (`backends/retcode.rs`) → `src/ta_common/ta_retcode.c`.
- `templates/c/ta_abstract_serve.c` — hand-written abstract-serve handlers `#include`d
  into the C JSON-RPC server (added to the server compile's `-I` path).

### Key Modules

| Module | Purpose |
|--------|---------|
| `parser` | Parses YAML metadata (via raw serde structs) into `FuncDef`; parses `.c` source directly into IR `Statement`/`Expr` (no intermediate raw-struct stage for the logic) |
| `ir` | Intermediate representation (`FuncDef`, `ParamType`, `Statement`, `Expr`, etc.) |
| `extractor` | Extracts indicator definitions from C source files → YAML |
| `backends/c.rs` | Generates C indicator implementations (guarded `TA_<N>` / `TA_S_<N>`, plus `TA_<N>_Private` where declared) |
| `backends/rust_lang.rs` | Generates Rust indicator implementations (concrete `f64`, guarded entry point plus `_private` where declared) |
| `backends/rust_doc.rs` | Renders each function's canonical `<name>.md` as rustdoc on the generated Rust methods (summary/formula/notes, `# Arguments` with YAML numbers injected, `# Errors`/`# Panics`, a runnable doctest, `#[doc(alias)]`, intra-doc `# See also` links) |
| `backends/java.rs` | Generates Java Core class methods |
| `backends/csharp.rs` | Generates the shipped C# indicators — one `Core_<NAME>.cs` (`public partial class Core`) per function; XML docs via `csharp_doc.rs`, condition folding shared with Java via `compat_fold.rs` |
| `backends/csharp_metadata.rs` | Generates the shipped C# introspection registry (`TALib.Metadata` under `csharp/library/src/metadata/`): the vocabularies, the model records, `FunctionCall`, and one factory per function carrying its two dispatch thunks. The C# JSON-RPC server answers the `ta_abstract` RPCs out of *this* registry — its csproj compiles the library sources — so `test_abstract.c` proves the shipped artifact, not a test-only copy |
| `backends/abstract_rows.rs` | **The backend-neutral `ta_abstract` row model.** One derivation of every function's metadata (flags, price bundling, parameter domains, unstable-period id), rendered by `rust_abstract`, `java_abstract` (server table), `java_metadata` (shipped registry) and `csharp_metadata`. Sum-typed `OptDomain` + closed `Group`/`InputKind`/`OutputKind` enums, so a renderer cannot silently mis-tag a domain. **C and `func_api_xml` deliberately do NOT render from it** — see below |
| `backends/ta_abstract_c.rs` | Generates `ta_abstract` introspection layer (tables, frames, group index, runtime API) |
| `backends/price_bundle.rs` | Folds the expanded price components back into the single `TA_Input_Price` descriptor (`inPriceHLC` + flags). Shared by the C, Rust and Java abstract backends — that name and flags word are **public ABI** (wrappers read them; ta-lib-python renders them as `{'prices': [...]}`), so they are derived once, from the YAML declaration carried on each `Input` as a `PriceRef`, never re-inferred from argument names |
| `backends/func_api_xml.rs` | Generates `ta_func_api.xml` metadata |
| `backends/docs_site.rs` | Generates the ta-lib.org website (`website/src/functions/<name>.md` + `index.md`) from each function's `ta_codegen/input/<name>/<name>.md` — written directly into the VuePress site source tree (`website/`), not `ta_codegen/output/` |
| `server_gen` | Generates JSON-RPC server wrappers + `src/ta_func/ta_func_stream_private.h` |
| `bench_gen` | Generates direct-call benchmark binary |
| `registry` | Function registry for tracking available indicators |

## Commands

```bash
# Runnable from ANY directory: the built binary locates the repo via its own
# path (the `ta_codegen/input/` marker). Override with `TA_CODEGEN_ROOT=/path/to/ta-lib`.
# `cargo run` from ta_codegen/generator/ works as before.
cargo run -- generate                        # Generate indicator code for all backends
cargo run -- generate --func=SMA,RSI         # Generate specific functions
cargo run -- generate --backend=rust         # Generate for specific backend

cargo run -- generate-servers                # Generate JSON-RPC servers for all languages
cargo run -- generate-servers --backend=c    # Generate server for specific language

cargo run -- build                           # Compile generated servers into executables
cargo run -- build --backend=c,java          # Build specific servers

cargo run -- extract                         # Extract all indicators from C source → YAML
cargo run -- extract --function=EMA          # Extract specific indicator
```

## Testing

```bash
cd ta_codegen/generator && cargo test            # Run all 445+ tests
cd ta_codegen/generator && cargo clippy          # Strict pedantic lints enabled
```

Tests are in `tests/backend_suite.rs` and `tests/integration_test.rs` — they verify IR-to-backend rendering, expression types, function signatures, and function variants across all backends.

Value gates that need the *generated* library live in the crate itself, as
`#[cfg(test)]` modules copied from `templates/rust/` (see
`RUST_TEMPLATE_MODULES`); run them with `cargo test --lib -p ta-lib` in
`ta_codegen/output/rust/`.

## Cross-Language Testing Architecture

**The big picture**: `ta_regtest` is the universal test runner. It should test ALL languages, not just C.

### Current State

**Fully working:**
- `codegen_pipe.c/h` in ta_regtest — complete subprocess pipe abstraction (fork, exec, stdin/stdout JSON-RPC)
- `test_codegen.c/h` in ta_regtest — full orchestration: multi-language loop, JSON helpers, `doRangeTest` integration, epsilon comparison (`1e-6`), language/function filters
- Server generation for all 4 languages (C, Java, C#, Rust)
- `ta_codegen build` compiles servers into executables in `bin/`

**What's working end-to-end:**
- Generic callback in `test_codegen.c` auto-generates JSON-RPC requests from ta_abstract metadata for every indicator
- `list_functions` implemented — servers report available indicators with parameter metadata
- `timing_ns` returned with each response — ta_regtest collects and prints a timing summary
- `set_unstable_period` and `set_compatibility` implemented for all 20 unstable-period functions

### How It Works

1. `ta_codegen generate-servers` produces a JSON-RPC server per language
2. `ta_codegen build` compiles them into executables in `bin/`
3. Each server reads JSON-RPC from stdin, dispatches to compiled indicators, writes responses to stdout
4. `ta_regtest` spawns each server as a subprocess via `codegen_pipe`
5. For each indicator, ta_regtest calls the C reference AND sends the same call to the server
6. `compare_codegen_output()` validates retCode, outBegIdx, outNbElement, and output values match

### What This Replaced

- **Rust FFI layer** (`rust/ffi/`) — legacy `extern "C"` wrappers letting C call Rust directly. Deleted in favor of server architecture.
- **Hand-written Rust test files** (`rust/tests/mult_test.rs`, `sma_test.rs`, `rsi_test.rs`) — legacy from manual porting phase. Deleted; all indicator testing goes through ta_regtest.
- **`ta_regtest_rust` CMake target** — linked ta_regtest against Rust staticlib. Deleted; replaced by server-based approach.

### Server Protocol

JSON-RPC over stdin/stdout.

**Request format:**
```json
{"method": "TA_SMA", "params": {"startIdx": 0, "endIdx": 251, "optInTimePeriod": 30, "inReal": [...]}}
```

**Input types vary by function:**
- `inReal` / `inReal0` / `inReal1` — for functions with `TA_Input_Real` params
- `inHigh`, `inLow`, `inClose`, etc. — for functions with `TA_Input_Price` params (STOCH, BBANDS, ADX, etc.)
- The server must handle both styles based on the function's signature

**Response format:**
```json
{"retCode": 0, "outBegIdx": 14, "outNBElement": 237, "outReal": [...], "timing_ns": 1842}
```

**Multi-output functions** (STOCH, BBANDS, MACD, etc.) return multiple arrays:
```json
{"retCode": 0, "outBegIdx": 14, "outNBElement": 50, "outReal": [...], "outReal1": [...], "outReal2": [...]}
```

**Integer output functions** (CDL* candlestick patterns, MINMAXINDEX) return:
```json
{"retCode": 0, "outBegIdx": 14, "outNBElement": 50, "outInteger": [...]}
```

**Server protocol is complete:**
- `list_functions` — servers report available indicators with parameter metadata
- `set_unstable_period` / `set_compatibility` — global state management implemented
- `timing_ns` — execution timing returned with every response
- All 20 unstable-period functions mapped in `func_unst_id()`
- Real-valued optional params use `json_find_double` (e.g., BBANDS `optInNbDevUp`, SAR `optInAcceleration`)
- Price input support (OHLCV arrays) for STOCH, BBANDS, ADX, MACD, etc.
- Multi-output support (BBANDS=3, MACD=3, STOCH=2) with `outReal`, `outReal1`, `outReal2`
- Integer output support (CDL* patterns, MINMAXINDEX) with `outInteger`

## The abstract layer: one row model, and one independent oracle

Four surfaces describe the same metadata. Three of them — Rust's `abstract_api`,
the Java server's inline table plus the shipped `io.github.talib.metadata`, and
the shipped C# `TALib.Metadata` — render `backends/abstract_rows.rs`. **C's
`ta_abstract_c` does not, and that is the point.**

`test_abstract.c` proves each language server's metadata against the C library's
`ta_abstract` at run time. That gate is only worth something while C's values
come from somewhere else: fold C into the shared rows and it becomes a generator
compared against itself, unable to fail on a wrong row. So C keeps its own
derivation (its own `get_precision`, its own fallbacks, and
`match_predefined_opt_input`'s dedup against hand-curated `TA_DEF_UI_*`
descriptors), and stays the oracle. `func_api_xml` is left out for the same
reason plus a different projection (display labels, legacy ordering).

That is stronger than it first looks. `match_predefined_opt_input` keys **only**
on `(param name, range min, range max, default)`, so for the ~80 of 122 opt slots
that match a predefined descriptor, C's `displayName`, `hint`, `precision` and
`suggested` triple are hand-written literals in `ta_abstract_c.rs` — not derived
from the YAML the other three read. Editing SMA's `display_name` or its
`suggested` triple moves three backends and not C, and the gate fails. What it
still cannot see is a wrong `default:` or a wrong function-level `hint:`, where
every derivation moves together.

Two pieces C *does* share are shared deliberately, because an independent gate
already covers them: `price_bundle` (its own unit tests) and the flag bit values
(`flag_sync` pins them against `include/ta_abstract.h`).

Per-backend derived *names* are never fields on a row. A Java or C# method name
is computed by the backend, with the very helper it emits the signature with, so
the registry cannot name a method that does not exist. The row keeps
`camel_case: Option<String>` unresolved for the same reason: the helpers
distinguish `None` from `Some`, and a pre-resolved `camelCaseName` would turn
`HT_DCPERIOD` into `hT_DCPERIOD` instead of `htDcperiod`.

## Rust Backend Details

### Concrete `f64` API (no generics)

Generated Rust indicators are methods on the `Core` struct using concrete
`f64` slices (`&[f64]` / `&mut [f64]`), `usize` indices, and `i32` optional
params. There is **no** generic `<T: TaFloat>` system and no `f32`/`_s`
variants — an earlier sealed-trait generics experiment was removed; the backend
is concrete-`f64` only.

### Function Variants Per Indicator

| Variant | Purpose |
|---------|---------|
| `fn xxx_lookback(...) -> usize` | Lookback (first valid output index) |
| `fn xxx(...)` | Guarded public API: validates params, pre-computes optimization values, delegates |
| `fn xxx_private(...)` | Only where the definition declares one (EMA). Extra pre-computed params (EMA's `k`), no validation prologue — its only caller is the guarded body above it |

Cross-indicator calls target the **guarded** entry point, which carries the
bounds-assert preamble; that preamble takes an empty-range escape so a call
computing nothing cannot panic.

### Documentation (rustdoc)

`backends/rust_doc.rs` renders the canonical `ta_codegen/input/<name>/<name>.md`
(parsed into `DocDef` by `parser/doc_md.rs`, attached as `FuncDef.doc`) as rustdoc
on all three variants, and every guarded function gets a **generated runnable
doctest** (252 bars of deterministic synthetic data, all params at defaults,
asserts `Success`). Crate-level docs, Cargo.toml package metadata, and the crate
README.md are emitted by `generate_rust_crate_scaffolding` in `main.rs`. Verify
with `cargo doc --no-deps` (must be warning-free — prose escaping of `[`/`<` is
load-bearing) and `cargo test --doc` in `ta_codegen/output/rust/`.

### Scratch-buffer election (issue #146)

Several C bodies elect one of their own *output* buffers as the scratch the
calculation runs in — `BBANDS` opens with `tempBuffer1 = outRealMiddleBand;` so it
needs no allocation at all. In C that is a pointer assignment. Rust has no pointer
to assign, so a naive rendering emits `outRealMiddleBand.to_vec()`: an allocation
plus a copy of bytes that are overwritten before they are read, sized by the
*caller's slice* rather than by the data range.

`backends::rust_lang::ScratchElection` (a Rust-only pass, applied to both batch
bodies at the top of `gen_impl_block`) restores C's shape by renaming the local to
the elected output. It leans on Rust's own aliasing rules — `&[T]` and `&mut [T]`
parameters can never overlap, and neither can two `&mut [T]` — which is also what
makes C's aliasing arms, its input-alias guard and its copy-back statically dead
here.

The rule is stated over the IR, and nothing in the pass names a function, a buffer
or an MA type: match an `if`/`else if`/…/`else` chain whose *every* condition is an
input↔output pointer equality and whose *every* arm is only `scratch = someOutput;`
elections; take the terminal `else`'s mapping; delete the chain and rename through
the rest of the enclosing block; drop any guard that became a self-comparison.
Clause 4 is load-bearing rather than cosmetic — left in place, `BBANDS`' copy-back
would read and write the same `&mut` slice in one statement, which is E0502.

Being general is not the same as being greedy. Requiring *every* arm to be an
election is what declines `STOCH`, `STOCHF` and `MAVP`: each mixes an allocation
and a `…IsAllocated = 1;` flag into a branch, which is a genuine in-place defence,
not an election (`MAVP` is inverted too — allocation in the `then`, election in the
`else`). Tolerating one allocating arm would reach them; that would be a widening
of this rule for a later change, never a per-function case. An election also stops
at the end of the block holding it, so `BBANDS`' general MA path and both stream
paths keep their real `vec![0.0; ...]` allocations, and the pass backs off entirely
if the local is assigned again while still in scope.

`BBANDS` is currently the only function in `input/` written in this shape.
`rust_scratch_election_declines_arms_that_allocate` in `tests/backend_suite.rs`
pins that by sweeping every indicator and asserting the pass fires for `bbands`
alone.

The C, Java and C# backends need none of this — they assign the pointer or
reference directly — so the transform must never change their output. `generate`
followed by `git diff` over `src/ta_func/`, `output/java/` and `output/csharp/` is
the check. `templates/rust/scratch_election.rs` is the value gate.

### Debug-safe decrements

C's `while (i-- > 0)` idiom lets an unsigned counter wrap past zero; the Rust
backend emits `wrapping_sub(1)` for post/pre-decrement so debug builds (and
doctests) behave like the regtest-verified release builds instead of panicking
on `attempt to subtract with overflow`.

### Known Code Quality Issues (non-blocking)

1. **`collect_for_loop_vars`** doesn't recurse into nested structures

## Linting

Strict Clippy pedantic lints are enabled in `src/lib.rs`. Allowed exceptions:
- `module_name_repetitions` — common in codegen
- `must_use_candidate` — codegen builders don't need this
- `format_push_string` — string building is the natural codegen pattern
- `doc_markdown` — generated doc comments come from upstream C

`rustfmt.toml`: edition 2021, max_width 100, use_field_init_shorthand true.

## Performance: C Server Compilation

- Server is single-TU (`#include .c` files) — do NOT switch to separate compilation, it causes CDL binary layout issues
- Candle settings are hoisted once into local `int`/`double` vars at the top of each function by `emit_c_unpacking()` — plain reads from `TA_Globals->candleSettings[...]`, no `volatile` cast
- Ternary chains (not switch statements) for numeric-case switches — matches reference macro pattern for compiler optimization
- CCI uses conditional reset (`idx++; if(idx>=max) idx=0`) not modulo — modulo costs ~10 cycles on ARM
- Full parameter validation (NULL checks, INTEGER_DEFAULT, range) is required — missing validation changes compiler register allocation
- `ta_ref_serve` is statically linked against `libta-lib.a` — MUST rebuild when cmake rebuilds the library, or benchmarks compare against stale code
- `regtest.py` auto-rebuilds ta_ref_serve in the cmake step
- Benchmark noise: full-suite runs have 10-20% variance from icache pressure. Use `ta_bench --function=NAME --iters=500` for ground truth.
- All servers and bench binaries call `TA_Initialize()` at startup — required for candle settings defaults.
- Thermal canary (SMA) runs between each indicator in ta_bench to normalize CPU thermal state
