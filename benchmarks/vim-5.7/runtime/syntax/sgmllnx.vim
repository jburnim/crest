" Vim syntax file
" Language:	SGML-linuxdoc (supported by old sgmltools-1.x)
"		(for more information, visit www.sgmltools.org)
" Maintainer:	Sung-Hyun Nam <namsh@kldp.org>
"		If you want to enhance and maintain, You can remove my name
"		and insert yours.
" Last Change:	1999 Sep 18

" Remove any old syntax stuff hanging around
syn clear
syn case ignore

" tags
syn region sgmllnxEndTag	start=+</+    end=+>+	contains=sgmllnxTagN,sgmllnxTagError
syn region sgmllnxTag	start=+<[^/]+ end=+>+	contains=sgmllnxTagN,sgmllnxTagError
syn match  sgmllnxTagN	contained +<\s*[-a-zA-Z0-9]\++ms=s+1	contains=sgmllnxTagName
syn match  sgmllnxTagN	contained +</\s*[-a-zA-Z0-9]\++ms=s+2	contains=sgmllnxTagName

syn region sgmllnxTag2	start=+<\s*[a-zA-Z]\+/+ keepend end=+/+	contains=sgmllnxTagN2
syn match  sgmllnxTagN2	contained +/.*/+ms=s+1,me=e-1

syn region sgmllnxSpecial	oneline start="&" end=";"

" tag names
syn keyword sgmllnxTagName contained article author date toc title sect verb
syn keyword sgmllnxTagName contained abstract tscreen p itemize item enum
syn keyword sgmllnxTagName contained descrip quote htmlurl code ref
syn keyword sgmllnxTagName contained tt tag bf
syn match   sgmllnxTagName contained "sect\d\+"

" Comments
syn region sgmllnxComment start=+<!--+ end=+-->+
syn region sgmllnxDocType start=+<!doctype+ end=+>+

if !exists("did_sgmllnx_syntax_inits")
  let did_sgmllnx_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link sgmllnxTag2	Function
  hi link sgmllnxTagN2	Function
  hi link sgmllnxTag	Special
  hi link sgmllnxEndTag	Special
  hi link sgmllnxParen	Special
  hi link sgmllnxEntity	Type
  hi link sgmllnxDocEnt    Type
  hi link sgmllnxTagName	Statement
  hi link sgmllnxComment	Comment
  hi link sgmllnxSpecial	Special
  hi link sgmllnxDocType   PreProc
  hi link sgmllnxTagError	Error
endif

let b:current_syntax = "sgmllnx"

" vim:set tw=78 ts=8 sts=8 sw=8 noet com=nb\:":
