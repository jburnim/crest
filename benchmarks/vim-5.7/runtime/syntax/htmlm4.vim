" Vim syntax file
" Language:	HTML and M4
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/htmlm4.vim
" Last Change:	1999 May 20

" Remove any old syntax stuff hanging around

" we define it here so that included files can test for it
if !exists("main_syntax")
  syn clear
  let main_syntax='htmlm4'
endif

so <sfile>:p:h/html.vim
syn case match

so <sfile>:p:h/m4.vim
syn cluster htmlPreproc add=@m4Top
syn cluster m4StringContents add=htmlTag,htmlEndTag

let b:current_syntax = "htmlm4"

if main_syntax == 'htmlm4'
  unlet main_syntax
endif
