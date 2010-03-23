" Vim syntax file
" Language:	Squid config file
" Maintainer:	Klaus Muth <muth@hagos.de>
" Last Change:	1999 Jun 14
" URL:		http://unitopia.uni-stuttgart.de/~monty/vim/syntax/squid.vim


" Remove any old syntax stuff hanging around
syn clear
" squid.conf syntax seems to be case insensitive
syn case ignore

syn keyword	squidTodo	contained TODO
syn match	squidComment	"#.*$" contains=squidTodo,squidTag
syn match	squidTag	contained "TAG: .*$"

" Lots & lots of Keywords!
syn keyword	squidConf	http_port icp_port mcast_groups
syn keyword	squidConf	tcp_incoming_address tcp_outgoing_address
syn keyword	squidConf	udp_incoming_address udp_outgoing_address
syn keyword	squidConf	cache_host cache_host_domain
syn keyword	squidConf	neighbor_type_domain inside_firewall
syn keyword	squidConf	local_domain local_ip firewall_ip
syn keyword	squidConf	single_parent_bypass source_ping
syn keyword	squidConf	neighbor_timeout hierarchy_stoplist
syn keyword	squidConf	cache_stoplist cache_stoplist_pattern
syn keyword	squidConf	cache_mem cache_swap cache_swap_low
syn keyword	squidConf	cache_swap_high cache_mem_low
syn keyword	squidConf	cache_mem_high maximum_object_size
syn keyword	squidConf	ipcache_size ipcache_low ipcache_high
syn keyword	squidConf	cache_dir cache_access_log cache_log
syn keyword	squidConf	cache_store_log cache_swap_log
syn keyword	squidConf	emulate_httpd_log log_mime_hdrs
syn keyword	squidConf	useragent_log pid_filename
syn keyword	squidConf	debug_options ident_lookup log_fqdn
syn keyword	squidConf	client_netmask ftpget_program
syn keyword	squidConf	ftpget_options ftp_user cache_dns_program
syn keyword	squidConf	dns_children dns_defnames unlinkd_program
syn keyword	squidConf	pinger_program redirect_program
syn keyword	squidConf	redirect_children wais_relay request_size
syn keyword	squidConf	refresh_pattern reference_age reference_age
syn keyword	squidConf	quick_abort quick_abort negative_ttl
syn keyword	squidConf	positive_dns_ttl negative_dns_ttl
syn keyword	squidConf	connect_timeout read_timeout client_lifetime
syn keyword	squidConf	shutdown_lifetime acl http_access miss_access
syn keyword	squidConf	cache_host_acl cache_mgr cache_effective_user
syn keyword	squidConf	visible_hostname cache_announce announce_to
syn keyword	squidConf	httpd_accel httpd_accel_with_proxy
syn keyword	squidConf	httpd_accel_uses_host_header dns_testnames
syn keyword	squidConf	logfile_rotate append_domain
syn keyword	squidConf	tcp_recv_bufsize ssl_proxy passthrough_proxy
syn keyword	squidConf	proxy_auth err_html_text udp_hit_obj
syn keyword	squidConf	udp_hit_obj_size memory_pools forwarded_for
syn keyword	squidConf	log_icp_queries minimum_direct_hops
syn keyword	squidConf	swap_level1_dirs
syn keyword	squidConf	swap_level2_dirs store_avg_object_size
syn keyword	squidConf	store_objects_per_bucket http_anonymizer
syn keyword	squidConf	fake_user_agent client_db
syn keyword	squidConf	netdb_low netdb_high netdb_ping_rate
syn keyword	squidConf	netdb_ping_period query_icmp
syn keyword	squidConf	icp_hit_stale reload_into_ims
syn keyword	squidConf	cachemgr_passwd

syn keyword	squidOpt	proxy-only weight ttl no-query default
syn keyword	squidOpt	round-robin multicast-responder
syn keyword	squidOpt	on off all deny allow

" Security Actions for cachemgr_passwd
syn keyword	squidAction	shutdown info parameter server_list
syn keyword	squidAction	client_list
syn match	squidAction	"stats/\(objects\|vm_objects\|utilization\|ipcache\|fqdncache\|dns\|redirector\|io\|reply_headers\|filedescriptors\|netdb\)"
syn match	squidAction	"log\(/\(status\|enable\|disable\|clear\)\)\="
syn match	squidAction	"squid\.conf"

" Keywords for the acl-config
syn keyword	squidAcl	url_regexp urlpath_regexp port proto
syn keyword	squidAcl	method browser user src

syn match	squidNumber	"\<\d\+\>"
syn match	squidIP		"\<\d\{1,3}\.\d\{1,3}\.\d\{1,3}\.\d\{1,3}\>"

" All config is in one line, so this has to be sufficient
" Make it fast like hell :)
syn sync minlines=3

if !exists("did_squid_syntax_inits")
  let did_squid_syntax_inits = 1
" The default methods for highlighting.  Can be overridden later
  hi link squidTodo	Todo
  hi link squidComment	Comment
  hi link squidTag	Special
  hi link squidConf	Keyword
  hi link squidOpt	Constant
  hi link squidAction	String
  hi link squidNumber	Number
  hi link squidIP	Number
  hi link squidAcl	Keyword
endif

let b:current_syntax = "squid"

" vim: ts=8
