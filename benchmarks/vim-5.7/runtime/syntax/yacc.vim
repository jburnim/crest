" Vim syntax file
" Language:	Yacc
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	April 8, 1999

" Remove any old syntax stuff hanging around
syn clear

" Read the C syntax to start with
source <sfile>:p:h/c.vim

" Clusters
syn cluster	yaccActionGroup	contains=yaccDelim,cInParen,cTodo,cIncluded,yaccDelim,yaccCurlyError,yaccUnionCurly,yaccUnion,cUserLabel,cOctalZero,cCppOut2,cCppSkip,cErrInBracket,cErrInParen,cOctalError
syn cluster	yaccUnionGroup	contains=yaccKey,cComment,yaccCurly,cType,cStructure,cStorageClass,yaccUnionCurly

" Yacc stuff
syn match	yaccDelim	"^[ \t]*[:|;]"
syn match	yaccOper	"@\d\+"

syn match	yaccKey	"^[ \t]*%\(token\|type\|left\|right\|start\|ident\)\>"
syn match	yaccKey	"[ \t]%\(prec\|expect\|nonassoc\)\>"
syn match	yaccKey	"\$\(<[a-zA-Z_][a-zA-Z_0-9]*>\)\=[\$0-9]\+"
syn keyword	yaccKeyActn	yyerrok yyclearin

syn match	yaccUnionStart	"^%union"	skipwhite skipnl nextgroup=yaccUnion
syn region	yaccUnion	contained matchgroup=yaccCurly start="{" matchgroup=yaccCurly end="}"	contains=@yaccUnionGroup
syn region	yaccUnionCurly	contained matchgroup=yaccCurly start="{" matchgroup=yaccCurly end="}" contains=@yaccUnionGroup
syn match	yaccBrkt	contained "[<>]"
syn match	yaccType	"<[a-zA-Z_][a-zA-Z0-9_]*>"	contains=yaccBrkt
syn match	yaccDefinition	"^[A-Za-z][A-Za-z0-9_]*[ \t]*:"

" special Yacc separators
syn match	yaccSectionSep	"^[ \t]*%%"
syn match	yaccSep	"^[ \t]*%{"
syn match	yaccSep	"^[ \t]*%}"

" I'd really like to highlight just the outer {}.  Any suggestions???
syn match	yaccCurlyError	"[{}]"
syn region	yaccAction	matchgroup=yaccCurly start="{" end="}" contains=ALLBUT,@yaccActionGroup

if !exists("did_yacc_syntax_inits")
  " The default methods for highlighting.  Can be overridden later
  let did_yacc_syntax_inits = 1

  " Internal yacc highlighting links
  hi link yaccBrkt	yaccStmt
  hi link yaccKey	yaccStmt
  hi link yaccOper	yaccStmt
  hi link yaccUnionStart	yaccKey

  " External yacc highlighting links
  hi link yaccCurly	Delimiter
  hi link yaccCurlyError	Error
  hi link yaccDefinition	Function
  hi link yaccDelim	Function
  hi link yaccKeyActn	Special
  hi link yaccSectionSep	Todo
  hi link yaccSep	Delimiter
  hi link yaccStmt	Statement
  hi link yaccType	Type

  " since Bram doesn't like my Delimiter :|
  hi link Delimiter	Type
endif

let b:current_syntax = "yacc"

" vim: ts=15
