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
<p><a href="#direct_call">3.1 Calling a function</a><br>
<a href="#output_size">3.2 Output size and lookback</a><br>
<a href="#retcode">3.3 Return codes</a><br>
<a href="#variants">3.4 Variants per indicator</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced features</a></p>

<blockquote>
<p><a href="#abstract">4.1 Abstraction layer</a><br>
<a href="#numerical_stability">4.2 Numerical stability</a><br>
<a href="#candle_settings">4.3 Candlestick settings</a><br>
<a href="#input_type">4.4 Input type</a><br>
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

### 3.1 Calling a function {#direct_call}

Every function follows the same pattern: it reads its inputs from slices you pass in and writes results into slices you allocate. It never writes more elements than you request, so the output only needs to cover the `startIdx`-to-`endIdx` range.

This is the **same calling pattern as the C library**, argument for argument and name for name — only the types change. Walking through `sma`, a simple moving average:

<pre>fn sma( &amp;self,
        <span class="ta-arg-range">startIdx: usize,</span>
        <span class="ta-arg-range">endIdx: usize,</span>
        <span class="ta-arg-in">inReal: &amp;[f64],</span>
        <span class="ta-arg-opt">optInTimePeriod: i32,</span>
        <span class="ta-arg-out">outBegIdx: &amp;mut usize,</span>
        <span class="ta-arg-out">outNBElement: &amp;mut usize,</span>
        <span class="ta-arg-out">outReal: &amp;mut [f64]</span> ) -&gt; RetCode
</pre>

All TA functions use this pattern, divided into four groups:

<ul>
<li><span class="ta-arg-range">The output is calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input slices.</span></li>
<li><span class="ta-arg-in">One or more input slices are then specified. Typically these are the "price" data. In this example there is only one input. All input parameter names start with "in".</span></li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. These give finer control specific to each function. Integer and enum parameters are i32, real parameters are f64.</span></li>
<li><span class="ta-arg-out">One or more output slices come last. The parameters outBegIdx and outNBElement always come just before the output slices.</span></li>
</ul>

```rust
use ta_lib::{Core, RetCode};

let core = Core::new();

let close: Vec<f64> = /* ...your closing prices... */;
let mut out = vec![0.0; close.len()];
let mut beg = 0usize;
let mut nb  = 0usize;
```

<pre>let rc = core.sma( <span class="ta-arg-range">0</span>, <span class="ta-arg-range">close.len() - 1</span>,
                   <span class="ta-arg-in">&amp;close</span>,
                   <span class="ta-arg-opt">30</span>,
                   <span class="ta-arg-out">&amp;mut beg</span>, <span class="ta-arg-out">&amp;mut nb</span>, <span class="ta-arg-out">&amp;mut out</span> );
</pre>

```rust
assert_eq!(rc, RetCode::Success);

// out[0..nb] holds the SMA; out[i] corresponds to input bar beg + i.
for i in 0..nb {
    println!("bar {} = {}", beg + i, out[i]);
}
```

`Core` is cheap to create and holds only the library's settings; construct one and reuse it.

### 3.2 Output size and lookback {#output_size}

An output is written only where the indicator is defined — a 30-period SMA has no value until the 30th bar. `beg` (`outBegIdx`) is the first valid bar and `nb` (`outNBElement`) is the count written; the rest of the slice is left untouched. Size the output slice to at least `endIdx - startIdx + 1`, or exactly with the lookback:

```rust
let lookback = core.sma_lookback(30);   // 29 for a 30-period SMA
```

The lookback is how many inputs are consumed before the first output.

Too little data is a success, not an error: a range shorter than the lookback simply produces no values and `nb` is `0`. When `nb` is `0`, ignore `beg`.

### 3.3 Return codes {#retcode}

The public API returns [`RetCode`](https://docs.rs/ta-lib) (`Success`, `BadParam`, `OutOfRangeStartIndex`, `OutOfRangeEndIndex`, `AllocErr`, `InternalError`); it also implements `std::error::Error`, so results compose with `?`.

Indexing is safe throughout: the crate is `#![forbid(unsafe_code)]`, so a violated bounds precondition panics rather than reading out of bounds. A call that returns `Success` with zero elements cannot panic.

### 3.4 Variants per indicator {#variants}

| Method | Purpose |
|--------|---------|
| `core.sma_lookback(..) -> usize` | first valid output index |
| `core.sma(..) -> RetCode` | guarded: validates parameters, then computes |

## 4.0 Advanced features {#advanced}

### 4.1 Abstraction layer {#abstract}

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
info.opt_inputs;  // &[OptInputInfo]  -- display_name, hint, domain
info.outputs;     // &[OutputInfo]    -- param_name, kind, flags

for_each_func(|f| println!("{} ({:?})", f.name, f.group));
```

Each optional parameter carries a typed `OptDomain` — `RealRange`, `IntegerRange`, `RealList` or `IntegerList` — with its bounds, default and suggested values, so a UI can build the right control without a lookup table of its own. It replaces C's `void* dataSet` plus type tag.

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

### 4.2 Numerical stability {#numerical_stability}

Some indicators are recursive, so their earliest values depend on how much history precedes them. The [unstable period](/api/unstable-period/) setting controls how many of those warm-up bars are discarded. It lives on `Core` and is set through the builder:

```rust
use ta_lib::{Core, FuncUnstId};

let core = Core::builder()
    .unstable_period(FuncUnstId::Ema, 10)
    .build();
```

### 4.3 Candlestick settings {#candle_settings}

The `CDL*` pattern functions judge each candle against tunable thresholds. See [candlestick settings](/api/candle-settings/) for the full list and defaults; the builder sets them the same way:

```rust
use ta_lib::{CandleSetting, CandleSettingType, Core};

let core = Core::builder()
    .candle_setting(
        CandleSettingType::BodyLong,
        CandleSetting { range_type: 0, avg_period: 10, factor: 1.2 },
    )
    .build();
```

### 4.4 Input type {#input_type}

`f64` only. C also ships `TA_S_*` variants taking `float` inputs; the crate has no equivalent and needs none, since those exist to spare a conversion pass in C code that already stores prices as `float`.

### 4.5 Threading {#multithreading}

A `Core` cannot change after `build()`, so it is `Send + Sync`. Share one read-only `Core` across threads (behind an `Arc`, say) and call indicators concurrently — no locking, and none of C's "configure once from a single thread, before going parallel" sequencing. To change a setting, build another `Core`, or derive one from an existing value with `to_builder()`.

## 5.0 Documentation {#docs}

Every function carries rustdoc rendered from its canonical description, including a runnable doctest. Browse it with `cargo doc --open`, or on [docs.rs](https://docs.rs/ta-lib).
