---
title: ADOSC
description: "Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the Accumulation/Distribution line. Highlights momentum in accumulation/distribution volume flow. Positive/rising suggests accumulation; negative/falling suggests distribution."
---

# ADOSC

## Summary

Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the Accumulation/Distribution line. Highlights momentum in accumulation/distribution volume flow. Positive/rising suggests accumulation; negative/falling suggests distribution.

## Formula

ad += ((close-low)-(high-close))/(high-low) * volume   (only when high>low)
fastEMA = fastk*ad + (1-fastk)*fastEMA,  fastk = 2/(optInFastPeriod+1)
slowEMA = slowk*ad + (1-slowk)*slowEMA,  slowk = 2/(optInSlowPeriod+1)
ADOSC = fastEMA - slowEMA

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Fast-EMA minus slow-EMA of the A/D line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 3 | 2–100000 | Period of the fast A/D EMA |
| `optInSlowPeriod` | integer | 10 | 2–100000 | Period of the slow A/D EMA |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Path-Dependent** <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`adosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adosc/adosc.c) · [`adosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adosc/adosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADOSC.c) |
| Rust | [`adosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/adosc.rs) |
| Java | [`Core_ADOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ADOSC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Chaikin A/D Oscillator, Chaikin Oscillator

## See Also

[AD](/functions/ad) · [EMA](/functions/ema)

## References

- Marc Chaikin
