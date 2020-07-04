module E = Errormsg
module DF = Dataflow
module UD = Usedef
module L = Liveness
module IH = Inthash
module U = Util
module S = Stats
val debug_fn : string ref
val doTime : bool ref
val time : string -> ('a -> 'b) -> 'a -> 'b
module IOS :
  sig
    type elt = int option
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
val debug : bool ref
val ih_inter : 'a IH.t -> 'b IH.t -> 'a IH.t
val ih_union : 'a IH.t -> 'a IH.t -> 'a IH.t
val iosh_singleton_lookup : IOS.t IH.t -> Cil.varinfo -> IOS.elt
val iosh_lookup : 'a IH.t -> Cil.varinfo -> 'a option
val iosh_defId_find : IOS.t IH.t -> int -> int option
val iosh_combine : IOS.t IH.t -> IOS.t IH.t -> IOS.t IH.t
val iosh_equals : IOS.t IH.t -> IOS.t IH.t -> bool
val iosh_replace : IOS.t IH.t -> int -> Cil.varinfo -> unit
val iosh_filter_dead : 'a -> 'b -> 'a
val proc_defs : UD.VS.t -> IOS.t IH.t -> (unit -> int) -> unit
val idMaker : unit -> int -> unit -> int
val iRDsHtbl : (int * bool, (unit * int * IOS.t IH.t) list) Hashtbl.t
val instrRDs :
  Cil.instr list ->
  int -> 'a * int * IOS.t IH.t -> bool -> (unit * int * IOS.t IH.t) list
type rhs = RDExp of Cil.exp | RDCall of Cil.instr
val rhsHtbl : (rhs * int * IOS.t IH.t) option IH.t
val getDefRhs :
  Cil.stmt IH.t ->
  ('a * int * IOS.t IH.t) IH.t -> int -> (rhs * int * IOS.t IH.t) option
val prettyprint :
  Cil.stmt IH.t ->
  ('a * int * IOS.t IH.t) IH.t -> unit -> 'b * 'c * IOS.t IH.t -> Pretty.doc
module ReachingDef :
  sig
    val name : string
    val debug : bool ref
    val mayReach : bool ref
    type t = unit * int * IOS.t IH.t
    val copy : 'a * 'b * 'c IH.t -> unit * 'b * 'c IH.t
    val stmtStartData : (unit * int * IOS.t IH.t) IH.t
    val defIdStmtHash : Cil.stmt IH.t
    val sidStmtHash : Cil.stmt IH.t
    val pretty : unit -> unit * int * IOS.t IH.t -> Pretty.doc
    val nextDefId : int ref
    val num_defs : Cil.stmt -> int
    val computeFirstPredecessor :
      Cil.stmt -> 'a * int * 'b IH.t -> unit * int * 'b IH.t
    val combinePredecessors :
      Cil.stmt -> old:t -> t -> (unit * int * IOS.t IH.t) option
    val doInstr :
      Cil.instr -> 'a * 'b * 'c -> (unit * int * IOS.t IH.t) DF.action
    val doStmt : Cil.stmt -> 'a * 'b * 'c -> (unit * 'b * 'c) DF.stmtaction
    val doGuard : 'a -> 'b -> 'c DF.guardaction
    val filterStmt : 'a -> bool
  end
module RD : sig val compute : Cil.stmt list -> unit end
val iosh_none_fill : IOS.t IH.t -> Cil.varinfo list -> unit
val clearMemos : unit -> unit
val computeRDs : Cil.fundec -> unit
val getRDs : int -> (unit * int * IOS.t IH.t) option
val getDefIdStmt : int -> Cil.stmt option
val getStmt : int -> Cil.stmt option
val getSimpRhs : int -> rhs option
val isDefInstr : Cil.instr -> int -> bool
val ppFdec : Cil.fundec -> Pretty.doc
class rdVisitorClass :
  object
    val mutable cur_rd_dat : (unit * int * IOS.t IH.t) option
    val mutable rd_dat_lst : (unit * int * IOS.t IH.t) list
    val mutable sid : int
    method get_cur_iosh : unit -> IOS.t IH.t option
    method queueInstr : Cil.instr list -> unit
    method unqueueInstr : unit -> Cil.instr list
    method vattr : Cil.attribute -> Cil.attribute list Cil.visitAction
    method vattrparam : Cil.attrparam -> Cil.attrparam Cil.visitAction
    method vblock : Cil.block -> Cil.block Cil.visitAction
    method vexpr : Cil.exp -> Cil.exp Cil.visitAction
    method vfunc : Cil.fundec -> Cil.fundec Cil.visitAction
    method vglob : Cil.global -> Cil.global list Cil.visitAction
    method vinit :
      Cil.varinfo -> Cil.offset -> Cil.init -> Cil.init Cil.visitAction
    method vinitoffs : Cil.offset -> Cil.offset Cil.visitAction
    method vinst : Cil.instr -> Cil.instr list Cil.visitAction
    method vlval : Cil.lval -> Cil.lval Cil.visitAction
    method voffs : Cil.offset -> Cil.offset Cil.visitAction
    method vstmt : Cil.stmt -> Cil.stmt Cil.visitAction
    method vtype : Cil.typ -> Cil.typ Cil.visitAction
    method vvdec : Cil.varinfo -> Cil.varinfo Cil.visitAction
    method vvrbl : Cil.varinfo -> Cil.varinfo Cil.visitAction
  end
