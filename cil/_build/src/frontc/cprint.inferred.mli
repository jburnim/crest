val version : string
type loc = { line : int; file : string; }
val lu : loc
val cabslu : Cabs.cabsloc
val curLoc : Cabs.cabsloc ref
val msvcMode : bool ref
val printLn : bool ref
val printLnComment : bool ref
val printCounters : bool ref
val printComments : bool ref
val width : int ref
val tab : int ref
val max_indent : int ref
val line : string ref
val line_len : int ref
val current : string ref
val current_len : int ref
val spaces : int ref
val follow : int ref
val roll : int ref
val new_line : unit -> unit
val space : unit -> unit
val indent : unit -> unit
val unindent : unit -> unit
val force_new_line : unit -> unit
val flush : unit -> unit
val commit : unit -> unit
val print_unescaped_string : string -> unit
val print_list : (unit -> unit) -> ('a -> 'b) -> 'a list -> unit
val print_commas : bool -> ('a -> 'b) -> 'a list -> unit
val print_string : string -> unit
val print_wstring : int64 list -> unit
val print_specifiers : Cabs.specifier -> unit
val print_type_spec : Cabs.typeSpecifier -> unit
val print_struct_name_attr : string -> string -> Cabs.attribute list -> unit
val print_decl : string -> Cabs.decl_type -> unit
val print_fields : Cabs.field_group list -> unit
val print_enum_items : Cabs.enum_item list -> unit
val print_onlytype : Cabs.specifier * Cabs.decl_type -> unit
val print_name : Cabs.name -> unit
val print_init_name : Cabs.init_name -> unit
val print_name_group : Cabs.name_group -> unit
val print_field_group : Cabs.field_group -> unit
val print_field : Cabs.name * Cabs.expression option -> unit
val print_init_name_group : Cabs.init_name_group -> unit
val print_single_name : Cabs.single_name -> unit
val print_params : Cabs.single_name list -> bool -> unit
val print_old_params : string list -> bool -> unit
val get_operator : Cabs.expression -> string * int
val print_comma_exps : Cabs.expression list -> unit
val print_init_expression : Cabs.init_expression -> unit
val print_expression : Cabs.expression -> unit
val print_expression_level : int -> Cabs.expression -> unit
val print_statement : Cabs.statement -> unit
val print_block : Cabs.block -> unit
val print_substatement : Cabs.statement -> unit
val print_attribute : Cabs.attribute -> unit
val print_attributes : Cabs.attribute list -> unit
val print_defs : Cabs.definition list -> unit
val print_def : Cabs.definition -> unit
val comprint : string -> unit
val comstring : string -> string
val printFile : out_channel -> Cabs.file -> unit
val set_tab : int -> unit
val set_width : int -> unit
