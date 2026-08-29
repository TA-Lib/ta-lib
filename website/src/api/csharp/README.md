---
title: C# Core API
description: "TALib: a native C# port with no P/Invoke, indicators as methods on a Core instance taking spans, bit-identical to the reference C library."
toc: false
---

::: warning Not yet released
The C# API is not yet released. Estimated release: **Q1 2027**.
:::

<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 Add it to your project</a></p>

<p><a href="#ta_func">3.0 Calling into TA-Lib</a></p>

<blockquote>
<p><a href="#direct_call">3.1 Batch Processing</a><br>
<a href="#output_size">3.2 Output Size and Lookback</a><br>
<a href="#retcode">3.3 Errors</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced Features</a></p>

<blockquote>
<p><a href="#abstract">4.1 Abstraction Layer</a><br>
<a href="#numerical_stability">4.2 Numerical Stability</a><br>
<a href="#candle_settings">4.3 Candlestick Settings</a><br>
<a href="#input_type">4.4 Input Type: float vs. double</a><br>
<a href="#multithreading">4.5 Threading</a><br>
<a href="#aot">4.6 Trimming and NativeAOT</a></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The .NET library is a native port of TA-Lib in the `TALib` namespace — no P/Invoke, no native dependency, pure managed C# targeting `net10.0`. Every indicator is a method on a `Core` instance, takes its series as spans, and is **bit-identical** to the reference C library over the same inputs.

The **Core API** provides:

- The [`Core`](#direct_call) type and the builder that configures it.
- The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Multiple `Core` instances can safely co-exist (say for different settings).
- Every TA function, each processing a whole array of data at once.
- An optional [abstraction layer](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead, see the companion [C# Streaming API](/api/csharp/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, C# has `new Core()` (or a configured `Core.Builder()...Build()`) ready immediately; a `Core` owns only managed state, so an unreferenced one is simply garbage-collected.

## 2.0 Add it to your project {#build}

NuGet packaging (`PackageId`, versioning) lands with the release milestone — until then, `dotnet pack` deliberately produces nothing. Reference the `TALib` project directly:

```xml
<ItemGroup>
  <ProjectReference Include="path/to/ta_codegen/output/csharp/library/TALib.csproj" />
</ItemGroup>
```

It targets `net10.0`.

## 3.0 Calling into TA-Lib {#ta_func}

Every indicator is exposed as a method on `Core`, taking the same startIdx/endIdx/inputs/optional-parameters/outputs shape as the C function it mirrors.

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from spans you pass in and writes its results into spans you allocate.

A function never writes more elements than you request, so the output span only needs to cover the `startIdx`-to-`endIdx` range.

As an example, let's walk through `SMA`, a method to calculate a moving average.

<pre>public OutRange SMA( <span class="ta-arg-range">int    startIdx,</span>
                     <span class="ta-arg-range">int    endIdx,</span>
                     <span class="ta-arg-in">ReadOnlySpan&lt;double&gt; inReal,</span>
                     <span class="ta-arg-opt">int    optInTimePeriod,</span>
                     <span class="ta-arg-out">Span&lt;double&gt; outReal</span> )
</pre>

All TA methods use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input spans.</span></li>
<li><span class="ta-arg-in">One or more input spans are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there is one optional input. These parameters give finer control specific to each function. Passing <code>int.MinValue</code> for an integer parameter, the real-default sentinel <code>-4e37</code> for a <code>double</code> parameter, or <code>MAType.DEFAULT</code> for an MA-type parameter selects that parameter's documented default.</span></li>
<li><span class="ta-arg-out">One or more output spans come last. In this example there is only one output (outReal). Where the values landed is the return value, not a parameter: on success you get an <code>OutRange</code>.</span></li>
</ul>

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>using TALib;

var core = new Core();

double[] close = /* ...your closing prices... */;
var outReal = new double[close.Length];

OutRange r = core.SMA(
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

Arrays convert to spans implicitly, so the call above and a call passed a slice of a larger buffer (`close.AsSpan(start, count)`) are both ordinary code — no copy either way. A span is never null, so passing `null` arrives as an empty span and is rejected by the length check as one (`ArgumentException` naming the parameter). Every input an indicator declares is checked, including the OHLC series a few candlestick patterns never read.

If you do not provide enough data to calculate even one value, the call still succeeds and `r.Count` is 0 (`r.IsEmpty`).

`OutRange` is a readonly struct with two components — `BegIdx` and `Count` — plus the conveniences `IsEmpty` and `Empty`. The component names match the C, Rust and Java surfaces (`outBegIdx` / `outNBElement`), so the same concept reads the same way in every backend.

Every indicator also has a `ReadOnlySpan<float>` overload — see [4.4](#input_type).

### 3.2 Output Size and Lookback {#output_size}

An indicator consumes a number of leading bars — its **lookback** — before it can produce anything. Query it with the matching `*_Lookback` method:

```csharp
int lookback = core.SMA_Lookback(30);   // 29
```

Output is written only where the indicator is defined: `outReal[0]` corresponds to input bar `r.BegIdx`, and nothing outside `0 .. r.Count - 1` is touched. The library never pads with `NaN`. A range shorter than the lookback is a **success with no values** (`r.Count == 0`), not an error.

### 3.3 Errors {#retcode}

The public methods throw rather than return a status code:

| Condition | Exception |
|---|---|
| `startIdx`/`endIdx` negative, above `Core.MAX_INDEX`, or `endIdx < startIdx` | `ArgumentOutOfRangeException` |
| An optional parameter outside its documented range | `ArgumentException` |
| Two outputs overlapping, or an output *partially* overlapping an input | `ArgumentException` |

Computing wholly in place is allowed and stays supported — passing the same buffer as both an input and an output is how several indicators are meant to be used. What is rejected is *partial* overlap, which only spans can express: two views of the same memory at different offsets make a body write through what it is still reading, and the result would be silently wrong rather than merely surprising.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstraction Layer {#abstract}

`TALib.Metadata.FunctionCatalog` describes every function at run time and calls it without naming it at compile time — the C# equivalent of C's [abstraction layer](/api/#abstract). It exists because a span cannot be boxed: the API cannot be invoked through `MethodInfo.Invoke`, so calling a function chosen at run time needs a typed path instead of reflection — which is also faster.

```csharp
using TALib.Metadata;

foreach (var f in Core.Functions.Where(f => f.Flags.HasFlag(FunctionFlags.Candlestick)))
{
    Console.WriteLine($"{f.Name}: {f.Hint}");
}
```

`Core.Functions` (an alias for `FunctionCatalog.Default`) implements `IReadOnlyList<FunctionInfo>`, so it is directly enumerable and LINQ-able, and is indexable by position or by name (`Core.Functions["SMA"]`). The name is matched with `StringComparer.OrdinalIgnoreCase`, so `"SMA"`, `"sma"` and `"Sma"` all resolve to the same function; `FunctionInfo.Name` stays the canonical `"SMA"`. Streamable functions carry `FunctionFlags.Stream`.

Binding arguments at run time goes through a `FunctionCall`, obtained from `FunctionInfo.CreateCall()`:

```csharp
var f = Core.Functions["SMA"];
var range = f.CreateCall()
    .SetInput(0, close)
    .SetOption(0, 30)
    .SetOutput(0, outReal)
    .Invoke(0, close.Length - 1);
```

An index out of range, a type that does not match the declared parameter, or an unbound input or output at call time throws `ArgumentException`. Optional parameters left unbound take their documented defaults. A `FunctionCall` is not thread-safe: confine one to one thread, or build one per call. The `FunctionCatalog` it comes from is immutable and shared freely.

### 4.2 Numerical Stability {#numerical_stability}

Some indicators are recursive, so their earliest values depend on how much history precedes them. The [unstable period](/api/unstable-period/) setting controls how many of those warm-up bars are discarded. It lives on `Core` and is set through the builder:

```csharp
var core = Core.Builder()
    .UnstablePeriod(FuncUnstId.RSI, 10)
    .Build();
```

The setters chain, so they cannot report a rejection at the point it happens; the first one is latched and surfaced by `Build()`, which throws `ArgumentOutOfRangeException`.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern methods judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults; the builder sets them the same way:

```csharp
var core = Core.Builder()
    .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 0.1)
    .Build();
```

### 4.4 Input Type: float vs. double {#input_type}

Every indicator also has a `ReadOnlySpan<float>` overload (`float[]` converts implicitly), for callers who store series at single precision; the arithmetic is `double` either way, so both overloads produce the same output, bit-for-bit.

### 4.5 Threading {#multithreading}

A `Core` is immutable once built, so it is safe to share read-only across threads and call any indicator concurrently — no locking, and no setup ordering to respect. To change a setting, build another `Core` with `Core.Builder()`.

### 4.6 Trimming and NativeAOT {#aot}

The library is annotated `IsAotCompatible`, uses no reflection, and publishes clean under `PublishAot` with `TrimMode=full`.

One publishing note worth knowing: at ILC's **default** instruction-set baseline, `Math.FusedMultiplyAdd` is compiled to a library call rather than the hardware FMA instruction. Values are unaffected — output is bit-identical across the JIT and both AOT baselines — but the indicators that lean on it are measurably slower (TRIX ~3.7x, DEMA ~2.3x, EMA ~1.5x, with SMA flat as a control). If you publish AOT and care about throughput, raise the baseline:

```xml
<IlcInstructionSet>x86-64-v3</IlcInstructionSet>
```

## 5.0 Documentation {#docs}

Every function ships XML doc comments rendered from the same canonical description as every other backend's docs, so your IDE's tooltips and IntelliSense are populated without a separate doc build. `GenerateDocumentationFile` is on and `CS1591` (a public member missing its doc comment) is an error, so the assembly can never ship undocumented.
