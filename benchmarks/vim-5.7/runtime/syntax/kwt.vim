" Vim syntax file
" Language:	kimwitu++
" Maintainer:	Michael Piefel <piefel@informatik.hu-berlin.de>
" Last Change:	23 December 1999

" Remove any old syntax stuff hanging around
syn clear
syn region cParen	transparent start='(' end=')' contains=ALLBUT,cParenError,cIncluded,cSpecial,cTodo,cUserLabel,kwtViewName

" Read the C++ syntax to start with
source <sfile>:p:h/cpp.vim

" kimwitu++ extentions

syn keyword cType	integer real casestring nocasestring voidptr

" avoid unparsing rule sth:view being scanned as label
syn clear   cUserCont
syn match   cUserCont	"^\s*\I\i*\s*:$" contains=cUserLabel contained
syn match   cUserCont	";\s*\I\i*\s*:$" contains=cUserLabel contained
syn match   cUserCont	"^\s*\I\i*\s*:[^:]"me=e-1 contains=cUserLabel contained
syn match   cUserCont	";\s*\I\i*\s*:[^:]"me=e-1 contains=cUserLabel contained

" highlight phylum decls
syn match   kwtPhylum	"^\I\i*:$"
syn match   kwtPhylum	"^\I\i*\s*{\s*\(!\|\I\)\i*\s*}\s*:$"

syn keyword kwtStatement with foreach uview rview storageclass list
syn match kwtSep	"^%}$"
syn match kwtSep	"^%{\(\s\+\I\i*\)*$"
syn region kwtViews	start="->\s*\(\[\|<\)"hs=e+1 end=":"he=s-1 contains=cComment
syn region kwtViews	start="^\s\+\(\[\|<\)"hs=e+1 end=":"he=s-1 contains=cComment
syn match kwtEndRule	"\(\]\|>\);"
"syn region cText	transparent start='{' end='}' contains=ALLBUT,cParenError,cIncluded,cSpecial,cTodo,cUserCont,cUserLabel,cBitField


if !exists("did_kwt_syntax_inits")
  let did_kwt_syntax_inits = 1
  hi link kwtStatement	Statement
  hi link kwtSep	Delimiter
  hi link kwtViews	Label
  hi link kwtPhylum	Type
  "hi link cText	Comment
endif

syn sync lines=300

let b:current_syntax = "kwt"

" vim: ts=8
