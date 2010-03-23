" Vim syntax file
" Language:	Microsoft VBScript Web Content (ASP)
" Maintainer:	Devin Weaver <ktohg@tritarget.com>
" URL:		http://www.tritarget.com/vim/syntax
" Last Change:	2000 May 01

" Remove any old syntax stuff hanging around
syn clear

if !exists("main_syntax")
  let main_syntax = 'aspvbs'
endif

so <sfile>:p:h/html.vim

syn cluster htmlPreProc add=AspVBScriptInsideHtmlTags

" Functions and methods that are in VB but will cause errors in an ASP page
" This is helpfull if your porting VB code to ASP
" I removed (Count, Item) because these are common variable names in AspVBScript
syn keyword AspVBSError contained Val Str CVar CVDate DoEvents GoSub Return GoTo
syn keyword AspVBSError contained Date Time Timer Stop LinkExecute
syn keyword AspVBSError contained Add With Type LinkPoke
syn keyword AspVBSError contained LinkRequest LinkSend Declare New Optional Sleep
syn keyword AspVBSError contained ParamArray Static Erl TypeOf Like LSet RSet Mid StrConv
" It may seem that most of these can fit into a keyword clause but keyword takes
" priority over all so I can't get the multi-word matches
syn match AspVBSError contained "\<Def[a-zA-Z0-9_]\+\>"
syn match AspVBSError contained "^\s*Open"
syn match AspVBSError contained "Debug\.[a-zA-Z0-9_]*"
syn match AspVBSError contained "^\s*[a-zA-Z0-9_]\+:"
syn match AspVBSError contained "[a-zA-Z0-9_]\+![a-zA-Z0-9_]\+"
syn match AspVBSError contained "^\s*#.*$"
syn match AspVBSError contained "\<As\s\+[a-zA-Z0-9_]*"
syn match AspVBSError contained "\<End\>\|\<Exit\>"
syn match AspVBSError contained "\<On\s\+Error\>\|\<On\>\|\<Error\>\|\<Resume\s\+Next\>\|\<Resume\>"
syn match AspVBSError contained "\<Option\s\+\(Base\|Compare\|Private\s\+Module\)\>"
syn match AspVBSError contained "\<Property\s\+\(Get\|Let\|Set\)\>"

" AspVBScript Reserved Words.
syn match AspVBSStatement contained "\<On\s\+Error\s\+\(Resume\s\+Next\|goto\s\+0\)\>\|\<Next\>"
syn match AspVBSStatement contained "\<End\s\+\(If\|For\|Select\|Function\|Sub\)\>"
syn match AspVBSStatement contained "\<Exit\s\+\(Do\|For\|Sub\|Function\)\>"
syn match AspVBSStatement contained "\<Option\s\+Explicit\>"
syn match AspVBSStatement contained "\<For\s\+Each\>\|\<For\>"
syn match AspVBSStatement contained "\<Set\>"
syn keyword AspVBSStatement contained Call Const Dim Do Loop Erase And
syn keyword AspVBSStatement contained Function If Then Else ElseIf Or
syn keyword AspVBSStatement contained Private Public Randomize ReDim
syn keyword AspVBSStatement contained Select Case Sub While Wend Not

" AspVBScript Functions
syn keyword AspVBSFunction contained Abs Array Asc Atn CBool CByte CCur CDate CDbl
syn keyword AspVBSFunction contained Chr CInt CLng Cos CreateObject CSng CStr Date
syn keyword AspVBSFunction contained DateAdd DateDiff DatePart DateSerial DateValue
syn keyword AspVBSFunction contained Day Exp Filter Fix FormatCurrency
syn keyword AspVBSFunction contained FormatDateTime FormatNumber FormatPercent
syn keyword AspVBSFunction contained GetObject Hex Hour InputBox InStr InStrRev Int
syn keyword AspVBSFunction contained IsArray IsDate IsEmpty IsNull IsNumeric
syn keyword AspVBSFunction contained IsObject Join LBound LCase Left Len LoadPicture
syn keyword AspVBSFunction contained Log LTrim Mid Minute Month MonthName MsgBox Now
syn keyword AspVBSFunction contained Oct Replace RGB Right Rnd Round RTrim
syn keyword AspVBSFunction contained ScriptEngine ScriptEngineBuildVersion
syn keyword AspVBSFunction contained ScriptEngineMajorVersion
syn keyword AspVBSFunction contained ScriptEngineMinorVersion Second Sgn Sin Space
syn keyword AspVBSFunction contained Split Sqr StrComp StrReverse String Tan Time
syn keyword AspVBSFunction contained TimeSerial TimeValue Trim TypeName UBound UCase
syn keyword AspVBSFunction contained VarType Weekday WeekdayName Year

" AspVBScript Methods
syn keyword AspVBSMethods contained Add AddFolders BuildPath Clear Close Copy
syn keyword AspVBSMethods contained CopyFile CopyFolder CreateFolder CreateTextFile
syn keyword AspVBSMethods contained Delete DeleteFile DeleteFolder DriveExists
syn keyword AspVBSMethods contained Exists FileExists FolderExists
syn keyword AspVBSMethods contained GetAbsolutePathName GetBaseName GetDrive
syn keyword AspVBSMethods contained GetDriveName GetExtensionName GetFile
syn keyword AspVBSMethods contained GetFileName GetFolder GetParentFolderName
syn keyword AspVBSMethods contained GetSpecialFolder GetTempName Items Keys Move
syn keyword AspVBSMethods contained MoveFile MoveFolder OpenAsTextStream
syn keyword AspVBSMethods contained OpenTextFile Raise Read ReadAll ReadLine Remove
syn keyword AspVBSMethods contained RemoveAll Skip SkipLine Write WriteBlankLines
syn keyword AspVBSMethods contained WriteLine

" AspVBScript Number Contstants
" Integer number, or floating point number without a dot.
syn match  AspVBSNumber	contained	"\<\d\+\>"
" Floating point number, with dot
syn match  AspVBSNumber	contained	"\<\d\+\.\d*\>"
" Floating point number, starting with a dot
syn match  AspVBSNumber	contained	"\.\d\+\>"

" String and Character Contstants
" removed (skip=+\\\\\|\\"+) because VB doesn't have backslash escaping in
" strings (or does it?)
syn region  AspVBSString	contained	  start=+"+  end=+"+ keepend

" AspVBScript Comments
syn region  AspVBSComment	contained start="^REM\s\|\sREM\s" end="$" contains=AspVBSTodo keepend
syn region  AspVBSComment   contained start="^'\|\s'"   end="$" contains=AspVBSTodo keepend
" misc. Commenting Stuff
syn keyword AspVBSTodo contained	TODO FIXME

" Cosmetic syntax errors commanly found in VB but not in AspVBScript
" AspVBScript doesn't use line numbers
syn region  AspVBSError	contained start="^\d" end="\s" keepend
" AspVBScript also doesn't have type defining variables
syn match   AspVBSError  contained "[a-zA-Z0-9_][\$&!#]"ms=s+1
" Since 'a%' is a VB variable with a type and in AspVBScript you can have 'a%>'
" I have to make a special case so 'a%>' won't show as an error.
syn match   AspVBSError  contained "[a-zA-Z0-9_]%\($\|[^>]\)"ms=s+1

" Top Cluster
syn cluster AspVBScriptTop contains=AspVBSStatement,AspVBSFunction,AspVBSMethods,AspVBSNumber,AspVBSString,AspVBSComment,AspVBSError

" Define AspVBScript delimeters
" <%= func("string_with_%>_in_it") %> This is illegal in ASP syntax.
syn region  AspVBScriptInsideHtmlTags keepend matchgroup=Delimiter start=+<%=\=+ end=+%>+ contains=@AspVBScriptTop
syn region  AspVBScriptInsideHtmlTags keepend matchgroup=Delimiter start=+<script\s\+language="\=vbscript"\=[^>]*\s\+runatserver[^>]*>+ end=+</script>+ contains=@AspVBScriptTop

" Synchronization
syn sync match AspVBSSyncGroup grouphere AspVBScriptInsideHtmlTags "<%"
" This is a kludge so the HTML will sync properly
syn sync match htmlHighlight groupthere htmlTag "%>"

if !exists("did_asp_syntax_inits")
  let did_asp_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  "hi link AspVBScript		Special
  hi link AspVBSLineNumber	Comment
  hi link AspVBSNumber		Number
  hi link AspVBSError		Error
  hi link AspVBSStatement	Statement
  hi link AspVBSString		String
  hi link AspVBSComment		Comment
  hi link AspVBSTodo		Todo
  hi link AspVBSFunction	Identifier
  hi link AspVBSMethods		PreProc
  hi link AspVBSEvents		Special
  hi link AspVBSTypeSpecifier	Type
endif

let b:current_syntax = "aspvbs"

if main_syntax == 'aspvbs'
  unlet main_syntax
endif

" vim: ts=8:sw=2:sts=0:noet
