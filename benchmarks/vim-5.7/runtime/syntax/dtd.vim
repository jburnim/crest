" Vim syntax file
" Language:	DTD (Document Type Definition for XML)
" Maintainer:	Johannes Zellner <johannes@zellner.org>
"		Author and previous maintainer:
"		Daniel Amyot <damyot@site.uottawa.ca>
" Last Change:	Dec 09 1999
" Filenames:	*.dtd
" URL:		http://www.zellner.org/vim/syntax/dtd.vim
" $Id: dtd.vim,v 1.3 1999/12/10 18:29:50 joze Exp $
"
"
" CREDITS:
" - original note of Daniel Amyot <damyot@site.uottawa.ca>:
"   This file is an adaptation of pascal.vim by Mario Eusebio
"   I'm not sure I understand all of the syntax highlight language,
"   but this file seems to do the job for simple DTD in XML.
"   This would have to be extended to cover the whole of SGML DTDs though.
"   Unfortunately, I don't know enough about the somewhat complex SGML
"   to do it myself. Volunteers are most welcomed!
"
"
" REFERENCES:
"   http://www.w3.org/TR/html40/
"   http://www.w3.org/TR/NOTE-html-970421
"
" TODO:
"   - improve synchronizing.

syn clear

if !exists("dtd_ignore_case")
    " I prefer having the case takes into consideration.
    syn case match
else
    syn case ignore
endif


" the following line makes the opening <! and
" closing > highlighted using 'dtdFunction'.
syn region dtdTag matchgroup=dtdFunction start=+<!+ end=+>+ matchgroup=NONE contains=dtdTag,dtdTagName,dtdError,dtdComment,dtdString,dtdAttrType,dtdAttrDef,dtdEnum,dtdParamEntityInst,dtdParamEntityDecl,dtdCard

if !exists("dtd_no_tag_errors")
    " mark everything as an error which starts with a <!
    " and is not overridden later. If this is annoying,
    " it can be switched off by setting the variable
    " dtd_no_tag_errors.
    syn region dtdError contained start=+<!+lc=2 end=+>+
endif

" if this is a html like comment hightlight also
" the opening <! and the closing > as Comment.
syn region dtdComment           start=+<![ \t]*--+ end=+-->+ contains=dtdTodo


" proper DTD comment
syn region dtdComment contained start=+--+ end=+--+ contains=dtdTodo


" Start tags (keywords). This is contained in dtdFunction.
" Note that everything not contained here will be marked
" as error.
syn match dtdTagName contained +<!\(ATTLIST\|DOCTYPE\|ELEMENT\|ENTITY\|NOTATION\|SHORTREF\|USEMAP\|\[\)+lc=2,hs=s+2


if !exists("dtd_no_param_entities")

  " highlight parameter entity declarations
  " and instances. Note that the closing `;'
  " is optional.

  " instances
  syn region dtdParamEntityInst oneline matchgroup=dtdParamEntityPunct start="%[-_a-zA-Z0-9.]\+"he=s+1,rs=s+1 skip=+[-_a-zA-Z0-9.]+ end=";\|\>" matchgroup=NONE contains=dtdParamEntityPunct
  syn match  dtdParamEntityPunct contained "\."

  " declarations
  " syn region dtdParamEntityDecl oneline matchgroup=dtdParamEntityDPunct start=+<!ENTITY % +lc=8 skip=+[-_a-zA-Z0-9.]+ matchgroup=NONE end="\>" contains=dtdParamEntityDPunct
  syn match dtdParamEntityDecl +<!ENTITY % [-_a-zA-Z0-9.]*+lc=8 contains=dtdParamEntityDPunct
  syn match  dtdParamEntityDPunct contained "%\|\."

endif


" Strings are between quotes
syn region dtdString    start=+"+ skip=+\\\\\|\\"+  end=+"+ contains=dtdAttrDef,dtdAttrType,dtdEnum,dtdParamEntityInst,dtdCard
syn region dtdString    start=+'+ skip=+\\\\\|\\'+  end=+'+ contains=dtdAttrDef,dtdAttrType,dtdEnum,dtdParamEntityInst,dtdCard

" Enumeration of elements or data between parenthesis
syn region dtdEnum matchgroup=dtdType start="(" end=")" matchgroup=NONE contains=dtdEnum,dtdParamEntityInst,dtdCard

" wildcards and operators
syn match  dtdCard contained "|"
syn match  dtdCard contained ","
syn match  dtdCard contained "&"
syn match  dtdCard contained "\?"
syn match  dtdCard contained "\*"
syn match  dtdCard contained "+"

" ...and finally, special cases.
syn match  dtdCard      "ANY"
syn match  dtdCard      "EMPTY"

"Attribute types
syn keyword dtdAttrType NMTOKEN  ENTITIES  NMTOKENS  ID  CDATA
syn keyword dtdAttrType IDREF  IDREFS
" ENTITY has to treated special for not overriding <!ENTITY
syn match   dtdAttrType +[^!]\<ENTITY+

"Attribute Definitions
syn match  dtdAttrDef   "#REQUIRED"
syn match  dtdAttrDef   "#IMPLIED"
syn match  dtdAttrDef   "#FIXED"

syn case match
" define some common keywords to mark TODO
" and important sections inside comments.
syn keyword dtdTodo contained TODO FIXME XXX

syn sync lines=250

if !exists("did_dtd_syntax_inits")
  let did_dtd_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link dtdFunction		Function
  hi link dtdTag		Normal
  hi link dtdType		Type
  hi link dtdAttrType		dtdType
  hi link dtdAttrDef		dtdType
  hi link dtdConstant		Constant
  hi link dtdString		dtdConstant
  hi link dtdEnum		dtdConstant
  hi link dtdCard		dtdFunction

  hi link dtdParamEntityInst	dtdConstant
  hi link dtdParamEntityPunct	dtdType
  hi link dtdParamEntityDecl	dtdType
  hi link dtdParamEntityDPunct	dtdComment

  hi link dtdComment		Comment
  hi link dtdTagName		Statement
  hi link dtdError		Error
  hi link dtdTodo		Todo
endif

let b:current_syntax = "dtd"

" vim: ts=8
