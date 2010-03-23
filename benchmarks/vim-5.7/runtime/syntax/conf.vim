" Vim syntax file
" Language:	generic configure file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2000 Feb 11

" remove any old syntax stuff hanging around
syn clear

syn keyword	confTodo	contained TODO FIXME XXX
" Avoid matching "text#text", used in /etc/disktab and /etc/gettytab
syn match	confComment	"^#.*" contains=confTodo
syn match	confComment	"\s#.*"ms=s+1 contains=confTodo
syn region	confString	start=+"+ skip=+\\\\\|\\"+ end=+"+ oneline
syn region	confString	start=+'+ skip=+\\\\\|\\'+ end=+'+ oneline

if !exists("did_conf_syntax_inits")
  let did_conf_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link confComment	Comment
  hi link confTodo	Todo
  hi link confString	String
endif

let b:current_syntax = "conf"

" vim: ts=8
