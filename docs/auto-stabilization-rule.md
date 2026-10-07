# Sizing an Auto-Stabilization rule

For a function that owns an unstable id: how to find its rule, the bars an Auto level
discards, and how to show the rule is the right size. What a level promises is section 2
of [the design](auto-stabilization-design.md); the tools and the meaning of their columns
are in [the study](studies/auto-stabilization/README.md).

## 1. Read the recursion

List every value the function carries from one bar to the next: its seed, its update, and
what each output does with it. Read the body that runs, not the formula: which value is
fed back, and when. Each item decides a part of the rule.

- **A fixed pole `p`**, `state = p*state + f(input)`. The difference between two starts
  shrinks by exactly `p` per bar on any input, so an e-fold costs `1/-ln(p)` bars. Round
  that up to the nearest ratio of small integers, taken with a ceiling: `n/2` for a pole at
  `(n-1)/(n+1)`, `n - 1/2` for one at `1 - 1/n`, `13/9` for one at 1/2, `5/2` for one at
  0.67. A looser ratio costs bars at every level. This is the per-level part, `K` times
  it. A ratio this tight leaves no bar over the need at a long period, so the count holds
  the level in exact arithmetic and a pair can sit a rounding error over it: say so on
  the page.
- **The gain between that state and the outputs.** A level is measured against `S`, the
  largest difference the seed causes on any output at any bar. The gain is how much more
  a late state difference shows than an early one, in e-folds. It is added to `K`: the
  rule is `(K + c)` times the bars per e-fold, and `c` is a fixed part. Never scale the
  level to make room for it. What contributes:
  - a non-linear map: its largest slope between two states both starts can hold. A value
    only a limiter produces is not one of them, both starts hold it or neither does;
  - a faster pole `q` after the state: `p/(p-q)`;
  - a repeated pole, or stages stepped together: the difference is the pole to the age
    times a polynomial of the age, and the seeds can cancel across its terms, so the first
    outputs are small and the peak comes later. The gain is that polynomial's worst growth
    against `S`, over every seed the slots can hold. It grows with `K`: `c` is sized at
    the largest `K` to hold, and the lookback comment names that `K`;
  - an output that repeats another a bar later: one bar;
  - the smallest `S` can be against the first state difference. Seeds can cancel part of
    each other in the first outputs, and every output counts: FISHER's trigger shows the
    earlier start's whole state at the first bar, which is what bounds the cancelling.
- **A coefficient the data moves**, or a state that feeds back into its own gain through
  clamps or a rate limiter. There is no bound, or one several times the need. The rule is
  sized by measurement. Where the coefficient depends on the input alone (FRAMA), both
  starts share it, the difference shrinks by exactly that coefficient each bar, and its
  floor gives a bound: cap the measured rule with it.
- **A step**: a limiter that snaps to a value, a comparison on the state whose branches
  differ where they meet, a test for an exact zero of a computed value, which rounding
  decides. A branch on the input alone is not one, both starts take it together, nor is a
  `max` or `min` with the state, which is continuous: a slope of 1. Two starts
  on either side of a step differ by it however close they are, so no count is a bound.
  Work out two things: whether the stepped value is fed back, and then for which inputs
  each side stays on its side (FISHER: a channel position between 0.986 and 0.995); and
  how often it happens, which only the census of section 2 shows: the probe's three
  series do not. Rare and short-lived: size
  the rule for the path where both starts step together and state the step as a limit.
  When steps are what drives the state, the function is a state machine and gets no rule.
  Where the output reads its state only through a step (HT_TRENDLINE: a period cut to a
  whole number of bars), the two starts are equal or a step apart, the need is the same at
  every level, and a level can only buy a rarer miss: size each on the tail of the need.
- **A rate that reaches zero.** Where the coefficient depends on the state against the
  price, find the region where a difference stops shrinking (MCGD: the line 20% under the
  price) and what input holds the state there. That is a limit to state as a condition on
  the input, and no multiple of the period removes it.

## 2. Measure the need

Build the library and the probe as the study's README says, from
`docs/studies/auto-stabilization/`:

```bash
/tmp/auto_stabilization_probe 40000 <NAME> > /tmp/need.tsv
python3 analyze.py /tmp/need.tsv /tmp/agg.json f=<NAME>    # one line per output and parameter set
python3 rules_vs_need.py /tmp/need.tsv                     # the library's count against the need
OVR="TimePeriod=2" /tmp/auto_stabilization_probe 40000 <NAME> > /tmp/need_min.tsv
/tmp/auto_stabilization_census 20000 1 <NAME>              # how often random series pass the count
```

- Read `A10` and `A19`, the need at each level, `A7` and `A16`, what the regression leg
  holds, and `C4` and `C8`, the significant digits. `rules_vs_need.py` prints them as
  `1e-4`, `1e-8`, `leg(e^-7)`, `leg(e^-16)`, `sig4` and `sig8`. The worst output, series
  and start is the need.
- The defaults and tripled periods run by themselves. Add an `OVR=` run for the shortest
  period, a long one, and each end of every real parameter the rule reads: a rule is only
  tested by moving what it scales with. `OVR` takes `name=value` pairs separated by
  commas, a name matching any parameter that contains it; its rows keep the
  label `defaults`.
- `analyze.py` shows the need whatever the lookback holds: the probe runs with every id
  at 0. `rules_vs_need.py` sets the count the library gives against it, so after changing
  a rule run `scripts/build.py generate`, rebuild the library and relink the probe.
- The census runs thousands of random series from two starts, at the defaults, tripled
  periods and the shortest ones. `max_10` and `max_19` are the largest need it met,
  `over10` and `over19` the trials above the count, `over7` and `over16` those above what
  the regression leg holds, `never` those that had not converged when the window ended. A
  constant need with nothing over is a linear kernel. A few trials far over, or any in
  `never`, is a step, or a state that locks: reproduce one before sizing anything. Many
  over on one kind of series only is a rule sized for another kind: name it as a limit.
- Random series do not reach the worst seed of a repeated pole or of several stages: T3
  needs 52 at most on fifty million pairs against the worst seed's 64. For a proven rule
  neither tool gives the count; section 3's kernel check does.
- The census ignores a difference under `1e-13` of the output's range, so an output whose
  seed difference is small against its range shows a `PREC_8` need cut short. Check a long
  need against the probe's `A19`.
- A count with no spare bar shows bars over at `PREC_8`, more of them the longer the
  period, and none over the regression leg: the arithmetic's rounding against `e^-19` of a
  small seed difference.
- On one row, `(A19 - A10) / 9` is the measured bars per e-fold. A linear output comes out
  at the derived figure; well above it on every row means the derivation missed a slower
  state. A non-linear output scatters around it.

## 3. Size the rule

- **A fixed part plus a per-level part.** A pure multiple of the level over-sizes the
  higher one. The per-level part is the bars per e-fold times `K` for a proven rule, or a
  multiple of `X` fitted to `A19 - A10` for a measured one. The fixed part is what is left.
- **A proven rule needs its worst case, which the three series do not reach.** Where the
  decay has a closed form, check the rule against it in `rules_check.py`, as the one-pole
  and SWAK sections do. Where it has none, iterate the kernel there from two starts, as
  the FISHER section does: over the states the earlier start can reach and the inputs that
  hold both starts where the gain is largest, every output, against `S` over the whole
  run, at every `K` the script lists. Derive the bound first and require the search to
  reach it: a search that stays under it has missed the worst case, or the bound is slack.
  `c` is the smallest integer with no violation. Iterate in exact fractions where the
  tolerance at the largest `K` is under a double's rounding, and take `S` at whole ages:
  a continuous envelope is not a bound at short periods.
- **When the worst seed is nothing like a price**, a proven count can be well above any
  need a series shows. Report both figures; choosing the measured one changes the rule's
  tier and is the owner's decision.
- **A proven rule well above the largest need the census met, at both levels,** is a
  slack bound: usually one taken against the seed's size where the level is measured
  against `S`. Redo the gain before accepting the rule.
- **A measured need above a proven rule** means the derivation is wrong, not that the
  rule needs margin, unless a step of section 1 is what produced it.
- **A measured rule covers the worst need at both levels**, at the defaults and at tripled
  periods, with margin. Give it a period term only where the need grows with the period,
  which three periods do not settle: run `CENSUS_PERIOD` from the smallest period to the
  thousands, and where the rule uses a root, the periods just under a square.
  Name the series it is sized on when one is left out, as the adaptive averages leave out
  the range-bound one.
- **A function that inherits a count adjusts it for what it does with the value.** Run the
  census on it by name. Where a window or a difference after the smoothed value shrinks
  its own largest difference, it needs more than it inherits: that is a gain like any
  other, and its lookback adds it with `TA_UNSTABLE_AUTO` on the inherited id (CKSP: its
  stop window; ADOSC: what two EMAs seeded together can cancel, a count in the fast
  period that `rules_check.py` holds against every way the two combine). A ratio
  of the inherited value (CVI) is treated like the owners' ratio outputs, and gets none.
  A difference of two poles looks unbounded as they meet and is not: it tends to the
  repeated pole's `(A + B*k) * pole^k`. Take the worst case over the seed's ratio before
  calling a gain unbounded. Giving bars back is not an offset: the stages inside the body
  would have to split differently, and the offset is never negative.
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
- **A quotient of two periods steps down** as the smaller one grows. Where a rule needs
  the log of a ratio, sum `min(f, s/2^j)` instead: each term is non-decreasing in both.
- **Non-decreasing in every period.** The functions that run two periods of one average
  (MACD, APO, MAVP and the like) place the shorter leg on that; a rule that shrinks as a
  period grows makes them read before a buffer with no error.
- **0 where the function does no smoothing**, a period of 1.
- **The same integer in every backend.** No `log`, `exp` or trigonometric call; `ceil`, a
  division and `sqrt` are safe. A half-integer count is `(5 * (K + c) + 1) / 2`. Keep an
  `(int)` of a `double` the whole right-hand side of an assignment to an `int` local
  placed before the read, one local per level when `K` is inside the cast (`kama.c`,
  `swak_bp.c`).

## 5. Record it

- **The lookback comment** says where the bars per e-fold and the fixed part come from, in
  the recursion's own terms, and names a step.
- **`website/src/api/unstable-period/README.md`** is the table of record. Add a row to
  "Bars discarded under a level" with how the rule is sized: `proof`, `measurement`, and
  the qualifiers `ratio` and `limiter` that the section defines. Add the function to one
  of the two groups under "Auto-Stabilization", by whether its output reaches zero, and a
  line under "What a level does not promise" for a case no count covers.
- **`src/tools/ta_regtest/ta_test_func/test_auto_stabilization.c`** keeps its own copy of
  every rule and its own list of owners: add the id to both, and a vector for each real
  parameter the rule reads. A bare `bin/ta_regtest` runs it.
- **The issue the function was specified on** gets the `rules_vs_need.py` lines and, for
  a proven rule, the `rules_check.py` lines.
