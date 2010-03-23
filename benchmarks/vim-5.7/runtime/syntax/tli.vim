" Vim syntax file
" Language:	TealInfo source files (*.tli)
" Maintainer:	Kurt W. Andrews <kandrews@fastrans.net>
" Last Change:	2000 Mar 22
" Version:      1.0

" Remove any old syntax stuff hanging around

syn clear

" TealInfo Objects

syn keyword tliObject LIST POPLIST WINDOW POPWINDOW OUTLINE CHECKMARK GOTO
syn keyword tliObject LABEL IMAGE RECT TRES PASSWORD POPEDIT POPIMAGE CHECKLIST

" TealInfo Fields

syn keyword tliField X Y W H BX BY BW BH SX SY FONT BFONT CYCLE DELAY TABS
syn keyword tliField STYLE BTEXT RECORD DATABASE KEY TARGET DEFAULT TEXT
syn keyword tliField LINKS MAXVAL

" TealInfo Styles

syn keyword tliStyle INVERTED HORIZ_RULE VERT_RULE NO_SCROLL NO_BORDER BOLD_BORDER
syn keyword tliStyle ROUND_BORDER ALIGN_RIGHT ALIGN_CENTER ALIGN_LEFT_START ALIGN_RIGHT_START
syn keyword tliStyle ALIGN_CENTER_START ALIGN_LEFT_END ALIGN_RIGHT_END ALIGN_CENTER_END
syn keyword tliStyle LOCKOUT BUTTON_SCROLL BUTTON_SELECT STROKE_FIND FILLED REGISTER

" String and Character constants

syn match tliSpecial	"@"
syn region tliString	start=+"+ end=+"+

"TealInfo Numbers, identifiers and comments

syn case ignore
syn match tliNumber 	"\d*"
syn match tliIdentifier	"\<\h\w*\>"
syn match tliComment 	"#.*"
syn case match

if !exists("did_tli_syntax_inits")
  let did_tli_syntax_inits = 1

  hi link tliNumber	Number
  hi link tliString	String
  hi link tliComment	Comment
  hi link tliSpecial	SpecialChar
  hi link tliIdentifier Identifier
  hi link tliObject     Statement
  hi link tliField      Type
  hi link tliStyle      PreProc
endif

let b:current_syntax = "tli"

" vim: ts=8
