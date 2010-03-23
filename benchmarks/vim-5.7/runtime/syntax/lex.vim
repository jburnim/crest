" Vim syntax file
" Language:	Lex
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	May 13, 1999

" Remove any old syntax stuff hanging around
syn clear

" Read the C syntax to start with
so <sfile>:p:h/c.vim

" --- Lex stuff ---

"I'd prefer to use lex.* , but it doesn't handle forward definitions yet
syn cluster lexListGroup		contains=lexAbbrvBlock,lexAbbrv,lexAbbrv,lexAbbrvRegExp,lexInclude,lexPatBlock,lexPat,lexBrace,lexPatString,lexPatTag,lexPatTag,lexPatComment,lexPatCodeLine,lexMorePat,lexPatSep,lexSlashQuote,lexPatCode,cInParen,cUserLabel,cOctalZero,cCppSkip,cErrInBracket,cErrInParen,cOctalError,cCppOut2
syn cluster lexListPatCodeGroup	contains=lexAbbrvBlock,lexAbbrv,lexAbbrv,lexAbbrvRegExp,lexInclude,lexPatBlock,lexPat,lexBrace,lexPatTag,lexPatTag,lexPatComment,lexPatCodeLine,lexMorePat,lexPatSep,lexSlashQuote,cInParen,cUserLabel,cOctalZero,cCppSkip,cErrInBracket,cErrInParen,cOctalError,cCppOut2

" Abbreviations Section
syn region lexAbbrvBlock	start="^\([a-zA-Z_]\+\t\|%{\)" end="^%%$"me=e-2	skipnl	nextgroup=lexPatBlock contains=lexAbbrv,lexInclude,lexAbbrvComment
syn match  lexAbbrv		"^\I\i*\s"me=e-1			skipwhite	contained nextgroup=lexAbbrvRegExp
syn match  lexAbbrv		"^%[sx]"					contained
syn match  lexAbbrvRegExp	"\s\S.*$"lc=1				contained nextgroup=lexAbbrv,lexInclude
syn region lexInclude	matchgroup=lexSep	start="^%{" end="%}"	contained	contains=ALLBUT,@lexListGroup
syn region lexAbbrvComment	start="^\s\+/\*"	end="\*/"

"%% : Patterns {Actions}
syn region lexPatBlock	matchgroup=Todo	start="^%%$" matchgroup=Todo end="^%%$"	skipnl skipwhite contains=lexPat,lexPatTag,lexPatComment
syn region lexPat		start=+\S+ skip="\\\\\|\\."	end="\s"me=e-1	contained nextgroup=lexMorePat,lexPatSep contains=lexPatString,lexSlashQuote,lexBrace
syn region lexBrace	start="\[" skip=+\\\\\|\\+		end="]"		contained
syn region lexPatString	matchgroup=String start=+"+	skip=+\\\\\|\\"+	matchgroup=String end=+"+	contained
syn match  lexPatTag	"^<\I\i*\(,\I\i*\)*>*"			contained nextgroup=lexPat,lexPatTag,lexMorePat,lexPatSep
syn match  lexPatTag	+^<\I\i*\(,\I\i*\)*>*\(\\\\\)*\\"+		contained nextgroup=lexPat,lexPatTag,lexMorePat,lexPatSep
syn region lexPatComment	start="^\s*/\*" end="\*/"		skipnl	contained contains=cTodo nextgroup=lexPatComment,lexPat,lexPatString,lexPatTag
syn match  lexPatCodeLine	".*$"					contained contains=ALLBUT,@lexListGroup
syn match  lexMorePat	"\s*|\s*$"			skipnl	contained nextgroup=lexPat,lexPatTag,lexPatComment
syn match  lexPatSep	"\s\+"					contained nextgroup=lexMorePat,lexPatCode,lexPatCodeLine
syn match  lexSlashQuote	+\(\\\\\)*\\"+				contained
syn region lexPatCode matchgroup=Delimiter start="{" matchgroup=Delimiter end="}"	skipnl contained contains=ALLBUT,@lexListPatCodeGroup

syn keyword lexCFunctions	BEGIN	input	unput	woutput	yyleng	yylook	yytext
syn keyword lexCFunctions	ECHO	output	winput	wunput	yyless	yymore	yywrap

" <c.vim> includes several ALLBUTs; these have to be treated so as to exclude lex* groups
syn cluster cParenGroup	add=lex.*
syn cluster cDefineGroup	add=lex.*
syn cluster cPreProcGroup	add=lex.*
syn cluster cMultiGroup	add=lex.*

" Synchronization
syn sync clear
syn sync minlines=300
syn sync match lexSyncPat	grouphere  lexPatBlock	"^%[a-zA-Z]"
syn sync match lexSyncPat	groupthere lexPatBlock	"^<$"
syn sync match lexSyncPat	groupthere lexPatBlock	"^%%$"

if !exists("did_lex_syntax_inits")
  let did_lex_synax_inits = 1
  hi link	lexSlashQuote	lexPat
  hi link	lexBrace		lexPat
  hi link lexAbbrvComment	lexPatComment

  hi link	lexAbbrv		SpecialChar
  hi link	lexAbbrvRegExp	Macro
  hi link	lexCFunctions	Function
  hi link	lexMorePat	SpecialChar
  hi link	lexPat		Function
  hi link	lexPatComment	Comment
  hi link	lexPatString	Function
  hi link	lexPatTag		Special
  hi link	lexSep		Delimiter
endif

let b:current_syntax = "lex"

" vim:ts=10
