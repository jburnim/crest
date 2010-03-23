" Vim syntax file
" Language:	SNNS network file
" Maintainer:	Davide Alberani <alberanid@bigfoot.com>
" Last Change:	29 Jan 2000
" Version:	0.1
" URL:		http://members.xoom.com/alberanid/vim/syntax/snnsnet.vim
"
" SNNS http://www-ra.informatik.uni-tuebingen.de/SNNS/
" is a simulator for neural networks.

" Remove any old syntax stuff hanging around
syn clear

syn match	snnsnetTitle	"no\."
syn match	snnsnetTitle	"type name"
syn match	snnsnetTitle	"unit name"
syn match	snnsnetTitle	"act\( func\)\="
syn match	snnsnetTitle	"out func"
syn match	snnsnetTitle	"site\( name\)\="
syn match	snnsnetTitle	"site function"
syn match	snnsnetTitle	"source:weight"
syn match	snnsnetTitle	"unitNo\."
syn match	snnsnetTitle	"delta x"
syn match	snnsnetTitle	"delta y"
syn keyword	snnsnetTitle	typeName unitName bias st position subnet layer sites name target z LLN LUN Toff Soff Ctype

syn match	snnsnetType	"SNNS network definition file [Vv]\d.\d.*" contains=snnsnetNumbers
syn match	snnsnetType	"generated at.*" contains=snnsnetNumbers
syn match	snnsnetType	"network name\s*:"
syn match	snnsnetType	"source files\s*:"
syn match	snnsnetType	"no\. of units\s*:.*" contains=snnsnetNumbers
syn match	snnsnetType	"no\. of connections\s*:.*" contains=snnsnetNumbers
syn match	snnsnetType	"no\. of unit types\s*:.*" contains=snnsnetNumbers
syn match	snnsnetType	"no\. of site types\s*:.*" contains=snnsnetNumbers
syn match	snnsnetType	"learning function\s*:"
syn match	snnsnetType	"pruning function\s*:"
syn match	snnsnetType	"subordinate learning function\s*:"
syn match	snnsnetType	"update function\s*:"

syn match	snnsnetSection	"unit definition section"
syn match	snnsnetSection	"unit default section"
syn match	snnsnetSection	"site definition section"
syn match	snnsnetSection	"type definition section"
syn match	snnsnetSection	"connection definition section"
syn match	snnsnetSection	"layer definition section"
syn match	snnsnetSection	"subnet definition section"
syn match	snnsnetSection	"3D translation section"
syn match	snnsnetSection	"time delay section"

syn match	snnsnetNumbers	"\d" contained
syn match	snnsnetComment	"#.*$" contains=snnsnetTodo
syn keyword	snnsnetTodo	TODO XXX FIXME contained

if !exists("did_snnsnet_syntax_inits")
  let did_snnsnet_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link snnsnetType		Type
  hi link snnsnetComment	Comment
  hi link snnsnetNumbers	Number
  hi link snnsnetSection	Statement
  hi link snnsnetTitle		Label
  hi link snnsnetTodo		Todo
endif

let b:current_syntax = "snnsnet"

" vim: ts=8
