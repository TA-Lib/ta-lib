---
title: Rust Streaming API
description: "Rust streaming API for live feeds: a stream handle carries indicator state from bar to bar at O(1) per update, bit-identical to the batch calls."
toc: false
---

::: warning Not yet released
The Rust API is not yet released. Estimated release: **Q1 2027**.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch method](/api/rust/) (`core.SMA`, `core.RSI`, …) would return by recomputing over the whole slice.

Each streamable function adds two constructors on `Core` and a handful of methods on its stream:

| Call | When | Does |
|------|------|------|
| `core.<NAME>_Open(history, params)` | once | validate params, consume warm-up history, return `(stream, value)` |
| `core.<NAME>_OpenAndFill(..)` | once, instead of `Open` | like `Open`, but also fills the output for **every** history bar, returning `(stream, OutRange)` — see [below](#full-history-output-openandfill) |
| `stream.update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `stream.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |

There is no `Close` — dropping the stream closes it (RAII).

## Example (SMA)

```rust
use ta_lib::Core;

let core = Core::new();

// Seed with warm-up history (>= SMA_Lookback(period) + 1 bars).
let history: Vec<f64> = /* ...your closing prices... */;
let (mut s, last) = core.SMA_Open(&history, 30)?;   // stream + value at the last history bar

// Each time a bar closes:
let v = s.update(new_close)?;                        // Err only for a non-finite bar

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
let provisional = s.peek(forming_close)?;            // state left unchanged

// dropping `s` closes the stream
```

`Open` returns a `Result` — `Err(RetCode::InsufficientHistory)` if there is too little history (another bar fixes it, so this is the one worth retrying), `Err(RetCode::BadParam)` if a parameter is out of range. `update` and `peek` return a `Result` too, and after a successful `Open` the only thing they reject is a **non-finite bar**, leaving the handle exactly as it was.

One narrow exception to "the handle is unchanged": a *composed* indicator drives its sub-stages through their own public update, so a value the library computed internally is re-checked there. If such an intermediate overflowed to an infinity, the rejection would surface after earlier sub-stages had advanced, and would name the sub-stage. It needs input magnitudes around 1e306 and up — the overflow class TA-Lib already treats as out of scope — but the guarantee is stated for the caller-supplied case, which is the one you can provoke. `update` never allocates.

**Non-finite input is rejected.** NaN and ±Inf are not supported as inputs anywhere in TA-Lib, but the streaming tier is the one that *enforces* it: every public streaming entry point checks, and rejects without touching the handle. The batch API does not filter — it computes on whatever it is given.

The difference is the retained state. Batch computes and forgets, so a NaN reaches the outputs depending on that bar and no others; a stream handle carries state forward, so a single non-finite bar would poison every value it produces afterwards, long after the feed recovers. Rejecting the bar and leaving the handle usable is more useful than accepting it and going permanently NaN.

This covers every bar value at update/peek, and a real optional parameter that is NaN — which a plain range check lets through, since `x < min` and `x > max` are both false for NaN.

It does **not** cover the warm-up history, or any other input **array**. Arrays are never scanned: keeping one free of NaN and infinities is the caller's responsibility. Passing a non-finite one is **undefined behaviour** — nothing is promised.


## Rules

- **Warm-up.** `Open` succeeds only if `history.len() >= <NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. After `Open`, the history can be dropped — the stream keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing; it runs the same transition on a copy. It takes `&self` and never writes the handle, so peeks may run concurrently. Where copying the handle means several allocations, the copy is held per thread and reused — only the first peek of that indicator on that thread allocates. That scratch lives as long as the thread: one handle copy per indicator a thread has peeked, holding its `Core` and buffers, which dropping your own handles does not release.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and candle settings are captured from the immutable `Core` at `Open` and cannot change during the stream's life.
- **Threads.** `update(&mut self)` makes the single-writer rule a **compile-time** guarantee — one exclusive writer per stream. Streams are `Send + Sync + Clone`; **cloning forks an independent stream**.
- **Don't persist** a stream across library versions.

## Full-history output (`OpenAndFill`)

`Open` gives you only the value at the last history bar. `OpenAndFill` also writes the output for **every** history bar — the same values the [batch method](/api/rust/) would produce — while still returning the live stream, in one pass:

```rust
let mut warmup = vec![0.0; history.len()];

let (mut s, filled) = core.SMA_OpenAndFill(&history, 30, &mut warmup)?;

// warmup[..filled.count] is the SMA over all of history; then stream on:
let v = s.update(new_close)?;
```

`OpenAndFill` takes the [batch method](/api/rust/)'s optional parameters and one slice per output, and returns the range it wrote as the same `OutRange` the batch method returns, beside the live stream. The output slices must not alias the input or each other.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. Multi-output functions return a tuple in batch output order; candlestick patterns return `i32`:

```rust
// MACD: one input, three outputs
let (mut s, (macd, signal, hist)) = core.MACD_Open(&history, 12, 26, 9)?;
let (macd, signal, hist) = s.update(new_close)?;

// A candlestick pattern returns i32
let (mut s, _) = core.CDLDOJI_Open(&open, &high, &low, &close)?;
let pattern: i32 = s.update(o, h, l, c)?;
```

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
