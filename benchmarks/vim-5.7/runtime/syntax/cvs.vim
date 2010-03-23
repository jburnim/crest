" Vim syntax file
" Language: CVS commit file
" Maintainer:  Matt Dunford (zoot@zotikos.com)
" Last Change: Tue May 30 14:45:43 PDT 2000

" Remove any old syntax stuff hanging around
syn clear

" syn region cvsText  start="^[^C][^V][^S][^:]" end="$"
syn region cvsLine  start="^CVS:" end="$" contains=cvsFile,cvsDir,cvsFiles
syn match cvsFile contained "\s\t.*"
syn match cvsDir  contained "Committing in.*$"
syn match cvsFiles contained "\S\+ Files:"

if !exists("did_cvs_syntax_inits")
  let did_cvs_syntax_inits = 1
  " hi link cvsText String
  hi link cvsLine Comment
  hi link cvsFile Identifier
  hi link cvsFiles cvsDir
  hi link cvsDir Statement
endif

let b:current_syntax = "cvs"
