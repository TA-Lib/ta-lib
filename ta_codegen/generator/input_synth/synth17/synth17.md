# SYNTH17

## Summary

Synthetic gate function: for an odd period, the input less the truncated input `optInTimePeriod-1` bars earlier (SYNTH15); for an even period, the input `optInTimePeriod-1` bars earlier less the input. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

odd n: outReal[i] = inReal[i] - trunc(inReal[i-(n-1)]), trunc clamping to 0 outside (-1e6, 1e6); even n: outReal[i] = inReal[i-(n-1)] - inReal[i]; n = 1 copies the input.

## Notes

- Covers a split Rust stream state in the dual-mode tier (#439): each arm's step stores into its own heap buffer inside a loop, `int` in one arm and `double` in the other. Each arm's opener therefore builds the other arm's buffer from its default, and the period-1 identity path in the shared prologue builds both. TRIMA, HMA and the DI/DM family are the only shipped dual-mode states, and none stores into a buffer inside a loop.
- Coverage trap: the arms must keep DIFFERENT buffers. Sharing one leaves no buffer that only the other arm holds, and the default-built half of each opener goes unexercised with every leg green.
- Coverage trap: the shifts must stay loops that store into the buffers, and the output store stays above every buffer store, as in SYNTH15.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The difference against the input `n-1` bars earlier, its sign and truncation set by the parity of `n`

## Parameters

- `optInTimePeriod` — Window length n; its parity selects the arm, and 1 takes the identity path

## Implementation

TA-Lib Definition: [`synth17.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth17/synth17.c) · [`synth17.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth17/synth17.yaml)
