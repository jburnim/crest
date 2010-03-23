" Vim syntax file
" Language:	GDMO
"		(ISO-10165-4; Guidelines for the Definition of Managed Object)
" Maintainer:	Gyuman Kim <violino@dooly.modacom.co.kr>
" URL:		http://dooly.modacom.co.kr/~violino/gdmo.vim
" Last Change:	1999 Jul 02

" Remove any old syntax stuff hanging around
syn clear

" keyword definitions
syn match   gdmoCategory      "MANAGED\s\+OBJECT\s\+CLASS"
syn keyword gdmoCategory      NOTIFICATION ATTRIBUTE BEHAVIOUR PACKAGE
syn match   gdmoCategory      "NAME\s\+BINDING"
syn match   gdmoRelationship  "DERIVED\s\+FROM"
syn match   gdmoRelationship  "SUPERIOR\s\+OBJECT\s\+CLASS"
syn match   gdmoRelationship  "SUBORDINATE\s\+OBJECT\s\+CLASS"
syn match   gdmoExtension     "AND\s\+SUBCLASSES"
syn match   gdmoDefinition    "DEFINED\s\+AS"
syn match   gdmoDefinition    "REGISTERED\s\+AS"
syn match   gdmoExtension     "ORDER\s\+BY"
syn match   gdmoReference     "WITH\s\+ATTRIBUTE"
syn match   gdmoReference     "WITH\s\+INFORMATION\s\+SYNTAX"
syn match   gdmoReference     "WITH\s\+REPLY\s\+SYNTAX"
syn match   gdmoReference     "WITH\s\+ATTRIBUTE\s\+SYNTAX"
syn match   gdmoExtension     "AND\s\+ATTRIBUTE\s\+IDS"
syn match   gdmoExtension     "MATCHES\s\+FOR"
syn match   gdmoReference     "CHARACTERIZED\s\+BY"
syn match   gdmoReference     "CONDITIONAL\s\+PACKAGES"
syn match   gdmoExtension     "PRESENT\s\+IF"
syn match   gdmoExtension     "DEFAULT\s\+VALUE"
syn match   gdmoExtension     "PERMITTED\s\+VALUES"
syn match   gdmoExtension     "REQUIRED\s\+VALUES"
syn match   gdmoExtension     "NAMED\s\+BY"
syn keyword gdmoReference     ATTRIBUTES NOTIFICATIONS
syn keyword gdmoExtension     DELETE CREATE
syn keyword gdmoExtension     EQUALITY SUBSTRINGS ORDERING
syn match   gdmoExtension     "REPLACE-WITH-DEFAULT"
syn match   gdmoExtension     "GET"
syn match   gdmoExtension     "GET-REPLACE"
syn match   gdmoExtension     "ADD-REMOVE"
syn match   gdmoExtension     "WITH-REFERENCE-OBJECT"
syn match   gdmoExtension     "WITH-AUTOMATIC-INSTANCE-NAMING"
syn match   gdmoExtension     "ONLY-IF-NO-CONTAINED-OBJECTS"


" Strings and constants
syn match   gdmoSpecial           contained "\\\d\d\d\|\\."
syn region  gdmoString            start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=gdmoSpecial
syn match   gdmoCharacter         "'[^\\]'"
syn match   gdmoSpecialCharacter  "'\\.'"
syn match   gdmoNumber            "0[xX][0-9a-fA-F]\+\>"
syn match   gdmoLineComment       "--.*"
syn match   gdmoLineComment       "--.*--"

syn match gdmoDefinition "^\s*[a-zA-Z][-a-zA-Z0-9_.\[\] \t{}]* *::="me=e-3
syn match gdmoBraces     "[{}]"

syn sync ccomment gdmoComment

if !exists("did_gdmo_syntax_inits")
  let did_gdmo_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link gdmoCategory         Structure
  hi link gdmoRelationship     Macro
  hi link gdmoDefinition       Statement
  hi link gdmoReference        Type
  hi link gdmoExtension        Operator
  hi link gdmoBraces           Function
  hi link gdmoSpecial          Special
  hi link gdmoString           String
  hi link gdmoCharacter        Character
  hi link gdmoSpecialCharacter gdmoSpecial
  hi link gdmoComment          Comment
  hi link gdmoLineComment      gdmoComment
  hi link gdmoType             Type
endif

let b:current_syntax = "gdmo"

" vim: ts=8
