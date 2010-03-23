" Vim syntax file
" Language:	Verilog
" Maintainer:	Mun Johl <mj@core.rose.hp.com>
" Last Update:	Tue Feb  1 14:39:57 PST 2000

" Remove any old syntax stuff hanging around
syn clear
set iskeyword=@,48-57,_,192-255,+,-,?


" A bunch of useful Verilog keywords
syn keyword verilogStatement   disable assign deassign force release
syn keyword verilogStatement   parameter function endfunction
syn keyword verilogStatement   always initial module endmodule or
syn keyword verilogStatement   task endtask
syn keyword verilogStatement   input output inout reg wire
syn keyword verilogStatement   posedge negedge wait
syn keyword verilogStatement   buf pullup pull0 pull1 pulldown
syn keyword verilogStatement   tri0 tri1 tri trireg
syn keyword verilogStatement   wand wor triand trior
syn keyword verilogStatement   defparam
syn keyword verilogStatement   integer real
syn keyword verilogStatement   time

syn keyword verilogLabel       begin end fork join
syn keyword verilogConditional if else case casex casez default endcase
syn keyword verilogRepeat      forever repeat while for

syn keyword verilogTodo contained TODO

syn match   verilogOperator "[&|~><!)(*#%@+/=?:;}{,.\^\-\[\]]"

syn region  verilogComment start="/\*" end="\*/" contains=verilogTodo
syn match   verilogComment "//.*" oneline

syn match   verilogGlobal "`[a-zA-Z0-9_]\+\>"
syn match   verilogGlobal "$[a-zA-Z0-9_]\+\>"

syn match   verilogConstant "\<[A-Z][A-Z0-9_]\+\>"

syn match   verilogNumber "\(\<\d\+\|\)'[bB]\s*[0-1_xXzZ?]\+\>"
syn match   verilogNumber "\(\<\d\+\|\)'[oO]\s*[0-7_xXzZ?]\+\>"
syn match   verilogNumber "\(\<\d\+\|\)'[dD]\s*[0-9_xXzZ?]\+\>"
syn match   verilogNumber "\(\<\d\+\|\)'[hH]\s*[0-9a-fA-F_xXzZ?]\+\>"
syn match   verilogNumber "\<[+-]\=[0-9_]\+\(\.[0-9_]*\|\)\(e[0-9_]*\|\)\>"

syn region  verilogString start=+"+  end=+"+

" Directives
syn match   verilogDirective   "//\s*synopsys\>.*$"
syn region  verilogDirective   start="/\*\s*synopsys\>" end="\*/"
syn region  verilogDirective   start="//\s*synopsys dc_script_begin\>" end="//\s*synopsys dc_script_end\>"

syn match   verilogDirective   "//\s*\$s\>.*$"
syn region  verilogDirective   start="/\*\s*\$s\>" end="\*/"
syn region  verilogDirective   start="//\s*\$s dc_script_begin\>" end="//\s*\$s dc_script_end\>"

"Modify the following as needed.  The trade-off is performance versus
"functionality.
syn sync lines=50

if !exists("did_verilog_syntax_inits")
  let did_verilog_syntax_inits = 1
 " The default methods for highlighting.  Can be overridden later

  hi link verilogCharacter       Character
  hi link verilogConditional     Conditional
  hi link verilogRepeat          Repeat
  hi link verilogString          String
  hi link verilogTodo            Todo
  hi link verilogComment         Comment
  hi link verilogConstant        Constant
  hi link verilogLabel           Label
  hi link verilogNumber          Number
  hi link verilogOperator        Special
  hi link verilogStatement       Statement
  hi link verilogGlobal          Define
  hi link verilogDirective       SpecialComment
endif

let b:current_syntax = "verilog"

" vim: ts=8
