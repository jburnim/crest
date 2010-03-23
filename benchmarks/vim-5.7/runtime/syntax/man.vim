" Vim syntax file
" Language:	Man page
" Maintainer:	Gautam H. Mudunuri <gmudunur@informatica.com>
" Last Change:	1999 Jun 14
" Version Info:

" clear any unwanted syntax defs
syn clear

syn case ignore
syn match  manReference       "[a-z][a-z0-9_]*([1-9][a-z]\{0,1})"
syn match  manTitle           "^\i\+([0-9]\+[a-z]\=).*"
syn match  manSectionHeading  "^[a-z][a-z ]*[a-z]$"
syn match  manOptionDesc      "^\s*[+-][a-z0-9]\S*"
" syn match  manHistory         "^[a-z].*last change.*$"

if !exists("did_man_syntax_inits")
        let did_man_syntax_inits = 1
        " The default methods for highlighting.  Can be overridden later
        hi link manTitle           Title
        hi link manSectionHeading  Statement
        hi link manOptionDesc      Constant
        " hi link manHistory         Comment
        hi link manReference       PreProc
endif

let b:current_syntax = "man"

" vim:ts=8
