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
<a href="#retcode">3.3 Results and Return Codes</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced Features</a></p>

<blockquote>
<p><a href="#abstract">4.1 Abstraction Layer</a><br>
<a href="#numerical_stability">4.2 Numerical Stability</a><br>
<a href="#candle_settings">4.3 Candlestick Settings</a><br>
<a href="#multithreading">4.4 Threading</a><br></p>
</blockquote>

<p><a href="#docs">5.0 Documentation</a></p>

## 1.0 Introduction {#intro}

The `ta-lib` crate is a native Rust port of TA-Lib — no C bindings, no `unsafe` at the call site. Every indicator is a method on `Core`, operates on `f64` slices, and is **bit-identical** to the reference C library over the same inputs.

The **Core API** provides:

- The [`Core`](#direct_call) type and the builder that configures it.
- The settings each `Core` carries: [unstable period](/api/unstable-period/) and [candlestick settings](/api/candle-settings/). Multiple `Core` instances can safely co-exist (say for different settings).
- Every TA function, each processing a whole array of data at once.
- An optional [abstraction layer](#abstract) for calling those functions dynamically.

To process a live feed one bar at a time instead of a whole array, see the companion [Rust Streaming API](/api/rust/stream/).

There is no initialization step and nothing to shut down. Where C requires `TA_Initialize` before any call and `TA_Shutdown` at exit, Rust has `Core::new()`, and the `Core` is dropped like any other value.

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
       <span class="ta-arg-opt">optInMAType: MAType,</span>
       <span class="ta-arg-out">outReal: &amp;mut [f64]</span> ) -&gt; Result&lt;OutRange, RetCode&gt;
</pre>

All TA functions use the same calling pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input slices.</span></li>
<li><span class="ta-arg-in">One or more input slices are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there are two optional inputs. These parameters give finer control specific to each function. If you do not care about a particular optIn, just pass Core::INTEGER_DEFAULT, Core::REAL_DEFAULT or MAType::DEFAULT (depending on the type), and the function substitutes its documented default.</span></li>
<li><span class="ta-arg-out">One or more output slices come last. In this example there is only one output (outReal). Where the values landed is the return value, not a parameter: on success you get an <code>OutRange</code>.</span></li>
</ul>

This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.

For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:

<pre>use ta_lib::{Core, MAType, RetCode};

let core = Core::new();

// ...initialize your closing prices here...
let close: Vec&lt;f64&gt; = vec![0.0; 400];
let mut out = vec![0.0; 400];

let range = core.MA( <span class="ta-arg-range">0</span>, <span class="ta-arg-range">399</span>,
                     <span class="ta-arg-in">&amp;close</span>,
                     <span class="ta-arg-opt">30</span>, <span class="ta-arg-opt">MAType::SMA</span>,
                     <span class="ta-arg-out">&amp;mut out</span> )?;

// The output is displayed here
for i in 0..range.count {
    println!("Day {} = {}", range.beg_idx + i, out[i]);
}
</pre>

After the call, read `range` to learn what was produced. Even though we requested the whole range (0 to 399), a 30-day average is not defined until the 30th day. Consequently, `range.beg_idx` will be 29 (zero-based) and `range.count` will be 400-29 = 371. In other words, only the first 371 elements of `out` are written, and they correspond to input elements 29 through 399.

As another example, if you had requested only the range 125 to 225, `range.beg_idx` would be 125 and `range.count` would be 101 (`endIdx` is inclusive: 225-125+1). The 30-day minimum is not a problem here, because the 125 closing prices before the requested range provide the needed history. As you may have guessed, only the first 101 elements of `out` are written; the rest is left untouched.

Here is another example. This time we calculate a 14-bar exponential moving average for a single price bar (say, the last one, at index 299):

<pre>let range = core.MA( <span class="ta-arg-range">299</span>, <span class="ta-arg-range">299</span>,
                     <span class="ta-arg-in">&amp;close</span>,
                     <span class="ta-arg-opt">14</span>, <span class="ta-arg-opt">MAType::EMA</span>,
                     <span class="ta-arg-out">&amp;mut out</span> )?;
</pre>

In this example, `range.beg_idx` will be 299, `range.count` will be 1, and only one value is written into `out`.

If you do not provide enough data to calculate even one value, the call still succeeds and `range.count` is 0 — `range.is_empty()` says so directly.

The input and the output are separate borrows, so a function cannot read and write the same buffer: pass a distinct output slice, and copy it back afterwards if you want the result in place.

`Core` is cheap to create and holds only the library's settings; construct one and reuse it.

### 3.2 Output Size and Lookback {#output_size}

It is important that the output slice is large enough — an undersized slice is `Err(RetCode::BadParam)`, never a write past the end. Here are three ways to determine the allocation size; all of them work for every TA function:

| Method | Description |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Input Matching   | `allocation_size = endIdx + 1;` <br> **Pros**: Easy to understand and implement. <br> **Cons**: Memory allocation unnecessarily large when requesting a small range. |
| Range Matching   | `allocation_size = endIdx - startIdx + 1;` <br> **Pros**: Easy to implement. <br> **Cons**: Allocation slightly larger than needed. Example: with startIdx = 0, a 30-period SMA wastes 29 elements because of the lookback. |
| Exact Allocation | Derived from the function's lookback — see the example below. <br> **Pros**: Allocates exactly what is needed. <br> **Cons**: Slightly more complex, and the only one that has to handle an out-of-range parameter. |

Each TA function has a matching `<NAME>_Lookback` method, taking the same optional parameters as the function itself. Example: for `SMA` it is `SMA_Lookback`.

The lookback is the number of input elements consumed before the first output can be calculated. Example: a simple moving average (SMA) of period 10 has a lookback of 9.

```rust
let lookback = core.SMA_Lookback(30)?;   // 29 for a 30-period SMA
```

A lookback method returns `Result<usize, RetCode>`, carrying `RetCode::BadParam` when a parameter is out of range — the same code the function itself would answer for it.

Putting it together, the exact allocation for any TA function:

```rust
let lookback = core.<NAME>_Lookback(..)?;

let temp = lookback.max(startIdx);
let allocation_size = if temp > endIdx { 0 } else { endIdx - temp + 1 };
let mut out = vec![0.0; allocation_size];
```

Too little data is a success, not an error: a range shorter than the lookback simply produces no values, and the returned range is empty (`count == 0`).

### 3.3 Results and Return Codes {#retcode}

Every TA function returns `Result<OutRange, RetCode>`, so it composes with `?`. `RetCode` implements `std::error::Error`.

On success you get an [`OutRange`](https://docs.rs/ta-lib): `beg_idx` is the input index of the first value written, and `count` is how many were written.

| Code | Meaning |
|------|---------|
| `RetCode::BadParam` | An optional parameter is outside its documented range, or a slice is too short: every input must cover `startIdx..=endIdx`, and every output must hold the number of values produced for that range. |
| `RetCode::OutOfRangeStartIndex` | `startIdx` is above `Core::MAX_INDEX` (100,000,000). |
| `RetCode::OutOfRangeEndIndex` | `endIdx` is above `Core::MAX_INDEX`, or below `startIdx`. |

`RetCode` also carries `Success` — the code C returns and the one the other ports expose — plus `AllocErr` and `InternalError`, which the safe Rust code paths do not produce.

Indexing is safe throughout: the crate is `#![forbid(unsafe_code)]`, so nothing here can read or write out of bounds. Slice sizes are checked before the call runs and reported as `BadParam`; a violated precondition anywhere below that is a panic, never memory corruption.

## 4.0 Advanced Features {#advanced}

### 4.1 Abstraction Layer {#abstract}

`ta_lib::abstract_api` describes every function at run time and calls it without naming it at compile time — the Rust equivalent of C's [abstraction layer](/api/#abstract). Useful for a UI, a scripting bridge, or anything that enumerates indicators.

```rust
use ta_lib::abstract_api::{for_each_func, get_func_handle};

// Look one up by name, or walk them all (FuncId::COUNT of them).
// The name is matched under an ASCII case fold, so "SMA", "sma" and "Sma"
// all resolve; `info.name` is still the canonical "SMA".
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
println!("{} values from bar {}", range.count, range.beg_idx);
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
use ta_lib::{CandleSetting, CandleSettingType, Core, RangeType};

let core = Core::builder()
    .candle_setting(
        CandleSettingType::BodyLong,
        CandleSetting { range_type: RangeType::RealBody, avg_period: 10, factor: 1.2 },
    )
    .build()?;
```

### 4.4 Threading {#multithreading}

A `Core` cannot change after `build()`, so it is `Send + Sync`. Share one read-only `Core` across threads (behind an `Arc`, say) and call indicators concurrently — no locking, and no setup ordering to respect.

To change a setting, build another `Core`, or derive one from an existing `Core` with `to_builder()`.

## 5.0 Documentation {#docs}

Every function carries rustdoc rendered from its canonical description, including a runnable doctest. Browse it with `cargo doc --open`, or on [docs.rs](https://docs.rs/ta-lib).
