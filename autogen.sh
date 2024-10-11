#!/bin/sh
echo aclocal
aclocal || exit
echo autoheader
autoheader || exit
if [[ `uname -o` -eq 'Darwin' ]]; then
    echo glibtoolize --copy --force
    glibtoolize --copy --force || exit
else
    echo libtoolize --copy --force
    libtoolize --copy --force || exit
fi
echo automake -a -c
automake -a -c || exit
echo autoconf
autoconf || exit
