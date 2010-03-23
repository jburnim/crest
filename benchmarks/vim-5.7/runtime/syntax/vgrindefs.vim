" Vim syntax file
" Language:	Vgrindefs
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	1997 Sep 14

" The Vgrindefs file is used to specify a language for vgrind

" Remove any old syntax stuff hanging around
syn clear

" Comments
syn match vgrindefsComment "^#.*"

" The fields that vgrind recognizes
syn match vgrindefsField ":ab="
syn match vgrindefsField ":ae="
syn match vgrindefsField ":pb="
syn match vgrindefsField ":bb="
syn match vgrindefsField ":be="
syn match vgrindefsField ":cb="
syn match vgrindefsField ":ce="
syn match vgrindefsField ":sb="
syn match vgrindefsField ":se="
syn match vgrindefsField ":lb="
syn match vgrindefsField ":le="
syn match vgrindefsField ":nc="
syn match vgrindefsField ":tl"
syn match vgrindefsField ":oc"
syn match vgrindefsField ":kw="

" Also find the ':' at the end of the line, so all ':' are highlighted
syn match vgrindefsField ":\\$"
syn match vgrindefsField ":$"
syn match vgrindefsField "\\$"

if !exists("did_vgrindefs_syntax_inits")
  let did_vgrindefs_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link vgrindefsField		Statement
  hi link vgrindefsComment	Comment
endif

let b:current_syntax = "vgrindefs"

" vim: ts=8
