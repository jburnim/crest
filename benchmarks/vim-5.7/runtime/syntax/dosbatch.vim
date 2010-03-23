" Vim syntax file
" Language:	MSDOS batch file (with NT command extensions)
" Maintainer:	Mike Williams <mrw@netcomuk.co.uk>
" Filenames:	*.bat
" Last Change:	14th May 1999
" Web Page:	N/A

" Remove any old syntax stuff hanging around
syn clear

" DOS bat files are case insensitive but case preserving!
syn case ignore

syn keyword dosbatchTodo contained	TODO

" Dosbat keywords
syn keyword dosbatchStatement	goto call exit
syn keyword dosbatchConditional	if else
syn keyword dosbatchRepeat	for

" Some operators - first lot are case sensitive!
syn case match
syn keyword dosbatchOperator    EQU NEQ LSS LEQ GTR GEQ
syn case ignore
syn match dosbatchOperator      "\s[-+\*/%]\s"
syn match dosbatchOperator      "="
syn match dosbatchOperator      "[-+\*/%]="
syn match dosbatchOperator      "\s\(&\||\|^\|<<\|>>\)=\=\s"
syn match dosbatchIfOperator    "if\s\+\(\(not\)\=\s\+\)\=\(exist\|defined\|errorlevel\|cmdextversion\)\="lc=2

" String - using "'s is a convenience rather than a requirement outside of FOR
syn match dosbatchString	"\"[^"]*\"" contains=dosbatchVariable,dosBatchArgument,@dosbatchNumber
syn match dosbatchString        "\<echo[^)>|]*"lc=4 contains=dosbatchVariable,dosbatchArgument,@dosbatchNumber
syn match dosbatchEchoOperator  "\<echo\s\+\(on\|off\)\s*$"lc=4

" For embedded commands
syn match dosbatchCmd		"(\s*'[^']*'"lc=1 contains=dosbatchString,dosbatchVariable,dosBatchArgument,@dosbatchNumber,dosbatchImplicit,dosbatchStatement,dosbatchConditional,dosbatchRepeat,dosbatchOperator

" Numbers - surround with ws to not include in dir and filenames
syn match dosbatchInteger       "[[:space:]=(/:]\d\+"lc=1
syn match dosbatchHex           "[[:space:]=(/:]0x\x\+"lc=1
syn match dosbatchBinary        "[[:space:]=(/:]0b[01]\+"lc=1
syn match dosbatchOctal         "[[:space:]=(/:]0\o\+"lc=1
syn cluster dosbatchNumber      contains=dosbatchInteger,dosbatchHex,dosbatchBinary,dosbatchOctal

" Command line switches
syn match dosbatchSwitch        "/\(\a\+\|?\)"

" Various special escaped char formats
syn match dosbatchSpecialChar   "\^[&|()<>^]"
syn match dosbatchSpecialChar   "\$[a-hl-npqstv_$+]"
syn match dosbatchSpecialChar   "%%"

" Environment variables
syn match dosbatchIdentifier    contained "\s\h\w*\>"
syn match dosbatchVariable	"%\h\w*%"
syn match dosbatchVariable	"%\h\w*:\*\=[^=]*=[^%]*%"
syn match dosbatchVariable	"%\h\w*:\~\d\+,\d\+%" contains=dosbatchInteger
syn match dosbatchSet           "\s\h\w*[+-]\==\{-1}" contains=dosbatchIdentifier,dosbatchOperator

" Args to bat files and for loops, etc
syn match dosbatchArgument	"%\(\d\|\*\)"
syn match dosbatchArgument	"%%[a-z]\>"
syn match dosbatchArgument	"%\~[fdpnxs]\+\(\($PATH:\)\=[a-z]\|\d\)\>"

" Line labels
syn match dosbatchLabel         "^\s*:\s*\h\w*\>"
syn match dosbatchLabel         "\<\(goto\|call\)\s\+:\h\w*\>"lc=4
syn match dosbatchLabel         "\<goto\s\+\h\w*\>"lc=4
syn match dosbatchLabel         ":\h\w*\>"

" Comments - usual rem but also two colons as first non-space is an idiom
syn match dosbatchComment	"^rem\($\|\s.*$\)"lc=3 contains=dosbatchTodo,@dosbatchNumber,dosbatchVariable,dosbatchArgument
syn match dosbatchComment	"\srem\($\|\s.*$\)"lc=4 contains=dosbatchTodo,@dosbatchNumber,dosbatchVariable,dosbatchArgument
syn match dosbatchComment	"\s*:\s*:.*$" contains=dosbatchTodo,@dosbatchNumber,dosbatchVariable,dosbatchArgument

" Comments in ()'s - still to handle spaces before rem
syn match dosbatchComment	"(rem[^)]*"lc=4 contains=dosbatchTodo,@dosbatchNumber,dosbatchVariable,dosbatchArgument

syn keyword dosbatchImplicit    append assoc at attrib break cacls cd chcp chdir
syn keyword dosbatchImplicit    chkdsk cls cmd color comp compact convert copy
syn keyword dosbatchImplicit    date del dir diskcomp diskcopy doskey echo endlocal
syn keyword dosbatchImplicit    erase fc find findstr format ftype
syn keyword dosbatchImplicit    graftabl help keyb label md mkdir mode more move
syn keyword dosbatchImplicit    path pause popd print prompt pushd rd recover rem
syn keyword dosbatchImplicit    ren rename replace restore rmdir set setlocal shift
syn keyword dosbatchImplicit    sort start subst time title tree type ver verify
syn keyword dosbatchImplicit    vol xcopy

if !exists("did_dosbatch_syntax_inits")
  let did_dosbatch_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link dosbatchTodo			Todo

  hi link dosbatchStatement		Statement
  hi link dosbatchCommands		dosbatchStatement
  hi link dosbatchLabel			Label
  hi link dosbatchConditional		Conditional
  hi link dosbatchRepeat		Repeat

  hi link dosbatchOperator              Operator
  hi link dosbatchEchoOperator          dosbatchOperator
  hi link dosbatchIfOperator            dosbatchOperator

  hi link dosbatchArgument		Identifier
  hi link dosbatchIdentifier            Identifier
  hi link dosbatchVariable		dosbatchIdentifier

  hi link dosbatchSpecialChar		SpecialChar
  hi link dosbatchString		String
  hi link dosbatchNumber		Number
  hi link dosbatchInteger		dosbatchNumber
  hi link dosbatchHex			dosbatchNumber
  hi link dosbatchBinary		dosbatchNumber
  hi link dosbatchOctal			dosbatchNumber

  hi link dosbatchComment		Comment
  hi link dosbatchImplicit		Function

  hi link dosbatchSwitch                Special

  hi link dosbatchCmd                   PreProc
endif

let b:current_syntax = "dosbatch"

" vim: ts=8
