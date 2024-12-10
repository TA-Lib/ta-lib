# Instructions for TA-Lib maintainers
**If you only want to install and use TA-Lib, there is nothing here for you... check instead the main README.md file.**

You must have python installed.

## How to push changes to Github?
Modifications (or PR) must be made on the 'dev' branch.

Before committing, run "scripts/sync.py" to:
 - Ensure your local dev branch is up-to-date with both remote dev and main branches.
 - Do various check and fixes on your code (e.g. update "x.y.z" versioning in various files).

Merge to main branch are done with "scripts/merge.py" by TA-Lib maintainers with the proper permissions.

## How to build with autotools
As needed, install dependencies:
```$ sudo apt install automake libtool autogen mcpp```

Build everything (includes gen_code and ta_regtest):
```
$ cd ta-lib
$ autoreconf -fi
$ ./configure
$ make
```
## How to update the "./configure" script
This will do all the needed autotools steps:
```$ autoreconf -fi```

Repeat whenever you need to refresh the makefiles.

## How to build with CMakeLists.txt
```
$ cd ta-lib
$ mkdir build
$ cd build
$ cmake ..
$ make
```
Libraries will in ```ta-lib/build``` and executable in ```ta-lib/bin```

## How to run gen_code
After ```make```, call ```gen_code``` located in ta-lib/bin

Do this to refresh many files and code variant (Rust, Java etc...)

You should call ```make`` again after gen_code to verify if the
potentially updated C code is still compiling.


## How to run ta_regtest
After ```make```, call ```ta_regtest``` located in ta-lib/src/tools

Exit code is 0 on success


## How to package the library
Call ```package.py``` located in ta-lib/scripts.

The produced packages are written in ta-lib/dist.
Only the packages supported by your host are created.

Test your local packages with ```test-dist.py```. This verifies from a TA-Lib user perspective. Notably, this simulates a ta-lib-python user.

You can push your generated package on the dev branch without worrying. The "nightly dev" Github action rebuilds, tests and replaces all packages as needed.

## How to do a new release?

Any dev with permission to merge to main branch can do a release.

(1) On the dev branch, edit the VERSION file in the root of the repos.

(2) Run "./scripts/sync.py". This ensures your dev branch is up-to-date (among other things).

(3) Run "./scripts/package.py" (good enough to package only on your favorite dev platform).

(4) Push to the dev branch.

(5) Wait up to one day for the "nightly dev" Github action to succeed. This will regenerate and test **all** the packages for **all** platforms. As needed, you can instead manually trig it.

(6) Merge dev into main with "./scripts/merge.py". At this point, the main branch is the release candidate with all the assets under "dist" folder.

(7) Wait up to one day for the "nightly main" Github action to succeed. This will perform a last check that all assets are OK. As needed, you can instead manually trig it.

(8) Manually trig the "publish-step-one" Github action. This will create the tag, draft release and attach all assets.

(9) Edit the Release notes on the Github website.

(10) Manually trig "publish-step-two" Github action. This will make the release public.

## How to test the packaged library
Call ```run_tests.py``` located in ta-lib/scripts

Exit code is 0 on success

The tests are performed only on the assets in ta-lib/dist

Recommended being done before a push to Github.

This script is also run by various Github actions (CI).


## Code Flow
C IDL (interface, function signature) -> gen_code -> C Function (complicated) -> gen_code -> Java

