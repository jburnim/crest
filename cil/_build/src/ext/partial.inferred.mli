module type AliasInfo =
  sig
    val setup : Cil.file -> unit
    val can_have_the_same_value : Cil.exp -> Cil.exp -> bool
    val resolve_function_pointer : Cil.exp -> Cil.fundec list
  end
module type Symex =
  sig
    type t
    val empty : t
    val equal : t -> t -> bool
    val assign : t -> Cil.lval -> Cil.exp -> Cil.exp * t
    val unassign : t -> Cil.lval -> t
    val assembly : t -> Cil.instr -> t
    val assume : t -> Cil.exp -> t
    val evaluate : t -> Cil.exp -> Cil.exp
    val join : t list -> t
    val call : t -> Cil.fundec -> Cil.exp list -> Cil.exp list * t
    val return : t -> Cil.fundec -> t
    val call_to_unknown_function : t -> t
    val debug : t -> unit
  end
type callGraphNode = {
  fd : Cil.fundec;
  mutable calledBy : Cil.fundec list;
  mutable calls : Cil.fundec list;
}
type callNodeHash = (Cil.varinfo, callGraphNode) Hashtbl.t
module type CallGraph =
  sig
    val compute : Cil.file -> callNodeHash
    val can_call : callNodeHash -> Cil.fundec -> Cil.fundec list
    val can_be_called_by : callNodeHash -> Cil.fundec -> Cil.fundec list
    val fundec_of_varinfo : callNodeHash -> Cil.varinfo -> Cil.fundec
  end
module type CallGraph' =
  sig
    type t
    val compute : Cil.file -> t
    val can_call : t -> Cil.fundec -> Cil.fundec list
    val can_be_called_by : t -> Cil.fundec -> Cil.fundec list
    val fundec_of_varinfo : t -> Cil.varinfo -> Cil.fundec
  end
module EasyAlias : AliasInfo
module PtranalAlias : AliasInfo
module EasyCallGraph :
  functor (A : AliasInfo) ->
    sig
      val cgCreateNode :
        (Cil.varinfo, callGraphNode) Hashtbl.t -> Cil.fundec -> unit
      val cgFindNode : ('a, 'b) Hashtbl.t -> 'a -> 'b
      val cgAddEdge : ('a, callGraphNode) Hashtbl.t -> 'a -> 'a -> unit
      class callGraphVisitor :
        (Cil.varinfo, callGraphNode) Hashtbl.t ->
        object
          val the_fun : Cil.varinfo option ref
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
      val compute : Cil.file -> (Cil.varinfo, callGraphNode) Hashtbl.t
      val can_call :
        (Cil.varinfo, callGraphNode) Hashtbl.t ->
        Cil.fundec -> Cil.fundec list
      val can_be_called_by :
        (Cil.varinfo, callGraphNode) Hashtbl.t ->
        Cil.fundec -> Cil.fundec list
      val fundec_of_varinfo :
        ('a, callGraphNode) Hashtbl.t -> 'a -> Cil.fundec
    end
module NeculaFolding :
  functor (A : AliasInfo) ->
    sig
      module IntMap :
        sig
          type key = int
          type +'a t
          val empty : 'a t
          val is_empty : 'a t -> bool
          val mem : key -> 'a t -> bool
          val add : key -> 'a -> 'a t -> 'a t
          val singleton : key -> 'a -> 'a t
          val remove : key -> 'a t -> 'a t
          val merge :
            (key -> 'a option -> 'b option -> 'c option) ->
            'a t -> 'b t -> 'c t
          val union : (key -> 'a -> 'a -> 'a option) -> 'a t -> 'a t -> 'a t
          val compare : ('a -> 'a -> int) -> 'a t -> 'a t -> int
          val equal : ('a -> 'a -> bool) -> 'a t -> 'a t -> bool
          val iter : (key -> 'a -> unit) -> 'a t -> unit
          val fold : (key -> 'a -> 'b -> 'b) -> 'a t -> 'b -> 'b
          val for_all : (key -> 'a -> bool) -> 'a t -> bool
          val exists : (key -> 'a -> bool) -> 'a t -> bool
          val filter : (key -> 'a -> bool) -> 'a t -> 'a t
          val partition : (key -> 'a -> bool) -> 'a t -> 'a t * 'a t
          val cardinal : 'a t -> int
          val bindings : 'a t -> (key * 'a) list
          val min_binding : 'a t -> key * 'a
          val min_binding_opt : 'a t -> (key * 'a) option
          val max_binding : 'a t -> key * 'a
          val max_binding_opt : 'a t -> (key * 'a) option
          val choose : 'a t -> key * 'a
          val choose_opt : 'a t -> (key * 'a) option
          val split : key -> 'a t -> 'a t * 'a option * 'a t
          val find : key -> 'a t -> 'a
          val find_opt : key -> 'a t -> 'a option
          val find_first : (key -> bool) -> 'a t -> key * 'a
          val find_first_opt : (key -> bool) -> 'a t -> (key * 'a) option
          val find_last : (key -> bool) -> 'a t -> key * 'a
          val find_last_opt : (key -> bool) -> 'a t -> (key * 'a) option
          val map : ('a -> 'b) -> 'a t -> 'b t
          val mapi : (key -> 'a -> 'b) -> 'a t -> 'b t
        end
      type reg = { rvi : Cil.varinfo; rval : Cil.exp; rmem : bool; }
      type t = reg IntMap.t
      val empty : 'a IntMap.t
      val equal : 'a -> 'a -> bool
      val dependsOnMem : bool ref
      class rewriteExpClass :
        t ->
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
      val rewriteExp : t -> Cil.exp -> Cil.exp * bool
      val eval : t -> Cil.exp -> Cil.exp
      val setMemory : reg IntMap.t -> reg IntMap.t
      val setRegister :
        reg IntMap.t -> Cil.varinfo -> Cil.exp * bool -> reg IntMap.t
      val resetRegister : 'a IntMap.t -> int -> 'a IntMap.t
      class findLval :
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
      val removeMappingsThatDependOn :
        reg IntMap.t -> Cil.lval -> reg IntMap.t
      val assign : t -> Cil.lval -> Cil.exp -> Cil.exp * t
      val unassign : reg IntMap.t -> Cil.lval -> reg IntMap.t
      val assembly : 'a -> 'b -> 'a
      val assume : 'a -> 'b -> 'a
      val evaluate : t -> Cil.exp -> Cil.exp
      val join2 : t -> t -> reg IntMap.t
      val join : t list -> t
      val call : t -> Cil.fundec -> Cil.exp list -> Cil.exp list * t
      val return : reg IntMap.t -> Cil.fundec -> reg IntMap.t
      val call_to_unknown_function : reg IntMap.t -> reg IntMap.t
      val debug : reg IntMap.t -> unit
    end
val contains_call : Cil.instr list -> bool
class callBBVisitor :
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
val calls_end_basic_blocks : Cil.file -> unit
class vidVisitor :
  object
    val count : int ref
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
val globally_unique_vids : Cil.file -> unit
module MakePartial :
  functor (S : Symex) (C : CallGraph) (A : AliasInfo) ->
    sig
      val debug : bool
      module LabelSet :
        sig
          type elt = Cil.label
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
      type sinfo = {
        incoming_state : (int, S.t) Hashtbl.t;
        reachable_preds : (int, bool) Hashtbl.t;
        mutable last_used_state : S.t option;
        mutable priority : int;
      }
      val sinfo_ht : (int, sinfo) Hashtbl.t
      val clear_sinfo : unit -> unit
      val get_sinfo : Cil.stmt -> sinfo
      val toposort_counter : int ref
      val add_edge : Cil.stmt -> Cil.stmt -> unit
      val toposort : callNodeHash -> Cil.stmt -> unit
      val changed_cfg : bool ref
      val partial_stmt :
        callNodeHash ->
        S.t -> Cil.stmt -> (Cil.stmt -> Cil.stmt -> S.t -> 'a) -> S.t
      val dataflow : Cil.file -> callNodeHash -> S.t -> Cil.stmt -> unit
      val simplify :
        Cil.file ->
        callNodeHash -> Cil.fundec -> (Cil.lval * Cil.exp) list -> unit
    end
module PartialAlgorithm :
  sig
    val use_ptranal_alias : bool ref
    val setup_alias_analysis : Cil.file -> unit
    val compute_callgraph : Cil.file -> callNodeHash
    val simplify :
      Cil.file ->
      callNodeHash -> Cil.fundec -> (Cil.lval * Cil.exp) list -> unit
  end
val partial : Cil.file -> string -> (Cil.lval * Cil.exp) list -> unit
class globalConstVisitor :
  object
    val mutable init_const : (Cil.lval * Cil.exp) list
    method get_initialized_constants : (Cil.lval * Cil.exp) list
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
val initialized_constants : bool ref
val root_fun : string ref
val do_feature_partial : Cil.file -> unit
val feature : Cil.featureDescr
