" Vim syntax file
" Language:	Java
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/java.vim
" Last Change:  2000 Jun 1

" Please check :help java.vim for comments on some of the options available.

" Remove any old syntax stuff hanging around
syn clear

" some characters that cannot be in a java program (outside a string)
syn match javaError "[\\@`]"
syn match javaError "<<<\|\.\.\|=>\|<>\|||=\|&&=\|[^-]->\|\*\/"

" use separate name so that it can be deleted in javacc.vim
syn match javaError2 "#\|=<"
hi link javaError2 javaError

" we define it here so that included files can test for it
if !exists("main_syntax")
  let main_syntax='java'
endif


" keyword definitions
syn keyword javaExternal        import native package
syn keyword javaError           goto const
syn keyword javaConditional     if else switch
syn keyword javaRepeat          while for do
syn keyword javaBoolean         true false
syn keyword javaConstant        null
syn keyword javaTypedef         this super
syn keyword javaOperator        new instanceof
syn keyword javaType            boolean char byte short int long float double
syn keyword javaType            void
syn keyword javaStatement       return
syn keyword javaStorageClass    static synchronized transient volatile final strictfp
syn keyword javaExceptions      throw try catch finally
syn keyword javaMethodDecl      synchronized throws
syn keyword javaClassDecl       extends implements interface
" to differentiate the keyword class from MyClass.class we use a match here
syn match   javaTypedef         "\.\s*\<class\>"ms=s+1
syn match   javaClassDecl       "^class\>"
syn match   javaClassDecl       "[^.]\s*\<class\>"ms=s+1
syn keyword javaBranch          break continue nextgroup=javaUserLabelRef skipwhite
syn match   javaUserLabelRef    "\k\+" contained
syn keyword javaScopeDecl       public protected private abstract

if exists("java_highlight_java_lang_ids") || exists("java_highlight_java_lang") || exists("java_highlight_all")
  " java.lang.*
  syn match javaLangClass "System"
  syn keyword javaLangClass  Cloneable Comparable Runnable Boolean Byte Class
  syn keyword javaLangClass  Character ClassLoader Compiler Double Float
  syn keyword javaLangClass  Integer Long Math Number Object Package Process
  syn keyword javaLangClass  Runtime RuntimePermission InheritableThreadLocal
  syn keyword javaLangClass  SecurityManager Short String
  syn keyword javaLangClass  StringBuffer Thread ThreadGroup
  syn keyword javaLangClass  ThreadLocal Throwable Void ArithmeticException
  syn keyword javaLangClass  ArrayIndexOutOfBoundsException
  syn keyword javaLangClass  ArrayStoreException ClassCastException
  syn keyword javaLangClass  ClassNotFoundException
  syn keyword javaLangClass  CloneNotSupportedException Exception
  syn keyword javaLangClass  IllegalAccessException
  syn keyword javaLangClass  IllegalArgumentException
  syn keyword javaLangClass  IllegalMonitorStateException
  syn keyword javaLangClass  IllegalStateException
  syn keyword javaLangClass  IllegalThreadStateException
  syn keyword javaLangClass  IndexOutOfBoundsException
  syn keyword javaLangClass  InstantiationException InterruptedException
  syn keyword javaLangClass  NegativeArraySizeException NoSuchFieldException
  syn keyword javaLangClass  NoSuchMethodException NullPointerException
  syn keyword javaLangClass  NumberFormatException RuntimeException
  syn keyword javaLangClass  SecurityException StringIndexOutOfBoundsException
  syn keyword javaLangClass  UnsupportedOperationException
  syn keyword javaLangClass  AbstractMethodError ClassCircularityError
  syn keyword javaLangClass  ClassFormatError Error ExceptionInInitializerError
  syn keyword javaLangClass  IllegalAccessError InstantiationError
  syn keyword javaLangClass  IncompatibleClassChangeError InternalError
  syn keyword javaLangClass  LinkageError NoClassDefFoundError
  syn keyword javaLangClass  NoSuchFieldError NoSuchMethodError
  syn keyword javaLangClass  OutOfMemoryError StackOverflowError
  syn keyword javaLangClass  ThreadDeath UnknownError UnsatisfiedLinkError
  syn keyword javaLangClass  UnsupportedClassVersionError VerifyError
  syn keyword javaLangClass  VirtualMachineError
  syn keyword javaLangObject clone equals finalize getClass hashCode
  syn keyword javaLangObject notify notifyAll toString wait
  hi link javaLangClass                   javaConstant
  hi link javaLangObject                  javaConstant
  syn cluster javaTop add=javaLangObject
endif

if filereadable(expand("<sfile>:p:h")."/javaid.vim")
  source <sfile>:p:h/javaid.vim
endif

if exists("java_space_errors")
  if !exists("java_no_trail_space_error")
    syn match	javaSpaceError	"\s\+$"
  endif
  if !exists("java_no_tab_space_error")
    syn match	javaSpaceError	" \+\t"me=e-1
  endif
endif

syn region  javaLabelRegion     transparent matchgroup=javaLabel start="\<case\>" matchgroup=NONE end=":" contains=javaNumber
syn match   javaUserLabel       "^\s*[_$a-zA-Z][_$a-zA-Z0-9_]*\s*:"he=e-1 contains=javaLabel
syn keyword javaLabel           default

if !exists("java_allow_cpp_keywords")
  syn keyword javaError auto delete enum extern friend inline redeclared
  syn keyword javaError register signed sizeof struct template typedef union
  syn keyword javaError unsigned operator
endif

" The following cluster contains all java groups except the contained ones
syn cluster javaTop add=javaExternal,javaError,javaError,javaBranch,javaLabelRegion,javaLabel,javaConditional,javaRepeat,javaBoolean,javaConstant,javaTypedef,javaOperator,javaType,javaType,javaStatement,javaStorageClass,javaExceptions,javaMethodDecl,javaClassDecl,javaClassDecl,javaClassDecl,javaScopeDecl,javaError,javaError2,javaUserLabel,javaLangObject


" Comments
syn keyword javaTodo             contained TODO FIXME XXX
syn region  javaCommentString    contained start=+"+ end=+"+ end=+$+ end=+\*/+me=s-1,he=s-1 contains=javaSpecial,javaCommentStar,javaSpecialChar,@Spell
syn region  javaComment2String   contained start=+"+  end=+$\|"+  contains=javaSpecial,javaSpecialChar,@Spell
syn match   javaCommentCharacter contained "'\\[^']\{1,6\}'" contains=javaSpecialChar
syn match   javaCommentCharacter contained "'\\''" contains=javaSpecialChar
syn match   javaCommentCharacter contained "'[^\\]'"
syn region  javaComment          start="/\*"  end="\*/" contains=javaCommentString,javaCommentCharacter,javaNumber,javaTodo,@Spell
syn match   javaCommentStar      contained "^\s*\*[^/]"me=e-1
syn match   javaCommentStar      contained "^\s*\*$"
syn match   javaLineComment      "//.*" contains=javaComment2String,javaCommentCharacter,javaNumber,javaTodo,@Spell
hi link javaCommentString javaString
hi link javaComment2String javaString
hi link javaCommentCharacter javaCharacter

syn cluster javaTop add=javaComment,javaLineComment

if !exists("java_ignore_javadoc")
  syntax case ignore
  " syntax coloring for javadoc comments (HTML)
  syntax include @javaHtml <sfile>:p:h/html.vim
  syn region  javaDocComment    start="/\*\*"  end="\*/" keepend contains=javaCommentTitle,@javaHtml,javaDocTags,javaTodo,@Spell
  syn region  javaCommentTitle  contained matchgroup=javaDocComment start="/\*\*"   matchgroup=javaCommentTitle keepend end="\.$" end="\.[ \t\r<&]"me=e-1 end="@"me=s-1,he=s-1 end="\*/"me=s-1,he=s-1 contains=@javaHtml,javaCommentStar,javaTodo,@Spell

  syn region javaDocTags contained start="{@link" end="}"
  syn match javaDocTags contained "@\(see\|param\|exception\|throws\)\s\+\S\+" contains=javaDocParam
  syn match javaDocParam contained "\s\S\+"
  syn match javaDocTags contained "@\(version\|author\|return\|deprecated\|since\)\>"
  syntax case match
endif

" match the special comment /**/
syn match   javaComment          "/\*\*/"

" Strings and constants
syn match   javaSpecialError     contained "\\."
syn match   javaSpecialCharError contained "[^']"
syn match   javaSpecialChar      contained "\\\([4-9]\d\|[0-3]\d\d\|[\"\\'ntbrf]\|u\x\{4\}\)"
syn region   javaString          start=+"+ end=+"+ end=+$+ contains=javaSpecialChar,javaSpecialError,@Spell
syn match   javaStringError      +"\([^"\\]\|\\.\)*$+
syn match   javaCharacter        "'[^']*'" contains=javaSpecialChar,javaSpecialCharError
syn match   javaCharacter        "'\\''" contains=javaSpecialChar
syn match   javaCharacter        "'[^\\]'"
syn match   javaNumber           "\<\(0[0-7]*\|0[xX]\x\+\|\d\+\)[lL]\=\>"
syn match   javaNumber           "\(\<\d\+\.\d*\|\.\d\+\)\([eE][-+]\=\d\+\)\=[fFdD]\="
syn match   javaNumber           "\<\d\+[eE][-+]\=\d\+[fFdD]\=\>"
syn match   javaNumber           "\<\d\+\([eE][-+]\=\d\+\)\=[fFdD]\>"

" unicode characters
syn match   javaSpecial "\\u\d\{4\}"

syn cluster javaTop add=javaString,javaCharacter,javaNumber,javaSpecial,javaStringError

if exists("java_highlight_functions")
  if java_highlight_functions == "indent"
    syn match  javaFuncDef "^\(\t\| \{8\}\)[_$a-zA-Z][_$a-zA-Z0-9_. \[\]]*([^-+*/()]*)" contains=javaScopeDecl,javaType,javaStorageClass
    syn region javaFuncDef start=+^\(\t\| \{8\}\)[$_a-zA-Z][$_a-zA-Z0-9_. \[\]]*([^-+*/()]*,\s*+ end=+)+ contains=javaScopeDecl,javaType,javaStorageClass
    syn match  javaFuncDef "^  [$_a-zA-Z][$_a-zA-Z0-9_. \[\]]*([^-+*/()]*)" contains=javaScopeDecl,javaType,javaStorageClass
    syn region javaFuncDef start=+^  [$_a-zA-Z][$_a-zA-Z0-9_. \[\]]*([^-+*/()]*,\s*+ end=+)+ contains=javaScopeDecl,javaType,javaStorageClass
  else
    " This line catches method declarations at any indentation>0, but it assumes
    " two things:
    "   1. class names are always capitalized (ie: Button)
    "   2. method names are never capitalized (except constructors, of course)
    syn region javaFuncDef start=+^\s\+\(\(public\|protected\|private\|static\|abstract\|final\|native\|synchronized\)\s\+\)*\(\(void\|boolean\|char\|byte\|short\|int\|long\|float\|double\|\([A-Za-z_][A-Za-z0-9_$]*\.\)*[A-Z][A-Za-z0-9_$]*\)\(\[\]\)*\s\+[a-z][A-Za-z0-9_$]*\|[A-Z][A-Za-z0-9_$]*\)\s*(+ end=+)+ contains=javaScopeDecl,javaType,javaStorageClass,javaComment,javaLineComment
  endif
  syn match  javaBraces  "[{}]"
  syn cluster javaTop add=javaFuncDef,javaBraces
endif

if exists("java_highlight_debug")

  " Strings and constants
  syn match   javaDebugSpecial          contained "\\\d\d\d\|\\."
  syn region  javaDebugString           contained start=+"+  end=+"+  contains=javaDebugSpecial
  syn match   javaDebugStringError      +"\([^"\\]\|\\.\)*$+
  syn match   javaDebugCharacter        contained "'[^\\]'"
  syn match   javaDebugSpecialCharacter contained "'\\.'"
  syn match   javaDebugSpecialCharacter contained "'\\''"
  syn match   javaDebugNumber           contained "\<\(0[0-7]*\|0[xX]\x\+\|\d\+\)[lL]\=\>"
  syn match   javaDebugNumber           contained "\(\<\d\+\.\d*\|\.\d\+\)\([eE][-+]\=\d\+\)\=[fFdD]\="
  syn match   javaDebugNumber           contained "\<\d\+[eE][-+]\=\d\+[fFdD]\=\>"
  syn match   javaDebugNumber           contained "\<\d\+\([eE][-+]\=\d\+\)\=[fFdD]\>"
  syn keyword javaDebugBoolean          contained true false
  syn keyword javaDebugType             contained null this super

  " to make this work you must define the highlighting for these groups
  syn region javaDebug start="System\.\(out\|err\)\.print\(ln\)*\s*" end=";" contains=javaDebug.*
  syn match javaDebug "[A-Za-z][a-zA-Z0-9_]*\.printStackTrace\s*(\s*)" contains=javaDebug.*
  syn region javaDebug  start="trace[SL]\=[ \t]*(" end=";" contains=javaDebug.*

  syn cluster javaTop add=javaDebug

  hi link javaDebug                 Debug
  hi link javaDebugString           DebugString
  hi link javaDebugStringError      javaError
  hi link javaDebugType             DebugType
  hi link javaDebugBoolean          DebugBoolean
  hi link javaDebugNumber           Debug
  hi link javaDebugSpecial          DebugSpecial
  hi link javaDebugSpecialCharacter DebugSpecial
  hi link javaDebugCharacter        DebugString

  hi link DebugString               String
  hi link DebugSpecial              Special
  hi link DebugBoolean              Boolean
  hi link DebugType                 Type
endif

if exists("java_mark_braces_in_parens_as_errors")
  syn match javaInParen          contained "[{}]"
  hi link   javaInParen          javaError
  syn cluster javaTop add=javaInParen
endif

" catch errors caused by wrong parenthesis
syn region  javaParen            transparent start="(" end=")" contains=@javaTop,javaParen
syn match   javaParenError       ")"
hi link     javaParenError       javaError

if !exists("java_minlines")
  let java_minlines = 10
endif
exec "syn sync ccomment javaComment minlines=" . java_minlines

if !exists("did_java_syntax_inits")
  let did_java_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link javaFuncDef                       Function
  hi link javaBraces                        Function
  hi link javaBranch                        Conditional
  hi link javaUserLabelRef                  javaUserLabel
  hi link javaLabel                         Label
  hi link javaUserLabel                     Label
  hi link javaConditional                   Conditional
  hi link javaRepeat                        Repeat
  hi link javaExceptions                    Exception
  hi link javaStorageClass                  StorageClass
  hi link javaMethodDecl                    javaStorageClass
  hi link javaClassDecl                     javaStorageClass
  hi link javaScopeDecl                     javaStorageClass
  hi link javaBoolean                       Boolean
  hi link javaSpecial                       Special
  hi link javaSpecialError                  Error
  hi link javaSpecialCharError              Error
  hi link javaString                        String
  hi link javaCharacter                     Character
  hi link javaSpecialChar		    SpecialChar
  hi link javaNumber                        Number
  hi link javaError                         Error
  hi link javaStringError                   Error
  hi link javaStatement                     Statement
  hi link javaOperator                      Operator
  hi link javaComment                       Comment
  hi link javaDocComment                    Comment
  hi link javaLineComment                   Comment
  hi link javaConstant			    javaBoolean
  hi link javaTypedef			    Typedef
  hi link javaTodo                          Todo

  hi link javaCommentTitle                  SpecialComment
  hi link javaDocTags			    Special
  hi link javaDocParam			    Function
  hi link javaCommentStar                   javaComment

  hi link javaType                          Type
  hi link javaExternal                      Include

  hi link htmlComment                       Special
  hi link htmlCommentPart                   Special
  hi link javaSpaceError                    Error

endif

let b:current_syntax = "java"

if main_syntax == 'java'
  unlet main_syntax
endif

let b:spell_options="contained"

" vim: ts=8
