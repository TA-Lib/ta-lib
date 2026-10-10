---
title: Auto-Stabilization
description: "The bars an Auto level of the unstable period adds for each function, what a level targets, and the known exceptions."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The bars an Auto level of the unstable period discards, and the known exceptions. What a level is and how to set one: [Unstable Period](/api/unstable-period/#auto). Accepted values: [rT3](/spec/settings-threads/#rt3). How the bars enter a lookback: [rL6](/spec/lookback/#rl6).

## The caller must {#caller}

- <a id="ask"></a>**Read the bars a level adds from the lookback call**, never from the table below ([ask](/spec/lookback/#ask)). The table says what that call computes. Validate against the release you ship ([pin](/spec/versions/#pin)).
- <a id="tolerance"></a>**Compare two starts with a tolerance, under a level too.** A level bounds what is left of the seed: two starts agree to the level's digits, not to the last bit ([rD3](/spec/versions/#rd3)).
  - The `E` and `W` counts of [rZ1](/spec/auto-stabilization/#rz1) have no bar to spare. At a long period the first value kept can sit above the level's threshold by the rounding of the arithmetic; the digits the level names still hold.
  - An output that divides by a state (DX, CMO, STOCHRSI, PPO, PVO) can sit a few times above the threshold at the count.
  - CVI divides its EMA by the same EMA some bars earlier, and can sit above the threshold for some bars past the count it inherits.
- <a id="check-exceptions"></a>**Check the [known exceptions](/spec/auto-stabilization/#exceptions) for the functions you use.**

## Bars a level adds {#bars}

<a id="rz1"></a>**rZ1** Under an Auto level, the function that owns an id adds the count of its row to its lookback. Each row is integer arithmetic, except `SWAK_BP` and `MAMA`: theirs is the `double` expression as written, rounded up. `X` is the level's digit count and `K` its factor: `PREC_4` is `X = 4`, `K = 10`; `PREC_8` is `X = 8`, `K = 19`. `K` is the level's number of e-folds: a `proof` count leaves at most `e^-K` of the seed's weight in the function's state. The level's threshold is `e^-K` of the largest difference two starts show at a setting of 0. `n` is the function's period, `isqrt` the integer square root, `E(n) = ceil(K*n/2)`, `W(n) = ceil(K*(2*n-1)/2)` and `H = 80 + 50*X`.

| Id | Bars added | Sized by |
|---|---|---|
| `EMA` | `E(n)` | proof |
| `RMA`, `ATR`, `PLUS_DM`, `MINUS_DM` | `W(n)` | proof |
| `NATR`, `RSI`, `CMO`, `PLUS_DI`, `MINUS_DI`, `DX`, `RVI` | `K*n` | proof, ratio |
| `ADX` | `(K+6)*n` | proof, ratio |
| `HA` | `ceil(13*K/9)` | proof |
| `SWAK_HP` | `ceil(K*n/6)` | proof |
| `SWAK_GAUSS`, `SWAK_BUTTER`, `SWAK_2PHP` | `ceil((K+3)*(n+2)/9)` | proof |
| `SWAK_BP` | `ceil((K+1)*n/(6*delta))` | proof |
| `FISHER` | `ceil(5*(K+6)/2)` | proof, limiter |
| `T3` | `ceil(11*(X+4)*n/8)` | measurement |
| `KAMA` | `25*X*isqrt(n)` | measurement |
| `FRAMA` | `9*(X+4)*(isqrt(n)+2)/2`, at most `99*K` | measurement |
| `VIDYA` | `2*X*(n+1)*isqrt(m)`, `m` the CMO period, at most `TA_INDEX_MAX` | measurement |
| `MCGD` | `5*X*n` | measurement |
| `HT_DCPERIOD`, `HT_DCPHASE`, `HT_PHASOR`, `HT_SINE` | `H` | measurement |
| `HT_TRENDLINE`, `HT_TRENDMODE` | `120 + 20*X` | measurement |
| `MAMA` | `H + ceil(2*K/max(fast, slow))`, `fast` and `slow` its two limits | measurement |
| `STC` | `ceil(5*K/2) + 3*(s+1)`, `s = max(fast, slow)`, on top of the `E(s)` it inherits | measurement |

The `E`, `W`, `K*n`, `T3`, `KAMA` and `VIDYA` rows give 0 at a period of 1, where the function does no smoothing.

`proof`: the count bounds what is left of the seed, for any input. `measurement`: the count is a formula sized on measured series, with margin; it is not a bound ([a longer count](/spec/auto-stabilization/#fixed-count)). `ratio` marks an output that divides by a smoothed value or by a price, `limiter` a state snapped at a limit ([no level](/spec/auto-stabilization/#no-level)).

<a id="rz2"></a>**rZ2** Under a level, a function that owns no id adds these bars of its own to the count it inherits ([rL6](/spec/lookback/#rl6)); under a count it adds none. CKSP adds its stop period less one to ATR's count. ADOSC adds `ceil((15*f + 3*t)/8)` to EMA's count at the longer of its two periods, with `f` the shorter period, `s` the longer, and `t` the sum of `min(f, s/2^j)` for `j` from 1 to 16, in integer divisions.

## What a level targets {#target}

`PREC_n` targets `n` significant digits: past the count, the first `n` significant digits of a value no longer depend on where the series starts. Like [rD2](/spec/versions/#rd2), it is a target, not a promise.

- An output that stays away from zero is measured against its value: the moving averages and every output on the price scale, ATR, NATR, RSI, the DI and DM pairs, ADX, RVI, STC, the Hilbert period and trendline.
- An output that reaches zero has no leading digit to keep there, and is measured against the size of its swing, two starts differing by about `10^-n` of it: CMO, DX, SWAK_HP, SWAK_BP, HT_PHASOR, HT_SINE, HT_DCPHASE, and the oscillators centred on zero such as MACD, APO, PPO, TRIX and FISHER.

At `PREC_8`, where the rounding of a function's own arithmetic is larger than the level, the last digits keep moving.

## Known exceptions {#exceptions}

Outside the cases below, a level is expected to meet its target, within the [tolerance](/spec/auto-stabilization/#tolerance) above and, at `PREC_8`, down to the function's own rounding. Each exception, the path-dependent one aside, also reaches every function computed through the one it names, directly or as an MA type ([rL6](/spec/lookback/#rl6)). Most of those tied to the market need a condition that persists for many bars: prices that stay flat, hold a narrow range, or climb steadily.

<a id="exact-or-not"></a>**HT_TRENDLINE and HT_TRENDMODE agree exactly or not at all.** HT_TRENDLINE averages the price over a cycle period cut to a whole number of bars, so two starts are equal once their whole periods have agreed for four bars; HT_TRENDMODE is a flag. A level sets how rare a later disagreement is, not how small: no tolerance absorbs it.

<a id="fixed-count"></a>**A longer count covers these.** A `measurement` row of [rZ1](/spec/auto-stabilization/#rz1) is not a bound. Set a count on that id, in a call after the one that sets the level.

- KAMA, FRAMA and VIDYA are sized for a market that trends or wanders. They slow down by design in a range-bound market and then need several times their count. `99*K` bars hold FRAMA's level on any input; VIDYA has no such bound (below).
- T3: a start whose six stages nearly cancel in the first outputs shows its difference later and can need a fifth more than the count. A count of `20*n` covers it at either level.
- MCGD is sized for a line that tracks the price: its rate falls as the line trails. In a steady rise the count runs short once the period times the rise per bar passes about 0.06.

<a id="no-level"></a>**No level, and no count, covers these.**

- A flat market. RSI, CMO, the DI pair, DX, RVI, TSI, SMI and STC divide one smoothed series by another, and ADX averages such a ratio. When prices stop moving both series shrink together, and the ratio keeps its dependence on the start.
- A limiter. FISHER replaces a smoothed position beyond 0.99 by 0.999. Two starts on either side of 0.99 differ again by a visible amount, at any bar, and the count restarts from there. While the price holds close to the top or the bottom of its channel without resting on it, the two do not meet.
- A Hilbert input that rests on one value for a whole dominant cycle, as an oscillator pinned at 0 or 100 does. The phase is undefined: HT_DCPHASE, HT_SINE and HT_TRENDMODE from two starts can differ by any amount on those bars, and MAMA does not converge. When the input moves again, every Hilbert function starts over.
- A price worth a few thousand ticks or less, for MAMA. On a bar where its in-phase component is zero to rounding, two starts can take different alphas: the difference reopens by up to the distance from the price to the line, and stays open where that keeps happening. A trending series shows none.
- MCGD in a steady rise whose period times the rise per bar passes about 0.10. The line stays 20% under the price, where its rate is zero, and past that it amplifies its start: two starts agree only after the price comes back to the line.
- VIDYA while its CMO rests at 0, as on a balanced range or a flat market. Its coefficient is 0 and it keeps its seed.
- CRSI, while closes keep moving in one direction. Two starts carry different streaks until the first reversal after the later start, so a run longer than the count leaves their difference past it.
- STOCHRSI with the MAMA or VIDYA type. Its FastK rests at 0 or 100, where MAMA does not converge and VIDYA's coefficient is 0.
- A path-dependent function, which stays path-dependent: no level changes AD, OBV or SAR ([metadata](/spec/lookback/#metadata)). SUPERTREND computes an ATR, so a level lengthens its lookback and promises nothing about its line.
