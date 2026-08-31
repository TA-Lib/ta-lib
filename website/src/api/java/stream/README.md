---
title: Java Streaming API
description: "Java streaming API for live feeds: a stream carries indicator state from bar to bar at O(1) per update, bit-identical to the batch calls."
toc: false
---

::: warning Not yet released
The Java API is not yet released. Estimated release: **Q1 2027**.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch method](/api/java/) (`core.SMA`, `core.RSI`, …) would return by recomputing over the whole array.

Each streamable function adds two factory methods on `Core` and a handful of methods on its stream (a class nested in `Core`, e.g. `Core.SmaStream` — unrelated to `java.util.stream`):

| Call | When | Does |
|------|------|------|
| `core.<name>Open(history, params)` | once | validate params, consume warm-up history, return a **stream** |
| `stream.update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `stream.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |

Two more calls, `openAndFill` and `updateAndFill`, write array output instead of a single value — see [Array-Fill Calls](#array-fill-calls) below.

Additional read-only [utility functions](#utility-calls) are available.

There is no `close` — a stream is ordinary heap state, so an unreferenced stream is simply garbage-collected.

## Example (SMA)

```java
import io.github.talib.Core;

Core core = Core.DEFAULT;

// Seed with warm-up history (>= SMA_Lookback(period) + 1 bars).
double[] history = /* ...your closing prices... */;
Core.SmaStream s = core.smaOpen(history, 30); // value() starts at the last history bar

// Each time a bar closes:
double v = s.update(newClose);                  // throws only on a non-finite bar

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
double provisional = s.peek(formingClose);      // state left unchanged
```

`open` returns the stream directly; its `value()` starts at the last history bar's value. After a successful `open`, the only thing `update` and `peek` reject is invalid input such as NaN or ±Inf. A rejected bar leaves the stream's **state** untouched — nothing is committed — but a rejected `update` still advances `outRange()` by one, its output being the previous one, held; `value()` answers the value(s) at the last bar the stream counted (see [Utility Calls](#utility-calls)). `peek` advances nothing.

## Rules

- **Warm-up.** `open` succeeds only if `history.length >= <NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. Too little history throws `InsufficientHistoryException` (see [Error model](#error-model)). After `open`, the history can be discarded — the stream keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing — call it as often as the forming bar ticks. `value()` re-reads the last committed value without recomputing.
- **Parameters are fixed at `open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and [candle settings](/api/#candle_settings) are read from the owning `Core` at `open`. Since `Core` is immutable they cannot change underneath a live stream — to stream with different settings, build a new `Core` and open from that.
- **Threads.** A stream is single-writer — `update`, `peek`, `value()`, and `clone()` must not race with an `update` on the same stream. With no concurrent `update`, `peek`/`value()`/`clone()` are read-only and safe to call concurrently after safe publication. Distinct streams (including `clone()` results) are fully independent.
- **Not serializable.** To checkpoint, retain the history and re-open — the result is bit-identical by contract.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. Multi-output functions return a small immutable `Value` record with one component per output, in batch output order; candlestick patterns return `int`:

```java
// MACD: one input, three outputs
Core.MacdStream m = core.macdOpen(history, 12, 26, 9);
Core.MacdStream.Value out = m.update(newClose);
// out.macd(), out.macdSignal(), out.macdHist()
// On JDK 21+ it also destructures:
//   if (out instanceof Core.MacdStream.Value(double macd, double signal, double hist)) { ... }

// A candlestick pattern returns int
Core.CdldojiStream c = core.cdldojiOpen(open, high, low, close);
int pattern = c.update(o, h, l, cl);
```

## Array-Fill Calls

`open` and `update` each write a single value. Two more calls write a full array instead — the same shape the [batch method](/api/java/) would produce — while still driving the stream:

| Call | When | Does |
|------|------|------|
| `core.<name>OpenAndFill(..)` | once, instead of `open` | like `open`, but also fills the output for **every** history bar |
| `stream.updateAndFill(bars, outs)` | instead of a loop of `update` | commit `n` closed bars and write the `n` values |

**`openAndFill`**

```java
import io.github.talib.OutRange;

double[] warmup = new double[history.length];

Core.SmaStream s = core.smaOpenAndFill(history, 30, warmup);
OutRange r = s.outRange();                      // the bars it has an output for

// warmup[0 .. r.count() - 1] is the SMA over all of history; then stream on:
double v = s.update(newClose);
```

The optional parameters and output arrays are exactly the [batch method](/api/java/)'s. The range written is reported on the returned stream as `outRange()` rather than through out-parameters — see [Utility Calls](#utility-calls) below. The output arrays must not alias the input or each other.

**`updateAndFill`**

```java
double[] out = new double[gap.length];

s.updateAndFill(gap, out);          // out[i] is the SMA at gap[i]
```

`updateAndFill` has no second return value for the range it wrote — call
`outRange()` afterward (see [Utility Calls](#utility-calls)).

It throws `IllegalArgumentException` before committing or counting anything if
the input arrays differ in length, an output is shorter than the bar count, or
an output is the same array as an input or as another output. A zero-length
call does nothing. An invalid bar (NaN or ±Inf) also throws
`IllegalArgumentException`, exactly as `update` does, and stops the call there:
the bars **before** it are committed with their values written, and the invalid
bar is counted but neither committed nor written to its output slot.
`outRange()` says where it stopped — its last bar is the rejected one, so it
counts one more than the values written.

## Utility Calls

| Call | When | Does |
|------|------|------|
| `stream.value()` | any time | the value(s) at the last bar the stream counted, without recomputing |
| `stream.clone()` | any time | an independent fork of the stream, at the same bar |
| `stream.outRange()` | any time | the bars the stream has an output for — the batch range over the same bars |

```java
Core.SmaStream s = core.smaOpen(history, 30);

double v = s.value();               // the value at the last bar s counted
Core.SmaStream fork = s.clone();    // independent from here on
OutRange r = s.outRange();          // the bars s has an output for
```

`value()` hands back what `open` or the last `update` already gave you: it
recomputes nothing and takes no bar. A single-output function returns `double`; a
multi-output one returns its nested `Value` record, so all its outputs come back at
once. `open` seeds it, an accepted bar replaces it, and a rejected bar holds it —
a held value is that bar's output — while `peek` leaves it alone. So it always
names the bar `outRange()` reports.

`clone()` gives a second, independent stream at the same bar: arrays are copied and
sub-streams cloned recursively, and the fork carries the value and the range
verbatim. The `Core` reference is shared, because a `Core` is immutable for a
stream's lifetime. It overrides `Object.clone()` but does not use the `Cloneable`
protocol — the body is a copy constructor, so it needs no marker interface and
throws no `CloneNotSupportedException`. It is the only way to fork a live stream —
the warm-up history is gone once `open` returns — and it is what makes `value()`
worth having, since a fork has no call that handed you its value.

`outRange()` reports the bars the stream has an output for: `(lookback,
historyLen - lookback)` at `open`, then one more for every bar the call accepts
as data, whether the step computes on it or it is turned down as non-finite — it
happened and holds a position in the series, and its output is the previous one,
held. That is what keeps two streams on the same feed positionally aligned when
one rejects a bar the other accepts. `peek` counts nothing, and neither does a
malformed call — a fault in the call is not a bar.

See [Rules](#rules) for when concurrent reads of these are safe.

## Error model

| Call | Behaviour |
|------|-----------|
| `<name>Open` / `<name>OpenAndFill` | Too little history throws `InsufficientHistoryException` (a subclass of `IllegalArgumentException` — catch it to accumulate more bars and retry). Out-of-range parameters throw plain `IllegalArgumentException`. |
| `update` / `peek` | `IllegalArgumentException` on invalid input such as NaN or ±Inf. The stream's state is untouched — nothing is committed — but a rejected `update` still advances `outRange()` by one, and `value()` answers the value(s) at the last bar the stream counted; `peek` advances nothing. Nothing else throws after a successful `open` (see the note below for the one composed-indicator corner). |
| `updateAndFill` | Ragged inputs, an output shorter than the bar count, or an output that is also an input or another output throw `IllegalArgumentException` — none of which commits or counts anything. An invalid bar (NaN or ±Inf) also throws `IllegalArgumentException`, having committed the bars before it and counted — but not committed — the invalid one. |
| `value` / `clone` / `outRange` | Never throw. |

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
