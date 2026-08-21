# CLAUDE.md - TA-Lib Code Generation Guide

## Architecture Overview

All indicator code is **generated** by a single generator, **`ta_codegen`**
(`ta_codegen/generator/`, Rust): it parses `ta_codegen/input/` → IR → renders
per-backend (C, Java, C#, Rust). The C backend is generated **in place** into
`src/ta_func` / `src/ta_abstract` (the shipped library); the Rust/Java/C#
bindings live under `ta_codegen/output/`. `generate` owns every committed file it
writes — the build-system source lists and the ta-lib.org function pages
included — so "regenerate, then `git status` is clean" is a total gate over the
tree.

**Why the C is generated in place and not symlinked** to `ta_codegen/output/c`: a whole-dir
symlink breaks autotools' per-dir libtool recursion (`make` enters the symlink's *physical*
path, so the Makefile's relative `../../libtool` fails with `Error 127`), and it would also
force a packaging dereference step. Real files in `src/` avoid both — and downstream
consumers (notably the PHP `trader` extension) glob `src/ta_func/*.c` straight out of the
released source tarball.

**Build separation (important):** the C build systems (CMake + autotools) build **only
C** — the library + the C tools (`ta_regtest`, `ta_bench`). `ta_codegen` is Rust and is
built/run with cargo via the developer script `scripts/build.py`; **CMake never invokes
cargo**, so a C-only setup needs no Rust toolchain.

**Toolchain per backend.** Generating any backend's *source* needs only cargo. Building
and testing one needs that language's toolchain, and a missing tool is a failure, not a
silent skip — narrow with `--backend=` / `build.py --language=` instead. Java needs a **JDK
and `unzip`**: Maven owns the jar, but comes from the committed wrapper
(`ta_codegen/output/java/library/mvnw`), which downloads the pinned, SHA-256-verified
Apache distribution itself. `ta_codegen build --backend=java` runs `./mvnw clean package`
and then tests *that* artifact — so there is no second builder, nothing tests a class
directory, and every machine builds with the same Maven. No account or credentials are
involved (signing and the Central upload sit behind the pom's `release` profile); only the
wrapper's first run needs the network.

The correctness baseline that all `ta_codegen` backends are verified against is
the frozen pre-cutover reference (the `reference-pre-cutover` tag, served as
`ta_ref_serve`) plus the hardcoded `ta_regtest` expected values.

See `ta_codegen/generator/CLAUDE.md` for ta_codegen internals and
`src/tools/ta_regtest/CLAUDE.md` for the test-runner spec.

### Source of Truth: ta_codegen/input/

`ta_codegen/input/` is the single source of truth for ALL generated code
(one directory per indicator).

- **YAML** = data, config, enums, IDL. Pure definitions with no logic.
- **C source files** = logic. Anything with computation.
- **No logic in YAML, ever.**

No hand-coded string literals for type definitions or scaffolding in the codegen.

The managed backends have **two** batch tiers: `public OutRange <N>(...)`, which
validates array lengths and throws on a rejection, and `<N>_Impl` — the
transcribed numerics, package-private in Java and `internal` in C#, keeping C's
`RetCode` + out-param shape. There is no third, catch-and-convert tier; #236
step 5 deleted it.

**Three suffixes, one meaning each** (#236 settled this; the names were three
words for two concepts before):

| suffix | means |
|---|---|
| `_Impl` | the numerics — a transcribed body, nothing else |
| `_Internal` | a **variant** of an entry point, not a tier: the `startIdx`-anchored composition seams `_OpenInternal` / `_OpenAndFillInternal` |
| `_OpenPass` | the single whole-history transcription the whole `_Open*` family shares |

The two axes are independent, so they compose: `<N>_OpenAndFillInternalImpl` is
the numerics *of* the anchored `_OpenAndFill` variant, and both halves are
load-bearing — `<N>_OpenAndFillInternal` is a real declared function that calls
it. It is the one name where both axes appear at once, which makes it look like
a stack of synonyms; it is not, and it has been queried twice. `Core` and
`Body` were rejected: `Core` is the struct a caller holds and the documented
contrast with the Streaming API, and `Body` reads as markup.

A cross-call inside a body calls the callee's *public* tier and lets its
rejection throw, so the callers that need a code back convert it themselves:
the JSON-RPC servers, and C#'s `FunctionCall.TryInvoke`. That conversion is
live only while cross-calls go through the public tier — the same fact that
withholds `MA` from `NoPhantomIoTest`'s sweep — so the two move together.

Do not hand-edit **generated** files under `ta_codegen/output/` — they are
overwritten on the next `generate`. The converse trap: some hand-written source
lives under `output/` too (the Java shared types, `pom.xml`, `Core.java` outside
the GENCODE markers, the test suites, the C# `TALib.csproj`); the generator
preserves those and never overwrites them.

## Quick Reference Commands

```bash
# Build (from any directory in the repo; binaries land in bin/)
scripts/build.py                # C library + all C tools (CMake)
scripts/build.py ta_regtest     # Just the C test runner (CMake)
scripts/build.py ta_codegen     # Rust codegen tool (cargo)
scripts/build.py generate       # Regenerate every committed source for all backends —
                                # libraries, JSON-RPC servers, benches (cargo; writes only,
                                # so no JDK or .NET SDK for the Java/C# sources)
scripts/build.py servers        # Generate + compile the JSON-RPC language servers (cargo),
                                # and refresh bin/ta_regtest so bin/ can be driven by hand

# Test
scripts/build.py regen-check    # The PR gate: regenerating must change nothing
                                # (cargo + Python only; the same command CI runs)
scripts/build.py test           # C reference tests only (quick)
scripts/build.py regtest        # Full pipeline: servers (cargo) + C tests + cross-language verification

# ta_codegen (run from ta_codegen/generator/)
cargo run -- generate                            # Generate everything, all backends
cargo run -- generate --func=SMA --backend=rust  # Specific function + backend
                                                 # (whole-corpus files — Core.java, the
                                                 # servers, the benches — are skipped)
cargo run -- generate-servers                    # Only the JSON-RPC servers (a narrowing
                                                 # of `generate`, for `build`)
cargo run -- build                               # Compile servers into bin/
cargo test                                       # ta_codegen's own test suite

# ta_regtest directly (from bin/)
./ta_regtest                                     # C reference tests only
./ta_regtest --codegen                           # C tests + all-language codegen verification
./ta_regtest --codegen --language=c,rust --function=RSI,SMA
```

## Cross-Language Regression Testing

`ta_regtest` is the **universal test runner** for all languages. Instead of
linking against each language's compiled code, it drives one generated JSON-RPC
server per language over stdin/stdout and compares every call against the C
reference.

A **correctness** request goes through each language's PUBLIC API, and the
server turns the exception back into the `retCode` / `outBegIdx` /
`outNBElement` wire shape — normalisation is the server's job, not the
library's. In **Java and C#** a request that declares itself timed
(`"timed":1`, which only `ta_bench` sends) calls the BODY — the numerics and
nothing else — inside the timed loop, because these servers are also the
cross-language
benchmark and nothing measured may quietly acquire the public tier's argument
checks. Rust has no such split and never did: `tools` is a separate crate, so
the public entry point is the only one it can reach, and `ta_bench
--language=rust` has always measured it. Flags, tolerances and the individual
gates are specified in
`src/tools/ta_regtest/CLAUDE.md`.

A new ta_regtest source file must be registered in BOTH `CMakeLists.txt` and the
autotools `Makefile.am` — the dist-verification CI path builds with autotools, so
a missing entry there breaks the nightly. `scripts/build.py check-source-lists`
verifies the two agree. `scripts/synth_gate.py` (nightly, and runnable locally)
covers generator constructs no shipped indicator uses.

## Rust Backend

Generated Rust lives in `ta_codegen/output/rust/` — a Cargo workspace: `library/`
is the shipped `ta-lib` crate, `tools/` holds the JSON-RPC server/bench.
Indicators are methods on a `Core` struct, one file per indicator.

- Indexing is safe: the crate is `#![forbid(unsafe_code)]`, so a violated bounds
  precondition panics — never undefined behavior. Each body carries a
  bounds-assert preamble (the LLVM proof that elides per-access bounds checks);
  it is skipped when the lookback clamp means the call computes nothing, so a
  call that returns `Success` with zero elements cannot panic.
- **Cross-indicator calls target `<N>_Impl`**, the crate-private entry point that keeps C's `RetCode` + out-param shape. (It validates parameters and the index range; array bounds are the `assert!` preamble above, not a length check — say which, because "guarded" once contrasted with a retired `Unguarded` tier and now has no anchor.) The public batch API is `pub fn <N>(...) -> Result<OutRange, RetCode>`. **Rust alone.** Java and C# route a cross-call to the callee's PUBLIC entry point (#236 step 3), which is what C has always done; Rust did not follow because its public tier is a thin `Result` adapter that adds no checks the body's asserts do not already make, so the move would buy nothing and would still owe the in-place `mem::swap` shim.
- Rustdoc, including a runnable doctest per function, is generated from each
  function's canonical `<name>.md`. Verify with `cargo doc --no-deps`
  (warning-free) and `cargo test --doc` in the crate.

## Adding or Modifying an Indicator

1. Edit the definition in `ta_codegen/input/<name>/` (C logic) and/or its YAML metadata
2. `cd ta_codegen/generator && cargo run -- generate` (optionally `--func=<NAME>`)
3. `scripts/build.py servers` to rebuild the language servers
4. `cd bin && ./ta_regtest --codegen --function=<NAME>` to verify all backends
   against the C reference
5. **Verify other languages' output is unchanged** when fixing one backend
   (`git diff` the generated files)

The `/new-ta-func` skill automates picking up and resuming this work.

## Two build flags that must stay in step

The generator's flags live in one place (`COMMON_GCC_FLAGS`, `main.rs`); two
flags are set by all three build systems (CMake, autotools, the generator) and
must stay in step:

- `-ffp-contract=off` — load-bearing for the FMA contract (PR #96), **not** a
  performance knob.
- `-fno-math-errno` — purely a performance knob (issue #192), and the one part
  of `-ffast-math` that cannot change a value. Do **not** weaken it to
  `-ffast-math` on the strength of that: the same output-hashing harness shows
  `-ffast-math` changing 70 functions. It is not effect-free either — it lets
  STDDEV's sqrt map vectorize and raise `FE_INVALID` on lanes the scalar guard
  skipped (values unaffected). That, and why clamping the radicand does not fix
  it, are in `CMakeLists.txt` next to the flag.

## Benchmarking

The `ta-bench` skill covers it: `ta_bench`, `ta_bench_direct`, `ta_bench_stream`
and `scripts/stream_ab.py`; what each ratio actually compares (the same source
builds six different binaries); streaming vs batch; and the `--shape=` input
corpus.
