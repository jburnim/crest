" Vim syntax file
" Language:	Lua
" Author:	Carlos Augusto Teixeira Mendes <cmendes@inf.puc-rio.br>
" Last Change:	09 october 1998
"
" Still has some syncing problems...


" Remove any old syntax stuff hanging around
syn clear
syn case match

"Comments
syn keyword luaTodo             contained TODO FIXME XXX
syn match   luaComment          "--.*$" contains=luaTodo

"catch errors caused by wrong parenthesis and wrong curly brackets or
"keywords placed outside their respective blocks

syn region luaParen		transparent start='(' end=')' contains=ALLBUT,luaError,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaCondStart,luaBlock,luaRepeatBlock,luaStatement
syn match   luaError		")"
syn match   luaError		"}"
syn match   luaError		"\<\(end\|else\|elseif\|then\|until\)\>"


"Function declaration
syn region  luaFunctionBlock    transparent matchgroup=luaFunction start="\<function\>" end="\<end\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd

"if then else elseif end
syn keyword luaCond		contained else

syn region  luaCondEnd		contained transparent matchgroup=luaCond start="\<then\>" end="\<end\>" contains=ALLBUT,luaTodo,luaSpecial

syn region  luaCondElseif	contained transparent matchgroup=luaCond start="\<elseif\>" end="\<then\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd

syn region  luaCondStart	transparent matchgroup=luaCond start="\<if\>" end="\<then\>"me=e-4 contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd nextgroup=luaCondEnd skipwhite skipempty



" do end block
syn region  luaBlock		transparent matchgroup=luaStatement start="\<do\>" end="\<end\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd

"repeat until and while do blocks
syn region  luaRepeatBlock	transparent matchgroup=luaRepeat start="\<repeat\>" end="\<until\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd

syn region  luaRepeatBlock	transparent matchgroup=luaRepeat start="\<while\>" end="\<do\>"me=e-2 contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd nextgroup=luaBlock skipwhite skipempty

"other keywords
syn keyword luaStatement	return local
syn keyword luaOperator		and or not
syn keyword luaConstant		nil

"Pre processor
syn match   luaPreProc          "^\s*$\(debug\|nodebug\|if\|ifnot\|end\|else\|endinput\)\>"

"Strings
syn match   luaSpecial		contained "\\[ntr]"
syn region  luaString		start=+'+  end=+'+ skip=+\\\\\|\\'+ contains=luaSpecial
syn region  luaString		start=+"+  end=+"+ skip=+\\\\\|\\"+ contains=luaSpecial
syn region  luaString		start=+\[\[+ end=+\]\]+

"integer number
syn match luaNumber		"\<[0-9]\+\>"
"floating point number, with dot, optional exponent
syn match luaFloat		"\<[0-9]\+\.[0-9]*\(e[-+]\=[0-9]\+\)\=\>"
"floating point number, starting with a dot, optional exponent
syn match luaFloat		"\.[0-9]\+\(e[-+]\=[0-9]\+\)\=\>"
"floating point number, without dot, with exponent
syn match luaFloat		"\<[0-9]\+e[-+]\=[0-9]\+\>"

"tables
syn region  luaTableBlock       transparent matchgroup=luaTable start="{" end="}" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaCondStart,luaBlock,luaRepeatBlock,luaStatement

"internal functions
syn keyword luaInternalFunc  assert call collectgarbage dofile copytagmethods
syn keyword luaInternalFunc  dostring error foreach foreachvar getglobal
syn keyword luaInternalFunc  newtag next nextvar print rawgetglobal
syn keyword luaInternalFunc  rawgettable rawsetglobal rawsettable seterrormethod
syn keyword luaInternalFunc  setglobal settagmethod gettagmethod settag tonumber
syn keyword luaInternalFunc  tostring tag type

"standard libraries
syn keyword luaStdLibFunc    setlocale execute remove rename tmpname
syn keyword luaStdLibFunc    getenv date clock exit debug print_stack
syn keyword luaStdLibFunc    readfrom writeto appendto read write
syn keyword luaStdLibFunc    abs sin cos tan asin
syn keyword luaStdLibFunc    acos atan atan2 ceil floor
syn keyword luaStdLibFunc    mod frexp ldexp sqrt min max log
syn keyword luaStdLibFunc    log10 exp deg rad random
syn keyword luaStdLibFunc    randomseed strlen strsub strlower strupper
syn keyword luaStdLibFunc    strchar strrep ascii strbyte format
syn keyword luaStdLibFunc    strfind gsub


"syncing method
syn sync minlines=100

if !exists("did_lua_syntax_inits")
  let did_lua_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link luaStatement		Statement
  hi link luaRepeat		Repeat
  hi link luaString		String
  hi link luaNumber		Number
  hi link luaFloat		Float
  hi link luaOperator		Operator
  hi link luaConstant		Constant
  hi link luaCond	        Conditional
  hi link luaFunction		Function
  hi link luaComment		Comment
  hi link luaTodo		Todo
  hi link luaTable		Structure
  hi link luaError		Error
  hi link luaSpecial		SpecialChar
  hi link luaPreProc		PreProc
  hi link luaInternalFunc	Identifier
  hi link luaStdLibFunc		Identifier
endif

let b:current_syntax = "lua"

" vim: ts=8
