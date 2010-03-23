" Vim syntax file
" Language   : TeX
" Version    : 5.6-4
" Maintainer : Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change: April 7, 2000
"
" Notes:
" 1. If you have a \begin{verbatim} that appears to overrun its boundaries,
"    use %stopzone.
" 2. Run-on equations ($..$ and $$..$$, particularly) can also be stopped
"    by suitable use of %stopzone.
" 3. If you have a slow computer, you may wish to modify
"
"    syn sync maxlines=200
"    syn sync minlines=50
"
"    to values that are more to your liking.
" 4. There is no match-syncing for $...$ and $$...$$; hence large
"    equation blocks constructed that way may exhibit syncing problems.
"    (there's no difference between begin/end patterns)

" Removes any old syntax stuff hanging around
syn clear

" Clusters
" --------
syn cluster texCmdGroup	contains=texCmdBody,texComment,texDelimiter,texDocumentType,texDocumentTypeArgs,texInput,texLength,texLigature,texMathDelim,texMathError,texMathOper,texNewCmd,texNewEnv,texRefZone,texSection,texSectionMarker,texSectionName,texSpecialChar,texStatement,texString,texTypeSize,texTypeStyle
syn cluster texEnvGroup	contains=texMatcher,texMathDelim,texSpecialChar,texStatement
syn cluster texMatchGroup	contains=@texMathZones,texAccent,texBadMath,texComment,texDefCmd,texDelimiter,texDocumentType,texInput,texLength,texLigature,texMatcher,texNewCmd,texNewEnv,texOnlyMath,texParen,texRefZone,texSection,texSpecialChar,texStatement,texString,texTypeSize,texTypeStyle,texZone,texInputFile
syn cluster texMathDelimGroup	contains=texMathDelimBad,texMathDelimKey,texMathDelimSet1,texMathDelimSet2
syn cluster texMathMatchGroup contains=@texMathZones,texComment,texDefCmd,texDelimiter,texDocumentType,texInput,texLength,texLigature,texMathDelim,texMathError,texMathMatcher,texMathOper,texNewCmd,texNewEnv,texRefZone,texSection,texSpecialChar,texStatement,texString,texTypeSize,texTypeStyle,texZone
syn cluster texMathZoneGroup	contains=texComment,texDelimiter,texLength,texMathDelim,texMathError,texMathMatcher,texMathOper,texRefZone,texSpecialChar,texStatement,texTypeSize,texTypeStyle
syn cluster texMathZones	contains=texMathZoneA,texMathZoneB,texMathZoneC,texMathZoneD,texMathZoneE,texMathZoneF,texMathZoneG,texMathZoneH,texMathZoneI,texMathZoneJ,texMathZoneK,texMathZoneL,texMathZoneM,texMathZoneN,texMathZoneO,texMathZoneP,texMathZoneQ,texMathZoneR,texMathZoneS,texMathZoneT,texMathZoneU,texMathZoneV,texMathZoneW

" Try to flag {}, [], and () mismatches
syn region texMatcher	matchgroup=Delimiter start="{" skip="\\\\\|\\[{}]"	end="}"	contains=@texMatchGroup
syn region texMatcher	matchgroup=Delimiter start="\["		end="]"	contains=@texMatchGroup
syn region texParen	start="("		   		end=")"	contains=@texMatchGroup
syn match  texError	"[}\])]"
syn match  texMathError	"}"	contained
syn region texMathMatcher	matchgroup=Delimiter start="{"  skip="\\\\\|\\}"  end="}" end="%stopzone" contained contains=@texMathMatchGroup

" TeX/LaTeX keywords
" Instead of trying to be All Knowing, I just match \..alphameric..
" Note that *.tex files may not have "@" in their \commands
syn match texStatement	"\\[a-zA-Z]\+"
let b:extfname=expand("%:e")
if b:extfname == "sty" || b:extfname == "cls" || b:extfname == "clo" || b:extfname == "dtx" || b:extfname == "ltx"
  syn match texStatement	"\\[a-zA-Z]*@[a-zA-Z@]*"
else
  syn match texError		"\\[a-zA-Z]*@[a-zA-Z@]*"
endif

" TeX/LaTeX delimiters
syn match texDelimiter	"&"
syn match texDelimiter	"\\\\"

" \begin{}/\end{} section markers
syn match texSectionMarker "\\begin\|\\end" nextgroup=texSectionName
syn region texSectionName matchgroup=Delimiter start="{" end="}" contained

" \documentclass, \documentstyle, \usepackage
syn match texDocumentType "\\documentclass\|\\documentstyle\|\\usepackage"		nextgroup=texSectionName,texDocumentTypeArgs
syn region texDocumentTypeArgs matchgroup=Delimiter start="\[" end="]" contained	nextgroup=texSectionName

" TeX input
syn match texInput	"\\input\s\+[a-zA-Z/.0-9]\+"hs=s+7		contains=texStatement
syn match texInputFile	"\\include\(graphics\|list\)\=\(\[.\{-}\]\)\=\s*{.\{-}}"	contains=texStatement,texInputCurlies
syn match texInputFile	"\\\(epsfig\|input\|usepackage\)\s*{.\{-}}"		contains=texStatement,texInputCurlies
syn match texInputCurlies	"[{}]"				contained

" Type Styles (LaTeX 2.09)
syn match texTypeStyle	"\\rm\>"
syn match texTypeStyle	"\\em\>"
syn match texTypeStyle	"\\bf\>"
syn match texTypeStyle	"\\it\>"
syn match texTypeStyle	"\\sl\>"
syn match texTypeStyle	"\\sf\>"
syn match texTypeStyle	"\\sc\>"
syn match texTypeStyle	"\\tt\>"

" Type Styles: attributes, commands, families, etc (LaTeX2E)
syn match texTypeStyle	"\\textbf\>"
syn match texTypeStyle	"\\textit\>"
syn match texTypeStyle	"\\textmd\>"
syn match texTypeStyle	"\\textrm\>"
syn match texTypeStyle	"\\textsc\>"
syn match texTypeStyle	"\\textsf\>"
syn match texTypeStyle	"\\textsl\>"
syn match texTypeStyle	"\\texttt\>"
syn match texTypeStyle	"\\textup\>"

syn match texTypeStyle	"\\mathbf\>"
syn match texTypeStyle	"\\mathcal\>"
syn match texTypeStyle	"\\mathit\>"
syn match texTypeStyle	"\\mathnormal\>"
syn match texTypeStyle	"\\mathrm\>"
syn match texTypeStyle	"\\mathsf\>"
syn match texTypeStyle	"\\mathtt\>"

syn match texTypeStyle	"\\rmfamily\>"
syn match texTypeStyle	"\\sffamily\>"
syn match texTypeStyle	"\\ttfamily\>"

syn match texTypeStyle	"\\itshape\>"
syn match texTypeStyle	"\\scshape\>"
syn match texTypeStyle	"\\slshape\>"
syn match texTypeStyle	"\\upshape\>"

syn match texTypeStyle	"\\bfseries\>"
syn match texTypeStyle	"\\mdseries\>"

" Some type sizes
syn match texTypeSize	"\\tiny\>"
syn match texTypeSize	"\\scriptsize\>"
syn match texTypeSize	"\\footnotesize\>"
syn match texTypeSize	"\\small\>"
syn match texTypeSize	"\\normalsize\>"
syn match texTypeSize	"\\large\>"
syn match texTypeSize	"\\Large\>"
syn match texTypeSize	"\\LARGE\>"
syn match texTypeSize	"\\huge\>"
syn match texTypeSize	"\\Huge\>"

" Sections, subsections, etc
syn match texSection	"\\\(sub\)*section\*\=\>"
syn match texSection	"\\\(title\|author\|part\|chapter\|paragraph\|subparagraph\)\>"
syn match texSection	"\\begin\s*{\s*abstract\s*}\|\\end\s*{\s*abstract\s*}"

" Bad Math (mismatched)
syn match texBadMath	"\\end\s*{\s*\(split\|align\|gather\|alignat\|flalign\|multline\)\s*}"
syn match texBadMath	"\\end\s*{\s*\(equation\|eqnarray\|displaymath\)\*\=\s*}"
syn match texBadMath	"\\[\])]"

" Math Zones
syn region texMathZoneA	start="\\begin\s*{\s*align\*\s*}"	end="\\end\s*{\s*align\*\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneB	start="\\begin\s*{\s*alignat\*\s*}"	end="\\end\s*{\s*alignat\*\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneC	start="\\begin\s*{\s*alignat\s*}"	end="\\end\s*{\s*alignat\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneD	start="\\begin\s*{\s*align\s*}"	end="\\end\s*{\s*align\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneE	start="\\begin\s*{\s*eqnarray\*\s*}"	end="\\end\s*{\s*eqnarray\*\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneF	start="\\begin\s*{\s*eqnarray\s*}"	end="\\end\s*{\s*eqnarray\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneG	start="\\begin\s*{\s*equation\*\s*}"	end="\\end\s*{\s*equation\*\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneH	start="\\begin\s*{\s*equation\s*}"	end="\\end\s*{\s*equation\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneI	start="\\begin\s*{\s*flalign\*\s*}"	end="\\end\s*{\s*flalign\*\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneJ	start="\\begin\s*{\s*flalign\s*}"	end="\\end\s*{\s*flalign\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneK	start="\\begin\s*{\s*gather\*\s*}"	end="\\end\s*{\s*gather\*\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneL	start="\\begin\s*{\s*gather\s*}"	end="\\end\s*{\s*gather\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneM	start="\\begin\s*{\s*math\*\s*}"	end="\\end\s*{\s*math\*\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneN	start="\\begin\s*{\s*math\s*}"	end="\\end\s*{\s*math\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneO	start="\\begin\s*{\s*multline\s*}"	end="\\end\s*{\s*multline\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneP	start="\\begin\s*{\s*split\s*}"	end="\\end\s*{\s*split\s*}"		keepend contains=@texMathZoneGroup
syn region texMathZoneQ	start="\\begin\s*{\s*displaymath\*\s*}"	end="\\end\s*{\s*displaymath\*\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneR	start="\\begin\s*{\s*displaymath\s*}"	end="\\end\s*{\s*displaymath\s*}"	keepend contains=@texMathZoneGroup
syn region texMathZoneS	start="\\begin\s*{\s*multline\*\s*}"	end="\\end\s*{\s*multline\*\s*}"	keepend contains=@texMathZoneGroup

syn region texMathZoneT	matchgroup=Delimiter start="\\("  matchgroup=Delimiter end="\\)\|%stopzone"	keepend contains=@texMathZoneGroup
syn region texMathZoneU	matchgroup=Delimiter start="\\\[" matchgroup=Delimiter end="\\]\|%stopzone"	keepend contains=@texMathZoneGroup
syn region texMathZoneV	matchgroup=Delimiter start="\$"   skip="\\\\\|\\\$" matchgroup=Delimiter end="\$" end="%stopzone" contains=@texMathZoneGroup
syn region texMathZoneW	matchgroup=Delimiter start="\$\$" matchgroup=Delimiter end="\$\$" end="%stopzone"	keepend contains=@texMathZoneGroup

syn match texMathOper	"[_^=]" contained

" \left..something.. and \right..something.. support
syn match   texMathDelimBad	contained	"."
syn match   texMathDelim		"\\\(left\|right\)\>"	nextgroup=texMathDelimSet1,texMathDelimSet2,texMathDelimBad
syn match   texMathDelim		"\\\(left\|right\)arrow\>"
syn match   texMathDelim		"\\lefteqn\>"
syn match   texMathDelimSet2	contained	"\\"	nextgroup=texMathDelimKey,texMathDelimBad
syn match   texMathDelimSet1	contained	"[<>()[\]|/.]\|\\[{}|]"
syn keyword texMathDelimKey	contained	Downarrow	Uparrow	downarrow	lceil	rangle	uparrow
syn keyword texMathDelimKey	contained	Rfloor	backslash	langle	lfloor	rceil

" texAccent (tnx to Karim Belabas) avoids annoying highlighting for accents
syn match texAccent	"\\[bcdvuH][^a-zA-Z]"me=e-1
syn match texAccent	"\\[bcdvuH]$"
syn match texAccent	+\\[=^.\~"`']+
syn match texAccent	+\\['=t'.c^ud"vb~Hr]{[a-zA-Z]}+
syn match texLigature	"\\\([ijolL]\|ae\|oe\|ss\|AA\|AE\|OE\)[^a-zA-Z]"me=e-1
syn match texLigature	"\\\([ijolL]\|ae\|oe\|ss\|AA\|AE\|OE\)$"

" special TeX characters  ( \$ \& \% \# \{ \} \_ \S \P )
syn match texSpecialChar	"\\[$&%#{}_]"
syn match texSpecialChar	"\\[SP@][^a-zA-Z]"me=e-1
syn match texSpecialChar	"\\\\"
syn match texOnlyMath		"[_^]"
syn match texSpecialChar	"\^\^[0-9a-f]\{2}\|\^\^\S"

" Comments:    Normal TeX LaTeX     :   %....
"              Documented TeX Format:  ^^A...    -and-  leading %s (only)
if b:extfname == "dtx"
  syn match texComment	"\^\^A.*$"
  syn match texComment	"^%\+"
else
  syn match texComment	"%.*$"
endif

" separate lines used for verb` and verb# so that the end conditions
" will appropriately terminate.  Ideally vim would let me save a
" character from the start pattern and re-use it in the end-pattern.
syn region texZone	start="\\begin{verbatim}"		end="\\end{verbatim}\|%stopzone"
syn region texZone	start="\\verb`"		end="`\|%stopzone"
syn region texZone	start="\\verb#"		end="#\|%stopzone"
syn region texZone	start="@samp{"		end="}\|%stopzone"
syn region texRefZone	matchgroup=texStatement start="\\cite{"	keepend end="}\|%stopzone"  contains=texComment,texDelimiter
syn region texRefZone	matchgroup=texStatement start="\\label{"	keepend end="}\|%stopzone"  contains=texComment,texDelimiter
syn region texRefZone	matchgroup=texStatement start="\\pageref{"	keepend end="}\|%stopzone"  contains=texComment,texDelimiter
syn region texRefZone	matchgroup=texStatement start="\\ref{"	keepend end="}\|%stopzone"  contains=texComment,texDelimiter

" handle newcommand, newenvironment
syn match  texNewCmd		"\\newcommand"			nextgroup=texCmdName skipwhite skipnl
syn region texCmdName contained matchgroup=Delimiter start="{"rs=s+1  end="}"	nextgroup=texCmdArgs,texCmdBody skipwhite skipnl
syn region texCmdArgs contained matchgroup=Delimiter start="\["rs=s+1 end="]"	nextgroup=texCmdBody skipwhite skipnl
syn region texCmdBody contained matchgroup=Delimiter start="{"rs=s+1 skip="\\\\\|\\[{}]"	matchgroup=Delimiter end="}" contains=@texCmdGroup
syn match  texNewEnv		"\\newenvironment"		nextgroup=texEnvName skipwhite skipnl
syn region texEnvName contained matchgroup=Delimiter start="{"rs=s+1  end="}"	nextgroup=texEnvBgn skipwhite skipnl
syn region texEnvBgn  contained matchgroup=Delimiter start="{"rs=s+1  end="}"	nextgroup=texEnvEnd skipwhite skipnl contains=@texEnvGroup
syn region texEnvEnd  contained matchgroup=Delimiter start="{"rs=s+1  end="}"	skipwhite skipnl contains=@texEnvGroup

syn match texDefCmd		"\\def"			nextgroup=texDefName skipwhite skipnl
syn match texDefName contained	"\\[a-zA-Z]\+"			nextgroup=texCmdBody skipwhite skipnl

" TeX Lengths
syn match  texLength	"\d\+\(\.\d\+\)\=\(bp\|cc\|cm\|dd\|em\|ex\|in\|mm\|pc\|pt\|sp\)"

" TeX String Delimiters
syn match texString	"\(``\|''\|,,\)"

" LaTeX synchronization
syn sync maxlines=200
syn sync minlines=50

syn sync match texSyncMathZoneA	grouphere texMathZoneA	"\\begin\s*{\s*align\*\s*}"
syn sync match texSyncMathZoneB	grouphere texMathZoneB	"\\begin\s*{\s*alignat\*\s*}"
syn sync match texSyncMathZoneC	grouphere texMathZoneC	"\\begin\s*{\s*alignat\s*}"
syn sync match texSyncMathZoneD	grouphere texMathZoneD	"\\begin\s*{\s*align\s*}"
syn sync match texSyncMathZoneE	grouphere texMathZoneE	"\\begin\s*{\s*eqnarray\*\s*}"
syn sync match texSyncMathZoneF	grouphere texMathZoneF	"\\begin\s*{\s*eqnarray\s*}"
syn sync match texSyncMathZoneG	grouphere texMathZoneG	"\\begin\s*{\s*equation\*\s*}"
syn sync match texSyncMathZoneH	grouphere texMathZoneH	"\\begin\s*{\s*equation\s*}"
syn sync match texSyncMathZoneI	grouphere texMathZoneI	"\\begin\s*{\s*flalign\*\s*}"
syn sync match texSyncMathZoneJ	grouphere texMathZoneJ	"\\begin\s*{\s*flalign\s*}"
syn sync match texSyncMathZoneK	grouphere texMathZoneK	"\\begin\s*{\s*gather\*\s*}"
syn sync match texSyncMathZoneL	grouphere texMathZoneL	"\\begin\s*{\s*gather\s*}"
syn sync match texSyncMathZoneM	grouphere texMathZoneM	"\\begin\s*{\s*math\*\s*}"
syn sync match texSyncMathZoneN	grouphere texMathZoneN	"\\begin\s*{\s*math\s*}"
syn sync match texSyncMathZoneO	grouphere texMathZoneO	"\\begin\s*{\s*multline\s*}"
syn sync match texSyncMathZoneP	grouphere texMathZoneP	"\\begin\s*{\s*split\s*}"
syn sync match texSyncMathZoneQ	grouphere texMathZoneQ	"\\begin\s*{\s*displaymath\*\s*}"
syn sync match texSyncMathZoneR	grouphere texMathZoneR	"\\begin\s*{\s*displaymath\s*}"
syn sync match texSyncMathZoneS	grouphere texMathZoneS	"\\begin\s*{\s*multline\*\s*}"
syn sync match texSyncMathZoneT	grouphere texMathZoneT	"\\("
syn sync match texSyncMathZoneU	grouphere texMathZoneU	"\\\["

syn sync match texSyncMathZoneA	groupthere NONE	"\\end\s*{\s*align\*\s*}"
syn sync match texSyncMathZoneB	groupthere NONE	"\\end\s*{\s*alignat\*\s*}"
syn sync match texSyncMathZoneC	groupthere NONE	"\\end\s*{\s*alignat\s*}"
syn sync match texSyncMathZoneD	groupthere NONE	"\\end\s*{\s*align\s*}"
syn sync match texSyncMathZoneE	groupthere NONE	"\\end\s*{\s*eqnarray\*\s*}"
syn sync match texSyncMathZoneF	groupthere NONE	"\\end\s*{\s*eqnarray\s*}"
syn sync match texSyncMathZoneG	groupthere NONE	"\\end\s*{\s*equation\*\s*}"
syn sync match texSyncMathZoneH	groupthere NONE	"\\end\s*{\s*equation\s*}"
syn sync match texSyncMathZoneI	groupthere NONE	"\\end\s*{\s*flalign\*\s*}"
syn sync match texSyncMathZoneJ	groupthere NONE	"\\end\s*{\s*flalign\s*}"
syn sync match texSyncMathZoneK	groupthere NONE	"\\end\s*{\s*gather\*\s*}"
syn sync match texSyncMathZoneL	groupthere NONE	"\\end\s*{\s*gather\s*}"
syn sync match texSyncMathZoneM	groupthere NONE	"\\end\s*{\s*math\*\s*}"
syn sync match texSyncMathZoneN	groupthere NONE	"\\end\s*{\s*math\s*}"
syn sync match texSyncMathZoneO	groupthere NONE	"\\end\s*{\s*multline\s*}"
syn sync match texSyncMathZoneP	groupthere NONE	"\\end\s*{\s*split\s*}"
syn sync match texSyncMathZoneQ	groupthere NONE	"\\end\s*{\s*displaymath\*\s*}"
syn sync match texSyncMathZoneR	groupthere NONE	"\\end\s*{\s*displaymath\s*}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end\s*{\s*multline\*\s*}"
syn sync match texSyncMathZoneT	groupthere NONE	"\\)"
syn sync match texSyncMathZoneU	groupthere NONE	"\\\]"
syn sync match texSyncStop		groupthere NONE	"%stopzone"

" The $..$ and $$..$$ make for impossible sync patterns.
" The following grouptheres coupled with minlines above
" help improve the odds of good syncing.
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{abstract}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{center}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{description}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{enumerate}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{itemize}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{table}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\end{tabular}"
syn sync match texSyncMathZoneS	groupthere NONE	"\\\(sub\)*section"

if !exists("did_tex_syntax_inits")
 let did_tex_syntax_inits = 1

 " TeX highlighting groups which should share similar highlighting
 hi link texBadMath	texError
 hi link texDefCmd	texDef
 hi link texDefName	texDef
 hi link texDocumentType	texCmdName
 hi link texDocumentTypeArgs	texCmdArgs
 hi link texInput	Todo
 hi link texInputCurlies	texDelimiter
 hi link texLigature	texSpecialChar
 hi link texMathDelimBad	texError
 hi link texMathDelimSet1	texMathDelim
 hi link texMathDelimSet2	texMathDelim
 hi link texMathDelimKey	texMathDelim
 hi link texMathMatcher	texMath
 hi link texMathZoneA	texMath
 hi link texMathZoneB	texMath
 hi link texMathZoneC	texMath
 hi link texMathZoneD	texMath
 hi link texMathZoneE	texMath
 hi link texMathZoneF	texMath
 hi link texMathZoneG	texMath
 hi link texMathZoneH	texMath
 hi link texMathZoneI	texMath
 hi link texMathZoneJ	texMath
 hi link texMathZoneK	texMath
 hi link texMathZoneL	texMath
 hi link texMathZoneM	texMath
 hi link texMathZoneN	texMath
 hi link texMathZoneO	texMath
 hi link texMathZoneP	texMath
 hi link texMathZoneQ	texMath
 hi link texMathZoneR	texMath
 hi link texMathZoneS	texMath
 hi link texMathZoneT	texMath
 hi link texMathZoneU	texMath
 hi link texMathZoneV	texMath
 hi link texMathZoneW	texMath
 hi link texOnlyMath	texError
 hi link texSectionMarker	texCmdName
 hi link texSectionName	texSection
 hi link texTypeSize	texType
 hi link texTypeStyle	texType

 " Basic TeX highlighting groups
 hi link texCmdArgs	Number
 hi link texCmdName	Statement
 hi link texComment	Comment
 hi link texDef	Statement
 hi link texDelimiter	Delimiter
 hi link texError	Error
 hi link texLength	Number
 hi link texMath	Special
 hi link texMathDelim	Statement
 hi link texMathOper	Operator
 hi link texNewCmd	Statement
 hi link texNewEnv	Statement
 hi link texRefZone	Special
 hi link texSection	PreCondit
 hi link texSpecialChar	SpecialChar
 hi link texStatement	Statement
 hi link texString	String
 hi link texType	Type
 hi link texZone	PreCondit
endif

unlet b:extfname
let   b:current_syntax = "tex"
" vim: ts=15
