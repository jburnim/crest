" Vim syntax file
" Language:	IDL (Interface Description Language)
" Maintainer:	Jody Goldberg <jodyg@idt.net>
" Last Change:	1997 Nov 20

" This is an experiment.  IDL's structure is simple enough to permit a full
" grammar based approach to rather than using a few heuristics.  The result
" is large and somewhat repetative but seems to work.

" Remove any old syntax stuff hanging around
syn clear

" Misc basic
syn match	idlId		contained "[a-zA-Z][a-zA-Z0-9_]*"
syn match	idlSemiColon	contained ";"
syn match	idlCommaArg	contained ","			skipempty skipwhite nextgroup=idlSimpDecl
syn region	idlArraySize1	contained start=:\[: end=:\]:	skipempty skipwhite nextgroup=idlArraySize1,idlSemiColon,idlCommaArg contains=idlArraySize1,idlLiteral
syn match   idlSimpDecl	 contained "[a-zA-Z][a-zA-Z0-9_]*"	skipempty skipwhite nextgroup=idlSemiColon,idlCommaArg,idlArraySize1
syn region  idlSting	 contained start=+"+  skip=+\\\(\\\\\)*"+  end=+"+
syn match   idlLiteral	 contained "[1-9]\d*\(\.\d*\)\="
syn match   idlLiteral	 contained "\.\d\+"
syn keyword idlLiteral	 contained TRUE FALSE

" Comments
syn keyword idlTodo contained	TODO FIXME XXX
syn region idlComment		start="/\*"  end="\*/" contains=idlTodo
syn match  idlComment		"//.*" contains=idlTodo
syn match  idlCommentError	"\*/"

" C style Preprocessor
syn region idlIncluded contained start=+"+  skip=+\\\(\\\\\)*"+  end=+"+
syn match  idlIncluded contained "<[^>]*>"
syn match  idlInclude		"^[ \t]*#[ \t]*include\>[ \t]*["<]" contains=idlIncluded, idlString
syn region idlPreCondit	start="^[ \t]*#[ \t]*\(if\>\|ifdef\>\|ifndef\>\|elif\>\|else\>\|endif\>\)"  skip="\\$"  end="$" contains=idlComment,idlCommentError
syn region idlDefine	start="^[ \t]*#[ \t]*\(define\>\|undef\>\)" skip="\\$" end="$" contains=idlLiteral, idlString

" Constants
syn keyword idlConst	const	skipempty skipwhite nextgroup=idlBaseType,idlBaseTypeInt

" Attribute
syn keyword idlROAttr	readonly	skipempty skipwhite nextgroup=idlAttr
syn keyword idlAttr	attribute	skipempty skipwhite nextgroup=idlBaseTypeInt,idlBaseType

" Types
syn region  idlD4	contained start="<" end=">"	skipempty skipwhite nextgroup=idlSimpDecl	contains=idlSeqType,idlBaseTypeInt,idlBaseType,idlLiteral
syn keyword idlSeqType	contained sequence		skipempty skipwhite nextgroup=idlD4
syn keyword idlBaseType		contained	float double char boolean octet any	skipempty skipwhite nextgroup=idlSimpDecl
syn keyword idlBaseTypeInt	contained	short long		skipempty skipwhite nextgroup=idlSimpDecl
syn keyword idlBaseType		contained	unsigned		skipempty skipwhite nextgroup=idlBaseTypeInt
syn region  idlD1		contained	start="<" end=">"	skipempty skipwhite nextgroup=idlSimpDecl	contains=idlString,idlLiteral
syn keyword idlBaseType		contained	string	skipempty skipwhite nextgroup=idlD1,idlSimpDecl
syn match   idlBaseType		contained	"[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*"	skipempty skipwhite nextgroup=idlSimpDecl

" Modules
syn region  idlModuleContent contained start="{" end="}"	skipempty skipwhite nextgroup=idlSemiColon contains=idlUnion,idlStruct,idlEnum,idlInterface,idlComment,idlTypedef,idlConst,idlException,idlModule
syn match   idlModuleName contained	"[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlModuleContent,idlSemiColon
syn keyword idlModule			module			skipempty skipwhite nextgroup=idlModuleName

" Interfaces
syn region  idlInterfaceContent contained start="{" end="}"	skipempty skipwhite nextgroup=idlSemiColon contains=idlUnion,idlStruct,idlEnum,idlComment,idlROAttr,idlAttr,idlOp,idlOneWayOp,idlException,idlConst,idlTypedef
syn match   idlInheritFrom2 contained "," skipempty skipwhite nextgroup=idlInheritFrom
syn match idlInheritFrom contained "[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*" skipempty skipwhite nextgroup=idlInheritFrom2,idlInterfaceContent
syn match idlInherit contained	":"		skipempty skipwhite nextgroup=idlInheritFrom
syn match   idlInterfaceName contained	"[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlInterfaceContent,idlInherit,idlSemiColon
syn keyword idlInterface		interface		skipempty skipwhite nextgroup=idlInterfaceName


" Raises
syn keyword idlRaises	contained raises	skipempty skipwhite nextgroup=idlRaises,idlContext,idlSemiColon

" Context
syn keyword idlContext	contained context	skipempty skipwhite nextgroup=idlRaises,idlContext,idlSemiColon

" Operation
syn match   idlParmList	contained "," skipempty skipwhite nextgroup=idlOpParms
syn region  idlArraySize contained start="\[" end="\]"	skipempty skipwhite nextgroup=idlArraySize,idlParmList contains=idlArraySize,idlLiteral
syn match   idlParmName contained "[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlParmList, idlArraySize
syn keyword idlParmInt	contained short long		skipempty skipwhite nextgroup=idlParmName
syn keyword idlParmType	contained unsigned		skipempty skipwhite nextgroup=idlParmInt
syn region  idlD3	contained start="<" end=">"	skipempty skipwhite nextgroup=idlParmName	contains=idlString,idlLiteral
syn keyword idlParmType	contained string		skipempty skipwhite nextgroup=idlD3,idlParmName
syn keyword idlParmType	contained void float double char boolean octet any	  skipempty skipwhite nextgroup=idlParmName
syn match   idlParmType	contained "[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*" skipempty skipwhite nextgroup=idlParmName
syn keyword idlOpParms	contained in out inout		skipempty skipwhite nextgroup=idlParmType

syn region idlOpContents contained start="(" end=")"	skipempty skipwhite nextgroup=idlRaises,idlContext,idlSemiColon contains=idlOpParms
syn match   idlOpName   contained "[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlOpContents
syn keyword idlOpInt	contained short long		skipempty skipwhite nextgroup=idlOpName
syn region  idlD2	contained start="<" end=">"	skipempty skipwhite nextgroup=idlOpName	contains=idlString,idlLiteral
syn keyword idlOp	contained unsigned		skipempty skipwhite nextgroup=idlOpInt
syn keyword idlOp	contained string		skipempty skipwhite nextgroup=idlD2,idlOpName
syn keyword idlOp	contained void float double char boolean octet any		skipempty skipwhite nextgroup=idlOpName
syn match   idlOp	contained "[a-zA-Z0-9_]\+[ \t]*\(::[ \t]*[a-zA-Z0-9_]\+\)*"	skipempty skipwhite nextgroup=idlOpName
syn keyword idlOp	contained void			skipempty skipwhite nextgroup=idlOpName
syn keyword idlOneWayOp	contained oneway		skipempty skipwhite nextgroup=idOp

" Enum
syn region  idlEnumContents contained start="{" end="}"		skipempty skipwhite nextgroup=idlSemiColon, idlSimpDecl contains=idlId
syn match   idlEnumName contained	"[a-zA-Z0-9_]\+"	skipempty skipwhite nextgroup=idlEnumContents
syn keyword idlEnum			enum			skipempty skipwhite nextgroup=idlEnumName

" Typedef
syn keyword idlTypedef			typedef			skipempty skipwhite nextgroup=idlBaseType, idlBaseTypeInt, idlSeqType

" Struct
syn region  idlStructContent contained start="{" end="}" skipempty skipwhite nextgroup=idlSemiColon, idlSimpDecl	contains=idlBaseType, idlBaseTypeInt, idlSeqType,idlComment, idlEnum, idlUnion
syn match   idlStructName contained	"[a-zA-Z0-9_]\+" skipempty skipwhite nextgroup=idlStructContent
syn keyword idlStruct			struct		 skipempty skipwhite nextgroup=idlStructName

" Exception
syn keyword idlException exception skipempty skipwhite nextgroup=idlStructName

" Union
syn match   idlColon contained ":"	skipempty skipwhite nextgroup=idlCase, idlSeqType,idlBaseType,idlBaseTypeInt
syn region  idlCaseLabel contained start="" skip="::" end=":"me=e-1	skipempty skipwhite nextgroup=idlColon contains=idlLiteral,idlString
syn keyword idlCase		contained case				skipempty skipwhite nextgroup=idlCaseLabel
syn keyword idlCase		contained default			skipempty skipwhite nextgroup=idlColon
syn region  idlUnionContent	contained start="{" end="}"		skipempty skipwhite nextgroup=idlSemiColon,idlSimpDecl	contains=idlCase
syn region  idlSwitchType	contained start="(" end=")"		skipempty skipwhite nextgroup=idlUnionContent
syn keyword idlUnionSwitch	contained switch			skipempty skipwhite nextgroup=idlSwitchType
syn match   idlUnionName	contained "[a-zA-Z0-9_]\+"		skipempty skipwhite nextgroup=idlUnionSwitch
syn keyword idlUnion		union				skipempty skipwhite nextgroup=idlUnionName

syn sync lines=200

if !exists("did_idl_syntax_inits")
  let did_idl_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link idlInclude	Include
  hi link idlPreProc	PreProc
  hi link idlPreCondit	PreCondit
  hi link idlDefine	Macro
  hi link idlIncluded	String
  hi link idlString	String
  hi link idlComment	Comment
  hi link idlTodo	Todo
  hi link idlLiteral	Number

  hi link idlModule	Keyword
  hi link idlInterface	Keyword
  hi link idlEnum	Keyword
  hi link idlStruct	Keyword
  hi link idlUnion	Keyword
  hi link idlTypedef	Keyword
  hi link idlException	Keyword

  hi link idlModuleName		Typedef
  hi link idlInterfaceName	Typedef
  hi link idlEnumName		Typedef
  hi link idlStructName		Typedef
  hi link idlUnionName		Typedef

  hi link idlBaseTypeInt	idlType
  hi link idlBaseType		idlType
  hi link idlSeqType		idlType
  hi link idlD1			Paren
  hi link idlD2			Paren
  hi link idlD3			Paren
  hi link idlD4			Paren
  "hi link idlArraySize		Paren
  "hi link idlArraySize1	Paren
  hi link idlModuleContent	Paren
  hi link idlUnionContent	Paren
  hi link idlStructContent	Paren
  hi link idlEnumContents	Paren
  hi link idlInterfaceContent	Paren

  hi link idlSimpDecl		Identifier
  hi link idlROAttr		StorageClass
  hi link idlAttr		Keyword
  hi link idlConst		StorageClass

  hi link idlOneWayOp	StorageClass
  hi link idlOp		idlType
  hi link idlParmType	idlType
  hi link idlOpName	Function
  hi link idlOpParms	StorageClass
  hi link idlParmName	Identifier
  hi link idlInheritFrom	Identifier

  hi link idlId		Constant
  "hi link idlCase	Keyword
  hi link idlCaseLabel	Constant
endif

let b:current_syntax = "idl"

" vim: ts=8
