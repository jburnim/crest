" Vim syntax file
" Language:	exports
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	May 18, 1998
" Notes:		This file includes both SysV and BSD 'isms

" Remove any old syntax stuff hanging around
syn clear

" Options: -word
syn keyword exportsKeyOptions contained	alldirs	nohide	ro	wsync
syn keyword exportsKeyOptions contained	kerb	o	rw
syn match exportsOptError contained	"[a-z]\+"

" Settings: word=
syn keyword exportsKeySettings contained	access	anon	root	rw
syn match exportsSetError contained	"[a-z]\+"

" OptSet: -word=
syn keyword exportsKeyOptSet contained	mapall	maproot	mask	network
syn match exportsOptSetError contained	"[a-z]\+"

" options and settings
syn match exportsSettings	"[a-z]\+="  contains=exportsKeySettings,exportsSetError
syn match exportsOptions	"-[a-z]\+"  contains=exportsKeyOptions,exportsOptError
syn match exportsOptSet	"-[a-z]\+=" contains=exportsKeyOptSet,exportsOptSetError

" Separators
syn match exportsSeparator	"[,:]"

" comments
syn match exportsComment	"^\s*#.*$"

if !exists("did_exports_syntax_inits")
  let did_exports_syntax_inits = 1
  hi link exportsKeyOptSet	exportsKeySettings
  hi link exportsOptSet	exportsSettings

  hi link exportsComment	Comment
  hi link exportsKeyOptions	Type
  hi link exportsKeySettings	Keyword
  hi link exportsOptions	Constant
  hi link exportsSeparator	Constant
  hi link exportsSettings	Constant

  hi link exportsOptError	Error
  hi link exportsOptSetError	Error
  hi link exportsSetError	Error
endif

let b:current_syntax = "exports"
" vim: ts=10
