" Vim syntax file
" Language:	kscript
" Maintainer:	Thomas Capricelli <orzel@yalbi.com>
" URL:		http://aquila.rezel.enst.fr/thomas/vim/kscript.vim
" CVS:		$Id: kscript.vim,v 1.1 2000/04/13 14:20:42 orzel Exp $

" Remove any old syntax stuff hanging around
syn clear

syn keyword	kscriptPreCondit	import from

syn keyword 	kscriptHardCoded	print println connect length arg mid upper lower isEmpty toInt toFloat findApplication
syn keyword	kscriptConditional	if else switch
syn keyword	kscriptRepeat		while for do foreach
syn keyword	kscriptExceptions	emit catch raise try signal
syn keyword	kscriptFunction		class struct enum
syn keyword	kscriptConst		FALSE TRUE false true
syn keyword	kscriptStatement	return delete
syn keyword	kscriptLabel		case default
syn keyword	kscriptStorageClass	const
syn keyword	kscriptType		in out inout var

syn keyword	kscriptTodo		contained TODO FIXME XXX

syn region	kscriptComment		start="/\*" end="\*/" contains=kscriptTodo
syn match	kscriptComment		"//.*" contains=kscriptTodo
syn match	kscriptComment		"#.*$" contains=kscriptTodo

syn region  	kscriptString		start=+'+  end=+'+ skip=+\\\\\|\\'+
syn region  	kscriptString		start=+"+  end=+"+ skip=+\\\\\|\\"+
syn region  	kscriptString		start=+"""+  end=+"""+
syn region  	kscriptString		start=+'''+  end=+'''+

if !exists("did_kscript_syntax_inits")
  let did_kscript_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later

  hi link kscriptConditional		Conditional
  hi link kscriptRepeat			Repeat
  hi link kscriptExceptions		Statement
  hi link kscriptFunction		Function
  hi link kscriptConst			Constant
  hi link kscriptStatement		Statement
  hi link kscriptLabel			Label
  hi link kscriptStorageClass		StorageClass
  hi link kscriptType			Type
  hi link kscriptTodo			Todo
  hi link kscriptComment		Comment
  hi link kscriptString			String
  hi link kscriptPreCondit		PreCondit
  hi link kscriptHardCoded		Statement


endif

let b:current_syntax = "kscript"

" vim: ts=8
