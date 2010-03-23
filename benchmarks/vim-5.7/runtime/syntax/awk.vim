" Vim syntax file
" Language:	awk, nawk, gawk, mawk
" Maintainer:	Antonio Colombo <antonio.colombo@jrc.org>
" Last Change:	1997 November 29

" AWK  ref.  is: Alfred V. Aho, Brian W. Kernighan, Peter J. Weinberger
" The AWK Programming Language, Addison-Wesley, 1988

" GAWK ref. is: Arnold D. Robbins
" Effective AWK Programming, A User's Guide for GNU Awk
" Edition 1.0.3, Free Software Foundation, 1997

" MAWK is a "new awk" meaning it implements AWK ref.
" mawk conforms to the Posix 1003.2 (draft 11.3)
" definition of the AWK language which contains a few features
" not described in the AWK book, and mawk provides a small number of extensions.

" TODO:
" Dig into the commented out syntax expressions below.

" Remove any old syntax stuff hanging around
syn clear

" A bunch of useful Awk keywords
" AWK  ref. p. 188
syn keyword awkStatement	break continue delete exit
syn keyword awkStatement	function getline next
syn keyword awkStatement	print printf return
" GAWK ref. p. 268
syn keyword awkStatement	nextfile
" AWK  ref. p. 42, GAWK ref. p. 272
syn keyword awkFunction	atan2 close cos exp int log rand sin sqrt srand
syn keyword awkFunction	gsub index length match split sprintf sub
syn keyword awkFunction	substr system
" GAWK ref. p. 273-274
syn keyword awkFunction	fflush gensub tolower toupper
syn keyword awkFunction	systime strftime

syn keyword awkConditional	if else
syn keyword awkRepeat	while for

syn keyword awkTodo		contained TODO

syn keyword awkPatterns	BEGIN END
" AWK  ref. p. 36
syn keyword awkVariables	ARGC ARGV FILENAME FNR FS NF NR
syn keyword awkVariables	OFMT OFS ORS RLENGTH RS RSTART SUBSEP
" GAWK ref. p. 260
syn keyword awkVariables	ARGIND CONVFMT ENVIRON ERRNO
syn keyword awkVariables	FIELDWIDTHS IGNORECASE RT RLENGTH

syn keyword awkRepeat	do

" Octal format character.
syn match   awkSpecialCharacter contained "\\[0-7]\{1,3\}"
syn keyword awkStatement	func nextfile
" Hex   format character.
syn match   awkSpecialCharacter contained "\\x[0-9A-Fa-f]\+"

syn match   awkFieldVars	"\$\d\+"

"catch errors caused by wrong parenthesis
syn region	awkParen	transparent start="(" end=")" contains=ALLBUT,awkParenError,awkSpecialCharacter,awkArrayElement,awkArrayArray,awkTodo,awkRegExp,awkBrktRegExp,awkBrackets,awkCharClass
syn match	awkParenError	")"
syn match	awkInParen	contained "[{}]"

" 64 lines for complex &&'s, and ||'s in a big "if"
syn sync ccomment awkParen maxlines=64

" Search strings & Regular Expressions therein.
syn region  awkSearch	oneline start="^[ \t]*/"ms=e start="\(,\|!\=\~\)[ \t]*/"ms=e skip="\\\\\|\\/" end="/" contains=awkBrackets,awkRegExp,awkSpecialCharacter
syn region  awkBrackets	contained start="\[\^\]\="ms=s+2 start="\[[^\^]"ms=s+1 end="\]"me=e-1 contains=awkBrktRegExp,awkCharClass
syn region  awkSearch	oneline start="[ \t]*/"hs=e skip="\\\\\|\\/" end="/" contains=awkBrackets,awkRegExp,awkSpecialCharacter

syn match   awkCharClass	contained "\[:[^:\]]*:\]"
syn match   awkBrktRegExp	contained "\\.\|.\-[^]]"
syn match   awkRegExp	contained "/\^"ms=s+1
syn match   awkRegExp	contained "\$/"me=e-1
syn match   awkRegExp	contained "[?.*{}|+]"

" String and Character constants
" Highlight special characters (those which have a backslash) differently
syn region  awkString	start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=awkSpecialCharacter,awkSpecialPrintf
syn match   awkSpecialCharacter contained "\\."

" Some of these combinations may seem weird, but they work.
syn match   awkSpecialPrintf	contained "%[-+ #]*\d*\.\=\d*[cdefgiosuxEGX%]"

" Numbers, allowing signs (both -, and +)
" Integer number.
syn match  awkNumber		"[+-]\=\<\d\+\>"
" Floating point number.
syn match  awkFloat		"[+-]\=\<\d\+\.\d+\>"
" Floating point number, starting with a dot.
syn match  awkFloat		"[+-]\=\<.\d+\>"
syn case ignore
"floating point number, with dot, optional exponent
syn match  awkFloat	"\<\d\+\.\d*\(e[-+]\=\d\+\)\=\>"
"floating point number, starting with a dot, optional exponent
syn match  awkFloat	"\.\d\+\(e[-+]\=\d\+\)\=\>"
"floating point number, without dot, with exponent
syn match  awkFloat	"\<\d\+e[-+]\=\d\+\>"
syn case match

"syn match  awkIdentifier	"\<[a-zA-Z_][a-zA-Z0-9_]*\>"

" Arithmetic operators: +, and - take care of ++, and --
"syn match   awkOperator	"+\|-\|\*\|/\|%\|="
"syn match   awkOperator	"+=\|-=\|\*=\|/=\|%="
"syn match   awkOperator	"^\|^="

" Comparison expressions.
"syn match   awkExpression	"==\|>=\|=>\|<=\|=<\|\!="
"syn match   awkExpression	"\~\|\!\~"
"syn match   awkExpression	"?\|:"
"syn keyword awkExpression	in

" Boolean Logic (OR, AND, NOT)
"syn match  awkBoolLogic	"||\|&&\|\!"

" This is overridden by less-than & greater-than.
" Put this above those to override them.
" Put this in a 'match "\<printf\=\>.*;\="' to make it not override
" less/greater than (most of the time), but it won't work yet because
" keywords allways have precedence over match & region.
" File I/O: (print foo, bar > "filename") & for nawk (getline < "filename")
"syn match  awkFileIO		contained ">"
"syn match  awkFileIO		contained "<"

" Expression separators: ';' and ','
syn match  awkSemicolon	";"
syn match  awkComma		","

syn match  awkComment	"#.*" contains=awkTodo

syn match  awkLineSkip	"\\$"

" Highlight array element's (recursive arrays allowed).
" Keeps nested array names' separate from normal array elements.
" Keeps numbers separate from normal array elements (variables).
syn match  awkArrayArray	contained "[^][, \t]*\["me=e-1
syn match  awkArrayElement	contained "[^][, \t]*"
syn region awkArray		transparent start="\[" end="\]" contains=awkArray,awkArrayElement,awkArrayArray,awkNumber,awkFloat

" 10 should be enough.
" (for the few instances where it would be more than "oneline")
syn sync ccomment awkArray maxlines=10

if !exists("did_awk_syntax_inits")
  let did_awk_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link awkConditional	Conditional
  hi link awkFunction	Function
  hi link awkRepeat	Repeat
  hi link awkStatement	Statement

  hi link awkString	String
  hi link awkSpecialPrintf Special
  hi link awkSpecialCharacter Special

  hi link awkSearch	String
  hi link awkBrackets	awkRegExp
  hi link awkBrktRegExp	awkNestRegExp
  hi link awkCharClass	awkNestRegExp
  hi link awkNestRegExp	Keyword
  hi link awkRegExp	Special

  hi link awkNumber	Number
  hi link awkFloat	Float

  hi link awkFileIO	Special
  "hi link awkOperator	Special
  "hi link awkExpression	Special
  hi link awkBoolLogic	Special

  hi link awkPatterns	Special
  hi link awkVariables	Special
  hi link awkFieldVars	Special

  hi link awkLineSkip	Special
  hi link awkSemicolon	Special
  hi link awkComma	Special
  "hi link awkIdentifier	Identifier

  hi link awkComment	Comment
  hi link awkTodo	Todo

  " Change this if you want nested array names to be highlighted.
  hi link awkArrayArray	awkArray
  hi link awkArrayElement Special

  hi link awkParenError	awkError
  hi link awkInParen	awkError
  hi link awkError	Error
endif

let b:current_syntax = "awk"

" vim: ts=8
