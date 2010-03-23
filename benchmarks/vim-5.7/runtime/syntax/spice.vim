" Vim syntax file
" Language:	Spice circuit simulator input netlist
" Maintainer:	Noam Halevy <Noam.Halevy.motorola.com>
" Last Change:	12/08/99
"
" This is based on sh.vim by Lennart Schultz
" but greatly simplified

" Remove any old syntax stuff hanging around
syn clear
" spice syntax is case INsensitive
syn case ignore

syn keyword	spiceTodo	contained TODO

syn match spiceComment  "^ \=\*.*$"
syn match spiceComment  "\$.*$"

" Numbers, all with engineering suffixes and optional units
"==========================================================
"floating point number, with dot, optional exponent
syn match spiceNumber  "\<[0-9]\+\.[0-9]*\(e[-+]\=[0-9]\+\)\=\(meg\=\|[afpnumkg]\)\="
"floating point number, starting with a dot, optional exponent
syn match spiceNumber  "\.[0-9]\+\(e[-+]\=[0-9]\+\)\=\(meg\=\|[afpnumkg]\)\="
"integer number with optional exponent
syn match spiceNumber  "\<[0-9]\+\(e[-+]\=[0-9]\+\)\=\(meg\=\|[afpnumkg]\)\="

" Misc
"=====
syn match   spiceWrapLineOperator       "\\$"
syn match   spiceWrapLineOperator       "^+"

syn match   spiceStatement      "^ \=\.\I\+"

" Matching pairs of parentheses
"==========================================
syn region  spiceParen transparent matchgroup=spiceOperator start="(" end=")" contains=ALLBUT,spiceParenError
syn region  spiceSinglequote matchgroup=spiceOperator start=+'+ end=+'+

" Errors
"=======
syn match spiceParenError ")"

" Syncs
" =====
syn sync minlines=50

if !exists("did_spice_syntax_inits")
  let did_spice_syntax_inits = 1
" The default methods for highlighting.  Can be overridden later
  hi link spiceTodo	Todo
  hi link spiceWrapLineOperator spiceOperator
  hi link spiceSinglequote      spiceExpr
  hi link spiceExpr             Function
  hi link spiceParenError       Error
  hi link spiceStatement        Statement
  hi link spiceNumber           Number
  hi link spiceComment          Comment
  hi link spiceOperator         Operator
endif

let b:current_syntax = "spice"

" insert the following to $VIM/syntax/scripts.vim
" to autodetect HSpice netlists and text listing output:
"
" " Spice netlists and text listings
" elseif getline(1) =~ 'spice\>' || getline("$") =~ '^\.end'
"   so <sfile>:p:h/spice.vim

" vim: ts=8
