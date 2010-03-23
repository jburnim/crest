" Vim syntax file
" Language:	SMIL (Synchronized Multimedia Integration Language)
" Maintainer:	Herve Foucher <Herve.Foucher@helio.org>
" URL:		http://www.helio.org/vim/syntax/smil.vim
" Last Change:	1999 Sep 02

" To learn more about SMIL, please refer to http://www.w3.org/AudioVideo/
" and to http://www.helio.org/products/smil/tutorial/

syn clear
" SMIL is case sensitive
syn case match

" illegal characters
syn match smilError "[<>&]"
syn match smilError "[()&]"

if !exists("main_syntax")
  let main_syntax = 'smil'
endif

" tags
syn match   smilSpecial  contained "\\\d\d\d\|\\."
syn match   smilSpecial  contained "("
syn match   smilSpecial  contained "id("
syn match   smilSpecial  contained ")"
syn keyword smilSpecial  contained remove freeze true false on off overdub caption new pause replace
syn keyword smilSpecial  contained first last
syn keyword smilSpecial  contained fill meet slice scroll hidden
syn region  smilString   contained start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=smilSpecial
syn region  smilString   contained start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=smilSpecial
syn match   smilValue    contained "=[\t ]*[^'" \t>][^ \t>]*"hs=s+1
syn region  smilEndTag             start=+</+    end=+>+              contains=smilTagN,smilTagError
syn region  smilTag                start=+<[^/]+ end=+>+              contains=smilTagN,smilString,smilArg,smilValue,smilTagError,smilEvent,smilCssDefinition
syn match   smilTagN     contained +<\s*[-a-zA-Z0-9]\++ms=s+1 contains=smilTagName,smilSpecialTagName
syn match   smilTagN     contained +</\s*[-a-zA-Z0-9]\++ms=s+2 contains=smilTagName,smilSpecialTagName
syn match   smilTagError contained "[^>]<"ms=s+1

" tag names
syn keyword smilTagName contained smil head body anchor a switch region layout meta
syn match   smilTagName contained "root-layout"
syn keyword smilTagName contained par seq
syn keyword smilTagName contained animation video img audio ref text textstream
syn match smilTagName contained "\<\(head\|body\)\>"


" legal arg names
syn keyword smilArg contained dur begin end href target id coords show title abstract author copyright alt
syn keyword smilArg contained left top width height fit src name content fill longdesc repeat type
syn match   smilArg contained "z-index"
syn match   smilArg contained " end-sync"
syn match   smilArg contained " region"
syn match   smilArg contained "background-color"
syn match   smilArg contained "system-bitrate"
syn match   smilArg contained "system-captions"
syn match   smilArg contained "system-overdub-or-caption"
syn match   smilArg contained "system-language"
syn match   smilArg contained "system-required"
syn match   smilArg contained "system-screen-depth"
syn match   smilArg contained "system-screen-size"
syn match   smilArg contained "clip-begin"
syn match   smilArg contained "clip-end"
syn match   smilArg contained "skip-content"


" SMIL Boston ext.
" This are new SMIL functionnalities seen on www.w3.org on August 3rd 1999

" Animation
syn keyword smilTagName contained animate set move
syn keyword smilArg contained calcMode from to by additive values origin path
syn keyword smilArg contained accumulate hold attribute
syn match   smilArg contained "xml:link"
syn keyword smilSpecial contained discrete linear spline parent layout
syn keyword smilSpecial contained top left simple

" Linking
syn keyword smilTagName contained area
syn keyword smilArg contained actuate behavior inline sourceVolume
syn keyword smilArg contained destinationVolume destinationPlaystate tabindex
syn keyword smilArg contained class style lang dir onclick ondblclick onmousedown onmouseup onmouseover onmousemove onmouseout onkeypress onkeydown onkeyup shape nohref accesskey onfocus onblur
syn keyword smilSpecial contained play pause stop rect circ poly child par seq

" Media Object
syn keyword smilTagName contained rtpmap
syn keyword smilArg contained port transport encoding payload clipBegin clipEnd
syn match   smilArg contained "fmt-list"

" Timing and Synchronization
syn keyword smilTagName contained excl
syn keyword smilArg contained beginEvent endEvent eventRestart endSync repeatCount repeatDur
syn keyword smilArg contained syncBehavior syncTolerance
syn keyword smilSpecial contained canSlip locked

" special characters
syn match smilSpecialChar "&[^;]*;"

if exists("smil_wrong_comments")
  syn region smilComment                start=+<!--+      end=+-->+
else
  syn region smilComment                start=+<!+        end=+>+   contains=smilCommentPart,smilCommentError
  syn match  smilCommentError contained "[^><!]"
  syn region smilCommentPart  contained start=+--+        end=+--+
endif
syn region smilComment                start=+<!DOCTYPE+ keepend end=+>+

if !exists("did_smil_syntax_inits")
  let did_smil_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link smilTag			Function
  hi link smilEndTag			Identifier
  hi link smilArg			Type
  hi link smilTagName			smilStatement
  hi link smilSpecialTagName		Exception
  hi link smilValue			Value
  hi link smilSpecialChar		Special

  hi link smilSpecial			Special
  hi link smilSpecialChar		Special
  hi link smilString			String
  hi link smilStatement			Statement
  hi link smilComment			Comment
  hi link smilCommentPart		Comment
  hi link smilPreProc			PreProc
  hi link smilValue			String
  hi link smilCommentError		smilError
  hi link smilTagError			smilError
  hi link smilError			Error
endif

let b:current_syntax = "smil"

if main_syntax == 'smil'
  unlet main_syntax
endif

" vim: ts=8
