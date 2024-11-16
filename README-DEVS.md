# Instructions for TA-Lib maintainers
**If you only want to install and use TA-Lib, there is nothing here for you... check instead the main README.md file.**

Development is done on Ubuntu, but the library can still be used on any Linux distribution, Windows and macOS.

For running most scripts, you must have python installed.

## How to push changes to Github?
For most, PR and direct push are to be made on the 'dev' branch.

Merge to main branch can be done only by members of the TA-Lib organization.

## How to update the "./configure" script
As needed, install dependencies:
```$ sudo apt install automake libtool autogen```

Create/update all the "Makefile" files:
```$ autoreconf -i```

Do this after git cloning/pulling, or whenever a new file need to be compiled.

## How to build
As needed, install dependencies:
```$ sudo apt install mcpp```

Build everything (includes gen_code and ta_regtest):
```
$ cd ta-lib
$ ./configure
$ make
```

## How to run gen_code
After ```make```, call ```gen_code``` located in ta-lib/src/tools

Do this to refresh many files and code variant (Rust, Java etc...)

You should call ```make`` again after gen_code to verify if the
potentially updated C code is still compiling.


## How to run ta_regtest
After ```make```, call ```ta_regtest``` located in ta-lib/src/tools

Exit code is 0 on success


## How to package the library
Call ```package.py``` located in ta-lib/scripts

The produced packages are written in ta-lib/dist
Only the packages supported by your host are created.

The content of ta-lib/dist should be committed to Github, both for user
convenience and also to facilitate GitHub CI.


## How to test the packaged library
Call ```run_tests.py``` located in ta-lib/scripts

Exit code is 0 on success

The tests are performed only on the assets in ta-lib/dist

Recommended being done before a push to Github.

This script is also run by various Github actions (CI).


## Code Flow
C IDL (interface, function signature) -> gen_code -> C Function (complicated) -> gen_code -> Java

