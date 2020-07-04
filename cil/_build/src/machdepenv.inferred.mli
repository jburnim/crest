module R = Str
module L = List
module H = Hashtbl
val preparse : string -> (string, string list) H.t
val errorWrap : string -> (string -> 'a) -> 'a
val getNthString : int -> ('a, 'b list) H.t -> 'a -> 'b
val getNthInt : int -> (string, string list) H.t -> string -> int
val getNthBool : int -> (string, string list) H.t -> string -> bool
val getBool : (string, string list) H.t -> string -> bool
val getInt : (string, string list) H.t -> string -> int
val getSizeof : (string, string list) H.t -> string -> int
val getAlignof : (string, string list) H.t -> string -> int
val respace : string -> string
val modelParse : string -> Machdep.mach
