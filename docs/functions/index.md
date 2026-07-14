---
title: Functions
description: "All TA-Lib technical analysis functions, grouped by category."
---

# TA-Lib Functions

All technical-analysis functions, grouped by category. Each page documents the formula, inputs, outputs, and links to the C / Rust / Java source.

## Cycle Indicators

- [HT_DCPERIOD](/functions/ht_dcperiod.md) — Hilbert Transform - Dominant Cycle Period
- [HT_DCPHASE](/functions/ht_dcphase.md) — Hilbert Transform - Dominant Cycle Phase
- [HT_PHASOR](/functions/ht_phasor.md) — Hilbert Transform - Phasor Components
- [HT_SINE](/functions/ht_sine.md) — Hilbert Transform - SineWave
- [HT_TRENDMODE](/functions/ht_trendmode.md) — Hilbert Transform - Trend vs Cycle Mode

## Math Operators

- [ADD](/functions/add.md) — Vector Arithmetic Add
- [DIV](/functions/div.md) — Vector Arithmetic Div
- [MAX](/functions/max.md) — Highest value over a specified period
- [MAXINDEX](/functions/maxindex.md) — Index of highest value over a specified period
- [MIN](/functions/min.md) — Lowest value over a specified period
- [MININDEX](/functions/minindex.md) — Index of lowest value over a specified period
- [MINMAX](/functions/minmax.md) — Lowest and highest values over a specified period
- [MINMAXINDEX](/functions/minmaxindex.md) — Indexes of lowest and highest values over a specified period
- [MULT](/functions/mult.md) — Vector Arithmetic Mult
- [SUB](/functions/sub.md) — Vector Arithmetic Subtraction
- [SUM](/functions/sum.md) — Summation

## Math Transform

- [ACOS](/functions/acos.md) — Vector Trigonometric ACos
- [ASIN](/functions/asin.md) — Vector Trigonometric ASin
- [ATAN](/functions/atan.md) — Vector Trigonometric ATan
- [CEIL](/functions/ceil.md) — Vector Ceil
- [COS](/functions/cos.md) — Vector Trigonometric Cos
- [COSH](/functions/cosh.md) — Vector Trigonometric Cosh
- [EXP](/functions/exp.md) — Vector Arithmetic Exp
- [FLOOR](/functions/floor.md) — Vector Floor
- [LN](/functions/ln.md) — Vector Log Natural
- [LOG10](/functions/log10.md) — Vector Log10
- [SIN](/functions/sin.md) — Vector Trigonometric Sin
- [SINH](/functions/sinh.md) — Vector Trigonometric Sinh
- [SQRT](/functions/sqrt.md) — Vector Square Root
- [TAN](/functions/tan.md) — Vector Trigonometric Tan
- [TANH](/functions/tanh.md) — Vector Trigonometric Tanh

## Momentum Indicators

- [ADX](/functions/adx.md) — Average Directional Movement Index
- [ADXR](/functions/adxr.md) — Average Directional Movement Index Rating
- [APO](/functions/apo.md) — Absolute Price Oscillator
- [AROON](/functions/aroon.md) — Aroon
- [AROONOSC](/functions/aroonosc.md) — Aroon Oscillator
- [BOP](/functions/bop.md) — Balance Of Power
- [CCI](/functions/cci.md) — Commodity Channel Index
- [CMO](/functions/cmo.md) — Chande Momentum Oscillator
- [DX](/functions/dx.md) — Directional Movement Index
- [IMI](/functions/imi.md) — Intraday Momentum Index
- [MACD](/functions/macd.md) — Moving Average Convergence/Divergence
- [MACDEXT](/functions/macdext.md) — MACD with controllable MA type
- [MACDFIX](/functions/macdfix.md) — Moving Average Convergence/Divergence Fix 12/26
- [MFI](/functions/mfi.md) — Money Flow Index
- [MINUS_DI](/functions/minus_di.md) — Minus Directional Indicator
- [MINUS_DM](/functions/minus_dm.md) — Minus Directional Movement
- [MOM](/functions/mom.md) — Momentum
- [PLUS_DI](/functions/plus_di.md) — Plus Directional Indicator
- [PLUS_DM](/functions/plus_dm.md) — Plus Directional Movement
- [PPO](/functions/ppo.md) — Percentage Price Oscillator
- [ROC](/functions/roc.md) — Rate of change : ((price/prevPrice)-1)*100
- [ROCP](/functions/rocp.md) — Rate of change Percentage: (price-prevPrice)/prevPrice
- [ROCR](/functions/rocr.md) — Rate of change ratio: (price/prevPrice)
- [ROCR100](/functions/rocr100.md) — Rate of change ratio 100 scale: (price/prevPrice)*100
- [RSI](/functions/rsi.md) — Relative Strength Index
- [STOCH](/functions/stoch.md) — Stochastic
- [STOCHF](/functions/stochf.md) — Stochastic Fast
- [STOCHRSI](/functions/stochrsi.md) — Stochastic Relative Strength Index
- [TRIX](/functions/trix.md) — 1-day Rate-Of-Change (ROC) of a Triple Smooth EMA
- [ULTOSC](/functions/ultosc.md) — Ultimate Oscillator
- [WILLR](/functions/willr.md) — Williams' %R

## Overlap Studies

- [ACCBANDS](/functions/accbands.md) — Acceleration Bands
- [BBANDS](/functions/bbands.md) — Bollinger Bands
- [DEMA](/functions/dema.md) — Double Exponential Moving Average
- [EMA](/functions/ema.md) — Exponential Moving Average
- [HT_TRENDLINE](/functions/ht_trendline.md) — Hilbert Transform - Instantaneous Trendline
- [KAMA](/functions/kama.md) — Kaufman Adaptive Moving Average
- [MA](/functions/ma.md) — Moving average
- [MAMA](/functions/mama.md) — MESA Adaptive Moving Average
- [MAVP](/functions/mavp.md) — Moving average with variable period
- [MIDPOINT](/functions/midpoint.md) — MidPoint over period
- [MIDPRICE](/functions/midprice.md) — Midpoint Price over period
- [SAR](/functions/sar.md) — Parabolic SAR
- [SAREXT](/functions/sarext.md) — Parabolic SAR - Extended
- [SMA](/functions/sma.md) — Simple Moving Average
- [T3](/functions/t3.md) — Triple Exponential Moving Average (T3)
- [TEMA](/functions/tema.md) — Triple Exponential Moving Average
- [TRIMA](/functions/trima.md) — Triangular Moving Average
- [WMA](/functions/wma.md) — Weighted Moving Average

## Pattern Recognition

- [CDL2CROWS](/functions/cdl2crows.md) — Two Crows
- [CDL3BLACKCROWS](/functions/cdl3blackcrows.md) — Three Black Crows
- [CDL3INSIDE](/functions/cdl3inside.md) — Three Inside Up/Down
- [CDL3LINESTRIKE](/functions/cdl3linestrike.md) — Three-Line Strike
- [CDL3OUTSIDE](/functions/cdl3outside.md) — Three Outside Up/Down
- [CDL3STARSINSOUTH](/functions/cdl3starsinsouth.md) — Three Stars In The South
- [CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers.md) — Three Advancing White Soldiers
- [CDLABANDONEDBABY](/functions/cdlabandonedbaby.md) — Abandoned Baby
- [CDLADVANCEBLOCK](/functions/cdladvanceblock.md) — Advance Block
- [CDLBELTHOLD](/functions/cdlbelthold.md) — Belt-hold
- [CDLBREAKAWAY](/functions/cdlbreakaway.md) — Breakaway
- [CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu.md) — Closing Marubozu
- [CDLCONCEALBABYSWALL](/functions/cdlconcealbabyswall.md) — Concealing Baby Swallow
- [CDLCOUNTERATTACK](/functions/cdlcounterattack.md) — Counterattack
- [CDLDARKCLOUDCOVER](/functions/cdldarkcloudcover.md) — Dark Cloud Cover
- [CDLDOJI](/functions/cdldoji.md) — Doji
- [CDLDOJISTAR](/functions/cdldojistar.md) — Doji Star
- [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) — Dragonfly Doji
- [CDLENGULFING](/functions/cdlengulfing.md) — Engulfing Pattern
- [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) — Evening Doji Star
- [CDLEVENINGSTAR](/functions/cdleveningstar.md) — Evening Star
- [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md) — Up/Down-gap side-by-side white lines
- [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md) — Gravestone Doji
- [CDLHAMMER](/functions/cdlhammer.md) — Hammer
- [CDLHANGINGMAN](/functions/cdlhangingman.md) — Hanging Man
- [CDLHARAMI](/functions/cdlharami.md) — Harami Pattern
- [CDLHARAMICROSS](/functions/cdlharamicross.md) — Harami Cross Pattern
- [CDLHIGHWAVE](/functions/cdlhighwave.md) — High-Wave Candle
- [CDLHIKKAKE](/functions/cdlhikkake.md) — Hikkake Pattern
- [CDLHIKKAKEMOD](/functions/cdlhikkakemod.md) — Modified Hikkake Pattern
- [CDLHOMINGPIGEON](/functions/cdlhomingpigeon.md) — Homing Pigeon
- [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md) — Identical Three Crows
- [CDLINNECK](/functions/cdlinneck.md) — In-Neck Pattern
- [CDLINVERTEDHAMMER](/functions/cdlinvertedhammer.md) — Inverted Hammer
- [CDLKICKING](/functions/cdlkicking.md) — Kicking
- [CDLKICKINGBYLENGTH](/functions/cdlkickingbylength.md) — Kicking - bull/bear determined by the longer marubozu
- [CDLLADDERBOTTOM](/functions/cdlladderbottom.md) — Ladder Bottom
- [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) — Long Legged Doji
- [CDLLONGLINE](/functions/cdllongline.md) — Long Line Candle
- [CDLMARUBOZU](/functions/cdlmarubozu.md) — Marubozu
- [CDLMATCHINGLOW](/functions/cdlmatchinglow.md) — Matching Low
- [CDLMATHOLD](/functions/cdlmathold.md) — Mat Hold
- [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) — Morning Doji Star
- [CDLMORNINGSTAR](/functions/cdlmorningstar.md) — Morning Star
- [CDLONNECK](/functions/cdlonneck.md) — On-Neck Pattern
- [CDLPIERCING](/functions/cdlpiercing.md) — Piercing Pattern
- [CDLRICKSHAWMAN](/functions/cdlrickshawman.md) — Rickshaw Man
- [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods.md) — Rising/Falling Three Methods
- [CDLSEPARATINGLINES](/functions/cdlseparatinglines.md) — Separating Lines
- [CDLSHOOTINGSTAR](/functions/cdlshootingstar.md) — Shooting Star
- [CDLSHORTLINE](/functions/cdlshortline.md) — Short Line Candle
- [CDLSPINNINGTOP](/functions/cdlspinningtop.md) — Spinning Top
- [CDLSTALLEDPATTERN](/functions/cdlstalledpattern.md) — Stalled Pattern
- [CDLSTICKSANDWICH](/functions/cdlsticksandwich.md) — Stick Sandwich
- [CDLTAKURI](/functions/cdltakuri.md) — Takuri (Dragonfly Doji with very long lower shadow)
- [CDLTASUKIGAP](/functions/cdltasukigap.md) — Tasuki Gap
- [CDLTHRUSTING](/functions/cdlthrusting.md) — Thrusting Pattern
- [CDLTRISTAR](/functions/cdltristar.md) — Tristar Pattern
- [CDLUNIQUE3RIVER](/functions/cdlunique3river.md) — Unique 3 River
- [CDLUPSIDEGAP2CROWS](/functions/cdlupsidegap2crows.md) — Upside Gap Two Crows
- [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md) — Upside/Downside Gap Three Methods

## Price Transform

- [AVGDEV](/functions/avgdev.md) — Average Deviation
- [AVGPRICE](/functions/avgprice.md) — Average Price
- [MEDPRICE](/functions/medprice.md) — Median Price
- [TYPPRICE](/functions/typprice.md) — Typical Price
- [WCLPRICE](/functions/wclprice.md) — Weighted Close Price

## Statistic Functions

- [BETA](/functions/beta.md) — Beta
- [CORREL](/functions/correl.md) — Pearson's Correlation Coefficient (r)
- [LINEARREG](/functions/linearreg.md) — Linear Regression
- [LINEARREG_ANGLE](/functions/linearreg_angle.md) — Linear Regression Angle
- [LINEARREG_INTERCEPT](/functions/linearreg_intercept.md) — Linear Regression Intercept
- [LINEARREG_SLOPE](/functions/linearreg_slope.md) — Linear Regression Slope
- [STDDEV](/functions/stddev.md) — Standard Deviation
- [TSF](/functions/tsf.md) — Time Series Forecast
- [VAR](/functions/var.md) — Variance

## Volatility Indicators

- [ATR](/functions/atr.md) — Average True Range
- [NATR](/functions/natr.md) — Normalized Average True Range
- [TRANGE](/functions/trange.md) — True Range

## Volume Indicators

- [AD](/functions/ad.md) — Chaikin A/D Line
- [ADOSC](/functions/adosc.md) — Chaikin A/D Oscillator
- [OBV](/functions/obv.md) — On Balance Volume
