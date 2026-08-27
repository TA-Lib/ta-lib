# SYNTH13

## Summary

Synthetic gate function: exercises a **cross-indicator call** in the fixture
corpus for the first time — four legs, each a call to `sma()` wrapped in a
different guard shape, so `ir_cleanup::drop_answered_cross_call_guards` and
`ir_cleanup::drop_inert_guards` run end to end through the real Rust, Java and
C# emitters instead of only their own unit tests. It exists only to verify the
code generator; it is never shipped (see
`ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[i] = 4 × SMA(real, optInTimePeriod)[i]

## Notes

- Four legs, four guard shapes, verified against `ta_codegen/input/` for which
  ones the shipped corpus already reaches:
  - **Leg A** — a bare `if( retCode != TA_SUCCESS )`. Dropped whole in
    Rust/Java/C#, as `apo.c`'s two guards already are.
  - **Leg B** — `if( (retCode != TA_SUCCESS) || ((int)*outNBElement == 0) )`,
    copied verbatim from `bbands.c`/`stoch.c`. Shortened to the count test
    alone.
  - **Leg C** — `savedRetCode = retCode;` sits between the call and its guard.
    The guard must survive untouched. No shipped function has this shape: the
    two-`free()` gap in `macdext.c` that motivates the "skip a non-mentioning
    statement" rule is a *different* case from this one, which tests the rule
    that stops the scan.
  - **Leg D** — `if( retCode == TA_SUCCESS )`. Grepping every `.c` under
    `ta_codegen/input/` for `== TA_SUCCESS` returns nothing — this operator is
    refused unconditionally, and until this fixture nothing exercised that.
    Its body is bookkeeping only (`savedRetCode`, read nowhere): the leg's own
    accumulate sits AFTER the guard, unconditional. Moving it inside would
    still pass every ir_cleanup check (the guard is refused either way) but
    would silently break the OTHER gate this fixture also exercises —
    `analyze_composed`'s tail scan replays a generic `if` verbatim in Open and
    excludes it from the per-bar Update pipeline entirely, so the streamed
    output would quietly stop matching the batch one. Only `stream_verify`'s
    bitwise batch-vs-stream comparison would catch that, not anything in
    `ir_cleanup.rs`.
  - A standalone `if( bufferIsAllocated ) { free( scratch ); }` after leg E,
    unrelated to any cross-call, covers `drop_inert_guards` on a plain
    deallocation guard (`stoch.c`/`stochrsi.c` write the identical line, so
    this one alone duplicates shipped coverage — kept anyway, as the fixture's
    clean baseline case, and as the one `free` the leak-check accepts as
    `scratch`'s replayable free for the streamed Open).
- Coverage trap: legs A, B, C and D each nest a `free()` inside their own
  `if( retCode != TA_SUCCESS ) { ... }` body. For A that whole guard folds
  away as a unit, taking the nested free with it — a fold that happens
  BEFORE `drop_deallocation` ever runs, so it never independently exercises
  that pass. For B, C and D the outer guard survives (shortened or
  untouched), so the nested free is what's left for `drop_deallocation`/
  `drop_inert_guards` to clean, *after* a guard that
  `drop_answered_cross_call_guards` left standing — collapsing every leg down
  to leg A's shape would lose that interaction silently, since the whole
  guard vanishing subsumes the nested one.
- Every leg reuses `optInTimePeriod` — same input, same range — so all five
  are numerically identical and `legBegIdx`/`legNbElement` are overwritten
  and re-read on every leg rather than kept distinct per leg (fewer locals);
  the guards are validated by which C construct folds, not by five distinct
  formulas.
- Every guard here is dead in practice: `startIdx`/`endIdx`/`optInTimePeriod`
  are already validated by synth13's own public tier before any leg runs, and
  every leg calls `sma()` with that same validated range, so `retCode` is
  `TA_SUCCESS` on every real call the gates make. That is what makes folding
  legs A/B correct rather than merely convenient, and it is also why legs
  C/D/E's surviving guards are safe to leave live — they never actually
  divert control flow during `--codegen`/`--xlang-hash`.

## Inputs

- `inReal` — Price series to average

## Outputs

- `outReal` — Five times the simple moving average of `inReal`

## Parameters

- `optInTimePeriod` — Time period, passed unchanged to every leg's `sma()` call

## Implementation

TA-Lib Definition: [`synth13.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth13/synth13.c) · [`synth13.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth13/synth13.yaml)

## Two things this fixture deliberately does not do

**It never writes `outReal` until every leg has read `inReal`.** Whole-buffer in
place is legal in the batch tier (rule N4), so a leg that wrote the output
before a later leg re-read the input would corrupt itself — the synth gate's
in-place probe catches exactly that, and did.

**It does not cover "control flow between the call and its guard."** The two
ways to express it here are both wrong: a loop writing a declared output models
issue #269, the defect the pin next door exists to forbid; and a loop
accumulating into a second local buffer is not streamable (`accum` accessed
outside `series[cursor]` form), which would cost the `stream` flag — and the
stream tier is where three of the cleanup sequence's seven hook sites live, the
half unit tests cannot reach. That property is covered instead by
`control_flow_between_the_two_stops_the_scan` in `backends::ir_cleanup`.
