# TA-Lib for .NET

[TA-Lib](https://ta-lib.org), the widely used technical-analysis library, as a pure managed
.NET library: 200+ indicators covering moving averages, momentum oscillators (RSI, MACD,
Stochastic), volatility (Bollinger Bands, ATR), volume, Hilbert Transform cycle analysis,
statistics, price transforms and candlestick patterns.

Every function is generated from the same canonical definitions as the C library and is
bit-identical to the C reference implementation over the same inputs. No native binaries, no
P/Invoke, no dependencies.

## Quick start

```csharp
using TALib;

var core = Core.Default;

double[] close = [ /* ...your closing prices... */ ];
var outReal = new double[close.Length];

OutRange r = core.Sma(0, close.Length - 1, close, 30, outReal);

// outReal[0 .. r.Count - 1] holds the SMA; outReal[i] is input bar r.BegIdx + i.
for (int i = 0; i < r.Count; i++)
    Console.WriteLine($"bar {r.BegIdx + i} = {outReal[i]}");
```

Every indicator is a method on `Core` with the same calling pattern: a `startIdx`/`endIdx`
range, input spans, parameters, caller-provided output spans, and an `OutRange` saying where
the values start and how many there are. A range that ends inside the lookback is a success
with no values, not an error. Invalid arguments throw.

## Configuration

`Core` is immutable. The value-affecting settings, the unstable period and the candlestick
thresholds, are chosen up front and then frozen, so one `Core` can be shared across threads:

```csharp
Core tuned = Core.Builder()
    .UnstablePeriod(FuncUnstId.EMA, 10)
    .Build();
```

## Live data

For a feed that arrives one bar at a time, each indicator also has a streaming form: open it
on the history you already have, then each closed bar in gives that bar's value out,
allocation-free and bit-identical to the batch call.

```csharp
Core.SmaStream s = core.SmaOpen(close, 30);

double v = s.Update(newClose);            // a closed bar: commits it
double provisional = s.Peek(formingClose); // the forming bar: commits nothing
```

## Documentation

- C# guide: <https://ta-lib.org/api/csharp/>, and the streaming tier: <https://ta-lib.org/api/csharp/stream/>
- Per-function reference (formulas, notes, sources): <https://ta-lib.org/functions/>
- Source: <https://github.com/TA-Lib/ta-lib>

## License

BSD-3-Clause, see [LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
