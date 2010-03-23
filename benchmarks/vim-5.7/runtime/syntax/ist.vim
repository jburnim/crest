" Vim syntax file
" Language:	Makeindex style file, *.ist
" Maintainer:	Peter Meszaros <pmeszaros@effice.hu>
" Last Change:	October 9, 1999

" Removes any old syntax stuff hanging around
syn clear

set iskeyword=$,@,48-57,_

syn case ignore
syn keyword IstInpSpec  actual  arg_close arg_open encap       escape
syn keyword IstInpSpec  keyword level     quote    range_close range_open
syn keyword IstInpSpec  page_compositor

syn keyword IstOutSpec  preamble         postamble        setpage_prefix   setpage_suffix   group_skip
syn keyword IstOutSpec  headings_flag    heading_prefix   heading_suffix
syn keyword IstOutSpec  lethead_flag     lethead_prefix   lethead_suffix
syn keyword IstOutSpec  symhead_positive symhead_negative numhead_positive numhead_negative
syn keyword IstOutSpec  item_0           item_1           item_2           item_01
syn keyword IstOutSpec  item_x1          item_12          item_x2
syn keyword IstOutSpec  delim_0          delim_1          delim_2
syn keyword IstOutSpec  delim_n          delim_r          delim_t
syn keyword IstOutSpec  encap_prefix     encap_infix      encap_suffix
syn keyword IstOutSpec  line_max         indent_space     indent_length
syn keyword IstOutSpec  suffix_2p        suffix_3p        suffix_mp


syn region  IstString      matchgroup=IstDoubleQuote start=+"+ skip=+\\"+ end=+"+ contains=IstSpecial
syn match   IstCharacter   "'.'"
syn match   IstNumber      "\d\+"
syn match   IstComment     "^[\t ]*%.*$"         contains=IstTodo
syn match   IstSpecial     "\\\\\|{\|}\|#\|\\n"  contained
syn match   IstTodo        "DEBUG\|TODO"         contained


if !exists("did_ist_syntax_inits")
 let did_ist_syntax_inits = 1

 hi link IstInpSpec	Type
 hi link IstOutSpec	Identifier
 hi link IstString	String
 hi link IstNumber	Number
 hi link IstComment	Comment
 hi link IstTodo	Todo
 hi link IstSpecial	Special
 hi link IstDoubleQuote	Label
 hi link IstCharacter	Label

endif

let b:current_syntax = "ist"

" vim: ts=16
