" Vim syntax file
" Language:	lite
" Maintainer:	Lutz Eymers <ixtab@polzin.com>
" URL:		http://www-public.rz.uni-duesseldorf.de/~eymers/vim/syntax
" Email:	Subject: send syntax_vim.tgz
" Last Change:	2000 Jan 05
"
" Options	lite_sql_query = 1 for SQL syntax highligthing inside strings
"		lite_minlines = x     to sync at least x lines backwards

" Remove any old syntax stuff hanging around
syn clear

if !exists("main_syntax")
  let main_syntax = 'lite'
endif

if main_syntax == 'lite'
  if exists("lite_sql_query")
    if lite_sql_query == 1
      syn include @liteSql <sfile>:p:h/sql.vim
    endif
  endif
endif

if (main_syntax == 'msql')
  if exists("msql_sql_query")
    if msql_sql_query == 1
      syn include @liteSql <sfile>:p:h/sql.vim
    endif
  endif
endif

syn cluster liteSql remove=sqlString,sqlComment

syn case match

" Internal Variables
syn keyword liteIntVar ERRMSG contained

" Comment
syn region liteComment		start="/\*" end="\*/" contains=liteTodo

" Function names
syn keyword liteFunctions  echo printf fprintf open close read
syn keyword liteFunctions  readln readtok
syn keyword liteFunctions  split strseg chop tr sub substr
syn keyword liteFunctions  test unlink umask chmod mkdir chdir rmdir
syn keyword liteFunctions  rename truncate link symlink stat
syn keyword liteFunctions  sleep system getpid getppid kill
syn keyword liteFunctions  time ctime time2unixtime unixtime2year
syn keyword liteFunctions  unixtime2year unixtime2month unixtime2day
syn keyword liteFunctions  unixtime2hour unixtime2min unixtime2sec
syn keyword liteFunctions  strftime
syn keyword liteFunctions  getpwnam getpwuid
syn keyword liteFunctions  gethostbyname gethostbyaddress
syn keyword liteFunctions  urlEncode setContentType includeFile
syn keyword liteFunctions  msqlConnect msqlClose msqlSelectDB
syn keyword liteFunctions  msqlQuery msqlStoreResult msqlFreeResult
syn keyword liteFunctions  msqlFetchRow msqlDataSeek msqlListDBs
syn keyword liteFunctions  msqlListTables msqlInitFieldList msqlListField
syn keyword liteFunctions  msqlFieldSeek msqlNumRows msqlEncode
syn keyword liteFunctions  exit fatal typeof
syn keyword liteFunctions  crypt addHttpHeader

" Conditional
syn keyword liteConditional  if else

" Repeat
syn keyword liteRepeat  while

" Operator
syn keyword liteStatement  break return continue

" Operator
syn match liteOperator  "[-+=#*]"
syn match liteOperator  "/[^*]"me=e-1
syn match liteOperator  "\$"
syn match liteRelation  "&&"
syn match liteRelation  "||"
syn match liteRelation  "[!=<>]="
syn match liteRelation  "[<>]"

" Identifier
syn match  liteIdentifier "$\h\w*" contains=liteIntVar,liteOperator
syn match  liteGlobalIdentifier "@\h\w*" contains=liteIntVar

" Include
syn keyword liteInclude  load

" Define
syn keyword liteDefine  funct

" Type
syn keyword liteType  int uint char real

" String
syn region liteString  keepend matchgroup=None start=+"+  skip=+\\\\\|\\"+  end=+"+ contains=liteIdentifier,liteSpecialChar,@liteSql

" Number
syn match liteNumber  "-\=\<\d\+\>"

" Float
syn match liteFloat  "\(-\=\<\d+\|-\=\)\.\d\+\>"

" SpecialChar
syn match liteSpecialChar "\\[abcfnrtv\\]" contained

syn match liteParentError "[)}\]]"

" Todo
syn keyword liteTodo TODO Todo todo contained

" dont syn #!...
syn match liteExec "^#!.*$"

" Parents
syn cluster liteInside contains=liteComment,liteFunctions,liteIdentifier,liteGlobalIdentifier,liteConditional,liteRepeat,liteStatement,liteOperator,liteRelation,liteType,liteString,liteNumber,liteFloat,liteParent

syn region liteParent matchgroup=Delimiter start="(" end=")" contains=@liteInside
syn region liteParent matchgroup=Delimiter start="{" end="}" contains=@liteInside
syn region liteParent matchgroup=Delimiter start="\[" end="\]" contains=@liteInside

" sync
if main_syntax == 'lite'
  if exists("lite_minlines")
    exec "syn sync minlines=" . lite_minlines
  else
    syn sync minlines=100
  endif
endif

if !exists("did_lite_syntax_inits")
  let did_lite_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link liteComment		Comment
  hi link liteString		String
  hi link liteNumber		Number
  hi link liteFloat		Float
  hi link liteIdentifier	Identifier
  hi link liteGlobalIdentifier	Identifier
  hi link liteIntVar		Identifier
  hi link liteFunctions		Function
  hi link liteRepeat		Repeat
  hi link liteConditional	Conditional
  hi link liteStatement		Statement
  hi link liteType		Type
  hi link liteInclude		Include
  hi link liteDefine		Define
  hi link liteSpecialChar	SpecialChar
  hi link liteParentError	liteError
  hi link liteError		Error
  hi link liteTodo		Todo
  hi link liteOperator		Operator
  hi link liteRelation		Operator
endif

let b:current_syntax = "lite"

if main_syntax == 'lite'
  unlet main_syntax
endif

" vim: ts=8
