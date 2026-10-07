# Sizing an Auto-Stabilization rule

For a function that owns an unstable id: how to find its rule, the bars an Auto level
discards, and how to show the rule is the right size. What a level promises is section 2
of [the design](auto-stabilization-design.md); the tools and the meaning of their columns
are in [the study](studies/auto-stabilization/README.md).

## 1. Read the recursion

List every value the function carries from one bar to the next: its seed, its update, and
what the output does with it. Each decides a part of the rule.

- **A fixed pole `p`**, `state = p*state + f(input)`. The difference between two starts
  shrinks by exactly `p` per bar on any input, so an e-fold costs `1/-ln(p)` bars. Round
  that up to a ratio of small integers the period scales: `(n+1)/2` for an EMA, `n` for
  Wilder, `5/2` for a pole at 0.67. This is the per-level part, `K` times it.
- **The gain between that state and the output.** A level is measured against `S`, the
  largest difference the seed causes on any output, so whatever amplifies a late
  difference more than an early one costs bars: a non-linear map (its largest slope), a
  second pole in cascade, a repeated or complex pole, seeds whose effects partly cancel in
  the first outputs. Express the gain in e-folds and add it to `K`: the rule is
  `(K + c)` times the bars per e-fold. `c` is a fixed part; never scale the level to make
  room for it.
- **A coefficient the data moves**, or a state that feeds back into its own gain through
  clamps or a rate limiter. There is no bound, or one several times the need. The rule is
  sized by measurement.
- **A step**: a limiter that snaps to a value, a comparison that picks a branch. Two starts
  on either side of it differ by the step however close they are, so no count is a bound.
  When the step only interrupts a decaying state, size the rule for the path without it and
  state the step as a limit. When steps are what drives the state, the function is a state
  machine and gets no rule.

## 2. Measure the need

Build the probe as the study's README says, then run the one function:

```bash
probe 40000 <NAME> > need.tsv
python3 analyze.py need.tsv agg.json f=<NAME>     # one line per output and parameter set
python3 rules_vs_need.py need.tsv                 # the library's count against the need
```

- Read `A10` and `A19`, the need at each level, `A7` and `A16`, what the regression leg
  holds, and `C4` and `C8`, the significant digits. The worst output, series and start is
  the need; a second output that lags the first by a bar needs a bar more.
- The defaults and tripled periods run by themselves. Add an `OVR=` run for the shortest
  period and for each end of every real parameter the rule reads: a rule is only tested by
  moving what it scales with.
- `(A19 - A10) / 9` is the measured bars per e-fold. For a fixed pole it must come out at
  or under the derived figure; when it does not, the derivation missed a slower state.
- `analyze.py` shows the need whatever the lookback holds: the probe runs with every id
  at 0. `rules_vs_need.py` sets the count the library gives against it.

## 3. Size the rule

- **A fixed part plus a per-level part.** A pure multiple of the level over-sizes the
  higher one. The per-level part is the bars per e-fold times `K` for a proven rule, or a
  multiple of `X` fitted to `A19 - A10` for a measured one. The fixed part is what is left.
- **A proven rule needs its worst case, which the three series do not reach.** Add the
  kernel to `rules_check.py`: iterate the recursion from two starts, with no library, over
  the states the earlier start can hold and the inputs that maximize the gain, and print
  the smallest `c` with no violation at every `K` it lists. A measured need above the rule
  anywhere means the derivation is wrong, not that the rule needs margin.
- **A measured rule covers the worst need at both levels**, at the defaults and at tripled
  periods, with margin. Give it a period term only where the need grows with the period.
  Name the series it is sized on when one is left out, as the adaptive averages leave out
  the range-bound one.
- **`rules_vs_need.py` flags.** `LEG4` or `LEG8` fails the regression leg: the rule is too
  short. `k4` and `k8` are acceptable only for an output that divides by a state, `sig4`
  and `sig8` only for an output that reaches zero.

## 4. Write it

The function reads its id once, in its own lookback and nowhere else, and the read's second
argument is the rule:

```c
int ema_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1
        + TA_UNSTABLE( TA_FUNC_UNST_EMA, ta_auto_stabilization_ema(K, optInTimePeriod) );
}
```

- **`K` and `X`** exist only inside the read. `K` is the level's number of e-folds (10 at
  `PREC_4`, 19 at `PREC_8`), `X` its digit count (4, 8). A proven rule is written in `K`,
  a measured one in `X`.
- **Helper.** A kernel several ids share is a helper in
  `ta_codegen/input/helpers/auto_stabilization.c` that takes `K` or `X` as an argument.
  Use the existing one when the kernel is the same; a rule only one function has is
  written in the read, unless it needs a saturating form.
- **One read per path.** Once in the lookback, or once in each of its `return`s. The body
  takes the count from its own lookback (`lookbackTotal` minus the structural part). A
  function that runs another's recursion calls that function's lookback and never reads
  its id.
- **Non-decreasing in every period.** MAVP, APO, PPO, PVO, MACD, STC, ADOSC and MACDFIX
  place a shorter-period leg on that; a rule that shrinks as a period grows makes them
  read before a buffer with no error.
- **0 where the function does no smoothing**, a period of 1.
- **The same integer in every backend.** No `log`, `exp` or trigonometric call; `ceil`, a
  division and `sqrt` are safe. Keep an `(int)` of a `double` the whole right-hand side of
  an assignment to an `int` local placed before the read, one local per level when `K` is
  inside the cast (`kama.c`, `swak_bp.c`).

## 5. Record it

- **The lookback comment** says where the bars per e-fold and the fixed part come from, in
  the recursion's own terms.
- **`website/src/api/unstable-period/README.md`**: a row in the rules table with how the
  rule is sized, the function in the digits group its output belongs to, and a line under
  the limits for a case no count covers.
- **`test_auto_stabilization.c`** keeps its own copy of every rule and its own list of
  owners: add the id to both, and a vector for each real parameter the rule reads.
- **The spec issue** gets the `rules_vs_need.py` lines and, for a proven rule, the
  `rules_check.py` line.
