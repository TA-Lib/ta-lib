---
title: Java Core API
toc: false
---

# Java Core API

::: warning Not yet released
The Java API is not yet released. Estimated release: **Q1 2027**.
:::

The Java library is a native port of TA-Lib in the `com.tictactec.ta.lib` package — no JNI, pure Java. Every indicator is a method on a `Core` instance, operates on `double[]` arrays, and is **bit-identical** to the C library over the same inputs.

To process a live feed one bar at a time instead of a whole array, see the companion [Java Streaming API](/api/java/stream/).

## Calling a function

Each indicator takes a `startIdx`/`endIdx` range, the inputs, the optional parameters, two `MInteger` boxes reporting where the valid output begins and how many were written, and the caller-provided output array(s):

```java
import com.tictactec.ta.lib.Core;
import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.RetCode;

Core core = new Core();                 // create once, reuse

double[] close = /* ...your closing prices... */;
double[] out   = new double[close.length];
MInteger begIdx      = new MInteger();
MInteger nbElement   = new MInteger();

RetCode rc = core.sma(
    0, close.length - 1,   // startIdx, endIdx
    close,                 // input(s)
    30,                    // optInTimePeriod
    begIdx, nbElement,     // where valid output starts, how many are valid
    out);                  // output(s)

// out[0 .. nbElement.value - 1] holds the SMA; out[i] is input bar begIdx.value + i.
for (int i = 0; i < nbElement.value; i++) {
    System.out.println("bar " + (begIdx.value + i) + " = " + out[i]);
}
```

`MInteger` is a tiny mutable box (`public int value`) — the library's out-parameter idiom, since Java cannot return by reference.

## Output size and lookback

An output is written only where the indicator is defined — a 30-period SMA has no value until the 30th bar. `begIdx.value` (`outBegIdx`) is the first valid bar and `nbElement.value` (`outNBElement`) is the count written; the rest of the array is left untouched. Size the output array to at least `endIdx - startIdx + 1`, or exactly with the lookback:

```java
int lookback = core.smaLookback(30);    // 29 for a 30-period SMA
```

The lookback is how many inputs are consumed before the first output. If there is too little data to produce even one value, `nbElement.value` is 0.

## Parameters and return codes

Integer parameters are `int`; real parameters are `double`; enumerated parameters use their enum type (e.g. `MAType.Sma`). Passing `Integer.MIN_VALUE` for an integer parameter (or the real-default sentinel `-4e37` for a `double` parameter) selects that parameter's documented default.

Every method returns a `RetCode`: `Success`, `BadParam`, `OutOfRangeStartIndex`, `OutOfRangeEndIndex`, `AllocErr`, or `InternalError`.

## `float` inputs

Every indicator is overloaded for `float[]` inputs as well as `double[]` — the `float` overload widens each element to `double` internally, so both produce identical output. Use it to feed price data already stored as `float` without copying.

## Settings and threading

Library settings live on the `Core` instance:

```java
core.SetUnstablePeriod(FuncUnstId.Ema, 10);
core.SetCompatibility(Compatibility.Metastock);
core.SetCandleSettings(/* ... */);
```

Do all such configuration **once, from a single thread**, before sharing the `Core`. After that, indicator calls on that instance are safe to make concurrently. To change a setting while streams or concurrent calls are live, use a separate `Core` instance.
