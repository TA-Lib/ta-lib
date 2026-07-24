---
title: CMO
description: "Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed average up-moves and down-moves. Identical to RSI except the numerator uses (gain-loss) instead of gain. Bounded in [-100,+100]; positive = net upward momentum, negative = net downward."
---

# CMO

## Summary

Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed average up-moves and down-moves. Identical to RSI except the numerator uses (gain-loss) instead of gain. Bounded in [-100,+100]; positive = net upward momentum, negative = net downward.

## Formula

d = P[t]-P[t-1]; over the initial period accumulate gain = sum of positive d, loss = sum of -d for negative d. Wilder-smooth each: prevGain = (prevGain*(period-1) + gain_today)/period (same for loss). CMO = 100 * (prevGain-prevLoss)/(prevGain+prevLoss); 0 when prevGain+prevLoss == 0.

## Notes

- Gains and losses are smoothed with Wilder's method (as in RSI) rather than the simple period sums of Chande's original definition.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — CMO oscillator value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Bars over which gains/losses are smoothed |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">✅</span> **Initial Unstable Period** <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cmo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmo/cmo.c) · [`cmo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmo/cmo.yaml)

| Native | File |
|--------|------|
| C | [`ta_CMO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CMO.c) |
| Rust | [`cmo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cmo.rs) |
| Java | [`Core_CMO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CMO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Chande Momentum Oscillator

## See Also

[RSI](/functions/rsi)

## References

- Tushar S. Chande, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
