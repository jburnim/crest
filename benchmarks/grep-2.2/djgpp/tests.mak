# Generated from Makefile.in for DJGPP v2
# Makefile.in generated automatically by automake 1.2h from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = ${DJDIR}
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DISTDIR =

pkgdatadir = $(datadir)/grep
pkglibdir = $(libdir)/grep
pkgincludedir = $(includedir)/grep

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = ${DJDIR}/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = true
PRE_INSTALL = true
POST_INSTALL = true
NORMAL_UNINSTALL = true
PRE_UNINSTALL = true
POST_UNINSTALL = true
CATALOGS = 
CATOBJEXT = 
CC = gcc
DATADIRNAME = share
GENCAT = 
GMOFILES =  de.gmo es.gmo fr.gmo ko.gmo nl.gmo no.gmo pl.gmo sl.gmo sv.gmo
GMSGFMT = 
GT_NO = 
GT_YES = #YES#
INCLUDE_LOCALE_H = "#include <locale.h>"
INSTOBJEXT = 
INTLDEPS = 
INTLLIBS = 
INTLOBJS = 
MAKEINFO = makeinfo
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = 
PACKAGE = grep
POFILES =  de.po es.po fr.po ko.po nl.po no.po pl.po sl.po sv.po
POSUB = 
RANLIB = ranlib
USE_INCLUDED_LIBINTL = no
USE_NLS = no
VERSION = @VERSION@
l = 

AWK=gawk

TESTS = warning.sh khadafy.sh spencer1.sh spencer2.sh status.sh empty.sh
EXTRA_DIST = $(TESTS) khadafy.lines khadafy.regexp \
             scriptgen.awk spencer1.tests spencer2.tests
CLEANFILES = tmp1.script tmp2.script khadafy.out
TESTS_ENVIRONMENT = GREP=$(top_builddir)/src/grep AWK=$(AWK)
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best

ifeq '$(basename $(notdir $(SHELL)))' 'sh'
CHECK_SHELL = @true
else
CHECK_SHELL = Grep-tests-require-Unix-like-shell-to-run
endif

all: Makefile

.SUFFIXES:
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps tests/Makefile

tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = tests


distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
check-TESTS: $(TESTS)
	@failed=0; all=0; \
	srcdir=$(srcdir); export srcdir; \
	for tst in $(TESTS); do \
	  if test -f $$tst; then dir=.; \
	  else dir="$(srcdir)"; fi; \
	  if $(TESTS_ENVIRONMENT) $$dir/$$tst; then \
	    all=`expr $$all + 1`; \
	    echo "PASS: $$tst"; \
	  elif test $$? -ne 77; then \
	    all=`expr $$all + 1`; \
	    failed=`expr $$failed + 1`; \
	    echo "FAIL: $$tst"; \
	  fi; \
	done; \
	if test "$$failed" -eq 0; then \
	  banner="All $$all tests passed"; \
	else \
	  banner="$$failed of $$all tests failed"; \
	fi; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"; \
	test "$$failed" -eq 0
info:
dvi:
check: all
	$(CHECK_SHELL)
	@utod $(srcdir)/khadafy.lines
	$(MAKE) check-TESTS
installcheck:
install-exec: 
	@$(NORMAL_INSTALL)

install-data: 
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: 

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-generic

clean:  clean-generic mostlyclean

distclean:  distclean-generic clean
	-rm -f config.status

maintainer-clean:  maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: tags distdir check-TESTS info dvi installcheck \
install-exec install-data install uninstall all installdirs \
mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
