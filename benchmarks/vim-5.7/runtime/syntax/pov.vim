" Vim syntax file
" Language:	povray screen description language
" Maintainer:	T. Scott Urban <urban@blarg.net>
" Last Change:	1999 Jun 14

" Remove any old syntax stuff hanging around
syn clear

" A bunch of useful Pov keywords
syn keyword povCommands		background camera global_settings light_source
syn keyword povObjects		atmosphere bicubic_patch blob box cone cylinder difference height_field intersection lathe merge mesh object plane polygon prism sor sphere superellipsoid text torus triangle union
syn keyword povTypes		x y z hf_gray_16
syn keyword povModifiers	rotate scale translate
syn keyword povDescriptors	adaptive ambient ambient_settings angle aperture area_light assumed_gamma blue blur_samples brick checker color color_map conic_sweep cubic_spline diffuse fade_distance fade_power falloff finish focal_point frequency green hexagon linear_spline linear_sweep location look_at looks_like max_trace_level metallic no_shadow normal open orthographic phong pigment quadratic_spline radius red reflection rgb roughness shadowless specular spotlight texture tga threshold tightness ttf turbulence u_step v_step wrinkles

syn keyword povConditional	if ifdef else switch

syn keyword povTodo contained	TODO FIXME XXX

" comments
syn region povComment	start="/\*" end="\*/" contains=povTodo
syn match  povComment	"//.*" contains=povTodo
syn match  povCommentError	"\*/"
syn sync ccomment povComment

" include statements
syn region povIncluded contained start=+"+ skip=+\\\\\|\\"+ end=+"+
syn match povIncluded contained "<[^>]*>"
syn match povInclude	"^#\s*include\>\s*["<]" contains=povIncluded

syn keyword povLabel		case default
syn keyword povRepeat		while for do

" String and Character constants
" Highlight special characters (those which have a backslash) differently
syn match   povSpecial contained "\\\d\d\d\|\\."
syn region  povString		  start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=povSpecial
syn match   povCharacter	  "'[^\\]'"
syn match   povSpecialCharacter  "'\\.'"

"catch errors caused by wrong parenthesis
syn region	povParen		transparent start='(' end=')' contains=ALLBUT,povParenError,povIncluded,povSpecial,povTodo,povUserLabel
syn match	povParenError	")"
syn match	povInParen	contained "[{}]"
hi link povParenError povError
hi link povInParen povError

"integer number, or floating point number without a dot and with "f".
syn case ignore
syn match  povNumber		"\<\d\+\(u\=l\=\|lu\|f\)\>"
"floating point number, with dot, optional exponent
syn match  povFloat		"\<\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\=\>"
"floating point number, starting with a dot, optional exponent
syn match  povFloat		"\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
"floating point number, without dot, with exponent
syn match  povFloat		"\<\d\+e[-+]\=\d\+[fl]\=\>"
"hex number
syn match  povNumber		"0x[0-9a-f]\+\(u\=l\=\|lu\)\>"
"syn match  cIdentifier	"\<[a-z_][a-z0-9_]*\>"
syn case match

syn keyword povType		int long short char void size_t
syn keyword povType		signed unsigned float double
syn keyword povStructure	struct union enum typedef
syn keyword povStorageClass	static register auto volatile extern const

syn region povPreCondit	start="^[ \t]*#[ \t]*\(if\>\|ifdef\>\|ifndef\>\|elif\>\|else\>\|endif\>\)"  skip="\\$"  end="$" contains=povComment,povString,povCharacter,povNumber,povCommentError
"syn match  cLineSkip	"\\$"
syn region povDefine		start="^[ \t]*#[ \t]*\(define\>\|undef\>\)" skip="\\$" end="$" contains=ALLBUT,povPreCondit,povIncluded,povInclude,povDefine,povInParen
syn region povPreProc		start="^[ \t]*#[ \t]*\(pragma\>\|line\>\|warning\>\|warn\>\|error\>\)" skip="\\$" end="$" contains=ALLBUT,povPreCondit,povIncluded,povInclude,povDefine,povInParen

" Highlight User Labels
syn region	povMulti		transparent start='?' end=':' contains=ALLBUT,povIncluded,povSpecial,povTodo,povUserLabel
" Avoid matching foo::bar() in C++ by requiring that the next char is not ':'
syn match	povUserLabel	"^[ \t]*[a-zA-Z0-9_]\+[ \t]*:$"
syn match	povUserLabel	";[ \t]*[a-zA-Z0-9_]\+[ \t]*:$"
syn match	povUserLabel	"^[ \t]*[a-zA-Z0-9_]\+[ \t]*:[^:]"me=e-1
syn match	povUserLabel	";[ \t]*[a-zA-Z0-9_]\+[ \t]*:[^:]"me=e-1

syn sync ccomment povComment minlines=10

if !exists("did_pov_syntax_inits")
  let did_pov_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later

  " comment
  hi link povComment		Comment
  " constants
  hi link povFloat		Float
  hi link povNumber		Number
  hi link povString		String
  hi link povCharacter		Character
  hi link povSpecialCharacter	povSpecial
  " identifier
  hi link povCommands		Function
  " statements
  hi link povStatement		Statement
  hi link povConditional	Conditional
  hi link povRepeat		Repeat
  hi link povLabel		Label
  hi link povUserLabel		Label
  hi link povOperator		Operator
  " preproc
  hi link povPreProc		PreProc
  hi link povInclude		Include
  hi link povIncluded		povString
  hi link povDefine		Define
  hi link povPreCondit		PreCondit
  " type
  hi link povType		Type
  hi link povStorageClass	StorageClass
  hi link povStructure		Structure
  hi link povDescriptors	Type
  " special
  hi link povSpecial		Special
  " error
  hi link povError		Error
  hi link povCommentError	povError
  " todo
  hi link povTodo		Todo

  hi link povObjects		Function
  hi link povModifiers		Operator
  hi link povTypes		Type
endif

let b:current_syntax = "pov"

" vim: ts=8
