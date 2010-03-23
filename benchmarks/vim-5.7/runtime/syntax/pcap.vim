" Vim syntax file
" Config file:	printcap
" Maintainer:	Lennart Schultz <Lennart.Schultz@ecmwf.int>
" Last Change:	1999 Jun 14

"define keywords
se isk=@,46-57,_,-,#,=,192-255
" Remove any old syntax stuff hanging around
syn clear
"first all the bad guys
syn match pcapBad '^.\+$'              "define any line as bad
syn match pcapBadword '\k\+' contained "define any sequence of keywords as bad
syn match pcapBadword ':' contained    "define any single : as bad
syn match pcapBadword '\\' contained   "define any single \ as bad
"then the good boys
" Boolean keywords
syn match pcapKeyword contained ':\(fo\|hl\|ic\|rs\|rw\|sb\|sc\|sf\|sh\)'
" Numeric Keywords
syn match pcapKeyword contained ':\(br\|du\|fc\|fs\|mx\|pc\|pl\|pw\|px\|py\|xc\|xs\)#\d\+'
" String Keywords
syn match pcapKeyword contained ':\(af\|cf\|df\|ff\|gf\|if\|lf\|lo\|lp\|nd\|nf\|of\|rf\|rg\|rm\|rp\|sd\|st\|tf\|tr\|vf\)=\k*'
" allow continuation
syn match pcapEnd ':\\$' contained
"
syn match pcapDefineLast '^\s\+.\+$' contains=pcapBadword,pcapKeyword
syn match pcapDefine '^\s\+.\+$' contains=pcapBadword,pcapKeyword,pcapEnd
syn match pcapHeader '^\k.\+\(|\k.\+\)*:\\$'
syn match pcapComment "#.*$"

syn sync minlines=50


if !exists("did_pcap_syntax_inits")
  let did_pcap_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link pcapBad WarningMsg
  hi link pcapBadword WarningMsg
  hi link pcapComment Comment
endif

let b:current_syntax = "pcap"

" vim: ts=8
