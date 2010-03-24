/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *		  OS/2 port by Paul Slootman
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * os_unix.c -- code for all flavors of Unix (BSD, SYSV, SVR4, POSIX, ...)
 *	     Also for OS/2, using the excellent EMX package!!!
 *	     Also for BeOS and Atari MiNT.
 *
 * A lot of this file was originally written by Juergen Weigert and later
 * changed beyond recognition.
 */

/*
 * Some systems have a prototype for select() that has (int *) instead of
 * (fd_set *), which is wrong. This define removes that prototype. We define
 * our own prototype below.
 */
#define select select_declared_wrong

#include "vim.h"

#ifdef USE_FONTSET
# ifdef X_LOCALE
#  include <X11/Xlocale.h>
# else
#  include <locale.h>
# endif
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include "os_unixx.h"	    /* unix includes for os_unix.c only */

/*
 * Use this prototype for select, some include files have a wrong prototype
 */
#undef select
#ifdef __BEOS__
# define select	beos_select
#endif

#if defined(HAVE_SELECT)
extern int   select __ARGS((int, fd_set *, fd_set *, fd_set *, struct timeval *));
#endif

#ifdef GPM_MOUSE
# include <gpm.h>
/* <linux/keyboard.h> contains defines conflicting with "keymap.h",
 * I just copied relevant defines here. A cleaner solution would be to put gpm
 * code into separate file and include there linux/keyboard.h
 */
/* #include <linux/keyboard.h> */
# define KG_SHIFT	0
# define KG_CTRL	2
# define KG_ALT		3
# define KG_ALTGR	1
# define KG_SHIFTL	4
# define KG_SHIFTR	5
# define KG_CTRLL	6
# define KG_CTRLR	7
# define KG_CAPSSHIFT	8

static void gpm_close __ARGS((void));
static int gpm_open __ARGS((void));
static int mch_gpm_process __ARGS((void));
#endif

/*
 * end of autoconf section. To be extended...
 */

/* Are the following #ifdefs still required? And why? Is that for X11? */

#if defined(ESIX) || defined(M_UNIX) && !defined(SCO)
# ifdef SIGWINCH
#  undef SIGWINCH
# endif
# ifdef TIOCGWINSZ
#  undef TIOCGWINSZ
# endif
#endif

#if defined(SIGWINDOW) && !defined(SIGWINCH)	/* hpux 9.01 has it */
# define SIGWINCH SIGWINDOW
#endif

#if defined(HAVE_X11) && defined(WANT_X11)
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/Xatom.h>
# ifdef XTERM_CLIP
#  include <X11/Intrinsic.h>
#  include <X11/Shell.h>
#  include <X11/StringDefs.h>
static Widget	xterm_Shell = (Widget)0;
static void xterm_update __ARGS((void));
# endif

Window	    x11_window = 0;
Display	    *x11_display = NULL;
int	    got_x_error = FALSE;

# ifdef WANT_TITLE
static int  get_x11_windis __ARGS((void));
static void set_x11_title __ARGS((char_u *));
static void set_x11_icon __ARGS((char_u *));
# endif
#endif

#ifdef WANT_TITLE
static int get_x11_title __ARGS((int));
static int get_x11_icon __ARGS((int));

static char_u	*oldtitle = NULL;
static int	did_set_title = FALSE;
static char_u	*oldicon = NULL;
static int	did_set_icon = FALSE;
#endif

static void may_core_dump __ARGS((void));

static int  WaitForChar __ARGS((long));
#ifdef __BEOS__
int  RealWaitForChar __ARGS((int, long, int *)) __attribute((crest_skip));
#else
static int  RealWaitForChar __ARGS((int, long, int *)) __attribute((crest_skip));
#endif

#ifdef XTERM_CLIP
static int do_xterm_trace __ARGS((void));
#define XT_TRACE_DELAY	50	/* delay for xterm tracing */
#endif

static void handle_resize __ARGS((void));

#if defined(SIGWINCH)
static RETSIGTYPE sig_winch __ARGS(SIGPROTOARG);
#endif
#if defined(SIGINT)
static RETSIGTYPE catch_sigint __ARGS(SIGPROTOARG);
#endif
#if defined(SIGALRM) && defined(HAVE_X11) && defined(WANT_X11) \
	&& defined(WANT_TITLE) && !defined(USE_GUI_GTK)
# define SET_SIG_ALARM
static RETSIGTYPE sig_alarm __ARGS(SIGPROTOARG);
#endif
static RETSIGTYPE deathtrap __ARGS(SIGPROTOARG);

static void set_signals __ARGS((void));
static void catch_signals __ARGS((RETSIGTYPE (*func_deadly)(), RETSIGTYPE (*func_other)()));
#ifndef __EMX__
static int  have_wildcard __ARGS((int, char_u **));
static int  have_dollars __ARGS((int, char_u **));
#endif

#ifndef NO_EXPANDPATH
static int	pstrcmp __ARGS((const void *, const void *));
static int	unix_expandpath __ARGS((struct growarray *gap, char_u *path, int wildoff, int flags));
#endif

#ifndef __EMX__
static int save_patterns __ARGS((int num_pat, char_u **pat, int *num_file, char_u ***file));
#endif

#ifndef SIG_ERR
# define SIG_ERR	((RETSIGTYPE (*)())-1)
#endif

static int	do_resize = FALSE;
#ifndef __EMX__
static char_u	*extra_shell_arg = NULL;
static int	show_shell_mess = TRUE;
#endif
static int	deadly_signal = 0;	    /* The signal we caught */

static int curr_tmode = TMODE_COOK;	/* contains current terminal mode */

#ifdef SYS_SIGLIST_DECLARED
/*
 * I have seen
 *  extern char *_sys_siglist[NSIG];
 * on Irix, Linux, NetBSD and Solaris. It contains a nice list of strings
 * that describe the signals. That is nearly what we want here.  But
 * autoconf does only check for sys_siglist (without the underscore), I
 * do not want to change everything today.... jw.
 * This is why AC_DECL_SYS_SIGLIST is commented out in configure.in
 */
#endif

static struct signalinfo
{
    int	    sig;	/* Signal number, eg. SIGSEGV etc */
    char    *name;	/* Signal name (not char_u!). */
    char    deadly;	/* Catch as a deadly signal? */
} signal_info[] =
{
#ifdef SIGHUP
    {SIGHUP,	    "HUP",	TRUE},
#endif
#ifdef SIGQUIT
    {SIGQUIT,	    "QUIT",	TRUE},
#endif
#ifdef SIGILL
    {SIGILL,	    "ILL",	TRUE},
#endif
#ifdef SIGTRAP
    {SIGTRAP,	    "TRAP",	TRUE},
#endif
#ifdef SIGABRT
    {SIGABRT,	    "ABRT",	TRUE},
#endif
#ifdef SIGEMT
    {SIGEMT,	    "EMT",	TRUE},
#endif
#ifdef SIGFPE
    {SIGFPE,	    "FPE",	TRUE},
#endif
#ifdef SIGBUS
    {SIGBUS,	    "BUS",	TRUE},
#endif
#ifdef SIGSEGV
    {SIGSEGV,	    "SEGV",	TRUE},
#endif
#ifdef SIGSYS
    {SIGSYS,	    "SYS",	TRUE},
#endif
#ifdef SIGALRM
    {SIGALRM,	    "ALRM",	FALSE},	/* Perl's alarm() can trigger it */
#endif
#ifdef SIGTERM
    {SIGTERM,	    "TERM",	TRUE},
#endif
#ifdef SIGVTALRM
    {SIGVTALRM,	    "VTALRM",	TRUE},
#endif
#ifdef SIGPROF
    {SIGPROF,	    "PROF",	TRUE},
#endif
#ifdef SIGXCPU
    {SIGXCPU,	    "XCPU",	TRUE},
#endif
#ifdef SIGXFSZ
    {SIGXFSZ,	    "XFSZ",	TRUE},
#endif
#ifdef SIGUSR1
    {SIGUSR1,	    "USR1",	TRUE},
#endif
#ifdef SIGUSR2
    {SIGUSR2,	    "USR2",	TRUE},
#endif
#ifdef SIGINT
    {SIGINT,	    "INT",	FALSE},
#endif
#ifdef SIGWINCH
    {SIGWINCH,	    "WINCH",	FALSE},
#endif
#ifdef SIGTSTP
    {SIGTSTP,	    "TSTP",	FALSE},
#endif
#ifdef SIGPIPE
    {SIGPIPE,	    "PIPE",	FALSE},
#endif
    {-1,	    "Unknown!", FALSE}
};

    void
mch_write(s, len)
    char_u  *s;
    int	    len;
{
    write(1, (char *)s, len);
    if (p_wd)		/* Unix is too fast, slow down a bit more */
	RealWaitForChar(read_cmd_fd, p_wd, NULL);
}

/*
 * mch_inchar(): low level input funcion.
 * Get a characters from the keyboard.
 * Return the number of characters that are available.
 * If wtime == 0 do not wait for characters.
 * If wtime == n wait a short time for characters.
 * If wtime == -1 wait forever for characters.
 */
    int
mch_inchar(buf, maxlen, wtime)
    char_u  *buf;
    int	    maxlen;
    long    wtime;	    /* don't use "time", MIPS cannot handle it */
{
    int		len;
#ifdef AUTOCMD
    static int	once_already = 0;
#endif

    /* Check if window changed size while we were busy, perhaps the ":set
     * columns=99" command was used. */
    if (do_resize)
	handle_resize();

    if (wtime >= 0)
    {
	while (WaitForChar(wtime) == 0)		/* no character available */
	{
	    if (!do_resize)	/* return if not interrupted by resize */
	    {
#ifdef AUTOCMD
		once_already = 0;
#endif
		return 0;
	    }
	    handle_resize();
	}
    }
    else	/* wtime == -1 */
    {
#ifdef AUTOCMD
	if (once_already == 2)
	    updatescript(0);
	else if (once_already == 1)
	{
	    setcursor();
	    once_already = 2;
	    return 0;
	}
	else
#endif
	/*
	 * If there is no character available within 'updatetime' seconds
	 * flush all the swap files to disk
	 * Also done when interrupted by SIGWINCH.
	 */
        if (WaitForChar(p_ut) == 0)
        {
#ifdef AUTOCMD
            if (has_cursorhold() && get_real_state() == NORMAL_BUSY)
            {
                apply_autocmds(EVENT_CURSORHOLD, NULL, NULL, FALSE, curbuf);
                update_screen(VALID);
		once_already = 1;
                return 0;
            }
            else
#endif
                updatescript(0);
        }
    }

    for (;;)	/* repeat until we got a character */
    {
	if (do_resize)	    /* window changed size */
	    handle_resize();
	/*
	 * we want to be interrupted by the winch signal
	 */
	WaitForChar(-1L);
	if (do_resize)	    /* interrupted by SIGWINCH signal */
	    continue;

	/*
	 * For some terminals we only get one character at a time.
	 * We want the get all available characters, so we could keep on
	 * trying until none is available
	 * For some other terminals this is quite slow, that's why we don't do
	 * it.
	 */
	len = read_from_input_buf(buf, (long)maxlen);
	if (len > 0)
	{
#ifdef OS2
	    int i;

	    for (i = 0; i < len; i++)
		if (buf[i] == 0)
		    buf[i] = K_NUL;
#endif
#ifdef AUTOCMD
            once_already = 0;
#endif
	    return len;
	}
    }
}

    static void
handle_resize()
{
    do_resize = FALSE;
    set_winsize(0, 0, FALSE);
}

/*
 * return non-zero if a character is available
 */
    int
mch_char_avail()
{
    return WaitForChar(0L);
}

#if defined(__EMX__) || defined(PROTO)
/* ARGSUSED */
    long_u
mch_avail_mem(special)
    int special;
{
    return ulimit(3, 0L);   /* always 32MB? */
}
#endif

    void
mch_delay(msec, ignoreinput)
    long	msec;
    int		ignoreinput;
{
    int		old_tmode;

    if (ignoreinput)
    {
	/* Go to cooked mode without echo, to allow SIGINT interrupting us
	 * here */
	old_tmode = curr_tmode;
	settmode(TMODE_SLEEP);

	/*
	 * Everybody sleeps in a different way...
	 * Prefer nanosleep(), some versions of usleep() can only sleep up to
	 * one second.
	 */
#ifdef HAVE_NANOSLEEP
	{
	    struct timespec ts;

	    ts.tv_sec = msec / 1000;
	    ts.tv_nsec = (msec % 1000) * 1000000;
	    (void)nanosleep(&ts, NULL);
	}
#else
# ifdef HAVE_USLEEP
	while (msec >= 1000)
	{
	    usleep((unsigned int)(999 * 1000));
	    msec -= 999;
	}
	usleep((unsigned int)(msec * 1000));
# else
#  ifndef HAVE_SELECT
	poll(NULL, 0, (int)msec);
#  else
#   ifdef __EMX__
	_sleep2(msec);
#   else
	{
	    struct timeval tv;

	    tv.tv_sec = msec / 1000;
	    tv.tv_usec = (msec % 1000) * 1000;
	    /*
	     * NOTE: Solaris 2.6 has a bug that makes select() hang here.  Get
	     * a patch from Sun to fix this.  Reported by Gunnar Pedersen.
	     */
	    select(0, NULL, NULL, NULL, &tv);
	}
#   endif /* __EMX__ */
#  endif /* HAVE_SELECT */
# endif /* HAVE_NANOSLEEP */
#endif /* HAVE_USLEEP */

	settmode(old_tmode);
    }
    else
	WaitForChar(msec);
}

/*
 * We need correct potatotypes for a signal function, otherwise mean compilers
 * will barf when the second argument to signal() is ``wrong''.
 * Let me try it with a few tricky defines from my own osdef.h	(jw).
 */
#if defined(SIGWINCH)
/* ARGSUSED */
    static RETSIGTYPE
sig_winch SIGDEFARG(sigarg)
{
    /* this is not required on all systems, but it doesn't hurt anybody */
    signal(SIGWINCH, (RETSIGTYPE (*)())sig_winch);
    do_resize = TRUE;
    SIGRETURN;
}
#endif

#if defined(SIGINT)
/* ARGSUSED */
    static RETSIGTYPE
catch_sigint SIGDEFARG(sigarg)
{
    /* this is not required on all systems, but it doesn't hurt anybody */
    signal(SIGINT, (RETSIGTYPE (*)())catch_sigint);
    got_int = TRUE;
    SIGRETURN;
}
#endif

#ifdef SET_SIG_ALARM
/*
 * signal function for alarm().
 */
/* ARGSUSED */
    static RETSIGTYPE
sig_alarm SIGDEFARG(sigarg)
{
    /* doesn't do anything, just to break a system call */
    SIGRETURN;
}
#endif

/*
 * This function handles deadly signals.
 * It tries to preserve any swap file and exit properly.
 * (partly from Elvis).
 */
    static RETSIGTYPE
deathtrap SIGDEFARG(sigarg)
{
    static int	    entered = 0;
#ifdef SIGHASARG
    int	    i;

    /* try to find the name of this signal */
    for (i = 0; signal_info[i].sig != -1; i++)
	if (sigarg == signal_info[i].sig)
	    break;
    deadly_signal = sigarg;
#endif

    full_screen = FALSE;	/* don't write message to the GUI, it might be
				 * part of the problem... */
    /*
     * If something goes wrong after entering here, we may get here again.
     * When this happens, give a message and try to exit nicely (resetting the
     * terminal mode, etc.)
     * When this happens twice, just exit, don't even try to give a message,
     * stack may be corrupt or something weird.
     */
    if (entered >= 2)
    {
	reset_signals();	/* don't catch any signals anymore */
	may_core_dump();
	exit(7);
    }
    if (entered++)
    {
	OUT_STR("Vim: Double signal, exiting\n");
	out_flush();
	reset_signals();	/* don't catch any signals anymore */
	getout(1);
    }

    sprintf((char *)IObuff, "Vim: Caught %s %s\n",
#ifdef SIGHASARG
		    "deadly signal", signal_info[i].name
#else
		    "some", "deadly signal"
#endif
			    );

    preserve_exit();		    /* preserve files and exit */

    SIGRETURN;
}

#ifdef _REENTRANT
/*
 * On Solaris with multi-threading, suspending might not work immediately.
 * Catch the SIGCONT signal, which will be used as an indication whether the
 * suspending has been done or not.
 */
static int sigcont_received;
static RETSIGTYPE sigcont_handler __ARGS(SIGPROTOARG);

/*
 * signal handler for SIGCONT
 */
/* ARGSUSED */
    static RETSIGTYPE
sigcont_handler SIGDEFARG(sigarg)
{
    sigcont_received = TRUE;
    SIGRETURN;
}
#endif

/*
 * If the machine has job control, use it to suspend the program,
 * otherwise fake it by starting a new shell.
 */
    void
mch_suspend()
{
    /* BeOS does have SIGTSTP, but it doesn't work. */
#if defined(SIGTSTP) && !defined(__BEOS__)
    out_flush();	    /* needed to make cursor visible on some systems */
    settmode(TMODE_COOK);
    out_flush();	    /* needed to disable mouse on some systems */

# ifdef _REENTRANT
    sigcont_received = FALSE;
# endif
    kill(0, SIGTSTP);	    /* send ourselves a STOP signal */
# ifdef _REENTRANT
    /* When we didn't suspend immediately in the kill(), do it now.  Happens
     * on multi-threaded Solaris. */
    if (!sigcont_received)
	pause();
# endif

# ifdef WANT_TITLE
    /*
     * Set oldtitle to NULL, so the current title is obtained again.
     */
    vim_free(oldtitle);
    oldtitle = NULL;
# endif
    settmode(TMODE_RAW);
    need_check_timestamps = TRUE;
#else
    suspend_shell();
#endif
}

    void
mch_windinit()
{
    Columns = 80;
    Rows = 24;

    out_flush();
    set_signals();
}

    static void
set_signals()
{
#if defined(SIGWINCH)
    /*
     * WINDOW CHANGE signal is handled with sig_winch().
     */
    signal(SIGWINCH, (RETSIGTYPE (*)())sig_winch);
#endif

    /*
     * We want the STOP signal to work, to make mch_suspend() work.
     * For "rvim" the STOP signal is ignored.
     */
#ifdef SIGTSTP
    signal(SIGTSTP, restricted ? SIG_IGN : SIG_DFL);
#endif
#ifdef _REENTRANT
    signal(SIGCONT, sigcont_handler);
#endif

    /*
     * We want to ignore breaking of PIPEs.
     */
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    /*
     * We want to catch CTRL-C (only works while in Cooked mode).
     */
#ifdef SIGINT
    signal(SIGINT, (RETSIGTYPE (*)())catch_sigint);
#endif

    /*
     * Ignore alarm signals (Perl's alarm() generates it).
     */
#ifdef SIGALRM
    signal(SIGALRM, SIG_IGN);
#endif

    /*
     * Arrange for other signals to gracefully shutdown Vim.
     */
    catch_signals(deathtrap, SIG_ERR);

#if defined(USE_GUI) && defined(SIGHUP)
    /*
     * When the GUI is running, ignore the hangup signal.
     */
    if (gui.in_use)
	signal(SIGHUP, SIG_IGN);
#endif
}

    void
reset_signals()
{
    catch_signals(SIG_DFL, SIG_DFL);
#ifdef _REENTRANT
    /* SIGCONT isn't in the list, because its default action is ignore */
    signal(SIGCONT, SIG_DFL);
#endif
}

    static void
catch_signals(func_deadly, func_other)
    RETSIGTYPE (*func_deadly)();
    RETSIGTYPE (*func_other)();
{
    int	    i;

    for (i = 0; signal_info[i].sig != -1; i++)
	if (signal_info[i].deadly)
	    signal(signal_info[i].sig, func_deadly);
	else if (func_other != SIG_ERR)
	    signal(signal_info[i].sig, func_other);
}

/*
 * Check_win checks whether we have an interactive stdout.
 */
/* ARGSUSED */
    int
mch_check_win(argc, argv)
    int	    argc;
    char    **argv;
{
#ifdef OS2
    /*
     * Store argv[0], may be used for $VIM.  Only use it if it is an absolute
     * name, mostly it's just "vim" and found in the path, which is unusable.
     */
    if (mch_isFullName(argv[0]))
	exe_name = vim_strsave((char_u *)argv[0]);
#endif
    if (isatty(1))
	return OK;
    return FAIL;
}

/*
 * Return TRUE if the input comes from a terminal, FALSE otherwise.
 */
    int
mch_input_isatty()
{
    if (isatty(read_cmd_fd))
	return TRUE;
    return FALSE;
}

#if defined(HAVE_X11) && defined(WANT_X11) \
	&& (defined(WANT_TITLE) || defined(XTERM_CLIP))
/*
 * A few functions shared by X11 title and clipboard code.
 */
static int x_error_handler __ARGS((Display *dpy, XErrorEvent *error_event));
static int x_error_check __ARGS((Display *dpy, XErrorEvent *error_event));
static int test_x11_window __ARGS((Display *dpy));

/*
 * X Error handler, otherwise X just exits!  (very rude) -- webb
 */
    static int
x_error_handler(dpy, error_event)
    Display	*dpy;
    XErrorEvent	*error_event;
{
    XGetErrorText(dpy, error_event->error_code, (char *)IObuff, IOSIZE);
    STRCAT(IObuff, "\nVim: Got X error\n");

    /* We cannot print a message and continue, because no X calls are allowed
     * here (causes my system to hang).  Silently continuing might be an
     * alternative... */
    preserve_exit();		    /* preserve files and exit */

    return 0;		/* NOTREACHED */
}

/*
 * Another X Error handler, just used to check for errors.
 */
/* ARGSUSED */
    static int
x_error_check(dpy, error_event)
    Display *dpy;
    XErrorEvent	*error_event;
{
    got_x_error = TRUE;
    return 0;
}

/*
 * Test if "dpy" and x11_window are valid by getting the window title.
 * I don't actually want it yet, so there may be a simpler call to use, but
 * this will cause the error handler x_error_check() to be called if anything
 * is wrong, such as the window pointer being invalid (as can happen when the
 * user changes his DISPLAY, but not his WINDOWID) -- webb
 */
    static int
test_x11_window(dpy)
    Display	*dpy;
{
    int			(*old_handler)();
    XTextProperty	text_prop;

    old_handler = XSetErrorHandler(x_error_check);
    got_x_error = FALSE;
    if (XGetWMName(dpy, x11_window, &text_prop))
	XFree((void *)text_prop.value);
    XSync(dpy, False);
    (void)XSetErrorHandler(old_handler);

    return (got_x_error ? FAIL : OK);
}
#endif

#ifdef WANT_TITLE

#if defined(HAVE_X11) && defined(WANT_X11)

static int get_x11_thing __ARGS((int get_title, int test_only));

/*
 * try to get x11 window and display
 *
 * return FAIL for failure, OK otherwise
 *
 * FIXME:
 * This code is responsible for setting the window manager name of the
 * editor's window. We should provide a special version of this for usage with
 * the GTK+ widget set, which wouldn't use any direct x11 calls. (--mdcki)
 */
    static int
get_x11_windis()
{
    char	    *winid;
    static int	    result = -1;
#define XD_NONE	 0	/* x11_display not set here */
#define XD_HERE	 1	/* x11_display opened here */
#define XD_GUI	 2	/* x11_display used from gui.dpy */
#define XD_XTERM 3	/* x11_display used from xterm_dpy */
    static int	    x11_display_from = XD_NONE;

    /* X just exits if it finds an error otherwise! */
    XSetErrorHandler(x_error_handler);

#if defined(USE_GUI_X11) || defined(USE_GUI_GTK)
    if (gui.in_use)
    {
	/*
	 * If the X11 display was opened here before, for the window where Vim
	 * was started, close that one now to avoid a memory leak.
	 */
	if (x11_display_from == XD_HERE && x11_display != NULL)
	{
	    XCloseDisplay(x11_display);
	    x11_display = NULL;
	}
	if (gui_get_x11_windis(&x11_window, &x11_display) == OK)
	{
	    x11_display_from = XD_GUI;
	    return OK;
	}
	return FAIL;
    }
    else if (x11_display_from == XD_GUI)
    {
	/* GUI must have stopped somehow, clear x11_display */
	x11_window = 0;
	x11_display = NULL;
	x11_display_from = XD_NONE;
    }
#endif

    /*
     * If WINDOWID not set, should try another method to find out
     * what the current window number is. The only code I know for
     * this is very complicated.
     * We assume that zero is invalid for WINDOWID.
     */
    if (x11_window == 0 && (winid = getenv("WINDOWID")) != NULL)
	x11_window = (Window)atol(winid);

#ifdef XTERM_CLIP
    if (xterm_dpy != NULL && x11_window != 0)
    {
	/*
	 * If the X11 display was opened here before, for the window where Vim
	 * was started, close that one now to avoid a memory leak.
	 */
	if (x11_display_from == XD_HERE && x11_display != NULL)
	    XCloseDisplay(x11_display);
	x11_display = xterm_dpy;
	x11_display_from = XD_XTERM;
	if (test_x11_window(x11_display) == FAIL)
	{
	    /* probably bad $WINDOWID */
	    x11_window = 0;
	    x11_display = NULL;
	    x11_display_from = XD_NONE;
	    return FAIL;
	}
	return OK;
    }
#endif

    if (x11_window == 0 || x11_display == NULL)
	result = -1;

    if (result != -1)	    /* Have already been here and set this */
	return result;	    /* Don't do all these X calls again */

    if (x11_window != 0 && x11_display == NULL)
    {
#ifdef SET_SIG_ALARM
	RETSIGTYPE (*sig_save)();

	/*
	 * Opening the Display may hang if the DISPLAY setting is wrong, or
	 * the network connection is bad.  Set an alarm timer to get out.
	 */
	sig_save = (RETSIGTYPE (*)())signal(SIGALRM,
						 (RETSIGTYPE (*)())sig_alarm);
	alarm(2);
#endif
	x11_display = XOpenDisplay(NULL);

#if defined(USE_FONTSET) && (defined(HAVE_LOCALE_H) || defined(X_LOCALE))
	setlocale(LC_ALL, "");
#endif

#ifdef SET_SIG_ALARM
	alarm(0);
	signal(SIGALRM, (RETSIGTYPE (*)())sig_save);
#endif
	if (x11_display != NULL)
	{
	    if (test_x11_window(x11_display) == FAIL)
	    {
		/* Maybe window id is bad */
		x11_window = 0;
		XCloseDisplay(x11_display);
		x11_display = NULL;
	    }
	    else
		x11_display_from = XD_HERE;
	}
    }
    if (x11_window == 0 || x11_display == NULL)
	return (result = FAIL);
    return (result = OK);
}

/*
 * Determine original x11 Window Title
 */
    static int
get_x11_title(test_only)
    int		test_only;
{
    int		retval;

    retval = get_x11_thing(TRUE, test_only);

    /* could not get old title: oldtitle == NULL */

    return retval;
}

/*
 * Determine original x11 Window icon
 */
    static int
get_x11_icon(test_only)
    int		test_only;
{
    int		retval = FALSE;

    retval = get_x11_thing(FALSE, test_only);

    /* could not get old icon, use terminal name */
    if (oldicon == NULL && !test_only)
    {
	if (STRNCMP(T_NAME, "builtin_", 8) == 0)
	    oldicon = T_NAME + 8;
	else
	    oldicon = T_NAME;
    }

    return retval;
}

    static int
get_x11_thing(get_title, test_only)
    int		get_title;	/* get title string */
    int		test_only;
{
    XTextProperty	text_prop;
    int			retval = FALSE;
    Status		status;

    if (get_x11_windis() == OK)
    {
	/* Get window/icon name if any */
	if (get_title)
	    status = XGetWMName(x11_display, x11_window, &text_prop);
	else
	    status = XGetWMIconName(x11_display, x11_window, &text_prop);

	/*
	 * If terminal is xterm, then x11_window may be a child window of the
	 * outer xterm window that actually contains the window/icon name, so
	 * keep traversing up the tree until a window with a title/icon is
	 * found.
	 */
	if (vim_is_xterm(T_NAME))
	{
	    Window	    root;
	    Window	    parent;
	    Window	    win = x11_window;
	    Window	   *children;
	    unsigned int    num_children;

	    while (!status || text_prop.value == NULL)
	    {
		if (!XQueryTree(x11_display, win, &root, &parent, &children,
							       &num_children))
		    break;
		if (children)
		    XFree((void *)children);
		if (parent == root || parent == 0)
		    break;

		win = parent;
		if (get_title)
		    status = XGetWMName(x11_display, win, &text_prop);
		else
		    status = XGetWMIconName(x11_display, win, &text_prop);
	    }
	}
	if (status && text_prop.value != NULL)
	{
	    retval = TRUE;
	    if (!test_only)
	    {
#ifdef USE_FONTSET
		if (text_prop.encoding == XA_STRING)
		{
#endif
		    if (get_title)
			oldtitle = vim_strsave((char_u *)text_prop.value);
		    else
			oldicon = vim_strsave((char_u *)text_prop.value);
#ifdef USE_FONTSET
		}
		else
		{
		    char    **cl;
		    Status  transform_status;
		    int	    n = 0;

		    transform_status = XmbTextPropertyToTextList(x11_display,
								 &text_prop,
								 &cl, &n);
		    if (transform_status >= Success && n > 0 && cl[0])
		    {
			if (get_title)
			    oldtitle = vim_strsave((char_u *) cl[0]);
			else
			    oldicon = vim_strsave((char_u *) cl[0]);
			XFreeStringList(cl);
		    }
		    else
		    {
			if (get_title)
			    oldtitle = vim_strsave((char_u *)text_prop.value);
			else
			    oldicon = vim_strsave((char_u *)text_prop.value);
		    }
		}
#endif
	    }
	    XFree((void *)text_prop.value);
	}
    }
    return retval;
}

/*
 * Set x11 Window Title
 *
 * get_x11_windis() must be called before this and have returned OK
 */
    static void
set_x11_title(title)
    char_u	*title;
{
#if XtSpecificationRelease >= 4
    XTextProperty text_prop;

    text_prop.value = title;
    text_prop.nitems = STRLEN(title);
    text_prop.encoding = XA_STRING;
    text_prop.format = 8;
    XSetWMName(x11_display, x11_window, &text_prop);
#else
    XStoreName(x11_display, x11_window, (char *)title);
#endif
    XFlush(x11_display);
}

/*
 * Set x11 Window icon
 *
 * get_x11_windis() must be called before this and have returned OK
 */
    static void
set_x11_icon(icon)
    char_u	*icon;
{
#if XtSpecificationRelease >= 4
    XTextProperty text_prop;

    text_prop.value = icon;
    text_prop.nitems = STRLEN(icon);
    text_prop.encoding = XA_STRING;
    text_prop.format = 8;
    XSetWMIconName(x11_display, x11_window, &text_prop);
#else
    XSetIconName(x11_display, x11_window, (char *)icon);
#endif
    XFlush(x11_display);
}

#else  /* HAVE_X11 && WANT_X11 */

/*ARGSUSED*/
    static int
get_x11_title(test_only)
    int	    test_only;
{
    return FALSE;
}

    static int
get_x11_icon(test_only)
    int	    test_only;
{
    if (!test_only)
    {
	if (STRNCMP(T_NAME, "builtin_", 8) == 0)
	    oldicon = T_NAME + 8;
	else
	    oldicon = T_NAME;
    }
    return FALSE;
}

#endif /* HAVE_X11 && WANT_X11 */

    int
mch_can_restore_title()
{
    return get_x11_title(TRUE);
}

    int
mch_can_restore_icon()
{
    return get_x11_icon(TRUE);
}

/*
 * Set the window title and icon.
 */
    void
mch_settitle(title, icon)
    char_u *title;
    char_u *icon;
{
    int		type = 0;
    static int	recursive = 0;

    if (T_NAME == NULL)	    /* no terminal name (yet) */
	return;
    if (title == NULL && icon == NULL)	    /* nothing to do */
	return;

    /* When one of the X11 functions causes a deadly signal, we get here again
     * recursively.  Avoid hanging then (something is probably locked). */
    if (recursive)
	return;
    ++recursive;

    /*
     * if the window ID and the display is known, we may use X11 calls
     */
#if defined(HAVE_X11) && defined(WANT_X11)
    if (get_x11_windis() == OK)
	type = 1;
#else
# ifdef USE_GUI_BEOS
    /* we always have a 'window' */
    type = 1;
# endif
#endif

    /*
     * Note: if "t_TS" is set, title is set with escape sequence rather
     *	     than x11 calls, because the x11 calls don't always work
     */

    if ((type || *T_TS != NUL) && title != NULL)
    {
	if (oldtitle == NULL)		/* first call, save title */
	    (void)get_x11_title(FALSE);

	if (*T_TS != NUL)		/* it's OK if t_fs is empty */
	    term_settitle(title);
#if defined(HAVE_X11) && defined(WANT_X11)
	else
	    set_x11_title(title);		/* x11 */
#else
# ifdef USE_GUI_BEOS
	else
	    gui_mch_settitle(title, icon);
# endif
#endif
	did_set_title = TRUE;
    }

    if ((type || *T_CIS != NUL) && icon != NULL)
    {
	if (oldicon == NULL)		/* first call, save icon */
	    get_x11_icon(FALSE);

	if (*T_CIS != NUL)
	{
	    out_str(T_CIS);			/* set icon start */
	    out_str_nf(icon);
	    out_str(T_CIE);			/* set icon end */
	    out_flush();
	}
#if defined(HAVE_X11) && defined(WANT_X11)
	else
	    set_x11_icon(icon);			/* x11 */
#endif
	did_set_icon = TRUE;
    }
    --recursive;
}

/*
 * Restore the window/icon title.
 * "which" is one of:
 *  1  only restore title
 *  2  only restore icon
 *  3  restore title and icon
 */
    void
mch_restore_title(which)
    int which;
{
    /* only restore the title or icon when it has been set */
    mch_settitle(((which & 1) && did_set_title) ?
			(oldtitle ? oldtitle : p_titleold) : NULL,
			      ((which & 2) && did_set_icon) ? oldicon : NULL);
}

#endif /* WANT_TITLE */

    int
vim_is_xterm(name)
    char_u *name;
{
    if (name == NULL)
	return FALSE;
    return (STRNICMP(name, "xterm", 5) == 0
		|| STRNICMP(name, "kterm", 5)==0
		|| STRCMP(name, "builtin_xterm") == 0);
}

#if defined(USE_MOUSE) || defined(PROTO)
/*
 * Return non-zero when using an xterm mouse, according to 'ttymouse'.
 * Return 1 for "xterm".
 * Return 2 for "xterm2".
 */
    int
use_xterm_mouse()
{
    if (STRNICMP(p_ttym, "xterm", 5) == 0)
    {
	if (isdigit(p_ttym[5]))
	    return atoi((char *)&p_ttym[5]);
	return 1;
    }
    return 0;
}
#endif

    int
vim_is_iris(name)
    char_u  *name;
{
    if (name == NULL)
	return FALSE;
    return (STRNICMP(name, "iris-ansi", 9) == 0
	    || STRCMP(name, "builtin_iris-ansi") == 0);
}

/*
 * Return TRUE if "name" is a terminal for which 'ttyfast' should be set.
 * This should include all windowed terminal emulators.
 */
    int
vim_is_fastterm(name)
    char_u  *name;
{
    if (name == NULL)
	return FALSE;
    if (vim_is_xterm(name) || vim_is_iris(name))
	return TRUE;
    return (   STRNICMP(name, "hpterm", 6) == 0
	    || STRNICMP(name, "sun-cmd", 7) == 0
	    || STRNICMP(name, "screen", 6) == 0
	    || STRNICMP(name, "rxvt", 4) == 0
	    || STRNICMP(name, "dtterm", 6) == 0);
}

/*
 * Insert user name in s[len].
 * Return OK if a name found.
 */
    int
mch_get_user_name(s, len)
    char_u  *s;
    int	    len;
{
    return mch_get_uname(getuid(), s, len);
}

/*
 * Insert user name for "uid" in s[len].
 * Return OK if a name found.
 */
    int
mch_get_uname(uid, s, len)
    uid_t	uid;
    char_u	*s;
    int		len;
{
#if defined(HAVE_PWD_H) && defined(HAVE_GETPWUID)
    struct passwd   *pw;

    if ((pw = getpwuid(uid)) != NULL
	    && pw->pw_name != NULL && *(pw->pw_name) != NUL)
    {
	STRNCPY(s, pw->pw_name, len);
	return OK;
    }
#endif
    sprintf((char *)s, "%d", (int)uid);	    /* assumes s is long enough */
    return FAIL;			    /* a number is not a name */
}

/*
 * Insert host name is s[len].
 */

#ifdef HAVE_SYS_UTSNAME_H
    void
mch_get_host_name(s, len)
    char_u  *s;
    int	    len;
{
    struct utsname vutsname;

    uname(&vutsname);
    STRNCPY(s, vutsname.nodename, len);
}
#else /* HAVE_SYS_UTSNAME_H */

# ifdef HAVE_SYS_SYSTEMINFO_H
#  define gethostname(nam, len) sysinfo(SI_HOSTNAME, nam, len)
# endif

    void
mch_get_host_name(s, len)
    char_u  *s;
    int	    len;
{
    gethostname((char *)s, len);
}
#endif /* HAVE_SYS_UTSNAME_H */

/*
 * return process ID
 */
    long
mch_get_pid()
{
    return (long)getpid();
}

#if !defined(HAVE_STRERROR) && defined(USE_GETCWD)
static char *strerror __ARGS((int));

    static char *
strerror(err)
    int err;
{
    extern int	    sys_nerr;
    extern char	    *sys_errlist[];
    static char	    er[20];

    if (err > 0 && err < sys_nerr)
	return (sys_errlist[err]);
    sprintf(er, "Error %d", err);
    return er;
}
#endif

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
    int
mch_dirname(buf, len)
    char_u  *buf;
    int	    len;
{
#if defined(USE_GETCWD)
    if (getcwd((char *)buf, len) == NULL)
    {
	STRCPY(buf, strerror(errno));
	return FAIL;
    }
    return OK;
#else
    return (getwd((char *)buf) != NULL ? OK : FAIL);
#endif
}

#if defined(__EMX__) || defined(PROTO)
/*
 * Replace all slashes by backslashes.
 * When 'shellslash' set do it the other way around.
 */
    void
slash_adjust(p)
    char_u  *p;
{
    while (*p)
    {
	if (*p == psepcN)
	    *p = psepc;
	++p;
    }
}
#endif

/*
 * Get absolute file name into buffer 'buf' of length 'len' bytes.
 *
 * return FAIL for failure, OK for success
 */
    int
mch_FullName(fname, buf, len, force)
    char_u *fname, *buf;
    int len;
    int	force;		/* also expand when already absolute path name */
{
    int	    l;
#ifdef OS2
    int	    only_drive;	/* only a drive letter is specified in file name */
#endif
#ifdef HAVE_FCHDIR
    int	    fd = -1;
    static int	dont_fchdir = FALSE;	/* TRUE when fchdir() doesn't work */
#endif
    char_u  olddir[MAXPATHL];
    char_u  *p;
    int	    retval = OK;

    *buf = NUL;
    if (fname == NULL)	/* always fail */
	return FAIL;

    if (force || !mch_isFullName(fname)) /* if forced or not an absolute path */
    {
	/*
	 * If the file name has a path, change to that directory for a moment,
	 * and then do the getwd() (and get back to where we were).
	 * This will get the correct path name with "../" things.
	 */
#ifdef OS2
	only_drive = 0;
	if (((p = vim_strrchr(fname, '/')) != NULL) ||
	    ((p = vim_strrchr(fname, '\\')) != NULL) ||
	    (((p = vim_strchr(fname,  ':')) != NULL) && ++only_drive))
#else
	if ((p = vim_strrchr(fname, '/')) != NULL)
#endif
	{
#ifdef HAVE_FCHDIR
	    /*
	     * Use fchdir() if possible, it's said to be faster and more
	     * reliable.  But on SunOS 4 it might not work.  Check this by
	     * doing a fchdir() right now.
	     */
	    if (!dont_fchdir)
	    {
		fd = open(".", O_RDONLY | O_EXTRA, 0);
		if (fd >= 0 && fchdir(fd) < 0)
		{
		    close(fd);
		    fd = -1;
		    dont_fchdir = TRUE;	    /* don't try again */
		}
	    }
#endif
	    if (
#ifdef HAVE_FCHDIR
		fd < 0 &&
#endif
			    mch_dirname(olddir, MAXPATHL) == FAIL)
	    {
		p = NULL;	/* can't get current dir: don't chdir */
		retval = FAIL;
	    }
	    else
	    {
#ifdef OS2
		/*
		 * compensate for case where ':' from "D:" was the only
		 * path separator detected in the file name; the _next_
		 * character has to be removed, and then restored later.
		 */
		if (only_drive)
		    p++;
#endif
		/* The directory is copied into buf[], to be able to remove
		 * the file name without changing it (could be a string in
		 * read-only memory) */
		if (p - fname >= len)
		    retval = FAIL;
		else
		{
		    STRNCPY(buf, fname, p - fname);
		    buf[p - fname] = NUL;
		    if (mch_chdir((char *)buf))
			retval = FAIL;
		    else
			fname = p + 1;
		    *buf = NUL;
		}
#ifdef OS2
		if (only_drive)
		{
		    p--;
		    if (retval != FAIL)
			fname--;
		}
#endif
	    }
	}
	if (mch_dirname(buf, len) == FAIL)
	{
	    retval = FAIL;
	    *buf = NUL;
	}
	l = STRLEN(buf);
	if (l && buf[l - 1] != '/')
	    STRCAT(buf, "/");
	if (p != NULL)
	{
#ifdef HAVE_FCHDIR
	    if (fd >= 0)
	    {
		fchdir(fd);
		close(fd);
	    }
	    else
#endif
		mch_chdir((char *)olddir);
	}
    }
    STRCAT(buf, fname);
#ifdef OS2
    slash_adjust(buf);
#endif
    return retval;
}

/*
 * return TRUE is fname is an absolute path name
 */
    int
mch_isFullName(fname)
    char_u	*fname;
{
#ifdef __EMX__
    return _fnisabs(fname);
#else
    return (*fname == '/' || *fname == '~');
#endif
}

/*
 * Get file permissions for 'name'.
 * Returns -1 when it doesn't exist.
 */
    long
mch_getperm(name)
    char_u *name;
{
    struct stat statb;

    if (stat((char *)name, &statb))
	return -1;
    return statb.st_mode;
}

/*
 * set file permission for 'name' to 'perm'
 *
 * return FAIL for failure, OK otherwise
 */
    int
mch_setperm(name, perm)
    char_u  *name;
    long    perm;
{
    return (chmod((char *)name, (mode_t)perm) == 0 ? OK : FAIL);
}

/*
 * Set hidden flag for "name".
 */
/* ARGSUSED */
    void
mch_hide(name)
    char_u	*name;
{
    /* can't hide a file */
}

/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory
 * return FALSE for error
 */
    int
mch_isdir(name)
    char_u *name;
{
    struct stat statb;

    if (*name == NUL)	    /* Some stat()s don't flag "" as an error. */
	return FALSE;
    if (stat((char *)name, &statb))
	return FALSE;
#ifdef _POSIX_SOURCE
    return (S_ISDIR(statb.st_mode) ? TRUE : FALSE);
#else
    return ((statb.st_mode & S_IFMT) == S_IFDIR ? TRUE : FALSE);
#endif
}

    void
mch_windexit(r)
    int r;
{
    settmode(TMODE_COOK);
    exiting = TRUE;
#ifdef USE_GUI
    if (!gui.in_use)
#endif
    {
#ifdef WANT_TITLE
	mch_restore_title(3);	/* restore xterm title and icon name */
#endif
	stoptermcap();
	/*
	 * A newline is only required after a message in the alternate screen.
	 * This is set to TRUE by wait_return().
	 */
	if (newline_on_exit || (msg_didout && !swapping_screen()))
	    out_char('\n');
	else
	    msg_clr_eos();	/* clear the rest of the display */

	/* Cursor may have been switched off without calling starttermcap()
	 * when doing "vim -u vimrc" and vimrc contains ":q". */
	if (full_screen)
	    cursor_on();
    }
    out_flush();
    ml_close_all(TRUE);		/* remove all memfiles */
    may_core_dump();
#ifdef USE_GUI
# ifndef USE_GUI_BEOS		/* BeOS always has GUI */
    if (gui.in_use)
# endif
	gui_exit(r);
#endif
    exit(r);
}

    static void
may_core_dump()
{
    if (deadly_signal != 0)
    {
	signal(deadly_signal, SIG_DFL);
	kill(getpid(), deadly_signal);	/* Die using the signal we caught */
    }
}

    void
mch_settmode(tmode)
    int		tmode;
{
    static int first = TRUE;

    /* Why is NeXT excluded here (and not in os_unixx.h)? */
#if defined(ECHOE) && defined(ICANON) && (defined(HAVE_TERMIO_H) || defined(HAVE_TERMIOS_H)) && !defined(__NeXT__)
    /*
     * for "new" tty systems
     */
# ifdef HAVE_TERMIOS_H
    static struct termios told;
	   struct termios tnew;
# else
    static struct termio told;
	   struct termio tnew;
# endif

    if (first)
    {
	first = FALSE;
# if defined(HAVE_TERMIOS_H)
	tcgetattr(read_cmd_fd, &told);
# else
	ioctl(read_cmd_fd, TCGETA, &told);
# endif
    }

    tnew = told;
    if (tmode == TMODE_RAW)
    {
	/*
	 * ~ICRNL enables typing ^V^M
	 */
	tnew.c_iflag &= ~ICRNL;
	tnew.c_lflag &= ~(ICANON | ECHO | ISIG | ECHOE
# if defined(IEXTEN) && !defined(__MINT__)
		    | IEXTEN	    /* IEXTEN enables typing ^V on SOLARIS */
				    /* but it breaks function keys on MINT */
# endif
				);
# ifdef ONLCR	    /* don't map NL -> CR NL, we do it ourselves */
	tnew.c_oflag &= ~ONLCR;
# endif
	tnew.c_cc[VMIN] = 1;		/* return after 1 char */
	tnew.c_cc[VTIME] = 0;		/* don't wait */
    }
    else if (tmode == TMODE_SLEEP)
	tnew.c_lflag &= ~(ECHO);

# if defined(HAVE_TERMIOS_H)
    tcsetattr(read_cmd_fd, TCSANOW, &tnew);
# else
    ioctl(read_cmd_fd, TCSETA, &tnew);
# endif

#else

    /*
     * for "old" tty systems
     */
# ifndef TIOCSETN
#  define TIOCSETN TIOCSETP	/* for hpux 9.0 */
# endif
    static struct sgttyb ttybold;
	   struct sgttyb ttybnew;

    if (first)
    {
	first = FALSE;
	ioctl(read_cmd_fd, TIOCGETP, &ttybold);
    }

    ttybnew = ttybold;
    if (tmode == TMODE_RAW)
    {
	ttybnew.sg_flags &= ~(CRMOD | ECHO);
	ttybnew.sg_flags |= RAW;
    }
    else if (tmode == TMODE_SLEEP)
	ttybnew.sg_flags &= ~(ECHO);
    ioctl(read_cmd_fd, TIOCSETN, &ttybnew);
#endif
    curr_tmode = tmode;
}

/*
 * Try to get the code for "t_kb" from the stty setting
 *
 * Even if termcap claims a backspace key, the user's setting *should*
 * prevail.  stty knows more about reality than termcap does, and if
 * somebody's usual erase key is DEL (which, for most BSD users, it will
 * be), they're going to get really annoyed if their erase key starts
 * doing forward deletes for no reason. (Eric Fischer)
 */
    void
get_stty()
{
    char_u  buf[2];
    char_u  *p;

    /* Why is NeXT excluded here (and not in os_unixx.h)? */
#if defined(ECHOE) && defined(ICANON) && (defined(HAVE_TERMIO_H) || defined(HAVE_TERMIOS_H)) && !defined(__NeXT__)
    /* for "new" tty systems */
# ifdef HAVE_TERMIOS_H
    struct termios keys;
# else
    struct termio keys;
# endif

# if defined(HAVE_TERMIOS_H)
    if (tcgetattr(read_cmd_fd, &keys) != -1)
# else
    if (ioctl(read_cmd_fd, TCGETA, &keys) != -1)
# endif
    {
	buf[0] = keys.c_cc[VERASE];
	intr_char = keys.c_cc[VINTR];
#else
    /* for "old" tty systems */
    struct sgttyb keys;

    if (ioctl(read_cmd_fd, TIOCGETP, &keys) != -1)
    {
	buf[0] = keys.sg_erase;
	intr_char = keys.sg_kill;
#endif
	buf[1] = NUL;
	add_termcode((char_u *)"kb", buf, FALSE);

	/*
	 * If <BS> and <DEL> are now the same, redefine <DEL>.
	 */
	p = find_termcode((char_u *)"kD");
	if (p != NULL && p[0] == buf[0] && p[1] == buf[1])
	    do_fixdel();
    }
#if 0
    }	    /* to keep cindent happy */
#endif
}

#if defined(USE_MOUSE) || defined(PROTO)
/*
 * set mouse clicks on or off (only works for xterms)
 */
    void
mch_setmouse(on)
    int	    on;
{
    static int	ison = FALSE;
    int		xterm_mouse_vers;

    if (on == ison)	/* return quickly if nothing to do */
	return;

    xterm_mouse_vers = use_xterm_mouse();
    if (xterm_mouse_vers > 0)
    {
	if (on)	/* enable mouse events, use mouse tracking if available */
	    out_str_nf((char_u *)
		       (xterm_mouse_vers > 1 ? "\033[?1002h" : "\033[?1000h"));
	else	/* disable mouse events, could probably always send the same */
	    out_str_nf((char_u *)
		       (xterm_mouse_vers > 1 ? "\033[?1002l" : "\033[?1000l"));
	ison = on;
    }
# ifdef GPM_MOUSE
    else
    {
	if (on)
	{
	    if (gpm_open())
		ison = TRUE;
	}
	else
	{
	    gpm_close();
	    ison = FALSE;
	}
    }
# endif
}

/*
 * Set the mouse termcode, depending on the 'term' and 'ttymouse' options.
 */
    void
check_mouse_termcode()
{
# ifdef XTERM_MOUSE
    if (use_xterm_mouse())
    {
	set_mouse_termcode(KS_MOUSE, (char_u *)(term_is_8bit(T_NAME)
						       ? "\233M" : "\033[M"));
	if (*p_mouse != NUL)
	{
	    /* force mouse off and maybe on to send possibly new mouse
	     * activation sequence to the xterm, with(out) drag tracing. */
	    mch_setmouse(FALSE);
	    setmouse();
	}
    }
    else
	del_mouse_termcode(KS_MOUSE);
# endif
# ifdef GPM_MOUSE
    if (!use_xterm_mouse())
	set_mouse_termcode(KS_MOUSE, (char_u *)"\033MG");
# endif
# ifdef NETTERM_MOUSE
    /* can be added always, there is no conflict */
    set_mouse_termcode(KS_NETTERM_MOUSE, (char_u *)"\033}");
# endif
# ifdef DEC_MOUSE
    /* conflicts with xterm mouse: "\033[" and "\033[M" */
    if (!use_xterm_mouse())
	set_mouse_termcode(KS_DEC_MOUSE, (char_u *)"\033[");
    else
	del_mouse_termcode(KS_DEC_MOUSE);
# endif
}
#endif

/*
 * set screen mode, always fails.
 */
/* ARGSUSED */
    int
mch_screenmode(arg)
    char_u   *arg;
{
    EMSG("Screen mode setting not supported");
    return FAIL;
}

/*
 * Try to get the current window size:
 * 1. with an ioctl(), most accurate method
 * 2. from the environment variables LINES and COLUMNS
 * 3. from the termcap
 * 4. keep using the old values
 * Return OK when size could be determined, FAIL otherwise.
 */
    int
mch_get_winsize()
{
    int		old_Rows = Rows;
    int		old_Columns = Columns;
    char_u	*p;

    Columns = 0;
    Rows = 0;

/*
 * For OS/2 use _scrsize().
 */
# ifdef __EMX__
    {
	int s[2];
	_scrsize(s);
	Columns = s[0];
	Rows = s[1];
    }
# endif

/*
 * 1. try using an ioctl. It is the most accurate method.
 *
 * Try using TIOCGWINSZ first, some systems that have it also define TIOCGSIZE
 * but don't have a struct ttysize.
 */
# ifdef TIOCGWINSZ
    {
	struct winsize	ws;

	if (ioctl(1, TIOCGWINSZ, &ws) == 0)
	{
	    Columns = ws.ws_col;
	    Rows = ws.ws_row;
	}
    }
# else /* TIOCGWINSZ */
#  ifdef TIOCGSIZE
    {
	struct ttysize	ts;

	if (ioctl(1, TIOCGSIZE, &ts) == 0)
	{
	    Columns = ts.ts_cols;
	    Rows = ts.ts_lines;
	}
    }
#  endif /* TIOCGSIZE */
# endif /* TIOCGWINSZ */

/*
 * 2. get size from environment
 */
    if (Columns == 0 || Rows == 0)
    {
	if ((p = (char_u *)getenv("LINES")))
	    Rows = atoi((char *)p);
	if ((p = (char_u *)getenv("COLUMNS")))
	    Columns = atoi((char *)p);
    }

#ifdef HAVE_TGETENT
/*
 * 3. try reading the termcap
 */
    if (Columns == 0 || Rows == 0)
	getlinecol();	/* get "co" and "li" entries from termcap */
#endif

/*
 * 4. If everything fails, use the old values
 */
    if (Columns <= 0 || Rows <= 0)
    {
	Columns = old_Columns;
	Rows = old_Rows;
	return FAIL;
    }

    check_winsize();

/* if size changed: screenalloc will allocate new screen buffers */
    return OK;
}

/*
 * try to set the window size to Rows and Columns
 */
    void
mch_set_winsize()
{
    if (*T_CWS)
    {
	/*
	 * NOTE: if you get an error here that term_set_winsize() is
	 * undefined, check the output of configure.  It could probably not
	 * find a ncurses, termcap or termlib library.
	 */
	term_set_winsize((int)Rows, (int)Columns);
	out_flush();
	screen_start();			/* don't know where cursor is now */
    }
}

    int
mch_call_shell(cmd, options)
    char_u	*cmd;
    int		options;	/* SHELL_*, see vim.h */
{
#ifdef USE_SYSTEM	/* use system() to start the shell: simple but slow */

    int	    x;
#ifndef __EMX__
    char_u  *newcmd;   /* only needed for unix */
#else
    /*
     * Set the preferred shell in the EMXSHELL environment variable (but
     * only if it is different from what is already in the environment).
     * Emx then takes care of whether to use "/c" or "-c" in an
     * intelligent way. Simply pass the whole thing to emx's system() call.
     * Emx also starts an interactive shell if system() is passed an empty
     * string.
     */
    char_u *p, *old;

    if (((old = (char_u *)getenv("EMXSHELL")) == NULL) || STRCMP(old, p_sh))
    {
	/* should check HAVE_SETENV, but I know we don't have it. */
	p = alloc(10 + strlen(p_sh));
	if (p)
	{
	    sprintf((char *)p, "EMXSHELL=%s", p_sh);
	    putenv((char *)p);	/* don't free the pointer! */
	}
    }
#endif

    out_flush();

    if (options & SHELL_COOKED)
	settmode(TMODE_COOK);	    /* set to normal mode */

#ifdef __EMX__
    if (cmd == NULL)
	x = system("");	/* this starts an interactive shell in emx */
    else
	x = system((char *)cmd);
    if (x == -1) /* system() returns -1 when error occurs in starting shell */
    {
	MSG_PUTS("\nCannot execute shell ");
	msg_outtrans(p_sh);
	msg_putchar('\n');
    }
#else /* not __EMX__ */
    if (cmd == NULL)
	x = system((char *)p_sh);
    else
    {
	newcmd = lalloc(STRLEN(p_sh)
		+ (extra_shell_arg == NULL ? 0 : STRLEN(extra_shell_arg))
		+ STRLEN(p_shcf) + STRLEN(cmd) + 4, TRUE);
	if (newcmd == NULL)
	    x = 0;
	else
	{
	    sprintf((char *)newcmd, "%s %s %s %s", p_sh,
		    extra_shell_arg == NULL ? "" : (char *)extra_shell_arg,
		    (char *)p_shcf,
		    (char *)cmd);
	    x = system((char *)newcmd);
	    vim_free(newcmd);
	}
    }
    if (x == 127)
    {
	MSG_PUTS("\nCannot execute shell sh\n");
    }
#endif	/* __EMX__ */
    else if (x && !(options & SHELL_SILENT))
    {
	msg_putchar('\n');
	msg_outnum((long)x);
	MSG_PUTS(" returned\n");
    }

    settmode(TMODE_RAW);		/* set to raw mode */
#ifdef OS2
    /* external command may change the window size in OS/2, so check it */
    ui_get_winsize();
#endif
#ifdef WANT_TITLE
    resettitle();
#endif
    return x;

#else /* USE_SYSTEM */	    /* don't use system(), use fork()/exec() */

#define EXEC_FAILED 122	    /* Exit code when shell didn't execute.  Don't use
			       127, some shell use that already */

    char_u	*newcmd = NULL;
    pid_t	pid;
    pid_t	wait_pid = 0;
#ifdef HAVE_UNION_WAIT
    union wait	status;
#else
    int		status = -1;
#endif
    int		retval = -1;
    char	**argv = NULL;
    int		argc;
    int		i;
    char_u	*p;
    int		inquote;
#ifdef USE_GUI
    int		pty_master_fd = -1;	    /* for pty's */
    int		pty_slave_fd = -1;
    char	*tty_name;
    int		fd_toshell[2];	    /* for pipes */
    int		fd_fromshell[2];
    int		pipe_error = FALSE;
# ifdef HAVE_SETENV
    char	envbuf[50];
# else
    static char	envbuf_Rows[20];
    static char	envbuf_Columns[20];
# endif
#endif
    int		did_settmode = FALSE; /* TRUE when settmode(TMODE_RAW) called */

    out_flush();
    if (options & SHELL_COOKED)
	settmode(TMODE_COOK);		/* set to normal mode */

    /*
     * 1: find number of arguments
     * 2: separate them and built argv[]
     */
    newcmd = vim_strsave(p_sh);
    if (newcmd == NULL)		/* out of memory */
	goto error;
    for (i = 0; i < 2; ++i)
    {
	p = newcmd;
	inquote = FALSE;
	argc = 0;
	for (;;)
	{
	    if (i == 1)
		argv[argc] = (char *)p;
	    ++argc;
	    while (*p && (inquote || (*p != ' ' && *p != TAB)))
	    {
		if (*p == '"')
		    inquote = !inquote;
		++p;
	    }
	    if (*p == NUL)
		break;
	    if (i == 1)
		*p++ = NUL;
	    p = skipwhite(p);
	}
	if (i == 0)
	{
	    argv = (char **)alloc((unsigned)((argc + 4) * sizeof(char *)));
	    if (argv == NULL)	    /* out of memory */
		goto error;
	}
    }
    if (cmd != NULL)
    {
	if (extra_shell_arg != NULL)
	    argv[argc++] = (char *)extra_shell_arg;
	argv[argc++] = (char *)p_shcf;
	argv[argc++] = (char *)cmd;
    }
    argv[argc] = NULL;

#ifdef USE_GUI
    /*
     * For the GUI: Try using a pseudo-tty to get the stdin/stdout of the
     * executed command into the Vim window.  Or use a pipe.
     */
    if (gui.in_use && show_shell_mess)
    {
	/*
	 * Try to open a master pty.
	 * If this works, open the slave pty.
	 * If the slave can't be opened, close the master pty.
	 */
	if (p_guipty)
	{
	    pty_master_fd = OpenPTY(&tty_name);	    /* open pty */
	    if (pty_master_fd >= 0 && ((pty_slave_fd =
				    open(tty_name, O_RDWR | O_EXTRA, 0)) < 0))
	    {
		close(pty_master_fd);
		pty_master_fd = -1;
	    }
	}
	/*
	 * If not opening a pty or it didn't work, try using pipes.
	 */
	if (pty_master_fd < 0)
	{
	    pipe_error = (pipe(fd_toshell) < 0);
	    if (!pipe_error)			    /* pipe create OK */
	    {
		pipe_error = (pipe(fd_fromshell) < 0);
		if (pipe_error)			    /* pipe create failed */
		{
		    close(fd_toshell[0]);
		    close(fd_toshell[1]);
		}
	    }
	    if (pipe_error)
	    {
		MSG_PUTS("\nCannot create pipes\n");
		out_flush();
	    }
	}
    }

    if (!pipe_error)			/* pty or pipe opened or not used */
#endif

    {
#ifdef __BEOS__
	beos_cleanup_read_thread();
#endif
	if ((pid = fork()) == -1)	/* maybe we should use vfork() */
	{
	    MSG_PUTS("\nCannot fork\n");
#ifdef USE_GUI
	    if (gui.in_use && show_shell_mess)
	    {
		if (pty_master_fd >= 0)		/* close the pseudo tty */
		{
		    close(pty_master_fd);
		    close(pty_slave_fd);
		}
		else				/* close the pipes */
		{
		    close(fd_toshell[0]);
		    close(fd_toshell[1]);
		    close(fd_fromshell[0]);
		    close(fd_fromshell[1]);
		}
	    }
#endif
	}
	else if (pid == 0)	/* child */
	{
	    reset_signals();		/* handle signals normally */

	    if (!show_shell_mess || (options & SHELL_EXPAND))
	    {
		int fd;

		/*
		 * Don't want to show any message from the shell.  Can't just
		 * close stdout and stderr though, because some systems will
		 * break if you try to write to them after that, so we must
		 * use dup() to replace them with something else -- webb
		 * Connect stdin to /dev/null too, so ":n `cat`" doesn't hang,
		 * waiting for input.
		 */
		fd = open("/dev/null", O_RDWR | O_EXTRA, 0);
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		/*
		 * If any of these open()'s and dup()'s fail, we just continue
		 * anyway.  It's not fatal, and on most systems it will make
		 * no difference at all.  On a few it will cause the execvp()
		 * to exit with a non-zero status even when the completion
		 * could be done, which is nothing too serious.  If the open()
		 * or dup() failed we'd just do the same thing ourselves
		 * anyway -- webb
		 */
		if (fd >= 0)
		{
		    dup(fd); /* To replace stdin  (file descriptor 0) */
		    dup(fd); /* To replace stdout (file descriptor 1) */
		    dup(fd); /* To replace stderr (file descriptor 2) */

		    /* Don't need this now that we've duplicated it */
		    close(fd);
		}
	    }
#ifdef USE_GUI
	    else if (gui.in_use)
	    {

#ifdef HAVE_SETSID
		(void)setsid();
#endif
		/* push stream discipline modules */
		if (options & SHELL_COOKED)
		    SetupSlavePTY(pty_slave_fd);
#ifdef TIOCSCTTY
		/* try to become controlling tty (probably doesn't work,
		 * unless run by root) */
		ioctl(pty_slave_fd, TIOCSCTTY, (char *)NULL);
#endif
		/* Simulate to have a dumb terminal (for now) */
#ifdef HAVE_SETENV
		setenv("TERM", "dumb", 1);
		sprintf((char *)envbuf, "%ld", Rows);
		setenv("ROWS", (char *)envbuf, 1);
		sprintf((char *)envbuf, "%ld", Rows);
		setenv("LINES", (char *)envbuf, 1);
		sprintf((char *)envbuf, "%ld", Columns);
		setenv("COLUMNS", (char *)envbuf, 1);
#else
		/*
		 * Putenv does not copy the string, it has to remain valid.
		 * Use a static array to avoid loosing allocated memory.
		 */
		putenv("TERM=dumb");
		sprintf(envbuf_Rows, "ROWS=%ld", Rows);
		putenv(envbuf_Rows);
		sprintf(envbuf_Rows, "LINES=%ld", Rows);
		putenv(envbuf_Rows);
		sprintf(envbuf_Columns, "COLUMNS=%ld", Columns);
		putenv(envbuf_Columns);
#endif

		if (pty_master_fd >= 0)
		{
		    close(pty_master_fd);   /* close master side of pty */

		    /* set up stdin/stdout/stderr for the child */
		    close(0);
		    dup(pty_slave_fd);
		    close(1);
		    dup(pty_slave_fd);
		    close(2);
		    dup(pty_slave_fd);

		    close(pty_slave_fd);    /* has been dupped, close it now */
		}
		else
		{
		    /* set up stdin for the child */
		    close(fd_toshell[1]);
		    close(0);
		    dup(fd_toshell[0]);
		    close(fd_toshell[0]);

		    /* set up stdout for the child */
		    close(fd_fromshell[0]);
		    close(1);
		    dup(fd_fromshell[1]);
		    close(fd_fromshell[1]);

		    /* set up stderr for the child */
		    close(2);
		    dup(1);
		}
	    }
#endif
	    /*
	     * There is no type cast for the argv, because the type may be
	     * different on different machines. This may cause a warning
	     * message with strict compilers, don't worry about it.
	     */
	    execvp(argv[0], argv);
	    exit(EXEC_FAILED);	    /* exec failed, return failure code */
	}
	else			/* parent */
	{
	    /*
	     * While child is running, ignore terminating signals.
	     */
	    catch_signals(SIG_IGN, SIG_ERR);

#ifdef USE_GUI

	    /*
	     * For the GUI we redirect stdin, stdout and stderr to our window.
	     */
	    if (gui.in_use && show_shell_mess)
	    {
#define BUFLEN 100		/* length for buffer, pseudo tty limit is 128 */
		char_u	    buffer[BUFLEN + 1];
		char_u	    ta_buf[BUFLEN + 1];	/* TypeAHead */
		int	    ta_len = 0;		/* valid chars in ta_buf[] */
		int	    len;
		int	    p_more_save;
		int	    old_State;
		int	    c;
		int	    toshell_fd;
		int	    fromshell_fd;

		if (pty_master_fd >= 0)
		{
		    close(pty_slave_fd);	/* close slave side of pty */
		    fromshell_fd = pty_master_fd;
		    toshell_fd = dup(pty_master_fd);
		}
		else
		{
		    close(fd_toshell[0]);
		    close(fd_fromshell[1]);
		    toshell_fd = fd_toshell[1];
		    fromshell_fd = fd_fromshell[0];
		}

		/*
		 * Write to the child if there are typed characters.
		 * Read from the child if there are characters available.
		 *   Repeat the reading a few times if more characters are
		 *   available. Need to check for typed keys now and then, but
		 *   not too often (delays when no chars are available).
		 * This loop is quit if no characters can be read from the pty
		 * (WaitForChar detected special condition), or there are no
		 * characters available and the child has exited.
		 * Only check if the child has exited when there is no more
		 * output. The child may exit before all the output has
		 * been printed.
		 *
		 * Currently this busy loops!
		 * This can probably dead-lock when the write blocks!
		 */
		p_more_save = p_more;
		p_more = FALSE;
		old_State = State;
		State = EXTERNCMD;	/* don't redraw at window resize */

		for (;;)
		{
		    /*
		     * Check if keys have been typed, write them to the child
		     * if there are any.  Don't do this if we are expanding
		     * wild cards (would eat typeahead).  Don't get extra
		     * characters when we already have one.
		     */
		    len = 0;
		    if (!(options & SHELL_EXPAND)
			    && (ta_len > 0
				|| (len = ui_inchar(ta_buf, BUFLEN, 10L)) > 0))
		    {
			/*
			 * For pipes:
			 * Check for CTRL-C: send interrupt signal to child.
			 * Check for CTRL-D: EOF, close pipe to child.
			 */
			if (len == 1 && (pty_master_fd < 0 || cmd != NULL))
			{
#ifdef SIGINT
			    /*
			     * Send SIGINT to the child's group or all
			     * processes in our group.
			     */
			    if (ta_buf[ta_len] == Ctrl('C')
					       || ta_buf[ta_len] == intr_char)
# ifdef HAVE_SETSID
				kill(-pid, SIGINT);
# else
				kill(0, SIGINT);
# endif
#endif
			    if (pty_master_fd < 0 && toshell_fd >= 0
					       && ta_buf[ta_len] == Ctrl('D'))
			    {
				close(toshell_fd);
				toshell_fd = -1;
			    }
			}

			/* replace K_BS by <BS> and K_DEL by <DEL> */
			for (i = ta_len; i < ta_len + len; ++i)
			{
			    if (ta_buf[i] == CSI && len - i > 2)
			    {
				c = TERMCAP2KEY(ta_buf[i + 1], ta_buf[i + 2]);
				if (c == K_DEL || c == K_KDEL || c == K_BS)
				{
				    mch_memmove(ta_buf + i + 1, ta_buf + i + 3,
						       (size_t)(len - i - 2));
				    if (c == K_DEL || c == K_KDEL)
					ta_buf[i] = DEL;
				    else
					ta_buf[i] = Ctrl('H');
				    len -= 2;
				}
			    }
			    else if (ta_buf[i] == '\r')
				ta_buf[i] = '\n';
			}

			/*
			 * For pipes: echo the typed characters.
			 * For a pty this does not seem to work.
			 */
			if (pty_master_fd < 0)
			{
			    for (i = ta_len; i < ta_len + len; ++i)
				if (ta_buf[i] == '\n' || ta_buf[i] == '\b')
				    msg_putchar(ta_buf[i]);
				else
				    msg_outtrans_len(ta_buf + i, 1);
			    windgoto(msg_row, msg_col);
			    out_flush();
			}

			ta_len += len;

			/*
			 * Write the characters to the child, unless EOF has
			 * been typed for pipes.  Write one character at a
			 * time, to avoid loosing too much typeahead.
			 */
			if (toshell_fd >= 0)
			{
			    len = write(toshell_fd, (char *)ta_buf, (size_t)1);
			    if (len > 0)
			    {
				ta_len -= len;
				mch_memmove(ta_buf, ta_buf + len, ta_len);
			    }
			}
		    }

		    /*
		     * Check if the child has any characters to be printed.
		     * Read them and write them to our window.	Repeat this as
		     * long as there is something to do, avoid the 10ms wait
		     * for mch_inchar(), or sending typeahead characters to
		     * the external process.
		     * TODO: This should handle escape sequences, compatible
		     * to some terminal (vt52?).
		     */
		    while (RealWaitForChar(fromshell_fd, 10L, NULL))
		    {
			len = read(fromshell_fd, (char *)buffer,
							      (size_t)BUFLEN);
			if (len <= 0)		    /* end of file or error */
			    goto finished;
			buffer[len] = NUL;
			msg_puts(buffer);
			windgoto(msg_row, msg_col);
			cursor_on();
			out_flush();
			if (got_int)
			    break;
		    }

		    /*
		     * Check if the child still exists, before checking for
		     * typed characters (otherwise we would loose typeahead).
		     */
#ifdef __NeXT__
		    wait_pid = wait4(pid, &status, WNOHANG, (struct rusage *) 0);
#else
		    wait_pid = waitpid(pid, &status, WNOHANG);
#endif
		    if ((wait_pid == (pid_t)-1 && errno == ECHILD)
			    || (wait_pid == pid && WIFEXITED(status)))
		    {
			wait_pid = pid;
			break;
		    }
		    wait_pid = 0;
		}
finished:
		p_more = p_more_save;

		/*
		 * Give all typeahead that wasn't used back to ui_inchar().
		 */
		if (ta_len)
		    ui_inchar_undo(ta_buf, ta_len);

		State = old_State;
		if (toshell_fd >= 0)
		    close(toshell_fd);
		close(fromshell_fd);
	    }
#endif /* USE_GUI */

	    /*
	     * Wait until our child has exited.
	     * Ignore wait() returning pids of other children and returning
	     * because of some signal like SIGWINCH.
	     * Don't wait if wait_pid was already set above, indicating the
	     * child already exited.
	     */
	    while (wait_pid != pid)
	    {
		wait_pid = wait(&status);
		if (wait_pid <= 0
#ifdef ECHILD
			&& errno == ECHILD
#endif
		   )
		    break;
	    }

	    /*
	     * Set to raw mode right now, otherwise a CTRL-C after
	     * catch_signals() will kill Vim.
	     */
	    settmode(TMODE_RAW);
	    did_settmode = TRUE;
	    set_signals();

	    /*
	     * Check the window size, in case it changed while executing the
	     * external command.
	     */
	    ui_get_winsize();

	    if (WIFEXITED(status))
	    {
		retval = WEXITSTATUS(status);
		if (retval)
		{
		    if (retval == EXEC_FAILED)
		    {
			MSG_PUTS("\nCannot execute shell ");
			msg_outtrans(p_sh);
			msg_putchar('\n');
		    }
		    else if (!(options & SHELL_SILENT))
		    {
			msg_putchar('\n');
			msg_outnum((long)retval);
			MSG_PUTS(" returned\n");
		    }
		}
	    }
	    else
		MSG_PUTS("\nCommand terminated\n");
	}
    }
    vim_free(argv);

error:
    if (!did_settmode)
	settmode(TMODE_RAW);		    /* set to raw mode */
#ifdef WANT_TITLE
    resettitle();
#endif
    vim_free(newcmd);

    return retval;

#endif /* USE_SYSTEM */
}

/*
 * Check for CTRL-C typed by reading all available characters.
 * In cooked mode we should get SIGINT, no need to check.
 */
    void
mch_breakcheck()
{
    if (curr_tmode == TMODE_RAW && RealWaitForChar(read_cmd_fd, 0L, NULL))
	fill_input_buf(FALSE);
}

/*
 * Wait "msec" msec until a character is available from the keyboard or from
 * inbuf[]. msec == -1 will block forever.
 * When a GUI is being used, this will never get called -- webb
 */
    static int
WaitForChar(msec)
    long	msec;
{
#ifdef GPM_MOUSE
    int		gpm_process_wanted;
#endif
#ifdef XTERM_CLIP
    int		rest;
#endif
    int		avail;

    if (!vim_is_input_buf_empty())	    /* something in inbuf[] */
	return 1;

#if defined(DEC_MOUSE)
    /* May need to query the mouse position. */
    if (WantQueryMouse)
    {
	WantQueryMouse = 0;
	mch_write((char_u *)"\033[1'|", 5);
    }
#endif

    /*
     * For GPM_MOUSE and XTERM_CLIP we loop here to process mouse events.
     * This is a bit complicated, because they might both be defined.
     */
#if defined(GPM_MOUSE) || defined(XTERM_CLIP)
# ifdef XTERM_CLIP
    rest = 0;
    if (do_xterm_trace())
	rest = msec;
# endif
    do
    {
# ifdef XTERM_CLIP
	if (rest != 0)
	{
	    msec = XT_TRACE_DELAY;
	    if (rest >= 0 && rest < XT_TRACE_DELAY)
		msec = rest;
	    if (rest >= 0)
		rest -= msec;
	}
# endif
# ifdef GPM_MOUSE
	gpm_process_wanted = 0;
	avail = RealWaitForChar(read_cmd_fd, msec, &gpm_process_wanted);
# else
	avail = RealWaitForChar(read_cmd_fd, msec, NULL);
# endif
	if (!avail)
	{
	    if (!vim_is_input_buf_empty())
		return 1;
# ifdef XTERM_CLIP
	    if (rest == 0 || !do_xterm_trace())
# endif
		break;
	}
    }
    while (FALSE
# ifdef GPM_MOUSE
	   || (gpm_process_wanted && mch_gpm_process() == 0)
# endif
# ifdef XTERM_CLIP
	   || (!avail && rest != 0)
# endif
	  );

#else
    avail =  RealWaitForChar(read_cmd_fd, msec, NULL);
#endif
    return avail;
}

/*
 * Wait "msec" msec until a character is available from file descriptor "fd".
 * Time == -1 will block forever.
 * When a GUI is being used, this will not be used for input -- webb
 * Returns also, when a request from Sniff is waiting -- toni.
 * Or when a Linux GPM mouse event is waiting.
 */
/* ARGSUSED */
#ifdef __BEOS__
    int
#else
    static  int
#endif
RealWaitForChar(fd, msec, check_for_gpm)
    int		fd;
    long	msec;
    int		*check_for_gpm;
{
    int		ret;

    while (1)
    {
#ifndef HAVE_SELECT
	struct pollfd   fds[4];
	int		nfd;
# ifdef XTERM_CLIP
	int		xterm_idx = -1;
# endif
# ifdef GPM_MOUSE
	int		gpm_idx = -1;
# endif

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	nfd = 1;

# ifdef USE_SNIFF
#  define SNIFF_IDX 1
	if (want_sniff_request)
	{
	    fds[SNIFF_IDX].fd = fd_from_sniff;
	    fds[SNIFF_IDX].events = POLLIN;
	    nfd++;
	}
# endif
# ifdef XTERM_CLIP
	if (xterm_Shell != (Widget)0)
	{
	    xterm_idx = nfd;
	    fds[nfd].fd = ConnectionNumber(xterm_dpy);
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
# endif
# ifdef GPM_MOUSE
	if (check_for_gpm != NULL && gpm_flag && gpm_fd >= 0)
	{
	    gpm_idx = nfd;
	    fds[nfd].fd = gpm_fd;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
# endif

	ret = poll(&fds, nfd, (int)msec);

# ifdef USE_SNIFF
	if (ret < 0)
	    sniff_disconnect(1);
	else if (want_sniff_request)
	{
	    if (fds[SNIFF_IDX].revents & POLLHUP)
		sniff_disconnect(1);
	    if (fds[SNIFF_IDX].revents & POLLIN)
		sniff_request_waiting = 1;
	}
# endif
# ifdef XTERM_CLIP
	if (xterm_Shell != (Widget)0 && (fds[xterm_idx].revents & POLLIN))
	{
	    xterm_update();      /* Maybe we should hand out clipboard */
	    if (vim_is_input_buf_empty())
		ret--;
	    if (msec < 0 && !ret)
		continue;
	}
# endif
# ifdef GPM_MOUSE
	if (gpm_idx >= 0 && (fds[gpm_idx].revents & POLLIN))
	{
	    *check_for_gpm = 1;
	    ret--;
	    if (msec < 0 && !ret)
		continue;
	}
# endif
	break;

#else /* HAVE_SELECT */

	struct timeval  tv;
	fd_set		rfds, efds;
	int		maxfd;

# ifdef __EMX__
	/* don't check for incoming chars if not in raw mode, because select()
	 * always returns TRUE then (in some version of emx.dll) */
	if (curr_tmode != TMODE_RAW)
	    return 0;
# endif

	if (msec >= 0)
	{
	    tv.tv_sec = msec / 1000;
	    tv.tv_usec = (msec % 1000) * (1000000/1000);
	}

	/*
	 * Select on ready for reading and exceptional condition (end of file).
	 */
	FD_ZERO(&rfds); /* calls bzero() on a sun */
	FD_ZERO(&efds);
	FD_SET(fd, &rfds);
# if !defined(__QNX__) && !defined(__CYGWIN32__)
	/* For QNX select() always returns 1 if this is set.  Why? */
	FD_SET(fd, &efds);
# endif
	maxfd = fd;

# ifdef USE_SNIFF
	if (want_sniff_request)
	{
	    FD_SET(fd_from_sniff, &rfds);
	    FD_SET(fd_from_sniff, &efds);
	    if (maxfd < fd_from_sniff)
		maxfd = fd_from_sniff;
	}
# endif
# ifdef XTERM_CLIP
	if (xterm_Shell != (Widget)0)
	{
	    FD_SET(ConnectionNumber(xterm_dpy), &rfds);
	    if (maxfd < ConnectionNumber(xterm_dpy))
		maxfd = ConnectionNumber(xterm_dpy);
	}
# endif
# ifdef GPM_MOUSE
	if (check_for_gpm != NULL && gpm_flag && gpm_fd >= 0)
	{
	    FD_SET(gpm_fd, &rfds);
	    FD_SET(gpm_fd, &efds);
	    if (maxfd < gpm_fd)
		maxfd = gpm_fd;
	}
# endif

	ret = select(maxfd + 1, &rfds, NULL, &efds, (msec >= 0) ? &tv : NULL);

# ifdef USE_SNIFF
	if (ret < 0 )
	    sniff_disconnect(1);
	else if (ret > 0 && want_sniff_request)
	{
	    if (FD_ISSET(fd_from_sniff, &efds))
		sniff_disconnect(1);
	    if (FD_ISSET(fd_from_sniff, &rfds))
		sniff_request_waiting = 1;
	}
# endif
# ifdef XTERM_CLIP
	if (ret > 0 && xterm_Shell != (Widget)0
		&& FD_ISSET(ConnectionNumber(xterm_dpy), &rfds))
	{
	    xterm_update();	      /* Maybe we should hand out clipboard */
	    if (vim_is_input_buf_empty())
		ret--;
	    if (msec < 0 && !ret)
		continue;
	}
# endif
# ifdef GPM_MOUSE
	if (ret > 0 && gpm_flag && check_for_gpm != NULL && gpm_fd >= 0)
	{
	    if (FD_ISSET(gpm_fd, &efds))
		gpm_close();
	    else if (FD_ISSET(gpm_fd, &rfds))
		*check_for_gpm = 1;
	}
# endif

	break;
#endif /* HAVE_SELECT */
    }
    return (ret > 0);
}

#ifndef NO_EXPANDPATH
    static int
pstrcmp(a, b)
    const void *a, *b;
{
    return (strcmp(*(char **)a, *(char **)b));
}

/*
 * Recursively expand one path component into all matching files and/or
 * directories.
 * "path" has backslashes before chars that are not to be expanded, starting
 * at "path + wildoff".
 * Return the number of matches found.
 */
    int
mch_expandpath(gap, path, flags)
    struct growarray	*gap;
    char_u		*path;
    int			flags;		/* EW_* flags */
{
    return unix_expandpath(gap, path, 0, flags);
}

    static int
unix_expandpath(gap, path, wildoff, flags)
    struct growarray	*gap;
    char_u		*path;
    int			wildoff;
    int			flags;		/* EW_* flags */
{
    char_u		*buf;
    char_u		*path_end;
    char_u		*p, *s, *e;
    int			start_len, c;
    char_u		*pat;
    DIR			*dirp;
    vim_regexp		*prog;
    struct dirent	*dp;
    int			starts_with_dot;
    int			matches;
    int			len;

    start_len = gap->ga_len;
    buf = alloc(STRLEN(path) + BASENAMELEN + 5);/* make room for file name */
    if (buf == NULL)
	return 0;

/*
 * Find the first part in the path name that contains a wildcard.
 * Copy it into buf, including the preceding characters.
 */
    p = buf;
    s = buf;
    e = NULL;
    path_end = path;
    while (*path_end)
    {
	/* May ignore a wildcard that has a backslash before it */
	if (path_end >= path + wildoff && rem_backslash(path_end))
	    *p++ = *path_end++;
	else if (*path_end == '/')
	{
	    if (e != NULL)
		break;
	    else
		s = p + 1;
	}
	else if (vim_strchr((char_u *)"*?[{~$", *path_end) != NULL)
	    e = p;
	*p++ = *path_end++;
    }
    e = p;
    *e = NUL;

    /* now we have one wildcard component between s and e */
    /* Remove backslashes between "wildoff" and the start of the wildcard
     * component. */
    for (p = buf + wildoff; p < s; ++p)
	if (rem_backslash(p))
	{
	    STRCPY(p, p + 1);
	    --e;
	    --s;
	}

    /* convert the file pattern to a regexp pattern */
    starts_with_dot = (*s == '.');
    pat = file_pat_to_reg_pat(s, e, NULL, FALSE);
    if (pat == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* compile the regexp into a program */
    reg_ic = FALSE;			    /* Don't ever ignore case */
    prog = vim_regcomp(pat, TRUE);
    vim_free(pat);

    if (prog == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* open the directory for scanning */
    c = *s;
    *s = NUL;
    dirp = opendir(*buf == NUL ? "." : (char *)buf);
    *s = c;

    /* Find all matching entries */
    if (dirp != NULL)
    {
	for (;;)
	{
	    dp = readdir(dirp);
	    if (dp == NULL)
		break;
	    if ((dp->d_name[0] != '.' || starts_with_dot)
			     && vim_regexec(prog, (char_u *)dp->d_name, TRUE))
	    {
		STRCPY(s, dp->d_name);
		len = STRLEN(buf);
		STRCPY(buf + len, path_end);
		if (mch_has_wildcard(path_end))	/* handle more wildcards */
		{
		    /* need to expand another component of the path */
		    /* remove backslashes for the remaining components only */
		    (void)unix_expandpath(gap, buf, len + 1, flags);
		}
		else
		{
		    /* no more wildcards, check if there is a match */
		    /* remove backslashes for the remaining components only */
		    if (*path_end)
			backslash_halve(buf + len + 1);
		    if (mch_getperm(buf) >= 0)	/* add existing file */
			addfile(gap, buf, flags);
		}
	    }
	}

	closedir(dirp);
    }

    vim_free(buf);
    vim_free(prog);

    matches = gap->ga_len - start_len;
    if (matches)
	qsort(((char_u **)gap->ga_data) + start_len, matches,
						   sizeof(char_u *), pstrcmp);
    return matches;
}
#endif

/*
 * mch_expand_wildcards() - this code does wild-card pattern matching using
 * the shell
 *
 * return OK for success, FAIL for error (you may lose some memory) and put
 * an error message in *file.
 *
 * num_pat is number of input patterns
 * pat is array of pointers to input patterns
 * num_file is pointer to number of matched file names
 * file is pointer to array of pointers to matched file names
 */

#ifndef SEEK_SET
# define SEEK_SET 0
#endif
#ifndef SEEK_END
# define SEEK_END 2
#endif

/* ARGSUSED */
    int
mch_expand_wildcards(num_pat, pat, num_file, file, flags)
    int		   num_pat;
    char_u	 **pat;
    int		  *num_file;
    char_u	***file;
    int		   flags;	/* EW_* flags */
{
    int		i;
    size_t	len;
    char_u	*p;
    int		dir;
#ifdef __EMX__
# define EXPL_ALLOC_INC	16
    char_u	**expl_files;
    size_t	files_alloced, files_free;
    char_u	*buf;
    int		has_wildcard;

    *num_file = 0;	/* default: no files found */
    files_alloced = EXPL_ALLOC_INC; /* how much space is allocated */
    files_free = EXPL_ALLOC_INC;    /* how much space is not used  */
    *file = (char_u **) alloc(sizeof(char_u **) * files_alloced);
    if (*file == NULL)
	return FAIL;

    for (; num_pat > 0; num_pat--, pat++)
    {
	expl_files = NULL;
	if (vim_strchr(*pat, '$') || vim_strchr(*pat, '~'))
	    /* expand environment var or home dir */
	    buf = expand_env_save(*pat);
	else
	    buf = vim_strsave(*pat);
	expl_files = NULL;
	has_wildcard = mch_has_wildcard(buf);  /* (still) wildcards in there? */
	if (has_wildcard)   /* yes, so expand them */
	    expl_files = (char_u **)_fnexplode(buf);

	/*
	 * return value of buf if no wildcards left,
	 * OR if no match AND EW_NOTFOUND is set.
	 */
	if (!has_wildcard || (expl_files == NULL && (flags & EW_NOTFOUND)))
	{   /* simply save the current contents of *buf */
	    expl_files = (char_u **)alloc(sizeof(char_u **) * 2);
	    if (expl_files != NULL)
	    {
		expl_files[0] = vim_strsave(buf);
		expl_files[1] = NULL;
	    }
	}
	vim_free(buf);

	/*
	 * Count number of names resulting from expansion,
	 * At the same time add a backslash to the end of names that happen to
	 * be directories, and replace slashes with backslashes.
	 */
	if (expl_files)
	{
	    for (i = 0; (p = expl_files[i]) != NULL; i++)
	    {
		dir = mch_isdir(p);
		/* If we don't want dirs and this is one, skip it */
		if ((dir && !(flags & EW_DIR)) || (!dir && !(flags & EW_FILE)))
		    continue;

		if (--files_free == 0)
		{
		    /* need more room in table of pointers */
		    files_alloced += EXPL_ALLOC_INC;
		    *file = (char_u **)vim_realloc(*file,
					   sizeof(char_u **) * files_alloced);
		    if (*file == NULL)
		    {
			emsg(e_outofmem);
			*num_file = 0;
			return FAIL;
		    }
		    files_free = EXPL_ALLOC_INC;
		}
		slash_adjust(p);
		if (dir)
		{
		    len = strlen(p);
		    if (((*file)[*num_file] = alloc(len + 2)) != NULL)
		    {
			strcpy((*file)[*num_file], p);
			(*file)[*num_file][len] = psepc;
			(*file)[*num_file][len+1] = 0;
		    }
		}
		else
		{
		    (*file)[*num_file] = vim_strsave(p);
		}

		/*
		 * Error message already given by either alloc or vim_strsave.
		 * Should return FAIL, but returning OK works also.
		 */
		if ((*file)[*num_file] == NULL)
		    break;
		(*num_file)++;
	    }
	    _fnexplodefree((char **)expl_files);
	}
    }
    return OK;

#else /* __EMX__ */

    int		j;
    char_u	*tempname;
    char_u	*command;
    FILE	*fd;
    char_u	*buffer;
#define STYLE_ECHO  0	    /* use "echo" to expand */
#define STYLE_GLOB  1	    /* use "glob" to expand, for csh */
#define STYLE_PRINT 2	    /* use "print -N" to expand, for zsh */
#define STYLE_BT    3	    /* `cmd` expansion, execute the pattern directly */
    int		shell_style = STYLE_ECHO;
    int		check_spaces;
    static int	did_find_nul = FALSE;
    int		ampersent = FALSE;

    *num_file = 0;	/* default: no files found */
    *file = NULL;

    /*
     * If there are no wildcards, just copy the names to allocated memory.
     * Saves a lot of time, because we don't have to start a new shell.
     */
    if (!have_wildcard(num_pat, pat))
	return save_patterns(num_pat, pat, num_file, file);

/*
 * get a name for the temp file
 */
    if ((tempname = vim_tempname('o')) == NULL)
    {
	emsg(e_notmp);
	return FAIL;
    }

/*
 * Let the shell expand the patterns and write the result into the temp file.
 * if expanding `cmd` execute it directly.
 * If we use csh, glob will work better than echo.
 * If we use zsh, print -N will work better than glob.
 */
    if (num_pat == 1 && *pat[0] == '`' && *(pat[0] + STRLEN(pat[0]) - 1) == '`')
	shell_style = STYLE_BT;
    else if ((len = STRLEN(p_sh)) >= 3)
    {
	if (STRCMP(p_sh + len - 3, "csh") == 0)
	    shell_style = STYLE_GLOB;
	else if (STRCMP(p_sh + len - 3, "zsh") == 0)
	    shell_style = STYLE_PRINT;
    }

    /* "unset nonomatch; print -N >" plus two is 29 */
    len = STRLEN(tempname) + 29;
    for (i = 0; i < num_pat; ++i)	/* count the length of the patterns */
	len += STRLEN(pat[i]) + 3;	/* add space and two quotes */
    command = alloc(len);
    if (command == NULL)
    {
	vim_free(tempname);
	return FAIL;
    }

    /*
     * Build the shell command:
     * - Set $nonomatch depending on EW_NOTFOUND (hopefully the shell
     *	 recognizes this).
     * - Add the shell command to print the expanded names.
     * - Add the temp file name.
     * - Add the file name patterns.
     */
    if (shell_style == STYLE_BT)
    {
	STRCPY(command, pat[0] + 1);		/* exclude first backtick */
	p = command + STRLEN(command) - 1;
	*p = ' ';				/* remove last backtick */
	while (p > command && vim_iswhite(*p))
	    --p;
	if (*p == '&')				/* remove trailing '&' */
	{
	    ampersent = TRUE;
	    *p = ' ';
	}
	STRCAT(command, ">");
    }
    else
    {
	if (flags & EW_NOTFOUND)
	    STRCPY(command, "set nonomatch; ");
	else
	    STRCPY(command, "unset nonomatch; ");
	if (shell_style == STYLE_GLOB)
	    STRCAT(command, "glob >");
	else if (shell_style == STYLE_PRINT)
	    STRCAT(command, "print -N >");
	else
	    STRCAT(command, "echo >");
    }
    STRCAT(command, tempname);
    if (shell_style != STYLE_BT)
	for (i = 0; i < num_pat; ++i)
	{
#ifdef USE_SYSTEM
	    STRCAT(command, " \"");	    /* need extra quotes because we */
	    STRCAT(command, pat[i]);	    /*	 start the shell twice */
	    STRCAT(command, "\"");
#else
	    STRCAT(command, " ");
	    STRCAT(command, pat[i]);
#endif
	}
    if (flags & EW_SILENT)
	show_shell_mess = FALSE;
    if (ampersent)
	STRCAT(command, "&");		    /* put the '&' back after the
					       redirection */

    /*
     * Using zsh -G: If a pattern has no matches, it is just deleted from
     * the argument list, otherwise zsh gives an error message and doesn't
     * expand any other pattern.
     */
    if (shell_style == STYLE_PRINT)
	extra_shell_arg = (char_u *)"-G";   /* Use zsh NULL_GLOB option */

    /*
     * If we use -f then shell variables set in .cshrc won't get expanded.
     * vi can do it, so we will too, but it is only necessary if there is a "$"
     * in one of the patterns, otherwise we can still use the fast option.
     */
    else if (shell_style == STYLE_GLOB && !have_dollars(num_pat, pat))
	extra_shell_arg = (char_u *)"-f";	/* Use csh fast option */

    /*
     * execute the shell command
     */
    i = call_shell(command,
		     SHELL_EXPAND | ((flags & EW_SILENT) ? SHELL_SILENT : 0));

    /* When running in the background, give it some time to create the temp
     * file, but don't wait for it to finish. */
    if (ampersent)
	mch_delay(10L, TRUE);

    extra_shell_arg = NULL;		/* cleanup */
    show_shell_mess = TRUE;
    vim_free(command);

    if (i)				/* mch_call_shell() failed */
    {
	mch_remove(tempname);
	vim_free(tempname);
	/*
	 * With interactive completion, the error message is not printed.
	 * However with USE_SYSTEM, I don't know how to turn off error messages
	 * from the shell, so screen may still get messed up -- webb.
	 */
#ifndef USE_SYSTEM
	if (!(flags & EW_SILENT))
#endif
	{
	    must_redraw = CLEAR;	/* probably messed up screen */
	    msg_putchar('\n');		/* clear bottom line quickly */
	    cmdline_row = Rows - 1;	/* continue on last line */
	}
	return FAIL;
    }

/*
 * read the names from the file into memory
 */
    fd = fopen((char *)tempname, "r");
    if (fd == NULL)
    {
	emsg2(e_notopen, tempname);
	vim_free(tempname);
	return FAIL;
    }
    fseek(fd, 0L, SEEK_END);
    len = ftell(fd);		    /* get size of temp file */
    fseek(fd, 0L, SEEK_SET);
    buffer = alloc(len + 1);
    if (buffer == NULL)
    {
	mch_remove(tempname);
	vim_free(tempname);
	fclose(fd);
	return FAIL;
    }
    i = fread((char *)buffer, 1, len, fd);
    fclose(fd);
    mch_remove(tempname);
    if (i != len)
    {
	emsg2(e_notread, tempname);
	vim_free(tempname);
	vim_free(buffer);
	return FAIL;
    }
    vim_free(tempname);

    /* file names are separated with Space */
    if (shell_style == STYLE_ECHO)
    {
	buffer[len] = '\n';		/* make sure the buffer ends in NL */
	p = buffer;
	for (i = 0; *p != '\n'; ++i)	/* count number of entries */
	{
	    while (*p != ' ' && *p != '\n')
		++p;
	    p = skipwhite(p);		/* skip to next entry */
	}
    }
    /* file names are separated with NL */
    else if (shell_style == STYLE_BT)
    {
	buffer[len] = NUL;		/* make sure the buffer ends in NUL */
	p = buffer;
	for (i = 0; *p != NUL; ++i)	/* count number of entries */
	{
	    while (*p != '\n' && *p != NUL)
		++p;
	    if (*p != NUL)
		++p;
	    p = skipwhite(p);		/* skip leading white space */
	}
    }
    else /* file names are separated with NUL */
    {
	/*
	 * Some versions of zsh use spaces instead of NULs to separate
	 * results.  Only do this when there is no NUL before the end of the
	 * buffer, otherwise we would never be able to use file names with
	 * embedded spaces when zsh does use NULs.
	 * When we found a NUL once, we know zsh is OK, set did_find_nul and
	 * don't check for spaces again.
	 */
	check_spaces = FALSE;
	if (shell_style == STYLE_PRINT && !did_find_nul)
	{
	    /* If there is a NUL, set did_find_nul, else set check_spaces */
	    if (len && (int)STRLEN(buffer) < len - 1)
		did_find_nul = TRUE;
	    else
		check_spaces = TRUE;
	}

	/*
	 * Make sure the buffer ends with a NUL.  For STYLE_PRINT there
	 * already is one, for STYLE_GLOB it needs to be added.
	 */
	if (len && buffer[len - 1] == NUL)
	    --len;
	else
	    buffer[len] = NUL;
	i = 0;
	for (p = buffer; p < buffer + len; ++p)
	    if (*p == NUL || (*p == ' ' && check_spaces))   /* count entry */
	    {
		++i;
		*p = NUL;
	    }
	if (len)
	    ++i;			/* count last entry */
    }
    if (i == 0)
    {
	/*
	 * Can happen when using /bin/sh and typing ":e $NO_SUCH_VAR^I".
	 * /bin/sh will happily expand it to nothing rather than returning an
	 * error; and hey, it's good to check anyway -- webb.
	 */
	vim_free(buffer);
	goto notfound;
    }
    *num_file = i;
    *file = (char_u **)alloc(sizeof(char_u *) * i);
    if (*file == NULL)
    {
	vim_free(buffer);
	return FAIL;
    }

    /*
     * Isolate the individual file names.
     */
    p = buffer;
    for (i = 0; i < *num_file; ++i)
    {
	(*file)[i] = p;
	/* Space or NL separates */
	if (shell_style == STYLE_ECHO || shell_style == STYLE_BT)
	{
	    while (!(shell_style == STYLE_ECHO && *p == ' ') && *p != '\n')
		++p;
	    if (p == buffer + len)		/* last entry */
		*p = NUL;
	    else
	    {
		*p++ = NUL;
		p = skipwhite(p);		/* skip to next entry */
	    }
	}
	else		/* NUL separates */
	{
	    while (*p && p < buffer + len)	/* skip entry */
		++p;
	    ++p;				/* skip NUL */
	}
    }

    /*
     * Move the file names to allocated memory.
     */
    for (j = 0, i = 0; i < *num_file; ++i)
    {
	/* Require the files to exist.	Helps when using /bin/sh */
	if (!(flags & EW_NOTFOUND) && mch_getperm((*file)[i]) < 0)
	    continue;

	/* check if this entry should be included */
	dir = (mch_isdir((*file)[i]));
	if ((dir && !(flags & EW_DIR)) || (!dir && !(flags & EW_FILE)))
	    continue;

	p = alloc((unsigned)(STRLEN((*file)[i]) + 1 + dir));
	if (p)
	{
	    STRCPY(p, (*file)[i]);
	    if (dir)
		STRCAT(p, "/");	    /* add '/' to a directory name */
	    (*file)[j++] = p;
	}
    }
    vim_free(buffer);
    *num_file = j;

    if (*num_file == 0)	    /* rejected all entries */
    {
	vim_free(*file);
	*file = NULL;
	goto notfound;
    }

    return OK;

notfound:
    if (flags & EW_NOTFOUND)
	return save_patterns(num_pat, pat, num_file, file);
    return FAIL;

#endif /* __EMX__ */
}

#ifndef __EMX__
    static int
save_patterns(num_pat, pat, num_file, file)
    int		num_pat;
    char_u	**pat;
    int		*num_file;
    char_u	***file;
{
    int		i;

    *file = (char_u **)alloc(num_pat * sizeof(char_u *));
    if (*file == NULL)
	return FAIL;
    for (i = 0; i < num_pat; i++)
	(*file)[i] = vim_strsave(pat[i]);
    *num_file = num_pat;
    return OK;
}
#endif


    int
mch_has_wildcard(p)
    char_u  *p;
{
#ifdef OS2
# ifdef VIM_BACKTICK
    return (vim_strpbrk(p, (char_u *)"?*$`~") != NULL);
# else
    return (vim_strpbrk(p, (char_u *)"?*$~") != NULL);
# endif
#else
    for ( ; *p; ++p)
    {
	if (*p == '\\' && p[1] != NUL)
	    ++p;
	else if (vim_strchr((char_u *)"*?[{`'~$", *p) != NULL)
	    return TRUE;
    }
    return FALSE;
#endif
}

#ifndef __EMX__
    static int
have_wildcard(num, file)
    int	    num;
    char_u  **file;
{
    int	    i;

    for (i = 0; i < num; i++)
	if (mch_has_wildcard(file[i]))
	    return 1;
    return 0;
}

    static int
have_dollars(num, file)
    int	    num;
    char_u  **file;
{
    int	    i;

    for (i = 0; i < num; i++)
	if (vim_strchr(file[i], '$') != NULL)
	    return TRUE;
    return FALSE;
}
#endif	/* ifndef __EMX__ */

#ifndef HAVE_RENAME
/*
 * Scaled-down version of rename(), which is missing in Xenix.
 * This version can only move regular files and will fail if the
 * destination exists.
 */
    int
mch_rename(src, dest)
    const char *src, *dest;
{
    struct stat	    st;

    if (stat(dest, &st) >= 0)	    /* fail if destination exists */
	return -1;
    if (link(src, dest) != 0)	    /* link file to new name */
	return -1;
    if (mch_remove(src) == 0)	    /* delete link to old name */
	return 0;
    return -1;
}
#endif /* !HAVE_RENAME */

#ifdef GPM_MOUSE
/*
 * Initializes connection with gpm (if it isn't already opened)
 * Return 1 if succeeded (or connection already opened), 0 if failed
 */
    static int
gpm_open()
{
    static Gpm_Connect gpm_connect; /* Must it be kept till closing ? */

    if (!gpm_flag)
    {
	gpm_connect.eventMask = (GPM_UP | GPM_DRAG | GPM_DOWN);
	gpm_connect.defaultMask = ~GPM_HARD;
	/* Default handling for mouse move*/
	gpm_connect.minMod = 0; /* Handle any modifier keys */
	gpm_connect.maxMod = 0xffff;
	if (Gpm_Open(&gpm_connect, 0) > 0)
	{
	    /* gpm library tries to handling TSTP causes
	     * problems. Anyways, we close connection to Gpm whenever
	     * we are going to suspend or starting an external process
	     * so we should'nt  have problem with this
	     */
	    signal(SIGTSTP, restricted ? SIG_IGN : SIG_DFL);
	    return 1; /* succeed */
	}
	if (gpm_fd == -2)
	    Gpm_Close(); /* We don't want to talk to xterm via gpm */
	return 0;
    }
    return 1; /* already open */
}

/*
 * Closes connection to gpm
 * returns non-zero if connection succesfully closed
 */
    static void
gpm_close()
{
    if (gpm_flag && gpm_fd >= 0) /* if Open */
	Gpm_Close();
}

/* Reads gpm event and adds special keys to input buf. Returns length of
 * generated key sequence.
 * This function is made after gui_send_mouse_event
 */
    static int
mch_gpm_process()
{
    int			button;
    static Gpm_Event	gpm_event;
    char_u		string[6];
    int_u		vim_modifiers;
    int			row,col;
    unsigned char	buttons_mask;
    unsigned char	gpm_modifiers;
    static unsigned char old_buttons = 0;

    Gpm_GetEvent(&gpm_event);

#ifdef USE_GUI
    /* Don't put events in the input queue now. */
    if (hold_gui_events)
	return 0;
#endif

    row = gpm_event.y - 1;
    col = gpm_event.x - 1;

    string[0] = ESC; /* Our termcode */
    string[1] = 'M';
    string[2] = 'G';
    switch (GPM_BARE_EVENTS(gpm_event.type))
    {
	case GPM_DRAG:
	    string[3] = MOUSE_DRAG;
	    break;
	case GPM_DOWN:
	    buttons_mask = gpm_event.buttons & ~old_buttons;
	    old_buttons = gpm_event.buttons;
	    switch (buttons_mask)
	    {
		case GPM_B_LEFT:
		    button = MOUSE_LEFT;
		    break;
		case GPM_B_MIDDLE:
		    button = MOUSE_MIDDLE;
		    break;
		case GPM_B_RIGHT:
		    button = MOUSE_RIGHT;
		    break;
		default:
		    return 0;
		    /*Don't know what to do. Can more than one button be
		     * reported in one event? */
	    }
	    string[3] = (char_u)(button | 0x20);
	    SET_NUM_MOUSE_CLICKS(string[3], gpm_event.clicks + 1);
	    break;
	case GPM_UP:
	    string[3] = MOUSE_RELEASE;
	    old_buttons &= ~gpm_event.buttons;
	    break;
	default:
	    return 0;
    }
    /*This code is based on gui_x11_mouse_cb in gui_x11.c */
    gpm_modifiers = gpm_event.modifiers;
    vim_modifiers = 0x0;
    /* I ignore capslock stats. Aren't we all just hate capslock mixing with
     * Vim commands ? Besides, gpm_event.modifiers is unsigned char, and
     * K_CAPSSHIFT is defined 8, so it probably isn't even reported
     */
    if (gpm_modifiers & ((1 << KG_SHIFT) | (1 << KG_SHIFTR) | (1 << KG_SHIFTL)))
	vim_modifiers |= MOUSE_SHIFT;

    if (gpm_modifiers & ((1 << KG_CTRL) | (1 << KG_CTRLR) | (1 << KG_CTRLL)))
	vim_modifiers |= MOUSE_CTRL;
    if (gpm_modifiers & ((1 << KG_ALT) | (1 << KG_ALTGR)))
	vim_modifiers |= MOUSE_ALT;
    string[3] |= vim_modifiers;
    string[4] = (char_u)(col + ' ' + 1);
    string[5] = (char_u)(row + ' ' + 1);
    add_to_input_buf(string, 6);
    return 6;
}
#endif /* GPM_MOUSE */

#if (defined(HAVE_X11) && defined(WANT_X11) && defined(XTERM_CLIP)) \
	|| defined(PROTO)
static int	    xterm_trace = -1;	/* default: disabled */
static int	    xterm_button;

/*
 * Setup a dummy window for selections in an xterm.
 * Currently only works for x11.
 */
    void
setup_xterm_clip()
{
    int		z = 0;
    char	*strp = "";
    Widget	AppShell;

    open_app_context();
    if (app_context != NULL && xterm_Shell == (Widget)0)
    {
	/* Ignore X errors while opening the display */
	XSetErrorHandler(x_error_check);

	xterm_dpy = XtOpenDisplay(app_context, xterm_display,
		"vim_xterm", "Vim_xterm", NULL, 0, &z, &strp);

	/* Now handle X errors normally. */
	XSetErrorHandler(x_error_handler);

	if (xterm_dpy == NULL)
	    return;

	/* Create a Shell to make converters work. */
	AppShell = XtVaAppCreateShell("vim_xterm", "Vim_xterm",
		applicationShellWidgetClass, xterm_dpy,
		NULL);
	if (AppShell == (Widget)0)
	    return;
	xterm_Shell = XtVaCreatePopupShell("VIM",
		topLevelShellWidgetClass, AppShell,
		XtNmappedWhenManaged, 0,
		XtNwidth, 1,
		XtNheight, 1,
		NULL);
	x11_setup_atoms(xterm_dpy);
	if (xterm_Shell == (Widget)0)
	    return;

	if (x11_display == NULL)
	    x11_display = xterm_dpy;

	XtRealizeWidget(xterm_Shell);
	XSync(xterm_dpy, False);
	xterm_update();
    }
    if (xterm_Shell != (Widget)0)
    {
	clip_init(TRUE);
	if (x11_window == 0 && (strp = getenv("WINDOWID")) != NULL)
	    x11_window = (Window)atol(strp);
	/* Check if $WINDOWID is valid. */
	if (test_x11_window(xterm_dpy) == FAIL)
	    x11_window = 0;
	if (x11_window != 0)
	    xterm_trace = 0;
    }
}

    void
start_xterm_trace(button)
    int button;
{
    if (x11_window == 0 || xterm_trace < 0 || xterm_Shell == (Widget)0)
	return;
    xterm_trace = 1;
    xterm_button = button;
    do_xterm_trace();
}


    void
stop_xterm_trace()
{
    if (xterm_trace < 0)
	return;
    xterm_trace = 0;
}

/*
 * Query the xterm pointer and generate mouse termcodes if necessary
 * return TRUE if dragging is active, else FALSE
 */
    static int
do_xterm_trace()
{
    Window		root, child;
    int			root_x, root_y;
    int			win_x, win_y;
    int			row, col;
    int_u		mask_return;
    char_u		buf[50];
    char_u		*strp;
    long		got_hints;
    static char_u	*mouse_code;
    static char_u	mouse_name[2] = {KS_MOUSE, K_FILLER};
    static int		prev_row = 0, prev_col = 0;
    static XSizeHints	xterm_hints;

    if (xterm_trace <= 0)
	return FALSE;

    if (xterm_trace == 1)
    {
	/* Get the hints just before tracking starts.  The font size might
	 * have changed recently */
	XGetWMNormalHints(xterm_dpy, x11_window, &xterm_hints, &got_hints);
	if (!(got_hints & PResizeInc)
		|| xterm_hints.width_inc <= 1
		|| xterm_hints.height_inc <= 1)
	{
	    xterm_trace = -1;  /* Not enough data -- disable tracing */
	    return FALSE;
	}

	/* Rely on the same mouse code for the duration of this */
	mouse_code = find_termcode(mouse_name);
	prev_row = mouse_row;
	prev_row = mouse_col;
	xterm_trace = 2;

	/* Find the offset of the chars, there might be a scrollbar on the
	 * left of the window and/or a menu on the top (eterm etc.) */
	XQueryPointer(xterm_dpy, x11_window, &root, &child, &root_x, &root_y,
		      &win_x, &win_y, &mask_return);
	xterm_hints.y = win_y - (xterm_hints.height_inc * mouse_row)
			      - (xterm_hints.height_inc / 2);
	if (xterm_hints.y <= xterm_hints.height_inc / 2)
	    xterm_hints.y = 2;
	xterm_hints.x = win_x - (xterm_hints.width_inc * mouse_col)
			      - (xterm_hints.width_inc / 2);
	if (xterm_hints.x <= xterm_hints.width_inc / 2)
	    xterm_hints.x = 2;
	return TRUE;
    }
    if (mouse_code == NULL)
    {
	xterm_trace = 0;
	return FALSE;
    }

    XQueryPointer(xterm_dpy, x11_window, &root, &child, &root_x, &root_y,
		  &win_x, &win_y, &mask_return);

    row = check_row((win_y - xterm_hints.y) / xterm_hints.height_inc);
    col = check_col((win_x - xterm_hints.x) / xterm_hints.width_inc);
    if (row == prev_row && col == prev_col)
	return TRUE;

    STRCPY(buf, mouse_code);
    strp = buf + STRLEN(buf);
    *strp++ = (xterm_button | MOUSE_DRAG) & ~0x20;
    *strp++ = (char_u)(col + ' ' + 1);
    *strp++ = (char_u)(row + ' ' + 1);
    *strp = 0;
    add_to_input_buf(buf, STRLEN(buf));

    prev_row = row;
    prev_col = col;
    return TRUE;
}

/*
 * Destroy the display, window and app_context.  Required for GTK.
 */
    void
clear_xterm_clip()
{
    if (xterm_Shell != (Widget)0)
    {
	XtDestroyWidget(xterm_Shell);
	xterm_Shell = (Widget)0;
    }
    if (xterm_dpy != NULL)
    {
#if 0
	/* Lesstif and Solaris crash here, lose some memory */
	XtCloseDisplay(xterm_dpy);
#endif
	if (x11_display == xterm_dpy)
	    x11_display = NULL;
	xterm_dpy = NULL;
    }
#if 0
    if (app_context != (XtAppContext)NULL)
    {
	/* Lesstif and Solaris crash here, lose some memory */
	XtDestroyApplicationContext(app_context);
	app_context = (XtAppContext)NULL;
    }
#endif
}

/*
 * Catch up with any queued X events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X event queue (& no timers pending), then we return
 * immediately.
 */
    static void
xterm_update()
{
    while (XtAppPending(app_context) && !vim_is_input_buf_full())
	XtAppProcessEvent(app_context, (XtInputMask)XtIMAll);
}

    int
clip_xterm_own_selection()
{
    if (xterm_Shell != (Widget)0)
	return clip_x11_own_selection(xterm_Shell);
    return FALSE;
}

    void
clip_xterm_lose_selection()
{
    if (xterm_Shell != (Widget)0)
	clip_x11_lose_selection(xterm_Shell);
}

    void
clip_xterm_request_selection()
{
    if (xterm_Shell != (Widget)0)
	clip_x11_request_selection(xterm_Shell, xterm_dpy);
}

    void
clip_xterm_set_selection()
{
    clip_x11_set_selection();
}
#endif
