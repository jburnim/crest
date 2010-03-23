" Vim syntax file
" Language:	JSP (Java Server Pages)
" Maintainer:	Rafael Garcia-Suarez <garcia_suarez@hotmail.com>
" URL:		http://altern.org/rgs/vim/syntax/jsp.vim
" Last Change:	2000 Apr 12

" Remove any old syntax stuff hanging around
syn clear

if !exists("main_syntax")
  let main_syntax = 'jsp'
endif

" Source HTML syntax
source <sfile>:p:h/html.vim

" Next syntax items are case-sensitive
syn case match

" Include Java syntax
syn include @jspJava <sfile>:p:h/java.vim

syn region jspScriptlet matchgroup=jspTag start=/<%/  keepend end=/%>/ contains=@jspJava
syn region jspComment                     start=/<%--/        end=/--%>/
syn region jspDecl      matchgroup=jspTag start=/<%!/ keepend end=/%>/ contains=@jspJava
syn region jspExpr      matchgroup=jspTag start=/<%=/ keepend end=/%>/ contains=@jspJava
syn region jspDirective                   start=/<%@/         end=/%>/ contains=htmlString,jspDirName,jspDirArg

syn keyword jspDirName contained include page taglib
syn keyword jspDirArg contained file uri prefix language extends import session buffer autoFlush
syn keyword jspDirArg contained isThreadSafe info errorPage contentType isErrorPage

if !exists("did_jsp_syntax_inits")
  let did_jsp_syntax_inits = 1
  " java.vim has redefined htmlComment highlighting
  hi link htmlComment     Comment
  hi link htmlCommentPart Comment
  " Be consistent with html highlight settings
  hi link jspComment      htmlComment
  hi link jspTag          htmlTag
  hi link jspDirective    jspTag
  hi link jspDirName      htmlTagName
  hi link jspDirArg       htmlArg
endif

let b:current_syntax = "jsp"

if main_syntax == 'jsp'
  unlet main_syntax
endif

" vim: ts=8
