" Vim syntax file
" Language:	ABEL
" Maintainer:	John Cook <john.cook@kla-tencor.com>
" Last Change:	1999 Sep 22

" Remove any old syn stuff hanging around
syn clear

" this language is oblivious to case
syn case ignore

" A bunch of keywords
syn keyword abelHeader		module title device options
syn keyword abelSection		declarations equations test_vectors end
syn keyword abelDeclaration	state truth_table state_diagram property
syn keyword abelType		pin node attribute constant macro library

syn keyword abelTypeId		com reg neg pos buffer dc reg_d reg_t contained
syn keyword abelTypeId		reg_sr reg_jk reg_g retain xor invert contained

syn keyword abelStatement	when then else if with endwith case endcase
syn keyword abelStatement	fuses expr trace

" option to omit obsolete statements
if exists("abel_obsolete_ok")
  syn keyword abelStatement enable flag in
else
  syn keyword abelError enable flag in
endif

" directives
syn match abelDirective "@alternate"
syn match abelDirective "@standard"
syn match abelDirective "@const"
syn match abelDirective "@dcset"
syn match abelDirective "@include"
syn match abelDirective "@page"
syn match abelDirective "@radix"
syn match abelDirective "@repeat"
syn match abelDirective "@irp"
syn match abelDirective "@expr"
syn match abelDirective "@if"
syn match abelDirective "@ifb"
syn match abelDirective "@ifnb"
syn match abelDirective "@ifdef"
syn match abelDirective "@ifndef"
syn match abelDirective "@ifiden"
syn match abelDirective "@ifniden"

syn keyword abelTodo contained TODO XXX FIXME

" wrap up type identifiers to differentiate them from normal strings
syn region abelSpecifier start='istype' end=';' contains=abelTypeIdChar,abelTypeId,abelTypeIdEnd keepend
syn match  abelTypeIdChar "[,']" contained
syn match  abelTypeIdEnd  ";" contained

" string contstants and special characters within them
syn match  abelSpecial contained "\\['\\]"
syn region abelString start=+'+ skip=+\\"+ end=+'+ contains=abelSpecial

" valid integer number formats (decimal, binary, octal, hex)
syn match abelNumber "\<[-+]\=[0-9]\+\>"
syn match abelNumber "\^d[0-9]\+\>"
syn match abelNumber "\^b[01]\+\>"
syn match abelNumber "\^o[0-7]\+\>"
syn match abelNumber "\^h[0-9a-f]\+\>"

" special characters
" (define these after abelOperator so ?= overrides ?)
syn match abelSpecialChar "[\[\](){},;:?]"

" operators
syn match abelLogicalOperator "[!#&$]"
syn match abelRangeOperator "\.\."
syn match abelAlternateOperator "[/*+]"
syn match abelAlternateOperator ":[+*]:"
syn match abelArithmeticOperator "[-%]"
syn match abelArithmeticOperator "<<"
syn match abelArithmeticOperator ">>"
syn match abelRelationalOperator "[<>!=]="
syn match abelRelationalOperator "[<>]"
syn match abelAssignmentOperator "[:?]\=="
syn match abelAssignmentOperator "?:="
syn match abelTruthTableOperator "->"

" signal extensions
syn match abelExtension "\.aclr\>"
syn match abelExtension "\.aset\>"
syn match abelExtension "\.clk\>"
syn match abelExtension "\.clr\>"
syn match abelExtension "\.com\>"
syn match abelExtension "\.fb\>"
syn match abelExtension "\.[co]e\>"
syn match abelExtension "\.l[eh]\>"
syn match abelExtension "\.fc\>"
syn match abelExtension "\.pin\>"
syn match abelExtension "\.set\>"
syn match abelExtension "\.[djksrtq]\>"
syn match abelExtension "\.pr\>"
syn match abelExtension "\.re\>"
syn match abelExtension "\.a[pr]\>"
syn match abelExtension "\.s[pr]\>"

" special constants
syn match abelConstant "\.[ckudfpxz]\."
syn match abelConstant "\.sv[2-9]\."

" one-line comments
syn region abelComment start=+"+ end=+"\|$+ contains=abelNumber,abelTodo

syn sync minlines=1

if !exists("did_abel_syn_inits")
  let did_abel_syn_inits = 1
  " The default methods for highlighting. Can be overridden later
  hi link abelHeader	  abelStatement
  hi link abelSection	  abelStatement
  hi link abelDeclaration abelStatement
  hi link abelLogicalOperator	 abelOperator
  hi link abelRangeOperator	 abelOperator
  hi link abelAlternateOperator  abelOperator
  hi link abelArithmeticOperator abelOperator
  hi link abelRelationalOperator abelOperator
  hi link abelAssignmentOperator abelOperator
  hi link abelTruthTableOperator abelOperator
  hi link abelSpecifier   abelStatement
  hi link abelOperator	  abelStatement
  hi link abelStatement   Statement
  hi link abelIdentifier  Identifier
  hi link abelTypeId	  abelType
  hi link abelTypeIdChar  abelType
  hi link abelType	  Type
  hi link abelNumber	  abelString
  hi link abelString	  String
  hi link abelConstant	  Constant
  hi link abelComment	  Comment
  hi link abelExtension   abelSpecial
  hi link abelSpecialChar abelSpecial
  hi link abelTypeIdEnd   abelSpecial
  hi link abelSpecial	  Special
  hi link abelDirective   PreProc
  hi link abelTodo	  Todo
  hi link abelError       Error
endif

let b:current_syntax = "abel"
" vim:ts=8
