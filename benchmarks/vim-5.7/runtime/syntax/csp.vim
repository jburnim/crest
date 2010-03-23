" Vim syntax file
" Language:	CSP (Communication Sequential Processes, using FDR input syntax)
" Maintainer:	Jan Bredereke <brederek@tzi.de>
" Version:	0.4.0
" Last change:	Tue Apr 25, 2000
" URL:          http://www.tzi.de/~brederek/vim/
" Copying:	You may distribute and use this file freely, in the same
"		way as the vim editor itself.

syn clear

" case is significant to FDR:
syn case match

" Block comments in CSP are between {- and -}
syn region cspComment	start="{-"  end="-}" contains=cspTodo
" Single-line comments start with --
syn region cspComment	start="--"  end="$" contains=cspTodo,cspRttComment

" Numbers:
syn match  cspNumber "\<\d\+\>"

" Conditionals:
syn keyword  cspConditional if then else

" Operators on processes:
" -> ? : ! ' ; /\ \ [] |~| [> & [[..<-..]] ||| [|..|] || [..<->..] ; : @ |||
syn match  cspOperator "->"
syn match  cspOperator "/\\"
syn match  cspOperator "[^/]\\"lc=1
syn match  cspOperator "\[\]"
syn match  cspOperator "|\~|"
syn match  cspOperator "\[>"
syn match  cspOperator "\[\["
syn match  cspOperator "\]\]"
syn match  cspOperator "<-"
syn match  cspOperator "|||"
syn match  cspOperator "[^|]||[^|]"lc=1,me=e-1
syn match  cspOperator "[^|{\~]|[^|}\~]"lc=1,me=e-1
syn match  cspOperator "\[|"
syn match  cspOperator "|\]"
syn match  cspOperator "\[[^>]"me=e-1
syn match  cspOperator "\]"
syn match  cspOperator "<->"
syn match  cspOperator "[?:!';@]"
syn match  cspOperator "\&"
syn match  cspOperator "\."

" (not on processes:)
" syn match  cspDelimiter	"{|"
" syn match  cspDelimiter	"|}"
" syn match  cspDelimiter	"{[^-|]"me=e-1
" syn match  cspDelimiter	"[^-|]}"lc=1

" Keywords:
syn keyword cspKeyword          length null head tail concat elem
syn keyword cspKeyword          union inter diff Union Inter member card
syn keyword cspKeyword          empty set Set Seq
syn keyword cspKeyword		true false and or not within let
syn keyword cspKeyword		nametype datatype diamond normal
syn keyword cspKeyword		sbisim tau_loop_factor model_compress
syn keyword cspKeyword		explicate
syn match cspKeyword		"transparent"
syn keyword cspKeyword		external chase prioritize
syn keyword cspKeyword		channel Events
syn keyword cspKeyword		extensions productions
syn keyword cspKeyword		Bool Int

" Reserved keywords:
syn keyword cspReserved		attribute embed module subtype

" Include:
syn keyword cspInclude		include

" Assertions:
syn keyword cspAssert		assert deterministic divergence free deadlock
syn keyword cspAssert		livelock
syn match cspAssert		"\[T="
syn match cspAssert		"\[F="
syn match cspAssert		"\[FD="
syn match cspAssert		"\[FD\]"
syn match cspAssert		"\[F\]"

" Types and Sets
" (first char a capital, later at least one lower case, no trailing underscore):
syn match cspType     "\<[A-Z_][A-Z_0-9]*[a-z]\(\|[A-Za-z_0-9]*[A-Za-z0-9]\)\>"

" Processes (all upper case, no trailing underscore):
" (For identifiers that could be types or sets, too, this second rule set
" wins.)
syn match cspProcess		"\<[A-Z_][A-Z_0-9]*[A-Z0-9]\>"
syn match cspProcess		"\<[A-Z_]\>"

" reserved identifiers for tool output (ending in underscore):
syn match cspReservedIdentifier	"\<[A-Za-z_][A-Za-z_0-9]*_\>"

" ToDo markers:
syn match cspTodo		"FIXME"	contained
syn match cspTodo		"!!!"	contained

" RT-Tester pseudo comments:
syn match cspRttComment		"^--\$\$AM_UNDEF"lc=2		contained
syn match cspRttComment		"^--\$\$AM_ERROR"lc=2		contained
syn match cspRttComment		"^--\$\$AM_WARNING"lc=2		contained
syn match cspRttComment		"^--\$\$AM_SET_TIMER"lc=2	contained
syn match cspRttComment		"^--\$\$AM_RESET_TIMER"lc=2	contained
syn match cspRttComment		"^--\$\$AM_ELAPSED_TIMER"lc=2	contained
syn match cspRttComment		"^--\$\$AM_OUTPUT"lc=2		contained
syn match cspRttComment		"^--\$\$AM_INPUT"lc=2		contained

syn sync lines=250

if !exists("did_csp_syntax_inits")
  let did_csp_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  " (The command groups are defined in $VIMRUNTIME/syntax/synload.vim)
  hi link cspComment		Comment
  hi link cspNumber		Number
  hi link cspConditional	Conditional
  hi link cspOperator		Delimiter
  hi link cspKeyword		Keyword
  hi link cspReserved		SpecialChar
  hi link cspInclude		Include
  hi link cspAssert		PreCondit
  hi link cspType		Type
  hi link cspProcess		Function
  hi link cspTodo		Todo
  hi link cspRttComment		Define
  hi link cspReservedIdentifier	Error
  " (Currently unused vim method: Debug)
endif

let b:current_syntax = "csp"

" vim: ts=8
