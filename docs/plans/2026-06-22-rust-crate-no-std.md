# Tracking: make the generated Rust crate `#![no_std]`

**Status:** Deferred / tracking only — do NOT forget. This is a follow-on cleanup to
the Rust abstract/introspection layer work, not a blocker for it.

**Assumption (to validate):** crate-wide `no_std` is strictly better and feasible.
Benefits: works on any target with (or eventually without) an allocator — WASM,
embedded — and enforces the same zero-/bounded-allocation discipline the C reference
already has with its stack temp buffers.

## Current state

- The generated crate `ta_codegen_output/rust/` is **`std`-only** — it declares
  `no_std` nowhere.
- The **only** heap blocker is runtime-sized `Vec<f64>` **temp/scratch buffers** in
  **17 of 163** indicator files. There is **no `String`/`format!` usage** anywhere in
  the indicator code.
- The forthcoming abstract/introspection module is `no_std`-clean by design
  (`&'static`/`const` tables, enums, slices — zero alloc), so it does not add to this
  debt.

### The 17 files using `Vec` temp buffers

`accbands, adxr, apo, atr, bbands, dema, ma, macd, macdext, mavp, natr, ppo, stoch,
stochf, stochrsi, tema, trix`

Example (atr.rs): `let mut tempBuffer: Vec<f64> = Vec::new(); ... tempBuffer =
vec![0.0_f64; ((lookbackTotal + (endIdx - startIdx) + 1) * 1) as usize];` — a
range-sized scratch buffer, mirroring the C reference's stack/`ARRAY` temp buffers.

## Approach options

**This is GENERATED code** — the fix belongs in the generator
(`tools/ta_codegen/src/backends/rust_lang.rs` temp-buffer emission, plus the crate
scaffolding emitted from `tools/ta_codegen/src/main.rs`). Do **not** hand-edit
`ta_codegen_output/`.

- **Option A — `extern crate alloc` (minimal, low risk, recommended first).**
  Emit `#![no_std]` + `extern crate alloc;` in `lib.rs` and switch the temp buffers to
  `alloc::vec::Vec`. The crate becomes `no_std` and runs on any target with an
  allocator (incl. WASM). Smallest diff; no signature/algorithm changes.

- **Option B — true zero-alloc (higher fidelity to C, more work).**
  Replace the runtime-sized `Vec` scratch with caller-provided scratch or
  bounded/stack buffers, matching C's stack temp buffers — removes the allocator
  requirement entirely. Touches indicator codegen for the 17 functions and possibly
  the public surface (scratch argument or an arena). Defer unless a no-allocator
  target is actually needed.

Recommended path: **A first** (broadly unblocks `no_std` with minimal risk), revisit
**B** only if an allocator-free target becomes a real requirement.

## Acceptance criteria

- [ ] Crate builds with `#![no_std]` (e.g. a `no_std` smoke/CI target).
- [ ] `ta_regtest --codegen --language=rust` still **161/161** — no behavior change.
- [ ] No hot-path (per-bar compute) perf regression vs the current `std` build.
- [ ] The abstract/introspection module remains `no_std`-clean.
- [ ] Change lives in the generator + scaffolding; `ta_codegen_output/` is regenerated,
      never hand-edited.

## See also

- Rust abstract/introspection layer (the active next step this follows).
- `docs/plans/2026-03-08-alpha-parity-design.md` (alpha scope; `no_std` was never in it).
