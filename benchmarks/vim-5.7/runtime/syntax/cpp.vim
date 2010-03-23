" Vim syntax file
" Language:	C++
" Maintainer:	Ken Shan <ccshan@post.harvard.edu>
" Last change:	2000 Apr 12

" Remove any old syntax stuff hanging around
syn clear

" Read the C syntax to start with
source <sfile>:p:h/c.vim

" C++ extentions

syn keyword cppStatement	new delete this friend using
syn keyword cppAccess		public protected private
syn keyword cppType		inline virtual explicit export bool wchar_t
syn keyword cppExceptions	throw try catch
syn keyword cppOperator		operator typeid
syn match cppCast		"\<\(const\|static\|dynamic\|reinterpret\)_cast\s*<"me=e-1
syn match cppCast		"\<\(const\|static\|dynamic\|reinterpret\)_cast\s*$"
syn keyword cppStorageClass	mutable
syn keyword cppStructure	class typename template namespace
syn keyword cppNumber	NPOS
syn keyword cppBoolean	true false

" The minimum and maximum operators in GNU C++

syn match cppMinMax "[<>]?"

if !exists("did_cpp_syntax_inits")
  let did_cpp_syntax_inits = 1
  hi link cppAccess	cppStatement
  hi link cppCast	cppStatement
  hi link cppExceptions	cppStatement
  hi link cppOperator	cppStatement
  hi link cppStatement	Statement
  hi link cppType	Type
  hi link cppStorageClass	StorageClass
  hi link cppStructure	Structure
  hi link cppNumber	Number
  hi link cppBoolean	Boolean
endif

let b:current_syntax = "cpp"

" vim: ts=8
