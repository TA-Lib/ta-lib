Tools intended for TA-Lib maintainers.

If you only want to install and use TA-Lib, there is nothing here for you... check instead the main README.md file.

See `README-DEVS.md` at the repo root for the build/test/release walkthroughs.

## Build and test

| Script | What it does |
|---|---|
| `build.py` | The developer build entry point: C library + C tools (CMake), and `ta_codegen` / `generate` / `servers` (cargo). CMake never invokes cargo. |
| `regtest.py` | Full pipeline: generate → build → correctness → benchmark. The nightly drives it three ways. |
| `python-dev.py` | Keeps `~/ta-lib-python` on **dev** in step with this worktree: builds the wrapper from both `_ta_lib.pyx` and the committed `_ta_lib.c`, runs its suite, checks that regenerating changes nothing, and diffs its enum/flag tables against `include/`. `sync` regenerates the drift. Commits nothing. |
| `abi.py` | `check`: the PR/nightly gate on the public C ABI and the shared library version. `sync.py` does the updating. |
| `quiet.py` | The shared quiet window for timing runs across sessions on one machine: `measure` holds it exclusively (exit 75 if taken; `--queue` waits its turn, first come first served), `noisy` marks a heavy job so measurers back off (`--defer` lets a running or queued measurement go first), `status` names the holder and the queue. Every wait is bounded. |
| `test_quiet.py` | Tests for `quiet.py`, each against a throwaway HOME. |
| `gen_test_reference.py` | Rebuilds `ta_regtest`'s baked numerical goldens (`src/tools/ta_regtest/ta_test_reference_golden.{h,c}`) from the datasets in `ta_test_reference.c`, in exact rational arithmetic. Run it when a dataset changes; `--check` verifies in place. Deliberately NOT on a gate — `ta_regtest --function=REFERENCE` catches a stale table at runtime, because the oracle stops reproducing it. |

## Verification gates

Pass/fail only — build something, drive it, exit non-zero. Each is one nightly job.

| Script | Nightly job | Gates |
|---|---|---|
| `synth_gate.py` | `synth-gate` | Generator surface no shipped indicator uses, via synthetic functions injected into a throwaway worktree (`ta_codegen/generator/input_synth/`) |
| `stream_sanitize.py` | `stream-sanitizers` | The C streaming API under ASan/UBSan/LSan — paths the batch sanitizer job never calls |
| `rust_stream_debug.py` | `cross-language-rust-debug` | The Rust streaming API under debug overflow checks; reuses the request generator from `stream_sanitize.py` |
| `bench_icount.py` | `dev-nightly` (`icount` job) | Retired instructions for all ~1000 C entry points against `.github/perf/icount-baseline-<arch>.tsv`, and again with `--shape=peg` against `icount-baseline-<arch>-peg.tsv`. Counts, not time: exact on a shared runner, which is what lets a 10% threshold mean anything. The baseline only ever moves down, so a sub-threshold regression is never absorbed; raising a row takes `--accept`, which names the rows and leaves every other row's accumulated best alone. Read its header for what a count cannot see |

Everything else CI gates on lives in `ta_regtest` (C), `abi.py check`, or a step
inside `build.py` / `regtest.py`.

## Release

| Script | When |
|---|---|
| `sync.py` | Before every commit. Two halves: it merges remote dev/main into local dev, and it refreshes versions, the shared library version (`ABI.manifest`, `ABI.released`, `TALIB_LIBRARY_VERSION`), `TA_LIB_SOURCES_DIGEST` and the website install page. Idempotent; `--accept-break` records removed or changed public API. Safe to run from anywhere — the merge half is **skipped automatically** where it cannot run (a `git worktree`, or a detached HEAD) and the metadata half still runs. See the header of the script |
| `merge.py` | Merge dev into main (maintainers) |
| `package.py` | Build this platform's `dist/` assets. Run by both nightlies |
| `test-dist.py` | Verify those assets as a user would, including a ta-lib-python build. Run by both nightlies |
| `pre-release-checks.py` | Gate for `release-step-1`/`-2`: version consistency, digest, CHANGELOG entry, assets present, `ABI.released` is the latest published release |
| `post-release-vcpkg.py` | After a release, update the microsoft/vcpkg port. `docs/release-runbooks/c-publish-runbook.md` step CP11 |
| `sync-website.py` | The website half of `sync.py` on its own, across every backend's page; `--check` exits non-zero if a page is behind or a registry could not be reached |

## Support (imported or called, never run directly)

| Path | Used by |
|---|---|
| `utilities/` | Versions, package digests, file/archive comparison, Windows `vcvarsall` — imported across the release scripts |
| `install_tests/` | MSI and Python-wheel install verification — imported by `test-dist.py` |
| `utilities/ta_ref.py` | Builds the serve of each `ta_ref/` member (`bin/ta_ref_<X_Y_Z>_serve`); imported by `build.py ref` and `regtest.py` |
