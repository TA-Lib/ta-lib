---
title: NVI
description: "Negative Volume Index: a running cumulative index that changes only on days when volume falls versus the prior day, compounding that day's percentage price change. The premise is that quiet, low-volume days reflect the actions of well-informed \"smart money\", so NVI is read as a proxy for that cohort's positioning."
---

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

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative negative volume index (seeded at 1000)

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Path-Dependent** <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

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
