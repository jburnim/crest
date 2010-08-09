
To compile Vim 5.7 for use with CREST:

(1) cd src

(2) ./configure --disable-gui --disable-gpm --without-x

(3) make crest



To run CREST:

    run_crest './vim -m -n -Z -i NONE -u NONE' ITERATIONS -STRATEGY 2> vim.log

NOTES:

 - Although unlikely, even with these command-line options it is
   possible for CREST to generate inputs that cause Vim to execute
   shell commands (e.g., 'rm -r ~').  Please exercise caution.

 - Vim appears to changes the way it behaves (i.e. how it reads input
   from the user) based on the capabilities of the terminal in which
   it is run.  To get the best results, execute the above 'run_crest'
   command inside a "virtual terminal" using the program 'screen'.

   (This can be accomplished by executing the command "screen" right
   before executing "run_crest ...".  Type "exit" after 'run_crest'
   completes to quit 'screen'.)

 - On some executions, CREST may report a number of failed assertions,
   beginning with:

       vim: base/symbolic_interpreter.cc:90: void
           crest::SymbolicInterpreter::Store(crest::id_t, crest::addr_t):
           Assertion `stack_.size() > 0' failed.

    This indicates that, during the concolic execution, Vim 5.7 called
    into uninstrumented code, which then called back into instrumented
    code.  CREST is not able to handle such behavior.  Any functions
    that can be called form uninstrumented code must be marked with
    __attribute((crest_skip)) so that CREST does not instrument them.
    (We have already marked functions out_char_nf, out_flush,
    ui_write, mch_write, and RealWaitForChar.)

    We are working on a better solution to this issue.
