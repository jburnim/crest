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
top_srcdir = .
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

top_builddir = .

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
AWK = gawk
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

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = intl po src tests

EXTRA_DIST = TODO make.com
ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = config.h
CONFIG_CLEAN_FILES = 
DIST_COMMON =  README ABOUT-NLS AUTHORS COPYING ChangeLog INSTALL \
Makefile.am Makefile.in NEWS THANKS TODO acconfig.h aclocal.m4 \
config.guess config.h-in configure configure.in install-sh missing \
mkinstalldirs stamp-h.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
default: all

.SUFFIXES:
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps Makefile

config.h: stamp-h
stamp-h: $(srcdir)/config.h-in
	@echo timestamp > stamp-h

$(srcdir)/config.h-in: $(srcdir)/stamp-h.in

mostlyclean-hdr:

clean-hdr:

distclean-hdr:
	-rm -f config.h

maintainer-clean-hdr:

# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# pass the desired values on the `make' command line.


all info dvi install:
	@echo "Making $@ in src"
	cd src ; $(MAKE) $@ ; cd ..

check:
	@echo "Making $@ in tests"
	cd tests ; $(MAKE) $@ ; cd ..

clean-sub:
	@echo "Making ${target} in src"
	cd src ; $(MAKE) ${target} ; cd ..
	@echo "Making ${target} in tests"
	cd tests ; $(MAKE) ${target} ; cd ..
	@echo "Making ${target} in po"
	cd po ; $(MAKE) ${target} ; cd ..
	@echo "Making ${target} in intl"
	cd intl ; $(MAKE) ${target} ; cd ..

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	$(MAKE) clean-sub target=$(shell echo $@ | sed s/-recursive//)

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	cd src ; $(MAKE) $@ ; cd ..

TAGS: $(HEADERS) $(SOURCES) config.h-in $(TAGS_DEPENDENCIES)
	cd src ; $(MAKE) $@ ; cd ..

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install

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
mostlyclean-am:  mostlyclean-hdr mostlyclean-tags mostlyclean-generic

clean-am:  clean-hdr clean-tags clean-generic mostlyclean-am

distclean-am:  distclean-hdr distclean-tags distclean-generic clean-am

maintainer-clean-am:  maintainer-clean-hdr maintainer-clean-tags \
		maintainer-clean-generic distclean-am

mostlyclean:  mostlyclean-recursive mostlyclean-am

clean:  clean-recursive clean-am

distclean:  distclean-recursive distclean-am
	-rm -f config.status

maintainer-clean:  maintainer-clean-recursive maintainer-clean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	-rm -f config.status

.PHONY: default mostlyclean-hdr distclean-hdr clean-hdr \
maintainer-clean-hdr install-data-recursive uninstall-data-recursive \
install-exec-recursive uninstall-exec-recursive installdirs-recursive \
uninstalldirs-recursive all-recursive check-recursive \
installcheck-recursive info-recursive dvi-recursive \
mostlyclean-recursive distclean-recursive clean-recursive \
maintainer-clean-recursive tags tags-recursive mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir info dvi \
installcheck all-recursive-am all-am install-exec install-data install \
uninstall all installdirs mostlyclean-generic distclean-generic \
clean-generic maintainer-clean-generic clean mostlyclean distclean \
maintainer-clean \
clean-sub


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
