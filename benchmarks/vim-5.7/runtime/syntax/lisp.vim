" Vim syntax file
" Language:	Lisp
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	March 21, 2000
" Version:	1.04
"  1.04: Mar 21 2000 : added Todo

" remove any old syntax stuff hanging around
syn clear
set iskeyword=42,43,45,47-58,60-62,64-90,97-122,_

" Clusters
syn cluster	lispAtomCluster	contains=lispAtomBarSymbol,lispAtomList,lispAtomNmbr0,lispComment,lispString
syn cluster	lispListCluster	contains=lispAtom,lispAtomBarSymbol,lispAtomMark,lispBQList,lispBarSymbol,lispComment,lispConcat,lispDecl,lispFunc,lispKey,lispList,lispNumber,lispSpecial,lispString,lispSymbol,lispVar

" Lists
syn match	lispSymbol	contained	![^()'`,"; \t]\+!
syn match	lispBarSymbol	contained	!|..\{-}|!
syn region	lispList	matchgroup=Delimiter start="(" skip="|.\{-}|"	matchgroup=Delimiter end=")" contains=@lispListCluster
syn region	lispBQList	matchgroup=PreProc   start="`("	skip="|.\{-}|"	matchgroup=PreProc   end=")" contains=@lispListCluster

" Atoms
syn match	lispAtomMark	"'"
syn match	lispAtom	"'("me=e-1	contains=lispAtomMark	nextgroup=lispAtomList
syn match	lispAtom	"'[^ \t()]\+"	contains=lispAtomMark
syn match	lispAtomBarSymbol	!'|..\{-}|!	contains=lispAtomMark
syn region	lispAtom	start=+'"+	skip=+\\"+ end=+"+
syn region	lispAtomList	contained	matchgroup=Special start="("	skip="|.\{-}|" matchgroup=Special end=")"	contains=@lispAtomCluster
syn match	lispAtomNmbr	contained	"\<\d\+"

" Standard Lisp Functions and Macros
syn keyword lispFunc	*	concatenate	get-properties	nreverse	setq
syn keyword lispFunc	+	cond	get-setf-method	nset-difference	seventh
syn keyword lispFunc	-	conjugate	getf	nstring-upcase	shadow
syn keyword lispFunc	/	cons	gethash	nsublis	shiftf
syn keyword lispFunc	<	consp	go	nsubst	short-site-name
syn keyword lispFunc	=	constantp	graphic-char-p	nsubst-if	signed-byte
syn keyword lispFunc	>	copy-alist	hash-table-p	nsubst-if-not	signum
syn keyword lispFunc	abs	copy-list	host-namestring	nsubstitute	simple-string-p
syn keyword lispFunc	access	copy-readtable	identity	nsubstitute-if	simple-vector-p
syn keyword lispFunc	acons	copy-seq	if	nth	sin
syn keyword lispFunc	acos	copy-symbol	if-exists	nthcdr	sinh
syn keyword lispFunc	acosh	copy-tree	ignore	null	sixth
syn keyword lispFunc	adjoin	cos	imagpart	numberp	sleep
syn keyword lispFunc	adjust-array	cosh	import	numerator	software-type
syn keyword lispFunc	alpha-char-p	count	in-package	nunion	some
syn keyword lispFunc	alphanumericp	count-if	in-package	oddp	sort
syn keyword lispFunc	and	count-if-not	incf	open	special
syn keyword lispFunc	append	ctypecase	inline	optimize	special-form-p
syn keyword lispFunc	apply	decf	input-stream-p	or	sqrt
syn keyword lispFunc	applyhook	declaration	inspect	output-stream-p	stable-sort
syn keyword lispFunc	apropos	declare	int-char	package-name	standard-char-p
syn keyword lispFunc	apropos-list	decode-float	integer-length	packagep	step
syn keyword lispFunc	aref	defconstant	integerp	pairlis	streamup
syn keyword lispFunc	array-dimension	defparameter	intern	parse-integer	string
syn keyword lispFunc	array-rank	defstruct	intersection	pathname	string-char
syn keyword lispFunc	array-rank-limit	defvar	isqrt	pathname-device	string-char-p
syn keyword lispFunc	arrayp	delete	keyword	pathname-host	string-downcase
syn keyword lispFunc	ash	delete-file	last	pathname-name	string-equal
syn keyword lispFunc	asin	delete-if	lcm	pathname-type	string-greaterp
syn keyword lispFunc	asinh	delete-if-not	ldb	pathnamep	string-lessp
syn keyword lispFunc	assert	denominator	ldb-test	peek-char	string-trim
syn keyword lispFunc	assoc	deposit-field	ldiff	phase	string-upcase
syn keyword lispFunc	assoc-if	describe	length	pi	string/=
syn keyword lispFunc	assoc-if-not	digit-char	let*	plusp	string<
syn keyword lispFunc	atan	digit-char-p	lisp	pop	string<=
syn keyword lispFunc	atanh	directory	list	position	string=
syn keyword lispFunc	bit	disassemble	list*	position-if	string>
syn keyword lispFunc	bit-and	do	list-length	position-if-not	string>=
syn keyword lispFunc	bit-andc1	do*	listen	pprint	stringp
syn keyword lispFunc	bit-andc2	do-all-symbols	listp	prin1	sublim
syn keyword lispFunc	bit-eqv	do-symbols	load	prin1-to-string	subseq
syn keyword lispFunc	bit-ior	documentation	log	princ	subsetp
syn keyword lispFunc	bit-nand	dolist	logand	princ-to-string	subst
syn keyword lispFunc	bit-nor	dotimes	logandc1	print	subst-if
syn keyword lispFunc	bit-not	dpb	logandc2	probe-file	subst-if-not
syn keyword lispFunc	bit-orc1	dribble	logcount	proclaim	subtypep
syn keyword lispFunc	bit-orc2	ecase	logeqv	prog	svref
syn keyword lispFunc	bit-vector-p	ed	logior	prog*	sxhash
syn keyword lispFunc	bit-xor	eighth	lognand	prog1	symbol-function
syn keyword lispFunc	block	elt	lognor	prog2	symbol-name
syn keyword lispFunc	boole	endp	lognot	progn	symbol-package
syn keyword lispFunc	both-case-p	eq	logorc1	progv	symbol-plist
syn keyword lispFunc	boundp	eql	logorc2	provide	symbol-value
syn keyword lispFunc	break	equal	logtest	psetf	symbolp
syn keyword lispFunc	butlast	equalp	logxor	psetq	sys
syn keyword lispFunc	byte	error	long-site-name	push	system
syn keyword lispFunc	byte-position	etypecase	loop	pushnew	t
syn keyword lispFunc	byte-size	eval	lower-case-p	putprop	tagbody
syn keyword lispFunc	car	eval-when	machine-type	quote	tailp
syn keyword lispFunc	catch	evalhook	machine-version	random	tan
syn keyword lispFunc	ccase	evenp	macro-function	random-state-p	tanh
syn keyword lispFunc	cdr	every	macroexpand	rassoc	tenth
syn keyword lispFunc	ceiling	exp	macroexpand-l	rassoc-if	terpri
syn keyword lispFunc	cerror	export	make-array	rassoc-if-not	the
syn keyword lispFunc	char	expt	make-array	rational	third
syn keyword lispFunc	char-bit	fboundp	make-char	rationalize	throw
syn keyword lispFunc	char-bits	fceiling	make-hash-table	rationalp	time
syn keyword lispFunc	char-bits-limit	ffloor	make-list	read	trace
syn keyword lispFunc	char-code	fifth	make-package	read-byte	tree-equal
syn keyword lispFunc	char-code-limit	file-author	make-pathname	read-char	truename
syn keyword lispFunc	char-downcase	file-length	make-sequence	read-eval-print	truncase
syn keyword lispFunc	char-equal	file-namestring	make-string	read-line	type
syn keyword lispFunc	char-font	file-position	make-symbol	readtablep	type-of
syn keyword lispFunc	char-font-limit	file-write-date	makunbound	realpart	typecase
syn keyword lispFunc	char-greaterp	fill	map	reduce	typep
syn keyword lispFunc	char-hyper-bit	fill-pointer	mapc	rem	unexport
syn keyword lispFunc	char-int	find	mapcan	remf	unintern
syn keyword lispFunc	char-lessp	find-if	mapcar	remhash	union
syn keyword lispFunc	char-meta-bit	find-if-not	mapcon	remove	unless
syn keyword lispFunc	char-name	find-package	maphash	remove-if	unread
syn keyword lispFunc	char-not-equal	find-symbol	mapl	remove-if-not	unsigned-byte
syn keyword lispFunc	char-not-lessp	finish-output	maplist	remprop	untrace
syn keyword lispFunc	char-super-bit	first	mask-field	rename-file	unuse-package
syn keyword lispFunc	char-upcase	float	max	rename-package	unwind-protect
syn keyword lispFunc	char/=	float-digits	member	replace	upper-case-p
syn keyword lispFunc	char<	float-precision	member-if	require	use-package
syn keyword lispFunc	char<=	float-radix	member-if-not	rest	user
syn keyword lispFunc	char=	float-sign	merge	return	values
syn keyword lispFunc	char>	floatp	merge-pathname	return-from	values-list
syn keyword lispFunc	char>=	floor	min	revappend	vector
syn keyword lispFunc	character	fmakunbound	minusp	reverse	vector-pop
syn keyword lispFunc	characterp	force-output	mismatch	room	vector-push
syn keyword lispFunc	check-type	fourth	mod	rotatef	vectorp
syn keyword lispFunc	cis	fresh-line	name-char	round	warn
syn keyword lispFunc	clear-input	fround	namestring	rplaca	when
syn keyword lispFunc	clear-output	ftruncate	nbutlast	rplacd	with-open-file
syn keyword lispFunc	close	ftype	nconc	sbit	write
syn keyword lispFunc	clrhash	funcall	nil	scale-float	write-byte
syn keyword lispFunc	code-char	function	nintersection	schar	write-char
syn keyword lispFunc	coerce	functionp	ninth	search	write-line
syn keyword lispFunc	commonp	gbitp	not	second	write-string
syn keyword lispFunc	compile	gcd	notany	set	write-to-string
syn keyword lispFunc	compile-file	gensym	notevery	set-char-bit	y-or-n-p
syn keyword lispFunc	compiler-let	gentemp	notinline	set-difference	yes-or-no-p
syn keyword lispFunc	complex	get	nreconc	setf	zerop
syn keyword lispFunc	complexp
syn match   lispFunc	"\<c[ad]\+r\>"

syn keyword lispFunc	adjustable-array-p		least-negative-short-float	nstring-capitalize
syn keyword lispFunc	array-dimension-limit	least-negative-single-float	nstring-downcase
syn keyword lispFunc	array-dimensions		least-positive-double-float	nsubstitute-if-not
syn keyword lispFunc	array-element-type		least-positive-long-float	package-nicknames
syn keyword lispFunc	array-has-fill-pointer-p	least-positive-short-float	package-shadowing-symbols
syn keyword lispFunc	array-in-bounds-p		least-positive-single-float	package-use-list
syn keyword lispFunc	array-row-major-index	lisp-implementation-type	package-used-by-list
syn keyword lispFunc	array-total-size		lisp-implementation-version	parse-namestring
syn keyword lispFunc	array-total-size-limit	list-all-packages		pathname-directory
syn keyword lispFunc	call-arguments-limit		long-float-epsilon		pathname-version
syn keyword lispFunc	char-control-bit		long-float-negative-epsilon	read-char-no-hang
syn keyword lispFunc	char-not-greaterp		machine-instance		read-delimited-list
syn keyword lispFunc	compiled-function-p		make-broadcast-stream	read-from-string
syn keyword lispFunc	decode-universal-time	make-concatenated-stream	read-preserving-whitespace
syn keyword lispFunc	define-modify-macro		make-dispatch-macro-character	remove-duplicates
syn keyword lispFunc	define-setf-method		make-echo-stream		set-dispatch-macro-character
syn keyword lispFunc	delete-duplicates		make-random-state		set-exclusive-or
syn keyword lispFunc	directory-namestring		make-string-input-stream	set-macro-character
syn keyword lispFunc	do-exeternal-symbols		make-string-output-stream	set-syntax-from-char
syn keyword lispFunc	double-float-epsilon		make-synonym-stream		shadowing-import
syn keyword lispFunc	double-float-negative-epsilon	make-two-way-stream		short-float-epsilon
syn keyword lispFunc	encode-universal-time	most-negative-double-float	simple-bit-vector-p
syn keyword lispFunc	enough-namestring		most-negative-fixnum		single-flaot-epsilon
syn keyword lispFunc	find-all-symbols		most-negative-long-float	single-float-negative-epsilon
syn keyword lispFunc	get-decoded-time		most-negative-short-float	software-version
syn keyword lispFunc	get-dispatch-macro-character	most-negative-single-float	stream-element-type
syn keyword lispFunc	get-internal-real-time	most-positive-double-float	string-capitalize
syn keyword lispFunc	get-internal-run-time	most-positive-fixnum		string-left-trim
syn keyword lispFunc	get-macro-character		most-positive-long-float	string-not-equal
syn keyword lispFunc	get-output-stream-string	most-positive-short-float	string-not-greaterp
syn keyword lispFunc	get-universal-time		most-positive-single-float	string-not-lessp
syn keyword lispFunc	hash-table-count		multiple-value-bind		string-right-strim
syn keyword lispFunc	integer-decode-float		multiple-value-call		user-homedir-pathname
syn keyword lispFunc	internal-time-units-per-second	multiple-value-list		vector-push-extend
syn keyword lispFunc	lambda-list-keywords		multiple-value-prog1		with-input-from-string
syn keyword lispFunc	lambda-parameters-limit	multiple-value-seteq		with-open-stream
syn keyword lispFunc	least-negative-double-float	multiple-values-limit	with-output-to-string
syn keyword lispFunc	least-negative-long-float	nset-exclusive-or

" Lisp Keywords (modifiers)
syn keyword lispKey	:abort	:device	:if-exists	:name	:rehash-size
syn keyword lispKey	:adjustable	:direction	:include	:named	:rename
syn keyword lispKey	:append	:directory	:index	:new-version	:size
syn keyword lispKey	:array	:displaced-to	:inherited	:nicknames	:start
syn keyword lispKey	:base	:element-type	:initial-element	:output	:start1
syn keyword lispKey	:case	:end	:initial-offset	:output-file	:start2
syn keyword lispKey	:circle	:end1	:initial-value	:overwrite	:stream
syn keyword lispKey	:conc-name	:end2	:input	:predicate	:supersede
syn keyword lispKey	:constructor	:error	:internal	:pretty	:test
syn keyword lispKey	:copier	:escape	:io	:print	:test-not
syn keyword lispKey	:count	:external	:junk-allowed	:print-function	:type
syn keyword lispKey	:create	:from-end	:key	:probe	:use
syn keyword lispKey	:default	:gensym	:length	:radix	:verbose
syn keyword lispKey	:defaults	:host	:level	:read-only	:version

syn keyword lispKey	:displaced-index-offset	:initial-contents		:rehash-threshold
syn keyword lispKey	:if-does-not-exist	:preserve-whitespace	:rename-and-delete

" Standard Lisp Variables
syn keyword lispVar	*applyhook*	*modules*	*print-circle*	*print-pretty*	*read-suppress*
syn keyword lispVar	*debug-io*	*package*	*print-escape*	*print-radix*	*readtable*
syn keyword lispVar	*error-output*	*print-array*	*print-gensym*	*query-io*	*standard-input*
syn keyword lispVar	*evalhook*	*print-base*	*print-length*	*random-state*	*terminal-io*
syn keyword lispVar	*features*	*print-case*	*print-level*	*read-base*	*trace-output*
syn keyword lispVar	*load-verbose*

syn keyword lispVar	*break-on-warnings*	*macroexpand-hook*	*standard-output*
syn keyword lispVar	*default-pathname-defaults*	*read-default-float-format*

" Strings
syn region	lispString	start=+"+	skip=+\\\\\|\\"+ end=+"+

" Shared with Xlisp, Declarations, Macros, Functions
syn keyword lispDecl	defmacro	defun	do-external-symbols flet	locally
syn keyword lispDecl	defsetf	do*	do-symbols	labels	macrolet
syn keyword lispDecl	deftype	do-all-symbols	dotimes	let	multiple-value-bind

syn match lispNumber	"\d\+"

syn match lispSpecial	"\*[a-zA-Z_][a-zA-Z_0-9-]*\*"
syn match lispSpecial	!#|[^()'`,"; \t]\+|#!
syn match lispSpecial	!#x[0-9a-fA-F]\+!
syn match lispSpecial	!#o[0-7]\+!
syn match lispSpecial	!#b[01]\+!
syn match lispSpecial	!#\\[ -\~]!
syn match lispSpecial	!#[':][^()'`,"; \t]\+!
syn match lispSpecial	!#([^()'`,"; \t]\+)!

syn match lispConcat	"\s\.\s"
syn match lispParenError	")"

" Comments
syn match lispComment	";.*$"	contains=lispTodo
syn keyword lispTodo contained	COMBAK	TODO	Todo
syn keyword lispTodo contained	COMBAK:	TODO:	Todo:

" synchronization
syn sync lines=100

if !exists("did_lisp_syntax_inits")
  let did_lisp_syntax_inits= 1
  hi link lispAtomNmbr	lispNumber
  hi link lispAtomMark	lispMark

  hi link lispAtom		Identifier
  hi link lispAtomBarSymbol	Special
  hi link lispBarSymbol	Special
  hi link lispComment	Comment
  hi link lispConcat		Statement
  hi link lispDecl		Statement
  hi link lispFunc		Statement
  hi link lispKey		Type
  hi link lispMark		Delimiter
  hi link lispNumber		Number
  hi link lispParenError	Error
  hi link lispSpecial	Type
  hi link lispString		String
  hi link lispTodo		Todo
  hi link lispVar		Statement
  endif

let b:current_syntax = "lisp"

" vim: ts=21 nowrap
