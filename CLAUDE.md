# CLAUDE.md - TA-Lib Code Generation Guide

## Architecture Overview

All indicator code is **generated** by a single generator, **`ta_codegen`**
(`ta_codegen/generator/`, Rust): it parses `ta_codegen/input/` → IR → renders
per-backend (C, Java, C#, Rust). The C backend is generated **in place** into
`src/ta_func` / `src/ta_abstract` (the shipped library); the Rust/Java/C#
bindings live under `ta_codegen/output/`. `generate` owns every committed file it
writes — the build-system source lists and the ta-lib.org function pages included
— so "regenerate, then `git status` is clean" is a total gate over the tree.

**Why the C is generated in place and not symlinked** to `ta_codegen/output/c`: a
whole-dir symlink breaks autotools' per-dir libtool recursion (`make` enters the
symlink's *physical* path, so the Makefile's relative `../../libtool` fails with
`Error 127`), and it would force a packaging dereference step. Real files in
`src/` avoid both — and downstream consumers (notably the PHP `trader` extension)
glob `src/ta_func/*.c` straight out of the released source tarball.

**Build separation (important):** the C build systems (CMake + autotools) build
**only C** — the library plus `ta_regtest` and `ta_bench`. `ta_codegen` is Rust,
built and run with cargo via `scripts/build.py`; **CMake never invokes cargo**, so
a C-only setup needs no Rust toolchain.

**Toolchain per backend.** Generating any backend's *source* needs only cargo.
Building and testing one needs that language's toolchain, and a missing tool is a
failure, not a silent skip — narrow with `--backend=` / `build.py --language=`
instead. Java needs a **JDK and `unzip`**: Maven comes from the committed wrapper
(`ta_codegen/output/java/library/mvnw`), which downloads the pinned,
SHA-256-verified Apache distribution itself. `ta_codegen build --backend=java`
runs `./mvnw clean package` and tests *that jar*, so nothing tests a class
directory and every machine builds with the same Maven. No credentials are
involved (signing and the Central upload sit behind the pom's `release` profile);
only the wrapper's first run needs the network.

The correctness baseline every backend is verified against is the frozen
pre-cutover reference (tag `reference-pre-cutover`, served as `ta_ref_serve`)
plus the hardcoded `ta_regtest` expected values.

See `ta_codegen/generator/CLAUDE.md` for generator internals and
`src/tools/ta_regtest/CLAUDE.md` for the test-runner spec.

### Source of Truth: ta_codegen/input/

One directory per indicator, and the single source of truth for ALL generated
code.

- **YAML** = data, config, enums, IDL. Pure definitions with no logic.
- **C source files** = logic. Anything with computation.
- **No logic in YAML, ever.** No hand-coded string literals for type definitions
  or scaffolding in the codegen either.

`generate` **writes back into this directory**, so a hand edit here is not always
the last word:

- Every `.c` is re-indented in place by a format pre-pass before anything else
  runs. It changes whitespace only, but it does overwrite hand indentation — so
  when the layout comes out wrong, fix `src/formatter.rs`, not the file.
- `internal_error_ids.yaml` hands each C internal-error guard its
  `TA_INTERNAL_ERROR(<id>)`, so the number a caller reports keeps naming the same
  guard across releases. Append-only — a full C run assigns the new keys and
  drops the dead ones; never renumber an existing entry by hand.

The managed backends have **two** batch tiers: `public OutRange <N>(...)`, which
validates array lengths and throws on a rejection, and `<N>_Impl` — the
transcribed numerics, package-private in Java and `internal` in C#, keeping C's
`RetCode` + out-param shape. There is no third, catch-and-convert tier.

**Two suffixes, one meaning each:**

| suffix | means |
|---|---|
| `_Impl` | the numerics — a transcribed body, nothing else |
| `_Internal` | a **variant** of an entry point, not a tier: the `startIdx`-anchored seams `_OpenInternal` / `_OpenAndFillInternal` |

That is the whole vocabulary, deliberately — one word per concept. C spells them
`<N>_OpenImpl` / `<N>_StepImpl` (plus its own `TA_<N>_ReleaseImpl`; Rust has
`Drop`, the managed backends have GC) and stays the reference spelling below.
Rust, Java and C# recase `<N>` and the verb to their own idiom rather than
mirroring C: Rust `<n>_open_impl`, Java `<n>OpenImpl`, C# `<N>OpenImpl` (`<n>` =
camelCase, `<N>` = PascalCase, the acronym single-capitalized — `Sma`, not
`SMA`). Neither tier is public anywhere, so no runtime gate can see the spelling:
`the_transition_tier_is_step_impl_in_every_backend` pins the *shape* — one word
per concept, in that backend's casing — not one literal string.

**The `_Open*` family is five methods, symmetric, two hops deep** (C's spelling;
the other three carry the same five hops in their own casing):

```
PUB  <N>_Open(in, params)                   -> <N>_OpenInternal(in, 0, params)
PKG  <N>_OpenInternal(in, startIdx, params) -> one sink per output; <N>_OpenImpl(.., 0)
PUB  <N>_OpenAndFill(in, params, outs)      -> aliasing guard; <N>_OpenAndFillInternal(in, 0, ..)
PKG  <N>_OpenAndFillInternal(in, sIdx, ..)  -> <N>_OpenImpl(.., 1)
PRV  <N>_OpenImpl(sp, in, sIdx, params, outBeg, outNb, outs, outStride)
```

Both public entries delegate at anchor 0, so **no seam is emitted unreachable**.
The guard sits on the public frame because that is the only one handed an array
it did not vet: the plain open sinks into fresh arrays, and a composed call's
destination is already proved disjoint by `SubCallStep::is_fusable`. `MA`
(Dispatch) and `MAVP` (PeriodBank) are exempt and hand-roll a body per entry
point — theirs differ by which callee tier they call and by an anchor clamp, not
by a stride — so their `_OpenImpl` takes no `outStride`, which is the
discriminator the gates key on rather than a name list.

A cross-call inside a body calls the callee's *public* tier, in all four
backends, and lets its rejection surface: a throw in Java and C#, an
`Err(RetCode)` in Rust, a returned code in C. The callers that need a code back
convert it themselves: the JSON-RPC servers, and C#'s `FunctionCall.TryInvoke`
(whose conversion is the direct path's too — its thunk calls the function's own
public overload, like C's frames and Java's `Dispatch`).

**Every metadata tier calls the public tier.** C (`ta_frame.c`), Java
(`Dispatch`), C# (`FunctionCatalog`'s `invoke` thunk) and Rust
(`ParamHolder::call`) all do, so binding a leg shorter than the requested range
is `TA_BAD_PARAM` in three of them and inexpressible in the fourth — C's setters
take a bare pointer and carry no length.

**And a rejected setter leaves the holder as it found it** — the price setter
validates every consumed component before it writes any. The case that makes it
worth a rule is a *re-bind*: on a fresh holder a partial write is masked by the
unbound-component report, but over a bundle that already worked, the rejected
call leaves the components at and after the offending one holding the previous
bundle, and the next call succeeds over a mixture of the two — in C# with no code
and no exception. C is deliberately NOT the ports' whole-struct assignment: its
macro keeps the flag guard so an unconsumed component is skipped rather than
clobbered, which makes the fix a no-op on every call that succeeds.
`metadata_price_setter_validates_before_writing` pins all four on the PR gate,
because the four runtime probes are nightly-only.

Do not hand-edit **generated** files under `ta_codegen/output/` — they are
overwritten on the next `generate`. The converse trap: some hand-written source
lives under `output/` too (the Java shared types, `Core.java` outside the GENCODE
markers, the test suites, the C# `TALib.csproj`); the generator preserves those
and never overwrites them. `pom.xml` is maintained by `ta_codegen`.

## Comments and docs: guidance, not narration

Assume the reader is an AI that can read the code faster than the prose about it.
A comment or a `.md` earns its place only by saying something the code cannot:

- **Default to none.** Code shows *how*. Write a comment only for a *why* a
  careful reader would not reach from the code in a minute.
- **Guidance, not description.** Which invariant is load-bearing, what breaks if
  you change it. Not what the lines below do.
- **Never write these.** A restatement of the lines below. The reasoning that led
  here. An alternative that was rejected. What the code used to be. "Fixed X",
  "per review". A pointer to a doc instead of the fact itself. All of it belongs
  in the issue and the commit message, which are timestamped and do not pretend
  to be current.
- **Never re-explain another file.** That copy goes stale the moment the original
  moves, and costs a reader either a wrong belief or a re-verification. When you
  find one that has drifted, delete it — do not re-sync it. Drift is the symptom;
  the duplication is the defect.
- **A pitfall earns a sentence only if it fails SILENTLY** — and state the rule,
  not the history. "Keep every `(int)` cast the whole right-hand side" beats a
  paragraph naming the emitter that used to get it wrong. Loud failures get
  rediscovered in minutes and need nothing.
- **Named internals are the tell** — a function, a struct field, a symbol out of
  a compiler error. Most likely to be renamed out from under the comment, and
  naming them is usually description wearing a rule's clothes.
- **Doc lines outnumbering the code they describe means cut, not balance.**
  Writing less is the weaker half; re-reading and deleting before you commit is
  the half that works.

Applies to `ta_codegen/input/**` headers and `.md` files as much as to the
generator's own source. `ta_codegen/generator/input_synth/README.md` states the
gate-fixture form of the rule.

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
scripts/build.py ta_ref_serve   # The frozen pre-cutover oracle, from the pinned-tag worktree
scripts/build.py regtest        # Servers (cargo) + C tests + cross-language verification.
                                # Needs bin/ta_ref_serve to already exist; build it with the
                                # target above. Building the oracle is build.py's job -- nothing
                                # on a test path repairs what it is about to measure.

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

`ta_regtest` is the **universal test runner**. Rather than linking each language's
compiled code, it drives one generated JSON-RPC server per language over
stdin/stdout and compares every call against the C reference.

A **correctness** request goes through each language's PUBLIC API, and the server
turns the exception back into the `retCode` / `outBegIdx` / `outNBElement` wire
shape — normalisation is the server's job, not the library's. In **Java and C#** a
request that declares itself timed (`"timed":1`, which only `ta_bench` sends)
calls the BODY inside the timed loop, because these servers are also the
cross-language benchmark and nothing measured may quietly acquire the public
tier's argument checks. Rust has no such split: `tools` is a separate crate, so
the public entry point is the only one it can reach. Flags, tolerances and the
individual gates are specified in `src/tools/ta_regtest/CLAUDE.md`.

A new ta_regtest source file must be registered in BOTH `CMakeLists.txt` and the
autotools `Makefile.am` — the dist-verification CI path builds with autotools, so
a missing entry there breaks the nightly. `scripts/build.py check-source-lists`
verifies the two agree. `scripts/synth_gate.py` (nightly, and runnable locally)
covers generator constructs no shipped indicator uses.

## Rust Backend

Generated Rust lives in `ta_codegen/output/rust/` — a Cargo workspace: `library/`
is the shipped `ta-lib` crate, `tools/` holds the JSON-RPC server and bench.
Indicators are methods on a `Core` struct, one file per indicator.

- **The public batch API is `pub fn <N>(...) -> Result<OutRange, RetCode>`, and it
  owns the argument contract** — index range, parameters, then every buffer
  length, answering `BadParam`. Its input bound takes no sub-lookback escape, so
  it is strictly stronger than the assert below and a `pub fn` call cannot reach
  one that would reject it. It is also the tier every cross-indicator call
  enters, so the same bound holds on the composed path.
- Indexing is safe: the crate is `#![forbid(unsafe_code)]`, so a violated bounds
  precondition panics rather than being undefined behavior. Each body carries a
  bounds-assert preamble (the LLVM proof that elides per-access bounds checks),
  skipped when the lookback clamp means the call computes nothing, so a call
  returning `Success` with zero elements cannot panic. Nothing but `pub fn <N>`
  and the phantom-I/O sweep reaches it, so a panic there is a generator bug —
  which is what an `assert!` is for.
- **Cross-indicator calls target the callee's PUBLIC entry point**, as in C, Java
  and C#. `<N>_Impl` stays the crate-private numerics tier — C's `RetCode` +
  out-param shape, which is what the transcription is written against — with only
  `pub fn <N>` and the phantom-I/O sweep calling it. `?` is unavailable inside
  `<N>_Impl`, which returns a bare `RetCode`, so each site binds the returned
  range with a `match`, assigns both out-params, then sets
  `retCode = RetCode::Success`; the guard that followed is folded out
  (`ir_cleanup::drop_answered_cross_call_guards`), while the assignment stays
  because some sites fold "success with zero output" into the same conditional. A
  `Result`-returning `<n>_open_impl` spells the error arm `return Err(_e)`. The
  `mem::swap` shim for an in-place callee is unchanged and still owed. Reach is
  what settles the tier choice: `no_phantom_io` probes `<N>_Impl`, so a callee's
  public input bound answering before any array is touched would blind it —
  `analyze_dispatch` admits a leading "nothing to produce" guard instead. C
  cannot converge the other way: a cross-call is cross-TU there, so a C `_Impl`
  could not be `static` and would be new ABI in the shipped `.so`.
- Rustdoc, including a runnable doctest per function, is generated from each
  function's canonical `<name>.md`. Verify with `cargo doc --no-deps`
  (warning-free) and `cargo test --doc` in the crate.

## Adding or Modifying an Indicator

1. Edit the definition in `ta_codegen/input/<name>/` (C logic) and/or its YAML
2. `cd ta_codegen/generator && cargo run -- generate` (optionally `--func=<NAME>`)
3. `scripts/build.py servers` to rebuild the language servers
4. `cd bin && ./ta_regtest --codegen --function=<NAME>` to verify every backend
   against the C reference
5. **Verify other languages' output is unchanged** when fixing one backend
   (`git diff` the generated files)

The `/new-ta-func` skill automates picking up and resuming this work.

## Two build flags that must stay in step

The generator's flags live in one place (`COMMON_GCC_FLAGS`, `main.rs`); two are
set by all three build systems (CMake, autotools, the generator) and must stay in
step:

- `-ffp-contract=off` — load-bearing for the FMA contract, **not** a performance
  knob.
- `-fno-math-errno` — purely a performance knob, and the one part of
  `-ffast-math` that cannot change a value. Do **not** weaken it to `-ffast-math`
  on the strength of that: the same output-hashing harness shows `-ffast-math`
  changing 70 functions. It is not effect-free either — it lets STDDEV's sqrt map
  vectorize and raise `FE_INVALID` on lanes the scalar guard skipped (values
  unaffected). That, and why clamping the radicand does not fix it, are in
  `CMakeLists.txt` next to the flag.

## Benchmarking

The `ta-bench` skill covers it: `ta_bench`, `ta_bench_direct`, `ta_bench_stream`
and `scripts/stream_ab.py`; what each ratio actually compares (the same source
builds six different binaries); streaming vs batch; and the `--shape=` input
corpus.
