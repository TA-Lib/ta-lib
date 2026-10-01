# Migrating from pandas-ta to TA-Lib

This file moves Python code from pandas-ta to ta-lib-python, the Python wrapper of TA-Lib (PyPI package `TA-Lib`, `import talib`). Numerical values can differ after migrating, even for an indicator of the same name: warm-up, [seeding](https://ta-lib.org/api/unstable-period/index.md) and edge cases are each library's own choice. If a model was trained on features from the old library, recompute every training feature with the new one and retrain the model before serving it; never mix features from the two. Pass every parameter explicitly, including the moving-average type (`matype`), since the defaults differ. ta-lib-python returns NaN for the warm-up bars, and their number can differ from the old library's, so `dropna()` can keep different rows. Pin the TA-Lib version a model was trained with, and store both `talib.__version__` (the Python wrapper) and `talib.__ta_version__` (the C library it runs) with the model. Below are only name mappings, same-name redefinitions, outputs on another scale and behaviour that switches the engine computing the values.

A TA-Lib function's parameters, defaults and output order are on `https://ta-lib.org/functions/<name>.md`, with the name in lowercase, for example [RSI](https://ta-lib.org/functions/rsi.md); the [function list](https://ta-lib.org/functions/index.md) has every function. Some functions named here are newer than some installed packages: `hasattr(talib, "ALMA")` tells whether the installed package has `ALMA`.

## Two paths

- Keep the `df.ta` API: switch to pandas-ta-classic.
- Call TA-Lib directly, for the indicators TA-Lib has.

The examples use `df`, a pandas DataFrame of bars indexed by timestamp, with lowercase `open`, `high`, `low`, `close` and `volume` columns.

## Keep the df.ta API with pandas-ta-classic

```bash
pip install pandas-ta-classic
```

```python
import pandas_ta_classic as ta  # was: import pandas_ta as ta

df.ta.rsi(length=14, append=True)
```

As of pandas-ta-classic 0.8.32, code written for pandas-ta 0.3.x mostly needs only the new import line. Code written for pandas-ta 0.4.x must also rename `study`, `Study`, `AllStudy` and `CommonStudy` to `strategy`, `Strategy`, `AllStrategy` and `CommonStrategy`; some 0.4.x indicators and parameter names do not exist in pandas-ta-classic.

Engine switching: pandas-ta computed with TA-Lib automatically whenever TA-Lib was installed. As of pandas-ta-classic 0.8.32, an indicator computes with its own code unless the call passes `talib=True`, so values can change even with TA-Lib installed. To compute with TA-Lib, install it and pass the flag on each call:

```python
rsi = df.ta.rsi(length=14, talib=True)
```

The flag can change the values of many indicators, and changes the definition of `cmo` and `stochrsi`: see them in the name map.

## Call TA-Lib directly

```bash
pip install "TA-Lib>=0.8.1"
```

ta-lib-python takes a pandas Series and returns a Series on the same index:

```python
import talib

close = df["close"]
df["rsi_14"] = talib.RSI(close, timeperiod=14)
upper, middle, lower = talib.BBANDS(close, timeperiod=20, nbdevup=2.0, nbdevdn=2.0, matype=talib.MA_Type.SMA)
```

The abstract API takes the whole DataFrame and reads its `open`, `high`, `low`, `close` and `volume` columns. A multi-output function returns a DataFrame:

```python
from talib import abstract

df["adx_14"] = abstract.ADX(df, timeperiod=14)
df = df.join(abstract.MACD(df, fastperiod=12, slowperiod=26, signalperiod=9))  # macd, macdsignal, macdhist
```

- The abstract API needs lowercase column names; it does not find `Close`. Rename first: `df = df.rename(columns=str.lower)`.
- Outputs come in TA-Lib's order, which each function's page lists: bands come back as upper, middle, lower.

## Name map

When TA-Lib has an indicator, it usually has pandas-ta's name in capitals: `rsi` is `RSI`. The table lists the other names, and the same names whose meaning or scale differs; the next section lists the outputs whose meaning, columns or scale change with pandas-ta's engine. An indicator that is not in the [function list](https://ta-lib.org/functions/index.md), or not in the installed package, stays on pandas-ta-classic. Notes on pandas-ta describe 0.3.14b0 and 0.4.71b0; notes on pandas-ta-classic describe 0.8.32.

| pandas-ta | TA-Lib | Note |
| --- | --- | --- |
| `adx` | `ADX`, `PLUS_DI`, `MINUS_DI` | pandas-ta 0.4.x's `ADXR` column averages `ADX` with its value `adxr_length` bars earlier, 2 by default; `ADXR(timeperiod=n)` averages it with its value n - 1 bars earlier. |
| `alma` | `ALMA` | With the default distribution offset (0.85), the `alma` of pandas-ta 0.3.x and of pandas-ta-classic gives the oldest bars of the window the most weight; `ALMA`, like pandas-ta 0.4.x's `alma`, gives the newest bars the most. |
| `aroon` | `AROON`, `AROONOSC` | |
| `bbands` | `BBANDS`; its `BBB` and `BBP` columns: `BBW`, `PERCENTB` | |
| `cg` | `CG` | pandas-ta 0.4.x's `cg` is `-(length + 1) - CG`, so it moves the opposite way. |
| `cksp` | `CKSP` | pandas-ta's `CKSPl`, computed from highs, is `CKSP`'s first output (`outHighStop`); `CKSPs`, computed from lows, is its second (`outLowStop`). |
| `cmo` | `CMO` or `CMOU` | pandas-ta's `cmo` is `CMO`. pandas-ta-classic's `cmo` is `CMOU`, a different indicator, and `CMO` with `talib=True`. |
| `crsi` (0.4.x) | none | `CRSI`'s streak is the signed length of the current run of higher or lower closes; pandas-ta's streak is the sign of each bar's change. pandas-ta-classic has no `crsi`. |
| `dm` | `PLUS_DM`, `MINUS_DM` | |
| `dpo` | `DPO` | pandas-ta's default (`centered=True`) is this series moved `length // 2 + 1` bars earlier, so each value uses later bars. |
| `eom` | `EMV` | pandas-ta's `divisor` defaults to 100000000; `EMV`'s volume divisor defaults to 10000. |
| `hl2` | `MEDPRICE` | |
| `hlc3` | `TYPPRICE` | |
| `kc` | `EMA`, `TRANGE` | Not `KC`, which centres on an EMA of the typical price with ATR bands. pandas-ta's middle line is `EMA(close, length)`, with bands `scalar * EMA(TRANGE(high, low, close), length)` above and below it. |
| `kst` | `KST` | pandas-ta's is 100 times TA-Lib's. |
| `linreg` | `LINEARREG`; with `angle`, `intercept`, `slope` or `tsf`: `LINEARREG_ANGLE`, `LINEARREG_INTERCEPT`, `LINEARREG_SLOPE`, `TSF` | `LINEARREG_ANGLE` is in degrees. In pandas-ta 0.3.x each value is one bar earlier on the same line: its `linreg` is `LINEARREG - LINEARREG_SLOPE`, and its `tsf=True` is `LINEARREG`. |
| `mad` | `AVGDEV` | |
| `nvi`, `pvi` | `NVI`, `PVI` | A different formula: TA-Lib compounds each bar's change; pandas-ta does not. |
| `ohlc4` | `AVGPRICE` | |
| `percent_return` | `ROCP` | |
| `psar` | `SAR` | |
| `pvt` | `PVT` | pandas-ta's is 100 times TA-Lib's. |
| `rvi` | `RVI` | pandas-ta smooths with an EMA by default and takes its standard deviation over `length` bars: `rvi(length=n, mamode="rma")` is `RVI(close, timeperiod=n, stddevperiod=n)`. |
| `smi` | `TSI` | pandas-ta's `smi` line is `TSI(close, firstperiod=slow, secondperiod=fast) / 100`. TA-Lib's `SMI` is the Stochastic Momentum Index, a different indicator. |
| `smma` (0.4.x) | `RMA` | |
| `stc` | none | `STC` recomputes its first stochastic whenever the MACD's range over the cycle is not zero; pandas-ta's `stc` recomputes it only while the lowest MACD in that window is above 0, and holds it otherwise. |
| `stdev` | `STDDEV` | |
| `stochrsi` | `STOCHRSI` | pandas-ta's `k` line is the `fastd` of `STOCHRSI(close, timeperiod=rsi_length, fastk_period=length, fastd_period=k)`; its `d` line is `SMA(fastd, d)`. pandas-ta-classic's lines are the same, but with `talib=True` they are `STOCHRSI`'s `fastk` and `fastd`. |
| `true_range` | `TRANGE` | |
| `uo` | `ULTOSC` | |
| `variance` | `VAR` | |
| `vidya` | `VIDYA` | pandas-ta's `length` is also the CMO period: `vidya(length=n)` is `VIDYA(close, timeperiod=n, cmoperiod=n)`. |
| `vwap` | `VWAP` | `VWAP` accumulates over its whole input; pandas-ta restarts at each `anchor` period, daily by default. For one value per session: `pd.concat(abstract.VWAP(g) for _, g in df.groupby(df.index.date))`. |
| `wcp` | `WCLPRICE` | |
| `zlma` | `ZLEMA` | |

## pandas-ta's two engines

pandas-ta 0.3.x and 0.4.x compute an indicator with TA-Lib whenever TA-Lib is installed, and with their own code otherwise, so one call can return different values on two machines. Find out which applied where the model was trained. Many outputs change value with the engine; these change meaning, columns or scale:

- `adx` (0.4.x): with TA-Lib, the `DMP` and `DMN` columns hold `PLUS_DM` and `MINUS_DM`; without it, `PLUS_DI` and `MINUS_DI`.
- `cci` (0.4.x): without TA-Lib, `cci` is `TYPPRICE - SMA(TYPPRICE, length) / (c * AVGDEV(TYPPRICE, length))`, where `c` is its 0.015 constant; that is not `CCI`.
- `dm`: without TA-Lib, `PLUS_DM` and `MINUS_DM` are `length` times pandas-ta's values.
- `linreg`: `angle=True` returns radians unless `degrees=True` is passed, except in 0.4.x with TA-Lib, which returns degrees. In 0.4.x without TA-Lib, `tsf=True` is `LINEARREG - LINEARREG_SLOPE` and `intercept=True` is `LINEARREG_INTERCEPT - LINEARREG_SLOPE`.
- `mama` (0.4.x): without TA-Lib, `mama` is not `MAMA`.
- `vidya` (0.4.x): with TA-Lib, `vidya` takes its CMO from `CMO`, which TA-Lib smooths, and no TA-Lib function computes it; without TA-Lib it is `VIDYA` as mapped above.
- `wcp` (0.4.x): without TA-Lib, pandas-ta's is 4 times `WCLPRICE`.

## Related

- [Functions](https://ta-lib.org/functions/index.md): every TA-Lib function with its parameters, defaults and outputs
- [Unstable Period](https://ta-lib.org/api/unstable-period/index.md): how TA-Lib seeds and warms up recursive indicators
- [Wrappers](https://ta-lib.org/wrappers/index.md): the wrappers, ta-lib-python and pandas-ta-classic among them
