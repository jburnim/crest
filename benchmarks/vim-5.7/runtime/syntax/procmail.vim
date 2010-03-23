" Vim syntax file
" Language:	Procmail definition file
" Maintainer:	vacancy [posted by Sonia Heimann, but she didn't feel like
"		maintaining this]
" Last Change:	1998 Apr 20

" Remove any old syntax stuff hanging around
syn clear

syn match   procmailComment      "#.*$" contains=procmailTodo
syn keyword   procmailTodo      contained Todo TBD

syn region  procmailString       start=+"+  skip=+\\"+  end=+"+

syn region procmailVarDeclRegion start="^\s*[a-zA-Z0-9_]\+\s*="hs=e-1 skip=+\\$+ end=+$+ contains=procmailVar,procmailVarDecl,procmailString
syn match procmailVarDecl contained "^\s*[a-zA-Z0-9_]\+"
syn match procmailVar "$[a-zA-Z0-9_]\+"

syn match procmailCondition contained "^\s*\*.*"

syn match procmailActionFolder contained "^\s*[-_a-zA-Z/]\+"
syn match procmailActionVariable contained "^\s*$[a-zA-Z_]\+"
syn region procmailActionForward start=+^\s*!+ skip=+\\$+ end=+$+
syn region procmailActionPipe start=+^\s*|+ skip=+\\$+ end=+$+
syn region procmailActionNested start=+^\s*{+ end=+^\s*}+ contains=procmailRecipe,procmailComment,procmailVarDeclRegion

syn region procmailRecipe start=+^\s*:.*$+ end=+^\s*\($\|}\)+me=e-1 contains=procmailComment,procmailCondition,procmailActionFolder,procmailActionVariable,procmailActionForward,procmailActionPipe,procmailActionNested,procmailVarDeclRegion

if !exists("did_procmail_syntax_inits")
  "let did_procmail_syntax_inits = 1
  hi link procmailComment Comment
  hi link procmailTodo    Todo

  hi link procmailRecipe   Statement
  "highlight link procmailCondition   Statement

  hi link procmailActionFolder procmailAction
  hi link procmailActionVariable procmailAction
  hi link procmailActionForward procmailAction
  hi link procmailActionPipe procmailAction
  hi link procmailAction	Function
  hi link procmailVar		Identifier
  hi link procmailVarDecl	Identifier

  hi link procmailString String
endif

let b:current_syntax = "procmail"

" vim: ts=8
