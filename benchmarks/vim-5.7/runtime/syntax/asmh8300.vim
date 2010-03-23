" Vim syntax file
" Language:	Hitachi H-8300h specific syntax for GNU Assembler
" Maintainer:	Kevin Dahlhausen <ap096@po.cwru.edu>
" Last Change:	1997 April 20

" Remove any old syntax stuff hanging around
syn clear

syn case ignore

syn match asmDirective "\.h8300[h]*"

"h8300[h] registers
syn match asmReg	"e\=r[0-7][lh]\="

"h8300[h] opcodes - order is important!
syn match asmOpcode "add\.[lbw]"
syn match asmOpcode "add[sx :]"
syn match asmOpcode "and\.[lbw]"
syn match asmOpcode "bl[deots]"
syn match asmOpcode "cmp\.[lbw]"
syn match asmOpcode "dec\.[lbw]"
syn match asmOpcode "divx[us].[bw]"
syn match asmOpcode "ext[su]\.[lw]"
syn match asmOpcode "inc\.[lw]"
syn match asmOpcode "mov\.[lbw]"
syn match asmOpcode "mulx[su]\.[bw]"
syn match asmOpcode "neg\.[lbw]"
syn match asmOpcode "not\.[lbw]"
syn match asmOpcode "or\.[lbw]"
syn match asmOpcode "pop\.[wl]"
syn match asmOpcode "push\.[wl]"
syn match asmOpcode "rotx\=[lr]\.[lbw]"
syn match asmOpcode "sha[lr]\.[lbw]"
syn match asmOpcode "shl[lr]\.[lbw]"
syn match asmOpcode "sub\.[lbw]"
syn match asmOpcode "xor\.[lbw]"
syn keyword asmOpcode "andc" "band" "bcc" "bclr" "bcs" "beq" "bf" "bge" "bgt"
syn keyword asmOpcode "bhi" "bhs" "biand" "bild" "bior" "bist" "bixor" "bmi"
syn keyword asmOpcode "bne" "bnot" "bnp" "bor" "bpl" "bpt" "bra" "brn" "bset"
syn keyword asmOpcode "bsr" "btst" "bst" "bt" "bvc" "bvs" "bxor" "cmp" "daa"
syn keyword asmOpcode "das" "eepmov" "eepmovw" "inc" "jmp" "jsr" "ldc" "movfpe"
syn keyword asmOpcode "movtpe" "mov" "nop" "orc" "rte" "rts" "sleep" "stc"
syn keyword asmOpcode "sub" "trapa" "xorc"

syn case match


" Read the general asm syntax
source <sfile>:p:h/asm.vim


if !exists("did_hitachi_syntax_inits")
  let did_hitachi_syntax_inits = 1

  hi link asmOpcode  Statement
  hi link asmRegister  Identifier

  " My default-color overrides:
  hi asmOpcode ctermfg=yellow
  hi asmReg	ctermfg=lightmagenta

endif

let b:current_syntax = "asmh8300"

" vim: ts=8
