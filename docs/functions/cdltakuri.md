---
title: CDLTAKURI
description: "Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify."
---

# CDLTAKURI

## Summary

Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the takuri pattern is detected, 0 otherwise. Never negative; the positive sign is a convention and does not by itself imply bullishness

## Implementation

TA-Lib Definition: [`cdltakuri.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.c) · [`cdltakuri.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTAKURI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTAKURI.c) |
| Rust | [`cdltakuri.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdltakuri.rs) |
| Java | [`Core_CDLTAKURI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLTAKURI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Takuri, Takuri line

## See Also

[CDLDRAGONFLYDOJI](/functions/cdldragonflydoji) · [CDLDOJI](/functions/cdldoji) · [CDLHAMMER](/functions/cdlhammer) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji)
