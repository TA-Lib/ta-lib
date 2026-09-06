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

One more call, `OpenAndFill`, writes array output instead of a single value — see [Array-Fill Open](#array-fill-open) below.

Additional read-only [utility functions](#utility-calls) are available.

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
- **Threads.** A stream is single-writer: an `Update` or `TA_<NAME>_Advance` must not race with any other call on the same stream. With no concurrent writer, `Peek`, `Value` and `TA_<NAME>_OutRange` read nothing but the handle — `Peek`'s `const` is load-bearing — and may run concurrently, as may `Clone`. Distinct streams — a `Clone` result included — are fully independent.

## Multi-input / multi-output

Inputs and outputs mirror the batch function — OHLCV in, one out-pointer per output:

```c
/* Candlestick: OHLC in, one int out */
TA_CDLDOJI_Update( s, open, high, low, close, &outInteger );

/* MACD: one in, three out */
TA_MACD_Update( s, close, &macd, &signal, &hist );
```

## Array-Fill Open

`Open` and `Update` each write a single value per output. One more call writes a full array instead — the same shape the [batch function](/api/) would produce — while still opening the stream:

| Call | When | Does |
|------|------|------|
| `TA_<NAME>_OpenAndFill` | once, instead of `Open` | like `Open`, but returns the output for **every** history bar |

```c
double out[300];                 /* one array per output */
int    begIdx, nbElement;

TA_SMA_OpenAndFill( &s, history, historyLen, period,
                    &begIdx, &nbElement, out );

/* out[0 .. nbElement-1] is the SMA over all of history; then stream on: */
TA_SMA_Update( s, newClose, &sma );
```

## Utility Calls

| Call | When | Does |
|------|------|------|
| `TA_<NAME>_Value` | any time | the value(s) at the last bar the stream counted, without recomputing |
| `TA_<NAME>_Clone` | any time | an independent fork of the stream, at the same bar |
| `TA_<NAME>_OutRange` | any time | the bars the stream has an output for — the batch range over the same bars |
| `TA_<NAME>_Advance` | after a bar you will not feed | counts that bar and nothing else |

```c
double v;
int begIdx, nbElement;
TA_SMA_Stream *fork = NULL;

TA_SMA_Value( s, &v );                      /* the value at the last bar s counted */
TA_SMA_Clone( s, &fork );                   /* independent from here on */
TA_SMA_OutRange( s, &begIdx, &nbElement );  /* the bars s has an output for */
TA_SMA_Advance( s );                        /* a bar you skipped, counted */
```

`Value` hands back what `Open` or the last `Update` already gave you: it recomputes
nothing and takes no bar. One out-pointer per output, so a multi-output function
answers all of them at once. `Open` seeds it, an accepted bar replaces it, and a
bar you skip with `TA_<NAME>_Advance` holds it — a held value is that bar's output
— while `Peek` and a rejected bar leave it alone. So it always names the bar
`TA_<NAME>_OutRange` reports. A declinable output
(MAMA's FAMA) is reported here even when the caller passed `NULL` for it everywhere
else.

`Clone` gives a second, independent stream at the same bar: its own copy of every
buffer and every sub-stream, carrying the value and the range verbatim. Both
streams must be `Close`d. It answers `TA_ALLOC_ERR` if any allocation fails,
leaving `*clone` NULL and the original untouched. This is the only way to fork a
live stream — the warm-up history is gone once `Open` returns, so there is
nothing to replay into a second one — and it is what makes `Value` worth having,
since a fork has no call that handed you its value.

`TA_<NAME>_OutRange` reports the bars the stream has an output for. A stream
opened over `historyLen` bars starts at `(lookback, historyLen - lookback)`, and
every `Update` it accepts adds one. A rejected `Update` adds nothing, and neither
does `Peek`. So after a stream has been carried over `nbBar` bars, by any mix of
`Open`, `Update` and `TA_<NAME>_Advance`, this reports what the batch call over
`(0, nbBar-1)` would. The count saturates at `TA_MAX_INDEX`.

`TA_<NAME>_Advance` is how you count a bar the stream was never fed — one an
`Update` rejected and that will not be re-fed, or a session with no print. It
moves the range by one and nothing else: the state is untouched and `Value` keeps
answering the previous output, which is that bar's output. Without it two streams
on one feed drift a bar apart the moment one of them skips, so decide at the
rejection: re-feed the bar with the corrected value, or count it here.

See [Rules](#rules) for when concurrent reads of these are safe.

## Error model

| Call | Returns |
|------|---------|
| `TA_<NAME>_Open` / `TA_<NAME>_OpenAndFill` | <ul><li>`TA_INSUFFICIENT_HISTORY` when `historyLen` is below `lookback + 1` — the one failure worth retrying, since another bar might fix it</li><li>`TA_OUT_OF_RANGE_START_INDEX` when `historyLen` is 0</li><li>`TA_OUT_OF_RANGE_END_INDEX` when `historyLen` exceeds `TA_MAX_INDEX + 1`</li><li>`TA_BAD_PARAM` — a NULL pointer, or a parameter out of range</li><li>`TA_ALLOC_ERR` — a memory allocation failure</li></ul>On any of these, `*stream` is NULL. |
| `TA_<NAME>_Update` / `TA_<NAME>_Peek` | `TA_BAD_PARAM` on NULL arguments, or invalid input such as NaN or ±Inf. A rejection changes nothing at all — no state, no output, and no range — so the next call sees exactly what the last accepted bar left. To count a rejected bar rather than re-feed it, call `TA_<NAME>_Advance` (see [Utility Calls](#utility-calls)). |
| `TA_<NAME>_Close`  | `TA_SUCCESS`; `TA_<NAME>_Close(NULL)` is a no-op |
| `TA_<NAME>_Value` | `TA_BAD_PARAM` on a NULL stream or a NULL out-pointer for a required output. A declinable output may be NULL, and is then simply not written. |
| `TA_<NAME>_Clone` | `TA_BAD_PARAM` on a NULL stream or a NULL `clone`; `TA_ALLOC_ERR` if any allocation fails. On either, `*clone` is NULL and the original is untouched. |
| `TA_<NAME>_OutRange` | `TA_BAD_PARAM` on a NULL argument |
| `TA_<NAME>_Advance` | `TA_BAD_PARAM` on a NULL argument |

## Discovering streamable functions

When driving TA-Lib through the [abstraction layer](/api/#abstract), streamable functions carry the `TA_FUNC_FLG_STREAM` flag in their function info.
