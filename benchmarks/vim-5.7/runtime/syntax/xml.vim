" Vim syntax file
" Language:	XML
" Maintainer:	Johannes Zellner <johannes@zellner.org>
"		Author and previous maintainer:
"		Paul Siegmann <pauls@euronet.nl>
" Filenames:	*.xml
" URL:		http://www.zellner.org/vim/syntax/xml.vim
" Last Change:	Dec 09 1999
" $Id: xml.vim,v 1.8 1999/12/10 18:22:19 joze Exp $

" CREDITS:
" - This syntax file will highlight xml tags and arguments.
"   The original version was derived by Paul Siegmann from
"   Claudio Fleiner's html.vim.
"
" - suggestions and patches by:
"       Rafael Garcia-Suarez
"
" REFERENCES:
"   http://www.w3.org/TR/1998/REC-xml-19980210
"   http://www.w3.org/XML/1998/06/xmlspec-report-19980910.htm
"

syn clear

" Following items are case-sensitive
" Case-insensitive rules can be specified by 'syn case ignore' later
syn case match

" mark illegal characters
syn match xmlError "[<>&]"


" tags
syn match   xmlSpecial  contained "\\\d\d\d\|\\."
syn region  xmlString   contained start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=xmlSpecial
syn region  xmlString   contained start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=xmlSpecial
syn region  xmlEndTag             start=+</+    end=+>+              contains=xmlTagError
syn region  xmlTag                start=+<[^/]+ end=+>+              contains=xmlString,xmlTagError
syn match   xmlTagError contained "[^>]<"ms=s+1

" special characters
syn match   xmlSpecialChar "&[^;]*;"

syn keyword xmlTodo         contained TODO FIXME XXX

" The real comments (this implements the comments as defined by xml,
" but not all xml pages actually conform to it. Errors are flagged.
syn region  xmlComment                start=+<!+        end=+>+ contains=xmlCommentPart,xmlString,xmlCommentError,xmlTodo
syn match   xmlCommentError contained "[^><!]"
syn region  xmlCommentPart  contained start=+--+        end=+--+

" DTD -- we use dtd.vim here
syn region  xmlDocType matchgroup=xmlDocTypeDecl start="<!DOCTYPE"he=s+2,rs=s+2 end=">" contains=xmlDocTypeKeyword,xmlInlineDTD,xmlString
syn keyword xmlDocTypeKeyword contained DOCTYPE PUBLIC SYSTEM
syn region  xmlInlineDTD contained start="\[" end="]" contains=@xmlDTD
syn include @xmlDTD <sfile>:p:h/dtd.vim

" Processing instructions
" This allows "?>" inside strings -- good idea?
syn region  xmlProcessing matchgroup=xmlProcessingDelim start="<?" end="?>" contains=xmlString

" CData sections
syn region  xmlCdata matchgroup=xmlCdataDecl start="<!\[CDATA\[" keepend end="]]>"

" synchronizing (does not always work if a comment includes legal
" xml tags, but doing it right would mean to always start
" at the first line, which is too slow)
syn sync match xmlHighlight groupthere NONE "<[/a-zA-Z]"
syn sync match xmlHighlightSkip "^.*['\"].*$"
syn sync minlines=10

if !exists("did_xml_syntax_inits")
  let did_xml_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  "
  hi link xmlTodo                      Todo
  hi link xmlTag                       Function
  hi link xmlEndTag                    Identifier
  hi link xmlSpecial                   Special
  hi link xmlSpecialChar               Special
  hi link xmlString                    String
  hi link xmlComment                   Comment
  hi link xmlCommentPart               Comment
  hi link xmlCommentError              xmlError
  hi link xmlTagError                  xmlError
  hi link xmlError                     Error

  hi link xmlProcessingDelim           Comment
  hi link xmlProcessing                Type
  hi link xmlCdata                     Normal
  hi link xmlCdataDecl                 String
  hi link xmlDocType                   Normal
  hi link xmlDocTypeDecl               Function
  hi link xmlDocTypeKeyword            Statement
  hi link xmlInlineDTD                 Function

endif

let b:current_syntax = "xml"

" vim: ts=8
