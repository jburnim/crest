" Vim syntax file
" Language:	CUPL simulation
" Maintainer:	John Cook <john.cook@kla-tencor.com>
" Last Change:	1999 Sep 21

" Remove any old syn stuff hanging around
syn clear

" Read the CUPL syntax to start with
source <sfile>:p:h/cupl.vim

" omit definition-specific stuff
syn clear cuplStatement
syn clear cuplFunction
syn clear cuplLogicalOperator
syn clear cuplArithmeticOperator
syn clear cuplAssignmentOperator
syn clear cuplEqualityOperator
syn clear cuplTruthTableOperator
syn clear cuplExtension

" simulation order statement
syn match  cuplsimOrder "order:" nextgroup=cuplsimOrderSpec skipempty
syn region cuplsimOrderSpec start="." end=";"me=e-1 contains=cuplComment,cuplsimOrderFormat,cuplBitVector,cuplSpecialChar,cuplLogicalOperator,cuplCommaOperator contained

" simulation base statement
syn match   cuplsimBase "base:" nextgroup=cuplsimBaseSpec skipempty
syn region  cuplsimBaseSpec start="." end=";"me=e-1 contains=cuplComment,cuplsimBaseType contained
syn keyword cuplsimBaseType octal decimal hex contained

" simulation vectors statement
syn match cuplsimVectors "vectors:"

" simulator format control
syn match cuplsimOrderFormat "%\d\+\>" contained

" simulator control
syn match cuplsimStimulus "[10ckpx]\+"
syn match cuplsimStimulus +'\(\x\|x\)\+'+
syn match cuplsimOutput "[lhznx*]\+"
syn match cuplsimOutput +"\x\+"+

syn sync minlines=1

" append to the highlighting links in cupl.vim
if !exists("did_cuplsim_syn_inits")
  let did_cuplsim_syn_inits = 1
  " The default methods for hiing.  Can be overridden later
  hi link cuplsimOrder	  cuplStatement
  hi link cuplsimBase	  cuplStatement
  hi link cuplsimBaseType cuplStatement
  hi link cuplsimVectors  cuplStatement
  hi link cuplsimStimulus    cuplNumber
  hi link cuplsimOutput      cuplNumber
  hi link cuplsimOrderFormat cuplNumber
endif

let b:current_syntax = "cuplsim"
" vim:ts=8
