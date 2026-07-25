---
title: CDLCONCEALBABYSWALL
description: "A four-candle pattern: two black marubozus, then a black candle that gaps down but pokes its upper shadow into the prior body, then a larger black candle fully engulfing the third. Bullish reversal signal. A hit signals a bullish reversal."
---

# CDLCONCEALBABYSWALL

## Summary

A four-candle pattern: two black marubozus, then a black candle that gaps down but pokes its upper shadow into the prior body, then a larger black candle fully engulfing the third. Bullish reversal signal. A hit signals a bullish reversal.

## Notes

- Does not verify the preceding downtrend the pattern classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a match, 0 otherwise; never emits -100 (pattern is always bullish)

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlconcealbabyswall.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlconcealbabyswall/cdlconcealbabyswall.c) · [`cdlconcealbabyswall.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlconcealbabyswall/cdlconcealbabyswall.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCONCEALBABYSWALL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCONCEALBABYSWALL.c) |
| Rust | [`cdlconcealbabyswall.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlconcealbabyswall.rs) |
| Java | [`Core_CDLCONCEALBABYSWALL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLCONCEALBABYSWALL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Concealing Baby Swallow

## See Also

[CDLMARUBOZU](/functions/cdlmarubozu) · [CDLENGULFING](/functions/cdlengulfing)
