" Vim syntax file
" Language:	Motif UIL (User Interface Language)
" Maintainer:	Thomas Koehler <jean-luc@picard.franken.de>
" Last Change:	1998 February 17

syn clear

" A bunch of useful keywords
syn keyword uilType	arguments	callbacks	color
syn keyword uilType	compound_string	controls	end
syn keyword uilType	exported	file		include
syn keyword uilType	module		object		procedure
syn keyword uilType	user_defined	xbitmapfile

syn keyword uilTodo contained	TODO

" String and Character contstants
" Highlight special characters (those which have a backslash) differently
syn match   uilSpecial contained "\\\d\d\d\|\\."
syn region  uilString		start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=uilSpecial
syn match   uilCharacter	"'[^\\]'"
syn region  uilString		start=+'+  skip=+\\\\\|\\"+  end=+'+  contains=uilSpecial
syn match   uilSpecialCharacter	"'\\.'"
syn match   uilSpecialStatement	"Xm[^ =(){}]*"
syn match   uilSpecialFunction	"MrmNcreateCallback"
syn match   uilRessource	"XmN[^ =(){}]*"

syn match  uilNumber		"-\=\<\d*\.\=\d\+\(e\=f\=\|[uU]\=[lL]\=\)\>"
syn match  uilNumber		"0[xX][0-9a-fA-F]\+\>"

syn region uilComment		start="/\*"  end="\*/" contains=uilTodo
syn match  uilComment		"!.*" contains=uilTodo
syn match  uilCommentError	"\*/"

syn region uilPreCondit		start="^#\s*\(if\>\|ifdef\>\|ifndef\>\|elif\>\|else\>\|endif\>\)"  skip="\\$"  end="$" contains=uilComment,uilString,uilCharacter,uilNumber,uilCommentError
syn match  uilIncluded contained "<[^>]*>"
syn match  uilInclude		"^#\s*include\s\+." contains=uilString,uilIncluded
syn match  uilLineSkip		"\\$"
syn region uilDefine		start="^#\s*\(define\>\|undef\>\)" end="$" contains=uilLineSkip,uilComment,uilString,uilCharacter,uilNumber,uilCommentError

syn sync ccomment uilComment

if !exists("did_uil_syntax_inits")
  let did_uil_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link uilCharacter		uilString
  hi link uilSpecialCharacter	uilSpecial
  hi link uilNumber		uilString
  hi link uilCommentError	uilError
  hi link uilInclude		uilPreCondit
  hi link uilDefine		uilPreCondit
  hi link uilIncluded		uilString
  hi link uilSpecialFunction	uilRessource
  hi link uilRessource		Identifier
  hi link uilSpecialStatement	Keyword
  hi link uilError		Error
  hi link uilPreCondit		PreCondit
  hi link uilType		Type
  hi link uilString		String
  hi link uilComment		Comment
  hi link uilSpecial		Special
  hi link uilTodo		Todo
endif

let b:current_syntax = "uil"

" vim: ts=8
