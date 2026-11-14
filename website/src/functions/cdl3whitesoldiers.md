---
title: "Three Advancing White Soldiers (CDL3WHITESOLDIERS)"
description: "A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short…"
---

## Summary

A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short upper shadow. A hit is a bullish reversal signal, most meaningful in a downtrend, which the code does not verify.

## Chart

<ta-chart src="/functions/cdl3whitesoldiers.svg" ratio="720:274" label="CDL3WHITESOLDIERS over WBTC/USDC (Uniswap v3), daily, 2021" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow up from the bottom of the pane to below the low for a bullish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdl3whitesoldiers.csv) · [Chart (SVG)](/functions/cdl3whitesoldiers.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `pattern_bull_bear` · `zero` · `positive`

:::

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.
- Bulkowski's testing found this reverses a downtrend 82% of the time, but cautions the high rate mostly reflects how rare downward breakouts are afterward — moves following an upward breakout perform poorly. ([thepatternsite.com](https://thepatternsite.com/ThreeWhiteSoldiers.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise; never negative (three white soldiers is always bullish)

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Three White Soldiers detected — a bullish reversal signal, most meaningful after a downtrend (unverified by the function) |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="A candlestick pattern; its values are listed under Output Values." data-tip="A candlestick pattern; its values are listed under Output Values.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

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
double open[252]; /* your data, oldest first */
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
int out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CDL3WHITESOLDIERS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d: [%d, ..., %d]\n", outNBElement, outBegIdx, out[0], out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0i32; 252];

let core = Core::new();
let range = core.cdl3whitesoldiers(0, open.len() - 1, &open, &high, &low, &close,
                                   &mut out)?;
println!("{} values from bar {}: [{}, ..., {}]", range.count, range.beg_idx, out[0], out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;
import java.util.Locale;

double[] open = new double[252]; // your data, oldest first
double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
int[] out = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.cdl3whitesoldiers(0, open.length - 1, open, high, low, close, out);
System.out.printf(Locale.ROOT, "%d values from bar %d: [%d, ..., %d]%n", r.count(), r.begIdx(), out[0], out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.Cdl3whitesoldiers(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine(FormattableString.Invariant($"{r.Count} values from bar {r.BegIdx}: [{outInteger[0]}, ..., {outInteger[r.Count - 1]}]"));
```

@tab TypeScript

```ts
import { cdl3whitesoldiers } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdl3whitesoldiers({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + ': [' + values[0] + ', ..., ' + values[values.length - 1] + ']');
```

@tab Go

```go
import (
	"fmt"
	"log"

	talib "ta-lib.github.io/go"
)

open := make([]float64, 252) // your data, oldest first
high := make([]float64, 252) // your data, oldest first
low := make([]float64, 252) // your data, oldest first
close := make([]float64, 252) // your data, oldest first
out := make([]int32, 252)

core := talib.NewCore()
r, err := core.CDL3WHITESOLDIERS(0, len(open)-1, open, high, low, close, out)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("%d values from bar %d: [%d, ..., %d]\n", r.NBElement, r.BegIdx, out[0], out[r.NBElement-1])
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdl3whitesoldiers.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.c) · [`cdl3whitesoldiers.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3WHITESOLDIERS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3WHITESOLDIERS.c) |
| Rust | [`cdl3whitesoldiers.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3whitesoldiers.rs) |
| Java | [`Core_CDL3WHITESOLDIERS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3WHITESOLDIERS.java) |
| C# | [`Core_CDL3WHITESOLDIERS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDL3WHITESOLDIERS.cs) |
| TypeScript | [`cdl3whitesoldiers.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdl3whitesoldiers.ts) |
| Go | [`cdl3whitesoldiers.go`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/go/cdl3whitesoldiers.go) |

Python: `talib.CDL3WHITESOLDIERS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Advancing White Soldiers, Three White Soldiers

## See Also

[CDL3BLACKCROWS](/functions/cdl3blackcrows.md) · [CDLADVANCEBLOCK](/functions/cdladvanceblock.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md)
