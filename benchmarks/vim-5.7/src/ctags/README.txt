Exuberant Ctags
===============
Author: Darren Hiebert (darren@hiebert.com, http://darren.hiebert.com)

This is a reimplementation of the much underused ctags(1) program and is
intended to be the mother of all ctags programs. I was motivated to write
this because no currently available ctags program supported generation of
tags for all possible tag candidates, and because most were easily fooled
by a number of contruct


What makes this ctags desirable?

1.  It supports C, C++, Eiffel, Fortran, and Java.

2.  It is capable of generating tags for all of the following language
    constructs:

        C/C++:
            macros
            enumerators
            function definitions, prototypes and declarations
            class, enum, struct, and union names
            class, struct, and union members
            namespaces
            typedefs
            variable definitions and extern declarations

        Eiffel:
            classes
            features
            local entities

        Fortran
            block data
            common blocks
            entry points
            functions
            interfaces
            lables
            modules
            namelists
            programs
            subroutines
            derived types

        Java:
            classes
            fields
            interfaces
            methods
            packages

3.  It is very robust in parsing code and is far less easily fooled by code
    containing #if preprocessor conditional constructs, using a conditional
    path selection algorithm to resolve complicated choices, and a fall-back
    algorithm when this one fails.

4.  Can also be used to print out a human-readable list of selected objects
    found in source files.

5.  Supports output of Emacs-style TAGS files ("etags").

6.  Supports UNIX, MSDOS, Windows 95/NT, OS/2, QNX, Amiga, QDOS, VMS, and
    Cray. Some pre-compiled binaries are available on the web site.


You can find Exuberant Ctags at the following locations:

    http://darren.hiebert.com/ctags/index.html   (Official web site)
    ftp://sunsite.unc.edu/pub/Linux/devel/lang/c/


Which brings us to the most frequently asked question:

  Q: Why is it called "Exuberant" ctags?
  A: Because one of the meanings of the word is:

     exuberant : produced in extreme abundance : PLENTIFUL syn see PROFUSE

Compare the tag file produced by Exuberant Ctags with that produced by any
other ctags and you will see how appropriate the name is.


This source code is distributed according to the terms of the GNU General
Public License. It is provided on an as-is basis and no responsibility is
accepted for its failure to perform as expected. It is worth at least as
much as you paid for it!

Exuberant Ctags was originally derived from and inspired by the ctags
program by Steve Kirkendall (kirkenda@cs.pdx.edu) that comes with the Elvis
vi clone (though almost none of the original code remains). This, too, is
freely available.

Please report any problems you find. The two problems I expect to be most
likely are either a tag which you expected but is missing, or a tag created
in error (shouldn't really be a tag). Please include a sample of code (the
definition) for the object which misbehaves.

--
vim:tw=76:sw=4:et:
