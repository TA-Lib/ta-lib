# Instructions for TA-Lib maintainers
**If you only want to install and use TA-Lib, there is nothing here for you... check instead https://ta-lib.org/install **

You must have python installed.

## How to push changes to Github?
Modifications (or PR) must be made on the 'dev' branch.

Before committing, run ```scripts/sync.py``` to:
 - Ensure your local dev branch is up-to-date with both remote dev and main branches.
 - Bring every version in the repo in step: the release version from the VERSION file, the shared library version from the public headers. It fails if public API that shipped was removed or changed: put it back, or re-run with `--accept-break` if that is intended.

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

For cross-language server testing (`servers`, `regtest` targets), also: JDK (`javac` + `java`) and .NET SDK (`dotnet`). The `libraries` target additionally needs `unzip`, for the committed Maven wrapper.

```
scripts/build.py                # Build the C library + all C tools (CMake)
scripts/build.py ta_regtest     # Build just the C test runner (CMake)
scripts/build.py ta_codegen     # Build the Rust codegen tool (cargo)
scripts/build.py generate       # Regenerate every committed source for all backends —
                                # libraries, JSON-RPC servers, benches (cargo only: writing
                                # the Java/C# sources needs no JDK or .NET SDK)
scripts/build.py servers        # Generate + compile JSON-RPC language servers (cargo)
scripts/build.py libraries      # Build + test the publishable Java/C# libraries (cargo)
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
cargo run -- build-libraries                     # Build + test the publishable Java/C# libraries
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

The procedure is `docs/release-runbooks/`, whose `README.md` holds the order: the
C publish first, then each language binding to its own registry, then the
post-publish version bump last.

## I want to modify the code... should I care to rebuild the packages?
No. Commit your source changes on dev and let the Github action repackage for
you; it may take up to a day to regenerate and test **all** platforms.

To re-package locally anyway: ```scripts/package.py``` builds the packages for
your host platform, and ```scripts/test-dist.py``` verifies them from a TA-Lib
user's perspective (notably simulating a ta-lib-python user). Avoid pushing
generated packages, but do not worry if you do — the "nightly dev" CI overwrites
them.
