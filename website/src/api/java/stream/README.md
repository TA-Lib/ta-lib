---
title: Java Streaming API
description: "Java streaming API for live feeds: a stream handle carries indicator state from bar to bar at O(1) per update, bit-identical to the batch calls."
toc: false
---

::: warning Not yet released
The Java API is not yet released. Estimated release: **Q1 2027**.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch method](/api/java/) (`core.SMA`, `core.RSI`, …) would return by recomputing over the whole array.

Each streamable function adds two factory methods on `Core` and a handful of methods on its stream (a class nested in `Core`, e.g. `Core.SMA_Stream` — unrelated to `java.util.stream`):

| Call | When | Does |
|------|------|------|
| `core.<NAME>_Open(history, params)` | once | validate params, consume warm-up history, return a **stream** |
| `core.<NAME>_OpenAndFill(..)` | once, instead of `Open` | like `Open`, but also fills the output for **every** history bar — see [below](#full-history-output-openandfill) |
| `stream.update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `stream.updateAndFill(bars, outs)` | instead of a loop of `update` | commit `n` closed bars and write the `n` values — see [below](#catch-up-n-bars-at-once-updateandfill) |
| `stream.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |
| `stream.value()` | any time | the most recently committed value |
| `stream.copy()` | any time | an independent copy of the stream |
| `stream.outRange()` | any time | the bars this stream has a value for — the batch range over the same bars |

There is no `close` — a stream is ordinary heap state, so an unreferenced stream is simply garbage-collected.

## Example (SMA)

```java
import io.github.talib.Core;

Core core = Core.DEFAULT;

// Seed with warm-up history (>= SMA_Lookback(period) + 1 bars).
double[] history = /* ...your closing prices... */;
Core.SMA_Stream s = core.SMA_Open(history, 30); // value() starts at the last history bar

// Each time a bar closes:
double v = s.update(newClose);                  // throws only on a non-finite bar

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
double provisional = s.peek(formingClose);      // state left unchanged
```

`Open` returns the stream directly; its `value()` starts at the last history bar's value. After a successful `Open`, the only thing `update` and `peek` reject is a **non-finite bar**, and they leave the handle exactly as it was.

## Rules

- **Warm-up.** `Open` succeeds only if `history.length >= <NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. Too little history throws `InsufficientHistoryException` (see [Error model](#error-model)). After `Open`, the history can be discarded — the stream keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing; it runs the same code on a copy and never writes the handle. Where the handle owns several arrays or a sub-stream, that copy is a scratch held per thread and reused, so it allocates nothing after the first peek of that indicator on that thread. It is held in a `ThreadLocal` for the life of the thread — one handle copy per indicator that thread has peeked, keeping its `Core` and arrays reachable. On a pooled thread that outlives a deployment, that is the usual `ThreadLocal` retention to be aware of. `value()` re-reads the last committed value without recomputing.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and candle settings are read from the owning `Core` at `Open`. Since `Core` is immutable they cannot change underneath a live stream — to stream with different settings, build a new `Core` and open from that.
- **Threads.** A stream is single-writer — `update`, `peek`, `value()`, and `copy()` must not race with an `update` on the same stream. With no concurrent `update`, `peek`/`value()`/`copy()` are read-only and safe to call concurrently after safe publication. Distinct streams (including `copy()` results) are fully independent.
- **Not serializable.** To checkpoint, retain the history and re-open — the result is bit-identical by contract.

## Full-history output (`OpenAndFill`)

`Open` gives you only the value at the last history bar. `OpenAndFill` also writes the output for **every** history bar — the same values the [batch method](/api/java/) would produce — while still returning the live stream, in one pass:

```java
import io.github.talib.OutRange;

double[] warmup = new double[history.length];

Core.SMA_Stream s = core.SMA_OpenAndFill(history, 30, warmup);
OutRange r = s.outRange();                      // the bars it has a value for

// warmup[0 .. r.count() - 1] is the SMA over all of history; then stream on:
double v = s.update(newClose);
```

The optional parameters and output arrays are exactly the [batch method](/api/java/)'s. The range written is reported on the returned handle as `outRange()` rather than through out-parameters. That accessor is on every stream, not just a filled one: it holds the bars the handle has a value for, which is what the batch call over the same bars reports — `(lookback, historyLen - lookback)` at open, one more per `update`, unchanged by `peek`. The output arrays must not alias the input or each other.

## Catch up n bars at once (`updateAndFill`)

Feeding a gap one `update` at a time works; `updateAndFill` does the same thing
in one call, writing one value per bar into your array:

```java
double[] out = new double[gap.length];

s.updateAndFill(gap, out);          // out[i] is the SMA at gap[i]
```

It is exactly `gap.length` back-to-back `update` calls — same values, same state
— with one set of argument checks instead of `n`. `s.outRange()` reports the bars
the stream has a value for, before and after.

That includes a call that fails partway. A non-finite bar throws
`IllegalArgumentException` exactly as `update` does, which means the bars
**before** it are already committed and their values already written; the range
tells you how many. It throws before committing anything if the input arrays
differ in length, an output is shorter than the bar count, or an output is the
same array as an input or as another output. A zero-length call does nothing.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. Multi-output functions return a small immutable `Value` record with one component per output, in batch output order; candlestick patterns return `int`:

```java
// MACD: one input, three outputs
Core.MACD_Stream m = core.MACD_Open(history, 12, 26, 9);
Core.MACD_Stream.Value out = m.update(newClose);
// out.macd(), out.macdSignal(), out.macdHist()
// On JDK 21+ it also destructures:
//   if (out instanceof Core.MACD_Stream.Value(double macd, double signal, double hist)) { ... }

// A candlestick pattern returns int
Core.CDLDOJI_Stream c = core.CDLDOJI_Open(open, high, low, close);
int pattern = c.update(o, h, l, cl);
```

## Error model

| Call | Behaviour |
|------|-----------|
| `<NAME>_Open` / `<NAME>_OpenAndFill` | Too little history throws `InsufficientHistoryException` (a subclass of `IllegalArgumentException` — catch it to accumulate more bars and retry). Out-of-range parameters, or output arrays that alias the input or each other (`OpenAndFill`), throw plain `IllegalArgumentException`. |
| `update` / `peek` | `IllegalArgumentException` on a non-finite bar value, leaving the handle unchanged. Nothing else throws after a successful `Open` (see the note below for the one composed-indicator corner). |
| `updateAndFill` | The same non-finite rejection, per bar — and it commits the bars ahead of the one it rejects. Ragged inputs, an output shorter than the bar count, and an output that is also an input or another output throw before it commits anything. |
| `value` / `copy` | Never throw. |

One narrow exception to "the handle is unchanged": a *composed* indicator drives its sub-stages through their own public update, so a value the library computed internally is re-checked there. If such an intermediate overflowed to an infinity, the rejection would surface after earlier sub-stages had advanced, and would name the sub-stage. It needs input magnitudes around 1e306 and up — the overflow class TA-Lib already treats as out of scope — but the guarantee is stated for the caller-supplied case, which is the one you can provoke.

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
