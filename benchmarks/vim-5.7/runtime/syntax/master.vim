" Vim syntax file
" Language:	Focus Master File
" Maintainer:	Rob Brady <robb@datatone.com>
" Last Change:	$Date: 1999/08/13 06:51:26 $
" URL: http://www.datatone.com/~robb/vim/syntax/master.vim
" $Revision: 1.4 $

" this is a very simple syntax file - I will be improving it
" add entire DEFINE syntax

" Remove any old syntax stuff hanging around
syn clear

syn case match

" A bunch of useful keywords
syn keyword masterKeyword	FILENAME SUFFIX SEGNAME SEGTYPE PARENT FIELDNAME
syn keyword masterKeyword	FIELD ALIAS USAGE INDEX MISSING ON
syn keyword masterKeyword	FORMAT CRFILE CRKEY
syn keyword masterDefine	DEFINE DECODE EDIT
syn region  masterString	start=+"+  end=+"+
syn region  masterString	start=+'+  end=+'+
syn match   masterComment	"\$.*"

if !exists("did_master_syntax_inits")
  let did_master_syntax_inits = 1
  hi link masterKeyword Keyword
  hi link masterComment Comment
  hi link masterString  String
endif

let b:current_syntax = "master"

" vim: ts=8
