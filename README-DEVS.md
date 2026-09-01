# Instructions for TA-Lib maintainers
**If you only want to install and use TA-Lib, there is nothing here for you... check instead https://ta-lib.org/install **

You must have python installed.

## How to push changes to Github?
Modifications (or PR) must be made on the 'dev' branch.

Before committing, run ```scripts/sync.py``` to:
 - Ensure your local dev branch is up-to-date with both remote dev and main branches.
 - Do various check and fixes on your code (e.g. update "x.y.z" versioning in various files).

Safe to run from anywhere, including a `git worktree`. The two bullets are
independent halves: the first needs to check out dev and main, which git refuses
when another worktree already holds them, so from a worktree (or a detached HEAD)
the script does the second half only and says which case it hit. That is the half
a feature branch wants anyway, and your local dev is left untouched — update it
from its own checkout. A source digest that lags needs no chasing either:
dev-nightly regenerates and commits it with the dist assets, and `merge.py`
refuses dev→main while the two disagree.

Merge to main branch are done with ```scripts/merge.py``` by TA-Lib maintainers with the proper permissions.

## How to update the "./configure" script
This will do all the needed autotools steps:
```$ autoreconf -fi```

Repeat whenever you need to refresh the makefiles.

## How to build and test with scripts/build.py

Prerequisites: CMake 3.18+, a C compiler (clang or gcc), and the Rust toolchain (`rustup`).

For cross-language server testing (`servers`, `regtest` targets), also: JDK (`javac` + `java`) and .NET SDK (`dotnet`).

```
scripts/build.py                # Build the C library + all C tools (CMake)
scripts/build.py ta_regtest     # Build just the C test runner (CMake)
scripts/build.py ta_codegen     # Build the Rust codegen tool (cargo)
scripts/build.py generate       # Regenerate every committed source for all backends —
                                # libraries, JSON-RPC servers, benches (cargo only: writing
                                # the Java/C# sources needs no JDK or .NET SDK)
scripts/build.py servers        # Generate + compile JSON-RPC language servers (cargo)
```

Built binaries go to `bin/`. CMake is configured automatically on first run. The C
library + C tools build with CMake (no Rust needed); `ta_codegen` builds with cargo via
the targets above — CMake never invokes cargo.

To run tests:
```
scripts/build.py test           # C reference tests only (quick)
scripts/build.py regtest        # Full pipeline: servers + C tests + cross-language verification
scripts/build.py regen-check    # The gate every PR runs (cargo + Python, ~1 min)
```

Run `regen-check` before opening a PR if you touched `ta_codegen/input/`:
everything it produces is committed, so an input edit whose regenerated output was
not committed fails CI. It is the same command the PR workflow runs. Only drift
*that run introduces* fails it — the rest of your working tree can be dirty.

For more control, run `ta_regtest` directly from `bin/`:
```
./ta_regtest                                               # C reference tests only
./ta_regtest --codegen                                     # C tests + all-language codegen
./ta_regtest --codegen --language=c,rust                   # Filter to specific languages
./ta_regtest --codegen --function=RSI,SMA                  # Filter to specific functions
```

## How to run ta_codegen

`ta_codegen` is the single code generator: it generates the C library (in place under `src/`), the Rust/Java/C# bindings, and the JSON-RPC test servers:

```
cd ta_codegen/generator
cargo run -- generate                            # Generate everything, all backends
cargo run -- generate --func=SMA --backend=rust  # Specific function + backend
cargo run -- generate-servers                    # Only the JSON-RPC servers
cargo run -- build                               # Compile servers
```

Generated output goes to `ta_codegen/output/` organized by language.

## How to build with CMakeLists.txt
```
$ cd ta-lib
$ mkdir build
$ cd build
$ cmake ..
$ make
```
Libraries will be in ```ta-lib/build``` and executable in ```ta-lib/bin```


## How to run ta_regtest
After ```make```, run ```ta_regtest``` from ```ta-lib/bin``` (CMake build) or ```ta-lib/src/tools/ta_regtest``` (autotools build)

Exit code is 0 on success


## How to do a new release?

Any dev with permission to merge to main branch can do a release.

(1) On the dev branch, edit the VERSION file in the root of the repos.

(2) Run "./scripts/sync.py". This ensures your dev branch is up-to-date (among other things).

(3) Push to the dev branch.

(4) Manually trig the "nightly dev" Github action. This will regenerate and test for **all** platforms. If you do not trig it, it will get run anyway once per day.

(5) Merge dev into main with "./scripts/merge.py". At this point, the main branch is the release candidate with all the assets under "dist" folder.

(6) Manually trig the "nightly main" Github action. This will perform a last round of check prior to alloweing for the release. If you do not trig it, it will get run anyway once per day.

(7) Manually trig the "Release (step-1)" Github action on main branch. This will tag, generate a draft release and attach all assets from the dist/ directory.

(8) Optionally edit the draft "Release notes" on the Github website. A good time to add thank you to contributors. You can still edit after the official release.

(9) Manually trig "Release (step 2)" Github action. This will make the release official/public and update the website.

(10) Verify the Github release page shows the new version with all assets attached and downloadable. The website (https://ta-lib.org/install) catches up on its own within a nightly cycle afterward — see "After a release" below.

(11) Run "./scripts/post-release-vcpkg.py" and follow the instructions to submit a PR to microsoft/vcpkg. Monitor the PR is eventually merged by vcpkg maintainers. This may take a few days.

(12) Monitor homebrew-core. The formula is updated within about an hour:
https://github.com/Homebrew/homebrew-core/blob/HEAD/Formula/t/ta-lib.rb


## After a release

Right after a release is public, open the next development version so `dev`/`main`
stop advertising an already-released version:

(A) On dev, bump the VERSION file to the next patch (e.g. `0.7.2` -> `0.7.3`). The exact number can be adjusted later; it only has to be higher.

(B) Add a `## [0.7.3] Not Released Yet` entry at the top of CHANGELOG.md.

(C) Run `./scripts/sync.py`, push dev, then merge to main as usual.

The website catches up on its own within a nightly cycle. Confirm with:

```bash
./scripts/sync-website.py --check   # non-zero if the website is behind
```


## I want to modify the code... should I care to rebuild the packages?
No. Commit your source changes on dev and let the Github action repackage for
you; it may take up to a day to regenerate and test **all** platforms.

To re-package locally anyway: ```scripts/package.py``` builds the packages for
your host platform, and ```scripts/test-dist.py``` verifies them from a TA-Lib
user's perspective (notably simulating a ta-lib-python user). Avoid pushing
generated packages, but do not worry if you do — the "nightly dev" CI overwrites
them.
