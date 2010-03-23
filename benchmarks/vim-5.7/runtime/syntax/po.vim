" Vim syntax file
" Language:	po (GNU gettext)
" Maintainer:	Sung-Hyun Nam <namsh@kldp.org>
" Last Change:	1999/12/14

" remove any old syntax stuff hanging around
syn clear

syn match  poComment	"^#.*$"
syn match  poSources	"^#:.*$"
syn match  poStatement	"^\(msgid\|msgstr\)"
syn match  poSpecial	contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"
syn match  poFormat	"%\(\d\+\$\)\=[-+' #0*]*\(\d*\|\*\|\*\d\+\$\)\(\.\(\d*\|\*\|\*\d\+\$\)\)\=\([hlL]\|ll\)\=\([diuoxXfeEgGcCsSpn]\|\[\^\=.[^]]*\]\)" contained
syn match  poFormat	"%%" contained
syn region poString	start=+"+ skip=+\\\\\|\\"+ end=+"+
			\ contains=poSpecial,poFormat

if !exists("did_po_syntax_inits")
  let did_po_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later.
  hi link poComment	Comment
  hi link poSources	PreProc
  hi link poStatement	Statement
  hi link poSpecial	Special
  hi link poFormat	poSpecial
  hi link poString	String
endif

let b:current_syntax = "po"

" vim:set ts=8 sts=8 sw=8 noet:
