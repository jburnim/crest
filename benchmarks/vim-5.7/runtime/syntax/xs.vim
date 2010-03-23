" Vim syntax file
" Language:	XS (Perl extension interface language)
" Maintainer:	Michael W. Dodge <sarge@pobox.com>
" Last Change:	1999 Mar 08

" Remove any old syntax stuff hanging around
syn clear

" Read the C syntax to start with
source <sfile>:p:h/c.vim

" XS extentions
" TODO: Figure out how to look for trailing '='.
syn keyword xsKeyword	MODULE PACKAGE PREFIX
syn keyword xsKeyword	OUTPUT: CODE: INIT: PREINIT: INPUT:
syn keyword xsKeyword	PPCODE: REQUIRE: CLEANUP: BOOT:
syn keyword xsKeyword	VERSIONCHECK: PROTOTYPES: PROTOTYPE:
syn keyword xsKeyword	ALIAS: INCLUDE: CASE:
" TODO: Figure out how to look for trailing '('.
syn keyword xsMacro	SV EXTEND PUSHs
syn keyword xsVariable	RETVAL NO_INIT
"syn match xsCast	"\<\(const\|static\|dynamic\|reinterpret\)_cast\s*<"me=e-1
"syn match xsCast	"\<\(const\|static\|dynamic\|reinterpret\)_cast\s*$"

if !exists("did_xs_syntax_inits")
  let did_xs_syntax_inits = 1
  hi link xsKeyword	Keyword
  hi link xsMacro	Macro
  hi link xsVariable	Identifier
endif

let b:current_syntax = "xs"

" vim: ts=8
