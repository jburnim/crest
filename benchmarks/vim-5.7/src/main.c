/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#if defined(MSDOS) || defined(WIN32)
# include <io.h>		/* for close() and dup() */
#endif

#define EXTERN
#include "vim.h"

#ifdef SPAWNO
# include <spawno.h>		/* special MSDOS swapping library */
#endif

#ifdef HAVE_TCL
# undef EXTERN			/* redefined in tcl.h */
# include <tcl.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

static void mainerr __ARGS((int, char_u *));
static void main_msg __ARGS((char *s));
static void usage __ARGS((void));
static int get_number_arg __ARGS((char_u *p, int *idx, int def));

/*
 * Type of error message.  These must match with errors[] in mainerr().
 */
#define ME_UNKNOWN_OPTION	0
#define ME_TOO_MANY_ARGS	1
#define ME_ARG_MISSING		2
#define ME_GARBAGE		3
#define ME_EXTRA_CMD		4

    static void
mainerr(n, str)
    int	    n;
    char_u  *str;
{
    static char	*(errors[]) =
    {
	"Unknown option",
	"Too many edit arguments",
	"Argument missing after",
	"Garbage after option",
	"Too many \"+command\" or \"-c command\" arguments",
    };

#if defined(UNIX) || defined(__EMX__) || defined(VMS)
    reset_signals();		/* kill us with CTRL-C here, if you like */
#endif

    mch_errmsg(longVersion);
    mch_errmsg("\n");
    mch_errmsg(errors[n]);
    if (str != NULL)
    {
	mch_errmsg(": \"");
	mch_errmsg((char *)str);
	mch_errmsg("\"");
    }
    mch_errmsg("\nMore info with: \"vim -h\"\n");

    mch_windexit(1);
}

/*
 * print a message with three spaces prepended and '\n' appended.
 */
    static void
main_msg(s)
    char *s;
{
    mch_msg("   ");
    mch_msg(s);
    mch_msg("\n");
}

    static void
usage()
{
    int		    i;
    static char_u *(use[]) =
    {
	(char_u *)"[file ..]       edit specified file(s)",
	(char_u *)"-               read text from stdin",
	(char_u *)"-t tag          edit file where tag is defined",
#ifdef QUICKFIX
	(char_u *)"-q [errorfile]  edit file with first error"
#endif
    };

#if defined(UNIX) || defined(__EMX__) || defined(VMS)
    reset_signals();		/* kill us with CTRL-C here, if you like */
#endif

    mch_msg(longVersion);
    mch_msg("\nusage:");
    for (i = 0; ; ++i)
    {
	mch_msg(" vim [options] ");
	mch_msg((char *)use[i]);
	if (i == (sizeof(use) / sizeof(char_u *)) - 1)
	    break;
	mch_msg("\n   or:");
    }

    mch_msg("\n\nOptions:\n");
    main_msg("--\t\t\tEnd of options");
#ifdef HAVE_OLE
    main_msg("-register\t\tRegister this gvim for OLE");
    main_msg("-unregister\t\tUnregister gvim for OLE");
#endif
#ifdef USE_GUI
    main_msg("-g\t\t\tRun using GUI (like \"gvim\")");
    main_msg("-f\t\t\tForeground: Don't fork when starting GUI");
#endif
    main_msg("-v\t\t\tVi mode (like \"vi\")");
    main_msg("-e\t\t\tEx mode (like \"ex\")");
    main_msg("-s\t\t\tSilent (batch) mode (only for \"ex\")");
    main_msg("-R\t\t\tReadonly mode (like \"view\")");
    main_msg("-Z\t\t\tRestricted mode (like \"rvim\")");
    main_msg("-m\t\t\tModifications (writing files) not allowed");
    main_msg("-b\t\t\tBinary mode");
#ifdef LISPINDENT
    main_msg("-l\t\t\tLisp mode");
#endif
    main_msg("-C\t\t\tCompatible with Vi: 'compatible'");
    main_msg("-N\t\t\tNot fully Vi compatible: 'nocompatible'");
    main_msg("-V[N]\t\tVerbose level");
    main_msg("-n\t\t\tNo swap file, use memory only");
    main_msg("-r\t\t\tList swap files and exit");
    main_msg("-r (with file name)\tRecover crashed session");
    main_msg("-L\t\t\tSame as -r");
#ifdef AMIGA
    main_msg("-f\t\t\tDon't use newcli to open window");
    main_msg("-d <device>\t\tUse <device> for I/O");
#endif
#ifdef RIGHTLEFT
    main_msg("-H\t\t\tstart in Hebrew mode");
#endif
#ifdef FKMAP
    main_msg("-F\t\t\tstart in Farsi mode");
#endif
    main_msg("-T <terminal>\tSet terminal type to <terminal>");
    main_msg("-o[N]\t\tOpen N windows (default: one for each file)");
    main_msg("+\t\t\tStart at end of file");
    main_msg("+<lnum>\t\tStart at line <lnum>");
    main_msg("-c <command>\t\tExecute <command> first");
    main_msg("-s <scriptin>\tRead commands from script file <scriptin>");
    main_msg("-w <scriptout>\tAppend commands to script file <scriptout>");
    main_msg("-W <scriptout>\tWrite commands to script file <scriptout>");
    main_msg("-u <vimrc>\t\tUse <vimrc> instead of any .vimrc");
#ifdef USE_GUI
    main_msg("-U <gvimrc>\t\tUse <gvimrc> instead of any .gvimrc");
#endif
#ifdef CRYPTV
    main_msg("-x\t\t\tEdit encrypted files");
#endif
#ifdef VIMINFO
    main_msg("-i <viminfo>\t\tUse <viminfo> instead of .viminfo");
#endif
    main_msg("-h\t\t\tprint Help (this message) and exit");
    main_msg("--version\t\tprint version information and exit");

#ifdef USE_GUI_X11
# ifdef USE_GUI_MOTIF
    mch_msg("\nOptions recognised by gvim (Motif version):\n");
# else
#  ifdef USE_GUI_ATHENA
    mch_msg("\nOptions recognised by gvim (Athena version):\n");
#  endif
# endif
    main_msg("-display <display>\tRun vim on <display>");
    main_msg("-iconic\t\tStart vim iconified");
# if 0
    main_msg("-name <name>\t\tUse resource as if vim was <name>");
    mch_msg("\t\t\t  (Unimplemented)\n");
# endif
    main_msg("-background <color>\tUse <color> for the background (also: -bg)");
    main_msg("-foreground <color>\tUse <color> for normal text (also: -fg)");
    main_msg("-font <font>\t\tUse <font> for normal text (also: -fn)");
    main_msg("-boldfont <font>\tUse <font> for bold text");
    main_msg("-italicfont <font>\tUse <font> for italic text");
    main_msg("-geometry <geom>\tUse <geom> for initial geometry (also: -geom)");
    main_msg("-borderwidth <width>\tUse a border width of <width> (also: -bw)");
    main_msg("-scrollbarwidth <width>\tUse a scrollbar width of <width> (also: -sw)");
    main_msg("-menuheight <height>\tUse a menu bar height of <height> (also: -mh)");
    main_msg("-reverse\t\tUse reverse video (also: -rv)");
    main_msg("+reverse\t\tDon't use reverse video (also: +rv)");
    main_msg("-xrm <resource>\tSet the specified resource");
#endif /* USE_GUI_X11 */
#if defined(USE_GUI) && defined(RISCOS)
    mch_msg("\nOptions recognised by gvim (RISC OS version):\n");
    main_msg("--columns <number>\tInitial width of window in columns");
    main_msg("--rows <number>\tInitial height of window in rows");
#endif
#ifdef USE_GUI_GTK
    mch_msg("\nOptions recognised by gvim (GTK+ version):\n");
    main_msg("-font <font>\t\tUse <font> for normal text (also: -fn)");
    main_msg("-geometry <geom>\tUse <geom> for initial geometry (also: -geom)");
    main_msg("-reverse\t\tUse reverse video (also: -rv)");
    main_msg("-display <display>\tRun vim on <display> (also: --display)");
#endif
    mch_windexit(1);
}

#if defined(GUI_DIALOG) || defined(CON_DIALOG)
static void check_swap_exists_action __ARGS((void));

/*
 * Check the result of the ATTENTION dialog:
 * When "Quit" selected, exit Vim.
 * When "Recover" selected, recover the file.
 */
    static void
check_swap_exists_action()
{
    if (swap_exists_action == SEA_QUIT)
	getout(1);
    if (swap_exists_action == SEA_RECOVER)
    {
	msg_scroll = TRUE;
	ml_recover();
	MSG_PUTS("\n");	/* don't overwrite the last message */
	cmdline_row = msg_row;
	do_modelines();
    }
}
#endif

#ifdef X_LOCALE
# include <X11/Xlocale.h>
#else
# ifdef HAVE_LOCALE_H
#  include <locale.h>
# endif
#endif

/* Maximum number of commands from + or -c options */
#define MAX_ARG_CMDS 10

#ifndef PROTO	    /* don't want a prototype for main() */
    int
# ifdef VIMDLL
_export
# endif
# ifdef USE_GUI_MSWIN
#  ifdef __BORLANDC__
_cdecl
#  endif
VimMain
# else
main
# endif
(argc, argv)
    int		    argc;
    char	  **argv;
{
    char_u	   *initstr;		    /* init string from environment */
    char_u	   *term = NULL;	    /* specified terminal name */
    char_u	   *fname = NULL;	    /* file name from command line */
    char_u	   *tagname = NULL;	    /* tag from -t option */
    char_u	   *use_vimrc = NULL;	    /* vimrc from -u option */
#ifdef QUICKFIX
    char_u	   *use_ef = NULL;	    /* 'errorfile' from -q option */
#endif
#ifdef CRYPTV
    int		    ask_for_key = FALSE;    /* -x argument */
#endif
    int		    n_commands = 0;	    /* no. of commands from + or -c */
    char_u	   *commands[MAX_ARG_CMDS]; /* commands from + or -c option */
    int		    no_swap_file = FALSE;   /* "-n" option used */
    int		    c;
    int		    i;
    int		    bin_mode = FALSE;	    /* -b option used */
    int		    window_count = 1;	    /* number of windows to use */
    int		    arg_idx = 0;	    /* index for arg_files[] */
    int		    had_minmin = FALSE;	    /* found "--" option */
    int		    argv_idx;		    /* index in argv[n][] */
    int		    want_full_screen = TRUE;
    int		    want_argument;	    /* option with argument */
#define EDIT_NONE   0	    /* no edit type yet */
#define EDIT_FILE   1	    /* file name argument[s] given, use arg_files[] */
#define EDIT_STDIN  2	    /* read file from stdin */
#define EDIT_TAG    3	    /* tag name argument given, use tagname */
#define EDIT_QF	    4	    /* start in quickfix mode */
    int		    edit_type = EDIT_NONE;  /* type of editing to do */
    int		    stdout_isatty;	    /* is stdout a terminal? */
    int		    input_isatty;	    /* is active input a terminal? */
    OPARG	    oa;			    /* operator arguments */

#ifdef RISCOS
    /* Turn off all the horrible filename munging in UnixLib. */
    __uname_control = __UNAME_NO_PROCESS;
#endif

#ifdef HAVE_TCL
    Tcl_FindExecutable(argv[0]);
#endif

#ifdef MEM_PROFILE
    atexit(vim_mem_profile_dump);
#endif

#ifdef __EMX__
    _wildcard(&argc, &argv);
#endif

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    setlocale(LC_ALL, "");	/* for ctype() and the like */
#endif

#ifdef MSWIN
    win32_init();		/* init toupper() and tolower() tables */
#endif

#ifdef USE_GUI_MAC
    /* Macinthosh needs this before any memory is allocated. */
    gui_prepare(&argc, argv);	/* Prepare for possibly starting GUI sometime */
#endif

#if defined(HAVE_DATE_TIME) && defined(VMS) && defined(VAXC)
    make_version();
#endif

    /*
     * Allocate space for the generic buffers (needed for set_init_1() and
     * EMSG2()).
     */
    if ((IObuff = alloc(IOSIZE)) == NULL
	    || (NameBuff = alloc(MAXPATHL)) == NULL)
	mch_windexit(0);

#ifdef XTERM_CLIP
    /*
     * Get the name of the display, before gui_prepare() removes it from
     * argv[].  Used for the xterm-clipboard display.
     */
    for (i = 0; i < argc; i++)
    {
	if (STRCMP(argv[i], "-display") == 0
#ifdef USE_GUI_GTK
		|| STRCMP(argv[i], "--display") == 0
#endif
		)
	{
	    xterm_display = argv[i + 1];
	    break;
	}
    }
#endif

#if defined(USE_GUI) && !defined(USE_GUI_MAC)
    gui_prepare(&argc, argv);	/* Prepare for possibly starting GUI sometime */
#endif

#ifdef USE_CLIPBOARD
    clip_init(FALSE);		/* Initialise clipboard stuff */
#endif

    /*
     * Check if we have an interactive window.
     * On the Amiga: If there is no window, we open one with a newcli command
     * (needed for :! to * work). mch_check_win() will also handle the -d
     * argument.
     */
    stdout_isatty = (mch_check_win(argc, argv) != FAIL);

    /*
     * allocate the first window and buffer. Can't do much without it
     */
    if ((curwin = win_alloc(NULL)) == NULL
	    || (curbuf = buflist_new(NULL, NULL, 1L, FALSE)) == NULL)
	mch_windexit(0);
    curwin->w_buffer = curbuf;
    curbuf->b_nwindows = 1;	/* there is one window */
    win_init(curwin);		/* init current window */
    init_yank();		/* init yank buffers */

    /*
     * Set the default values for the options.
     * First find out the home directory, needed to expand "~" in options.
     */
    init_homedir();		/* find real value of $HOME */
    set_init_1();

    /*
     * If the executable name starts with "r" we disable shell commands.
     * If the next character is "g" we run the GUI version.
     * If the next characters are "view" we start in readonly mode.
     * If the next characters are "ex" we start in ex mode.
     */
    initstr = gettail((char_u *)argv[0]);

    if (initstr[0] == 'r')
    {
	restricted = TRUE;
	++initstr;
    }

    if (initstr[0] == 'g')
    {
#ifdef USE_GUI
	gui.starting = TRUE;
	++initstr;
#else
	mch_errmsg((char *)e_nogvim);
	mch_windexit(2);
#endif
    }

    if (STRNCMP(initstr, "view", 4) == 0)
    {
	readonlymode = TRUE;
	curbuf->b_p_ro = TRUE;
	p_uc = 10000;			/* don't update very often */
    }

    if (STRNCMP(initstr, "ex", 2) == 0)
    {
	exmode_active = TRUE;
	change_compatible(TRUE);	/* set 'compatible' */
    }

    ++argv;
    --argc;

#ifndef macintosh
    /*
     * Allocate arg_files[], big enough to hold all potential file name
     * arguments.
     */
    arg_files = (char_u **)alloc((unsigned)(sizeof(char_u *) * (argc + 1)));
    if (arg_files == NULL)
	mch_windexit(2);
#else
    arg_files = NULL;
#endif
    arg_file_count = 0;

    /*
     * Process the command line arguments.
     */
    argv_idx = 1;	    /* active option letter is argv[0][argv_idx] */
    while (argc > 0)
    {
	/*
	 * "+" or "+{number}" or "+/{pat}" or "+{command}" argument.
	 */
	if (argv[0][0] == '+' && !had_minmin)
	{
	    if (n_commands >= MAX_ARG_CMDS)
		mainerr(ME_EXTRA_CMD, NULL);
	    argv_idx = -1;	    /* skip to next argument */
	    if (argv[0][1] == NUL)
		commands[n_commands++] = (char_u *)"$";
	    else
		commands[n_commands++] = (char_u *)&(argv[0][1]);
	}

	/*
	 * Option argument.
	 */
	else if (argv[0][0] == '-' && !had_minmin)
	{
	    want_argument = FALSE;
	    c = argv[0][argv_idx++];
	    switch (c)
	    {
	    case NUL:		/* "vim -"  read from stdin */
				/* "ex -" silent mode */
		if (exmode_active)
		    silent_mode = TRUE;
		else
		{
		    if (edit_type != EDIT_NONE)
			mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
		    edit_type = EDIT_STDIN;
		    read_cmd_fd = 2;	/* read from stderr instead of stdin */
		}
		argv_idx = -1;		/* skip to next argument */
		break;

	    case '-':		/* "--" don't take any more options */
				/* "--help" give help message */
				/* "--version" give version message */
		if (STRCMP(argv[0] + argv_idx, "help") == 0)
		    usage();
		if (STRCMP(argv[0] + argv_idx, "version") == 0)
		{
		    Columns = 80;	/* need to init Columns */
		    list_version();
		    mch_windexit(1);
		}
		if (argv[0][argv_idx])
		    mainerr(ME_UNKNOWN_OPTION, (char_u *)argv[0]);
		had_minmin = TRUE;
		argv_idx = -1;		/* skip to next argument */
		break;

	    case 'b':		/* "-b" binary mode */
		bin_mode = TRUE;    /* postpone to after reading .exrc files */
		break;

	    case 'C':		/* "-C"  Compatible */
		change_compatible(TRUE);
		break;

	    case 'e':		/* "-e" Ex mode */
		exmode_active = TRUE;
		break;

	    case 'f':		/* "-f"  GUI: run in foreground.  Amiga: open
				window directly, not with newcli */
#ifdef USE_GUI
		gui.dofork = FALSE;	/* don't fork() when starting GUI */
#endif
		break;

	    case 'g':		/* "-g" start GUI */
#ifdef USE_GUI
		gui.starting = TRUE;	/* start GUI a bit later */
#else
		mch_errmsg((char *)e_nogvim);
		mch_windexit(2);
#endif
		break;

	    case 'F':		/* "-F" start in Farsi mode: rl + fkmap set */
#ifdef FKMAP
		curwin->w_p_rl = p_fkmap = TRUE;
#else
		mch_errmsg((char *)e_nofarsi);
		mch_windexit(2);
#endif
		break;

	    case 'h':		/* "-h" give help message */
		usage();
		break;

	    case 'H':		/* "-H" start in Hebrew mode: rl + hkmap set */
#ifdef RIGHTLEFT
		curwin->w_p_rl = p_hkmap = TRUE;
#else
		mch_errmsg((char *)e_nohebrew);
		mch_windexit(2);
#endif
		break;

	    case 'l':		/* "-l" lisp mode, 'lisp' and 'showmatch' on */
#ifdef LISPINDENT
		curbuf->b_p_lisp = TRUE;
		p_sm = TRUE;
#endif
		break;

	    case 'm':		/* "-m"  no writing of files */
		p_write = FALSE;
		break;

	    case 'N':		/* "-N"  Nocompatible */
		change_compatible(FALSE);
		break;

	    case 'n':		/* "-n" no swap file */
		no_swap_file = TRUE;
		break;

	    case 'o':		/* "-o[N]" open N windows */
		/* default is 0: open window for each file */
		window_count = get_number_arg((char_u *)argv[0], &argv_idx, 0);
		break;

#ifdef QUICKFIX
	    case 'q':		/* "-q" QuickFix mode */
		if (edit_type != EDIT_NONE)
		    mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
		edit_type = EDIT_QF;
		if (argv[0][argv_idx])		/* "-q{errorfile}" */
		{
		    use_ef = (char_u *)argv[0] + argv_idx;
		    argv_idx = -1;
		}
		else if (argc > 1)		/* "-q {errorfile}" */
		    want_argument = TRUE;
		break;
#endif

	    case 'R':		/* "-R" readonly mode */
		readonlymode = TRUE;
		curbuf->b_p_ro = TRUE;
		p_uc = 10000;			/* don't update very often */
		break;

	    case 'r':		/* "-r" recovery mode */
	    case 'L':		/* "-L" recovery mode */
		recoverymode = 1;
		break;

	    case 's':
		if (exmode_active)	/* "-s" silent (batch) mode */
		    silent_mode = TRUE;
		else		/* "-s {scriptin}" read from script file */
		    want_argument = TRUE;
		break;

	    case 't':		/* "-t {tag}" or "-t{tag}" jump to tag */
		if (edit_type != EDIT_NONE)
		    mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
		edit_type = EDIT_TAG;
		if (argv[0][argv_idx])		/* "-t{tag}" */
		{
		    tagname = (char_u *)argv[0] + argv_idx;
		    argv_idx = -1;
		}
		else				/* "-t {tag}" */
		    want_argument = TRUE;
		break;

	    case 'V':		/* "-V{N}"	Verbose level */
		/* default is 10: a little bit verbose */
		p_verbose = get_number_arg((char_u *)argv[0], &argv_idx, 10);
		break;

	    case 'v':		/* "-v"  Vi-mode (as if called "vi") */
		exmode_active = FALSE;
#ifdef USE_GUI
		gui.starting = FALSE;	/* don't start GUI */
#endif
		break;

	    case 'w':		/* "-w{number}"	set window height */
				/* "-w {scriptout}"	write to script */
		if (isdigit(((char_u *)argv[0])[argv_idx]))
		{
		    argv_idx = -1;
		    break;			/* not implemented, ignored */
		}
		want_argument = TRUE;
		break;

#ifdef CRYPTV
	    case 'x':	    /* "-x"  encrypted reading/writing of files */
		ask_for_key = TRUE;
		break;
#endif

	    case 'Z':		/* "-Z"  restricted mode */
		restricted = TRUE;
		break;

	    case 'c':		/* "-c {command}" execute command */
	    case 'd':		/* "-d {device}" device (for Amiga) */
	    case 'i':		/* "-i {viminfo}" use for viminfo */
	    case 'T':		/* "-T {terminal}" terminal name */
	    case 'u':		/* "-u {vimrc}" vim inits file */
	    case 'U':		/* "-U {gvimrc}" gvim inits file */
	    case 'W':		/* "-W {scriptout}" overwrite */
		want_argument = TRUE;
		break;

	    default:
		mainerr(ME_UNKNOWN_OPTION, (char_u *)argv[0]);
	    }

	    /*
	     * Handle options with argument.
	     */
	    if (want_argument)
	    {
		/*
		 * Check for garbage immediately after the option letter.
		 */
		if (argv[0][argv_idx] != NUL)
		    mainerr(ME_GARBAGE, (char_u *)argv[0]);

		--argc;
		if (argc < 1)
		    mainerr(ME_ARG_MISSING, (char_u *)argv[0]);
		++argv;
		argv_idx = -1;

		switch (c)
		{
		case 'c':	/* "-c {command}" execute command */
		    if (n_commands >= MAX_ARG_CMDS)
			mainerr(ME_EXTRA_CMD, NULL);
		    commands[n_commands++] = (char_u *)argv[0];
		    break;

	    /*	case 'd':   This is handled in mch_check_win() */

#ifdef QUICKFIX
		case 'q':	/* "-q {errorfile}" QuickFix mode */
		    use_ef = (char_u *)argv[0];
		    break;
#endif

		case 'i':	/* "-i {viminfo}" use for viminfo */
		    use_viminfo = (char_u *)argv[0];
		    break;

		case 's':	/* "-s {scriptin}" read from script file */
		    if (scriptin[0] != NULL)
		    {
			mch_errmsg("Attempt to open script file again: \"");
			mch_errmsg(argv[-1]);
			mch_errmsg(" ");
			mch_errmsg(argv[0]);
			mch_errmsg("\"\n");
			mch_windexit(2);
		    }
		    if ((scriptin[0] = mch_fopen(argv[0], READBIN)) == NULL)
		    {
			mch_errmsg("Cannot open \"");
			mch_errmsg(argv[0]);
			mch_errmsg("\" for reading\n");
			mch_windexit(2);
		    }
		    if (save_typebuf() == FAIL)
			mch_windexit(2);	/* out of memory */
		    break;

		case 't':	/* "-t {tag}" */
		    tagname = (char_u *)argv[0];
		    break;

		case 'T':	/* "-T {terminal}" terminal name */
		    /*
		     * The -T term option is always available and when
		     * HAVE_TERMLIB is supported it overrides the environment
		     * variable TERM.
		     */
#ifdef USE_GUI
		    if (term_is_gui((char_u *)argv[0]))
			gui.starting = TRUE;	/* start GUI a bit later */
		    else
#endif
			term = (char_u *)argv[0];
		    break;

		case 'u':	/* "-u {vimrc}" vim inits file */
		    use_vimrc = (char_u *)argv[0];
		    break;

		case 'U':	/* "-U {gvimrc}" gvim inits file */
#ifdef USE_GUI
		    use_gvimrc = (char_u *)argv[0];
#endif
		    break;

		case 'w':	/* "-w {scriptout}" append to script file */
		case 'W':	/* "-W {scriptout}" overwrite script file */
		    if (scriptout != NULL)
		    {
			mch_errmsg("Attempt to open script file again: \"");
			mch_errmsg(argv[-1]);
			mch_errmsg(" ");
			mch_errmsg(argv[0]);
			mch_errmsg("\"\n");
			mch_windexit(2);
		    }
		    if ((scriptout = mch_fopen(argv[0],
				    c == 'w' ? APPENDBIN : WRITEBIN)) == NULL)
		    {
			mch_errmsg("cannot open \"");
			mch_errmsg(argv[0]);
			mch_errmsg("\" for output\n");
			mch_windexit(2);
		    }
		    break;
		}
	    }
	}

	/*
	 * File name argument.
	 */
	else
	{
	    argv_idx = -1;	    /* skip to next argument */
	    if (edit_type != EDIT_NONE && edit_type != EDIT_FILE)
		mainerr(ME_TOO_MANY_ARGS, (char_u *)argv[0]);
	    edit_type = EDIT_FILE;
	    arg_files[arg_file_count] = vim_strsave((char_u *)argv[0]);
	    if (arg_files[arg_file_count] != NULL)
		++arg_file_count;
	}

	/*
	 * If there are no more letters after the current "-", go to next
	 * argument.  argv_idx is set to -1 when the current argument is to be
	 * skipped.
	 */
	if (argv_idx <= 0 || argv[0][argv_idx] == NUL)
	{
	    --argc;
	    ++argv;
	    argv_idx = 1;
	}
    }

    /*
     * On some systems, when we compile with the GUI, we always use it.  On Mac
     * there is no terminal version, and on Windows we can't figure out how to
     * fork one off with :gui.
     */
#ifdef ALWAYS_USE_GUI
    gui.starting = TRUE;
#else
# ifdef USE_GUI_X11
    /*
     * Check if the GUI can be started.  Reset gui.starting if not.
     * Can't do this with GTK, need to fork first.
     * Don't know about other systems, stay on the safe side and don't check.
     */
    if (gui.starting && gui_init_check() == FAIL)
	gui.starting = FALSE;
# endif
#endif

    /*
     * May expand wildcards in file names.
     */
    if (arg_file_count > 0)
    {
#if (!defined(UNIX) && !defined(__EMX__)) || defined(ARCHIE)
	char_u	    **new_arg_files;
	int	    new_arg_file_count;

	if (expand_wildcards(arg_file_count, arg_files, &new_arg_file_count,
			&new_arg_files, EW_FILE|EW_NOTFOUND|EW_ADDSLASH) == OK
		&& new_arg_file_count > 0)
	{
	    FreeWild(arg_file_count, arg_files);
	    arg_file_count = new_arg_file_count;
	    arg_files = new_arg_files;
	}
#endif
	fname = arg_files[0];
    }
    if (arg_file_count > 1)
	printf("%d files to edit\n", arg_file_count);
#ifdef MSWIN
    else if (arg_file_count == 1)
    {
	/*
	 * If there is one filename, fully qualified, we have very probably
	 * been invoked from explorer, so change to the file's directory.
	 */
	if (fname[0] != NUL && fname[1] == ':' && fname[2] == '\\')
	{
	    /* If the cd fails, it doesn't matter.. */
	    (void)vim_chdirfile(fname);
	}
    }
#endif

    RedrawingDisabled = TRUE;

    /*
     * When listing swap file names, don't do cursor positioning et. al.
     */
    if (recoverymode && fname == NULL)
	want_full_screen = FALSE;

    /*
     * When certain to start the GUI, don't check capabilities of terminal.
     */
#if defined(ALWAYS_USE_GUI) || defined(USE_GUI_X11)
    if (gui.starting)
	want_full_screen = FALSE;
#endif

    /*
     * mch_windinit() sets up the terminal (window) for use.  This must be
     * done after resetting full_screen, otherwise it may move the cursor
     * (MSDOS).
     * Note that we may use mch_windexit() before mch_windinit()!
     */
    mch_windinit();

    /*
     * Print a warning if stdout is not a terminal.
     * When starting in Ex mode and commands come from a file, set Silent mode.
     */
    input_isatty = mch_input_isatty();
    if (exmode_active)
    {
	if (!input_isatty)
	    silent_mode = TRUE;
    }
    else if (want_full_screen && (!stdout_isatty || !input_isatty)
#ifdef USE_GUI
	    /* don't want the delay when started from the desktop */
	    && !gui.starting
#endif
	    )
    {
	if (!stdout_isatty)
	    mch_errmsg("Vim: Warning: Output is not to a terminal\n");
	if (!input_isatty)
	    mch_errmsg("Vim: Warning: Input is not from a terminal\n");
	out_flush();
	ui_delay(2000L, TRUE);
    }

    if (want_full_screen)
    {
	termcapinit(term);	/* set terminal name and get terminal
				   capabilities (will set full_screen) */
	screen_start();		/* don't know where cursor is now */
    }

    /*
     * Set the default values for the options that use Rows and Columns.
     */
    screenalloc(FALSE);		/* allocate screen buffers */
    ui_get_winsize();		/* inits Rows and Columns */
    screenalloc(FALSE);		/* may re-allocate screen buffers */
    set_init_2();

    firstwin->w_height = Rows - 1;
    cmdline_row = Rows - 1;

    /*
     * Don't call msg_start() if the GUI is expected to start, it switches the
     * cursor off.  Only need to avoid it when want_full_screen could not have
     * been reset above.
     * Also don't do it when reading from stdin (the program writing to the
     * pipe might use the cursor).
     */
    if (full_screen && edit_type != EDIT_STDIN
#if defined(USE_GUI) && !defined(ALWAYS_USE_GUI) && !defined(USE_GUI_X11)
	    && !gui.starting
#endif
       )
	msg_start();	    /* in case a mapping or error message is printed */
    msg_scroll = TRUE;
    no_wait_return = TRUE;

    init_mappings();		/* set up initial mappings */

    init_highlight(TRUE);	/* set the default highlight groups */
#ifdef CURSOR_SHAPE
    parse_guicursor();		/* set cursor shapes from 'guicursor' */
#endif

    /*
     * If -u option given, use only the initializations from that file and
     * nothing else.
     */
    if (use_vimrc != NULL)
    {
	if (STRCMP(use_vimrc, "NONE") == 0)
	{
#ifdef USE_GUI
	    if (use_gvimrc == NULL)	    /* don't load gvimrc either */
		use_gvimrc = use_vimrc;
#endif
	}
	else
	{
	    if (do_source(use_vimrc, FALSE, FALSE) != OK)
		EMSG2("Cannot read from \"%s\"", use_vimrc);
	}
    }
    else if (!silent_mode)
    {
	/*
	 * Get system wide defaults, if the file name is defined.
	 */
#ifdef SYS_VIMRC_FILE
	(void)do_source((char_u *)SYS_VIMRC_FILE, FALSE, FALSE);
#endif

	/*
	 * Try to read initialization commands from the following places:
	 * - environment variable VIMINIT
	 * - user vimrc file (s:.vimrc for Amiga, ~/.vimrc otherwise)
	 * - second user vimrc file ($VIM/.vimrc for Dos)
	 * - environment variable EXINIT
	 * - user exrc file (s:.exrc for Amiga, ~/.exrc otherwise)
	 * - second user exrc file ($VIM/.exrc for Dos)
	 * The first that exists is used, the rest is ignored.
	 */
	if (process_env((char_u *)"VIMINIT", TRUE) != OK)
	{
	    if (do_source((char_u *)USR_VIMRC_FILE, TRUE, TRUE) == FAIL
#ifdef USR_VIMRC_FILE2
		&& do_source((char_u *)USR_VIMRC_FILE2, TRUE, TRUE) == FAIL
#endif
#ifdef USR_VIMRC_FILE3
		&& do_source((char_u *)USR_VIMRC_FILE3, TRUE, TRUE) == FAIL
#endif
		&& process_env((char_u *)"EXINIT", FALSE) == FAIL
		&& do_source((char_u *)USR_EXRC_FILE, FALSE, FALSE) == FAIL)
	    {
#ifdef USR_EXRC_FILE2
		(void)do_source((char_u *)USR_EXRC_FILE2, FALSE, FALSE);
#endif
	    }
	}

	/*
	 * Read initialization commands from ".vimrc" or ".exrc" in current
	 * directory.  This is only done if the 'exrc' option is set.
	 * Because of security reasons we disallow shell and write commands
	 * now, except for unix if the file is owned by the user or 'secure'
	 * option has been reset in environmet of global ".exrc" or ".vimrc".
	 * Only do this if VIMRC_FILE is not the same as USR_VIMRC_FILE or
	 * SYS_VIMRC_FILE.
	 */
	if (p_exrc)
	{
#if defined(UNIX) || defined(VMS)
	    {
		struct stat s;

		/* if ".vimrc" file is not owned by user, set 'secure' mode */

		if (mch_stat(VIMRC_FILE, &s) || s.st_uid !=
# ifdef UNIX
				getuid()
# else	 /* VMS */
				((getgid() << 16) | getuid())
# endif
		    )
		    secure = p_secure;
	    }
#else
	    secure = p_secure;
#endif

	    i = FAIL;
	    if (fullpathcmp((char_u *)USR_VIMRC_FILE,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#ifdef USR_VIMRC_FILE2
		    && fullpathcmp((char_u *)USR_VIMRC_FILE2,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#endif
#ifdef USR_VIMRC_FILE3
		    && fullpathcmp((char_u *)USR_VIMRC_FILE3,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#endif
#ifdef SYS_VIMRC_FILE
		    && fullpathcmp((char_u *)SYS_VIMRC_FILE,
				      (char_u *)VIMRC_FILE, FALSE) != FPC_SAME
#endif
				)
		i = do_source((char_u *)VIMRC_FILE, TRUE, TRUE);

	    if (i == FAIL)
	    {
#if defined(UNIX) || defined(VMS)
		struct stat s;

		/* if ".exrc" is not owned by user set 'secure' mode */
		if (mch_stat(EXRC_FILE, &s) || s.st_uid !=
# ifdef UNIX
				getuid()
# else	 /* VMS */
				((getgid() << 16) | getuid())
# endif
		    )
		    secure = p_secure;
		else
		    secure = 0;
#endif
		if (	   fullpathcmp((char_u *)USR_EXRC_FILE,
				      (char_u *)EXRC_FILE, FALSE) != FPC_SAME
#ifdef USR_EXRC_FILE2
			&& fullpathcmp((char_u *)USR_EXRC_FILE2,
				      (char_u *)EXRC_FILE, FALSE) != FPC_SAME
#endif
				)
		    (void)do_source((char_u *)EXRC_FILE, FALSE, FALSE);
	    }
	}
	if (secure == 2)
	    need_wait_return = TRUE;
	secure = 0;
    }

    /*
     * Recovery mode without a file name: List swap files.
     * This uses the 'dir' option, therefore it must be after the
     * initializations.
     */
    if (recoverymode && fname == NULL)
    {
	recover_names(NULL, TRUE, 0);
	mch_windexit(0);
    }

    /*
     * Set a few option defaults after reading .vimrc files:
     * 'title' and 'icon', Unix: 'shellpipe' and 'shellredir'.
     */
    set_init_3();

    /*
     * "-n" argument: Disable swap file by setting 'updatecount' to 0.
     * Note that this overrides anything from a vimrc file.
     */
    if (no_swap_file)
	p_uc = 0;

#ifdef FKMAP
    if (curwin->w_p_rl && p_altkeymap)
    {
	p_hkmap = FALSE;	/* Reset the Hebrew keymap mode */
	p_fkmap = TRUE;		/* Set the Farsi keymap mode */
    }
#endif

    if (bin_mode)		    /* "-b" argument used */
    {
	set_options_bin(curbuf->b_p_bin, 1);
	curbuf->b_p_bin = 1;	    /* binary file I/O */
    }

#ifdef USE_GUI
    if (gui.starting)
	gui_start();		/* will set full_screen to TRUE */
#endif

#ifdef VIMINFO
    /*
     * Read in registers, history etc, but not marks, from the viminfo file
     */
    if (*p_viminfo != NUL)
	read_viminfo(NULL, TRUE, FALSE, FALSE);
#endif

#ifdef SPAWNO		/* special MSDOS swapping library */
    init_SPAWNO("", SWAP_ANY);
#endif

#ifdef QUICKFIX
    /*
     * "-q errorfile": Load the error file now.
     * If the error file can't be read, exit before doing anything else.
     */
    if (edit_type == EDIT_QF)
    {
	if (use_ef != NULL)
	    set_string_option_direct((char_u *)"ef", -1, use_ef, TRUE);
	if (qf_init(p_ef, p_efm) < 0)
	{
	    out_char('\n');
	    mch_windexit(3);
	}
    }
#endif

    /*
     * Don't set the file name if there was a command in .vimrc that already
     * loaded the file
     */
    if (curbuf->b_ffname == NULL)
    {
	(void)setfname(fname, NULL, TRUE);  /* includes maketitle() */
	++arg_idx;			    /* used first argument name */
    }

    /*
     * Start putting things on the screen.
     * Scroll screen down before drawing over it
     * Clear screen now, so file message will not be cleared.
     */
    starting = NO_BUFFERS;
    no_wait_return = FALSE;
    if (!exmode_active)
	msg_scroll = FALSE;

#ifdef USE_GUI
    /*
     * This seems to be required to make callbacks to be called now, instead
     * of after things have been put on the screen, which then may be deleted
     * when getting a resize callback.
     */
    if (gui.in_use)
	gui_wait_for_chars(50L);
#endif

    /*
     * If "-" argument given: Read file from stdin.
     * Do this before starting Raw mode, because it may change things that the
     * writing end of the pipe doesn't like, e.g., in case stdin and stderr
     * are the same terminal: "cat | vim -".
     * Using autocommands here may cause trouble...
     */
    if (edit_type == EDIT_STDIN && !recoverymode)
    {
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	/* When getting the ATTENTION prompt here, use a dialog */
	swap_exists_action = SEA_DIALOG;
#endif
	no_wait_return = TRUE;
	i = msg_didany;
	(void)open_buffer(TRUE);	/* create memfile and read file */
	no_wait_return = FALSE;
	msg_didany = i;
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	check_swap_exists_action();
	swap_exists_action = SEA_NONE;
#endif
#if !(defined(AMIGA) || defined(macintosh))
	/*
	 * Close stdin and dup it from stderr.  Required for GPM to work
	 * properly, and for running external commands.
	 * Is there any other system that cannot do this?
	 */
	close(0);
	dup(2);
#endif
    }

    /*
     * When done something that is not allowed or error message call
     * wait_return.  This must be done before starttermcap(), because it may
     * switch to another screen. It must be done after settmode(TMODE_RAW),
     * because we want to react on a single key stroke.
     * Call settmode and starttermcap here, so the T_KS and T_TI may be
     * defined by termcapinit and redifined in .exrc.
     */
    settmode(TMODE_RAW);
    if (need_wait_return || msg_didany)
	wait_return(TRUE);

    starttermcap();	    /* start termcap if not done by wait_return() */
#ifdef USE_MOUSE
    setmouse();				/* may start using the mouse */
#endif
    if (scroll_region)
	scroll_region_reset();		/* In case Rows changed */

    scroll_start();

    /*
     * Don't clear the screen when starting in Ex mode, unless using the GUI.
     */
    if (exmode_active
#ifdef USE_GUI
			&& !gui.in_use
#endif
					)
	must_redraw = CLEAR;
    else
	screenclear();			/* clear screen */

#ifdef CRYPTV
    if (ask_for_key)
	(void)get_crypt_key(TRUE);
#endif

    no_wait_return = TRUE;

    /*
     * Create the number of windows that was requested.
     */
    if (window_count == 0)
	window_count = arg_file_count;
    if (window_count > 1)
    {
	/* Don't change the windows if there was a command in .vimrc that
	 * already split some windows */
	if (firstwin->w_next == NULL)
	    window_count = make_windows(window_count);
	else
	    window_count = win_count();
    }
    else
	window_count = 1;

    if (recoverymode)			/* do recover */
    {
	msg_scroll = TRUE;		/* scroll message up */
	ml_recover();
	msg_scroll = FALSE;
	if (curbuf->b_ml.ml_mfp == NULL) /* failed */
	    getout(1);
	do_modelines();			/* do modelines */
    }
    else
    {
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	/* When getting the ATTENTION prompt here, use a dialog */
	swap_exists_action = SEA_DIALOG;
#endif

	/*
	 * Open a buffer for windows that don't have one yet.
	 * Commands in the .vimrc might have loaded a file or split the window.
	 * Watch out for autocommands that delete a window.
	 */
#ifdef AUTOCMD
	/*
	 * Don't execute Win/Buf Enter/Leave autocommands here
	 */
	++autocmd_no_enter;
	++autocmd_no_leave;
#endif
	for (curwin = firstwin; curwin != NULL; curwin = curwin->w_next)
	{
	    curbuf = curwin->w_buffer;
	    if (curbuf->b_ml.ml_mfp == NULL)
	    {
		(void)open_buffer(FALSE);   /* create memfile and read file */
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
		check_swap_exists_action();
#endif
#ifdef AUTOCMD
		curwin = firstwin;	    /* start again */
#endif
	    }
	    ui_breakcheck();
	    if (got_int)
	    {
		(void)vgetc();	/* only break the file loading, not the rest */
		break;
	    }
	}
#ifdef AUTOCMD
	--autocmd_no_enter;
	--autocmd_no_leave;
#endif
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	swap_exists_action = SEA_NONE;
#endif
	curwin = firstwin;
	curbuf = curwin->w_buffer;
    }

    /* Ex starts at last line of the file */
    if (exmode_active)
	curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;

#ifdef AUTOCMD
    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif
    setpcmark();

#ifdef QUICKFIX
    /*
     * When started with "-q errorfile" jump to first error now.
     */
    if (edit_type == EDIT_QF)
	qf_jump(0, 0, FALSE);
#endif

    /*
     * If opened more than one window, start editing files in the other
     * windows.  Make_windows() has already opened the windows.
     */
#ifdef AUTOCMD
    /*
     * Don't execute Win/Buf Enter/Leave autocommands here
     */
    ++autocmd_no_enter;
    ++autocmd_no_leave;
#endif
    for (i = 1; i < window_count; ++i)
    {
	if (curwin->w_next == NULL)	    /* just checking */
	    break;
	win_enter(curwin->w_next, FALSE);

	/* Only open the file if there is no file in this window yet (that can
	 * happen when .vimrc contains ":sall") */
	if (curbuf == firstwin->w_buffer || curbuf->b_ffname == NULL)
	{
	    curwin->w_arg_idx = arg_idx;
	    /* edit file from arg list, if there is one */
	    (void)do_ecmd(0,
			 arg_idx < arg_file_count ? arg_files[arg_idx] : NULL,
					  NULL, NULL, ECMD_LASTL, ECMD_HIDE);
	    if (arg_idx == arg_file_count - 1)
		arg_had_last = TRUE;
	    ++arg_idx;
	}
	ui_breakcheck();
	if (got_int)
	{
	    (void)vgetc();	/* only break the file loading, not the rest */
	    break;
	}
    }
#ifdef AUTOCMD
    --autocmd_no_enter;
#endif
    win_enter(firstwin, FALSE);		/* back to first window */
#ifdef AUTOCMD
    --autocmd_no_leave;
#endif
    if (window_count > 1)
	win_equal(curwin, FALSE);	/* adjust heights */

    /*
     * If there are more file names in the argument list than windows,
     * put the rest of the names in the buffer list.
     */
    while (arg_idx < arg_file_count)
	(void)buflist_add(arg_files[arg_idx++]);

    /*
     * Shorten any of the filenames, but only when absolute.
     */
    shorten_fnames(FALSE);

    /*
     * Need to jump to the tag before executing the '-c command'.
     * Makes "vim -c '/return' -t main" work.
     */
    if (tagname)
    {
	STRCPY(IObuff, "ta ");

	STRCAT(IObuff, tagname);
	do_cmdline(IObuff, NULL, NULL, DOCMD_NOWAIT|DOCMD_VERBOSE);
    }

    if (n_commands > 0)
    {
	/*
	 * We start commands on line 0, make "vim +/pat file" match a
	 * pattern on line 1.
	 */
	curwin->w_cursor.lnum = 0;
	sourcing_name = (char_u *)"command line";
	for (i = 0; i < n_commands; ++i)
	    do_cmdline(commands[i], NULL, NULL, DOCMD_NOWAIT|DOCMD_VERBOSE);
	sourcing_name = NULL;
	if (curwin->w_cursor.lnum == 0)
	    curwin->w_cursor.lnum = 1;

#ifdef QUICKFIX
	/* When started with "-q errorfile" jump to first again. */
	if (edit_type == EDIT_QF)
	    qf_jump(0, 0, FALSE);
#endif
    }

    RedrawingDisabled = FALSE;
    redraw_later(NOT_VALID);
    no_wait_return = FALSE;
    starting = 0;

    /* start in insert mode */
    if (p_im)
	need_start_insertmode = TRUE;

#ifdef AUTOCMD
    apply_autocmds(EVENT_VIMENTER, NULL, NULL, FALSE, curbuf);
#endif

#if defined(WIN32) && !defined(USE_GUI_WIN32)
    mch_set_winsize_now();	    /* Allow winsize changes from now on */
#endif

    /* If ":startinsert" command used, stuff a dummy command to be able to
     * call normal_cmd(), which will then start Insert mode. */
    if (restart_edit)
	stuffReadbuff((char_u *)"\034\016");

    /*
     * main command loop
     */
    clear_oparg(&oa);
    for (;;)
    {
	if (stuff_empty())
	{
	    if (need_check_timestamps)
		check_timestamps(FALSE);
	    if (need_wait_return)	/* if wait_return still needed ... */
		wait_return(FALSE);	/* ... call it now */
	    if (need_start_insertmode && goto_im())
	    {
		need_start_insertmode = FALSE;
		stuffReadbuff((char_u *)"i");	/* start insert mode next */
		/* skip the fileinfo message now, because it would be shown
		 * after insert mode finishes! */
		need_fileinfo = FALSE;
	    }
	}
	if (got_int && !global_busy)
	{
	    if (!quit_more)
		(void)vgetc();		/* flush all buffers */
	    got_int = FALSE;
	}
	if (!exmode_active)
	    msg_scroll = FALSE;
	quit_more = FALSE;

	/*
	 * If skip redraw is set (for ":" in wait_return()), don't redraw now.
	 * If there is nothing in the stuff_buffer or do_redraw is TRUE,
	 * update cursor and redraw.
	 */
	if (skip_redraw || exmode_active)
	    skip_redraw = FALSE;
	else if (do_redraw || stuff_empty())
	{
	    /*
	     * Before redrawing, make sure w_topline is correct, and w_leftcol
	     * if lines don't wrap, and w_skipcol if lines wrap.
	     */
	    update_topline();
	    validate_cursor();

	    if (VIsual_active)
		update_curbuf(INVERTED);/* update inverted part */
	    else if (must_redraw)
		update_screen(must_redraw);
	    else if (redraw_cmdline || clear_cmdline)
		showmode();
	    redraw_statuslines();
	    /* display message after redraw */
	    if (keep_msg != NULL)
		msg_attr(keep_msg, keep_msg_attr);
	    if (need_fileinfo)		/* show file info after redraw */
	    {
		fileinfo(FALSE, TRUE, FALSE);
		need_fileinfo = FALSE;
	    }

	    emsg_on_display = FALSE;	/* can delete error message now */
	    msg_didany = FALSE;		/* reset lines_left in msg_start() */
	    do_redraw = FALSE;
	    showruler(FALSE);

	    setcursor();
	    cursor_on();
	}
#ifdef USE_GUI
	if (need_mouse_correct)
	    gui_mouse_correct();
#endif

	/*
	 * Update w_curswant if w_set_curswant has been set.
	 * Postponed until here to avoid computing w_virtcol too often.
	 */
	update_curswant();

	/*
	 * If we're invoked as ex, do a round of ex commands.
	 * Otherwise, get and execute a normal mode command.
	 */
	if (exmode_active)
	    do_exmode();
	else
	    normal_cmd(&oa, TRUE);
    }
    /*NOTREACHED*/
#if !defined(MSDOS) || defined(DJGPP)
    return 0;	/* Borland C++ gives a "not reached" error message here */
#endif
}
#endif /* PROTO */

/*
 * Get a (optional) count for a Vim argument.
 */
    static int
get_number_arg(p, idx, def)
    char_u	*p;	    /* pointer to argument */
    int		*idx;	    /* index in argument, is incremented */
    int		def;	    /* default value */
{
    if (isdigit(p[*idx]))
    {
	def = atoi((char *)&(p[*idx]));
	while (isdigit(p[*idx]))
	    *idx = *idx + 1;
    }
    return def;
}

/*
 * Get an evironment variable, and execute it as Ex commands.
 * Returns FAIL if the environment variable was not executed, OK otherwise.
 */
    int
process_env(env, is_viminit)
    char_u	*env;
    int		is_viminit; /* when TRUE, called for VIMINIT */
{
    char_u	*initstr;
    char_u	*save_sourcing_name;
    linenr_t	save_sourcing_lnum;

    if ((initstr = mch_getenv(env)) != NULL && *initstr != NUL)
    {
	if (is_viminit)
	    vimrc_found();
	save_sourcing_name = sourcing_name;
	save_sourcing_lnum = sourcing_lnum;
	sourcing_name = env;
	sourcing_lnum = 0;
	do_cmdline(initstr, NULL, NULL, DOCMD_NOWAIT|DOCMD_VERBOSE);
	sourcing_name = save_sourcing_name;
	sourcing_lnum = save_sourcing_lnum;
	return OK;
    }
    return FAIL;
}

    void
getout(r)
    int		    r;
{
    exiting = TRUE;

    /* Position the cursor on the last screen line, below all the text */
#ifdef USE_GUI
    if (!gui.in_use)
#endif
	windgoto((int)Rows - 1, 0);

#ifdef AUTOCMD
    apply_autocmds(EVENT_VIMLEAVEPRE, NULL, NULL, FALSE, curbuf);
#endif

#ifdef VIMINFO
    if (*p_viminfo != NUL)
    {
	/* Write out the registers, history, marks etc, to the viminfo file */
	msg_didany = FALSE;
	write_viminfo(NULL, FALSE);
	if (msg_didany)		/* make the user read the error message */
	{
	    no_wait_return = FALSE;
	    wait_return(FALSE);
	}
    }
#endif /* VIMINFO */

#ifdef AUTOCMD
    apply_autocmds(EVENT_VIMLEAVE, NULL, NULL, FALSE, curbuf);

    /* Position the cursor again, the autocommands may have moved it */
# ifdef USE_GUI
    if (!gui.in_use)
# endif
	windgoto((int)Rows - 1, 0);
#endif

#ifdef HAVE_PERL_INTERP
    perl_end();
#endif

    mch_windexit(r);
}

/*
 * When FKMAP is defined, also compile the Farsi source code.
 */
#if defined(FKMAP) || defined(PROTO)
# include "farsi.c"
#endif
