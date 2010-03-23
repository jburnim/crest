" Vim syntax file
" Language:   SML
" Maintainers:   Fabrizio Zeno Cornelli <zeno@filibusta.crema.unimi.it>
" Last change:   2000 May 20

" There are two sets of highlighting in here:
" If the "sml_highlighting_clean" variable exists, it is rather sparse.
" Otherwise you get more highlighting.

" Remove any old syntax stuff hanging around.
syn clear

" Sml is case sensitive.
syn case match

" Very simple highlighting for comments, clause heads and
" character codes.  It respects sml strings and atoms.

syn region   smlComment     start=+(\*+ end=+\*)+

" syn keyword  smlKeyword
syn keyword  smlStatement 	 let in end
syn keyword  smlFunction 	 fun val
syn match    smlCharCode     +0'\\\=.+
syn region   smlString       start=+"+ skip=+\\"+ end=+"+
syn region   smlAtom         start=+'+ skip=+\\'+ end=+'+

if !exists("sml_highlighting_clean")

  " some keywords
  " some common predicates are also highlighted as keywords
  " is there a better solution?

  syn keyword smlKeyword   abstype local struct sig  with
  syn keyword smlKeyword   if then else case of fn and
  syn keyword smlKeyword   datatype type exception open infix infixr nonfix
  syn keyword smlKeyword   handle raise withtype
  syn keyword smlKeyword   while do

  syn keyword   smlType 	real list integer
syn keyword smlStructures  Array BinIO BinPrimIO
syn keyword smlStructures  Bool Byte Char
syn keyword smlStructures  CharArray CharVector CommandLine
syn keyword smlStructures  Date General IEEEReal
syn keyword smlStructures  Int IO LargeInt
syn keyword smlStructures  LargeReal LargeWord List
syn keyword smlStructures  ListPair Math Option
syn keyword smlStructures  OS OS.FileSys OS.IO
syn keyword smlStructures  OS.Path OS.Process Position
syn keyword smlStructures  Real SML90 String
syn keyword smlStructures  StringCvt Substring TextIO
syn keyword smlStructures  TextPrimIO Time Timer
syn keyword smlStructures  Vector Word Word8
syn keyword smlStructures  Word8Array Word8Vector


  syn match   smlOperator ">=\|=\|<\|>\|<=\|::"
  syn match   smlOperator "quot\|rem\|div\|mod\|=>\||"

  syn match   smlNumber            "\<[0123456789]*\>"
  syn match   smlSpecialCharacter  ";"



endif

syn sync ccomment maxlines=50

if !exists("did_sml_syntax_inits")

  let did_sml_syntax_inits = 1

  " The default methods for highlighting.  Can be overridden later

  hi link smlComment    Comment
  hi link smlCharCode   Special

  if exists ("sml_highlighting_clean")

    hi link smlKeyword          Statement
    hi link smlStatement        Statement
    hi link smlFunction         Statement

  else

    hi link smlStatement        Statement
    hi link smlKeyword          Keyword
    hi link smlFunction         Function
    hi link smlSpecialCharacter Special
    hi link smlNumber           Number
    hi link smlAtom             String
    hi link smlString           String
    hi link smlOperator         Operator
    hi link smlType             Type
    hi link smlStructures	    Type



  endif

endif

let b:current_syntax = "sml"

" vim: ts=28
