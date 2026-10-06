# Auto-Stabilization: how fast two starts agree

Measurements behind [the Auto-Stabilization design](../../auto-stabilization-design.md): for every function,
how the value at a bar computed from a later start approaches the value computed from bar 0.

## What is here

| File | What it does |
|---|---|
| `auto_stabilization_probe.c` | Runs every function through the abstraction layer from bar 0 and from six later starts, on three synthetic series, and writes one row per (series, function, parameter set, output, start) |
| `analyze.py` | Folds those rows into one line per output, worst case over series and starts, and a class per function |
| `rules_vs_need.py` | Evaluates the design's rule for each unstable id, at both levels, against the measured need |
| `rules_check.py` | Checks the one-pole, SWAK and T3 rules against the kernels' decay laws at seven values of K and a grid of periods, with no library involved. The calibrated rules are not in it: section 3 of `results.txt` holds those |
| `tie_break.c` | Compares MAXINDEX and MININDEX from two starts on a series full of equal extremes. It counted mismatches at the commit `results.txt` names, and counts none since a tie names the most recent bar (#503) |
| `results.txt` | The output of all of the above at the commit named on its first line |

## Running it

From the repository root, after `scripts/build.py`:

```bash
cd docs/studies/auto-stabilization
gcc -O2 -I../../../include auto_stabilization_probe.c ../../../cmake-build/libta-lib.a -lm -o /tmp/auto_stabilization_probe
/tmp/auto_stabilization_probe 40000 > /tmp/probe.tsv          # every function; add a name to run one
OVR="FastLimit=0.2,SlowLimit=0.02" /tmp/auto_stabilization_probe 40000 MAMA   # one function, other parameters
python3 analyze.py /tmp/probe.tsv /tmp/agg.json summary
python3 analyze.py /tmp/probe.tsv /tmp/agg.json nonexact
python3 rules_vs_need.py /tmp/probe.tsv ../../..
python3 rules_check.py
```

Link the static library by path. `-lta-lib` picks up an installed TA-Lib instead of this tree's.

## Method

- **Series.** 40000 bars each: a geometric random walk (`rw`), a one-bar zigzag riding a slow
  drift (`zz`), and alternating 5000-bar up and down trends with little noise (`tr`). Prices are
  rounded to two decimals. The zigzag is the hard case for the adaptive averages: its efficiency
  ratio and its CMO sit near their minimum.
- **Starts.** Bar 0 against bars 1, 2, 5, 33, 250 and 1000. Every unstable period is 0, so the
  later run's first output is the function's own seed convention.
- **Parameter sets.** The defaults; every integer parameter whose name contains `Period`
  tripled; every MA type on every MA-type parameter.
- **Age.** A bar's age is the number of outputs the later run has already produced. An Auto
  count is a number of ages to discard.
- **Need.** For a tolerance `e^-K`, the first age from which the two runs stay within it for the
  rest of the series. `A<K>` measures against the largest difference between the two runs over
  all of the function's outputs (the seed discrepancy), `B<K>` against the range of the
  from-bar-0 output over the compared bars. `K` is 10 for the design's `PREC_4` level and 19
  for `PREC_8`; the regression leg holds `K - 3`, so 7 and 16.
- **Significant digits.** `C<d>` is the first age from which the two runs agree to `d`
  significant digits of the value: a difference of at most `10^-d` of the larger magnitude.
- **Bit-identity.** `Z` is the first age from which the two runs are bit-identical, -1 when
  they never are.
- **Classes.** `EXACT`: no difference at all. `ROUND`: differences below 1e-9 of the range.
  `CONV`: larger differences that fall under `e^-14`. `SLOW`: within `e^-7` but never within
  `e^-14`. `NEVER`: still above 1e-3 of the largest difference at the end of the series.

## What it does not show

- Real market data. The three shapes bracket the adaptive averages; they are not a market.
- Periods beyond three times the default.
- The Rust, Java and C# libraries.
- SWAK_BP away from its default delta. MAMA is measured at seven limit settings (section 6
  of `results.txt`).
- A measured need beyond `K` of about 21 is unreliable in the `rw` series: prices there grow by
  orders of magnitude, so late rounding differences are large against an early seed difference.

## Reading `results.txt`

Section 2 is the table to read: every output that is not bit-identical, at the defaults, with
periods tripled, and with each MA type. A `-1` in an `A` or `B` column means the tolerance was
never held to the end.

- `MAXINDEX` and `MINMAXINDEX` appear as `CONV` because of the tie-break in section 5, which
  #503 has since made independent of the start, not because they carry state.
- `CRSI` is `ROUND` at its defaults because its rank window is longer than its RSI legs need.
- `CORREL` at a period of 90 is classed `NEVER`: its rounding difference is 3.5e-8 of a narrow
  range, above the 1e-9 line `analyze.py` draws for `ROUND`. It carries no state.
- Section 2's other columns: `lb` is the lookback at 0, `S` the largest difference on that
  output, `S/R` the same against the output's range, `tail` the largest difference over the last
  tenth of the ages against `S`, and the last column the `A10` need on each series.
- Section 3 compares a rule with the need only for the function that owns the id, at the
  defaults and tripled periods. KAMA, FRAMA and VIDYA are compared on `rw` and `tr` only: their
  counts are sized for a market that trends or wanders, and `zz` takes several times longer. Its
  flags: `LEG4` and `LEG8` mean the count is short of what the leg holds (none is); `k4` and
  `k8` that it is short at the level's own `e^-K`, which happens for ratio outputs; `sig4` and
  `sig8` that the value does not agree to that many significant digits at the count, which
  happens for outputs that pass through zero.
- The MA-type rows of section 2 are not compared with a rule by any script here. The cases
  that do not meet the count the design gives: the KAMA and VIDYA types on `zz`; the same two
  types in PVO on every series, because the volume here is independent noise; the VIDYA and the
  MAMA type in STOCHRSI, whose FastK rests at 0 or 100; and BBW with the T3 type on `tr`, where
  the seed difference is 5e-6 and the leg's threshold applied to it is at the band's own
  rounding.
