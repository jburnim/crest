module E = Errormsg
module H = Hashtbl
val doSfi : bool ref
val doSfiReads : bool ref
val doSfiWrites : bool ref
val skipFunctions : (string, unit) H.t
val mustSfiFunction : Cil.fundec -> bool
type dataLocation =
    InResult
  | InArg of int
  | InArgTimesArg of int * int
  | PointedToByArg of int
val extractData : dataLocation -> Cil.exp list -> Cil.lval option -> Cil.exp
val allocators : (string, dataLocation * dataLocation) H.t
val deallocators : (string, dataLocation) H.t
val is_bitfield : Cil.offset -> bool
val addr_of_lv : Cil.lval -> Cil.exp
val mustLogLval : bool -> Cil.lval -> bool
val mkProto :
  string -> (string * Cil.typ * Cil.attributes) list -> Cil.fundec
val logReads : Cil.fundec
val callLogRead : Cil.lval -> Cil.instr
val logWrites : Cil.fundec
val callLogWrite : Cil.lval -> Cil.instr
val logStackFrame : Cil.fundec
val callLogStack : string -> Cil.instr
val logAlloc : Cil.fundec
val callLogAlloc :
  dataLocation ->
  dataLocation -> Cil.exp list -> Cil.lval option -> Cil.instr
val logFree : Cil.fundec
val callLogFree :
  dataLocation -> Cil.exp list -> Cil.lval option -> Cil.instr
class sfiVisitorClass : Cil.cilVisitor
val doit : Cil.file -> unit
val feature : Cil.featureDescr
