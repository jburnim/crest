" Vim syntax file
" Language:		Clean
" Author:		Pieter van Engelen <pietere@sci.kun.nl>
" Co-Author:	Arthur van Leeuwen <arthurvl@sci.kun.nl>
" Last Change:	Fri Feb  6 15:02:40 CET 1998

syn clear

" Some Clean-keywords
syn keyword cleanConditional if case
syn keyword cleanLabel let! with where in of
syn keyword cleanInclude from import
syn keyword cleanSpecial Start
syn keyword cleanKeyword infixl infixr infix
syn keyword cleanBasicType Int Real Char Bool String
syn keyword cleanSpecialType World ProcId Void Files File
syn keyword cleanModuleSystem module implementation definition system
syn keyword cleanTypeClass class instance export

" To do some Denotation Highlighting
syn keyword cleanBoolDenot True False
syn region  cleanStringDenot start=+"+ end=+"+
syn match cleanCharDenot "'.'"
syn match cleanCharsDenot "'.*'" contained
syn match cleanIntegerDenot "[+-~]\=\<\(\d\+\|0[0-7]\+\|0x[0-9A-Fa-f]\+\)\>"
syn match cleanRealDenot "[+-~]\=\<\d\+\.\d+\(E[+-~]\=\d+\)\="

" To highlight the use of lists, tuples and arrays
syn region cleanList start="\[" end="\]" contains=ALL
syn region cleanRecord start="{" end="}" contains=ALL
syn region cleanArray start="{:" end=":}" contains=ALL
syn match cleanTuple "([^=]*,[^=]*)" contains=ALL

" To do some Comment Highlighting
syn region cleanComment start="/\*"  end="\*/"
syn match cleanComment "//.*"

" Now for some useful typedefinitionrecognition
syn match cleanFuncTypeDef "\([a-zA-Z].*\|(\=[-~@#$%^?!+*<>\/|&=:]\+)\=\)[ \t]*\(infix[lr]\=\)\=[ \t]*\d\=[ \t]*::.*->.*" oneline contains=cleanSpecial

if !exists("did_clean_syntax_init")
   let did_clean_syntax_init = 1
   " Comments
   hi link cleanComment      Comment
   " Constants and denotations
   hi link cleanCharsDenot   String
   hi link cleanStringDenot  String
   hi link cleanCharDenot    Character
   hi link cleanIntegerDenot Number
   hi link cleanBoolDenot    Boolean
   hi link cleanRealDenot    Float
   " Identifiers
   " Statements
   hi link cleanTypeClass    Keyword
   hi link cleanConditional  Conditional
   hi link cleanLabel        Label
   hi link cleanKeyword      Keyword
   " Generic Preprocessing
   hi link cleanInclude      Include
   hi link cleanModuleSystem PreProc
   " Type
   hi link cleanBasicType    Type
   hi link cleanSpecialType  Type
   hi link cleanFuncTypeDef  Typedef
   " Special
   hi link cleanSpecial      Special
   hi link cleanList         Special
   hi link cleanArray        Special
   hi link cleanRecord       Special
   hi link cleanTuple        Special
   " Error
   " Todo
endif

let b:current_syntax = "clean"

" vim: ts=4
