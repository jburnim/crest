/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * proto.h: include the (automatically generated) function prototypes
 */

/*
 * Don't include these while generating prototypes.  Prevents problems when
 * files are missing.
 */
#if !defined(PROTO) && !defined(NOPROTO)

/*
 * Machine-dependent routines.
 */
# if (!defined(HAVE_X11) || !defined(WANT_X11)) && !defined(USE_GUI_GTK)
#  define Display int	/* avoid errors in function prototypes */
#  define Widget int
# endif
# ifdef AMIGA
#  include "os_amiga.pro"
# endif
# if defined(UNIX) || defined(__EMX__)
#  include "os_unix.pro"
# endif
# ifdef MSDOS
#  include "os_msdos.pro"
# endif
# ifdef WIN16
#  include "os_win16.pro"
# endif
# ifdef WIN32
#  include "os_win32.pro"
# endif
# ifdef VMS
#  include "os_vms.pro"
# endif
# ifdef __BEOS__
#  include "os_beos.pro"
# endif
# ifdef macintosh
#  include "os_mac.pro"
# endif
# ifdef RISCOS
#  include "os_riscos.pro"
# endif

# include "buffer.pro"
# include "charset.pro"
# ifdef UNIX
#  include "if_cscope.pro"
# endif
# include "digraph.pro"
# include "edit.pro"
# include "eval.pro"
# include "ex_cmds.pro"
# include "ex_docmd.pro"
# include "ex_getln.pro"
# include "fileio.pro"
# include "getchar.pro"
# ifdef HANGUL_INPUT
#  include "hangulin.pro"
# endif
# include "main.pro"
# include "mark.pro"
# ifndef MESSAGE_FILE
void
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg __ARGS((char_u *, ...));	/* cannot be produced automatically */
void
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg_attr __ARGS((int, char_u *, ...));
# endif
# include "memfile.pro"
# include "memline.pro"
# ifdef WANT_MENU
#  include "menu.pro"
# endif
# include "message.pro"
# include "misc1.pro"
# include "misc2.pro"
#ifndef HAVE_STRPBRK	    /* not generated automatically from misc2.c */
char_u *vim_strpbrk __ARGS((char_u *s, char_u *charset));
#endif
# if defined(MULTI_BYTE) || defined(USE_XIM)
#  include "multbyte.pro"
# endif
# include "normal.pro"
# include "ops.pro"
# include "option.pro"
# include "quickfix.pro"
# include "regexp.pro"
# include "screen.pro"
# include "search.pro"
# include "syntax.pro"
# include "tag.pro"
# include "term.pro"
# if defined(HAVE_TGETENT) && (defined(AMIGA) || defined(VMS))
#  include "termlib.pro"
# endif
# include "ui.pro"
# include "undo.pro"
# include "version.pro"
# include "window.pro"

# ifdef HAVE_PYTHON
#  include "if_python.pro"
# endif

# ifdef HAVE_TCL
#  include "if_tcl.pro"
# endif

# ifdef USE_GUI
#  include "gui.pro"
#  include "pty.pro"
#  if !defined(HAVE_SETENV) && !defined(HAVE_PUTENV)
extern int putenv __ARGS((const char *string));		/* from pty.c */
#   ifdef USE_VIMPTY_GETENV
extern char_u *vimpty_getenv __ARGS((const char_u *string));	/* from pty.c */
#   endif
#  endif
#  ifdef USE_GUI_WIN16
#   include "gui_w16.pro"
#  endif
#  ifdef USE_GUI_WIN32
#   include "gui_w32.pro"
#   ifdef HAVE_OLE
#    include "if_ole.pro"
#   endif
#  endif
#  ifdef USE_GUI_GTK
#   include "gui_gtk.pro"
#   include "gui_gtk_x11.pro"
#  endif
#  ifdef USE_GUI_MOTIF
#   include "gui_motif.pro"
#  endif
#  ifdef USE_GUI_ATHENA
#   include "gui_athena.pro"
#ifdef USE_BROWSE
extern char *vim_SelFile __ARGS((Widget toplevel, char *prompt, char *init_path, int (*show_entry)(), int x, int y, GuiColor fg, GuiColor bg));
#endif
#  endif
#  ifdef USE_GUI_BEOS
#   include "gui_beos.pro"
#  endif
#  ifdef USE_GUI_MAC
#   include "gui_mac.pro"
#  endif
#  ifdef USE_GUI_X11
#   include "gui_x11.pro"
#  endif
#  if defined(USE_GUI_AMIGA)
#    include "gui_amiga.pro"
#  endif
#  ifdef RISCOS
#   include "gui_riscos.pro"
#  endif
# endif	/* USE_GUI */

/*
 * The perl include files pollute the namespace, therfore proto.h must be
 * included before the perl include files.  But then CV is not defined, which
 * is used in if_perl.pro.  To get around this, the perl prototype files are
 * not included here for the perl files.  Use a dummy define for CV for the
 * other files.
 */
#if defined(HAVE_PERL_INTERP) && !defined(IN_PERL_FILE)
# define CV void
# ifdef __BORLANDC__
#  pragma option -pc
# endif
# include "if_perl.pro"
# ifdef __BORLANDC__
#  pragma option -p.
# endif
# include "if_perlsfio.pro"
#endif

#ifdef __BORLANDC__
# define _PROTO_H
#endif
#endif /* !PROTO && !NOPROTO */
