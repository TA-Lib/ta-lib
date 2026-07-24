---
title: HT_DCPERIOD
description: "Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period. Output is the estimated dominant cycle length in bars (clamped to 6-50)."
---

# HT_DCPERIOD

## Summary

Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period. Output is the estimated dominant cycle length in bars (clamped to 6-50).

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Smoothed dominant cycle period in bars

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">✅</span> **Initial Unstable Period** <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`ht_dcperiod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.c) · [`ht_dcperiod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_DCPERIOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_DCPERIOD.c) |
| Rust | [`ht_dcperiod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_dcperiod.rs) |
| Java | [`Core_HT_DCPERIOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_HT_DCPERIOD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform Dominant Cycle Period, Dominant Cycle Period

## See Also

[HT_DCPHASE](/functions/ht_dcphase) · [HT_PHASOR](/functions/ht_phasor) · [HT_SINE](/functions/ht_sine) · [HT_TRENDMODE](/functions/ht_trendmode) · [MAMA](/functions/mama) · [WMA](/functions/wma)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
