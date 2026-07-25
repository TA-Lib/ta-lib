---
title: FLOOR
description: "Vector floor: rounds each input value down to the nearest integer. Element-wise math transform."
---

# FLOOR

## Summary

Vector floor: rounds each input value down to the nearest integer. Element-wise math transform.

## Formula

outReal[i] = floor(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Each input rounded down to nearest integer

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`floor.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/floor/floor.c) · [`floor.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/floor/floor.yaml)

| Native | File |
|--------|------|
| C | [`ta_FLOOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FLOOR.c) |
| Rust | [`floor.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/floor.rs) |
| Java | [`Core_FLOOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_FLOOR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## See Also

[CEIL](/functions/ceil)
