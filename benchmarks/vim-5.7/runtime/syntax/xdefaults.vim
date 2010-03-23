" Vim syntax file
" Language:	X resources files like ~/.Xdefaults (xrdb)
" Maintainer:	Johannes Zellner <johannes@zellner.org>
"		Author and previous maintainer:
"		Gautam H. Mudunuri <gmudunur@informatica.com>
" Last Change:	Sat, 08 Apr 2000 11:26:31 +0200
" URL:		http://www.zellner.org/vim/syntax/xdefaults.vim
" $Id: xdefaults.vim,v 1.4 2000/04/10 11:40:25 joze Exp $
"
" REFERENCES:
"   xrdb manual page
"   xrdb source: ftp://ftp.x.org/pub/R6.4/xc/programs/xrdb/xrdb.c


syn clear

" turn case on
syn case match


if !exists("xdefaults_no_colon_errors")
    " mark lines which do not contain a colon as errors.
    " This does not really catch all errors but only lines
    " which contain at least two WORDS and no colon. This
    " was done this way so that a line is not marked as
    " error while typing (which would be annoying).
    syntax match xdefaultsErrorLine "^\s*[a-zA-Z.*]\+\s\+[^: 	]\+"
endif


" syn region  xdefaultsLabel   start=+^[^:]\{-}:+he=e-1 skip=+\\+ end="$"
syn match   xdefaultsLabel   +^[^:]\{-}:+he=e-1                      contains=xdefaultsPunct
syn region  xdefaultsValue   keepend start=+:+lc=1 skip=+\\+ end=+$+ contains=xdefaultsSpecial,xdefaultsLabel,xdefaultsLineEnd

syn match   xdefaultsSpecial	contained +#override+
syn match   xdefaultsSpecial	contained +#augment+
syn match   xdefaultsPunct	contained +[.*:]+
syn match   xdefaultsLineEnd	contained +\\$+
syn match   xdefaultsLineEnd	contained +\\n\\$+
syn match   xdefaultsLineEnd	contained +\\n$+



" COMMENTS

" note, that the '!' must be at the very first position of the line
syn match   xdefaultsComment "^!.*$"                     contains=xdefaultsTodo

" lines starting with a '#' mark and which are not preprocessor
" lines are skipped.  This is not part of the xrdb documentation.
" It was reported by Bram Moolenaar and could be confirmed by
" having a look at xrdb.c:GetEntries()
syn match   xdefaultsCommentH		"^#.*$"
"syn region  xdefaultsComment start="^#"  end="$" keepend contains=ALL
syn region  xdefaultsComment start="/\*" end="\*/"       contains=xdefaultsTodo

syntax match xdefaultsCommentError	"\*/"

syn keyword xdefaultsTodo contained TODO FIXME XXX



" PREPROCESSOR STUFF

syn region	xdefaultsPreProc	start="^\s*#\s*\(if\|ifdef\|ifndef\|elif\|else\|endif\)\>" skip="\\$" end="$" contains=xdefaultsSymbol
if !exists("xdefaults_no_if0")
  syn region	xdefaultsCppOut		start="^\s*#\s*if\s\+0\>" end=".\|$" contains=xdefaultsCppOut2
  syn region	xdefaultsCppOut2	contained start="0" end="^\s*#\s*\(endif\>\|else\>\|elif\>\)" contains=xdefaultsCppSkip
  syn region	xdefaultsCppSkip	contained start="^\s*#\s*\(if\>\|ifdef\>\|ifndef\>\)" skip="\\$" end="^\s*#\s*endif\>" contains=xdefaultsCppSkip
endif
syn region	xdefaultsIncluded	contained start=+"+ skip=+\\\\\|\\"+ end=+"+
syn match	xdefaultsIncluded	contained "<[^>]*>"
syn match	xdefaultsInclude	"^\s*#\s*include\>\s*["<]" contains=xdefaultsIncluded
syn cluster	xdefaultsPreProcGroup	contains=xdefaultsPreProc,xdefaultsIncluded,xdefaultsInclude,xdefaultsDefine
syn region	xdefaultsDefine		start="^\s*#\s*\(define\|undef\)\>" skip="\\$" end="$" contains=ALLBUT,@xdefaultsPreProcGroup,xdefaultsCommentH,xdefaultsErrorLine
syn region	xdefaultsPreProc	start="^\s*#\s*\(pragma\>\|line\>\|warning\>\|warn\>\|error\>\)" skip="\\$" end="$" keepend contains=ALLBUT,@xdefaultsPreProcGroup,xdefaultsCommentH,xdefaultsErrorLine



" symbols as defined by xrdb
syn keyword xdefaultsSymbol contained SERVERHOST
syn match   xdefaultsSymbol contained "SRVR_[a-zA-Z0-9_]\+"
syn keyword xdefaultsSymbol contained HOST
syn keyword xdefaultsSymbol contained DISPLAY_NUM
syn keyword xdefaultsSymbol contained CLIENTHOST
syn match   xdefaultsSymbol contained "CLNT_[a-zA-Z0-9_]\+"
syn keyword xdefaultsSymbol contained RELEASE
syn keyword xdefaultsSymbol contained REVISION
syn keyword xdefaultsSymbol contained VERSION
syn keyword xdefaultsSymbol contained VENDOR
syn match   xdefaultsSymbol contained "VNDR_[a-zA-Z0-9_]\+"
syn match   xdefaultsSymbol contained "EXT_[a-zA-Z0-9_]\+"
syn keyword xdefaultsSymbol contained NUM_SCREENS
syn keyword xdefaultsSymbol contained SCREEN_NUM
syn keyword xdefaultsSymbol contained BITS_PER_RGB
syn keyword xdefaultsSymbol contained CLASS
syn keyword xdefaultsSymbol contained StaticGray GrayScale StaticColor PseudoColor TrueColor DirectColor
syn match   xdefaultsSymbol contained "CLASS_\(StaticGray\|GrayScale\|StaticColor\|PseudoColor\|TrueColor\|DirectColor\)"
syn keyword xdefaultsSymbol contained COLOR
syn match   xdefaultsSymbol contained "CLASS_\(StaticGray\|GrayScale\|StaticColor\|PseudoColor\|TrueColor\|DirectColor\)_[0-9]\+"
syn keyword xdefaultsSymbol contained HEIGHT
syn keyword xdefaultsSymbol contained WIDTH
syn keyword xdefaultsSymbol contained PLANES
syn keyword xdefaultsSymbol contained X_RESOLUTION
syn keyword xdefaultsSymbol contained Y_RESOLUTION

if !exists("did_xdefaults_syntax_inits")
    let did_xdefaults_syntax_inits = 1
    " The default methods for highlighting.  Can be overridden later
    hi link xdefaultsLabel		Type
    hi link xdefaultsValue		Constant
    hi link xdefaultsComment		Comment
    hi link xdefaultsCommentH		xdefaultsComment
    hi link xdefaultsPreProc		PreProc
    hi link xdefaultsInclude		xdefaultsPreProc
    hi link xdefaultsCppSkip		xdefaultsCppOut
    hi link xdefaultsCppOut2		xdefaultsCppOut
    hi link xdefaultsCppOut		Comment
    hi link xdefaultsIncluded		String
    hi link xdefaultsDefine		Macro
    hi link xdefaultsSymbol		Statement
    hi link xdefaultsSpecial		Statement
    hi link xdefaultsErrorLine		Error
    hi link xdefaultsCommentError	Error
    hi link xdefaultsPunct		Normal
    hi link xdefaultsLineEnd		Special
endif

let b:current_syntax = "xdefaults"

" vim:ts=8
