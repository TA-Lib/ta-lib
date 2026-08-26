# SYNTH4

## Summary

Synthetic gate function: a smoothing recursion split across an explicit `_private` variant, so the guarded entry point pre-computes a decoupled real parameter and passes it down. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

S(i) = (x(i) - S(i-1)) * k + S(i-1), with k = 2 / (period + 1) supplied by the guarded entry point; outInteger[i] = ((int)(S(i) * 8)) mod 1024.

## Notes

- Covers the explicit `<name>_private` variant: a second entry point holding the algorithm, taking an extra parameter the guarded entry point pre-computes and passes down. Verified against `ta_codegen/input/`: no shipped body declares one — #183 folded the last one away — so this fixture is the construct's only coverage in the tree, across the parser, the IR, all four backends, the single-precision tier and the streaming emitter.
- Named by the generator's own suite, so this is a `cargo test` dependency and not only a nightly one: `src/backends/c.rs` (`test_c_private_omits_range_checks`) and `src/registry.rs` load it by name, and `ta_codegen/generator/CLAUDE.md` documents the `_Private` tier against it.
- Coverage trap: the guarded entry point derives the extra parameter from `optInTimePeriod` only AFTER the validation prologue has resolved a sentinel into the default. That ordering is the point of taking a derived parameter rather than a plain one; move the derivation above the prologue and the fixture stops pinning it.
- The recursion makes the derived parameter a float multiply operand, so this also pins FMA site selection: a private extra parameter is a parameter and never a body declaration, so no site here fuses — identically in all four backends. A backend that started fusing on its own breaks the bit parity the gate compares.
- Issue #183.

## Inputs

- `inReal` — Input series to smooth into the state

## Outputs

- `outInteger` — The quantized 10-bit state after each bar

## Parameters

- `optInTimePeriod` — Warm-up window, and the source of the smoothing factor the guarded variant derives

## Implementation

TA-Lib Definition: [`synth4.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth4/synth4.c) · [`synth4.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth4/synth4.yaml)
