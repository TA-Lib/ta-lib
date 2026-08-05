# ta-lib-dispatch

Runtime CPU-feature dispatch for fused multiply-add (FMA), used internally
by the [`ta-lib`](https://crates.io/crates/ta-lib) crate — part of the
[TA-Lib](https://ta-lib.org) project.

## Why this exists

[Fused multiply-add](https://en.wikipedia.org/wiki/Multiply%E2%80%93accumulate_operation)
(FMA) computes `a * b + c` in one rounding step instead of two, and has
been a hardware x86-64 instruction since 2013 (Haswell). A published crate
has to run on any x86-64 CPU, though, so it can't require that instruction
at compile time — the safe fallback is a software `fma()` call through
libm, which costs up to ~7x in execution speed on FMA-heavy code.

This crate is one macro: check `is_x86_feature_detected!("fma")` once,
then call whichever of two compiled clones — one built with
`#[target_feature(enable = "fma")]`, one without — matches the CPU. Both
produce identical, correctly-rounded results; only speed changes.

## Not a novel trick

Runtime CPU-feature dispatch for numerical code is standard practice —
NumPy and OpenBLAS both select their FMA/SIMD kernels the same way, at
runtime, based on detected CPU features.

## Do you need this crate directly?

Almost certainly not. `ta-lib` already depends on an exact-pinned version
of this crate — the macro is an internal contract between the two, not a
public API.

## License

BSD-3-Clause — see
[LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
