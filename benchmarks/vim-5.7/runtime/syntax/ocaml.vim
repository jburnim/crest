" Vim syntax file
" Language:	OCAML
" Maintainers:	Markus Mottl	  <mottl>@miss.wu-wien.ac.at
"		Karl-Heinz Sylla <Karl-Heinz.Sylla>@gmd.de
" URL:		http://miss.wu-wien.ac.at/~mottl/vim/syntax/ocaml.vim
" Last Change:	1999 Oct  10 - small fix for modules (MM)


" Remove any old syntax stuff hanging around.
syn clear


" OCAML is case sensitive.
syn case match


" lowercase identifier - the standard way to match
syn match    ocamlLCIdentifier /\<\(\l\|_\)\(\w\|'\)*\>/

syn match    ocamlKeyChar    "|"

" Errors
syn match    ocamlBraceErr   "}"
syn match    ocamlBrackErr   "\]"
syn match    ocamlParenErr   ")"
syn match    ocamlArrErr     "|]"
syn match    ocamlStreamErr  ">]"

syn match    ocamlCommentErr "\*)"

syn match    ocamlCountErr   "\<downto\>"
syn match    ocamlCountErr   "\<to\>"
syn match    ocamlDoErr      "\<do\>"
syn match    ocamlDoneErr    "\<done\>"
syn match    ocamlThenErr    "\<then\>"

" Error-highlighting of "end" without synchronization:
" as keyword or as error (default)
if exists("ocaml_noend_error")
  syn match    ocamlKeyword    "\<end\>"
else
  syn match    ocamlEndErr     "\<end\>"
endif

" Some convenient clusters
syn cluster  ocamlAllErrs contains=ocamlBraceErr,ocamlBrackErr,ocamlParenErr,ocamlCommentErr,ocamlCountErr,ocamlDoErr,ocamlDoneErr,ocamlEndErr,ocamlThenErr

syn cluster  ocamlAENoParen contains=ocamlBraceErr,ocamlBrackErr,ocamlCommentErr,ocamlCountErr,ocamlDoErr,ocamlDoneErr,ocamlEndErr,ocamlThenErr

syn cluster  ocamlContained contains=ocamlTodo,ocamlPreDef,ocamlModParam,ocamlModParam1,ocamlPreMPRestr,ocamlMPRestr,ocamlMPRestr1,ocamlMPRestr2,ocamlMPRestr3,ocamlModRHS,ocamlFuncWith,ocamlFuncStruct,ocamlModTypeRestr,ocamlModTRWith,ocamlWith,ocamlWithRest,ocamlModType,ocamlFullMod


" Enclosing delimiters
syn region   ocamlEncl transparent matchgroup=ocamlKeyword start="(" matchgroup=ocamlKeyword end=")" contains=ALLBUT,@ocamlContained,ocamlParenErr
syn region   ocamlEncl transparent matchgroup=ocamlKeyword start="{" matchgroup=ocamlKeyword end="}"  contains=ALLBUT,@ocamlContained,ocamlBraceErr
syn region   ocamlEncl transparent matchgroup=ocamlKeyword start="\[" matchgroup=ocamlKeyword end="\]" contains=ALLBUT,@ocamlContained,ocamlBrackErr
syn region   ocamlEncl transparent matchgroup=ocamlKeyword start="\[|" matchgroup=ocamlKeyword end="|\]" contains=ALLBUT,@ocamlContained,ocamlArrErr
syn region   ocamlEncl transparent matchgroup=ocamlKeyword start="\[<" matchgroup=ocamlKeyword end=">\]" contains=ALLBUT,@ocamlContained,ocamlStreamErr


" Comments
syn region   ocamlComment start="(\*" end="\*)" contains=ocamlComment,ocamlTodo
syn keyword  ocamlTodo contained TODO FIXME XXX


" Objects
syn region   ocamlEnd matchgroup=ocamlKeyword start="\<object\>" matchgroup=ocamlKeyword end="\<end\>" contains=ALLBUT,@ocamlContained,ocamlEndErr


" Blocks
syn region   ocamlEnd matchgroup=ocamlKeyword start="\<begin\>" matchgroup=ocamlKeyword end="\<end\>" contains=ALLBUT,@ocamlContained,ocamlEndErr


" "for"
syn region   ocamlNone matchgroup=ocamlKeyword start="\<for\>" matchgroup=ocamlKeyword end="\<\(to\|downto\)\>" contains=ALLBUT,@ocamlContained,ocamlCountErr


" "do"
syn region   ocamlDo matchgroup=ocamlKeyword start="\<do\>" matchgroup=ocamlKeyword end="\<done\>" contains=ALLBUT,@ocamlContained,ocamlDoneErr


" "if"
syn region   ocamlNone matchgroup=ocamlKeyword start="\<if\>" matchgroup=ocamlKeyword end="\<then\>" contains=ALLBUT,@ocamlContained,ocamlThenErr


"" Modules

" "struct"
syn region   ocamlStruct matchgroup=ocamlModule start="\<struct\>" matchgroup=ocamlModule end="\<end\>" contains=ALLBUT,@ocamlContained,ocamlEndErr

" "sig"
syn region   ocamlSig matchgroup=ocamlModule start="\<sig\>" matchgroup=ocamlModule end="\<end\>" contains=ALLBUT,@ocamlContained,ocamlEndErr,ocamlModule
syn region   ocamlModSpec matchgroup=ocamlKeyword start="\<module\>" matchgroup=ocamlModule end="\<\u\(\w\|'\)*\>" contained contains=@ocamlAllErrs,ocamlComment skipwhite skipempty nextgroup=ocamlModTRWith,ocamlMPRestr

" "open"
syn region   ocamlNone matchgroup=ocamlKeyword start="\<open\>" matchgroup=ocamlModule end="\<\u\(\w\|'\)*\(\.\u\(\w\|'\)*\)*\>" contains=@ocamlAllErrs,ocamlComment

" "include"
syn region   ocamlNone matchgroup=ocamlKeyword start="\<include\>" matchgroup=ocamlModPath end="\<\(\u\(\w\|'\)*\.\)*\w\(\w\|'\)*\>" contains=o@ocamlAllErrs,camlComment

" "module" - somewhat complicated stuff ;-)
syn region   ocamlModule matchgroup=ocamlKeyword start="\<module\>" matchgroup=ocamlModule end="\<\u\(\w\|'\)*\>" contains=@ocamlAllErrs,ocamlComment skipwhite skipempty nextgroup=ocamlPreDef
syn region   ocamlPreDef start="."me=e-1 matchgroup=ocamlKeyword end="\l\|="me=e-1 contained contains=@ocamlAllErrs,ocamlComment,ocamlModParam,ocamlModTypeRestr,ocamlModTRWith nextgroup=ocamlModPreRHS
syn region   ocamlModParam start="([^*]" end=")" contained contains=@ocamlAENoParen,ocamlModParam1
syn match    ocamlModParam1 "\<\u\(\w\|'\)*\>" contained skipwhite skipempty nextgroup=ocamlPreMPRestr

syn region   ocamlPreMPRestr start="."me=e-1 end=")"me=e-1 contained contains=@ocamlAllErrs,ocamlComment,ocamlMPRestr,ocamlModTypeRestr

syn region   ocamlMPRestr start=":" end="."me=e-1 contained contains=@ocamlComment skipwhite skipempty nextgroup=ocamlMPRestr1,ocamlMPRestr2,ocamlMPRestr3
syn region   ocamlMPRestr1 matchgroup=ocamlModule start="\ssig\s\=" matchgroup=ocamlModule end="\<end\>" contained contains=ALLBUT,@ocamlContained,ocamlEndErr,ocamlModule
syn region   ocamlMPRestr2 start="\sfunctor\(\s\|(\)\="me=e-1 matchgroup=ocamlKeyword end="->" contained contains=@ocamlAllErrs,ocamlComment,ocamlModParam skipwhite skipempty nextgroup=ocamlFuncWith
syn match    ocamlMPRestr3 "\w\(\w\|'\)*\(\.\w\(\w\|'\)*\)*" contained
syn match  ocamlModPreRHS "=" contained skipwhite skipempty nextgroup=ocamlModParam,ocamlFullMod
syn region  ocamlModRHS start="." end=".\w\|([^*]"me=e-2 contained contains=ocamlComment skipwhite skipempty nextgroup=ocamlModParam,ocamlFullMod
syn match   ocamlFullMod "\<\u\(\w\|'\)*\(\.\u\(\w\|'\)*\)*" contained skipwhite skipempty nextgroup=ocamlFuncWith

syn region  ocamlFuncWith start="("me=e-1 end=")" contained contains=ocamlComment,ocamlWith,ocamlFuncStruct
syn region  ocamlFuncStruct matchgroup=ocamlModule start="[^a-zA-Z]struct\>"hs=s+1 matchgroup=ocamlModule end="\<end\>" contains=ALLBUT,@ocamlContained,ocamlEndErr

syn match    ocamlModTypeRestr "\<\w\(\w\|'\)*\(\.\w\(\w\|'\)*\)*\>" contained
syn region   ocamlModTRWith start=":\s*("hs=s+1 end=")" contained contains=@ocamlAENoParen,ocamlWith
syn match    ocamlWith "\<\(\u\(\w\|'\)*\.\)*\w\(\w\|'\)*\>" contained skipwhite skipempty nextgroup=ocamlWithRest
syn region   ocamlWithRest start="[^)]" end=")"me=e-1 contained contains=ALLBUT,@ocamlContained

" "module type"
syn region   ocamlKeyword start="\<module\s*type\>" matchgroup=ocamlModule end="\<\w\(\w\|'\)*\>" contains=ocamlComment skipwhite skipempty nextgroup=ocamlMTDef
syn match    ocamlMTDef "=\s*\w\(\w\|'\)*\>"hs=s+1,me=s

syn keyword  ocamlKeyword  and as assert class
syn keyword  ocamlKeyword  constraint else
syn keyword  ocamlKeyword  exception external fun function
syn keyword  ocamlKeyword  in inherit initializer
syn keyword  ocamlKeyword  land lazy let match
syn keyword  ocamlKeyword  method mutable new of
syn keyword  ocamlKeyword  parser private raise rec
syn keyword  ocamlKeyword  try type
syn keyword  ocamlKeyword  val virtual when while with

syn keyword  ocamlType     array bool char exn float format int
syn keyword  ocamlType     list option string unit

syn keyword  ocamlOperator asr lor lsl lsr lxor mod not or

syn keyword  ocamlBoolean      true false
syn match    ocamlConstructor  "(\s*)"
syn match    ocamlConstructor  "\[\s*\]"
syn match    ocamlConstructor  "\[|\s*>|]"
syn match    ocamlConstructor  "\[<\s*>\]"
syn match    ocamlConstructor  "\u\(\w\|'\)*\>"

" Module prefix
syn match    ocamlModPath      "\u\(\w\|'\)*\."he=e-1

syn match    ocamlCharacter    "'.'\|'\\\d\d\d'\|'\\[\'ntbr]'"
syn match    ocamlCharErr      "'\\\d\d'\|'\\\d'"
syn match    ocamlCharErr      "'\\\d\d'\|'\\\d'"
syn match    ocamlCharErr      "'\\[^\'ntbr]'"
syn region   ocamlString       start=+"+ skip=+\\\\\|\\"+ end=+"+

syn match    ocamlFunDef       "->"
syn match    ocamlRefAssign    ":="
syn match    ocamlTopStop      ";;"
syn match    ocamlOperator     "\^"
syn match    ocamlOperator     "::"
syn match    ocamlOperator     "<-"
syn match    ocamlAnyVar       "\<_\>"
syn match    ocamlKeyChar      "!"
syn match    ocamlKeyChar      "|[^\]]"me=e-1
syn match    ocamlKeyChar      ";"
syn match    ocamlKeyChar      "?"
syn match    ocamlKeyChar      "\*"
syn match    ocamlKeyChar      "="

syn match    ocamlNumber        "\<-\=\d\+\>"
syn match    ocamlNumber        "\<-\=0[x|X]\x\+\>"
syn match    ocamlNumber        "\<-\=0[o|O]\o\+\>"
syn match    ocamlNumber        "\<-\=0[b|B][01]\+\>"
syn match    ocamlFloat         "\<-\=\d\+\.\d*\([eE][-+]\=\d\+\)\=[fl]\=\>"

" synchronization
syn sync minlines=20
syn sync maxlines=500

syn sync match ocamlDoSync      grouphere  ocamlDo      "\<do\>"
syn sync match ocamlDoSync      groupthere ocamlDo      "\<done\>"
syn sync match ocamlEndSync     grouphere  ocamlEnd     "\<\(begin\|object\)\>"
syn sync match ocamlEndSync     groupthere ocamlEnd     "\<end\>"
syn sync match ocamlStructSync  grouphere  ocamlStruct  "\<struct\>"
syn sync match ocamlStructSync  groupthere ocamlStruct  "\<end\>"
syn sync match ocamlSigSync     grouphere  ocamlSig     "\<sig\>"
syn sync match ocamlSigSync     groupthere ocamlSig     "\<end\>"

if !exists("did_ocaml_syntax_inits")
  " The default methods for highlighting.  Can be overridden later
  let did_ocaml_syntax_inits = 1

  hi link ocamlBraceErr     Error
  hi link ocamlBrackErr     Error
  hi link ocamlParenErr     Error
  hi link ocamlStreamErr    Error
  hi link ocamlArrErr       Error

  hi link ocamlCommentErr   Error

  hi link ocamlCountErr     Error
  hi link ocamlDoErr        Error
  hi link ocamlDoneErr      Error
  hi link ocamlEndErr       Error
  hi link ocamlThenErr      Error

  hi link ocamlCharErr      Error

  hi link ocamlComment      Comment

  hi link ocamlModPath      Include
  hi link ocamlModule       Include
  hi link ocamlModParam1    Include
  hi link ocamlModType      Include
  hi link ocamlMPRestr3     Include
  hi link ocamlFullMod      Include
  hi link ocamlModTypeRestr Include
  hi link ocamlWith         Include
  hi link ocamlMTDef        Include

  hi link ocamlConstructor  Constant

  hi link ocamlModPreRHS    Keyword
  hi link ocamlMPRestr2     Keyword
  hi link ocamlEnvKeyword   Keyword
  hi link ocamlKeyword      Keyword
  hi link ocamlFunDef       Keyword
  hi link ocamlRefAssign    Keyword
  hi link ocamlKeyChar      Keyword
  hi link ocamlAnyVar       Keyword
  hi link ocamlTopStop      Keyword
  hi link ocamlOperator     Keyword

  hi link ocamlBoolean      Boolean
  hi link ocamlCharacter    Character
  hi link ocamlNumber       Number
  hi link ocamlFloat        Float
  hi link ocamlString       String

  hi link ocamlType         Type

  hi link ocamlTodo         Todo

  hi link ocamlEncl         Keyword

endif

let b:current_syntax = "ocaml"
