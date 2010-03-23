" Vim syntax file
" Language:	erlang (ERicsson LANGuage)
"		http://www.erlang.org
"		http://www.ericsson.se/erlang
" Maintainer:	Kre¹imir Mar¾iæ (Kresimir Marzic) <kmarzic@fly.srk.fer.hr>
" Last Change:	Sun, 02-Jan-2000
" URL:		http://fly.srk.fer.hr/~kmarzic/vim/syntax/erlang.vim


" There are two sets of highlighting in here:
" One is "erlang_characters", another "erlang_keywords"
" If you want to disable keywords highlighting, put in your .vimrc:
"	let erlang_keywords=1
" If you want to disable special characters highlighting, put in
" your .vimrc:
"	let erlang_characters=1


" Remove any old syntax stuff hanging around.
syn clear

" erlang is case sensitive.
syn case match


" Very simple highlighting for comments, clause heads and
" character codes. It respects erlang strings and atoms.


if ! exists("erlang_characters")
	syn match    erlangComment   +%.*+
	syn keyword  erlangKeyword   module export
	syn region   erlangString1   start=+"+ skip=+\\"+ end=+"+
	syn region   erlangString2   start=+'+ skip=+\\'+ end=+'+
	" some sybols
	syn match erlangOperator "=/=\|=:=\|=<\|==\|>=\|/=\|<\|>\|="
	syn match erlangNumber "\<[0123456789]*\>"
	syn match erlangSpecialCharacter "\[\|\]\|(\|)\|{\|}\||"
	" syn match erlangSpecialCharacter ";\|,\|\[\|\]\|(\|)\|{\|}\|:\||"
	syn match erlangSpecialCharacterCommand "!\|->\|\."
endif


if ! exists("erlang_keywords")
	" all function
	syn keyword erlangFunction  abs append apply atom_to_list binary
	syn keyword erlangFunction  concat_binary binary_to_list binary_to_term
	syn keyword erlangFunction  concat_binary date element erase exit float
	syn keyword erlangFunction  float_to_list get get_keys group_leader halt
	syn keyword erlangFunction  hash hd integer_to_list length link
	syn keyword erlangFunction  list_to_atom list_to_binary list_to_float
	syn keyword erlangFunction  list_to_integer list_to_pid list_to_touple
	syn keyword erlangFunction  make_ref now open_port pid_to_list
	syn keyword erlangFunction  process_flag process_info processes put
	syn keyword erlangFunction  register registered round self send
	syn keyword erlangFunction  setelement size spawn spawn_link split_binary
	syn keyword erlangFunction  throw time tl trunc tuple_to_list unlink
	syn keyword erlangFunction  unregister whereis
	syn keyword erlangFunction  alive check_process_code delete_module
	syn keyword erlangFunction  disconnect_node get_cookie is_alive
	syn keyword erlangFunction  load_module math module_load monitor_node
	syn keyword erlangFunction  node nodes pre_load purge_module set_cookie
	syn keyword erlangFunction  statistics term_to_binary

	" all keywords
	syn keyword erlangKeyword  end endif else elseif if of after
	syn keyword erlangKeyword  receive when div rem band bor bxor bsl bsr
	syn keyword erlangKeyword  atom constant float integer list number
	syn keyword erlangKeyword  pid port reference tuple binary
	syn keyword erlangKeyword  case case_clause
	syn keyword erlangKeyword  EXIT
endif


if ! exists("did_erlang_syntax_inits")
	let did_erlang_syntax_inits = 1
	hi link erlangComment Comment
	hi link erlangKeyword Keyword
	hi link erlangString1 String
	hi link erlangString2 String
	hi link erlangFunction Function
	hi link erlangSpecialCharacter Special
	hi link erlangNumber Number
	hi link erlangOperator Operator
	hi link erlangSpecialCharacterCommand Keyword
	hi link erlangModules Keyword
endif


let b:current_syntax = "erlang"
