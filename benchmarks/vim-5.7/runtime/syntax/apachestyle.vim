" Vim syntax file
" Language:	Apache-style config files (Apache, ProFTPd, etc)
" Maintainer:	Christian Hammers <ch@westend.com>
" URL:		none
" Last Change:	1999 Oct 28

" Apache-style config files look this way:
"
" Option	value
" Option	value1 value2
" <Section Name?>
"	Option	value
"	<SubSection Name?>
"	# Comments...
"	</SubSection>
" </Section>

" Remove any old syntax stuff hanging around
syn clear
syn case ignore

" specials
syn match  apachestyleComment	"^\s*#.*"

" options and values
syn region apachestyleOption	start="^\s*[A-Za-z]" end="$\|\s"

" tags
syn region apachestyleTag	start=+<+ end=+>+ contains=apachestyleTagN,apachestyleTagError
syn match  apachestyleTagN	contained +<[/\s]*[-a-zA-Z0-9]\++ms=s+1
syn match  apachestyleTagError	contained "[^>]<"ms=s+1


if !exists("did_apachestyle_syntax_inits")
  let did_apachestyle_syntax_inits = 1

  hi link apachestyleComment			Comment
  hi link apachestyleOption			String
  hi link apachestyleTag			Special
  hi link apachestyleTagN			Identifier
  hi link apachestyleTagError			Error
endif

let b:current_syntax = "apachestyle"
" vim: ts=8
