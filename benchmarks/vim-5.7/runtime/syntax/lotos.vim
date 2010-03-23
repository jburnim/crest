" Vim syntax file
" Language:	LOTOS (Language Of Temporal Ordering Specifications, IS8807)
" Maintainer:	Daniel Amyot <damyot@csi.uottawa.ca>
" Last Change:	Wed Aug 19 1998
" URL:		http://lotos.csi.uottawa.ca/~damyot/vim/lotos.vim
" This file is an adaptation of pascal.vim by Mario Eusebio
" I'm not sure I understand all of the syntax highlight language,
" but this file seems to do the job for standard LOTOS.

syn clear

syn case ignore

"Comments in LOTOS are between (* and *)
syn region lotosComment	start="(\*"  end="\*)" contains=lotosTodo

"Operators [], [...], >>, ->, |||, |[...]|, ||, ;, !, ?, :, =, ,, :=
syn match  lotosDelimiter       "[][]"
syn match  lotosDelimiter	">>"
syn match  lotosDelimiter	"->"
syn match  lotosDelimiter	"\[>"
syn match  lotosDelimiter	"[|;!?:=,]"

"Regular keywords
syn keyword lotosStatement	specification endspec process endproc
syn keyword lotosStatement	where behaviour behavior
syn keyword lotosStatement      any let par accept choice hide of in
syn keyword lotosStatement	i stop exit noexit

"Operators from the Abstract Data Types in IS8807
syn keyword lotosOperator	eq ne succ and or xor implies iff
syn keyword lotosOperator	not true false
syn keyword lotosOperator	Insert Remove IsIn NotIn Union Ints
syn keyword lotosOperator	Minus Includes IsSubsetOf
syn keyword lotosOperator	lt le ge gt 0

"Sorts in IS8807
syn keyword lotosSort		Boolean Bool FBoolean FBool Element
syn keyword lotosSort		Set String NaturalNumber Nat HexString
syn keyword lotosSort		HexDigit DecString DecDigit
syn keyword lotosSort		OctString OctDigit BitString Bit
syn keyword lotosSort		Octet OctetString

"Keywords for ADTs
syn keyword lotosType	type endtype library endlib sorts formalsorts
syn keyword lotosType	eqns formaleqns opns formalopns forall ofsort is
syn keyword lotosType   for renamedby actualizedby sortnames opnnames
syn keyword lotosType   using

syn sync lines=250

if !exists("did_lotos_syntax_inits")
  let did_lotos_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link lotosStatement		Statement
  hi link lotosProcess			Label
  hi link lotosOperator			Operator
  hi link lotosSort			Function
  hi link lotosType			Type
  hi link lotosComment			Comment
  hi link lotosDelimiter                String
endif

let b:current_syntax = "lotos"

" vim: ts=8
