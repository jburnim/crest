" Vim syntax file
" Language:	CTRL-H (e.g., ASCII manpages)
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	1999 Oct 25

" Existing syntax is kept, this file can be used as an addition

" Recognize underlined text: _^Hx
syntax match CtrlHUnderline /_\b./  contains=CtrlHHide

" Recognize bold text: x^Hx
syntax match CtrlHBold /\(.\)\b\1/  contains=CtrlHHide

" Hide the CTRL-H (backspace)
syntax match CtrlHHide /.\b/  contained

if !exists("did_ctrlh_syntax_inits")
  let did_ctrlh_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link CtrlHHide Ignore
  hi CtrlHUnderline term=underline cterm=underline gui=underline
  hi CtrlHBold term=bold cterm=bold gui=bold
endif

" vim: ts=8
