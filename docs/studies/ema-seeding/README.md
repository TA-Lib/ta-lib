# EMA seeding evaluation

**Question.** `TA_EMA` (CLASSIC) seeds its recursion with an SMA of the first
`period` samples. TradingView `ta.ema`, Tulip, TradeStation `XAverage` and
TA-Lib's own METASTOCK compatibility mode seed with the **first sample**. Which
seed carries the smaller error?

**Verdict.** Keep the SMA seed — but the defensible claim is *"every value the
library publishes is one it stands behind"*, **not** *"the SMA seed is more
accurate"*. That second claim is false at a common bar and should not be made.
The split is quantified in §6 and §7.

Raised by [#238](https://github.com/TA-Lib/ta-lib/issues/238) (`TA_SMI`), which recommended `TA_EMA` on consistency grounds and never measures
accuracy — it only tabulates `|arm A - Tulip|`, the gap between two arms, with no
third party to say which one is wrong. This document supplies the third party and
generalises the answer to the whole EMA family. The SMI-specific numbers are in
[`smi-case-study.md`](smi-case-study.md); the code, data and full output are
beside this file.

---

## 1. What TA-Lib actually does, verified against the shipped library

`ema_probe.c` links `libta-lib.a` and calls the real `TA_EMA`. Three facts, all
bit-checked rather than read off the source:

* **The first output IS the plain p-bar SMA** ending at `startIdx` —
  `TA_EMA(400,400,close,25)` is bit-identical to `mean(close[376..400])`.
* **`TA_EMA` re-seeds at `startIdx - lookbackTotal`**, so it is start-dependent:
  `TA_EMA(400,400)` and `TA_EMA(0,500)[400]` differ by **0.199** on gData close.
  This matters for everything below — the "first published value" row is not a
  warm-up curiosity, it is what *any* narrow-range TA-Lib call returns.
* The study's model of the CLASSIC rule reproduces `TA_EMA(0,60,close,25)`
  **bit-for-bit, all 37 outputs** (the SMA seed must be summed sequentially, as
  the C does, or it lands 1 ULP away).

`TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, k)` moves the seed anchor back by `k` and
runs `k` extra recursion steps before publishing — i.e. it buys decay, not a
different seed.

## 2. The two rules, and who ships which

| | seed | first published bar | ships it |
|---|---|---|---|
| **rule S** | `SMA(x[t0-p+1 .. t0])` | `p-1` | TA-Lib CLASSIC, R `TTR::EMA` |
| **rule T** | `x[t0]` | `0` | TradingView `ta.ema`, Tulip, TradeStation `XAverage`, TA-Lib `TA_COMPATIBILITY_METASTOCK` |

R `TTR` is the closest independent analogue of what we ship: *"The EMA result is
initialized with the n-period sample average at period n."* TradingView's
`ta.ema` is `na(sum[1]) ? src : alpha*src + (1-alpha)*nz(sum[1])` — note that
Pine's `ta.rma` **does** use an SMA seed, so this is a per-function choice there,
not a house style.

## 3. The question reduces to one scalar

A seeded EMA reproduces the converged EMA exactly, plus one decaying term:

```
y_seed(n) - y*(n)  =  (1-alpha)^(n-t0) * (seed - y*(t0))
```

where `y*` is the EMA an implementation with unlimited history would print.
Neither seed is the truth; both estimate `y*(t0)`. The decay factor depends on
`alpha` alone — **identical for both rules** — so the whole question is *how big
is `seed - y*(t0)`, and at which bar is it charged*. Rule T seeds `p-1` bars
earlier, so by rule S's first output bar its error has already been damped by
`(1-alpha)^(p-1)`; every "common bar" figure below gives it that credit.

## 4. Analytic: seed quality

`Var(seed - y*)` in closed form for a unit-variance process (study part A).
`common = sd(e_S) / [(1-a)^(p-1) · sd(e_T)]` — the head-to-head at the same bar,
below 1 meaning rule S wins there.

| process, p=30 | sd(e_T) | sd(e_S) | e_S/e_T | common | trend bias e_T | e_S |
|---|---|---|---|---|---|---|
| white noise | 0.9509 | 0.0950 | 0.100 | 0.691 | 14.5 | 0.00 |
| AR(1) φ=0.50 | 0.9217 | 0.1511 | 0.164 | 1.134 | 14.5 | 0.00 |
| AR(1) φ=0.90 | 0.7564 | 0.2190 | 0.289 | 2.002 | 14.5 | 0.00 |
| AR(1) φ=0.99 | 0.3499 | 0.0963 | 0.275 | 1.904 | 14.5 | 0.00 |
| random walk | 2.6473 | 0.6863 | 0.259 | 1.793 | 14.5 | 0.00 |

Three results:

* **As a seed, the SMA is 3.5-10x better** under every process tested.
* **At a common bar the ordering reverses on persistent (i.e. realistic) series.**
  An EMA that has already run `p-1` bars has only ~14% of its weight left on the
  seed, and that beats a fresh SMA by ~1.8x. This is rule T's genuine advantage.
* **The trend-bias column is exact, not statistical, and it is why the SMA seed
  exists at all.** `alpha = 2/(p+1)` puts the EMA's centre of mass at
  `(1-alpha)/alpha = (p-1)/2` bars back — *precisely* the centre of mass of a
  p-bar SMA. On any locally linear stretch the SMA seed is unbiased to machine
  precision; the last-value seed is off by `(p-1)/2 × slope`. **The p-bar SMA is
  not an arbitrary convention: it is the seed `alpha = 2/(p+1)` was matched to.**

Confirmed empirically: on gData close at p=30, rule T's mean seed error is
**+0.748% of the series sd** against rule S's **-0.006%**. Prediction for rule T
is `(p-1)/2 × drift = 14.5 × 0.05528/bar = 0.763%`; measured 0.748%, so the bias
term is understood to under 2%.

## 5. Empirical: one EMA on real data

gData close, 10000 real daily bars, every data-start offset; error vs the
converged EMA as a percentage of the series sd. IBM (6741 bars, 1999-2026) agrees
on every line to within ~10%.

| p | e_T first out | e_S first out | e_T @common | ratio | bias e_T | bias e_S |
|---|---|---|---|---|---|---|
| 5 | 4.300% | 1.401% | 0.849% | 1.649 | 0.111% | 0.000% |
| 12 | 7.527% | 2.129% | 1.198% | 1.777 | 0.293% | -0.002% |
| 26 | 11.315% | 3.092% | 1.652% | 1.871 | 0.653% | -0.005% |
| **30** (EMA's default) | **12.149%** | **3.293%** | **1.756%** | 1.875 | 0.748% | -0.006% |

Rule S's first published value is **3.7x** better than rule T's; at a common bar
rule T is **1.9x** better. Both hold across periods and across both corpora.

## 6. Empirical: cascades — the shape DEMA/TEMA/TRIX/T3 actually ship

DEMA, TEMA, TRIX, MACD, EFI and T3 do not call `TA_EMA`; they inline the same
recursion with the **same per-stage CLASSIC seed** (an SMA of that stage's first
`p` inputs). All of them except `T3` also carry the METASTOCK branch that seeds
from `inReal[0]`; `T3` ships CLASSIC only. So the seed error compounds `n` times
and the lookback grows as `n*(p-1)`. gData close, same methodology:

| stages | p | lookback | e_T first out | e_S first out | e_T @common | ratio |
|---|---|---|---|---|---|---|
| 1 EMA | 30 | 29 | 12.212% | 3.310% | 1.765% | 1.875 |
| 2 DEMA | 30 | 58 | 18.882% | 2.187% | 1.335% | 1.639 |
| 3 TEMA/TRIX | 30 | 87 | 21.362% | 1.602% | 0.904% | 1.772 |
| 6 T3 | 30 | 174 | **29.652%** | **0.952%** | 0.288% | 3.308 |

(The one-stage row differs from §5's p=30 row in the first decimal: the two parts
sample slightly different offset ranges. Nothing else is shared between them.)

**Depth sharpens both effects, in opposite directions**, and the report is
incomplete without saying so:

* Rule S's first published value gets *better* with depth (3.3% → 0.95%): the
  lookback grows, and an SMA-of-SMA-of-… is an increasingly good stand-in for a
  6-fold-smoothed target. Rule T's gets *worse* (12.2% → 29.7%): at its first bar
  every stage still equals its own input, so what it publishes is the **raw
  series**, and the raw series is further from a 6-fold-smoothed target than from
  a 1-fold one. The gap at T3 is **31x**.
* At a common bar rule T's advantage also grows (1.88 → 3.31), because 174 bars
  of history buy six stages' worth of decay.

The SMI case study is the sharpest instance of the first bullet: rule T's first
published SMI is **exactly** the raw unsmoothed `100·num/(0.5·den)` — max
difference 0 at every offset in both corpora — printed at full ±100 amplitude
under the SMI label, rms error 52 SMI points against rule S's 14.

## 7. The honest counter-finding

At a common bar index, first-sample seeding plus `p-1` recursion steps beats a
fresh p-bar SMA by **1.6-3.3x** on real data. Per bar of history rule T therefore
reaches any given tolerance slightly sooner — for SMI(13,25,2), 67 bars against
76 to hold rms under 1 SMI point.

If accuracy at the lookback bar were the *only* criterion, the optimum is
**neither shipped arm**: seed with the first sample **and** keep TA-Lib's
lookback. For SMI(13,25,2) that is rms 8.06 at bar 37, against 14.0 for CLASSIC
and 52.0 for Tulip/TradingView. That combination is
`TA_COMPATIBILITY_METASTOCK` plus an unstable period — a library-wide change to
`TA_EMA` and the six functions that inline it, breaking 25 years of published values for every
function in §8. **Not proposed.** It is recorded here so it is not discovered
later as a surprise, and so that "TA-Lib's seed is the accurate one" is not
repeated as if it were established.

## 8. Scope — what a change here would touch

| how it uses the EMA seed | functions |
|---|---|
| inlines the recursion + CLASSIC seed directly | `EMA`, `DEMA`, `TEMA`, `TRIX`, `MACD`, `EFI` |
| 6 chained stages, each SMA-seeded | `T3` |
| routes through `ma()` → `ema()` | `MA` (MAType=EMA), `APO`, `PPO`, `PVO`, `MACDEXT` |
| routes through `macd()` | `MACDFIX` |
| inlines the recursion + CLASSIC seed directly, 3 stages plus a signal | `SMI` (#238) |

Every one of them reaches `TA_FUNC_UNST_EMA` through `ema_lookback()` — `DEMA`
and `TEMA`/`TRIX` multiply it by 2 and 3, `MACD` adds it twice (slow + signal) —
**except `T3`, which carries its own `TA_FUNC_UNST_T3`**. All of them inherit the
start-dependence noted in §1.

## 9. Verdict

Keep the SMA seed, on these grounds and in this order:

1. **The value each library hands you first.** Rule S wins by 3.7x at a single
   EMA and by up to 31x at T3 depth, and rule T's first value is not an
   approximation of the smoothed indicator — it is the unsmoothed series. This is
   the metric a lookback contract is *about*, and because `TA_EMA` re-seeds at
   `startIdx - lookback`, it governs every narrow-range call, not just the first
   bar of a long one.
2. **The SMA seed is the seed `alpha = 2/(p+1)` was designed for** — matched
   centre of mass, exactly zero trend bias (measured -0.006% of sd against rule
   T's +0.748%).
3. **Consistency.** Fourteen shipped functions share this seed; `TTR`, the
   nearest independent library, uses the same rule.
4. Against that stands §7, which is real and is not to be papered over.

Nothing here argues for changing an oracle policy: Tulip/TradingView remain valid
**tail** oracles for EMA-derived functions — bit-exact once the transient has
decayed below double precision, tolerance-only inside it.

## Reproduce

```bash
cd docs/studies/ema-seeding
python3 seed_study.py            # ~3 s, writes results.txt (checked in)

# optional: pin the CLASSIC rule against the shipped library
cc -I ../../../include -o /tmp/ema_probe ema_probe.c \
   ../../../src/tools/ta_regtest/ta_gDataClose.c <path-to>/libta-lib.a -lm
/tmp/ema_probe
```

Corpora are real daily OHLC only, no synthetic data: **gData** (10000 bars,
in-tree) and **IBM** (6741 bars, 1999-11-01..2026-08-20, checked in). numpy is
the only dependency; no network.

## Sources

- [R `TTR` moving averages](https://search.r-project.org/CRAN/refmans/TTR/html/MovingAverages.html) — "initialized with the n-period sample average at period n"
- [Pine `ta.ema` reference](https://pineify.app/pine-script-ta-ema) — `na(sum[1]) ? src : …`
- [TradeStation `XAverage`](https://help.tradestation.com/10_00/eng/tsdevhelp/elword/function/xaverage_function_.htm) — "the effects of the first price will never be completely removed"
- Tulip `beta/smi.c`, in `ta-lib-oracles/tulip_serve/vendor/tulipindicators`
- `ta_codegen/input/{ema,dema,tema,trix,macd,efi,t3}/*.c` — the shipped seeds
