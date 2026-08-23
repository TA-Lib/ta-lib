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
| `TA_<NAME>_OpenAndFill` | once, instead of `Open`        | like `Open`, but returns the output for **every** history bar — see [below](#get-the-full-history-output-openandfill) |
| `TA_<NAME>_Update` | once per **closed** bar             | commit one bar, return the new value |
| `TA_<NAME>_UpdateAndFill` | instead of a loop of `Update` | commit `barCount` closed bars and write the `barCount` values — see [below](#catch-up-n-bars-at-once-updateandfill) |
| `TA_<NAME>_Peek`   | any time on the **forming** bar     | evaluate a provisional bar **without** committing state |
| `TA_<NAME>_Close`  | once                                | free the stream |

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
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and candle settings are first read at `Open` and must not change during the stream's life.
- **Threads.** A stream is single-writer: never drive one stream from two threads at once (even `Peek`, despite its `const`). Distinct streams are fully independent.
- **Don't persist** a stream across library versions.

## Get the full history output (`OpenAndFill`)

`Open` gives you only the value at the last history bar. `OpenAndFill` gives you the output for **every** history bar — the same array the [batch function](/api/) would produce — while still opening the live stream.

```c
double out[300];                 /* one array per output */
int    begIdx, nbElement;

TA_SMA_OpenAndFill( &s, history, historyLen, period,
                    &begIdx, &nbElement, out );

/* out[0 .. nbElement-1] is the SMA over all of history; then stream on: */
TA_SMA_Update( s, newClose, &sma );
```

The optional parameters and outputs (`outBegIdx`, `outNBElement`, one array per output) are exactly the [batch API](/api/)'s; everything else matches `Open`.

## Catch up n bars at once (`UpdateAndFill`)

Feeding a gap one `Update` at a time works; `UpdateAndFill` does the same thing
in one call, writing one value per bar into your array:

```c
double gap[64], out[64];         /* one output array per output */

TA_SMA_UpdateAndFill( s, gap, 64, out );   /* out[i] is the SMA at gap[i] */
```

It is exactly `barCount` back-to-back `Update` calls — same values, same state —
with one set of argument checks instead of `barCount`. There is no `outBegIdx` /
`outNBElement` pair: `TA_StreamOutRange( s, &begIdx, &nbElement )` reports the
bars the handle has a value for, before and after.

That includes a call that fails partway. A non-finite bar is rejected exactly as
`Update` rejects it, which means the bars **before** it are already committed and
their values already written; the range tells you how many. Outputs must not
alias the inputs or each other, and `barCount == 0` is a no-op.

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
| `TA_<NAME>_Open` / `TA_<NAME>_OpenAndFill` | `TA_INSUFFICIENT_HISTORY` when `historyLen` is below `lookback + 1` — the one failure worth retrying, since another bar fixes it — or `TA_BAD_PARAM` (bad param, empty history) or `TA_ALLOC_ERR`; `*stream` is NULL on failure. `OpenAndFill` also requires non-NULL, non-overlapping output arguments. |
| `TA_<NAME>_Update` / `TA_<NAME>_Peek` | `TA_BAD_PARAM` on NULL arguments, and on a non-finite bar value — in which case the handle is left exactly as it was |
| `TA_<NAME>_UpdateAndFill` | `TA_BAD_PARAM` on NULL arguments, a negative `barCount`, an output aliasing an input or another output — none of which commits anything — and on a non-finite bar, which commits the bars before it |
| `TA_<NAME>_Close`  | `TA_SUCCESS`; `TA_<NAME>_Close(NULL)` is a no-op |

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
