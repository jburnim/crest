" Vim syntax file
" Language:	lilo
" Maintainer:	Klaus Muth <mh@hagos.de>
" Last Change:	1999 Jun 14
" URL:		http://unitopia.mud.de/~monty/vim/syntax/lilo.vim
"
" Sorry, not much different colors in it. Any line without color must be
" an error (eiter in the syntax file or in the lilo.conf file ;).

" Remove any old syntax stuff hanging around.
syn clear

syn case match

" global options /w parameter
syn keyword	liloPOption	backup boot default delay disk disktab
syn keyword	liloPOption	initrd install map message
syn keyword	liloPOption	password serial timeout
syn keyword	liloPOption	verbose
syn match	liloPOption	"force-backup"

" global options /wo parameter
syn keyword	liloOption	compact linear
syn keyword	liloOption	lock nowarn optional prompt restricted
syn match	liloOption	"fix-table"
syn match	liloOption	"ignore-table"

" per-image /w parameter:
syn keyword	liloPOption	range loader table label alias append
syn keyword	liloPOption	literal ramdisk root vga

" per-image /wo parameter:
syn keyword	liloOption	unsafe
syn match	liloOption	"read-only"
syn match	liloOption	"read-write"

syn match	liloImage	"\s*\(image\|other\)\s*=.*$"
syn match	liloPath	"\/.*"
syn match	liloNumber	"\d\+"

if !exists("did_lilo_syntax_inits")
  let did_lilo_syntax_inits = 1
  " The default methods for highlighting. Can be overridden later.
  hi link liloPOption	Keyword
  hi link liloOption	Keyword
  hi link liloPath	Constant
  hi link liloNumber	Number
  hi link liloImage	Title
endif

let b:current_syntax = "lilo"

" vim: ts=8
