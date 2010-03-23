" Vim syntax file
" Language:		lace
" Maintainer:	Jocelyn Fiat <utilities@eiffel.com>
" Last Change:	1999 Jun 14

" Copyright Interactive Software Engineering, 1998
" You are free to use this file as you please, but
" if you make a change or improvement you must send
" it to the maintainer at <utilities@eiffel.com>


" Remove any old syntax stuff hanging around
syn clear

" LACE is case insensitive, but the style guide lines are not.

if !exists("lace_case_insensitive")
	syn case match
else
	syn case ignore
endif

" A bunch of useful LACE keywords
syn keyword laceTopStruct		system root default option visible cluster
syn keyword laceTopStruct		external generate end
syn keyword laceOptionClause	collect assertion debug optimize trace
syn keyword laceOptionClause	profile inline precompiled multithreaded
syn keyword laceOptionClause	exception_trace dead_code_removal
syn keyword laceOptionClause	array_optimization
syn keyword laceOptionClause	inlining_size inlining
syn keyword laceOptionClause	console_application dynamic_runtime
syn keyword laceOptionClause	line_generation
syn keyword laceOptionMark		yes no all
syn keyword laceOptionMark		require ensure invariant loop check
syn keyword laceClusterProp		use include exclude
syn keyword laceAdaptClassName	adapt ignore rename as
syn keyword laceAdaptClassName	creation export visible
syn keyword laceExternal		include_path object makefile

" Operators
syn match   laceOperator		"\$"
syn match   laceBrackets		"[[\]]"
syn match   laceExport			"[{}]"

" Constants
syn keyword laceBool		true false
syn keyword laceBool		True False
syn region  laceString		start=+"+ skip=+%"+ end=+"+ contains=laceEscape,laceStringError
syn match   laceEscape		contained "%[^/]"
syn match   laceEscape		contained "%/\d\+/"
syn match   laceEscape		contained "^[ \t]*%"
syn match   laceEscape		contained "%[ \t]*$"
syn match   laceStringError	contained "%/[^0-9]"
syn match   laceStringError	contained "%/\d\+[^0-9/]"
syn match   laceStringError	"'\(%[^/]\|%/\d\+/\|[^'%]\)\+'"
syn match   laceCharacter	"'\(%[^/]\|%/\d\+/\|[^'%]\)'" contains=laceEscape
syn match   laceNumber		"-\=\<\d\+\(_\d\+\)*\>"
syn match   laceNumber		"\<[01]\+[bB]\>"
syn match   laceNumber		"-\=\<\d\+\(_\d\+\)*\.\(\d\+\(_\d\+\)*\)\=\([eE][-+]\=\d\+\(_\d\+\)*\)\="
syn match   laceNumber		"-\=\.\d\+\(_\d\+\)*\([eE][-+]\=\d\+\(_\d\+\)*\)\="
syn match   laceComment		"--.*" contains=laceTodo


syn case match

" Case sensitive stuff

syn keyword laceTodo		TODO XXX FIXME
syn match	laceClassName	"\<[A-Z][A-Z0-9_]*\>"
syn match	laceCluster		"[a-zA-Z][a-zA-Z0-9_]*\s*:"
syn match	laceCluster		"[a-zA-Z][a-zA-Z0-9_]*\s*(\s*[a-zA-Z][a-zA-Z0-9_]*\s*)\s*:"

" Catch mismatched parentheses
syn match laceParenError	")"
syn match laceBracketError	"\]"
syn region laceGeneric		transparent matchgroup=laceBrackets start="\[" end="\]" contains=ALLBUT,laceBracketError
syn region laceParen		transparent start="(" end=")" contains=ALLBUT,laceParenError

" Should suffice for even very long strings and expressions
syn sync lines=40

if !exists("did_lace_syntax_inits")
  let did_lace_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link laceTopStruct			PreProc

  hi link laceOptionClause		Statement
  hi link laceOptionMark		Constant
  hi link laceClusterProp		Label
  hi link laceAdaptClassName	Label
  hi link laceExternal			Statement
  hi link laceCluster			ModeMsg

  hi link laceEscape			Special

  hi link laceBool				Boolean
  hi link laceString			String
  hi link laceCharacter			Character
  hi link laceClassName			Type
  hi link laceNumber			Number

  hi link laceOperator			Special
  hi link laceArray				Special
  hi link laceExport			Special
  hi link laceCreation			Special
  hi link laceBrackets			Special
  hi link laceConstraint		Special

  hi link laceComment			Comment

  hi link laceError				Error
  hi link laceStringError		Error
  hi link laceParenError		Error
  hi link laceBracketError		Error
  hi link laceTodo				Todo
endif

let b:current_syntax = "lace"

" vim: ts=4
