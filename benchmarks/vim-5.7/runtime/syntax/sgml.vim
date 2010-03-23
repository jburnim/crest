" Vim syntax file
" Language:	SGML-DTD (supported by sgmltools-2.x and DocBook)
"		For more information, visit
"		http://nis-www.lanl.gov/~rosalia/mydocs/docbook-intro.html
"		ftp://sourceware.cygnus.com/pub/docbook-tools
" Maintainer:	Sung-Hyun Nam <namsh@kldp.org>
"		If you want to enhance and maintain, You can remove my name
"		and insert yours.
" Last Change:	1999/12/10

" Remove any old syntax stuff hanging around
syn clear
syn case ignore

" tags
syn match   sgmlTagError ">"
syn match   sgmlErrInTag contained "<"
syn region  sgmlEndTag	start=+</[a-zA-Z]+ end=+>+ contains=sgmlTagName
syn match   sgmlEndTag	"</>"
syn region  sgmlTag	start=+<[a-zA-Z]+ end=+>+
			\ contains=sgmlTagName,sgmlAssign
syn region  sgmlStr	contained start=+L\="+ end=+"+
syn region  sgmlAssign  contained start=+=+hs=e+1 end=+[ \t\>]+me=s-1
			\ contains=sgmlStr
syn region  sgmlSpecial oneline start="&" end=";"
syn region  sgmlComment start=+<!--+ end=+-->+
syn region  sgmlDocEnt  contained start="<!\(entity\|element\)\s" end=">"
			\ contains=sgmlStr
syn region  sgmlDocEntI contained start=+\[+ end=+]+ contains=sgmlDocEnt
syn region  sgmlDocType start=+<!doctype\s+ end=+>+
			\ contains=sgmlDocEntI,sgmlStr

" tag names for DTD DocBook V3.[01]
syn match   sgmlTagName contained "sect\d\+"
syn match   sgmlTagName contained "sect\d\+info"
syn match   sgmlTagName contained "refsect\d\+"
syn keyword sgmlTagName contained abbrev abstract accel acronym action address
				\ affiliation alt anchor answer appendix
				\ application
				\ area areaset areaspec arg artheader article
				\ artpagenums attribution author authorblurb
				\ authorgroup authorinitials bibliodiv
				\ biblioentry bibliography bibliomixed
				\ bibliomset biblioset blockquote book
				\ bookbiblio bookinfo bridgehead callout
				\ calloutlist caption caution chapter citation
				\ citerefentry citetitle city classname
				\ cmdsynopsis colophon colspec command comment
				\ computeroutput constant copyright corpauthor
				\ corpname country database date dedication
				\ docinfo edition editor email emphasis entry
				\ envar epigraph equation errorcode errorname
				\ errortype example fax figure filename
				\ firstname firstterm footnote footnoteref
				\ foreignphrase formalpara funcdef funcparams
				\ funcprototype funcsynopsis funcsynopsisinfo
				\ function glossary glossdef glossdiv
				\ glossentry glosslist glosssee glossseealso
				\ glossterm graphic group guibutton guiicon
				\ guilabel guimenu guimenuitem guisubmenu
				\ hardware holder honorific imagedata
				\ imageobject important index indexdiv
				\ indexentry indexterm informalequation
				\ informalexample informalfigure informaltable
				\ inlineequation inlinegraphic
				\ inlinemediaobject interface
				\ interfacedefinition isbn issn issuenum
				\ itemizedlist jobtitle keycap keycode
				\ keycombo keysym legalnotice lineannotation
				\ link listitem literal literallayout manvolnum
				\ markup medialabel mediaobject member
				\ mousebutton msg msgaud msgentry msgexplan
				\ msginfo msglevel msgmain msgorig msgrel
				\ msgset msgsub msgtext note objectinfo option
				\ optional orderedlist orgdiv orgname
				\ otheraddr othername pagenums para paramdef
				\ parameter part partintro phone phrase
				\ postcode preface primary primaryie procedure
				\ productname programlisting programlistingco
				\ prompt property pubdate publisher
				\ publishername qandadiv qandaentry qandaset
				\ question quote refdescriptor refentry
				\ refentrytitle reference refmeta refmiscinfo
				\ refname
				\ refnamediv refpurpose refsynopsisdiv
				\ releaseinfo replaceable returnvalue
				\ revhistory revision revnumber revremark row
				\ sbr screen screeninfo screenshot secondary
				\ secondaryie section see seealso seealsoie
				\ seeie seg seglistitem segmentedlist segtitle
				\ seriesinfo set setinfo sgmltag shortaffil
				\ sidebar simpara simplelist simplesect
				\ spanspec state step street structfield
				\ structname subscript substeps subtitle
				\ superscript surname symbol synopsis
				\ systemitem table tbody term tertiaryie
				\ textobject tgroup thead tip title
				\ titleabbrev toc token trademark type ulink
				\ userinput varargs variablelist varlistentry
				\ varname videodata videoobject void volumenum
				\ warning wordasword xref year

if !exists("did_sgml_syntax_inits")
  let did_sgml_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link sgmlTag	Special
  hi link sgmlEndTag	Special
  hi link sgmlEntity	Type
  hi link sgmlDocEnt    Type
  hi link sgmlTagName	Statement
  hi link sgmlComment	Comment
  hi link sgmlSpecial	Special
  hi link sgmlDocType   PreProc
  hi link sgmlStr	String
  hi link sgmlAssign	String
  hi link sgmlTagError	Error
  hi link sgmlErrInTag	Error
endif

let b:current_syntax = "sgml"

" vim:set tw=78 ts=8 sts=2 sw=2 noet com=nb\:":
