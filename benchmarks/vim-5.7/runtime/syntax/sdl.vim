" Vim syntax file
" Language:	SDL
" Maintainer:	Harald Böhme
" Last Change:	Mon Mar 16 12:13:33 MET 1998
" This syntax file is a adaption of the C syntax to SDL syntax

syn clear
syntax case ignore

" A bunch of useful SDL keywords
syn keyword sdlStatement	task else nextstate
syn keyword sdlStatement	in out with from interface
syn keyword sdlStatement	to via env and use fpar
syn keyword sdlStatement	process procedure block system service type
syn keyword sdlStatement	endprocess endprocedure endblock endsystem endservice
syn keyword sdlStatement	package endpackage
syn keyword sdlStatement	channel endchannel signalroute connect
syn keyword sdlStatement	synonym dcl signal gate timer signallist
syn keyword sdlStatement	create output set reset call
syn keyword sdlStatement	operators literals all
syn keyword sdlNewState		state endstate
syn keyword sdlInput		input start stop return none
syn keyword sdlConditional	decision enddecision join
syn keyword sdlVirtual		virtual redefined finalized adding inherits
syn keyword sdlExported		remote exported imported


" String and Character contstants
" Highlight special characters (those which have a backslash) differently
syn match   sdlSpecial		contained "\\\d\d\d\|\\."
syn region  sdlString		start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=cSpecial
syn region  sdlString		start=+'+  skip=+''+  end=+'+

syn case ignore
syn case match

set iskeyword=@,48-57,_,192-255,-

syn region sdlComment		start="/\*"  end="\*/"
syn region sdlComment		start="comment"  end=";"
syn region sdlComment		start="--" end="--\|$"
syn match  sdlCommentError	"\*/"

syntax case ignore
syn keyword sdlOperator		presend
syn keyword sdlType		Integer Natural Duration Pid Boolean Time
syn keyword sdlType		Charstring IA5String
syn keyword sdlType		self now sender offspring
syn keyword sdlType		SET OF BOOLEAN INTEGER REAL BIT OCTET
syn keyword sdlType		SEQUENCE CHOICE
syn keyword sdlType		STRING OBJECT IDENTIFIER NULL
syn keyword sdlStructure	newtype endnewtype asntype endasntype syntype endsyntype
syn keyword sdlException	exceptionhandler endexceptionhandler onexception
syn keyword sdlException	catch new

syn sync ccomment sdlComment

if !exists("did_sdl_syntax_inits")
  let did_sdl_syntax_inits = 1
  hi link sdlException	Label
  hi link sdlConditional	sdlStatement
  hi link sdlVirtual	sdlStatement
  hi link sdlExported	sdlFlag
  hi link sdlCommentError	sdlError
  hi link sdlOperator	Operator
  hi link sdlStructure	sdlType
  hi sdlStatement	term=bold ctermfg=4 guifg=Blue
  hi sdlFlag		term=bold ctermfg=4 guifg=Blue gui=italic
  hi sdlNewState	term=italic ctermfg=2 guifg=Magenta gui=underline
  hi sdlInput		term=bold guifg=Red
  hi link sdlType	Type
  hi link sdlString	String
  hi link sdlComment	Comment
  hi link sdlSpecial	Special
  hi link sdlError	Error
endif

let b:current_syntax = "sdl"

" vim: ts=8
