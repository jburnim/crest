CREST
=====

CREST is a concolic test generation tool for C.

Thanks for downloading and trying out CREST.

You can get the latest version of CREST, as well as news and
announcements at CREST's homepage: https://burn.im/crest .

If you want to cite CREST, please refer to the (short) paper: Burnim,
Sen, "Heuristics for Dynamic Test Generation", Proceedings of the 23rd
IEEE/ACM International Conference on Automated Software Engineering
(ASE), 2008.

You can find a list of papers using CREST at
https://burn.im/crest/#publications .

NOTE: CREST is no longer being actively developed, but questions are
still answered on the CREST-users mailing list --
crest-users@googlegroups.com and
https://groups.google.com/forum/#!forum/crest-users .

Preparing CIL
=====
Go to crest/cil diretory. You mush have OCaml and OCamlbuild installed properly. Then execute following commands:

$ ./configure

$ make

Remember that, every time you make changes to CIL, you need to execute 'make' command to reflect the changes.

Initial  Setup for CREST
=====
First, you need to setup Yices SMT Solver. PLease install version 1.0 from https://yices.csl.sri.com/old/download-yices1.html. For installation guideline follow https://yices.csl.sri.com/.

Then you need to change the location of Yices in crest/src/makefile

Now, go to crest/src directory and execute 'make' command. CREST is ready to use.

Preparing a Program for CREST
=====

To use CREST on a C program, use functions CREST_int, CREST_char,
etc., declared in "crest.h", to generate symbolic inputs for your
program.  For examples, see the programs in test/.

For simple, single-file programs, you can use the build script
"bin/crestc" to instrument and compile your test program.

CREST can be used to instrument multi-file programs, too --
instructions may be added later.  In the meantime, you can take a look
at an example, instrumented form of grep-2.2, available at
https://github.com/jburnim/crest/tree/master/benchmarks/grep-2.2 .
For further information, please see this
[post](https://groups.google.com/forum/#!topic/crest-users/KwgP9JkajOw)
on the CREST-users mailing list.


Running Crest
=====

CREST is run on an instrumented program as:

    bin/run_crest PROGRAM NUM_ITERATIONS -STRATEGY

Possibly strategies include: dfs, cfg, random, uniform_random, random_input.
Some strategies take optional parameters.

Example commands to test the "test/uniform_test.c" program:

    cd test
    ../bin/crestc uniform_test.c
    ../bin/run_crest ./uniform_test 10 -dfs

This should produce output roughly like:

    ... [GARBAGE] ...
    Read 8 branches.
    Read 13 nodes.
    Wrote 6 branch edges.

    Iteration 0 (0s): covered 0 branches [0 reach funs, 0 reach branches].
    Iteration 1 (0s): covered 1 branches [1 reach funs, 8 reach branches].
    Iteration 2 (0s): covered 3 branches [1 reach funs, 8 reach branches].
    Iteration 3 (0s): covered 5 branches [1 reach funs, 8 reach branches].
    Iteration 4 (0s): covered 7 branches [1 reach funs, 8 reach branches].
    GOAL!
    Iteration 5 (0s): covered 8 branches [1 reach funs, 8 reach branches].

NOTE: run_crest and crestc currently leave a lot of files lying
around, some of which are temporary and some of which must be kept.
In particular, "cfg_branches" and "branches" are output by the
instrumentation process and are needed to run run_crest, and run_crest
produces "coverage", a list of the ID's of all covered branches.


Setup
=====

CREST depends on Yices 1, an SMT solver tool and library available at
http://yices.csl.sri.com/old/download-yices1.shtml.  To build and run
CREST, you must download and install Yices *version 1* and change
YICES_DIR in src/Makefile to point to Yices location.

CREST uses CIL to instrument C programs for testing.  A modified
distribution of CIL is included in directory cil/.  To build CIL,
simply run "configure" and "make" in the cil/ directory.

Finally, CREST can be built by running "make" in the src/ directory.


License
=====

CREST is distributed under the revised BSD license.  See LICENSE for
details.

This distribution includes a modified version of CIL, a tool for
parsing, analyzing, and transforming C programs.  CIL is written by
George Necula, Scott McPeak, Wes Weimer, Ben Liblit, and Matt Harren.
It is also distributed under the revised BSD license.  See cil/LICENSE
for details.
