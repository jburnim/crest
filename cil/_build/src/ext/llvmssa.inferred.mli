module H = Hashtbl
module S = String
val z : int
val llvmSsa :
  Llvmgen.llvmGenerator ->
  Llvmgen.llvmBlock list ->
  Cil.varinfo list -> Cil.varinfo list -> Llvmgen.llvmBlock list
