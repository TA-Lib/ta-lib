# Migrating from ta to TA-Lib

This file moves Python code from the [ta](https://pypi.org/project/ta/) package (bukosabino/ta) to ta-lib-python, the Python wrapper of TA-Lib (PyPI package `TA-Lib`, `import talib`); the map describes ta 0.11.0. Numerical values can differ after migrating, even for an indicator of the same name: warm-up, [seeding](https://ta-lib.org/api/unstable-period/index.md) and edge cases are each library's own choice. If a model was trained on features from the old library, recompute every training feature with the new one and retrain the model before serving it; never mix features from the two. Pass every parameter explicitly, including the moving-average type (`matype`), since the defaults differ. ta-lib-python returns NaN for the warm-up bars, and their number can differ from the old library's, so `dropna()` can keep different rows. Pin the TA-Lib version a model was trained with, and store both `talib.__version__` (the Python wrapper) and `talib.__ta_version__` (the C library it runs) with the model. Below are only name mappings, same-name redefinitions and outputs on another scale.

A TA-Lib function's parameters, defaults and output order are on `https://ta-lib.org/functions/<name>.md`, with the name in lowercase, for example [RSI](https://ta-lib.org/functions/rsi.md); the [function list](https://ta-lib.org/functions/index.md) has every function. Some functions named here are newer than some installed packages: `hasattr(talib, "STC")` tells whether the installed package has `STC`.

## Install

```bash
pip install "TA-Lib>=0.8.1"
```

## Calling pattern

In ta, an indicator class computes one indicator and returns each output from a method; a module function returns one output. In ta-lib-python, one function computes an indicator and returns all its outputs at once: a single output as a Series (a NumPy array, given arrays), several as a tuple in the order the function's page lists them.

Below, `df` is a pandas DataFrame with `open`, `high`, `low`, `close` and `volume` columns.

ta:

```python
from ta.momentum import RSIIndicator
from ta.volatility import BollingerBands

df["rsi"] = RSIIndicator(close=df["close"], window=14).rsi()
bb = BollingerBands(close=df["close"], window=20, window_dev=2)
df["bb_high"] = bb.bollinger_hband()
df["bb_low"] = bb.bollinger_lband()
```

TA-Lib:

```python
import talib

df["rsi"] = talib.RSI(df["close"], timeperiod=14)
upper, middle, lower = talib.BBANDS(df["close"], timeperiod=20, nbdevup=2, nbdevdn=2, matype=0)
df["bb_high"] = upper
df["bb_low"] = lower
```

TA-Lib fixes some constants that ta takes as parameters: KAMA's fast and slow periods (2 and 30), CCI's 0.015, ULTOSC's 4, 2, 1 weights and STC's 3-bar smoothing. If a ta call changes one of them, no TA-Lib function computes that indicator.

## Names that mean something else

Same name, different indicator:

- ta's `VolumeWeightedAveragePrice` averages over a rolling window (14 bars by default). TA-Lib's [VWAP](https://ta-lib.org/functions/vwap.md) accumulates from the first bar; the rolling form is [VWMA](https://ta-lib.org/functions/vwma.md) of the typical price.
- ta's `KeltnerChannel` defaults to Keltner's original form: a simple average of the typical price, with bands from the bar range. TA-Lib's [KC](https://ta-lib.org/functions/kc.md) is an EMA of the typical price with ATR bands, so use the composition in the map. With `original_version=False`, the middle band is `EMA(close, window)` and the upper band `EMA(close, window) + multiplier * ATR(high, low, close, window_atr)` (subtract for the lower); that is not `KC` either.
- ta's `StochasticOscillator` is the fast stochastic, TA-Lib's [STOCHF](https://ta-lib.org/functions/stochf.md). TA-Lib's [STOCH](https://ta-lib.org/functions/stoch.md) returns the slow %K and %D.

Same indicator, other names:

- ta's `stochrsi_k()` is TA-Lib's `fastd`, and `stochrsi_d()` is one more SMA on top of it.
- TA-Lib's `AROON` returns the down line first.

Another scale:

- ta's StochRSI outputs run from 0 to 1, TA-Lib's [STOCHRSI](https://ta-lib.org/functions/stochrsi.md) from 0 to 100.
- ta's ease of movement is scaled by 1e8. TA-Lib's [EMV](https://ta-lib.org/functions/emv.md) divides volume by its volume divisor, 10000 by default: pass 1e8 for ta's scale.

## Name map

Every name in the TA-Lib column is in the `talib` module; `high`, `low`, `close` and `volume` are Series. The numbers are ta 0.11.0's class defaults, in TA-Lib's argument order: use the windows your code passes. A bold row is code to run once; the rows under it use the names it defines. A ta module function such as `ta.momentum.rsi()` maps to the row of the output it returns; when the code passes it no window, take the default from that function's signature, since a few differ from their class's.

### ta.volume

| ta | TA-Lib |
|---|---|
| `AccDistIndexIndicator.acc_dist_index()` | `AD(high, low, close, volume)` |
| `OnBalanceVolumeIndicator.on_balance_volume()` | `OBV(close, volume)` |
| `ChaikinMoneyFlowIndicator.chaikin_money_flow()` | `CMF(high, low, close, volume, 20)` |
| `ForceIndexIndicator.force_index()` | `EFI(close, volume, 13)` |
| **`EaseOfMovementIndicator`** | `em = (MOM(MEDPRICE(high, low), 1) * (high - low) * 1e8 / volume).where(volume > 0, 0)` |
| `.ease_of_movement()` | `em`, or `EMV(high, low, volume, 1, 1e8)` |
| `.sma_ease_of_movement()` | `SMA(em, 14)`, or `EMV(high, low, volume, 14, 1e8)` |
| `VolumePriceTrendIndicator.volume_price_trend()` | `PVT(close, volume)` |
| `VolumeWeightedAveragePrice.volume_weighted_average_price()` | `VWMA(TYPPRICE(high, low, close), volume, 14)` |
| `MFIIndicator.money_flow_index()` | `MFI(high, low, close, volume, 14)` |
| `NegativeVolumeIndexIndicator.negative_volume_index()` | `NVI(close, volume)` |

### ta.volatility

| ta | TA-Lib |
|---|---|
| `AverageTrueRange.average_true_range()` | `ATR(high, low, close, 14)` |
| **`BollingerBands`** | `upper, middle, lower = BBANDS(close, 20, 2, 2, 0)` |
| `.bollinger_hband()` | `upper` |
| `.bollinger_mavg()` | `middle` |
| `.bollinger_lband()` | `lower` |
| `.bollinger_wband()` | `(upper - lower) / middle * 100`, or `BBW(close, 20, 2, 2, 0)` |
| `.bollinger_pband()` | `(close - lower) / (upper - lower)`, or `PERCENTB(close, 20, 2, 2, 0)` |
| `.bollinger_hband_indicator()` | `(close > upper).astype(float)` |
| `.bollinger_lband_indicator()` | `(close < lower).astype(float)` |
| **`KeltnerChannel`** | `middle = SMA(TYPPRICE(high, low, close), 20); upper = middle + SMA(high - low, 20); lower = middle - SMA(high - low, 20)` |
| `.keltner_channel_hband()` | `upper` |
| `.keltner_channel_mband()` | `middle` |
| `.keltner_channel_lband()` | `lower` |
| `.keltner_channel_wband()` | `(upper - lower) / middle * 100` |
| `.keltner_channel_pband()` | `(close - lower) / (upper - lower)` |
| `.keltner_channel_hband_indicator()` | `(close > upper).astype(float)` |
| `.keltner_channel_lband_indicator()` | `(close < lower).astype(float)` |
| **`DonchianChannel`** | `upper, middle, lower = DONCHIAN(high, low, 20)` |
| `.donchian_channel_hband()` | `upper` |
| `.donchian_channel_mband()` | `middle` |
| `.donchian_channel_lband()` | `lower` |
| `.donchian_channel_wband()` | `(upper - lower) / SMA(close, 20) * 100` |
| `.donchian_channel_pband()` | `(close - lower) / (upper - lower)` |
| **`UlcerIndex`** | `drawdown = 100 * (close - MAX(close, 14)) / MAX(close, 14)` |
| `.ulcer_index()` | `SQRT(SUM(drawdown * drawdown, 14) / 14)` |

### ta.trend

| ta | TA-Lib |
|---|---|
| **`MACD`** | `macd, signal, hist = MACD(close, 12, 26, 9)` |
| `.macd()` | `macd` |
| `.macd_signal()` | `signal` |
| `.macd_diff()` | `hist` |
| `SMAIndicator.sma_indicator()` | `SMA(close, window)` |
| `EMAIndicator.ema_indicator()` | `EMA(close, 14)` |
| `WMAIndicator.wma()` | `WMA(close, 9)` |
| **`VortexIndicator`** | `plus, minus = VORTEX(high, low, close, 14)` |
| `.vortex_indicator_pos()` | `plus` |
| `.vortex_indicator_neg()` | `minus` |
| `.vortex_indicator_diff()` | `plus - minus` |
| `TRIXIndicator.trix()` | `TRIX(close, 15)` |
| `MassIndex.mass_index()` | `MASSI(high, low, 9, 25)` |
| `DPOIndicator.dpo()` | `DPO(close, 20)` |
| **`KSTIndicator`** | `kst = SMA(ROC(close, 10), 10) + 2 * SMA(ROC(close, 15), 10) + 3 * SMA(ROC(close, 20), 10) + 4 * SMA(ROC(close, 30), 15); signal = SMA(kst, 9)`, or `kst, signal = KST(close, 10, 15, 20, 30, 10, 10, 10, 15, 9)` |
| `.kst()` | `kst` |
| `.kst_sig()` | `signal` |
| `.kst_diff()` | `kst - signal` |
| **`IchimokuIndicator`** | `conversion = MIDPRICE(high, low, 9); base = MIDPRICE(high, low, 26)` |
| `.ichimoku_conversion_line()` | `conversion` |
| `.ichimoku_base_line()` | `base` |
| `.ichimoku_a()` | `(conversion + base) / 2` |
| `.ichimoku_b()` | `MIDPRICE(high, low, 52)` |
| `STCIndicator.stc()` | `STC(close, 23, 50, 10)` |
| `ADXIndicator.adx()` | `ADX(high, low, close, 14)` |
| `ADXIndicator.adx_pos()` | `PLUS_DI(high, low, close, 14)` |
| `ADXIndicator.adx_neg()` | `MINUS_DI(high, low, close, 14)` |
| `CCIIndicator.cci()` | `CCI(high, low, close, 20)` |
| **`AroonIndicator`** | `down, up = AROON(high, low, 25)` |
| `.aroon_up()` | `up` |
| `.aroon_down()` | `down` |
| `.aroon_indicator()` | `AROONOSC(high, low, 25)` |
| **`PSARIndicator`** | `sar = SAR(high, low, 0.02, 0.2); rising = sar.where(sar < close); falling = sar.where(sar > close)` |
| `.psar()` | `sar` |
| `.psar_up()` | `rising` |
| `.psar_down()` | `falling` |
| `.psar_up_indicator()` | `(rising.notna() & rising.shift().isna()).astype(float)` |
| `.psar_down_indicator()` | `(falling.notna() & falling.shift().isna()).astype(float)` |

With `IchimokuIndicator(visual=True)`, shift the span A and span B lines forward by the base-line window: `.shift(window2)`, 26 by default.

### ta.momentum

| ta | TA-Lib |
|---|---|
| `RSIIndicator.rsi()` | `RSI(close, 14)` |
| **`StochRSIIndicator`** | `fastk, fastd = STOCHRSI(close, 14, 14, 3, 0)` |
| `.stochrsi()` | `fastk / 100` |
| `.stochrsi_k()` | `fastd / 100` |
| `.stochrsi_d()` | `SMA(fastd, 3) / 100` |
| `TSIIndicator.tsi()` | `TSI(close, 25, 13)` |
| `UltimateOscillator.ultimate_oscillator()` | `ULTOSC(high, low, close, 7, 14, 28)` |
| **`StochasticOscillator`** | `fastk, fastd = STOCHF(high, low, close, 14, 3, 0)` |
| `.stoch()` | `fastk` |
| `.stoch_signal()` | `fastd` |
| `KAMAIndicator.kama()` | `KAMA(close, 10)` |
| `ROCIndicator.roc()` | `ROC(close, 12)` |
| `AwesomeOscillatorIndicator.awesome_oscillator()` | `AO(high, low, 5, 34)` |
| `WilliamsRIndicator.williams_r()` | `WILLR(high, low, close, 14)` |
| **`PercentagePriceOscillator`** | `ppo = PPO(close, 12, 26, 1)` |
| `.ppo()` | `ppo` |
| `.ppo_signal()` | `EMA(ppo, 9)` |
| `.ppo_hist()` | `ppo - EMA(ppo, 9)` |
| **`PercentageVolumeOscillator`** | `pvo = PVO(volume, 12, 26, 1)` |
| `.pvo()` | `pvo` |
| `.pvo_signal()` | `EMA(pvo, 9)` |
| `.pvo_hist()` | `pvo - EMA(pvo, 9)` |

### ta.others

| ta | TA-Lib |
|---|---|
| `DailyReturnIndicator.daily_return()` | `ROC(close, 1)` |
| `DailyLogReturnIndicator.daily_log_return()` | `LN(ROCR(close, 1)) * 100` |
| `CumulativeReturnIndicator.cumulative_return()` | none: `(close / close.iloc[0] - 1) * 100` |

## Replacing add_all_ta_features

TA-Lib has no single call that adds every indicator. List the features you use, each as a column name, a function, its input columns and its parameters, and build the frame in a loop. Keeping ta's column names leaves the rest of the pipeline's code unchanged.

```python
import pandas as pd
import talib

# (column or tuple of columns, function, input columns, parameters)
FEATURES = [
    ("volume_mfi", talib.MFI, ("high", "low", "close", "volume"), {"timeperiod": 14}),
    ("volatility_atr", talib.ATR, ("high", "low", "close"), {"timeperiod": 10}),
    (("volatility_bbh", "volatility_bbm", "volatility_bbl"), talib.BBANDS, ("close",),
     {"timeperiod": 20, "nbdevup": 2, "nbdevdn": 2, "matype": 0}),
    (("trend_macd", "trend_macd_signal", "trend_macd_diff"), talib.MACD, ("close",),
     {"fastperiod": 12, "slowperiod": 26, "signalperiod": 9}),
    (("trend_aroon_down", "trend_aroon_up"), talib.AROON, ("high", "low"), {"timeperiod": 25}),
    ("momentum_rsi", talib.RSI, ("close",), {"timeperiod": 14}),
    ("momentum_roc", talib.ROC, ("close",), {"timeperiod": 12}),
]


def talib_features(df, features=FEATURES):
    prices = df[["open", "high", "low", "close", "volume"]].astype(float)
    out = {}
    for columns, function, inputs, params in features:
        result = function(*(prices[name] for name in inputs), **params)
        if isinstance(columns, tuple):
            out.update(zip(columns, result))
        else:
            out[columns] = result
    return pd.DataFrame(out, index=df.index)


df = df.join(talib_features(df))
```

Write every parameter into the list, taking each window from ta's `add_all_ta_features` source rather than from the map: it passes its own windows to some classes (as of ta 0.11.0, 10 bars for ATR and the Keltner Channel, whose classes default to 14 and 20). Add the compositions from the map after the loop, for example `df["momentum_stoch_rsi"] = talib.STOCHRSI(df["close"], 14, 14, 3, 0)[0] / 100`.

## Related

- [Functions](https://ta-lib.org/functions/index.md): every TA-Lib function with its parameters, defaults and outputs
- [Unstable Period](https://ta-lib.org/api/unstable-period/index.md): how TA-Lib seeds and warms up recursive indicators
- [Wrappers](https://ta-lib.org/wrappers/index.md): the wrappers, ta-lib-python among them
