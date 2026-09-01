# ta_codegen — TA-Lib Code Generation Tool

Reads the per-indicator definitions in `ta_codegen/input/` — the source of truth
— and produces the indicator implementations for every language, the
introspection layers, and the JSON-RPC servers the cross-language regression
tests drive.

## Architecture

```
ta_codegen/input/            (per-indicator .c logic + YAML metadata)
       ↓
    parser                   (YAML → raw serde structs → IR;
                              .c source → IR Statement/Expr directly, no raw stage)
       ↓
    ir                       (FuncDef + Statement/Expr)
       ↓
  ┌────┴─────────────┐
backends            server_gen / bench_gen
  ↓                      ↓
src/ta_func/*.c          JSON-RPC servers, bench binaries,
src/ta_abstract/         src/ta_func/ta_func_stream_private.h
include/ta_func.h        (all generated IN PLACE)
ta_codegen/output/       per-language products: library/ (shipped) + tools/ (server, bench)
  c/tools/               (the C library itself ships from src/)
  rust/library/ + tools/           a Cargo workspace
  java/library/ + tools/ + fragments/   fragments are per-function method bodies:
                                   an intermediate that ships nowhere, re-rendered
                                   into Core.java from the IR and inlined into the
                                   server from disk
  csharp/library/ + tools/         shipped TALib package incl. src/metadata/
```

**Hand-written library templates** live under `templates/` — the generator's own
assets, kept out of `input/` (indicator definitions only) and out of `output/`
(100% generated):

- `templates/rust/types.rs` — the `Core` / `RetCode` / `CoreBuilder` /
  `CandleSettings` scaffolding, copied verbatim into the crate.
- Four more, copied verbatim and declared `#[cfg(test)]` in the generated
  `mod.rs`, so none ships in a release build: the scratch-buffer election value
  gate, the streaming tier's non-finite input rejection, a handle's `OutRange`
  against batch, and DIV's zero-divisor result. `cargo test --tests -p ta-lib` is
  the only thing that runs them — `clippy --all-targets` compiles the test target
  without executing it, and `--tests` rather than `--lib` because it must also
  reach `library/tests/`.
- `templates/c/ta_retcode.c.template` — spliced with `ta_retcode.csv`.
- `templates/c/ta_abstract_serve.c` — abstract-serve handlers `#include`d into
  the C server.

Every Rust template is listed in `main.rs`'s `RUST_TEMPLATE_MODULES` (test-only
ones also in `RUST_TEST_ONLY_MODULES`) and in `RustBackend::clean_keep`, so
`generate` copies them in and never deletes them. Adding one means touching all
three. A fifth `#[cfg(test)]` module is **generated, not copied**: the phantom-I/O
sweep, `src/ta_func/no_phantom_io.rs`. It lives in `src/` rather than `tests/`
because it probes the crate-private `<N>_Impl`, is listed in
`RUST_GENERATED_TEST_MODULES`, and is the one `clean_keep` entry that is not a
template — the stale-file sweep would otherwise delete a `.rs` in `src/ta_func/`
that names no indicator.

### Key Modules

| Module | Purpose |
|--------|---------|
| `parser` | YAML → `FuncDef`; `.c` → IR directly. Also classifies each function in a file as base / `_private` / `_ALT<n>` and reads its `PRAGMA` decorations |
| `ir` | `FuncDef`, `ParamType`, `Statement`, `Expr` |
| `backends/c.rs` | C indicators: guarded `TA_<N>` / `TA_S_<N>`, plus `TA_<N>_Private` where declared |
| `backends/rust_lang.rs` | Rust indicators (concrete `f64`) |
| `backends/java.rs`, `csharp.rs` | Java `Core` methods; one `Core_<NAME>.cs` per function |
| `backends/{c,rust,java,csharp}_stream.rs` | The four streaming emitters, each rendering the *same* backend-neutral analysis from `streaming.rs`. A fifth backend means writing only its emitter |
| `backends/rust_doc.rs`, `csharp_doc.rs` | Per-language rendering of each function's canonical `<name>.md` |
| `backends/ir_cleanup.rs` | Backend-selected, length-preserving IR passes between the streaming decision and rendering |
| `backends/abstract_rows.rs` | The backend-neutral `ta_abstract` row model, rendered by Rust, Java and C# — but deliberately not by C (below) |
| `backends/{rust,java,csharp}_metadata.rs`, `java_abstract.rs` | The per-language introspection registries |
| `backends/ta_abstract_c.rs` | C's `ta_abstract` layer: tables, frames, group index, runtime API |
| `backends/price_bundle.rs` | Folds expanded price components back into the single `TA_Input_Price` descriptor (`inPriceHLC` + flags) — **public ABI**, so derived once from each `Input`'s `PriceRef` and never re-inferred from argument names |
| `backends/func_api_xml.rs` | `ta_func_api.xml` |
| `backends/docs_site.rs` | The ta-lib.org pages, written into `website/`, not `output/` |
| `server_gen`, `bench_gen` | JSON-RPC servers; direct-call benchmark binaries |
| `registry` | Which indicators exist, and each backend's spelling of a name |

**The stream `NameMap` prefixes are shared on purpose.** `fma::stream_base`
strips exactly `sp->`, `sp.` and `cur_` to decide integer-vs-float typing, so a
backend inventing its own spelling silently changes which sites fuse `a*b+c` —
~1 ULP, with nothing pointing at the cause.

## Commands

```bash
# Runnable from ANY directory: the binary locates the repo via its own path (the
# `ta_codegen/input/` marker). Override with TA_CODEGEN_ROOT=/path/to/ta-lib.
cargo run -- generate                        # Everything: libraries + servers + benches
cargo run -- generate --func=SMA,RSI         # Specific functions
cargo run -- generate --backend=rust         # Specific backend
cargo run -- generate-servers [--backend=c]  # Only the JSON-RPC servers
cargo run -- build [--backend=c,java]        # Compile generated servers into bin/
```

`generate` writes **everything committed**, so "regenerate, then `git status` is
clean" is a total gate over the tree. `generate-servers` and `generate-bench` are
narrowings for callers that rebuild a server without a full regeneration; they
own no path `generate` does not write. Emitting `.java` or `.cs` is text, so none
of this needs a JDK or the .NET SDK — those belong to `build`.

The exception is `--func=`: whole-corpus files (`Core.java`, the servers, the
benches) are skipped, because rendering them from a filtered set would drop every
function the filter excluded. A `--func` iteration loop must end with one bare
`generate` before committing.

## Testing

```bash
cargo test      # tests/*_suite.rs, topic-scoped, shared harness in tests/common/
cargo clippy    # strict pedantic lints
```

Value gates that need the *generated* library live in the crate itself as
`#[cfg(test)]` modules (see the templates above); run them with
`cargo test --tests -p ta-lib` from `ta_codegen/output/rust/`.

Cross-language verification is `ta_regtest`'s job, and
`src/tools/ta_regtest/CLAUDE.md` is its spec — including the wire format and the
traps in driving a server by hand.

## Alternate implementations (`PRAGMA TA_ALT`)

Some functions need genuinely different algorithms per API tier: the rolling
extrema run a block-batched Van Herk scan in batch, which cannot be transcribed
into a per-bar automaton. An input `.c` may declare `<name>_ALT<n>` alongside
`<name>`, decorated with `/* PRAGMA TA_ALT={<api>,<lang>} */`; the authoring
contract is in `docs/ta_codegen_input_code.md`.

**There is exactly one resolution point.** `ir::FuncDef::resolved_for(lang)`
returns a view whose `body` is the `(BATCH, lang)` winner and whose
`stream_source()` is the `(STREAM, lang)` winner. It is called at each language
backend's `generate` entry, and again inside each `*_stream::generate`, which is
idempotent and covers the callers that bypass the backend. Everything downstream
keeps reading `body` / `stream_source()` and needs to know nothing. Teaching the
scattered selection sites about the language instead would make *missing one*
silent: it would quietly render the base. `resolved_for` pins **both** tiers even
where the base wins one, because `stream_source()` falls back to `body` and
`body` is about to become the batch winner — SYNTH6 is the only shape where an
alternate claims BATCH, so a resolver leaking the batch body into the stream
fails there and nowhere else.

**Nothing but the emitted code can prove which body won.** An alternate is
generator input, not a symbol, so every value-comparison gate passes whichever
body was selected, and the `/* Using min_ALT1 ... */` marker is rendered *from*
the resolution and would agree with a resolver that chose wrong.
`tests/alt_suite.rs` checks the emitted statements against SYNTH5 and SYNTH6 —
the same algorithm with the tiers swapped — so neither an always-base nor an
always-alternate bug satisfies both.

## The abstract layer: one row model, and one independent oracle

Rust's `abstract_api`, Java's server table and shipped registry, and C#'s
`TALib.Metadata` all render `abstract_rows.rs`. **C's `ta_abstract_c` does not,
and that is the point:** `test_abstract.c` proves each server's metadata against
the C library at run time, and folding C into the shared rows would leave a
generator compared against itself. `func_api_xml` stays out for the same reason
plus a different projection (display labels, legacy ordering).

C's dedup against its hand-written `TA_DEF_UI_*` descriptors is a **total
equality test**, and has to stay one: a slot reuses a constant only when it
already says exactly what the YAML says — name, display name, hint, flags,
default, range, suggested triple, precision — so a fold is pure `.rodata` dedup
with no semantic content. Keying on a subset lets a new function inherit another
function's wording, and the divergence then surfaces only after a four-language
server build, phrased as though the server were wrong.

The price is worth naming: for a folded slot the gate can no longer fail on
`displayName` / `hint` / `suggested`, since equality is what selected the
descriptor. Editing SMA's `display_name` shows up instead as a diff to
`src/ta_abstract/tables/table_s.c`, which `regen-check` pins. What no gate sees
is a wrong `default:` or a wrong function-level `hint:`, where every derivation
moves together.

The two pieces C *does* share are shared because an independent gate already
covers them: `price_bundle` (its own unit tests) and the flag bit values
(`flag_sync` pins them against `include/ta_abstract.h`).

A row carries no derived *name*. There is one name — the YAML `name` — and every
backend spells it verbatim (C alone prefixes `TA_`), so there is nothing to keep
in sync and no way to name a method that does not exist.

## Rust Backend Details

Concrete `f64` only: `&[f64]` / `&mut [f64]`, `usize` indices, `i32` optional
params. No generic `<T: TaFloat>`, no `f32` variants.

| Variant | Purpose |
|---------|---------|
| `pub fn <N>_Lookback(...) -> Result<usize, RetCode>` | First valid output index |
| `pub fn <N>(...) -> Result<OutRange, RetCode>` | The batch API, and the tier that **owns the argument contract**: index range, then parameters, then every input and output length, before it calls `<N>_Impl` |
| `pub(crate) fn <N>_Impl(...) -> RetCode` | The body. Keeps C's shape — a code plus `&mut outBegIdx` / `&mut outNBElement` — because that is what the transcribed bodies are written against, and it is where the FMA dispatch sits. Not a cross-call target |
| `fn <N>_Private(...)` | Only where the definition declares one; extra pre-computed params, no validation prologue. No shipped indicator declares one — the construct is carried by the `SYNTH4` gate fixture |

Cross-indicator calls target the **public** wrapper, as in C, Java and C#. `?` is
unavailable at those sites (the caller returns a bare `RetCode`), so
`render_cross_indicator_call` drops the two out-meta arguments, binds the
returned range with a `match`, assigns both out-params from it, and sets
`retCode = Success`. The `if( retCode != SUCCESS )` that followed can no longer
be taken and is folded out by `ir_cleanup::drop_answered_cross_call_guards`; the
assignment stays, because some sites fold "success with zero output" into the
same conditional and that half survives alone.

`<N>_Impl` still carries the bounds-assert preamble with its empty-range escape,
so a call computing nothing cannot panic. Only `pub fn <N>` and the phantom-I/O
sweep meet it, which makes it the LLVM proof and the sweep's target rather than a
guard on the cross-call path.

`rust_doc::guarded_docs` documents the **public** wrapper, so its `# Arguments`
must match that signature, not `_Impl`'s.
`rust_public_entry_documents_exactly_its_parameters` pins the two together;
rustdoc has no lint for documenting a parameter that does not exist.

Every guarded function gets a **generated runnable doctest** (252 bars of
deterministic synthetic data, defaults everywhere, propagating with `?` and
asserting a non-empty `OutRange`). Verify with `cargo doc --no-deps` — must be
warning-free, prose escaping of `[`/`<` is load-bearing — and `cargo test --doc`.

### Scratch-buffer election

Several C bodies elect one of their own *output* buffers as the scratch the
calculation runs in (`BBANDS` opens with `tempBuffer1 = outRealMiddleBand;`, so
it allocates nothing). In C that is a pointer assignment; Rust has no pointer to
assign, so a naive rendering emits `outRealMiddleBand.to_vec()` — an allocation
plus a copy of bytes overwritten before they are read, sized by the *caller's
slice* rather than by the data range.

`ScratchElection` restores C's shape by renaming the local to the elected output,
leaning on Rust's own aliasing rules (`&[T]` and `&mut [T]` parameters can never
overlap, nor can two `&mut [T]`) — which is also what makes C's aliasing arms,
its input-alias guard and its copy-back statically dead here.

The rule is stated over the IR and names no function, buffer or MA type: match an
`if`/`else if`/…/`else` chain whose *every* condition is an input↔output pointer
equality and whose *every* arm is only `scratch = someOutput;`; take the terminal
`else`'s mapping; delete the chain and rename through the rest of the enclosing
block; drop any guard that became a self-comparison. That last clause is
load-bearing: left in place, `BBANDS`' copy-back would read and write the same
`&mut` slice in one statement, which is E0502.

Requiring *every* arm to be an election is what declines `STOCH`, `STOCHF` and
`MAVP`: each mixes an allocation and an `…IsAllocated = 1;` flag into a branch,
which is a genuine in-place defence rather than an election. Tolerating one
allocating arm would reach them — a widening of the rule for a later change,
never a per-function case. An election stops at the end of the block holding it,
so `BBANDS`' general MA path and both stream paths keep their real allocations,
and the pass backs off entirely if the local is assigned again while in scope.
`rust_scratch_election_declines_arms_that_allocate` sweeps every indicator and
asserts the pass fires for `bbands` alone.

The other backends assign the reference directly and must see no change from
this: `generate` then `git diff` over `src/ta_func/`, `output/java/` and
`output/csharp/`.

### Debug-safe decrements

C's `while (i-- > 0)` lets an unsigned counter wrap past zero, so the Rust
backend emits `wrapping_sub(1)` for post/pre-decrement — debug builds and
doctests then behave like the regtest-verified release builds instead of
panicking on `attempt to subtract with overflow`.

## Linting

Strict Clippy pedantic in `src/lib.rs`, with `module_name_repetitions`,
`must_use_candidate`, `format_push_string` and `doc_markdown` allowed.
`rustfmt.toml`: edition 2021, max_width 100, `use_field_init_shorthand`.

## Performance: C server compilation

- The server is single-TU (`#include`s the `.c` files). Do NOT switch to separate
  compilation — it causes CDL binary layout issues.
- Candle settings are hoisted once into local `int`/`double` vars at the top of
  each function by `emit_c_unpacking()` — plain reads, no `volatile` cast.
- Ternary chains, not switch statements, for numeric-case switches.
- CCI uses a conditional reset (`idx++; if(idx>=max) idx=0`), not modulo — modulo
  costs ~10 cycles on ARM.
- Full parameter validation is required even where it looks redundant: removing
  it changes compiler register allocation.
- `ta_ref_serve` is statically linked against `libta-lib.a` and MUST be rebuilt
  when cmake rebuilds the library, or benchmarks compare against stale code.
  `regtest.py` rebuilds it in the cmake step.
- Full-suite benchmark runs carry 10-20% variance from icache pressure; use
  `ta_bench --function=NAME --iters=500` for ground truth. A thermal canary (SMA)
  runs between indicators to normalize CPU state.
- Every server and bench binary calls `TA_Initialize()` at startup — required for
  the candle-settings defaults.
