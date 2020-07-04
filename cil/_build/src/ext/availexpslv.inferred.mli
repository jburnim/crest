module E = Errormsg
module DF = Dataflow
module UD = Usedef
module IH = Inthash
module H = Hashtbl
module U = Util
module S = Stats
val debug : bool ref
val doTime : bool ref
val time : string -> ('a -> 'b) -> 'a -> 'b
val ignore_inst : (Cil.instr -> bool) ref
val ignore_call : (Cil.instr -> bool) ref
val registerIgnoreInst : (Cil.instr -> bool) -> unit
val registerIgnoreCall : (Cil.instr -> bool) -> unit
module LvExpHash :
  sig
    type key = Cil.lval
    type 'a t
    val create : int -> 'a t
    val clear : 'a t -> unit
    val reset : 'a t -> unit
    val copy : 'a t -> 'a t
    val add : 'a t -> key -> 'a -> unit
    val remove : 'a t -> key -> unit
    val find : 'a t -> key -> 'a
    val find_opt : 'a t -> key -> 'a option
    val find_all : 'a t -> key -> 'a list
    val replace : 'a t -> key -> 'a -> unit
    val mem : 'a t -> key -> bool
    val iter : (key -> 'a -> unit) -> 'a t -> unit
    val filter_map_inplace : (key -> 'a -> 'a option) -> 'a t -> unit
    val fold : (key -> 'a -> 'b -> 'b) -> 'a t -> 'b -> 'b
    val length : 'a t -> int
    val stats : 'a t -> Hashtbl.statistics
  end
val lvh_equals : Cil.exp LvExpHash.t -> Cil.exp LvExpHash.t -> bool
val lvh_pretty : unit -> Cil.exp LvExpHash.t -> Pretty.doc
val lvh_combine :
  Cil.exp LvExpHash.t -> Cil.exp LvExpHash.t -> Cil.exp LvExpHash.t
class memReadOrAddrOfFinderClass :
  bool ref ->
  object
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
val exp_has_mem_read : Cil.exp -> bool
val lval_has_mem_read : Cil.lval -> bool
val offset_has_mem_read : Cil.offset -> bool
val lvh_kill_mem : Cil.exp LvExpHash.t -> unit
class viFinderClass :
  Cil.varinfo ->
  bool ref ->
  object
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
val instr_has_vi : Cil.varinfo -> Cil.instr -> bool
val exp_has_vi : Cil.varinfo -> Cil.exp -> bool
val lval_has_vi : Cil.varinfo -> Cil.lval -> bool
val lvh_kill_vi : Cil.exp LvExpHash.t -> Cil.varinfo -> unit
class lvalFinderClass :
  Cil.lval ->
  bool ref ->
  object
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
val exp_has_lval : Cil.lval -> Cil.exp -> bool
val lval_has_lval : Cil.lval -> Cil.lhost * Cil.offset -> bool
val lvh_kill_lval : Cil.exp LvExpHash.t -> Cil.lval -> unit
class volatileFinderClass :
  bool ref ->
  object
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
val exp_is_volatile : Cil.exp -> bool
class addrOfOrGlobalFinderClass :
  bool ref ->
  object
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
val exp_has_addrof_or_global : Cil.exp -> bool
val lval_has_addrof_or_global : Cil.lval -> bool
val lvh_kill_addrof_or_global : 'a LvExpHash.t -> unit
val lvh_handle_inst : Cil.instr -> Cil.exp LvExpHash.t -> Cil.exp LvExpHash.t
module AvailableExps :
  sig
    val name : string
    val debug : bool ref
    type t = Cil.exp LvExpHash.t
    val copy : 'a LvExpHash.t -> 'a LvExpHash.t
    val stmtStartData : Cil.exp LvExpHash.t IH.t
    val pretty : unit -> Cil.exp LvExpHash.t -> Pretty.doc
    val computeFirstPredecessor : 'a -> 'b -> 'b
    val combinePredecessors :
      Cil.stmt -> old:t -> t -> Cil.exp LvExpHash.t option
    val doInstr : Cil.instr -> 'a -> Cil.exp LvExpHash.t DF.action
    val doStmt : 'a -> 'b -> 'c DF.stmtaction
    val doGuard : 'a -> 'b -> 'c DF.guardaction
    val filterStmt : 'a -> bool
  end
module AE : sig val compute : Cil.stmt list -> unit end
val computeAEs : Cil.fundec -> unit
val getAEs : int -> Cil.exp LvExpHash.t option
val instrAEs :
  Cil.instr list ->
  'a -> Cil.exp LvExpHash.t -> 'b -> Cil.exp LvExpHash.t list
class aeVisitorClass :
  object
    val mutable ae_dat_lst : Cil.exp LvExpHash.t list
    val mutable cur_ae_dat : Cil.exp LvExpHash.t option
    val mutable sid : int
    method get_cur_eh : unit -> Cil.exp LvExpHash.t option
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
