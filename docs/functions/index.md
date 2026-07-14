---
title: Functions
description: "All TA-Lib technical analysis functions, grouped by category."
---

# TA-Lib Functions

All technical-analysis functions, grouped by category. Each page documents the formula, inputs, outputs, and links to the C / Rust / Java source.

## Cycle Indicators

- [HT_DCPERIOD](/functions/ht_dcperiod) — Hilbert Transform - Dominant Cycle Period
- [HT_DCPHASE](/functions/ht_dcphase) — Hilbert Transform - Dominant Cycle Phase
- [HT_PHASOR](/functions/ht_phasor) — Hilbert Transform - Phasor Components
- [HT_SINE](/functions/ht_sine) — Hilbert Transform - SineWave
- [HT_TRENDMODE](/functions/ht_trendmode) — Hilbert Transform - Trend vs Cycle Mode

## Math Operators

- [ADD](/functions/add) — Vector Arithmetic Add
- [DIV](/functions/div) — Vector Arithmetic Div
- [MAX](/functions/max) — Highest value over a specified period
- [MAXINDEX](/functions/maxindex) — Index of highest value over a specified period
- [MIN](/functions/min) — Lowest value over a specified period
- [MININDEX](/functions/minindex) — Index of lowest value over a specified period
- [MINMAX](/functions/minmax) — Lowest and highest values over a specified period
- [MINMAXINDEX](/functions/minmaxindex) — Indexes of lowest and highest values over a specified period
- [MULT](/functions/mult) — Vector Arithmetic Mult
- [SUB](/functions/sub) — Vector Arithmetic Subtraction
- [SUM](/functions/sum) — Summation

## Math Transform

- [ACOS](/functions/acos) — Vector Trigonometric ACos
- [ASIN](/functions/asin) — Vector Trigonometric ASin
- [ATAN](/functions/atan) — Vector Trigonometric ATan
- [CEIL](/functions/ceil) — Vector Ceil
- [COS](/functions/cos) — Vector Trigonometric Cos
- [COSH](/functions/cosh) — Vector Trigonometric Cosh
- [EXP](/functions/exp) — Vector Arithmetic Exp
- [FLOOR](/functions/floor) — Vector Floor
- [LN](/functions/ln) — Vector Log Natural
- [LOG10](/functions/log10) — Vector Log10
- [SIN](/functions/sin) — Vector Trigonometric Sin
- [SINH](/functions/sinh) — Vector Trigonometric Sinh
- [SQRT](/functions/sqrt) — Vector Square Root
- [TAN](/functions/tan) — Vector Trigonometric Tan
- [TANH](/functions/tanh) — Vector Trigonometric Tanh

## Momentum Indicators

- [ADX](/functions/adx) — Average Directional Movement Index
- [ADXR](/functions/adxr) — Average Directional Movement Index Rating
- [APO](/functions/apo) — Absolute Price Oscillator
- [AROON](/functions/aroon) — Aroon
- [AROONOSC](/functions/aroonosc) — Aroon Oscillator
- [BOP](/functions/bop) — Balance Of Power
- [CCI](/functions/cci) — Commodity Channel Index
- [CMO](/functions/cmo) — Chande Momentum Oscillator
- [DX](/functions/dx) — Directional Movement Index
- [IMI](/functions/imi) — Intraday Momentum Index
- [MACD](/functions/macd) — Moving Average Convergence/Divergence
- [MACDEXT](/functions/macdext) — MACD with controllable MA type
- [MACDFIX](/functions/macdfix) — Moving Average Convergence/Divergence Fix 12/26
- [MFI](/functions/mfi) — Money Flow Index
- [MINUS_DI](/functions/minus_di) — Minus Directional Indicator
- [MINUS_DM](/functions/minus_dm) — Minus Directional Movement
- [MOM](/functions/mom) — Momentum
- [PLUS_DI](/functions/plus_di) — Plus Directional Indicator
- [PLUS_DM](/functions/plus_dm) — Plus Directional Movement
- [PPO](/functions/ppo) — Percentage Price Oscillator
- [ROC](/functions/roc) — Rate of change : ((price/prevPrice)-1)*100
- [ROCP](/functions/rocp) — Rate of change Percentage: (price-prevPrice)/prevPrice
- [ROCR](/functions/rocr) — Rate of change ratio: (price/prevPrice)
- [ROCR100](/functions/rocr100) — Rate of change ratio 100 scale: (price/prevPrice)*100
- [RSI](/functions/rsi) — Relative Strength Index
- [STOCH](/functions/stoch) — Stochastic
- [STOCHF](/functions/stochf) — Stochastic Fast
- [STOCHRSI](/functions/stochrsi) — Stochastic Relative Strength Index
- [TRIX](/functions/trix) — 1-day Rate-Of-Change (ROC) of a Triple Smooth EMA
- [ULTOSC](/functions/ultosc) — Ultimate Oscillator
- [WILLR](/functions/willr) — Williams' %R

## Overlap Studies

- [ACCBANDS](/functions/accbands) — Acceleration Bands
- [BBANDS](/functions/bbands) — Bollinger Bands
- [DEMA](/functions/dema) — Double Exponential Moving Average
- [EMA](/functions/ema) — Exponential Moving Average
- [HT_TRENDLINE](/functions/ht_trendline) — Hilbert Transform - Instantaneous Trendline
- [KAMA](/functions/kama) — Kaufman Adaptive Moving Average
- [MA](/functions/ma) — Moving average
- [MAMA](/functions/mama) — MESA Adaptive Moving Average
- [MAVP](/functions/mavp) — Moving average with variable period
- [MIDPOINT](/functions/midpoint) — MidPoint over period
- [MIDPRICE](/functions/midprice) — Midpoint Price over period
- [SAR](/functions/sar) — Parabolic SAR
- [SAREXT](/functions/sarext) — Parabolic SAR - Extended
- [SMA](/functions/sma) — Simple Moving Average
- [T3](/functions/t3) — Triple Exponential Moving Average (T3)
- [TEMA](/functions/tema) — Triple Exponential Moving Average
- [TRIMA](/functions/trima) — Triangular Moving Average
- [WMA](/functions/wma) — Weighted Moving Average

## Pattern Recognition

- [CDL2CROWS](/functions/cdl2crows) — Two Crows
- [CDL3BLACKCROWS](/functions/cdl3blackcrows) — Three Black Crows
- [CDL3INSIDE](/functions/cdl3inside) — Three Inside Up/Down
- [CDL3LINESTRIKE](/functions/cdl3linestrike) — Three-Line Strike
- [CDL3OUTSIDE](/functions/cdl3outside) — Three Outside Up/Down
- [CDL3STARSINSOUTH](/functions/cdl3starsinsouth) — Three Stars In The South
- [CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers) — Three Advancing White Soldiers
- [CDLABANDONEDBABY](/functions/cdlabandonedbaby) — Abandoned Baby
- [CDLADVANCEBLOCK](/functions/cdladvanceblock) — Advance Block
- [CDLBELTHOLD](/functions/cdlbelthold) — Belt-hold
- [CDLBREAKAWAY](/functions/cdlbreakaway) — Breakaway
- [CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu) — Closing Marubozu
- [CDLCONCEALBABYSWALL](/functions/cdlconcealbabyswall) — Concealing Baby Swallow
- [CDLCOUNTERATTACK](/functions/cdlcounterattack) — Counterattack
- [CDLDARKCLOUDCOVER](/functions/cdldarkcloudcover) — Dark Cloud Cover
- [CDLDOJI](/functions/cdldoji) — Doji
- [CDLDOJISTAR](/functions/cdldojistar) — Doji Star
- [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji) — Dragonfly Doji
- [CDLENGULFING](/functions/cdlengulfing) — Engulfing Pattern
- [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar) — Evening Doji Star
- [CDLEVENINGSTAR](/functions/cdleveningstar) — Evening Star
- [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite) — Up/Down-gap side-by-side white lines
- [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji) — Gravestone Doji
- [CDLHAMMER](/functions/cdlhammer) — Hammer
- [CDLHANGINGMAN](/functions/cdlhangingman) — Hanging Man
- [CDLHARAMI](/functions/cdlharami) — Harami Pattern
- [CDLHARAMICROSS](/functions/cdlharamicross) — Harami Cross Pattern
- [CDLHIGHWAVE](/functions/cdlhighwave) — High-Wave Candle
- [CDLHIKKAKE](/functions/cdlhikkake) — Hikkake Pattern
- [CDLHIKKAKEMOD](/functions/cdlhikkakemod) — Modified Hikkake Pattern
- [CDLHOMINGPIGEON](/functions/cdlhomingpigeon) — Homing Pigeon
- [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows) — Identical Three Crows
- [CDLINNECK](/functions/cdlinneck) — In-Neck Pattern
- [CDLINVERTEDHAMMER](/functions/cdlinvertedhammer) — Inverted Hammer
- [CDLKICKING](/functions/cdlkicking) — Kicking
- [CDLKICKINGBYLENGTH](/functions/cdlkickingbylength) — Kicking - bull/bear determined by the longer marubozu
- [CDLLADDERBOTTOM](/functions/cdlladderbottom) — Ladder Bottom
- [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji) — Long Legged Doji
- [CDLLONGLINE](/functions/cdllongline) — Long Line Candle
- [CDLMARUBOZU](/functions/cdlmarubozu) — Marubozu
- [CDLMATCHINGLOW](/functions/cdlmatchinglow) — Matching Low
- [CDLMATHOLD](/functions/cdlmathold) — Mat Hold
- [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar) — Morning Doji Star
- [CDLMORNINGSTAR](/functions/cdlmorningstar) — Morning Star
- [CDLONNECK](/functions/cdlonneck) — On-Neck Pattern
- [CDLPIERCING](/functions/cdlpiercing) — Piercing Pattern
- [CDLRICKSHAWMAN](/functions/cdlrickshawman) — Rickshaw Man
- [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods) — Rising/Falling Three Methods
- [CDLSEPARATINGLINES](/functions/cdlseparatinglines) — Separating Lines
- [CDLSHOOTINGSTAR](/functions/cdlshootingstar) — Shooting Star
- [CDLSHORTLINE](/functions/cdlshortline) — Short Line Candle
- [CDLSPINNINGTOP](/functions/cdlspinningtop) — Spinning Top
- [CDLSTALLEDPATTERN](/functions/cdlstalledpattern) — Stalled Pattern
- [CDLSTICKSANDWICH](/functions/cdlsticksandwich) — Stick Sandwich
- [CDLTAKURI](/functions/cdltakuri) — Takuri (Dragonfly Doji with very long lower shadow)
- [CDLTASUKIGAP](/functions/cdltasukigap) — Tasuki Gap
- [CDLTHRUSTING](/functions/cdlthrusting) — Thrusting Pattern
- [CDLTRISTAR](/functions/cdltristar) — Tristar Pattern
- [CDLUNIQUE3RIVER](/functions/cdlunique3river) — Unique 3 River
- [CDLUPSIDEGAP2CROWS](/functions/cdlupsidegap2crows) — Upside Gap Two Crows
- [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods) — Upside/Downside Gap Three Methods

## Price Transform

- [AVGDEV](/functions/avgdev) — Average Deviation
- [AVGPRICE](/functions/avgprice) — Average Price
- [MEDPRICE](/functions/medprice) — Median Price
- [TYPPRICE](/functions/typprice) — Typical Price
- [WCLPRICE](/functions/wclprice) — Weighted Close Price

## Statistic Functions

- [BETA](/functions/beta) — Beta
- [CORREL](/functions/correl) — Pearson's Correlation Coefficient (r)
- [LINEARREG](/functions/linearreg) — Linear Regression
- [LINEARREG_ANGLE](/functions/linearreg_angle) — Linear Regression Angle
- [LINEARREG_INTERCEPT](/functions/linearreg_intercept) — Linear Regression Intercept
- [LINEARREG_SLOPE](/functions/linearreg_slope) — Linear Regression Slope
- [STDDEV](/functions/stddev) — Standard Deviation
- [TSF](/functions/tsf) — Time Series Forecast
- [VAR](/functions/var) — Variance

## Volatility Indicators

- [ATR](/functions/atr) — Average True Range
- [NATR](/functions/natr) — Normalized Average True Range
- [TRANGE](/functions/trange) — True Range

## Volume Indicators

- [AD](/functions/ad) — Chaikin A/D Line
- [ADOSC](/functions/adosc) — Chaikin A/D Oscillator
- [OBV](/functions/obv) — On Balance Volume
