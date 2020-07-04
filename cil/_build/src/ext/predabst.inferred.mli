module E = Errormsg
module P = Ptranal
module DF = Dataflow
module IH = Inthash
module H = Hashtbl
module U = Util
module S = Stats
val debug : bool ref
val collectPredicates : (Cil.fundec -> Cil.exp list) ref
val ignoreInstruction : (Cil.instr -> bool) ref
val instrHasNoSideEffects : (Cil.instr -> bool) ref
val getPredsFromInstr : (Cil.instr -> Cil.exp list) ref
module ExpIntHash :
  sig
    type key = Cil.exp
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
module type TRANSLATOR =
  sig
    type exp
    type unop = exp -> exp
    type binop = exp -> exp -> exp
    val mkTrue : unit -> exp
    val mkFalse : unit -> exp
    val mkAnd : binop
    val mkOr : binop
    val mkNot : unop
    val mkIte : exp -> exp -> exp -> exp
    val mkImp : binop
    val mkEq : binop
    val mkNe : binop
    val mkLt : binop
    val mkLe : binop
    val mkGt : binop
    val mkGe : binop
    val mkPlus : binop
    val mkTimes : binop
    val mkMinus : binop
    val mkDiv : binop
    val mkMod : binop
    val mkLShift : binop
    val mkRShift : binop
    val mkBAnd : binop
    val mkBXor : binop
    val mkBOr : binop
    val mkNeg : unop
    val mkCompl : unop
    val mkVar : string -> exp
    val mkConst : int -> exp
    val isValid : exp -> bool
  end
module NullTranslator : TRANSLATOR
module type SOLVER =
  sig
    type exp
    val transExp : Cil.exp -> exp
    val isValid : exp -> exp -> bool
  end
module Solver :
  functor (T : TRANSLATOR) ->
    sig
      type exp = T.exp
      exception NYI
      val transUnOp : Cil.unop -> T.exp -> T.exp
      val transBinOp : Cil.binop -> T.exp -> T.exp -> T.exp
      val transExp : Cil.exp -> T.exp
      val isValid : T.exp -> T.exp -> bool
    end
module NullSolver :
  sig
    type exp = NullTranslator.exp
    exception NYI
    val transUnOp : Cil.unop -> NullTranslator.exp -> NullTranslator.exp
    val transBinOp :
      Cil.binop ->
      NullTranslator.exp -> NullTranslator.exp -> NullTranslator.exp
    val transExp : Cil.exp -> NullTranslator.exp
    val isValid : NullTranslator.exp -> NullTranslator.exp -> bool
  end
module PredAbst :
  functor (S : SOLVER) ->
    sig
      type boolLat = True | False | Top | Bottom
      val combineBoolLat : boolLat -> boolLat -> boolLat
      val d_bl : unit -> boolLat -> Pretty.doc
      type funcSig = {
        mutable fsFormals : Cil.varinfo list;
        mutable fsReturn : Cil.varinfo option;
        mutable fsAllPreds : Cil.exp list;
        mutable fsFPPreds : Cil.exp list;
        mutable fsRetPreds : Cil.exp list;
      }
      type stmtState =
          ILState of boolLat IH.t list
        | StmState of boolLat IH.t
      type absState = stmtState IH.t
      type context = {
        mutable cFuncSigs : funcSig IH.t;
        mutable cPredicates : Cil.exp IH.t;
        mutable cRPredMap : int ExpIntHash.t;
        mutable cNextPred : int;
      }
      val emptyContext : unit -> context
      class returnFinderClass :
        Cil.varinfo option ref ->
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
      val findReturn : Cil.fundec -> Cil.varinfo option
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
      val expContainsVi : Cil.exp -> Cil.varinfo -> bool
      class derefFinderClass :
        'a ->
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
      val expContainsDeref : Cil.exp -> 'a -> bool
      class globalFinderClass :
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
      val expContainsGlobal : Cil.exp -> bool
      class aliasFinderClass :
        Cil.exp ->
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
      val expHasAlias : Cil.exp -> Cil.exp -> bool
      val makeFormalPreds : Cil.varinfo list -> Cil.exp list -> Cil.exp list
      val makeReturnPreds :
        Cil.varinfo option ->
        Cil.varinfo list ->
        Cil.varinfo list -> Cil.exp list -> Cil.exp list -> Cil.exp list
      val funSigHash : funcSig IH.t
      class funcSigMakerClass :
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
      val makeFunctionSigs : Cil.file -> funcSig IH.t
      val h_equals : 'a IH.t -> 'a IH.t -> bool
      val hl_equals : 'a IH.t list -> 'a IH.t list -> bool
      val h_combine : boolLat IH.t -> boolLat IH.t -> boolLat IH.t
      val hl_combine :
        boolLat IH.t list -> boolLat IH.t list -> boolLat IH.t list
      val substitute : Cil.exp -> Cil.lval -> Cil.exp -> Cil.exp
      val weakestPrecondition : Cil.instr -> Cil.exp -> Cil.exp option
      val getPred : context -> int -> Cil.exp
      val buildPreAndTest :
        context ->
        boolLat IH.t ->
        boolLat IH.t ->
        Cil.exp list -> bool -> Cil.instr option -> boolLat IH.t
      val handleSetInstr :
        context ->
        Cil.exp list ->
        boolLat IH.t -> Cil.instr -> boolLat IH.t -> boolLat IH.t
      val handleCallInstr :
        context ->
        Cil.exp list ->
        boolLat IH.t -> Cil.instr -> boolLat IH.t -> boolLat IH.t
      val fixForExternCall :
        context ->
        boolLat IH.t -> Cil.lval option -> Cil.exp list -> boolLat IH.t
      val handleIl : context -> Cil.instr list -> stmtState -> stmtState
      val handleStmt : context -> Cil.stmt -> stmtState -> stmtState
      val handleBranch : context -> Cil.exp -> stmtState -> stmtState
      val listInit : int -> 'a -> 'a list
      val currentContext : context
      module PredFlow :
        sig
          val name : string
          val debug : bool ref
          type t = stmtState
          val copy : stmtState -> stmtState
          val stmtStartData : stmtState IH.t
          val pretty : unit -> stmtState -> Pretty.doc
          val computeFirstPredecessor : Cil.stmt -> stmtState -> stmtState
          val combinePredecessors :
            Cil.stmt -> old:t -> t -> stmtState option
          val doInstr : 'a -> 'b -> 'c DF.action
          val doStmt : Cil.stmt -> stmtState -> stmtState DF.stmtaction
          val doGuard : Cil.exp -> stmtState -> stmtState DF.guardaction
          val filterStmt : 'a -> bool
        end
      module PA : sig val compute : Cil.stmt list -> unit end
      val registerFile : Cil.file -> unit
      val makePreds : Cil.exp list -> unit
      val makeAllBottom : Cil.exp IH.t -> boolLat IH.t
      val analyze : Cil.fundec -> unit
      val getPAs : int -> stmtState option
      class paVisitorClass :
        object
          val mutable cur_pa_dat : boolLat IH.t option
          val mutable pa_dat_lst : boolLat IH.t list
          val mutable sid : int
          method get_cur_dat : unit -> boolLat IH.t option
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
      val query : boolLat IH.t -> Cil.exp -> boolLat
    end
