#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_COPYRIGHT([(c) Mario Fortier 1999-2026])
AC_INIT([ta-lib], m4_esyscmd([cat VERSION | tr -d '\n']), [https://github.com/TA-Lib/ta-lib/issues], [ta-lib], [https://github.com/TA-Lib/ta-lib])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/ta_func/ta_AD.c])
AC_CONFIG_HEADERS([include/ta_config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

# Checks for programs.
AM_PROG_AR
AC_PROG_CC
LT_INIT

# FMA numerical contract (PR #96) — mirrors CMakeLists.txt: -ffp-contract=off makes
# explicit fma() the only fusion (no auto-contraction of the un-fused a*b+c), for
# cross-clone/-language determinism. Added only if the compiler accepts it.
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -ffp-contract=off"
AC_MSG_CHECKING([whether $CC accepts -ffp-contract=off])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no]); CFLAGS="$save_CFLAGS"])

# -fno-math-errno (issue #192) — mirrors CMakeLists.txt: lets sqrt() compile to
# the bare instruction and vectorize, instead of being pinned scalar by the ISO C
# requirement to set errno on a domain error. The library never reads errno. It
# cannot change a computed VALUE, but it does change FP exception state — see the
# FE_INVALID note in CMakeLists.txt before assuming it is inert.
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fno-math-errno"
AC_MSG_CHECKING([whether $CC accepts -fno-math-errno])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no]); CFLAGS="$save_CFLAGS"])

# Symbol visibility — mirrors CMakeLists.txt's C_VISIBILITY_PRESET. Without it
# every non-static symbol lands in the shared library's dynamic table, so
# TA_LIB_API is inert on ELF and a prototype that forgets it is caught only by a
# Windows build. With it the attribute means the same thing on every platform.
# Probed, so a compiler that rejects it still configures -- and check_abi's
# export gate fails loudly rather than the surface silently widening.
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"
AC_MSG_CHECKING([whether $CC accepts -fvisibility=hidden])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no]); CFLAGS="$save_CFLAGS"])

# The companion linker map (src/libta-lib.map) localizes the target_clones
# resolvers -fvisibility=hidden cannot reach. Probed the same way; Darwin has no
# --version-script and needs no map, since target_clones is glibc-only.
TA_VERSION_SCRIPT_FLAGS=""
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -Wl,--version-script=$srcdir/src/libta-lib.map -Wl,--no-undefined-version"
AC_MSG_CHECKING([whether $CC accepts -Wl,--version-script])
AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])],
  [AC_MSG_RESULT([yes])
   TA_VERSION_SCRIPT_FLAGS='-Wl,--version-script=$(top_srcdir)/src/libta-lib.map -Wl,--no-undefined-version'],
  [AC_MSG_RESULT([no])])
LDFLAGS="$save_LDFLAGS"
AC_SUBST([TA_VERSION_SCRIPT_FLAGS])

# Warnings — mirrors CMakeLists.txt, including why -Wunused-parameter and
# -Wunknown-pragmas are off (uniform OHLCV signature; the deliberate
# FP_CONTRACT pragma GCC ignores). Probed, so a compiler that rejects them
# still configures. Not -Werror: the dist build runs on whatever the packager
# has.
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas"
AC_MSG_CHECKING([whether $CC accepts -Wall -Wextra])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no]); CFLAGS="$save_CFLAGS"])

# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS([float.h inttypes.h limits.h locale.h stddef.h stdint.h stdlib.h string.h unistd.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_STRCOLL
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor isascii localeconv mblen memmove memset modf pow sqrt strcasecmp strchr strerror strncasecmp strrchr strstr strtol strtoul])

# Checks for libm
LT_LIB_M
AC_SUBST([LIBM])

# The library's ABI generation, as libtool current:revision:age. NOT the release
# version -- the soname libtool derives from it is `current - age`, and it must
# change only when the ABI does. Both build systems read this line; CMake parses
# it out of this file so the two cannot drift apart.
#
#   interfaces added, none removed/changed -> c+1 : 0   : a+1   (soname unchanged)
#   any interface removed or changed       -> c+1 : 0   : 0     (soname +1)
#   implementation only                    -> c   : r+1 : a     (soname unchanged)
#
# 2:0:1 -- 0.8.1 removes the 162 TA_*_Unguarded/TA_EMA_Private entry points and
# drops TA_FuncInfo.camelCaseName, so it is not the 0.7.1 ABI that shipped as
# libta-lib.so.0. The `:1` age is #400 re-exporting TA_Set/GetCompatibility with
# TA_LIB_API: an addition, so the soname stays libta-lib.so.1. #400 also stops
# exporting 1,516 internals, which moves no interface anyone was promised and no
# line in ABI.manifest -- and libta-lib.so.1 has not shipped, so there is nothing
# for it to break compatibility WITH.
#
# 3:0:2 -- TA_RVIR and its streaming entry points are added and nothing is
# removed or changed, so the soname stays libta-lib.so.1.
TALIB_LIBRARY_VERSION=3:0:2

AC_SUBST(TALIB_LIBRARY_VERSION)

AC_CONFIG_FILES([Makefile src/Makefile src/ta_abstract/Makefile src/ta_common/Makefile src/ta_func/Makefile src/tools/Makefile src/tools/ta_regtest/Makefile ta-lib.pc ta-lib.spec ta-lib.dpkg])
AC_OUTPUT
