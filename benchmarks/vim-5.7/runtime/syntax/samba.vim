" Vim syntax file
" Language:     samba configuration files (smb.conf)
" Maintainer:   Rafael Garcia-Suarez <garcia_suarez@hotmail.com>
" URL:          http://altern.org/rgs/vim/syntax/samba.vim
" Last change:  2000-04-26

" Don't forget to run your config file through testparm(1)!

syn clear
syn case ignore

syn match sambaParameter /^[a-zA-Z \t]\+=/ contains=sambaKeyword
syn match sambaSection /^\s*\[[a-zA-Z0-9_\-. ]\+\]/
syn match sambaMacro /%[SPugUGHvhmLMNpRdaIT]/
syn match sambaComment /^[;#].*/
syn match sambaContinue /\\$/
syn keyword sambaBoolean true false yes no

" Keywords for Samba 2.0.5a
syn keyword sambaKeyword contained account acl action add address admin aliases
syn keyword sambaKeyword contained allow alternate always announce anonymous
syn keyword sambaKeyword contained archive as auto available bind blocking
syn keyword sambaKeyword contained bmpx break browsable browse browseable ca
syn keyword sambaKeyword contained cache case casesignames cert certDir
syn keyword sambaKeyword contained certFile change char character chars chat
syn keyword sambaKeyword contained ciphers client clientcert code coding
syn keyword sambaKeyword contained command comment compatibility config
syn keyword sambaKeyword contained connections contention controller copy
syn keyword sambaKeyword contained create deadtime debug debuglevel default
syn keyword sambaKeyword contained delete deny descend dfree dir directory
syn keyword sambaKeyword contained disk dns domain domains dont dos dot drive
syn keyword sambaKeyword contained driver encrypt encrypted equiv exec fake
syn keyword sambaKeyword contained file files filetime filetimes filter follow
syn keyword sambaKeyword contained force fstype getwd group groups guest
syn keyword sambaKeyword contained hidden hide home homedir hosts include
syn keyword sambaKeyword contained interfaces interval invalid keepalive
syn keyword sambaKeyword contained kernel key ldap length level level2 limit
syn keyword sambaKeyword contained links list lm load local location lock
syn keyword sambaKeyword contained locking locks log logon logons logs lppause
syn keyword sambaKeyword contained lpq lpresume lprm machine magic mangle
syn keyword sambaKeyword contained mangled mangling map mask master max mem
syn keyword sambaKeyword contained message min mode modes mux name names
syn keyword sambaKeyword contained netbios nis notify nt null offset ok ole
syn keyword sambaKeyword contained only open oplock oplocks options order os
syn keyword sambaKeyword contained output packet page panic passwd password
syn keyword sambaKeyword contained passwords path permissions pipe port
syn keyword sambaKeyword contained postexec postscript prediction preexec
syn keyword sambaKeyword contained prefered preferred preload preserve print
syn keyword sambaKeyword contained printable printcap printer printers
syn keyword sambaKeyword contained printing program protocol proxy public
syn keyword sambaKeyword contained queuepause queueresume raw read readonly
syn keyword sambaKeyword contained realname remote require resign resolution
syn keyword sambaKeyword contained resolve restrict revalidate rhosts root
syn keyword sambaKeyword contained script security sensitive server servercert
syn keyword sambaKeyword contained service services set share shared short
syn keyword sambaKeyword contained size smb smbrun socket space ssl stack stat
syn keyword sambaKeyword contained status strict string strip suffix support
syn keyword sambaKeyword contained symlinks sync syslog system time timeout
syn keyword sambaKeyword contained times timestamp to trusted ttl unix update
syn keyword sambaKeyword contained use user username users valid version veto
syn keyword sambaKeyword contained volume wait wide wins workgroup writable
syn keyword sambaKeyword contained write writeable xmit

" New keywords for Samba 2.0.6
syn keyword sambaKeyword contained hook hires pid uid close rootpreexec

" New keywords for Samba 2.0.7
syn keyword sambaKeyword contained utmp wtmp hostname consolidate
syn keyword sambaKeyword contained inherit source environment

if !exists("did_samba_syntax_inits")
  let did_samba_syntax_inits=1
  hi link sambaParameter Normal
  hi link sambaKeyword Type
  hi link sambaSection Statement
  hi link sambaMacro PreProc
  hi link sambaComment Comment
  hi link sambaContinue Operator
  hi link sambaBoolean Constant
endif

let b:current_syntax = "samba"

" vim: ts=8
