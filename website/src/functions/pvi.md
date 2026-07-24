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

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative positive volume index (seeded at 1000)

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Path-Dependent** <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

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
