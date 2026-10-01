# Migrating from talipp to TA-Lib

This file moves a [talipp](https://github.com/nardew/talipp) incremental-indicator pipeline to the streaming handles of ta-lib-python, the Python wrapper of TA-Lib (PyPI package `TA-Lib`, `import talib`); the notes describe talipp 2.7.0. `talib.stream.<NAME>` opens a handle on your history, then takes one bar at a time, with the same function names and parameters as the batch `talib.<NAME>`. Numerical values can differ after migrating, even for an indicator of the same name: warm-up, [seeding](https://ta-lib.org/api/unstable-period/index.md) and edge cases are each library's own choice. If a model was trained on features from the old library, recompute every training feature with the new one and retrain the model before serving it; never mix features from the two. Pass every parameter explicitly, by keyword, since the defaults differ. ta-lib-python returns NaN for the warm-up bars, where talipp holds `None`, and their number can differ from talipp's. Pin the TA-Lib version a model was trained with, and store both `talib.__version__` (the Python wrapper) and `talib.__ta_version__` (the C library it runs) with the model. Below are only API and name mappings, same-name redefinitions and outputs on another scale.

A TA-Lib function's parameters, defaults and output order are on `https://ta-lib.org/functions/<name>.md`, with the name in lowercase, for example [RSI](https://ta-lib.org/functions/rsi.md); the [function list](https://ta-lib.org/functions/index.md) has every function. Some functions named here are newer than some installed packages: `hasattr(talib.stream, "KST")` tells whether the installed package has `KST`.

## Install

```bash
pip install "TA-Lib>=0.8.1"
```

Require 0.8.1 or later: before it, `talib.stream.<NAME>` returned the last value rather than a handle, and code written for that can misread a handle silently ([CHANGELOG](https://github.com/TA-Lib/ta-lib-python/blob/master/CHANGELOG)).

## Example

Open a handle on the closed bars you have, `peek` the forming bar as it ticks, and `update` once it closes:

```python
import numpy as np
import talib
from talib import stream

rng = np.random.default_rng(0)
closes = 100 + rng.normal(0, 1, 500).cumsum().round(2)
history, live = closes[:400], closes[400:]

rsi = stream.RSI(history, timeperiod=14)   # a handle; needs at least 15 bars
print(rsi.value)                            # RSI at the last history bar

for close in live:
    for tick in (close - 0.10, close + 0.05, close):
        provisional = rsi.peek(tick)        # forming bar: nothing is committed
    rsi.update(close)                       # the bar closed: commit it

assert rsi.value == talib.RSI(closes, timeperiod=14)[-1]   # bit for bit
```

## Concept map

| talipp | ta-lib-python |
|---|---|
| `SMA(20, input_values=history)`, or start empty and `add()` | Open on at least lookback + 1 bars: `stream.SMA(history, timeperiod=20)`. Below that it raises `talib.InsufficientHistory`, which means "not yet": add a bar and open again. |
| `add(value)` for a closed bar | `h.update(value)`, which returns that bar's value |
| `add()` on a bar's first tick, `update()` on later ticks | `h.peek(value)` on every tick returns what `update` would, and commits nothing. Call `h.update(value)` once, when the bar closes. |
| `remove()` | Keep `prev = h.copy()` before each `update`; `h = prev` undoes it. |
| `ind[-1]` | `h.value` |
| The output list | Store the values you need. `stream.SMA.open_and_fill(history, timeperiod=20)` returns the handle and the batch series over the history. |
| `OHLCV` inputs, `input_modifier=` | Pass the fields the function takes, in order: arrays to open (`stream.ATR(high, low, close)`), scalars per bar (`h.update(hi, lo, cl)`). |
| `input_indicator=` | Feed one handle's value into the next: `b.update(a.update(x))`, and `b.peek(a.peek(x))` for the forming bar. Open `b` on the series from `a`'s `open_and_fill`. |
| `input_sampling=` | Bucket ticks into bars yourself: `peek` while a bucket is open, `update` once when it closes. |
| `None` input | `h.advance()` counts a bar without feeding it. `update` rejects NaN and infinity and leaves the handle unchanged. |
| `purge_oldest()` | Nothing to purge: a handle keeps only its state. |
| Pickling | Handles do not pickle. Reopen from stored history; `h.copy()` forks one within a process. |

A multi-output function returns a plain tuple in the batch function's order, which `abstract.Function('BBANDS').output_names` lists (`from talib import abstract`): `BBANDS` gives `(upperband, middleband, lowerband)` and `AROON` gives `(aroondown, aroonup)`.

## Indicator names

TA-Lib's parameter names and defaults: `abstract.Function('TSI').parameters` lists them (TSI's `firstperiod` is the long one).

These talipp classes map to a TA-Lib function of another name, or need care:

| talipp | TA-Lib | Note |
|---|---|---|
| AccuDist | [AD](https://ta-lib.org/functions/ad.md) | |
| ADX | [ADX](https://ta-lib.org/functions/adx.md) | The DI lines are [PLUS_DI](https://ta-lib.org/functions/plus_di.md) and [MINUS_DI](https://ta-lib.org/functions/minus_di.md). |
| BB | [BBANDS](https://ta-lib.org/functions/bbands.md) | |
| ChaikinOsc | [ADOSC](https://ta-lib.org/functions/adosc.md) | |
| CoppockCurve | [COPPOCK](https://ta-lib.org/functions/coppock.md) | |
| DonchianChannels | [DONCHIAN](https://ta-lib.org/functions/donchian.md) | |
| ForceIndex | [EFI](https://ta-lib.org/functions/efi.md) | |
| KeltnerChannels | [KC](https://ta-lib.org/functions/kc.md) | talipp's centre line is a moving average of the close; `KC`'s is an EMA of the typical price. For talipp's channel, open an `EMA` handle on the close and an `ATR` handle, and add or subtract the multiplier times the `ATR` value. |
| MassIndex | [MASSI](https://ta-lib.org/functions/massi.md) | `fastperiod` is both EMA periods; `slowperiod` is the summing window. |
| McGinleyDynamic | [MCGD](https://ta-lib.org/functions/mcgd.md) | |
| MeanDev | [AVGDEV](https://ta-lib.org/functions/avgdev.md) | |
| ParabolicSAR | [SAR](https://ta-lib.org/functions/sar.md) | |
| SMMA | [RMA](https://ta-lib.org/functions/rma.md) | |
| STC | [STC](https://ta-lib.org/functions/stc.md) | TA-Lib smooths both stochastic stages with a factor of 0.5, which is talipp's `stoch_smoothing_period=3` with `stoch_ma_type=MAType.EMA`; talipp's default, `MAType.SMA`, has no TA-Lib function. |
| StdDev | [STDDEV](https://ta-lib.org/functions/stddev.md) | |
| Stoch | [STOCHF](https://ta-lib.org/functions/stochf.md) | TA-Lib's [STOCH](https://ta-lib.org/functions/stoch.md) is the slow stochastic, a different line. |
| StochRSI | [STOCHRSI](https://ta-lib.org/functions/stochrsi.md) | With `k_smoothing_period` above 1, open [STOCH](https://ta-lib.org/functions/stoch.md) on the RSI series from `stream.RSI.open_and_fill`, passed as high, low and close: `fastk_period` is `stoch_period`, `slowk_period` is `k_smoothing_period`, `slowd_period` is `d_smoothing_period`. |
| TRIX | [TRIX](https://ta-lib.org/functions/trix.md) | 100 times smaller in TA-Lib: multiply by 100 for talipp's scale. |
| UO | [ULTOSC](https://ta-lib.org/functions/ultosc.md) | |
| VTX | [VORTEX](https://ta-lib.org/functions/vortex.md) | |
| Williams | [WILLR](https://ta-lib.org/functions/willr.md) | |

These talipp indicators have a TA-Lib function of the same name: ALMA, AO, Aroon, ATR, BOP, CCI, CHOP, DEMA, DPO, EMA, EMV, HMA, IBS, KAMA, KST, MACD, NATR, OBV, ROC, RSI, SMA, SuperTrend, T3, TEMA, TSI, VWAP, VWMA, WMA and ZLEMA.

Where TA-Lib fixes a setting that talipp exposes, such as the moving-average type, a talipp value other than TA-Lib's has no single-function equivalent; the function's page gives TA-Lib's value. Two exceptions: [MACDEXT](https://ta-lib.org/functions/macdext.md) takes a moving-average type for each MACD line, and [SAREXT](https://ta-lib.org/functions/sarext.md) takes ParabolicSAR's initial acceleration separately and returns the stop negated while short.

Compose these from several handles: SOBV (SMA over OBV), SFX (ATR, STDDEV, SMA), ChandeKrollStop (MAX, MIN, ATR), Ichimoku (MIDPRICE; keep your own buffer for the displaced lines) and RogersSatchell (SUM over your own per-bar term).

No TA-Lib function: KVO, PivotsHL, TTM, ZigZag.

## Training and live features

A handle opened on the same history as a batch call, then fed the same bars, returns the batch function's values bit for bit, so features computed in batch for training equal the ones a live handle produces. Keep that history: a handle does not pickle, so after a restart reopen it on the stored bars, starting from the same first bar as the training batch.

## Related

- [C/C++ Streaming API](https://ta-lib.org/api/stream/index.md): the C streams the Python handles wrap
- [Functions](https://ta-lib.org/functions/index.md): every TA-Lib function with its parameters, defaults and outputs
- [Wrappers](https://ta-lib.org/wrappers/index.md): the wrappers, ta-lib-python among them
