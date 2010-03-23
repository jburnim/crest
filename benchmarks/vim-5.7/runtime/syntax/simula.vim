" Vim syntax file
" Language:	Simula
" Maintainer:	Haakon Riiser <hakonrk@fys.uio.no>
" Last Change:	1999 Mar 10

" Clear old syntax defs
syn clear

" No case sensitivity in Simula
syn case	ignore

syn match	simulaComment		"^%.*$" contains=simulaTodo
syn region	simulaComment		start="!\|\<comment\>" end=";" contains=simulaTodo

" Text between the keyword 'end' and either a semicolon or one of the
" keywords 'end', 'else', 'when' or 'otherwise' is also a comment
syn region	simulaComment		start="\<end\>"lc=3 matchgroup=Statement end=";\|\<\(end\|else\|when\|otherwise\)\>"

syn match	simulaCharError		"'.\{-2,}'"
syn match	simulaCharacter		"'.'"
syn match	simulaCharacter		"'!\d\{-}!'" contains=simulaSpecialChar
syn match	simulaString		'".\{-}"' contains=simulaSpecialChar,simulaTodo

syn keyword	simulaBoolean		true false
syn keyword	simulaCompound		begin end
syn keyword	simulaConditional	else if otherwise then until when
syn keyword	simulaConstant		none notext
syn keyword	simulaFunction		procedure
syn keyword	simulaOperator		eq eqv ge gt imp in is le lt ne new not qua
syn keyword	simulaRepeat		while for
syn keyword	simulaReserved		activate after at before delay go goto label prior reactivate switch to
syn keyword	simulaStatement		do inner inspect step this
syn keyword	simulaStorageClass	external hidden name protected value
syn keyword	simulaStructure		class
syn keyword	simulaType		array boolean character integer long real short text virtual
syn match	simulaAssigned		"\<\h\w*\s*\((.*)\)\=\s*:\(=\|-\)"me=e-2
syn match	simulaOperator		"[&:=<>+\-*/]"
syn match	simulaOperator		"\<and\(\s\+then\)\=\>"
syn match	simulaOperator		"\<or\(\s\+else\)\=\>"
syn match	simulaReferenceType	"\<ref\s*(.\{-})"
syn match	simulaSemicolon		";"
syn match	simulaSpecial		"[(),.]"
syn match	simulaSpecialCharErr	"!\d\{-4,}!" contained
syn match	simulaSpecialCharErr	"!!" contained
syn match	simulaSpecialChar	"!\d\{-}!" contains=simulaSpecialCharErr contained
syn match	simulaTodo		"xxx\+" contained

" Integer number (or float without `.')
syn match	simulaNumber		"-\=\<\d\+\>"
" Real with optional exponent
syn match	simulaReal		"-\=\<\d\+\(\.\d\+\)\=\(&&\=[+-]\=\d\+\)\=\>"
" Real starting with a `.', optional exponent
syn match	simulaReal		"-\=\.\d\+\(&&\=[+-]\=\d\+\)\=\>"

if !exists("did_simula_syntax_inits")
    let did_simula_syntax_inits = 1
    hi link simulaAssigned		Identifier
    hi link simulaBoolean		Boolean
    hi link simulaCharacter		Character
    hi link simulaCharError		Error
    hi link simulaComment		Comment
    hi link simulaCompound		Statement
    hi link simulaConditional		Conditional
    hi link simulaConstant		Constant
    hi link simulaFunction		Function
    hi link simulaNumber		Number
    hi link simulaOperator		Operator
    hi link simulaReal			Float
    hi link simulaReferenceType		Type
    hi link simulaRepeat		Repeat
    hi link simulaReserved		Error
    hi link simulaSemicolon		Statement
    hi link simulaSpecial		Special
    hi link simulaSpecialChar		SpecialChar
    hi link simulaSpecialCharErr	Error
    hi link simulaStatement		Statement
    hi link simulaStorageClass		StorageClass
    hi link simulaString		String
    hi link simulaStructure		Structure
    hi link simulaTodo			Todo
    hi link simulaType			Type
endif

let b:current_syntax = "simula"
" vim: sts=4 sw=4 ts=8
