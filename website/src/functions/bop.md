---
title: BOP
description: "Balance Of Power compares where the close sits relative to the open, normalized by the bar's high-low range. A per-bar oscillator with no smoothing. Positive: close above open (buyers dominated); negative: sellers dominated."
---

# BOP

## Summary

Balance Of Power compares where the close sits relative to the open, normalized by the bar's high-low range. A per-bar oscillator with no smoothing. Positive: close above open (buyers dominated); negative: sellers dominated.

## Formula

BOP = (Close - Open) / (High - Low)

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Balance of Power value per bar

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`bop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bop/bop.c) · [`bop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bop/bop.yaml)

| Native | File |
|--------|------|
| C | [`ta_BOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BOP.c) |
| Rust | [`bop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/bop.rs) |
| Java | [`Core_BOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_BOP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Balance Of Power, Balance of Power
