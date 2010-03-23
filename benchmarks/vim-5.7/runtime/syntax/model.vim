" Vim syntax file
" Language:	Model
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	1997 Sep 14

" very basic things only (based on the vgrindefs file).
" If you use this language, please improve it, and send me the patches!

" Remove any old syntax stuff hanging around
syn clear

" A bunch of keywords
syn keyword modelKeyword abs and array boolean by case cdnl char copied dispose
syn keyword modelKeyword div do dynamic else elsif end entry external FALSE false
syn keyword modelKeyword fi file for formal fortran global if iff ift in integer include
syn keyword modelKeyword inline is lbnd max min mod new NIL nil noresult not notin od of
syn keyword modelKeyword or procedure public read readln readonly record recursive rem rep
syn keyword modelKeyword repeat res result return set space string subscript such then TRUE
syn keyword modelKeyword true type ubnd union until varies while width

" Special keywords
syn keyword modelBlock beginproc endproc

" Comments
syn region modelComment start="\$" end="\$" end="$"

" Strings
syn region modelString start=+"+ end=+"+

" Character constant (is this right?)
syn match modelString "'."

if !exists("did_model_syntax_inits")
  let did_model_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link modelKeyword	Statement
  hi link modelBlock	PreProc
  hi link modelComment	Comment
  hi link modelString	String
endif

let b:current_syntax = "model"

" vim: ts=8
