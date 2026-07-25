---
title: TRANGE
description: "True Range: the greatest of today's high-low span and the two gaps between yesterday's close and today's high/low. Base volatility measure used to build ATR/NATR. Larger values mean wider or gappier bars (higher volatility)."
---

# TRANGE

## Summary

True Range: the greatest of today's high-low span and the two gaps between yesterday's close and today's high/low. Base volatility measure used to build ATR/NATR. Larger values mean wider or gappier bars (higher volatility).

## Formula

TR = max( high - low, |prevClose - high|, |prevClose - low| )

## Notes

- The first bar produces no value because it has no prior close; unlike some definitions, it does not fall back to the high-low range for that bar.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — True Range value per bar

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`trange.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trange/trange.c) · [`trange.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trange/trange.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRANGE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRANGE.c) |
| Rust | [`trange.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/trange.rs) |
| Java | [`Core_TRANGE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_TRANGE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

True Range, TR

## See Also

[ATR](/functions/atr) · [NATR](/functions/natr)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
