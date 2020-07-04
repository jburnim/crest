module E = Errormsg
module VS :
  sig
    type elt = Cil.varinfo
    type t
    val empty : t
    val is_empty : t -> bool
    val mem : elt -> t -> bool
    val add : elt -> t -> t
    val singleton : elt -> t
    val remove : elt -> t -> t
    val union : t -> t -> t
    val inter : t -> t -> t
    val diff : t -> t -> t
    val compare : t -> t -> int
    val equal : t -> t -> bool
    val subset : t -> t -> bool
    val iter : (elt -> unit) -> t -> unit
    val map : (elt -> elt) -> t -> t
    val fold : (elt -> 'a -> 'a) -> t -> 'a -> 'a
    val for_all : (elt -> bool) -> t -> bool
    val exists : (elt -> bool) -> t -> bool
    val filter : (elt -> bool) -> t -> t
    val partition : (elt -> bool) -> t -> t * t
    val cardinal : t -> int
    val elements : t -> elt list
    val min_elt : t -> elt
    val min_elt_opt : t -> elt option
    val max_elt : t -> elt
    val max_elt_opt : t -> elt option
    val choose : t -> elt
    val choose_opt : t -> elt option
    val split : elt -> t -> t * bool * t
    val find : elt -> t -> elt
    val find_opt : elt -> t -> elt option
    val find_first : (elt -> bool) -> t -> elt
    val find_first_opt : (elt -> bool) -> t -> elt option
    val find_last : (elt -> bool) -> t -> elt
    val find_last_opt : (elt -> bool) -> t -> elt option
    val of_list : elt list -> t
  end
val getUseDefFunctionRef :
  (Cil.exp -> Cil.exp list -> VS.t * VS.t * Cil.exp list) ref
val considerVariableUse : (Cil.varinfo -> bool) ref
val considerVariableDef : (Cil.varinfo -> bool) ref
val considerVariableAddrOfAsUse : (Cil.varinfo -> bool) ref
val considerVariableAddrOfAsDef : (Cil.varinfo -> bool) ref
val extraUsesOfExpr : (Cil.exp -> VS.t) ref
val onlyNoOffsetsAreDefs : bool ref
val ignoreSizeof : bool ref
val varUsed : VS.t ref
val varDefs : VS.t ref
class useDefVisitorClass : Cil.cilVisitor
val useDefVisitor : useDefVisitorClass
val computeUseExp : ?acc:VS.t -> Cil.exp -> VS.t
val computeUseDefInstr :
  ?acc_used:VS.t -> ?acc_defs:VS.t -> Cil.instr -> VS.t * VS.t
val computeUseDefStmtKind :
  ?acc_used:VS.t -> ?acc_defs:VS.t -> Cil.stmtkind -> VS.t * VS.t
val computeDeepUseDefStmtKind :
  ?acc_used:VS.t -> ?acc_defs:VS.t -> Cil.stmtkind -> VS.t * VS.t
val computeUseLocalTypes : ?acc_used:VS.t -> Cil.fundec -> VS.t
