---
title: .NET Core API
description: "TALib for .NET: a native C# port with no P/Invoke, indicators as methods on a Core instance taking spans, bit-identical to the reference C library."
toc: false
---

::: warning Not yet released
The .NET API is not yet released. Estimated release: **Q1 2027**.
:::

The .NET library is a native port of TA-Lib in the `TALib` namespace — no P/Invoke, no native dependency, pure managed C# targeting `net10.0`. Every indicator is a method on a `Core` instance, takes its series as spans, and is **bit-identical** to the reference C library over the same inputs.

To process a live feed one bar at a time instead of a whole array, see the companion [.NET Streaming API](/api/csharp/stream/).

## Calling a function

Each indicator takes a `startIdx`/`endIdx` range, the inputs, the optional parameters, and the caller-provided output buffer(s). It returns an `OutRange` describing where the valid output begins and how many values were written:

```csharp
using TALib;

var core = new Core();

double[] close = /* ...your closing prices... */;
var outReal = new double[close.Length];

OutRange r = core.SMA(
    0, close.Length - 1,   // startIdx, endIdx
    close,                 // input(s)
    30,                    // optInTimePeriod
    outReal);              // output(s)

// outReal[0 .. r.Count - 1] holds the SMA; outReal[i] is input bar r.BegIdx + i.
for (int i = 0; i < r.Count; i++)
{
    Console.WriteLine($"bar {r.BegIdx + i} = {outReal[i]}");
}
```

Inputs are `ReadOnlySpan<double>` and outputs `Span<double>`, so you can hand an indicator a window of a larger buffer without copying it:

```csharp
core.SMA(0, count - 1, close.AsSpan(start, count), 30, outReal);
```

Arrays still work everywhere — `double[]` converts to a span implicitly, so the call above and the one before it are both ordinary code. There is a `float[]`/`ReadOnlySpan<float>` overload of every indicator too, for callers who store series at single precision; the arithmetic is `double` either way.

Two consequences worth knowing. A span is never null, so passing `null` arrives as an empty span and is rejected by the length check as one (`ArgumentException` naming the parameter) — except for the few candlestick patterns that declare an OHLC series they never read, which are not checked at all. And a span cannot be boxed, so the API cannot be invoked through `MethodInfo.Invoke` — to call indicators chosen at run time, use the catalogue below, which is the supported path and is faster besides.

`OutRange` is a readonly struct with two components — `BegIdx` and `Count` — plus the conveniences `IsEmpty` and `Empty`. The component names match the C, Rust and Java surfaces (`outBegIdx` / `outNBElement`), so the same concept reads the same way in every backend.

## Output size and lookback

An indicator consumes a number of leading bars — its **lookback** — before it can produce anything. Query it with the matching `*_Lookback` method:

```csharp
int lookback = core.SMA_Lookback(30);   // 29
```

Output is written only where the indicator is defined: `outReal[0]` corresponds to input bar `r.BegIdx`, and nothing outside `0 .. r.Count - 1` is touched. The library never pads with `NaN`. A range shorter than the lookback is a **success with no values** (`r.Count == 0`), not an error.

## Parameter defaults

Integer parameters accept `int.MinValue`, and real parameters `-4e37`, to select the documented default:

```csharp
core.SMA(0, n - 1, close, int.MinValue, outReal);   // same as passing 30
```

## Errors

The public methods throw rather than return a status code:

| Condition | Exception |
|---|---|
| `startIdx`/`endIdx` negative, above `Core.MAX_INDEX`, or `endIdx < startIdx` | `ArgumentOutOfRangeException` |
| An optional parameter outside its documented range | `ArgumentException` |
| Two outputs overlapping, or an output *partially* overlapping an input | `ArgumentException` |

Computing wholly in place is allowed and stays supported — passing the same buffer as both an input and an output is how several indicators are meant to be used. What is rejected is *partial* overlap, which only spans can express: two views of the same memory at different offsets make a body write through what it is still reading, and the result would be silently wrong rather than merely surprising.

## Settings

Unstable periods and candlestick thresholds are per-`Core`. A `Core` built with the defaults needs no configuration; to change them, build one:

```csharp
var core = Core.Builder()
    .UnstablePeriod(FuncUnstId.RSI, 10)
    .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 0.1)
    .Build();
```

A `Core` is immutable once built, so it is safe to share read-only across threads and call any indicator concurrently.

## Discovering functions at run time

`Core.Functions` is a generated catalogue — a static table, not reflection, so it survives trimming and NativeAOT:

```csharp
foreach (var f in Core.Functions)
{
    Console.WriteLine($"{f.Name}  {f.Group}");
}
```

## Trimming and NativeAOT

The library is annotated `IsAotCompatible`, uses no reflection, and publishes clean under `PublishAot` with `TrimMode=full`.

One publishing note worth knowing: at ILC's **default** instruction-set baseline, `Math.FusedMultiplyAdd` is compiled to a library call rather than the hardware FMA instruction. Values are unaffected — output is bit-identical across the JIT and both AOT baselines — but the indicators that lean on it are measurably slower (TRIX ~3.7x, DEMA ~2.3x, EMA ~1.5x, with SMA flat as a control). If you publish AOT and care about throughput, raise the baseline:

```xml
<IlcInstructionSet>x86-64-v3</IlcInstructionSet>
```
