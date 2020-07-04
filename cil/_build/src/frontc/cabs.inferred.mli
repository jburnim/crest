type cabsloc = {
  lineno : int;
  filename : string;
  byteno : int;
  ident : int;
}
type typeSpecifier =
    Tvoid
  | Tchar
  | Tbool
  | Tshort
  | Tint
  | Tlong
  | Tint64
  | Tfloat
  | Tdouble
  | Tsigned
  | Tsizet
  | Tunsigned
  | Tnamed of string
  | Tstruct of string * field_group list option * attribute list
  | Tunion of string * field_group list option * attribute list
  | Tenum of string * enum_item list option * attribute list
  | TtypeofE of expression
  | TtypeofT of specifier * decl_type
and storage = NO_STORAGE | AUTO | STATIC | EXTERN | REGISTER
and funspec = INLINE | VIRTUAL | EXPLICIT
and cvspec = CV_CONST | CV_VOLATILE | CV_RESTRICT
and spec_elem =
    SpecTypedef
  | SpecCV of cvspec
  | SpecAttr of attribute
  | SpecStorage of storage
  | SpecInline
  | SpecType of typeSpecifier
  | SpecPattern of string
and specifier = spec_elem list
and decl_type =
    JUSTBASE
  | PARENTYPE of attribute list * decl_type * attribute list
  | ARRAY of decl_type * attribute list * expression
  | PTR of attribute list * decl_type
  | PROTO of decl_type * single_name list * bool
and name_group = specifier * name list
and field_group = specifier * (name * expression option) list
and init_name_group = specifier * init_name list
and name = string * decl_type * attribute list * cabsloc
and init_name = name * init_expression
and single_name = specifier * name
and enum_item = string * expression * cabsloc
and definition =
    FUNDEF of single_name * block * cabsloc * cabsloc
  | DECDEF of init_name_group * cabsloc
  | TYPEDEF of name_group * cabsloc
  | ONLYTYPEDEF of specifier * cabsloc
  | GLOBASM of string * cabsloc
  | PRAGMA of expression * cabsloc
  | LINKAGE of string * cabsloc * definition list
  | TRANSFORMER of definition * definition list * cabsloc
  | EXPRTRANSFORMER of expression * expression * cabsloc
and file = string * definition list
and block = {
  blabels : string list;
  battrs : attribute list;
  bstmts : statement list;
}
and asm_details = {
  aoutputs : (string option * string * expression) list;
  ainputs : (string option * string * expression) list;
  aclobbers : string list;
}
and statement =
    NOP of cabsloc
  | COMPUTATION of expression * cabsloc
  | BLOCK of block * cabsloc
  | SEQUENCE of statement * statement * cabsloc
  | IF of expression * statement * statement * cabsloc
  | WHILE of expression * statement * cabsloc
  | DOWHILE of expression * statement * cabsloc
  | FOR of for_clause * expression * expression * statement * cabsloc
  | BREAK of cabsloc
  | CONTINUE of cabsloc
  | RETURN of expression * cabsloc
  | SWITCH of expression * statement * cabsloc
  | CASE of expression * statement * cabsloc
  | CASERANGE of expression * expression * statement * cabsloc
  | DEFAULT of statement * cabsloc
  | LABEL of string * statement * cabsloc
  | GOTO of string * cabsloc
  | COMPGOTO of expression * cabsloc
  | DEFINITION of definition
  | ASM of attribute list * string list * asm_details option * cabsloc
  | TRY_EXCEPT of block * expression * block * cabsloc
  | TRY_FINALLY of block * block * cabsloc
and for_clause = FC_EXP of expression | FC_DECL of definition
and binary_operator =
    ADD
  | SUB
  | MUL
  | DIV
  | MOD
  | AND
  | OR
  | BAND
  | BOR
  | XOR
  | SHL
  | SHR
  | EQ
  | NE
  | LT
  | GT
  | LE
  | GE
  | ASSIGN
  | ADD_ASSIGN
  | SUB_ASSIGN
  | MUL_ASSIGN
  | DIV_ASSIGN
  | MOD_ASSIGN
  | BAND_ASSIGN
  | BOR_ASSIGN
  | XOR_ASSIGN
  | SHL_ASSIGN
  | SHR_ASSIGN
and unary_operator =
    MINUS
  | PLUS
  | NOT
  | BNOT
  | MEMOF
  | ADDROF
  | PREINCR
  | PREDECR
  | POSINCR
  | POSDECR
and expression =
    NOTHING
  | UNARY of unary_operator * expression
  | LABELADDR of string
  | BINARY of binary_operator * expression * expression
  | QUESTION of expression * expression * expression
  | CAST of (specifier * decl_type) * init_expression
  | CALL of expression * expression list
  | COMMA of expression list
  | CONSTANT of constant
  | PAREN of expression
  | VARIABLE of string
  | EXPR_SIZEOF of expression
  | TYPE_SIZEOF of specifier * decl_type
  | EXPR_ALIGNOF of expression
  | TYPE_ALIGNOF of specifier * decl_type
  | INDEX of expression * expression
  | MEMBEROF of expression * string
  | MEMBEROFPTR of expression * string
  | GNU_BODY of block
  | EXPR_PATTERN of string
and constant =
    CONST_INT of string
  | CONST_FLOAT of string
  | CONST_CHAR of int64 list
  | CONST_WCHAR of int64 list
  | CONST_STRING of string
  | CONST_WSTRING of int64 list
and init_expression =
    NO_INIT
  | SINGLE_INIT of expression
  | COMPOUND_INIT of (initwhat * init_expression) list
and initwhat =
    NEXT_INIT
  | INFIELD_INIT of string * initwhat
  | ATINDEX_INIT of expression * initwhat
  | ATINDEXRANGE_INIT of expression * expression
and attribute = string * expression list
