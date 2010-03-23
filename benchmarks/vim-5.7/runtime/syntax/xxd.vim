" Vim syntax file
" Language:		bin using xxd
" Version:		5.4-3
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	March 19, 1999
"  Notes: use :help xxd   to see how to invoke it

" Removes any old syntax stuff hanging around
syn clear

syn match xxdAddress			"^[0-9a-f]\+:"		contains=xxdSep
syn match xxdSep	contained	":"
syn match xxdAscii				"  .\{,16\}\r\=$"hs=s+2	contains=xxdDot
syn match xxdDot	contained	"[.\r]"

if !exists("did_xxd_syntax_inits")
 let did_xxd_syntax_inits = 1

 hi link xxdAddress	Constant
 hi link xxdSep		Identifier
 hi link xxdAscii	Statement
endif

let b:current_syntax = "xxd"

" vim: ts=4
