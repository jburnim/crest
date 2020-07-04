exception Eof
exception InternalError of string
module H = Hashtbl
module E = Errormsg
val keywords : (string, Formatparse.token) H.t
val scan_ident : string -> Formatparse.token
val init : prog:string -> Lexing.lexbuf
val finish : unit -> unit
val error : string -> 'a
val scan_escape : string -> string
val get_value : char -> int
val scan_hex_escape : string -> string
val scan_oct_escape : string -> string
val wbtowc : string -> string
val wstr_to_warray : string -> string
val getArgName : Lexing.lexbuf -> int -> string
val __ocaml_lex_tables : Lexing.lex_tables
val initial : Lexing.lexbuf -> Formatparse.token
val __ocaml_lex_initial_rec : Lexing.lexbuf -> int -> Formatparse.token
val comment : Lexing.lexbuf -> unit
val __ocaml_lex_comment_rec : Lexing.lexbuf -> int -> unit
val endline : Lexing.lexbuf -> Formatparse.token
val __ocaml_lex_endline_rec : Lexing.lexbuf -> int -> Formatparse.token
