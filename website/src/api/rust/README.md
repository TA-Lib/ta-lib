---
title: Rust Core API
description: "The ta-lib Rust crate: a native port with no C bindings, indicators as methods on Core over f64 slices, bit-identical to the reference C library."
toc: false
---

::: warning Not yet released
The Rust API is not yet released. Estimated release: **Q1 2027**.
:::

<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 Add it to your project</a></p>

<p><a href="#ta_func">3.0 Calling into TA-Lib</a></p>

<blockquote>
<p><a href="#direct_call">3.1 Batch Processing</a><br>
<a href="#output_size">3.2 Output Size and Lookback</a><br>
<a href="#retcode">3.3 Return Codes</a><br>
<a href="#variants">3.4 Variants per Indicator</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced Features</a></p>

<blockquote>
<p><a href="#abstract">4.1 Abstraction Layer</a><br>
<a href="#numerical_stability">4.2 Numerical Stability</a><br>
<a href="#candle_settings">4.3 Candlestick Settings</a><br>
<a href="#input_type">4.4 Input Type</a><br>
<a href="#multithreading">4.5 Threading</a><br></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The `ta-lib` crate is a native Rust port of TA-Lib — no C bindings, no `unsafe` at the call site. Every indicator is a method on a `Core` value, operates on `f64` slices, and is **bit-identical** to the reference C library over the same inputs.

The **Core API** provides:

- The [`Core`](#direct_call) value and the builder that configures it.
- The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Nothing is global.
- Every TA function, each processing a whole array of data at once.
- An optional [abstraction layer](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead of a whole array, see the companion [Rust Streaming API](/api/rust/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, Rust has `Core::new()`, and the value is dropped like any other.

## 2.0 Add it to your project {#build}

```toml
[dependencies]
ta-lib = "0.8"
```

## 3.0 Calling into TA-Lib {#ta_func}

### 3.1 Batch Processing {#direct_call}

Every function follows the same simple pattern: it reads its inputs from slices you pass in and writes its results into slices you allocate.

A function never writes more elements than you request, so the output slice only needs to cover the `startIdx`-to-`endIdx` range.

As an example, let's walk through `MA`, a method to calculate a moving average.

<pre>fn MA( &amp;self,
       <span class="ta-arg-range">startIdx: usize,</span>
       <span class="ta-arg-range">endIdx: usize,</span>
       <span class="ta-arg-in">inReal: &amp;[f64],</span>
       <span class="ta-arg-opt">optInTimePeriod: i32,</span>
       <span class="ta-arg-opt">optInMAType: i32,</span>
       <span class="ta-arg-out">outBegIdx: &amp;mut usize,</span>
       <span class="ta-arg-out">outNBElement: &amp;mut usize,</span>
       <span class="ta-arg-out">outReal: &amp;mut [f64]</span> ) -&gt; RetCode
</pre>

All TA functions use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input slices.</span></li>
<li><span class="ta-arg-in">One or more input slices are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there are two optional inputs. These parameters give finer control specific to each function. Integer and enum parameters are i32, real parameters are f64. If you do not care about a particular optIn, just pass INTEGER_DEFAULT or REAL_DEFAULT (depending on the type) and the function substitutes its documented default.</span></li>
<li><span class="ta-arg-out">One or more output slices come last. In this example there is only one output (outReal). The parameters outBegIdx and outNBElement always come just before the output slices.</span></li>
</ul>

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

```rust
use ta_lib::{Core, RetCode};

let core = Core::new();

let close: Vec<f64> = vec![0.0; 400];  // ...initialize your closing prices here...
let mut out = vec![0.0; 400];
let mut out_beg = 0usize;
let mut out_nb  = 0usize;
```

<pre>let ret_code = core.MA( <span class="ta-arg-range">0</span>, <span class="ta-arg-range">399</span>,
                        <span class="ta-arg-in">&amp;close</span>,
                        <span class="ta-arg-opt">30</span>, <span class="ta-arg-opt">0 /* SMA */</span>,
                        <span class="ta-arg-out">&amp;mut out_beg</span>, <span class="ta-arg-out">&amp;mut out_nb</span>, <span class="ta-arg-out">&amp;mut out</span> );
</pre>

```rust
assert_eq!(ret_code, RetCode::Success);

// The output is displayed here
for i in 0..out_nb {
    println!("Day {} = {}", out_beg + i, out[i]);
}
```

After the call, it is important to check the values written into `out_beg` and `out_nb`. Even though we requested the whole range (0 to 399), a 30-day average is not defined until the 30th day. Consequently, `out_beg` will be 29 (zero-based) and `out_nb` will be 400-29 = 371. In other words, only the first 371 elements of `out` are written, and they correspond to input elements 29 through 399.

As another example, if you had requested only the range 125 to 225, `out_beg` would be 125 and `out_nb` would be 101 (`endIdx` is inclusive: 225-125+1). The 30-day minimum is not a problem here, because the 125 closing prices before the requested range provide the needed history. As you may have guessed, only the first 101 elements of `out` are written; the rest is left untouched.

Here is another example. This time we calculate a 14-bar exponential moving average for a single price bar (say, the last one, at index 299):

<pre>let ret_code = core.MA( <span class="ta-arg-range">299</span>, <span class="ta-arg-range">299</span>,
                        <span class="ta-arg-in">&amp;close</span>,
                        <span class="ta-arg-opt">14</span>, <span class="ta-arg-opt">1 /* EMA */</span>,
                        <span class="ta-arg-out">&amp;mut out_beg</span>, <span class="ta-arg-out">&amp;mut out_nb</span>, <span class="ta-arg-out">&amp;mut out</span> );
</pre>

In this example, `out_beg` will be 299, `out_nb` will be 1, and only one value is written into `out`.

If you do not provide enough data to calculate even one value, `out_nb` will be 0 and `out_beg` should be ignored.

The input and the output are separate borrows, so a function cannot read and write the same buffer: pass a distinct output slice, and copy it back afterwards if you want the result in place.

`Core` is cheap to create and holds only the library's settings; construct one and reuse it.

### 3.2 Output Size and Lookback {#output_size}

It is important that the output slice is large enough — the crate is `#![forbid(unsafe_code)]`, so an undersized slice panics rather than writing past the end. Here are three ways to determine the allocation size; all of them work for every TA function:

| Method | Description |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Input Matching   | `allocation_size = endIdx + 1;` <br> **Pros**: Easy to understand and implement. <br> **Cons**: Memory allocation unnecessarily large when requesting a small range. |
| Range Matching   | `allocation_size = endIdx - startIdx + 1;` <br> **Pros**: Easy to implement. <br> **Cons**: Allocation slightly larger than needed. Example: with startIdx = 0, a 30-period SMA wastes 29 elements because of the lookback. |
| Exact Allocation | `let lookback = core.MA_Lookback(30, 0);` <br> `let temp = lookback.max(startIdx);` <br> `let allocation_size = if temp > endIdx { 0 } else { endIdx - temp + 1 };` <br> **Pros**: Allocates exactly what is needed. <br> **Cons**: Slightly more complex. |

Each TA function has a matching `<NAME>_Lookback` method, taking the same optional parameters as the function itself. Example: for `SMA` it is `SMA_Lookback`.

The lookback is the number of input elements consumed before the first output can be calculated. Example: a simple moving average (SMA) of period 10 has a lookback of 9.

```rust
let lookback = core.SMA_Lookback(30);   // 29 for a 30-period SMA
```

A lookback method returns `usize::MAX` when a parameter is out of range. Check for that before using the value as an allocation size.

Too little data is a success, not an error: a range shorter than the lookback simply produces no values, and `outNBElement` is `0`. When it is `0`, ignore `outBegIdx`.

### 3.3 Return Codes {#retcode}

Every TA function returns a [`RetCode`](https://docs.rs/ta-lib). `Success` means the call completed and wrote its outputs; on anything else, treat `outBegIdx` and `outNBElement` as undefined and the output slices as untouched. The codes a caller normally encounters:

| Code | Meaning |
|------|---------|
| `RetCode::Success` | No error. |
| `RetCode::BadParam` | An optional parameter is outside its documented range. |
| `RetCode::OutOfRangeStartIndex` | `startIdx` is above `MAX_INDEX` (100,000,000). |
| `RetCode::OutOfRangeEndIndex` | `endIdx` is above `MAX_INDEX`, or below `startIdx`. |

`RetCode` implements `std::error::Error`, so results compose with `?`. It also carries `AllocErr` and `InternalError`, which the safe Rust code paths do not produce.

Indexing is safe throughout: the crate is `#![forbid(unsafe_code)]`, so a violated slice-size precondition panics rather than reading or writing out of bounds. A call that returns `Success` with zero elements cannot panic.

### 3.4 Variants per Indicator {#variants}

| Method | Purpose |
|--------|---------|
| `core.SMA_Lookback(..) -> usize` | inputs consumed before the first output |
| `core.SMA(..) -> RetCode` | guarded: validates parameters, then computes |

## 4.0 Advanced Features {#advanced}

### 4.1 Abstraction Layer {#abstract}

`ta_lib::abstract_api` describes every function at run time and calls it without naming it at compile time — the Rust equivalent of C's [abstraction layer](/api/#abstract). Useful for a UI, a scripting bridge, or anything that enumerates indicators.

```rust
use ta_lib::abstract_api::{for_each_func, get_func_handle};

// Look one up by name, or walk all 168.
let id = get_func_handle("SMA").expect("unknown function");
let info = id.info();

info.name;        // "SMA"
info.group;       // Group::OverlapStudies
info.hint;        // one-line description
info.inputs;      // &[InputInfo]     -- param_name, kind, flags
info.opt_inputs;  // &[OptInputInfo]  -- display_name, hint, kind
info.outputs;     // &[OutputInfo]    -- param_name, kind, flags

for_each_func(|f| println!("{} ({:?})", f.name, f.group));
```

Each optional parameter carries a typed `OptInputType` — `RealRange`, `IntegerRange`, `RealList` or `IntegerList` — with its bounds, default and suggested values, so a UI can build the right control without a lookup table of its own. It replaces C's type tag plus `void* dataSet` — two separate fields there, with the cast done by hand.

Binding arguments at run time goes through a `ParamHolder`:

```rust
let core = Core::new();
let mut out = vec![0.0; close.len()];

let mut call = id.new_call(&core);
call.set_input(0, &close)?;          // set_price_input / set_int_input also exist
call.set_opt(0, 30)?;                // takes i32 or f64
call.set_output(0, &mut out)?;

let range = call.call(0, close.len() - 1)?;
println!("{} values from bar {}", range.nb_element, range.beg_idx);
```

Optional parameters left unset carry the same default sentinel an omitted argument does in C, so "unset" and "explicitly the default" are one code path. `FuncId::COUNT` is the registry size, and `MAX_INPUTS` / `MAX_OPT_INPUTS` / `MAX_OUTPUTS` bound the slots.

### 4.2 Numerical Stability {#numerical_stability}

Some indicators are recursive, so their earliest values depend on how much history precedes them. The [unstable period](/api/unstable-period/) setting controls how many of those warm-up bars are discarded. It lives on `Core` and is set through the builder:

```rust
use ta_lib::{Core, FuncUnstId};

let core = Core::builder()
    .unstable_period(FuncUnstId::EMA, 10)
    .build()?;
```

The setters are infallible so that they chain; `build()` reports a rejected
argument once, as `RetCode::BadParam`.

### 4.3 Candlestick Settings {#candle_settings}

The `CDL*` pattern functions judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults; the builder sets them the same way:

```rust
use ta_lib::{CandleSetting, CandleSettingType, Core};

let core = Core::builder()
    .candle_setting(
        CandleSettingType::BodyLong,
        CandleSetting { range_type: 0, avg_period: 10, factor: 1.2 },
    )
    .build()?;
```

### 4.4 Input Type {#input_type}

`f64` only. C also ships `TA_S_*` variants taking `float` inputs; the crate has no equivalent and needs none, since those exist to spare a conversion pass in C code that already stores prices as `float`.

### 4.5 Threading {#multithreading}

A `Core` cannot change after `build()`, so it is `Send + Sync`. Share one read-only `Core` across threads (behind an `Arc`, say) and call indicators concurrently — no locking, and none of C's "configure once from a single thread, before going parallel" sequencing. To change a setting, build another `Core`, or derive one from an existing value with `to_builder()`.

## 5.0 Documentation {#docs}

Every function carries rustdoc rendered from its canonical description, including a runnable doctest. Browse it with `cargo doc --open`, or on [docs.rs](https://docs.rs/ta-lib).
