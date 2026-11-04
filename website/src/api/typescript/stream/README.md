---
title: TypeScript Streaming API
description: "TypeScript streaming API for live feeds: a stream carries indicator state from bar to bar, so an update never recomputes the history; its values are bit-identical to the batch calls outside a few stated exceptions."
toc: false
---

::: warning Not yet released
The TypeScript library is not on npm yet. Until it is, [build it from source](/api/typescript/#build).
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so a new bar never costs a pass over the history.

Every value is **bit-identical** to what the [batch function](/api/typescript/) would return by recomputing over the whole array (see [detailed specs](/spec/streaming/#rh1) for a few exceptions).

Each streamable function adds two opening functions and a stream class, e.g. `SmaStream`, with a handful of methods:

| Call | When | Does |
|------|------|------|
| `<name>Open(history, params)` | once | validate params, consume warm-up history, return a **stream** |
| `stream.update(bar)` | once per **closed** bar | commit one bar and answer the new value |
| `stream.peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |

A single-output function answers with a `number`. A multi-output one writes into a sink you pass and own: see [Multi-input / multi-output](#multi-input-multi-output).

One more call, `<name>OpenAndFill`, writes array output instead of a single value: see [Array-Fill Open](#array-fill-open) below.

Additional [utility functions](#utility-calls) are available.

There is no `close`: an unreferenced stream is simply garbage-collected.

## Example (SMA)

```ts
import { smaOpen } from 'ta-lib';

// Seed with warm-up history (>= smaLookback(period) + 1 bars).
const history = new Float64Array([ /* ...your closing prices... */ ]);
const s = smaOpen(history, 30);           // value() starts at the last history bar

// Each time a bar closes:
const v = s.update(newClose);             // throws on a non-finite bar, or past INDEX_MAX

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
const provisional = s.peek(formingClose); // state left unchanged
```

`open` returns the stream directly; its `value()` starts at the last history bar's value. After a successful `open`, what `update` and `peek` reject is invalid input such as NaN or ±Inf; `update` also rejects a bar past `Core.INDEX_MAX`, the last index the batch API addresses. A rejection changes nothing at all: no state, no value, and no range. To count a rejected bar rather than re-feed it, call `advance()`; `value()` then answers the value(s) at the last bar the stream counted (see [Utility Calls](#utility-calls)).

## Rules

- **Warm-up.** `open` succeeds only if `history.length >= <name>Lookback(params) + 1`: with fewer bars there is no defined value yet. Too little history throws `TALibError` with `code` `'TA_INSUFFICIENT_HISTORY'` (see [Error model](#error-model)). After `open`, the history can be discarded: the stream keeps everything it needs.
- **Closed vs forming bar.** `update` commits state irreversibly, so use it only for **closed** bars. `peek` returns exactly the value the next `update` would, without committing: call it as often as the forming bar ticks. `value()` re-reads the last committed value without recomputing.
- **The unstable period is resolved once, at `open`.** The lookback above is the one of the `Core` the stream is opened on. Under an [Auto level](/api/unstable-period/#auto) it is the Auto lookback, so `open` needs that many bars plus one.
- **Parameters are fixed at `open`.** Changing a parameter means a new stream. The [unstable period](/api/typescript/#numerical_stability) and [candle settings](/api/typescript/#candle_settings) come from the `Core` passed as the opener's last argument, or `Core.DEFAULT`. Since `Core` is immutable they cannot change underneath a live stream: to stream with different settings, build a new `Core` and open with that.

## Multi-input / multi-output

Inputs and outputs mirror the batch function. A multi-output function writes its outputs into a `<Name>Out`, a plain mutable object with one field per output, in batch output order, that **you** allocate and pass in. Candlestick patterns return a `number`:

```ts
import { cdldojiOpen, macdOpen, MacdOut } from 'ta-lib';

// MACD: one input, three outputs
const m = macdOpen(history, 12, 26, 9);
const out = new MacdOut();               // allocate once, reuse every bar
m.update(newClose, out);
// out.macd, out.macdSignal, out.macdHist

// A candlestick pattern returns a number
const c = cdldojiOpen(open, high, low, close);
const pattern = c.update(o, h, l, cl);
```

Reusing one sink is the point: `update`, `peek` and `value` overwrite its fields rather than allocating a new one. The price is that **its contents are only valid until the next call that writes it**. Copy the fields out if the reading has to outlive the call, or allocate one sink per slot.

## Array-Fill Open

`open` and `update` each write a single value. One more call writes a full array instead, the same shape the [batch function](/api/typescript/) would produce, while still opening the stream:

| Call | When | Does |
|------|------|------|
| `<name>OpenAndFill(..)` | once, instead of `open` | like `open`, but also fills the output for **every** history bar |

```ts
import { smaOpenAndFill } from 'ta-lib';

const warmup = new Float64Array(history.length);

const s = smaOpenAndFill(history, 30, warmup);
const r = s.outRange();                   // the bars it has an output for

// warmup[0 .. r.count - 1] is the SMA over all of history; then stream on:
const v = s.update(newClose);
```

The optional parameters and output arrays are exactly the [batch function](/api/typescript/)'s. The range written is reported on the returned stream as `outRange()`: see [Utility Calls](#utility-calls) below. The output arrays must not share memory with the input or with each other.

## Utility Calls

| Call | When | Does |
|------|------|------|
| `stream.value()` / `stream.value(out)` | any time | the value(s) at the last bar the stream counted, without recomputing |
| `stream.clone()` | any time | an independent fork of the stream, at the same bar |
| `stream.outRange()` | any time | the bars the stream has an output for: the batch range over the same bars |
| `stream.advance()` | after a bar you will not feed | advances the range without affecting any other internal state of the stream |

```ts
const s = smaOpen(history, 30);

const v = s.value();                // the value at the last bar s counted
const fork = s.clone();             // independent from here on
const r = s.outRange();             // the bars s has an output for
s.advance();                        // a bar you skipped, counted
```

## Error model

Every rejection is a `TALibError`; branch on its `code`.

| Call | Behaviour |
|------|-----------|
| `<name>Open` / `<name>OpenAndFill` | Too little history throws `'TA_INSUFFICIENT_HISTORY'`: catch it to accumulate more bars and retry. Out-of-range parameters throw `'TA_BAD_PARAM'`. |
| `update` / `peek` | <ul><li>`'TA_BAD_PARAM'` on invalid input such as NaN or ±Inf</li><li>`'TA_OUT_OF_RANGE_END_INDEX'` from `update` (never `peek`) once the range has reached bar `Core.INDEX_MAX`, the last index the batch API addresses</li></ul>A rejection changes nothing at all: no state, no value, and no range. |
| `advance` | `'TA_OUT_OF_RANGE_END_INDEX'` once the range has reached bar `Core.INDEX_MAX`, the last index the batch API addresses. |
| `value` / `clone` / `outRange` | Never throw, except `value(out)` on a multi-output stream: it throws `'TA_BAD_PARAM'` when `out` is absent or not an object, as `update` and `peek` do. |

## Discovering streamable functions

When driving TA-Lib through the [Abstract API](/api/typescript/#abstract), streamable functions carry the `FuncFlags.STREAMING` bit in `FuncInfo.flags`.
