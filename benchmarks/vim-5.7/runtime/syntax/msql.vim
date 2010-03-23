" Vim syntax file
" Language:	msql
" Maintainer:	Lutz Eymers <ixtab@polzin.com>
" URL:		http://www-public.rz.uni-duesseldorf.de/~eymers/vim/syntax
" Email:	Subject: send syntax_vim.tgz
" Last Change:	2000 Jan 05
"
" Options	msql_sql_query = 1 for SQL syntax highligthing inside strings
"		msql_minlines = x     to sync at least x lines backwards

" Remove any old syntax stuff hanging around
syn clear

if !exists("main_syntax")
  let main_syntax = 'msql'
endif

so <sfile>:p:h/html.vim
syn cluster htmlPreproc add=msqlRegion

syn case match

" Internal Variables
syn keyword msqlIntVar ERRMSG contained

" Env Variables
syn keyword msqlEnvVar SERVER_SOFTWARE SERVER_NAME SERVER_URL GATEWAY_INTERFACE contained
syn keyword msqlEnvVar SERVER_PROTOCOL SERVER_PORT REQUEST_METHOD PATH_INFO  contained
syn keyword msqlEnvVar PATH_TRANSLATED SCRIPT_NAME QUERY_STRING REMOTE_HOST contained
syn keyword msqlEnvVar REMOTE_ADDR AUTH_TYPE REMOTE_USER CONTEN_TYPE  contained
syn keyword msqlEnvVar CONTENT_LENGTH HTTPS HTTPS_KEYSIZE HTTPS_SECRETKEYSIZE  contained
syn keyword msqlEnvVar HTTP_ACCECT HTTP_USER_AGENT HTTP_IF_MODIFIED_SINCE  contained
syn keyword msqlEnvVar HTTP_FROM HTTP_REFERER contained

" Inlclude lLite
syn include @msqlLite <sfile>:p:h/lite.vim

" Msql Region
syn region msqlRegion matchgroup=Delimiter start="<!$" start="<![^!->D]" end=">" contains=@msqlLite,msql.*

" sync
if exists("msql_minlines")
  exec "syn sync minlines=" . msql_minlines
else
  syn sync minlines=100
endif

if !exists("did_msql_syntax_inits")
  let did_msql_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link msqlComment		Comment
  hi link msqlString		String
  hi link msqlNumber		Number
  hi link msqlFloat		Float
  hi link msqlIdentifier	Identifier
  hi link msqlGlobalIdentifier	Identifier
  hi link msqlIntVar		Identifier
  hi link msqlEnvVar		Identifier
  hi link msqlFunctions		Function
  hi link msqlRepeat		Repeat
  hi link msqlConditional	Conditional
  hi link msqlStatement		Statement
  hi link msqlType		Type
  hi link msqlInclude		Include
  hi link msqlDefine		Define
  hi link msqlSpecialChar	SpecialChar
  hi link msqlParentError	Error
  hi link msqlTodo		Todo
  hi link msqlOperator		Operator
  hi link msqlRelation		Operator
endif

let b:current_syntax = "msql"

if main_syntax == 'msql'
  unlet main_syntax
endif

" vim: ts=8
