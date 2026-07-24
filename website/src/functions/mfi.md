---
title: MFI
description: "Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing positive vs negative money flow over a period. A volume-based analog of RSI. >80 overbought, <20 oversold."
---

# MFI

## Summary

Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing positive vs negative money flow over a period. A volume-based analog of RSI. >80 overbought, <20 oversold.

## Formula

TP = (High+Low+Close)/3; MF = TP*Volume, classed positive if TP>prevTP, negative if TP<prevTP, neither if equal. MFI = 100 * posSumMF/(posSumMF+negSumMF).

## Notes

- When the typical price is unchanged from the prior bar, that bar's money flow is counted as neither positive nor negative.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Money Flow Index

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window for summing money flow |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`mfi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mfi/mfi.c) · [`mfi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mfi/mfi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MFI.c) |
| Rust | [`mfi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mfi.rs) |
| Java | [`Core_MFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MFI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Money Flow Index

## See Also

[RSI](/functions/rsi) · [AD](/functions/ad) · [ADOSC](/functions/adosc)

## References

- Gene Quong & Avrum Soudack, *Volume-Weighted RSI: Money Flow*, Technical Analysis of Stocks & Commodities, V.7:3 (March 1989)
