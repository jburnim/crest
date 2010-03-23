" Vim syntax file
" Language:	S-Lang
" Maintainer:	Jan Hlavacek <lahvak@math.ohio-state.edu>
" Last Change:	980216

" Remove any old syntax stuff hanging around
syn clear

syn keyword slangStatement	break return continue EXECUTE_ERROR_BLOCK
syn match slangStatement	"\<X_USER_BLOCK[0-4]\>"
syn keyword slangLabel		case
syn keyword slangConditional	!if if else switch
syn keyword slangRepeat		while for _for loop do forever
syn keyword slangDefinition	define typedef variable struct
syn keyword slangOperator	or and andelse orelse shr shl xor not
syn keyword slangBlock		EXIT_BLOCK ERROR_BLOCK
syn match slangBlock		"\<USER_BLOCK[0-4]\>"
syn keyword slangConstant	NULL
syn keyword slangType		Integer_Type Double_Type Complex_Type String_Type Struct_Type Ref_Type Null_Type Array_Type DataType_Type

syn match slangOctal		"\<0\d\+\>" contains=slangOctalError
syn match slangOctalError	"[89]\+" contained
syn match slangHex		"\<0[xX][0-9A-Fa-f]*\>"
syn match slangDecimal		"\<[1-9]\d*\>"
syn match slangFloat		"\<\d\+\."
syn match slangFloat		"\<\d\+\.\d\+\([Ee][-+]\=\d\+\)\=\>"
syn match slangFloat		"\<\d\+\.[Ee][-+]\=\d\+\>"
syn match slangFloat		"\<\d\+[Ee][-+]\=\d\+\>"
syn match slangFloat		"\.\d\+\([Ee][-+]\=\d\+\)\=\>"
syn match slangImaginary	"\.\d\+\([Ee][-+]\=\d*\)\=[ij]\>"
syn match slangImaginary	"\<\d\+\(\.\d*\)\=\([Ee][-+]\=\d\+\)\=[ij]\>"

syn region slangString oneline start='"' end='"' skip='\\"'
syn match slangCharacter	"'[^\\]'"
syn match slangCharacter	"'\\.'"
syn match slangCharacter	"'\\[0-7]\{1,3}'"
syn match slangCharacter	"'\\d\d\{1,3}'"
syn match slangCharacter	"'\\x[0-7a-fA-F]\{1,2}'"

syn match slangDelim		"[][{};:,]"
syn match slangOperator		"[-%+/&*=<>|!~^@]"

"catch errors caused by wrong parenthesis
syn region slangParen	matchgroup=slangDelim transparent start='(' end=')' contains=ALLBUT,slangParenError
syn match slangParenError	")"

syn match slangComment		"%.*$"
syn keyword slangOperator	sizeof

syn region slangPreCondit start="^\s*#\s*\(ifdef\>\|ifndef\>\|iftrue\>\|ifnfalse\>\|iffalse\>\|ifntrue\>\|if\$\|ifn\$\|\|elif\>\|else\>\|endif\>\)" skip="\\$" end="$" contains=cComment,slangString,slangCharacter,slangNumber

" Default links
if !exists("did_slang_syntax_inits")
  let did_slang_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link slangDefinition	Type
  hi link slangBlock		slangDefinition
  hi link slangLabel		Label
  hi link slangConditional	Conditional
  hi link slangRepeat		Repeat
  hi link slangCharacter	Character
  hi link slangFloat		Float
  hi link slangImaginary	Float
  hi link slangDecimal		slangNumber
  hi link slangOctal		slangNumber
  hi link slangHex		slangNumber
  hi link slangNumber		Number
  hi link slangParenError	Error
  hi link slangOctalError	Error
  hi link slangOperator		Operator
  hi link slangStructure	Structure
  hi link slangInclude		Include
  hi link slangPreCondit	PreCondit
  hi link slangError		Error
  hi link slangStatement	Statement
  hi link slangType		Type
  hi link slangString		String
  hi link slangConstant		Constant
  hi link slangRangeArray	slangConstant
  hi link slangComment		Comment
  hi link slangSpecial		SpecialChar
  hi link slangTodo		Todo
  hi link slangDelim		Delimiter
endif

let b:current_syntax = "slang"

" vim: ts=8
