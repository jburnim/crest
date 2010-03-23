" Vim syntax file
" Language:	avenue
" Maintainer:	Jan-Oliver Wagner <Jan-Oliver.Wagner@usf.Uni-Osnabrueck.DE>
" Last Change:	1998 September 11

" Avenue is the ArcView built-in language. ArcView is
" a desktop GIS by ESRI. Though it is a built-in language
" and a built-in editor is provided, the use of VIM increases
" development speed.
" I use some technologies to automatically load avenue scripts
" into ArcView.

" Remove any old syntax stuff hanging around
syn clear

" Avenue is entirely case-insensitive.
syn case ignore

" The keywords

syn keyword aveStatement	if then elseif else end break exit return
syn keyword aveStatement	for each in continue while

" String

syn region aveString		start=+"+ end=+"+

" Integer number
syn match  aveNumber		"[+-]\=\<[0-9]\+\>"

" Operator

syn keyword aveOperator		or and max min xor mod by

" Variables

syn keyword aveFixVariables	av nil self false true nl tab cr tab
syn match globalVariables	"_[a-zA-Z][a-zA-Z0-9]*"
syn match aveVariables		"[a-zA-Z][a-zA-Z0-9_]*"
syn match aveConst		"#[A-Z][A-Z_]+"

" Procedures (class methods) - not working, why?

syn match avClassMethods	"\.[a-zA-Z][a-zA-Z]+"

" Comments

syn match aveComment	"'.*"

" Typical Typos

" for C programmers:
syn match aveTypos	"=="
syn match aveTypos	"!="

if !exists("did_ave_syntax_inits")
  let did_ave_syntax_inits = 1

  hi link aveStatement		Statement

  hi link aveString		String
  hi link aveNumber		Number

  hi link aveFixVariables	Special
  hi link aveVariables		Identifier
  hi link globalVariables	Special
  hi link aveConst		Special

  hi link aveClassMethods	Function

  hi link aveOperators		Operator
  hi link aveComment		Comment

  hi link aveTypos		Error
endif

let b:current_syntax = "ave"

" vim: ts=8
