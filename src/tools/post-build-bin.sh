#!/bin/sh

# This is a work around for a slibtool bug with --mode=execute
#
# With slibtool the gen_code binary is created in the .libs directory while GNU
# libtool outputs in the same directory as the Makefile. This means that cp(1)
# needs to be invoked with $(LIBTOOL) --mode=execute.
#
# However slibtool currently has a bug where the destination argument is dropped
# which will result in the command failing.
#
# See https://bugs.gentoo.org/790770

set -eu

mkdir -p ../../../bin
cp "${1}" ../../../bin
