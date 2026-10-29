---
url: 'https://ta-lib.org/api/java/index.md'
description: >-
  io.github.talib: a native Java port with no JNI, indicators as methods on a
  Core instance over double arrays, bit-identical to the reference C library
  apart from last-bit rounding in the math library.
---
# Java Core API

[![Maven Central](https://img.shields.io/maven-central/v/io.github.ta-lib/ta-lib)](https://central.sonatype.com/artifact/io.github.ta-lib/ta-lib)

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
<a href="#multithreading">4.6 Threading</a></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The Java library is a native port of TA-Lib in the `io.github.talib` package — no JNI, pure Java. Every indicator is a method on a `Core` instance, operates on `double[]` arrays (or `float[]`, see [4.4](#input_type)), and is **bit-identical** to the reference C library over the same inputs, with one exception beyond TA-Lib's control: where a calculation calls a transcendental math function (such as `exp`, `log`, `sin` or `atan`), Java's math library can round the result differently from the C library in the last bit ([rD1](/spec/versions/#rd1)).

The **Core API** provides:

* The [`Core`](#direct_call) type and the builder that configures it.
* The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Multiple `Core` instances can safely co-exist (say for different settings).
* Every TA function, each processing a whole array of data at once.
* An optional [Abstract API](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead, see the companion [Java Streaming API](/api/java/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, Java has `Core.DEFAULT` (or a configured `Core.builder()...build()`) ready immediately, and an unreferenced `Core` is simply garbage-collected.

## 2.0 Add it to your project {#build}

```xml
<dependency>
    <groupId>io.github.ta-lib</groupId>
    <artifactId>ta-lib</artifactId>
    <version>0.8.3</version>
</dependency>
```

Every released version is listed on [Maven Central](https://central.sonatype.com/artifact/io.github.ta-lib/ta-lib).

## 3.0 Calling into TA-Lib {#ta_func}

Every indicator is exposed as a method on `Core`, taking the same startIdx/endIdx/inputs/optional-parameters/outputs shape as the C function it mirrors.

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from arrays you pass in and writes its results into arrays you allocate.

A function never writes more elements than you request, so the output array only needs to cover the `startIdx`-to-`endIdx` range.

As an example, let's walk through `SMA`, a method to calculate a moving average.

<pre>public OutRange sma( <span class="ta-arg-range">int      startIdx,</span>
                     <span class="ta-arg-range">int      endIdx,</span>
                     <span class="ta-arg-in">double[] inReal,</span>
                     <span class="ta-arg-opt">int      optInTimePeriod,</span>
                     <span class="ta-arg-out">double[] outReal</span> )
</pre>

All TA methods use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input arrays.</span></li>
<li><span class="ta-arg-in">One or more input arrays are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there is one optional input. These parameters give finer control specific to each function. If you do not care about a particular optIn, just pass <code>Core.INTEGER_DEFAULT</code> or <code>Core.REAL_DEFAULT</code> (depending on the type). For a moving-average type, use <code>MAType.DEFAULT</code>.</span></li>
<li><span class="ta-arg-out">One or more output arrays come last. In this example there is only one output (outReal). Where the values landed is the return value, not a parameter: on success you get an <code>OutRange</code>.</span></li>
</ul>

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>import io.github.talib.Core;
import io.github.talib.OutRange;

double[] close = /* ...your closing prices... */;
double[] out   = new double[close.length];

OutRange r = Core.DEFAULT.sma(
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

If you do not provide enough data to calculate even one value, the call still succeeds and `r.count()` is 0 (`r.isEmpty()`).

`OutRange` is an immutable record with two components, `begIdx()` and `count()` (C's `outBegIdx` and `outNBElement`), plus the conveniences `isEmpty()` and `EMPTY`.

### 3.2 Output Size and Lookback {#output_size}

Size the output array to at least `endIdx - startIdx + 1`, or exactly with the lookback (see the [three sizing methods](/api/#output_size)):

```java
int lookback = Core.DEFAULT.smaLookback(30);    // 29 for a 30-period SMA
```

Each TA method has a matching `<name>Lookback` method, taking the same optional parameters as the method itself. The lookback is how many inputs are consumed before the first output.

### 3.3 Display Shift {#display_shift}

A few indicators are conventionally drawn at another bar than the one that computes them. The values are never moved: the matching `<name>DisplayShift` method gives the number of bars to shift one output when charting it, positive ahead and negative behind. It is 0 for almost every function.

```java
int shift = Core.DEFAULT.dpoDisplayShift(20, 0);   // period 20, first output: -11
```

The same answer comes from the [Abstract API](/api/abstract/). The rules are in the [specification](/spec/lookback/#display-shift).

### 3.4 Errors {#retcode}

Misuse throws rather than returning a return code:

| Mistake | Exception |
|---|---|
| `startIdx`/`endIdx` negative, above `Core.INDEX_MAX`, or `endIdx < startIdx` | `TALibIndexException` |
| Optional parameter outside its documented range | `TALibArgumentException` |
| Two outputs sharing one array | `TALibArgumentException` |
| An array too short for the range requested, including an `endIdx` past the end of the input | `TALibArgumentException` |
| A null array | `TALibArgumentException` |

Each extends the platform type you would reach for — `TALibIndexException` an
`IndexOutOfBoundsException`, the rest an `IllegalArgumentException` — so catching
either shape works, and every one carries its `RetCode`.

A rejected call leaves every array untouched.

A `NaN` or `±Inf` inside an input series is not detected. Clean the series before calling.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstract API {#abstract}

The `io.github.talib.metadata` package describes every function at run time and calls it without naming it at compile time. Useful for a UI, a scripting bridge, or anything that enumerates indicators. See the [Abstract API](/api/abstract/) page.

### 4.2 Numerical Stability {#numerical_stability}

Your value changed when you fed the same bar more history? That is by design: recursive functions converge as history accumulates. See [Unstable Period](/api/unstable-period/) for how to mitigate that.

Rounding is a separate axis: floating-point error accumulates over a very long series, which is one reason a call is capped at [`Core.INDEX_MAX`](#index_range).

Every function documentation page carries a [numerical-stability property](/functions/stability): how much the value at a given bar depends on where the series you passed in begins.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern methods judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults; the builder sets them the same way:

```java
import io.github.talib.CandleSettingType;
import io.github.talib.Core;
import io.github.talib.RangeType;

Core core = Core.builder()
    .candleSetting(CandleSettingType.BODY_LONG, RangeType.REAL_BODY, 10, 1.0)
    .build();
```

Each setter throws `IllegalArgumentException` at once if an argument is out of range, so `build()` cannot fail.

### 4.4 Input Type: float vs. double {#input_type}

Every indicator is overloaded for `float[]` inputs as well as `double[]`. The `float` overload converts each element to `double` when read, so both produce the same output, bit-for-bit. Use it to feed price data already stored as `float` without a conversion copy.

### 4.5 Index Range {#index_range}

`Core.INDEX_MAX` is the largest value `startIdx` or `endIdx` may take: **100,000,000**. It's a sanity bound. Past it, a call is more likely a caller bug than a real need, and it's also untested territory for overflow and rounding error.

### 4.6 Threading {#multithreading}

**`Core` is immutable**, so one instance is safe to share across any number of threads with no synchronization.

Use `Core.DEFAULT` for the all-defaults instance. There are no setters: to change a setting, derive a new instance with `core.toBuilder()`. Read a configured unstable period back with `core.unstablePeriod(FuncUnstId.EMA)`.

## 5.0 Documentation {#docs}

Every function has Javadoc, published to Maven Central as the artifact's `javadoc` jar, which an IDE fetches alongside the library.
