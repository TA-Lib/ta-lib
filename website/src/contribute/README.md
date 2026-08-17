---
title: Contribute a TA Function
description: "Contribute a new TA function to TA-Lib: agree the spec in the open, hand the implementation to an AI agent, prove it against golden values from an oracle."
---

# How to Contribute a New TA Function

::: tip Just want to request a function, not implement it?
Then add it to the [New TA Functions board](https://github.com/orgs/TA-Lib/projects/1) and someone else might implement it for you.
:::

TA-Lib development is **human-driven specs, AI-driven code**.

The implementation is meant to be written by an AI agent: hand it this page, which contains everything needed to do the job.

Humans agree, in the open, what a function computes before code is written: its name, formula, inputs, parameters and defaults.

No function is ever integrated without hardcoded golden values in the regression tests, taken from an independent oracle or a published sample. The regression tests are the final arbiter of correctness, and they are human-reviewed.

The test harness is exhaustive (verifies many edge cases) and you should not have to modify these. Any modification to the harness requires explicit human approval. Adding and registering your own function's test is expected. "The harness" means the shared machinery: the comparison gates, tolerances and other functions' tests.

## What a contribution looks like

TA-Lib generates all of its per-function code from a single source of truth. One function is one directory, `ta_codegen/input/<name>/`, holding three files:

| File | Contains |
| --- | --- |
| `<name>.yaml` | Metadata: inputs, outputs, parameters, defaults, group, flags. Data only, never logic. |
| `<name>.c` | The algorithm: a `<name>_lookback()` and a `<name>()` batch function, in plain portable C. |
| `<name>.md` | Documentation: summary, the formula in its original algebraic form, references. |

From these three files, the `ta_codegen` generator produces everything the project ships: the C library, native Rust, Java and .NET implementations, streaming variants, the test servers, benchmarks, and this website's function page. You never write Rust, Java or C# by hand, and you never edit generated files.

Every backend is then verified by `ta_regtest`, which drives all languages through JSON-RPC servers and cross-checks their outputs against the C reference, bit-for-bit in the strictest gates.

## Development setup

Any of Linux, macOS, Windows or WSL2. You need:

- git, a C compiler (gcc/clang/MSVC), CMake ≥ 3.18
- The Rust toolchain via [rustup](https://rustup.rs) (the generator is written in Rust)
- Recommended, for full cross-language verification: a JDK (`javac`/`java`) and the .NET SDK (`dotnet`). Without them, pass `--language=c,rust` to `scripts/build.py servers` and `scripts/regtest.py`. The C and Rust gates still verify your function, and CI runs the full matrix.

```bash
git clone https://github.com/TA-Lib/ta-lib.git
cd ta-lib
git switch dev              # contributions branch from and merge into dev
scripts/build.py            # build the C library + tools
scripts/build.py test       # run the C regression tests (confirms your setup works)
scripts/build.py servers    # build the generator + the per-language test servers
```

`scripts/build.py` checks prerequisites per target and configures CMake automatically on first run.

## Steps

1. **Agree on the spec first.** Claim a card on the [board](https://github.com/orgs/TA-Lib/projects/1), or open an issue if there is none. Settle the name, group, inputs, parameters with defaults, flags (does it stream?), and the formula with a citable reference before writing code. This is the human-approved part.
2. **Write the three input files** in `ta_codegen/input/<name>/`. Start by copying a similar shipped function; `ta_codegen/input/cmf/` is a small, recent example. Input files carry no license header — the generator injects the BSD-3-Clause notice into every generated file — but the `.c` opens with a contributor and change-history block: add your initials there, and a one-line MMDDYY entry describing your change. The file-format references are in the repository: [`docs/ta_codegen_input_yaml.md`](https://github.com/TA-Lib/ta-lib/blob/dev/docs/ta_codegen_input_yaml.md), [`ta_codegen_input_code.md`](https://github.com/TA-Lib/ta-lib/blob/dev/docs/ta_codegen_input_code.md) and [`ta_codegen_input_doc.md`](https://github.com/TA-Lib/ta-lib/blob/dev/docs/ta_codegen_input_doc.md). When in doubt, read your checkout's copy; it tracks the generator you are running.
3. **Generate** all backends: `scripts/build.py generate`, then `scripts/build.py servers` to rebuild the language servers from the regenerated sources.
4. **Verify**: run `scripts/regtest.py`, the full pipeline (it also builds the frozen pre-cutover reference server that `ta_regtest --codegen` needs). A brand-new function is reported as *skipped* by the generic sweep, because it does not exist in the frozen reference; the next step is what verifies it. The first run takes ~30 minutes and checks out the frozen reference into a sibling `ta-lib-ref` worktree; `--no-perftest` skips the benchmarks a correctness change does not need.
5. **Prove the values.** A new function needs a regression test with golden values from an independent source: a published reference implementation or worked examples from the literature. Document the source, its version and the tolerance at the test call site; against a double-precision oracle ~1e-12 relative is typical, and anything looser needs a written justification. See `src/tools/ta_regtest/ta_test_func/test_composite.c` for the pattern, and register the test in four places: a prototype in `ta_test_func.h`, a `DO_TEST` entry in `ta_regtest.c` whose tag names every function the group covers, and the file added to both `CMakeLists.txt` and `src/tools/ta_regtest/Makefile.am` (`scripts/build.py check-source-lists` verifies the two agree). While iterating, `cd bin && ./ta_regtest --codegen --function=<NAME>` re-runs just your group, with every call cross-checked bitwise against all language servers.
6. **Check the diff.** After regenerating, `git diff` should touch only files belonging to your function. Unrelated churn in other generated files means something is wrong.
7. **Open a pull request** against the `dev` branch, citing the spec issue and the verification source.

Stuck at any step? Ask on [Discord](https://discord.com/invite/Erb6SwsVbH) or comment on your spec issue.

## Notes for AI assistants

You are implementing a TA function for TA-Lib, following the steps above on behalf of a human contributor. Additional constraints and pointers:

**Ground truth is the repository, not this page.** Read, in order:

- `CLAUDE.md` at the repo root
- In `docs/`: `ta_codegen_input_yaml.md`, then `ta_codegen_input_code.md`, then `ta_codegen_input_doc.md`

If this page and the repo disagree, the repo wins; it is versioned with the code. If you are Claude Code, the repo ships a `/new-ta-func` skill that automates this workflow; use it. It is an accelerator, not a requirement: everything it does is covered by this page and the repository docs.

**Invariants.** Violating any of these fails review:

- No logic in YAML, ever. No metadata in the `.c` file.
- Never edit generated output: `src/ta_func/`, `src/ta_abstract/` and the per-function sources under `ta_codegen/output/` are overwritten on the next `generate`. Change `ta_codegen/input/` and regenerate. Not all of `output/` is generated — the hand-written scaffolding, the build files (`pom.xml`, `TALib.csproj`) and the Java and C# test suites live there too, and `generate` preserves them; treat those as harness, per the rule above. A generated file says so in its header, so a file without that banner is hand-written.
- `<name>.md` documents the original algebra of the indicator, never implementation artifacts: no zero-guards, epsilon comparisons or `period == 1` special cases in the formula.
- In the `.c` input, call other TA functions by their bare lowercase name (`sma(...)`, `ema_lookback(...)`); the generator resolves each to the language's native symbol.
- Your function may be called with an output array aliasing one of its inputs — `outReal == inClose` is a supported, tested calling convention. Within a bar, read every input value you need *before* writing that bar's output; a trailing index can reach the slot you just wrote. Carry what you need in a scalar. Every function is checked, bitwise, on every input/output pair.
- New functions do not support `TA_SetCompatibility`. The compatibility constants are preserved for the functions that already honour one, and the Rust, Java and C# APIs expose no such setting — so honouring it in a new function makes its C output diverge from three backends that cannot read it. Copying an EMA-shaped function will hand you a METASTOCK seeding arm; drop it.
- Enums, groups and other shared surfaces are generated; search the generator before hand-adding one anywhere.
- If `generate` panics on a C construct, do not contort the algorithm to dodge the parser. Match the style of a shipped input file, or raise it on the spec issue: parser extensions are generator changes and need maintainer sign-off.

**Definition of done:**

- `scripts/regtest.py` passes end to end (C tests + all-language verification).
- The new function has a non-vacuous test: golden values from an independent source, with source, version and tolerance documented at the call site. A test that cannot fail on a wrong formula does not count. To show it cannot, break what it guards and watch it go red — and in a generated tree, confirm the break actually landed: mutate `ta_codegen/input/`, regenerate, then check the mutation is present in the generated source. A green gate over a mutation that never reached the code means *sabotage never applied*, not *guard verified*.
- Regeneration is idempotent: running `generate` again leaves `git status` clean.
- The generator's own suite passes: `cd ta_codegen/generator && cargo test`. Some of its tests are inventories keyed on a function's properties; a new function may belong in one. A failure naming your function in a test you never touched is the inventory asking to be updated, not a regression.
- The generated `website/src/functions/<name>.md` is correct; its sections are gated against the YAML by the generator. Optional live preview: `cd website && pnpm install && pnpm docs:dev` (needs Node and pnpm).

**DO NOT:**

- **DO NOT** compute golden values yourself and attribute them to a reference implementation. What makes an oracle independent is not the citation but the execution: the values must be *produced by running* the cited source at the cited version, or transcribed unchanged from a table published in it. Re-deriving the formula and labelling the result with a vendor's name is the same computation twice with a false provenance attached — worse than no oracle, because it agrees with your implementation for exactly the reason it must not. If you cannot run the source, say so on the spec issue rather than substituting your own arithmetic; a maintainer may be able to run it for you.
- **DO NOT** invent parameter defaults or ranges. The `<name>.yaml` spec is the human-approved source of truth for acceptable ranges and defaults; ask the contributor.
- **DO NOT** modify the test harness or loosen a tolerance to make a gate pass. Harness changes require explicit human approval.
- **DO NOT** include unrelated generated-file churn in the pull request.
- **DO NOT** commit or push without explicit human approval.
