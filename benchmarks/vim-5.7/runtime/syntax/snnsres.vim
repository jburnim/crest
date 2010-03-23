" Vim syntax file
" Language:	SNNS result file
" Maintainer:	Davide Alberani <alberanid@bigfoot.com>
" Last Change:	29 Jan 2000
" Version:	0.1
" URL:		http://members.xoom.com/alberanid/vim/syntax/snnsres.vim
"
" SNNS http://www-ra.informatik.uni-tuebingen.de/SNNS/
" is a simulator for neural networks.

" Remove any old syntax stuff hanging around
syn clear

" the accepted fields in the header
syn match	snnsresNoHeader	"No\. of patterns\s*:\s*" contained
syn match	snnsresNoHeader	"No\. of input units\s*:\s*" contained
syn match	snnsresNoHeader	"No\. of output units\s*:\s*" contained
syn match	snnsresNoHeader	"No\. of variable input dimensions\s*:\s*" contained
syn match	snnsresNoHeader	"No\. of variable output dimensions\s*:\s*" contained
syn match	snnsresNoHeader	"Maximum input dimensions\s*:\s*" contained
syn match	snnsresNoHeader	"Maximum output dimensions\s*:\s*" contained
syn match	snnsresNoHeader	"startpattern\s*:\s*" contained
syn match	snnsresNoHeader "endpattern\s*:\s*" contained
syn match	snnsresNoHeader "input patterns included" contained
syn match	snnsresNoHeader "teaching output included" contained
syn match 	snnsresGen	"generated at.*" contained contains=snnsresNumbers
syn match	snnsresGen	"SNNS result file [Vv]\d\.\d" contained contains=snnsresNumbers

" the header, what is not an accepted field, is an error
syn region	snnsresHeader	start="^SNNS" end="^\s*[-+\.]\=[0-9#]"me=e-2 contains=snnsresNoHeader,snnsresNumbers,snnsresGen

" numbers inside the header
syn match	snnsresNumbers	"\d" contained
syn match	snnsresComment	"#.*$" contains=snnsresTodo
syn keyword	snnsresTodo	TODO XXX FIXME contained

if !exists("did_snnsres_syntax_inits")
  let did_snnsres_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link snnsresGen		Statement
  hi link snnsresHeader		Statement
  hi link snnsresNoHeader	Define
  hi link snnsresNumbers	Number
  hi link snnsresComment	Comment
  hi link snnsresTodo		Todo
endif

let b:current_syntax = "snnsres"

" vim: ts=8
