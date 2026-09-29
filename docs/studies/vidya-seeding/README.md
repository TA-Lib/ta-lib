# VIDYA seeding evaluation

**Question.** `TA_VIDYA` ([#474](https://github.com/TA-Lib/ta-lib/issues/474)) steps
`V += k_t * (x - V)` with `k_t = alpha * |CMOU_t| / 100`, `alpha = 2/(n+1)`, and CMOU first defined
at bar m. The lookback is ruled to be m. Which seed, computed from bars `s..s+m-1` only, gives a
first published value (bar `s+m`) closest to what an implementation with unlimited history prints
at the same bar?

**Verdict.** Use **rule A**: seed with the first bar of the window, `V = x[s]`, and run VIDYA's own
step over `x[s+1..s+m-1]`, each step's CMO taken over the changes seen so far:

```
V = x[s]
for j = 1 .. m-1:
    Su, Sd = sums of max(d_i, 0), max(-d_i, 0) over i = s+1 .. s+j
    k = alpha * (fabs((100*(Su - Sd)) / (Su + Sd)) / 100.0)     (0 when Su + Sd == 0)
    V = ((x[s+j] - V) * k) + V
first output at s+m, with the full m-change CMOU, as now
```

The lookback stays m, and no bar is read that the provisional rule R does not read. In `vidya.c`
the up and down sums already accumulate over those changes, so the step moves into that loop.
At the first output A's rms error is **38.6% lower than R's at the defaults (12, 9) on IBM and
39.4% lower on the gData segments**; over the grid, on both corpora, it is 16.8% to 69.3% lower, in
10 of 10 offset blocks except one pair at 9 of 10. Against S (SMA seed) it is **5.4% lower at (12, 9) on IBM and
7.0% on the gData segments**; over the grid, 2.3% to 5.4% lower on IBM and 2.5% to 7.4% on the gData
segments. **A loses to S at two pairs**: (2, 2), by 7.7% (IBM) and 14.2% (gData segments), and
(9, 30), by 3.7% and 11.6%. No eligible rule beats A elsewhere by more than 0.3%.

## 1. Rules

Every rule's seed is the line's value at `s+m-1`; the first output is `s+m`.

| rule | seed at `s+m-1` | lookback |
|---|---|---|
| R | `x[s+m-1]` (#474's provisional rule; KAMA's and LEAN's) | m |
| S | SMA of `x[s..s+m-1]` (pandas-ta-classic seeds SMA(x, L), L = n = m) | m |
| Sn | SMA of the last `min(n, m)` bars | m |
| E | `x[s]`, then a plain alpha-EMA step (`k = alpha`) over `x[s+1..s+m-1]` | m |
| X0 | `x[s]`, held (`k = 0`) until the first CMOU | m |
| **A** | `x[s]`, then VIDYA's step with the CMO of the changes seen so far | m |
| W | R, published n bars later (information only, not eligible) | m+n |

X0 models trading-signals 8.3.0 (`x[0]` seed, first output at P, #474's `VIDYA.js:23-25,31`). The
library source was not read here. The model reproduces #474's measurements of that library against
R to the digits printed: 7.5e-2 at bar 9 and 4.8e-7 at bar 100 at (12, 9), and the maxima from bar
P and from bar 100 at P = 9, 12, 14 (results §0). From bar 200 on, #474's figures also carry the
library's own step spelling, which the model does not copy. S likewise reproduces #474's pandas-ta-classic row. A is the
warm-up shape ta4j's `VIDYAIndicator` uses (`x[0]`, CMO over partial windows from bar 1, #474),
with alpha at `2/(n+1)` instead of ta4j's 1. S and Sn coincide wherever n ≥ m.

## 2. Method

The corpora, reference and statistics are the MCGD study's
([`../mcgd-seeding`](../mcgd-seeding/README.md)), imported from it. Close prices of **IBM** (6741
bars) and **gData** (10000 bars); gData's price-level splices cut it into **segments**, each its own
series, and the whole series is kept as a stress case. The gain `k_t` is computed with `cmou.c`'s
running sums and `nullRun` reset.

The (n, m) grid: the defaults (12, 9); the MAType pairing `m = (3n+2)/4` for n = 2, 5, 10, 14, 20,
30, 50; n = m = 2, 9, 14, 30; the skewed (30, 9) and (9, 30).

The reference `y*` is the recursion from bar 0 (`x[0]` held until the first CMOU), used from bar
500 on, and only where two other runs agree with it to 1e-12 relative: `x[100]` held at bar 100,
and the SMA of bars 0 to m-1 at bar m-1. The rest are excluded and counted (results §H). The error
is `(y - y*)/y*`, over every start offset `s ≥ 500`, all rules over the same offsets.
VIDYA is scale-equivariant and translation-equivariant: `VIDYA(a*x + b) = a*VIDYA(x) + b`, since
the CMO reads differences and `|CMO|` ignores the sign of a. The error in price units therefore
ignores the level, but the relative error scales with `1/level`; it is used as in the MCGD study,
because the corpora are positive prices at varying levels.

**VIDYA's memory is long, so exclusions are large at long periods.** The mean of `|CMOU|/100` is
0.17 to 0.72 (results §C), so the seed's influence outlasts an EMA(n)'s, 1.4 to 6.0 times as long
(p50, IBM). Bars back until
`prod(1-k) < 1e-12`, p50 on IBM: 499 at (12, 9) against EMA(12)'s 166, and 4128 at (50, 38).
Offsets compared on IBM run from every offset at (2, 2), (5, 4), (10, 8) and (9, 9) down to 2688
of 6153 at (50, 38); on the gData segments to 622 of 6599 at (30, 30), and none at (50, 38),
which is therefore absent from that corpus's tables.

The harness's gain and step reproduce #474's 60-digit goldens for rule R at (12, 9) to at most
2.14e-16 relative (bar 9 109.6246782630551, bar 10 109.61069647274289, bar 100 120.18604396370417,
bar 1199 284.69011624413423 against 284.69011624413429; results §0).

## 3. Measured

rms relative error at the first output `s+m` (W at `s+m+n`), and A against R and S (results §E):

| corpus | (n, m) | R | S | A | W | A vs R | A vs S |
|---|---|---|---|---|---|---|---|
| IBM | (12, 9) | 3.409e-2 | 2.214e-2 | 2.093e-2 | 1.880e-2 | 38.6% lower, 10/10 | 5.4% lower, 10/10 |
| IBM | (2, 2) | 4.329e-3 | 1.997e-3 | 2.151e-3 | 1.518e-3 | 50.3% lower, 10/10 | 7.7% higher, 0/10 |
| IBM | (5, 4) | 1.530e-2 | 8.402e-3 | 8.213e-3 | 6.555e-3 | 46.3% lower, 10/10 | 2.3% lower, 10/10 |
| IBM | (14, 14) | 3.912e-2 | 2.362e-2 | 2.260e-2 | 2.372e-2 | 42.2% lower, 10/10 | 4.3% lower, 10/10 |
| IBM | (30, 30) | 6.758e-2 | 4.683e-2 | 4.445e-2 | 4.665e-2 | 34.2% lower, 10/10 | 5.1% lower, 9/10 |
| IBM | (50, 38) | 8.906e-2 | 6.663e-2 | 6.355e-2 | 6.060e-2 | 28.6% lower, 10/10 | 4.6% lower, 7/10 |
| IBM | (30, 9) | 5.510e-2 | 4.747e-2 | 4.585e-2 | 3.010e-2 | 16.8% lower, 10/10 | 3.4% lower, 10/10 |
| IBM | (9, 30) | 3.924e-2 | 1.273e-2 | 1.320e-2 | 2.570e-2 | 66.4% lower, 10/10 | 3.7% higher, 3/10 |
| gData segments | (12, 9) | 3.781e-2 | 2.464e-2 | 2.291e-2 | 1.981e-2 | 39.4% lower, 10/10 | 7.0% lower, 10/10 |
| gData segments | (2, 2) | 4.625e-3 | 1.950e-3 | 2.227e-3 | 1.432e-3 | 51.9% lower, 10/10 | 14.2% higher, 0/10 |
| gData segments | (20, 15) | 6.215e-2 | 4.401e-2 | 4.074e-2 | 3.628e-2 | 34.5% lower, 10/10 | 7.4% lower, 10/10 |
| gData segments | (30, 30) | 4.993e-2 | 3.490e-2 | 3.402e-2 | 3.571e-2 | 31.9% lower, 9/10 | 2.5% lower, 6/10 |
| gData segments | (9, 30) | 4.627e-2 | 1.272e-2 | 1.419e-2 | 2.962e-2 | 69.3% lower, 10/10 | 11.6% higher, 2/10 |

"10/10" counts the offset blocks, of 10, in which A's rms is lower.

- **Where A loses.** At m = 2, A is one step from `x[s]` whose CMO sees a single change, so
  `k = alpha` unless the change is 0: A equals E there, and a 2-bar mean is closer. Averaging the
  first h bars before A's steps (H(h), results §D) does not help elsewhere: at every pair but those
  two, H1 = A is the best hybrid. At (9, 30), H22 beats both (IBM 1.100e-2 against S 1.273e-2).
- **Close seconds.** E is 0.2% lower than A at IBM (30, 9) and 0.3% at gData segments (30, 23),
  and worse than A everywhere else except (2, 2), where they coincide. R and X0 are the two worst
  eligible rules at every pair but (30, 9), where S and Sn are worse than X0.
- **Distribution.** A's p95 is lower than S's except at (2, 2) and (9, 30) (results §I). Its max
  is not: it is higher than S's at most pairs, up to 37% (IBM (50, 38): 2.184e-1 against
  1.599e-1). A beats S on 50% to 60% of single offsets where its rms is lower.
- **Extra lookback.** W (R published n bars later) is lower than A at every pair with n > m, e.g.
  10.2% lower at IBM (12, 9), and at (2, 2); it is higher at n = m ≥ 9 and at (9, 30). R published
  m bars later, at `s+2m`, still does not reach A's first output at most pairs, e.g. IBM (12, 9):
  2.15e-2 against 2.093e-2.
- **Later bars** (results §G). A stays ahead of R at every bar of the 1000 followed, on both
  corpora, at every pair measured. Against S it stays ahead except at (2, 2) and (9, 30) from the
  first output; IBM (50, 38) from bar 133 after it (S rms 2.5e-2) by at most 1%; and elsewhere
  only once S's rms is below 1.7e-6. The gData segments' (30, 23), (30, 30) and (50, 38) have too
  few converged offsets to follow 1000 bars.
- **Splices** (gData whole): A beats S at every pair but (2, 2) (4.2% higher), from 2.9% lower at
  (5, 4) to 42.7% lower at (9, 30).

## 4. What A changes for the oracles

The oracles serve one period, so n = m. Each line below runs the oracle's seed on CMOU and TA-Lib's
step; the Wilder CMO keeps LEAN apart after its first bar whatever the seed (#474 M2). Bars after
the first output until the oracle stays within 1e-14 of A, and of R for comparison (results §F):

| IBM, n = m | trading-signals (`x[0]`) vs A / vs R | pandas-ta-classic (SMA) vs A / vs R | LEAN seed (`x[m-1]`) vs A / vs R |
|---|---|---|---|
| 2 | 36/49 · 37/50 | 35/46 · 36/49 | 36/49 · identical |
| 9 | 392/499 · 402/502 | 372/479 · 394/498 | 395/498 · identical |
| 12 | 600/761 · 620/778 | 577/721 · 607/764 | 611/764 · identical |
| 14 | 753/908 · 780/926 | 728/871 · 763/908 | 769/911 · identical |
| 30 | 2245/2513 · 2342/2570 | 2237/2428 · 2293/2526 | 2318/2535 · identical |

Cells are p50/max over offsets. On #474's 1200-bar synthetic series, from bar 0:

| series | trading-signals vs A / vs R | pandas vs A / vs R | LEAN seed vs A / vs R | A vs R at the first output / bar 100 |
|---|---|---|---|---|
| (9, 9) | bar 175 / 176 | 159 / 168 | 167 / identical | 1.9e-2 / 2.4e-9 |
| (12, 12) | 233 / 234 | 201 / 226 | 226 / identical | 1.7e-2 / 1.3e-7 |
| (14, 14) | 278 / 282 | 230 / 271 | 271 / identical | 2.1e-2 / 1.1e-6 |
| (30, 30) | 1149 / 1120 | 1065 / 1156 | 1158 / identical | 1.0e-1 / 1.2e-2 |
| (12, 9) | 224 / 227 | 206 / 220 | 221 / identical | 2.8e-2 / 1.8e-7 |

- trading-signals and pandas-ta-classic meet A sooner than R at every n = m but the
  synthetic (30, 30), where trading-signals meets it 29 bars later. #474's plan to compare them
  from bar 800 holds at P ≤ 14.
- LEAN's first value no longer matches: #474's anchor-only LEAN check (`outBegIdx` and
  `outReal[0]`) keeps `outBegIdx` = m, and the value anchor goes, since A moves the first output by
  1.7e-2 to 1.0e-1 relative on the synthetic series.
- #474's M9 goldens for bars 9 to 100 carry the seed and must be recomputed; bar 100 moves by
  1.8e-7 at (12, 9).

## Reproduce

```bash
cd docs/studies/vidya-seeding
python3 seed_study.py          # ~100 s, writes results.txt; numpy only, no network
```

The corpora come from `../ema-seeding` and the harness pieces from `../mcgd-seeding`, both imported.
