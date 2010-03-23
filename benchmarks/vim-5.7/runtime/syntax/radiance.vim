" Vim syntax file
" Language:	Radiance Scene Description
" Maintainer:	Georg Mischler <schorsch@schorsch.com>
" Last Change:	01. Nov. 1998

" Radiance is a lighting simulation software package written
" by Gregory Ward-Larson ("the computer artist formerly known
" as Greg Ward"), then at lbnl.
"
" http://radsite.lbl.gov/radiance/HOME.html
"
" Of course, there is also information available about it
" from http://www.schorsch.com/ ;-)


" We take a minimalist approach here, highlighting just
" the essential properties of each object, its type and Id,
" as well as comments, external command names and the
" null-modifier "void".


" Remove any old syntax stuff hanging around
syn clear

" The null-modifier
syn keyword radianceKeyword void

" The different kinds of scene description object types
" Reference types
syn keyword radianceExtraType contained alias instance
" Surface types
syn keyword radianceSurfType contained source
syn keyword radianceSurfType contained ring polygon
syn keyword radianceSurfType contained sphere bubble
syn keyword radianceSurfType contained cone cup
syn keyword radianceSurfType contained cylinder tube
" Emitting material types
syn keyword radianceLightType contained light glow
syn keyword radianceLightType contained illum spotlight
" Material types
syn keyword radianceMatType contained mirror mist
syn keyword radianceMatType contained prism1 prism2
syn keyword radianceMatType contained metal plastic trans
syn keyword radianceMatType contained metal2 plastic2 trans2
syn keyword radianceMatType contained metfunc plasfunc transfunc
syn keyword radianceMatType contained metdata plasdata transdata
syn keyword radianceMatType contained dielectric interface glass
syn keyword radianceMatType contained BRTDfunc antimatter
" Pattern modifier types
syn keyword radiancePatType contained colorfunc brightfunc
syn keyword radiancePatType contained colordata brightdata
syn keyword radiancePatType contained colorpict
syn keyword radiancePatType contained colortext brighttext
" Texture modifier types
syn keyword radianceTexType contained texfunc texdata
" Mixture types
syn keyword radianceMixType contained mixfunc mixdata mixtext

" Each type name is followed by an ID: all up to the next "end of word".
" We need this complicated end of word pattern, because I am to lazy to
" figure out the correct setting of "iskeyword"...
" This doesn't work correctly, if the id is one of the type names of the
" same class (which is legal!), in which case the id will get type color,
" and the int count gets id color.

syn region radianceID start="\<alias\>" end="[^ \t]\($\|\s\)" contains=radianceExtraType
syn region radianceID start="\<instance\>" end="[^ \t]\($\|\s\)" contains=radianceExtraType

syn region radianceID start="\<source\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<ring\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<polygon\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<sphere\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<bubble\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<cone\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<cup\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<cylinder\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType
syn region radianceID start="\<tube\>" end="[^ \t]\($\|\s\)" contains=radianceSurfType

syn region radianceID start="\<light\>" end="[^ \t]\($\|\s\)" contains=radianceLightType
syn region radianceID start="\<glow\>" end="[^ \t]\($\|\s\)" contains=radianceLightType
syn region radianceID start="\<illum\>" end="[^ \t]\($\|\s\)" contains=radianceLightType
syn region radianceID start="\<spotlight\>" end="[^ \t]\($\|\s\)" contains=radianceLightType

syn region radianceID start="\<mirror\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<mist\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<prism1\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<prism2\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<metal\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<plastic\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<trans\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<metal2\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<plastic2\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<trans2\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<metfunc\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<plasfunc\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<transfunc\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<metdata\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<plasdata\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<transdata\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<dielectric\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<interface\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<glass\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<BRTDfunc\>" end="[^ \t]\($\|\s\)" contains=radianceMatType
syn region radianceID start="\<antimatter\>" end="[^ \t]\($\|\s\)" contains=radianceMatType

syn region radianceID start="\<colorfunc\>" end="[^ \t]\($\|\s\)" contains=radiancePatType
syn region radianceID start="\<brightfunc\>" end="[^ \t]\($\|\s\)" contains=radiancePatType
syn region radianceID start="\<colordata\>" end="[^ \t]\($\|\s\)" contains=radiancePatType
syn region radianceID start="\<brightdata\>" end="[^ \t]\($\|\s\)" contains=radiancePatType
syn region radianceID start="\<colorpict\>" end="[^ \t]\($\|\s\)" contains=radiancePatType
syn region radianceID start="\<colortext\>" end="[^ \t]\($\|\s\)" contains=radiancePatType
syn region radianceID start="\<brighttext\>" end="[^ \t]\($\|\s\)" contains=radiancePatType

syn region radianceID start="\<texfunc\>" end="[^ \t]\($\|\s\)" contains=radianceTexType
syn region radianceID start="\<texdata\>" end="[^ \t]\($\|\s\)" contains=radianceTexType

syn region radianceID start="\<mixfunc\>" end="[^ \t]\($\|\s\)" contains=radianceMixType
syn region radianceID start="\<mixdata\>" end="[^ \t]\($\|\s\)" contains=radianceMixType
syn region radianceID start="\<mixtext\>" end="[^ \t]\($\|\s\)" contains=radianceMixType

" external commands (generators, xform et al.)
syn match radianceCommand "^\s*!\s*[^\s]\+\>"

" The usual suspects
syn keyword radianceTodo contained	TODO XXX
syn match radianceComment	"#.*$" contains=radianceTodo


if !exists("did_radiance_syntax_inits")
  let did_radiance_syntax_inits = 1
  " The default methods for highlighting. Can be overridden later
  hi link radianceKeyword	Keyword
  hi link radianceExtraType	Type
  hi link radianceSurfType	Type
  hi link radianceLightType	Type
  hi link radianceMatType	Type
  hi link radiancePatType	Type
  hi link radianceTexType	Type
  hi link radianceMixType	Type
  hi link radianceComment	Comment
  hi link radianceCommand	Function
  hi link radianceID		String
  hi link radianceTodo		Todo
endif

let b:current_syntax = "radiance"

" vim: ts=8
