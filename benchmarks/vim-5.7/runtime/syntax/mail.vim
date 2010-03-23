" Vim syntax file
" Language:	Mail file
" Maintainer:	Felix von Leitner <leitner@math.fu-berlin.de>
" Last Change:	1999 Aug 27

" Remove any old syntax stuff hanging around
syn clear

" The mail header is recognized starting with a "keyword:" line and ending
" with an empty line or other line that can't be in the header.
" All lines of the header are highlighted
" For "From " matching case is required, not for the rest.
syn region	mailHeader	start="^From " skip="^[ \t]" end="^[-A-Za-z0-9/]*[^-A-Za-z0-9/:]"me=s-1 end="^[^:]*$"me=s-1 end="^---*" contains=mailHeaderKey,mailSubject

syn case ignore

syn region	mailHeader	start="^\(Newsgroups:\|From:\|To:\|Cc:\|Bcc:\|Reply-To:\|Subject:\|Return-Path:\|Received:\|Date:\|Replied:\)" skip="^[ \t]" end="^[-a-z0-9/]*[^-a-z0-9/:]"me=s-1 end="^[^:]*$"me=s-1 end="^---*" contains=mailHeaderKey,mailSubject

syn region	mailHeaderKey	contained start="^\(From\|To\|Cc\|Bcc\|Reply-To\).*" skip=",$" end="$" contains=mailEmail
syn match	mailHeaderKey	contained "^Date"

syn match	mailSubject	contained "^Subject.*"

syn match	mailEmail	contained "[_=a-z\.+A-Z0-9-]\+@[a-zA-Z0-9\./\-]\+"
syn match	mailEmail	contained "<.\{-}>"

syn region	mailSignature	start="^-- *$" end="^$"

" even and odd quoted lines
" removed ':', it caused too many bogus highlighting
" order is imporant here!
syn match	mailQuoted1	"^\([A-Za-z]\+>\|[]|}>]\).*$"
syn match	mailQuoted2	"^\(\([A-Za-z]\+>\|[]|}>]\)[ \t]*\)\{2}.*$"
syn match	mailQuoted3	"^\(\([A-Za-z]\+>\|[]|}>]\)[ \t]*\)\{3}.*$"
syn match	mailQuoted4	"^\(\([A-Za-z]\+>\|[]|}>]\)[ \t]*\)\{4}.*$"
syn match	mailQuoted5	"^\(\([A-Za-z]\+>\|[]|}>]\)[ \t]*\)\{5}.*$"
syn match	mailQuoted6	"^\(\([A-Za-z]\+>\|[]|}>]\)[ \t]*\)\{6}.*$"

" Need to sync on the header.  Assume we can do that within a hundred lines
syn sync lines=100

if !exists("did_mail_syntax_inits")
  let did_mail_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link mailHeaderKey		Type
  hi link mailHeader		Statement
  hi link mailQuoted1		Comment
  hi link mailQuoted3		Comment
  hi link mailQuoted5		Comment
  hi link mailQuoted2		Identifier
  hi link mailQuoted4		Identifier
  hi link mailQuoted6		Identifier
  hi link mailSignature		PreProc
  hi link mailEmail		Special
  hi link mailSubject		String
endif

let b:current_syntax = "mail"

" vim: ts=8
