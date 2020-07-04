module H = Hashtbl
module IH = Inthash
val sliceFile : Cil.file -> string -> int -> unit
val doEpicenter : bool ref
val epicenterName : string ref
val epicenterHops : int ref
val feature : Cil.featureDescr
