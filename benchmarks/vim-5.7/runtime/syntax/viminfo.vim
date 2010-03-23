" Vim syntax file
" Language:	Vim .viminfo file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	1999 Aug 19

" Remove any old syntax stuff hanging around
syn clear

" The lines that are NOT recognized
syn match viminfoError "^[^\t].*"

" The one-character one-liners that are recognized
syn match viminfoStatement "^[/&$@:?=%!<]"

" The two-character one-liners that are recognized
syn match viminfoStatement "^['>"]."
syn match viminfoStatement "^\~[/&]"
syn match viminfoStatement "^\~[hH]"
syn match viminfoStatement "^\~[mM][sS][lL][eE]\d\+\~\=[/&]"

" Comments
syn match viminfoComment "^#.*"

if !exists("did_viminfo_syntax_inits")
  let did_viminfo_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link viminfoComment		Comment
  hi link viminfoError		Error
  hi link viminfoStatement	Statement
endif

let b:current_syntax = "viminfo"

" vim: ts=8
