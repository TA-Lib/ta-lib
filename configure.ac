#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_COPYRIGHT([(c) Mario Fortier 1999-2024])
AC_INIT([ta-lib],[git],[https://github.com/TA-Lib/ta-lib])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/ta_func/ta_AD.c])
AC_CONFIG_HEADERS([include/ta_config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

# Checks for programs.
AM_PROG_AR
AC_PROG_CC
LT_INIT

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

# Versioning:
# Only change this if library is no longer
# ABI compatible with previous version
# (e.g. function declaration changed)
TALIB_LIBRARY_VERSION=0:0:0

AC_SUBST(TALIB_LIBRARY_VERSION)

AC_CONFIG_FILES([Makefile src/Makefile src/ta_abstract/Makefile src/ta_common/Makefile src/ta_func/Makefile src/tools/Makefile src/tools/gen_code/Makefile src/tools/ta_regtest/Makefile ta-lib.pc ta-lib.spec ta-lib.dpkg])
AC_OUTPUT
