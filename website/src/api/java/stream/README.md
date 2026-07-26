---
title: Java Streaming API
toc: false
---

# Java Streaming API

::: warning Not yet released
The Java API is not yet released. Estimated release: **Q1 2027**.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch method](/api/java/) (`core.sma`, `core.rsi`, …) would return by recomputing over the whole array.

Each streamable function adds two factory methods on `Core` and a handful of methods on its stream (a class nested in `Core`, e.g. `Core.SmaStream` — unrelated to `java.util.stream`):

| Call | When | Does |
|------|------|------|
| `core.<name>Open(history, params)` | once | validate params, consume warm-up history, return a **stream** |
| `core.<name>OpenAndFill(..)` | once, instead of `Open` | like `Open`, but also fills the output for **every** history bar — see [below](#full-history-output-openandfill) |
| `stream.update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `stream.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |
| `stream.value()` | any time | the most recently committed value |
| `stream.copy()` | any time | an independent copy of the stream |

There is no `close` — a stream is ordinary heap state, so an unreferenced stream is simply garbage-collected.

## Example (SMA)

```java
import io.github.talib.Core;

Core core = Core.DEFAULT;

// Seed with warm-up history (>= smaLookback(period) + 1 bars).
double[] history = /* ...your closing prices... */;
Core.SmaStream s = core.smaOpen(history, 30);   // value() starts at the last history bar

// Each time a bar closes:
double v = s.update(newClose);                  // always a value; never throws after open

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
double provisional = s.peek(formingClose);      // state left unchanged
```

`Open` returns the stream directly; its `value()` starts at the last history bar's value. After a successful `Open`, `update` and `peek` never throw.

## Rules

- **Warm-up.** `Open` succeeds only if `history.length >= <name>Lookback(params) + 1` — with fewer bars there is no defined value yet. Too little history throws `InsufficientHistoryException` (see [Error model](#error-model)). After `Open`, the history can be discarded — the stream keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing; it runs the same code on a throwaway deep copy (which allocates for windowed indicators — `update` is the cheaper path). `value()` re-reads the last committed value without recomputing.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and candle settings are read from the owning `Core` at `Open`. Since `Core` is immutable they cannot change underneath a live stream — to stream with different settings, build a new `Core` and open from that.
- **Threads.** A stream is single-writer — `update`, `peek`, `value()`, and `copy()` must not race with an `update` on the same stream. With no concurrent `update`, `peek`/`value()`/`copy()` are read-only and safe to call concurrently after safe publication. Distinct streams (including `copy()` results) are fully independent.
- **Not serializable.** To checkpoint, retain the history and re-open — the result is bit-identical by contract.

## Full-history output (`OpenAndFill`)

`Open` gives you only the value at the last history bar. `OpenAndFill` also writes the output for **every** history bar — the same values the [batch method](/api/java/) would produce — while still returning the live stream, in one pass:

```java
import io.github.talib.OutRange;

double[] warmup = new double[history.length];

Core.SmaStream s = core.smaOpenAndFill(history, 30, warmup);
OutRange r = s.fillRange();                     // what was written, on the handle

// warmup[0 .. r.count() - 1] is the SMA over all of history; then stream on:
double v = s.update(newClose);
```

The optional parameters and output arrays are exactly the [batch method](/api/java/)'s. The filled range is reported on the returned handle as `fillRange()` (`null` after a plain `Open`, which fills nothing) rather than through out-parameters. The output arrays must not alias the input or each other.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. Multi-output functions return a small immutable `Value` class with one final field per output, in batch output order; candlestick patterns return `int`:

```java
// MACD: one input, three outputs
Core.MacdStream m = core.macdOpen(history, 12, 26, 9);
Core.MacdStream.Value out = m.update(newClose);
// out.macd, out.macdSignal, out.macdHist

// A candlestick pattern returns int
Core.CdlDojiStream c = core.cdlDojiOpen(open, high, low, close);
int pattern = c.update(o, h, l, cl);
```

## Error model

| Call | Behaviour |
|------|-----------|
| `<name>Open` / `<name>OpenAndFill` | Too little history throws `InsufficientHistoryException` (a subclass of `IllegalArgumentException` — catch it to accumulate more bars and retry). Out-of-range parameters, or output arrays that alias the input or each other (`OpenAndFill`), throw plain `IllegalArgumentException`. |
| `update` / `peek` / `value` / `copy` | Never throw after a successful `Open`. |

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
