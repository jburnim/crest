" Vim syntax file
" Language:	Lout
" Maintainer:	Christian V. J. Bruessow <cvjb@bigfoot.de>
" Last Change:	Fre 18 Jun 1999 13:42:06 MEST

" Lout: Basser Lout document formatting system.

" Remove any old syntax stuff hanging around
syn clear

" Lout is case sensitive
syn case match

" Synchronization
syn sync lines=1000
set iskeyword=@,48-57,.,@-@,_,192-255

" Some special keywords
syn keyword loutTodo contained TODO lout Lout LOUT
syn keyword loutDefine def macro

" Some big structures
syn keyword loutKeyword @Begin @End @Figure @Tab
syn keyword loutKeyword @Book @Doc @Document @Report
syn keyword loutKeyword @Introduction @Abstract @Appendix
syn keyword loutKeyword @Chapter @Section @BeginSections @EndSections

" All kind of Lout keywords
syn match loutFunction '\<@[^ \t{}]\+\>'

" Braces -- Don`t edit these lines!
syn match loutMBraces '[{}]'
syn match loutIBraces '[{}]'
syn match loutBBrace '[{}]'
syn match loutBIBraces '[{}]'
syn match loutHeads '[{}]'

" Unmatched braces.
syn match loutBraceError '}'

" End of multi-line definitions, like @Document, @Report and @Book.
syn match loutEOmlDef '^//$'

" Grouping of parameters and objects.
syn region loutObject transparent matchgroup=Delimiter start='{' matchgroup=Delimiter end='}' contains=ALLBUT,loutBraceError

" The NULL object has a special meaning
syn keyword loutNULL {}

" Comments
syn region loutComment start='\#' end='$' contains=loutTodo

" Double quotes
syn region loutSpecial start=+"+ skip=+\\\\\|\\"+ end=+"+

" ISO-LATIN-1 characters created with @Char, or Adobe symbols
" created with @Sym
syn match loutSymbols '@\(\(Char\)\|\(Sym\)\)\s\+[A-Za-z]\+'

" Include files
syn match loutInclude '@IncludeGraphic\s\+\k\+'
syn region loutInclude start='@\(\(SysInclude\)\|\(IncludeGraphic\)\|\(Include\)\)\s*{' end='}'

" Tags
syn match loutTag '@\(\(Tag\)\|\(PageMark\)\|\(PageOf\)\|\(NumberOf\)\)\s\+\k\+'
syn region loutTag start='@Tag\s*{' end='}'

" Equations
syn match loutMath '@Eq\s\+\k\+'
syn region loutMath matchgroup=loutMBraces start='@Eq\s*{' matchgroup=loutMBraces end='}' contains=ALLBUT,loutBraceError
"
" Fonts
syn match loutItalic '@I\s\+\k\+'
syn region loutItalic matchgroup=loutIBraces start='@I\s*{' matchgroup=loutIBraces end='}' contains=ALLBUT,loutBraceError
syn match loutBold '@B\s\+\k\+'
syn region loutBold matchgroup=loutBBraces start='@B\s*{' matchgroup=loutBBraces end='}' contains=ALLBUT,loutBraceError
syn match loutBoldItalic '@BI\s\+\k\+'
syn region loutBoldItalic matchgroup=loutBIBraces start='@BI\s*{' matchgroup=loutBIBraces end='}' contains=ALLBUT,loutBraceError
syn region loutHeadings matchgroup=loutHeads start='@\(\(Title\)\|\(Caption\)\)\s*{' matchgroup=loutHeads end='}' contains=ALLBUT,loutBraceError

if !exists("did_lout_syntax")
	let did_lout_syntax = 1
	" The default methods for highlighting. Can be overrriden later.
	hi link loutTodo Todo
	hi link loutDefine Define
	hi link loutEOmlDef Define
	hi link loutFunction Function
	hi link loutBraceError Error
	hi link loutNULL Special
	hi link loutComment Comment
	hi link loutSpecial Special
	hi link loutSymbols Character
	hi link loutInclude Include
	hi link loutKeyword Keyword
	hi link loutTag Tag
	hi link loutMath Number
	hi link loutMBraces loutMath

	hi loutItalic term=italic cterm=italic gui=italic
	hi link loutIBraces loutItalic
	hi loutBold term=bold cterm=bold gui=bold
	hi link loutBBraces loutBold
	hi loutBoldItalic term=bold,italic cterm=bold,italic gui=bold,italic
	hi link loutBIBraces loutBoldItalic
	hi loutHeadings term=bold cterm=bold guifg=indianred
	hi link loutHeads loutHeadings
endif

let b:current_syntax = "lout"
