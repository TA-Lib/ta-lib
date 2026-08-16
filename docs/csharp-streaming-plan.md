# C# streaming tier — implementation plan

The .NET streaming API is the last unbuilt piece of the streaming campaign.
C, Rust and Java each stream all 172 functions and are verified bit-exact
against their own batch tier by a generated in-server harness; C# has no
`csharp_stream.rs`, no stream section in `Core_<NAME>.cs`, and no
`stream_verify` handler. Its metadata catalogue nonetheless already publishes
`FunctionFlags.Stream` on all 172 functions — "a streaming API exists for this
function" is, in the C# package alone, currently false.

This is the execution runbook for closing that. It is written to be driven
without supervision: every stage names the command that proves it landed and
the sabotage that proves the proof is not vacuous.

**Working branch `csharp-stream`, worktree `/home/olet/ta-lib-csharp-stream`.**
Delete this file at S10; its durable content folds into
`docs/streaming-api-design.md`.

Related: `docs/streaming-api-design.md` (the cross-language contract, which
already obliges .NET to a managed emitter), `ta_codegen/generator/CLAUDE.md`,
`src/tools/ta_regtest/CLAUDE.md`, issue #84.

---

## 1. What gets built

A managed C# emitter — **not** a P/Invoke wrapper over the C handles. That
choice is already made (`streaming-api-design.md:294-300`): a P/Invoke tier
would own native memory and need `SafeHandle`/`IDisposable`, a worse API for a
worse reason.

`ta_codegen/generator/src/backends/csharp_stream.rs` (~3,300 lines), the
sibling of `java_stream.rs` (3,385) / `rust_stream.rs` (3,816) /
`c_stream.rs` (3,945). It consumes the same backend-neutral `streaming.rs`
layer — `StreamPlan`, `StreamModel`, `build_transition`, the 14-method
`NameMap` trait — and renders C# through the existing `csharp::render_*`
walkers. **`streaming.rs` and the three shipped stream emitters are not
touched.** That is a deliberate property, not laziness: it makes the other
three backends byte-frozen *by construction*, so the regeneration gate proves
rather than merely checks them while 3,400 new lines land.

Three facts make C# structurally easier than Java was:

- **No fragment splice.** `csharp.rs::generate` opens `public partial class
  Core {` and closes it in the same function; the stream section goes in
  before that closing brace of the existing per-function `Core_<NAME>.cs`.
  Java had to splice into one shared `Core.java` between GENCODE markers.
- **No server copy.** `TaCodegenServe.csproj` globs `../library/*.cs` and
  `../library/src/**/*.cs`, so the server *is* the shipped partial class. The
  Java server textually inlines its own copy of Core's methods. A C# stream
  section reaches the library, the server and the test assembly the moment it
  is emitted, with `internal` reach in all three.
- **Only four private items to promote** in `csharp.rs` (below). Signatures
  are already byte-parallel to `java.rs`'s.

The corpus is **172** functions. `docs/streaming-api-design.md` still says
161/165; S10 reconciles it.

---

## 2. API surface

### Naming

| concern | decision | why |
|---|---|---|
| handle | `public sealed class <NAME>_Stream`, nested in `partial class Core` | C# nested ≡ Java static-nested. `_Stream` for cross-language parity. CA1034/CA1707/CA1711 are a recorded waiver — verified none fire at the csproj's `AnalysisMode`. |
| multi-output value | `public readonly record struct <NAME>_Value(...)`, **sibling** nested type | CS0102: a nested type `Value` plus a member `Value` does not compile. The sibling keeps the member name `Value` every language's docs reference. |
| value members | `strip_prefix("out")`, else verbatim | `outSlowK→SlowK`, `outMACDSignal→MACDSignal`, `outMinIdx→MinIdx`. All 24 distinct output names are already valid PascalCase. |
| methods | `Update` / `Peek` / `Value` / `Clone` / `FillRange` | `Clone()` over Java's `copy()`: the .NET verb, Rust's name, and analyzer-clean at `AnalysisMode=All` for a *concrete-typed* `Clone()` on a non-`ICloneable` type. |
| out-meta | `out int outBegIdx, out int outNBElement` | Already the batch convention. No `MInteger`. |
| peek scratch | `[ThreadStatic] private static <NAME>_Stream? peekScratch;` | `ThreadLocal<T>` is itself `IDisposable`, the wrong signal inside the one tier whose thesis is "needs no `Dispose`". No inline initializer (runs on the first thread only) ⇒ `is null` check. |
| field visibility | every handle field `internal` | A *sibling* nested type cannot reach another's `private` members, and `MAVP_Stream`'s copy ctor constructs `MA_Stream` copies. One rule beats per-field analysis; `internal` is invisible to consumers. |
| constructors | all `internal` | No caller-minted handles, and `System.Text.Json` deserialization fails rather than producing a half-built handle — the positive act C# needs where Java gets not-serializable for free. |
| message prefix | `"<NAME> open: "` / `"<NAME> openAndFill: "` | Cross-language contract (`streaming-api-design.md:274-275`). Deliberately *not* `Core.Failure`'s `"TA_<NAME>: "`; an emitter comment says so, so nobody unifies them by accident. |

### SMA — single output, `StreamPlan::Loop`, the shape 153 of 172 get

```csharp
   public sealed class SMA_Stream
   {
      internal Core core;
      internal int optInTimePeriod;
      internal double periodTotal;
      internal int ringPos_trailingIdx, ringCap_trailingIdx;
      internal double[] ring_trailingIdx_inReal = [];   // CS8618 + CS0649 are errors here
      internal double cur_outReal;
      internal OutRange fillRange;

      internal SMA_Stream( Core core ) { this.core = core; }
      internal SMA_Stream( SMA_Stream o ) { /* scalars; new double[n] + Array.Copy */ }
      internal void CopyFrom( SMA_Stream o ) { /* length-branched in-place Array.Copy */ }

      public OutRange FillRange => fillRange;
      public double Update( double inReal ) { core.SMA_StreamStep(this, inReal); return cur_outReal; }
      public double Peek( double inReal )
      { SMA_Stream s = new SMA_Stream(this); core.SMA_StreamStep(s, inReal); return s.cur_outReal; }
      public double Value => cur_outReal;
      public SMA_Stream Clone() => new SMA_Stream(this);
   }

   internal void SMA_StreamStep( SMA_Stream sp, double inReal ) { /* build_transition IR */ }

   private  RetCode     SMA_OpenCore( SMA_Stream sp, double[] inReal, int startIdx, int optInTimePeriod,
                                      out int outBegIdx, out int outNBElement, double[] outReal, int outStride );
   internal SMA_Stream  SMA_OpenInternal( double[] inReal, int startIdx, int optInTimePeriod );
   public   SMA_Stream  SMA_Open( double[] inReal, int optInTimePeriod );
   public   SMA_Stream  SMA_OpenAndFill( double[] inReal, int optInTimePeriod, double[] outReal );
```

The step body is `build_transition`'s IR through `csharp::render_statement_ctx`
with the same `FmaVarSets` — byte-for-byte the Java step modulo prefix
spelling. **The step stays a method on `Core`, not on the handle**: transcribed
bodies render unstable-period reads as `this.unstablePeriod[(int)FuncUnstId.X]`
(`csharp.rs:1828-1836`), which only compiles inside a `Core` instance method,
and the `core.Step(sp,x)` vs `this.Step(x)` difference measured 3.44–3.54 vs
3.39–3.47 ns/bar — indistinguishable.

The Java `OutMode` stride merge ports unchanged: one transcription of the batch
body, `out[idx * outStride]`, stride 0 → a `double[1]` sink for `Open`,
stride 1 → the caller's array for `OpenAndFill`.

New hand-written, in `Core.cs` beside `Failure` — **not** a reuse of it, which
maps `OutOfRangeEndIndex → ArgumentOutOfRangeException("endIdx")`, meaningless
for a caller with no `endIdx` parameter:

```csharp
   internal static Exception StreamFailure( string funcName, string what, RetCode retCode )
      => retCode switch
      {
         RetCode.OutOfRangeEndIndex => new InsufficientHistoryException(
                                           funcName + " " + what + ": history shorter than lookback + 1"),
         RetCode.InternalError      => new InvalidOperationException(funcName + " " + what + ": internal error"),
         _                          => new ArgumentException(funcName + " " + what + ": " + retCode),
      };
```

One helper instead of Java's four-line reject tail at ~520 emitted sites: it
single-sources the prefix the gate's reject legs grep, and removes ~2,000 lines
of emitted text.

New hand-written `library/InsufficientHistoryException.cs` (one level above the
`clean_glob("Core_", ".cs")` sweep): `sealed : ArgumentException`, three ctors,
and **no** `[Serializable]`/`SerializationInfo` ctor — SYSLIB0051 is obsolete
and `TreatWarningsAsErrors` makes it a build error, which mechanically enforces
the doc's not-serializable commitment.

### BBANDS — multi-output, `StreamPlan::Composed`

```csharp
   public readonly record struct BBANDS_Value( double RealUpperBand,
                                               double RealMiddleBand,
                                               double RealLowerBand );
```

**Java's `cachedValue` field is deleted, not ported.** Measured: the
record-struct return is 9.69–10.0 ns/bar and **0 B/update**; the Java-shaped
class-with-cached-field is 11.33–12.06 ns/bar and 40 B/update. The cache exists
in Java only to make `value()` allocation-free; a struct makes it free by
construction. `Deconstruct` comes free: `var (up, mid, low) = s.Update(bar);`.

Two consequences carried into the gate:

- Record-struct equality says `+0.0 == -0.0` and `NaN == NaN` — **the Java
  `Value` contract does not port verbatim**, and the doc comment must say so.
- Java's `value()==update` leg asserts *reference identity* of the cached
  record. A returned struct is copied to the caller's frame, so C# compares
  **per component**, and by output type (below).

Array fields initialise `= []` (C# 12 collection expression, lowers to the
cached `Array.Empty<T>()`), sub-handle fields `= null!`. Both are inert —
`OpenCore` overwrites every one before the handle escapes and the constructors
are `internal` — but they are mandatory: CS8618 and CS0649 are errors here.

### MA — `StreamPlan::Dispatch`, and MAVP — `StreamPlan::PeriodBank`

`MA_Stream` holds `internal object? sub`, tagged by `optInMAType`. All three
switch tables — copy ctor, `CopyFrom`, step — plus the three hand-rolled open
bodies come from **one** `DispatchPlan::arms` walk. A new MAType handled in one
and missed in another is the landmine; `emit_dispatch` takes a single
`&[DispatchArm]` and never re-derives.

The `default:` arm throws **`ArgumentException`**, not `InvalidOperationException`
— the gate catches `ArgumentException` only, so an IOE escapes to the server's
outermost catch and surfaces as `STREAM SET MISMATCH`, a failure that names the
wrong problem.

Dispatch alternatives were measured (switch+castclass 5.55–5.72, interface
monomorphic 4.41–4.74, bimorphic 4.48–4.64 ns/bar): the "a virtual call costs a
bar" premise is **false on .NET 10**. The switch still wins on parity,
profile-independence and not adding a type hierarchy across 172 handles — but
the measurement goes in the emitter comment so nobody re-optimizes on intuition
in either direction.

`MAVP_Stream` holds `internal MA_Stream[] bank = []`, copied **element-wise** in
both the copy ctor and `CopyFrom` — `Array.Clone` is shallow, and a shallow
bank lets `Peek` advance the live handle's sub-streams. `CopyFrom` branches on
length rather than Java's per-element null test (under `Nullable=enable` a null
test on a non-nullable element is a diagnostic). Every slot opens at the
**shared** `MA_Lookback(optInMaxPeriod)` anchor. Array covariance is a
non-issue: `stelem.ref` checks fire on *stores* to reference-type arrays, and
the only stores happen in Open and the copy ctor.

### XML docs

`GenerateDocumentationFile` + `TreatWarningsAsErrors` makes CS1591 an **error**.
One `emit_xml_member(summary, params, exceptions)` helper, so documentation is
all-or-nothing per member by construction (CS1573 fires only when *some* params
are documented). Every public member gets `<summary>`; openers get complete
`<param>` sets from the existing `csharp_doc` machinery plus `<exception>` for
`InsufficientHistoryException`/`ArgumentException`/`ArgumentNullException`.
`cref` rule: `<see cref="SMA_Stream"/>` and `<see cref="SMA_Open"/>` are legal;
**never** `<see cref="Core.SMA"/>` — an overload set, CS0419.

---

## 3. Emitter architecture

### Wiring

One call site, the structural mirror of `java.rs:481-484`, inserted before the
closing `out.push_str("}\n")` at `csharp.rs:281`:

```rust
    if func.streaming {
        out.push_str(&super::csharp_stream::generate(func, enums, registry, helpers));
    }
```

Nothing else in the pipeline moves: `out_subdir`, `file_name` and `clean_glob`
are unchanged, and `generate --func=X` stays correct for the C# *library*.

### `csharp.rs` / `csharp_doc.rs` changes — visibility only

1. `emit_opt_param_validation` (`csharp.rs:310`) → `pub(crate)`
2. `render_hoisted_blocks` (`csharp.rs:858`) → `pub(crate)`
3. `render_csharp_switch_label` (`csharp.rs:1427`) → `pub(crate)`
4. `opt_param_type_str` (`csharp.rs:391`) → `pub(crate)`
5. `csharp_doc::output_desc` (`csharp_doc.rs:192`) → `pub(super)`

Plus a two-function surface-type seam so the batch and stream tiers cannot
drift and a later span migration is a two-line change rather than a rewrite:
`cs_series_in() -> "double[]"`, `cs_series_out() -> "double[]"`, with
`cs_type_str`'s `RealPointer|RealArray` arm and every emitted stream signature
routed through it. **The span flip itself is not in this series** (§8).

Needing nothing: `CsRenderCtx` (already `pub(crate)` with `pub(crate)` fields),
`cs_type_str`, `render_statement_ctx`, `render_expr`, `RESERVED_WORDS`, the
nine `java.rs` helpers `csharp.rs` already imports,
`candle_settings::emit_csharp_unpacking`.

### The NameMap seam — prefixes copied verbatim, not re-invented

```rust
fn state(&self, n)      -> format!("sp.{n}")
fn output(&self, n)     -> Expr::Var(format!("sp.cur_{n}"))   // not Rust's PointerDeref
fn ring_buf(&self, v,a) -> format!("sp.ring_{v}_{a}")         // ...ringPos_/ringCap_/ringLag_
fn win_buf/win_pos/win_cap, circ_buf -> "sp.cb_{s}", extrema_buf -> "sp.x_{a}", extrema_mask -> "sp.xMask"
```

**Hard requirement, not aesthetics.** `fma::stream_base` (`fma.rs:127-134`)
strips exactly `sp->`, then `sp.`, then `cur_` to decide integer-vs-float
typing. Keeping Java's spellings means **zero `fma.rs` change**; any other
scheme requires extending `stream_base`, and getting that wrong is a ~1 ULP
cross-language divergence with nothing pointing at the cause.

FMA name-sets come from `func.stream_source()` — **not** `private_body`, **not**
`body` — with bar inputs seeded into `real_vars`.

`stream_ctx` sets `single_precision: false` — **the stream tier is double-only,
no `float[]` overload, ever**.

### Emission rules — the measured "do not" list

Each is a measurement, not a preference. They go in the module header with the
numbers, because their whole value is stopping a later reader from
re-optimizing on intuition.

- **R1. No `MethodImpl` attributes.** SMA `Update` is 3.39–3.51 ns/bar
  inlinable vs 6.69–7.10 behind `NoInlining`; the JIT's IL-size heuristic
  already inlines the small steps and correctly declines the ~400-byte CDL
  steps. `AggressiveInlining` on a CDL step bloats every call site for nothing.
- **R2. `Update`/`Peek`/`Value` stay thin** — no validation, no null checks
  (there are no array arguments), no logging. That is what keeps them inlinable.
- **R3. `Value` is an expression-bodied property.** No `cachedValue` field.
- **R4. No unsafe indexing.** `MemoryMarshal.GetArrayDataReference` +
  `Unsafe.Add` measured 4.26–4.55 vs 3.44–3.47 ns/bar — a **regression**,
  reproduced 3×.
- **R5. No array-hoisting pass** (3.35–3.48 vs 3.39–3.47 — noise). Hoist only a
  *counted* loop bound, where it genuinely drops a check.
- **R6. No `state`→`temp` demotion in `streaming.rs` on C#'s account** (5.62–5.77
  vs 5.69–5.75 — nothing).
- **R7. No `[StructLayout]`.** The CLR uses `LayoutKind.Auto` for reference
  types and reorders fields itself; `Sequential` would disable that.
- **R8. Rings stay `double[]`/`int[]` fields.** `Span<T>` cannot be a field
  (CS8345); `Memory<T>` costs a span materialization per access.
- **R9. Copy constructor uses `new T[n]` + `Array.Copy`, never
  `(double[])x.Clone()`** — 2.3× (26.0–30.6 ns vs 58.4–70.2 on a 29-element
  ring, 3 launches). This is on `Peek`'s path for ~86 functions.

### C# spelling substitutions

`double inReal[]` → `double[] inReal` · `MInteger x`/`x.value` → `out int x`/`x`
with `out _` discards · `.length` → `.Length` · `.clone()` → `new T[n]` +
`Array.Copy` · `System.arraycopy` → `Array.Copy` (**same argument order**) ·
`(Object)a == (Object)b` → `ReferenceEquals(a,b)` (also compiles for
cross-typed `double[]`/`int[]` output pairs, where `==` would not) ·
`Integer.MIN_VALUE` → `int.MinValue` · `Object sub` → `object? sub` +
`(SMA_Stream)sp.sub!` · `instanceof X y` → `is X y` · `ThreadLocal<T>` →
`[ThreadStatic] static T?` · `IllegalArgumentException`/`IllegalStateException`
→ `ArgumentException`/`InvalidOperationException` · `{@code}`/`{@link}` →
`<c>`/`<see cref>` · bare `case SMA:` → `case MAType.SMA:` — **ALL CAPS**, the
emitted enum is `SMA = 0 … HMA = 9, DISABLED = 10, DEFAULT = 11`.

### Landmines the emitter must honour

- **`PRAGMA TA_ALT`** on MAX, MIN, MIDPOINT, MIDPRICE, MINMAX, WILLR: each
  carries an `_ALT1` body claiming the STREAM tier, so analyzing `func.body`
  silently analyzes the Van Herk block scan instead. **`csharp_stream.rs` must
  never touch `func.body`** — derive `stream_source()` once in `generate` and
  thread it.
- **Do not hand-type the identity predicate.** The batch body emits
  `optInTimePeriod <= 1 || optInMAType == MAType.DISABLED`; a hand-typed `== 1`
  is a value divergence at period 0. Render `streaming::identity_step_branch`.
- **Candle-snapshot local names are load-bearing.** `fma::expr_is_float_typed`
  classifies an operand float by the `_factor` *suffix* (`fma.rs:196-205`); the
  snapshot locals are emitted as text, never as IR `VarDecl`s, so nothing else
  types them. A renamed local flips the FMA fusion sites on 57 CDL functions,
  ~1 ULP, invisible to every structural gate. The step must declare
  `<S>_rangeType` (int), `<S>_avgPeriod` (int), `<S>_factor` (double),
  byte-identical to Java's spelling.
- **Composed `cur_*` seeds from the FUNCTION outputs** (`CurSource::Scratch`),
  not the producer model.
- **`sc_<out>` aliasing election (#205)** with its pinned
  `composed_sub_call_destination_funcs` set — **do not widen it**. STOCH's
  in-place slow-K stays unfused.
- **ADXR's sub-lag ring** is pushed *after* every read of the oldest slot, and
  seeded from the still-live intermediate array's tail.
- **The fast-path block is filtered out of `cp.region`** — transcribe
  `cp.region` (an owned `Vec`, precisely because of that filter), never
  `func.body[..tail_start]`.
- **MACDEXT's sub-open anchors read out-meta locals of the transcribed region**
  (`outNbElement1`; APO/PPO/PVO read `fastNb`; STOCHRSI mixes `outBegIdx2` with
  `dummyNBElement`) — always via `streaming::batch_call_out_args`.
- Own-lookback precheck **before any sub-open** in all three composing tiers, so
  a reject reads `"MA open:"` and not `"SMA open:"`.
- `matype_map` empty everywhere except Dispatch.

---

## 4. The gate

The real gate is one JSON-RPC method computed **in-server**: `stream_verify`,
emitted by a new `emit_csharp_sv_func` / `generate_csharp_stream_verify` in
`server_gen.rs` (~620 lines, mirroring `server_gen.rs:5402-6022`). It runs both
arms — batch and stream — inside the same C# process on the same
server-generated inputs, and reports `ok`, `legs`, `badBar`, `peek_ok`,
`fill_checked`, `fill_ok`, `benign` and the reject counters.

**It is built at S2, at one third of the corpus**, and driven by
`scripts/csharp_stream_check.py` while the `ta_regtest` capability probe stays
dark. At S9 the probe flips and the same handlers become the gate. This is the
single most important sequencing decision in the plan: the alternative — a
separate C# parity test as an interim oracle — is a second implementation of
the same checklist that must agree with the first forever or drift silently,
which is exactly the class of bug this repo has been burned by.

### The dark response

The driver does `strstr(responseBuf, "not_streamable")` — a **substring** match
(`test_codegen.c:3686`). The dark response is pinned to:

```
{"error":"csharp stream tier under construction"}
```

It must not contain the token `not_streamable` anywhere. Copying Java's literal
`{"error":"not_streamable"}` flips the all-or-nothing set-parity gate ON at
S2 with 51 of 172 handlers, reddening every `regtest.py` run for six stages for
a reason unrelated to the work in flight. S2 gates on `grep -c not_streamable
TaCodegenServe.cs == 0` until S9.

### Legs, and the C#-specific rules

The 20-leg Java checklist ports: bit-exact trajectory vs batch, the fill leg vs
`batch(0,n-1)`, expected-reject probes, `Value`==update, `Peek` does not commit,
`Clone` independence, the `int.MinValue`==default sentinel pair, aliasing
rejects, the candle-settings rounds, the unstable-period variants. C# then
differs in seven places:

1. **The comparator is selected per OUTPUT TYPE, not per leg.** Real outputs →
   `SvBne` (same-tier, `BitConverter.DoubleToInt64Bits`) / `SvXtierNe`
   (cross-tier, ±0-tolerant). **Integer outputs → plain `!=` on both tiers,
   never cast to double** — MINMAXINDEX's members are `int` and would not
   compile through `DoubleToInt64Bits`, and a `(double)` cast would silently
   weaken a strict leg. Never `==`/`Equals` on a `<NAME>_Value`: record-struct
   equality says `+0.0 == -0.0`, which makes every strict leg vacuous.
2. **Full aliasing cross product.** Java probes two pairs — output0≡input0 and
   output1≡output0. For every real output *i* and real input *j*, one probe;
   for every same-typed output pair *i<j*, one probe. O(9) for the widest
   function. This matters most in the composed tier, where
   `fill_scratch_may_alias_output` deliberately makes `sc_<out>` alias the
   caller's array for eight functions, and where the failure in C# is a wrong
   *value*, not an exception.
3. **A real mid-stream candle-settings leg.** `Core.candleSettings` is
   `internal readonly CandleSetting[]` — the *reference* is readonly, the
   elements are assignable, and the server compiles the library sources — so
   the harness can open a stream, overwrite the settings the step reads, drive
   the remaining bars, and require them still to match. Without it the
   snapshot's entire purpose is unexercised: every existing round builds a
   fresh `Core` before opening, so a step reading live settings reads exactly
   what the snapshot holds.
4. **A restructured allocation probe.** After the value sweep, a separate loop
   containing nothing but `Update`, accumulating one component into a `sink`
   consumed into a field so the calls cannot be elided, bracketed by
   `GC.GetAllocatedBytesForCurrentThread()`, required to be exactly 0 and
   reported as `updAlloc`. Folding it into the trajectory sweep would red
   spuriously — that loop peeks every 7th bar, and `Peek` allocates on the ~83
   handles where `scratch_pays` is false. Never box the returned value inside
   the measured region (32 bytes, measured).
5. **Out-of-list enum needs a real reject check.** Java's "type safety IS the
   rejection" shortcut does not port: `MAType` is int-backed and
   `(MAType)int.MinValue` is representable.
6. **`compatibility != 0` is explicitly refused with an error**, never a silent
   Default re-run — `COMPATIBILITY()` panics the C# renderer, so the
   seed-boundary rules are inert here.
7. **`SvXtierNe`'s `zsign` accumulator is per-request**, never static: one
   process answers many requests.

### The ±0 hole, and the one gate that closes it

`SvXtierNe` counts bits that differ but compare `==` as `benign` and never
fails. That is right for stream-vs-batch — but there is **no second observer**:
`stream_verify` is intra-process per language, `--xlang-hash` is batch-only, and
the driver accumulates `benign` into a per-language counter it prints and never
compares. So a C#-stream-only defect whose sole symptom is a zero's sign is
green in `ta_regtest`, in `regen-check` and in the nightly.

**Add a cross-language equality check on `benign` and `legs`, Java row vs C#
row, per function, hard-failing on any difference.** The plumbing exists —
`test_codegen.c` already keys a results table by `langIndex`, and all languages
loop in one process. The comparison must be **Java↔C# and not against C**:
measured, `Math.Min(-0.0, 0.0)` returns `-0.0` in C# and Java, while the C
`min()` macro ternary returns `+0.0`, so C legitimately has a different benign
population. Java and C# render identically and must agree exactly.

This is the highest-value gate the design was missing.

### What the fuzz port needs

New `templates/csharp/FuzzData.cs` (~550 lines) — all 9 shapes including the
hand-built `FUZZ_CANDLE` catalog; splitmix64 on `ulong` with `unchecked` and
logical `>>`. Self-checked against the driver's `xlang_in_hash_local` (FNV-1a,
basis 1469598103934665603, prime 1099511628211, over little-endian O,H,L,C,V,OI,
then fmix64).

Two defects in the existing self-check that C# must not inherit:

- **It runs at exactly one seed** (`gen_seed = 7`, `n = 240`) while the vector
  loop uses others. A port that is bit-identical at seed 7 and wrong elsewhere
  passes and is then undetectable, because *both* arms consume the same
  server-generated array — they agree perfectly on the wrong data. Widen to a
  small fixed seed set (7 plus two of the loop's real seeds).
- **It fails open and silent.** If `in_hash` is absent it `break`s, and the
  summary prints only when something was checked. So gate on the *literal
  printed line*, `Fuzz-port self-check: 9/9 shapes bit-identical`, not on the
  absence of a failure message.

### Process topology is load-bearing

The C# server is strictly single-threaded and sequential, so one `[ThreadStatic]`
scratch per handle type persists across **all** param vectors, shapes,
unstable-period legs and fresh `Core` instances in a run. That is what makes a
whole class of `CopyFrom` defects detectable — a scratch carrying vector 1's
period into vector 2, `MA_Stream.CopyFrom` switching on `this.optInMAType`
instead of `o.optInMAType`, a sub-handle's `CopyFrom` not refreshing `core`.
All three vanish if the driver spawns a server per function, per vector or per
shape — which is the obvious way to write that script.

`csharp_stream_check.py` **must send every vector for a function down one server
process, in the driver's order, and must assert it did** (count requests per
spawn). It must also build its param vectors from the *same rules* as
`test_codegen.c:2528-2790` and assert the vector count against
`STREAM_MAX_VEC=128`, or the script and the driver drift and S9 surfaces a wall
of failures.

### Generator-side tests

`tests/csharp_stream_suite.rs`, created in S1 and grown one group per tier — the
direct port of `java_stream_suite.rs` (531 lines, 21 tests) and its structural
properties: the emit ratchet with a hard floor, one merged `OpenCore` per
function with two wrappers, the one-element sink at stride zero, stride-scaled
output writes, the fill wrapper's aliasing guards, exempt tiers keeping two
bodies, the composed copy-out stride guard, the identity fast path
short-circuiting at stride zero. These pin what no value gate can see: "the
`OpenCore` merge silently degraded to two transcriptions" is value-neutral.

Plus four targeted tests:

- C# and Java handle **field-name sets identical**, per function (the cheap
  substitute for extracting the field-spec layer into `streaming.rs`).
- Candle-snapshot **local names** exactly `<S>_rangeType`/`_avgPeriod`/`_factor`,
  and no `sp.cs_` after the unpacking prologue — the only possible gate for the
  FMA suffix heuristic.
- Comparator discipline: every peek/`Value`/copy/sentinel site calls `SvBne`,
  no such site calls `SvXtierNe`, and vice versa for the cross-tier sites.
- `DispatchPlan::arms` ∪ `unsupported_labels()` ∪ identity labels covers every
  variant of the dispatching enum except `DEFAULT` — fail generation otherwise.

And `tests/alt_suite.rs` widens from a 3-tuple to a 4-tuple with a C# arm
(~15 lines). It is the **only** mechanism in the tree that can catch a wrong
`TA_ALT` body resolution — "nothing but the emitted code can prove which body
won" (`generator/CLAUDE.md:216-225`), and today it does not look at C# at all.

---

## 5. Operating rules for the run

These exist because the plan is executed unsupervised. They are not advice.

**Regenerate loop.** Between every edit:
`cargo run -- generate > /tmp/gen.log 2>&1` — **redirected to a file, never
piped through `head`/`less`** (SIGPIPE kills it mid-run and deletes ~650 tracked
files; it fails *open*, exit 0; a second uninterrupted run heals it) — then
`scripts/build.py servers`. **`--func=` is forbidden for any stage whose gate
drives a server**: under a filter the JSON-RPC servers are skipped
(`main.rs:745-757`) while the C# metadata catalogue is still rendered
whole-corpus (`main.rs:713-727`), so the C# tree looks entirely healthy and only
the server is stale. Java's equivalent skip is loud because `Core.java` visibly
loses methods; C#'s is silent. `csharp_stream_check.py` refuses to run if the
handler count in `TaCodegenServe.cs` disagrees with the emitter's census.

**Lint every stage, not just the last.** The generator crate carries
`#![deny(clippy::pedantic)]` and the nightly runs `cargo clippy --all-targets
-- -D warnings` as a hard gate, but neither `regen-check` nor the PR gate lints.
3,400 new lines of `format!`-heavy emitter code will fire `too_many_lines`,
`too_many_arguments`, `needless_pass_by_value`, `items_after_statements` and
`similar_names` naturally. Every stage from S1 runs `scripts/build.py clippy`
and `cargo test --manifest-path ta_codegen/generator/Cargo.toml`. Running it
once at the end means bisecting a pedantic wall. **Never `cargo fmt` the
generator crate** to quiet it.

**Commit at every stage boundary.** `regen_check` snapshots the already-dirty
set and subtracts it from the drift (`build.py:456-490`), which is correct for
a developer with two files open and wrong for an eleven-stage run: an
uncommitted tree with hundreds of modified generated paths excludes precisely
the files under test, and both regen gates become self-satisfying. Same for the
`git diff` byte-identity gates. This needs the owner's standing permission
(§9).

**Half-written tree recovery.** `csharp_stream::generate` panics on
`validate_streamable` failure, and `generate` cleans stale output before it
writes — so a panic on function 140 of 172 leaves a partially deleted
`output/csharp/library/src/`. The recovery is **re-run `generate` to
completion**. Never `git checkout` the output tree on a panic.

**Abort and park.** A stage that does not reach its gate in three attempts
halts and reports; it does not proceed. The designated safe state is **the
emitter landed with the probe dark** — the tier compiles, ships nothing
user-visible that is wrong, and gates nothing. No stage is ever forced forward
by the all-or-nothing set-parity check, because that check is not live until
S9. If a tier cannot be ported, park at the previous stage boundary with the
probe dark and report.

---

## 6. Staging

Estimates are working days at the granularity the workflow could defend; treat
them as relative weights.

### S0 — plumbing only (~1 day)

Five visibility promotions; the `cs_series_in`/`cs_series_out` seam;
`pub mod csharp_stream;` in `backends/mod.rs`; `csharp_stream.rs` exporting its
six public items with `generate()` returning `String::new()`; the hook before
`csharp.rs:281`. Hand-written: `library/InsufficientHistoryException.cs`,
`Core.StreamFailure`, `OutRange.Empty`.

**Deliberately *not* in S0** — see §8: the `ThrowIfNull` sweep, the CS0219
pragma rescope, the `OutRange` equality members, the Span-rationale comment
rewrite.

*Gate.* `regen-check` green. A full `generate` redirected to a file, then
`git diff --stat` shows **zero** changed bytes under `src/ta_func/`,
`output/rust/`, `output/java/`. `dotnet build` of the library, test and server
csprojs: 0 warnings, 0 errors; all 4 suites pass (2886 checks). clippy + cargo
test green.

*Sabotage.* (i) Make the stub emit one marker comment and regenerate —
**exactly 172** `Core_*.cs` must change and nothing else. Without this the
byte-identity gate is vacuous, because a stub emitting nothing trivially
changes nothing. (ii) Revert any one visibility promotion — `cargo build` must
fail, proving it is load-bearing and not cargo-culted. (iii) Add
`[Serializable]` + a `SerializationInfo` ctor to `InsufficientHistoryException`
— the build must fail with SYSLIB0051-as-error.

### S1 — the shared spine + Loop T1/T2, 51 functions (~5 days)

`CsStreamNames`/`CsComposedNames`; field layout; handle class, `Value` type,
peek scratch, `Update`/`Peek`/`Value`/`Clone`; `stream_ctx`, step emission,
identity branch; `map_open_return`, the `OutMode` stride merge, validation,
capture, identity fast path; reject conversion, open wrappers,
`OpenAndFillInternal`; the `ReferenceEquals` aliasing guard; `emit_loop`. A
temporary tier gate emits only `Loop` plans with empty rings/windows/circs and
no extrema.

Also: `tests/csharp_stream_suite.rs` with the structural group (~120 lines);
`alt_suite.rs` widened to include C#; the field-name-parity test.

**S1 must not end without executing a bar.** Its gate as originally drafted was
a compile and a grep — 1,300 lines and 55% of the emitter with the first
execution deferred to S2, so a wrong step body, an inverted stride or a
mis-seeded capture would all compile cleanly and be discovered when 51
functions are wrong at once *and* the new harness is simultaneously unproven.
Pull the minimum viable slice of S2's harness forward: one function, the
trajectory leg, the fill leg. Under a day.

*Gate.* Builds clean; exactly 51 files carry a stream section and 121 do not;
the suite, field-parity and alt tests pass; clippy + cargo test green; SMA opens
on 100 bars and 40 `Update`s match the batch array bitwise.

*Sabotage.* Drop an `= []` initializer → CS8618 + CS0649. Rename a
`<NAME>_Value` to `Value` → CS0102. Remove a `<summary>` → CS1591. Omit one
`<param>` where others are documented → CS1573. Drop a field → the parity test
reds. Perturb the SMA step by 1 ULP → the new behavioural leg reds. *(The first
four prove the C# compiler works; only the last two test the emitter — which is
exactly why the last two are the ones that had to be added.)*

### S2 — the real gate harness, at one third of the corpus (~5 days)

`sv_csharp_input_array`, `emit_csharp_sv_func` (the full checklist, with the
seven C# rules of §4), `generate_csharp_stream_verify`,
`csharp_server_stream_scaffolding`, `HandleFuzzInHash`;
`templates/csharp/FuzzData.cs`; two dispatch arms in `generate_csharp_server`;
`scripts/csharp_stream_check.py`. The probe answers the pinned dark string.
The cross-language Java↔C# `benign`/`legs` check goes in here.

**The `bench_mode` stub stays.** Deleting it does not enable streaming
benchmarks — the code immediately after it is the ordinary batch marshalling and
timing loop, so an `--mode=open` request would fall through and be timed as a
batch call and reported in the `open` column. That is precisely what the stub's
own comment was written to prevent. Either leave it (chosen: it is honest and
costs nothing) or replace it with real open/openfill arms and gate on a non-zero
timing.

*Gate.* `fuzz_in_hash` matches the driver's local hash across all 9 shapes ×
every seed the vector loop emits. `csharp_stream_check.py` green over all 51:
`ok=1`, `peek_ok=1`, `fill_checked=1`, `fill_ok=1`, `legs>0`, `updAlloc=0`, and
the answered set equals the emitter's census. `grep -c not_streamable
TaCodegenServe.cs == 0`.

*Sabotage.* Flip a splitmix64 constant → the hash self-check diverges. Perturb
one step by 1 ULP → that function reds naming the bar. Make `Peek` write the
handle → `peek_ok=0`. Make `Clone` copy a ring by reference → copy leg reds.
Open at `lookback` instead of `lookback+1` → short-history accepted; throw a
bare `ArgumentException` instead of `InsufficientHistoryException` → wrong-type
red. Change the FMA name-set source from `stream_source()` to `body` → an
EMA-family function reds, proving *this* gate enforces the fusion-site contract
rather than assuming it. Insert `var _ = new double[1];` into one step →
`updAlloc` reds; box the returned value inside the measured loop → `updAlloc`
reds; delete the `sink` consumption → `updAlloc` wrongly reads 0, demonstrating
why the sink is load-bearing. Emit `-0.0` where the batch emits `+0.0` → the run
stays green on `ok` and **reds on the new Java/C# benign check**. Make
`CopyFrom` skip the parameter fields → a function reds on `peek_ok` at the
second vector, *and passes* if the script is switched to one process per vector
— that second run is the demonstration that the topology is what makes the leg
non-vacuous. Temporarily answer the literal `not_streamable` → `ta_regtest`
reds with `STREAM SET MISMATCH`, proving the flip is the only thing between the
stages and the gate. Then revert all of it.

### S3 — Loop T3, 90 functions (~4 days)

Ring/window/circ/parity branches of the field layout and capture (capacity
arithmetic, buffer materialisation, phase-preserving fills); the
`cs_<S>_{rangeType,avgPeriod,factor}` snapshot triple for 57 CDL functions; R9
copy semantics on every array field. Tier gate loosens to all `Loop` without
extrema.

*Gate.* 141 functions green, with the four candle rounds (defaults /
avgPeriod+3 / avgPeriod=0 / rangeType=Shadows) each building a **new**
`CandleSetting` through `CoreBuilder`, and an assertion that four rounds
actually ran per CDL function. **Plus the mid-stream mutation leg of §4.3.**

*Sabotage.* Make the step read live `core.candleSettings` → the *mid-stream*
leg reds while all four rounds stay green (the rounds alone cannot detect this:
each builds a fresh `Core` before opening, so live and snapshot hold the same
value — the originally-drafted sabotage here was provably vacuous). Shorten a
ring's capture capacity by one slot → the prefix sweep reds at the first
wrapped bar. Replace `CopyFrom`'s in-place `Array.Copy` with a no-op → that
function's peek *and* copy legs red. Rename `_factor` to `_fac` in the emitter →
the candle-local cargo test reds (no runtime leg can see this; the text
assertion *is* the gate).

### S4 — Loop T4, 12 functions (~2 days)

The absolute-index extrema automaton: the 2^30 rebase preamble, the
power-of-two extrema ring, `sp.xMask`, the carried cursor, index fields pinned
to 32-bit.

*Gate.* 153 functions green, **plus `benign > 0` over the
MIN/MAX/MIDPOINT/MIDPRICE/MINMAX/WILLR rows** — this family is exactly where a
strictly-bitwise cross-tier comparator reds on ±0, so a nonzero benign count is
what proves the tolerant comparator is in use. Per the #147 lesson, drive the
boundary vectors (`range.min`, `min+1`), not only the defaults vector, or the
±0 cell is never reached and the comparator is inert. The 2^30 rebase is
unreachable at n=240 in every backend — pin it with a cargo test that the
preamble is emitted for every model with extrema.

*Sabotage.* Switch the cross-tier compare to strict → reds on the MIN/MAX
family; restore and confirm green **with `benign > 0`**. Perturb the same values
to 1e-300 → reds with `benign:0`. Delete the rebase preamble → the cargo test
reds. Point one same-tier leg at `SvXtierNe` → the comparator-discipline test
reds. *(Record explicitly that rewriting `& xMask` as `% cap` would red nothing
— for a power-of-two capacity they are the same value, which is why the rewrite
is safe, not a gate hole.)*

### S5 — DualMode, 7 functions (~2 days)

EFI, HMA, MINUS_DI, MINUS_DM, PLUS_DI, PLUS_DM, TRIMA. Scalar union
(conflicting `VarType`s = hard assert), union fields, complement capture,
predicate rendering (non-boolean IR wrapped as `(x) != 0`), the identity branch
hoisted **above** the mode predicate.

*Gate.* 160 green, with vectors forced to cross **both** arms per function (HMA
period 2 and 3 for arm A, above 3 for arm B) and a per-function assertion that
at least two distinct vectors executed and neither arm's leg count is 0.

*Sabotage.* Delete the complement capture → the `Clone`/`CopyFrom` leg of at
least one function reds. *(The C# failure mode is a stale-or-empty array rather
than Java's NPE, which is why the COPY leg and not the update leg is the
detector.)* Move the identity branch inside **arm A** → HMA at period 1 reds
with a NaN.

*(Corrected during execution: this originally said "inside arm B … because
period 1 lands in arm A". That is backwards. HMA's arm-A predicate is
`optInTimePeriod == 2 || optInTimePeriod == 3`, so period 1 falls through to
arm **B** — moving the branch there is value-neutral and the sabotage passes
silently. HMA is also the only dual-mode function with a step identity branch
at all. Hoisting above the predicate is still the right emission, because it is
a property of the function rather than of a mode; only the stated reason was
wrong.)*

### S6 — Dispatch, MA (~3 days)

**Must precede Composed and PeriodBank**: MA is the callee of 13 of the 18
composed sub-calls and the sole PeriodBank callee. `object? sub`; all three
switches from one `arms` walk; three hand-rolled open bodies, each with its own
`MA_Lookback` precheck first; identity rendered, never hand-typed; MAMA's
`OutSlot::Discard`; `matype_map` populated for this tier only; the reject-arm
machinery ported even though `unsupported_labels()` is currently empty.

*Gate.* 161 green, MA swept over all 10 MAType arms plus `DISABLED` plus period
1 and 0, with `Clone` and `Peek` per arm. `MA_Open(hist, 30, (MAType)99)`
rejected by a real reject-parity check. A reject message carries `"MA open:"`
and never `"SMA open:"` — assert on the text. The arm-coverage cargo test
passes.

*Sabotage.* Swap two arms **in the copy-constructor switch only**, leaving the
step switch correct → the copy-independence leg reds. *(This is the failure a
step-only test cannot see, and the highest-value sabotage in the plan.)* Delete
the own-lookback precheck → the prefix assertion reds. Hand-type the identity as
`== 1` where the IR says `<= 1` → the period-0 vector reds. Make
`MA_Stream.CopyFrom` switch on `this.optInMAType` instead of `o.optInMAType` →
the MAType sweep reds, and reds *only* when two different MATypes are peeked in
one process. Add a synthetic eleventh MAType to a throwaway enums fixture →
`generate` must fail loudly rather than emit a switch with an unreachable
default.

### S7 — PeriodBank, MAVP (~2 days)

`MA_Stream[] bank`, element-wise copy in both paths (length-branched, no null
tests), the clamp + lockstep step with the hoisted loop bound, two hand-rolled
open bodies (Scalar seeds every slot at the shared `MA_Lookback(maxPeriod)`
anchor; Fill genuinely replays history), no `OpenAndFillInternal`,
`minPeriod > maxPeriod → BadParam`.

*Gate.* 162 green, **with the period-selector series overridden to a ramp
min−1 … max+1** fed identically to batch and stream — without it every bank slot
but `maxPeriod` is vacuous. Plus a `Clone`-fork leg (fork at mid-series, drive
both to the end).

*Sabotage.* Replace the element-wise bank copy with `(MA_Stream[])bank.Clone()`
→ the `Clone` and `Peek` legs red, because the shallow clone lets `Peek` advance
the live handle's sub-streams. Seed each slot at its own lookback instead of the
shared anchor → every ramp position below `maxPeriod` reds. **Drop the ramp
override and re-run the first sabotage → it must now PASS**, which is the
demonstration that the ramp is what makes the leg non-vacuous.

### S8 — Composed, 10 functions (~6 days)

The largest tier. `CsComposedNames`, composed cur-scalars, `forc` shell
dropping, cursor mapping, map-step transform, composed step/open. `sc_<out>`
scratch with the #205 aliasing election; per-sub INSERT splicing by
`region_len + sub.tail_idx`; fusion via `<CALLEE>_OpenAndFillInternal` where
`is_fusable` allows (STOCH's in-place slow-K stays unfused); #203 copy elision;
`double_address_of_vars` over the **combined** region ++ tail; ADXR's sub-lag
ring. **Order by risk:** STDDEV (1 sub, 0 inter) → the seven other loopless →
STOCH/STOCHF (T4 producer) → ADXR (sole sub-lag ring).

*Gate.* 172/172, every function `fill_checked=1` and `legs>0`, and the count
equals the number of `FunctionCatalog.Default` rows carrying
`FunctionFlags.Stream` — the all-or-nothing precondition for S9. New legs:
`OpenAndFill` with outputs sized exactly `historyLen − lookback` must not throw
(the #205 alias path); the **full aliasing cross product** must throw
`ArgumentException` and mint no handle; ADXR's lag ring exercised by a `Clone`
fork within `period−1` bars of the open.

*Sabotage.* Force `fill_scratch_may_alias_output` true unconditionally → at
least one of the eight unscreened functions mismatches. Push ADXR's lag ring
before the combine-map read → the first `period−1` `Update` bars red.
Transcribe `func.body[..tail_start]` instead of `cp.region` → BBANDS and MACDEXT
red on the fast-path parameter values. Seed `cur_*` from the producer model
instead of the function outputs → STOCH's `Value` reds at open. Mark STOCH's
in-place slow-K fusable → fails to compile or reds. Remove the aliasing guard
for exactly one non-(0,0) pair → that pair's probe reds while the original two
stay green — the control arm proving the new probes are doing the work.

### S9 — flip the gate on (~3 days)

The one-line probe change; `scripts/synth_gate.py:155` floor `len(stream) < 3`
→ `< 4`; `StreamSmokeTest.cs` (auto-discovered by `AllTests`' reflection over
`TALib.Test.*Test` with `public static int Run()`), covering the user-facing
contract — lifecycle, peek-does-not-commit, `Clone` fork/converge, `FillRange`
semantics, typed short-history exception, aliasing rejection, `int.MinValue`
equivalence, per-`Core` candle capture, a zero-allocation assertion over
`Update`, and a permanent home for the `benign > 0` property. **Every count
derived from `FunctionCatalog.Default`, never a literal** — `synth_gate` injects
fixtures into `input/`, and a hardcoded corpus count breaks the *build* before
any gate leg runs. *(The existing C# suites are clean here:
`CatalogFacts.FunctionCount` is generated, so the Java `StreamSmokeTest`
landmine has no counterpart yet. The rule is prospective.)*

*Gate.* `./ta_regtest --codegen --language=csharp` runs the stream pass with
172/172 set parity, no `STREAM VACUOUS`, no `STREAM FILL VACUOUS`, and the
literal line `Fuzz-port self-check: 9/9 shapes bit-identical` **printed** for
the C# server. Then `scripts/regtest.py --codegen` and
`./ta_regtest --codegen --xlang-hash` green. `regen-check` re-run against a
clean tree.

*Sabotage.* Delete one function's `sv_` handler → `STREAM SET MISMATCH`. Make
one return `legs:0` → `STREAM VACUOUS`. Make one return `ok=1` unconditionally
→ the 1-ULP perturbation and the legs floor must **still** red, proving the
flags are computed and not hardcoded. Remove the C# `fuzz_in_hash` handler →
the self-check line disappears while the run still exits 0, demonstrating that
*absence* is the failure mode. **Revert only the probe change** (keeping the
emitter intact, so everything still compiles) → `synth_gate` reds at floor 4 and
would have passed at floor 3. *(Reverting `generate` to the stub instead, as
first drafted, deletes every `<NAME>_Stream` the harness references — the C#
server fails to compile and `synth_gate` dies at the build step, proving nothing
about the floor.)* Derive one `StreamSmokeTest` count from a literal → the build
breaks under `synth_gate`.

### S10 — documentation (~2 days)

`docs/streaming-api-design.md`: drop "the sole remaining streaming work";
reconcile 161/165 → 172; add .NET to the delivery surface; fix the "(the plan —
staging step 6)" cross-reference (it is step 8); correct "a tiny value class in
Java/.NET" for a `readonly record struct`; note that the JLS 17.5 safe-publication
sentence has no ECMA-335 analogue — a returned record struct is copied to the
caller's frame, a *stronger* guarantee.

New `website/src/api/csharp/README.md` (batch — does not exist either) and
`api/csharp/stream/README.md`, a ".NET API" sidebar block, and
`install/README.md:17` stops pointing .NET readers at the Rust and Java pages.

**Four durable surfaces the docs step must not miss:** `CHANGELOG.md` (a live
`## [0.8.1] Not Released Yet` block with zero occurrences of "stream" in the
whole file — the entire streaming tier, in every language, is unrecorded);
root `CLAUDE.md:319` ("`ta_bench_stream` is **C only**. For the Rust and Java
streaming tiers…" — false the moment C# streams); `generator/CLAUDE.md`'s
backend→output table; `scripts/README.md`'s script→CI-job table (a new script
with no CI job needs a row saying so).

Plus the NativeAOT publishing note (§7).

*Gate.* VuePress `npm run build` succeeds **and** a scripted check asserts both
new pages exist and are reachable from the sidebar (a dead sidebar link builds
fine and 404s at runtime). Every code sample in the new pages is extracted into
the test project as a compiled doc-sample file, so a stale sample is a build
error. Greps: `docs/streaming-api-design.md` returns nothing for "sole remaining
streaming work", "staging step 6" or "161 functions"; `CHANGELOG.md` contains a
streaming line under 0.8.1; `generator/CLAUDE.md` mentions `csharp_stream.rs`.

*Sabotage.* Change one doc sample's method name to one that does not exist →
the test project fails to build.

---

## 7. C# batch tier — what the audit found

The batch tier is in better shape than a first read suggests. It builds
warning-free under `TreatWarningsAsErrors` + `GenerateDocumentationFile`, uses
zero reflection, and NativeAOT-publishes with `TrimMode=full` into a 1.99 MB
self-contained binary that runs correctly — no IL2026/IL3050 anywhere in the
library. All 4 suites pass (2886 checks).

Three measured results matter, on AMD Ryzen 7 PRO 8840U / WSL2 / dotnet
10.0.110, every arm pinned with `taskset`, interleaved with alternating order,
min-of-N estimator, reproduced across 3–5 process launches:

1. **NativeAOT at its default instruction-set baseline lowers
   `Math.FusedMultiplyAdd` to `call fma@plt`** — zero `vfmadd` in the binary.
   TRIX 3.72×, DEMA 2.32×, EMA 1.53× slower, with SMA flat at 1.01× as the
   no-FMA control. Output hashes are **byte-identical** across JIT /
   AOT-default / AOT-v3, so the fix (`<IlcInstructionSet>x86-64-v3`) is free.
   This lands the cost on the streaming tier's *best* numbers — the EMA
   recursion family is where streaming wins biggest — so the publishing note is
   a S10 deliverable, and must exist before either tier is on NuGet.
2. **Range-sized scratch is plain `new double[n]` and lands on the LOH.**
   BBANDS(EMA) allocates 3.2 MB per call at n=200k and triggered 18 gen2
   collections in 200 calls at n=20k; 118 of 172 functions allocate per call.
   The honest position: **the streaming tier is itself the answer** for the
   repeated-call case those figures describe — `Open` + `Update` replaces
   calling batch on a growing window, and `Update` allocates nothing at all.
   Say that in the docs rather than pooling under a bitwise gate.
3. **The JIT does not eliminate bounds checks** in the generated loop shape
   (3/bar in SMA's main loop, ~8–10 in CDLDOJI) — but the measured *ceiling*
   for removing them is 1.00× on SMA (divide-bound) and 1.18–1.22× on a
   candle-shaped loop. The Rust backend's explicit-preamble investment does not
   transfer; the only mechanism that reaches the ceiling requires spans.

Two things ride along with the stream work because they are genuinely coupled:
**`Core.StreamFailure`** (`Failure` misrenders the stream's in-band
short-history signal) and **`OutRange.Empty`** (`FillRange` makes `OutRange` a
property on all 172 handles). Everything else is deferred (§8).

One correction lands as documentation in S0: the pinned rationale at
`csharp.rs:8-14` claims `Span<T>` has no `==`. That is false —
`ReadOnlySpan<T>` has had ref+length `==` since .NET Core 2.1, and all three
cited blockers survive a span migration. The decision to stay on `double[]`
stands, on the real grounds (the transcribed Open body's `double[]`-taking call
sites, and non-span-capable binding languages), but a wrong pinned reason
guarantees the next reader re-derives it wrong. Same for the stale
`case MAType.Sma:` comments at `csharp.rs:288` and `:1424`, which contradict the
emitted ALL-CAPS members.

---

## 8. Rejected and deferred

**Rejected outright.**

- *Extracting the field-spec / scratch-predicate layer into `streaming.rs`.* It
  rewrites three shipped backends during the bring-up of a fourth. The real risk
  it targets — a field present in one backend's handle and absent in another, a
  copy-semantics divergence no output hash can see — is covered by the ~40-line
  field-name-parity cargo test at 1% of the cost. Revisit as its own issue after
  the tier is green.
- *A separate C# parity test as the interim oracle.* A second implementation of
  the same 20-leg checklist that must agree with `emit_csharp_sv_func` forever
  or drift silently. Building the real `sv_` early produces the artifact that
  *becomes* the gate.
- *The `Span<T>` migration inside this series.* Not on the technical objection —
  that does not stand — but on sequencing: it is API-breaking, and it replaces
  the reference-equality aliasing reject (which the design doc names the managed
  .NET emitter as obliged to mirror, and which the gate pins by test) with
  `MemoryExtensions.Overlaps`, a semantic change to a gated reject. Doing an API
  break on the one tier with no oracle yet inverts the order of risk. The
  two-line seam ships so the later flip is cheap.
- *`ArrayPool<double>` for Open-time scratch.* The stream `OpenCore` **is** the
  transcribed batch body, so pooling there either changes the batch tier too or
  forks the transcription — and `Rent` returns dirty memory while several
  CDL/HT bodies rely on zero-init, a value-changing hazard inside a bitwise-gated
  tier.
- *Generic-math collapse of the `float[]` overload* (43.3% of IL).
  `IBinaryFloatingPointIeee754<T>` cannot be closed — a caller can instantiate
  over `Half`/`NFloat`, which no gate has run. It buys size, not speed. And it
  is moot here: the stream tier is double-only.
- *Bounds-check elimination, `[MethodImpl]`, array hoisting, `[StructLayout]`,
  `state`→`temp` demotion.* All measured zero-or-negative (§3, R1–R9). The
  emitter's complexity budget goes to the gate instead.
- *Cosmetic codegen churn* — 10,589 redundant `(double)` casts, `i = i + 1` →
  `i++`, `while` vs `for`, zero-initialisers. Measured free at run time,
  faithful to the shared C source four backends read; churning 70,000 generated
  lines for aesthetics against a bit-exactness constraint is a bad trade.
- *Renaming `MAX_INDEX` → `MaxIndex`, `SMA_Lookback` → `SmaLookback`, ALL-CAPS
  enum members, or `_Stream` → `_Handle`.* Cross-language parity names pinned
  through `Lang::CSharp` in `registry.rs`; changing them desynchronises every
  cross-indicator call site. One paragraph of rationale plus the analyzer waiver
  ships instead.
- *`TryOpen` / a public `RetCode` path.* **Open throws.** Insufficient history is
  knowable in advance and cheaply (`SMA_Lookback(period) + 1`),
  `InsufficientHistoryException` is a cross-language contract, and it matches
  Java's throw, Rust's `Result` and C's `TA_BAD_PARAM`. If `TrySMA(..., out
  OutRange)` ever lands, `TrySMA_Open(..., out SMA_Stream?)` lands symmetrically
  in the same change. Nothing here forecloses it.
- *A C# lane in `scripts/stream_ab.py`.* There is no C# performance claim to
  defend — no C# row in any benchmark table. Ship Java's measured
  `scratch_pays` predicate unchanged. When it does happen, it needs a control
  arm: the tool must first reproduce a *known* effect (switch the copy ctor to
  `.Clone()` and require ~2× regression on array-owning functions with the
  array-less ones flat) before it is trusted to settle an unknown one.

**Deferred to their own issues** (all uncoupled from streaming):

- **`ArgumentNullException.ThrowIfNull` across the ~330 public wrappers.** It is
  a shipped-batch-API change with no relationship to streaming, and landing it
  here is actively dangerous: `BatchApiTest.cs:224` pins
  `CheckThrows<NullReferenceException>`, and `run_csharp_tests` is called from
  `build_csharp_library` inside `cargo run -- build` — so a red C# suite
  hard-fails `scripts/build.py servers` and `scripts/regtest.py`, blocking every
  later stage, while the S0 gate asserts the suites pass. It is also
  ungateable here: the server calls the **internal** `RetCode` cores, not the
  public wrappers, so no gate leg would exercise a single changed method. It
  remains the hard prerequisite for any span migration.
- **The CS0219 pragma rescope**, the **`OutRange` equality members** and the
  **Span-comment rewrite** — none is a prerequisite for any streaming code, and
  the pragma rescope overturns a pinned decision and must be drawn to cover the
  stream section too or S1's build breaks the moment a step emits a dead temp.
  *(`OutRange.Empty` alone does ride along — it is the only piece `FillRange`
  needs.)*
- `ReadOnlySpan<PriceComponents>` in the metadata binder — one token,
  value-neutral, removes a 48 B/call floor. Five-minute follow-up.
- `stackalloc` for the ~40 small fixed `new double[2..5]` scratch arrays.
  Value-neutral in principle, but those bodies are transcribed into `OpenCore`
  and a `Span<double>` local cannot become a handle field, so the conversion
  must exclude any array the capture epilogue materialises into `sp.*` — real
  analysis the stream emitter has not been written against yet. After S9, when a
  regression is attributable.
- Catalogue eager-init (38 ms / 154 KB on first touch of `Core.Functions`) — a
  docs sentence; R2R/AOT removes most of it free.
- `max()`/`min()` NaN divergence across backends — pre-existing, out-of-contract
  (fuzz corpora feed finite prices), and **identical in Java**, so not
  C#-introduced. The cheapest correct fix is rendering the C macros as explicit
  ternaries in Java and C#, which the generator already proves it can do. Never
  in this series, where it would be a value change inside the tier under test.

**Closed non-issues** — verified, recorded so nobody spends a stage on them:
`double→int` is saturating in C# and matches Java exactly; the csproj sets no
`CheckForOverflowUnderflow`, so the tier is unchecked and the 2^30 rebase cannot
throw; `Math.FusedMultiplyAdd` is IEEE correctly-rounded on every lowering path
(JIT `vfmadd`, AOT libm call, ARM `fmadd`), so the AOT baseline is a speed cliff
and never a value one; `MetadataTest`'s public-constructor sweep is a closed
list of ten types, not a reflection walk, so 172 handle classes do not trip it;
`AllTests` discovers suites by reflection, so `StreamSmokeTest` needs no
registration; the C# server genuinely does not embed a `Core` copy, so a new
hand-written file in `library/` is picked up by both globs automatically; and
C# never had Java's #215 shallow-clone candle-poisoning hazard —
`CoreBuilder.Build()` hands over defensive clones and `CandleSetting` is
immutable, so the CDL snapshot is parity and defence-in-depth here, not a fix.

---

## 9. Decisions needed from the repo owner

1. **Commit at every stage boundary?** `regen_check` subtracts the already-dirty
   set, so an uncommitted eleven-stage tree makes both regeneration gates
   self-satisfying. The standing rule is no commit unless asked; this asks once,
   for the working branch only.
2. **Does the first NuGet push wait for the span decision?** `IsPackable=false`
   keeps the API-break window open, and it closes at the first push, not at this
   merge. Shipping on `double[]` with a two-line seam is only correct if no
   release happens between this merge and that flip. If a 0.8.1 push is planned
   first, the flip has to be scheduled in the same milestone — or the array
   surface is accepted as permanent, and `csharp.rs:8-14` should record that as
   the decision rather than a deferral.
3. **Should the PR gate build and test C#?** Today the C# suites run only as a
   side effect of `cargo run -- build --backend=csharp` inside nightly jobs;
   `pr-codegen-gate.yml` is regen-check only and never builds C#. So every gate
   in S1–S9 runs at *nightly* cadence on CI, with `dotnet run` as the
   developer-side substitute — a one-day feedback loop on a 3,400-line new
   emitter.
4. **How much C# website surface is in scope?** C# has *no* API section at all —
   no batch page, no sidebar block — so the stream page has no parent. S10
   assumes both get written. If only the stream page is in scope, S10 shrinks to
   a stub parent and the `install/README.md` correction defers.

**Two decisions taken that are worth a veto:** `Clone()` over Java's `copy()`
(the .NET verb, Rust's name, analyzer-clean — but it is a public API name across
172 types and a one-way door); and the census is **172**, not the doc's 161/165.
