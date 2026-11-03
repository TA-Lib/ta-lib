---
url: 'https://ta-lib.org/api/typescript/index.md'
description: >-
  ta-lib for TypeScript and JavaScript: a pure TypeScript port compiled to
  standard ES modules, with no WASM and no native addon, indicators as functions
  over typed arrays, bit-identical to the reference C library apart from
  last-bit rounding in the math library.
---
# TypeScript Core API

::: warning Not yet released
The TypeScript library is not on npm yet. Until it is, build it from source ([2.0](#build)).
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
<a href="#input_type">4.4 Input and Output Arrays</a><br>
<a href="#index_range">4.5 Index Range</a><br>
<a href="#settings">4.6 Changing Settings</a></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The TypeScript library is a native port of TA-Lib: pure TypeScript compiled to standard ES modules, with no WASM, no native addon and no dependencies. Every indicator is a function exported by the `ta-lib` package. It reads its series from arrays you pass in, writes into typed arrays you allocate, and is **bit-identical** to the reference C library over the same inputs, with one exception beyond TA-Lib's control: where a calculation calls a transcendental math function (such as `exp`, `log`, `sin` or `atan`), the JavaScript engine's `Math` functions can round the result differently from the C library in the last bit ([rD1](/spec/versions/#rd1)).

The **Core API** provides:

* The [`Core`](#candle_settings) type and the `CoreBuilder` that configures it.
* The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Pass a `Core` as the last argument of any call; without one, the call uses `Core.DEFAULT`.
* Every TA function, each processing a whole array of data at once.
* An optional [Abstract API](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead, see the companion [TypeScript Streaming API](/api/typescript/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, TypeScript has `Core.DEFAULT` ready immediately, and an unreferenced `Core` is simply garbage-collected.

## 2.0 Add it to your project {#build}

The package is not on npm yet. Until it is, build it from a clone of the [repository](https://github.com/TA-Lib/ta-lib):

```bash
cd ta_codegen/output/ts
npm ci
node_modules/.bin/tsc -p library
```

Then depend on the built directory from your project's `package.json`:

```json
"dependencies": {
  "ta-lib": "file:path/to/ta-lib/ta_codegen/output/ts/library"
}
```

The package is ESM only, with no top-level await, so `require('ta-lib')` works too. It needs Node.js 22.12 or later, and the library imports nothing specific to Node.

## 3.0 Calling into TA-Lib {#ta_func}

Every indicator is a function taking the same startIdx/endIdx/inputs/optional-parameters/outputs shape as the C function it mirrors.

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from arrays you pass in and writes its results into typed arrays you allocate.

A function never writes more elements than you request, so the output array only needs to cover the `startIdx`-to-`endIdx` range.

As an example, let's walk through `SMA`, a function to calculate a moving average.

<pre>function sma( <span class="ta-arg-range">startIdx: number,</span>
              <span class="ta-arg-range">endIdx: number,</span>
              <span class="ta-arg-in">inReal: ArrayLike&lt;number&gt;,</span>
              <span class="ta-arg-opt">optInTimePeriod: number,</span>
              <span class="ta-arg-out">outReal: Float64Array,</span>
              core?: Core ): OutRange
</pre>

All TA functions use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input arrays.</span></li>
<li><span class="ta-arg-in">One or more input arrays are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there is one optional input. These parameters give finer control specific to each function. If you do not care about a particular optIn, just pass <code>Core.INTEGER_DEFAULT</code> or <code>Core.REAL_DEFAULT</code> (depending on the type). For a moving-average type, use <code>MAType.DEFAULT</code>.</span></li>
<li><span class="ta-arg-out">One or more output arrays come next. In this example there is only one output (outReal). Where the values landed is the return value, not a parameter: on success you get an <code>OutRange</code>.</span></li>
</ul>

The optional `core` comes last: the settings to compute with.

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>import { sma } from 'ta-lib';

const close = new Float64Array([ /* ...your closing prices... */ ]);
const out = new Float64Array(close.length);

const r = sma(
    <span class="ta-arg-range">0</span>, <span class="ta-arg-range">close.length - 1</span>,
    <span class="ta-arg-in">close</span>,
    <span class="ta-arg-opt">30</span>,
    <span class="ta-arg-out">out</span> );

// out[0 .. r.count - 1] holds the SMA; out[i] is input bar r.begIdx + i.
for (let i = 0; i &lt; r.count; i++) {
  console.log(`bar ${r.begIdx + i} = ${out[i]}`);
}
</pre>

After the call, read `r` to learn what was produced. Even though we requested the whole range (`0` to `close.length - 1`), a 30-day average is not defined until the 30th day. Consequently `r.begIdx` will be 29 (zero-based) and `r.count` will be `close.length - 29`. In other words, only that many elements of `out` are written, corresponding to input elements 29 through the end.

If you do not provide enough data to calculate even one value, the call still succeeds and `r.count` is 0 (`r.isEmpty()`).

`OutRange` has two read-only fields, `begIdx` and `count` (C's `outBegIdx` and `outNBElement`), plus the conveniences `isEmpty()` and `OutRange.EMPTY`.

### 3.2 Output Size and Lookback {#output_size}

Size the output array to at least `endIdx - startIdx + 1`, or exactly with the lookback (see the [three sizing methods](/api/#output_size)):

```ts
import { smaLookback } from 'ta-lib';

const lookback = smaLookback(30);   // 29 for a 30-period SMA
```

Each TA function has a matching `<name>Lookback` function, taking the same optional parameters as the function itself. The lookback is how many inputs are consumed before the first output.

### 3.3 Display Shift {#display_shift}

A few indicators are conventionally drawn at another bar than the one that computes them. The values are never moved: the matching `<name>DisplayShift` function gives the number of bars to shift one output when charting it, positive ahead and negative behind. It is 0 for almost every function.

```ts
import { dpoDisplayShift } from 'ta-lib';

const shift = dpoDisplayShift(20, 0);   // period 20, first output: -11
```

The same answer comes from the [Abstract API](/api/abstract/). The rules are in the [specification](/spec/lookback/#display-shift).

### 3.4 Errors {#retcode}

Misuse throws a `TALibError` rather than returning a return code. Its `code` names the condition with C's spelling; branch on it. Its `retCode` is C's number for the same code.

| Mistake | `code` |
|---|---|
| `startIdx` negative or above `Core.INDEX_MAX` | `'TA_OUT_OF_RANGE_START_INDEX'` |
| `endIdx` negative, above `Core.INDEX_MAX`, or below `startIdx` | `'TA_OUT_OF_RANGE_END_INDEX'` |
| Optional parameter outside its documented range | `'TA_BAD_PARAM'` |
| An index or integer parameter that is not a 32-bit integer, such as `14.5`, or a real parameter that is not a number, such as `'0.02'` | `'TA_BAD_PARAM'` |
| Two outputs sharing one array | `'TA_BAD_PARAM'` |
| An array too short for the range requested, including an `endIdx` past the end of the input | `'TA_BAD_PARAM'` |
| An absent or empty array | `'TA_BAD_PARAM'` |

```ts
import { sma, TALibError } from 'ta-lib';

try {
  sma(0, close.length - 1, close, 0, out);
} catch (e) {
  if (e instanceof TALibError && e.code === 'TA_BAD_PARAM') {
    // e.retCode is 2, as C's TA_BAD_PARAM
  }
}
```

A rejected call leaves every array untouched.

You can pass the same `Float64Array` as an input and an output to compute in place. Avoid a partial overlap: its result is [unspecified](/spec/inputs-outputs/#no-overlap).

A `NaN` or `±Inf` inside an input series is not detected. Clean the series before calling.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstract API {#abstract}

The `ta-lib/metadata` entry point describes every function at run time and calls it without naming it at compile time. Useful for a UI, a scripting bridge, or anything that enumerates indicators. See the [Abstract API](/api/abstract/) page.

### 4.2 Numerical Stability {#numerical_stability}

Your value changed when you fed the same bar more history? That is by design: recursive functions converge as history accumulates. See [Unstable Period](/api/unstable-period/) for how to mitigate that.

Rounding is a separate axis: floating-point error accumulates over a very long series, which is one reason a call is capped at [`Core.INDEX_MAX`](#index_range).

Every function documentation page carries a [numerical-stability property](/functions/stability): how much the value at a given bar depends on where the series you passed in begins.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern functions judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults. The builder sets them, and the call takes the built `Core` as its last argument:

```ts
import { CandleSettingType, CoreBuilder, RangeType, cdldoji } from 'ta-lib';

const core = new CoreBuilder()
  .candleSetting(CandleSettingType.BODY_DOJI, RangeType.HIGH_LOW, 10, 0.1)
  .build();

const pattern = new Int32Array(close.length);
cdldoji(0, close.length - 1, open, high, low, close, pattern, core);
```

Each setter throws `TALibError` (`'TA_BAD_PARAM'`) at once if an argument is out of range, so `build()` cannot fail.

### 4.4 Input and Output Arrays {#input_type}

An input is any `ArrayLike<number>`. A `Float64Array` is read in place; any other form, such as a `number[]` or a `Float32Array`, is first copied into a `Float64Array`, each element converted to a double. A `Float32Array` therefore gives the same output, bit for bit, as a `Float64Array` holding the same values.

An output is a `Float64Array`, or an `Int32Array` for an integer output such as a candlestick pattern.

### 4.5 Index Range {#index_range}

`Core.INDEX_MAX` is the largest value `startIdx` or `endIdx` may take: **100,000,000**. It's a sanity bound. Past it, a call is more likely a caller bug than a real need, and it's also untested territory for overflow and rounding error.

### 4.6 Changing Settings {#settings}

**`Core` is immutable.** There are no setters: to change a setting, derive a new instance with `core.toBuilder()`. Read a configured unstable period back with `core.unstablePeriod(FuncUnstId.EMA)`.

## 5.0 Documentation {#docs}

Every exported function and type carries TSDoc in the package's `.d.ts` files, so an editor shows it on hover with no separate doc build.
