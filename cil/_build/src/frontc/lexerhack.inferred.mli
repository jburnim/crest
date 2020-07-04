module E = Errormsg
val add_identifier : (string -> unit) ref
val add_type : (string -> unit) ref
val push_context : (unit -> unit) ref
val pop_context : (unit -> unit) ref
val currentPattern : string ref
