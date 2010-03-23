" Vim syntax file
" Language:	Abaqus finite element input file
" Maintainer:	Carl Osterwisch <osterwischc@asme.org>
" Last Change:	8th Feb 2000

" Remove any old syntax stuff hanging around
syn clear

" Abaqus comment lines
syn match abaqusComment	"^\*\*.*$"

" Abaqus keyword lines
syn match abaqusKeywordLine "^\*\h.*" contains=abaqusKeyword,abaqusParameter,abaqusValue
syn match abaqusKeyword "^\*\h[^,]*" contained
syn match abaqusParameter ",[^,=]\+"lc=1 contained
syn match abaqusValue 	"=\s*[^,]*"lc=1 contained

" Illegal syntax
syn match abaqusBadLine	"^\s\+\*.*"

if !exists("did_abaqus_syntax_inits")
	let did_abaqus_syntax_inits = 1
	" The default methods for highlighting.  Can be overridden later
	hi link abaqusComment	Comment
	hi link abaqusKeyword	Statement
	hi link abaqusParameter	Identifier
	hi link abaqusValue	Constant
	hi link abaqusBadLine Error
endif

let b:current_syntax = "abaqus"
