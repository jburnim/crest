module E = Errormsg
val isConstType : Cil.typ -> bool
val compareExp : Cil.exp -> Cil.exp -> bool
val compareLval : Cil.lval -> Cil.lval -> bool
val stripNopCasts : Cil.exp -> Cil.exp
val compareExpStripCasts : Cil.exp -> Cil.exp -> bool
val stripCastsForPtrArith : Cil.exp -> Cil.exp
val compareTypes :
  ?ignoreSign:bool ->
  ?importantAttr:(Cil.attribute -> bool) -> Cil.typ -> Cil.typ -> bool
val compareTypesNoAttributes : ?ignoreSign:bool -> Cil.typ -> Cil.typ -> bool
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
val isTypeVolatile : Cil.typ -> bool
val stripCastsDeepForPtrArith : Cil.exp -> Cil.exp
val stripCastsForPtrArithLval : Cil.lval -> Cil.lval
val stripCastsForPtrArithOff : Cil.offset -> Cil.offset
val compareExpDeepStripCasts : Cil.exp -> Cil.exp -> bool
val compareAttrParam : Cil.attrparam -> Cil.attrparam -> bool
