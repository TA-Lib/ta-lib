# ta-lib-dispatch

Runtime CPU-feature dispatch for one hot path in the
[`ta-lib`](https://crates.io/crates/ta-lib) technical-analysis crate: fused
multiply-add (FMA).

## Why this exists

A crate published to crates.io has to run on every x86-64 CPU, including
machines from before hardware FMA existed (pre-2013) — so `ta-lib` cannot
simply compile with `target-feature=+fma`, or it would `SIGILL` on that
hardware. The safe fallback is a software `fma()` call through libm, but on
any CPU since Haswell that throws away a real hardware instruction
(`vfmadd`) for a slow function call. Measured on `ta-lib`'s own FMA-heavy
indicators before this crate existed, that cost was not subtle:

| function  | libm `fma()` (baseline)    | hardware `vfmadd` (this crate) |
|-----------|-----------------------------|---------------------------------|
| T3        | 7.29x slower than the C reference | 1.12x |
| WCLPRICE  | 6.56x slower                | 1.20x |
| ADOSC     | 4.99x slower                | 1.62x |

`ta-lib-dispatch` closes that gap with one macro: check the CPU once at
runtime, then call whichever of two compiled clones — one built with
`#[target_feature(enable = "fma")]`, one plain — actually matches the
hardware underneath. Both clones compute IEEE-754 correctly-rounded FMAs, so
which one runs changes speed, never bits — `ta-lib`'s cross-language
bitwise regression suite is green either way. Full measurement methodology:
[TA-Lib/ta-lib#156](https://github.com/TA-Lib/ta-lib/issues/156).

## Not a novel trick

Runtime CPU-feature dispatch for hot arithmetic is established practice,
not something specific to this crate:

- glibc resolves `memcpy`, `strlen`, and `fma()` itself this way, via
  `ifunc`, for every Linux binary — `ta-lib`'s own C backend uses the
  compiler-level equivalent (`__attribute__((target_clones(...)))`) for the
  same reason.
- [`memchr`](https://crates.io/crates/memchr) — a dependency of `regex` and
  most of the crates.io ecosystem — dispatches its SIMD byte-search
  routines at runtime the same way: a feature check, then a call into
  whichever compiled variant matches the CPU.

## Do you need this crate directly?

Almost certainly not. If you depend on `ta-lib`, this crate is already
pulled in transitively — `ta-lib` depends on an exact-pinned
`ta-lib-dispatch` version, since the macro is an internal contract between
the two crates, not a versioned public API. There's nothing here meant to
be called from outside the `ta-lib` workspace.

## What's inside

One macro, `dispatch_fma!`, and the single `unsafe` block in the entire
`ta-lib` workspace — isolated here on purpose, so the main `ta-lib` crate
can stay `#![forbid(unsafe_code)]`. That `unsafe` is one hardware-intrinsic
call, guarded immediately above by the feature-detection check that makes
it sound.

## License

BSD-3-Clause — see
[LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
