---
title: TRIX
description: "1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum oscillator that filters out price moves shorter than the chosen period. Oscillates around zero; sign, zero-crossings and slope signal momentum direction."
---

# TRIX

## Summary

1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum oscillator that filters out price moves shorter than the chosen period. Oscillates around zero; sign, zero-crossings and slope signal momentum direction.

## Formula

E1 = EMA(inReal, n); E2 = EMA(E1, n); E3 = EMA(E2, n); TRIX = ROC_1(E3) = 100 * (E3_today/E3_yesterday - 1)

## Notes

- The final rate-of-change step yields 0 when the previous smoothed value is exactly zero, rather than being undefined.

## Inputs

- `inReal` — Source series to smooth

## Outputs

- `outReal` — 1-day percent ROC of the triple EMA

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | EMA period used at each of the three smoothing passes |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`trix.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trix/trix.c) · [`trix.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trix/trix.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRIX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRIX.c) |
| Rust | [`trix.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/trix.rs) |
| Java | [`Core_TRIX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_TRIX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Triple Exponential Average

## See Also

[EMA](/functions/ema) · [ROC](/functions/roc) · [ROCR](/functions/rocr) · [TEMA](/functions/tema)

## References

- Jack K. Hutson, Technical Analysis of Stocks & Commodities (1980s)
