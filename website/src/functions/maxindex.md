---
title: "Index of highest value over a specified period (MAXINDEX)"
description: "Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value."
---

## Summary

Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value.

## Chart

<ta-chart src="/functions/maxindex.svg" ratio="720:416" label="MAXINDEX over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with close at the bar outInteger names as a solid line; below it, a step pane of its own for the integer output outInteger."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/maxindex.csv) · [Chart (SVG)](/functions/maxindex.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `bar_index`

:::

## Formula

outInteger[i] = index of max(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the highest value, the index of the most recent of them is returned.

## Inputs

- `inReal` — Input series to scan

## Outputs

- `outInteger` — Absolute index (into inReal) of the highest value in each window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length over which the max is located |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
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
double series[252]; /* your data, oldest first */
int out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MAXINDEX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d: [%d, ..., %d]\n", outNBElement, outBegIdx, out[0], out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0i32; 252];

let core = Core::new();
let range = core.maxindex(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}: [{}, ..., {}]", range.count, range.beg_idx, out[0], out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;
import java.util.Locale;

double[] series = new double[252]; // your data, oldest first
int[] out = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.maxindex(0, series.length - 1, series, 30, out);
System.out.printf(Locale.ROOT, "%d values from bar %d: [%d, ..., %d]%n", r.count(), r.begIdx(), out[0], out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.Maxindex(0, series.Length - 1, series, 30, outInteger);
Console.WriteLine(FormattableString.Invariant($"{r.Count} values from bar {r.BegIdx}: [{outInteger[0]}, ..., {outInteger[r.Count - 1]}]"));
```

@tab TypeScript

```ts
import { maxindex } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = maxindex(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + ': [' + values[0] + ', ..., ' + values[values.length - 1] + ']');
```

@tab Go

```go
import (
	"fmt"
	"log"

	talib "ta-lib.github.io/go"
)

series := make([]float64, 252) // your data, oldest first
out := make([]int32, 252)

core := talib.NewCore()
r, err := core.MAXINDEX(0, len(series)-1, series, 30, out)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("%d values from bar %d: [%d, ..., %d]\n", r.NBElement, r.BegIdx, out[0], out[r.NBElement-1])
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`maxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.c) · [`maxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAXINDEX.c) |
| Rust | [`maxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/maxindex.rs) |
| Java | [`Core_MAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAXINDEX.java) |
| C# | [`Core_MAXINDEX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MAXINDEX.cs) |
| TypeScript | [`maxindex.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/maxindex.ts) |
| Go | [`maxindex.go`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/go/maxindex.go) |

Python: `talib.MAXINDEX`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Index of Highest Value, Highest Value Index, argmax

## See Also

[MAX](/functions/max.md) · [MININDEX](/functions/minindex.md) · [MIN](/functions/min.md) · [MINMAXINDEX](/functions/minmaxindex.md)
