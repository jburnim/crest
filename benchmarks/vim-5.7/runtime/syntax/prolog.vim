" Vim syntax file
" Language:    PROLOG
" Maintainers: Ralph Becket <rwab1@cam.sri.co.uk>,
"              Thomas Koehler <jean-luc@picard.franken.de>
" Last Change: 1997 December 28

" There are two sets of highlighting in here:
" If the "prolog_highlighting_clean" variable exists, it is rather sparse.
" Otherwise you get more highlighting.

" Remove any old syntax stuff hanging around.
syn clear

" Prolog is case sensitive.
syn case match

" Very simple highlighting for comments, clause heads and
" character codes.  It respects prolog strings and atoms.

syn region   prologCComment     start=+/\*+ end=+\*/+
syn match    prologComment      +%.*+

syn keyword  prologKeyword      module meta_predicate multifile dynamic
syn match    prologCharCode     +0'\\\=.+
syn region   prologString       start=+"+ skip=+\\"+ end=+"+
syn region   prologAtom         start=+'+ skip=+\\'+ end=+'+
syn region   prologClauseHead   start=+^[a-z][^(]*(+ end=+:-\|\.\|-->+

if !exists("prolog_highlighting_clean")

  " some keywords
  " some common predicates are also highlighted as keywords
  " is there a better solution?
  syn keyword prologKeyword   abolish current_output  peek_code
  syn keyword prologKeyword   append  current_predicate       put_byte
  syn keyword prologKeyword   arg     current_prolog_flag     put_char
  syn keyword prologKeyword   asserta fail    put_code
  syn keyword prologKeyword   assertz findall read
  syn keyword prologKeyword   at_end_of_stream        float   read_term
  syn keyword prologKeyword   atom    flush_output    repeat
  syn keyword prologKeyword   atom_chars      functor retract
  syn keyword prologKeyword   atom_codes      get_byte        set_input
  syn keyword prologKeyword   atom_concat     get_char        set_output
  syn keyword prologKeyword   atom_length     get_code        set_prolog_flag
  syn keyword prologKeyword   atomic  halt    set_stream_position
  syn keyword prologKeyword   bagof   integer setof
  syn keyword prologKeyword   call    is      stream_property
  syn keyword prologKeyword   catch   nl      sub_atom
  syn keyword prologKeyword   char_code       nonvar  throw
  syn keyword prologKeyword   char_conversion number  true
  syn keyword prologKeyword   clause  number_chars    unify_with_occurs_check
  syn keyword prologKeyword   close   number_codes    var
  syn keyword prologKeyword   compound        once    write
  syn keyword prologKeyword   copy_term       op      write_canonical
  syn keyword prologKeyword   current_char_conversion open    write_term
  syn keyword prologKeyword   current_input   peek_byte       writeq
  syn keyword prologKeyword   current_op      peek_char

  syn match   prologOperator "=\\=\|=:=\|\\==\|=<\|==\|>=\|\\=\|\\+\|<\|>\|="
  syn match   prologAsIs     "===\|\\===\|<=\|=>"

  syn match   prologNumber            "\<[0123456789]*\>"
  syn match   prologCommentError      "\*/"
  syn match   prologSpecialCharacter  ";"
  syn match   prologSpecialCharacter  "!"
  syn match   prologQuestion          "?-.*\."  contains=prologNumber


endif

syn sync ccomment maxlines=50

if !exists("did_prolog_syntax_inits")

  let did_prolog_syntax_inits = 1

  " The default methods for highlighting.  Can be overridden later

  hi link prologComment    Comment
  hi link prologCComment   Comment
  hi link prologCharCode   Special

  if exists ("prolog_highlighting_clean")

    hi link prologKeyword      Statement
    hi link prologClauseHead   Statement

  else

    hi link prologKeyword          Keyword
    hi link prologClauseHead       Constant
    hi link prologQuestion         PreProc
    hi link prologSpecialCharacter Special
    hi link prologNumber           Number
    hi link prologAsIs             Normal
    hi link prologCommentError     Error
    hi link prologAtom             String
    hi link prologString           String
    hi link prologOperator         Operator

  endif

endif

let b:current_syntax = "prolog"

" vim: ts=28
