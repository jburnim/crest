" Vim syntax file
"    Language:	PL/SQL - Oracle Procedural SQL
"  Maintainer:	Jeff Lanzarotta <roc_head@yahoo.com>
" 					Originally by C. Laurence Gonsalves <clgonsal@kami.com>
"         URL:	http://web.qx.net/lanzarotta/vim/syntax/plsql.vim
" Last Change:	2000 Feb 24

" Remove any old syntax stuff hanging around.
syn clear
syn case ignore

" Todo.
syn keyword	plsqlTodo		TODO FIXME XXX DEBUG NOTE

syn match   plsqlGarbage	"[^ \t()]"
syn match   plsqlIdentifier	"[a-z][a-z0-9$_#]*"
syn match   plsqlHostIdentifier	":[a-z][a-z0-9$_#]*"

" Symbols.
syn match   plsqlSymbol		"\(;\|,\|\.\)"

" Operators.
syn match   plsqlOperator	"\(+\|-\|\*\|/\|=\|<\|>\|@\|\*\*\|!=\|\~=\)"
syn match   plsqlOperator	"\(^=\|<=\|>=\|:=\|=>\|\.\.\|||\|<<\|>>\|\"\)"

" SQL keywords.
syn keyword plsqlSQLKeyword	ACCESS ADD ALL ALTER AND ANY AS ASC AUDIT
syn keyword plsqlSQLKeyword	BETWEEN BY CHECK CLUSTER COLUMN COMMENT
syn keyword plsqlSQLKeyword	COMPRESS CONNECT CREATE CURRENT
syn keyword plsqlSQLKeyword	DEFAULT DELETE DESC DISTINCT DROP ELSE
syn keyword plsqlSQLKeyword	EXCLUSIVE EXISTS FILE FROM GRANT
syn keyword plsqlSQLKeyword	GROUP HAVING IDENTIFIED IMMEDIATE IN INCREMENT
syn keyword plsqlSQLKeyword	INDEX INITIAL INSERT INTERSECT INTO IS
syn keyword plsqlSQLKeyword	LEVEL LIKE LOCK MAXEXTENTS MODE NOAUDIT
syn keyword plsqlSQLKeyword	NOCOMPRESS NOT NOWAIT OF OFFLINE
syn keyword plsqlSQLKeyword	ON ONLINE OPTION OR ORDER PCTFREE PRIOR
syn keyword plsqlSQLKeyword	PRIVILEGES PUBLIC RENAME RESOURCE REVOKE
syn keyword plsqlSQLKeyword	ROW ROWLABEL ROWS SELECT SESSION SET
syn keyword plsqlSQLKeyword	SHARE START SUCCESSFUL SYNONYM SYSDATE
syn keyword plsqlSQLKeyword	THEN TO TRIGGER UID UNION UNIQUE UPDATE
syn keyword plsqlSQLKeyword	USER VALIDATE VALUES VIEW
syn keyword plsqlSQLKeyword	WHENEVER WHERE WITH
syn keyword plsqlSQLKeyword	REPLACE

" PL/SQL's own keywords.
syn keyword plsqlKeyword	ABORT ACCEPT ARRAY ARRAYLEN ASSERT ASSIGN AT
syn keyword plsqlKeyword	AUTHORIZATION AVG BASE_TABLE BEGIN BODY CASE
syn keyword plsqlKeyword	CHAR_BASE CLOSE CLUSTERS COLAUTH COMMIT
syn keyword plsqlKeyword	CONSTANT CRASH CURRVAL DATABASE DATA_BASE DBA
syn keyword plsqlKeyword	DEBUGOFF DEBUGON DECLARE DEFINTION DELAY
syn keyword plsqlKeyword	DIGITS DISPOSE DO ENTRY EXCEPTION
syn keyword plsqlKeyword	EXCEPTION_INIT EXIT FETCH FORM FUNCTION
syn keyword plsqlKeyword	GENERIC GOTO INDEXES INDICATOR INTERFACE
syn keyword plsqlKeyword	LIMITED MAX MIN MINUS MISLABEL MOD
syn keyword plsqlKeyword	NATURALN NEW NEXTVAL NUMBER_BASE OPEN OTHERS
syn keyword plsqlKeyword	OUT PACKAGE PARTITION PLS_INTEGER POSITIVEN
syn keyword plsqlKeyword	PRAGMA PRIVATE PROCEDURE RAISE RANGE REF
syn keyword plsqlKeyword	RELEASE REMR RETURN REVERSE ROLLBACK ROWNUM
syn keyword plsqlKeyword	ROWTYPE RUN SAVEPOINT SCHEMA SEPERATE SPACE
syn keyword plsqlKeyword	SQL SQLCODE SQLERRM STATEMENT STDDEV SUBTYPE
syn keyword plsqlKeyword	SUM TABAUTH TABLES TASK TERMINATE TYPE USE
syn keyword plsqlKeyword	VARIANCE VIEWS WHEN WORK WRITE XOR
syn match   plsqlKeyword	"\<END\>"

if exists("plsql_highlight_triggers")
    syn keyword plsqlTrigger	INSERTING UPDATING DELETING
endif

" Conditionals.
syn keyword plsqlConditional	ELSIF ELSE IF
syn match   plsqlConditional	"\<END\s\+IF\>"

" Loops.
syn keyword plsqlRepeat		FOR LOOP WHILE
syn match   plsqlRepeat		"\<END\s\+LOOP\>"

" Various types of comments.
syn match   plsqlComment	"--.*$" contains=plsqlTodo
syn region  plsqlComment	start="/\*" end="\*/" contains=plsqlTodo
syn sync ccomment plsqlComment

" To catch unterminated string literals.
syn match   plsqlStringError	"'.*$"

" Various types of literals.
syn match   plsqlIntLiteral	"[+-]\=[0-9]\+"
syn match   plsqlFloatLiteral	"[+-]\=\([0-9]*\.[0-9]\+\|[0-9]\+\.[0-9]\+\)\(e[+-]\=[0-9]\+\)\="
syn match   plsqlCharLiteral	"'[^']'"
syn match   plsqlStringLiteral	"'\([^']\|''\)*'"
syn keyword plsqlBooleanLiteral	TRUE FALSE NULL

" The built-in types.
syn keyword plsqlStorage	BINARY_INTEGER BOOLEAN CHAR CURSOR DATE DECIMAL
syn keyword plsqlStorage	FLOAT INTEGER LONG MLSLABEL NATURAL NUMBER
syn keyword plsqlStorage	POSITIVE RAW REAL RECORD ROWID SMALLINT TABLE
syn keyword plsqlStorage	VARCHAR VARCHAR2

" A type-attribute is really a type.
syn match   plsqlTypeAttribute	":\=[a-z][a-z0-9$_#]*%\(TYPE\|ROWTYPE\)\>"

" All other attributes.
syn match   plsqlAttribute	"%\(NOTFOUND\|ROWCOUNT\|FOUND\|ISOPEN\)\>"

" This'll catch mis-matched close-parens.
syn region plsqlParen		transparent start='(' end=')' contains=ALLBUT,plsqlParenError
syn match plsqlParenError	")"

if !exists("did_plsql_syntax_inits")
	let did_plsql_syntax_inits = 1

	" These are general categories that should maybe become standard.
	hi link Attribute		Macro
	hi link Query		Function
	hi link Event		Function

	hi link plsqlAttribute	Attribute
	hi link plsqlBooleanLiteral	Boolean
	hi link plsqlCharLiteral	Character
	hi link plsqlComment	Comment
	hi link plsqlConditional	Conditional
	hi link plsqlFloatLiteral	Float
	hi link plsqlGarbage	Error
	hi link plsqlHostIdentifier	Label
	hi link plsqlIdentifier	Normal
	hi link plsqlIntLiteral	Number
	hi link plsqlOperator	Operator
	hi link plsqlParen		Normal
	hi link plsqlParenError	Error
	hi link plsqlKeyword	Keyword
	hi link plsqlRepeat		Repeat
	hi link plsqlStorage	StorageClass
	hi link plsqlSQLKeyword	Query
	hi link plsqlStringError	Error
	hi link plsqlStringLiteral	String
	hi link plsqlSymbol		Normal
	hi link plsqlTrigger	Event
	hi link plsqlTypeAttribute	StorageClass
	hi link plsqlTodo	Todo
endif

let b:current_syntax = "plsql"

" vim: ts=3
