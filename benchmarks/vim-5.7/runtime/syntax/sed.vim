" Vim syntax file
" Language:	sed
" Maintainer:	Haakon Riiser <hakonrk@fys.uio.no>
" Last Change:	1999 Jul 02
"
" Special thanks go to Preben "Peppe" Guldberg for a lot of help, and, in
" particular, his clever way of matching the substitute command.

" Clear old syntax defs
syn clear

syn match sedError	"\S"

syn match sedSemicolon	";"
syn match sedAddress	"[[:digit:]$]"
syn match sedAddress	"\d\+\~\d\+"
syn region sedAddress	matchgroup=Special start="/\(\\/\)\=" skip="[^\\]\(\\\\\)*\\/" end="/I\=" contains=sedTab,sedRegexpMeta
syn match sedComment	"^\s*#.*$"
syn match sedFunction	"[dDgGhHlnNpPqx=]\($\|;\)" contains=sedSemicolon
syn match sedLabel	"^\s*:.*$"
syn match sedLineCont	"^\(\\\\\)*\\$" contained
syn match sedLineCont	"[^\\]\(\\\\\)*\\$"ms=e contained
syn match sedSpecial	"[{},!]"
if exists("highlight_sedtabs")
    syn match sedTab	"\t" contained
endif

" Append/Change/Insert
syn region sedACI	matchgroup=sedFunction start="[aci]\\$" matchgroup=NONE end="^.*$" contains=sedLineCont,sedTab

syn region sedBranch	matchgroup=sedFunction start="[bt]" end="$"
syn region sedRW	matchgroup=sedFunction start="[rw]" end="$"

" Substitution/transform with various delimiters
syn region sedFlagwrite	    matchgroup=sedFlag start="w" end="$" contained
syn match sedFlag	    "[[:digit:]gpI]*\(;\|$\|w\)" contains=sedFlagwrite,sedSemicolon contained
syn match sedRegexpMeta	    "[.*^$]" contained
syn match sedRegexpMeta	    "\\." contains=sedTab contained
syn match sedRegexpMeta	    "\[.\{-}\]" contains=sedTab contained
syn match sedRegexpMeta	    "\\{\d\*,\d*\\}" contained
syn match sedRegexpMeta	    "\\(.\{-}\\)" contains=sedTab contained
syn match sedReplaceMeta    "&\|\\\($\|.\)" contains=sedTab contained

" Metacharacters: $ * . \ ^ [ ~
" @ (ascii 64) is used as delimiter and treated on its own below
let __sed_metacharacters = '$*.\^[~'
let __sed_i = 32
while __sed_i <= 126
    let __sed_delimiter = escape(nr2char(__sed_i), __sed_metacharacters)
	if __sed_i != 64
	    exe 'syn region sedAddress matchgroup=Special start=@\\'.__sed_delimiter.'\(\\'.__sed_delimiter.'\)\=@ skip=@[^\\]\(\\\\\)*\\'.__sed_delimiter.'@ end=@'.__sed_delimiter.'I\=@ contains=sedTab'
	    exe 'syn region sedRegexp'.__sed_i  'matchgroup=Special start=@'.__sed_delimiter.'\(\\\\\|\\'.__sed_delimiter.'\)*@ skip=@[^\\'.__sed_delimiter.']\(\\\\\)*\\'.__sed_delimiter.'@ end=@'.__sed_delimiter.'@me=e-1 contains=sedTab,sedRegexpMeta keepend contained nextgroup=sedReplacement'.__sed_i
	    exe 'syn region sedReplacement'.__sed_i 'matchgroup=Special start=@'.__sed_delimiter.'\(\\\\\|\\'.__sed_delimiter.'\)*@ skip=@[^\\'.__sed_delimiter.']\(\\\\\)*\\'.__sed_delimiter.'@ end=@'.__sed_delimiter.'@ contains=sedTab,sedReplaceMeta keepend contained nextgroup=sedFlag'
	endif
    let __sed_i = __sed_i + 1
endwhile
syn region sedAddress matchgroup=Special start=+\\@\(\\@\)\=+ skip=+[^\\]\(\\\\\)*\\@+ end=+@I\=+ contains=sedTab,sedRegexpMeta
syn region sedRegexp64 matchgroup=Special start=+@\(\\\\\|\\@\)*+ skip=+[^\\@]\(\\\\\)*\\@+ end=+@+me=e-1 contains=sedTab,sedRegexpMeta keepend contained nextgroup=sedReplacement64
syn region sedReplacement64 matchgroup=Special start=+@\(\\\\\|\\@\)*+ skip=+[^\\@]\(\\\\\)*\\@+ end=+@+ contains=sedTab,sedReplaceMeta keepend contained nextgroup=sedFlag

" Since the syntax for the substituion command is very similar to the
" syntax for the transform command, I use the same pattern matching
" for both commands.  There is one problem -- the transform command
" (y) does not allow any flags.  To save memory, I ignore this problem.
syn match sedST	"[sy]" nextgroup=sedRegexp\d\+

if !exists("did_sed_syntax_inits")
    let did_sed_syntax_inits = 1
    hi link sedAddress		Macro
    hi link sedACI		NONE
    hi link sedBranch		Label
    hi link sedComment		Comment
    hi link sedDelete		Function
    hi link sedError		Error
    hi link sedFlag		Type
    hi link sedFlagwrite	Constant
    hi link sedFunction		Function
    hi link sedLabel		Label
    hi link sedLineCont		Special
    hi link sedPutHoldspc	Function
    hi link sedReplaceMeta	Special
    hi link sedRegexpMeta	Special
    hi link sedRW		Constant
    hi link sedSemicolon	Special
    hi link sedST		Function
    hi link sedSpecial		Special
    if exists("highlight_sedtabs")
        hi link sedTab		Todo
    endif
    let __sed_i = 32
    while __sed_i <= 126
	exe "hi link sedRegexp".__sed_i		"Macro"
	exe "hi link sedReplacement".__sed_i	"NONE"
	let __sed_i = __sed_i + 1
    endwhile
endif

unlet __sed_i __sed_delimiter __sed_metacharacters

let b:current_syntax = "sed"

" vim: sts=4 sw=4 ts=8
