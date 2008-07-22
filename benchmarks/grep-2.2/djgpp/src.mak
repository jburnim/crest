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
mandir = ${prefix}/info
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

AUTOMAKE_OPTIONS=no-dependencies

LN = ln

bin_PROGRAMS = grep egrep fgrep
grep_SOURCES = grep.c grep.h \
               dfa.c dfa.h \
               kwset.c  kwset.h \
               obstack.c obstack.h \
               getopt.c getopt1.c getopt.h \
               search.c getpagesize.h system.h
egrep_SOURCES = 
fgrep_SOURCES = 
LDADD =   regex.o 
localedir = $(prefix)/share/locale
INCLUDES = -I../intl -DLOCALEDIR=\"$(localedir)\"
man_MANS = grep.1 fgrep.1 egrep.1
EXTRA_DIST = grep.1 egrep.man fgrep.man \
             regex.c regex.h
CLEANFILES = egrep.1 fgrep.1
mkinstalldirs = ${DJDIR}/bin/gmkdir -p
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS = 
LDFLAGS = 
LIBS = 
grep_OBJECTS =  grep.o dfa.o kwset.o obstack.o getopt.o getopt1.o \
search.o
grep_LDADD = $(LDADD)
grep_DEPENDENCIES =    regex.o
grep_LDFLAGS = 
egrep_OBJECTS = 
egrep_LDADD = $(LDADD)
egrep_DEPENDENCIES =   grep
egrep_LDFLAGS = 
fgrep_OBJECTS = 
fgrep_LDADD = $(LDADD)
fgrep_DEPENDENCIES =   grep
fgrep_LDFLAGS = 
CFLAGS = -g -O2
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS) -o $@
man1dir = $(mandir)/man1
MANS = $(man_MANS)

NROFF = groff -s -man
DIST_COMMON =  Makefile.am Makefile.in alloca.c btowc.c memchr.c regex.c


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
SOURCES = $(grep_SOURCES) $(egrep_SOURCES) $(fgrep_SOURCES)
OBJECTS = $(grep_OBJECTS) $(egrep_OBJECTS) $(fgrep_OBJECTS)

all: Makefile $(PROGRAMS) $(MANS)

.SUFFIXES:
.SUFFIXES: .S .c .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps src/Makefile


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) grep $(DESTDIR)$(bindir)/grep
	$(INSTALL_PROGRAM) egrep.exe $(DESTDIR)$(bindir)/egrep.exe
	$(INSTALL_PROGRAM) fgrep.exe $(DESTDIR)$(bindir)/fgrep.exe

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

grep: $(grep_OBJECTS) $(grep_DEPENDENCIES)
	$(LINK) $(grep_LDFLAGS) $(grep_OBJECTS) $(grep_LDADD) $(LIBS)

egrep: $(egrep_OBJECTS) $(egrep_DEPENDENCIES)
	stubify -g egrep
	stubedit egrep.exe runfile=grep
	command.com /c copy egrep.exe egrep > NUL

fgrep: $(fgrep_OBJECTS) $(fgrep_DEPENDENCIES)
	stubify -g fgrep
	stubedit fgrep.exe runfile=grep
	command.com /c copy fgrep.exe fgrep > NUL

$(OBJECTS): ../config.h
grep.o kwset.o search.o: system.h
getopt.o getopt1.o grep.o: getopt.h
grep.o search.o: grep.h
kwset.o search.o: kwset.h
kwset.o obstack.o: obstack.h
dfa.o regex.o search.o: regex.h
grep.o: dosbuf.c getpagesize.h

install-man: $(MANS)
	$(mkinstalldirs) $(DESTDIR)$(mandir)
	$(NROFF) grep.1 > grep.1n
	$(NROFF) egrep.1 > egrep.1n
	$(NROFF) fgrep.1 > fgrep.1n
	$(INSTALL_DATA) grep.1n $(mandir)/grep.1
	$(INSTALL_DATA) egrep.1n $(mandir)/egrep.1
	$(INSTALL_DATA) fgrep.1n $(mandir)/fgrep.1
	-rm -f grep.1n [fe]grep.1n


tags: TAGS

ID: $(HEADERS) $(SOURCES)
	mkid -f./ID $(SOURCES) $(HEADERS)

# The call to $(sort) avoids using a UNix-like shell.  I don't want to
# require Bash or its ilk for something as simple as generating TAGS.
TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	etags $(ETAGS_ARGS) $$tags  $(sort $(SOURCES) $(HEADERS))

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS

install-data: install-man

install: install-exec install-data all

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(bindir) $(DESTDIR)$(mandir)


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
mostlyclean:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

clean:  clean-binPROGRAMS clean-compile clean-tags clean-generic \
		mostlyclean

distclean:  distclean-binPROGRAMS distclean-compile distclean-tags \
		distclean-generic clean
	-rm -f config.status

maintainer-clean:  maintainer-clean-binPROGRAMS maintainer-clean-compile \
		maintainer-clean-tags maintainer-clean-generic \
		distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile install-man uninstall-man tags \
mostlyclean-tags distclean-tags clean-tags maintainer-clean-tags \
distdir info dvi installcheck install-exec install-data install \
uninstall all installdirs mostlyclean-generic distclean-generic \
clean-generic maintainer-clean-generic clean mostlyclean distclean \
maintainer-clean


fgrep.1: fgrep.man
	sed -e "s%man1/@grep@%grep.1%g" $(srcdir)/fgrep.man > $@

egrep.1: egrep.man
	sed -e "s%man1/@grep@%grep.1%g" $(srcdir)/egrep.man > $@

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
