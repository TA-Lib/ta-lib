//! Generates the shipped C# indicator implementations — a port of `java.rs` over
//! the shared `ExprEmitter` / `StatementEmitter` walkers.
//!
//! **Status: stub.** `CSharpBackend::emits_lib_files()` is still `false` and the
//! C# JSON-RPC server is still the P/Invoke harness, so nothing calls this yet.
//! The decisions below were settled before any emitter code was written; they are
//! recorded here so the port does not relitigate them mid-way.
//!
//! # Pinned decisions
//!
//! - **Public array surface is `double[]`, not `Span<double>`.** Three separate
//!   mechanisms are array *reference equality*: BBANDS' scratch election
//!   (`if (inReal == outRealUpperBand)`), the output-distinctness reject (#108),
//!   and the `float[]`-overload comparison fold. `Span<T>` has no `==`, so
//!   choosing spans breaks all three at once and drags Rust's `ScratchElection`
//!   pass into C#. A span-based facade can sit on top later, where reference
//!   identity does not apply.
//!
//! - **Cores return `RetCode`; only the public wrapper throws.** Cross-indicator
//!   callees return RetCode that callers *inspect* (MA, BBANDS, MAVP, STOCHRSI,
//!   SAR). Throwing cores would make those inspection sites dead code the
//!   renderer has to delete. Keeping RetCode internally means zero body changes.
//!
//! - **Scratch buffers are `new double[n]`, not `ArrayPool`, in v1.** ArrayPool
//!   has no lifetime story in the current IR lowering: Java renders
//!   `CircBuf::Destroy` and `free()` as the empty string because it is GC'd, and
//!   11 malloc'ing functions plus 8 runtime-sized CIRCBUF functions each have
//!   ~10 early-return paths that would every one need a matching `Return`.
//!   Revisit as a perf task only after the bitwise gate is green.
//!
//! - **One file per indicator, `public partial class Core`.** This replaces
//!   Java's GENCODE-marker splicing, deletes the `java_shipped.rs` equivalent
//!   outright, and makes `generate --func=SMA` correct for the first time (the
//!   Java path disables shipped-`Core.java` regeneration under `--func` because
//!   a subset would drop every other indicator from the spliced file).
//!
//! - **`out int outBegIdx, out int outNBElement`, with a generator-emitted
//!   `outBegIdx = 0; outNBElement = 0;` seeding prologue.** Every guarded core
//!   returns `RetCode.OutOfRangeStartIndex` before either is assigned, which is
//!   CS0177 on ~670 generated methods. The seeding must not perturb any emitted
//!   value.
//!
//! - **Qualified switch labels (`case MAType.Sma:`) plus a mandatory `default:`
//!   arm** on any switch assigning a local that is read afterwards. This is the
//!   exact inverse of `render_java_switch_label`, which strips the qualifier
//!   because qualified labels are Java 21+.
//!
//! - **Method names are PascalCase**, i.e. an identity pass over the YAML
//!   `camel_case`. This must agree with `Lang::CSharp` in `registry.rs` or every
//!   cross-indicator call targets a method that does not exist.

use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, FuncDef};
use crate::registry::Registry;
use std::collections::HashMap;

/// Render the per-indicator C# source for `func`.
///
/// Returns an empty string while the backend is a stub; `CSharpBackend`'s
/// `emits_lib_files()` is `false`, so no caller writes the result.
#[allow(clippy::implicit_hasher)]
pub fn generate(
    _func: &FuncDef,
    _enums: &HashMap<String, EnumDef>,
    _registry: &Registry,
    _helpers: &HelperRegistry,
) -> String {
    String::new()
}
