" Vim syntax file
" Language:	DCL (Digital Command Language - vms)
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	April 2, 1999

" Removes any old syntax stuff hanging around
syn clear

set iskeyword=$,@,48-57,_

syn case ignore
syn keyword dclInstr	accounting	del[ete]	gen[cat]	mou[nt]	run
syn keyword dclInstr	all[ocate]	dep[osit]	gen[eral]	ncp	run[off]
syn keyword dclInstr	ana[lyze]	dia[gnose]	gos[ub]	ncs	sca
syn keyword dclInstr	app[end]	dif[ferences]	got[o]	on	sea[rch]
syn keyword dclInstr	ass[ign]	dir[ectory]	hel[p]	ope[n]	set
syn keyword dclInstr	att[ach]	dis[able]	ico[nv]	pas[cal]	sho[w]
syn keyword dclInstr	aut[horize]	dis[connect]	if	pas[sword]	sor[t]
syn keyword dclInstr	aut[ogen]	dis[mount]	ini[tialize]	pat[ch]	spa[wn]
syn keyword dclInstr	bac[kup]	dpm[l]	inq[uire]	pca	sta[rt]
syn keyword dclInstr	cal[l]	dqs	ins[tall]	pho[ne]	sto[p]
syn keyword dclInstr	can[cel]	dsr	job	pri[nt]	sub[mit]
syn keyword dclInstr	cc	dst[graph]	lat[cp]	pro[duct]	sub[routine]
syn keyword dclInstr	clo[se]	dtm	lib[rary]	psw[rap]	swx[cr]
syn keyword dclInstr	cms	dum[p]	lic[ense]	pur[ge]	syn[chronize]
syn keyword dclInstr	con[nect]	edi[t]	lin[k]	qde[lete]	sys[gen]
syn keyword dclInstr	con[tinue]	ena[ble]	lmc[p]	qse[t]	sys[man]
syn keyword dclInstr	con[vert]	end[subroutine]	loc[ale]	qsh[ow]	tff
syn keyword dclInstr	cop[y]	eod	log[in]	rea[d]	then
syn keyword dclInstr	cre[ate]	eoj	log[out]	rec[all]	typ[e]
syn keyword dclInstr	cxx	exa[mine]	lse[dit]	rec[over]	uil
syn keyword dclInstr	cxx[l_help]	exc[hange]	mac[ro]	ren[ame]	unl[ock]
syn keyword dclInstr	dea[llocate]	exi[t]	mai[l]	rep[ly]	ves[t]
syn keyword dclInstr	dea[ssign]	fdl	mer[ge]	req[uest]	vie[w]
syn keyword dclInstr	deb[ug]	flo[wgraph]	mes[sage]	ret[urn]	wai[t]
syn keyword dclInstr	dec[k]	fon[t]	mms	rms	wri[te]
syn keyword dclInstr	def[ine]	for[tran]

syn keyword dclLexical	f$context	f$edit	  f$getjpi	f$message	f$setprv
syn keyword dclLexical	f$csid	f$element	  f$getqui	f$mode	f$string
syn keyword dclLexical	f$cvsi	f$environment	  f$getsyi	f$parse	f$time
syn keyword dclLexical	f$cvtime	f$extract	  f$identifier	f$pid	f$trnlnm
syn keyword dclLexical	f$cvui	f$fao	  f$integer	f$privilege	f$type
syn keyword dclLexical	f$device	f$file_attributes f$length	f$process	f$user
syn keyword dclLexical	f$directory	f$getdvi	  f$locate	f$search	f$verify

syn match   dclMdfy	"/\I\i*"	nextgroup=dclMdfySet,dclMdfySetString
syn match   dclMdfySet	"=[^ \t"]*"	contained
syn region  dclMdfySet	matchgroup=dclMdfyBrkt start="=\[" matchgroup=dclMdfyBrkt end="]"	contains=dclMdfySep
syn region  dclMdfySetString	start='="'	skip='""'	end='"'	contained
syn match   dclMdfySep	"[:,]"	contained

" Numbers
syn match   dclNumber	"\d\+"

" Varname (mainly to prevent dclNumbers from being recognized when part of a dclVarname)
syn match   dclVarname	"\I\i*"

" Filenames (devices, paths)
syn match   dclDevice	"\I\i*\(\$\I\i*\)\=:[^=]"me=e-1		nextgroup=dclDirPath,dclFilename
syn match   dclDirPath	"\[\(\I\i*\.\)*\I\i*\]"		contains=dclDirSep	nextgroup=dclFilename
syn match   dclFilename	"\I\i*\$\(\I\i*\)\=\.\(\I\i*\)*\(;\d\+\)\="	contains=dclDirSep
syn match   dclFilename	"\I\i*\.\(\I\i*\)\=\(;\d\+\)\="	contains=dclDirSep	contained
syn match   dclDirSep	"[[\].;]"

" Strings
syn region  dclString	start='"'	skip='""'	end='"'

" $ stuff and comments
syn match   dclStart	"^\$"	skipwhite nextgroup=dclExe
syn match   dclContinue	"-$"
syn match   dclComment	"^\$!.*$"	contains=dclStart,dclTodo
syn match   dclExe	"\I\i*"	contained
syn match   dclTodo	"DEBUG\|TODO"	contained

" Assignments and Operators
syn match   dclAssign	":==\="
syn match   dclAssign	"="
syn match   dclOper	"--\|+\|\*\|/"
syn match   dclLogOper	"\.[a-zA-Z][a-zA-Z][a-zA-Z]\=\." contains=dclLogical,dclLogSep
syn keyword dclLogical contained	and	ge	gts	lt	nes
syn keyword dclLogical contained	eq	ges	le	lts	not
syn keyword dclLogical contained	eqs	gt	les	ne	or
syn match   dclLogSep	"\."		contained

" @command procedures
syn match   dclCmdProcStart	"@"			nextgroup=dclCmdProc
syn match   dclCmdProc	"\I\i*\(\.\I\i*\)\="	contained
syn match   dclCmdProc	"\I\i*:"		contained	nextgroup=dclCmdDirPath,dclCmdProc
syn match   dclCmdDirPath	"\[\(\I\i*\.\)*\I\i*\]"	contained	nextgroup=delCmdProc

" labels
syn match   dclGotoLabel	"^\$\s*\I\i*:\s*$"	contains=dclStart

" parameters
syn match   dclParam	"'\I[a-zA-Z0-9_$]*'\="

" () matching (the clusters are commented out until a vim/vms comes out for v5.2+)
"syn cluster dclNextGroups	contains=dclCmdDirPath,dclCmdProc,dclCmdProc,dclDirPath,dclFilename,dclFilename,dclMdfySet,dclMdfySetString,delCmdProc,dclExe,dclTodo
"syn region  dclFuncList	matchgroup=Delimiter start="(" matchgroup=Delimiter end=")" contains=ALLBUT,@dclNextGroups
syn region  dclFuncList	matchgroup=Delimiter start="(" matchgroup=Delimiter end=")" contains=ALLBUT,dclCmdDirPath,dclCmdProc,dclCmdProc,dclDirPath,dclFilename,dclFilename,dclMdfySet,dclMdfySetString,delCmdProc,dclExe,dclTodo
syn match   dclError	")"

if !exists("did_dcl_syntax_inits")
 let did_dcl_syntax_inits = 1
 hi link dclLogOper	dclError
 hi link dclLogical	dclOper
 hi link dclLogSep	dclSep

 hi link dclAssign	Operator
 hi link dclCmdProc	Special
 hi link dclCmdProcStart	Operator
 hi link dclComment	Comment
 hi link dclContinue	Statement
 hi link dclDevice	Identifier
 hi link dclDirPath	Identifier
 hi link dclDirPath	Identifier
 hi link dclDirSep	Delimiter
 hi link dclError	Error
 hi link dclExe		Statement
 hi link dclFilename	NONE
 hi link dclGotoLabel	Label
 hi link dclInstr	Statement
 hi link dclLexical	Function
 hi link dclMdfy	Type
 hi link dclMdfyBrkt	Delimiter
 hi link dclMdfySep	Delimiter
 hi link dclMdfySet	Type
 hi link dclMdfySetString	String
 hi link dclNumber	Number
 hi link dclOper	Operator
 hi link dclParam	Special
 hi link dclSep		Delimiter
 hi link dclStart	Delimiter
 hi link dclString	String
 hi link dclTodo	Todo
endif

let b:current_syntax = "dcl"

" vim: ts=16
