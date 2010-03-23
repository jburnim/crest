" Vim syntax file
" Language:		configure.in script: M4 with sh
" Maintainer:	Christian Hammesr <ch@lathspell.westend.com>
" Last Change:	2000 Apr 15

" Well, I actually even do not know much about m4. This explains why there
" is probably very much missing here, yet !
" But I missed a good hilighting when editing my GNU autoconf/automake
" script, so I wrote this quick and dirty patch.


" remove any old syntax stuff hanging around
syn clear

" define the config syntax
syn match   configdelimiter "[()\[\];,]"
syn match   configoperator  "[=|&\*\+\<\>]"
syn match   configcomment   "\(dnl.*\)\|\(#.*\)"
syn match   configfunction  "\<[A-Z_][A-Z0-9_]*\>"
syn match   confignumber    "[-+]\=\<\d\+\(\.\d*\)\=\>"
syn keyword configkeyword   if then else fi test for in do done
syn keyword configspecial   cat rm eval
syn region  configstring    start=+"+ skip=+\\"+ end=+"+
syn region  configstring    start=+`+ skip=+\\'+ end=+'+
syn region  configstring    start=+`+ skip=+\\'+ end=+`+

if !exists("did_config_syntax_inits")
  let did_config_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link configdelimiter Delimiter
  hi link configoperator  Operator
  hi link configcomment   Comment
  hi link configfunction  Function
  hi link confignumber    Number
  hi link configkeyword   Keyword
  hi link configspecial   Special
  hi link configstring    String
endif

let b:current_syntax = "config"

" vim: ts=4
