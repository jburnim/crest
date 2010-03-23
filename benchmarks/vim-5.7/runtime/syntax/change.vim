" Vim syntax file
" Language:	WEB Changes
" Maintainer:	Andreas Scherer <andreas.scherer@pobox.com>
" Last Change:	August 23, 1999

" Details of the change mechanism of the WEB and CWEB languages can be found
" in the articles by Donald E. Knuth and Silvio Levy cited in "web.vim" and
" "cweb.vim" respectively.

" Remove any old syntax stuff hanging around
syn clear

" We distinguish two groups of material, (a) stuff between @x..@y, and
" (b) stuff between @y..@z. WEB/CWEB ignore everything else in a change file.
syn region changeFromMaterial start="^@x.*$"ms=e+1 end="^@y.*$"me=s-1
syn region changeToMaterial start="^@y.*$"ms=e+1 end="^@z.*$"me=s-1

if !exists("did_change_syntax_inits")
  let did_change_syntax_inits = 1
  " The default methods for highlighting. Can be overridden later.
  hi link changeFromMaterial String
  hi link changeToMaterial Statement
endif

let b:current_syntax = "change"

" vim: ts=8
