" Vim syntax file
" Language:	SiCAD (procedure language)
" Maintainer:	Zsolt Branyiczky <zbranyiczky@lmark.mgx.hu>
" Last Change:	1999 Dec 20
" URL:		http://lmark.mgx.hu/download/vim/syntax/sicad.vim

" spaces are used in (auto)indents since sicad hates tabulator characters
set expandtab

" remove any old syntax stuff hanging around
syn clear

" used syntax highlighting in a sql command
syn include @SQL <sfile>:p:h/sql.vim

" ignore case
syn case ignore

" most important commands - not listed by ausku
syn keyword sicadStatement	define
syn keyword sicadStatement	dialog
syn keyword sicadStatement	do
syn keyword sicadStatement	dop contained
syn keyword sicadStatement	end
syn keyword sicadStatement	enddo
syn keyword sicadStatement	endp
syn keyword sicadStatement	erroff
syn keyword sicadStatement	erron
syn keyword sicadStatement	exitp
syn keyword sicadGoto		goto contained
syn keyword sicadStatement	hh
syn keyword sicadStatement	if
syn keyword sicadStatement	in
syn keyword sicadStatement	msgsup
syn keyword sicadStatement	out
syn keyword sicadStatement	padd
syn keyword sicadStatement	parbeg
syn keyword sicadStatement	parend
syn keyword sicadStatement	pdoc
syn keyword sicadStatement	pprot
syn keyword sicadStatement	procd
syn keyword sicadStatement	procn
syn keyword sicadStatement	psav
syn keyword sicadStatement	psel
syn keyword sicadStatement	psymb
syn keyword sicadStatement	ptrace
syn keyword sicadStatement	ptstat
syn keyword sicadStatement	set
syn keyword sicadstatement      sql contained
syn keyword sicadStatement	step
syn keyword sicadStatement	sys
syn keyword sicadStatement	ww

" functions
syn match sicadStatement	"\<atan("me=e-1
syn match sicadStatement	"\<atan2("me=e-1
syn match sicadStatement	"\<cos("me=e-1
syn match sicadStatement	"\<dist("me=e-1
syn match sicadStatement	"\<exp("me=e-1
syn match sicadStatement	"\<log("me=e-1
syn match sicadStatement	"\<log10("me=e-1
syn match sicadStatement	"\<sin("me=e-1
syn match sicadStatement	"\<sqrt("me=e-1
syn match sicadStatement	"\<tanh("me=e-1
syn match sicadStatement	"\<x("me=e-1
syn match sicadStatement	"\<y("me=e-1
syn match sicadStatement	"\<v("me=e-1
syn match sicadStatement	"\<x%g\=p[0-9]\{1,2}\>"me=s+1
syn match sicadStatement	"\<y%g\=p[0-9]\{1,2}\>"me=s+1

" logical operators
syn match sicadOperator	"\.and\."
syn match sicadOperator	"\.ne\."
syn match sicadOperator	"\.not\."
syn match sicadOperator	"\.eq\."
syn match sicadOperator	"\.ge\."
syn match sicadOperator	"\.gt\."
syn match sicadOperator	"\.le\."
syn match sicadOperator	"\.lt\."
syn match sicadOperator	"\.or\."

" variable name
syn match sicadIdentifier	"%g\=[irpt][0-9]\{1,2}\>"
syn match sicadIdentifier	"%g\=l[0-9]\>"   " separated logical varible
syn match sicadIdentifier	"%g\=[irptl]("me=e-1
syn match sicadIdentifier	"%error\>"
syn match sicadIdentifier	"%nsel\>"
syn match sicadIdentifier	"%nvar\>"
syn match sicadIdentifier	"%scl\>"
syn match sicadIdentifier	"%wd\>"
syn match sicadIdentifier	"\$[irt][0-9]\{1,2}\>" contained  " in sql statements

" label
syn match sicadLabel1	"^ *\.[a-z][a-z0-9]\{0,7} \+[^ ]"me=e-1
syn match sicadLabel1	"^ *\.[a-z][a-z0-9]\{0,7}\*"me=e-1
syn match sicadLabel2	"\<goto \.\=[a-z][a-z0-9]\{0,7}\>" contains=sicadGoto
syn match sicadLabel2	"\<goto\.[a-z][a-z0-9]\{0,7}\>" contains=sicadGoto

" boolean
syn match sicadBoolean	"\.[ft]\."
" integer without sign
syn match sicadNumber	"\<[0-9]\+\>"
" floating point number, with dot, optional exponent
syn match sicadFloat	"\<[0-9]\+\.[0-9]*\(e[-+]\=[0-9]\+\)\=\>"
" floating point number, starting with a dot, optional exponent
syn match sicadFloat	"\.[0-9]\+\(e[-+]\=[0-9]\+\)\=\>"
" floating point number, without dot, with exponent
syn match sicadFloat	"\<[0-9]\+e[-+]\=[0-9]\+\>"

" without this extraString definition a ' ;  ' could stop the comment
syn region sicadString_ transparent start=+'+ end=+'+ oneline contained
" string
syn region sicadString start=+'+ end=+'+ oneline

" comments - nasty ones in sicad

" - ' *  blabla' or ' *  blabla;'
syn region sicadComment	start="^ *\*" skip='\\ *$' end=";"me=e-1 end="$" contains=sicadString_
" - ' .LABEL03 *  blabla' or ' .LABEL03 *  blabla;'
syn region sicadComment start="^ *\.[a-z][a-z0-9]\{0,7} *\*" skip='\\ *$' end=";"me=e-1 end="$" contains=sicadLabel1,sicadString_
" - '; * blabla' or '; * blabla;'
syn region sicadComment start="; *\*"ms=s+1 skip='\\ *$' end=";"me=e-1 end="$" contains=sicadString_
" - comments between docbeg and docend
syn region sicadComment	matchgroup=sicadStatement start="\<docbeg\>" end="\<docend\>"

" catch \ at the end of line
syn match sicadLineCont "\\ *$"

" parameters in dop block - for the time being it is not used
"syn match sicadParameter " [a-z][a-z0-9]*[=:]"me=e-1 contained
" dop block - for the time being it is not used
syn region sicadDopBlock transparent matchgroup=sicadStatement start='\<dop\>' skip='\\ *$' end=';'me=e-1 end='$' contains=ALL

" sql block - new highlighting mode is used (see syn include)
syn region sicadSqlBlock transparent matchgroup=sicadStatement start='\<sql\>' skip='\\ *$' end=';'me=e-1 end='$' contains=@SQL,sicadIdentifier,sicadLineCont

" synchronizing
syn sync clear  " clear sync used in sql.vim
syn sync match sicadSyncComment groupthere NONE "\<docend\>"
syn sync match sicadSyncComment grouphere sicadComment "\<docbeg\>"
" next line must be examined too
syn sync linecont "\\ *$"

" catch error caused by tabulator key
syn match sicadError "\t"
" catch errors caused by wrong parenthesis
"syn region sicadParen transparent start='(' end=')' contains=ALLBUT,sicadParenError
syn region sicadParen transparent start='(' skip='\\ *$' end=')' end='$' contains=ALLBUT,sicadParenError
syn match sicadParenError ')'
"syn region sicadApostrophe transparent start=+'+ end=+'+ contains=ALLBUT,sicadApostropheError
"syn match sicadApostropheError +'+
" not closed apostrophe
"syn region sicadError start=+'+ end=+$+ contains=ALLBUT,sicadApostropheError
"syn match sicadApostropheError +'[^']*$+me=s+1 contained

" SICAD keywords
syn keyword sicadStatement	abst add adrin aib aibzsn
syn keyword sicadStatement	aidump aifgeo aisbrk alknam alknr
syn keyword sicadStatement	alksav alksel alktrc alopen ansbo
syn keyword sicadStatement	aractiv ararea arareao arbuffer archeck
syn keyword sicadStatement	arcomv arcont arconv arcopy arcopyo
syn keyword sicadStatement	arcorr arcreate arerror areval arflfm
syn keyword sicadStatement	arflop arfrast argbkey argenf argraph
syn keyword sicadStatement	argrapho arinters arkompfl arlisly arnext
syn keyword sicadStatement	aroverl arovers arpars arrefp arselect
syn keyword sicadStatement	arset arstruct arunify arupdate arvector
syn keyword sicadStatement	arveinfl arvflfl arvoroni ausku basis
syn keyword sicadStatement	basisaus basisdar basisnr bebos befl
syn keyword sicadStatement	befla befli befls beo beorta
syn keyword sicadStatement	beortn bep bepan bepap bepola
syn keyword sicadStatement	bepoln bepsn bepsp ber berili
syn keyword sicadStatement	berk bewz bkl bli bma
syn keyword sicadStatement	bmakt bmakts bmbm bmerk bmerw
syn keyword sicadStatement	bmerws bminit bmk bmorth bmos
syn keyword sicadStatement	bmoss bmpar bmsl bmsum bmsums
syn keyword sicadStatement	bmver bmvero bmw bo bta
syn keyword sicadStatement	buffer bvl bw bza bzap
syn keyword sicadStatement	bzd bzgera bzorth cat catel
syn keyword sicadStatement	cdbdiff ce close comp conclose
syn keyword sicadStatement	coninfo conopen conread contour conwrite
syn keyword sicadStatement	cop copel cr cs cstat
syn keyword sicadStatement	cursor d da dal dasp
syn keyword sicadStatement	dasps dataout dcol dd defsr
syn keyword sicadStatement	del delel deskrdef df dfn
syn keyword sicadStatement	dfns dfpos dfr dgd dgm
syn keyword sicadStatement	dgp dgr dh diaus dir
syn keyword sicadStatement	disbsd dkl dktx dkur dlgfix
syn keyword sicadStatement	dlgfre dma dprio dr druse
syn keyword sicadStatement	dsel dskinfo dsr dv dve
syn keyword sicadStatement	eba ebd ebs edbsdbin edbssnin
syn keyword sicadStatement	edbsvtin edt egaus egdef egdefs
syn keyword sicadStatement	eglist egloe egloenp egloes egxx
syn keyword sicadStatement	eib ekur ekuradd elpos epg
syn keyword sicadStatement	esau esauadd esek eta etap
syn keyword sicadStatement	etav feparam ficonv filse fl
syn keyword sicadStatement	fli flin flini flinit flkor
syn keyword sicadStatement	fln flnli flop flout flowert
syn keyword sicadStatement	flparam flraster flsy flsyd flsym
syn keyword sicadStatement	flsyms flsymt fmtatt fmtdia fpg
syn keyword sicadStatement	gbadddb gbaim gbanrs gbatw gbau
syn keyword sicadStatement	gbaudit gbclosp gbcreem gbcreld gbcresdb
syn keyword sicadStatement	gbcretd gbde gbdeldb gbdelem gbdelld
syn keyword sicadStatement	gbdeltd gbdisdb gbdisem gbdisld gbdistd
syn keyword sicadStatement	gbebn gbemau gbepsv gbgetdet gbgetes
syn keyword sicadStatement	gbgetmas gbgqel gbgqelr gbgqsa gbgrant
syn keyword sicadStatement	gbler gblerb gblerf gbles gblocdic
syn keyword sicadStatement	gbmgmg gbmntdb gbmoddb gbnam gbneu
syn keyword sicadStatement	gbopenp gbpoly gbpos gbpruef gbps
syn keyword sicadStatement	gbqgel gbqgsa gbreldic gbresem gbrevoke
syn keyword sicadStatement	gbsav gbsbef gbsddk gbsicu gbsrt
syn keyword sicadStatement	gbss gbstat gbsysp gbszau gbubp
syn keyword sicadStatement	gbueb gbunmdb gbuseem gbw gbweg
syn keyword sicadStatement	gbwieh gbzt gelp gera hgw
syn keyword sicadStatement	hpg hr0 hra hrar inchk
syn keyword sicadStatement	inf infd inst kbml kbmls
syn keyword sicadStatement	kbmm kbmms kbmt kbmtdps kbmts
syn keyword sicadStatement	khboe khbol khdob khe khetap
syn keyword sicadStatement	khfrw khktk khlang khld khmfrp
syn keyword sicadStatement	khmks khpd khpfeil khpl khprofil
syn keyword sicadStatement	khrand khsa khsabs khsd khsdl
syn keyword sicadStatement	khse khskbz khsna khsnum khsob
syn keyword sicadStatement	khspos khtrn khver khzpe khzpl
syn keyword sicadStatement	kib kldat klleg klsch klsym
syn keyword sicadStatement	klvert kmpg kmtlage kmtp kmtps
syn keyword sicadStatement	kodef kodefp kok kokp kolae
syn keyword sicadStatement	kom kontly kopar koparp kopg
syn keyword sicadStatement	kosy kp kr krsek krtclose
syn keyword sicadStatement	krtopen ktk lad lae laesel
syn keyword sicadStatement	language lasso lbdes lcs ldesk
syn keyword sicadStatement	ldesks le leak leattdes leba
syn keyword sicadStatement	lebas lebaznp lebd lebm lebv
syn keyword sicadStatement	lebvaus lebvlist lede ledel ledepo
syn keyword sicadStatement	ledepol ledepos leder ledm lee
syn keyword sicadStatement	leeins lees lege lekr lekrend
syn keyword sicadStatement	lekwa lekwas lel lelh lell
syn keyword sicadStatement	lelp lem lena lend lenm
syn keyword sicadStatement	lep lepe lepee lepko lepl
syn keyword sicadStatement	lepmko lepmkop lepos leposm leqs
syn keyword sicadStatement	leqsl leqssp leqsv leqsvov les
syn keyword sicadStatement	lesch lesr less lestd let
syn keyword sicadStatement	letaum letl lev levtm levtp
syn keyword sicadStatement	levtr lew lewm lexx lfs
syn keyword sicadStatement	li lldes lmode loedk loepkt
syn keyword sicadStatement	lop lose lp lppg lppruef
syn keyword sicadStatement	lr ls lsop lsta lstat
syn keyword sicadStatement	ly lyaus lz lza lzae
syn keyword sicadStatement	lzbz lze lznr lzo lzpos
syn keyword sicadStatement	ma ma0 ma1 mad map
syn keyword sicadStatement	mapoly mcarp mccfr mccgr mcclr
syn keyword sicadStatement	mccrf mcdf mcdma mcdr mcdrp
syn keyword sicadStatement	mcdve mcebd mcgse mcinfo mcldrp
syn keyword sicadStatement	md me mefd mefds minmax
syn keyword sicadStatement	mipg ml mmdbf mmdellb mmdir
syn keyword sicadStatement	mmfsb mminfolb mmlapp mmlbf mmlistlb
syn keyword sicadStatement	mmmsg mmreadlb mmsetlb mnp mpo
syn keyword sicadStatement	mr mra ms msav msgout
syn keyword sicadStatement	msgsnd msp mspf mtd nasel
syn keyword sicadStatement	ncomp new nlist nlistlt nlistly
syn keyword sicadStatement	nlistnp nlistpo np npa npdes
syn keyword sicadStatement	npe npem npinfa npruef npsat
syn keyword sicadStatement	npss npssa ntz oa oan
syn keyword sicadStatement	odel odf odfx oj oja
syn keyword sicadStatement	ojaddsk ojaef ojaefs ojaen ojak
syn keyword sicadStatement	ojaks ojakt ojakz ojalm ojatkis
syn keyword sicadStatement	ojatt ojbsel ojckon ojde ojdtl
syn keyword sicadStatement	ojeb ojebd ojel ojesb ojesbd
syn keyword sicadStatement	ojex ojezge ojko ojlb ojloe
syn keyword sicadStatement	ojlsb ojmos ojnam ojpda ojpoly
syn keyword sicadStatement	ojprae ojs ojsak ojsort ojstrukt
syn keyword sicadStatement	ojsub ojtdef ojx old op
syn keyword sicadStatement	opa opa1 open opnbsd orth
syn keyword sicadStatement	osanz ot otp otrefp param
syn keyword sicadStatement	paranf pas passw pda pg
syn keyword sicadStatement	pg0 pgauf pgaufsel pgb pgko
syn keyword sicadStatement	pgm pgr pgvs pily pkpg
syn keyword sicadStatement	plot plotf plotfr pmap pmdata
syn keyword sicadStatement	pmdi pmdp pmeb pmep pminfo
syn keyword sicadStatement	pmlb pmli pmlp pmmod pnrver
syn keyword sicadStatement	poa pos posa posaus post
syn keyword sicadStatement	printfr protect prs prsym qualif
syn keyword sicadStatement	rahmen raster rasterd rbbackup rbchange
syn keyword sicadStatement	rbcmd rbcopy rbcut rbdbcl rbdbload
syn keyword sicadStatement	rbdbop rbdbwin rbdefs rbedit rbfdel
syn keyword sicadStatement	rbfill rbfload rbfnew rbfree rbg
syn keyword sicadStatement	rbinfo rbpaste rbrstore rbsnap rbsta
syn keyword sicadStatement	rbvtor rcol re reb refunc
syn keyword sicadStatement	ren renel rk rkpos rohr
syn keyword sicadStatement	rohrpos rpr rr rr0 rra
syn keyword sicadStatement	rrar rs samtosdb sav savx
syn keyword sicadStatement	scol scopy scopye sdbtosam sddk
syn keyword sicadStatement	sdwr se selaus selpos seman
syn keyword sicadStatement	semi sesch setscl sge sid
syn keyword sicadStatement	sie sig sigp skk skks
syn keyword sicadStatement	sn sn21 snpa snpar snparp
syn keyword sicadStatement	snpd snpi snpkor snpl snpm
syn keyword sicadStatement	sob sob0 sobloe sobs sof
syn keyword sicadStatement	sop split spr sqdadd sqdlad
syn keyword sicadStatement	sqdold sqdsav sr sres
syn keyword sicadStatement	srt sset stat stdtxt string
syn keyword sicadStatement	strukt strupru suinfl suinflk suinfls
syn keyword sicadStatement	supo supo1 sva svr sy
syn keyword sicadStatement	sya syly sysout syu syux
syn keyword sicadStatement	taa tabeg tabl tabm tam
syn keyword sicadStatement	tanr tapg tapos tarkd tas
syn keyword sicadStatement	tase tb tbadd tbd tbext
syn keyword sicadStatement	tbget tbint tbout tbput tbsat
syn keyword sicadStatement	tbsel tbstr tcaux tccable tcchkrep
syn keyword sicadStatement	tccond tcdbg tcinit tcmodel tcnwe
syn keyword sicadStatement	tcpairs tcpath tcscheme tcse tcselc
syn keyword sicadStatement	tcstar tcstrman tcsubnet tcsymbol tctable
syn keyword sicadStatement	tcthrcab tctrans tctst tdb tdbdel
syn keyword sicadStatement	tdbget tdblist tdbput tgmod titel
syn keyword sicadStatement	tmoff tmon tp tpa tps
syn keyword sicadStatement	tpta tra trans transkdo transopt
syn keyword sicadStatement	transpro trm trpg trrkd trs
syn keyword sicadStatement	ts tsa tx txa txchk
syn keyword sicadStatement	txcng txju txl txp txpv
syn keyword sicadStatement	txtcmp txv txz uiinfo uistatus
syn keyword sicadStatement	umdk umdk1 umdka umge umr
syn keyword sicadStatement	verbo verflli verif verly versinfo
syn keyword sicadStatement	vfg wabsym wzmerk zdrhf zdrhfn
syn keyword sicadStatement	zdrhfw zdrhfwn zefp zfl zflaus
syn keyword sicadStatement	zka zlel zlels zortf zortfn
syn keyword sicadStatement	zortfw zortfwn zortp zortpn zparb
syn keyword sicadStatement	zparbn zparf zparfn zparfw zparfwn
syn keyword sicadStatement	zparp zparpn zwinkp zwinkpn
" other commands excluded by ausku
syn keyword sicadStatement	oldd ps psw psopen pdadd
syn keyword sicadStatement      psclose psprw psparam psstat psres
syn keyword sicadStatement      savd

if !exists("did_sicad_syntax_inits")

  let did_sicad_syntax_inits = 1
  hi link sicadLabel		PreProc
  hi link sicadLabel1		sicadLabel
  hi link sicadLabel2		sicadLabel
  hi link sicadConditional	Conditional
  hi link sicadBoolean		Boolean
  hi link sicadNumber		Number
  hi link sicadFloat		Float
  hi link sicadOperator		Operator
  hi link sicadStatement	Statement
  hi link sicadParameter	sicadStatement
  hi link sicadGoto		sicadStatement
  hi link sicadLineCont		sicadStatement
  hi link sicadString		String
  hi link sicadComment		Comment
  hi link sicadSpecial		Special
  hi link sicadIdentifier	Type
"  hi link sicadIdentifier	Identifier
  hi link sicadError		Error
  hi link sicadParenError	sicadError
  hi link sicadApostropheError	sicadError
  hi link sicadStringError	sicadError
  hi link sicadCommentError	sicadError
  hi link sqlStatement	        Special  " modified highlight group in sql.vim

endif

let b:current_syntax = "sicad"

" vim: ts=8
