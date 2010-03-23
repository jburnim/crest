#
# Makefile for the Vim documentation on Unix
#
# If you get "don't know how to make scratch", first run make in the source
# directory.  Or remove the include below.

AWK = awk

# include the config.mk from the source directory.  It's only needed to set
# AWK, used for "make html".  Comment this out if the include gives problems.
include ../../src/config.mk

DOCS = \
	autocmd.txt \
	change.txt \
	cmdline.txt \
	develop.txt \
	digraph.txt \
	editing.txt \
	eval.txt \
	farsi.txt \
	gui.txt \
	gui_w32.txt \
	gui_x11.txt \
	hangulin.txt \
	help.txt \
	howto.txt \
	if_cscope.txt \
	if_ole.txt \
	if_perl.txt \
	if_python.txt \
	if_sniff.txt \
	if_tcl.txt \
	index.txt \
	insert.txt \
	intro.txt \
	map.txt \
	message.txt \
	motion.txt \
	multibyte.txt \
	options.txt \
	os_amiga.txt \
	os_beos.txt \
	os_dos.txt \
	os_mac.txt \
	os_mint.txt \
	os_msdos.txt \
	os_os2.txt \
	os_riscos.txt \
	os_unix.txt \
	os_vms.txt \
	os_win32.txt \
	pattern.txt \
	quickfix.txt \
	quotes.txt \
	recover.txt \
	repeat.txt \
	rightleft.txt \
	scroll.txt \
	starting.txt \
	syntax.txt \
	tagsearch.txt \
	term.txt \
	tips.txt \
	todo.txt \
	uganda.txt \
	undo.txt \
	various.txt \
	version4.txt \
	version5.txt \
	vi_diff.txt \
	visual.txt \
	windows.txt

HTMLS = \
	autocmd.html \
	change.html \
	cmdline.html \
	develop.html \
	digraph.html \
	editing.html \
	eval.html \
	farsi.html \
	gui.html \
	gui_w32.html \
	gui_x11.html \
	hangulin.html \
	help.html \
	howto.html \
	if_cscope.html \
	if_ole.html \
	if_perl.html \
	if_python.html \
	if_sniff.html \
	if_tcl.html \
	index.html \
	insert.html \
	intro.html \
	map.html \
	message.html \
	motion.html \
	multibyte.html \
	options.html \
	os_amiga.html \
	os_beos.html \
	os_dos.html \
	os_mac.html \
	os_mint.html \
	os_msdos.html \
	os_os2.html \
	os_riscos.html \
	os_unix.html \
	os_vms.html \
	os_win32.html \
	pattern.html \
	quickfix.html \
	quotes.html \
	recover.html \
	repeat.html \
	rightleft.html \
	scroll.html \
	starting.html \
	syntax.html \
	tags.html \
	tagsearch.html \
	term.html \
	tips.html \
	todo.html \
	uganda.html \
	undo.html \
	various.html \
	version4.html \
	version5.html \
	vi_diff.html \
	visual.html \
	windows.html

.SUFFIXES:
.SUFFIXES: .c .o .txt .html

all: tags vim.man vimtutor.man xxd.man ctags.man

tags: doctags $(DOCS)
	./doctags $(DOCS) | sort >tags
	uniq -d -2 tags

doctags: doctags.c
	$(CC) doctags.c -o doctags

vim.man: vim.1
	nroff -man vim.1 | sed -e s/.//g > vim.man

vimtutor.man: vimtutor.1
	nroff -man vimtutor.1 | sed -e s/.//g > vimtutor.man

xxd.man: xxd.1
	nroff -man xxd.1 | sed -e s/.//g > xxd.man

ctags.man: ctags.1
	nroff -man ctags.1 | sed -e s/.//g > ctags.man

ctags.1: ../../src/ctags/ctags.1
	cp ../../src/ctags/ctags.1 ctags.1

# Awk version of .txt to .html conversion.
html: noerrors tags tags.ref $(HTMLS)
	@if test -f errors.log; then more errors.log; fi

noerrors:
	-rm -f errors.log

.txt.html:
	$(AWK) -f makehtml.awk $< >$@

tags.ref tags.html: tags
	$(AWK) -f maketags.awk tags >tags.html

# Perl version of .txt to .html conversion.
# There can't be two rules to produce a .html from a .txt file.
# Just run over all .txt files each time one changes.  It's fast anyway.
perlhtml: tags $(DOCS)
	./vim2html.pl tags $(DOCS)

clean:
	-rm doctags *.html tags.ref

# These files are in the extra archive, skip if not present

farsi.txt:
	touch farsi.txt

gui_w32.txt:
	touch gui_w32.txt

if_ole.txt:
	touch if_ole.txt

os_amiga.txt:
	touch os_amiga.txt

os_beos.txt:
	touch os_beos.txt

os_dos.txt:
	touch os_dos.txt

os_mac.txt:
	touch os_mac.txt

os_mint.txt:
	touch os_mint.txt

os_msdos.txt:
	touch os_msdos.txt

os_os2.txt:
	touch os_os2.txt

os_riscos.txt:
	touch os_riscos.txt

os_win32.txt:
	touch os_win32.txt
