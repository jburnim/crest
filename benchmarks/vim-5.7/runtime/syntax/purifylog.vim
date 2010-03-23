" Vim syntax file
" Language:	purify log files
" Maintainer:	Gautam H. Mudunuri <gmudunur@informatica.com>
" Last Change:	1999 Jun 14

" clear any unwanted syntax defs
syn clear

" Purify header
syn match purifyLogHeader      "^\*\*\*\*.*$"

" Informational messages
syn match purifyLogFIU "^FIU:.*$"
syn match purifyLogMAF "^MAF:.*$"
syn match purifyLogMIU "^MIU:.*$"
syn match purifyLogSIG "^SIG:.*$"
syn match purifyLogWPF "^WPF:.*$"
syn match purifyLogWPM "^WPM:.*$"
syn match purifyLogWPN "^WPN:.*$"
syn match purifyLogWPR "^WPR:.*$"
syn match purifyLogWPW "^WPW:.*$"
syn match purifyLogWPX "^WPX:.*$"

" Warning messages
syn match purifyLogABR "^ABR:.*$"
syn match purifyLogBSR "^BSR:.*$"
syn match purifyLogBSW "^BSW:.*$"
syn match purifyLogFMR "^FMR:.*$"
syn match purifyLogMLK "^MLK:.*$"
syn match purifyLogMSE "^MSE:.*$"
syn match purifyLogPAR "^PAR:.*$"
syn match purifyLogPLK "^PLK:.*$"
syn match purifyLogSBR "^SBR:.*$"
syn match purifyLogSOF "^SOF:.*$"
syn match purifyLogUMC "^UMC:.*$"
syn match purifyLogUMR "^UMR:.*$"

" Corrupting messages
syn match purifyLogABW "^ABW:.*$"
syn match purifyLogBRK "^BRK:.*$"
syn match purifyLogFMW "^FMW:.*$"
syn match purifyLogFNH "^FNH:.*$"
syn match purifyLogFUM "^FUM:.*$"
syn match purifyLogMRE "^MRE:.*$"
syn match purifyLogSBW "^SBW:.*$"

" Fatal messages
syn match purifyLogCOR "^COR:.*$"
syn match purifyLogNPR "^NPR:.*$"
syn match purifyLogNPW "^NPW:.*$"
syn match purifyLogZPR "^ZPR:.*$"
syn match purifyLogZPW "^ZPW:.*$"

if !exists("did_purifyLog_syntax_inits")
	let did_purifyLog_syntax_inits = 1

	" The default methods for highlighting.  Can be overridden later

	hi link purifyLogFIU purifyLogInformational
	hi link purifyLogMAF purifyLogInformational
	hi link purifyLogMIU purifyLogInformational
	hi link purifyLogSIG purifyLogInformational
	hi link purifyLogWPF purifyLogInformational
	hi link purifyLogWPM purifyLogInformational
	hi link purifyLogWPN purifyLogInformational
	hi link purifyLogWPR purifyLogInformational
	hi link purifyLogWPW purifyLogInformational
	hi link purifyLogWPX purifyLogInformational

	hi link purifyLogABR purifyLogWarning
	hi link purifyLogBSR purifyLogWarning
	hi link purifyLogBSW purifyLogWarning
	hi link purifyLogFMR purifyLogWarning
	hi link purifyLogMLK purifyLogWarning
	hi link purifyLogMSE purifyLogWarning
	hi link purifyLogPAR purifyLogWarning
	hi link purifyLogPLK purifyLogWarning
	hi link purifyLogSBR purifyLogWarning
	hi link purifyLogSOF purifyLogWarning
	hi link purifyLogUMC purifyLogWarning
	hi link purifyLogUMR purifyLogWarning

	hi link purifyLogABW purifyLogCorrupting
	hi link purifyLogBRK purifyLogCorrupting
	hi link purifyLogFMW purifyLogCorrupting
	hi link purifyLogFNH purifyLogCorrupting
	hi link purifyLogFUM purifyLogCorrupting
	hi link purifyLogMRE purifyLogCorrupting
	hi link purifyLogSBW purifyLogCorrupting

	hi link purifyLogCOR purifyLogFatal
	hi link purifyLogNPR purifyLogFatal
	hi link purifyLogNPW purifyLogFatal
	hi link purifyLogZPR purifyLogFatal
	hi link purifyLogZPW purifyLogFatal

	hi link purifyLogHeader        Comment
	hi link purifyLogInformational PreProc
	hi link purifyLogWarning       Type
	hi link purifyLogCorrupting    Error
	hi link purifyLogFatal	       Error
endif

let b:current_syntax = "purifylog"

" vim:ts=8
