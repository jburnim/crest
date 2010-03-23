" Vim syntax file
" Language:	JavaScript
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/javascript.vim
" Last Change:	1999 Apr 20


" Remove any old syntax stuff hanging around
syn clear
syn case ignore

if !exists("main_syntax")
  let main_syntax = 'javascript'
endif

let b:current_syntax = "javascript"

syn match   javaScriptLineComment      "\/\/.*$"
syn match   javaScriptCommentSkip      "^[ \t]*\*\($\|[ \t]\+\)"
syn region  javaScriptCommentString    start=+"+  skip=+\\\\\|\\"+  end=+"+ end=+\*/+me=s-1,he=s-1 contains=javaScriptSpecial,javaScriptCommentSkip,@htmlPreproc
syn region  javaScriptComment2String   start=+"+  skip=+\\\\\|\\"+  end=+$\|"+  contains=javaScriptSpecial,@htmlPreproc
syn region  javaScriptComment          start="/\*"  end="\*/" contains=javaScriptCommentString,javaScriptCharacter,javaScriptNumber
syn match   javaScriptSpecial          "\\\d\d\d\|\\."
syn region  javaScriptStringD          start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=javaScriptSpecial,@htmlPreproc
syn region  javaScriptStringS          start=+'+  skip=+\\\\\|\\'+  end=+'+  contains=javaScriptSpecial,@htmlPreproc
syn match   javaScriptSpecialCharacter "'\\.'"
syn match   javaScriptNumber           "-\=\<\d\+L\=\>\|0[xX][0-9a-fA-F]\+\>"
syn keyword javaScriptConditional      if else
syn keyword javaScriptRepeat           while for
syn keyword javaScriptBranch           break continue
syn keyword javaScriptOperator         new in
syn keyword javaScriptType             this var
syn keyword javaScriptStatement        return with
syn keyword javaScriptFunction         function
syn keyword javaScriptBoolean          true false
syn match   javaScriptBraces           "[{}]"

" catch errors caused by wrong parenthesis
syn match   javaScriptInParen     contained "[{}]"
syn region  javaScriptParen       transparent start="(" end=")" contains=javaScriptParen,javaScript.*
syn match   javaScrParenError  ")"

if main_syntax == "javascript"
  syn sync ccomment javaScriptComment
endif

if !exists("did_javascript_syntax_inits")
  let did_javascript_syntax_inits = 1
  hi link javaScriptComment           Comment
  hi link javaScriptLineComment       Comment
  hi link javaScriptSpecial           Special
  hi link javaScriptStringS           String
  hi link javaScriptStringD           String
  hi link javaScriptCharacter         Character
  hi link javaScriptSpecialCharacter  javaScriptSpecial
  hi link javaScriptNumber            javaScriptValue
  hi link javaScriptConditional       Conditional
  hi link javaScriptRepeat            Repeat
  hi link javaScriptBranch            Conditional
  hi link javaScriptOperator          Operator
  hi link javaScriptType              Type
  hi link javaScriptStatement         Statement
  hi link javaScriptFunction          Function
  hi link javaScriptBraces            Function
  hi link javaScriptError             Error
  hi link javaScrParenError           javaScriptError
  hi link javaScriptInParen           javaScriptError
  hi link javaScriptBoolean           Boolean
endif

let b:current_syntax = "javascript"
if main_syntax == 'javascript'
  unlet main_syntax
endif

" vim: ts=8
