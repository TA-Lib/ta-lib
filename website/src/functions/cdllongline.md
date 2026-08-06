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

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdllongline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.c) · [`cdllongline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLINE.c) |
| Rust | [`cdllongline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdllongline.rs) |
| Java | [`Core_CDLLONGLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLLONGLINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Long Line Candle, Long Line

## See Also

[CDLSHORTLINE](/functions/cdlshortline) · [CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu) · [CDLMARUBOZU](/functions/cdlmarubozu) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji)
