---
title: "Vector Floor (FLOOR)"
description: "Element-wise floor (round down to the nearest integer) of the input series."
---

## Summary

Element-wise floor (round down to the nearest integer) of the input series.

## Formula

outReal[i] = floor(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Each input rounded down to nearest integer

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`floor.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/floor/floor.c) · [`floor.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/floor/floor.yaml)

| Native | File |
|--------|------|
| C | [`ta_FLOOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FLOOR.c) |
| Rust | [`floor.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/floor.rs) |
| Java | [`Core_FLOOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FLOOR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

[CEIL](/functions/ceil.md)

## References

- Wikipedia, *Floor and ceiling functions*: [en.wikipedia.org/wiki/Floor_and_ceiling_functions](https://en.wikipedia.org/wiki/Floor_and_ceiling_functions)
