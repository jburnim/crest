" WebMacro syntax file
" Language:     WebMacro
" Maintainer:   Claudio Fleiner <claudio@fleiner.com>
" URL:          http://www.fleiner.com/vim/syntax/webmacro.vim
" Last Change:  2000 Feb 13

" webmacro is a nice little language that you should
" check out if you use java servlets.
" webmacro: http://www.webmacro.org

" Remove any old syntax stuff hanging around
syn clear

if !exists("main_syntax")
  let main_syntax = 'webmacro'
endif


so <sfile>:p:h/html.vim

syn cluster htmlPreProc add=webmacroIf,webmacroUse,webmacroBraces,webmacroParse,webmacroInclude,webmacroSet,webmacroForeach,webmacroComment

syn match webmacroVariable "\$[a-zA-Z0-9.()]*;\="
syn match webmacroNumber "[-+]\=\d\+[lL]\=" contained
syn keyword webmacroBoolean true false contained
syn match webmacroSpecial "\\." contained
syn region  webmacroString   contained start=+"+ end=+"+ contains=webmacroSpecial,webmacroVariable
syn region  webmacroString   contained start=+'+ end=+'+ contains=webmacroSpecial,webmacroVariable
syn region webmacroList contained matchgroup=Structure start="\[" matchgroup=Structure end="\]" contains=webmacroString,webmacroVariable,webmacroNumber,webmacroBoolean,webmacroList

syn region webmacroIf start="#if" start="#else" end="{"me=e-1 contains=webmacroVariable,webmacroNumber,webmacroString,webmacroBoolean,webmacroList nextgroup=webmacroBraces
syn region webmacroForeach start="#foreach" end="{"me=e-1 contains=webmacroVariable,webmacroNumber,webmacroString,webmacroBoolean,webmacroList nextgroup=webmacroBraces
syn match webmacroSet "#set .*$" contains=webmacroVariable,webmacroNumber,webmacroNumber,webmacroBoolean,webmacroString,webmacroList
syn match webmacroInclude "#include .*$" contains=webmacroVariable,webmacroNumber,webmacroNumber,webmacroBoolean,webmacroString,webmacroList
syn match webmacroParse "#parse .*$" contains=webmacroVariable,webmacroNumber,webmacroNumber,webmacroBoolean,webmacroString,webmacroList
syn region webmacroUse matchgroup=PreProc start="#use .*" matchgroup=PreProc end="^-.*" contains=webmacroHash,@HtmlTop
syn region webmacroBraces matchgroup=Structure start="{" matchgroup=Structure end="}" contained transparent
syn match webmacroBracesError "[{}]"
syn match webmacroComment "##.*$"
syn match webmacroHash "[#{}\$]" contained

if !exists("did_webmacro_syntax_inits")
  hi link webmacroComment CommentTitle
  hi link webmacroVariable PreProc
  hi link webmacroIf webmacroStatement
  hi link webmacroForeach webmacroStatement
  hi link webmacroSet webmacroStatement
  hi link webmacroInclude webmacroStatement
  hi link webmacroParse webmacroStatement
  hi link webmacroStatement Function
  hi link webmacroNumber Number
  hi link webmacroBoolean Boolean
  hi link webmacroSpecial Special
  hi link webmacroString String
  hi link webmacroBracesError Error
endif

let b:current_syntax = "webmacro"

if main_syntax == 'webmacro'
  unlet main_syntax
endif
