/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * This file defines the Ex commands.
 * When DO_DECLARE_EXCMD is defined, the table with ex command names and
 * options results.
 * When DO_DECLARE_EXCMD is NOT defined, the enum with all the Ex commands
 * results.
 * This clever trick was invented by Ron Aaron.
 */

/*
 * When adding an Ex command:
 * 1. Add an entry in the table below.  Keep it sorted on the shortest
 *    version of the command name that works.  If it doesn't start with a
 *    lower case letter, add it at the end.
 * 2. Add a "case: CMD_xxx" in the big switch in ex_docmd.c.
 * 3. Add an entry in the index for Ex commands at ":help ex-cmd-index".
 * 4. Add documentation in ../doc/xxx.txt.  Add a tag for both the short and
 *    long name of the command.
 */

#undef EXCMD	    /* just in case */
#ifdef DO_DECLARE_EXCMD
# define EXCMD(a, b, c)  {(char_u *)b, c}
#else
# define EXCMD(a, b, c)  a
#endif

#ifdef RANGE
# undef RANGE			/* SASC on Amiga defines it */
#endif

#define RANGE	   0x01		/* allow a linespecs */
#define BANG	   0x02		/* allow a ! after the command name */
#define EXTRA	   0x04		/* allow extra args after command name */
#define XFILE	   0x08		/* expand wildcards in extra part */
#define NOSPC	   0x10		/* no spaces allowed in the extra part */
#define	DFLALL	   0x20		/* default file range is 1,$ */
#define NODFL	   0x40		/* do not default to the current file name */
#define NEEDARG	   0x80		/* argument required */
#define TRLBAR	  0x100		/* check for trailing vertical bar */
#define REGSTR	  0x200		/* allow "x for register designation */
#define COUNT	  0x400		/* allow count in argument, after command */
#define NOTRLCOM  0x800		/* no trailing comment allowed */
#define ZEROR	 0x1000		/* zero line number allowed */
#define USECTRLV 0x2000		/* do not remove CTRL-V from argument */
#define NOTADR	 0x4000		/* number before command is not an address */
#define EDITCMD	 0x8000		/* has "+command" argument */
#define BUFNAME 0x10000		/* accepts buffer name */
#define FILES	(XFILE | EXTRA)	/* multiple extra files allowed */
#define WORD1	(EXTRA | NOSPC)	/* one extra word allowed */
#define FILE1	(FILES | NOSPC)	/* 1 file allowed, defaults to current file */
#define NAMEDF	(FILE1 | NODFL)	/* 1 file allowed, defaults to "" */
#define NAMEDFS	(FILES | NODFL)	/* multiple files allowed, default is "" */

/*
 * This array maps ex command names to command codes.
 * The order in which command names are listed below is significant --
 * ambiguous abbreviations are always resolved to be the first possible match
 * (e.g. "r" is taken to mean "read", not "rewind", because "read" comes
 * before "rewind").
 * Not supported commands are included to avoid ambiguities.
 */
#ifdef DO_DECLARE_EXCMD
static struct cmdname
{
    char_u  *cmd_name;	/* name of the command */
    long_u   cmd_argt;	/* command line arguments permitted/needed/used */
} cmdnames[] =
#else
enum CMD_index
#endif
{
    EXCMD(CMD_append,	"append",	BANG|RANGE|ZEROR|TRLBAR),
    EXCMD(CMD_abbreviate,"abbreviate",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_abclear,	"abclear",	TRLBAR),
    EXCMD(CMD_all,	"all",		RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_amenu,	"amenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_anoremenu,"anoremenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_args,	"args",		BANG|NAMEDFS|EDITCMD|TRLBAR),
    EXCMD(CMD_argument, "argument",	BANG|RANGE|NOTADR|COUNT|EXTRA|EDITCMD|TRLBAR),
    EXCMD(CMD_ascii,	"ascii",	TRLBAR),
    EXCMD(CMD_autocmd,	"autocmd",	BANG|EXTRA|NOTRLCOM|USECTRLV),
    EXCMD(CMD_augroup,	"augroup",	WORD1|TRLBAR),
    EXCMD(CMD_aunmenu,	"aunmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_buffer,	"buffer",	BANG|RANGE|NOTADR|BUFNAME|COUNT|EXTRA|TRLBAR),
    EXCMD(CMD_bNext,	"bNext",	BANG|RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_ball,	"ball",		RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_badd,	"badd",		NEEDARG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_bdelete,	"bdelete",	BANG|RANGE|NOTADR|BUFNAME|COUNT|EXTRA|TRLBAR),
    EXCMD(CMD_behave,	"behave",	NEEDARG|WORD1|TRLBAR),
    EXCMD(CMD_blast,	"blast",	BANG|RANGE|TRLBAR),
    EXCMD(CMD_bmodified,"bmodified",	BANG|RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_bnext,	"bnext",	BANG|RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_bprevious,"bprevious",	BANG|RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_brewind,	"brewind",	BANG|RANGE|TRLBAR),
    EXCMD(CMD_break,	"break",	TRLBAR),
    EXCMD(CMD_browse,	"browse",	NEEDARG|EXTRA|NOTRLCOM),
    EXCMD(CMD_buffers,	"buffers",	TRLBAR),
    EXCMD(CMD_bunload,	"bunload",	BANG|RANGE|NOTADR|BUFNAME|COUNT|EXTRA|TRLBAR),
    EXCMD(CMD_change,	"change",	BANG|RANGE|COUNT|TRLBAR),
    EXCMD(CMD_cNext,	"cNext",	RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_cabbrev,	"cabbrev",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cabclear, "cabclear",	TRLBAR),
    EXCMD(CMD_call,	"call",		RANGE|NEEDARG|EXTRA|NOTRLCOM),
    EXCMD(CMD_cc,	"cc",		RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_cd,	"cd",		NAMEDF|TRLBAR),
    EXCMD(CMD_center,	"center",	TRLBAR|RANGE|EXTRA),
    EXCMD(CMD_cfile,	"cfile",	TRLBAR|FILE1|BANG),
    EXCMD(CMD_chdir,	"chdir",	NAMEDF|TRLBAR),
    EXCMD(CMD_checkpath,"checkpath",	TRLBAR|BANG),
    EXCMD(CMD_clist,	"clist",	BANG|EXTRA|TRLBAR),
    EXCMD(CMD_clast,	"clast",	RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_close,	"close",	BANG|TRLBAR),
    EXCMD(CMD_cmap,	"cmap",		EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cmapclear,"cmapclear",	TRLBAR),
    EXCMD(CMD_cmenu,	"cmenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cnext,	"cnext",	RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_cnewer,	"cnewer",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_cnfile,	"cnfile",	RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_cnoremap, "cnoremap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cnoreabbrev,"cnoreabbrev",EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cnoremenu,"cnoremenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_copy,	"copy",		RANGE|EXTRA|TRLBAR),
    EXCMD(CMD_colder,	"colder",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_command,	"command",	EXTRA|BANG|NOTRLCOM|USECTRLV),
    EXCMD(CMD_comclear,	"comclear",	TRLBAR),
    EXCMD(CMD_continue, "continue",	TRLBAR),
    EXCMD(CMD_confirm,  "confirm",	NEEDARG|EXTRA|NOTRLCOM),
    EXCMD(CMD_cprevious,"cprevious",	RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_cquit,	"cquit",	TRLBAR|BANG),
    EXCMD(CMD_crewind,	"crewind",	RANGE|NOTADR|COUNT|TRLBAR|BANG),
    EXCMD(CMD_cscope,   "cscope",       EXTRA|NOTRLCOM),
    EXCMD(CMD_cstag,	"cstag",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_cunmap,	"cunmap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cunabbrev,"cunabbrev",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_cunmenu,	"cunmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_delete,	"delete",	RANGE|REGSTR|COUNT|TRLBAR),
    EXCMD(CMD_delcommand,"delcommand",	NEEDARG|WORD1|TRLBAR),
    EXCMD(CMD_delfunction,"delfunction",NEEDARG|WORD1|TRLBAR),
    EXCMD(CMD_display,	"display",	EXTRA|NOTRLCOM|TRLBAR),
    EXCMD(CMD_digraphs, "digraphs",	EXTRA|TRLBAR),
    EXCMD(CMD_djump,	"djump",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_dlist,	"dlist",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_doautocmd,"doautocmd",	EXTRA|TRLBAR),
    EXCMD(CMD_doautoall,"doautoall",	EXTRA|TRLBAR),
    EXCMD(CMD_dsearch,	"dsearch",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_dsplit,	"dsplit",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_edit,	"edit",		BANG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_echo,	"echo",		EXTRA|NOTRLCOM),
    EXCMD(CMD_echohl,	"echohl",	EXTRA|TRLBAR),
    EXCMD(CMD_echon,	"echon",	EXTRA|NOTRLCOM),
    EXCMD(CMD_else,	"else",		TRLBAR),
    EXCMD(CMD_elseif,	"elseif",	EXTRA|NOTRLCOM),
    EXCMD(CMD_exemenu,	"emenu",	NEEDARG+EXTRA+TRLBAR+NOTRLCOM),
    EXCMD(CMD_endif,	"endif",	TRLBAR),
    EXCMD(CMD_endfunction,"endfunction",TRLBAR),
    EXCMD(CMD_endwhile,	"endwhile",	TRLBAR),
    EXCMD(CMD_ex,	"ex",		BANG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_execute,	"execute",	EXTRA|NOTRLCOM),
    EXCMD(CMD_exit,	"exit",		RANGE|BANG|FILE1|DFLALL|TRLBAR),
    EXCMD(CMD_file,	"file",		BANG|FILE1|TRLBAR),
    EXCMD(CMD_files,	"files",	TRLBAR),
    EXCMD(CMD_filetype,	"filetype",	NEEDARG|WORD1|TRLBAR),
    EXCMD(CMD_find,	"find",		BANG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_fixdel,	"fixdel",	TRLBAR),
    EXCMD(CMD_function,	"function",	EXTRA|BANG),
    EXCMD(CMD_global,	"global",	RANGE|BANG|EXTRA|DFLALL),
    EXCMD(CMD_goto,	"goto",		RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_grep,	"grep",		NEEDARG|EXTRA|NOTRLCOM|TRLBAR|XFILE),
    EXCMD(CMD_gui,	"gui",		BANG|NAMEDFS|EDITCMD|TRLBAR),
    EXCMD(CMD_gvim,	"gvim",		BANG|NAMEDFS|EDITCMD|TRLBAR),
    EXCMD(CMD_help,	"help",		EXTRA|NOTRLCOM),
    EXCMD(CMD_helpfind,	"helpfind",	EXTRA|NOTRLCOM),
    EXCMD(CMD_highlight,"highlight",	BANG|EXTRA|TRLBAR),
    EXCMD(CMD_hide,	"hide",		BANG|TRLBAR),
    EXCMD(CMD_history,	"history",	EXTRA|TRLBAR),
    EXCMD(CMD_insert,	"insert",	BANG|RANGE|TRLBAR),
    EXCMD(CMD_iabbrev,	"iabbrev",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_iabclear, "iabclear",	TRLBAR),
    EXCMD(CMD_if,	"if",		EXTRA|NOTRLCOM),
    EXCMD(CMD_ijump,	"ijump",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_ilist,	"ilist",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_imap,	"imap",		EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_imapclear,"imapclear",	TRLBAR),
    EXCMD(CMD_imenu,	"imenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_inoremap, "inoremap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_inoreabbrev,"inoreabbrev",EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_inoremenu,"inoremenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_intro,	"intro",	TRLBAR),
    EXCMD(CMD_isearch,	"isearch",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_isplit,	"isplit",	BANG|RANGE|DFLALL|EXTRA),
    EXCMD(CMD_iunmap,	"iunmap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_iunabbrev,"iunabbrev",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_iunmenu,	"iunmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_join,	"join",		BANG|RANGE|COUNT|TRLBAR),
    EXCMD(CMD_jumps,	"jumps",	TRLBAR),
    EXCMD(CMD_k,	"k",		RANGE|WORD1|TRLBAR),
    EXCMD(CMD_list,	"list",		RANGE|COUNT|TRLBAR),
    EXCMD(CMD_last,	"last",		EXTRA|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_left,	"left",		TRLBAR|RANGE|EXTRA),
    EXCMD(CMD_let,	"let",		EXTRA|NOTRLCOM),
    EXCMD(CMD_ls,	"ls",		TRLBAR),
    EXCMD(CMD_move,	"move",		RANGE|EXTRA|TRLBAR),
    EXCMD(CMD_mark,	"mark",		RANGE|WORD1|TRLBAR),
    EXCMD(CMD_make,	"make",		EXTRA|NOTRLCOM|TRLBAR|XFILE),
    EXCMD(CMD_map,	"map",		BANG|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_mapclear,	"mapclear",	BANG|TRLBAR),
    EXCMD(CMD_marks,	"marks",	EXTRA|TRLBAR),
    EXCMD(CMD_menu,	"menu",		RANGE|NOTADR|ZEROR|BANG|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_messages,	"messages",	TRLBAR),
    /* EXCMD(CMD_mfstat,	"mfstat",	TRLBAR),  for debugging */
    EXCMD(CMD_mkexrc,	"mkexrc",	BANG|FILE1|TRLBAR),
    EXCMD(CMD_mksession,"mksession",	BANG|FILE1|TRLBAR),
    EXCMD(CMD_mkvimrc,	"mkvimrc",	BANG|FILE1|TRLBAR),
    EXCMD(CMD_mode,	"mode",		WORD1|TRLBAR),
    EXCMD(CMD_next,	"next",		RANGE|NOTADR|BANG|NAMEDFS|EDITCMD|TRLBAR),
    EXCMD(CMD_new,	"new",		BANG|FILE1|RANGE|NOTADR|EDITCMD|TRLBAR),
    EXCMD(CMD_nmap,	"nmap",		EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_nmapclear,"nmapclear",	TRLBAR),
    EXCMD(CMD_nmenu,	"nmenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_nnoremap, "nnoremap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_nnoremenu,"nnoremenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_noremap,	"noremap",	BANG|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_nohlsearch,"nohlsearch",	TRLBAR),
    EXCMD(CMD_noreabbrev,"noreabbrev",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_noremenu, "noremenu",	RANGE|NOTADR|ZEROR|BANG|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_normal,	"normal",	RANGE|BANG|EXTRA|NEEDARG|NOTRLCOM|USECTRLV),
    EXCMD(CMD_number,	"number",	RANGE|COUNT|TRLBAR),
    EXCMD(CMD_nunmap,	"nunmap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_nunmenu,	"nunmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_open,	"open",		TRLBAR),	/* not supported */
    EXCMD(CMD_omap,	"omap",		EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_omapclear,"omapclear",	TRLBAR),
    EXCMD(CMD_omenu,	"omenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_only,	"only",		BANG|TRLBAR),
    EXCMD(CMD_onoremap, "onoremap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_onoremenu,"onoremenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_options,	"options",	TRLBAR),
    EXCMD(CMD_ounmap,	"ounmap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_ounmenu,	"ounmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_print,	"print",	RANGE|COUNT|TRLBAR),
    EXCMD(CMD_pclose,	"pclose",	BANG|TRLBAR),
    EXCMD(CMD_perl,	"perl",		RANGE|EXTRA|DFLALL|NEEDARG),
    EXCMD(CMD_perldo,	"perldo",	RANGE|EXTRA|DFLALL|NEEDARG),
    EXCMD(CMD_pop,	"pop",		RANGE|NOTADR|BANG|COUNT|TRLBAR|ZEROR),
    EXCMD(CMD_ppop,	"ppop",		RANGE|NOTADR|BANG|COUNT|TRLBAR|ZEROR),
    EXCMD(CMD_preserve, "preserve",	TRLBAR),
    EXCMD(CMD_previous, "previous",	EXTRA|RANGE|NOTADR|COUNT|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_promptfind, "promptfind",	EXTRA|NOTRLCOM),
    EXCMD(CMD_promptrepl, "promptrepl", EXTRA|NOTRLCOM),
    EXCMD(CMD_ptag,	"ptag",		RANGE|NOTADR|BANG|WORD1|TRLBAR|ZEROR),
    EXCMD(CMD_ptNext,	"ptNext",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_ptjump,	"ptjump",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_ptlast,	"ptlast",	BANG|TRLBAR),
    EXCMD(CMD_ptnext,	"ptnext",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_ptprevious,"ptprevious",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_ptrewind,	"ptrewind",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_ptselect,	"ptselect",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_put,	"put",		RANGE|BANG|REGSTR|TRLBAR|ZEROR),
    EXCMD(CMD_pwd,	"pwd",		TRLBAR),
    EXCMD(CMD_python,	"python",	RANGE|EXTRA|NEEDARG),
    EXCMD(CMD_pyfile,	"pyfile",	RANGE|FILE1|NEEDARG),
    EXCMD(CMD_quit,	"quit",		BANG|TRLBAR),
    EXCMD(CMD_qall,	"qall",		BANG|TRLBAR),
    EXCMD(CMD_read,	"read",		BANG|RANGE|NAMEDF|TRLBAR|ZEROR),
    EXCMD(CMD_recover,	"recover",	BANG|FILE1|TRLBAR),
    EXCMD(CMD_redo,	"redo",		TRLBAR),
    EXCMD(CMD_redir,	"redir",	BANG|FILES|TRLBAR),
    EXCMD(CMD_registers,"registers",	EXTRA|NOTRLCOM|TRLBAR),
    EXCMD(CMD_resize,	"resize",	TRLBAR|WORD1),
    EXCMD(CMD_retab,	"retab",	TRLBAR|RANGE|DFLALL|BANG|WORD1),
    EXCMD(CMD_return,	"return",	EXTRA|NOTRLCOM),
    EXCMD(CMD_rewind,	"rewind",	EXTRA|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_right,	"right",	TRLBAR|RANGE|EXTRA),
    EXCMD(CMD_rviminfo, "rviminfo",	BANG|FILE1|TRLBAR),
    EXCMD(CMD_substitute,"substitute",	RANGE|EXTRA),
    EXCMD(CMD_sNext,	"sNext",	EXTRA|RANGE|NOTADR|COUNT|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_sargument,"sargument",	BANG|RANGE|NOTADR|COUNT|EXTRA|EDITCMD|TRLBAR),
    EXCMD(CMD_sall,	"sall",		RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_sbuffer,	"sbuffer",	BANG|RANGE|NOTADR|BUFNAME|COUNT|EXTRA|TRLBAR),
    EXCMD(CMD_sbNext,	"sbNext",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_sball,	"sball",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_sblast,	"sblast",	TRLBAR),
    EXCMD(CMD_sbmodified,"sbmodified",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_sbnext,	"sbnext",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_sbprevious,"sbprevious",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_sbrewind, "sbrewind",	TRLBAR),
    EXCMD(CMD_set,	"set",		EXTRA|TRLBAR),
    EXCMD(CMD_sfind,	"sfind",	BANG|FILE1|RANGE|NOTADR|EDITCMD|TRLBAR),
    EXCMD(CMD_shell,	"shell",	TRLBAR),
#ifdef USE_GUI_MSWIN
    EXCMD(CMD_simalt,	"simalt",	NEEDARG|WORD1|TRLBAR),
#endif
    EXCMD(CMD_sleep,	"sleep",	RANGE|COUNT|NOTADR|EXTRA|TRLBAR),
    EXCMD(CMD_slast,	"slast",	EXTRA|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_smagic,	"smagic",	RANGE|EXTRA),
    EXCMD(CMD_snext,	"snext",	RANGE|NOTADR|BANG|NAMEDFS|EDITCMD|TRLBAR),
    EXCMD(CMD_sniff,	"sniff",	EXTRA|TRLBAR),
    EXCMD(CMD_snomagic,	"snomagic",	RANGE|EXTRA),
    EXCMD(CMD_source,	"source",	BANG|NAMEDF|TRLBAR),
    EXCMD(CMD_split,	"split",	BANG|FILE1|RANGE|NOTADR|EDITCMD|TRLBAR),
    EXCMD(CMD_sprevious,"sprevious",	EXTRA|RANGE|NOTADR|COUNT|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_srewind,	"srewind",	EXTRA|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_stop,	"stop",		TRLBAR|BANG),
    EXCMD(CMD_stag,	"stag",		RANGE|NOTADR|BANG|WORD1|TRLBAR|ZEROR),
    EXCMD(CMD_startinsert,"startinsert",BANG|TRLBAR),
    EXCMD(CMD_stjump,	"stjump",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_stselect,	"stselect",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_sunhide,	"sunhide",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_suspend,	"suspend",	TRLBAR|BANG),
    EXCMD(CMD_sview,	"sview",	NEEDARG|RANGE|BANG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_swapname,	"swapname",	TRLBAR),
    EXCMD(CMD_syntax,	"syntax",	EXTRA|NOTRLCOM),
    EXCMD(CMD_syncbind,	"syncbind",	TRLBAR),
    EXCMD(CMD_t,	"t",		RANGE|EXTRA|TRLBAR),
    EXCMD(CMD_tNext,	"tNext",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_tag,	"tag",		RANGE|NOTADR|BANG|WORD1|TRLBAR|ZEROR),
    EXCMD(CMD_tags,	"tags",		TRLBAR),
    EXCMD(CMD_tcl,	"tcl",		RANGE|EXTRA|NEEDARG),
    EXCMD(CMD_tcldo,	"tcldo",	RANGE|DFLALL|EXTRA|NEEDARG),
    EXCMD(CMD_tclfile,	"tclfile",	RANGE|FILE1|NEEDARG),
    EXCMD(CMD_tearoff,	"tearoff",	NEEDARG|EXTRA|TRLBAR|NOTRLCOM),
    EXCMD(CMD_tjump,	"tjump",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_tlast,	"tlast",	BANG|TRLBAR),
    EXCMD(CMD_tmenu,	"tmenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_tnext,	"tnext",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_tprevious,"tprevious",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_trewind,	"trewind",	RANGE|NOTADR|BANG|TRLBAR|ZEROR),
    EXCMD(CMD_tselect,	"tselect",	BANG|TRLBAR|WORD1),
    EXCMD(CMD_tunmenu,	"tunmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_undo,	"undo",		TRLBAR),
    EXCMD(CMD_unabbreviate,"unabbreviate",EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_unhide,	"unhide",	RANGE|NOTADR|COUNT|TRLBAR),
    EXCMD(CMD_unlet,	"unlet",	BANG|EXTRA|NEEDARG|TRLBAR),
    EXCMD(CMD_unmap,	"unmap",	BANG|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_unmenu,	"unmenu",	BANG|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_update,	"update",	RANGE|BANG|FILE1|DFLALL|TRLBAR),
    EXCMD(CMD_vglobal,	"vglobal",	RANGE|EXTRA|DFLALL),
    EXCMD(CMD_version,	"version",	EXTRA|TRLBAR),
    EXCMD(CMD_visual,	"visual",	RANGE|BANG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_view,	"view",		RANGE|BANG|FILE1|EDITCMD|TRLBAR),
    EXCMD(CMD_vmap,	"vmap",		EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_vmapclear,"vmapclear",	TRLBAR),
    EXCMD(CMD_vmenu,	"vmenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_vnoremap, "vnoremap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_vnoremenu,"vnoremenu",	RANGE|NOTADR|ZEROR|EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_vunmap,	"vunmap",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_vunmenu,	"vunmenu",	EXTRA|TRLBAR|NOTRLCOM|USECTRLV),
    EXCMD(CMD_write,	"write",	RANGE|BANG|FILE1|DFLALL|TRLBAR),
    EXCMD(CMD_wNext,	"wNext",	RANGE|NOTADR|BANG|FILE1|TRLBAR),
    EXCMD(CMD_wall,	"wall",		BANG|TRLBAR),
    EXCMD(CMD_while,	"while",	EXTRA|NOTRLCOM),
    EXCMD(CMD_winsize,	"winsize",	EXTRA|NEEDARG|TRLBAR),
    EXCMD(CMD_winpos,	"winpos",	EXTRA|TRLBAR),
    EXCMD(CMD_wnext,	"wnext",	RANGE|NOTADR|BANG|FILE1|TRLBAR),
    EXCMD(CMD_wprevious,"wprevious",	RANGE|NOTADR|BANG|FILE1|TRLBAR),
    EXCMD(CMD_wq,	"wq",		RANGE|BANG|FILE1|DFLALL|TRLBAR),
    EXCMD(CMD_wqall,	"wqall",	BANG|FILE1|DFLALL|TRLBAR),
    EXCMD(CMD_wviminfo, "wviminfo",	BANG|FILE1|TRLBAR),
    EXCMD(CMD_xit,	"xit",		RANGE|BANG|FILE1|DFLALL|TRLBAR),
    EXCMD(CMD_xall,	"xall",		BANG|TRLBAR),
    EXCMD(CMD_yank,	"yank",		RANGE|REGSTR|COUNT|TRLBAR),
    EXCMD(CMD_z,	"z",		RANGE|EXTRA|TRLBAR),

/* commands that don't start with a lowercase letter */
    EXCMD(CMD_bang,	"!",		RANGE|BANG|NAMEDFS),
    EXCMD(CMD_pound,	"#",		RANGE|COUNT|TRLBAR),
    EXCMD(CMD_and,	"&",		RANGE|EXTRA),
    EXCMD(CMD_star,	"*",		RANGE|EXTRA|TRLBAR),
    EXCMD(CMD_lshift,	"<",		RANGE|COUNT|TRLBAR),
    EXCMD(CMD_equal,	"=",		RANGE|TRLBAR),
    EXCMD(CMD_rshift,	">",		RANGE|COUNT|TRLBAR),
    EXCMD(CMD_at,	"@",		RANGE|EXTRA|TRLBAR),
    EXCMD(CMD_Next,	"Next",		EXTRA|RANGE|NOTADR|COUNT|BANG|EDITCMD|TRLBAR),
    EXCMD(CMD_Print,	"Print",	RANGE|COUNT|TRLBAR),
    EXCMD(CMD_X,	"X",		TRLBAR),
    EXCMD(CMD_tilde,	"~",		RANGE|EXTRA),

#ifndef DO_DECLARE_EXCMD
#ifdef USER_COMMANDS
    CMD_SIZE,		/* MUST be after all real commands! */
    CMD_USER = -1	/* User-defined command */
#else
    CMD_SIZE	/* MUST be the last one! */
#endif
#endif
};

#ifndef DO_DECLARE_EXCMD
typedef enum CMD_index CMDIDX;

/*
 * Arguments used for Ex commands.
 */
typedef struct exarg
{
    char_u	*arg;	    /* argument of the command */
    char_u	*nextcmd;   /* next command (NULL if none) */
    char_u	*cmd;	    /* the name of the command (except for :make) */
    CMDIDX	cmdidx;	    /* the index for the command */
    long	argt;	    /* flags for the command */
    int		skip;	    /* don't execute the command, only parse it */
    int		forceit;    /* TRUE if ! present */
    int		addr_count; /* the number of addresses given */
    linenr_t	line1;	    /* the first line number */
    linenr_t	line2;	    /* the second line number or count */
    char_u	*do_ecmd_cmd; /* +command argument to be used in edited file */
    linenr_t	do_ecmd_lnum; /* the line number in an edited file */
    int		append;	    /* TRUE with ":w >>file" command */
    int		usefilter;  /* TRUE with ":w !command" and ":r!command" */
    int		amount;	    /* number of '>' or '<' for shift command */
    int		regname;    /* register name (NUL if none) */
#ifdef USER_COMMANDS
    int		useridx;    /* user command index */
#endif
} EXARG;
#endif
