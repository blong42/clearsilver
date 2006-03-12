#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.


aclocal -I m4
autoheader
autoconf
./configure $*
