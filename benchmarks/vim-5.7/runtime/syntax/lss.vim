" Vim syntax file
" Language:	Lynx 2.7.1 style file
" Maintainer:	Scott Bigham <dsb@cs.duke.edu>
" Last Change:	1997 Nov 23

" clear any unwanted syntax defs
syntax clear

" This setup is probably atypical for a syntax highlighting file, because
" most of it is not really intended to be overrideable.  Instead, the
" highlighting is supposed to correspond to the highlighting specified by
" the .lss file entries themselves; ie. the "bold" keyword should be bold,
" the "red" keyword should be red, and so forth.  The exceptions to this
" are comments, of course, and the initial keyword identifying the affected
" element, which will inherit the usual Identifier highlighting.

syn match lssElement "^[^:]\+" nextgroup=lssMono

syn match lssMono ":[^:]\+" contained nextgroup=lssFgColor contains=lssReverse,lssUnderline,lssBold,lssStandout

syn keyword	lssBold		bold		contained
syn keyword	lssReverse	reverse		contained
syn keyword	lssUnderline	underline	contained
syn keyword	lssStandout	standout	contained

syn match lssFgColor ":[^:]\+" contained nextgroup=lssBgColor contains=lssRedFg,lssBlueFg,lssGreenFg,lssBrownFg,lssMagentaFg,lssCyanFg,lssLightgrayFg,lssGrayFg,lssBrightredFg,lssBrightgreenFg,lssYellowFg,lssBrightblueFg,lssBrightmagentaFg,lssBrightcyanFg

syn case ignore
syn keyword	lssRedFg		red		contained
syn keyword	lssBlueFg		blue		contained
syn keyword	lssGreenFg		green		contained
syn keyword	lssBrownFg		brown		contained
syn keyword	lssMagentaFg		magenta		contained
syn keyword	lssCyanFg		cyan		contained
syn keyword	lssLightgrayFg		lightgray	contained
syn keyword	lssGrayFg		gray		contained
syn keyword	lssBrightredFg		brightred	contained
syn keyword	lssBrightgreenFg	brightgreen	contained
syn keyword	lssYellowFg		yellow		contained
syn keyword	lssBrightblueFg		brightblue	contained
syn keyword	lssBrightmagentaFg	brightmagenta	contained
syn keyword	lssBrightcyanFg		brightcyan	contained
syn case match

syn match lssBgColor ":[^:]\+" contained contains=lssRedBg,lssBlueBg,lssGreenBg,lssBrownBg,lssMagentaBg,lssCyanBg,lssLightgrayBg,lssGrayBg,lssBrightredBg,lssBrightgreenBg,lssYellowBg,lssBrightblueBg,lssBrightmagentaBg,lssBrightcyanBg,lssWhiteBg

syn case ignore
syn keyword	lssRedBg		red		contained
syn keyword	lssBlueBg		blue		contained
syn keyword	lssGreenBg		green		contained
syn keyword	lssBrownBg		brown		contained
syn keyword	lssMagentaBg		magenta		contained
syn keyword	lssCyanBg		cyan		contained
syn keyword	lssLightgrayBg		lightgray	contained
syn keyword	lssGrayBg		gray		contained
syn keyword	lssBrightredBg		brightred	contained
syn keyword	lssBrightgreenBg	brightgreen	contained
syn keyword	lssYellowBg		yellow		contained
syn keyword	lssBrightblueBg		brightblue	contained
syn keyword	lssBrightmagentaBg	brightmagenta	contained
syn keyword	lssBrightcyanBg		brightcyan	contained
syn keyword	lssWhiteBg		white		contained
syn case match

syn match lssComment "#.*$"

if !exists("did_lss_syntax_inits")
  let did_lss_syntax_inits = 1

  hi link lssComment Comment
  hi link lssElement Identifier

  hi	lssBold		term=bold cterm=bold
  hi	lssReverse	term=reverse cterm=reverse
  hi	lssUnderline	term=underline cterm=underline
  hi	lssStandout	term=standout cterm=standout

  hi	lssRedFg		ctermfg=red
  hi	lssBlueFg		ctermfg=blue
  hi	lssGreenFg		ctermfg=green
  hi	lssBrownFg		ctermfg=brown
  hi	lssMagentaFg		ctermfg=magenta
  hi	lssCyanFg		ctermfg=cyan
  hi	lssGrayFg		ctermfg=gray
  if $COLORTERM == "rxvt"
    " On rxvt's, bright colors are activated by setting the bold attribute.
    hi	lssLightgrayFg		ctermfg=gray cterm=bold
    hi	lssBrightredFg		ctermfg=red cterm=bold
    hi	lssBrightgreenFg	ctermfg=green cterm=bold
    hi	lssYellowFg		ctermfg=yellow cterm=bold
    hi	lssBrightblueFg		ctermfg=blue cterm=bold
    hi	lssBrightmagentaFg	ctermfg=magenta cterm=bold
    hi	lssBrightcyanFg		ctermfg=cyan cterm=bold
  else
    hi	lssLightgrayFg		ctermfg=lightgray
    hi	lssBrightredFg		ctermfg=lightred
    hi	lssBrightgreenFg	ctermfg=lightgreen
    hi	lssYellowFg		ctermfg=yellow
    hi	lssBrightblueFg		ctermfg=lightblue
    hi	lssBrightmagentaFg	ctermfg=lightmagenta
    hi	lssBrightcyanFg		ctermfg=lightcyan
  endif

  hi	lssRedBg		ctermbg=red
  hi	lssBlueBg		ctermbg=blue
  hi	lssGreenBg		ctermbg=green
  hi	lssBrownBg		ctermbg=brown
  hi	lssMagentaBg		ctermbg=magenta
  hi	lssCyanBg		ctermbg=cyan
  hi	lssLightgrayBg		ctermbg=lightgray
  hi	lssGrayBg		ctermbg=gray
  hi	lssBrightredBg		ctermbg=lightred
  hi	lssBrightgreenBg	ctermbg=lightgreen
  hi	lssYellowBg		ctermbg=yellow
  hi	lssBrightblueBg		ctermbg=lightblue
  hi	lssBrightmagentaBg	ctermbg=lightmagenta
  hi	lssBrightcyanBg		ctermbg=lightcyan
  hi	lssWhiteBg		ctermbg=white ctermfg=black
endif

let b:current_syntax = "lss"

" vim: ts=8
