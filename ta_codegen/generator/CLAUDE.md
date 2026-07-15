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
java.rs, dotnet.rs    include/ta_func_unguarded.h
ta_abstract_c.rs
  ↓
src/ta_func/*.c          (C indicator code — generated IN PLACE)
src/ta_abstract/         (ta_abstract introspection layer — generated IN PLACE)
ta_codegen/output/       (per-language products: library/ (shipped) + tools/ (server/bench))
  c/tools/               (server + bench + aggregation TUs; C library ships from src/)
  rust/library/ + rust/tools/  (ta-lib crate + server/bench — a Cargo workspace)
  java/library/ + java/tools/  (shipped package + meta/  +  JSON-RPC server)
  dotnet/tools/          (P/Invoke server; no managed library)
include/ta_func.h        (generated public header)
```

**Hand-written library templates** (not indicator algorithms, not generated) live under
`ta_codegen/generator/templates/` — the generator's own assets, kept out of `input/`
(which holds only the 161 indicator definitions) and out of `output/` (100% generated):
- `templates/rust/types.rs` — the `Core` / `RetCode` / `CoreBuilder` / `CandleSettings`
  scaffolding, copied verbatim into the Rust crate (`output/rust/library/src/ta_func/types.rs`).
- `templates/c/ta_retcode.c.template` — spliced with `src/ta_common/ta_retcode.csv`
  (`backends/retcode.rs`) → `src/ta_common/ta_retcode.c`.
- `templates/c/ta_abstract_serve.c` — hand-written abstract-serve handlers `#include`d
  into the C JSON-RPC server (added to the server compile's `-I` path).
- `templates/c/ta_abstract_dump.c` — standalone dev tool dumping the ta_abstract API as JSON.

### Key Modules

| Module | Purpose |
|--------|---------|
| `parser` | Parses YAML metadata (via raw serde structs) into `FuncDef`; parses `.c` source directly into IR `Statement`/`Expr` (no intermediate raw-struct stage for the logic) |
| `ir` | Intermediate representation (`FuncDef`, `ParamType`, `Statement`, `Expr`, etc.) |
| `extractor` | Extracts indicator definitions from C source files → YAML |
| `backends/c.rs` | Generates C indicator implementations (guarded + unguarded variants) |
| `backends/rust_lang.rs` | Generates Rust indicator implementations (concrete `f64`, guarded + unguarded variants) |
| `backends/rust_doc.rs` | Renders each function's canonical `<name>.md` as rustdoc on the generated Rust methods (summary/formula/notes, `# Arguments` with YAML numbers injected, `# Errors`/`# Panics`, a runnable doctest, `#[doc(alias)]`, intra-doc `# See also` links) |
| `backends/java.rs` | Generates Java Core class methods |
| `backends/dotnet.rs` | Generates .NET P/Invoke wrappers |
| `backends/ta_abstract_c.rs` | Generates `ta_abstract` introspection layer (tables, frames, group index, runtime API) |
| `backends/func_api_xml.rs` | Generates `ta_func_api.xml` metadata |
| `backends/docs_site.rs` | Generates the ta-lib.org website (`website/src/functions/<name>.md` + `index.md`) from each function's `ta_codegen/input/<name>/<name>.md` — written directly into the VuePress site source tree (`website/`), not `ta_codegen/output/` |
| `server_gen` | Generates JSON-RPC server wrappers + `include/ta_func_unguarded.h` |
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

## Cross-Language Testing Architecture

**The big picture**: `ta_regtest` is the universal test runner. It should test ALL languages, not just C.

### Current State

**Fully working:**
- `codegen_pipe.c/h` in ta_regtest — complete subprocess pipe abstraction (fork, exec, stdin/stdout JSON-RPC)
- `test_codegen.c/h` in ta_regtest — full orchestration: multi-language loop, JSON helpers, `doRangeTest` integration, epsilon comparison (`1e-6`), language/function filters
- Server generation for all 4 languages (C, Java, .NET, Rust)
- `ta_codegen build` compiles servers into executables in `bin/`

**What's working end-to-end:**
- Generic callback in `test_codegen.c` auto-generates JSON-RPC requests from ta_abstract metadata for all 161 indicators
- `list_functions` implemented — servers report available indicators with parameter metadata
- `timing_ns` returned with each response — ta_regtest collects and prints a timing summary
- `set_unstable_period` and `set_compatibility` implemented for all 24 unstable-period functions

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
- All 24 unstable-period functions mapped in `func_unst_id()`
- Real-valued optional params use `json_find_double` (e.g., BBANDS `optInNbDevUp`, SAR `optInAcceleration`)
- Price input support (OHLCV arrays) for STOCH, BBANDS, ADX, MACD, etc.
- Multi-output support (BBANDS=3, MACD=3, STOCH=2) with `outReal`, `outReal1`, `outReal2`
- Integer output support (CDL* patterns, MINMAXINDEX) with `outInteger`

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
| `fn xxx_unguarded(...)` | Cross-indicator calls: no range checks, `get_unchecked` indexing inside an `unsafe` block |

Cross-indicator calls always use `_unguarded` to avoid double-validation.
Functions with extra internal params (e.g. EMA's `k` factor) get an additional
`fn xxx_private(...)` exposing them; the guarded/unguarded variants pre-compute
the params and delegate to it. There are **no** `_unchecked` /
`_unguarded_unchecked` variants.

### Documentation (rustdoc)

`backends/rust_doc.rs` renders the canonical `ta_codegen/input/<name>/<name>.md`
(parsed into `DocDef` by `parser/doc_md.rs`, attached as `FuncDef.doc`) as rustdoc
on all three variants, and every guarded function gets a **generated runnable
doctest** (252 bars of deterministic synthetic data, all params at defaults,
asserts `Success`). Crate-level docs, Cargo.toml package metadata, and the crate
README.md are emitted by `generate_rust_crate_scaffolding` in `main.rs`. Verify
with `cargo doc --no-deps` (must be warning-free — prose escaping of `[`/`<` is
load-bearing) and `cargo test --doc` in `ta_codegen/output/rust/`.

### Debug-safe decrements

C's `while (i-- > 0)` idiom lets an unsigned counter wrap past zero; the Rust
backend emits `wrapping_sub(1)` for post/pre-decrement so debug builds (and
doctests) behave like the regtest-verified release builds instead of panicking
on `attempt to subtract with overflow`.

### Known Code Quality Issues (non-blocking)

1. **`collect_for_loop_vars`** doesn't recurse into nested structures
2. **`gen_opt_param_validation`** silently skips Real/Enum optional params

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
- Benchmark noise: full 161-indicator runs have 10-20% variance from icache pressure. Use `ta_bench --function=NAME --iters=500` for ground truth.
- All servers and bench binaries call `TA_Initialize()` at startup — required for candle settings defaults.
- Thermal canary (SMA) runs between each indicator in ta_bench to normalize CPU thermal state
