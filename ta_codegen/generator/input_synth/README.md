# Synthetic gate functions (never shipped)

Each `synth<n>/` directory here is a complete function definition in the exact
`ta_codegen/input/<name>/` layout (`.yaml` + `.c` + `.md`), but it is **not** part
of the library. `scripts/synth_gate.py` copies every `synth<n>/` into
`ta_codegen/input/` inside a **throwaway git worktree**, regenerates all backends
there, and runs the usual cross-language gates on the result:

- `ta_regtest --codegen --function=SYNTH` — structural sweep + the
  batch-vs-stream `stream_verify` / OpenAndFill bitwise gates, all four servers.
- `ta_regtest --xlang-hash --function=SYNTH` — batch output parity, Rust/Java/C#
  against the in-process C golden, bitwise (fuzz shapes x seeds x sizes x params).
- `synth_values.py` — hand-derived golden VALUES per fixture, all four servers.
  The two legs above are both *comparative*: one diffs a fixture's stream tier
  against its own batch tier, the other diffs three languages against C. A
  fixture that is wrong the SAME way everywhere passes both. This leg is the
  only one that knows what the numbers should be, so a new fixture needs a row
  in its `GOLDEN` table or it is exercised without ever being checked.

The point: exercise generator constructs (bitwise operators, truthiness
conditions, do-while, switch-on-expression, `PRAGMA TA_ALT` alternates, ...)
that no shipped indicator uses,
with the same end-to-end machinery real functions get — without a fake function
ever appearing in the shipped API. The gate runs nightly in CI
(`dev-nightly-tests.yml`, job `synth-gate`) and locally via
`python3 scripts/synth_gate.py` (no flags; snapshots your dirty tree).

**What it does NOT cover: the generator's own `cargo test`.** The three legs
above are value and parity gates. The static sweeps in
`ta_codegen/generator/tests/` — the ones that read emitted TEXT for structural
properties, which is the class of defect no value gate can see — run only
against the shipped corpus, on the PR gate and in the nightly, never against an
input/ these fixtures have been injected into. So a sweep that says "over the
whole corpus" has never seen a fixture. Pointing `cargo test` at an injected
tree today fails six of them: one is a shipped-corpus inventory a fixture
legitimately changes, two are a peek fallback SYNTH3 is the only thing that
reaches, and three are helpers or metadata that do not model a construct only
the fixtures use. Issue #327 carries the classification; do not add the leg
before those are resolved, or it lands with an allowlist and proves nothing.

Two mechanics worth knowing before touching the script:

- **Post-generate C rebuild.** `regtest.py` builds the C library *before*
  generating (fine for shipped functions, whose generated C is committed). The
  injected SYNTH sources exist only after generation, so the script rebuilds
  the library + `ta_regtest` afterwards — otherwise the in-process golden
  lacks them and the abstract-parity gate fails on an XML length mismatch.
- **Anti-vacuity.** The script asserts each leg swept EXACTLY the number of
  `synth<n>` fixtures, per language. A refactor that silently stops
  enumerating them (e.g. a filter or registration regression) fails the gate
  instead of passing an empty run.

## Adding a synthetic function

1. Create `synth<n+1>/` here with the standard `.yaml`/`.c`/`.md` triple.
   Keep the `SYNTH<n>` name — the digit suffix is the namespace that guarantees
   no clash with a real TA function, and `--function=SYNTH` substring-matches
   the whole family, so the gate picks it up with no other change.
2. Give it `flags: [stream]` and integer outputs where possible — integer
   comparisons are exact in every gate, and streaming coverage is the point.
   Add `path_dependent` too if the body carries state across the main loop that
   a later `startIdx` cannot rebuild from its warm-up window: the
   range-stability leg compares a sub-range call against a full-range one, and
   without the flag it fails `RANGE TEST FAILED (code=162)`. SYNTH1 and SYNTH4
   both need it, and each says why on the flag.
3. Add its row to `synth_values.py`'s `GOLDEN` table, derived by hand from the
   `.c` you just wrote — not from what the generator emits, which is the thing
   under test. Without a row the fixture still runs, and still proves nothing
   about its own values.
4. Run `python3 scripts/synth_gate.py` until green.

## What a fixture's docs may say

The `.md` is authoritative and the `.c` header is the licence/contributors/
change-history block plus a pointer to it. Keep to three things, all of which
the code cannot state for itself:

1. **Which generator construct this exercises, and why the shipped corpus never
   reaches it.** Establish the second half by grepping `ta_codegen/input/`, not
   by trusting a neighbouring fixture's prose.
2. **What would SILENTLY reduce this fixture's own coverage** — a trap about the
   FIXTURE, not about a backend. SYNTH10 is the model: reorder its `.yaml`
   outputs and coverage drops to what MAMA already gives, gate still green.
3. **The issue number.** The post-mortem lives there and in the commit message,
   which are timestamped and do not pretend to be current.

Everything else goes: how each backend renders the construct, how each one once
broke, symbols out of a compiler error, named emitter internals. The gate's
answer is pass/fail in four backends, and a failure is root-caused from the
failure. See the root `CLAUDE.md` for the general rule.

## Rules of thumb for staying language-neutral

The gate compares bitwise, so: no transcendentals (Java/.NET libm differs —
those calls drop to a 1e-9 tolerance), no negative shift operands, keep
accumulator state non-negative, keep every `(int)` cast of a double as the WHOLE
right-hand side of an assignment (the generator rejects nested forms loudly — it
cannot see runtime guards, #160), and GUARD every double→int cast to
`[0, huge)`.

That last one is the trap worth internalising: the fuzz corpus carries
1e9-magnitude, signed and non-finite bars, and an out-of-range or NEGATIVE
double→int conversion is defined differently in each target language — undefined
behaviour in C; a saturating `as` in Rust, where the generator's default target
is `usize` and a negative therefore becomes 0; a clamp to
`Integer.MIN_VALUE`/`MAX_VALUE` in Java, with NaN to 0; and an unchecked
truncation in C#. Fold negative, non-finite and extreme bars to 0.0 with plain
comparisons first (`synth1.c`); comparisons are IEEE-identical everywhere.
