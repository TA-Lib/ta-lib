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

## How to build with CMakeLists.txt
```
$ cd ta-lib
$ mkdir build
$ cd build
$ cmake ..
$ make
```
Libraries will be in ```ta-lib/build``` and executable in ```ta-lib/bin```

## How to run gen_code
After ```make```, call ```gen_code``` located in ta-lib/bin

Do this to refresh many files and code variant (Rust, Java etc...)

You should call ```make`` again after gen_code to verify if the
potentially updated C code is still compiling.


## How to run ta_regtest
After ```make```, call ```ta_regtest``` located in ta-lib/src/tools

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

(8) Optionally edit the draft "Release notes" on the Github website and verify that everything looks fine.

(9) Manually trig "Release (step 2)" Github action. This will make the release public, update the website and create a PR on homebrew-core.

(10) Verify that https://ta-lib.org/install reflects the new version.

(11) Monitor that the PR on homebrew-core is working as expected. The following formula will eventually be updated (may take an hour):
https://github.com/Homebrew/homebrew-core/blob/30106807361198c58a395de65547694427adf229/Formula/t/ta-lib.rb

## I want to modify the code... should I care to rebuild the packages?
No.

Commit your source code changes in devs and... just let the Github action do all the repackaging for you.

It may take up to one day for the CI to regenerate and test for **all** platforms.

The rest of this section is only if you want to re-package locally.

You can call the ```scripts/package.py``` to generate the packages for your hosting platform.

You can test your packages with ```scripts/test-dist.py```. This verifies from a TA-Lib user perspective. Notably, this simulates a ta-lib-python user.

Try to avoid pushing your generated packages to the TA-Lib repo, but do not worry if you do. As needed, they will get overwritten by the "nightly dev" CI.
