---
title: OBV
description: "On Balance Volume: a running cumulative total of volume, added on up-price bars and subtracted on down-price bars. Relates volume flow to price direction."
---

# OBV

## Summary

On Balance Volume: a running cumulative total of volume, added on up-price bars and subtracted on down-price bars. Relates volume flow to price direction.

## Formula

OBV[i] = OBV[i-1] + (inReal[i] > inReal[i-1] ? V[i] : inReal[i] < inReal[i-1] ? -V[i] : 0); seed OBV[startIdx] = V[startIdx]

## Inputs

- `inReal` — Price series, typically close
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative on-balance volume

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Path-Dependent** <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`obv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/obv/obv.c) · [`obv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/obv/obv.yaml)

| Native | File |
|--------|------|
| C | [`ta_OBV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_OBV.c) |
| Rust | [`obv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/obv.rs) |
| Java | [`Core_OBV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_OBV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

On Balance Volume

## References

- Joseph Ensign Granville, B. Granville, *Granville's New Strategy of Daily Stock Market Timing for Maximum Profit*, Simon & Schuster (ISBN 0133634329)
