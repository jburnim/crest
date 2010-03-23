#
# Makefile to run al tests for Vim
#

# Note: If tests 2, 15 and 26 fail, you probably have 'nocompatible' set.

VIMPROG = ../vim

SCRIPTS = test1.out test2.out test3.out test4.out test5.out test6.out \
		test7.out test8.out test9.out test10.out test11.out \
		test12.out  test13.out test14.out test15.out test17.out \
		test18.out test19.out test20.out test21.out test22.out \
		test23.out test24.out test25.out test26.out test27.out \
		test28.out test29.out test30.out test31.out test32.out \
		test33.out test34.out test35.out test36.out test37.out \
		test38.out test39.out

SCRIPTS_GUI = test16.out

.SUFFIXES: .in .out

nongui:	nolog $(SCRIPTS)
	@echo
	@cat test.log
	@echo ALL DONE

gui:	nolog $(SCRIPTS) $(SCRIPTS_GUI)
	@echo
	@cat test.log
	@echo ALL DONE

$(SCRIPTS) $(SCRIPTS_GUI): $(VIMPROG)

clean:
	-rm -rf *.out test.log

.in.out:
	-rm -f $*.failed
	$(VIMPROG) -u vimrc.unix -U NONE -s dotest.in $*.in
	/bin/sh -c "if diff test.out $*.ok; \
		then mv -f test.out $*.out; \
		else echo $* FAILED >>test.log; mv -f test.out $*.failed; \
		fi"
	-rm -f Xdotest

nolog:
	-echo Test results: >test.log
