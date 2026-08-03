# Synthetic gate functions (never shipped)

Each `synth<n>/` directory here is a complete function definition in the exact
`ta_codegen/input/<name>/` layout (`.yaml` + `.c` + `.md`), but it is **not** part
of the library. `scripts/synth_gate.py` copies every `synth<n>/` into
`ta_codegen/input/` inside a **throwaway git worktree**, regenerates all backends
there, and runs the usual cross-language gates on the result:

- `ta_regtest --codegen-only --function=SYNTH` — structural sweep + the
  batch-vs-stream `stream_verify` / OpenAndFill bitwise gates, all four servers.
- `ta_regtest --xlang-hash --function=SYNTH` — batch output parity, Rust/Java/C#
  against the in-process C golden, bitwise (fuzz shapes x seeds x sizes x params).

The point: exercise generator constructs (bitwise operators, truthiness
conditions, do-while, switch-on-expression, ...) that no shipped indicator uses,
with the same end-to-end machinery real functions get — without a fake function
ever appearing in the shipped API. The gate runs nightly in CI
(`dev-nightly-tests.yml`, job `synth-gate`) and locally via
`python3 scripts/synth_gate.py` (no flags; snapshots your dirty tree).

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
3. Run `python3 scripts/synth_gate.py` until green.

Rules of thumb for staying language-neutral (the gate compares bitwise):
no transcendentals (Java/.NET libm differs — those calls drop to 1e-9
tolerance), no negative shift operands, keep accumulator state non-negative,
keep every `(int)` cast of a double as the WHOLE right-hand side of an
assignment (the generator rejects nested forms loudly — it cannot see runtime
guards, #160), and GUARD every double→int cast to `[0, huge)`: the fuzz corpus includes
1e9-magnitude and signed bars, and out-of-range or NEGATIVE `(int)` conversion
is defined differently in C (UB / truncate), Rust (saturates — negatives
become 0), Java (clamps) and C# — fold negative/non-finite/extreme bars to
0.0 with plain comparisons first (see `synth1.c`; the gate caught both the
extreme-magnitude and the negative-value divergence on its first runs).
