---
title: "Vector Square Root (SQRT)"
description: "Element-wise square root of the input series."
---

## Summary

Element-wise square root of the input series.

## Formula

outReal[i] = sqrt(inReal[i])

## Notes

- A negative input has no real square root, so those elements come out NaN.

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Square root of each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`sqrt.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.c) · [`sqrt.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.yaml)

| Native | File |
|--------|------|
| C | [`ta_SQRT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SQRT.c) |
| Rust | [`sqrt.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sqrt.rs) |
| Java | [`Core_SQRT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SQRT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Square Root

## References

- Wikipedia, *Square root*: [en.wikipedia.org/wiki/Square_root](https://en.wikipedia.org/wiki/Square_root)
