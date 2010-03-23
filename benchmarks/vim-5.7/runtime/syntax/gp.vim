" Vim syntax file
" Language:	gp (version 2.0)
" Maintainer:	Karim Belabas <Karim.Belabas@math.u-psud.fr>
" Last change:	2000 June 23

syntax clear
" some control statements
syntax keyword gpStatement	break return next
syntax keyword gpConditional	if
syntax keyword gpRepeat		until while for fordiv forprime forstep forvec
syntax keyword gpLocal          local

syntax keyword gpInterfaceKey	buffersize colors compatible debug debugmem
syntax keyword gpInterfaceKey	echo format help histsize log logfile output
syntax keyword gpInterfaceKey	parisize path primelimit prompt psfile
syntax keyword gpInterfaceKey	realprecision seriesprecision simplify
syntax keyword gpInterfaceKey	strictmatch timer

syntax match   gpInterface	"^\s*\\[a-z].*"
syntax keyword gpInterface	default
syntax keyword gpInput		read input

" functions
syntax match gpFunRegion "^\s*[a-zA-Z][_a-zA-Z0-9]*(.*)\s*=\s*[^ \t=]"me=e-1 contains=gpFunction,gpArgs
syntax match gpFunRegion "^\s*[a-zA-Z][_a-zA-Z0-9]*(.*)\s*=\s*$" contains=gpFunction,gpArgs
syntax match gpArgs contained "[a-zA-Z][_a-zA-Z0-9]*"
syntax match gpFunction contained "^\s*[a-zA-Z][_a-zA-Z0-9]*("me=e-1

" String and Character constants
" Highlight special (backslash'ed) characters differently
syntax match  gpSpecial contained "\\[ent\\]"
syntax region gpString  start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=gpSpecial

"comments
syntax region gpComment	start="/\*"  end="\*/" contains=gpTodo
syntax match  gpComment "\\\\.*" contains=gpTodo
syntax keyword gpTodo contained	TODO
syntax sync ccomment gpComment minlines=10

"catch errors caused by wrong parenthesis
syntax region gpParen		transparent start='(' end=')' contains=ALLBUT,gpParenError,gpTodo,gpFunction,gpArgs,gpSpecial
syntax match gpParenError	")"
syntax match gpInParen contained "[{}]"

if !exists("did_gp_syntax_inits")
  let did_gp_syntax_inits = 1
  highlight link gpConditional	Conditional
  highlight link gpRepeat	Repeat
  highlight link gpError	Error
  highlight link gpParenError	gpError
  highlight link gpInParen	gpError
  highlight link gpStatement	Statement
  highlight link gpString	String
  highlight link gpComment	Comment
  highlight link gpInterface	Type
  highlight link gpInput	Type
  highlight link gpInterfaceKey Statement
  highlight link gpFunction	Function
  highlight link gpLocal 	Type

  " contained ones
  highlight link gpSpecial	Special
  highlight link gpTodo		Todo
  highlight link gpArgs		Type
endif

let b:current_syntax = "gp"
" vim: ts=8
