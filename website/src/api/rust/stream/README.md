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
| `core.<name>_open(history, params)` | once | validate params, consume warm-up history, return `(stream, value)` |
| `stream.update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `stream.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |
| `stream.out_range()` | any time | the bars this stream has a value for — the batch range over the same bars |

Two more calls, `open_and_fill` and `update_and_fill`, write array output instead of a single value — see [Array-Fill Calls](#array-fill-calls) below.

There is no `close` — dropping the stream closes it (RAII).

## Example (SMA)

```rust
use ta_lib::Core;

let core = Core::new();

// Seed with warm-up history (>= SMA_Lookback(period) + 1 bars).
let history: Vec<f64> = /* ...your closing prices... */;
let (mut s, last) = core.sma_open(&history, 30)?;   // stream + value at the last history bar

// Each time a bar closes:
let v = s.update(new_close)?;                        // Err only for a non-finite bar

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
let provisional = s.peek(forming_close)?;            // state left unchanged

// dropping `s` closes the stream
```

`open` returns a `Result` — `Err(RetCode::InsufficientHistory)` if there is too little history (another bar might fix it, so this is the one worth retrying), `Err(RetCode::BadParam)` if a parameter is out of range. `update` and `peek` return a `Result` too, and after a successful `open` the only thing they reject is invalid input such as NaN or ±Inf. The handle is left untouched on an error.

## Rules

- **Warm-up.** `open` succeeds only if `history.len() >= <NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. After `open`, the history can be dropped — the stream keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing — call it as often as the forming bar ticks.
- **Parameters are fixed at `open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and [candle settings](/api/#candle_settings) are captured from the immutable `Core` at `open` and cannot change during the stream's life.
- **Threads.** `update(&mut self)` makes the single-writer rule a **compile-time** guarantee — one exclusive writer per stream. `peek(&self)` never writes the handle, so peeks may run concurrently. Streams are `Send + Sync + Clone`; **cloning forks an independent stream**.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. Multi-output functions return a tuple in batch output order; candlestick patterns return `i32`:

```rust
// MACD: one input, three outputs
let (mut s, (macd, signal, hist)) = core.macd_open(&history, 12, 26, 9)?;
let (macd, signal, hist) = s.update(new_close)?;

// A candlestick pattern returns i32
let (mut s, _) = core.cdldoji_open(&open, &high, &low, &close)?;
let pattern: i32 = s.update(o, h, l, c)?;
```

## Array-Fill Calls

`open` and `update` each write a single value. Two more calls write a full slice instead — the same shape the [batch method](/api/rust/) would produce — while still driving the stream:

| Call | When | Does |
|------|------|------|
| `core.<name>_open_and_fill(..)` | once, instead of `open` | like `open`, but also fills the output for **every** history bar, returning `(stream, OutRange)` |
| `stream.update_and_fill(bars, outs)` | instead of a loop of `update` | commit `n` closed bars and write the `n` values |

**`open_and_fill`**

```rust
let mut warmup = vec![0.0; history.len()];

let (mut s, filled) = core.sma_open_and_fill(&history, 30, &mut warmup)?;

// warmup[..filled.count] is the SMA over all of history; then stream on:
let v = s.update(new_close)?;
```

`open_and_fill` takes the [batch method](/api/rust/)'s optional parameters and one slice per output, and returns the range it wrote as the same `OutRange` the batch method returns, beside the live stream. The output slices must not alias the input or each other.

**`update_and_fill`**

```rust
let mut out = vec![0.0; gap.len()];

s.update_and_fill(&gap, &mut out)?;    // out[i] is the SMA at gap[i]
```

`s.out_range()` reports the bars
the handle has a value for, before and after; there is no second return value
for it.

`Err(RetCode::BadParam)` before anything is committed if the input slices
differ in length or an output is shorter than the bar count; a zero bar count
is a successful no-op. An invalid bar (NaN or ±Inf) also returns
`Err(RetCode::BadParam)`, exactly as `update` does, but commits the valid bars
**before** it — their values are already written, and `s.out_range()` tells you
how many.

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
