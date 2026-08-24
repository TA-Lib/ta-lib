Tools intended for TA-Lib maintainers.

If you only want to install and use TA-Lib, there is nothing here for you... check instead the main README.md file.

See `README-DEVS.md` at the repo root for the build/test/release walkthroughs.

## Build and test

| Script | What it does |
|---|---|
| `build.py` | The developer build entry point: C library + C tools (CMake), and `ta_codegen` / `generate` / `servers` (cargo). CMake never invokes cargo. |
| `regtest.py` | Full pipeline: generate → build → correctness → benchmark. The nightly drives it three ways. |
| `gen_test_reference.py` | Rebuilds `ta_regtest`'s baked numerical goldens (`src/tools/ta_regtest/ta_test_reference_golden.{h,c}`) from the datasets in `ta_test_reference.c`, in exact rational arithmetic. Run it when a dataset changes; `--check` verifies in place. Deliberately NOT on a gate — `ta_regtest --function=REFERENCE` catches a stale table at runtime, because the oracle stops reproducing it. |

## Verification gates

Pass/fail only — build something, drive it, exit non-zero. Each is one nightly job.

| Script | Nightly job | Gates |
|---|---|---|
| `synth_gate.py` | `synth-gate` | Generator surface no shipped indicator uses, via synthetic functions injected into a throwaway worktree (`ta_codegen/generator/input_synth/`) |
| `stream_sanitize.py` | `stream-sanitizers` | The C streaming API under ASan/UBSan/LSan — paths the batch sanitizer job never calls |
| `rust_stream_debug.py` | `cross-language-rust-debug` | The Rust streaming API under debug overflow checks; reuses the request generator from `stream_sanitize.py` |

Everything else CI gates on lives in `ta_regtest` (C) or is a step inside
`build.py` / `regtest.py`, not a script here.

## Release

| Script | When |
|---|---|
| `sync.py` | Before every commit. Two halves: it merges remote dev/main into local dev, and it refreshes versions + `TA_LIB_SOURCES_DIGEST`. Safe to run from anywhere — the merge half is **skipped automatically** where it cannot run (a `git worktree`, or a detached HEAD) and the metadata half still runs. See the header of the script |
| `merge.py` | Merge dev into main (maintainers) |
| `package.py` | Build this platform's `dist/` assets. Run by both nightlies |
| `test-dist.py` | Verify those assets as a user would, including a ta-lib-python build. Run by both nightlies |
| `pre-release-checks.py` | Gate for `release-step-1`/`-2`: version consistency, digest, CHANGELOG entry, assets present |
| `post-release-vcpkg.py` | Open the microsoft/vcpkg PR after a release |
| `sync-website.py` | Point the website install page at the latest *published* release; `--check` to test only |

## Support (imported or called, never run directly)

| Path | Used by |
|---|---|
| `utilities/` | Versions, package digests, file/archive comparison, Windows `vcvarsall` — imported across the release scripts |
| `install_tests/` | MSI and Python-wheel install verification — imported by `test-dist.py` |
| `serve_version.py` | Builds a "serve of another version" oracle from a pinned worktree — imported by `regtest.py` |
| `build_064_serve.py` | Builds `bin/ta_064_serve` (the frozen v0.6.4 oracle for `ta_regtest --fuzz-064`) — called by `build.py` |
