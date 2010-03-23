" Vim syntax file
" Language:	Stored Procedures
" Maintainer:	Jeff Lanzarotta
" URL:		http://web.qx.net/lanzarotta/vim/syntax/stp.vim
" Last Change:	2000 January 6

" Remove any old syntax stuff hanging around
syn clear

syn case ignore

" The STP reserved words, defined as keywords.

syn keyword stpSpecial   	null

syn keyword stpKeyword          as avg begin break by clustered
syn keyword stpKeyword          count create cursor deallocate
syn keyword stpKeyword          else end execute exists fetch for from go grant
syn keyword stpKeyword          if index insert into max min nonclustered on
syn keyword stpKeyword          order output procedure public return select set
syn keyword stpKeyword          sum table to uniue update values where while round

syn keyword stpOperator         asc not and or desc group having
syn keyword stpOperator         in any some all between exists
syn keyword stpOperator         like escape with
syn keyword stpOperator         union intersect minus
syn keyword stpOperator         prior distinct
syn keyword stpOperator         sysdate

syn keyword stpStatement        alter analyze audit comment commit create continue close
syn keyword stpStatement        declare delete drop explain grant insert lock noaudit
syn keyword stpStatement	rename revoke rollback savepoint select set print
syn keyword stpStatement        truncate update open exec execute output recompile

syn keyword stpFunction		ltrim rtrim

syn keyword stpType             char character date datetime float int integer
syn keyword stpType             long money smalldatetime smallint tinyint varchar

syn keyword stpTodo		TODO FIXME XXX DEBUG NOTE

" Strings and characters:
syn region stpString		start=+"+  skip=+\\\\|\\"+  end=+"+
syn region stpString		start=+'+  skip=+\\\\|\\"+  end=+'+

" Numbers:
syn match stpNumber		"-\=\<\d*\.\=[0-9_]\>"

" Comments:
syn region stpComment    start="/\*"  end="\*/" contains=stpTodo
syn match stpComment	"--.*" contains=stpTodo

if !exists("did_stp_syntax_inits")
  let did_stp_syntax_inits = 1
  " The default methods for highlighting. Can be overridden later.
  hi link stpComment	Comment
  hi link stpKeyword	Keyword
  hi link stpNumber	Number
  hi link stpOperator	Operator
  hi link stpSpecial	Special
  hi link stpStatement	Statement
  hi link stpString	String
  hi link stpType	Type
  hi link stpTodo	Todo
  hi link stpFunction	Function
endif

let b:current_syntax = "stp"

" vim: ts=8
