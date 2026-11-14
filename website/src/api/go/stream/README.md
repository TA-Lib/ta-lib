---
title: Go Streaming API
description: "Go streaming API for live feeds: a stream carries indicator state from bar to bar, so an update never recomputes the history; its values are bit-identical to the batch calls outside a few stated exceptions."
toc: false
---

::: warning Not yet released
The Go module is planned for official release in Q4 2026.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so a new bar never costs a pass over the history.

Every value is **bit-identical** to what the [batch method](/api/go/) would return by recomputing over the whole slice (see [detailed specs](/spec/streaming/#rh1) for a few exceptions).

Each function adds two opening methods on `Core` and a stream type, e.g. `SMAStream`, with a handful of methods:

| Call | When | Does |
|------|------|------|
| `core.<NAME>Open(history, params)` | once | validate params, consume warm-up history, return the stream and its value at the last history bar |
| `s.Update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `s.Peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |

One more call, `<NAME>OpenAndFill`, also writes slice output: see [Array-Fill Open](#array-fill-open) below.

Additional [utility functions](#utility-calls) are available.

There is no `Close`: an unreferenced stream is garbage-collected.

## Example (SMA)

```go
core := talib.NewCore()

// Seed with warm-up history (at least SMALookback(period) + 1 bars).
history := make([]float64, 100) // ...your closing prices, oldest first...
s, last, err := core.SMAOpen(history, 30) // stream + value at the last history bar
if err != nil {
	log.Fatal(err)
}

// Each time a bar closes:
v, err := s.Update(newClose) // BadParam on a non-finite bar
if err != nil {
	log.Fatal(err)
}

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
p, err := s.Peek(formingClose) // state left unchanged
```

`Open` returns an error: `talib.InsufficientHistory` if there is too little history (another bar might fix it, so this is the one worth retrying), `talib.BadParam` if a parameter is out of range. `Update` and `Peek` return an error too, and after a successful `Open` what they reject is invalid input such as NaN or ±Inf, as `talib.BadParam`. `Update` also rejects a bar past `talib.IndexMax`, the last index the batch API addresses. A rejection changes nothing at all: no state, no value, and no range.

## Rules

- **Warm-up.** `Open` succeeds only if `len(history) >= <NAME>Lookback(params) + 1`: with fewer bars there is no defined value yet. After `Open`, the history can be discarded: the stream keeps everything it needs.
- **Closed vs forming bar.** `Update` commits state irreversibly, so use it only for **closed** bars. `Peek` returns exactly the value the next `Update` would, without committing: call it as often as the forming bar ticks.
- **The unstable period is resolved once, at `Open`.** The lookback above is the one of the `Core` the stream is opened on. Under an [Auto level](/api/unstable-period/#auto) it is the Auto lookback, so `Open` needs that many bars plus one.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. The [unstable period](/api/go/#numerical_stability) and [candle settings](/api/go/#candle_settings) are those of the `Core` the stream is opened on: configure it first, and do not call its setters while a stream opened on it is in use.
- **Goroutines.** A stream belongs to one goroutine: no call on it may run while another runs. `Clone` forks an independent stream another goroutine can drive.

## Multi-input / multi-output

Inputs and outputs mirror the batch method. A multi-output function returns its outputs positionally, in batch output order; a candlestick pattern returns an `int32`:

```go
// MACD: one input, three outputs
m, macd, signal, hist, err := core.MACDOpen(history, 12, 26, 9)
if err != nil {
	log.Fatal(err)
}
macd, signal, hist, err = m.Update(newClose)

// A candlestick pattern returns an int32
c, _, err := core.CDLDOJIOpen(open, high, low, close)
if err != nil {
	log.Fatal(err)
}
pattern, err := c.Update(o, h, l, cl)
```

## Array-Fill Open

`Open` and `Update` each return single values. One more call writes a full slice instead, the same shape the [batch method](/api/go/) would produce, while still opening the stream:

| Call | When | Does |
|------|------|------|
| `core.<NAME>OpenAndFill(..)` | once, instead of `Open` | like `Open`, but also fills the output for **every** history bar, returning the stream and an `OutRange` |

```go
warmup := make([]float64, len(history))

s, filled, err := core.SMAOpenAndFill(history, 30, warmup)
if err != nil {
	log.Fatal(err)
}

// warmup[:filled.NBElement] is the SMA from history bar filled.BegIdx; then stream on:
v, err := s.Update(newClose)
```

`OpenAndFill` takes the [batch method](/api/go/)'s optional parameters and one slice per output, and returns the range it wrote as the same `OutRange` the batch method returns, beside the live stream. An output must not share memory with the input or with another output: that is `talib.BadParam`, but a slice whose capacity a three-index expression cut (`s[i:j:k]`, as `slices.Clip` makes) escapes the check unless both start at one element. `OpenAndFill` checks its outputs before the warm-up, so an empty output is `talib.BadParam` even when the history is too short.

## Utility Calls

| Call | When | Does |
|------|------|------|
| `s.Value()` | any time | the value(s) at the last bar the stream counted, without recomputing |
| `s.Clone()` | any time | an independent fork of the stream, at the same bar |
| `s.OutRange()` | any time | the bars the stream has an output for: the batch range over the same bars |
| `s.Advance()` | after a bar you will not feed | advances the range without affecting any other internal state of the stream |

```go
v := s.Value()    // the value at the last bar s counted
fork := s.Clone() // independent from here on
r := s.OutRange() // the bars s has an output for
if err := s.Advance(); err != nil { // a bar you skipped, counted
	log.Fatal(err)
}
```

The first three return no error: they read what the stream already holds, so there is nothing to reject. `Advance()` returns one: it moves the range, and the range cannot pass `talib.IndexMax`.

## Error model

Every error is a `RetCode`; test for one with `errors.Is`.

| Call | Behaviour |
|------|-----------|
| `<NAME>Open` / `<NAME>OpenAndFill` | Too little history, an empty one included, returns `talib.InsufficientHistory`: accumulate more bars and retry. An out-of-range parameter returns `talib.BadParam`, and so do histories of different lengths and an `OpenAndFill` output that is empty, too short or shares memory; `OpenAndFill` checks its outputs before the warm-up, so an empty output is `talib.BadParam` even when the history is too short. A history longer than `talib.IndexMax + 1` bars returns `talib.OutOfRangeEndIndex`. |
| `Update` / `Peek` | <ul><li>`talib.BadParam` on invalid input such as NaN or ±Inf</li><li>`talib.OutOfRangeEndIndex` from `Update` (never `Peek`) once the range has reached bar `talib.IndexMax`, the last index the batch API addresses</li></ul>A rejection changes nothing at all: no state, no value, and no range. |
| `Advance` | `talib.OutOfRangeEndIndex` once the range has reached bar `talib.IndexMax`. |
