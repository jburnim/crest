@echo off
echo Configuring Grep for DJGPP...
update djgpp/config.h ./config.h
sed -e /@VERSION@/d djgpp/main.mak > Makefile
sed -n -e /^VERSION/p configure >> Makefile
sed -e /@VERSION@/d djgpp/src.mak > src\Makefile
sed -n -e /^VERSION/p configure >> src\Makefile
sed -e /@VERSION@/d djgpp/tests.mak > tests\Makefile
sed -n -e /^VERSION/p configure >> tests\Makefile
sed -e /@VERSION@/d djgpp/intl.mak > intl\Makefile
sed -n -e /^VERSION/p configure >> intl\Makefile
sed -e /@VERSION@/d djgpp/po.mak > po\Makefile
sed -n -e /^VERSION/p configure >> po\Makefile
sed -e '/^#/d' ./intl/linux-msg.sed > intl\po2msg.sed
sed -e '/^#.*[^\\]$/d' -e '/^#$/d' djgpp/po2tbl.sed > intl\po2tbl.sed
if exist make.com ren make.com vmsmake.com
if exist config.h.in REN config.h.in config.h-in
if exist intl\po2tbl.sed.in REN intl\po2tbl.sed.in po2tbl-in.sed
if exist intl\Makefile.in.in REN intl\Makefile.in.in Makefile.in-in
echo Done.
