" Vim syntax file
" Language:	php3 PHP 3.0
" Maintainer:	Lutz Eymers <ixtab@polzin.com>
" URL:		http://www-public.rz.uni-duesseldorf.de/~eymers/vim/syntax
" Email:	Subject: send syntax_vim.tgz
" Last Change:	2000 Jan 05
"
" Options	php3_sql_query = 1  for SQL syntax highligthing inside strings
"		php3_minlines = x  to sync at least x lines backwards
"		php3_baselib = 1  for highlighting baselib functions

" Remove any old syntax stuff hanging around
syn clear


if !exists("main_syntax")
  let main_syntax = 'php3'
endif

so <sfile>:p:h/html.vim
syn cluster htmlPreproc add=php3RegionInsideHtmlTags

if exists( "php3_sql_query")
  if php3_sql_query == 1
    syn include @php3Sql <sfile>:p:h/sql.vim
  endif
endif
syn cluster php3Sql remove=sqlString,sqlComment

syn case match

" Env Variables
syn keyword php3EnvVar SERVER_SOFTWARE SERVER_NAME SERVER_URL GATEWAY_INTERFACE   contained
syn keyword php3EnvVar SERVER_PROTOCOL SERVER_PORT REQUEST_METHOD PATH_INFO  contained
syn keyword php3EnvVar PATH_TRANSLATED SCRIPT_NAME QUERY_STRING REMOTE_HOST contained
syn keyword php3EnvVar REMOTE_ADDR AUTH_TYPE REMOTE_USER CONTEN_TYPE  contained
syn keyword php3EnvVar CONTENT_LENGTH HTTPS HTTPS_KEYSIZE HTTPS_SECRETKEYSIZE  contained
syn keyword php3EnvVar HTTP_ACCECT HTTP_USER_AGENT HTTP_IF_MODIFIED_SINCE  contained
syn keyword php3EnvVar HTTP_FROM HTTP_REFERER contained
syn keyword php3EnvVar PHP_SELF PHP_AUTH_TYPE PHP_AUTH_USER PHP_AUTH_PW contained

" Internal Variables
syn keyword php3IntVar GLOBALS HTTP_GET_VARS HTTP_POST_VARS HTTP_COOKIE_VARS contained
syn keyword php3IntVar PHP_ERRMSG PHP_SELF contained

syn case ignore

" Comment
syn region php3Comment		start="/\*" skip="?>" end="\*/"  contained contains=php3Todo
syn match php3Comment		"#.*$"  contained contains=php3Todo
syn match php3Comment		"//.*$"  contained contains=php3Todo

" Function names
syn keyword php3Functions ada_afetch ada_autocommit ada_close ada_commit ada_connect ada_exec ada_fetchrow ada_fieldname ada_fieldnum ada_fieldtype ada_freeresult ada_numfields ada_numrows ada_result ada_resultall ada_rollback contained
syn keyword php3Functions apache_lookup_uri apache_note getallheaders virtual contained
syn keyword php3Functions array array_count_values array_flip array_keys array_merge array_pad array_pop array_push array_reverse array_shift array_slice array_splice array_unshift array_values array_walk arsort asort compact count current each end extract in_array key krsort ksort list next pos prev range reset rsort shuffle sizeof sort uasort uksort usort contained
syn keyword php3Functions aspell_new aspell_check aspell_check-raw aspell_suggest contained
syn keyword php3Functions bcadd bccomp bcdiv bcmod bcmul bcpow bcscale bcsqrt bcsub contained
syn keyword php3Functions JDToGregorian GregorianToJD JDToJulian JulianToJD JDToJewish JewishToJD JDToFrench FrenchToJD JDMonthName JDDayOfWeek easter_date easter_days contained
syn keyword php3Functions cpdf_set_creator cpdf_set_title cpdf_set_subject cpdf_set_keywords cpdf_open cpdf_close cpdf_page_init cpdf_finalize_page cpdf_finalize cpdf_output_buffer cpdf_save_to_file cpdf_set_current_page cpdf_begin_text cpdf_end_text cpdf_show cpdf_show_xy cpdf_text cpdf_set_font cpdf_set_leading cpdf_set_text_rendering cpdf_set_horiz_scaling cpdf_set_text_rise cpdf_set_text_matrix cpdf_set_text_pos cpdf_set_char_spacing cpdf_set_word_spacing cpdf_continue_text cpdf_stringwidth cpdf_save cpdf_restore cpdf_translate cpdf_scale cpdf_rotate cpdf_setflat cpdf_setlinejoin cpdf_setlinecap cpdf_setmiterlimit cpdf_setlinewidth cpdf_setdash cpdf_moveto cpdf_rmoveto cpdf_curveto cpdf_lineto cpdf_rlineto cpdf_circle cpdf_arc cpdf_rect cpdf_closepath cpdf_stroke cpdf_closepath_stroke cpdf_fill cpdf_fill_stroke cpdf_closepath_fill_stroke cpdf_clip cpdf_setgray_fill cpdf_setgray_stroke cpdf_setgray cpdf_setrgbcolor_fill cpdf_setrgbcolor_stroke cpdf_setrgbcolor cpdf_add_outline cpdf_set_page_animation cpdf_import_jpeg cpdf_place_inline_image cpdf_add_annotation contained
syn keyword php3Functions checkdate date getdate gettimeofday gmdate gmmktime gmstrftime microtime mktime strftime time contained
syn keyword php3Functions dbase_create dbase_open dbase_close dbase_pack dbase_add_record dbase_replace_record dbase_delete_record dbase_get_record dbase_get_record_with_names dbase_numfields dbase_numrecords contained
syn keyword php3Functions dbmopen dbmclose dbmexists dbmfetch dbminsert dbmreplace dbmdelete dbmfirstkey dbmnextkey dblist contained
syn keyword php3Functions chdir dir closedir opendir readdir rewinddir contained
syn keyword php3Functions dl contained
syn keyword php3Functions escapeshellcmd exec passthru system contained
syn keyword php3Functions fdf_open fdf_close fdf_create fdf_save fdf_get_value fdf_set_value fdf_next_field_name fdf_set_ap fdf_set_status fdf_get_status fdf_set_file fdf_get_file contained
syn keyword php3Functions filepro filepro_fieldname filepro_fieldtype filepro_fieldwidth filepro_retrieve filepro_fieldcount filepro_rowcount contained
syn keyword php3Functions basename chgrp chmod chown clearstatcache copy delete dirname diskfreespace fclose feof fgetc fgetcsv fgets fgetss file file_exists fileatime filectime filegroup fileinode filemtime fileowner fileperms filesize filetype flock fopen fpassthru fputs fread fseek ftell fwrite set_file_buffer is_dir is_executable is_file is_link is_readable is_writeable link linkinfo mkdir pclose popen readfile readlink rename rewind rmdir stat lstat symlink tempnam touch umask unlink contained
syn keyword php3Functions ftp_connect ftp_login ftp_pwd ftp_cdup ftp_chdir ftp_mkdir ftp_rmdir ftp_nlist ftp_rawlist ftp_systype ftp_pasv ftp_get ftp_fget ftp_put ftp_fput ftp_size ftp_mdtm ftp_rename ftp_delete ftp_quit contained
syn keyword php3Functions header setcookie contained
syn keyword php3Functions hw_Array2Objrec hw_Children hw_ChildrenObj hw_Close hw_Connect hw_Cp hw_Deleteobject hw_DocByAnchor hw_DocByAnchorObj hw_DocumentAttributes hw_DocumentBodyTag hw_DocumentContent hw_DocumentSetContent hw_DocumentSize hw_ErrorMsg hw_EditText hw_Error hw_Free_Document hw_GetParents hw_GetParentsObj hw_GetChildColl hw_GetChildCollObj hw_GetRemote hw_GetRemoteChildren hw_GetSrcByDestObj hw_GetObject hw_GetAndLock hw_GetText hw_GetObjectByQuery hw_GetObjectByQueryObj hw_GetObjectByQueryColl hw_GetObjectByQueryCollObj hw_GetChildDocColl hw_GetChildDocCollObj hw_GetAnchors hw_GetAnchorsObj hw_Mv hw_Identify hw_InCollections hw_Info hw_InsColl hw_InsDoc hw_InsertDocument hw_InsertObject hw_mapid hw_Modifyobject hw_New_Document hw_Objrec2Array hw_OutputDocument hw_pConnect hw_PipeDocument hw_Root hw_Unlock hw_Who hw_Username contained
syn keyword php3Functions ibase_connect ibase_pconnect ibase_close ibase_query ibase_fetch_row ibase_free_result ibase_prepare ibase_bind ibase_execute ibase_free_query ibase_timefmt contained
syn keyword php3Functions ifx_connect ifx_pconnect ifx_close ifx_query ifx_prepare ifx_do ifx_error ifx_errormsg ifx_affected_rows ifx_getsqlca ifx_fetch_row ifx_htmltbl_result ifx_fieldtypes ifx_fieldproperties ifx_num_fields ifx_num_rows ifx_free_result ifx_create_char ifx_free_char ifx_update_char ifx_get_char ifx_create_blob ifx_copy_blob ifx_free_blob ifx_get_blob ifx_update_blob ifx_blobinfile_mode ifx_textasvarchar ifx_byteasvarchar ifx_nullformat ifxus_create_slob ifx_free_slob ifxus_close_slob ifxus_open_slob ifxus_tell_slob ifxus_seek_slob ifxus_read_slob ifxus_write_slob contained
syn keyword php3Functions GetImageSize ImageArc ImageChar ImageCharUp ImageColorAllocate ImageColorAt ImageColorClosest ImageColorExact ImageColorResolve ImageColorSet ImageColorsForIndex ImageColorsTotal ImageColorTransparent ImageCopyResized ImageCreate ImageCreateFromGif ImageDashedLine ImageDestroy ImageFill ImageFilledPolygon ImageFilledRectangle ImageFillToBorder ImageFontHeight ImageFontWidth ImageGif ImageInterlace ImageLine ImageLoadFont ImagePolygon ImagePSBBox ImagePSEncodeFont ImagePSFreeFont ImagePSLoadFont ImagePSText ImageRectangle ImageSetPixel ImageString ImageStringUp ImageSX ImageSY ImageTTFBBox ImageTTFText contained
syn keyword php3Functions imap_append imap_base64 imap_body imap_check imap_close imap_createmailbox imap_delete imap_deletemailbox imap_expunge imap_fetchbody imap_fetchstructure imap_header imap_headers imap_listmailbox imap_getmailboxes imap_listsubscribed imap_getsubscribed imap_mail_copy imap_mail_move imap_num_msg imap_num_recent imap_open imap_ping imap_renamemailbox imap_reopen imap_subscribe imap_undelete imap_unsubscribe imap_qprint imap_8bit imap_binary imap_scanmailbox imap_mailboxmsginfo imap_rfc822_write_address imap_rfc822_parse_adrlist imap_setflag_full imap_clearflag_full imap_sort imap_fetchheader imap_uid imap_msgno imap_search imap_last_error imap_errors imap_alerts imap_status contained
syn keyword php3Functions error_log error_reporting extension_loaded getenv get_cfg_var get_current_user get_magic_quotes_gpc get_magic_quotes_runtime getlastmod getmyinode getmypid getmyuid getrusage phpinfo phpversion putenv set_magic_quotes_runtime set_time_limit contained
syn keyword php3Functions ldap_add ldap_mod_add ldap_mod_del ldap_mod_replace ldap_bind ldap_close ldap_connect ldap_count_entries ldap_delete ldap_dn2ufn ldap_explode_dn ldap_first_attribute ldap_first_entry ldap_free_result ldap_get_attributes ldap_get_dn ldap_get_entries ldap_get_values ldap_get_values_len ldap_list ldap_modify ldap_next_attribute ldap_next_entry ldap_read ldap_search ldap_unbind ldap_err2str ldap_errno ldap_error contained
syn keyword php3Functions mail contained
syn keyword php3Functions Abs Acos Asin Atan Atan2 base_convert BinDec Ceil Cos DecBin DecHex DecOct Exp Floor getrandmax HexDec Log Log10 max min mt_rand mt_srand mt_getrandmax number_format OctDec pi pow rand round Sin Sqrt srand Tan contained
syn keyword php3Functions mcal_open mcal_close mcal_fetch_event mcal_list_events mcal_store_event mcal_delete_event mcal_snooze mcal_list_alarms mcal_event_init mcal_event_set_category mcal_event_set_title mcal_event_set_description mcal_event_set_start mcal_event_set_end mcal_event_set_alarm mcal_event_set_class mcal_is_leap_year mcal_days_in_month mcal_date_valid mcal_time_valid mcal_day_of_week mcal_day_of_year mcal_date_compare mcal_next_recurrence mcal_event_set_recur_daily mcal_event_set_recur_weekly mcal_event_set_recur_monthly_mday mcal_event_set_recur_monthly_wday mcal_event_set_recur_yearly mcal_fetch_current_stream_event contained
syn keyword php3Functions mcrypt_get_cipher_name mcrypt_get_block_size mcrypt_get_key_size mcrypt_create_iv mcrypt_cbc mcrypt_cfb mcrypt_ecb mcrypt_ofb contained
syn keyword php3Functions mhash_get_hash_name mhash_get_block_size mhash_count mhash contained
syn keyword php3Functions connection_aborted connection_status connection_timeout define defined die eval exit func_get_arg func_get_args func_num_args function_exists get_browser ignore_user_abort iptcparse leak pack register_shutdown_function serialize sleep uniqid unpack unserialize usleep contained
syn keyword php3Functions msql msql_affected_rows msql_close msql_connect msql_create_db msql_createdb msql_data_seek msql_dbname msql_drop_db msql_dropdb msql_error msql_fetch_array msql_fetch_field msql_fetch_object msql_fetch_row msql_fieldname msql_field_seek msql_fieldtable msql_fieldtype msql_fieldflags msql_fieldlen msql_free_result msql_freeresult msql_list_fields msql_listfields msql_list_dbs msql_listdbs msql_list_tables msql_listtables msql_num_fields msql_num_rows msql_numfields msql_numrows msql_pconnect msql_query msql_regcase msql_result msql_select_db msql_selectdb msql_tablename contained
syn keyword php3Functions mssql_close mssql_connect mssql_data_seek mssql_fetch_array mssql_fetch_field mssql_fetch_object mssql_fetch_row mssql_field_seek mssql_free_result mssql_num_fields mssql_num_rows mssql_pconnect mssql_query mssql_result mssql_select_db contained
syn keyword php3Functions mysql_affected_rows mysql_change_user mysql_close mysql_connect mysql_create_db mysql_data_seek mysql_db_query mysql_drop_db mysql_errno mysql_error mysql_fetch_array mysql_fetch_field mysql_fetch_lengths mysql_fetch_object mysql_fetch_row mysql_field_name mysql_field_seek mysql_field_table mysql_field_type mysql_field_flags mysql_field_len mysql_free_result mysql_insert_id mysql_list_fields mysql_list_dbs mysql_list_tables mysql_num_fields mysql_num_rows mysql_pconnect mysql_query mysql_result mysql_select_db mysql_tablename contained
syn keyword php3Functions checkdnsrr closelog debugger_off debugger_on fsockopen gethostbyaddr gethostbyname gethostbynamel getmxrr getprotobyname getprotobynumber getservbyname getservbyport openlog pfsockopen set_socket_blocking syslog contained
syn keyword php3Functions yp_get_default_domain yp_order yp_master yp_match yp_first yp_next yp_errno yp_err_string contained
syn keyword php3Functions OCIDefineByName OCIBindByName OCILogon OCIPLogon OCINLogon OCILogOff OCIExecute OCICommit OCIRollback OCINewDescriptor OCIRowCount OCINumCols OCIResult OCIFetch OCIFetchInto OCIFetchStatement OCIColumnIsNULL OCIColumnSize OCIServerVersion OCIStatementType OCINewCursor OCIFreeStatement OCIFreeCursor OCIColumnName OCIColumnType OCIParse OCIError OCIInternalDebug contained
syn keyword php3Functions odbc_autocommit odbc_binmode odbc_close odbc_close_all odbc_commit odbc_connect odbc_cursor odbc_do odbc_exec odbc_execute odbc_fetch_into odbc_fetch_row odbc_field_name odbc_field_type odbc_field_len odbc_free_result odbc_longreadlen odbc_num_fields odbc_pconnect odbc_prepare odbc_num_rows odbc_result odbc_result_all odbc_rollback odbc_setoption contained
syn keyword php3Functions Ora_Bind Ora_Close Ora_ColumnName Ora_ColumnType Ora_Commit Ora_CommitOff Ora_CommitOn Ora_Error Ora_ErrorCode Ora_Exec Ora_Fetch Ora_GetColumn Ora_Logoff Ora_Logon Ora_Open Ora_Parse Ora_Rollback contained
syn keyword php3Functions preg_match preg_match_all preg_replace preg_split preg_quote preg_grep Pattern Modifiers Pattern Syntax contained
syn keyword php3Functions PDF_get_info PDF_set_info_creator PDF_set_info_title PDF_set_info_subject PDF_set_info_keywords PDF_set_info_author PDF_open PDF_close PDF_begin_page PDF_end_page PDF_show PDF_show_xy PDF_set_font PDF_set_leading PDF_set_text_rendering PDF_set_horiz_scaling PDF_set_text_rise PDF_set_text_matrix PDF_set_text_pos PDF_set_char_spacing PDF_set_word_spacing PDF_continue_text PDF_stringwidth PDF_save PDF_restore PDF_translate PDF_scale PDF_rotate PDF_setflat PDF_setlinejoin PDF_setlinecap PDF_setmiterlimit PDF_setlinewidth PDF_setdash PDF_moveto PDF_curveto PDF_lineto PDF_circle PDF_arc PDF_rect PDF_closepath PDF_stroke PDF_closepath_stroke PDF_fill PDF_fill_stroke PDF_closepath_fill_stroke PDF_endpath PDF_clip PDF_setgray_fill PDF_setgray_stroke PDF_setgray PDF_setrgbcolor_fill PDF_setrgbcolor_stroke PDF_setrgbcolor PDF_add_outline PDF_set_transition PDF_set_duration PDF_open_gif PDF_open_memory_image PDF_open_jpeg PDF_close_image PDF_place_image PDF_put_image PDF_execute_image pdf_add_annotation contained
syn keyword php3Functions pg_Close pg_cmdTuples pg_Connect pg_DBname pg_ErrorMessage pg_Exec pg_Fetch_Array pg_Fetch_Object pg_Fetch_Row pg_FieldIsNull pg_FieldName pg_FieldNum pg_FieldPrtLen pg_FieldSize pg_FieldType pg_FreeResult pg_GetLastOid pg_Host pg_loclose pg_locreate pg_loopen pg_loread pg_loreadall pg_lounlink pg_lowrite pg_NumFields pg_NumRows pg_Options pg_pConnect pg_Port pg_Result pg_tty contained
syn keyword php3Functions posix_kill posix_getpid posix_getppid posix_getuid posix_geteuid posix_getgid posix_getegid posix_setuid posix_setgid posix_getgroups posix_getlogin posix_getpgrp posix_setsid posix_setpgid posix_getpgid posix_setsid posix_uname posix_times posix_ctermid posix_ttyname posix_isatty posix_getcwd posix_mkfifo posix_getgrnam posix_getgrgid posix_getpwnam posix_getpwuid posix_getrlimit contained
syn keyword php3Functions recode_string recode_file contained
syn keyword php3Functions ereg ereg_replace eregi eregi_replace split sql_regcase contained
syn keyword php3Functions sem_get sem_acquire sem_release shm_attach shm_detach shm_remove shm_put_var shm_get_var shm_remove_var contained
syn keyword php3Functions session_start session_destroy session_name session_module_name session_save_path session_id session_register session_unregister session_is_registered session_decode session_encode contained
syn keyword php3Functions snmpget snmpset snmpwalk snmpwalkoid snmp_get_quick_print snmp_set_quick_print contained
syn keyword php3Functions solid_close solid_connect solid_exec solid_fetchrow solid_fieldname solid_fieldnum solid_freeresult solid_numfields solid_numrows solid_result contained
syn keyword php3Functions AddCSlashes AddSlashes bin2hex Chop Chr chunk_split convert_cyr_string crypt echo explode flush get_html_translation_table get_meta_tags htmlentities htmlspecialchars implode join ltrim md5 Metaphone nl2br Ord parse_str print printf quoted_printable_decode QuoteMeta rawurldecode rawurlencode setlocale similar_text soundex sprintf strcasecmp strchr strcmp strcspn strip_tags StripCSlashes StripSlashes stristr strlen strpos strrchr str_repeat strrev strrpos strspn strstr strtok strtolower strtoupper str_replace strtr substr substr_replace trim ucfirst ucwords contained
syn keyword php3Functions sybase_affected_rows sybase_close sybase_connect sybase_data_seek sybase_fetch_array sybase_fetch_field sybase_fetch_object sybase_fetch_row sybase_field_seek sybase_free_result sybase_num_fields sybase_num_rows sybase_pconnect sybase_query sybase_result sybase_select_db contained
syn keyword php3Functions base64_decode base64_encode parse_url urldecode urlencode contained
syn keyword php3Functions doubleval empty gettype intval is_array is_double is_float is_int is_integer is_long is_object is_real is_string isset settype strval unset contained
syn keyword php3Functions vm_adduser vm_addalias vm_passwd vm_delalias vm_deluser contained
syn keyword php3Functions wddx_serialize_value wddx_serialize_vars wddx_packet_start wddx_packet_end wddx_add_vars wddx_deserialize contained
syn keyword php3Functions xml_parser_create xml_set_object xml_set_element_handler xml_set_character_data_handler xml_set_processing_instruction_handler xml_set_default_handler xml_set_unparsed_entity_decl_handler xml_set_notation_decl_handler xml_set_external_entity_ref_handler xml_parse xml_get_error_code xml_error_string xml_get_current_line_number xml_get_current_column_number xml_get_current_byte_index xml_parser_free xml_parser_set_option xml_parser_get_option utf8_decode utf8_encode contained
syn keyword php3Functions gzclose gzeof gzfile gzgetc gzgets gzgetss gzopen gzpassthru gzputs gzread gzrewind gzseek gztell gzwrite readgzfile contained

if exists( "php3_baselib" )
  if php3_baselib == 1
    syn keyword php3Baselib query next_record num_rows affected_rows nf f p num_fields haltmsg seek link_id query_id table_names nextid connect halt free page_close register unregister is_registered delete url purl self_url pself_url hidden_session add_query padd_query reimport_get_vars reimport_post_vars reimport_cookie_vars set_container set_tokenname release_token put_headers get_id get_id put_id freeze thaw gc reimport_any_vars start url purl login_if is_authenticated auth_preauth auth_loginform auth_validatelogin auth_refreshlogin auth_registerform auth_doregister start check have_perm permsum perm_invalid register unregister delete url purl self_url pself_url reimport_get_vars reimport_post_vars reimport_cookie_vars get_id put_id freeze thaw gc reimport_any_vars start  contained
    syn keyword php3Functions page_open page_close
  endif
endif

" Conditional
syn keyword php3Conditional  if else elseif endif switch endswitch contained

" Repeat
syn keyword php3Repeat  do for while endwhile contained

" Repeat
syn keyword php3Label  case default switch  contained

" Statement
syn keyword php3Statement  break return continue exit contained

" Keyword
syn keyword php3Keyword var contained

" Type
syn keyword php3Type int integer real double float string array object contained

" Structure
syn keyword php3Structure class extends contained

" StorageClass
syn keyword php3StorageClass global static contained

" Operator
syn match php3Operator  "[-=+%^&|*!.~?:]" contained
syn match php3Operator  "[-+*/%^&|.]=" contained
syn match php3Operator  "/[^*/]"me=e-1 contained
syn match php3Operator  "\$" contained
syn match php3Operator  "&&\|\<and\>" contained
syn match php3Operator  "||\|\<x\=or\>" contained
syn match php3Operator  "->" contained
syn match php3Relation  "[!=<>]=" contained
syn match php3Relation  "[<]" contained
syn match php3Relation  "[<>]" contained

" Identifier
syn match php3Identifier "$\h\w*" contained contains=php3EnvVar,php3IntVar,php3Operator

" Methoden
syn match php3Methods "->\h\w*" contained contains=php3Baselib,php3Operator

" Include
syn keyword php3Include  include require contained

" Define
syn keyword php3Define  Function cfunction new contained

" Boolean
syn keyword php3Boolean true false contained

" String
syn region php3StringDouble keepend matchgroup=None start=+"+ skip=+\\\\\|\\"+  end=+"+ contains=php3Identifier,php3SpecialChar,@php3Sql contained
syn region php3StringSingle keepend matchgroup=None start=+'+ skip=+\\\\\|\\'+  end=+'+ contains=php3SpecialChar,@php3Sql contained

" Number
syn match php3Number  "-\=\<\d\+\>" contained

" Float
syn match php3Float  "\(-\=\<\d+\|-\=\)\.\d\+\>" contained

" SpecialChar
syn match php3SpecialChar "\\[abcfnrtyv\\]" contained
syn match php3SpecialChar "\\\d\{3}" contained contains=php3OctalError
syn match php3SpecialChar "\\x[0-9a-fA-F]\{2}" contained

" Error
syn match php3OctalError "[89]" contained
syn match php3ParentError "[)}\]]" contained

" Todo
syn keyword php3Todo TODO Todo todo contained

" Parents
syn cluster php3Inside contains=php3Comment,php3Functions,php3Identifier,php3Conditional,php3Repeat,php3Label,php3Statement,php3Operator,php3Relation,php3StringSingle,php3StringDouble,php3Number,php3Float,php3SpecialChar,php3Parent,php3ParentError,php3Include,php3Keyword,php3Type,php3IdentifierParent,php3Boolean,php3Structure,php3Methods

syn cluster php3Top contains=@php3Inside,php3Define,php3ParentError,php3StorageClass

syn region php3Parent	matchgroup=Delimiter start="(" end=")" contained contains=@php3Inside
syn region php3Parent	matchgroup=Delimiter start="{" end="}" contained contains=@php3Top
syn region php3Parent	matchgroup=Delimiter start="\[" end="\]" contained contains=@php3Inside

syn region php3Region keepend matchgroup=Delimiter start="<?\(php\)\=" skip=+".\{-}?>.\{-}"\|'.\{-}?>.\{-}'\|/\*.\{-}?>.\{-}\*/+ end="?>" contains=@php3Top
syn region php3RegionInsideHtmlTags keepend matchgroup=Delimiter start="<?\(php\)\=" end="?>" contains=@php3Top contained

syn region php3Region keepend matchgroup=Delimiter start=+<script language="php">+ skip=+".\{-}</script>.\{-}"\|'.\{-}</script>.\{-}'\|/\*.\{-}</script>.\{-}\*/+ end=+</script>+ contains=@php3Top
syn region php3RegionInsideHtmlTags keepend matchgroup=Delimiter start=+<script language="php">+  end=+</script>+ contains=@php3Top contained

" sync
if exists("php3_minlines")
  exec "syn sync minlines=" . php3_minlines
else
  syn sync minlines=100
endif

if !exists("did_php3_syntax_inits")
  let did_php3_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link php3Comment		Comment
  hi link php3Boolean		Boolean
  hi link php3StorageClass	StorageClass
  hi link php3Structure		Structure
  hi link php3StringSingle	String
  hi link php3StringDouble	String
  hi link php3Number		Number
  hi link php3Float		Float
  hi link php3Identifier	Identifier
  hi link php3IntVar		Identifier
  hi link php3EnvVar		Identifier
  hi link php3Functions		Function
  hi link php3Baselib		Function
  hi link php3Repeat		Repeat
  hi link php3Conditional	Conditional
  hi link php3Label		Label
  hi link php3Statement		Statement
  hi link php3Keyword		Statement
  hi link php3Type		Type
  hi link php3Include		Include
  hi link php3Define		Define
  hi link php3SpecialChar	SpecialChar
  hi link php3ParentError	Delimiter
  hi link php3OctalError	Error
  hi link php3Todo		Todo
  hi link php3Operator		Operator
  hi link php3Relation		Operator
endif

let b:current_syntax = "php3"

if main_syntax == 'php3'
  unlet main_syntax
endif

" vim: ts=8
