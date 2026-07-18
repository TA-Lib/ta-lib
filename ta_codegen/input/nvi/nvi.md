# NVI

## Summary

Negative Volume Index: a running cumulative index that changes only on days when
volume falls versus the prior day, compounding that day's percentage price change.
The premise is that quiet, low-volume days reflect the actions of well-informed
"smart money", so NVI is read as a proxy for that cohort's positioning.

## Formula

NVI[startIdx] = 1000

For each subsequent bar i:

    NVI[i] = NVI[i-1] + ( inVolume[i] < inVolume[i-1]
                          ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * NVI[i-1]
                          : 0 )

The index carries forward unchanged on bars whose volume did not fall (and on the
degenerate case of a zero previous close, which would otherwise divide by zero).

## Inputs

- `inClose` — Closing price series, providing the bar-over-bar percentage change
- `inVolume` — Volume series, compared bar-over-bar to gate each update

## Outputs

- `outReal` — Cumulative negative volume index (seeded at 1000)

## Implementation

TA-Lib Definition: [`nvi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/nvi/nvi.c) · [`nvi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/nvi/nvi.yaml)

| Native | File |
|--------|------|
| C | [`ta_NVI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_NVI.c) |
| Rust | [`nvi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/nvi.rs) |
| Java | [`Core_NVI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_NVI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Negative Volume Index

## References

- Norman G. Fosback, *Stock Market Logic*, The Institute for Econometric Research (ISBN 0917604482)
