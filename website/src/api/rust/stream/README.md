---
title: Rust Streaming API
toc: false
---

# Rust Streaming API

::: warning Not yet released
The Rust API is not yet released. Estimated release: **Q1 2027**.
:::

The [batch methods](/api/rust/) (`core.sma`, `core.rsi`, …) recompute over a whole slice. The **streaming API** returns a small owned handle that carries state between bars, so each new bar costs O(1) — and every value is **bit-identical** to the batch method over the same series. Ideal for live feeds.

Each streamable function adds two constructors on `Core` and a handful of methods on its handle:

| Call | When | Does |
|------|------|------|
| `core.<name>_open(history, params)` | once | validate params, consume warm-up history, return `(handle, value)` |
| `core.<name>_open_and_fill(..)` | once, instead of `open` | like `open`, but also fills the output for **every** history bar — see [below](#full-history-output-open-and-fill) |
| `handle.update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `handle.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |

There is no `close` — dropping the handle closes the stream (RAII).

## Example (SMA)

```rust
use ta_lib::Core;

let core = Core::new();

// Seed with warm-up history (>= sma_lookback(period) + 1 bars).
let history: Vec<f64> = /* ...your closing prices... */;
let (mut s, last) = core.sma_open(&history, 30)?;   // handle + value at the last history bar

// Each time a bar closes:
let v = s.update(new_close);                         // always a value; never allocates

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
let provisional = s.peek(forming_close);             // state left unchanged

// dropping `s` closes the stream
```

`open` returns a `Result` — `Err(RetCode::BadParam)` if a parameter is out of range or there is too little history. After a successful `open`, `update` and `peek` are **infallible** (they return the value directly) and `update` never allocates.

## Rules

- **Warm-up.** `open` succeeds only if `history.len() >= <name>_lookback(params) + 1` — with fewer bars there is no defined value yet. After `open`, the history can be dropped — the handle keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing; it runs the same transition on a throwaway clone (which allocates for windowed indicators — `update` is the allocation-free path).
- **Parameters are fixed at `open`.** Changing a parameter means a new stream. [Unstable period](/api/#unstable_period), compatibility, and candle settings are captured from the immutable `Core` at `open` and cannot change during the stream's life.
- **Threads.** `update(&mut self)` makes the single-writer rule a **compile-time** guarantee — one exclusive writer per handle. Handles are `Send + Sync + Clone`; **cloning forks an independent stream**.
- **Don't persist** a handle across library versions.

## Full-history output (`open_and_fill`)

`open` gives you only the value at the last history bar. `open_and_fill` also writes the output for **every** history bar — the same values the [batch method](/api/rust/) would produce — while still returning the live handle, in one pass:

```rust
let mut beg = 0usize;
let mut nb = 0usize;
let mut warmup = vec![0.0; history.len()];

let mut s = core.sma_open_and_fill(&history, 30, &mut beg, &mut nb, &mut warmup)?;

// warmup[0..nb] is the SMA over all of history; then stream on:
let v = s.update(new_close);
```

The optional parameters and outputs (`outBegIdx`, `outNBElement`, one slice per output) are exactly the [batch method](/api/rust/)'s; the output slices must not alias the input or each other.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. Multi-output functions return a tuple in batch output order; candlestick patterns return `i32`:

```rust
// MACD: one input, three outputs
let (mut s, (macd, signal, hist)) = core.macd_open(&history, 12, 26, 9)?;
let (macd, signal, hist) = s.update(new_close);

// A candlestick pattern returns i32
let (mut s, _) = core.cdldoji_open(&open, &high, &low, &close)?;
let pattern: i32 = s.update(o, h, l, c);
```

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
