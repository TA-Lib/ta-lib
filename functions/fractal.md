---
url: 'https://ta-lib.org/functions/fractal.md'
description: 'Williams Fractal: a causal swing-pivot detector.'
---
# Williams Fractal (FRACTAL)

## Summary

Williams Fractal: a causal swing-pivot detector. A bar is a swing high when its high strictly exceeds the highs of the `optInLeftBars` bars before it and the `optInRightBars` bars after it; a swing low is the mirror on the lows. Bill Williams' original is the symmetric five-candle case; independent left and right arms generalise it.

The right arm cannot be known until it has closed, so the value is written on the bar that completes the pivot, `optInRightBars` bars after the pivot itself. Each output value therefore describes the bar `optInRightBars` back, not the bar it is written at: a flag at output index `k` names input bar `outBegIdx + k - optInRightBars`, whose price is `inHigh[...]` / `inLow[...]` at that index. The display shift both outputs report, `-optInRightBars`, is that offset: a chart draws each flag that many bars to the left.

The two outputs are independent flags rather than one signed value, because an outside bar can be a swing high and a swing low at once.

## Chart

<ta-chart src="/functions/fractal.svg" ratio="720:274" label="FRACTAL over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outSwingHigh as a diamond above the high of every bar it flags moved 2 bars back and outSwingLow as a diamond above the high of every bar it flags moved 2 bars back."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/fractal.csv) · [Chart (SVG)](/functions/fractal.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outSwingHigh`: `display_shift` · `pattern_bool` · `zero` · `positive`
* `outSwingLow`: `display_shift` · `pattern_bool` · `zero` · `positive`

:::

## Formula

With `L = optInLeftBars`, `R = optInRightBars` and pivot `c = i - R`:

swingHigh(i) = 100 if High\[c] > High\[j] for every j in \[c-L, c+R] other than c, else 0.

swingLow(i) = 100 if Low\[c] < Low\[j] for every j in \[c-L, c+R] other than c, else 0.

## Notes

* Strict on both sides: a bar tied with any other bar of its window is not a pivot. TradingView's Pine runtime differs: its `ta.pivothigh` / `ta.pivotlow` let a tie with an older bar stand and let a tie with a newer bar cancel, i.e. non-strict left and strict right — so a plateau Pine reports as a pivot is not one here.
* Each output is decided on its own side: a high tied with any other high in the window forces `outSwingHigh` to 0 while leaving `outSwingLow` free to fire 100, and the mirror holds. Only a window flat in both series emits 0 on both.

## Inputs

* `inHigh` — High price series
* `inLow` — Low price series

## Outputs

* `outSwingHigh` — 100 when the bar `optInRightBars` back is a strict swing high, 0 otherwise
* `outSwingLow` — 100 when the bar `optInRightBars` back is a strict swing low, 0 otherwise

## Output Values

The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

### `outSwingHigh`

| Value | Meaning |
|-------|---------|
| 0 | The bar `optInRightBars` back is not a swing high |
| 100 | The bar `optInRightBars` back is a strict swing high |

### `outSwingLow`

| Value | Meaning |
|-------|---------|
| 0 | The bar `optInRightBars` back is not a swing low |
| 100 | The bar `optInRightBars` back is a strict swing low |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInLeftBars` | integer | 2 | 1–100000 | Bars before the pivot that it must strictly dominate |
| `optInRightBars` | integer | 2 | 1–100000 | Bars after the pivot that it must strictly dominate, and the delay before the verdict is reported |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Display Shift** <span class="flag-tip" tabindex="0" role="note" aria-label="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted." data-tip="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted.">i</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

</div>

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
int swingHigh[252], swingLow[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_FRACTAL( 0, n - 1, high, low, 2, 2, &outBegIdx, &outNBElement, swingHigh, swingLow );
printf( "%d values from bar %d; last swingHigh = %d\n", outNBElement, outBegIdx, swingHigh[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut swing_high = vec![0i32; 252];
let mut swing_low = vec![0i32; 252];

let core = Core::new();
let range = core.fractal(0, high.len() - 1, &high, &low, 2, 2,
                         &mut swing_high, &mut swing_low)?;
println!("{} values from bar {}; last swing_high = {}", range.count, range.beg_idx, swing_high[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
int[] swingHigh = new int[252];
int[] swingLow = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.fractal(0, high.length - 1, high, low, 2, 2, swingHigh, swingLow);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last swingHigh = " + swingHigh[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var swingHigh = new int[252];
var swingLow = new int[252];

var core = Core.Default;
OutRange r = core.Fractal(0, high.Length - 1, high, low, 2, 2, swingHigh, swingLow);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last swingHigh = {swingHigh[r.Count - 1]}");
```

@tab TypeScript

```ts
import { fractal } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, outSwingHigh, outSwingLow } = fractal({ high, low }, {
  leftBars: 2,
  rightBars: 2,
});
console.log(outSwingHigh.length + ' values from bar ' + begIdx + '; last outSwingHigh = ' + outSwingHigh[outSwingHigh.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`fractal.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fractal/fractal.c) · [`fractal.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fractal/fractal.yaml)

| Native | File |
|--------|------|
| C | [`ta_FRACTAL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FRACTAL.c) |
| Rust | [`fractal.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/fractal.rs) |
| Java | [`Core_FRACTAL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FRACTAL.java) |
| C# | [`Core_FRACTAL.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_FRACTAL.cs) |
| TypeScript | [`fractal.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/fractal.ts) |

Python: `talib.FRACTAL` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Williams Fractal, Fractals, Swing High, Swing Low, Pivot High, Pivot Low

## See Also

[AROON](/functions/aroon.md) · [MAXINDEX](/functions/maxindex.md) · [MININDEX](/functions/minindex.md) · [MINMAXINDEX](/functions/minmaxindex.md)

## References

* Bill Williams, *Trading Chaos*, John Wiley & Sons (1995)
* [TradingView: Williams Fractal](https://www.tradingview.com/support/solutions/43000591663-williams-fractal/)
