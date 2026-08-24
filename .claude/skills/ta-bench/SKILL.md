---
name: ta-bench
description: Benchmarking TA-Lib — ta_bench, ta_bench_direct, ta_bench_stream and scripts/stream_ab.py, what each ratio actually compares (the six-binary build matrix), streaming vs batch, and the --shape= input corpus. Use when running a benchmark in this repo, interpreting a speedup or ratio, or defending a performance number.
---

# Benchmarking TA-Lib

```bash
# Full pipeline (builds everything, regens, tests, benchmarks)
scripts/regtest.py

# Benchmark specific indicators (trustworthy — isolated, high iterations)
cd bin && ./ta_bench --language=cref,c --function=RSI,SMA --points=100000 --iters=500

# Full benchmark (noisy — use for overview, verify outliers in isolation)
cd bin && ./ta_bench --language=cref,c --points=100000 --iters=200
```

**Gotcha:** `ta_ref_serve` is statically linked — rebuild when `libta-lib.a`
changes or benchmarks are invalid. `regtest.py` handles this automatically.

Both hand-written benches report the **spread** of their own repeated passes,
because a bare median is silent about whether the box was quiet enough for it
to mean anything — at `--iters=50` the same five functions read 0.57–0.81x, at
`--iters=200` they read 1.00x. Read the spread before the ratio. `--max-spread=N`
(percent, default 25) exits non-zero when the run is too noisy to interpret, and
`ta_bench_direct --jsonl=PATH` appends a run record for tracking over time.

`ta_bench_direct`'s ratio is `ta_bench_cg` (single TU, `-flto`) over
`libta-lib.a` (separate TUs, no LTO) — **a build-configuration difference, not
an algorithm one**, which is why binary layout alone can move it further than
the old ±10% colour band. It now colours only outside `--no-signal` (default
1.20x) and only when the row's own spread is narrower than the effect claimed.
`--reps=N` samples both arms instead of just the reference.

`ta_bench` sends `no_output:1`, so servers return timings without serialising
the output arrays — it only ever reads `timing_ns`. Without it a 100k-point run
spends ~97% of its wall clock formatting and parsing JSON nobody looks at.
Anything that needs the values (`--codegen`, `--xlang-hash`, `server_verify`)
simply omits the flag. `cref` is a frozen binary and predates it, so runs
including `cref` stay slower than C-only ones.

## The same source, six binaries

Every benchmark ratio in this tree compares two *builds*, and they are not the
same build. Measured `.text` on x86-64 gcc:

| binary | build | TU model | bytes |
|---|---|---|---|
| `libta-lib.a` | CMake Release | separate TUs, no LTO | 2,890,487 |
| `libta-lib.so` | CMake Release | separate TUs, PIC | 2,870,694 |
| `ta_codegen_serve_c` | `gcc -O3 -flto` | single TU + ta_abstract | 3,940,182 |
| `ta_bench_stream` | `gcc -O3 -flto` | single TU + streaming | 1,955,238 |
| `ta_bench_cg` | `gcc -O3 -flto` | single TU, indicators only | 1,021,939 |
| autotools `libta-lib` | libtool | separate TUs, no LTO | not built here |


3.9x between the extremes, from identical source. The two build flags that
all three build systems must keep in step are stated in the root `CLAUDE.md`.

Which tool measures which:

- `ta_bench_direct` — C-ref column is `libta-lib.a`, C column is `ta_bench_cg`.
  Its ratio is therefore rows 1 vs 5 above.
- `ta_bench --language=c` — `ta_codegen_serve_c` (row 3), *not* `ta_bench_cg`.
- `ta_bench --language=cref` — `ta_ref_serve`, the frozen pre-cutover source.
  Different code, not just a different build; the only cross-*version* number.
- `ta_bench_stream` — itself, both arms, which is why its speedup column is the
  one ratio here that isn't cross-configuration.

Consequences worth internalising before quoting any number: a function's ns from
`ta_bench_direct` and from `ta_bench --language=c` are not comparable; a ratio
near 1.0 in `ta_bench_direct` means single-TU + LTO bought nothing for that
function, not that the two are the same code path; and layout alone moves these
ratios further than the old ±10% colour band allowed, which is why the band is
now 1.20x and spread-gated.

### A/B of one change has a trap the table above does not show

The three `-flto` rows are **single-TU** builds — `ta_bench_stream.c` alone
`#include`s 181 `.c` files — so `-flto` there is nearly a no-op and the
compiler re-decides inlining for every function on every build. In an A/B those
decisions can differ **between the arms**. On #252 `TA_ATR_Update` was outlined
in one arm and inlined in the other, and `ta_bench_stream` reported +16% for a
function that is unchanged in the shipped build. Two defences:

- **Carry a control function** whose generated code is byte-identical across
  the arms, and believe nothing smaller than its excursion. RSI once read −55%
  on identical emitted C; 107 unchanged functions put that run's real floor at
  ±15%.
- **When the mechanism is memory traffic or aliasing, measure the shipped
  build.** A throwaway harness linking `libta-lib.a`, one function per process,
  min of N, read ±3% on the same change. That is an ad-hoc check, not a seventh
  instrument — do not add it to `bin/`.

**Is it worth optimizing at all?** The non-inlined call floor — a separate-TU
function that does nothing but write `*out` — is **1.06 ns**. That is ~30% of
SMA's 3.5 ns `Update`, ~15% of MFI's 6.9 ns, and ~1.5% of HT_TRENDLINE's 69 ns.
A saving well under that floor on a cheap function is one no caller can
observe.

## Streaming vs batch

`ta_bench_stream` answers the question streaming has to justify itself on: is
`TA_<NAME>_Update` actually cheaper than recomputing the last bar with the batch
call? Its `speedup` column is `batch_last_ns / update_ns` — above 1 means
streaming wins. Both halves are measured in one TU, one input, one layout, so
unlike `ta_bench_direct`'s ratio it is not comparing two build configurations.

```bash
cd bin && ./ta_bench_stream --points=20000 --iters=50
./ta_bench_stream --points=20000 --iters=50 --min-ratio=0.35   # exits 1 if any func is below
```

`ta_bench_stream` is **C only**. For the Rust and Java streaming tiers,
`scripts/stream_ab.py` A/Bs `update` (or `peek`) per bar — or `open`, which times
the whole warm-up instead of one bar — between the working tree
and a git revision — same generated harness compiled against two copies of the
library, interleaved rounds with alternating arm order, every streaming function
so the untouched ones are the control. It reads only the generated Rust crate and
Java fragments (no ta_abstract, no servers, no C build) and derives every call
from the emitted signatures, so adding an indicator needs no edit there.

**C# streams too, but has no lane in either tool** — there is no C# row in any
benchmark table, and `ta_bench --mode=open` answers `unsupported_mode` for it.
So the C# peek-scratch election is Java's predicate shipped unchanged, and the
emitted docs deliberately state what `Peek` allocates rather than claiming it is
cheaper, which is a comparison nothing here has measured. Adding a C# lane needs
a control arm first: reproduce a *known* effect (switch the copy constructor to
`Clone()` and require ~2x on array-owning functions while the array-less ones
stay flat) before trusting the tool to settle an unknown one.

```bash
scripts/stream_ab.py --base=origin/dev                                  # both languages
scripts/stream_ab.py --base=HEAD~1 --lang=rust --call=peek --mark=MIN,MAX
scripts/stream_ab.py --base=origin/dev --call=open --mark=BBANDS,STDDEV   # the Open tier
```

Current shape: median ~1.6x, but **~25 stream slower than
batch** and another ~50 sit under 1.5x. Recursive/multi-stage state wins big
(`HT_TRENDLINE` ~24x, `TRIX`/`TEMA` ~16x); window-recomputers and stateless
patterns lose (`AVGDEV`, `MAVP`, `MIDPRICE`, `WILLR`, CDL*) because the handle
buys nothing and costs indirection. Those losers overlap the rolling-extremum
family — see the corpus note below.

`--min-ratio` is a cliff detector, not a quality bar: run to run the worst ratio
moves 0.42–0.50 and the worst function's *name* changes, so a threshold near 1.0
just flaps. 0.35 has headroom while still failing on a real regression.

## Benchmark input corpus

Some indicators have input-dependent cost, so which series you measure on is
part of the measurement. `src/tools/ta_bench/bench_corpus.h` holds the corpus —
one deterministic generator, shared by `ta_bench`, `ta_bench_direct` and the
generated `ta_bench_cg` / `ta_bench_stream`. Select a class with `--shape=`:

```bash
cd bin && ./ta_bench --list-shapes            # the input classes and what each reaches

# random walk (default: the historical seed-42 series) and GBM — the acceptance gate
./ta_bench --language=cref,c --function=WILLR --shape=randwalk --iters=500
./ta_bench --language=cref,c --function=WILLR --shape=gbm      --iters=500

# alternating trend/chop legs — the class rolling min/max degrades on
for s in trend-chop-0.5p trend-chop-1p trend-chop-2p trend-chop-4p; do
  ./ta_bench --language=cref,c --function=WILLR --shape=$s --period=30 --iters=500
done
```

The rolling min/max caches the window extremum and rescans the window when that
extremum is the bar dropping out of it, so its cost depends on how often that
happens. On a zero-drift walk the rate decays as ~1/sqrt(period); on a trending
leg it is set by the drift/noise ratio instead and barely moves with the period,
so the two separate further the longer the window (1.1x the rescan rate at
period 14, 3x at period 200). `randwalk` alone cannot see that — issue #147.

The tail shapes are not peers: `constant` is the worst case at `2*(period-1)`
comparisons per bar, exactly twice `mono-up`/`mono-down`. Flat input pins both
extrema because the rescan compares with strict `>`/`<` and leaves the cached
index on `trailingIdx`, so the `>=`/`<=` fast-path arms never run; a monotone
ramp pins only one of the two.

**Which tier that still describes** matters, because #147 replaced half of it.
The batch tier of MIN, MAX, MINMAX, MIDPOINT, MIDPRICE and WILLR is now a Van
Herk / Gil-Werman block scan: branchless, a fixed number of comparisons per bar
at any period, input-independent. So for those six, `constant`, `mono-*` and
`trend-chop-*` all cost the same through `ta_bench --language=c` (the batch
call) and the shape sweep says nothing about them. The rescan — and everything
above — is still what STOCH and STOCHF run, and still what the *streaming* tier
of all six runs, which is what `ta_bench --shape=... --mode=open` and
`ta_bench_stream`'s `update_ns` measure. Reach for the shape sweep when the arm
under test is one of those; for the six functions' batch arm it is inert.

`--shape` is opt-in and `randwalk` reproduces the pre-corpus series bit for bit,
so a default run costs and measures exactly what it did before. `--seed` picks
the stream; `--regime-period` the window the trend/chop regime length is relative
to (defaults to `--period` when given, else 14); `--trend-strength` the trend-leg
drift in per-bar standard deviations (default 0.5 — sweep it to see how the cost
responds to trend/noise). `--verify-corpus` checks every shape is reproducible
and produces valid OHLC, at the `--points` you pass it.

`--list-shapes` groups the classes by what they are for, and the grouping
matters. The rescan rate depends only on the *rank order* of the bars, so
`randwalk-lo`, `randwalk-hi` and `gbm` cannot move it however much they change
the magnitudes — measured within 1% of `randwalk` at period 14/30/200. They are
controls, useful for numerical-conditioning questions (deadbands, cancellation,
ratio-based indicators), not stressors. Only `trend-chop-*` varies the rescan
rate; `mono-*` and `constant` are the analytic tail.

One documented exemption in `--verify-corpus`: the walk family floors `low` at
1.0 but leaves `close` unclamped, so `low <= min(open,close)` fails on 32 bars of
`randwalk` at n=100000 (11 with a negative close). That is inherited from the
pre-corpus generator and is preserved deliberately — clamping `close` would break
the byte-for-byte reproduction of the historical seed-42 series, which matters
more on a timing-only corpus. Every other predicate holds for every shape.

The corpus is timing-only — it is never hashed and is unrelated to
`fuzz_data.h`, whose `FUZZ_*` shape list is iterated by `--fuzz-064` /
`--xlang-hash`. Keep it that way: adding a shape there changes what those gates
compare (see the note at `test_variants.c:148`).
