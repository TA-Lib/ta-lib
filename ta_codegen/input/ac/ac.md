# AC

## Summary

Bill Williams' Accelerator/Decelerator Oscillator (*New Trading Dimensions*, 1998): the rate at which market momentum is itself speeding up or slowing down. Where the Awesome Oscillator ([`AO`](/functions/ao)) measures momentum, this measures the change in that momentum, by taking the oscillator's distance above or below its own moving average.

Because acceleration turns before speed does, the reading changes sign ahead of the oscillator it is built from — it is meant as the early half of a pair, not as a signal on its own. Above zero acceleration is with the bulls, below zero with the bears, and it is drawn as a zero-centred histogram whose colour convention is the bar-to-bar change: rising bars accelerating, falling bars decelerating. Williams' rule of thumb is that two same-coloured bars are what confirms the turn, which is why the sign and the direction matter more than the level.

The oscillator is one leg of Williams' Profitunity system, alongside the Awesome Oscillator ([`AO`](/functions/ao)) and the Alligator.

## Formula

median_t = ( high_t + low_t ) / 2  
AO_t = SMA(median, fast)_t − SMA(median, slow)_t  
AC_t = AO_t − SMA(AO, signal)_t

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Distance of the Awesome Oscillator ([`AO`](/functions/ao)) from its own moving average, centred on zero

## Parameters

- `optInFastPeriod` — Number of bars in the short moving average of the median price.
- `optInSlowPeriod` — Number of bars in the long moving average of the median price.
- `optInSignalPeriod` — Number of bars in the moving average taken over the oscillator.

## Implementation

TA-Lib Definition: [`ac.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ac/ac.c) · [`ac.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ac/ac.yaml)

| Native | File |
|--------|------|
| C | [`ta_AC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AC.c) |
| Rust | [`ac.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ac.rs) |
| Java | [`Core_AC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AC.java) |
| C# | [`Core_AC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_AC.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Accelerator Oscillator, Decelerator Oscillator, Accelerator/Decelerator, Bill Williams Accelerator, AC Oscillator

## See Also

AO · MACD · MEDPRICE · PPO · SMA

## References

- Bill Williams, *New Trading Dimensions*, Wiley, 1998, and *Trading Chaos*, define the Accelerator/Decelerator as the Awesome Oscillator ([`AO`](/functions/ao)) less the 5-period simple moving average of that oscillator.
- trading-signals 8.3.0 (`momentum/AC`) computes the same form on the same inputs and reports its first value at the same bar. Its moving average re-sums the stored window on every bar where TA-Lib rolls a running total, so the two agree to rounding rather than to the bit.
