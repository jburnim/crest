" Vim syntax file
" Language:	X Pixmap v2
" Maintainer:	Steve Wall (steve_wall@redcom.com)
" Last Change:	1999 Oct 04
" Version:	5.5
"
" Made from xpm.vim by Ronald Schild <rs@scutum.de>

" Remove any old syntax stuff hanging around
syn clear

syn region  xpm2PixelString	start="^"  end="$"  contains=@xpm2Colors
syn keyword xpm2Todo		TODO FIXME XXX  contained
syn match   xpm2Comment		"\!.*$"  contains=xpm2Todo


if has("gui_running")

let color  = ""
let chars  = ""
let colors = 0
let cpp    = 0
let n      = 0
let i      = 1

while i <= line("$")		" scanning all lines

   let s = getline(i)
   if match(s,"\!.*$") != -1
      let s = matchstr(s, "^[^\!]*")
   endif
   if s != ""			" does line contain a string?

      if n == 0			" first string is the Values string

	 " get the 3rd value: colors = number of colors
	 let colors = substitute(s, '\s*\d\+\s\+\d\+\s\+\(\d\+\).*', '\1', '')
	 " get the 4th value: cpp = number of character per pixel
	 let cpp = substitute(s, '\s*\d\+\s\+\d\+\s\+\d\+\s\+\(\d\+\).*', '\1', '')

	 " highlight the Values string as normal string (no pixel string)
	 exe 'syn match xpm2Values /'.s.'/'
	 hi link xpm2Values Statement

	 let n = 1		" n = color index

      elseif n <= colors	" string is a color specification

	 " get chars = <cpp> length string representing the pixels
	 " (first incl. the following whitespace)
	 let chars = substitute(s, '\(.\{'.cpp.'}\s\).*', '\1', '')

	 " now get color, first try 'c' key if any (color visual)
	 let color = substitute(s, '.*\sc\s\+\(.\{-}\)\s*\(\(g4\=\|[ms]\)\s.*\)*\s*', '\1', '')
	 if color == s
	    " no 'c' key, try 'g' key (grayscale with more than 4 levels)
	    let color = substitute(s, '.*\sg\s\+\(.\{-}\)\s*\(\(g4\|[ms]\)\s.*\)*\s*', '\1', '')
	    if color == s
	       " next try: 'g4' key (4-level grayscale)
	       let color = substitute(s, '.*\sg4\s\+\(.\{-}\)\s*\([ms]\s.*\)*\s*', '\1', '')
	       if color == s
		  " finally try 'm' key (mono visual)
		  let color = substitute(s, '.*\sm\s\+\(.\{-}\)\s*\(s\s.*\)*\s*', '\1', '')
		  if color == s
		     let color = ""
		  endif
	       endif
	    endif
	 endif

	 " Vim cannot handle RGB codes with more than 6 hex digits
	 if color =~ '#\x\{10,}$'
	    let color = substitute(color, '\(\x\x\)\x\x', '\1', 'g')
	 elseif color =~ '#\x\{7,}$'
	    let color = substitute(color, '\(\x\x\)\x', '\1', 'g')
	 " nor with 3 digits
	 elseif color =~ '#\x\{3}$'
	    let color = substitute(color, '\(\x\)\(\x\)\(\x\)', '0\10\20\3', '')
	 endif

	 " escape meta characters in patterns
	 let s = escape(s, '/\*^$.~[]')
	 let chars = escape(chars, '/\*^$.~[]')

	 " now create syntax items
	 " highlight the color string as normal string (no pixel string)
	 exe 'syn match xpm2Col'.n.'Def /'.s.'/ contains=xpm2Col'.n.'inDef'
	 exe 'hi link xpm2Col'.n.'Def Constant'

	 " but highlight the first whitespace after chars in its color
	 exe 'syn match xpm2Col'.n.'inDef /"'.chars.'/hs=s+'.(cpp+1).' contained'
	 exe 'hi link xpm2Col'.n.'inDef xpm2Color'.n

	 " remove the following whitespace from chars
	 let chars = substitute(chars, '.$', '', '')

	 " and create the syntax item contained in the pixel strings
	 exe 'syn match xpm2Color'.n.' /'.chars.'/ contained'
	 exe 'syn cluster xpm2Colors add=xpm2Color'.n

	 " if no color or color = "None" show background
	 if color == ""  ||  substitute(color, '.*', '\L&', '') == 'none'
	    exe 'hi xpm2Color'.n.' guifg=bg guibg=NONE'
	 else
	    exe 'hi xpm2Color'.n." guifg='".color."' guibg='".color."'"
	 endif
	 let n = n + 1
      else
	 break		" no more color string
      endif
   endif
   let i = i + 1
endwhile

unlet color chars colors cpp n i s

endif		" has("gui_running")

if !exists("did_xpm2_syntax_inits")
   let did_xpm2_syntax_inits = 1
   hi link xpm2Type		Type
   hi link xpm2StorageClass	StorageClass
   hi link xpm2Todo		Todo
   hi link xpm2Comment		Comment
   hi link xpm2PixelString	String
endif

let b:current_syntax = "xpm2"

" vim: ts=8:sw=3:noet:
