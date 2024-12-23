ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign 1.4
SUBDIRS = src src/tools

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ta-lib.pc

# Distribute all .h files in the src directory and its subdirectories
HEADER_FILES = $(shell find src -name '*.h')

# Distribute some C files containing static data used for ta_regtest
GDATA_FILES = $(shell find . -name 'ta_gData*.c')

# Distribute additional bash scripts needed to "make all"
BUILD_SHELL_FILES = src/tools/post-build-bin.sh

# Add more files to EXTRA_DIST to be included on "make dist"
# This is only for files that are not auto-discovered by autotools.
EXTRA_DIST = $(HEADER_FILES) $(GDATA_FILES) $(BUILD_SHELL_FILES) VERSION
