---
title: Java Core API
description: "io.github.talib: a native Java port with no JNI, indicators as methods on a Core instance over double arrays, bit-identical to the reference C library."
toc: false
---

::: warning Not yet released
The Java API is not yet released. Estimated release: **Q1 2027**.
:::

The Java library is a native port of TA-Lib in the `io.github.talib` package — no JNI, pure Java. Every indicator is a method on a `Core` instance, operates on `double[]` arrays, and is **bit-identical** to the reference C library over the same inputs.

To process a live feed one bar at a time instead of a whole array, see the companion [Java Streaming API](/api/java/stream/).

## Calling a function

Each indicator takes a `startIdx`/`endIdx` range, the inputs, the optional parameters, and the caller-provided output array(s). It returns an `OutRange` describing where the valid output begins and how many values were written:

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] close = /* ...your closing prices... */;
double[] out   = new double[close.length];

OutRange r = Core.DEFAULT.SMA(
    0, close.length - 1,   // startIdx, endIdx
    close,                 // input(s)
    30,                    // optInTimePeriod
    out);                  // output(s)

// out[0 .. r.count() - 1] holds the SMA; out[i] is input bar r.begIdx() + i.
for (int i = 0; i < r.count(); i++) {
    System.out.println("bar " + (r.begIdx() + i) + " = " + out[i]);
}
```

`OutRange` is an immutable record with two components — `begIdx()` and `count()` — plus the conveniences `isEmpty()` and `EMPTY`. The component names match the C and Rust surfaces (`outBegIdx` / `outNBElement`), so the same concept reads the same way in every backend.

## Output size and lookback

An output is written only where the indicator is defined — a 30-period SMA has no value until the 30th bar. `begIdx()` is the first valid bar and `count()` is the number written; the rest of the array is left untouched, never padded with NaN. Size the output array to at least `endIdx - startIdx + 1`, or exactly with the lookback:

```java
int lookback = Core.DEFAULT.SMA_Lookback(30);    // 29 for a 30-period SMA
```

The lookback is how many inputs are consumed before the first output.

**Too little data is a success, not an error.** A valid range shorter than the lookback simply produces no values: `count()` is 0 and `isEmpty()` is true. No exception is thrown — this matches the C library's `TA_SUCCESS` with `outNBElement == 0`. Nothing is written, so the output array's length is not checked on such a call — it may even be zero-length. The input is still checked, though: an `endIdx` past the end of the series you passed is a mistake worth hearing about in any range, and an empty range would otherwise hide it behind a "no data yet" result.

## Parameters and errors

Integer parameters are `int`; real parameters are `double`; enumerated parameters use their enum type (e.g. `MAType.SMA`). Passing `Integer.MIN_VALUE` for an integer parameter, the real-default sentinel `-4e37` for a `double` parameter, or `MAType.DEFAULT` for an MA-type parameter selects that parameter's documented default.

Misuse throws rather than returning an error code:

| Mistake | Exception |
|---|---|
| `startIdx`/`endIdx` out of range, or `endIdx < startIdx` | `IndexOutOfBoundsException` |
| Optional parameter outside its documented range | `IllegalArgumentException` |
| Two outputs sharing one array | `IllegalArgumentException` |
| An array too short for the range requested | `IllegalArgumentException` |
| A null input or output array | `NullPointerException` |

Array lengths are checked before anything is written, so a rejected call leaves every buffer untouched. This is the batch API; the [streaming API](./stream/) does not check capacity — an undersized `OpenAndFill` output faults inside the fill. An input must reach `endIdx`; an output must hold the values actually produced, `endIdx - max(startIdx, lookback) + 1`. The message names the array and both sizes — `SMA: outReal has length 3, needs 191`.

## `float` inputs

Every indicator is overloaded for `float[]` inputs as well as `double[]` — the `float` overload widens each element to `double` before computing, so a result beyond `float` range still lands correctly in the `double` output. Use it to feed price data already stored as `float` without copying.

Because the two overloads differ only in the input array type, a bare `null` argument is ambiguous; cast it (`(double[]) null`) if you ever need to pass one.

## Settings and threading

**`Core` is immutable.** Every field is final and the settings it carries are deeply immutable, so one instance is safe to share across any number of threads with no synchronization — even when published racily (JLS 17.5 final-field semantics). There are no locks on any call path.

Use `Core.DEFAULT` for the all-defaults instance, or build a configured one:

```java
import io.github.talib.CandleSettingType;
import io.github.talib.Core;
import io.github.talib.FuncUnstId;
import io.github.talib.RangeType;

Core core = Core.builder()
    .unstablePeriod(FuncUnstId.EMA, 10)
    .candleSetting(CandleSettingType.BodyLong, RangeType.RealBody, 10, 1.0)
    .build();
```

There are no setters: to change a setting, derive a new instance with `core.toBuilder()`. Read a configured unstable period back with `core.unstablePeriod(FuncUnstId.EMA)` — the same name the builder writes it under, since a `Core` is immutable and has no writer to distinguish it from.
