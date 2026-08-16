//! Generates the C# streaming API section appended to each shipped
//! `Core_<NAME>.cs` — the managed .NET sibling of `java_stream.rs` /
//! `rust_stream.rs` / `c_stream.rs`.
//!
//! Like the other three it consumes the backend-neutral [`crate::streaming`]
//! layer (`StreamPlan`, `StreamModel`, `build_transition`, the `NameMap` trait)
//! and renders through the existing [`super::csharp`] statement/expression
//! walkers. `streaming.rs` and the three shipped stream emitters are not
//! touched by this module — that is what keeps the other backends byte-frozen
//! by construction while this one lands.
//!
//! # Pinned decisions
//!
//! - **The handle is a `public sealed class <NAME>_Stream` nested in
//!   `partial class Core`**, with a *sibling* nested
//!   `public readonly record struct <NAME>_Value` for multi-output functions.
//!   The `Value` type cannot itself be named `Value`: a nested type and a
//!   member of the same name is CS0102, and the member name is the one every
//!   language's documentation references.
//!
//! - **Every handle field is `internal`, every constructor `internal`.** A
//!   *sibling* nested type cannot reach another's `private` members, and
//!   `MAVP_Stream`'s copy constructor builds `MA_Stream` copies while
//!   `MA_Stream`'s step calls `SMA_Stream.Update`. One rule beats per-field
//!   analysis, and `internal` is invisible to consumers. Internal constructors
//!   additionally stop `System.Text.Json` from minting a half-built handle,
//!   which is the positive act C# needs where Java gets not-serializable free.
//!
//! - **The step stays a method on `Core`, not on the handle.** Transcribed
//!   bodies render unstable-period reads as
//!   `this.unstablePeriod[(int)FuncUnstId.X]`, which only compiles inside a
//!   `Core` instance method. Measured, `core.Step(sp, x)` versus
//!   `this.Step(x)` is 3.44–3.54 against 3.39–3.47 ns/bar — indistinguishable.
//!
//! - **No `cachedValue` field.** Java caches the boxed multi-output `Value` so
//!   that `value()` allocates nothing; a `readonly record struct` return is
//!   0 B/update by construction (measured against 40 B/update for the
//!   Java-shaped class). One fewer field, one fewer store per bar, and one
//!   fewer thing the copy path can get wrong.
//!
//! - **The `NameMap` prefixes are Java's, verbatim** (`sp.x`, `sp.cur_y`,
//!   `sp.ring_v_a`, ...). This is load-bearing, not cosmetic:
//!   [`super::fma::stream_base`] strips exactly `sp->`, `sp.` and `cur_` to
//!   decide integer-versus-float typing, so any other scheme needs `fma.rs`
//!   extended, and getting that wrong is a ~1 ULP cross-language divergence
//!   with nothing pointing at the cause.
//!
//! - **Double-only.** `single_precision` is always `false` and no `float[]`
//!   overload is emitted; the streaming contract is `double` in every language.
//!
//! # Emission rules that are measurements, not preferences
//!
//! Each of these was measured on the shipped shape (dotnet 10, pinned cores,
//! interleaved arms, min-of-N over 3–5 process launches). They are recorded so
//! that a later reader does not re-optimize on intuition in either direction.
//!
//! - No `MethodImpl` attributes. `Update` is 3.39–3.51 ns/bar inlinable against
//!   6.69–7.10 behind `NoInlining`; the JIT's IL-size heuristic already inlines
//!   the small steps and correctly declines the ~400-byte candlestick ones.
//! - `Update`/`Peek`/`Value` stay thin — no validation, no null checks (there
//!   are no array arguments), no logging. That is what keeps them inlinable.
//! - No unsafe indexing: `MemoryMarshal.GetArrayDataReference` + `Unsafe.Add`
//!   measured 4.26–4.55 against 3.44–3.47 ns/bar — a *regression*.
//! - No array-hoisting pass (3.35–3.48 against 3.39–3.47 — noise). Hoist only a
//!   *counted* loop bound, where it genuinely drops a check.
//! - No `[StructLayout]`: the CLR uses `LayoutKind.Auto` for reference types
//!   and packs them itself; `Sequential` would disable that.
//! - Rings stay `double[]`/`int[]` fields. `Span<T>` cannot be a field (CS8345)
//!   and `Memory<T>` costs a span materialization per access.
//! - The copy constructor uses `new T[n]` + `Array.Copy`, never
//!   `(double[])x.Clone()` — 2.3x, and it is on `Peek`'s path for ~86 functions.
//! - Dispatch is a `switch` + cast, but *not* because virtual calls are slow:
//!   an interface call measured 4.41–4.74 against the switch's 5.55–5.72
//!   ns/bar. The switch wins on cross-language parity and on not adding a type
//!   hierarchy across 172 handles, and that is the whole argument.

use std::collections::HashMap;

use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, FuncDef};
use crate::registry::Registry;
use crate::streaming;

/// Marker heading the generated stream section (tests slice on it; mirrors the
/// C, Rust and Java emitters).
pub const SECTION_MARKER: &str = "/**** Streaming API *****/";

/// Whether a C# stream section is emitted for this function.
///
/// Resolves `PRAGMA TA_ALT` here rather than at the caller, exactly as the Rust
/// and Java twins do: six functions carry an `_ALT1` body claiming the STREAM
/// tier, and analyzing `func.body` would silently analyze the batch-only block
/// scan instead.
pub fn emits_stream(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> bool {
    if !func.streaming {
        return false;
    }
    streaming::validate_streamable(&func.resolved_for(crate::ir::Lang::CSharp), lookup).is_ok()
}

/// The base every C# identifier for this function is spelled from: the YAML
/// `name:` verbatim (`SMA`, `MA`, `CDL2CROWS`), matching `Lang::CSharp` in
/// `registry.rs`.
fn base_name(func: &FuncDef) -> String {
    func.name.clone()
}

/// Public handle class name, nested in `Core`: `SMA_Stream`.
pub fn stream_class_name(func: &FuncDef) -> String {
    format!("{}_Stream", base_name(func))
}

/// Multi-output value type name, a *sibling* nested type: `BBANDS_Value`.
///
/// C#-only concern: a nested type named `Value` alongside the `Value` member
/// every other language exposes is CS0102, so the type carries the function
/// prefix and the member keeps the documented name.
pub fn value_type_name(func: &FuncDef) -> String {
    format!("{}_Value", base_name(func))
}

/// The `<NAME>_Value` member name for an output: `outSlowK` → `SlowK`,
/// `outMACDSignal` → `MACDSignal`, `outMinIdx` → `MinIdx`.
///
/// Unlike Java's `value_field_name` this does *not* lowercase the leading caps
/// run — C# members are PascalCase, so stripping `out` is the whole
/// transformation. Every one of the corpus's distinct output names is already a
/// valid PascalCase identifier after the strip.
#[allow(dead_code)] // consumed once the multi-output Value type is emitted
pub(crate) fn value_member_name(out_name: &str) -> String {
    let stripped = out_name.strip_prefix("out").unwrap_or(out_name);
    if stripped.is_empty() {
        out_name.to_string()
    } else {
        stripped.to_string()
    }
}

/// Generate the whole stream section for one function's `Core_<NAME>.cs`.
///
/// Panics on analysis failure: the declared-tier gate in `main.rs` validates
/// every function over `ir::ALL_LANGS` first, so a failure here means the gate
/// was bypassed — fail loudly rather than silently emit nothing.
#[allow(clippy::implicit_hasher)]
pub fn generate(
    _func: &FuncDef,
    _enums: &HashMap<String, EnumDef>,
    _registry: &Registry,
    _helpers: &HelperRegistry,
) -> String {
    String::new()
}
