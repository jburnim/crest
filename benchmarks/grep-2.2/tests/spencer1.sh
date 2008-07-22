#! /bin/sh
# Regression test for GNU grep.

: ${srcdir=.}

failures=0

# . . . and the following by Henry Spencer.

${AWK-awk} -f $srcdir/scriptgen.awk $srcdir/spencer1.tests > tmp1.script

sh tmp1.script && exit $failures
exit 1
