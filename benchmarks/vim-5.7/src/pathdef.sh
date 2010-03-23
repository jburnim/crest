#! /bin/sh
#
# pathdef.sh: adjust pathdef.c for link.sed, if it exists
#
if test -s link.sed; then
  cp pathdef.c pathdef.tmp
  sed -f link.sed <pathdef.tmp >pathdef.c
  rm -f pathdef.tmp
fi

# vim:set sw=2 et:
