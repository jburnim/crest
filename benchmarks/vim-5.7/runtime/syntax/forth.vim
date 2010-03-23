" Vim syntax file
" Language:		FORTH
" Maintainer:	Christian V. J. Br=FCssow <cvjb@bigfoot.de>
" Last Change:	Fre 18 Jun 1999 13:44:14 MEST

" The list of keywords is incomplete, compared with the offical ANS
" wordlist. If you use this language, please improve it, and send me
" the patches.

" Remove any old syntax stuff hanging around
syn clear

" Synchronization method
syn sync ccomment maxlines=200

" Forth is not case sensitive
syn case ignore

" Some special, non-FORTH keywords
syn keyword forthTodo contained TODO FIXME XXX FSF
syn match forthTodo contained 'Copyright\(\s([Cc])\)\=\(\s[0-9]\{2,4}\)\=\s\='
syn match forthTodo contained 'Free\sSoftware\sFoundation\(,\sInc[.]\=\)\='

" Characters allowed in keywords
" I don't know if 128-255 are allowed in ANS-FORHT
set iskeyword=!,@,33-64,A-Z,91-96,a-z,123-126,128-255

" Keywords

" * basic (return)stack manipulation and control words
syn keyword forthStack -ROT . .R .S 2DROP 2DUP 2OVER 2ROT 2SWAP 3DUP 4DUP
syn keyword forthStack ?DUP D. D.R DEPTH DROP DUP F. FDEPTH FDUP FDROP FE. FLIP
syn keyword forthStack FOVER FROT FS. FSWAP NIP OVER PICK ROLL ROT SP0 SP@
syn keyword forthStack SWAP TOS TUCK U. U.R UD. UD.R
syn keyword forthRStack 2>R 2R> 2R@ >R R> R@ RP0 RP@ TOR

" * basic character operations
syn keyword forthCharOps (.) -TRAILING /STRING ?UPPERCASE ASCII BL CAPS CAPS-COMP
syn keyword forthCharOps CHAR CHAR+ CHARS COMP COMPARE CONTROL COUNT CR EMIT EMIT?
syn keyword forthCharOps EXPECT FIND KEY KEY? LENGTH SPACE SPACES TYPE UPC UPPER
syn keyword forthCharOps WORD

" * char-number converion
syn keyword forthConversion <# # #> #S (NUMBER) (NUMBER?) CONVERT D>F D>S DIGIT DPL
syn keyword forthConversion F>D HLD HOLD NUMBER S>D SIGN

" * Interptreter, Wordbook, Compiler
syn keyword forthForth ABORT ALLOCATE ASSEMBLER BASE BYE COLD DECIMAL DEBUG
syn keyword forthForth DUMP EXECUTE EXIT HERE HEX INTERPRET NOOP PAD PERFORM QUERY
syn keyword forthForth QUIT SEARCH SEE VIEW WORDS
syn region forthForth start=+ABORT"+ skip=+\\"+ end=+"+

" * Vocabularies
syn keyword forthVocs ALSO FORTH ONLY ORDER ROOT SEAL USER VOCS

" * basic mathematical and logical operators
syn keyword forthOperators = * */ */MOD *D + - / /MOD 1+ 1- 2* 2+ 2- 2/ 8* ABS AND
syn keyword forthOperators D+ D- D2* D2/ DABS F* F** F+ F- F/ FABS FM/MOD LSHIFT
syn keyword forthOperators M* M*/ M+ M/MOD MOD NOT OR RSHIFT U*D D2/ UM* UM/MOD XOR

" * basic address operations
syn keyword forthAdrOps ! ' +! , .ID 2! 2@ >BODY >FLOAT >IN >LINK >NAME >NEXT
syn keyword forthAdrOps >NUMBER >VIEW @ ALLOT BLANK BODY> BOUNDS C! C, C@ CMOVE
syn keyword forthAdrOps CMOVE> CRESET CSET CTOGGLE ERASE DF! DF@ F! F@ FILL L>NAME
syn keyword forthAdrOps LINK> MOVE N>LINK SF! SF@ VIEW>

" * Iterations
syn keyword forthIteration +LOOP ?DO ?LEAVE AGAIN BEGIN DO ENDOF I J LEAVE LOOP OF
syn keyword forthIteration RECURSE RECURSIVE REPEAT UNLOOP UNTIL WHILE

" * Conditionals
syn keyword forthConditional ?BRANCH BRANCH CASE ELSE ENDCASE ENDIF IF THEN
syn keyword forthConditional 0< 0<= 0<> 0= 0> 0>= < <= <> > >= BETWEEN D0< D0=
syn keyword forthConditional D< D= D> DU< F0< F0= F< FALSE OFF ON TRUE
syn keyword forthConditional U< U<= U> U>=

" Numbers
syn match forthInteger '\<-\=[0-9.]*[0-9]\+\>'
syn match forthFloat '\<-\=[0-9]*[.]\=[0-9]\+[Ee][0-9]\+\>'

" Comments
syn match forthComment '\\\s.*$' contains=forthTodo
syn region forthComment start='\\S\s' end='.*' contains=forthTodo
syn match forthComment '\.(\s[^)]*)' contains=forthTodo
syn region forthComment start='(\s' skip='\\)' end=')' contains=forthTodo
"syn match forthComment '(\s[^\-]*\-\-[^\-]*)' contains=forthTodo

" Include files
syn match forthInclude '^INCLUDE\s\+\k\+'

" Definition of new words
syn match forthColonDef '\<:\s*[^ \t]\+\>'
syn keyword forthEndOfColonDef ;
syn keyword forthDefine ;CODE ;USES 2CONSTANT 2VARIABLE 2LITERAL ACTIVATE
syn keyword forthDefine ASSOCIATIVE: BACKGROUND: CASE: CODE COMPILE COMPILE,
syn keyword forthDefine CONSTANT CREATE DEFER DEFINITIONS DLITERAL DOES>
syn keyword forthDefine FCONSTANT FLITERAL FORGET FVARIABLE IMMEDIATE IS
syn keyword forthDefine LITERAL SLITERAL TASK: VARIABLE VOCABULARY ]
syn match forthDefine '\<\[\>'
syn match forthDefine "\[']"
syn match forthDefine '\[COMPILE]'

" Strings
syn region forthString start=+\.\"+ skip=+\\"+ end=+"+

if !exists("did_forth_syntax")
	let did_forth_syntax = 1
	" The default methods for highlighting. Can be overriden later.
	hi link forthTodo Todo
	hi link forthInteger Number
	hi link forthFloat Float
	hi link forthComment Comment
	hi link forthInclude Include
	hi link forthString String
	hi link forthStack Number
	hi link forthRStack Number
	hi link forthCharOps Character
	hi link forthConversion SpecialChar
	hi link forthForth Special
	hi link forthVocs Special
	hi link forthOperators Operator
	hi link forthAdrOps Operator
	hi link forthIteration Repeat
	hi link forthConditional Conditional
	hi link forthColonDef Define
	hi link forthEndOfColonDef forthColonDef
	hi link forthDefine forthColonDef
endif

let b:current_syntax = "forth"

" vim:ts=3:sw=3
