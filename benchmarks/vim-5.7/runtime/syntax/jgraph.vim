" Vim syntax file
" Language:	jgraph (graph plotting utility)
" Maintainer:	Jonas Munsin jmunsin@iki.fi
" Last Change:	1999 Jun 14
" this syntax file is not yet complete


" Remove any old syntax stuff hanging around
syn clear
syn case match

" comments
syn region	jgraphComment	start="(\* " end=" \*)"

syn keyword	jgraphCmd	newcurve newgraph marktype
syn keyword	jgraphType	xaxis yaxis

syn keyword	jgraphType	circle box diamond triangle x cross ellipse
syn keyword	jgraphType	xbar ybar text postscript eps none general

syn keyword	jgraphType	solid dotted dashed longdash dotdash dodotdash
syn keyword	jgraphType	dotdotdashdash pts

"integer number, or floating point number without a dot. - or no -
syn match  jgraphNumber          "\<-\=\d\+\>"
"floating point number, with dot - or no -
syn match  jgraphNumber          "\<-\=\d\+\.\d*\>"
"floating point number, starting with a dot - or no -
syn match  jgraphNumber          "\-\=\.\d\+\>"


hi link jgraphComment	Comment
hi link jgraphCmd	Identifier
hi link jgraphType	Type
hi link jgraphNumber	Number


let b:current_syntax = "jgraph"
