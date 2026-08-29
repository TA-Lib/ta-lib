---
title: Java Core API
description: "io.github.talib: a native Java port with no JNI, indicators as methods on a Core instance over double arrays, bit-identical to the reference C library."
toc: false
---

::: warning Not yet released
The Java API is not yet released. Estimated release: **Q1 2027**.
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
<a href="#multithreading">4.5 Threading</a></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The Java library is a native port of TA-Lib in the `io.github.talib` package — no JNI, pure Java. Every indicator is a method on a `Core` instance, operates on `double[]` arrays (or `float[]`, see [4.4](#input_type)), and is **bit-identical** to the reference C library over the same inputs.

The **Core API** provides:

- The [`Core`](#direct_call) type and the builder that configures it.
- The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Multiple `Core` instances can safely co-exist (say for different settings).
- Every TA function, each processing a whole array of data at once.
- An optional [abstraction layer](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead, see the companion [Java Streaming API](/api/java/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, Java has `Core.DEFAULT` (or a configured `Core.builder()...build()`) ready immediately, and an unreferenced `Core` is simply garbage-collected.

## 2.0 Add it to your project {#build}

```xml
<dependency>
    <groupId>io.github.ta-lib</groupId>
    <artifactId>ta-lib</artifactId>
    <version>0.8.1</version>
</dependency>
```

## 3.0 Calling into TA-Lib {#ta_func}

Every indicator is exposed as a method on `Core`, taking the same startIdx/endIdx/inputs/optional-parameters/outputs shape as the C function it mirrors.

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from arrays you pass in and writes its results into arrays you allocate.

A function never writes more elements than you request, so the output array only needs to cover the `startIdx`-to-`endIdx` range.

As an example, let's walk through `SMA`, a method to calculate a moving average.

<pre>public OutRange SMA( <span class="ta-arg-range">int      startIdx,</span>
                     <span class="ta-arg-range">int      endIdx,</span>
                     <span class="ta-arg-in">double[] inReal,</span>
                     <span class="ta-arg-opt">int      optInTimePeriod,</span>
                     <span class="ta-arg-out">double[] outReal</span> )
</pre>

All TA methods use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input arrays.</span></li>
<li><span class="ta-arg-in">One or more input arrays are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there is one optional input. These parameters give finer control specific to each function. Passing <code>Integer.MIN_VALUE</code> for an integer parameter, the real-default sentinel <code>-4e37</code> for a <code>double</code> parameter, or <code>MAType.DEFAULT</code> for an MA-type parameter selects that parameter's documented default.</span></li>
<li><span class="ta-arg-out">One or more output arrays come last. In this example there is only one output (outReal). Where the values landed is the return value, not a parameter: on success you get an <code>OutRange</code>.</span></li>
</ul>

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>import io.github.talib.Core;
import io.github.talib.OutRange;

double[] close = /* ...your closing prices... */;
double[] out   = new double[close.length];

OutRange r = Core.DEFAULT.SMA(
    <span class="ta-arg-range">0</span>, <span class="ta-arg-range">close.length - 1</span>,
    <span class="ta-arg-in">close</span>,
    <span class="ta-arg-opt">30</span>,
    <span class="ta-arg-out">out</span> );

// out[0 .. r.count() - 1] holds the SMA; out[i] is input bar r.begIdx() + i.
for (int i = 0; i &lt; r.count(); i++) {
    System.out.println("bar " + (r.begIdx() + i) + " = " + out[i]);
}
</pre>

After the call, read `r` to learn what was produced. Even though we requested the whole range (`0` to `close.length - 1`), a 30-day average is not defined until the 30th day. Consequently `r.begIdx()` will be 29 (zero-based) and `r.count()` will be `close.length - 29`. In other words, only that many elements of `out` are written, corresponding to input elements 29 through the end.

If you do not provide enough data to calculate even one value, the call still succeeds and `r.count()` is 0 — `r.isEmpty()` says so directly.

`OutRange` is an immutable record with two components — `begIdx()` and `count()` — plus the conveniences `isEmpty()` and `EMPTY`. The component names match the C, Rust and C# surfaces (`outBegIdx` / `outNBElement`), so the same concept reads the same way in every backend.

Every indicator is overloaded for `float[]` inputs as well as `double[]` — see [4.4](#input_type).

### 3.2 Output Size and Lookback {#output_size}

An output is written only where the indicator is defined — a 30-period SMA has no value until the 30th bar. `begIdx()` is the first valid bar and `count()` is the number written; the rest of the array is left untouched, never padded with NaN. Size the output array to at least `endIdx - startIdx + 1`, or exactly with the lookback:

```java
int lookback = Core.DEFAULT.SMA_Lookback(30);    // 29 for a 30-period SMA
```

Each TA method has a matching `<NAME>_Lookback` method, taking the same optional parameters as the method itself. The lookback is how many inputs are consumed before the first output.

**Too little data is a success, not an error.** A valid range shorter than the lookback simply produces no values: `count()` is 0 and `isEmpty()` is true. No exception is thrown — this matches the C library's `TA_SUCCESS` with `outNBElement == 0`. Nothing is written, so the output array's length is not checked on such a call — it may even be zero-length. The input is still checked, though: an `endIdx` past the end of the series you passed is a mistake worth hearing about in any range, and an empty range would otherwise hide it behind a "no data yet" result.

### 3.3 Errors {#retcode}

Misuse throws rather than returning a return code:

| Mistake | Exception |
|---|---|
| `startIdx`/`endIdx` out of range, or `endIdx < startIdx` | `IndexOutOfBoundsException` |
| Optional parameter outside its documented range | `IllegalArgumentException` |
| Two outputs sharing one array | `IllegalArgumentException` |
| An array too short for the range requested | `IllegalArgumentException` |
| A null input or output array | `IllegalArgumentException` |

Array lengths are checked before anything is written, so a rejected call leaves every buffer untouched. This is the batch API; in the [streaming API](/api/java/stream/) only `updateAndFill` checks capacity — an undersized `OpenAndFill` output faults inside the fill instead. An input must reach `endIdx`; an output must hold the values actually produced, `endIdx - max(startIdx, lookback) + 1`. The message names the array and both sizes — `SMA: outReal has length 3, needs 191`.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstraction Layer {#abstract}

The `io.github.talib.metadata` package describes every function at run time and calls it without naming it at compile time — the Java equivalent of C's [abstraction layer](/api/#abstract). Useful for a UI, a scripting bridge, or anything that enumerates indicators.

```java
import io.github.talib.metadata.FunctionInfo;
import io.github.talib.metadata.Functions;

FunctionInfo f = Functions.byName("SMA");  // or "sma", or "Sma" -- matched
                                           // under an ASCII case fold

f.name();       // "SMA" -- always the canonical spelling
f.group();      // "Overlap Studies"
f.hint();       // one-line description
f.inputs();     // List<InputInfo>    -- one entry per input
f.optInputs();  // List<OptInputInfo> -- one entry per optional parameter
f.outputs();    // List<OutputInfo>   -- one entry per output

Functions.all().forEach(fi -> System.out.println(fi.name() + " (" + fi.group() + ")"));
```

Binding arguments at run time goes through a `ParamHolder`, obtained from `FunctionInfo#newCall()`:

```java
FunctionInfo f = Functions.byName("SMA");
OutRange r = f.newCall()
    .setInput(0, close)
    .setOptInput(0, 30)
    .setOutput(0, out)
    .call(0, close.length - 1);
```

Everything is validated against the `FunctionInfo` row: an index out of bounds, a type that does not match the declared parameter, or an unset parameter at `call()` time throws `IllegalArgumentException`. The call itself then behaves exactly like the typed method, including throwing on misuse and returning an empty `OutRange` when the range is shorter than the lookback. A `ParamHolder` is not thread-safe: confine one to one thread, or build one per call.

Streamable functions carry the `FuncFlags.STREAMING` bit in `FunctionInfo#flags()` — check it with `f.hasFlags(FuncFlags.STREAMING)`.

### 4.2 Numerical Stability {#numerical_stability}

Some indicators are recursive, so their earliest values depend on how much history precedes them. The [unstable period](/api/unstable-period/) setting controls how many of those warm-up bars are discarded. It lives on `Core` and is set through the builder:

```java
import io.github.talib.Core;
import io.github.talib.FuncUnstId;

Core core = Core.builder()
    .unstablePeriod(FuncUnstId.EMA, 10)
    .build();
```

Each setter throws immediately (`IllegalArgumentException`) if the period is out of range — unlike Rust and C#, a Java builder has no `build()`-time rejection to defer to.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern methods judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults; the builder sets them the same way:

```java
import io.github.talib.CandleSettingType;
import io.github.talib.Core;
import io.github.talib.RangeType;

Core core = Core.builder()
    .candleSetting(CandleSettingType.BodyLong, RangeType.RealBody, 10, 1.0)
    .build();
```

### 4.4 Input Type: float vs. double {#input_type}

Every indicator is overloaded for `float[]` inputs as well as `double[]` — the `float` overload widens each element to `double` before computing, so a result beyond `float` range still lands correctly in the `double` output. Both overloads produce the same output, bit-for-bit. Use it to feed price data already stored as `float` without copying.

Because the two overloads differ only in the input array type, a bare `null` argument is ambiguous; cast it (`(double[]) null`) if you ever need to pass one.

### 4.5 Threading {#multithreading}

**`Core` is immutable.** Every field is final and the settings it carries are deeply immutable, so one instance is safe to share across any number of threads with no synchronization — even when published racily (JLS 17.5 final-field semantics). There are no locks on any call path.

Use `Core.DEFAULT` for the all-defaults instance. There are no setters: to change a setting, derive a new instance with `core.toBuilder()`. Read a configured unstable period back with `core.unstablePeriod(FuncUnstId.EMA)` — the same name the builder writes it under, since a `Core` is immutable and has no writer to distinguish it from.

## 5.0 Documentation {#docs}

Every function's Javadoc is rendered from the same canonical description as every other backend's docs. Browse it with `mvn javadoc:javadoc`, or, once published, on javadoc.io.
