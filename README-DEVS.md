# Instructions for TA-Lib maintainers
**You do not need to do any of this if you are only a TA-Lib user.**

Development is done on Ubuntu, but the library can still be used on any Linux distribution, Windows and macOS.

## How to generate "./configure" script
Install dependencies:
```$ sudo apt install automake libtool autogen```

Create/update all the "Makefile" related files:
```$ autoreconf -i```

## How to build gen_code
Install dependencies:
```$ sudo apt install mcpp```

Build everything (includes gen_code):
```
$ cd ta-lib
$ ./configure
$ make
```

Must be in a specific directory to run it:
```
$ cd ta-lib/bin
$ ./gen_code
```
