# Auto warm-up: an "Auto" unstable period

**Status:** design for [#492](https://github.com/TA-Lib/ta-lib/issues/492), with its decisions
ruled by the owner (section 9). Nothing is implemented. The measurements are reproducible from
[`studies/auto-warm-up/`](studies/auto-warm-up/README.md).

## 1. Summary

`TA_SetUnstablePeriod(id, n)` discards a fixed number of warm-up bars per id, whatever the
function's parameters. Auto is a small set of further values the same setter accepts, one per
level. Under a level each function discards output until the seed of its recursion has lost all
but `e^-K` of its weight, the count comes from the call's own parameters, and the lookback
reports it.

- Two levels: `TA_UNSTABLE_AUTO_PREC_4` (`K = 10`) and `TA_UNSTABLE_AUTO_PREC_8` (`K = 19`).
  The number is how many significant digits of an output no longer depend on where the data
  starts (section 2.1). More levels can follow as further constants.
- New constants only. No new function, no new state, no signature change in any language.
  Default off.
- Each unstable id has one rule: a function of that function's parameters and `K`. Functions
  that own no id get Auto through the lookbacks they already call.
- A count never depends on the data, so the lookback stays a function of parameters and
  settings, and all four languages return the same one.
- From bar 0, a function whose lookback reads its own id and no other reports under Auto the
  values it reports today, starting later.

## 2. What Auto promises

A recursive function carries state from bar to bar. A call seeds that state at
`startIdx - lookback` and the seed's share of the state shrinks every bar. Two calls that start
at different bars use different seeds, so they disagree at the same bar until both seeds have
faded.

**The promise is about the seed's weight.** After the Auto count, what the seed still
contributes is at most `e^-K` of the largest difference it ever caused. For a one-pole smoother
with coefficient `a` the weight after `m` bars is exactly `(1-a)^m`, which is below `e^(-a*m)`,
so `m = ceil(K/a)` bars are enough for any input. Every rule in section 3 is that statement, or
the nearest thing the function's recursion allows.

In half-lives: `e^-10` is `2^-14.4` and `e^-19` is `2^-27.4`, so a count is about 14 half-lives
of the kernel at the first level and 27 at the second. An EMA's half-life is
`ln 2 / -ln(1 - 2/(n+1))`, about `0.35*(n+1)` bars. The rule replaces the logarithm
with the bound `-ln(1-a) >= a`, which costs about `K/2` bars at any period and keeps the count
free of `log`, so every backend computes the same one.

Each rule is derived from the recurrence, the coefficient and the seed read in the function's
source, then checked by measurement (section 10). Where the source gives no usable coefficient, or
only a bound too long to use, the count is sized by measurement and says so.

Four limits belong with the promise.

- **It is neither bit-identity nor agreement of the outputs within `e^-K`.** The count bounds
  the seed's weight. Outputs of two starts then agree within about `e^-(K-3)` of their largest
  difference (section 6.1). Many recursive outputs do become bit-identical after enough bars,
  but windowed functions that keep running totals differ between starts by rounding: up to
  8e-10 of the output's range at the default parameters, and 3.5e-8 at tripled periods (CORREL
  at a period of 90).
- **A ratio of two states converges only while its divisor is alive.** RSI, CMO, the DI pair,
  DX, RVI, TSI, SMI, STC and others divide one smoothed series by another, and ADX averages such
  a ratio. Both terms decay at the same rate on a market that stops moving, so the ratio freezes
  with its start dependence intact, for any warm-up. The rule bounds the smoothed sums; a ratio
  of them follows while prices keep changing.
- **Some recursions have no bound, or one too long to use.** Section 3.2 gives each rule a tier
  that says what it guarantees. The adaptive averages are sized for a market that trends or
  wanders, not for a range-bound one.
- **The calibrated rules are sized on price series.** MAMA and the Hilbert functions do not
  converge on an input that sits on one value for runs of bars. STOCHRSI smoothed with the MAMA
  type is the case measured: its FastK rests at 0 or 100, and two starts fed identical FastK
  values still differ thousands of bars later.

The cost in bars is set mostly by the period. Each factor of ten in the weight costs an EMA
about `1.15*(n+1)` bars, so no level makes a long period cheap:

| Call | Lookback today | `PREC_4` | `PREC_8` |
|---|---|---|---|
| EMA(30) | 29 | 184 | 324 |
| EMA(200) | 199 | 1204 | 2109 |
| RSI(14) | 14 | 154 | 280 |
| MACD(12,26,9) | 33 | 218 | 385 |

### 2.1 What a level means to a user

`PREC_n`: once the warm-up is discarded, the first `n` significant digits of a value no longer
depend on where the data starts. Significant digits, not decimals: at `PREC_4` that is
`45.67y`, `1.234y` or `0.0001234y`, with `y` the first digit that can still move.

This was measured for every id at both levels: the bars until two starts agree to `n`
significant digits of the value, against the rule's count.

- **It holds as stated for every output that stays away from zero**: the moving averages and
  everything on the price scale, ATR, NATR, RSI, the DI and DM pairs, ADX, RVI, STC, the Hilbert
  period and trendline. On the price scale the count usually delivers about two digits more:
  EMA(30) agrees to four digits after 72 bars and to six after 140, against a `PREC_4` count
  of 155.
- **For an output that reaches zero, count the digits from the size of its swing.** A value near
  zero has no leading digits to keep, whatever the warm-up. CMO, DX, the high-pass and band-pass
  SWAK filters, HT_PHASOR, HT_SINE and HT_DCPHASE are in this group, with MACD, APO, PPO, TRIX and
  the other oscillators centred on zero. For them `PREC_4` means the difference is about one
  ten-thousandth of the largest difference two starts ever show, which is about the size of the
  oscillator's swing. Outputs that divide by a state (DX, CMO, STOCHRSI, PPO, PVO) can be a few
  times above that at the count (section 6.1).

`PREC_8` asks more than some outputs can show: section 9, D1.

## 3. Classification

### 3.1 How a function is classified

Three questions, answered from the function's code.

1. **Does it carry a value from one bar to the next that is not a function of a bounded window
   of inputs?** If not, the function needs no warm-up. It is *pointwise* when the output at a bar
   uses that bar only, and a *window* otherwise. A window either recomputes from its bars, and
   two starts then give the same bits, or keeps running totals (add the newest bar, drop the
   oldest), and two starts then differ by rounding.
2. **Does the carried value forget its seed?** A smoother forgets at the rate of its coefficient.
   A total that is never windowed never forgets (*accumulation*). Discrete state such as a trend
   direction or a ratcheted band forgets only when the data happens to rewrite it (*state
   machine*). Neither gets a rule: no count computed from the parameters makes them
   start-independent.
3. **Whose recursion is it?** A function that carries the recursion in its own code under its
   own unstable id is a *leaf*, and its rule is written for it. A function whose lookback is
   built from other functions' lookbacks is *composed*, or an *MA dispatch* when the callee is
   chosen by an MA-type parameter. Those two inherit.

Questions 1 and 2 were answered twice, independently:

- by reading every function's input source for the state it carries, its seed and its lookback;
- by measurement: every function, through the abstraction layer, from bar 0 and from six later
  starts on three synthetic series, comparing the two runs bar by bar.

At the default parameters the two agree on every function, with the exceptions section 3.5
lists.

Question 3 is already mechanical. The generator derives "owns an id", "inherits from" and
"depends on the MA type" from the lookback call graph and publishes them as each function page's
Numerical Stability line. Auto adds one fact per id, the rule, and the same derivation carries
it to every inheriting function.

### 3.2 The rule for each unstable id

`n` is the function's period, `K` the tolerance constant. Counts are bars added to the lookback
the function has with its unstable period at 0.

- `E(n) = ceil(K*(n+1)/2)`, the EMA kernel, coefficient `2/(n+1)`.
- `W(n) = K*n`, the Wilder kernel, coefficient `1/n`.
- `H = 80 + 50*X`, the Hilbert pipeline: a fixed settling time plus a part per digit.
- `K` is 10 at `PREC_4` and 19 at `PREC_8`: the number of e-folds that takes the seed's weight
  under `10^-4` and `10^-8`. `X` is the level's digit count, 4 or 8. A proven rule is written in
  `K`, because it follows from the kernel's decay; a calibrated one for an adaptive average is
  a constant times `X`.
- `isqrt(n)` is the integer square root, written `(int)sqrt((double)n)` as in HMA's lookback.
- The counts in the table are for the two levels at the default parameters.
- Every rule returns 0 where the function does no smoothing: a period of 1 for EMA, RMA, ATR,
  NATR, RVI, T3, KAMA and VIDYA, and the period-1 arms the DI and DM functions already have.

| Id | Rule | `PREC_4` / `PREC_8`, defaults | Tier | Why |
|---|---|---|---|---|
| `EMA` | `E(n)` | 155 / 295 | proven | one pole at `1 - 2/(n+1)` |
| `RMA`, `ATR`, `PLUS_DM`, `MINUS_DM` | `W(n)` | RMA 300 / 570, the others 140 / 266 | proven | one pole at `1 - 1/n` |
| `NATR`, `RSI`, `CMO`, `PLUS_DI`, `MINUS_DI`, `DX`, `RVI` | `W(n)` | 140 / 266 | proven, ratio | the same pole; the output divides by a state or a price |
| `ADX` | `(K+6)*n` | 224 / 350 | proven, ratio | two Wilder stages under one id. `K+4` bounds the linear cascade; the second stage averages the DX ratio, so its state carries the ratio limit of section 2 and the rest is margin for it |
| `T3` | `ceil((K+20)*(n+1)/2)` | 90 / 117 | proven | six EMA stages stepped together under one id. The cascade's state weight after `m` bars is at most the chance of five or fewer successes in `m` trials of probability `2/(n+1)`; `K+20` keeps it under `e^-K` with room for the output weights at any volume factor |
| `HA` | `2*K` | 20 / 38 | proven | one pole at 1/2 |
| `SWAK_HP` | `ceil(K*P/6)` | 34 / 64 | proven | one pole at `tan(pi/4 - pi/P)`, which never exceeds `e^(-2*pi/P)` |
| `SWAK_GAUSS`, `SWAK_BUTTER`, `SWAK_2PHP` | `ceil((K+5)*(P+2)/9)` | 37 / 59 | proven | a repeated real pole: the mismatch moves as `(A + B*k) * r^k`, and `K+5` covers the linear term against its own peak |
| `SWAK_BP` | `ceil((K+1)*P/(6*delta))` | 367 / 667 | proven | a complex pole pair of radius `sqrt(abp)`, set by `P/delta`; a period-only rule would be several times too short |
| `KAMA` | `25*X*isqrt(n)` | 500 / 1000 | calibrated | the smoothing constant follows the efficiency ratio, between `(2/31)^2` and `(2/3)^2`. Sized on trending and random-walk data, where the need grows about with the square root of the period. The floor alone would give `ceil(961*K/4)`, 2403 bars at `PREC_4` whatever the period |
| `FRAMA` | `80*X` | 320 / 640 | calibrated | alpha follows the fractal dimension, between `e^-4.6` and 1. Sized on trending and random-walk data, where the need barely moves with the period. The floor alone would give `100*K`, 1000 bars at `PREC_4` |
| `VIDYA` | `2*X*(n+1)*isqrt(m)`, `m = optInCMOPeriod`, saturating at `TA_INDEX_MAX` | 312 / 624 | calibrated | the coefficient is `2/(n+1)` times `abs(CMO)/100` and can be 0, so no bound exists. On random-walk data `abs(CMO)` shrinks about with the square root of its period, which the formula follows |
| `MCGD` | `5*X*n` | 280 / 560 | calibrated | the coefficient depends on the line itself and is Wilder's `1/n` when the line is on the price. No bound exists: a line more than 20% below the price amplifies its seed |
| `HT_DCPERIOD`, `HT_DCPHASE`, `HT_PHASOR`, `HT_SINE`, `HT_TRENDLINE`, `HT_TRENDMODE` | `H` | 280 / 480 | calibrated | no parameter. The period estimate feeds back into its own filter gains through clamps and a rate limiter, so no bound is derivable |
| `MAMA` | `H + ceil(2*K/max(fast, slow))` | 320 / 556 | calibrated | the Hilbert pipeline has to settle before the two smoothers see the same alpha; then FAMA, the slower one, at the rate of the larger limit, which is where alpha usually sits when fast is not below slow. Dividing by the smaller limit is the bound, several times longer |
| `STC` | `2*K + 3*(s+1)`, `s = max(fast, slow)`, on top of the `E(s)` it inherits | 173 / 191 | calibrated | two poles at 1/2 run together, plus six time constants of the slow EMA for what the two range divisions magnify |

The tiers:

- **Proven.** The count bounds the seed's remaining weight for every input. For the two-pole
  SWAK rules the weight is measured against the largest difference the seed causes, not against
  its initial size: a repeated or complex pole first amplifies a mismatch held in one slot.
  "Ratio" marks an output that divides by a state (section 2).
- **Calibrated.** The count is a formula sized on measurements, with margin, and held by the
  regression leg on a committed corpus. It is not a bound.

Two groups are calibrated, for different reasons.

- **The Hilbert functions, MAMA and STC** have no usable coefficient to derive a count from.
  Their counts are sized on all three kinds of series the study uses.
- **The adaptive averages, KAMA, FRAMA, VIDYA and MCGD,** change their own rate with the market.
  Their counts are sized for a market that trends or wanders (ruled, section 9, D3). In a
  range-bound market KAMA, FRAMA and VIDYA slow down by design and take several times longer:
  KAMA(30) needs 2148 bars at `PREC_4` against a count of 500, FRAMA(16) 514 against 320,
  VIDYA(12, 9) 581 against 312. MCGD is slowest in a trend, and its count covers all three kinds
  of series at the periods measured. A caller who needs more sets a fixed count on that id.

The SWAK_BP and MAMA rules read a real parameter. Each is defined as its `double` expression in
one fixed order, `ceil( (double)((K+1)*P) / (6.0*optInDelta) )` and
`ceil( (double)(2*K) / max(fast, slow) )`, not as the real-number formula: the two can differ by
one bar where the quotient is an integer.

Every rule is non-decreasing in the period, and three things rest on that:

- MAVP, APO, PPO and PVO index a shorter-period pass on the assumption that the MA lookback
  never shrinks as the period grows. A rule that broke it would read before a buffer with no
  error.
- The re-anchored MACD of section 5.3 needs `ema_lookback(fast) <= ema_lookback(slow)` to keep
  its fast seed inside the history its lookback reserved.
- ADOSC and MACDFIX give the shorter leg the longer period's count, which covers it only while
  the rule does not decrease.

Reserved ids (`UNUSED_n`) accept Auto and move nothing, as they accept any count today.

### 3.3 Inheritance

A function that owns no id inherits through its lookback, which already combines its callees'
lookbacks in one of three ways. Auto changes nothing in these expressions: each callee lookback
resolves its own count from the period it is given.

- **Sum**, for stages in sequence: MACD is `ema_lookback(max(fast, slow)) +
  ema_lookback(signal)`, so it adds `E(max(fast, slow)) + E(signal)`.
- **Longest path**, for legs that meet on one bar: KC is the longer of its EMA path and its ATR
  path.
- **Dispatch**, on an MA-type parameter. `M(n, type)` in the table below is:

| MA type | `M(n, type)` | Tier |
|---|---|---|
| SMA, WMA, TRIMA, HMA, ALMA, `DISABLED` | 0 | |
| EMA, ZLEMA | `E(n)` | proven |
| DEMA | `2*E(n)` | proven |
| TEMA | `3*E(n)` | proven |
| RMA | `W(n)` | proven |
| T3 | `ceil((K+20)*(n+1)/2)` | proven |
| KAMA | `25*X*isqrt(n)` | calibrated |
| MAMA | `H + 4*K` (the dispatcher fixes the limits at 0.5 and 0.05) | calibrated |
| VIDYA | `2*X*(n+1)*isqrt(m)`, with the dispatcher's `m = (3*n+2)/4`, saturating | calibrated |
| `DEFAULT` | that of the parameter's documented default type | |
| any type at a period of 1 | 0 | |

Summing is conservative. In DEMA the first EMA has run through two counts by the first output
where one would do. The alternative is a hand-tuned count per composite, and every such count is
one more number to be wrong; a longer warm-up costs history, a shorter one breaks the promise.

### 3.4 Every function

"Measured" is what the two-start measurement saw at the default parameters, with every unstable
period at 0. This table is the design-time classification; delete it when the generated function
pages carry the rule.

| Function | Class | Auto adds to the lookback | Rule tier | Measured |
|---|---|---|---|---|
| `AC` | Window | 0 |  | rounding only |
| `ACCBANDS` | Window | 0 |  | rounding only |
| `ACOS` | Pointwise | 0 |  | bit-identical |
| `AD` | Accumulation | none |  | never |
| `ADD` | Pointwise | 0 |  | bit-identical |
| `ADOSC` | Composed | E(max(fast, slow)) (through EMA) | inherited | converges |
| `ADR` | Window | 0 |  | bit-identical (running totals, exact on these series) |
| `ADX` | Leaf | (K+6)*n | proven, ratio | converges |
| `ADXR` | Composed | (K+6)*n (through ADX) | inherited | converges |
| `ALMA` | Window | 0 |  | bit-identical |
| `AO` | Window | 0 |  | rounding only |
| `APO` | MA dispatch | M(max(fast, slow), type) | inherited | converges |
| `AROON` | Window | 0 |  | bit-identical |
| `AROONOSC` | Window | 0 |  | bit-identical |
| `ASI` | Accumulation | none |  | never |
| `ASIN` | Pointwise | 0 |  | bit-identical |
| `ATAN` | Pointwise | 0 |  | bit-identical |
| `ATR` | Leaf | W(n) | proven | converges |
| `AVGDEV` | Window | 0 |  | bit-identical |
| `AVGPRICE` | Pointwise | 0 |  | bit-identical |
| `BBANDS` | MA dispatch | M(n, type), when that path is the longer one | inherited | rounding only |
| `BBW` | MA dispatch | as BBANDS | inherited | rounding only |
| `BETA` | Window | 0 |  | rounding only |
| `BOP` | Pointwise | 0 |  | bit-identical |
| `CCI` | Window | 0 |  | rounding only |
| `CEIL` | Pointwise | 0 |  | bit-identical |
| `CG` | Window | 0 |  | bit-identical |
| `CHOP` | Window | 0 |  | bit-identical |
| `CHOPTR` | Window | 0 |  | bit-identical |
| `CKSP` | Composed | W(n) (through ATR) | inherited | converges |
| `CMF` | Window | 0 |  | rounding only |
| `CMO` | Leaf | W(n) | proven, ratio | converges |
| `CMOU` | Window | 0 |  | bit-identical (running totals, exact on these series) |
| `COPPOCK` | Window | 0 |  | rounding only |
| `CORREL` | Window | 0 |  | rounding only (3.5e-8 of range at a tripled period) |
| `COS` | Pointwise | 0 |  | bit-identical |
| `COSH` | Pointwise | 0 |  | bit-identical |
| `CRSI` | Composed | longest of the RSI path with W(n), the streak path with W(streak), and the rank path with 0 (through RSI) | inherited | rounding only at the defaults |
| `CTI` | Window | 0 |  | rounding only |
| `CUMSUM` | Accumulation | none |  | never |
| `CVI` | Composed | E(n) (through EMA) | inherited | converges |
| `DEMA` | Composed | 2*E(n) (through EMA) | inherited | converges |
| `DIV` | Pointwise | 0 |  | bit-identical |
| `DONCHIAN` | Window | 0 |  | bit-identical |
| `DPO` | Window | 0 |  | rounding only |
| `DX` | Leaf | W(n) | proven, ratio | converges |
| `EFI` | Composed | E(n) (through EMA) | inherited | converges |
| `EMA` | Leaf | E(n) | proven | converges |
| `EMV` | Window | 0 |  | rounding only |
| `ER` | Window | 0 |  | bit-identical (running totals, exact on these series) |
| `ERI` | Composed | E(n) (through EMA) | inherited | converges |
| `EXP` | Pointwise | 0 |  | bit-identical |
| `FLOOR` | Pointwise | 0 |  | bit-identical |
| `FOSC` | Window | 0 |  | rounding only |
| `FRACTAL` | Window | 0 |  | bit-identical |
| `FRAMA` | Leaf | 80*X | calibrated | converges |
| `HA` | Leaf | 2K | proven | converges |
| `HMA` | Window | 0 |  | rounding only |
| `HT_DCPERIOD` | Leaf | H | calibrated | converges |
| `HT_DCPHASE` | Leaf | H | calibrated | converges |
| `HT_PHASOR` | Leaf | H | calibrated | converges |
| `HT_SINE` | Leaf | H | calibrated | converges |
| `HT_TRENDLINE` | Leaf | H | calibrated | converges |
| `HT_TRENDMODE` | Leaf | H | calibrated | converges |
| `IBS` | Pointwise | 0 |  | bit-identical |
| `IMI` | Window | 0 |  | bit-identical |
| `KAMA` | Leaf | 25*X*isqrt(n) | calibrated | converges |
| `KC` | Composed | longer of the EMA path with E(n) and the ATR path with W(atr) (through EMA, ATR) | inherited | converges |
| `KDJ` | MA dispatch | M(slowK, type) + M(slowD, type) | inherited | converges |
| `KST` | Window | 0 |  | rounding only |
| `KSTEXT` | MA dispatch | longest leg with M(ma_i, type), plus M(signal, type) | inherited | rounding only |
| `KURTOSIS` | Window | 0 |  | rounding only |
| `LINEARREG` | Window | 0 |  | rounding only |
| `LINEARREG_ANGLE` | Window | 0 |  | rounding only |
| `LINEARREG_INTERCEPT` | Window | 0 |  | rounding only |
| `LINEARREG_SLOPE` | Window | 0 |  | rounding only |
| `LN` | Pointwise | 0 |  | bit-identical |
| `LOG10` | Pointwise | 0 |  | bit-identical |
| `MA` | MA dispatch | M(n, type) | inherited | rounding only |
| `MACD` | Composed | E(max(fast, slow)) + E(signal) (through EMA) | inherited | converges |
| `MACDEXT` | MA dispatch | longer of M(fast, type) and M(slow, type), plus M(signal, type) | inherited | rounding only |
| `MACDFIX` | Composed | E(26) + E(signal) (through EMA) | inherited | converges |
| `MAMA` | Leaf | H + ceil(2K/max(fast, slow)) | calibrated | converges |
| `MARKETFI` | Pointwise | 0 |  | bit-identical |
| `MASSI` | Composed | 2*E(fast) (through EMA) | inherited | converges |
| `MAVP` | MA dispatch | M(maxPeriod, type) | inherited | rounding only |
| `MAX` | Window | 0 |  | bit-identical |
| `MAXINDEX` | Window | 0 |  | bit-identical except on ties |
| `MCGD` | Leaf | 5*X*n | calibrated | converges |
| `MEDIAN` | Window | 0 |  | bit-identical |
| `MEDPRICE` | Pointwise | 0 |  | bit-identical |
| `MFI` | Window | 0 |  | rounding only |
| `MIDPOINT` | Window | 0 |  | bit-identical |
| `MIDPRICE` | Window | 0 |  | bit-identical |
| `MIN` | Window | 0 |  | bit-identical |
| `MININDEX` | Window | 0 |  | bit-identical except on ties |
| `MINMAX` | Window | 0 |  | bit-identical |
| `MINMAXINDEX` | Window | 0 |  | bit-identical except on ties |
| `MINUS_DI` | Leaf | W(n) | proven, ratio | converges |
| `MINUS_DM` | Leaf | W(n) | proven | converges |
| `MOM` | Window | 0 |  | bit-identical |
| `MULT` | Pointwise | 0 |  | bit-identical |
| `NATR` | Leaf | W(n) | proven, ratio | converges |
| `NVI` | Accumulation | none |  | never |
| `OBV` | Accumulation | none |  | never |
| `PERCENTB` | MA dispatch | as BBANDS | inherited | rounding only |
| `PERCENTILE` | Window | 0 |  | bit-identical |
| `PERCENTRANK` | Window | 0 |  | bit-identical |
| `PLUS_DI` | Leaf | W(n) | proven, ratio | converges |
| `PLUS_DM` | Leaf | W(n) | proven | converges |
| `PPO` | MA dispatch | M(max(fast, slow), type) | inherited | converges |
| `PVI` | Accumulation | none |  | never |
| `PVO` | MA dispatch | M(max(fast, slow), type) | inherited | converges |
| `PVT` | Accumulation | none |  | never |
| `QSTICK` | Window | 0 |  | bit-identical (running totals, exact on these series) |
| `RMA` | Leaf | W(n) | proven | converges |
| `ROC` | Window | 0 |  | bit-identical |
| `ROCP` | Window | 0 |  | bit-identical |
| `ROCR` | Window | 0 |  | bit-identical |
| `ROCR100` | Window | 0 |  | bit-identical |
| `RSI` | Leaf | W(n) | proven, ratio | converges |
| `RVI` | Leaf | W(n), n = optInTimePeriod | proven, ratio | converges |
| `RVIR` | Composed | W(n) (through RVI) | inherited | converges |
| `RVOL` | Window | 0 |  | bit-identical (running totals, exact on these series) |
| `SAR` | State machine | none |  | re-synchronises, no bound |
| `SAREXT` | State machine | none |  | re-synchronises, no bound |
| `SI` | Window | 0 |  | bit-identical |
| `SIN` | Pointwise | 0 |  | bit-identical |
| `SINH` | Pointwise | 0 |  | bit-identical |
| `SMA` | Window | 0 |  | rounding only |
| `SMI` | Composed | E(slow) + E(fast) + E(signal) (through EMA) | inherited | converges |
| `SQRT` | Pointwise | 0 |  | bit-identical |
| `STC` | Leaf | 2K + 3(s+1), s = max(fast, slow), on top of E(s) | calibrated | converges |
| `STDDEV` | Window | 0 |  | rounding only |
| `STOCH` | MA dispatch | M(slowK, type) + M(slowD, type) | inherited | rounding only |
| `STOCHF` | MA dispatch | M(fastD, type) | inherited | rounding only |
| `STOCHRSI` | Composed | W(n) + M(fastD, type) (through RSI, MA type) | inherited | converges |
| `SUB` | Pointwise | 0 |  | bit-identical |
| `SUM` | Window | 0 |  | rounding only |
| `SUPERTREND` | State machine | W(n), on the ATR only |  | re-synchronises, no bound |
| `SWAK_2PHP` | Leaf | ceil((K+5)(P+2)/9) | proven | converges |
| `SWAK_BP` | Leaf | ceil((K+1)*P/(6*delta)) | proven | converges |
| `SWAK_BUTTER` | Leaf | ceil((K+5)(P+2)/9) | proven | converges |
| `SWAK_GAUSS` | Leaf | ceil((K+5)(P+2)/9) | proven | converges |
| `SWAK_HP` | Leaf | ceil(K*P/6) | proven | converges |
| `T3` | Leaf | ceil((K+20)(n+1)/2) | proven | converges |
| `TAN` | Pointwise | 0 |  | bit-identical |
| `TANH` | Pointwise | 0 |  | bit-identical |
| `TEMA` | Composed | 3*E(n) (through EMA) | inherited | converges |
| `TRANGE` | Window | 0 |  | bit-identical |
| `TRIMA` | Window | 0 |  | rounding only |
| `TRIX` | Composed | 3*E(n) (through EMA) | inherited | converges |
| `TSF` | Window | 0 |  | rounding only |
| `TSI` | Composed | E(first) + E(second) (through EMA) | inherited | converges |
| `TYPPRICE` | Pointwise | 0 |  | bit-identical |
| `ULTOSC` | Window | 0 |  | bit-identical at the defaults, rounding only at tripled periods |
| `VAR` | Window | 0 |  | rounding only |
| `VHF` | Window | 0 |  | bit-identical |
| `VIDYA` | Leaf | 2*X*(n+1)*isqrt(m), m = optInCMOPeriod | calibrated | converges |
| `VORTEX` | Window | 0 |  | bit-identical (running totals, exact on these series) |
| `VWAP` | Accumulation | none |  | never (dilutes as 1/t) |
| `VWMA` | Window | 0 |  | rounding only |
| `WAD` | Accumulation | none |  | never |
| `WCLPRICE` | Pointwise | 0 |  | bit-identical |
| `WILLR` | Window | 0 |  | bit-identical |
| `WMA` | Window | 0 |  | rounding only |
| `ZLEMA` | Composed | E(n) (through EMA) | inherited | converges |
| every `CDL*` function | Window | 0 | | bit-identical on these series |

### 3.5 What the classification found

Where the reading and the measurement differ, and facts the tree states differently today.
The index tie-break is scheduled in section 8. The period-1 item is part of the rules of
section 3.2. The rest are separate decisions.

- **SUPERTREND inherits ATR's id but its bands do not converge by decay.** The unstable period
  warms the ATR; the trend flag and both bands are seeded on the first reported bar at every
  setting. Auto lengthens its lookback as a fixed count does today and promises nothing about
  the line.
- **SAR, SAREXT and SUPERTREND usually re-synchronise.** Two starts became bit-identical after a
  data-dependent number of bars in every run measured (at most 113 for SAR, 1367 for
  SUPERTREND). No parameter bounds that number, so they stay without a rule, but "the
  difference persists for the whole series" on the stability page describes the accumulations,
  not these three.
- **VWAP is neither.** The difference between two starts shrinks as the early segment's share of
  cumulative volume, roughly as `1/t`. Not a constant offset, not an `e^-K` kernel, no rule.
- **MAXINDEX, MININDEX and MINMAXINDEX depend on the start when the window holds a tie.** A call
  that starts at a bar reports the first of two equal extremes; a call that slid there from an
  earlier bar reports the last. Reproduced on tie-heavy data (`tie_break.c` in the study).
- **Some windows measure bit-identical although they keep running totals.** ADR, CMOU, ER,
  QSTICK, ULTOSC and VORTEX sum price differences and RVOL sums integer volumes, all exactly on
  the study's two-decimal series; ULTOSC already differs by rounding at tripled periods. The
  candle functions that read a candle average keep running totals too, so their integer output
  can differ between starts where a comparison against the average falls within rounding.
- **CRSI's streak is carried state with no id.** It resets completely at the first reversal or
  unchanged close, so it is a short-lived state machine inside a function published as
  converging. At the defaults the rank window already gives both RSI legs about 98 steps.
- **A fixed count is charged where there is no recursion.** `ema_lookback(1)` is the unstable
  period although EMA at a period of 1 copies its input, and the same holds for RMA, KAMA, VIDYA,
  T3, ATR, NATR and RVI. `ma_lookback(1, any type)` is 0 for the same computation. Auto resolves
  to 0 there; fixed counts keep today's behaviour.

## 4. The API

### 4.1 One constant per level

`TA_UNSTABLE_AUTO_PREC_4` is `TA_INDEX_MAX + 4` and `TA_UNSTABLE_AUTO_PREC_8` is
`TA_INDEX_MAX + 8`: the offset is the digit count. The setter accepts `[0, TA_INDEX_MAX]` and
the levels that exist; everything else stays `TA_BAD_PARAM`, the other offsets included. A later
level, or a different kind of Auto, is one more constant and one more accepted value. The rest
of this document writes `TA_UNSTABLE_AUTO` for any of them.

```c
TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, TA_UNSTABLE_AUTO_PREC_4 );  /* every id on Auto */
TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 0 );           /* except RSI: last write wins */
```

The level is per id, so two ids can sit at different levels.

The values are constrained from four sides:

- above `TA_INDEX_MAX`, which a library from 0.8.1 on refuses with `TA_BAD_PARAM`. Releases up
  to 0.7.1 have no upper bound in the setter: they store the value and read it as a count. They
  carry another soname, so a binary linked against this library does not load them;
- below 2^31: Java and C# take an `int`, Rust stores an `i32`, and a negative C `int` arrives
  above 2^31 and must stay refused;
- not `INT_MAX`, which the C regtest server uses as its saturation value for an oversized
  integer on the wire;
- close to the ceiling, so that code which reads one as a count throughout (a release whose
  setter stores it) behaves as with a count longer than any legal series: no output, or
  `TA_INSUFFICIENT_HISTORY`, never an index out of range.

Write each macro as `(TA_INDEX_MAX+4)`: `scripts/abi.py` measures a macro only when its text is
capitals, digits, parentheses and operators, and an unmeasured macro is not gated.

The ports hold each constant on `Core` under the constant naming rule:
`Core::UNSTABLE_AUTO_PREC_4` (a `u32`, the type the setter takes and the getter returns),
`Core.UNSTABLE_AUTO_PREC_4`, `Core.UnstableAutoPrec4`. The builder call is the existing one:

```rust
let core = Core::builder()
    .unstable_period(FuncUnstId::ALL, Core::UNSTABLE_AUTO_PREC_4)
    .build()?;
```

### 4.2 The getter returns what was set

`TA_GetUnstablePeriod` and the three `Core` getters return the stored value, so under Auto they
return `TA_UNSTABLE_AUTO`.

- Save and restore keeps working: `saved = get(id); ...; set(id, saved)` puts Auto back. A getter
  that answered 0 would turn Auto off on the way through, silently.
- Code that adds the getter's value to its own lookback arithmetic gets a number that is
  obviously wrong (a hundred million) instead of one that is quietly short. The specification
  already tells callers to ask for the lookback and never compute it.
- No getter for "the count Auto resolves to" is added. That count is the function's lookback
  under Auto minus its lookback at 0, and the lookback call is how a caller asks.

### 4.3 What does not change

- **Default.** `TA_Initialize` and `TA_Shutdown` leave every id at 0. Auto does not survive a
  re-initialisation.
- **The owner's values from bar 0.** An id never changes what its owner reports from bar 0:
  under Auto, as under any fixed count, a function whose lookback reads its own id and no other
  reports the same values minus the bars it no longer reports (rule rL6). That does not carry to
  a function built from several stages or legs. A stage seeded on an earlier stage's first
  reported bars moves its seed window with every change of count, as it does today between two
  fixed counts (DEMA, TEMA, TRIX, the MACD signal, MASSI, TSI, SMI, KDJ, and STC through EMA's
  id). Their values from bar 0 move within the tolerance. SUPERTREND seeds its trend and bands
  on the first reported bar, so its values near that bar can move by a band width, as they do
  today between two fixed counts.
- **Batch.** A range that ends before the Auto lookback answers `TA_SUCCESS` with no output.
- **Streams.** `Open` needs `lookback + 1` bars and answers `TA_INSUFFICIENT_HISTORY` below that.
  A stream reads the unstable period inside `Open` only, in all four languages, and no later bar
  depends on a stored count: the count is resolved once, from the setting in effect at `Open` and
  the stream's parameters, and a handle is never re-warmed. In C the rule that no setting changes
  while a stream is open stands.
- **The abstraction layer.** `TA_GetLookback` forwards to the function's lookback and needs
  nothing.
- **ABI.** Each macro is an addition: no soname change. The PR carries the `ABI.manifest` that
  `scripts/sync.py` rewrites.

### 4.4 ta-lib-python

No new call. The wrapper's existing `set_unstable_period(name, period)` takes a level as it
takes a count, with the same meaning as in C: `set_unstable_period('ALL', AUTO_PREC_4)` puts
every id on Auto, and a per-id call after it sets one id to another level, to a count, or to 0.

The wrapper already asks C for the lookback on every call, with that call's parameters, in its
function, abstract and stream APIs, so the NaN padding follows with no change. Three things it
does need:

- the level constants, taken from the header it compiles against;
- an id table that names every id. Per-id control reaches only the ids the table knows, and it
  is behind the C enum;
- a clear error when the shared library it runs against predates Auto. Today that is a generic
  `TA_BAD_PARAM` exception.

Integer outputs are padded with 0, not NaN. At `PREC_4` the first 343 bars of HT_TRENDMODE read
"not in trend mode", against 63 today. The wrapper's page should say so.

## 5. Mechanism

### 5.1 One read of an id, in its owner's lookback

Today an input `.c` reads `TA_GetUnstablePeriod(TA_FUNC_UNST_X)` wherever it needs the count: in
the lookback, and in many bodies again as a skip counter, as a repeated lookback expression, or
in a seed index. A count that depends on the parameters has to be the same count at every one of
those sites, and a site that disagrees with its lookback reads bars the lookback did not
reserve.

So the first step removes the other sites, with no change to any output:

- A body takes the count from its own lookback: `unst = lookbackTotal - <structural part>`. EMA
  and RSI are already written this way.
- A function reads only its own id. EFI's lookback, which reads EMA's id, becomes
  `1 + ema_lookback(n)`, the expression its comment already gives.
- `generate` refuses an unstable read anywhere but the lookback of the function that owns the
  id, refuses a live id that no lookback reads, and refuses a function flagged
  `unstable_period` whose lookback reads none. Today the flag and the id are tied only by a
  test. A lookback may read its id once per return arm, as KAMA's and VIDYA's do.

### 5.2 The read carries the rule

The dialect's read gains a second argument, the Auto count for this call as an expression in
`K`:

```c
int ema_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1
        + TA_UNSTABLE( TA_FUNC_UNST_EMA, ta_warmup_ema(K, optInTimePeriod) );
}
```

`TA_UNSTABLE(id, count)` is the setting when it is a count. When the setting is a level, it is
`count` with `K` and `X` bound to that level's values: the generator renders one inlined
expression per level and a select on the stored value. With every read in the owner's lookback,
the rule is written once per id, in the helper its reads name.

- **The formulas are helpers.** `ta_warmup_ema`, `ta_warmup_wilder` and the per-function ones
  live in `ta_codegen/input/helpers/`, where single-return `int` and `double` functions are
  inlined into all four backends. Every helper takes `K` or `X` as an argument, and the read
  supplies them from the stored level: the dialect has no named constants. No lookback calls a
  helper today, so this path is new coverage. The shared type classifier treats a `ta_` call as
  floating point unless its name is on a short list of integer helpers: the new integer helpers
  join that list, or the classifier learns to read a helper's return type.
- **Four rules go through a `double`, which needs care in Rust.** SWAK_BP and MAMA divide by a
  real parameter. KAMA and VIDYA take an integer square root, which the dialect writes
  `(int)sqrt((double)n)` as HMA's lookback does. The Rust backend renders `(int)` of a `double`
  as a cast to `usize`, accepts it only as the whole right-hand side of an assignment, and does
  not see a cast that arrives through an inlined helper. So each of the four lookbacks stages
  the cast through an `int` local before its read: the root for KAMA and VIDYA, and for SWAK_BP
  and MAMA one count per level, since `K` sits inside their `ceil`. A further level then adds a
  local to those two lookbacks.
- **The lookback stays straight-line.** The ring-lag analysis inlines a callee lookback only
  when it is declarations, assignments and one return. An `if (auto)` inside `ema_lookback`
  would make it opaque to every caller's proof; a call expression does not. The analysis matches
  the read by name and has to learn the new spelling, or every lag that contains a read is
  silently unproven. Two reads of one id at different periods no longer cancel in a difference,
  so a proof that relied on it keeps its run-time guard: step 2's regeneration diff has to be
  read for guards that reappear.
- **"An unstable id without a rule" is refused at `generate`.** The parser checks no arity
  today, and the render arms fall back to slot 0 on a malformed read. The one-argument read is
  refused by name, and a two-argument read whose first argument is not this function's id is
  refused, in the same up-front gate as the checks of 5.1. That is the issue's first "done
  when".
- **Rendering.** One arm per backend serves the lookback, the batch body and the stream opener.
  C renders a macro beside today's `TA_GLOBALS_UNSTABLE_PERIOD`; Rust, Java and C# a select on
  the stored value.
- **Hand-written places that learn the level constants:** the C setter; the constant and the builder
  bound in the Rust template, Java and C#; the Java server's embedded `Core`; and the Java and
  C# servers' own `set_unstable_period` handlers, which restate the bound. The C and Rust
  servers call the library setter.
- **No rule calls `log`, `exp` or a trigonometric function.** The four that go through a
  `double` use a multiplication, a division, a `ceil` or a square root, each correctly rounded
  everywhere. VIDYA's saturation is `min(count, 100000000)`, taken before the product can
  overflow.

### 5.3 Coincidences that one count per id was hiding

Three bodies place a leg by a difference of periods: MACD, MACDFIX and STC seed their fast EMA
on the tail of the slow seed window. With one count per id that bar is exactly where the leg's
own lookback would put it. Under Auto it is not, because the count depends on the period.

- **MACD must move.** An all-EMA MACDEXT delegates to MACD in batch, while its stream composes
  three MAs, each anchored at its own lookback, and `stream_verify` holds the two bit for bit.
  MACD anchors its fast EMA at `ema_lookback(fast)`. The anchor is the same bar for any fixed
  count, so no value moves today.
- **STC should move with it.** Its regression suite holds the STC line bit for bit to
  `TA_EMA(fast) - TA_EMA(slow)` at non-zero EMA counts. Re-anchored, the identity holds under
  Auto and that leg can take an Auto pass. Left alone, its fast leg gets the slow leg's count
  and the identity becomes a fixed-count gate.
- **MACDFIX stays.** It has no composed twin, and its fixed factors mean no `TA_EMA` composite
  could hold a moved leg. Its 12-bar leg keeps `E(26)`.

The re-anchored seed path differs from today's only when two periods resolve different counts,
which no fixed count produces. Step 1's gates cannot exercise it; its first gates are in step 3.

Other functions start a leg earlier than its own lookback by design (ADOSC, CRSI, and APO, PPO
and PVO on their shorter leg). They are not coincidences and do not move.

Two identity gates need two ids to resolve the same count at the same period: `RMA(TRANGE)`
against ATR, and RVI against its two RMA legs. RMA, ATR and RVI all resolve `W(n)` on Auto and 0
at a period of 1. Changing one of the three rules breaks a gate, and so does putting only one id
of a pair on Auto.

### 5.4 Integer range

At `PREC_8`, the larger level: every rule but VIDYA's is a multiple of one period, of its square
root, or a constant, except SWAK_BP (`P/delta`, about 133000 bars at its bounds) and MAMA (4280
at its smallest limits). At the period cap of 100000 the largest is MCGD's, 4.0 million bars (ADX
2.5 million, T3 1.95 million), and the deepest lookback, STOCH, KDJ or KSTEXT with TEMA in both
MA slots, is 6.4 million: a sixteenth of `TA_INDEX_MAX`, so a long enough legal series always
meets it.

VIDYA's rule multiplies one period by the square root of another. It passes `TA_INDEX_MAX` only
near the caps: from a time period of about 19800 with the CMO period at its cap, and from about
34000 when the two are equal. It saturates there, computed so the product cannot overflow, and a
call at the ceiling reports nothing, as with a fixed count at the ceiling today. Two saturated
terms in one lookback (VIDYA in both MA slots of STOCH) sum to 2e8, inside `int`.

No guard is needed beyond the one at the setter.

## 6. Verification

### 6.1 The two-start leg

For every function, on each series separately: compute from bar 0 and from later starts, and
compare at every bar both report.

- **Class.** A function flagged `path_dependent` is an accumulation or a state machine. Any
  other call is a window when its Auto count is 0 and converging otherwise, so an MA dispatch at
  its SMA default is a window.
- **Windows.** Equal bits for the functions the harness already lists as exact. For the rest,
  `abs(a - b) <= T` with `T = max(1e-10, 1e-7 * R)` and `R` the output's range over the compared
  bars. Today's absolute 1e-10 holds on the 252-bar history only. Integer outputs, the candle
  patterns among them, are compared for equal values. For the candle functions that read an
  average this holds only while their totals sum exactly, which needs two-decimal prices whose
  bar ranges are small against the price. That is a property the corpus has to have, and the leg
  states it.
- **Converging calls.** With every id at 0, the largest difference between the two runs over
  all of the function's outputs and all starts on the series is its seed difference `S`. With
  every id on Auto, every compared bar must satisfy `abs(a - b) <= max(e^-(K-3) * S, T)`. A call
  whose `S` is under `T` is not counted as a comparison. The 3 in the exponent, a factor of 20,
  is for outputs that divide by a state or by a range of states, and for differences of two
  smoothed legs whose seed errors partly cancel in `S`: they pass `e^-K` at their count by up to
  a fifth (STOCHRSI at `PREC_4`: 171 bars against 140).
- **The count itself.** For each id, with that id alone on Auto, the owner's lookback minus its
  lookback at 0 must equal the rule, which the test computes from its own copy of the table in
  section 3.2. A linear kernel gets the same allowance as a ratio, so the criterion alone would
  pass a rule a third too short for it at `PREC_4` and a sixth at `PREC_8`; this is what holds
  the rules exactly, and what catches a helper transcribed wrong.
- **Accumulations and state machines** are not compared. A function flagged `path_dependent`
  that meets the converging criterion on every series and start, with `S` above `T` on at least
  one, fails the leg: that is how a wrong flag is caught. NVI and PVI can be
  identical from two early starts, which is why one series is not enough.
- **Stated exclusions.** KAMA, FRAMA, VIDYA and the KAMA and VIDYA MA arms are compared on the
  trend and random-walk series only: their counts are sized for those (section 9, D3), and the
  range-bound series takes several times longer. STOCHRSI with the MAMA or the VIDYA type is not
  compared: its FastK rests at 0 or 100, where MAMA does not converge and the VIDYA coefficient is
  0. PVO with the KAMA or the VIDYA type is compared only if the corpus's volume itself trends or
  wanders: on volume drawn as noise those arms are range-bound on every series. CRSI is not
  compared when its Auto count is above 0: its streak is a short-lived state machine (section 3.5)
  that restarts a difference at the first reversal, whatever the warm-up. MAXINDEX, MININDEX and
  MINMAXINDEX are compared on bars whose window holds its extreme once, until their tie-break is
  start-independent.
- **Vectors.** Defaults; every integer period at its minimum; every integer period tripled;
  every MA type on every MA-type parameter; and the real parameters a rule reads: SWAK_BP's
  delta at 0.05 and 0.5, and MAMA's limits at (0.01, 0.01), (0.99, 0.99), (0.99, 0.01) and
  (0.01, 0.99), the last with fast below slow. A rule is only tested by moving what it scales
  with.
- **Corpus.** Three series of their own: a trend, a random walk and a range-bound one. The longest
  Auto lookbacks among the vectors, at `PREC_8`, are MAMA at its smallest limit at about 4300 bars
  and VIDYA(36, 27) at about 3000, so each series is 8192 bars, with later starts no further than
  bar 1000. All three series are compared for the proven rules and for the Hilbert, MAMA and STC
  counts; the range-bound one is where the Hilbert and MAMA counts are slowest.
- **Levels.** The count check runs at every level. The two-start comparison holds `PREC_4`
  wherever `S` is more than about 1e-4 of the output's range: its threshold is about 1e-3 of
  `S`. Where `S` is smaller (T3 and its MA arm, bands over TEMA) `T` is the criterion and the
  count check alone holds the level. At `PREC_8` the threshold is about 1e-7 of `S`, the size of
  the window tolerance, so with `T` as floor the second pass would show nothing. For that pass
  the floor is a committed number per function and output: the largest two-start difference
  over the last quarter of each corpus series with every id at 0, times 4, measured once when
  the corpus is committed and not in the run it gates. Most outputs reach bit-identity and get
  a floor of rounding size; ADOSC, MAMA, MASSI, RVI, RVIR, the Hilbert period, phase, phasor and
  sine outputs, and bands over a recursive MA type do not. An output whose floor is above its
  `PREC_8` threshold is held at `PREC_8` by the count check alone, and the leg lists it.
- **Non-vacuity.** One counter per class with a floor, and a floor on the number of compared
  bars per call: an Auto lookback longer than the series compares nothing.
- **Where.** In-process C, in the bare `ta_regtest` run, so it runs in every nightly job. No PR
  gate runs `ta_regtest` today; the generate-time refusals of section 5 are the PR-gate half.

### 6.2 Existing gates that change

- **The setter's bounds.** `TA_INDEX_MAX + 1`, 2147483647 and 4294967295 are asserted refused
  in C, in each port's own tests and on the wire for every server, and stay refused. Pins are
  added for each level's value (accepted, read back) and for an offset that is no level
  (refused).
- **Lookback equality across languages.** Today's comparison under a non-zero setting runs at
  the default parameters only, and the per-vector lookback leg pins every id at 0. An Auto count
  depends on the parameters, so the Auto pass is the per-vector leg with every id on Auto: its
  vectors already carry the integer minima and every MA type, and gain one with every integer
  period at its maximum, the only way to reach VIDYA's saturating arm. This is the one check
  that the four renderings of every rule agree.
- **Stream against batch.** The unstable leg of `stream_verify` gains an Auto pass, including
  the short-history rejection at the Auto lookback. Its series is 240 bars and each server caps
  a request at 256: under Auto many of the default calls an unstable id reaches report nothing
  there, most of them at `PREC_8`. The pass needs a longer series, with the cap raised in the
  four servers, or shorter periods.
- **The MACDEXT diagonal.** The all-EMA vector that reaches the MACD delegation is the defaults
  only. Its Auto lookback is 218 bars at `PREC_4`, which leaves 22 bars of that 240-bar series,
  and 385 at `PREC_8`, which leaves none. The gate of section 5.3 needs a longer series or an
  all-EMA vector with different fast and slow periods that fits, such as (2, 3, 2).
- **"Exactly that many bars."** The shift test raises an id by 5 and requires five more bars and
  the same remaining values. Under Auto the same test requires the rule's count and the same
  remaining values. It runs on 252 bars with 512-bar buffers, where many owners report nothing
  under Auto: it needs the series of section 6.1, or its "same remaining values" half moves into
  that leg.
- **The first bar read.** The check that poisons every bar before `startIdx - lookback` runs at
  unstable periods 0 and 5, at the defaults, on 252 bars. It gains an Auto pass on the longer
  series: with a count that depends on the period it is the run-time check for section 5.1, that
  no site reads a bar its lookback did not reserve.
- **The range sweep's envelope** reads `TA_GetUnstablePeriod` as a number. It runs with fixed
  counts and keeps doing so.
- **Instruction counts.** A select executed once per call adds a handful of instructions to rows
  of about 140,000 or more. Nothing is listed, but the baseline only moves down, so the drift
  stays against the old count until a row is accepted by name. No stream step reads the setting,
  so the per-bar rows do not move.

## 7. Documentation

| Page | Change |
|---|---|
| `api/unstable-period` | Auto under approach 3, leading with `PREC_4`; what a level means (section 2.1); the rule table; the tiers; what the promise is and is not. The figure's "stable" boundary is drawn at 1e-3 of price and would contradict the Auto count: redraw or recaption |
| `functions/stability` and every function page (generated) | the rule beside "Initial Unstable Period"; the MA-type table gains the `M(n, type)` column. Derived from the same lookback read as today's line |
| spec rL6 | "adds exactly that many bars" holds for a count; under Auto the id adds its rule's count |
| spec rL8 | under Auto a `period1_identity` function has a lookback of 0 at a period of 1 |
| spec rL5 | still true: the count depends on parameters and settings only |
| spec lookback, the inheritance recipe | "every id set above that first lookback" has to name a count, not `TA_UNSTABLE_AUTO` |
| spec rT3, or a new rule beside it | the accepted values are `[0, TA_INDEX_MAX]` and `TA_UNSTABLE_AUTO`, and a getter returns the value the setter stored. rT4, the wildcard read, is unchanged |
| spec streaming | no rule changes: `lookback + 1` already follows the setting |
| the four streaming API pages | the setting is resolved once at `Open`; the history an `Open` needs under Auto is the Auto lookback plus one |
| spec constants table | a row per level |
| `website/config/llms-key-facts.md` | the unstable-period fact: a count drops that many outputs, Auto drops the rule's count |
| `docs/spec-conformance.md` | the rT3 rationale, the rL4 and rL6 gate descriptions, the new leg |
| input `.md` files that state a fixed-count relation by hand (MASSI, ERI, STC and others) | reworded so they hold for both |
| `docs/ta_codegen_input_code.md`, the contributor page and the `new-ta-func` skill | an id needs a rule, its tier and its helper; the read takes two arguments, only in its owner's lookback |

The level's meaning is on the Unstable Period page only, with links to the specification's
rules for the mechanics (section 9, D5).

## 8. Order of work

1. **One read per id** (section 5.1) and the MACD and STC anchors (section 5.3), with the
   generate-time refusals. No output changes. The gates that can see one are `ref` and the bare
   regtest; `--codegen` and `xlang-hash` compare the languages with each other and show only
   that they still agree.
2. **The level constants and the rules.** The constants in four languages, the setters, the
   servers, the two-argument read, the helpers.
3. **The leg** of section 6.1 and the Auto passes of section 6.2. A rule that fails here is
   changed here.
4. **Pages and specification.**
5. **ta-lib-python**, as a PR into its `dev`.

Steps 1, 4 and 5 each leave the tree releasable. Steps 2 and 3 are one releasable unit: a
lookback is what callers size history by, so either they land together or step 2 keeps every
setter refusing `TA_UNSTABLE_AUTO` until the leg is green.

The tie-break of the index functions either
lands before step 3, or the leg names it as an exemption that the later change removes.

## 9. Rulings

Every decision this design needed is ruled.

**D1. The levels.** Ruled (owner, 2026-10-04):

- Auto is a value of the existing unstable-period setting in every language, Python included.
  One can set a fixed count or an `AUTO_PREC_x` level, per id or for all, with no change of
  meaning. There is no separate switch such as `set_auto_warm_up`.
- The first two levels are `TA_UNSTABLE_AUTO_PREC_4` and `TA_UNSTABLE_AUTO_PREC_8`, with the
  values `TA_INDEX_MAX + 4` and `TA_INDEX_MAX + 8`. More may follow.
- `PREC_4` is the level the pages lead with.

Bars added to the lookback, by digits of seed weight:

| Level | `K` | EMA(30) | EMA(200) | RSI(14) | MACD(12,26,9) | KAMA(30) |
|---|---|---|---|---|---|---|
| 3 | 7 | 109 | 704 | 98 | 130 | 375 |
| 4 | 10 | 155 | 1005 | 140 | 185 | 500 |
| 6 | 14 | 217 | 1407 | 196 | 259 | 750 |
| 8 | 19 | 295 | 1910 | 266 | 352 | 1000 |
| 10 | 24 | 372 | 2412 | 336 | 444 | 1250 |

- **Why 4.** On the price scale a moving average's two-start difference is about 1% of the
  price before any warm-up, so a weight of 1e-4 leaves about one part in a million of the price:
  far below a cent on a price between 1 and 99. It is 29% fewer bars than level 6.
- **Why 8 and not 10.** At `PREC_8` every converging function but RVI comes within `e^-19` of
  its seed difference at the default parameters; RVI's own variance arithmetic stops it near
  6e-9. At level 10 about ten outputs stop short for the same reason (HT_DCPHASE, HT_SINE, the
  MAMA line, ADOSC, HT_DCPERIOD, HT_PHASOR, FAMA, RVIR).
- **Why not "bit-level".** The issue's `K = 37` costs 3.7 times the bars of `PREC_4` and 1.9
  times those of `PREC_8`, and it could not be promised as bit-identity: about ten converging
  outputs never become bit-identical on the study's series, because rounding in their own
  arithmetic takes over.
- **Cost of a level.** Every rule is a function of `K`, and the proven ones were checked at seven
  values from 7 to 37, 10 and 19 among them, so a level is one constant, one accepted value, and
  one more local in the two lookbacks that divide by a real parameter. It is also one more pass of
  every Auto gate, and one more sizing of the calibrated rules. The identity gates of section 5.3
  need both ids at the same level.

**D2. The getter under Auto.** Ruled (owner, 2026-10-04): it returns the level constant
(section 4.2). A caller observes the effect of a level through the lookback call.

**D3. Bounds or typical counts for the adaptive averages.** Ruled (owner, 2026-10-04): typical
counts, each a constant times the level's digit count `X`, with a period term where the need
grows with the period.

| Function | Count | `PREC_4` / `PREC_8` | Need, trend or random walk | Need, range-bound |
|---|---|---|---|---|
| KAMA(30) | `25*X*isqrt(n)` | 500 / 1000 | 329 / 550 | 2148 / 4054 |
| KAMA(90) | | 900 / 1800 | 570 / 1245 | 2154 / 4059 |
| FRAMA(16) | `80*X` | 320 / 640 | 113 / 216 | 514 / 937 |
| FRAMA(48) | | 320 / 640 | 159 / 194 | 586 / 1044 |
| VIDYA(12, 9) | `2*X*(n+1)*isqrt(m)` | 312 / 624 | 210 / 380 | 581 / 1102 |
| VIDYA(36, 27) | | 1480 / 2960 | 1073 / 2006 | 4990 / 9482 |
| MCGD(14) | `5*X*n` | 280 / 560 | 151 / 286 | 137 / 260 |
| MCGD(42) | | 840 / 1680 | 584 / 1103 | 429 / 806 |

("Need" is the bars until two starts stay within the level's weight of their largest
difference, at `PREC_4` and `PREC_8`.)

- **What was given up.** KAMA and FRAMA have a guaranteed count, from the floor of their
  smoothing constant: 2403 and 1000 bars at `PREC_4`, whatever the period. It asks for about ten
  years of daily bars before KAMA reports anything, so the typical count was chosen. VIDYA and
  MCGD have no guaranteed count at all.
- **What the typical count does not cover.** A range-bound market: these averages slow down there
  by design, and the last column is what they then need. The study's range is a one-bar
  alternation, the hardest case; with an even CMO period VIDYA's coefficient is near zero there
  and it converges more than ten times slower still. MCGD is the exception: it is slowest in a
  trend, and its count covers all three series at the periods measured.
- **Why the period terms.** KAMA's typical need grows about with the square root of its period
  (329 bars at 30, 570 at 90, at `PREC_4`), because a longer efficiency window sees a smaller
  ratio. VIDYA's follows its EMA period and the square root of its CMO period. MCGD's follows
  its period, as Wilder's does. FRAMA's barely moves.
- **What a caller can do.** Set a fixed count on that id. The floor-based figures above are the
  safe ones for KAMA and FRAMA.

**D4. A level as a plain value in Rust, Java and C#.** Ruled (owner, 2026-10-04): a plain
constant through the existing builder call, like the other backends. No signature changes.

**D5. Publishing the tolerance.** Ruled (owner, 2026-10-04): split by audience. The Unstable
Period page explains a level for a human reader: the significant-digits wording of section 2.1,
the count per id, and which counts are proven and which are sized by measurement. The
specification states the mechanics only, each held exactly by a test: the setter accepts a count
or a level, the getter returns what was set, under a level an id adds its rule's count, and the
count depends only on parameters and settings. The Unstable Period page links to those rules for
the advanced details. The specification states no tolerance, so the 2026-10-03 ruling on #497
("no figure on the pages") stands for it.

**D6. Path-dependent functions that inherit an id.** Ruled (owner, 2026-10-04):

- The inheritance stays mechanical. A level lengthens the lookback of ADOSC (through EMA) and
  of SUPERTREND (through ATR) as a fixed count does today.
- ADOSC is reclassified: it loses its `path_dependent` flag and reads "Initial Unstable Period,
  inherited from EMA". Both of its EMAs are seeded on the same first A/D value, so the constant
  offset between two starts' A/D lines cancels in the difference and what remains decays at the
  slower EMA's rate: at the defaults, within `e^-10` after 51 bars, against a `PREC_4` count of
  55. Done in issue #502, ahead of the two-start leg, which would otherwise have reported ADOSC
  as wrongly flagged.
- SUPERTREND stays path-dependent.

**D7. The Hilbert functions, MAMA and STC.** Ruled (owner, 2026-10-05): the counts below, each
with a fixed part and a part that grows with the level, so that neither level is over-sized.

| Function | Count | `PREC_4` / `PREC_8` | Worst need measured | Margin |
|---|---|---|---|---|
| the six Hilbert functions | `80 + 50*X` | 280 / 480 | 235 / 396 | 19% / 21% |
| MAMA(0.5, 0.05) | `H + ceil(2*K/max(fast, slow))` | 320 / 556 | 138 / 290 | 2.3 / 1.9 times |
| STC(69, 150, 30) | `2*K + 3*(s+1)`, on top of `E(s)` | 1228 / 1926 | 995 / 1562 | 23% / 23% |

MAMA's need follows its larger limit when fast is not below slow, measured at these settings:

| fast, slow | Need, `PREC_4` / `PREC_8` | Count |
|---|---|---|
| 0.5, 0.05 | 138 / 290 | 320 / 556 |
| 0.5, 0.01 | 153 / 306 | 320 / 556 |
| 0.2, 0.05 | 227 / 417 | 380 / 670 |
| 0.2, 0.02 | 294 / 534 | 380 / 670 |
| 0.1, 0.05 | 331 / 607 | 480 / 860 |
| 0.05, 0.05 | 418 / 773 | 680 / 1240 |
| 0.02, 0.02 | 1054 / 1949 | 1280 / 2380 |

- With fast below slow alpha rests on the smaller limit for part of the time. MAMA(0.01, 0.02)
  needs 1384 / 2534 bars against a count of 1280 / 2380: 8% and 6% past it at the level's
  `e^-K`, while the leg's threshold holds (992 / 2144).
- A pure multiple of `X` was rejected for the Hilbert count: the need grows by 69% from `PREC_4`
  to `PREC_8`, not by 100%, because part of it is the pipeline's own settling time.
- The count for MAMA stays generous at its default limits: it carries the full Hilbert count,
  of which MAMA needs less than the Hilbert functions do.
- At limits of 0.9 and 0.3 MAMA cannot show `PREC_8`: its output tracks the price so closely
  that rounding in the Hilbert state dominates.
- All three are sized on price series and held by the two-start leg only. MAMA and the Hilbert
  functions do not hold on an input that rests on one value (section 2).

**D8. Where this design departs from the issue as first filed (2026-09-30).** Ruled (owner,
2026-10-05): accepted. The issue's text was then rewritten to match this design.

- `K` is 10 or 19 by level, where the issue asks to choose between 14 and 37 (D1).
- T3 is `ceil((K+20)*(n+1)/2)`, 90 bars at the default and `PREC_4`, where "the sum of its six
  EMAs" is 180. The six stages are stepped together, so one longer count bounds them.
- ADX is `(K+6)*n`, 224, where the Wilder family's `K*n` is 140: it is two stages.
- The five SWAK ids, absent from the issue's table, each have a rule.
- Four rules go through a `double`: SWAK_BP and MAMA divide by a real parameter, KAMA and VIDYA
  take an integer square root. The issue asks for integer arithmetic so that every backend
  agrees; these operations are correctly rounded and agree too.
- The promise is the seed's weight in the states, not agreement of the outputs within the
  tolerance whatever the start: ratio outputs pass `e^-K` at their count by up to a fifth, and
  the calibrated tier is not a bound.
- Path-dependent functions that inherit an id are affected (D6).
- The adaptive averages use typical counts (D3), where the issue names "a bound from the slowest
  alpha, or a measured bound".
- No `set_auto_warm_up` in ta-lib-python: its existing `set_unstable_period` takes a level (D1).

## 10. Evidence

[`studies/auto-warm-up/`](studies/auto-warm-up/README.md) holds the probe, its analysis and the
results. In short:

- **Method.** Every function through the abstraction layer, on 40000 bars of three synthetic
  series (random walk, a one-bar zigzag in a slow drift, alternating 5000-bar trends), from bar
  0 and from starts 1, 2, 5, 33, 250 and 1000, at the defaults, with every integer period
  tripled, and with each MA type. For each output, the age (bars since the later start's first
  output) from which the two runs stay within `e^-K` of their largest difference, for seven
  values of `K` from 7 to 28, and from which they agree to 4, 6 and 8 significant digits of the
  value.
- **Classes.** At the default parameters every function landed in the class its source
  predicts, with the exceptions of section 3.5: pointwise functions and windows bit-identical or
  within 8e-10 of range, accumulations never, everything else converging. At tripled periods
  CORREL(90) reaches 3.5e-8 of range.
- **Rules against need.** Worst case over the three series and six starts, in bars after the
  lookback at 0, against the largest difference between the two runs. "Need" is at the level's
  `e^-K`; "leg" is at `e^-(K-3)`, what the leg of section 6.1 holds.

| Call | `PREC_4` rule | Need | Leg | `PREC_8` rule | Need | Leg |
|---|---|---|---|---|---|---|
| EMA(30) | 155 | 150 | 105 | 295 | 285 | 240 |
| EMA(90) | 455 | 450 | 315 | 865 | 856 | 720 |
| RMA(30) | 300 | 295 | 207 | 570 | 561 | 472 |
| ATR(14) | 140 | 135 | 95 | 266 | 257 | 216 |
| RSI(14) | 140 | 149 | 117 | 266 | 265 | 226 |
| RSI(42) | 420 | 430 | 312 | 798 | 823 | 695 |
| DX(42) | 420 | 444 | 347 | 798 | 855 | 746 |
| ADX(14) | 224 | 179 | 134 | 350 | 308 | 266 |
| ADX(42) | 672 | 556 | 416 | 1050 | 949 | 818 |
| T3(5) | 90 | 34 | 27 | 117 | 66 | 51 |
| T3(15) | 240 | 119 | 89 | 312 | 237 | 190 |
| HA | 20 | 15 | 11 | 38 | 28 | 24 |
| SWAK_HP(20) | 34 | 32 | 22 | 64 | 60 | 51 |
| SWAK_BUTTER(20) | 37 | 28 | 21 | 59 | 48 | 41 |
| SWAK_BP(20, 0.1) | 367 | 322 | 228 | 667 | 609 | 512 |
| KAMA(30), trend and random walk | 500 | 329 | 247 | 1000 | 550 | 477 |
| FRAMA(16), trend and random walk | 320 | 113 | 71 | 640 | 216 | 186 |
| VIDYA(12, 9), trend and random walk | 312 | 210 | 151 | 624 | 380 | 313 |
| MCGD(14) | 280 | 151 | 106 | 560 | 286 | 241 |
| MCGD(42) | 840 | 584 | 410 | 1680 | 1103 | 935 |
| Hilbert functions, worst | 280 | 235 | 187 | 480 | 396 | 338 |
| MAMA(0.5, 0.05) | 320 | 138 | 104 | 556 | 290 | 240 |
| STC(23, 50, 10), with `E(50)` | 428 | 280 | 209 | 676 | 529 | 442 |
| STC(69, 150, 30), with `E(150)` | 1228 | 995 | 663 | 1926 | 1562 | 1438 |
| MACD(12, 26, 9) | 185 | 143 | 104 | 352 | 260 | 221 |
| DEMA(30) | 310 | 208 | 158 | 590 | 350 | 303 |
| TEMA(30) | 465 | 228 | 176 | 885 | 377 | 328 |
| KC(20, 10) | 105 | 100 | 70 | 200 | 190 | 160 |
| PVO(12, 26) | 135 | 145 | 107 | 257 | 262 | 224 |
| STOCHRSI(14, 5, 3) | 140 | 171 | 110 | 266 | 304 | 243 |
| KDJ(9, 3, 3) | 60 | 32 | 24 | 114 | 56 | 48 |
| ADOSC(3, 10) | 55 | 51 | 36 | 105 | 95 | 81 |

  Every rule in this table, and every id owner at the defaults and at tripled periods, covers
  the leg's threshold at both levels. At `e^-K` the need passes the rule where the output
  divides by a state or by a range of states, by up to a fifth (STOCHRSI), and by a few percent
  in PVO and APO, where the two legs' seed errors partly cancel.
- **Significant digits.** At the rule's count the two runs agree to 4 significant digits of the
  value at `PREC_4`, and to 8 at `PREC_8`, for every id owner whose output stays away from zero.
  They do not for CMO, DX, SWAK_HP, SWAK_2PHP, SWAK_BP, HT_PHASOR and HT_SINE, whose outputs
  pass through zero, nor for HT_DCPHASE at 8 digits (section 2.1).
- **The MA-type runs.** The inherited cases that do not meet their count are the limits already
  stated: the KAMA and VIDYA arms on the zigzag, which the leg does not compare; the same two
  arms in PVO on every series, because the study's volume is noise; the VIDYA arm and the MAMA
  arm in STOCHRSI, whose FastK rests at 0 or 100; and BBW with the T3 type on the trend series,
  where the leg's threshold applied to a seed difference of 5e-6 is at the band's own rounding.
- **The proven rules were also checked against the kernels' decay laws**, at seven values of
  `K` from 7 to 37 and a grid of periods, with no violation.
- **Not measured.** Real market data; periods beyond three times the default; SWAK_BP away
  from its default delta; the ports (the rules are a few arithmetic operations, and the
  cross-language lookback gate is what holds them equal).
