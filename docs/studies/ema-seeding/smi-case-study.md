# TA_SMI: which EMA seed carries the smaller error?

Supporting measurement for [#238](https://github.com/TA-Lib/ta-lib/issues/238).

Issue #238 recommended arm A (`TA_EMA`, SMA-seeded) over arm B (Tulip/TradingView,
seeded with the first sample) on the grounds of *consistency with the rest of the
library*. It never measures which arm is more **accurate**, because it only ever
tabulates `|A - B|` — the gap between two arms, with no third party to say which
one is wrong. This study supplies the missing third party.

Two questions were asked:

1. **Does Blau recommend a seeding rule?**
2. **Independently of Blau: which seed has the lower error?**

Short answers: **(1) No — he specifies none, and his own platform used the
first-sample rule.** **(2) It depends on which value you measure, and the
honest split is below.** The numbers do reinforce keeping `TA_EMA`, but not via
the claim "the SMA seed is simply more accurate" — that claim is false at a
common bar, and the write-up says so.

---

## 0. The two rules, and what "error" means

| | seed | first published bar | who ships it |
|---|---|---|---|
| **rule S** | `SMA(x[t0-p+1 .. t0])` | `p-1` | TA-Lib `TA_EMA` (CLASSIC), R `TTR::EMA` |
| **rule T** | `x[t0]` — for the SMI denominator that first sample is `HH-LL`, the "H-L seed" | `0` | TradingView `ta.ema`, Tulip `ti_smi`, TradeStation `XAverage` |

Neither seed is the truth. The truth is the **converged EMA** — what an
implementation with unlimited history would print at that bar. Both rules are
estimators of it, and a seeded EMA's error is *exactly*

```
y_seed(n) - y*(n)  =  (1-alpha)^(n-t0) * (seed - y*(t0))
```

The decay factor `(1-alpha)^(n-t0)` depends on `alpha` alone — **identical for
both rules**. So the entire seeding question reduces to one scalar: how big is
`seed - y*(t0)`, and at which bar is it charged.

### Both arms are pinned to real implementations

Nothing here is a model of what TA-Lib or Tulip "probably" does:

* `ema_probe.c` links `cmake-build/libta-lib.a` and calls `TA_EMA(0,60,close,25)`.
  All 37 outputs are **bit-identical** to this study's rule S.
* The same probe confirms `TA_EMA`'s first output is the plain 25-bar SMA ending
  at `startIdx` (bit-identical), and that `TA_EMA` **re-seeds at
  `startIdx - lookback`**: `TA_EMA(400,400)` and `TA_EMA(0,500)[400]` differ by
  0.199 on gData close. That matters — it means the "first published value" row
  below is not a warm-up curiosity, it is what *any* narrow-range TA-Lib call
  returns.
* Rule T is Tulip `beta/smi.c` verbatim (`ema_r_num = ema_s_num = num[q-1]`,
  `ema_r_den = ema_s_den = den[q-1]`). Part D reproduces **every printed digit**
  of issue #238's own arm-vs-Tulip table, which was itself measured against a
  compiled `ti_smi` — including `-10.8035818118385 / -25.4984363633175` at bar 37
  and "bit-identical from bar 67" at (10,3,3).

Corpora, all real daily OHLC: **gData** (10000 bars, in-tree), **IBM** (6741 bars,
1999-11-01..2026-08-20), **gData[0:252]** (the issue's own corpus — it is labelled
"TA_SREF" there but cites the gData files, and only gData reproduces its numbers;
the separate `TA_SREF_*_daily_ref_0_PRIV` arrays are different data).

---

## 1. Blau's recommendation: there is none

| source | what it says about initialisation |
|---|---|
| Blau, "Stochastic Momentum", *TASC* v11:1 (Jan 1993); *Momentum, Direction and Divergence* (Wiley 1995) | States the EMA as the bare recursion `EMA(k) = EMA(k-1) + 2/(n+1) * (price(k) - EMA(k-1))`. **No initial condition, no warm-up guidance, no discard rule.** The formula defines the filter's steady state and is silent on the boot. |
| TradeStation `XAverage` — the platform Blau wrote in (his book is EasyLanguage-based) | `XAverage = XAverage[1] + SmoothingFactor * (Price - XAverage[1])`, seeded on the first computable bar from the price itself — **rule T**. *Secondary evidence:* the seeding line comes from EasyLanguage references rather than from TradeStation's own function page, which only goes as far as *"the effects of the first price will never be completely removed, but its weight will continuously shrink"* (true of an SMA seed too, so that sentence alone does not settle it). |
| R `TTR::EMA` — the only independent library implementation of `SMI()`, and the source of the 13/25/2/9 defaults #238 proposes | *"The EMA result is initialized with the n-period sample average at period n."* This is **rule S**, exactly TA-Lib's. TTR also warns that EMA-family indicators are *"unstable in the short-term"* because they use their own previous values. |
| TradingView `ta.ema` | `sum := na(sum[1]) ? src : alpha*src + (1-alpha)*nz(sum[1])` — **rule T**. (Pine's `ta.rma` seeds with an SMA; `ta.ema` does not. TradingView's built-in SMI is `200 * emaEma(relativeRange, lengthD) / emaEma(highestLowestRange, lengthD)` with defaults %K=10, %D=3, and `emaEma` is `ta.ema(ta.ema(src,len),len)`.) |
| Tulip `beta/smi.c` | **rule T**, as above. |

So provenance does **not** favour us: Blau said nothing, and the platform he
authored on used the first-sample rule. The only independent *library*
implementation of SMI uses ours. Precedent is split 3-1 against, on a question
its author never ruled on — which is precisely why it has to be decided on
measurement.

---

## 2. What the measurement says

### 2a. Seed quality, in closed form

`Var(seed - y*)` evaluated exactly for a unit-variance process (`seed_study.py`
part A). `common` = the head-to-head at the **same bar index**, giving rule T
full credit for having started `p-1` bars earlier: `sd(e_S) / [(1-a)^(p-1) sd(e_T)]`.

| process, p=25 | sd(e_T) | sd(e_S) | e_S/e_T | common | trend bias e_T | e_S |
|---|---|---|---|---|---|---|
| white noise | 0.9414 | 0.1040 | 0.110 | 0.754 | 12.0 | 0.00 |
| AR(1) φ=0.90 | 0.7236 | 0.2147 | 0.297 | 2.026 | 12.0 | 0.00 |
| random walk | 2.4000 | 0.6268 | 0.261 | 1.783 | 12.0 | 0.00 |

Three things fall out:

* **As a seed, the SMA is 3.8–9x better** under every process tested.
* **At a common bar the ordering reverses on realistic (persistent) series.**
  An EMA that has already run `p-1` bars has only 15% of its weight left on its
  seed, and that beats a fresh SMA. This is rule T's genuine advantage and it is
  not small — 1.8x at p=25.
* **The trend-bias column is exact, not statistical, and it is the reason the
  SMA seed exists.** `alpha = 2/(p+1)` puts the EMA's centre of mass at
  `(1-alpha)/alpha = (p-1)/2` bars back — *precisely* the centre of mass of a
  p-bar SMA. On any locally linear stretch the SMA seed is unbiased to machine
  precision; the last-value seed is off by `(p-1)/2 × slope`. The p-bar SMA is
  not an arbitrary convention: it is the seed `alpha = 2/(p+1)` was matched to.

Measured on gData close at p=25, rule T's mean seed error is **+0.627% of the
series sd** against rule S's **-0.005%** — zero to within noise. The prediction
for rule T is `(p-1)/2 × drift = 12 × 0.05528/bar = 0.632%` of sd; measured
0.627%, so the bias term is understood to under 1%.

### 2b. End-to-end SMI, in SMI points

gData, 2272 independent data-start offsets, SMI(13,25,2). IBM agrees within a few
percent on every line.

| | rule T (TV/Tulip) | rule S (TA-Lib) |
|---|---|---|
| **first published value** | bar 12: rms **52.0**, p95 **99.0**, max **145.9** | bar 37: rms **14.0**, p95 **26.5**, max **47.8** |
| worst error anywhere it publishes | rms 52.0, max 145.9 | rms 14.0, max 47.8 |
| both at bar 37 | rms **8.06** | rms **14.0** |
| amplitude vs truth at its first bar | **1.67x** too volatile | **1.06x** |
| bars of history for rms < 1.0 pt | 67 | 76 |
| max abs value published (bound is 100) | 100.00000000000011 | 97.21 |

At TradingView's own defaults, SMI(10,3,3): first published value rms **34.8**
(p95 69.9, max 122.5) for rule T against **7.48** (p95 15.0, max 28.2) for rule S;
at the common bar 13, 5.04 against 7.48; bars to 1 pt, 16 against 17.

The single most telling line is one that needs no statistics:

> **Rule T's first published value is not a double-smoothed SMI at all. It is the
> raw, unsmoothed `100*num/(0.5*den)` — max difference 0, exactly, at every
> offset in both corpora.** At bar `q-1` all four of its EMAs still equal their
> own inputs, so TradingView and Tulip print the bare stochastic momentum under
> the SMI label, at full ±100 amplitude, with no marker that it is not the
> indicator. Rule S's first published value is a genuinely smoothed object of
> roughly the right span — which is why its amplitude ratio is 1.06 rather than
> 1.67, and its error 3.7x smaller.

Both rules preserve the `|SMI| <= 100` bound (it follows from `|num| <= den/2`
plus positive EMA weights, and survives in floating point to 1.1e-13). That is a
free invariant worth asserting in the test suite.

### 2c. The issue's own table, attributed

Same 252-bar geometry as #238, on gData windows that have history in front of them
(2309 windows), over the bars **both** arms publish:

| SMI(13,25,2) | rms | max |
|---|---|---|
| `\|A-B\|` disagreement | 11.26 | 35.51 |
| rule S (TA-Lib) error | **14.06** | 47.78 |
| rule T (TV/Tulip) error | **8.07** | 43.95 |

So of the 14.69-point gap #238 reports at (13,25,2), **the median split is about
65% ours, 35% theirs** — measured over the bars where both publish, i.e. after
TA-Lib's lookback, where rule T has its head start. Anyone reading #238's table as
"Tulip is wrong by 14.69 points" has it backwards.

---

## 3. Verdict

**Keep `TA_EMA` (arm A) — but not on the grounds that it is uniformly more
accurate, because it is not.** Stating it precisely:

1. **On the value each library actually hands you first, rule S wins by 3.5-4.7x**
   (rms 14.0 vs 52.0 SMI points at 13/25/2; 7.5 vs 34.8 at 10/3/3), and rule T's
   first value is the *unsmoothed* indicator, not a warm-up approximation of the
   smoothed one. Because `TA_EMA` re-seeds at `startIdx - lookback`, this is the
   value every narrow-range TA-Lib call returns — not just the first one. This is
   the metric that matches what a lookback contract promises: *every published
   value is one the library stands behind.*
2. **The SMA seed is the seed `alpha = 2/(p+1)` was designed for** — matched
   centre of mass, exactly zero trend bias, confirmed at 0.005% of sd against
   rule T's 0.627%.
3. **Against that, rule T is genuinely better at a common bar, by 1.6-2.2x**,
   worth about 8-10 bars of extra history at r=25 (67 vs 76 bars to 1 SMI point).
   The cost of collecting that head start is publishing ~25 bars of near-raw
   output first. That is the trade, stated plainly.
4. Blau adjudicates nothing here; his platform is on rule T's side, `TTR` — the
   implementation whose defaults #238 adopts — is on ours.

If accuracy at the lookback bar were the *only* criterion, the optimum is neither
arm: seed with the first sample **and** keep TA-Lib's lookback (rms 8.06 at bar
37, better than both). That is `TA_COMPATIBILITY_METASTOCK` plus an unstable
period, it is a library-wide change to `TA_EMA` rather than an SMI decision, and
it would break 25 years of published values for every EMA-derived function. Not
proposed — but it is the honest reading of the numbers and should not be
discovered later as a surprise.

Nothing here disturbs #238's other conclusions: the front end is confirmed, the
transient is the ordinary `TA_FUNC_UNST_EMA` story, and Tulip remains a valid
**tail** oracle.

---

## Reproduce

```bash
cd docs/studies/ema-seeding
python3 seed_study.py            # ~2 s, writes results.txt

# optional: pin rule S against the shipped library
cc -I ../../../include -o /tmp/ema_probe ema_probe.c \
   ../../../src/tools/ta_regtest/ta_gDataClose.c <path-to>/libta-lib.a -lm
/tmp/ema_probe
```

`results.txt` is the full output, checked in. Requires numpy; no other
dependency, no network.

## Sources

- [TASC v11:1 (Jan 1993), Blau, "Stochastic Momentum"](https://store.traders.com/-v11-c01-stochas-pdf.html) (paywalled; not read directly)
- [MQL5: William Blau's Indicators and Trading Systems](https://www.mql5.com/en/articles/190) — Blau's EMA recursion, no initial condition
- [TradeStation `XAverage`](https://help.tradestation.com/10_00/eng/tsdevhelp/elword/function/xaverage_function_.htm) and [`XAverageOrig`](https://help.tradestation.com/10_00/eng/tsdevhelp/elword/function/xaverageorig_function_.htm)
- [R `TTR` moving averages](https://search.r-project.org/CRAN/refmans/TTR/html/MovingAverages.html) — "initialized with the n-period sample average at period n"
- [TradingView: Stochastic Momentum Index](https://www.tradingview.com/support/solutions/43000707882-stochastic-momentum-index-smi/) — built-in formula and defaults
- [Pine `ta.ema` reference](https://pineify.app/pine-script-ta-ema) — `na(sum[1]) ? src : ...`
- Tulip `beta/smi.c` (in `ta-lib-oracles/tulip_serve/vendor/tulipindicators`)
