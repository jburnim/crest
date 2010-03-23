" Vim syntax file
" Language:	sendmail
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	June 29, 1999

" Remove any old syntax stuff hanging around
syn clear

" Comments
syn match smComment	"^#.*$"

" Operators
syn match smOper	"$"

" Definitions, Classes, Files, Options, Precedence, Trusted Users, Mailers
syn match smDefine	"^[CDFPT]."
syn match smDefine	"^O[AaBcdDeFfgHiLmNoQqrSsTtuvxXyYzZ]"
syn match smDefine	"^O\s"he=e-1
syn match smDefine	"^M[a-zA-Z0-9]\+,"he=e-1

" Header Format  H?list-of-mailer-flags?name: format
syn match smHeaderSep contained "[?:]"
syn match smHeader	"^H\(?[a-zA-Z]\+?\)\=[-a-zA-Z_]\+:" contains=smHeaderSep

" Variables
syn match smVar		"\$[a-z\.\|]"

" Rulesets
syn match smRuleset	"^S\d*"

" Rewriting Rules
syn match smRewrite	"^R"			skipwhite skipnl nextgroup=smRewriteLhsToken,smRewriteLhsUser

syn match smRewriteLhsUser	contained "[^\t$]\+"		skipwhite skipnl nextgroup=smRewriteLhsToken,smRewriteLhsSep
syn match smRewriteLhsToken	contained "\(\$[-*+]\|\$[-=][A-Za-z]\)\+"	skipwhite skipnl nextgroup=smRewriteLhsUser,smRewriteLhsSep

syn match smRewriteLhsSep	contained "\t\+"			skipwhite skipnl nextgroup=smRewriteRhsToken,smRewriteRhsUser

syn match smRewriteRhsUser	contained "[^\t$]\+"		skipwhite skipnl nextgroup=smRewriteRhsToken,smRewriteRhsSep
syn match smRewriteRhsToken	contained "\(\$\d\|\$>\d\|\$#\|\$@\|\$:[-_a-zA-Z]\+\|\$[[\]]\|\$@\|\$:\|\$[A-Za-z]\)\+" skipwhite skipnl nextgroup=smRewriteRhsUser,smRewriteRhsSep

syn match smRewriteRhsSep	contained "\t\+"			skipwhite skipnl nextgroup=smRewriteComment,smRewriteRhsSep
syn match smRewriteRhsSep	contained "$"

syn match smRewriteComment	contained "[^\t$]*$"

" Clauses
syn match smClauseError		"\$\."
syn match smElse		contained	"\$|"
syn match smClauseCont	contained	"^\t"
syn region smClause	matchgroup=Delimiter start="\$?." matchgroup=Delimiter end="\$\." contains=smElse,smClause,smVar,smClauseCont

if !exists("did_sm_syntax_inits")
  let did_sm_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link smClause	Special
  hi link smClauseError	Error
  hi link smComment	Comment
  hi link smDefine	Statement
  hi link smElse		Delimiter
  hi link smHeader	Statement
  hi link smHeaderSep	String
  hi link smRewrite	Statement
  hi link smRewriteComment	Comment
  hi link smRewriteLhsToken	String
  hi link smRewriteLhsUser	Statement
  hi link smRewriteRhsToken	String
  hi link smRuleset	Statement
  hi link smVar		String
  endif

let b:current_syntax = "sm"

" vim: ts=18
