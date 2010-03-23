" Vim syntax file
" Language:	Vim help file
" Maintainer:	Bram Moolenaar (Bram@vim.org)
" Last Change:	2000 Jun 06

" Remove any old syntax stuff hanging around
syn clear

syn match helpHeadline		"^[A-Z ]\+[ ]\+\*"me=e-1
syn match helpSectionDelim	"^=\{3,}.*==$"
syn match helpSectionDelim	"^-\{3,}.*--$"
syn match helpExampleStart	"^>" nextgroup=helpExample
syn match helpExample		".*" contained
syn match helpHyperTextJump	"|[#-)!+-~]\+|"
syn match helpHyperTextEntry	"\*[#-)!+-~]\+\*\s"he=e-1
syn match helpHyperTextEntry	"\*[#-)!+-~]\+\*$"
syn match helpVim		"Vim version [0-9.a-z]\+"
syn match helpVim		"VIM REFERENCE.*"
syn match helpOption		"'[a-z]\{2,\}'"
syn match helpOption		"'t_..'"
syn match helpHeader		".*\~$"me=e-1 nextgroup=helpIgnore
syn match helpIgnore		"." contained
syn keyword helpNote		note Note NOTE note: Note: NOTE:
syn match helpSpecial		"\<N\>"
syn match helpSpecial		"(N\>"ms=s+1
syn match helpSpecial		"\[N]"
" avoid highlighting N  N in help.txt
syn match helpSpecial		"N  N"he=s+1
syn match helpSpecial		"Nth"me=e-2
syn match helpSpecial		"N-1"me=e-2
syn match helpSpecial		"{[-a-zA-Z0-9'":%#=[\]<>.]\+}"
syn match helpSpecial		"\s\[[-a-zA-Z0-9_]\{2,}]"ms=s+1
syn match helpSpecial		"<[-a-zA-Z0-9_]\+>"
syn match helpSpecial		"<[SCM]-.>"
syn match helpSpecial		"\[range]"
syn match helpSpecial		"\[line]"
syn match helpSpecial		"\[count]"
syn match helpSpecial		"\[offset]"
syn match helpSpecial		"\[cmd]"
syn match helpSpecial		"\[num]"
syn match helpSpecial		"\[+num]"
syn match helpSpecial		"\[-num]"
syn match helpSpecial		"CTRL-."
syn match helpSpecial		"CTRL-Break"
syn match helpSpecial		"CTRL-{char}"
syn region helpNotVi		start="{Vi[: ]" start="{not" start="{only" end="}" contains=helpLeadBlank,helpHyperTextJump
syn match helpLeadBlank		"^\s\+"

if !exists("did_help_syntax_inits")
  let did_help_syntax_inits = 1

  hi link helpExampleStart	helpIgnore
  hi link helpIgnore		Ignore
  hi link helpHyperTextJump	Subtitle
  hi link helpHyperTextEntry	String
  hi link helpHeadline		Statement
  hi link helpHeader		PreProc
  hi link helpSectionDelim	PreProc
  hi link helpVim		Identifier
  hi link helpExample		Comment
  hi link helpOption		Type
  hi link helpNotVi		Special
  hi link helpSpecial		Special
  hi link helpNote		Todo
  hi link Subtitle		Identifier
endif

let b:current_syntax = "help"

" vim: ts=8
