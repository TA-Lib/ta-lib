# Instructions for TA-Lib maintainers
**You do not need to do any of this if you are only a TA-Lib user.**

# How to generate "./configure" script
Dependencies to install:
- automake
- libtool
- autogen

On ubuntu, just do the following:
```sudo apt install automake libtool autogen```

... then execute:
```
$ mkdir -p m4
$ autoreconf -i
```

# How to generate the .tar.gz
```
$ ./configure
$ make dist
```