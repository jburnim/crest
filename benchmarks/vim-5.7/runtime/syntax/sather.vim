" Vim syntax file
" Language:	Sather/pSather
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/sather.vim
" Last Change:	1998 Jan 12

" Sather is a OO-language developped at the International Computer Science
" Institute (ICSI) in Berkeley, CA. pSather is a parallel extension to Sather.
" Homepage: http://www.icsi.berkeley.edu/~sather
" Sather files use .sa as suffix

" Remove any old syntax stuff hanging around
syn clear

" keyword definitions
syn keyword satherExternal       extern
syn keyword satherBranch         break continue
syn keyword satherLabel          when then
syn keyword satherConditional    if else elsif end case typecase assert with
syn match satherConditional      "near$"
syn match satherConditional      "far$"
syn match satherConditional      "near *[^(]"he=e-1
syn match satherConditional      "far *[^(]"he=e-1
syn keyword satherSynchronize    lock guard sync
syn keyword satherRepeat         loop parloop do
syn match satherRepeat           "while!"
syn match satherRepeat           "break!"
syn match satherRepeat           "until!"
syn keyword satherBoolValue      true false
syn keyword satherValue          self here cluster
syn keyword satherOperator       new "== != & ^ | && ||
syn keyword satherOperator       and or not
syn match satherOperator         "[#!]"
syn match satherOperator         ":-"
syn keyword satherType           void attr where
syn match satherType           "near *("he=e-1
syn match satherType           "far *("he=e-1
syn keyword satherStatement      return
syn keyword satherStorageClass   static const
syn keyword satherExceptions     try raise catch
syn keyword satherMethodDecl     is pre post
syn keyword satherClassDecl      abstract value class include
syn keyword satherScopeDecl      public private readonly


syn match   satherSpecial           contained "\\\d\d\d\|\\."
syn region  satherString            start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=satherSpecial
syn match   satherCharacter         "'[^\\]'"
syn match   satherSpecialCharacter  "'\\.'"
syn match   satherNumber          "-\=\<\d\+L\=\>\|0[xX][0-9a-fA-F]\+\>"
syn match   satherCommentSkip     contained "^\s*\*\($\|\s\+\)"
syn region  satherComment2String  contained start=+"+  skip=+\\\\\|\\"+  end=+$\|"+  contains=satherSpecial
syn match   satherComment         "--.*" contains=satherComment2String,satherCharacter,satherNumber


syn sync ccomment satherComment


if !exists("did_sather_syntax_inits")
  let did_sather_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link satherBranch		satherStatement
  hi link satherLabel		satherStatement
  hi link satherConditional	satherStatement
  hi link satherSynchronize	satherStatement
  hi link satherRepeat		satherStatement
  hi link satherExceptions	satherStatement
  hi link satherStorageClass	satherDeclarative
  hi link satherMethodDecl	satherDeclarative
  hi link satherClassDecl	satherDeclarative
  hi link satherScopeDecl	satherDeclarative
  hi link satherBoolValue	satherValue
  hi link satherSpecial		satherValue
  hi link satherString		satherValue
  hi link satherCharacter	satherValue
  hi link satherSpecialCharacter	satherValue
  hi link satherNumber		satherValue
  hi link satherStatement	Statement
  hi link satherOperator	Statement
  hi link satherComment		Comment
  hi link satherType		Type
  hi link satherValue		String
  hi link satherString		String
  hi link satherSpecial		String
  hi link satherCharacter	String
  hi link satherDeclarative	Type
  hi link satherExternal	PreCondit
endif

let b:current_syntax = "sather"

" vim: ts=8
