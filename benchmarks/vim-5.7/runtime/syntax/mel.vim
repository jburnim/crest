" Vim syntax file
" Language:	MEL (Maya Extension Language)
" Maintainer:	Robert Minsk <egbert@centropolisfx.com>
" Last Change:	May 27 1999
" Based on:	Bram Moolenaar <Bram@vim.org> C syntax file

" Remove any old syntax stuff hanging around
sy clear

" when wanted, highlight trailing white space and spaces before tabs
if exists("mel_space_errors")
  sy match	melSpaceError	"\s\+$"
  sy match	melSpaceError	" \+\t"me=e-1
endif

" A bunch of usefull MEL keyworks
sy keyword	melBoolean	true false yes no on off

sy keyword	melFunction	proc
sy match	melIdentifier	"\$\(\a\|_\)\w*"

sy keyword	melStatement	break continue return
sy keyword	melConditional	if else switch
sy keyword	melRepeat	while for do in
sy keyword	melLabel	case default
sy keyword	melOperator	size eval env exists whatIs
sy keyword	melKeyword	alias
sy keyword	melException	catch error warning

sy keyword	melInclude	source

sy keyword	melType		int float string vector matrix
sy keyword	melStorageClass	global

sy keyword	melDebug	trace

sy keyword	melTodo		contained TODO FIXME XXX

" MEL data types
sy match	melCharSpecial	contained "\\[ntr\\"]"
sy match	melCharError	contained "\\[^ntr\\"]"

sy region	melString	start=+"+ skip=+\\"+ end=+"+ contains=melCharSpecial,melCharError

sy case ignore
sy match	melInteger	"\<\d\+\(e[-+]\=\d\+\)\=\>"
sy match	melFloat	"\<\d\+\(e[-+]\=\d\+\)\=f\>"
sy match	melFloat	"\<\d\+\.\d*\(e[-+]\=\d\+\)\=f\=\>"
sy match	melFloat	"\.\d\+\(e[-+]\=\d\+\)\=f\=\>"
sy case match

sy match	melCommaSemi	contained "[,;]"
sy region	melMatrixVector	start=/<</ end=/>>/ contains=melInteger,melFloat,melIdentifier,melCommaSemi

sy cluster	melGroup	contains=melFunction,melStatement,melConditional,melLabel,melKeyword,melStorageClass,melTODO,melCharSpecial,melCharError,melCommaSemi

" catch errors caused by wrong parenthesis
sy region	melParen	transparent start='(' end=')' contains=ALLBUT,@melGroup,melParenError,melInParen
sy match	melParenError	")"
sy match	melInParen	contained "[{}]"

" comments
sy region	melComment	start="/\*" end="\*/" contains=melTodo,melSpaceError
sy match	melComment	"//.*" contains=melTodo,melSpaceError
sy match	melCommentError "\*/"

sy region	melQuestionColon matchgroup=melConditional transparent start='?' end=':' contains=ALLBUT,@melGroup

if !exists("mel_minlines")
  let mel_minlines=15
endif
exec "sy sync ccomment melComment minlines=" . mel_minlines

if !exists("did_mel_syntax_inits")
  let did_mel_syntax_inits=1
  hi link melBoolean	Boolean
  hi link melFunction	Function
  hi link melIdentifier	Identifier
  hi link melStatement	Statement
  hi link melConditional Conditional
  hi link melRepeat	Repeat
  hi link melLabel	Label
  hi link melOperator	Operator
  hi link melKeyword	Keyword
  hi link melException	Exception
  hi link melInclude	Include
  hi link melType	Type
  hi link melStorageClass StorageClass
  hi link melDebug	Debug
  hi link melTodo	Todo
  hi link melCharSpecial SpecialChar
  hi link melString	String
  hi link melInteger	Number
  hi link melFloat	Float
  hi link melMatrixVector Float
  hi link melComment	Comment
  hi link melError	Error
  hi link melSpaceError	melError
  hi link melCharError	melError
  hi link melParenError	melError
  hi link melInParen	melError
  hi link melCommentError melError
  hi melCommaSemi	NONE
endif

let b:current_syntax = "mel"
