---
url: 'https://ta-lib.org/api/csharp/index.md'
description: >-
  TALib: a native C# port with no P/Invoke, indicators as methods on a Core
  instance taking spans, bit-identical to the reference C library apart from
  last-bit rounding in the math library.
---
# C# Core API

<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 Add it to your project</a></p>

<p><a href="#ta_func">3.0 Calling into TA-Lib</a></p>

<blockquote>
<p><a href="#direct_call">3.1 Batch Processing</a><br>
<a href="#output_size">3.2 Output Size and Lookback</a><br>
<a href="#display_shift">3.3 Display Shift</a><br>
<a href="#retcode">3.4 Errors</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced Features</a></p>

<blockquote>
<p><a href="#abstract">4.1 Abstract API</a><br>
<a href="#numerical_stability">4.2 Numerical Stability</a><br>
<a href="#candle_settings">4.3 Candlestick Settings</a><br>
<a href="#input_type">4.4 Input Type: float vs. double</a><br>
<a href="#index_range">4.5 Index Range</a><br>
<a href="#multithreading">4.6 Threading</a><br>
<a href="#aot">4.7 Trimming and NativeAOT</a></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The .NET library is a native port of TA-Lib in the `TALib` namespace — no P/Invoke, no native dependency, pure managed C# targeting `net10.0`. Every indicator is a method on a `Core` instance, takes its series as spans, and is **bit-identical** to the reference C library over the same inputs, with one exception beyond TA-Lib's control: where a calculation calls a transcendental math function (such as `exp`, `log`, `sin` or `atan`), .NET's math library can round the result differently from the C library in the last bit ([rD1](/spec/versions/#rd1)).

The **Core API** provides:

* The [`Core`](#direct_call) type and the builder that configures it.
* The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Multiple `Core` instances can safely co-exist (say for different settings).
* Every TA function, each processing a whole array of data at once.
* An optional [Abstract API](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead, see the companion [C# Streaming API](/api/csharp/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, C# has `Core.Default` (or a configured `Core.Builder()...Build()`) ready immediately; a `Core` owns only managed state, so an unreferenced one is simply garbage-collected.

## 2.0 Add it to your project {#build}

```xml
<ItemGroup>
  <PackageReference Include="TA-Lib" Version="0.8.3" />
</ItemGroup>
```

Or run `dotnet add package TA-Lib`. It targets `net10.0`, so it needs the .NET 10 SDK or later.

Every released version is listed on [NuGet](https://www.nuget.org/packages/TA-Lib).

## 3.0 Calling into TA-Lib {#ta_func}

Every indicator is exposed as a method on `Core`, taking the same startIdx/endIdx/inputs/optional-parameters/outputs shape as the C function it mirrors.

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from spans you pass in and writes its results into spans you allocate.

A function never writes more elements than you request, so the output span only needs to cover the `startIdx`-to-`endIdx` range.

As an example, let's walk through `SMA`, a method to calculate a moving average.

<pre>public OutRange Sma( <span class="ta-arg-range">int    startIdx,</span>
                     <span class="ta-arg-range">int    endIdx,</span>
                     <span class="ta-arg-in">ReadOnlySpan&lt;double&gt; inReal,</span>
                     <span class="ta-arg-opt">int    optInTimePeriod,</span>
                     <span class="ta-arg-out">Span&lt;double&gt; outReal</span> )
</pre>

All TA methods use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input spans.</span></li>
<li><span class="ta-arg-in">One or more input spans are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there is one optional input. These parameters give finer control specific to each function. If you do not care about a particular optIn, just pass <code>Core.IntegerDefault</code> or <code>Core.RealDefault</code> (depending on the type). For a moving-average type, use <code>MAType.DEFAULT</code>.</span></li>
<li><span class="ta-arg-out">One or more output spans come last. In this example there is only one output (outReal). Where the values landed is the return value, not a parameter: on success you get an <code>OutRange</code>.</span></li>
</ul>

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>using TALib;

var core = Core.Default;

double[] close = [ /* ...your closing prices... */ ];
var outReal = new double[close.Length];

OutRange r = core.Sma(
    <span class="ta-arg-range">0</span>, <span class="ta-arg-range">close.Length - 1</span>,
    <span class="ta-arg-in">close</span>,
    <span class="ta-arg-opt">30</span>,
    <span class="ta-arg-out">outReal</span> );

// outReal[0 .. r.Count - 1] holds the SMA; outReal[i] is input bar r.BegIdx + i.
for (int i = 0; i &lt; r.Count; i++)
{
    Console.WriteLine($"bar {r.BegIdx + i} = {outReal[i]}");
}
</pre>

After the call, read `r` to learn what was produced. Even though we requested the whole range (`0` to `close.Length - 1`), a 30-day average is not defined until the 30th day. Consequently `r.BegIdx` will be 29 (zero-based) and `r.Count` will be `close.Length - 29`. In other words, only that many elements of `outReal` are written, corresponding to input elements 29 through the end.

Arrays convert to spans implicitly, so you can also pass a slice of a larger buffer (`close.AsSpan(start, count)`) without copying it. `startIdx` and `endIdx` then index the slice, not the array it came from: a slice holds no bars before its first element ([rP1](/spec/inputs-outputs/#rp1)).

If you do not provide enough data to calculate even one value, the call still succeeds and `r.Count` is 0 (`r.IsEmpty`).

`OutRange` is a `readonly record struct` with two components, `BegIdx` and `Count` (C's `outBegIdx` and `outNBElement`), plus the conveniences `IsEmpty` and `Empty`.

### 3.2 Output Size and Lookback {#output_size}

Size the output span to at least `endIdx - startIdx + 1`, or exactly with the lookback (see the [three sizing methods](/api/#output_size)):

```csharp
int lookback = core.SmaLookback(30);   // 29 for a 30-period SMA
```

Each TA method has a matching `<Name>Lookback` method, taking the same optional parameters as the method itself. The lookback is how many inputs are consumed before the first output.

### 3.3 Display Shift {#display_shift}

A few indicators are conventionally drawn at another bar than the one that computes them. The values are never moved: the matching `<Name>DisplayShift` method gives the number of bars to shift one output when charting it, positive ahead and negative behind. It is 0 for almost every function.

```csharp
int shift = core.DpoDisplayShift(20, 0);   // period 20, first output: -11
```

The same answer comes from the [Abstract API](/api/abstract/). The rules are in the [specification](/spec/lookback/#display-shift).

### 3.4 Errors {#retcode}

The public methods throw rather than return a status code:

| Condition | Exception |
|---|---|
| `startIdx`/`endIdx` negative, above `Core.IndexMax`, or `endIdx < startIdx` | `TALibArgumentOutOfRangeException` |
| An optional parameter outside its documented range | `TALibArgumentException` |
| An input span that does not reach `endIdx`, or an output span shorter than the values produced | `TALibArgumentException` naming the span |
| Two outputs that are the same buffer ([rB6](/spec/errors/#rb6)) | `TALibArgumentException` |

Each extends the framework type you would reach for, so `catch (ArgumentException)` still works, and each carries its `RetCode`.

You can pass the same buffer as an input and an output to compute in place. Avoid a partial overlap: its result is [unspecified](/spec/inputs-outputs/#no-overlap).

A `NaN` or `±Inf` inside an input series is not detected. Clean the series before calling.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstract API {#abstract}

`TALib.Metadata.FunctionCatalog` describes every function at run time and calls it without naming it at compile time. See the [Abstract API](/api/abstract/) page.

`Core.Functions` is an `IReadOnlyList<FuncInfo>`, so it is enumerable, LINQ-able, and indexable by position or by name.

### 4.2 Numerical Stability {#numerical_stability}

Your value changed when you fed the same bar more history? That is by design: recursive functions converge as history accumulates. See [Unstable Period](/api/unstable-period/) for how to mitigate that.

Rounding is a separate axis: floating-point error accumulates over a very long series, which is one reason a call is capped at [`Core.IndexMax`](#index_range).

Every function documentation page carries a [numerical-stability property](/functions/stability): how much the value at a given bar depends on where the series you passed in begins.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern methods judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults; the builder sets them the same way:

```csharp
var core = Core.Builder()
    .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 0.1)
    .Build();
```

Each setter throws `ArgumentOutOfRangeException` at once if an argument is out of range, so `Build()` cannot fail.

### 4.4 Input Type: float vs. double {#input_type}

Every indicator also has a `ReadOnlySpan<float>` overload (`float[]` converts implicitly). The overload converts each element to `double` when read, so both overloads produce the same output, bit-for-bit. Use it to feed price data already stored as `float` without a conversion copy.

### 4.5 Index Range {#index_range}

`Core.IndexMax` is the largest value `startIdx` or `endIdx` may take: **100,000,000**. It's a sanity bound. Past it, a call is more likely a caller bug than a real need, and it's also untested territory for overflow and rounding error.

### 4.6 Threading {#multithreading}

A `Core` is immutable once built, so one instance is safe to share across any number of threads with no synchronization. `Core.Default` is the all-defaults instance. To change a setting, build another `Core` with `Core.Builder()`.

### 4.7 Trimming and NativeAOT {#aot}

The library is annotated `IsAotCompatible`, uses no reflection, and publishes clean under `PublishAot` with `TrimMode=full`.

At ILC's default instruction-set baseline, some indicators (EMA, DEMA, TRIX among them) run slower than under the JIT; values are unaffected. If you publish AOT and care about throughput, raise the baseline:

```xml
<IlcInstructionSet>x86-64-v3</IlcInstructionSet>
```

## 5.0 Documentation {#docs}

Every function ships XML doc comments, so your IDE's tooltips and IntelliSense are populated without a separate doc build.
