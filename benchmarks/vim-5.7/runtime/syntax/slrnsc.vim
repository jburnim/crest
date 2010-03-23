" Vim syntax file
" Language:	Slrn score file
" Maintainer:	Preben "Peppe" Guldberg (c928400@student.dtu.dk)
" Last Change:	Thu Apr  2 14:02:43 1998

" Remove any old syntax stuff hanging around
syn clear

syn match slrnscComment		"%.*$"
syn match slrnscSectionCom	".].*"lc=2

" characters in newsgroup names
set isk=@,48-57,.,-,_,+

syn match slrnscGroup		contained "\(\k\|\*\)\+"
syn match slrnscNumber		contained "\d\+"
syn match slrnscDate		contained "\(\d\{1,2}[-/]\)\{2}\d\{4}"
syn match slrnscDelim		contained ":"
syn match slrnscComma		contained ","
syn match slrnscOper		contained "\~"
syn match slrnscEsc		contained "\\[ecC<>.]"
syn match slrnscEsc		contained "[?^]"
syn match slrnscEsc		contained "[^\\]$\s*$"lc=1

syn region slrnscSection	matchgroup=slrnscSectionStd start="^\s*\[" end='\]' contains=slrnscGroup,slrnscComma,slrnscSectionCom
syn region slrnscSection	matchgroup=slrnscSectionNot start="^\s*\[\~" end='\]' contains=slrnscGroup,slrnscCommas,slrnscSectionCom

syn keyword slrnscItem		contained Expires From Lines References Subject Xref

syn match slrnscItemFill	contained ".*$" skipempty nextgroup=slrnscScoreItem contains=slrnscEsc

syn match slrnscScoreItem	contained "^\s*Expires:\s*\(\d\{1,2}[-/]\)\{2}\d\{4}\s*$" skipempty nextgroup=slrnscScoreItem contains=slrnscItem,slrnscDelim,slrnscDate
syn match slrnscScoreItem	contained "^\s*\~\=Lines:\s*\d\+\s*$" skipempty nextgroup=slrnscScoreItem contains=slrnscOper,slrnscItem,slrnscDelim,slrnscNumber
syn match slrnscScoreItem	contained "^\s*\~\=\(From\|References\|Subject\|Xref\):" nextgroup=slrnscItemFill contains=slrnscOper,slrnscItem,slrnscDelim
syn match slrnscScoreItem	contained "^\s*%.*$" skipempty nextgroup=slrnscScoreItem contains=slrnscComment

syn keyword slrnscScore		contained Score
syn match slrnScoreLine		"^\s*Score::\=\s\+=\=-\=\d\+\s*$" skipempty nextgroup=slrnscScoreItem contains=slrnscScore,slrnscDelim,slrnscOper,slrnscNumber

if !exists("did_slrnsc_syntax_inits")
  let did_slrnsc_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link slrnscComment		Comment
  hi link slrnscSectionCom	slrnscComment
  hi link slrnscGroup		String
  hi link slrnscNumber		Number
  hi link slrnscDate		Special
  hi link slrnscDelim		Delimiter
  hi link slrnscComma		SpecialChar
  hi link slrnscOper		SpecialChar
  hi link slrnscEsc		String
  hi link slrnscSectionStd	Type
  hi link slrnscSectionNot	Delimiter
  hi link slrnscItem		Statement
  hi link slrnscScore		Keyword
endif

let b:current_syntax = "slrnsc"

"EOF	vim: ts=8 noet tw=200 sw=8 sts=0
