---
title: Contribute a TA Function
---

# How to Contribute a New TA Function

::: tip Just want to request a function — not implement it?
Check the [New TA Functions board](https://github.com/orgs/TA-Lib/projects/1). If the function you want is not already there, open a [GitHub issue](https://github.com/TA-Lib/ta-lib/issues) naming the function and a published reference for its formula, and it will be added to the board for anyone to pick up.
:::

TA-Lib development is **human-driven specs, AI-driven code**. What a function computes — its name, formula, inputs, parameters and defaults — is agreed between humans, in the open, before code is written. The implementation is meant to be written by an AI agent: hand it this page — it contains everything needed to do the job.

No function is ever integrated without hardcoded golden values — from an independent oracle or a published sample — in the regression tests. The regression tests are the final arbiter of correctness, and they are human-reviewed.

The test harness is exhaustive (verifies many edge cases) and you should not have to modify these. Any modification to the harness requires explicit human approval. Adding and registering your own function's test is expected — "the harness" means the shared machinery: the comparison gates, tolerances and other functions' tests.

## What a contribution looks like

TA-Lib generates all of its per-function code from a single source of truth. One function is one directory, `ta_codegen/input/<name>/`, holding three files:

| File | Contains |
| --- | --- |
| `<name>.yaml` | Metadata: inputs, outputs, parameters, defaults, group, flags. Data only — never logic. |
| `<name>.c` | The algorithm: a `<name>_lookback()` and a `<name>()` batch function, in plain portable C. |
| `<name>.md` | Documentation: summary, the formula in its original algebraic form, references. |

From these three files, the `ta_codegen` generator produces everything the project ships: the C library, native Rust, Java and .NET implementations, streaming variants, the test servers, benchmarks, and this website's function page. You never write Rust, Java or C# by hand, and you never edit generated files.

Every backend is then verified by `ta_regtest`, which drives all languages through JSON-RPC servers and cross-checks their outputs against the C reference — bit-for-bit in the strictest gates.

## Development setup

Any of Linux, macOS, Windows or WSL2. You need:

- git, a C compiler (gcc/clang/MSVC), CMake ≥ 3.18
- The Rust toolchain via [rustup](https://rustup.rs) (the generator is written in Rust)
- Recommended, for full cross-language verification: a JDK (`javac`/`java`) and the .NET SDK (`dotnet`). Without them, pass `--language=c,rust` to `scripts/build.py servers` and `scripts/regtest.py` — the C and Rust gates still verify your function, and CI runs the full matrix.

```bash
git clone https://github.com/TA-Lib/ta-lib.git
cd ta-lib
git switch dev              # contributions branch from and merge into dev
scripts/build.py            # build the C library + tools
scripts/build.py test       # run the C regression tests — confirms your setup works
scripts/build.py servers    # build the generator + the per-language test servers
```

`scripts/build.py` checks prerequisites per target and configures CMake automatically on first run.

## Steps

1. **Agree on the spec first.** Claim a card on the [board](https://github.com/orgs/TA-Lib/projects/1), or open an issue if there is none. Settle the name, group, inputs, parameters with defaults, flags (does it stream?), and the formula with a citable reference before writing code. This is the human-approved part.
2. **Write the three input files** in `ta_codegen/input/<name>/`. Start by copying a similar shipped function — `ta_codegen/input/cmf/` is a small, recent example. Keep the BSD-3-Clause license header, and add your initials and a one-line MMDDYY entry to its contributor table. The file-format references are in the repository: [`docs/ta_codegen_input_yaml.md`](https://github.com/TA-Lib/ta-lib/blob/dev/docs/ta_codegen_input_yaml.md), [`ta_codegen_input_code.md`](https://github.com/TA-Lib/ta-lib/blob/dev/docs/ta_codegen_input_code.md) and [`ta_codegen_input_doc.md`](https://github.com/TA-Lib/ta-lib/blob/dev/docs/ta_codegen_input_doc.md) — when in doubt, read your checkout's copy; it tracks the generator you are running.
3. **Generate** all backends: `scripts/build.py generate`, then `scripts/build.py servers` to rebuild the language servers from the regenerated sources.
4. **Verify**: run `scripts/regtest.py` — the full pipeline (it also builds the frozen pre-cutover reference server that `ta_regtest --codegen` needs). A brand-new function is reported as *skipped* by the generic sweep, because it does not exist in the frozen reference; the next step is what verifies it. The first run takes ~30 minutes and checks out the frozen reference into a sibling `ta-lib-ref` worktree; `--no-perftest` skips the benchmarks a correctness change does not need.
5. **Prove the values.** A new function needs a regression test with golden values from an independent source — a published reference implementation or worked examples from the literature. Document the source, its version and the tolerance at the test call site; against a double-precision oracle ~1e-12 relative is typical, and anything looser needs a written justification. See `src/tools/ta_regtest/ta_test_func/test_composite.c` for the pattern, and register the test in four places: a prototype in `ta_test_func.h`, a `DO_TEST` entry in `ta_regtest.c` whose tag names every function the group covers, and the file added to both `CMakeLists.txt` and `src/tools/ta_regtest/Makefile.am` (`scripts/build.py check-source-lists` verifies the two agree). While iterating, `cd bin && ./ta_regtest --codegen --function=<NAME>` re-runs just your group, with every call cross-checked bitwise against all language servers.
6. **Check the diff.** After regenerating, `git diff` should touch only files belonging to your function. Unrelated churn in other generated files means something is wrong.
7. **Open a pull request** against the `dev` branch, citing the spec issue and the verification source.

Stuck at any step? Ask on [Discord](https://discord.com/invite/Erb6SwsVbH) or comment on your spec issue.

## Notes for AI assistants

You are implementing a TA function for TA-Lib, following the steps above on behalf of a human contributor. Additional constraints and pointers:

**Ground truth is the repository, not this page.** Read, in order: `CLAUDE.md` at the repo root, then `docs/ta_codegen_input_yaml.md`, `docs/ta_codegen_input_code.md` and `docs/ta_codegen_input_doc.md`. If this page and the repo disagree, the repo wins — it is versioned with the code. If you are Claude Code, the repo ships a `/new-ta-func` skill that automates this workflow; use it. It is an accelerator, not a requirement — everything it does is covered by this page and the repository docs.

**Invariants** — violating any of these fails review:

- No logic in YAML, ever. No metadata in the `.c` file.
- Never edit generated output: `src/ta_func/`, `src/ta_abstract/` and everything under `ta_codegen/output/` are overwritten on the next `generate`. Change `ta_codegen/input/` and regenerate.
- `<name>.md` documents the original algebra of the indicator, never implementation artifacts — no zero-guards, epsilon comparisons or `period == 1` special cases in the formula.
- In the `.c` input, call other TA functions by their bare lowercase name (`sma(...)`, `ema_lookback(...)`); the generator resolves each to the language's native symbol.
- Enums, groups and other shared surfaces are generated; search the generator before hand-adding one anywhere.
- If `generate` panics on a C construct, do not contort the algorithm to dodge the parser — match the style of a shipped input file, or raise it on the spec issue: parser extensions are generator changes and need maintainer sign-off.

**Definition of done:**

- `scripts/regtest.py` passes end to end (C tests + all-language verification).
- The new function has a non-vacuous test: golden values from an independent source, with source, version and tolerance documented at the call site. A test that cannot fail on a wrong formula does not count.
- Regeneration is idempotent: running `generate` again leaves `git status` clean.
- The generator's own suite passes: `cd ta_codegen/generator && cargo test`.
- The generated `website/src/functions/<name>.md` is correct — its sections are gated against the YAML by the generator. Optional live preview: `cd website && pnpm install && pnpm docs:dev` (needs Node and pnpm).

**DO NOT:**

- **DO NOT** invent parameter defaults or ranges — the `<name>.yaml` spec is the human-approved source of truth for acceptable ranges and defaults; ask the contributor.
- **DO NOT** modify the test harness or loosen a tolerance to make a gate pass — harness changes require explicit human approval.
- **DO NOT** include unrelated generated-file churn in the pull request.
- **DO NOT** commit or push without explicit human approval.
