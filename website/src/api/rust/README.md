---
title: Rust Core API
toc: false
---

# Rust Core API

::: warning Not yet released
The Rust API is not yet released. Estimated release: **Q1 2027**.
:::

The `ta-lib` crate is a native Rust port of TA-Lib — no C bindings, no `unsafe` at the call site. Every indicator is a method on a `Core` value, operates on `f64` slices, and is **bit-identical** to the C library over the same inputs.

To process a live feed one bar at a time instead of a whole array, see the companion [Rust Streaming API](/api/rust/stream/).

## Add it to your project

```toml
[dependencies]
ta-lib = "0.8"
```

## Calling a function

Each indicator takes a `startIdx`/`endIdx` range into the input slice, the inputs, the optional parameters, and `&mut` output(s). It writes into a caller-provided output slice and reports where the valid output begins:

```rust
use ta_lib::{Core, RetCode};

let core = Core::new();

let close: Vec<f64> = /* ...your closing prices... */;
let mut out = vec![0.0; close.len()];
let mut beg = 0usize;
let mut nb = 0usize;

let rc = core.sma(
    0, close.len() - 1,   // startIdx, endIdx
    &close,               // input(s)
    30,                   // optInTimePeriod
    &mut beg, &mut nb,    // where valid output starts, how many are valid
    &mut out,             // output(s)
);
assert_eq!(rc, RetCode::Success);

// out[0..nb] holds the SMA; out[i] corresponds to input bar beg + i.
for i in 0..nb {
    println!("bar {} = {}", beg + i, out[i]);
}
```

`Core` is cheap to create and holds only the library's settings; construct one and reuse it.

## Output size and lookback

An output is written only where the indicator is defined — a 30-period SMA has no value until the 30th bar. `beg` (`outBegIdx`) is the first valid bar and `nb` (`outNBElement`) is the count written; the rest of the slice is left untouched. Size the output slice to at least `endIdx - startIdx + 1`, or exactly with the lookback:

```rust
let lookback = core.sma_lookback(30);   // 29 for a 30-period SMA
```

The lookback is how many inputs are consumed before the first output.

## Variants per indicator

| Method | Purpose |
|--------|---------|
| `core.sma_lookback(..) -> usize` | first valid output index |
| `core.sma(..) -> RetCode` | guarded: validates parameters, then computes |
| `core.sma_unguarded(..) -> RetCode` | no range checks — used internally for cross-indicator calls |

The public API returns [`RetCode`](https://docs.rs/ta-lib) (`Success`, `BadParam`, `OutOfRangeStartIndex`, `OutOfRangeEndIndex`, `AllocErr`, `InternalError`); it also implements `std::error::Error`, so results compose with `?`.

Enum parameters (e.g. an `MAType`) and integer parameters are `i32`; real parameters are `f64`.

## Settings

Library settings — [unstable period](/api/#unstable_period), compatibility, and candle settings — live on `Core` and are **immutable after construction**. Set them through the builder:

```rust
use ta_lib::{Core, Compatibility, FuncUnstId};

let core = Core::builder()
    .compatibility(Compatibility::Metastock)
    .unstable_period(FuncUnstId::Ema, 10)
    .build();
```

Because a `Core` cannot change after `build()`, it is `Send + Sync` — share one read-only `Core` across threads (e.g. behind an `Arc`) and call indicators concurrently. To change a setting, build a new `Core`.

## Documentation

Every function carries rustdoc rendered from its canonical description, including a runnable doctest. Browse it with `cargo doc --open`, or on [docs.rs](https://docs.rs/ta-lib).
