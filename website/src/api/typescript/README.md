---
title: TypeScript Core API
description: "ta-lib for TypeScript and JavaScript: a native port compiled to standard ES modules, with no native addon, indicators as functions over typed arrays, bit-identical to the reference C library apart from last-bit rounding in the math library. WebAssembly-accelerated, with automatic TypeScript fallback."
toc: false
---

::: warning Not yet released
The TypeScript library is planned for official release in Q4 2026. Until then it is not on npm: [build it from source](#build).
:::

<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 Add it to your project</a></p>

<blockquote>
<p><a href="#wasm">2.1 WebAssembly</a></p>
</blockquote>

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

The TypeScript library is a native port of TA-Lib: TypeScript compiled to standard ES modules, with no native addon and no dependencies. WebAssembly-accelerated, with automatic TypeScript fallback ([2.1](#wasm)). Every indicator is a function exported by the `ta-lib` package. It reads its series from arrays you pass in, returns its results in typed arrays, and is **bit-identical** to the reference C library over the same inputs, with one exception beyond TA-Lib's control: where a calculation calls a transcendental math function (such as `exp`, `log`, `sin` or `atan`), the JavaScript engine's `Math` functions can round the result differently from the C library in the last bit ([rD1](/spec/versions/#rd1)).

The **Core API** provides:

- The [`Core`](#candle_settings) type and the `CoreBuilder` that configures it.
- The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Pass a `Core` as the `core` option of any call; without one, the call uses `Core.DEFAULT`.
- Every TA function, each processing a whole array of data at once.
- An optional [Abstract API](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead, see the companion [TypeScript Streaming API](/api/typescript/stream/).

No call needs an initialization step, and there is nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, TypeScript has `Core.DEFAULT` ready immediately, and an unreferenced `Core` is simply garbage-collected. The optional `ready()` loads the WebAssembly engine ([2.1](#wasm)).

## 2.0 Add it to your project {#build}

Until the package is on npm, build it from a clone of the [repository](https://github.com/TA-Lib/ta-lib), with Rust installed for the WebAssembly engine:

```bash
scripts/build.py wasm
cd ta_codegen/output/ts
npm ci
node_modules/.bin/tsc -p library
node ../../../scripts/ts_wasm_bytes.mjs library
```

Then depend on the built directory from your project's `package.json`:

```json
"dependencies": {
  "ta-lib": "file:path/to/ta-lib/ta_codegen/output/ts/library"
}
```

The published package is emitted from the same sources twice: `--removeComments --declaration false` for the JavaScript, so its `.js` keep no comment but the licence block, and `--emitDeclarationOnly` for the declarations, which keep the documentation. The package is ESM only, with no top-level await, so `require('ta-lib')` works too, on every supported Node.js line. It needs Node.js 22.12 or later, and the library imports nothing specific to Node. A TypeScript consumer needs TypeScript 5.7 or later, since the declarations spell typed arrays with their buffer type, and a CommonJS TypeScript project 5.8 or later with `module: nodenext`. Jest needs its ESM mode. A library that depends on `ta-lib` should declare it a peer dependency, so that an application holds one copy: a call rejects a `Core` built by another copy.

### 2.1 WebAssembly {#wasm}

WebAssembly-accelerated, with automatic TypeScript fallback. The `ta-lib` and `ta-lib/abstract` entries run each batch call in their WebAssembly engine or in TypeScript, choosing by function and by the number of bars the call reads, with or without your own output buffers, and the results are the same either way, bit for bit on finite inputs. A call runs in TypeScript, with the same values, when it passes a `core` other than the `Core.DEFAULT` object, computes in place, or can reach a transcendental function.

There is nothing to configure. In Node.js the engine loads on the first call that uses it, and awaiting `ready()` first moves its one-time compile out of that call. In a browser, and in any bundle built without the `node` export condition, calls run in TypeScript until `ready()` has loaded the engine, which each Worker calls for itself:

```ts
import { ready } from 'ta-lib';

const { engine, reason } = await ready();   // 'wasm' once loaded, or 'js' and the reason
```

`ready()` never rejects. A loaded engine still runs only the calls its routing sends it, so `'wasm'` does not mean a given call ran there.

The engine needs WebAssembly relaxed SIMD with a fused multiply-add. Where that is missing, or a Content-Security-Policy forbids compiling WebAssembly (`'wasm-unsafe-eval'` in `script-src` allows it), the package runs its TypeScript code with the same values; under such a policy, a page that calls `ready()` also gets the browser's CSP violation report. A bundler that splits code fetches the engine's bytes only when `ready()` is called; one that does not puts them in the main bundle. `ta-lib/js` and `ta-lib/js/abstract` are the same APIs without `ready`, with WebAssembly never loaded, for a page with a strict Content-Security-Policy or a bundle-size budget. A build for the `workerd`, `edge-light` or `react-native` export condition resolves `ta-lib` and `ta-lib/abstract` to entries with no engine, where every call runs in TypeScript and `ready()` resolves to `'js'` with reason `'unavailable'`. `getRuntimeInfo('wasm.engine')` from `ta-lib/abstract` is 1 once the engine is loaded and 0 otherwise, and `'wasm.reason'` says why ([run-time information](/spec/abstract/#runtime-info)).

A stream is a TypeScript object, of one class in every entry ([Streaming API](/api/typescript/stream/)).

## 3.0 Calling into TA-Lib {#ta_func}

Every indicator is a function taking its input arrays, then one optional object of options, and returning its results.

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from arrays you pass in and returns its results in typed arrays, allocated for you unless you pass your own.

As an example, let's walk through `SMA`, a function to calculate a moving average.

<pre>function sma( <span class="ta-arg-in">real: ArrayLike&lt;number&gt;,</span>
              options?: {
                <span class="ta-arg-opt">timePeriod?: number,</span>
                <span class="ta-arg-range">startIdx?: number,</span>
                <span class="ta-arg-range">endIdx?: number,</span>
                core?: Core,
                <span class="ta-arg-out">outReal?: Float64Array&lt;ArrayBuffer&gt;</span> } ): { begIdx: number, values: Float64Array&lt;ArrayBuffer&gt; }
</pre>

All TA functions use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-in">The input arrays come first. Typically, these are the "price" data. In this example there is only one input. The components of a price input are one object, keyed <code>open</code>, <code>high</code>, <code>low</code>, <code>close</code>, <code>volume</code> and <code>openInterest</code>: <code>atr({ high, low, close })</code>, <code>obv(close, { volume })</code>. An object with more components than the function reads, a whole bar, passes.</span></li>
<li><span class="ta-arg-opt">The parameters are keys of the options object, named as in C without <code>optIn</code> and in camelCase (<a href="/spec/#rj1">rJ1</a>): <code>timePeriod</code>, <code>nbDevUp</code>, <code>maType</code>, <code>fastDMaType</code>. They give finer control specific to each function. A key you omit, or set to <code>undefined</code>, takes its default; <code>Core.INTEGER_DEFAULT</code>, <code>Core.REAL_DEFAULT</code> and <code>MAType.DEFAULT</code> select it too.</span></li>
<li><span class="ta-arg-range">The output is calculated only for the range from startIdx to endIdx, zero-based and inclusive indices into the input arrays. Omitted, the range is the whole input. The lookback's bars before startIdx are still read, which slicing the input there would lose.</span></li>
<li><span class="ta-arg-out">Output buffers are optional, one key per output (outReal here): see <a href="#input_type">4.4</a>. Without them, each output is allocated.</span></li>
</ul>

The optional `core` key holds the settings to compute with.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>import { sma } from 'ta-lib';

const close = new Float64Array([ /* ...your closing prices... */ ]);

const { begIdx, values } = sma( <span class="ta-arg-in">close</span>, { <span class="ta-arg-opt">timePeriod: 30</span> } );

// values[i] is the SMA at input bar begIdx + i.
for (let i = 0; i &lt; values.length; i++) {
  console.log(`bar ${begIdx + i} = ${values[i]}`);
}
</pre>

The result says what was produced. Even though the whole series was requested, a 30-day average is not defined until the 30th day. Consequently `begIdx` will be 29 (zero-based) and `values.length` will be `close.length - 29`: the values for input elements 29 through the end. Nothing is padded.

If you do not provide enough data to calculate even one value, the call still succeeds: `begIdx` is 0 and `values` is empty.

A function with several outputs returns one array per output, each named after its C output in camelCase without C's type word (<code>outRealUpperBand</code> is <code>outUpperBand</code>), and all of the same length:

```ts
import { macd } from 'ta-lib';

const { begIdx, outMacd, outMacdSignal, outMacdHist } = macd(close);
```

### 3.2 Output Size and Lookback {#output_size}

A buffer you pass must hold the values the call writes, and at least one element: the input length always suffices, and the lookback gives the exact count (see the [three sizing methods](/api/#output_size)):

```ts
import { smaLookback } from 'ta-lib';

const lookback = smaLookback({ timePeriod: 30 });   // 29 for a 30-period SMA
```

Each TA function has a matching `<name>Lookback` function. It takes the function's own options object and ignores the option keys it has no use for, so one object serves both calls. The lookback is how many inputs are consumed before the first output. Parameters the function rejects give `Core.LOOKBACK_INVALID` (-1).

### 3.3 Display Shift {#display_shift}

A few indicators are conventionally drawn at another bar than the one that computes them. The values are never moved: the matching `<name>DisplayShift` function gives the number of bars to shift one output when charting it, positive ahead and negative behind. It is 0 for almost every function. It takes the output's index, then the options object.

```ts
import { dpoDisplayShift } from 'ta-lib';

const shift = dpoDisplayShift(0, { timePeriod: 20 });   // first output, period 20: -11
```

Rejected parameters, or an index that names no output, give `Core.DISPLAY_SHIFT_INVALID`. The same answer comes from the [Abstract API](/api/abstract/). The rules are in the [specification](/spec/lookback/#display-shift).

### 3.4 Errors {#retcode}

Misuse throws a `TALibError` rather than returning a return code. Its `code` names the condition with C's spelling; branch on it rather than on `instanceof`, which fails across two copies of the package. Its `retCode` is C's number for the same code.

| Mistake | `code` |
|---|---|
| `startIdx` negative or above `Core.INDEX_MAX` | `'TA_OUT_OF_RANGE_START_INDEX'` |
| `endIdx` negative, above `Core.INDEX_MAX`, or below `startIdx` | `'TA_OUT_OF_RANGE_END_INDEX'` |
| Parameter outside its documented range | `'TA_BAD_PARAM'` |
| An index or integer parameter that is not a 32-bit integer, such as `14.5`, or a real parameter that is not a number, such as `'0.02'` | `'TA_BAD_PARAM'` |
| Options that are not a plain object, such as an array, a `Map` or a `Core` | `'TA_BAD_PARAM'` |
| An option key the function does not have, such as a misspelt `timeperiod` | `'TA_BAD_PARAM'` |
| Two output buffers sharing memory, or a buffer partly overlapping a `Float64Array` input | `'TA_BAD_PARAM'` |
| A `core` that is not a `Core` | `'TA_BAD_PARAM'` |
| An input too short for the range requested, including an `endIdx` past its end, or a buffer too short for the values written | `'TA_BAD_PARAM'` |
| An absent or empty input, an input that is not array-like, a price object that is not an object, or an empty buffer | `'TA_BAD_PARAM'` |
| A buffer that is not a `Float64Array`, or not an `Int32Array` for an integer output | `'TA_BAD_PARAM'` |

```ts
import { sma, type TALibError } from 'ta-lib';

try {
  sma(close, { timePeriod: 0 });
} catch (e) {
  const err = e as TALibError;
  if (err?.code === 'TA_BAD_PARAM') {
    // err.retCode is 2, as C's TA_BAD_PARAM
  }
}
```

A rejected call leaves every buffer untouched.

A `NaN` or `±Inf` inside an input series is not detected, and neither is an element that is not a number, which is converted (`null` becomes 0). Clean the series before calling.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstract API {#abstract}

The `ta-lib/abstract` entry point describes every function at run time and calls it without naming it at compile time, routing each call as `ta-lib` does ([2.1](#wasm)); `ta-lib/js/abstract` is the same API without `ready`, with WebAssembly never loaded. Either one references every function. Useful for a UI, a scripting bridge, or anything that enumerates indicators. See the [Abstract API](/api/abstract/) page.

### 4.2 Numerical Stability {#numerical_stability}

Your value changed when you fed the same bar more history? That is by design: recursive functions converge as history accumulates. See [Unstable Period](/api/unstable-period/) for how to mitigate that.

Rounding is a separate axis: floating-point error accumulates over a very long series, which is one reason a call is capped at [`Core.INDEX_MAX`](#index_range).

Every function documentation page carries a [numerical-stability property](/functions/stability): how much the value at a given bar depends on where the series you passed in begins.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern functions judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults. The builder sets them, and the call takes the built `Core` as its `core` option:

```ts
import { CandleSettingType, CoreBuilder, RangeType, cdldoji } from 'ta-lib';

const core = new CoreBuilder()
  .candleSetting(CandleSettingType.BODY_DOJI, RangeType.HIGH_LOW, 10, 0.05)
  .build();

const { begIdx, values } = cdldoji({ open, high, low, close }, { core });   // values: Int32Array<ArrayBuffer>
```

A call that passes a configured `Core` runs in TypeScript ([2.1](#wasm)). Each setter throws `TALibError` (`'TA_BAD_PARAM'`) at once if an argument is out of range, so `build()` cannot fail.

### 4.4 Input and Output Arrays {#input_type}

An input is any `ArrayLike<number>`. A `Float64Array` is read in place; any other form, such as a `number[]` or a `Float32Array`, is first copied into a `Float64Array`, each element converted to a double. A `Float32Array` therefore gives the same output, bit for bit, as a `Float64Array` holding the same values.

An output is a `Float64Array<ArrayBuffer>`, or an `Int32Array<ArrayBuffer>` for an integer output such as a candlestick pattern: a buffer you pass is typed as backed by an `ArrayBuffer` (one over a `SharedArrayBuffer` needs a cast; the call does not inspect it), and a result goes to a `Blob` or a `fetch` body without one. Annotate a buffer you keep in a variable, a field or a parameter as `Float64Array<ArrayBuffer>` (`Int32Array<ArrayBuffer>`): TypeScript can reject a plain `Float64Array` annotation there, with a message that names `SharedArrayBuffer`. To reuse memory, pass your own buffer under the output's key; the result returns a view of it, trimmed to the values written:

```ts
const buf = new Float64Array(close.length);
const { values } = sma(close, { timePeriod: 30, outReal: buf });   // values: a view of buf, no copy
```

A single output's key is `outReal` or `outInteger`; a multi-output function's keys are its result names (`outMacd`, `outMacdSignal`, `outMacdHist`). Each buffer is optional on its own: an omitted one is allocated. A buffer is written from index 0, sized as in [3.2](#output_size), and must not share memory with another buffer.

You can pass the same `Float64Array` as an input and as an output buffer to compute in place. A buffer that overlaps a `Float64Array` input only partly is `'TA_BAD_PARAM'`; any other input is copied first. To feed a result into the next call in place, pass its view as both: `sma(r.values, { outReal: r.values })`.

### 4.5 Index Range {#index_range}

`Core.INDEX_MAX` is the largest value `startIdx` or `endIdx` may take: **100,000,000**. It's a sanity bound. Past it, a call is more likely a caller bug than a real need, and it's also untested territory for overflow and rounding error.

### 4.6 Changing Settings {#settings}

**`Core` is immutable.** There are no setters: to change a setting, derive a new instance with `core.toBuilder()`. Read a configured unstable period back with `core.unstablePeriod(FuncUnstId.EMA)`, a candle setting with `core.candleSetting(CandleSettingType.BODY_DOJI)`.

## 5.0 Documentation {#docs}

Every exported function and type carries TSDoc in the package's `.d.ts` files, so an editor shows it on hover with no separate doc build.
