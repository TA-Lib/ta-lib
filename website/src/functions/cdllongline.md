---
title: CDLLONGLINE
description: "A single-candle pattern: a long real body with short upper and short lower shadow. The signal direction follows the candle color (bullish if white, bearish if black). Signals strong directional conviction on the bar: +100 white/bullish, -100 black/bearish. Not intrinsically a reversal or continuation signal."
---

# CDLLONGLINE

## Summary

A single-candle pattern: a long real body with short upper and short lower shadow. The signal direction follows the candle color (bullish if white, bearish if black). Signals strong directional conviction on the bar: +100 white/bullish, -100 black/bearish. Not intrinsically a reversal or continuation signal.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a white (close>=open) long line, -100 on a black long line, 0 when no pattern

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdllongline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.c) · [`cdllongline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLINE.c) |
| Rust | [`cdllongline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdllongline.rs) |
| Java | [`Core_CDLLONGLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLLONGLINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Long Line Candle, Long Line

## See Also

[CDLSHORTLINE](/functions/cdlshortline) · [CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu) · [CDLMARUBOZU](/functions/cdlmarubozu) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji)
