" Vim syntax file
" Language:	Elm Filter rules
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	November 11, 1998

" remove any old syntax stuff hanging around
syn clear

syn keyword	elmfiltAction	delete execute executec forward forwardc leave save savecopy
syn match	elmfiltArg	"[^\\]%[&0-9]"lc=1	contained
syn keyword	elmfiltCond	lines always subject sender from to lines received
syn region	elmfiltMatch	start="/" skip="\\/" end="/"
syn match	elmfiltNumber	"\d\+"
syn keyword	elmfiltOper	and not matches
syn match	elmfiltOper	"\~"
syn match	elmfiltOper	"<=\|>=\|!=\|<\|<\|=\|(\|)"
syn keyword	elmfiltRule	if then
syn region	elmfiltString	start='"' skip='"\(\\\\\)*\\"' end='"'	contains=elmfiltArg
syn match	elmfiltComment	"^#.*$"

if !exists("did_elmfilt_syntax_inits")
  let did_elmfilt_syntax_inits= 1
  hi link elmfiltAction	Statement
  hi link elmfiltArg	Special
  hi link elmfiltComment	Comment
  hi link elmfiltCond	Type
  hi link elmfiltMatch	Special
  hi link elmfiltNumber	Number
  hi link elmfiltOper	Operator
  hi link elmfiltRule	Statement
  hi link elmfiltString	String
  endif
let b:current_syntax = "elmfilt"
" vim: ts=9
