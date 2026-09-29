# MCGD seeding evaluation

**Question.** `TA_MCGD` ([#471](https://github.com/TA-Lib/ta-lib/issues/471)) needs a seed for
its recursion `MD += (x - MD) / (N * (x/MD)^4)`. Which seed rule gives a first published value
closest to what an implementation with unlimited history prints at the same bar?

**Verdict.** Use **rule T'**: seed with the first bar of the window, `MD = x[s]`, run the recursion
over the remaining `N-1` bars, and publish from `s+N-1`. The lookback stays `N-1`, the same as the
provisional SMA seed (rule S). At the first published bar T''s rms error is lower than S's at
every N from 2 to 50, on both corpora, and in 10 of 10 offset blocks each time. It is **15% to 18%
lower for the typical N = 10 to 30**, and 7% to 20% lower at every other N but IBM N=2 (31%, where
one offset carries 45% of S's error; 14% without it). At the common bar `s+2N-2` it is 7% to 18%
lower. No period up to 50 favours S, so a rule that switches seed by period has nothing to gain. At an equal bar T' beats every seed built as a plain average of prices (§4, §5).
On IBM at N ≥ 10, S catches up 2.4N to 4.8N bars after the first output. After that the lead
changes hands, and T''s rms is at most 25% higher than S's, on errors of 2.4e-3 and below (§5).

## 1. Rules

| rule | seed | first output | lookback |
|---|---|---|---|
| S | SMA of `x[s..s+N-1]` | `s+N-1` | `N-1` |
| T | `x[s]` | `s` | 0 |
| **T'** | `x[s]`, recursion over `x[s+1..s+N-1]` | `s+N-1` | `N-1` |
| L | `x[s+N-1]` | `s+N-1` | `N-1` |
| E | RMA-weighted mean of `x[s..s+N-1]`, weights `(1-1/N)^k` | `s+N-1` | `N-1` |
| W | SMA of `x[s..s+2N-2]` | `s+2N-2` | `2N-2` |

T and T' follow the same trajectory; they differ only in which bars are published. W is the SMA
whose centre of mass, `N-1` bars back, matches the recursion's at `x/MD = 1`. S's is half that.

## 2. Method

The corpora are close prices from two daily series: **IBM** (6741 bars) and **gData** (10000 bars).
gData holds six one-bar rises of 26% to 299%, while no other bar rises more than 18% or falls more
than 12%. These are price-level splices. After one, the gain `1/(N*r^4)` drops by up to 253x, and
the line takes hundreds of bars to catch up whatever the seed. The primary gData corpus is therefore
the spliced segments, each treated as its own series. The 504- and 97-bar segments are too short to
contribute. The whole series is kept as a stress case.

The periods are N = 2, 3, 4, 5, 10, 14, 20, 30, 50 and 200. The reference `y*` is the recursion
run from bar 0, seeded with `x[0]`. It is used from bar 500 on, and only where two other runs agree
with it to 1e-12 relative: one seeded with `x[100]` at bar 100, and one seeded with the SMA of bars
0 to N-1. The rest are excluded and counted, so N=200
has no usable reference on any corpus except the stress case. The start offsets begin at 500 rather
than #471's 450, so that rule T's first output at `s` also has 500 bars of history behind it. The
error is `(y - y*) / y*`: MCGD is scale-equivariant, so a relative error is comparable across price
levels. For each (corpus, N), all rules are compared over the same offsets.

The harness's rule S matches #471's 60-digit goldens to 2.7e-16, and the primary's worked example
exactly. The committed `results.txt` was produced with `--oracle`. That option compared rule S with
an independent SMA-seeded implementation computing in decimal, which agreed to at most 1.25e-14
relative on every bar of both corpora at N = 5, 10, 14, 20, 50 and 200; the other periods print
"not dumped". Those dumps are not in the tree, and a run without `--oracle` prints "not compared"
in their place.

## 3. Linearised analysis

Near `x/MD = 1` the step is RMA's, with gain `1/N`. Each rule's seed, and `y*` itself, is then a
unit-sum linear filter of the input. The table gives the error sd per unit innovation sd for a
random walk (rw) and for white noise (wn), and the lag each rule leaves on a trend (`results.txt`
§A):

| N | S rw | T' rw | W rw | S lag | T' lag | S wn | T' wn |
|---|---|---|---|---|---|---|---|
| 5 | 0.597 | 0.546 | 0.380 | 2.00 | 1.64 | 0.205 | 0.345 |
| 14 | 1.107 | 0.955 | 0.651 | 6.50 | 4.96 | 0.127 | 0.361 |
| 50 | 2.176 | 1.830 | 1.245 | 24.50 | 18.21 | 0.068 | 0.366 |

On a random walk, the model that fits prices, T' beats S by 9% to 17% for N ≥ 5. At N=2 the two
kernels are identical, so whatever T' gains there (§5) is purely nonlinear. S under-lags by `(N-1)/2`
bars. T' uses the recursion's own kernel for `N-1` bars and puts the remaining weight,
`(1-1/N)^(N-1)` (0.41 at N=5, 0.37 for large N), on one bar. On white noise S wins, but prices are
not white noise. In this linear model W beats both, by 30% to 43%.

## 4. The nonlinear term

The 4th power makes a bar below the line pull harder than a bar above it pushes. So the converged
line sits below any plain average of prices, by about `4*E[u^2]` for N ≥ 10, where
`u = price/line - 1` (results §C, IBM):

| N | SMA_N above y* | SMA_2N-1 above y* | 4*E[u^2] |
|---|---|---|---|
| 5 | +0.30% | +0.28% | +0.19% |
| 14 | +0.75% | +0.68% | +0.63% |
| 50 | +2.33% | +2.07% | +2.26% |

This bias grows with N and with volatility. A seed that is a plain average of prices (S, E, W)
cannot carry it. T' carries part of it, because its last `N-1` bars go through the recursion
itself, so its mean error is about 0.6 of S's (IBM N=14: +0.44% against +0.76%). On IBM this cancels
W's linear advantage. On gData it narrows it (§5).

## 5. Measured

rms relative error at each rule's first output, and at the common bar `s+2N-2` (results §E):

| corpus | N | S | **T'** | W (N-1 bars later) | S @common | T' @common | T' vs S at first output |
|---|---|---|---|---|---|---|---|
| IBM | 2 | 8.975e-3 | 6.193e-3 | 9.877e-3 | 4.456e-3 | 3.736e-3 | 31.0% lower, 10/10 blocks |
| IBM | 14 | 2.004e-2 | 1.658e-2 | 1.691e-2 | 8.701e-3 | 7.459e-3 | 17.2% lower, 10/10 |
| IBM | 30 | 2.787e-2 | 2.285e-2 | 2.315e-2 | 1.189e-2 | 9.993e-3 | 18.0% lower, 10/10 |
| IBM | 50 | 4.108e-2 | 3.288e-2 | 3.255e-2 | 1.927e-2 | 1.584e-2 | 19.9% lower, 10/10 |
| gData segments | 2 | 5.860e-3 | 5.460e-3 | 5.218e-3 | 3.152e-3 | 2.943e-3 | 6.8% lower, 10/10 |
| gData segments | 14 | 2.547e-2 | 2.133e-2 | 1.770e-2 | 1.239e-2 | 1.026e-2 | 16.3% lower, 10/10 |
| gData segments | 30 | 5.308e-2 | 4.478e-2 | 3.891e-2 | 3.168e-2 | 2.730e-2 | 15.6% lower, 10/10 |
| gData segments | 50 | 1.026e-1 | 8.762e-2 | 7.642e-2 | 6.700e-2 | 5.673e-2 | 14.6% lower, 10/10 |

The gain over S is smallest at N = 2 to 5 on the gData segments (7% to 12%), and 15% to 18% for
N = 10 to 30 on both corpora. T''s p95 is lower at every N ≤ 50. Its max is not: at IBM N=20, 30
and 50 and gData segments N=3, 4 and 5, T''s single worst offset is worse than S's, by 1% to 3%
except at IBM N=30 (1.18e-1 against 1.04e-1, 13%). T' wins on 54% to 65% of individual offsets. The rms lead
comes mostly from the offsets where S is far off.

- **T and L are the worst** at every N ≤ 50, with rms 1.2x to 2.0x S's. A single bar is not an
  estimate of an N-bar smoother. E, the RMA kernel truncated to N bars, is up to 13% worse than S
  (3% better only at IBM N=2, which one offset dominates), because it shortens the lag the wrong
  way.
- **W is not worth its lookback.** At its own first output W ranges from 1% better to 16% worse than T'
  on IBM (59% at N=2, 24% without its worst offset), and is 4% to 21% better on the gData segments, but it has used `N-1` more bars. Give T'
  those same bars, by comparing at `s+2N-2`, and T' is ahead by 1.35x to 2.64x (IBM N=14: 7.46e-3
  against 1.69e-2; 2.64x is IBM N=2).
- **Hybrids do not help.** An SMA of the first `m` bars followed by the recursion (results §D) gets
  steadily worse as `m` grows from 1 (T') to N (S), at every N on both corpora. Its offsets differ
  slightly from §E's, because it does not need a converged reference at `s`, so H1 and HN are close
  to, but not equal to, T' and S there.
- **Later bars** (results §G). On IBM at N ≥ 10, T' falls behind S between 2.4N and 4.8N bars after
  the first output (N=14: from bar 39, where S's rms is 1.5e-3). After that the lead changes hands,
  and T''s rms is at most 25% higher than S's (N=50, where S's rms is 2e-8). At N ≤ 5 on IBM, and
  at N ≤ 30 on the gData segments, T' falls behind only once S's rms is below 4e-7. At N=50 on
  the gData segments it falls behind from an rms of 8e-5, by up to 6%.
- **Convergence.** At N=14 (IBM) the error stays below 1e-3 after 28 bars (p50) for T' and 30 for S.
- **Splices** (gData whole): with the jumps included, the rules' rms error is 18% (N=2) to 296%
  (N=200). The seed hardly matters there: T' still beats S, by 0.5% to 6%.

## 6. What T' changes for oracles

An SMA-seeded implementation stays within 1e-14 of T' only once the seed difference has decayed.
On IBM (results §F), the number of bars after the first output, p50 / max over offsets:

| N | 5 | 10 | 14 | 20 | 30 | 50 |
|---|---|---|---|---|---|---|
| bars | 118 / 148 | 256 / 319 | 372 / 448 | 544 / 689 | 858 / 1087 | 1530 / 1812 |

On #471's 1200-bar synthetic series, the two agree to 1e-14 from bar 37 (N=2), 303 (N=10) and
454 (N=14). At N=50 and N=200 they never agree on those 1200 bars. An implementation seeded with the
first bar, and running the same recursion, is bit-identical to T' from the first output.

## Reproduce

```bash
cd docs/studies/mcgd-seeding
python3 seed_study.py          # ~40 s, writes results.txt; numpy only, no network
```

The corpora are loaded from `../ema-seeding`. The harness (`study`, `report`) takes the recursion's
vectorised step and the list of rules, so a later recursive function supplies its own step and rules
and reuses it.
