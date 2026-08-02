# Instructions for TA-Lib maintainers
**If you only want to install and use TA-Lib, there is nothing here for you... check instead https://ta-lib.org/install **

You must have python installed.

## How to push changes to Github?
Modifications (or PR) must be made on the 'dev' branch.

Before committing, run ```scripts/sync.py``` to:
 - Ensure your local dev branch is up-to-date with both remote dev and main branches.
 - Do various check and fixes on your code (e.g. update "x.y.z" versioning in various files).

Merge to main branch are done with ```scripts/merge.py``` by TA-Lib maintainers with the proper permissions.

## How to update the "./configure" script
This will do all the needed autotools steps:
```$ autoreconf -fi```

Repeat whenever you need to refresh the makefiles.

## How to build and test with scripts/build.py

Prerequisites: CMake 3.18+, a C compiler (clang or gcc), and the Rust toolchain (`rustup`).

For cross-language server testing (`servers`, `regtest`, `regtest-only` targets), also: JDK (`javac` + `java`) and .NET SDK (`dotnet`).

```
scripts/build.py                # Build the C library + all C tools (CMake)
scripts/build.py ta_regtest     # Build just the C test runner (CMake)
scripts/build.py ta_codegen     # Build the Rust codegen tool (cargo)
scripts/build.py generate       # Regenerate per-function source for all backends (cargo)
scripts/build.py servers        # Generate + compile JSON-RPC language servers (cargo)
```

Built binaries go to `bin/`. CMake is configured automatically on first run. The C
library + C tools build with CMake (no Rust needed); `ta_codegen` builds with cargo via
the targets above — CMake never invokes cargo.

To run tests:
```
scripts/build.py test           # C reference tests only (quick)
scripts/build.py regtest        # Full pipeline: servers + C tests + cross-language verification
scripts/build.py regtest-only   # Codegen verification only (skip C reference tests)
```

For more control, run `ta_regtest` directly from `bin/`:
```
./ta_regtest                                               # C reference tests only
./ta_regtest --codegen                                     # C tests + all-language codegen
./ta_regtest --codegen-only                                # Codegen only (all languages)
./ta_regtest --codegen --language=c,rust                   # Filter to specific languages
./ta_regtest --codegen --function=RSI,SMA                  # Filter to specific functions
```

## How to run ta_codegen

`ta_codegen` is the single code generator: it generates the C library (in place under `src/`), the Rust/Java/C# bindings, and the JSON-RPC test servers:

```
cd ta_codegen/generator
cargo run -- generate                            # Generate indicator code for all backends
cargo run -- generate --func=SMA --backend=rust  # Specific function + backend
cargo run -- generate-servers                    # Generate JSON-RPC servers
cargo run -- build                               # Compile servers
cargo run -- extract                             # Extract indicators from C source → YAML
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

(12) Monitor homebrew-core. The following formula will eventually be updated (may take an hour):
https://github.com/Homebrew/homebrew-core/blob/30106807361198c58a395de65547694427adf229/Formula/t/ta-lib.rb


## After a release

Right after a release is public, open the next development version so `dev`/`main`
stop advertising an already-released version:

(A) On dev, bump the VERSION file to the next patch (e.g. `0.7.2` -> `0.7.3`). You can adjust the actual versionlater, the key here is it just need to be different/higher.

(B) Add a `## [0.7.3] Not Released Yet` entry at the top of CHANGELOG.md.

(C) Run `./scripts/sync.py`, push dev, then merge to main as usual.

The website catches up on its own within a nightly cycle. Confirm with:

```bash
./scripts/sync-website.py --check   # non-zero if the website is behind
```


## I want to modify the code... should I care to rebuild the packages?
No.

Commit your source code changes in devs and... just let the Github action do all the repackaging for you.

It may take up to one day for the CI to regenerate and test for **all** platforms.

The rest of this section is only if you want to re-package locally.

You can call the ```scripts/package.py``` to generate the packages for your hosting platform.

You can test your packages with ```scripts/test-dist.py```. This verifies from a TA-Lib user perspective. Notably, this simulates a ta-lib-python user.

Try to avoid pushing your generated packages to the TA-Lib repo, but do not worry if you do. As needed, they will get overwritten by the "nightly dev" CI.
