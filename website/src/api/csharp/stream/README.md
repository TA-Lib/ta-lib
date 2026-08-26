---
title: C# Streaming API
description: "C# streaming API for live feeds: a stream handle carries indicator state from bar to bar at O(1) per update, bit-identical to the batch calls, and Update allocates nothing."
toc: false
---

::: warning Not yet released
The C# API is not yet released. Estimated release: **Q1 2027**.
:::

The **streaming API** is built for live feeds: open a stream once, then feed it one bar at a time. The stream carries its state from bar to bar, so each new bar costs O(1) — and every value is **bit-identical** to what the [batch method](/api/csharp/) (`core.SMA`, `core.RSI`, …) would return by recomputing over the whole array.

Each streamable function adds two factory methods on `Core` and a handful of members on its handle (a class nested in `Core`, e.g. `Core.SMA_Stream`):

| Call | When | Does |
|------|------|------|
| `core.<NAME>_Open(history, params)` | once | validate params, consume warm-up history, return a **handle** |
| `handle.Update(bar)` | once per **closed** bar | commit one bar, return the new value |
| `handle.Peek(bar)` | any time on the **forming** bar | evaluate a provisional bar **without** committing |
| `handle.Value` | any time | the most recently committed value |
| `handle.Clone()` | any time | an independent deep copy of the handle |
| `handle.OutRange` | any time | the bars this handle has a value for — the batch range over the same bars |

Two more calls, `OpenAndFill` and `UpdateAndFill`, write array output instead of a single value — see [Array-Fill Calls](#array-fill-calls) below.

There is **no `Dispose`**: a handle owns only managed state — its arrays, its sub-handles and a `Core` reference — so an unreferenced handle is simply collected. The handle types deliberately do not implement `IDisposable`.

## Example (SMA)

```csharp
using TALib;

var core = new Core();

// Seed with warm-up history (>= SMA_Lookback(period) + 1 bars).
double[] history = /* ...your closing prices... */;
Core.SMA_Stream s = core.SMA_Open(history, 30);  // Value starts at the last history bar

// Each time a bar closes:
double v = s.Update(newClose);                   // throws only on a non-finite bar

// Intra-bar, on the not-yet-closed bar (repeat as the price ticks):
double provisional = s.Peek(formingClose);       // state left unchanged
```

`Open` returns the handle directly; its `Value` starts at the last history bar's value. After a successful `Open`, the only thing `Update` and `Peek` reject is a **non-finite bar**, and they leave the handle exactly as it was.

## Rules

- **Warm-up.** `Open` succeeds only if `history.Length >= <NAME>_Lookback(params) + 1` — with fewer bars there is no defined value yet. Too little history throws `InsufficientHistoryException` (see [Error model](#error-model)). After `Open`, the history can be discarded — the handle keeps everything it needs.
- **Closed vs forming bar.** `Update` commits state irreversibly, so use it only for **closed** bars. `Peek` returns exactly the value the next `Update` would, without committing; it runs the same code on a copy and never writes the handle. `Value` re-reads the last committed value without recomputing.
- **Allocation.** `Update` allocates nothing — not handle state, and not a return value even for multi-output indicators, because those return a `readonly record struct`. `Peek` is different: where the handle owns several arrays or a sub-handle, the copy is a scratch held per thread and reused, so it allocates nothing after that thread's first peek of that indicator; otherwise `Peek` copies the handle and allocates in proportion to the state the indicator carries. If you peek on every tick and that matters, hold the value `Update` returns instead.
- **Parameters are fixed at `Open`.** Changing a parameter means a new stream. [Unstable period](/api/#numerical_stability) and candle settings are read from the owning `Core` at `Open`. Since `Core` is immutable they cannot change underneath a live handle — to stream with different settings, build a new `Core` and open from that.
- **Threads.** A handle is single-writer — `Update`, `Peek`, `Value` and `Clone()` must not race with an `Update` on the same handle. With no concurrent `Update`, `Peek`/`Value`/`Clone()` never write the handle and may run concurrently. Distinct handles (a `Clone()` result included) are fully independent.
- **Spans, not arrays.** Series parameters are `ReadOnlySpan<double>` in and `Span<double>` out, so a warm-up window can be a slice of a larger buffer with no copy. Arrays convert implicitly, so `SMA_Open(history, 30)` on a `double[]` is unchanged. Because a span is never null, a null history arrives as an empty span and is rejected as one.
- **Not serializable.** The constructors are `internal`, so no partially built handle can be minted or deserialized. To checkpoint, retain the history and re-open — the result is bit-identical by contract.

## Multi-input / multi-output

`Update` and `Peek` take one argument per input series, in the batch call's order, and return one value per output. Multi-output indicators return a generated `readonly record struct` named after the function, whose members are the output names with the leading `out` stripped:

```csharp
Core.BBANDS_Stream b = core.BBANDS_Open(history, 20, 2.0, 2.0, MAType.SMA);

BBANDS_Value v = b.Update(newClose);
Console.WriteLine($"{v.RealUpperBand} {v.RealMiddleBand} {v.RealLowerBand}");

// It deconstructs, too:
var (upper, middle, lower) = b.Value;
```

::: tip Equality on a value type
These are record structs, so `==` is .NET's `double` equality: `NaN` equals `NaN` **and** `+0.0` equals `-0.0`. (Java's record differs on the second.) Compare `BitConverter.DoubleToInt64Bits` per component when bit-level identity is what you mean.
:::

## Array-Fill Calls

`Open` and `Update` each write a single value. Two more calls write a full array instead — the same shape the [batch method](/api/csharp/) would produce — while still driving the handle:

| Call | When | Does |
|------|------|------|
| `core.<NAME>_OpenAndFill(..)` | once, instead of `Open` | like `Open`, but also fills the output for **every** history bar |
| `handle.UpdateAndFill(bars, outs)` | instead of a loop of `Update` | commit `n` closed bars and write the `n` values |

**`OpenAndFill`** — `Open` gives you only the value at the last history bar. `OpenAndFill` also writes the output for **every** history bar — the same values the [batch method](/api/csharp/) would produce — while still returning the live handle, in one pass:

```csharp
double[] history = /* ...your closing prices... */;
var outReal = new double[history.Length];

Core.SMA_Stream s = core.SMA_OpenAndFill(history, 30, outReal);

OutRange r = s.OutRange;    // the bars it has a value for
// outReal[0 .. r.Count - 1] == what core.SMA(0, history.Length - 1, ...) writes
// ...and s is live, ready for Update.
```

The output arguments are the batch call's, in the same order. An output may not overlap an input, or another output — that throws `ArgumentException` and mints no handle. With spans that means genuine memory overlap, not just the same buffer: two slices of one array that share even one element are rejected.

**`UpdateAndFill`** — feeding a gap one `Update` at a time works; `UpdateAndFill` does the same thing
in one call, writing one value per bar into your span:

```csharp
double[] outReal = new double[gap.Length];

s.UpdateAndFill(gap, outReal);      // outReal[i] is the SMA at gap[i]
```

It is exactly `gap.Length` back-to-back `Update` calls — same values, same state
— with one set of argument checks instead of `n`. `s.OutRange` reports the bars
the handle has a value for, before and after.

That includes a call that fails partway. A non-finite bar throws
`ArgumentException` exactly as `Update` does, which means the bars **before** it
are already committed and their values already written; the range tells you how
many. It throws before committing anything if the input spans differ in length,
an output is shorter than the bar count, or an output overlaps an input or
another output. An empty call does nothing.

## Error model

`Open` and `OpenAndFill` throw. After a successful open the only thing `Update` and `Peek` reject is a non-finite bar; `UpdateAndFill` adds ragged inputs, an output shorter than the bar count and an overlapping output, all three before it commits anything. `Value` and `Clone()` never throw.

| Condition | Exception |
|---|---|
| Fewer than `lookback + 1` history bars | `InsufficientHistoryException` |
| An optional parameter outside its documented range | `ArgumentException` |
| An output array aliasing an input or another output | `ArgumentException` |
| A non-finite bar, or a non-finite real parameter | `ArgumentException` |

One narrow exception to "the handle is unchanged": a *composed* indicator drives its sub-stages through their own public update, so a value the library computed internally is re-checked there. If such an intermediate overflowed to an infinity, the rejection would surface after earlier sub-stages had advanced, and would name the sub-stage. It needs input magnitudes around 1e306 and up — the overflow class TA-Lib already treats as out of scope — but the guarantee is stated for the caller-supplied case, which is the one you can provoke.

`InsufficientHistoryException` derives from `ArgumentException`, so you can catch it specifically — it is the one routine, data-dependent rejection — or catch every open failure uniformly. Messages carry a stable `"<NAME> open: "` prefix, and it is always the *called* function's name: `core.MA_Open(...)` rejecting reports `MA open:`, never the name of whatever moving average it delegates to.

Insufficient history is knowable in advance, so it need not be exceptional in your code: compare against `<NAME>_Lookback(params) + 1` before opening.

## Absolute-index outputs

`MININDEX`, `MAXINDEX` and `MINMAXINDEX` report bar indices. In the streaming tier those count bars fed to the stream rather than positions in an array, and the basis is shifted once that count passes 2^30 — so treat an index as a position within the current window, not as an identifier to store and compare against one read much later.

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

That is discovery only. Unlike the batch tier, there is no name-based way to *open* a stream — `FunctionCall` binds batch calls, and nothing binds streams. Opening one means calling its typed `<NAME>_Open` directly.

This is deliberate rather than an oversight, and the same is true of the other language bindings. A generic opener would have to return a handle whose type varies per function, and `Update` varies in both arity and return type, so the values would have to be boxed — which costs an allocation per bar, on the one path this whole tier exists to keep allocation-free. Worth designing properly if there is a call for it; not worth guessing at.
