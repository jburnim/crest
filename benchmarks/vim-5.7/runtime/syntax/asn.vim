" Vim syntax file
" Language:	ASN.1
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/asn.vim
" Last Change:	1998 Mar 7

" Remove any old syntax stuff hanging around
syn clear

" keyword definitions
syn keyword asnExternal		DEFINITIONS BEGIN END IMPORTS EXPORTS FROM
syn match   asnExternal		"\<IMPLICIT\s\+TAGS\>"
syn match   asnExternal		"\<EXPLICIT\s\+TAGS\>"
syn keyword asnFieldOption	DEFAULT OPTIONAL
syn keyword asnTagModifier	IMPLICIT EXPLICIT
syn keyword asnTypeInfo		ABSENT PRESENT SIZE UNIVERSAL APPLICATION PRIVATE
syn keyword asnBoolValue	TRUE FALSE
syn keyword asnNumber		MIN MAX
syn match   asnNumber		"\<PLUS-INFINITY\>"
syn match   asnNumber		"\<MINUS-INFINITY\>"
syn keyword asnType		INTEGER REAL STRING BIT BOOLEAN OCTET NULL EMBEDDED PDV
syn keyword asnType		BMPString IA5String TeletexString GeneralString GraphicString ISO646String NumericString PrintableString T61String UniversalString VideotexString VisibleString
syn keyword asnType		ANY DEFINED
syn match   asnType		"\.\.\."
syn match   asnType		"OBJECT\s\+IDENTIFIER"
syn match   asnType		"TYPE-IDENTIFIER"
syn keyword asnType		UTF8String
syn keyword asnStructure	CHOICE SEQUENCE SET OF ENUMERATED CONSTRAINED BY WITH COMPONENTS CLASS

" Strings and constants
syn match   asnSpecial		contained "\\\d\d\d\|\\."
syn region  asnString		start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=asnSpecial
syn match   asnCharacter	"'[^\\]'"
syn match   asnSpecialCharacter "'\\.'"
syn match   asnNumber		"-\=\<\d\+L\=\>\|0[xX][0-9a-fA-F]\+\>"
syn match   asnLineComment	"--.*"
syn match   asnLineComment	"--.*--"

syn match asnDefinition "^\s*[a-zA-Z][-a-zA-Z0-9_.\[\] \t{}]* *::="me=e-3 contains=asnType
syn match asnBraces     "[{}]"

syn sync ccomment asnComment

if !exists("did_asn_syntax_inits")
  let did_asn_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link asnDefinition		Function
  hi link asnBraces		Function
  hi link asnStructure		Statement
  hi link asnBoolValue		Boolean
  hi link asnSpecial		Special
  hi link asnString		String
  hi link asnCharacter		Character
  hi link asnSpecialCharacter	asnSpecial
  hi link asnNumber		asnValue
  hi link asnComment		Comment
  hi link asnLineComment	asnComment
  hi link asnType		Type
  hi link asnTypeInfo		PreProc
  hi link asnValue		Number
  hi link asnExternal		Include
  hi link asnTagModifier	Function
  hi link asnFieldOption	Type
endif

let b:current_syntax = "asn"

" vim: ts=8
