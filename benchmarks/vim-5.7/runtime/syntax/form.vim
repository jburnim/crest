" Vim syntax file
" Language:	FORM
" Maintainer:	Michael M. Tung <michael.tung@uni-mainz.de>
" Last Change:	2000 May 17

" First public release based on 'Symbolic Manipulation with FORM'
" by J.A.M. Vermaseren, CAN, Netherlands, 1991.
" This syntax file is still in development. Please send suggestions
" to the maintainer.

" Remove any old syntax stuff hanging around
syn clear

syn case ignore

" A bunch of useful FORM keywords
syn keyword formType		global local
syn keyword formHeaderStatement	symbol symbols cfunction cfunctions
syn keyword formHeaderStatement	function functions vector vectors
syn keyword formHeaderStatement	set sets index indices
syn keyword formHeaderStatement	dimension dimensions unittrace
syn keyword formStatement	id identify drop skip
syn keyword formStatement	write nwrite
syn keyword formStatement	format print nprint load save
syn keyword formStatement	bracket brackets
syn keyword formStatement	multiply count match only discard
syn keyword formStatement	trace4 traceN contract symmetrize antisymmetrize
syn keyword formConditional	if else endif while
syn keyword formConditional	repeat endrepeat label goto

" some special functions
syn keyword formStatement	g_ gi_ g5_ g6_ g7_ 5_ 6_ 7_
syn keyword formStatement	e_ d_ delta_ theta_ sum_ sump_

" pattern matching for keywords
syn match   formComment		"^\ *\*.*$"
syn match   formComment		"\;\ *\*.*$"
syn region  formString		start=+"+  end=+"+
syn region  formString		start=+'+  end=+'+
syn match   formPreProc		"^\=\#[a-zA-z][a-zA-Z0-9]*\>"
syn match   formNumber		"\<\d\+\>"
syn match   formNumber		"\<\d\+\.\d*\>"
syn match   formNumber		"\.\d\+\>"
syn match   formNumber		"-\d" contains=Number
syn match   formNumber		"-\.\d" contains=Number
syn match   formNumber		"i_\+\>"
syn match   formNumber		"fac_\+\>"
syn match   formDirective	"^\=\.[a-zA-z][a-zA-Z0-9]*\>"

" hi User Labels
syn sync ccomment formComment minlines=10

if !exists("did_form_syn_inits")
  let did_form_syn_inits = 1
  " The default methods for hi-ing.  Can be overridden later
  hi link formConditional	Conditional
  hi link formNumber		Number
  hi link formStatement		Statement
  hi link formComment		Comment
  hi link formPreProc		PreProc
  hi link formDirective		PreProc
  hi link formType		Type
  hi link formString		String

  if !exists("form_enhanced_color")
    hi link formHeaderStatement	Statement
  else
  " enhanced color mode
    hi link formHeaderStatement	HeaderStatement
    " dark and a light background for local types
    if &background == "dark"
      hi HeaderStatement term=underline ctermfg=LightGreen guifg=LightGreen gui=bold
    else
      hi HeaderStatement term=underline ctermfg=DarkGreen guifg=SeaGreen gui=bold
    endif
    " change slightly the default for dark gvim
    if has("gui_running") && &background == "dark"
      hi Conditional guifg=LightBlue gui=bold
      hi Statement guifg=LightYellow
    endif
  endif
endif

  let b:current_syntax = "form"

" vim: ts=8
