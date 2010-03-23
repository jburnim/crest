" Vim syntax file
" Language:	SAS
" Maintainer:	Phil Hanna <pehanna@yahoo.com>
" Last Change:	18 May 1999

syn clear

syn case ignore

syn region sasString  start=+"+  skip=+\\\\\|\\"+  end=+"+
syn region sasString  start=+'+  skip=+\\\\\|\\"+  end=+'+

syn match sasNumber  "-\=\<\d*\.\=[0-9_]\>"

syn region sasComment    start="/\*"  end="\*/"
syn match sasComment  "^\s*\*.*;"

syn keyword sasStep           RUN QUIT
syn match   sasStep        "^\s*DATA\s"
syn match   sasStep        "^\s*PROC\s"

syn keyword sasConditional    DO ELSE END IF THEN UNTIL WHILE

syn keyword sasStatement      ABORT ARRAY ATTRIB BY CALL CARDS CARDS4 CATNAME
syn keyword sasStatement      CONTINUE DATALINES DATALINES4 DELETE DISPLAY
syn keyword sasStatement      DM DROP ENDSAS ERROR FILE FILENAME FOOTNOTE
syn keyword sasStatement      FORMAT GOTO INFILE INFORMAT INPUT KEEP
syn keyword sasStatement      LABEL LEAVE LENGTH LIBNAME LINK LIST LOSTCARD
syn keyword sasStatement      MERGE MISSING MODIFY OPTIONS OUTPUT PAGE
syn keyword sasStatement      PUT REDIRECT REMOVE RENAME REPLACE RETAIN
syn keyword sasStatement      RETURN SELECT SET SKIP STARTSAS STOP TITLE
syn keyword sasStatement      UPDATE WAITSAS WHERE WINDOW X

syn match   sasStatement      "FOOTNOTE\d"
syn match   sasStatement      "TITLE\d"

syn match   sasMacro      "\%do"
syn match   sasMacro      "\%else"
syn match   sasMacro      "\%end"
syn match   sasMacro      "\%if"
syn match   sasMacro      "\%let"
syn match   sasMacro      "\%macro"
syn match   sasMacro      "\%mend"
syn match   sasMacro      "\%put"
syn match   sasMacro      "\%then"
syn match   sasMacro      "\%to"
syn match   sasMacro      "\%while"

" SAS Functions

syn keyword sasFunction ABS ADDR AIRY ARCOS ARSIN ATAN ATTRC ATTRN
syn keyword sasFunction BAND BETAINV BLSHIFT BNOT BOR BRSHIFT BXOR
syn keyword sasFunction BYTE CDF CEIL CEXIST CINV CLOSE CNONCT COLLATE
syn keyword sasFunction COMPBL COMPOUND COMPRESS COS COSH CSS CUROBS
syn keyword sasFunction CV DACCDB DACCDBSL DACCSL DACCSYD DACCTAB
syn keyword sasFunction DAIRY DATE DATEJUL DATEPART DATETIME DAY
syn keyword sasFunction DCLOSE DEPDB DEPDBSL DEPDBSL DEPSL DEPSL
syn keyword sasFunction DEPSYD DEPSYD DEPTAB DEPTAB DEQUOTE DHMS
syn keyword sasFunction DIF DIGAMMA DIM DINFO DNUM DOPEN DOPTNAME
syn keyword sasFunction DOPTNUM DREAD DROPNOTE DSNAME ERF ERFC EXIST
syn keyword sasFunction EXP FAPPEND FCLOSE FCOL FDELETE FETCH FETCHOBS
syn keyword sasFunction FEXIST FGET FILEEXIST FILENAME FILEREF FINFO
syn keyword sasFunction FINV FIPNAME FIPNAMEL FIPSTATE FLOOR FNONCT
syn keyword sasFunction FNOTE FOPEN FOPTNAME FOPTNUM FPOINT FPOS
syn keyword sasFunction FPUT FREAD FREWIND FRLEN FSEP FUZZ FWRITE
syn keyword sasFunction GAMINV GAMMA GETOPTION GETVARC GETVARN HBOUND
syn keyword sasFunction HMS HOSTHELP HOUR IBESSEL INDEX INDEXC
syn keyword sasFunction INDEXW INPUT INPUTC INPUTN INT INTCK INTNX
syn keyword sasFunction INTRR IRR JBESSEL JULDATE KURTOSIS LAG LBOUND
syn keyword sasFunction LEFT LENGTH LGAMMA LIBNAME LIBREF LOG LOG10
syn keyword sasFunction LOG2 LOGPDF LOGPMF LOGSDF LOWCASE MAX MDY
syn keyword sasFunction MEAN MIN MINUTE MOD MONTH MOPEN MORT N
syn keyword sasFunction NETPV NMISS NORMAL NOTE NPV OPEN ORDINAL
syn keyword sasFunction PATHNAME PDF PEEK PEEKC PMF POINT POISSON POKE
syn keyword sasFunction PROBBETA PROBBNML PROBCHI PROBF PROBGAM
syn keyword sasFunction PROBHYPR PROBIT PROBNEGB PROBNORM PROBT PUT
syn keyword sasFunction PUTC PUTN QTR QUOTE RANBIN RANCAU RANEXP
syn keyword sasFunction RANGAM RANGE RANK RANNOR RANPOI RANTBL RANTRI
syn keyword sasFunction RANUNI REPEAT RESOLVE REVERSE REWIND RIGHT
syn keyword sasFunction ROUND SAVING SCAN SDF SECOND SIGN SIN SINH
syn keyword sasFunction SKEWNESS SOUNDEX SPEDIS SQRT STD STDERR STFIPS
syn keyword sasFunction STNAME STNAMEL SUBSTR SUM SYMGET SYSGET SYSMSG
syn keyword sasFunction SYSPROD SYSRC SYSTEM TAN TANH TIME TIMEPART
syn keyword sasFunction TINV TNONCT TODAY TRANSLATE TRANWRD TRIGAMMA
syn keyword sasFunction TRIM TRIMN TRUNC UNIFORM UPCASE USS VAR
syn keyword sasFunction VARFMT VARINFMT VARLABEL VARLEN VARNAME
syn keyword sasFunction VARNUM VARRAY VARRAYX VARTYPE VERIFY VFORMAT
syn keyword sasFunction VFORMATD VFORMATDX VFORMATN VFORMATNX VFORMATW
syn keyword sasFunction VFORMATWX VFORMATX VINARRAY VINARRAYX VINFORMAT
syn keyword sasFunction VINFORMATD VINFORMATDX VINFORMATN VINFORMATNX
syn keyword sasFunction VINFORMATW VINFORMATWX VINFORMATX VLABEL
syn keyword sasFunction VLABELX VLENGTH VLENGTHX VNAME VNAMEX VTYPE
syn keyword sasFunction VTYPEX WEEKDAY YEAR YYQ ZIPFIPS ZIPNAME ZIPNAMEL
syn keyword sasFunction ZIPSTATE

" End of SAS Functions

if !exists("did_sas_syntax_inits")
  let did_sas_syntax_inits = 1
  hi link sasComment            Comment
  hi link sasConditional        Statement
  hi link sasStep               Statement
  hi link sasFunction           Function
  hi link sasMacro              PreProc
  hi link sasNumber             Number
  hi link sasStatement          Statement
  hi link sasString             String
endif

let b:current_syntax = "sas"

" vim: ts=8
