" Vim syntax file
" Language:	RCS log output
" Maintainer:	Joe Karthauser <joe@freebsd.org>
" Last Change:	1999 Jan 02

" Remove any old syntax stuff hanging around
syn clear

syn match rcslogRevision	"^revision.*$"
syn match rcslogFile		"^RCS file:.*"
syn match rcslogDate		"^date: .*$"

if !exists("did_rcslog_syntax_inits")
  let did_rcslog_syntax_inits = 1

  hi link rcslogFile		Type
  hi link rcslogRevision	Constant
  hi link rcslogDate		Identifier

endif

let b:current_syntax = "rcslog"

" vim: ts=8
