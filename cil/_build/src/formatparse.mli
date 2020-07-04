type token =
  | IDENT of (string)
  | CST_CHAR of (string)
  | CST_INT of (string)
  | CST_FLOAT of (string)
  | CST_STRING of (string)
  | CST_WSTRING of (string)
  | NAMED_TYPE of (string)
  | EOF
  | CHAR
  | INT
  | DOUBLE
  | FLOAT
  | VOID
  | INT64
  | INT32
  | ENUM
  | STRUCT
  | TYPEDEF
  | UNION
  | SIGNED
  | UNSIGNED
  | LONG
  | SHORT
  | VOLATILE
  | EXTERN
  | STATIC
  | CONST
  | RESTRICT
  | AUTO
  | REGISTER
  | ARG_e of (string)
  | ARG_eo of (string)
  | ARG_E of (string)
  | ARG_u of (string)
  | ARG_b of (string)
  | ARG_t of (string)
  | ARG_d of (string)
  | ARG_lo of (string)
  | ARG_l of (string)
  | ARG_i of (string)
  | ARG_o of (string)
  | ARG_va of (string)
  | ARG_f of (string)
  | ARG_F of (string)
  | ARG_A of (string)
  | ARG_v of (string)
  | ARG_k of (string)
  | ARG_c of (string)
  | ARG_s of (string)
  | ARG_p of (string)
  | ARG_P of (string)
  | ARG_I of (string)
  | ARG_S of (string)
  | ARG_g of (string)
  | SIZEOF
  | ALIGNOF
  | EQ
  | ARROW
  | DOT
  | EQ_EQ
  | EXCLAM_EQ
  | INF
  | SUP
  | INF_EQ
  | SUP_EQ
  | MINUS_EQ
  | PLUS_EQ
  | STAR_EQ
  | PLUS
  | MINUS
  | STAR
  | SLASH
  | PERCENT
  | TILDE
  | AND
  | PIPE
  | CIRC
  | EXCLAM
  | AND_AND
  | PIPE_PIPE
  | INF_INF
  | SUP_SUP
  | PLUS_PLUS
  | MINUS_MINUS
  | RPAREN
  | LPAREN
  | RBRACE
  | LBRACE
  | LBRACKET
  | RBRACKET
  | COLON
  | SEMICOLON
  | COMMA
  | ELLIPSIS
  | QUEST
  | BREAK
  | CONTINUE
  | GOTO
  | RETURN
  | SWITCH
  | CASE
  | DEFAULT
  | WHILE
  | DO
  | FOR
  | IF
  | THEN
  | ELSE
  | SLASH_EQ
  | PERCENT_EQ
  | AND_EQ
  | PIPE_EQ
  | CIRC_EQ
  | INF_INF_EQ
  | SUP_SUP_EQ
  | ATTRIBUTE
  | INLINE
  | ASM
  | TYPEOF
  | FUNCTION__
  | PRETTY_FUNCTION__
  | LABEL__
  | BUILTIN_VA_ARG
  | BUILTIN_VA_LIST
  | BLOCKATTRIBUTE
  | DECLSPEC
  | MSASM of (string)
  | MSATTR of (string)
  | PRAGMA

val initialize :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> unit
val expression :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> ((string * Cil.formatArg) list -> Cil.exp) * (Cil.exp -> Cil.formatArg list option)
val typename :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> ((string * Cil.formatArg) list -> Cil.typ) * (Cil.typ -> Cil.formatArg list option)
val offset :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> (Cil.typ -> (string * Cil.formatArg) list -> Cil.offset) * (Cil.offset -> Cil.formatArg list option)
val lval :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> ((string * Cil.formatArg) list -> Cil.lval) * (Cil.lval -> Cil.formatArg list option)
val instr :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> (Cil.location -> (string * Cil.formatArg) list -> Cil.instr) * (Cil.instr -> Cil.formatArg list option)
val stmt :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> ((string -> Cil.typ -> Cil.varinfo) -> Cil.location -> (string * Cil.formatArg) list -> Cil.stmt)
val stmt_list :
  (Lexing.lexbuf  -> token) -> Lexing.lexbuf -> ((string -> Cil.typ -> Cil.varinfo) -> Cil.location -> (string * Cil.formatArg) list -> Cil.stmt list)
