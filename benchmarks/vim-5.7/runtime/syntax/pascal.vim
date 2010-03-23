" Vim syntax file
" Language:	Pascal
" Previous Maintainer:	Mario Eusebio <bio@dq.fct.unl.pt>
" Current  Maintainer:  Xavier Crégut <xavier.cregut@enseeiht.fr>
" Last Change:	2000 Mar 11

" Remove any old syntax stuff hanging around
syn clear

syn case ignore

syn keyword pascalStatement		program begin end
syn keyword pascalLabel			case goto
syn keyword pascalConditional		if else then
syn keyword pascalRepeat		do for while to until

syn keyword pascalTodo contained	TODO

" String
syn region  pascalString	start=+'+  end=+'+

"syn match  pascalIdentifier		"\<[a-zA-Z_][a-zA-Z0-9_]*\>"

"syn match  pascalDelimiter		"[()]"

syn match  pascalMatrixDelimiter	"[][]"

"if you prefer you can highlight the range
"syn match  pascalMatrixDelimiter	"[\d\+\.\.\d\+]"

"syn match  pascalNumber		"-\=\<\d\+\.\d\+[dD]-\=\d\+\>"
"syn match  pascalNumber		"-\=\<\d\+\.\d\+[eE]-\=\d\+\>"
"syn match  pascalNumber		"-\=\<\d\+\.\d\+\>"
syn match  pascalNumber		"-\=\<\d\+\>"
syn match  pascalByte		"\$[0-9a-fA-F]\+\>"

" If you don't like tabs
"syn match pascalShowTab "\t"
"syn match pascalShowTabc "\t"

syn region pascalComment	start="(\*"  end="\*)" contains=pascalTodo
syn region pascalComment	start="{"  end="}" contains=pascalTodo

syn keyword pascalOperator	and array boolean delete dispose div downto
syn keyword pascalOperator	true false file get in input insert
syn keyword pascalOperator	interactive keyboard label maxint
syn keyword pascalOperator	mod new nil not of or output packed
syn keyword pascalOperator	put read readln repeat
syn keyword pascalOperator	reset rewrite seek set
syn keyword pascalOperator	with write writeln
syn keyword pascalStructure	record

syn keyword pascalType	char const integer real text
syn keyword pascalType	var type string

syn keyword pascalFunction	procedure function

syn sync lines=250

if !exists("pascal_traditional")
  syn keyword pascalOperator		xor shl shr
  syn keyword pascalStructure		object
  syn keyword pascalStatement		asm unit interface implementation uses
  syn keyword pascalStatement		inherited forward
  syn keyword pascalStatement		constructor destructor
  syn keyword pascalModifier		inline virtual external assembler
  syn keyword pascalModifier		near far absolute interrupt
  syn keyword pascalAcces		private public
endif

if !exists("did_pascal_syntax_inits")
  let did_pascal_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link pascalAcces			Statement
  hi link pascalByte			Number
  hi link pascalComment			Comment
  hi link pascalConditional		Conditional
  hi link pascalFunction		Function
  hi link pascalLabel			Label
  hi link pascalMatrixDelimiter		Identifier
  hi link pascalModifier		Type
  hi link pascalNumber			Number
  hi link pascalOperator		Operator
  hi link pascalRepeat			Repeat
  hi link pascalStatement		Statement
  hi link pascalString			String
  hi link pascalStructure		Structure
  hi link pascalTodo			Todo
  hi link pascalType			Type
  hi link pascalUnclassified		Statement

"optional highlighting
  "hi link pascalDelimiter		Identifier

  "hi link pascalShowTab			Error
  "hi link pascalShowTabc		Error

  "hi link pascalIdentifier		Identifier
endif

let b:current_syntax = "pascal"

" vim: ts=8 sw=2
