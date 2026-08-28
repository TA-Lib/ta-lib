---
title: C/C++ Streaming API
description: "The TA-Lib C/C++ streaming API for live feeds: open a stream once, feed one bar at a time at O(1) per bar, with values bit-identical to the batch functions."
toc: false
---

::: warning Not yet released
This feature is planned for v0.8.x.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch function](/api/) (`TA_SMA`, `TA_RSI`, …) would return by recomputing over the whole array.

Every TA function gets these calls:

| Call | When | Does |
|------|------|------|
| `TA_<NAME>_Open`   | once                                | validate params, consume warm-up history, return a **stream** + current value |
| `TA_<NAME>_Update` | once per **closed** bar             | commit one bar, return the new value |
| `TA_<NAME>_Peek`   | any time on the **forming** bar     | evaluate a provisional bar **without** committing state |
| `TA_<NAME>_Close`  | once                                | free the stream |

Two more calls, `OpenAndFill` and `UpdateAndFill`, write array output instead of a single value — see [Array-Fill Calls](#array-fill-calls) below.

## Example (SMA)

```c
TA_SMA_Stream *s;
double sma;

int    period     = 30;
int    historyLen  = 30;   /* must be >= TA_SMA_Lookback(period) + 1 */

/* Seed with warm-up history. */
double history[30] = { /* ...your closing prices... */ };
if( TA_SMA_Open( &s, history, historyLen, period, &sma ) != TA_SUCCESS )
    return; /* s is NULL on failure */

/* Each time a bar closes: */
TA_SMA_Update( s, newClose, &sma );
printf( "SMA = %f\n", sma );

/* Intra-bar, on the not-yet-closed bar (repeat as the price ticks): */
TA_SMA_Peek( s, formingClose, &sma );   /* state left unchanged */

TA_SMA_Close( s );
```

## Rules

- **Warm-up.** `Open` succeeds only if `historyLen >= TA_<NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. After `Open`, the history buffer can be freed — the stream keeps everything it needs.
- **Closed vs forming bar.** `Update` commits state irreversibly, so use it only for **closed** bars. `Peek` returns the exact value `Update` would, but without committing — call it as often as the forming bar ticks.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and [candle settings](/api/#candle_settings) are first read at `Open` and must not change during the stream's life.
- **Threads.** A stream is single-writer: never drive one stream from two threads at once (even `Peek`, despite its `const`). Distinct streams are fully independent.

## Multi-input / multi-output

Inputs and outputs mirror the batch function — OHLCV in, one out-pointer per output:

```c
/* Candlestick: OHLC in, one int out */
TA_CDLDOJI_Update( s, open, high, low, close, &outInteger );

/* MACD: one in, three out */
TA_MACD_Update( s, close, &macd, &signal, &hist );
```

## Array-Fill Calls

`Open` and `Update` each write a single value per output. Two more calls write a full array instead — the same shape the [batch function](/api/) would produce — while still driving the stream:

| Call | When | Does |
|------|------|------|
| `TA_<NAME>_OpenAndFill` | once, instead of `Open` | like `Open`, but returns the output for **every** history bar |
| `TA_<NAME>_UpdateAndFill` | instead of a loop of `Update` | commit `barCount` closed bars and write a value for each |

**`OpenAndFill`**

```c
double out[300];                 /* one array per output */
int    begIdx, nbElement;

TA_SMA_OpenAndFill( &s, history, historyLen, period,
                    &begIdx, &nbElement, out );

/* out[0 .. nbElement-1] is the SMA over all of history; then stream on: */
TA_SMA_Update( s, newClose, &sma );
```

**`UpdateAndFill`**

```c
double gap[64], out[64];         /* one output array per output */

TA_SMA_UpdateAndFill( s, gap, 64, out );   /* out[i] is the SMA at gap[i] */
```

## Error model

| Call | Returns |
|------|---------|
| `TA_<NAME>_Open` / `TA_<NAME>_OpenAndFill` | <ul><li>`TA_INSUFFICIENT_HISTORY` when `historyLen` is below `lookback + 1` — the one failure worth retrying, since another bar might fix it</li><li>`TA_OUT_OF_RANGE_START_INDEX` when `historyLen` is 0</li><li>`TA_OUT_OF_RANGE_END_INDEX` when `historyLen` exceeds `TA_MAX_INDEX + 1`</li><li>`TA_BAD_PARAM` — a NULL pointer, or a parameter out of range</li><li>`TA_ALLOC_ERR` — a memory allocation failure</li></ul>On any of these, `*stream` is NULL. |
| `TA_<NAME>_Update` / `TA_<NAME>_Peek` | `TA_BAD_PARAM` on NULL arguments, or invalid input such as NaN or ±Inf. The stream is left untouched on an error. |
| `TA_<NAME>_UpdateAndFill` | `TA_BAD_PARAM` on NULL arguments, a negative `barCount`, or an output aliasing an input or another output — none of which commits anything. An invalid bar (NaN or ±Inf) also returns `TA_BAD_PARAM`, but commits the valid bars before it. |
| `TA_<NAME>_Close`  | `TA_SUCCESS`; `TA_<NAME>_Close(NULL)` is a no-op |

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
