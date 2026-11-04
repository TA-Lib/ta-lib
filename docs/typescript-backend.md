# TypeScript backend

**Status:** this is the living plan for the TypeScript backend of `ta_codegen`, and the tracking issue links here. Rows marked Ruled were decided by the owner on 2026-11-01. The owner left the module format, the array types and the error model to this design. Those rows read Proposed and stand unless he reopens them. The implementation session updates each phase's Status line and appends a section 8 row at each deliverable checkpoint and each phase end. Nothing else in this file records progress.

## 1. Goal and scope

- **What ships.** An npm package that `ta_codegen` generates the same way it generates the Java and C# libraries. For every function it carries the batch tier, lookback, display shift and streaming tier, plus the metadata (abstract) API. It is pure TypeScript compiled to standard ES modules, with no runtime dependencies. It runs on Node and on any runtime that loads standard ESM.
- **How it is gated.** It is gated like the other managed backends. A generated JSON-RPC server is driven by `ta_regtest --codegen` and `--xlang-hash` and compared against the C library. Every call must match bit for bit, except a call that reaches a transcendental function (D11).
- **What does not ship:**
  - WASM
  - a native addon
  - a browser-specific bundle (standard ESM is the browser story)
  - a CommonJS build
  - a single-precision (`TA_S_`) surface

## 2. Decisions

| # | Decision | Choice | Why | Status |
|---|---|---|---|---|
| D1 | Implementation | Pure TypeScript compiled to JS. No WASM, no native addon | One artifact for every JS runtime, emitted by the managed-backend machinery | Ruled (owner, 2026-11-01) |
| D2 | npm name | `ta-lib`. The name is one constant in the TS scaffolding emitter | Matches the Python and Rust package names | Ruled (owner, 2026-11-01); transferred 2026-11-02 (O1) |
| D3 | Module format, min Node | ESM only, with no top-level await anywhere and no `node:` imports in the library. `exports` has `"."` and `"./metadata"`, each with `"types"` first and `"default"` last. `"sideEffects": false`. `engines.node` is `>=22.12.0`, and CI runs 22, 24 and 26, plus exactly the `engines` floor for the packed tarball and the suites that import it | Every supported Node line can `require()` an ESM-only package. One artifact means no dual-package hazard breaking `instanceof TALibError` | Proposed |
| D4 | TS compile target | `target` and `lib` ES2022, plus `strict`, `erasableSyntaxOnly`, `verbatimModuleSyntax`, `isolatedModules`, `declaration` and `stripInternal`. No `enum` or `namespace` (use `as const` objects). `noUncheckedIndexedAccess` is off. `types: []` in the library tsconfig. `module` and `moduleResolution` `nodenext`; relative imports are spelled `./x.ts` with `rewriteRelativeImportExtensions`, so tsc emits `.js` and `node --test` runs the sources unchanged. Library `rootDir: src`, `outDir: dist`. TypeScript is pinned in the tooling lockfile | ES2022 runs on every supported runtime. Erasable syntax lets Node run the sources by type stripping. Use no option that TS 6.0 deprecates, so 7.x also compiles it | Proposed |
| D5 | Input arrays | `ArrayLike<number>`: any typed array or a dense `number[]`. `Float64Array` is the recommended form and is read in place. The public tier and `Open` copy every other form into a `Float64Array` once on entry | Numerical libraries take typed arrays. JS TA libraries and platform data (OHLC objects, row arrays) produce `number[]`. The copy is for correctness: it applies ToNumber, so a `number[]` holding numeric strings (common from CSV or JSON parsing) cannot reach a body where `+` concatenates and `<` compares text. Phase 4 measures the copy's cost only | Proposed |
| D6 | Output arrays | Caller-owned `Float64Array` for real outputs and `Int32Array` for integer outputs. Never `number[]` | Bit-faithful to float64. `Int32Array` matches C `int`. Hands off to Arrow and similar with no copy. A `number[]` would grow silently on an out-of-range write | Proposed |
| D7 | Single precision | No `TA_S_` analogue: a `Float32Array` is an `ArrayLike<number>`, widened exactly by the D5 copy. The server serves the float leg's `use_float` by passing the widened values as a `Float32Array` to the same public tier | C's `TA_S_<F>` widens every float input and is bit-identical to `TA_<F>` on the widened doubles, so a widened `Float32Array` is the same contract. The float leg then gates the non-`Float64Array` path | Proposed |
| D8 | Error model | Throw one class, `TALibError extends Error`, carrying a stable `code` (`'TA_BAD_PARAM'`, ...) that is the documented contract, and a numeric `retCode`. A JS `TypeError` or `RangeError` from misuse is never wrapped. No Result unions | Throwing is idiomatic TS and Node. A `code` match survives duplicate package copies and realms, where `instanceof` does not. Warm-up is an output range, not an error | Proposed |
| D9 | FMA | Every fused site runs `runtime/fma.ts`'s `fma(a, b, c)`: software, exact and correctly rounded. Sites are chosen by the shared `fma.rs`. A site runs it as a call or, in a body with three or more fused sites, as its body copied in by the generator from that file, which stays the single swap point | Bit-exactness lets the existing harness gate TS with no tolerance | Ruled (owner, 2026-11-01); copy ruled (owner, 2026-11-02) |
| D10 | Integer semantics | Emitted from IR types: a C type pass (`Int32`, `Real`, `Bool`) over every expression. An integer `/`, an integer `%` and every real-to-int conversion, explicit or implicit, truncate with `\| 0`. Generation fails on any expression it cannot type | JS has no integer type, and a wrong guess fails silently | Ruled (owner, 2026-11-01) |
| D11 | Transcendental functions | The JS engine's `Math.*`. A call that reaches one (the function carries `uses_transcendental`, or an MA-type parameter selects MAMA or ALMA) compares in the transcendental lane at 1e-11, scaled like Java's 1e-9 (relative above magnitude 1, absolute below). Every other call, and same-language stream against batch, stays bitwise | `Math.*` is neither correctly rounded nor glibc: Node 22 against glibc 2.40 measured at most 2 ULP per call. This is the exception Java and C# already have (rD1), two orders tighter. V8's `Math.*` and Java's give the same maxima on the ill-conditioned functions (KSTEXT, MAMA), so the gap is the libm, amplified by the function. Consequence: such a call can also differ between JS engines and between engine versions. HT_DCPHASE and HT_SINE have no tolerance-sized bound on input that stays flat for a whole dominant cycle, in every language with a lane, Java and C# included: the cycle's in-phase and quadrature sums are then cancellation noise, so a 1-ulp `sin` or `cos` difference can flip the phase quadrant, and C's own output moves by hundreds of degrees under one | Ruled (owner, 2026-11-01): 1e-11 (O2) |
| D12 | Function shape and settings | One ES module per function, exporting free functions. A trailing optional `core: Core = Core.DEFAULT` carries unstable periods and candle settings. `Core` is immutable and built by `CoreBuilder`. A stream captures its `Core` at open | Tree-shakable, and keeps the Java and C# `Core`/`CoreBuilder` words | Proposed |
| D13 | Optional parameters | Positional, in C order, with verbatim names (naming spec E1). `Core.INTEGER_DEFAULT` or `Core.REAL_DEFAULT` selects the default. Enum parameters are typed as the enum's literal union plus the integer sentinel. An integer parameter is rejected unless it passes `Number.isInteger` and fits int32, and a real one unless it is a number | One signature shape across five backends, a 1:1 map for the metadata tier, and no object per call | Proposed |
| D14 | Allocating convenience form | None in the first release | Keeps a single public batch tier, which is the one the harness gates | Proposed |
| D15 | Streaming handle | Class `<N>Stream`, returned by `<n>Open` and `<n>OpenAndFill`, with `update`, `peek`, `value`, `outRange`, `advance` and `clone`. A single-output function returns a `number`. A multi-output function writes into a caller-owned `<N>Out`. No `close()` | Garbage collection frees the handle. The sink avoids an allocation per bar, as in Java | Proposed |
| D16 | Naming and tiers | Java casing: functions lowerCamel, types UpperCamel. `<n>Impl`, `<n>OpenInternal` and `<n>OpenAndFillInternal` are exported from their module, tagged `@internal`, and absent from the package barrel. `<n>OpenImpl` and `<n>StepImpl` stay module-local. Handle state fields are public and tagged `@internal`, because a module-local function cannot reach a TS `private` member | This is the CLAUDE.md vocabulary recast. The `exports` map plus an explicit barrel is TS's package-private | Proposed |
| D17 | Backend id | `--backend=ts`, `--language=ts`, label `TypeScript`, release tag `ts-v<version>` | Follows the `java-v` / `rust-v` / `csharp-v` pattern | Proposed |
| D18 | Metadata API | Subpath `./metadata` with `Functions.byName` / `all` / `groups`, `ParamHolder` (`setInput`, `setOptInput`, `setOutput`, `lookback`, `displayShift`, `call`), `FuncInfo`, the flags, `FunctionDescription` and `getRuntimeInfo`. `call` invokes the public tier. The server answers every abstract RPC from this registry | Naming spec section 7 words. One table, and no table private to the server | Proposed |
| D19 | Docs | TSDoc generated from each `<name>.md` through `doc_meta.rs`. Each `@example` is also emitted into a generated test that `node --test` runs | The analogue of the Rust doctests | Proposed |
| D20 | Package layout | See the layout under this table | The C# model: the server compiles the shipped source | Proposed |
| D21 | Toolchain | Building and testing need Node `>=22.18` (type stripping on by default, so `node --test` runs `.ts` tests) and npm. TypeScript and `@types/node` come from the committed lockfile through `npm ci`, which `build_ts` runs itself, and only the first run needs the network. `scripts/node_toolchain.py` supplies a pinned Node of each CI major on any machine. A missing tool is a failure; narrow with `--backend` / `--language` | Same rule as the JDK and the .NET SDK. Consumers need only D3's `>=22.12` | Proposed |

Package layout under `ta_codegen/output/ts/` (D20):

- **Tooling root:** `package.json` (private, no workspaces; devDependencies `typescript` and `@types/node` pinned to the lowest supported Node major) and `package-lock.json`. Only the tools and test tsconfigs list `node` in `types`, so the library compiles with no Node types (D3).
- **`library/`:** the published package.
  - Generated:
    - `package.json` (version from `VERSION`)
    - `src/func/<lowerCamel>.ts`
    - `src/index.ts` (explicit export lists)
    - `src/enums.ts`
    - `src/metadata/`
    - `test/gen/*.test.ts`: `generate` owns the directory and deletes any file there it did not write
  - Hand-written:
    - `src/runtime/`
    - `test/` outside `test/gen/`
    - `README.md`
- **`tools/`:** the generated server, the fuzz port and a tsconfig that compiles `../library/src`.
- **No bench package:** `ta_bench` times the server's `timed` path.

## 3. Public API sketch

```ts
import { sma, smaLookback, macd, smaOpen, macdOpen, MacdOut,
         ema, CoreBuilder, FuncUnstId, TALibError } from 'ta-lib';

const n = close.length;                         // close: Float64Array (number[] also accepted)
const out = new Float64Array(n);
const { begIdx, count } = sma(0, n - 1, close, 20, out);
// out[k] is the value at bar begIdx + k, for k < count; smaLookback(20) bars are consumed first

const m = new Float64Array(n), s = new Float64Array(n), h = new Float64Array(n);
macd(0, n - 1, close, 12, 26, 9, m, s, h);

const st = smaOpen(history, 20);                // TALibError 'TA_INSUFFICIENT_HISTORY' if too short
st.value();                                     // value at the last history bar
const v = st.update(bar);                       // commits a closed bar
const p = st.peek(formingBar);                  // commits nothing

const ms = macdOpen(history, 12, 26, 9);
const mo = new MacdOut();
ms.update(bar, mo);                             // mo.macd, mo.macdSignal, mo.macdHist

const core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 100).build();
ema(0, n - 1, close, 30, out, core);

try { sma(0, n - 1, close, 0, out); }
catch (e) { if (e instanceof TALibError && e.code === 'TA_BAD_PARAM') { /* e.retCode */ } }
```

```ts
export type RetCodeName = `TA_${Exclude<keyof typeof RetCode, 'SUCCESS'>}`;
export class TALibError extends Error {
  override readonly name = 'TALibError';
  readonly code: RetCodeName;       // the contract; match on this
  readonly retCode: RetCode;        // same number C, Java and C# report
}
```

## 4. Architecture

### 4.1 Generator modules (`ta_codegen/generator/src/`)

| New module | Cloned from | Notes |
|---|---|---|
| `backends/ts.rs` | `java.rs` (leaf `ExprEmitter`/`StatementEmitter`, out-parameters, `gen_public_wrapper`, `gen_argument_checks`) | One module per function: lookback, display shift, `<n>Impl` and public `<n>`. Call `FuncDef::resolved_for(Lang::Ts)` first. Import the helpers C# already takes from `java.rs` (`is_boolean_expr`, `bool_ternary_collapse`, `collect_address_of_vars`, `collect_double_address_of_vars`, `build_matype_map`, ...). Out-parameters copy Java exactly: `MInteger { value }` and a 1-element `Float64Array`. Allocation: `malloc` becomes `new Float64Array`/`new Int32Array`, `memcpy`/`memmove` become `set` or `copyWithin`, and `memset` becomes `fill` |
| `backends/ctype.rs` | none (new, shared) | The D10 type pass, seeded from `VarDecl` types, YAML `ParamType`, helper signatures, `MathFn` return types and stream-state field types, using C's usual arithmetic conversions. It marks an implicit real-to-int conversion (assignment to an int local, return from an int helper, an int argument) as a cast. A store into an `Int32Array` element needs no mark, because the store truncates. TS is its only consumer. Leave the other backends' typing alone |
| `backends/ts_stream.rs` | `java_stream.rs` | Handle class, the five-hop `_Open*` family, the `<N>Out` sink, and tape frames for MA (Dispatch) and MAVP (PeriodBank), which take no `outStride`. Supplies `streaming.rs`'s `NameMap` |
| `backends/ts_doc.rs` | `java_doc.rs` | TSDoc plus the generated example tests |
| `backends/ts_enums.rs` | `csharp_enums.rs` | Whole-file `as const` objects: `MAType`, `FuncUnstId`. `RetCode`, `CandleSettingType` and `RangeType` are hand-written in `runtime/`, as in Java |
| `backends/ts_metadata.rs` | `java_metadata.rs` | Renders `abstract_rows::rows()`. Its `Dispatch` analogue calls the public tier |
| `backends/ts_phantom_io.rs` | `rust_phantom_io.rs` | Generated sweep test (H14) |
| `server_gen.rs::generate_ts_server` | C# for the model (the server compiles library source), `generate_java_server` for handler text | Reuse `expand_input_names`, `output_json_key`, `func_unst_id`, `price_input_to_ref` and `doc_produced_extent`. Add `TS_FUZZ = include_str!("../templates/ts/fuzzData.ts")`, plus a `ts_predicate_expr` pin in `predicate_form_tests` |
| `ride_gen/ts.rs` | `ride_gen/java.rs` | The full field and diagnostic set |
| `stream_verify_gen/ts.rs` | `stream_verify_gen/java.rs` | `SV_RANGE_MASK_TS` takes all five sites, since the server reaches `_OpenInternal`. Reuse `sv_reject_condition` and `sv_guard_enum_ints` |

Template `ta_codegen/generator/templates/ts/fuzzData.ts`: a bit-exact port of `fuzz_data.h`. splitmix64 runs in BigInt (`BigInt.asUintN(64, ...)`) or as a hi/lo pair. The hash ports `fuzz_hash_init`, `fuzz_hash_bytes` and `fuzz_hash_fin`: FNV-1a 64 with the repo's basis `1469598103934665603`, which is not the standard one (copy the constants from `fuzz_data.h`, never retype them), followed by the murmur fmix64 finalizer. Without the finalizer no hash matches.

Edits to existing modules:

- `ir.rs`:
  - add `Lang::Ts` (`as_str`, `from_backend_name`) and grow `ALL_LANGS`
  - add `LangClaim::Ts` (`parse`, and `covers`; `AllLanguages` already covers every `Lang`)
  - every `ALL_LANGUAGES` alternate applies to TS: `resolved_for(Lang::Ts)` must select ALMA's batch alternate and every `{STREAM,ALL_LANGUAGES}` stream alternate. ALMA compares in the transcendental lane, so the cross-language gate cannot see a wrong choice there; a TS arm in `alt_suite` pins it
- `registry.rs::resolve_call`: a `Ts` arm using `camel_words`. It must agree with the emitter, or cross-calls break.
- `common.rs::enum_limit_names_of`: a `Ts` arm.
- `candle_settings.rs`: `emit_ts_unpacking`.
- `naming.rs`: a TS/JS reserved-word list, and a test that pins one.
- `backends/mod.rs`:
  - add `TsBackend` to `all()`
  - `out_subdir` is `library/src/func/`, which `clean_glob` covers
  - `clean_keep` lists the hand-written files
- `backends/runtime_info.rs`: `ts_fn()` and `TS_SERVER_HANDLER`.
- `main.rs`:
  - a TS tail in `generate()` for enums, metadata, `library/package.json` from `VERSION` (model: `generate_rust_crate_scaffolding`), `index.ts` and `library/test/gen/` (stale files removed)
  - a `build_servers` arm `"ts" => build_ts`
  - phase 3: a `build_libraries` arm `build_ts_library`, with `"ts"` added to its default list
  - the usage text
- `docs_site.rs`: `implementation_section` picks TS up from `all()`. `build_index()` hardcodes a language list; fix it in phase 3.

Not cloned:

- `java_keyscan.rs` (a HotSpot workaround)
- `java_shipped.rs` and the gencode digest (there is no spliced Core)
- `java_abstract.rs` (there is no second table)
- `stream_frame.rs`, `variant_frame.rs`, `meta_frame.rs` and `bench_gen.rs` (C only)
- `select_chain.rs`, unless phase 4 measures a reason

Shared and unchanged: `streaming.rs`, `fma.rs`, `ir_cleanup.rs`, `cond_fold.rs`, `divisor_guard.rs`, `expr_walk.rs`, `stmt_walk.rs`, `builtins.rs`, `abstract_rows.rs`, `price_bundle.rs`, `doc_meta.rs`, `helper_registry.rs`. An edit to a shared module that changes another backend's output (check U, section 6) needs a reason in section 8, and those backends are then timed (phase 4). No `ts*.rs` code tests a function name: a fix is a construct rule. Never relax a floor, a `VACUOUS` check or a bound to get green.

New generator suites: `ts_render_statement_suite.rs`, `ts_int_semantics_suite.rs` (the section 5 structural gate) and `ts_stream_suite.rs`. Register them in `tests/all/main.rs`.

### 4.2 Server

`tools/taCodegenServe.ts`:

- **Build.** `build_ts` first runs `npm ci` in `ta_codegen/output/ts` when `node_modules/.ta-lock.sha256` is missing or does not hold the SHA-256 of `package-lock.json`, and writes that stamp after a successful `npm ci` (npm's hidden lockfile never equals `package-lock.json`, so comparing the two reinstalls every build); a failure is fatal. It then wipes `bin/ta_codegen_ts/`, writes `{"type":"module"}` there as `package.json`, and runs `node_modules/.bin/tsc -p ta_codegen/output/ts/tools` into it. Never bare `tsc`, and never `npx tsc` before `npm ci`: with no `node_modules`, npx fetches an unrelated package named `tsc`. `build_ts_library` takes the same `npm ci` step.
- **Launch.** From `bin/`: `node ta_codegen_ts/tools/taCodegenServe.js`, with whichever `node` is first on PATH.
- **Contract.** It is the Java and C# servers' contract, specified in `src/tools/ta_regtest/CLAUDE.md`.

What is specific to TS:

- **Transport.** Hex, `usesSeed=0`. Encode and decode real arrays through a `DataView` with explicit endianness, never as JSON numbers: `JSON.stringify` turns -0 into `0` and NaN into `null`. `out_hash` is FNV-1a 64 over the raw bytes (I32 for integer outputs, F64 for real), then `fuzz_hash_fin`, in BigInt.
- **Correctness requests** call the public tier. A `TALibError` becomes `retCode = e.retCode`, with `outBegIdx` and `outNBElement` both 0. Any other throw reaches the top-level handler, which answers `{"error":"<name>: <msg>"}` and keeps reading. Never convert it to a code.
- **`use_float`.** Pass the widened inputs as a `Float32Array` to the same public tier (D7).
- **Timed requests.** `"timed":1` calls `<n>Impl` directly. Time with `process.hrtime.bigint()`; iteration 0 is a discarded warm-up.
- **Optional parameters.** An absent one means the default.
- **Output buffers.** Every output is pre-filled with the tail canaries, and `out_tail_dirty` is reported on success.
- **Input.** Read stdin with `readline`; a request line reaches 2 MB.

### 4.3 Harness (`src/tools/ta_regtest/`, `src/tools/ta_bench/`)

- `test_codegen.c`:
  - `argv_ts[] = {"node","ta_codegen_ts/tools/taCodegenServe.js",NULL}` and an `ALL_LANGUAGES` row `{"ts","TypeScript",argv_ts}`.
  - The lane's bound becomes per language, set in one place: `codegen_lang_needs_transcendental_tol` turns into a function returning the language's bound (0 for a bitwise language), and every site that reads `CODEGEN_TRANSCENDENTAL_TOL` today (`--codegen`, `server_verify`, `--xlang-hash`) reads it instead. `test_codegen.h` defines the TS bound as one constant, `1e-11`, beside Java's and C#'s `1e-9`, so changing it is a one-line edit. A non-zero bound brings the `xlang_illcond` skip with it and turns off `codegen_call_needs_vmath_tol`.
  - Every tolerance-lane compare keeps a per-language, per-function maximum of the scaled deviation, and a function over the bound keeps sweeping so its maximum is complete. For each language with a non-zero bound, `--codegen` and `--xlang-hash` end with one `TRANSCENDENTAL MAX [<lang>] TA_<F> <dev>` line per function compared in the lane (phase 1).
  - Section 8 records the TS lines of unfiltered `--xlang-hash` runs at the phase 1 and phase 2 exits. Over 1e-11, first confirm the deviation comes from the engine's `Math.*` and not from an emitter bug, then give the owner one row per function (maximum, shape, seed, parameters, and the C and TS values in `%a`), mark the phase `blocked on O2 (<functions>)`, and continue with work that does not depend on the answer. Never widen the constant without a ruling.
  - `codegen_lang_can_pass_enum_sentinel` is true for ts.
  - The float-leg tests (`!= "rust"`, and `server_verify.c::sv_serves_float`) already include TS, so the server must serve `use_float` from phase 1.
  - `stream_fuzz_port_selfcheck`: fail when a non-C server checks zero shapes. Today a missing `in_hash` skips silently, and a broken port then stays self-consistent through every stream leg. Prove it red with a server that drops `in_hash`.
  - `check_stream_counter_parity` also compares TS against Java, per function (phase 2).
  - A floor that fails when a server which passed the stream probe streams zero functions on an unfiltered run, proven red (phase 2). Today every stream floor is guarded by `streamFunctions > 0`.
  - `xlang_hash` `servers[]` gets a row with `usesSeed=0`, and its "valid:" error text.
  - `codegen_check_gencode_digest` stays Java only.
- `server_verify.h`: `SV_MAX_PIPES` goes from 4 to 5.
- `ta_regtest.c`:
  - a `svServers[]` row and the help text (phase 1)
  - the abstract-parity block in phase 3, matched with the exact-token `svLanguageEnabled`, not `strstr`
- `test_abstract.c`: an accessor for the number of functions the metadata pass compared, which the parity line prints. `codegen_pipe.c`: no change.
- `ta_bench.c`: `argv_ts` and a `LANGUAGES[]` row.
- `src/tools/ta_regtest/CLAUDE.md`: the `--language` table and the launch line.

`CMakeLists.txt`: the `regtest` target's `--language` list gains `ts` and its `COMMENT` names TypeScript (phase 1); `build.py regtest`, which macos-cross-language runs, otherwise never starts TS. No ta_regtest source file is added, so the source lists are unchanged.

### 4.4 Scripts

- `utilities/common.py`: `PREREQS_NODE` (checks `node --version` >= 22.18 and `npm`), `LANG_PREREQS["ts"]`, `CODEGEN_BACKENDS` and `PREREQS_BUILD_SERVERS` in phase 1. `LIBRARY_PREREQS["ts"]` in phase 3, together with `build_ts_library`: `synth_gate.py` and an unfiltered `build.py libraries` package every `LIBRARY_PREREQS` backend.
- `build.py`:
  - `build_xlanghash` rows, default and error text
  - `TARGET_PREREQS['xlang-hash']`
  - the help texts
  - `REGEN_PROBE_FILES` gets one generated library module and the server. Without them, `regen-check` cannot see a path that `generate` stopped writing.
- `synth_gate.py`: `LANGS` and the tally regex display name. `ta_codegen/generator/input_synth/synth_values.py`: a `("ts", ["node", "ta_codegen_ts/tools/taCodegenServe.js"])` row in `servers`, or the golden-value leg never runs TS.
- `check_stream_retcodes.py`: a TS row (phase 2).
- Phase 3:
  - `utilities/versions.py`: `get_version_string_ts` / `set_version_string_ts` for `library/package.json`, in both `sync_versions` and `check_versions`, as Cargo.toml has. `generate` writes the same value, so a `VERSION` bump followed by `sync.py` leaves `regen-check` clean.
  - `pre-release-checks.py`: its version list, and `library/package.json` and `library/README.md` in `claim_paths`.
  - `utilities/website.py`: an npm lookup and a `PAGES` row.
- New scripts:
  - `node_toolchain.py` plus its pin file (phase 0)
  - `ts_fma_check.py` plus `ts_fma_check/golden.c` (phase 0)
  - `ts_package_check.py` plus `ts_consumer_check/` (phase 3)

### 4.5 CI

- Phase 1, in the same commit that makes Node a prerequisite of the job's command: `actions/setup-node` in the `nightly-jobs.yml` jobs `cross-language`, `xlang-hash`, `synth-gate` and `macos-cross-language`.
- Phase 3:
  - `pr-codegen-gate.yml`: a `ts-compiles` job with `actions/setup-node`, then `npm ci`, `tsc` over the library, the hand-written tests and the server, and `ts_fma_check.py --random=1000000`.
  - `nightly-jobs.yml`: a new `typescript` job: `build.py libraries --language=ts` on Node 22, 24 and 26 (an `actions/setup-node` matrix pinned to the majors of `node_toolchain.py`), then `ts_package_check.py`, then `ts_fma_check.py` with a date-derived seed. The `arm64-linux` narrowing job is unchanged.
  - `.github/dependabot.yml`: an npm entry for `/ta_codegen/output/ts`, with `target-branch: dev` and a `cooldown`.

## 5. Numerical semantics

The rule behind every row is the same: semantics come from the type pass, never from a name heuristic. The FMA site selection is the one exception. It calls `fma.rs` exactly as Java and C# do, heuristics included, because the fused sites must match C's.

| # | Hazard | Where | Emitter rule | Gate that catches it |
|---|---|---|---|---|
| H1 | Untyped expression | Any body | Generation fails | `generate` (loud) |
| H2 | Integer `/` yields a fraction | Lookbacks, auto-stabilization rules, median midpoints, DPO, ZLEMA, HMA, ... | `((a) / (b)) \| 0`. Compound `i /= k` expands to `i = ((i) / (k)) \| 0` | `ts_int_semantics_suite`; a lookback mismatch in `--codegen` |
| H3 | Integer `%` yields -0 | `today % 2` (HT family, MAMA), FRAMA, TRIMA, PERCENTILE | `((a) % (b)) \| 0` | `ts_int_semantics_suite` |
| H4 | Real to int, explicit or implicit | `(int)ceil`, `(int)floor`, `(int)sqrt`, `(int)DCPeriod`, the MAVP period; implicit at an int local, int return or int argument (none shipped today, and `gcc` does not warn on one) | `(x) \| 0`, which truncates and clears -0. Never `Math.trunc` alone, which keeps -0. Rely on the corpus's clamps for range | `ts_int_semantics_suite`; the HT stream legs |
| H5 | Integer -0 leaking into real arithmetic | Integer `Neg`, `Mul`, `Mod`, and any integer produced from a real | Wrap each such producer in `\| 0`. Integer outputs, rings and scratch are `Int32Array` | `ts_int_semantics_suite` counts them; xlang-hash raw bits (reached sites only) |
| H6 | Bool used as a number, or a number tested for truth | Candle helpers, `!ptr`, `!TA_IS_ZERO` | A Bool used as a value becomes `(c ? 1 : 0)`. A non-Bool condition gets an explicit `!== 0`. Reuse `is_boolean_expr` and `bool_ternary_collapse` | `tsc --strict`; an assert in the type pass |
| H7 | Real max/min | `max(`/`min(`, `fmin`/`fmax` in the candle helpers, running extremes | `a > b ? a : b` and `a < b ? a : b`, in C's operand order. `Math.max` and `Math.min` differ from C on NaN and on ±0 | Text gate in `ts_render_statement_suite`: no `Math.max(`, `Math.min(`, `Math.round(` or `>>>` in library output |
| H8 | Shifts and masks | TRIMA `>> 1`, `& (live-1)` | As written. JS bit operations are int32, like C `int` | Same text gate |
| H9 | Fused-site selection | Every site C fuses | `build_fma_var_sets`, `fuse_operands` and `canonicalize_accumulator_add`, called where `java.rs` calls them. Copy the compound rule of `c.rs` `assign()`: `x += a*b` is not fused and `x = x + a*b` is | xlang-hash and server_verify, bitwise |
| H10 | Stream and batch fuse different sites | Stream step and peek bodies | `NameMap` state and output names keep the `sp.` / `cur_` prefixes that `fma::stream_base` strips | stream_verify same-tier, bitwise |
| H11 | Software fma rounding | `runtime/fma.ts` | Phase 0 | `ts_fma_check.py`, then xlang-hash on every fusing function |
| H12 | Transcendental results | Flagged functions; MAMA or ALMA through an MA-type parameter | Engine `Math.*` | The transcendental lane at 1e-11 (D11) |
| H13 | Literal text | Every real literal | Shortest round-trip text (Rust `{:?}`), never a fixed precision | xlang-hash |
| H14 | Out-of-bounds typed-array access | Every body | The public tier checks every length before `<n>Impl`. A read past the end gives NaN and a write is dropped, both silently | Generated phantom-I/O sweep, which passes Proxy-guarded arrays that throw on an out-of-range index and range-check the index arguments of `set`, `subarray`, `fill` and `copyWithin`; `out_tail_dirty` for writes into the slack |
| H15 | Aliasing views | The OpenAndFill and batch public-tier guards; C's `inReal == outReal` scratch election | Overlap means the same `buffer` with intersecting byte ranges. C pointer equality means the same `buffer` and the same `byteOffset`. Never `===`. The batch public tier, as C#'s, rejects two outputs that overlap and an output overlapping a converted input, unless the output is that input in place: a `Float64Array` of the same `buffer`, `byteOffset` and `length` | stream_verify aliasing rejects; library tests with `subarray` views |
| H16 | Non-`Float64Array` inputs | Public tier, `Open` | Copy into a `Float64Array` on entry (D5) | The float leg (a `Float32Array` through the public tier); library test: a `number[]` of numbers and a `number[]` of numeric strings give the `Float64Array` bits on every function |
| H17 | Non-integer or out-of-int32 integer parameter; non-number real parameter | Public tier, `Open` | Reject before the range checks, keeping the `INT_MIN` and `-4e37` sentinels; the lookback answers -1 | Library test; `open_validation_suite` arm |
| H18 | Bit access on a big-endian host | The fma helper, the server codec | `DataView` with explicit endianness, or a one-time check that picks the word order | `ts_fma_check.py`; server_verify |

**Structural gate (`ts_int_semantics_suite`).** It runs the type pass over every shipped function's batch, open, step and peek bodies and their helpers. Its axes:

- Each `Div` or `Mod` whose result is `Int32`, each `Cast` to `Integer`/`Index` of a `Real`, and each implicit real-to-int conversion carries the truncating form.
- Each integer `Neg`, `Mul` and `Mod` feeding a real context or a store is wrapped.
- Converse: no `Div` whose result is `Real` carries `| 0`.

Each axis has one non-vacuity assert (found more than zero times); the implicit axis asserts that its sweep visited more than zero int locals, returns and arguments, since no shipped site exists. The suite takes its verdict from the same type pass the emitter uses, so prove it red in both directions before relying on it:

1. Drop `| 0` from the emitter's integer-division arm; the suite fails.
2. Force one known real division (for example SMA's `sum / period`) to `Int32` in `ctype.rs`; the suite fails.

## 6. Phases

All phases share one branch, and each phase ends with every local gate green. Run heavy commands under `scripts/quiet.py noisy <session> --defer=60 -- <cmd>`. Every timing run goes under `scripts/quiet.py measure`. The gates below omit those wrappers.

Hosts: every non-timing job, the website build included, runs on the orchestrator, in parallel where independent, except the two that need a worker's own platform (phase 0 item 6, phase 4 item 6's macos-1 leg). Never time on the orchestrator. amd-1, intel-1 and macos-1 run timing only, and are often held by another set. Claim a host through the other set orchestrators (none holds it; announce the hold; send `TIMING DONE <host>` when the window ends), and queue with `quiet.py measure --queue=900` in short windows; do other work while a host is held or on exit 75. amd-1 is the most contended. Never ask the owner for a timing window. Worker hosts never install Node; a job takes it from `node_toolchain.py`, with `TA_NODE_CACHE` and `npm_config_cache` in the job's scratch directory and every path absolute.

Three command sets recur:

- **G**, every step of `pr-codegen-gate.yml`:
  ```
  python3 scripts/build.py regen-check
  cargo test --doc -p ta-lib --manifest-path ta_codegen/output/rust/Cargo.toml
  cargo test --no-fail-fast --manifest-path ta_codegen/generator/Cargo.toml
  cargo clippy --all-targets --manifest-path ta_codegen/generator/Cargo.toml -- -D warnings
  RUSTDOCFLAGS='-D warnings' cargo doc --no-deps -p ta-lib --manifest-path ta_codegen/output/rust/Cargo.toml
  cargo clippy --all-targets --manifest-path ta_codegen/output/rust/Cargo.toml -- -D warnings
  cargo test --tests -p ta-lib --manifest-path ta_codegen/output/rust/Cargo.toml
  python3 scripts/build.py && python3 scripts/abi.py check --require-artifact
  ```
  plus the `java-compiles` steps (`javac` of the library with doclint, `check_java_inline_budget.py`, the suites, the server) and the `csharp-compiles` steps (`dotnet build` of `TALib.csproj`, the test project and the server), copied from the workflow, and from phase 3 the `ts-compiles` job. `regen-check` skips dirty paths, so also run `scripts/build.py generate` twice and confirm that the second run changes nothing in `git diff`.
- **T**, the TS compile set:
  ```
  cd ta_codegen/output/ts && npm ci
  npx tsc -p library --noEmit && npx tsc -p library/test --noEmit && npx tsc -p tools --noEmit
  ```
- **U**, other backends untouched, after `build.py generate`:
  ```
  BASE=$(git merge-base HEAD origin/dev)
  git diff --exit-code $BASE -- src/ta_func src/ta_abstract include ':!include/ta_common.h' ta_codegen/output/rust ta_codegen/output/java ta_codegen/output/csharp
  ! git diff $BASE -- include/ta_common.h | grep -E '^[-+][^-+]' | grep -vq TA_LIB_SOURCES_DIGEST
  ! git diff $BASE -- website/src/functions ':!website/src/functions/index.md' | grep -E '^[-+]' | grep -Evq '^(\+\+\+|---) |TypeScript'
  ```
  The function pages may differ only by the added TypeScript row. A failure is not a red gate: it names the backends phase 4 must time, and section 8 records why their output changed.

Every phase end also checks that each file `grep -li csharp ta_codegen/generator/tests/all/*.rs` lists has a TS arm, or a reason in section 8 (naming the phase that adds the arm, if one does).

### Parallel tracks and collision points

Run independent tracks as subagents, each in its own scratch worktree at a named SHA, returning a patch:

- phase 0: `fma.ts` | `golden.c` and `ts_fma_check.py` | `errors.ts`, `core.ts`, `outRange.ts` | `node_toolchain.py`
- phase 1: `ctype.rs` and its suite | `fuzzData.ts` and the server codec | the harness C edits | scripts and CI | `ts_enums.rs` and `ts_phantom_io.rs`
- phase 3: metadata | docs | packaging | website | runbook

Only the orchestrating session edits `ir.rs`, `backends/mod.rs`, `main.rs`, `tests/all/main.rs` and the dispatch parts of `server_gen.rs` and `test_codegen.c`, and only it runs `generate` and `build.py servers` in the branch worktree, after each merge.

### Phase 0: runtime helpers

Status: Done (2026-11-01)

**Deliverables**

1. `ta_codegen/output/ts/` tooling root:
   - pin the newest stable `typescript`, and `@types/node` at the lowest supported Node major, in the lockfile
   - add `.gitignore` entries for `node_modules/` and `library/dist/`
   - add `library/tsconfig.json` (D4)
2. Hand-written `library/src/runtime/`:
   - `fma.ts`
   - `errors.ts` (`TALibError`, `RetCode`)
   - `outRange.ts`
   - `core.ts` (`Core`, `CoreBuilder`, `CandleSetting`, settings enums)
   - `bits.ts` (DataView helpers)
3. `scripts/ts_fma_check.py` and `scripts/ts_fma_check/golden.c`:
   - The C side calls libm `fma`, which is correctly rounded on every platform, so the goldens are machine-independent.
   - It streams triples and results to a Node checker.
   - Hard-case families:
     - ties
     - near-total cancellation (`c = -RN(a*b)` ± k ulp)
     - products near overflow whose result is finite
     - finite `a*b` that overflows double with finite `c` (result ±Inf)
     - `c` near ±DBL_MAX with `c + a*b` at the overflow rounding boundary
     - subnormal results and subnormal error terms
     - `fma(±0, x, ±0)` signs
     - Inf·0
     - a finite product with an infinite `c`
     - NaN operands
   - It also takes `--random=N --seed=S`.
4. `PREREQS_NODE` in `utilities/common.py`.
5. `scripts/node_toolchain.py <major>`, in the style of the Maven wrapper:
   - A committed pin file holds one exact version per major (22, 24, 26) and, per platform tarball (linux-x64, linux-arm64, darwin-arm64), its SHA-256 from nodejs.org `SHASUMS256.txt`.
   - On first use it downloads the official tarball, verifies it, and unpacks it into a cache: `~/.cache/ta-lib/node` by default, `TA_NODE_CACHE` when set. A worker job always sets `TA_NODE_CACHE` to its scratch directory.
   - It prints the version's `bin` directory. A checksum mismatch deletes the download and exits non-zero.
6. A job on intel-1 and on macos-1 runs `node --version; npm --version`; section 8 records the answers. Hosts without a system Node `>=22.18` go into the landing report, because an unfiltered `build.py servers` there fails once TS lands.

**Algorithm guidance for `fma`**

- **Fast path.** Dekker TwoProduct with a Veltkamp split, TwoSum, then round-to-odd of the low sum followed by one round-to-nearest add (Boldo and Melquiond). It is exact only inside this window:
  - the split cannot overflow (`|a|`, `|b|` below about 2^996)
  - the error term is normal (product exponent at least about -969)
  - the product exponent is below the overflow bound, so `RN(a*b)` is finite
  - `|c| + |RN(a*b)|` stays below the overflow threshold, so TwoSum cannot round to Inf
- **Other cases.** Send every case outside that window to a simple exact path: BigInt scaled integers, rounded once to nearest even. It can be slow, because indicator data rarely reaches it.
- **No allocation.** Use module-level scratch for the bit access.

**Gotchas (silent)**

- A naive `p + e + c` double-rounds on ties.
- With finite `a` and `b` and an infinite `c`, the result is `c`, even when `a*b` overflows in double.
- An exact cancellation gives +0, unless both addends are -0.
- Reading the bits through a `Float64Array`/`Uint32Array` alias depends on platform endianness (H18).
- Compare NaN results by NaN-ness only.

**Exit gate**

```
cd ta_codegen/output/ts && npm ci && npx tsc -p library --noEmit
python3 scripts/ts_fma_check.py --random=100000000 --seed=1
python3 scripts/ts_fma_check.py --random=100000000 --seed=2
for v in 22 24 26; do "$(python3 scripts/node_toolchain.py $v)/node" --version | grep -q "^v$v\." || echo "node $v FAILED"; done
```

Also:

- G passes, and the loop prints nothing.
- Red proof, recorded in section 8: in a scratch copy with an empty `TA_NODE_CACHE`, corrupt one pin's SHA-256 and confirm `node_toolchain.py` exits non-zero and leaves no unpacked tree.
- Red proof, recorded in section 8: replace the round-to-odd step with round-to-nearest, confirm the check fails on the tie family, then restore it.

### Phase 1: batch tier, server, cross-language gates

Status: Done (2026-11-02), exit gate run together with phase 2's

**Deliverables**

1. The registrations in 4.1 except `build_ts_library`, the harness rows in 4.3 except the phase 2 and phase 3 items, the phase 1 scripts in 4.4, and the phase 1 setup-node steps in 4.5.
2. `ctype.rs` plus `ts_int_semantics_suite`, proven red in both directions.
3. `ts.rs`, `ts_enums.rs`, generated `library/package.json` and `index.ts`. The per-function modules hold lookback, display shift, `<n>Impl` and the public tier. The argument-check order is Java's, with the D13 integer check first.
4. The `ts_phantom_io.rs` sweep, the H16 input test and the aliasing-view test, all run by `node --test`.
5. The server:
   - batch handlers, including `use_float`
   - `list_functions`
   - the three settings methods, with every check made before any write
   - `eval_predicate`
   - `TA_GetRuntimeInfo`
   - `fuzz_in_hash`
   - `load_data`
   - `stream_verify`, answering `{"error":"not_streamable"}` for now
   - `bench_mode` 0 only
6. TS arms in the batch-shaped generator suites (`render_features_suite`, `display_shift_suite`, `enums_suite`, `cond_comment_suite`, `arg_order_suite`, `alt_suite`, `naming.rs`), plus `ts_render_statement_suite`.
7. Any suite that iterates `backends::all()` over stream text excludes TS with a `TS-PHASE-2` marker.

**Gotchas (silent)**

- **Skipped stream and ride passes.** A server that answers `not_streamable` and sends no `ride_*` fields makes ta_regtest skip the stream and ride passes for that language and still report green. That is expected here, and phase 2's gate closes it.
- **Fuzz port.** The fuzz port must already be bit-exact; the zero-shape failure in 4.3 makes a missing `in_hash` loud.
- **Missing abstract legs.** There is no abstract block until phase 3, so the abstract legs are absent for TS. Do not read their absence as a pass.
- **`TA_ALT` alternates.** TS must take every `ALL_LANGUAGES` alternate and no Java or C# one. An entry point that skips `resolved_for(Lang::Ts)` gets both wrong.
- **Stale library.** The TS server compiles `library/src`, and `build.py servers` regenerates only the server. After any edit to an input `.c` or a generator module, run `build.py generate` and then `build.py servers --language=c,ts` before any `--codegen` or `--xlang-hash` run or red proof. An unwiped `bin/ta_codegen_ts` passes every gate with old code.
- **synth_gate snapshot.** It tests HEAD plus tracked edits, so run it after the new files are committed and check that its `testing tree <sha>` line names that commit.
- **Lane coverage.** Confirm that the transcendental lane covers only D11 calls: every other TS call in the xlang-hash report must be counted as bitwise.

**Exit gate**

G and T pass, and:

```
python3 scripts/build.py generate
cd ta_codegen/output/ts && npx tsc -p library && node --test 'library/test/**/*.test.ts'
python3 scripts/build.py servers --language=c,ts
cd bin && ./ta_regtest --codegen --language=c,ts
cd bin && ./ta_regtest --xlang-hash --language=c,ts
out=$(python3 scripts/build.py regtest) && grep -q 'Codegen verification: TypeScript' <<<"$out"
cd bin && ./ta_regtest --xlang-hash
env -u TA_CODEGEN_ROOT python3 scripts/synth_gate.py
```

Also:

- The TS section of the `--codegen` output (from `Codegen verification: TypeScript` to the next `Codegen verification:` line) prints `Fuzz-port self-check: N/N shapes bit-identical` with N above zero.
- U is run and its result recorded in section 8.

### Phase 2: streaming tier

Status: Done (2026-11-02)

**Deliverables**

1. `ts_stream.rs`: handle classes, the `_Open*` family, sinks and tape frames. Add `<n>Open` / `<n>OpenAndFill` and `bench_mode` 1/2 to the server.
2. `ride_gen/ts.rs` and `stream_verify_gen/ts.rs`, wired into the server.
3. The phase 2 harness items in 4.3: TS in `check_stream_counter_parity`, and the zero-streaming-functions floor, proven red.
4. TS arms in:
   - `ride_along_suite` (`COMPARE_NEEDLE`, `servers()`, and the throwing-backend probes, which compare `code` for TS)
   - `streaming_dispatch_suite` (`sv_range_sites_mask_matches_the_declared_set`, `the_transition_tier_is_step_impl_in_every_backend`)
   - `managed_stream_suite`, including both no-write sweeps (`no_managed_tape_peek_writes_the_handle_or_the_tape`, `no_managed_peek_writes_its_handle`), each with its swept-count and handle-read non-vacuity asserts
   - `open_validation_suite`, `out_range_advance_suite`, `candle_range_suite`
   - the new `ts_stream_suite`
   - a TS row in `check_stream_retcodes.py`
5. Remove every `TS-PHASE-2` exclusion.

**Gotchas (silent)**

- **Prefixes.** The H10 prefixes: any other spelling makes stream and batch fuse different sites.
- **Field visibility.** Handle fields are public and `@internal` (D16). Neither TS `private` nor `#private` works: the module-local frames could not reach them.
- **Peek writes.** No runtime leg catches a peek that writes the handle or a ring. stream_verify peeks bar t and then updates bar t with the same arguments, so a peek that stores what update would store leaves identical state. There is no compile-time barrier either. The `managed_stream_suite` no-write sweeps are the whole gate.
- **`clone()`.** It must `.slice()` every typed array and clone every sub-stream. The clone leg catches a shared ring.
- **`update` order.** As Java: the `INDEX_MAX` ceiling, then the multi-output sink check, then the finite-bar check, then `<n>StepImpl`, then the count.
- **Ride cache.** Key it by function identity, and invalidate it on every settings change (`rideGen`).

**Exit gate**

- Phase 1's exit gate passes.
- The TS section of the `--codegen --language=c,ts` output (and of an unfiltered `--codegen` run) shows:
  - a `ride-along:` line
  - no `Stream verify: not supported` line
  - no `STREAM COUNTER MISMATCH` line naming TS
  - no `VACUOUS` line
- The new zero-streaming-functions floor is in place, and its red proof is recorded in section 8.
- `grep -rn TS-PHASE-2 ta_codegen/generator` prints nothing.

### Phase 3: metadata, docs, packaging, CI, website, runbook

Status: Done (2026-11-02)

**Deliverables**

1. **Metadata.**
   - `ts_metadata.rs`, `getRuntimeInfo` and `FunctionDescription`. The server answers every abstract RPC from the shipped registry.
   - The `ta_regtest.c` abstract-parity block for TS, printing `  abstract parity (TypeScript): <N> functions` on success. Without a `--language` filter a missing server only prints a skip, so the gate greps this line under one.
   - A TS arm in `indicator_variants_suite::metadata_price_setter_validates_before_writing`. The price setter validates every component before it writes any.
2. **Docs.**
   - `ts_doc.rs`, with the example tests generated into `library/test/gen/`.
   - A generator test that every public declaration the TS emitters write carries a TSDoc block (the CS1591 analogue).
   - The example tests import the package by its own name (package self-reference), so a rename (O1) is one constant.
   - TS arms in `spec_catalog_suite`, `abstract_rows_suite`, `doc_emphasis_suite` and `circbuf_suite`.
3. **Packaging.**
   - Final `library/package.json` fields: `name`, `description` (containing `200+ indicators`), `exports`, `types`, `sideEffects`, `files`, `engines`, `license`, `repository`.
   - A hand-written `library/README.md` containing `200+ indicators` (the only place that carries install lines until the first publish).
   - `LICENSE` copied.
   - `build_ts_library`, `LIBRARY_PREREQS["ts"]` and the `build_libraries` default-list entry, landed together: `npm ci`, `tsc` to `dist/`, `npm pack`, install the tarball into a throwaway project, then run against it the tests that import `ta-lib` by self-reference (the example tests and the public-API tests). The phantom sweep and other tests that import `../src` stay source-tree tests, run by the phase 1 `node --test` glob.
   - `build.py libraries --language=ts --node=22,24,26` (or `all`) runs the suite once per version with `node_toolchain.py`'s `bin` first on PATH.
   - `ts_package_check.py`:
     - the exact tarball entries
     - byte-identical `LICENSE` and README
     - version equals `VERSION`
     - a consumer that loads the package by `import` and by `require()` (which proves there is no top-level await)
     - no `@internal` name in the `.d.ts` files or the runtime barrel
     - a diff of the packed declarations against a committed API snapshot taken at each publish (the `PublicAPI.Shipped.txt` analogue). Until the first publish the snapshot is absent and the diff is skipped; the publish runbook creates it.
4. **Versions.** The phase 3 `versions.py` and `pre-release-checks.py` entries in 4.4. `website.py` gets `latest_npm_version()`: a 404 answers `NOT_PUBLISHED` and deprecated versions are skipped. It also ignores 0.9.0, 0.10.0 and 0.11.0, the previous holder's versions (O1), as `latest_crate_minor` ignores the old crate; with nothing left it answers `NOT_PUBLISHED`. A unit check feeds it a packument holding only those three and expects `NOT_PUBLISHED`.
5. **CI** as in 4.5 phase 3, and the dependabot entry.
6. **Website.**
   - New pages: `website/src/api/typescript/README.md` and `api/typescript/stream/README.md`.
   - Updated pages:
     - sidebar `en.ts`
     - `llms.ts` and `llms-key-facts.md`
     - homepage and `sma-example.md` tabs
     - install page
     - the `code-tabs#lang` blocks in `api/abstract`, `api/candle-settings` and `api/unstable-period`
     - spec pages: rD1 and the transcendental paragraph in `spec/versions` (TS takes them from the JS engine), the errors table, `spec/streaming`, `spec/abstract`
     - `website/src/functions/index.md`, generated by `build_index()`
7. **Repo docs.**
   - Root `CLAUDE.md`: the backend count, the toolchain per backend and the API-tier text.
   - `src/tools/ta_regtest/CLAUDE.md` and `ta_codegen/generator/CLAUDE.md`.
   - A TS column in `docs/naming-spec.md`.
   - The per-language shape in `docs/streaming-api-design.md`.
8. **Runbook.**
   - `docs/release-runbooks/typescript-publish-runbook.md`, in the house shape: step ids `TP1...`, and pre-flight `build.py libraries --language=ts`, `servers --language=c,ts`, `--codegen` and `--xlang-hash --language=c,ts`, and `ts_package_check.py`.
   - Then `npm publish --dry-run` as the last reversible step, then `npm publish`.
   - Then verify from a clean cache, record the API snapshot, and tag `ts-v<version>`.
   - Treat a published version as immutable.
   - Add a line to `README.md` step 2 and a mention in `post-publish-runbook.md`.

**Gotchas (silent)**

- **Abstract block.** The abstract block is the coverage: without it every abstract leg short-circuits green.
- **Lockfile.** The tooling root has no workspaces and the library has no dependencies, so the lockfile never records the library version. Otherwise `sync.py` would churn it.
- **Barrel.** An `export *` in the barrel leaks `@internal` names at runtime, because `stripInternal` strips types only. The barrel must carry explicit lists.
- **Lookup failure.** A website lookup that returns None instead of `NOT_PUBLISHED` makes `sync-website.py --check` exit 2 every night.
- **Docs text.** No `npm install` line on the website before the first publish. No corpus counts in any doc.

**Exit gate**

G and T pass, and:

```
python3 scripts/build.py libraries --language=ts --node=all
python3 scripts/ts_package_check.py
python3 scripts/build.py servers && cd bin && ./ta_regtest --codegen
cd bin && out=$(./ta_regtest --codegen --language=c,ts) && grep -Eq 'abstract parity \(TypeScript\): [1-9]' <<<"$out"
cd bin && ./ta_regtest --xlang-hash
a=$(git diff) && python3 scripts/sync.py && [ "$a" = "$(git diff)" ]
python3 -c "import sys; sys.path.insert(0,'scripts'); from utilities.versions import check_versions; sys.exit(0 if check_versions('.') else 1)"
grep -q '200+ indicators' ta_codegen/output/ts/library/package.json && grep -q '200+ indicators' ta_codegen/output/ts/library/README.md
python3 scripts/sync-website.py --check
cd website && npx pnpm@11.2.2 install --frozen-lockfile && NODE_OPTIONS=--max_old_space_size=8192 npx pnpm@11.2.2 run docs:build
actionlint                                  # on a ./ copy of the changed workflows
```

Also:

- `actionlint` is a pinned release fetched into the session scratchpad with `gh release download <tag> --repo rhysd/actionlint --pattern '*linux_amd64.tar.gz'`, or the `rhysd/actionlint` docker image run with `--memory`.
- `pre-release-checks.py` itself runs only in a release: it needs `dist/` assets and a fresh CHANGELOG entry. Never build `dist/` or touch `CHANGELOG.md` to make it pass.
- In a scratch worktree, change `VERSION` and run `sync.py`, then `a=$(git diff) && python3 scripts/build.py generate && [ "$a" = "$(git diff)" ]` succeeds.
- U is run and its result recorded in section 8.

### Phase 4: performance pass and adversarial review

Status: Done (2026-11-02)

**Deliverables**

1. Load the `ta-bench` skill first. Phase 4 never blocks landing: its timings inform D3 and D5, which stay Proposed until measured.
2. Measurements:
   - **Where:** amd-1 and intel-1, with macos-1 as an extra data point (host rules in section 6). Every host times the same pinned Node 22 from `node_toolchain.py`.
   - **Schedule:** dispatch the timing jobs at a pinned commit as soon as phase 3 is green, and run item 5 meanwhile; run item 6 once every item 5 fix is committed. Split each host's run into short `quiet.py measure` windows by `--function` lists under 400 characters (longer ones truncate silently), each carrying C SMA and EMA control rows; check that each window's row count matches its list. Read the controls first, and re-queue once a window whose C SMA or EMA median falls outside that row's range in the host's first window.
   - **intel-1:** taskset a P-core. `ta_bench` uses the skill's `--iters` (200 at 100k bars); the 2000-iteration floor is for `ta_bench_stream` only.
   - **Format:** each figure as "X% slower/faster than C (a ns vs b ns)", with range and median and the host named.
   - **What:**
     - **Batch:** every function under `ta_bench --language=c,rust,ts`, at 2000 and 100k bars. Rust is timed through its public tier and TS through the body; state this next to the numbers.
     - **Software fma:** its cost on the fusing functions, against a measurement-only build where the helper is `a*b + c`. That build is never shipped, because its values differ.
     - **D5:** the cost of the entry copy for `number[]` and `Float32Array` inputs, against a `Float64Array` input.
     - **D3:** barrel import time against a per-function subpath.
     - **Streaming:** the C-relative figure comes from `ta_bench --language=c,rust,ts --mode=open` and `--mode=openfill`. Per-bar `update` and `peek` have no cross-language tool: a TS arm in `scripts/stream_ab.py`, with a commit that already has TS streaming as `--base`, reports only the relative change of a phase 4 edit and is never compared with C.
     - **JIT tiers:** functions that never reach the optimizing tier (`node --trace-opt --trace-deopt` under `ta_bench`), the analogue of Java's inline budget. Not a timing job: run it on the orchestrator with the pinned Node 22.
3. Record each result in the decision row it settles (D3, D5), and in section 8.
4. If U shows another backend's output changed, time every such backend under the same rules.
5. Adversarial review of the whole branch:
   - refute by default, with fan-out sized to the diff
   - findings merged by file:line
   - every fix re-reviewed
   - `/code-review` given two explicit SHAs
6. Landing round (O4), from a fresh `git worktree add` of the landing SHA in a scratch path (no `node_modules`):
   - the `ts-compiles` steps, `build.py libraries --language=ts --node=all`, `ts_package_check.py` and `ts_fma_check.py --random=1000000 --seed=<date>`
   - the macos-cross-language commands as a build job on macos-1, under `quiet.py noisy`: `build.py servers --language=c,ts`, `ta_regtest --codegen --language=c,ts` from `bin/`, `build.py xlang-hash --language=c,ts` and `build.py libraries --language=ts`
   - a leg that did not run is reported as not run, never as passed

**Gotchas (silent)**

- A one-bar batch call is not a target.
- Never tune for monotonic or held series.
- Composed functions page-fault their scratch at 100k bars, so read the 2000-bar rows too.

**Exit gate**

- Item 2's table exists for every function at 2000 and 100k bars on each host that ran it; for each host still held when everything else is ready, section 8 says `not measured on <host>`. The same rule covers item 4.
- Every review finding is fixed or refuted in writing.
- The gates of phases 0 to 3 pass at the final SHA, and item 6 is done.
- Report to the owner with the branch ready to land: the measurements, the hosts not measured or not run, and the hosts without a system Node `>=22.18` (phase 0). Landing is the owner's call.

## 7. Open items

- **O1. npm name and first version.** Settled (owner, 2026-11-02): `npm owner ls ta-lib` lists `mario4tier`. Ruled (owner, 2026-11-03): no backend publishes a version above the latest Core release, so the first publish, by hand, is 0.8.3; from the next Core release on, npm follows `VERSION` like every other backend. The previous holder's 0.9.0, 0.10.0 and 0.11.0 are deprecated, never unpublished: the package has public dependents. The first publish is the runbook with these differences:
  - TP1 runs in a scratch worktree detached at a green main commit, not at `v0.8.3` (which predates this backend). Set `VERSION` to 0.8.3, run `build.py generate`, check that `git status --porcelain` lists only `VERSION` and the three generated manifests (two `Cargo.toml`, `package.json`), and commit them locally; never push that commit. The package gate then runs without `--allow-dirty`, which would also pack any stray edit. The TS package embeds no version outside `package.json`, so the tarball is the main commit's code.
  - After TP4, deprecate the three by exact version, never a bare `ta-lib`:
    ```bash
    for v in 0.9.0 0.10.0 0.11.0; do npm deprecate "ta-lib@$v" "An earlier, unrelated package. The official TA-Lib library: npm install ta-lib@latest"; done
    for v in 0.8.3 0.9.0 0.10.0 0.11.0; do echo "$v: $(npm view "ta-lib@$v" deprecated)"; done
    ```
    0.8.3 must print nothing and each of the other three the message: a mistyped one-time password fails one iteration and the loop goes on.
  - Then refuse tokens: `npm access set mfa=publish ta-lib`. The package's Settings page on npmjs.com must then show Publishing access as "Require two-factor authentication and disallow tokens". The name came with its previous holder's setting, and without this any write token on the account can publish it.
  - TP5 runs in that worktree. TP6 tags the main commit, the local commit's parent, `ts-v0.8.3`.
  - Then, on dev, the website commit: drop the "Not yet released" banner from both TypeScript API pages, replace 2.0's build-from-source text with `npm install ta-lib@0.8.3`, and add a TypeScript row to the README's install table. `sync.py` can do none of it: it only rewrites an `npm install ta-lib@X.Y.Z` line that exists, and `sync-website.py --check` passes while none does. Then `merge.py`, whose push to main deploys the site.
- **O2. Transcendental lane (D11).** Ruled (owner, 2026-11-01): 1e-11 for TS, held in one constant. The owner started at 1e-13 and loosened it once the phase 1 maxima showed `Math.*` against glibc reaching 9.6e-12 (SWAK_BP; section 8). Anything above 1e-11 goes back to the owner. Every call outside the lane stays bitwise. Java and C# stay at 1e-9. It does not touch the exact-FMA ruling.
- **O3. Publish credentials.** The first publish is by hand, from the owner's npm account with 2FA, once O1 settles; nothing before it waits on this. Trusted publishing (GitHub OIDC naming one workflow file) with `--provenance` is a later choice. Until then the TS backend ships only through dev and main. Ruled (owner, 2026-11-01).
- **O4. Landing.** Commit per phase on one branch; land on dev once, as one block, after phase 4 and a last review and test round showing nothing else broke. No nightly sees the branch before then, so that round runs the TS CI jobs (4.5) locally. Ruled (owner, 2026-11-01).

## 8. Status log

| date | phase.deliverable | SHA | gates passed | open failures / owner questions |
|---|---|---|---|---|
| 2026-11-01 | 0.1-0.2 | 4e0ea74b4 | `npm ci`, T, `node --test` (10/10); `typescript` 7.0.2 (per-platform compiler packages for linux-x64, linux-arm64, darwin-arm64 in the lockfile), `@types/node` 22.20.5 | `node --test <dir>/` fails on Node 22; run it with a quoted glob or from inside `library/` |
| 2026-11-01 | 0.3 | 3049b3f1f | `ts_fma_check.py --random=100000000` seeds 1 and 2: 111,000,000 cases each, 0 mismatches. Red proof: round-to-nearest in place of round-to-odd fails `ties-sticky` (3,327 of 1,000,000), every other family 0 | none |
| 2026-11-01 | 0.4-0.5 | 811a836f5 | pins 22.23.4, 24.22.2, 26.13.0 (`.tar.gz`); the exit loop prints nothing. Red proof: one corrupted linux-x64 SHA-256 with an empty `TA_NODE_CACHE` exits 1 and leaves the cache empty | linux-arm64 and darwin-arm64 pins not yet exercised |
| 2026-11-01 | 0.6 | a4d3347ae | intel-1: Node v20.19.3, npm 10.8.2 (nvm). macos-1: Node v22.3.0, npm 10.8.1 (nvm). Orchestrator: Node v22.23.3 | both workers are below 22.18: landing report |
| 2026-11-01 | 0 exit | 3049b3f1f | G (every pr-codegen-gate step), T, `generate` twice with no diff | none |
| 2026-11-01 | 1 (lane) | dfa42f702 | TS lane maxima over 1e-13 in `--xlang-hash --language=c,ts` (seed 1793592318): APO 1.705e-13 (shape 2, seed 1, MAType ALMA), PPO 1.729e-13 (shape 6, seed 2, MAType ALMA), HT_DCPHASE 2.440e-13 (shape 0, seed 1), MACDEXT 2.911e-12 (shape 2, slow MAType ALMA), MAMA 3.184e-12 (shape 9, 0.21/0.05), KSTEXT 7.941e-12 (shapes 6 and 9, ROCMAType MAMA), SWAK_BP 9.604e-12 (shape 5, 21/0.1 and 23/0.1). With every `Math.*` replaced by glibc's result for the same argument, TS equals C bitwise on those parameter sets | Owner ruled 1e-11 |
| 2026-11-02 | 1, 2 exit | a0b80e392 | G; T; `node --test` (17/17); `build.py servers`; `--codegen` c,ts and unfiltered (TS: fuzz port 10/10, ride-along 8894 verdicts over 240 functions, no VACUOUS, no STREAM COUNTER MISMATCH, no "not supported"); `--xlang-hash` c,ts and unfiltered PASS; `build.py regtest` with the TypeScript section; `synth_gate` (5 languages); `check_stream_retcodes.py`; no `TS-PHASE-2`. U: no change to C, Rust, Java, C# or the function pages beyond the TypeScript row | Phase 1 could not exit alone: with no stream tier, the ride-along and stream-set floors fail rather than skip, so phases 1 and 2 share one exit run. TS arms still missing in doc_emphasis, abstract_rows, circbuf, spec_catalog, indicator_variants (phase 3); csharp_stream_suite is C#-only, its TS counterpart is ts_stream_suite |
| 2026-11-02 | 1, 2 (lane) | a0b80e392 | unfiltered `--xlang-hash` TS maxima: SWAK_BP 9.604e-12, KSTEXT 7.941e-12, MAMA 3.184e-12, MACDEXT 2.911e-12, all under 1e-11 | none |
| 2026-11-02 | 2 | a0b80e392 | zero-streaming floor red proof: every function skipped in `stream_one_function` fails with `STREAM VACUOUS ... streamed 0 functions` (exit 1111); with the floor disabled the same sabotage passes | none |
| 2026-11-02 | 3 exit | 4c7ac1459 | G with `ts-compiles`; phases 1 and 2 gates again; `build.py libraries --language=ts --node=all` (tarball suite on Node 22.23.4, 24.22.2, 26.13.0); `ts_package_check.py`; `abstract parity (TypeScript): 240 functions`; unfiltered `--codegen` and `--xlang-hash`; `sync.py` idempotent; `check_versions`; `200+ indicators` in package.json and README; `sync-website.py --check`; website build; unfiltered `build.py libraries`. Red proofs: abstract parity (a changed default), package check (an `@internal` re-export, a top-level await, a narrowed `files`), phantom sweep (five SMA sabotages), H16, H15, H17, the stream-sink order. U: unchanged | Phase 1 deliverable 4 (phantom sweep, H16, aliasing) landed in phase 3. The tests found two aliasing bugs, both fixed: SQZMOM and SUPERTREND accepted a real and an integer output at one address, and a multi-output stream counted a bar before rejecting an absent sink |
| 2026-11-02 | 4.2 JIT tiers | 4c7ac1459 | Node 22.23.4: every body, public function, `update` and `peek` reaches TurboFan at default parameters. Two costs found: after a long first call, 234 bodies deopt once per call at the out-parameter store after the loop until a normal compile lands (100 to 1275 calls); the software `fma` stays a real call past V8's cumulative inlining budget in 33 bodies (e.g. T3, MAMA, the HT family) | timing pending |
| 2026-11-02 | 4.2 timing | 5d995aac2 (4c7ac1459 plus a measurement-only probe) | `ta_bench --language=c,rust,ts`, every function, batch at 2000 and 100k bars, open and openfill at 2000, on intel-1 (P-core pinned), amd-1 and macos-1 (extra data point). TS is timed through the body, Rust through its public tier. Median TS vs C over every function: intel-1 152% slower at 2000 bars and 216% slower at 100k; amd-1 171% and 238% slower; macos-1 188% and 273% slower. Rust vs C: within 1% on intel-1 and amd-1. Worst row on every host: T3, e.g. intel-1 2764% slower at 100k (5,094,572 ns vs 177,911 ns) | Software fma is the dominant TS cost: against a measurement-only `a*b+c` build it costs more than 20% in 56 of 75 fusing functions on intel-1 and 61 on amd-1 (T3: 2173% on intel-1) |
| 2026-11-02 | 4.2 D5, D3 | 5d995aac2 | D5, `number[]` input through the public tier: intel-1 1.0 ns per bar at 2000 bars (SMA 94% slower) and 2.2 ns per bar at 100k (SMA 213% slower); `Float32Array` 70 to 75% of that. D3, median import time: barrel 118 ms (intel-1), 229 ms (amd-1), 99 ms (macos-1); one function module 4 to 8 ms; `./metadata` about the barrel's | D3 and D5 stay as designed: a per-function subpath would need an export that exposes the `@internal` tiers at run time (owner's call) |
| 2026-11-02 | 4 perf | 2ec63459d | A/B on intel-1 and amd-1 (adb0efdc5 against e2eb8b091 and 885d53735): the `fma` helper split into an inlinable fast path and `fmaSlow`, plus copying its body into loops with three or more fused sites, makes TS 6% to 10% faster by geomean on both hosts, the changed functions 29% to 40% (intel-1 STC 61% faster, 6,583,036 ns vs 2,537,990 ns at 100k). No changed function is more than 5% slower on both hosts. Accepted rows: PERCENTB open about 10% slower on both hosts (module unchanged, window dependent); ERI, EFI, ATR 3% to 5% slower on intel-1 only. A one-time warm-up of every body was rejected: SAREXT 18% to 67% slower on both hosts in every leg, and a repeatable intel-1 slow mode at 100k (BBANDS 177% slower) | Both merged (2ec63459d, ff012f47f) under D9's 2026-11-02 wording |
| 2026-11-02 | 4.6 macOS leg | 9024634e1 | macos-1, fresh worktree: darwin-arm64 pins for 22, 24, 26 verify; `build.py servers --language=c,ts`; `--codegen --language=c,ts` (240/240, abstract parity 240, fuzz port 10/10); `xlang-hash --language=c,ts` PASS with the Linux maxima; `build.py libraries --language=ts --node=all` (724/724 on each); `ts_package_check.py`; `ts_fma_check.py --random=1000000` | rerun at the landing SHA |
| 2026-11-02 | 4.2 vs Java, C# | c231b55f9 | `ta_bench --language=c,java,csharp,ts`, every function, batch at 2000 and 100k bars, intel-1 and amd-1. TS median against Java: 115% slower at 2000 bars and 82% slower at 100k on intel-1, 133% and 66% slower on amd-1. Every function more than 5 times Java's time runs exact software `fma` on a loop-carried chain (T3 at most: intel-1 3,160,190 ns vs Java 178,671 ns at 100k). Among functions that never reach `fma`, the slowest is CRSI at about 4.5 times Java, through PERCENTRANK's count loop | The `fma` chain is D9's price. The count loop is fixed below |
| 2026-11-02 | 4 perf | 50d3eeba1 | An else-less `if` that only adds one to an integer local renders as `x += +(cond)`. A/B on intel-1 and amd-1 against c231b55f9, batch: PERCENTRANK 21% and 35% faster at 100k (intel-1 11,842,046 ns vs 9,337,971 ns), CRSI 53% to 54% faster, CCI flat. Stream `update`: CRSI 40% faster, PERCENTRANK 19% and 34% faster. `stream_ab`'s `peek` read PERCENTRANK and CRSI 10% to 12% slower on both hosts: it peeks a handle that never advances, so the branch predictor learns the fixed window. Peeking then updating a moving 100k-bar feed reads PERCENTRANK 13% and 16% faster and CRSI 46% to 49% faster (intel-1 303 ns vs 263 ns) | Merged |
| 2026-11-02 | 4.5 coverage parity | c231b55f9..0e51a6cd7 | Adversarial audit of TS test reach against Java, C# and Rust: 23 findings survived refutation and 5 were refuted. Fixed: the TS lanes of every cross-backend generator sweep (peek copies, input checks, composed fusion, overlap guards, cross-typed aliasing, enum numbers, dead guards); library suites porting the batch, opener, stream and metadata contracts over every function, with an exact test manifest; checked typed arrays for `--codegen` and `--xlang-hash` (nightly), which found no out-of-bounds access in any function; a fail-closed transcendental table; nightly jobs for the engines floor and Windows. Behaviour fixes the new tests forced: the batch tier rejects a partial overlap as C# does; an opener names an absent history or leg; a partial overlap on a short history answers `TA_INSUFFICIENT_HISTORY`; `Core`, its settings and every exported enum and flag object are frozen; two vacuous C# overlap assertions now reach the guard. Three re-review rounds followed; the last found one doc clause too broad, now scoped | Every finding fixed |
| 2026-11-02 | 4 exit | 0e51a6cd7 | A/B against c231b55f9 on intel-1 and amd-1, every function, batch at 2000 and 100k bars, C as the control (median ratio 0.993 to 1.001): TS median unchanged on both hosts, geomean 0.4% to 0.9% faster; CRSI 53% faster, PERCENTRANK 15% to 35%. Rows that read up to 16% slower on one host (intel-1 100k: CDLSEPARATINGLINES, STOCHF, STOCHRSI; amd-1: CMO, CMOU, SAREXT, CMF, MAX at 2000) read within 4% on the other, and STOCHF's timed body is unchanged. Stream `update`, every function: median unchanged on both hosts. Stream `open` on 14 functions covering every opener shape: within 1% on intel-1 and 4% on amd-1, CRSI and PERCENTRANK faster. Landing round from a fresh worktree: `ts-compiles`, `build.py libraries --language=ts --node=all` (Node 22.23.4, 24.22.2, 26.13.0), `ts_package_check.py`, `ts_fma_check.py --random=1000000 --seed=20261102`, checked-array self-test. macOS leg on macos-1: servers, `--codegen` and `--xlang-hash` for c,ts (also under checked typed arrays), libraries, package gate, fma check. Phases 0 to 3 gates at this SHA, all 38 passing, then `--codegen` and `--xlang-hash` for c,ts under checked typed arrays | Done |
