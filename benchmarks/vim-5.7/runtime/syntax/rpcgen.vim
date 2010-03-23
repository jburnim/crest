" Vim syntax file
" Language:	rpcgen
" Version:	5.4-1
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	April 23, 1999

" Removes any old syntax stuff hanging around
syn clear

" Read the C syntax to start with
source <sfile>:p:h/c.vim

syn keyword rpcProgram	program				skipnl skipwhite nextgroup=rpcProgName
syn match   rpcProgName	contained	"\<\i\I*\>"	skipnl skipwhite nextgroup=rpcProgZone
syn region  rpcProgZone	contained	matchgroup=Delimiter start="{" matchgroup=Delimiter end="}\s*=\s*\(\d\+\|0x[23]\x\{7}\)\s*;"me=e-1 contains=rpcVersion,cComment,rpcProgNmbrErr
syn keyword rpcVersion	contained	version		skipnl skipwhite nextgroup=rpcVersName
syn match   rpcVersName	contained	"\<\i\I*\>"	skipnl skipwhite nextgroup=rpcVersZone
syn region  rpcVersZone	contained	matchgroup=Delimiter start="{" matchgroup=Delimiter end="}\s*=\s*\d\+\s*;"me=e-1 contains=cType,cStructure,cStorageClass,rpcDecl,rpcProcNmbr,cComment
syn keyword rpcDecl	contained	string
syn match   rpcProcNmbr	contained	"=\s*\d\+;"me=e-1
syn match   rpcProgNmbrErr contained	"=\s*0x[^23]\x*"ms=s+1
syn match   rpcPassThru			"^\s*%.*$"

if !exists("did_rpcgen_syntax_inits")
 let did_rpcgen_syntax_inits = 1
 hi link rpcProgName	rpcName
 hi link rpcProgram	rpcStatement
 hi link rpcVersName	rpcName
 hi link rpcVersion	rpcStatement

 hi link rpcDecl	cType
 hi link rpcPassThru	cComment

 hi link rpcName	Special
 hi link rpcProcNmbr	Delimiter
 hi link rpcProgNmbrErr	Error
 hi link rpcStatement	Statement
endif

let b:current_syntax = "rpcgen"

" vim: ts=8
