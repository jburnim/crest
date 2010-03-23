" Vim syntax file
" Language:	Dracula
" Maintainer:	Scott Bordelon <slb@artisan.com>
" Last Change:	after Wed Nov 11 9:58:29 PDT 1997
" Extensions:	drac.*,*.drac,*.drc,*.lvs,*.lpe
" Comment:	Dracula is an industry-standard language created by CADENCE (a
"		company specializing in Electronics Design Automation), for
"		the purposes of Design Rule Checking, Layout vs. Schematic
"		verification, and Layout Parameter Extraction.

" Remove any old syntax stuff hanging around
syn clear

" Ignore case
syn case ignore

" A bunch of useful Dracula keywords

"syn match   draculaIdentifier

syn keyword draculaStatement   indisk primary outdisk printfile system
syn keyword draculaStatement   mode scale resolution listerror keepdata
syn keyword draculaStatement   datatype by lt gt output label range touch
syn keyword draculaStatement   inside outside within overlap outlib
syn keyword draculaStatement   schematic model unit parset
syn match   draculaStatement   "flag-\(non45\|acuteangle\|offgrid\)"
syn match   draculaStatement   "text-pri-only"
syn match   draculaStatement   "[=&]"
syn match   draculaStatement   "\[[^,]*\]"
syn match   draculastatement   "^ *\(sel\|width\|ext\|enc\|area\|shrink\|grow\|length\)"
syn match   draculastatement   "^ *\(or\|not\|and\|select\|size\|connect\|sconnect\|int\)"
syn match   draculastatement   "^ *\(softchk\|stamp\|element\|parasitic cap\|attribute cap\)"
syn match   draculastatement   "^ *\(lextract\|equation\|lpeselect\|lpechk\|attach\)"
syn match   draculaStatement   "\(temporary\|connect\)-layer"
syn match   draculaStatement   "batch-queue"
syn match   draculaStatement   "cnames-csen"
syn match   draculaStatement   "filter-lay-opt"
syn match   draculaStatement   "filter-sch-opt"
syn match   draculaStatement   "power-node"
syn match   draculaStatement   "ground-node"
syn match   draculaStatement   "subckt-name"

syn match   draculaType        "\*description"
syn match   draculaType        "\*input-layer"
syn match   draculaType        "\*operation"
syn match   draculaType        "\*end"

syn match   draculaComment ";.*"

syn match   draculaPreProc "^#.*"

"Modify the following as needed.  The trade-off is performance versus
"functionality.
syn sync lines=50

if !exists("did_dracula_syntax_inits")
  let did_dracula_syntax_inits = 1
 " The default methods for highlighting.  Can be overridden later

  hi link draculaIdentifier Identifier
  hi link draculaStatement  Statement
  hi link draculaType       Type
  hi link draculaComment    Comment
  hi link draculaPreProc    PreProc
endif

let b:current_syntax = "dracula"

" vim: ts=8
