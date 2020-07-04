exception Unimplemented of string
exception Bug
val getOption : 'a option -> 'a
val isFloatingType : Cil.typ -> bool
val isSignedType : Cil.typ -> bool
val isVaListType : Cil.typ -> bool
val isCompType : Cil.typ -> bool
val defaultPromotion : Cil.typ -> Cil.typ option
val integralKind : Cil.typ -> Cil.ikind
val typeArrayOf : Cil.typ -> Cil.typ
val typePointsTo : Cil.typ -> Cil.typ
val typeOfLhost : Cil.lhost -> Cil.typ
val fieldIndexOf : Cil.fieldinfo -> int
val intConstValue : Cil.exp -> int64
val mkAddrOfExp : Cil.exp -> Cil.exp
val gSig :
  Pretty.doc ->
  Cil.typ * (string * Cil.typ * Cil.attributes) list option * bool *
  Cil.attributes -> Pretty.doc
val gType : Cil.typ -> Pretty.doc
val dgType : unit -> Cil.typ -> Pretty.doc
val gFunctionSig : Cil.varinfo -> Pretty.doc
val dgFunctionSig : unit -> Cil.varinfo -> Pretty.doc
