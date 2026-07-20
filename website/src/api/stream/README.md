---
title: C/C++ Streaming API
toc: false
---

# C/C++ Streaming API

::: warning Not yet released
This feature is planned for v0.8.x.
:::

The [batch functions](/api/) (`TA_SMA`, `TA_RSI`, …) recompute over a whole array. The **streaming API** keeps a small handle that carries state between bars, so each new bar costs O(1) — and the result is **bit-identical** to the batch function. Ideal for live feeds.

Every TA function gets these calls:

| Call | When | Does |
|------|------|------|
| `TA_<NAME>_Open`   | once                                | validate params, consume warm-up history, return a **handle** + current value |
| `TA_<NAME>_OpenAndFill` | once, instead of `Open`        | like `Open`, but returns the output for **every** history bar — see [below](#get-the-full-history-output-openandfill) |
| `TA_<NAME>_Update` | once per **closed** bar             | commit one bar, return the new value |
| `TA_<NAME>_Peek`   | any time on the **forming** bar     | evaluate a provisional bar **without** committing state |
| `TA_<NAME>_Close`  | once                                | free the handle |

## Example (SMA)

```c
TA_SMA_Stream *s;
double sma;

int    period     = 30;
int    historyLen  = 30;   /* must be >= TA_SMA_Lookback(period) + 1 */

/* Seed with warm-up history. */
double history[30] = { /* ...your closing prices... */ };
if( TA_SMA_Open( &s, history, historyLen, period, &sma ) != TA_SUCCESS )
    return; /* *s is NULL on failure */

/* Each time a bar closes: */
TA_SMA_Update( s, newClose, &sma );
printf( "SMA = %f\n", sma );

/* Intra-bar, on the not-yet-closed bar (repeat as the price ticks): */
TA_SMA_Peek( s, formingClose, &sma );   /* state left unchanged */

TA_SMA_Close( s );
```

## Rules

- **Warm-up.** `Open` succeeds only if `historyLen >= TA_<NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. After `Open`, the history buffer can be freed — the handle keeps everything it needs.
- **Closed vs forming bar.** `Update` commits state irreversibly, so use it only for **closed** bars. `Peek` returns the exact value `Update` would, but without committing — call it as often as the forming bar ticks.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#unstable_period) and candle settings are first read at `Open` and must not change during the stream's life.
- **Threads.** A handle is single-writer: never drive one handle from two threads at once (even `Peek`, despite its `const`). Distinct handles are fully independent.
- **Don't persist** a handle across library versions.

## Get the full history output (`OpenAndFill`)

`Open` gives you only the value at the last history bar. `OpenAndFill` gives you the output for **every** history bar — the same array the [batch function](/api/) would produce — while still opening the live handle.

```c
double out[300];                 /* one array per output */
int    begIdx, nbElement;

TA_SMA_OpenAndFill( &s, history, historyLen, period,
                    &begIdx, &nbElement, out );

/* out[0 .. nbElement-1] is the SMA over all of history; then stream on: */
TA_SMA_Update( s, newClose, &sma );
```

The optional parameters and outputs (`outBegIdx`, `outNBElement`, one array per output) are exactly the [batch API](/api/)'s; everything else matches `Open`.

## Multi-input / multi-output

Inputs and outputs mirror the batch function — OHLCV in, one out-pointer per output:

```c
/* Candlestick: OHLC in, one int out */
TA_CDLDOJI_Update( s, open, high, low, close, &outInteger );

/* MACD: one in, three out */
TA_MACD_Update( s, close, &macd, &signal, &hist );
```

## Error model

| Call | Returns |
|------|---------|
| `TA_<NAME>_Open` / `TA_<NAME>_OpenAndFill` | `TA_BAD_PARAM` (bad param, or `historyLen` too small) or `TA_ALLOC_ERR`; `*stream` is NULL on failure. `OpenAndFill` also requires non-NULL, non-overlapping output arguments. |
| `TA_<NAME>_Update` / `TA_<NAME>_Peek` | `TA_BAD_PARAM` only on NULL arguments |
| `TA_<NAME>_Close`  | `TA_SUCCESS`; `TA_<NAME>_Close(NULL)` is a no-op |

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
