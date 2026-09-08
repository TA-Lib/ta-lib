---
title: C# Streaming API
description: "C# streaming API for live feeds: a stream carries indicator state from bar to bar at O(1) per update, bit-identical to the batch calls, and Update allocates nothing."
toc: false
---

::: warning Not yet released
The C# API is not yet released. Estimated release: **Q1 2027**.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch method](/api/csharp/) (`core.SMA`, `core.RSI`, …) would return by recomputing over the whole array.

Each streamable function adds two factory methods on `Core` and a handful of members on its stream (a class nested in `Core`, e.g. `Core.SmaStream`):

| Call | When | Does |
|------|------|------|
| `core.<Name>Open(history, params)` | once | validate params, consume warm-up history, return a **stream** |
| `stream.Update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `stream.Peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |

One more call, `OpenAndFill`, writes array output instead of a single value — see [Array-Fill Open](#array-fill-open) below.

Additional read-only [utility functions](#utility-calls) are available.

There is **no `Dispose`**: a stream owns only managed state — its arrays, its sub-streams and a `Core` reference — so an unreferenced stream is simply collected. The stream types deliberately do not implement `IDisposable`.

## Example (SMA)

```csharp
using TALib;

var core = new Core();

// Seed with warm-up history (>= SMA_Lookback(period) + 1 bars).
double[] history = /* ...your closing prices... */;
Core.SmaStream s = core.SmaOpen(history, 30);  // Value starts at the last history bar

// Each time a bar closes:
double v = s.Update(newClose);                   // throws on a non-finite bar, or past MAX_INDEX

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
double provisional = s.Peek(formingClose);       // state left unchanged
```

`Open` returns the stream directly; its `Value` starts at the last history bar's value. After a successful `Open`, what `Update` and `Peek` reject is invalid input such as NaN or ±Inf; `Update` also rejects a bar past `Core.MAX_INDEX`, the last index the batch API addresses. A rejection changes nothing at all — no state, no value, and no range. To count a rejected bar rather than re-feed it, call `Advance()`; `Value` then answers the value(s) at the last bar the stream counted (see [Utility Calls](#utility-calls)).

## Rules

- **Warm-up.** `Open` succeeds only if `history.Length >= <NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. Too little history throws `InsufficientHistoryException` (see [Error model](#error-model)). After `Open`, the history can be discarded — the stream keeps everything it needs.
- **Closed vs forming bar.** `Update` commits state irreversibly, so use it only for **closed** bars. `Peek` returns exactly the value the next `Update` would, without committing — call it as often as the forming bar ticks. `Value` re-reads the last committed value without recomputing.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and [candle settings](/api/#candle_settings) are read from the owning `Core` at `Open`. Since `Core` is immutable they cannot change underneath a live stream — to stream with different settings, build a new `Core` and open from that.
- **Threads.** A stream is single-writer: `Update` must not race with any other call on the same stream. Processing forks are possible by cloning the stream, and each clone becomes fully independent and can be updated concurrently.
- **Spans, not arrays.** Series parameters are `ReadOnlySpan<double>` in and `Span<double>` out, so a warm-up window can be a slice of a larger buffer with no copy. Arrays convert implicitly, so `SmaOpen(history, 30)` on a `double[]` is unchanged. Because a span is never null, a null history arrives as an empty span and is rejected as one.
- **Not serializable.** The constructors are `internal`, so no partially built stream can be minted or deserialized. To checkpoint, retain the history and re-open — the result is bit-identical by contract.

## Multi-input / multi-output

`Update` and `Peek` take one argument per input series, in the batch call's order, and return one value per output. Multi-output indicators return a generated `readonly record struct` named after the function, whose members are the output names with the leading `out` stripped:

```csharp
Core.BbandsStream b = core.BbandsOpen(history, 20, 2.0, 2.0, MAType.SMA);

BbandsValue v = b.Update(newClose);
Console.WriteLine($"{v.RealUpperBand} {v.RealMiddleBand} {v.RealLowerBand}");

// It deconstructs, too:
var (upper, middle, lower) = b.Value;
```

::: tip Equality on a value type
These are record structs, so `==` is .NET's `double` equality: `NaN` equals `NaN` **and** `+0.0` equals `-0.0`. (Java's record differs on the second.) Compare `BitConverter.DoubleToInt64Bits` per component when bit-level identity is what you mean.
:::

## Array-Fill Open

`Open` and `Update` each write a single value. One more call writes a full array instead — the same shape the [batch method](/api/csharp/) would produce — while still opening the stream:

| Call | When | Does |
|------|------|------|
| `core.<Name>OpenAndFill(..)` | once, instead of `Open` | like `Open`, but also fills the output for **every** history bar |

```csharp
double[] history = /* ...your closing prices... */;
var outReal = new double[history.Length];

Core.SmaStream s = core.SmaOpenAndFill(history, 30, outReal);

OutRange r = s.OutRange;    // the bars it has an output for
// outReal[0 .. r.Count - 1] == what core.SMA(0, history.Length - 1, ...) writes
// ...and s is live, ready for Update.
```

The output arguments are the batch call's, in the same order. An output may not overlap an input, or another output — that throws `ArgumentException` and mints no stream. With spans that means genuine memory overlap, not just the same buffer: two slices of one array that share even one element are rejected.

## Utility Calls

| Call | When | Does |
|------|------|------|
| `stream.Value` | any time | the value(s) at the last bar the stream counted, without recomputing |
| `stream.Clone()` | any time | an independent fork of the stream, at the same bar |
| `stream.OutRange` | any time | the bars the stream has an output for — the batch range over the same bars |
| `stream.Advance()` | after a bar you will not feed | advances the range without affecting any other internal state of the stream |

```csharp
Core.SmaStream s = core.SmaOpen(history, 30);

double v = s.Value;                 // the value at the last bar s counted
Core.SmaStream fork = s.Clone();    // independent from here on
OutRange r = s.OutRange;            // the bars s has an output for
s.Advance();                        // a bar you skipped, counted
```

`Clone()` is a typed `Clone()`, deliberately not `ICloneable` — that interface
returns `object` and leaves deep-versus-shallow unsaid, and this is neither.

See [Rules](#rules) for when concurrent reads of these are safe.

## Error model

`Open` and `OpenAndFill` throw. After a successful open, `Update` and `Peek` reject invalid input such as NaN or ±Inf, and `Update` also rejects a bar past `Core.MAX_INDEX`. A rejection changes nothing at all — no state, no value, and no range; to count a rejected bar rather than re-feed it, call `Advance()`. `Value`, `Clone()` and `OutRange` never throw; `Advance()` throws only at the `MAX_INDEX` ceiling.

| Condition | Exception |
|---|---|
| Fewer than `lookback + 1` history bars | `InsufficientHistoryException` |
| An optional parameter outside its documented range | `ArgumentException` |
| A non-finite bar (NaN or ±Inf), or a non-finite real parameter | `ArgumentException` |
| A bar past `Core.MAX_INDEX` — from `Update` or `Advance`; `Peek` counts no bar and is not subject to it | `ArgumentException` carrying `RetCode.OutOfRangeEndIndex`. It does not clear: open a new stream on a shorter history. |

`InsufficientHistoryException` derives from `ArgumentException`, so you can catch it specifically — it is the one routine, data-dependent rejection — or catch every open failure uniformly. Messages carry a stable `"<NAME> open: "` prefix, and it is always the *called* function's name: `core.MaOpen(...)` rejecting reports `MA open:`, never the name of whatever moving average it delegates to.

Insufficient history is knowable in advance, so it need not be exceptional in your code: compare against `<NAME>_Lookback(params) + 1` before opening.

## Absolute-index outputs

`MININDEX`, `MAXINDEX` and `MINMAXINDEX` report bar indices. In the streaming tier those count bars fed to the stream rather than positions in an array — so treat an index as a position within this handle's own window, not as one you can compare against an index a different handle reported.

## Discovering streamable functions

The catalogue flags them, so you do not have to hardcode a list:

```csharp
using TALib.Metadata;

foreach (var f in Core.Functions)
{
    if ((f.Flags & FunctionFlags.Stream) != 0)
    {
        Console.WriteLine(f.Name);
    }
}
```

That is discovery only. Unlike the batch tier, there is no name-based way to *open* a stream — `FunctionCall` binds batch calls, and nothing binds streams. Opening one means calling its typed `<Name>Open` directly.

This is deliberate rather than an oversight, and the same is true of the other language bindings. A generic opener would have to return a stream whose type varies per function, and `Update` varies in both arity and return type, so the values would have to be boxed — which costs an allocation per bar, on the one path this whole tier exists to keep allocation-free. Worth designing properly if there is a call for it; not worth guessing at.
