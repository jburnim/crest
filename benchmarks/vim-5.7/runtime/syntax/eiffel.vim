" Eiffel syntax file
" Language:	Eiffel
" Maintainer:	Reimer Behrends <behrends@cse.msu.edu>
"		With much input from Jocelyn Fiat <fiat@eiffel.com>
" See http://www.cse.msu.edu/~behrends/vim/ for the most current version.
" Last Change:	1999 Sep 19

" Remove any old syntax stuff hanging around

syn clear

" Option handling

if exists("eiffel_ignore_case")
  syn case ignore
else
  syn case match
  if exists("eiffel_pedantic") || exists("eiffel_strict")
    syn keyword eiffelError	current void result precursor none
    syn keyword eiffelError	CURRENT VOID RESULT PRECURSOR None
    syn keyword eiffelError	TRUE FALSE
  endif
  if exists("eiffel_pedantic")
    syn keyword eiffelError	true false
    syn match eiffelError	"\<[a-z_]\+[A-Z][a-zA_Z_]*\>"
    syn match eiffelError	"\<[A-Z][a-z_]*[A-Z][a-zA-Z_]*\>"
  endif
  if exists("eiffel_lower_case_predef")
    syn keyword eiffelPredefined current void result precursor
  endif
endif

if exists("eiffel_hex_constants")
  syn match  eiffelNumber	"\d[0-9a-fA-F]*[xX]"
endif

" Keyword definitions

syn keyword eiffelTopStruct	indexing feature creation inherit
syn match   eiffelTopStruct	"\<class\>"
syn match   eiffelKeyword	"\<end\>"
syn match   eiffelTopStruct	"^end\>\(\s*--\s\+class\s\+\<[A-Z][A-Z0-9_]*\>\)\=" contains=eiffelClassName
syn match   eiffelBrackets	"[[\]]"
syn match eiffelBracketError	"\]"
syn region eiffelGeneric	transparent matchgroup=eiffelBrackets start="\[" end="\]" contains=ALLBUT,eiffelBracketError,eiffelGenericDecl,eiffelStringError,eiffelStringEscape,eiffelGenericCreate,eiffelTopStruct
if exists("eiffel_ise")
  syn match   eiffelCreate	"\<create\>"
  syn match   eiffelTopStruct	contained "\<create\>"
  syn match   eiffelGenericCreate  contained "\<create\>"
  syn match   eiffelTopStruct	"^create\>"
  syn region  eiffelGenericDecl	transparent matchgroup=eiffelBrackets contained start="\[" end="\]" contains=ALLBUT,eiffelCreate,eiffelTopStruct,eiffelGeneric,eiffelBracketError,eiffelStringEscape,eiffelStringError,eiffelBrackets
  syn region  eiffelClassHeader	start="^class\>" end="$" contains=ALLBUT,eiffelCreate,eiffelGenericCreate,eiffelGeneric,eiffelStringEscape,eiffelStringError,eiffelBrackets
endif
syn keyword eiffelDeclaration	is do once deferred unique local
syn keyword eiffelDeclaration	Unique
syn keyword eiffelProperty	expanded obsolete separate frozen
syn keyword eiffelProperty	prefix infix
syn keyword eiffelInheritClause	rename redefine undefine select export as
syn keyword eiffelAll		all
syn keyword eiffelKeyword	external alias
syn keyword eiffelStatement	if else elseif inspect
syn keyword eiffelStatement	when then
syn match   eiffelAssertion	"\<require\(\s\+else\)\=\>"
syn match   eiffelAssertion	"\<ensure\(\s\+then\)\=\>"
syn keyword eiffelAssertion	check
syn keyword eiffelDebug		debug
syn keyword eiffelStatement	from until loop
syn keyword eiffelAssertion	variant
syn match   eiffelAssertion	"\<invariant\>"
syn match   eiffelTopStruct	"^invariant\>"
syn keyword eiffelException	rescue retry

syn keyword eiffelPredefined	Current Void Result Precursor

" Operators
syn match   eiffelOperator	"\<and\(\s\+then\)\=\>"
syn match   eiffelOperator	"\<or\(\s\+else\)\=\>"
syn keyword eiffelOperator	xor implies not
syn keyword eiffelOperator	strip old
syn keyword eiffelOperator	Strip
syn match   eiffelOperator	"\$"
syn match   eiffelCreation	"!"
syn match   eiffelExport	"[{}]"
syn match   eiffelArray		"<<"
syn match   eiffelArray		">>"
syn match   eiffelConstraint	"->"
syn match   eiffelOperator	"[@#|&][^ \e\t\b%]*"

" Special classes
syn keyword eiffelAnchored	like
syn keyword eiffelBitType	BIT

" Constants
if !exists("eiffel_pedantic")
  syn keyword eiffelBool	true false
endif
syn keyword eiffelBool		True False
syn region  eiffelString	start=+"+ skip=+%"+ end=+"+ contains=eiffelStringEscape,eiffelStringError
syn match   eiffelStringEscape	contained "%[^/]"
syn match   eiffelStringEscape	contained "%/\d\+/"
syn match   eiffelStringEscape	contained "^[ \t]*%"
syn match   eiffelStringEscape	contained "%[ \t]*$"
syn match   eiffelStringError	contained "%/[^0-9]"
syn match   eiffelStringError	contained "%/\d\+[^0-9/]"
syn match   eiffelBadConstant	"'\(%[^/]\|%/\d\+/\|[^'%]\)\+'"
syn match   eiffelBadConstant	"''"
syn match   eiffelCharacter	"'\(%[^/]\|%/\d\+/\|[^'%]\)'" contains=eiffelStringEscape
syn match   eiffelNumber	"-\=\<\d\+\(_\d\+\)*\>"
syn match   eiffelNumber	"\<[01]\+[bB]\>"
syn match   eiffelNumber	"-\=\<\d\+\(_\d\+\)*\.\(\d\+\(_\d\+\)*\)\=\([eE][-+]\=\d\+\(_\d\+\)*\)\="
syn match   eiffelNumber	"-\=\.\d\+\(_\d\+\)*\([eE][-+]\=\d\+\(_\d\+\)*\)\="
syn match   eiffelComment	"--.*" contains=eiffelTodo

syn case match

" Case sensitive stuff

syn keyword eiffelTodo		contained TODO XXX FIXME
syn match   eiffelClassName	"\<[A-Z][A-Z0-9_]*\>"

" Catch mismatched parentheses
syn match eiffelParenError	")"
syn region eiffelParen		transparent start="(" end=")" contains=ALLBUT,eiffelParenError,eiffelStringError,eiffelStringEscape

" Should suffice for even very long strings and expressions
syn sync lines=40

if !exists("did_eiffel_syntax_inits")
  let did_eiffel_syntax_inits = 1
  " The default methods for hilighting.  Can be overridden later
  hi link eiffelKeyword		Statement
  hi link eiffelProperty	Statement
  hi link eiffelInheritClause	Statement
  hi link eiffelStatement	Statement
  hi link eiffelDeclaration	Statement
  hi link eiffelAssertion	Statement
  hi link eiffelDebug		Statement
  hi link eiffelException	Statement
  hi link eiffelGenericCreate	Statement


  hi link eiffelTopStruct	PreProc

  hi link eiffelAll		Special
  hi link eiffelAnchored	Special
  hi link eiffelBitType		Special


  hi link eiffelBool		Boolean
  hi link eiffelString		String
  hi link eiffelCharacter	Character
  hi link eiffelClassName	Type
  hi link eiffelNumber		Number

  hi link eiffelStringEscape	Special

  hi link eiffelOperator	Special
  hi link eiffelArray		Special
  hi link eiffelExport		Special
  hi link eiffelCreation	Special
  hi link eiffelBrackets	Special
  hi link eiffelGeneric		Special
  hi link eiffelGenericDecl	Special
  hi link eiffelConstraint	Special
  hi link eiffelCreate		Special

  hi link eiffelPredefined	Constant

  hi link eiffelComment		Comment

  hi link eiffelError		Error
  hi link eiffelBadConstant	Error
  hi link eiffelStringError	Error
  hi link eiffelParenError	Error
  hi link eiffelBracketError	Error

  hi link eiffelTodo		Todo
endif

let b:current_syntax = "eiffel"

" vim: ts=8
