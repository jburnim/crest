" Vim syntax file
" Language:	Microsoft Module-Definition (.def) File
" Maintainer:	Rob Brady <robb@datatone.com>
" Last Change:	$Date: 1999/08/17 00:19:18 $
" URL: http://www.datatone.com/~robb/vim/syntax/def.vim
" $Revision: 1.3 $

" Remove any old syntax stuff hanging around
syn clear

syn case ignore

syn match defComment	";.*"

syn keyword defKeyword	LIBRARY STUB EXETYPE DESCRIPTION CODE WINDOWS DOS
syn keyword defKeyword	RESIDENTNAME PRIVATE EXPORTS IMPORTS SEGMENTS
syn keyword defKeyword	HEAPSIZE DATA
syn keyword defStorage	LOADONCALL MOVEABLE DISCARDABLE SINGLE
syn keyword defStorage	FIXED PRELOAD

syn match   defOrdinal	"@\d\+"

syn region  defString	start=+'+ end=+'+

syn match   defNumber	"\d+"
syn match   defNumber	"0x\x\+"


if !exists("did_def_syntax_inits")
  let did_def_syntax_inits = 1
  hi link defComment	Comment
  hi link defKeyword	Keyword
  hi link defStorage	StorageClass
  hi link defString	String
  hi link defNumber	Number
  hi link defOrdinal	Operator
endif

let b:current_syntax = "def"

" vim: ts=8
