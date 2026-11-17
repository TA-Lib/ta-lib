---
title: Go Core API
description: "The official TA-Lib in pure Go: every technical analysis function as a method on Core over float64 slices, batch and streaming, with no C library to install."
toc: false
---

::: warning Not yet released
The Go module is planned for official release in Q4 2026.
:::

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
<a href="#input_type">4.4 Input and Output Slices</a><br>
<a href="#index_range">4.5 Index Range</a><br>
<a href="#goroutines">4.6 Goroutines</a></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The official TA-Lib in pure Go: every technical analysis function, batch and streaming. No C library to install.

Every indicator is a method on `Core`: it reads `float64` slices you pass in and writes its results into slices you allocate. The module has no dependencies.

The **Core API** provides:

- The [`Core`](#direct_call) type, made by `NewCore` or used as a zero value: both hold the default settings.
- The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Several `Core` values can co-exist, say for different settings.
- Every TA function, each processing a whole slice of data at once.

To process a live feed one bar at a time instead, see the companion [Go Streaming API](/api/go/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, Go has `talib.NewCore()`, and an unreferenced `Core` is garbage-collected.

## 2.0 Add it to your project {#build}

```go
import talib "ta-lib.github.io/go"
```

It needs Go 1.22 or later.

On x86-64, build with `GOAMD64=v3` when every machine that runs the program has AVX2 and FMA: the functions that use a fused multiply-add, such as T3, MACD and TEMA, run faster, with the same results. A v3 binary does not start on an older CPU.

## 3.0 Calling into TA-Lib {#ta_func}

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from slices you pass in, writes its results into slices you allocate, and returns where the values landed.

As an example, let's walk through `MA`, a method to calculate a moving average.

<pre>func (c *Core) MA( <span class="ta-arg-range">startIdx, endIdx int,</span>
                   <span class="ta-arg-in">inReal []float64,</span>
                   <span class="ta-arg-opt">optInTimePeriod int,</span>
                   <span class="ta-arg-opt">optInMAType MAType,</span>
                   <span class="ta-arg-out">outReal []float64</span> ) (OutRange, error)
</pre>

All TA functions use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output is calculated only for the range from startIdx to endIdx, zero-based and inclusive indices into the input slices. Bars before startIdx are still read as warm-up, which slicing the input would lose.</span></li>
<li><span class="ta-arg-in">One or more input slices come next, oldest bar first. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional parameters follow. They give finer control specific to each function. To take a parameter's documented default, pass <code>talib.IntegerDefault</code>, <code>talib.RealDefault</code> or <code>talib.MATypeDEFAULT</code>, depending on its type.</span></li>
<li><span class="ta-arg-out">One or more output slices come last. In this example there is only one output (outReal). Where the values landed is the return value: on success you get an <code>OutRange</code>.</span></li>
</ul>

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

```go
core := talib.NewCore()

closes := make([]float64, 400) // ...your closing prices, oldest first...
out := make([]float64, len(closes))

r, err := core.MA(0, 399, closes, 30, talib.MATypeSMA, out)
if err != nil {
	log.Fatal(err)
}

// out[i] is the SMA at input bar r.BegIdx+i.
for i := 0; i < r.NBElement; i++ {
	fmt.Printf("Day %d = %f\n", r.BegIdx+i, out[i])
}
```

After the call, read `r` to learn what was produced. Even though we requested the whole range (0 to 399), a 30-day average is not defined until the 30th day. Consequently, `r.BegIdx` will be 29 (zero-based) and `r.NBElement` will be 400-29 = 371. Only the first 371 elements of `out` are written, and they correspond to input elements 29 through 399. Nothing is padded.

If you had requested only the range 125 to 225, `r.BegIdx` would be 125 and `r.NBElement` 101 (`endIdx` is inclusive). The 125 closing prices before the range provide the history the average needs.

If you do not provide enough data to calculate even one value, the call still succeeds and `r.NBElement` is 0.

A function with several outputs takes one slice per output, in the order of its Outputs list, and reports one range for all of them:

```go
macd := make([]float64, len(closes))
signal := make([]float64, len(closes))
hist := make([]float64, len(closes))

r, err := core.MACD(0, len(closes)-1, closes, 12, 26, 9, macd, signal, hist)
```

`Core` is cheap to create and holds only the library's settings; create one and reuse it.

### 3.2 Output Size and Lookback {#output_size}

An output slice must hold the values the call writes, and at least one element: a short or empty one is `talib.BadParam`, never a write past the end. The input length always suffices, and the lookback gives the exact count (see the [three sizing methods](/api/#output_size)).

Each TA function has a matching `<NAME>Lookback` method, taking the same optional parameters as the function itself. Example: for `SMA` it is `SMALookback`.

```go
lookback, err := core.SMALookback(30) // 29 for a 30-period SMA
```

The lookback is the number of input elements consumed before the first output. A parameter out of range returns `talib.BadParam`, the same error the function itself would return for it.

Putting it together, the exact allocation for any TA function:

```go
lookback, err := core.SMALookback(30)
if err != nil {
	log.Fatal(err)
}

size := 1
if first := max(lookback, startIdx); first <= endIdx {
	size = endIdx - first + 1
}
out := make([]float64, size)
```

### 3.3 Display Shift {#display_shift}

A few indicators are conventionally drawn at another bar than the one that computes them. The values are never moved: the matching `<NAME>DisplayShift` method gives the number of bars to shift one output when charting it, positive ahead and negative behind. It is 0 for almost every function.

```go
shift, err := core.DPODisplayShift(20, 0) // period 20, first output: -11
```

It returns `talib.BadParam` for a parameter the lookback rejects or an index that names no output. The same answer comes from the [Abstract API](/api/abstract/). The rules are in the [specification](/spec/lookback/#display-shift).

### 3.4 Errors {#retcode}

Every TA function returns `(OutRange, error)`. A failed call returns a `RetCode` as its error and writes nothing; test for one with `errors.Is`.

| Error | Mistake |
|------|---------|
| `talib.OutOfRangeStartIndex` | `startIdx` negative or above `talib.IndexMax` |
| `talib.OutOfRangeEndIndex` | `endIdx` negative, above `talib.IndexMax`, or below `startIdx` |
| `talib.BadParam` | A parameter outside its documented range |
| `talib.BadParam` | An input too short for the range requested, or an output too short for the values written |
| `talib.BadParam` | An empty input or output |
| `talib.BadParam` | An output sharing memory with an input or another output, other than in place, unless a three-index slice hides it ([4.4](#input_type)) |

```go
_, err := core.SMA(0, len(closes)-1, closes, 0, out)
if errors.Is(err, talib.BadParam) {
	// a period of 0 is out of range
}
```

A `RetCode`'s `Error()` is C's name for it, such as `TA_BAD_PARAM`. `talib.InternalError` is a bug in TA-Lib to report ([rB8](/spec/errors/#rb8)).

A `NaN` or `±Inf` inside an input series is not detected. Clean the series before calling.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstract API {#abstract}

The package `ta-lib.github.io/go/abstract` is the [Abstract API](/api/abstract/) in Go: `abstract.Lookup` and `abstract.All` describe every function at run time (group, flags, inputs, parameters with their domains and defaults, outputs), and a `ParamHolder` from `NewCall` binds the arguments of a function chosen by name and calls it through the same method on `talib.Core` a direct call uses. It is a package of its own, so a program that imports only `talib` links none of it.

### 4.2 Numerical Stability {#numerical_stability}

Your value changed when you fed the same bar more history? That is by design: recursive functions converge as history accumulates. See [Unstable Period](/api/unstable-period/) for how to mitigate that.

Rounding is a separate axis: floating-point error accumulates over a very long series, which is one reason a call is capped at [`talib.IndexMax`](#index_range).

Every function documentation page carries a [numerical-stability property](/functions/stability): how much the value at a given bar depends on where the series you passed in begins.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern functions judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults. Set them on the `Core` you call with:

```go
core := talib.NewCore()
if err := core.SetCandleSetting(talib.CandleSettingTypeBodyDoji, talib.RangeTypeHighLow, 10, 0.1); err != nil {
	log.Fatal(err)
}

out := make([]int32, len(close))
r, err := core.CDLDOJI(0, len(close)-1, open, high, low, close, out)
```

A setter returns `talib.BadParam` for an argument out of range and changes nothing.

### 4.4 Input and Output Slices {#input_type}

An input is a `[]float64`. An output is a `[]float64`, or an `[]int32` for an integer output such as a candlestick pattern, and is written from index 0.

You can pass the same slice, with the same start and length, as an input and as an output to compute in place. An output may not otherwise share memory with an input or with another output: the call returns `talib.BadParam`. A slice whose capacity a three-index expression cut (`s[i:j:k]`, as `slices.Clip` makes) escapes the check: such a call can succeed with wrong values.

### 4.5 Index Range {#index_range}

`talib.IndexMax` is the largest value `startIdx` or `endIdx` may take: **100,000,000**. It's a sanity bound. Past it, a call is more likely a caller bug than a real need, and it's also untested territory for overflow and rounding error.

### 4.6 Goroutines {#goroutines}

A `Core` is safe to share between goroutines once configured: call its setters only while no other goroutine uses it and no stream opened on it is in use. For different settings, use another `Core`. A [stream](/api/go/stream/#rules) belongs to one goroutine.

## 5.0 Documentation {#docs}

Every exported name carries a doc comment, and every function a runnable example. Read them with `go doc`.
