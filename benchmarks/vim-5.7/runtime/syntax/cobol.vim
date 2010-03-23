" Vim syntax file
" Language:	COBOL
" Maintainers:	Sitaram Chamarty <sitaram@dimensional.com> and
"		James Mitchell <james_mitchell@acm.org>
" Last Change:	1999 December 21

" MOST important - else most of the keywords wont work!
set isk=@,48-57,-
" set up other basic parameters
syn clear
syn case ignore

syn keyword cobolReserved contained ACCEPT ACCESS ADD ADDRESS ADVANCING AFTER ALPHABET ALPHABETIC
syn keyword cobolReserved ALPHABETIC-LOWER ALPHABETIC-UPPER ALPHANUMERIC ALPHANUMERIC-EDITED ALS
syn keyword cobolReserved ALTERNATE AND ANY ARE AREA AREAS ASCENDING ASSIGN AT AUTHOR BEFORE BINARY
syn keyword cobolReserved BLANK BLOCK BOTTOM BY CANCEL CBLL CD CF CH CHARACTER CHARACTERS CLASS
syn keyword cobolReserved CLOCK-UNITS CLOSE COBOL CODE CODE-SET COLLATING COLUMN COMMA COMMON
syn keyword cobolReserved COMMUNICATIONS COMPUTATIONAL COMPUTE CONFIGURATION CONTENT CONTINUE
syn keyword cobolReserved CONTROL CONVERTING CORR CORRESPONDING COUNT CURRENCY DATA DATE DATE-COMPILED
syn keyword cobolReserved DATE-WRITTEN DAY DAY-OF-WEEK DE DEBUG-CONTENTS DEBUG-ITEM DEBUG-LINE
syn keyword cobolReserved DEBUG-NAME DEBUG-SUB-1 DEBUG-SUB-2 DEBUG-SUB-3 DEBUGGING DECIMAL-POINT
syn keyword cobolReserved DELARATIVES DELETE DELIMITED DELIMITEER DEPENDING DESCENDING DESTINATION
syn keyword cobolReserved DETAIL DISABLE DISPLAY DIVIDE DIVISION DOWN DUPLICATES DYNAMIC EGI ELSE EMI
syn keyword cobolReserved ENABLE END-ADD END-COMPUTE END-DELETE END-DIVIDE END-EVALUATE END-IF
syn keyword cobolReserved END-MULTIPLY END-OF-PAGE END-PERFORM END-READ END-RECEIVE END-RETURN
syn keyword cobolReserved END-REWRITE END-SEARCH END-START END-STRING END-SUBTRACT END-UNSTRING
syn keyword cobolReserved END-WRITE ENVIRONMENT EQUAL ERROR ESI EVALUATE EVERY EXCEPTION EXIT
syn keyword cobolReserved EXTEND EXTERNAL FALSE FD FILE FILE-CONTROL FILLER FINAL FIRST FOOTING FOR FROM
syn keyword cobolReserved GENERATE GIVING GLOBAL GREATER GROUP HEADING HIGH-VALUE HIGH-VALUES I-O
syn keyword cobolReserved I-O-CONTROL IDENTIFICATION IN INDEX INDEXED INDICATE INITIAL INITIALIZE
syn keyword cobolReserved INITIATE INPUT INPUT-OUTPUT INSPECT INSTALLATION INTO IS JUST
syn keyword cobolReserved JUSTIFIED KEY LABEL LAST LEADING LEFT LENGTH LOCK MEMORY
syn keyword cobolReserved MERGE MESSAGE MODE MODULES MOVE MULTIPLE MULTIPLY NATIVE NEGATIVE NEXT NO NOT
syn keyword cobolReserved NUMBER NUMERIC NUMERIC-EDITED OBJECT-COMPUTER OCCURS OF OFF OMITTED ON OPEN
syn keyword cobolReserved OPTIONAL OR ORDER ORGANIZATION OTHER OUTPUT OVERFLOW PACKED-DECIMAL PADDING
syn keyword cobolReserved PAGE PAGE-COUNTER PERFORM PF PH PIC PICTURE PLUS POINTER POSITION POSITIVE
syn keyword cobolReserved PRINTING PROCEDURE PROCEDURES PROCEDD PROGRAM PROGRAM-ID PURGE QUEUE QUOTES
syn keyword cobolReserved RANDOM RD READ RECEIVE RECORD RECORDS REDEFINES REEL REFERENCE REFERENCES
syn keyword cobolReserved RELATIVE RELEASE REMAINDER REMOVAL REPLACE REPLACING REPORT REPORTING
syn keyword cobolReserved REPORTS RERUN RESERVE RESET RETURN RETURNING REVERSED REWIND REWRITE RF RH
syn keyword cobolReserved RIGHT ROUNDED RUN SAME SD SEARCH SECTION SECURITY SEGMENT SEGMENT-LIMITED
syn keyword cobolReserved SELECT SEND SENTENCE SEPARATE SEQUENCE SEQUENTIAL SET SIGN SIZE SORT
syn keyword cobolReserved SORT-MERGE SOURCE SOURCE-COMPUTER SPECIAL-NAMES STANDARD
syn keyword cobolReserved STANDARD-1 STANDARD-2 START STATUS STOP STRING SUB-QUEUE-1 SUB-QUEUE-2
syn keyword cobolReserved SUB-QUEUE-3 SUBTRACT SUM SUPPRESS SYMBOLIC SYNC SYNCHRONIZED TABLE TALLYING
syn keyword cobolReserved TAPE TERMINAL TERMINATE TEST TEXT THAN THEN THROUGH THRU TIME TIMES TO TOP
syn keyword cobolReserved TRAILING TRUE TYPE UNIT UNSTRING UNTIL UP UPON USAGE USE USING VALUE VALUES
syn keyword cobolReserved VARYING WHEN WITH WORDS WORKING-STORAGE WRITE
syn match   cobolReserved "\<CONTAINS\>"
syn match   cobolReserved "\<\(IF\|INVALID\|END\|EOP\)\>"
syn match   cobolReserved "\<ALL\>"

syn match   cobolNumber       "\<-\=\d*\.\=\d\+\>"
syn match   cobolAreaA        "^......"

syn keyword cobolConstant SPACE SPACES NULL ZERO ZEROES ZEROS LOW-VALUE LOW-VALUES

syn match   cobolTodo         "todo" contained
syn match   cobolComment      "^.\{6}\*.*"lc=6,hs=s+6 contains=cobolTodo
syn match   cobolComment      "^.\{6}/.*"lc=6,hs=s+6 contains=cobolTodo
syn region  cobolComment      start="*>" end="$" contains=cobolTodo
syn match   cobolCompiler     "^.\{6}$.*"lc=6,hs=s+6
syn match   cobolContinue     "^.\{6}-"

syn match   cobolBadLine      "^.\{6}[^ D*$/].*"lc=6,hs=s+6

syn keyword cobolGoTo         GO GOTO
syn keyword cobolCopy         COPY
" cobolBAD: things that are BAD NEWS!
syn keyword cobolBAD          ALTER ENTER RENAMES

" cobolWatch: things that are important when trying to understand a program
syn keyword cobolWatch        OCCURS DEPENDING VARYING BINARY COMP REDEFINES
syn keyword cobolWatch        REPLACING RUN
syn match   cobolWatch        "COMP-[123456XN]"
syn keyword cobolEXECs        EXEC END-EXEC

syn match   cobolParas        "^.\{6} \{1,4}[A-Z0-9][^"]\{-}\."lc=6,hs=s+7

syn match   cobolDecl         "^.\{6} \{1,4}\(0\=1\|77\|78\) "lc=6,hs=s+7,he=e-1
syn match   cobolDecl         "^.\{6} \+[1-4]\d "lc=6,hs=s+7,he=e-1
syn match   cobolDecl         "^.\{6} \+0\=[2-9] "lc=6,hs=s+7,he=e-1
syn match   cobolDecl         "^.\{6} \+66 "lc=6,hs=s+7,he=e-1

syn match   cobolWatch        "^.\{6} \+88 "lc=6,hs=s+7,he=e-1

syn match   cobolBadID        "\k\+-\($\|[^A-Z0-9]\)"
syn match   cobolBadID        "[^A-Z0-9]-\d*[A-Z-]\+"hs=s+1

syn keyword cobolCALLs        CALL CANCEL GOBACK PERFORM INVOKE
syn match   cobolCALLs        "EXIT \+PROGRAM"
syn match   cobolExtras       /\<VALUE \+\d\+\./hs=s+6,he=e-1

" syn match   cobolNumber       "\<-\=\d*\.\=\d\+\>"

syn match   cobolString       /".\{-}"/
syn match   cobolString       /'.\{-}'/

syn region  cobolLine       start="^.\{6} " end="$" contains=ALL
if exists("cobol_legacy_code")
    syn region  cobolCondFlow     contains=ALLBUT,cobolLine,cobolCondFlow start="\<\(IF\|INVALID\|END\|EOP\)\>" skip=/"[^"]\+"/ end="\."
endif

if ! exists("cobol_legacy_code")
    " catch junk in columns 1-6 for modern code
    syn match cobolBAD      "^ \{0,5\}[^ ].*"
endif

" many legacy sources have junk in columns 1-6: must be before others
" Stuff after column 72 is in error - must be after all other "match" entries
if exists("cobol_legacy_code")
    syn match   cobolBadLine      "^.\{6}[^*/].\{66,\}"
else
    syn match   cobolBadLine      "^.\{6}.\{67,\}"
endif

if !exists("did_cobol_syntax_inits")
  let did_cobol_syntax_inits = 1
"    hi link cobolJunk     Error
    hi link cobolBAD      Error
    hi link cobolBadID    Error
    hi link cobolBadLine  Error
    hi link cobolCALLs    Function
    hi link cobolComment  Comment
    hi link cobolAreaA    LineNr
    hi link cobolCompiler PreProc
    hi link cobolCondFlow Special
    hi link cobolCopy     PreProc
    hi link cobolDecl     Type
    hi link cobolEXECs    Special
    hi link cobolExtras   Special
    hi link cobolGoTo     Special
    hi link cobolConstant Constant
    hi link cobolNumber   Constant
    hi link cobolParas    Function
    hi link cobolReserved Statement
    hi link cobolString   Constant
    hi link cobolTodo     Todo
    hi link cobolWatch    Special
endif

let b:current_syntax = "cobol"
