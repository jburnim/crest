" Vim syntax file
" Language:		shell (sh) Korn shell (ksh) bash (sh)
" Maintainer:		Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Previous Maintainer:	Lennart Schultz <Lennart.Schultz@ecmwf.int>
" Last Change:	June 6, 2000
" Version: 1.16
"
" Using the following VIM variables:
" b:is_kornshell               if defined, enhance with kornshell syntax
" b:is_bash                    if defined, enhance with bash syntax
"
" This file includes many ideas from Éric Brunet (eric.brunet@ens.fr)
" Belated History:
" v1.14 : Mar 24 2000 : ksh did support but bash does too: ${#[#@]} constructs
"         Apr 17 2000 : included $((...)) for ksh

" Remove any old syntax stuff hanging around
syn clear
" sh syntax is case sensitive
syn case match

" This one is needed INSIDE a CommandSub, so that
" `echo bla` be correct
syn region shEcho matchgroup=shStatement start="\<echo\>"  skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|()]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=shNumber,shCommandSub,shSinglequote,shDeref,shSpecialVar,shSpecial,shOperator,shDoubleQuote,shCharClass
syn region shEcho matchgroup=shStatement start="\<print\>" skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|()]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=shNumber,shCommandSub,shSinglequote,shDeref,shSpecialVar,shSpecial,shOperator,shDoubleQuote,shCharClass

" This must be after the strings, so that bla \" be correct
syn region shEmbeddedEcho contained matchgroup=shStatement start="\<print\>" skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|`)]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=shNumber,shSinglequote,shDeref,shSpecialVar,shSpecial,shOperator,shDoubleQuote,shCharClass

"Error Codes
syn match   shDoError "\<done\>"
syn match   shIfError "\<fi\>"
syn match   shInError "\<in\>"
syn match   shCaseError ";;"
syn match   shEsacError "\<esac\>"
syn match   shCurlyError "}"
syn match   shParenError ")"
if exists("b:is_kornshell")
 syn match     shDTestError "]]"
endif
syn match     shTestError "]"

" Options interceptor
syn match   shOption  "\s[\-+][a-zA-Z0-9]\+\>"ms=s+1

" error clusters:
"================
syn cluster shErrorList	contains=shCaseError,shCurlyError,shDTestError,shDerefError,shDoError,shEsacError,shIfError,shInError,shParenError,shTestError
syn cluster shErrorFuncList	contains=shDerefError
syn cluster shErrorLoopList	contains=shCaseError,shDTestError,shDerefError,shDoError,shInError,shParenError,shTestError
syn cluster shErrorCaseList	contains=shCaseError,shDerefError,shDTestError,shDoError,shInError,shParenError,shTestError
syn cluster shErrorColonList	contains=shDerefError
syn cluster shErrorNoneList	contains=shDerefError

" clusters: contains=@... clusters
"==================================
syn cluster shCaseEsacList	contains=shCaseStart,shCase,shCaseBar,shCaseIn,shComment,shDeref,shCommandSub
syn cluster shIdList	contains=shCommandSub,shWrapLineOperator,shIdWhiteSpace,shDeref,shSpecial
syn cluster shDblQuoteList	contains=shCommandSub,shDeref,shSpecial,shPosnParm

" clusters: contains=ALLBUT,@... clusters
"=========================================
syn cluster shCaseList	contains=shFunction,shCase,shCaseStart,shCaseBar,shDblBrace,shDerefOp,shDerefText,@shErrorCaseList,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shColonList	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shDerefOp,shDerefText,shFunction,shTestOpr,@shErrorColonList,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shCommandSubList1	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shCommandSub,shDerefOp,shDerefText,shEcho,shFunction,shTestOpr,@shErrorList,shDerefVar,shDerefOpError,shStringSpecial,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shCommandSubList2	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shDerefOp,shDerefText,shEcho,shFunction,shTestOpr,@shErrorList,shDerefVar,shDerefOpError,shStringSpecial,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shLoopList	contains=shFunction,@shErrorLoopList,shCase,shCaseStart,shInEsac,shCaseBar,shDerefOp,shDerefText,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shExprList1	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shDerefOp,shDerefText,shFunction,shSetList,@shErrorNoneList,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shExprList2	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shDerefOp,shDerefText,@shErrorNoneList,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shSubShList	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shDerefOp,shDerefText,shParenError,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shDerefError,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shTestList	contains=shCase,shCaseStart,shCaseBar,shDblBrace,shDTestError,shDerefError,shDerefOp,shDerefText,shExpr,shFunction,shSetList,shTestError,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier
syn cluster shFunctionList	contains=shCase,shCaseStart,shCaseBar,shDblBrace,@shErrorFuncList,shDerefOp,shDerefText,shFunction,shDerefVar,shDerefOpError,shStringSpecial,shSkipInitWS,shIdWhiteSpace,shDerefTextError,shPattern,shSetIdentifier

" Tests
"======
syn region  shExpr transparent matchgroup=shOperator start="\[" skip=+\\\\\|\\$+ end="\]" contains=ALLBUT,@shTestList
syn region  shExpr transparent matchgroup=shStatement start="\<test\>" skip=+\\\\\|\\$+ matchgroup=NONE end="[;&|]"me=e-1 end="$" contains=ALLBUT,@shExprList1
if exists("b:is_kornshell")
 syn region  shDblBrace transparent matchgroup=shOperator start="\[\[" skip=+\\\\\|\\$+ end="\]\]" contains=ALLBUT,@shTestList
endif
syn match   shTestOpr contained "[!=]\|-.\>\|-\(nt\|ot\|ef\|eq\|ne\|lt\|le\|gt\|ge\)\>"

" do, if
syn region shDo  transparent matchgroup=shConditional start="\<do\>" matchgroup=shConditional end="\<done\>" contains=ALLBUT,@shLoopList
syn region shIf  transparent matchgroup=shConditional start="\<if\>" matchgroup=shConditional end="\<fi\>"   contains=ALLBUT,@shLoopList
syn region shFor matchgroup=shStatement start="\<for\>" end="\<in\>" end="\<do\>"me=e-2	contains=ALLBUT,@shLoopList

" case
syn region  shCaseEsac	matchgroup=shConditional start="\<case\>" matchgroup=shConditional end="\<esac\>" contains=@shCaseEsacList
syn keyword shCaseIn	contained skipwhite skipnl in			nextgroup=shCase,shCaseStart,shCaseBar,shComment
syn region  shCase	contained skipwhite skipnl matchgroup=shConditional start="[^$()]\{-})"ms=s,hs=e  end=";;" end="esac"me=s-1 contains=ALLBUT,@shCaseList nextgroup=shCase,shCaseStart,shCaseBar,shComment
syn match   shCaseStart	contained skipwhite skipnl "("		nextgroup=shCase
syn match   shCaseBar	contained "[^|)]\{-}|"hs=e			nextgroup=shCase,shCaseStart,shCaseBar

syn region shExpr  transparent matchgroup=shOperator start="{" end="}"		contains=ALLBUT,@shExprList2
syn region shSubSh transparent matchgroup=shOperator start="(" end=")"		contains=ALLBUT,@shSubShList

" Misc
"=====
syn match   shOperator	"[!&;|]"
syn match   shOperator	"\[\[[^:]\|]]"
syn match   shCharClass	"\[:\(backspace\|escape\|return\|xdigit\|alnum\|alpha\|blank\|cntrl\|digit\|graph\|lower\|print\|punct\|space\|upper\|tab\):\]"
syn match   shOperator	"!\=="	skipwhite nextgroup=shPattern
syn match   shPattern	"\<\S\+"	contained contains=shSinglequote,shDoublequote,shDeref
syn match   shWrapLineOperator "\\$"
syn region  shCommandSub   start="`" skip="\\`" end="`" contains=ALLBUT,@shCommandSubList1

" $(..) is not supported by sh (Bourne shell).  However, apparently
" some systems (HP?) have as their /bin/sh a (link to) Korn shell
" (ie. Posix compliant shell).  /bin/ksh should work for those
" systems too, however, so the following syntax will flag $(..) as
" an Error under /bin/sh.  By consensus of vimdev'ers!
if exists("b:is_kornshell") || exists("b:is_bash")
 syn region shCommandSub matchgroup=shDeref start="$(" end=")" contains=ALLBUT,@shCommandSubList2
 syn region shArithmetic matchgroup=shDeref start="$((" end="))" contains=ALLBUT,@shCommandSubList2
 syn match shSkipInitWS contained	"^\s\+"
else
 syn region shCommandSub matchgroup=Error start="$(" end=")" contains=ALLBUT,@shCommandSubList2
endif

if exists("b:is_bash")
 syn keyword bashSpecialVariables contained	BASH	HISTCONTROL	LANG	OPTERR	PWD
 syn keyword bashSpecialVariables contained	BASH_ENV	HISTFILE	LC_ALL	OPTIND	RANDOM
 syn keyword bashSpecialVariables contained	BASH_VERSINFO	HISTFILESIZE	LC_COLLATE	OSTYPE	REPLY
 syn keyword bashSpecialVariables contained	BASH_VERSION	HISTIGNORE	LC_MESSAGES	PATH	SECONDS
 syn keyword bashSpecialVariables contained	CDPATH	HISTSIZE	LINENO	PIPESTATUS	SHELLOPTS
 syn keyword bashSpecialVariables contained	DIRSTACK	HOME	MACHTYPE	PPID	SHLVL
 syn keyword bashSpecialVariables contained	EUID	HOSTFILE	MAIL	PROMPT_COMMAND	TIMEFORMAT
 syn keyword bashSpecialVariables contained	FCEDIT	HOSTNAME	MAILCHECK	PS1	TIMEOUT
 syn keyword bashSpecialVariables contained	FIGNORE	HOSTTYPE	MAILPATH	PS2	UID
 syn keyword bashSpecialVariables contained	GLOBIGNORE	IFS	OLDPWD	PS3	auto_resume
 syn keyword bashSpecialVariables contained	GROUPS	IGNOREEOF	OPTARG	PS4	histchars
 syn keyword bashSpecialVariables contained	HISTCMD	INPUTRC
 syn keyword bashStatement		chmod	fgrep	install	rm	sort
 syn keyword bashStatement		clear	find	less	rmdir	strip
 syn keyword bashStatement		du	gnufind	ls	rpm	tail
 syn keyword bashStatement		egrep	gnugrep	mkdir	sed	touch
 syn keyword bashStatement		expr	grep	mv	sleep
 syn keyword bashAdminStatement	daemon	killproc	reload	start	stop
 syn keyword bashAdminStatement	killall	nice	restart	status
endif

if exists("b:is_kornshell")
 syn keyword kshSpecialVariables contained	CDPATH	HISTFILE	MAILCHECK	PPID	RANDOM
 syn keyword kshSpecialVariables contained	COLUMNS	HISTSIZE	MAILPATH	PS1	REPLY
 syn keyword kshSpecialVariables contained	EDITOR	HOME	OLDPWD	PS2	SECONDS
 syn keyword kshSpecialVariables contained	ENV	IFS	OPTARG	PS3	SHELL
 syn keyword kshSpecialVariables contained	ERRNO	LINENO	OPTIND	PS4	TMOUT
 syn keyword kshSpecialVariables contained	FCEDIT	LINES	PATH	PWD	VISUAL
 syn keyword kshSpecialVariables contained	FPATH	MAIL
 syn keyword kshStatement		cat	expr	less	printenv	strip
 syn keyword kshStatement		chmod	fgrep	ls	rm	stty
 syn keyword kshStatement		clear	find	mkdir	rmdir	tail
 syn keyword kshStatement		cp	grep	mv	sed	touch
 syn keyword kshStatement		du	install	nice	sort	tput
 syn keyword kshStatement		egrep	killall
endif

syn match   shSource	"^\.\s"
syn match   shSource	"\s\.\s"
syn region  shColon	start="^\s*:" end="$\|" end="#"me=e-1 contains=ALLBUT,@shColonList

" Comments
"=========
syn keyword	shTodo    contained	TODO
syn match	shComment		"#.*$" contains=shTodo

" String and Character constants
"===============================
syn match   shNumber		"-\=\<\d\+\>"
syn match   shSpecial	contained	"\\\d\d\d\|\\[abcfnrtv]"
syn region  shSinglequote	matchgroup=shOperator start=+'+ end=+'+		contains=shStringSpecial
syn region  shDoubleQuote     matchgroup=shOperator start=+"+ skip=+\\"+ end=+"+	contains=@shDblQuoteList,shStringSpecial
syn match   shStringSpecial	contained	"[^[:print:]]"
syn match   shSpecial		"\\[\\\"\'`$]"

" File redirection highlighted as operators
"==========================================
syn match	shRedir	"\d\=>\(&[-0-9]\)\="
syn match	shRedir	"\d\=>>-\="
syn match	shRedir	"\d\=<\(&[-0-9]\)\="
syn match	shRedir	"\d<<-\="

" Shell Input Redirection (Here Documents)
syn region shHereDoc matchgroup=shRedir start="<<\s*\**END[a-zA-Z_0-9]*\**"  matchgroup=shRedir end="^END[a-zA-Z_0-9]*$"
syn region shHereDoc matchgroup=shRedir start="<<-\s*\**END[a-zA-Z_0-9]*\**" matchgroup=shRedir end="^\t*END[a-zA-Z_0-9]*$"
syn region shHereDoc matchgroup=shRedir start="<<\s*\**EOF\**"  matchgroup=shRedir end="^EOF$"
syn region shHereDoc matchgroup=shRedir start="<<-\s*\**EOF\**" matchgroup=shRedir end="^\t*EOF$"
syn region shHereDoc matchgroup=shRedir start="<<\s*\**\.\**"  matchgroup=shRedir end="^\.$"
syn region shHereDoc matchgroup=shRedir start="<<-\s*\**\.\**" matchgroup=shRedir end="^\t*\.$"

" Identifiers
"============
syn match  shVariable "\<\h\w*="me=e-1	nextgroup=shSetIdentifier
syn match  shIdWhiteSpace  contained	"\s"
syn match  shSetIdentifier contained	"=" nextgroup=shString,shPattern
if exists("b:is_bash")
 syn region shSetList matchgroup=shStatement start="\<\(declare\|typeset\|local\|export\|set\|unset\)\>[^/]"me=e-1 end="$" matchgroup=shOperator end="[;&]"me=e-1 matchgroup=NONE end="#\|="me=e-1 contains=@shIdList
elseif exists("b:is_kornshell")
 syn region shSetList matchgroup=shStatement start="\<\(typeset\|set\|export\|unset\)\>[^/]"me=e-1 end="$" matchgroup=shOperator end="[;&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
else
 syn region shSetList matchgroup=shStatement start="\<\(set\|export\|unset\)\>[^/]"me=e-1 end="$" matchgroup=shOperator end="[;&]" matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
endif

" The [^/] in the start pattern is a kludge to avoid bad
" highlighting with cd /usr/local/lib...
syn region shFunction	transparent	matchgroup=shFunctionName start="^\s*\<\h\w*\>\s*()\s*{" end="}" contains=ALLBUT,@shFunctionList
syn region shDeref	oneline	start="\${" end="}"	contains=shDeref,shDerefVar
syn match  shDeref		"\$\h\w*\>"
syn match  shPosnParm		"\$[-#@*$?!0-9]"
syn match  shDerefVar	contained	"\d\+\>"		nextgroup=shDerefOp,shDerefError,shDerefOpError,shExpr
syn match  shDerefVar	contained	"\h\w*\>"		nextgroup=shDerefOp,shDerefError,shDerefOpError,shExpr
if exists("b:is_bash")
 syn match  shDerefVar	contained	"[-@*?!]"		nextgroup=shDerefOp,shDerefError,shDerefOpError,shExpr
else
 syn match  shDerefVar	contained	"[-#@*?!]"		nextgroup=shDerefOp,shDerefError,shDerefOpError,shExpr
endif
syn match  shDerefVar	contained	"\$[^{(]"me=s+1	nextgroup=shDerefOp,shDerefError,shDerefOpError,shExpr
syn match  shDerefOpError	contained	"[^:}[]"
syn match  shDerefOp	contained	":\=[-=?+]"		nextgroup=shDerefText
syn region shDerefText	contained	start="[^{}]"	end="}"me=e-1	contains=shDeref,shCommandSub,shDoubleQuote,shSingleQuote,shDerefTextError
syn match  shDerefTextError	contained	"^."
syn match  shDerefError	contained	"\s.\{-}}"me=e-1
syn region shDerefText	contained	start="{"	end="}"	contains=shDeref,shCommandSub

if exists("b:is_kornshell") || exists("b:is_bash")
 syn match shDerefVar	contained	"#\(\d\+\|\h\w*\)"	nextgroup=shDerefOp,shDerefError,shDerefOpError,shExpr
 syn match shDerefOp	contained	"##\|%%\|[#%]"		nextgroup=shDerefText
endif

" A bunch of useful sh keywords
syn keyword shStatement	break	eval	newgrp	return	ulimit
syn keyword shStatement	cd	exec	pwd	shift	umask
syn keyword shStatement	chdir	exit	read	test	wait
syn keyword shStatement	continue	kill	readonly	trap
syn keyword shConditional	elif	else	then	while

if exists("b:is_kornshell") || exists("b:is_bash")
 syn keyword shFunction	function
 syn keyword shRepeat	select	until
 syn keyword shStatement	alias	fg	integer	printf	times
 syn keyword shStatement	autoload	functions	jobs	r	true
 syn keyword shStatement	bg	getopts	let	stop	type
 syn keyword shStatement	false	hash	nohup	suspend	unalias
 syn keyword shStatement	fc	history	print	time	whence

 if exists("b:is_bash")
  syn keyword shStatement	bind	disown	help	popd	shopt
  syn keyword shStatement	builtin	enable	logout	pushd	source
  syn keyword shStatement	dirs
 else
  syn keyword shStatement	login	newgrp
 endif
endif

" Syncs
" =====
if !exists("sh_minlines")
  let sh_minlines = 200
endif
if !exists("sh_maxlines")
  let sh_maxlines = 2 * sh_minlines
endif
exec "syn sync minlines=" . sh_minlines . " maxlines=" . sh_maxlines
syn sync match shDoSync       grouphere  shDo       "\<do\>"
syn sync match shDoSync       groupthere shDo       "\<done\>"
syn sync match shIfSync       grouphere  shIf       "\<if\>"
syn sync match shIfSync       groupthere shIf       "\<fi\>"
syn sync match shForSync      grouphere  shFor      "\<for\>"
syn sync match shForSync      groupthere shFor      "\<in\>"
syn sync match shCaseEsacSync grouphere  shCaseEsac "\<case\>"
syn sync match shCaseEsacSync groupthere shCaseEsac "\<esac\>"

" Highlighting
" ============
if !exists("did_sh_syntax_inits")
 " The default methods for highlighting. Can be overridden later
 let did_sh_syntax_inits = 1

 hi link shCaseBar		shConditional
 hi link shCaseIn		shConditional
 hi link shCaseStart		shConditional
 hi link shColon		shStatement
 hi link shDeref		shShellVariables
 hi link shDerefOp		shOperator
 hi link shDerefVar		shShellVariables
 hi link shDoubleQuote		shString
 hi link shEcho		shString
 hi link shEmbeddedEcho		shString
 hi link shHereDoc		shString
 hi link shOption		shCommandSub
 hi link shPattern		shString
 hi link shPosnParm		shShellVariables
 hi link shRedir		shOperator
 hi link shSinglequote		shString
 hi link shSource		shOperator
 hi link shStringSpecial		shSpecial
 hi link shTestOpr		shConditional
 hi link shVariable		shSetList
 hi link shWrapLineOperator		shOperator

 if exists("b:is_bash")
  hi link bashAdminStatement		shStatement
  hi link bashSpecialVariables	shShellVariables
  hi link bashStatement		shStatement
 endif
 if exists("b:is_kornshell")
  hi link kshSpecialVariables		shShellVariables
  hi link kshStatement		shStatement
 endif

 hi link shCaseError		Error
 hi link shCurlyError		Error
 hi link shDerefError		Error
 hi link shDerefOpError		Error
 hi link shDerefTextError		Error
 hi link shDoError		Error
 hi link shEsacError		Error
 hi link shIfError		Error
 hi link shInError		Error
 hi link shParenError		Error
 hi link shTestError		Error
 if exists("b:is_kornshell")
  hi link shDTestError		Error
 endif

 hi link shArithmetic		Special
 hi link shCharClass		Identifier
 hi link shCommandSub		Special
 hi link shComment		Comment
 hi link shConditional		Conditional
 hi link shFunction		Function
 hi link shFunctionName		Function
 hi link shNumber		Number
 hi link shOperator		Operator
 hi link shRepeat		Repeat
 hi link shSetList		Identifier
 hi link shShellVariables		PreProc
 hi link shSpecial		Special
 hi link shStatement		Statement
 hi link shString		String
 hi link shTodo		Todo
endif

" Current Syntax
" ==============
if exists("b:is_bash")
 let b:current_syntax = "bash"
elseif exists("b:is_kornshell")
 let b:current_syntax = "ksh"
else
 let b:current_syntax = "sh"
endif

" vim: ts=15
