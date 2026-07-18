---
title: PVI
description: "Positive Volume Index: a running cumulative index that changes only on days when volume rises versus the prior day, compounding that day's percentage price change. The premise is that active, high-volume days reflect the actions of the less-informed \"crowd\", so PVI is read as a proxy for that cohort's positioning."
---

# PVI

## Summary

Positive Volume Index: a running cumulative index that changes only on days when
volume rises versus the prior day, compounding that day's percentage price change.
The premise is that active, high-volume days reflect the actions of the
less-informed "crowd", so PVI is read as a proxy for that cohort's positioning.

## Formula

PVI[startIdx] = 1000

For each subsequent bar i:

    PVI[i] = PVI[i-1] + ( inVolume[i] > inVolume[i-1]
                          ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * PVI[i-1]
                          : 0 )

The index carries forward unchanged on bars whose volume did not rise (and on the
degenerate case of a zero previous close, which would otherwise divide by zero).

## Inputs

- `inClose` — Closing price series, providing the bar-over-bar percentage change
- `inVolume` — Volume series, compared bar-over-bar to gate each update

## Outputs

- `outReal` — Cumulative positive volume index (seeded at 1000)

## Implementation

TA-Lib Definition: [`pvi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvi/pvi.c) · [`pvi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvi/pvi.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVI.c) |
| Rust | [`pvi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvi.rs) |
| Java | [`Core_PVI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_PVI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Positive Volume Index

## References

- Norman G. Fosback, *Stock Market Logic*, The Institute for Econometric Research (ISBN 0917604482)
