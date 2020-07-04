module H = Hashtbl
module S = String
exception NotConstant
type llvmBlock = {
  lblabel : string;
  mutable lbbody : llvmInstruction list;
  mutable lbterminator : llvmTerminator;
  mutable lbpreds : llvmBlock list;
}
and llvmInstruction = {
  mutable liresult : llvmLocal option;
  liop : llvmOp;
  mutable liargs : llvmValue list;
}
and llvmTerminator =
    TUnreachable
  | TDead
  | TRet of llvmValue list
  | TBranch of llvmBlock
  | TCond of llvmValue * llvmBlock * llvmBlock
  | TSwitch of llvmValue * llvmBlock * (int64 * llvmBlock) list
and llvmValue =
    LGlobal of llvmGlobal
  | LLocal of llvmLocal
  | LBool of bool
  | LInt of int64 * Cil.ikind
  | LFloat of float * Cil.fkind
  | LUndef
  | LZero
  | LNull of llvmType
  | LPhi of llvmValue * llvmBlock
  | LType of llvmType
  | LGetelementptr of llvmValue list
  | LCast of llvmCast * llvmValue * llvmType
  | LBinary of llvmBinop * llvmValue * llvmValue * llvmType
  | LCmp of llvmCmp * llvmValue * llvmValue
  | LFcmp of llvmFCmp * llvmValue * llvmValue
  | LSelect of llvmValue * llvmValue * llvmValue
and llvmLocal = string * llvmType
and llvmGlobal = string * llvmType
and llvmType = Cil.typ
and llvmOp =
    LIassign
  | LIphi
  | LIgetelementptr
  | LIload
  | LIstore
  | LIcall
  | LIalloca
  | LIbinary of llvmBinop
  | LIcmp of llvmCmp
  | LIfcmp of llvmFCmp
  | LIselect
  | LIcast of llvmCast
  | LIva_arg
and llvmBinop =
    LBadd
  | LBsub
  | LBmul
  | LBudiv
  | LBsdiv
  | LBfdiv
  | LBurem
  | LBsrem
  | LBfrem
  | LBshl
  | LBlshr
  | LBashr
  | LBand
  | LBor
  | LBxor
and llvmCmp =
    LCeq
  | LCne
  | LCslt
  | LCult
  | LCsle
  | LCule
  | LCsgt
  | LCugt
  | LCsge
  | LCuge
and llvmFCmp =
    LCFoeq
  | LCFone
  | LCFolt
  | LCFole
  | LCFogt
  | LCFoge
  | LCFord
  | LCFueq
  | LCFune
  | LCFult
  | LCFule
  | LCFugt
  | LCFuge
and llvmCast =
    LAtrunc
  | LAzext
  | LAsext
  | LAuitofp
  | LAsitofp
  | LAfptoui
  | LAfptosi
  | LAfptrunc
  | LAfpext
  | LAinttoptr
  | LAptrtoint
  | LAbitcast
val binopName : llvmBinop -> string
val cmpName : llvmCmp -> string
val fcmpName : llvmFCmp -> string
val castName : llvmCast -> string
val i1Type : Cil.typ
val i32Type : Cil.typ
val i8starType : Cil.typ
val llvmTypeOf : llvmValue -> llvmType
val llvmLocalType : Cil.typ -> bool
val llvmUseLocal : Cil.varinfo -> bool
val llvmDoNotUseLocal : Cil.varinfo -> bool
val llvmDestinations : llvmTerminator -> llvmBlock list
val llvmValueEqual : llvmValue -> llvmValue -> bool
val llocal : Cil.varinfo -> llvmLocal
val lglobal : Cil.varinfo -> llvmGlobal
val lvar : Cil.varinfo -> llvmValue
val lint : int -> Cil.typ -> llvmValue
val lzero : Cil.typ -> llvmValue
val mkIns : llvmOp -> llvmLocal -> llvmValue list -> llvmInstruction
val mkVoidIns : llvmOp -> llvmValue list -> llvmInstruction
val mkTrueIns : llvmLocal -> llvmValue -> llvmInstruction
val llvmEscape : string -> string
val llvmValueNegate : llvmValue -> llvmValue
val llvmCastOp : Cil.typ -> Cil.typ -> llvmCast
class type llvmGenerator =
  object
    method addString : string -> llvmGlobal
    method addWString : int64 list -> llvmGlobal
    method mkConstant : Cil.constant -> llvmValue
    method mkConstantExp : Cil.exp -> llvmValue
    method mkFunction : Cil.fundec -> llvmBlock list
    method printBlocks : unit -> llvmBlock list -> Pretty.doc
    method printGlobals : unit -> Pretty.doc
    method printValue : unit -> llvmValue -> Pretty.doc
    method printValueNoType : unit -> llvmValue -> Pretty.doc
  end
class llvmGeneratorClass : llvmGenerator
