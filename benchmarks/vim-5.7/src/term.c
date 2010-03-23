/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
 *
 * term.c: functions for controlling the terminal
 *
 * primitive termcap support for Amiga, MSDOS, and Win32 included
 *
 * NOTE: padding and variable substitution is not performed,
 * when compiling without HAVE_TGETENT, we use tputs() and tgoto() dummies.
 */

/*
 * Some systems have a prototype for tgetstr() with (char *) instead of
 * (char **). This define removes that prototype. We include our own prototype
 * below.
 */

#define tgetstr tgetstr_defined_wrong
#include "vim.h"

#ifdef HAVE_TGETENT
# ifdef HAVE_TERMIOS_H
#  include <termios.h>	    /* seems to be required for some Linux */
# endif
# ifdef HAVE_TERMCAP_H
#  include <termcap.h>
# endif

/*
 * A few linux systems define outfuntype in termcap.h to be used as the third
 * argument for tputs().
 */
# ifdef VMS
#  define TPUTSFUNCAST
# else
#  ifdef HAVE_OUTFUNTYPE
#   define TPUTSFUNCAST (outfuntype)
#  else
#   define TPUTSFUNCAST (int (*)())
#  endif
# endif
#endif

#undef tgetstr

/*
 * Here are the builtin termcap entries.  They are not stored as complete
 * Tcarr structures, as such a structure is too big.
 *
 * The entries are compact, therefore they normally are included even when
 * HAVE_TGETENT is defined. When HAVE_TGETENT is defined, the builtin entries
 * can be accessed with "builtin_amiga", "builtin_ansi", "builtin_debug", etc.
 *
 * Each termcap is a list of builtin_term structures. It always starts with
 * KS_NAME, which separates the entries.  See parse_builtin_tcap() for all
 * details.
 * bt_entry is either a KS_xxx code (< 0x100), or a K_xxx code.
 */
struct builtin_term
{
    int		bt_entry;
    char	*bt_string;
};

/* start of keys that are not directly used by Vim but can be mapped */
#define BT_EXTRA_KEYS	0x101

static struct builtin_term *find_builtin_term __ARGS((char_u *name));
static void parse_builtin_tcap __ARGS((char_u *s));
static void term_color __ARGS((char_u *s, int n));
static void gather_termleader __ARGS((void));
#if defined(USE_GUI) \
    || (defined(USE_MOUSE) && (!defined(UNIX) || defined(XTERM_MOUSE)))
static int get_bytes_from_buf __ARGS((char_u *, char_u *, int));
#endif
static int term_is_builtin __ARGS((char_u *name));
static int term_7to8bit __ARGS((char_u *p));

#ifdef HAVE_TGETENT
static char_u *tgetent_error __ARGS((char_u *, char_u *));

/*
 * Here is our own prototype for tgetstr(), any prototypes from the include
 * files have been disabled by the define at the start of this file.
 */
char		*tgetstr __ARGS((char *, char **));

/* When set: Get terminal version when switched to RAW mode */
static int need_get_crv = TRUE;

/*
 * Don't declare these variables if termcap.h contains them.
 * Autoconf checks if these variables should be declared extern (not all
 * systems have them).
 * Some versions define ospeed to be speed_t, but that is incompatible with
 * BSD, where ospeed is short and speed_t is long.
 */
# ifndef HAVE_OSPEED
#  ifdef OSPEED_EXTERN
extern short ospeed;
#   else
short ospeed;
#   endif
# endif
# ifndef HAVE_UP_BC_PC
#  ifdef UP_BC_PC_EXTERN
extern char *UP, *BC, PC;
#  else
char *UP, *BC, PC;
#  endif
# endif

# define TGETSTR(s, p)	vim_tgetstr((s), (p))
# define TGETENT(b, t)	tgetent((char *)(b), (char *)(t))
static char_u *vim_tgetstr __ARGS((char *s, char_u **pp));
#endif /* HAVE_TGETENT */

struct builtin_term builtin_termcaps[] =
{

#if defined(USE_GUI)
/*
 * Motif/Athena pseudo term-cap.
 */
    {(int)KS_NAME,	"gui"},
    {(int)KS_CE,	"\033|$"},
    {(int)KS_AL,	"\033|i"},
# ifdef TERMINFO
    {(int)KS_CAL,	"\033|%p1%dI"},
# else
    {(int)KS_CAL,	"\033|%dI"},
# endif
    {(int)KS_DL,	"\033|d"},
# ifdef TERMINFO
    {(int)KS_CDL,	"\033|%p1%dD"},
    {(int)KS_CS,	"\033|%p1%d;%p2%dR"},
# else
    {(int)KS_CDL,	"\033|%dD"},
    {(int)KS_CS,	"\033|%d;%dR"},
# endif
    {(int)KS_CL,	"\033|C"},
    {(int)KS_ME,	"\033|31H"},	/* 31 = HL_ALL,		H = off */
    {(int)KS_MR,	"\033|1h"},	/* 1  = HL_INVERSE,	h = on */
    {(int)KS_MD,	"\033|2h"},	/* 2  = HL_BOLD,	h = on */
    {(int)KS_SE,	"\033|16H"},	/* 16 = HL_STANDOUT,	H = off */
    {(int)KS_SO,	"\033|16h"},	/* 16 = HL_STANDOUT,	h = on */
    {(int)KS_UE,	"\033|8H"},	/* 8  = HL_UNDERLINE,	H = off */
    {(int)KS_US,	"\033|8h"},	/* 8  = HL_UNDERLINE,	h = on */
    {(int)KS_CZR,	"\033|4H"},	/* 4  = HL_ITALIC,	H = off */
    {(int)KS_CZH,	"\033|4h"},	/* 4  = HL_ITALIC,	h = on */
    {(int)KS_VB,	"\033|f"},
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},	/* cursur-left = BS */
    {(int)KS_ND,	"\014"},	/* cursor-right = CTRL-L */
# ifdef TERMINFO
    {(int)KS_CM,	"\033|%p1%d;%p2%dM"},
# else
    {(int)KS_CM,	"\033|%d;%dM"},
# endif
	/* there are no key sequences here, the GUI sequences are recognized
	 * in check_termcodes() */
#endif

#ifndef NO_BUILTIN_TCAPS
# if defined(RISCOS) || defined(ALL_BUILTIN_TCAPS)
/*
 * Default for the Acorn.
 */
    {(int)KS_NAME,	"riscos"},
    {(int)KS_CL,	"\014"},		/* Cls and Home Cursor */
    {(int)KS_CM,	"\001%d\001%d\002"},	/* Position cursor */

    {(int)KS_CCO,	"16"},			/* Allow 16 colors */

    {(int)KS_CAF,	"\001%d\021"},		/* Set foreground colour */
    {(int)KS_CAB,	"\001%d\022"},		/* Set background colour */


    {(int)KS_ME,	"\004"},		/* Normal mode */
    {(int)KS_MR,	"\005"},		/* Reverse */

    {(int)KS_VI,	"\016"},		/* Cursor invisible */
    {(int)KS_VE,	"\017"},		/* Cursor visible */
    {(int)KS_VS,	"\018"},		/* Cursor very visible */

    {(int)KS_CS,	"\001%d\001%d\003"},	/* Set scroll region */
    {(int)KS_SR,	"\023"},		/* Scroll text down */
    {K_UP,		"\217"},
    {K_DOWN,		"\216"},
    {K_LEFT,		"\214"},
    {K_RIGHT,		"\215"},
    {K_S_UP,		"\237"},
    {K_S_DOWN,		"\236"},
    {K_S_LEFT,		"\234"},
    {K_S_RIGHT,		"\235"},

    {K_F1,		"\201"},
    {K_F2,		"\202"},
    {K_F3,		"\203"},
    {K_F4,		"\204"},
    {K_F5,		"\205"},
    {K_F6,		"\206"},
    {K_F7,		"\207"},
    {K_F8,		"\208"},
    {K_F9,		"\209"},
    {K_F10,		"\312"},
    {K_F11,		"\313"},
    {K_F12,		"\314"},
    {K_S_F1,		"\221"},
    {K_S_F2,		"\222"},
    {K_S_F3,		"\223"},
    {K_S_F4,		"\224"},
    {K_S_F5,		"\225"},
    {K_S_F6,		"\226"},
    {K_S_F7,		"\227"},
    {K_S_F8,		"\228"},
    {K_S_F9,		"\229"},
    {K_S_F10,		"\332"},
    {K_S_F11,		"\333"},
    {K_S_F12,		"\334"},
    {K_BS,		"\010"},
    {K_INS,		"\315"},
    {K_DEL,		"\177"},
    {K_HOME,		"\036"},
    {K_END,		"\213"},
    {K_PAGEUP,		"\237"},
    {K_PAGEDOWN,	"\236"},
# endif	/* Acorn terminal */


# if defined(AMIGA) || defined(ALL_BUILTIN_TCAPS)
/*
 * Amiga console window, default for Amiga
 */
    {(int)KS_NAME,	"amiga"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
    {(int)KS_CL,	"\014"},
    {(int)KS_VI,	"\033[0 p"},
    {(int)KS_VE,	"\033[1 p"},
    {(int)KS_ME,	"\033[0m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MD,	"\033[1m"},
    {(int)KS_SE,	"\033[0m"},
    {(int)KS_SO,	"\033[33m"},
    {(int)KS_US,	"\033[4m"},
    {(int)KS_UE,	"\033[0m"},
    {(int)KS_CZH,	"\033[3m"},
    {(int)KS_CZR,	"\033[0m"},
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {K_UP,		"\233A"},
    {K_DOWN,		"\233B"},
    {K_LEFT,		"\233D"},
    {K_RIGHT,		"\233C"},
    {K_S_UP,		"\233T"},
    {K_S_DOWN,		"\233S"},
    {K_S_LEFT,		"\233 A"},
    {K_S_RIGHT,		"\233 @"},
    {K_S_TAB,		"\233Z"},
    {K_F1,		"\233\060~"},/* some compilers don't dig "\2330" */
    {K_F2,		"\233\061~"},
    {K_F3,		"\233\062~"},
    {K_F4,		"\233\063~"},
    {K_F5,		"\233\064~"},
    {K_F6,		"\233\065~"},
    {K_F7,		"\233\066~"},
    {K_F8,		"\233\067~"},
    {K_F9,		"\233\070~"},
    {K_F10,		"\233\071~"},
    {K_S_F1,		"\233\061\060~"},
    {K_S_F2,		"\233\061\061~"},
    {K_S_F3,		"\233\061\062~"},
    {K_S_F4,		"\233\061\063~"},
    {K_S_F5,		"\233\061\064~"},
    {K_S_F6,		"\233\061\065~"},
    {K_S_F7,		"\233\061\066~"},
    {K_S_F8,		"\233\061\067~"},
    {K_S_F9,		"\233\061\070~"},
    {K_S_F10,		"\233\061\071~"},
    {K_HELP,		"\233?~"},
    {K_INS,		"\233\064\060~"},	/* 101 key keyboard */
    {K_PAGEUP,		"\233\064\061~"},	/* 101 key keyboard */
    {K_PAGEDOWN,	"\233\064\062~"},	/* 101 key keyboard */
    {K_HOME,		"\233\064\064~"},	/* 101 key keyboard */
    {K_END,		"\233\064\065~"},	/* 101 key keyboard */

    {BT_EXTRA_KEYS,	""},
    {TERMCAP2KEY('#', '2'), "\233\065\064~"},	/* shifted home key */
    {TERMCAP2KEY('#', '3'), "\233\065\060~"},	/* shifted insert key */
    {TERMCAP2KEY('*', '7'), "\233\065\065~"},	/* shifted end key */
# endif

# if defined(__BEOS__) || defined(ALL_BUILTIN_TCAPS)
/*
 * almost standard ANSI terminal, default for bebox
 */
    {(int)KS_NAME,	"beos-ansi"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
#ifdef BEOS_PR_OR_BETTER
#  ifdef TERMINFO
    {(int)KS_CS,	"\033[%i%p1%d;%p2%dr"},
#  else
    {(int)KS_CS,	"\033[%i%d;%dr"},	/* scroll region */
#  endif
#endif
    {(int)KS_CL,	"\033[H\033[2J"},
#ifdef notyet
    {(int)KS_VI,	"[VI]"}, /* cursor invisible, VT320: CSI ? 25 l */
    {(int)KS_VE,	"[VE]"}, /* cursor visible, VT320: CSI ? 25 h */
#endif
    {(int)KS_ME,	"\033[m"},	/* normal mode */
    {(int)KS_MR,	"\033[7m"},	/* reverse */
    {(int)KS_MD,	"\033[1m"},	/* bold */
    {(int)KS_SO,	"\033[31m"},	/* standout mode: red */
    {(int)KS_SE,	"\033[m"},	/* standout end */
    {(int)KS_CZH,	"\033[35m"},	/* italic: purple */
    {(int)KS_CZR,	"\033[m"},	/* italic end */
    {(int)KS_US,	"\033[4m"},	/* underscore mode */
    {(int)KS_UE,	"\033[m"},	/* underscore end */
    {(int)KS_CCO,	"8"},		/* allow 8 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},/* set background color */
    {(int)KS_CAF,	"\033[3%p1%dm"},/* set foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},	/* set background color */
    {(int)KS_CAF,	"\033[3%dm"},	/* set foreground color */
#  endif
    {(int)KS_OP,	"\033[m"},	/* reset colors */
    {(int)KS_MS,	"y"},		/* safe to move cur in reverse mode */
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
    {(int)KS_SR,	"\033M"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
#if defined(BEOS_DR8)
    {(int)KS_DB,	""},		/* hack! see screen.c */
#endif

    {K_UP,		"\033[A"},
    {K_DOWN,		"\033[B"},
    {K_LEFT,		"\033[D"},
    {K_RIGHT,		"\033[C"},
# endif

# if defined(UNIX) || defined(ALL_BUILTIN_TCAPS) || defined(SOME_BUILTIN_TCAPS) || defined(__EMX__)
/*
 * standard ANSI terminal, default for unix
 */
    {(int)KS_NAME,	"ansi"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
    {(int)KS_CL,	"\033[H\033[2J"},
    {(int)KS_ME,	"\033[0m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
# endif

# if defined(MSDOS) || defined(ALL_BUILTIN_TCAPS) || defined(__EMX__)
/*
 * These codes are valid when nansi.sys or equivalent has been installed.
 * Function keys on a PC are preceded with a NUL. These are converted into
 * K_NUL '\316' in mch_inchar(), because we cannot handle NULs in key codes.
 * CTRL-arrow is used instead of SHIFT-arrow.
 */
#ifdef __EMX__
    {(int)KS_NAME,	"os2ansi"},
#else
    {(int)KS_NAME,	"pcansi"},
    {(int)KS_DL,	"\033[M"},
    {(int)KS_AL,	"\033[L"},
#endif
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CL,	"\033[2J"},
    {(int)KS_ME,	"\033[0m"},
    {(int)KS_MR,	"\033[5m"},	/* reverse: black on lightgrey */
    {(int)KS_MD,	"\033[1m"},	/* bold: white text */
    {(int)KS_SE,	"\033[0m"},	/* standout end */
    {(int)KS_SO,	"\033[31m"},	/* standout: white on blue */
    {(int)KS_CZH,	"\033[34;43m"},	/* italic mode: blue text on yellow */
    {(int)KS_CZR,	"\033[0m"},	/* italic mode end */
    {(int)KS_US,	"\033[36;41m"},	/* underscore mode: cyan text on red */
    {(int)KS_UE,	"\033[0m"},	/* underscore mode end */
    {(int)KS_CCO,	"8"},		/* allow 8 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},/* set background color */
    {(int)KS_CAF,	"\033[3%p1%dm"},/* set foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},	/* set background color */
    {(int)KS_CAF,	"\033[3%dm"},	/* set foreground color */
#  endif
    {(int)KS_OP,	"\033[0m"},	/* reset colors */
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {K_UP,		"\316H"},
    {K_DOWN,		"\316P"},
    {K_LEFT,		"\316K"},
    {K_RIGHT,		"\316M"},
    {K_S_LEFT,		"\316s"},
    {K_S_RIGHT,		"\316t"},
    {K_F1,		"\316;"},
    {K_F2,		"\316<"},
    {K_F3,		"\316="},
    {K_F4,		"\316>"},
    {K_F5,		"\316?"},
    {K_F6,		"\316@"},
    {K_F7,		"\316A"},
    {K_F8,		"\316B"},
    {K_F9,		"\316C"},
    {K_F10,		"\316D"},
    {K_F11,		"\316\205"},	/* guessed */
    {K_F12,		"\316\206"},	/* guessed */
    {K_S_F1,		"\316T"},
    {K_S_F2,		"\316U"},
    {K_S_F3,		"\316V"},
    {K_S_F4,		"\316W"},
    {K_S_F5,		"\316X"},
    {K_S_F6,		"\316Y"},
    {K_S_F7,		"\316Z"},
    {K_S_F8,		"\316["},
    {K_S_F9,		"\316\\"},
    {K_S_F10,		"\316]"},
    {K_S_F11,		"\316\207"},	/* guessed */
    {K_S_F12,		"\316\210"},	/* guessed */
    {K_INS,		"\316R"},
    {K_DEL,		"\316S"},
    {K_HOME,		"\316G"},
    {K_END,		"\316O"},
    {K_PAGEDOWN,	"\316Q"},
    {K_PAGEUP,		"\316I"},
# endif

# if defined(MSDOS)
/*
 * These codes are valid for the pc video.  The entries that start with ESC |
 * are translated into conio calls in os_msdos.c. Default for MSDOS.
 */
    {(int)KS_NAME,	"pcterm"},
    {(int)KS_CE,	"\033|K"},
    {(int)KS_AL,	"\033|L"},
    {(int)KS_DL,	"\033|M"},
#  ifdef TERMINFO
    {(int)KS_CS,	"\033|%i%p1%d;%p2%dr"},
#  else
    {(int)KS_CS,	"\033|%i%d;%dr"},
#  endif
    {(int)KS_CL,	"\033|J"},
    {(int)KS_ME,	"\033|0m"},	/* normal */
    {(int)KS_MR,	"\033|112m"},	/* reverse: black on lightgrey */
    {(int)KS_MD,	"\033|15m"},	/* bold: white text */
    {(int)KS_SE,	"\033|0m"},	/* standout end */
    {(int)KS_SO,	"\033|31m"},	/* standout: white on blue */
    {(int)KS_CZH,	"\033|225m"},	/* italic mode: blue text on yellow */
    {(int)KS_CZR,	"\033|0m"},	/* italic mode end */
    {(int)KS_US,	"\033|67m"},	/* underscore mode: cyan text on red */
    {(int)KS_UE,	"\033|0m"},	/* underscore mode end */
    {(int)KS_CCO,	"16"},		/* allow 16 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033|%p1%db"},	/* set background color */
    {(int)KS_CAF,	"\033|%p1%df"},	/* set foreground color */
#  else
    {(int)KS_CAB,	"\033|%db"},	/* set background color */
    {(int)KS_CAF,	"\033|%df"},	/* set foreground color */
#  endif
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033|%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033|%i%d;%dH"},
#  endif
#ifdef DJGPP
    {(int)KS_VB,	"\033|B"},	/* visual bell */
#endif
    {K_UP,		"\316H"},
    {K_DOWN,		"\316P"},
    {K_LEFT,		"\316K"},
    {K_RIGHT,		"\316M"},
    {K_S_LEFT,		"\316s"},
    {K_S_RIGHT,		"\316t"},
    {K_S_TAB,		"\316\017"},
    {K_F1,		"\316;"},
    {K_F2,		"\316<"},
    {K_F3,		"\316="},
    {K_F4,		"\316>"},
    {K_F5,		"\316?"},
    {K_F6,		"\316@"},
    {K_F7,		"\316A"},
    {K_F8,		"\316B"},
    {K_F9,		"\316C"},
    {K_F10,		"\316D"},
    {K_F11,		"\316\205"},
    {K_F12,		"\316\206"},
    {K_S_F1,		"\316T"},
    {K_S_F2,		"\316U"},
    {K_S_F3,		"\316V"},
    {K_S_F4,		"\316W"},
    {K_S_F5,		"\316X"},
    {K_S_F6,		"\316Y"},
    {K_S_F7,		"\316Z"},
    {K_S_F8,		"\316["},
    {K_S_F9,		"\316\\"},
    {K_S_F10,		"\316]"},
    {K_S_F11,		"\316\207"},
    {K_S_F12,		"\316\210"},
    {K_INS,		"\316R"},
    {K_DEL,		"\316S"},
    {K_HOME,		"\316G"},
    {K_END,		"\316O"},
    {K_PAGEDOWN,	"\316Q"},
    {K_PAGEUP,		"\316I"},
    {K_KPLUS,		"\316N"},
    {K_KMINUS,		"\316J"},
    {K_KMULTIPLY,	"\3167"},
# endif

# if defined(WIN32) || defined(ALL_BUILTIN_TCAPS) || defined(__EMX__)
/*
 * These codes are valid for the Win32 Console .  The entries that start with
 * ESC | are translated into console calls in os_win32.c.  The function keys
 * are also translated in os_win32.c.
 */
    {(int)KS_NAME,	"win32"},
    {(int)KS_CE,	"\033|K"},	/* clear to end of line */
    {(int)KS_AL,	"\033|L"},	/* add new blank line */
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033|%p1%dL"},	/* add number of new blank lines */
#  else
    {(int)KS_CAL,	"\033|%dL"},	/* add number of new blank lines */
#  endif
    {(int)KS_DL,	"\033|M"},	/* delete line */
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033|%p1%dM"},	/* delete number of lines */
#  else
    {(int)KS_CDL,	"\033|%dM"},	/* delete number of lines */
#  endif
    {(int)KS_CL,	"\033|J"},	/* clear screen */
    {(int)KS_CD,	"\033|j"},	/* clear to end of display */
    {(int)KS_VI,	"\033|v"},	/* cursor invisible */
    {(int)KS_VE,	"\033|V"},	/* cursor visible */

    {(int)KS_ME,	"\033|0m"},	/* normal */
    {(int)KS_MR,	"\033|112m"},	/* reverse: black on lightgray */
    {(int)KS_MD,	"\033|15m"},	/* bold: white on black */
#if 1
    {(int)KS_SO,	"\033|31m"},	/* standout: white on blue */
    {(int)KS_SE,	"\033|0m"},	/* standout end */
#else
    {(int)KS_SO,	"\033|F"},	/* standout: high intensity */
    {(int)KS_SE,	"\033|f"},	/* standout end */
#endif
    {(int)KS_CZH,	"\033|225m"},	/* italic: blue text on yellow */
    {(int)KS_CZR,	"\033|0m"},	/* italic end */
    {(int)KS_US,	"\033|67m"},	/* underscore: cyan text on red */
    {(int)KS_UE,	"\033|0m"},	/* underscore end */
    {(int)KS_CCO,	"16"},		/* allow 16 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033|%p1%db"},	/* set background color */
    {(int)KS_CAF,	"\033|%p1%df"},	/* set foreground color */
#  else
    {(int)KS_CAB,	"\033|%db"},	/* set background color */
    {(int)KS_CAF,	"\033|%df"},	/* set foreground color */
#  endif

    {(int)KS_MS,	"y"},		/* save to move cur in reverse mode */
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033|%i%p1%d;%p2%dH"},/* cursor motion */
#  else
    {(int)KS_CM,	"\033|%i%d;%dH"},/* cursor motion */
#  endif
    {(int)KS_VB,	"\033|B"},	/* visual bell */
    {(int)KS_TI,	"\033|S"},	/* put terminal in termcap mode */
    {(int)KS_TE,	"\033|E"},	/* out of termcap mode */
#  ifdef TERMINFO
    {(int)KS_CS,	"\033|%i%p1%d;%p2%dr"},/* scroll region */
#  else
    {(int)KS_CS,	"\033|%i%d;%dr"},/* scroll region */
#  endif

    {K_UP,		"\316H"},
    {K_DOWN,		"\316P"},
    {K_LEFT,		"\316K"},
    {K_RIGHT,		"\316M"},
    {K_S_UP,		"\316\304"},
    {K_S_DOWN,		"\316\317"},
    {K_S_LEFT,		"\316\311"},
    {K_S_RIGHT,		"\316\313"},
    {K_S_TAB,		"\316\017"},
    {K_F1,		"\316;"},
    {K_F2,		"\316<"},
    {K_F3,		"\316="},
    {K_F4,		"\316>"},
    {K_F5,		"\316?"},
    {K_F6,		"\316@"},
    {K_F7,		"\316A"},
    {K_F8,		"\316B"},
    {K_F9,		"\316C"},
    {K_F10,		"\316D"},
    {K_F11,		"\316\205"},
    {K_F12,		"\316\206"},
    {K_S_F1,		"\316T"},
    {K_S_F2,		"\316U"},
    {K_S_F3,		"\316V"},
    {K_S_F4,		"\316W"},
    {K_S_F5,		"\316X"},
    {K_S_F6,		"\316Y"},
    {K_S_F7,		"\316Z"},
    {K_S_F8,		"\316["},
    {K_S_F9,		"\316\\"},
    {K_S_F10,		"\316]"},
    {K_S_F11,		"\316\207"},
    {K_S_F12,		"\316\210"},
    {K_INS,		"\316R"},
    {K_DEL,		"\316S"},
    {K_HOME,		"\316G"},
    {K_END,		"\316O"},
    {K_PAGEDOWN,	"\316Q"},
    {K_PAGEUP,		"\316I"},
    {K_KPLUS,		"\316N"},
    {K_KMINUS,		"\316J"},
    {K_KMULTIPLY,	"\3167"},
# endif

# if defined(VMS) || defined(ALL_BUILTIN_TCAPS)
/*
 * VT320 is working as an ANSI terminal compatible DEC terminal.
 * (it covers VT1x0, VT2x0 and VT3x0 up to VT320 on VMS as well)
 * Note: K_F1...K_F5 are for internal use, should not be defined.
 * TODO:- rewrite ESC[ codes to CSI
 *      - keyboard languages (CSI ? 26 n)
 */
    {(int)KS_NAME,	"vt320"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
    {(int)KS_CL,	"\033[H\033[2J"},
    {(int)KS_ME,	"\033[0m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {K_UP,		"\033[A"},
    {K_DOWN,		"\033[B"},
    {K_RIGHT,		"\033[C"},
    {K_LEFT,		"\033[D"},
    {K_F6,		"\033[17~"},
    {K_F7,		"\033[18~"},
    {K_F8,		"\033[19~"},
    {K_F9,		"\033[20~"},
    {K_F10,		"\033[21~"},
/*  {K_F11,		"\033[23~"},   (ESC) should not define, sometimes does not work */
    {K_F12,		"\033[24~"},
    {K_F13,		"\033[25~"},
    {K_F14,		"\033[26~"},
    {K_F15,		"\033[28~"},	/* Help */
    {K_F16,		"\033[29~"},	/* Select */
    {K_F17,		"\033[31~"},
    {K_F18,		"\033[32~"},
    {K_F19,		"\033[33~"},
    {K_F20,		"\033[34~"},
    {K_INS,		"\033[2~"},
    {K_DEL,		"\033[3~"},
    {K_HOME,		"\033[1~"},
    {K_END,		"\033[4~"},
    {K_PAGEUP,		"\033[5~"},
    {K_PAGEDOWN,	"\033[6~"},
    {K_KPLUS,		"\033Ok"},	/* keypad plus */
    {K_KMINUS,		"\033Om"},	/* keypad minus */
    {K_KDIVIDE,		"\033Oo"},	/* keypad / */
    {K_KMULTIPLY,	"\033Oj"},	/* keypad * */
    {K_KENTER,		"\033OM"},	/* keypad Enter */
    {K_BS,		"\x7f"},	/* for some reason 0177 doesn't work */
# endif

# if defined(ALL_BUILTIN_TCAPS) || defined(__MINT__)
/*
 * Ordinary vt52
 */
    {(int)KS_NAME,	"vt52"},
    {(int)KS_CE,	"\033K"},
    {(int)KS_CD,	"\033J"},
    {(int)KS_CM,	"\033Y%+ %+ "},
    {(int)KS_LE,	"\010"},
#  ifdef __MINT__
    {(int)KS_AL,	"\033L"},
    {(int)KS_DL,	"\033M"},
    {(int)KS_CL,	"\033E"},
    {(int)KS_SR,	"\033I"},
    {(int)KS_VE,	"\033e"},
    {(int)KS_VI,	"\033f"},
    {(int)KS_SO,	"\033p"},
    {(int)KS_SE,	"\033q"},
    {K_UP,		"\033A"},
    {K_DOWN,		"\033B"},
    {K_LEFT,		"\033D"},
    {K_RIGHT,		"\033C"},
    {K_S_UP,		"\033a"},
    {K_S_DOWN,		"\033b"},
    {K_S_LEFT,		"\033d"},
    {K_S_RIGHT,		"\033c"},
    {K_F1,		"\033P"},
    {K_F2,		"\033Q"},
    {K_F3,		"\033R"},
    {K_F4,		"\033S"},
    {K_F5,		"\033T"},
    {K_F6,		"\033U"},
    {K_F7,		"\033V"},
    {K_F8,		"\033W"},
    {K_F9,		"\033X"},
    {K_F10,		"\033Y"},
    {K_S_F1,		"\033p"},
    {K_S_F2,		"\033q"},
    {K_S_F3,		"\033r"},
    {K_S_F4,		"\033s"},
    {K_S_F5,		"\033t"},
    {K_S_F6,		"\033u"},
    {K_S_F7,		"\033v"},
    {K_S_F8,		"\033w"},
    {K_S_F9,		"\033x"},
    {K_S_F10,		"\033y"},
    {K_INS,		"\033I"},
    {K_HOME,		"\033E"},
    {K_PAGEDOWN,	"\033b"},
    {K_PAGEUP,		"\033a"},
#  else
    {(int)KS_AL,	"\033T"},
    {(int)KS_DL,	"\033U"},
    {(int)KS_CL,	"\033H\033J"},
    {(int)KS_ME,	"\033SO"},
    {(int)KS_MR,	"\033S2"},
    {(int)KS_MS,	"y"},
#  endif
# endif

# if defined(UNIX) || defined(ALL_BUILTIN_TCAPS) || defined(SOME_BUILTIN_TCAPS) || defined(__EMX__)
/*
 * The xterm termcap is missing F14 and F15, because they send the same
 * codes as the undo and help key, although they don't work on all keyboards.
 */
    {(int)KS_NAME,	"xterm"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CS,	"\033[%i%p1%d;%p2%dr"},
#  else
    {(int)KS_CS,	"\033[%i%d;%dr"},
#  endif
    {(int)KS_CL,	"\033[H\033[2J"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_ME,	"\033[m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MD,	"\033[1m"},
    {(int)KS_UE,	"\033[m"},
    {(int)KS_US,	"\033[4m"},
    {(int)KS_MS,	"y"},
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
    {(int)KS_SR,	"\033M"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {(int)KS_KS,	"\033[?1h\033="},
    {(int)KS_KE,	"\033[?1l\033>"},
#  ifdef SAVE_XTERM_SCREEN
    {(int)KS_TI,	"\0337\033[?47h"},
    {(int)KS_TE,	"\033[2J\033[?47l\0338"},
#  endif
    {(int)KS_CIS,	"\033]1;"},
    {(int)KS_CIE,	"\007"},
    {(int)KS_TS,	"\033]2;"},
    {(int)KS_FS,	"\007"},
#  ifdef TERMINFO
    {(int)KS_CWS,	"\033[8;%p1%d;%p2%dt"},	/* height, width in chars */
    {(int)KS_CWP,	"\033[3;%p1%d;%p2%dt"}, /* X, Y in pixels */
#  else
    {(int)KS_CWS,	"\033[8;%d;%dt"},	/* height, width in chars */
    {(int)KS_CWP,	"\033[3;%d;%dt"},	/* X, Y in pixels */
#  endif
    {(int)KS_CRV,	"\033[>c"},
    {K_UP,		"\033OA"},
    {K_DOWN,		"\033OB"},
    {K_RIGHT,		"\033OC"},
    {K_LEFT,		"\033OD"},
    {K_S_UP,		"\033O2A"},
    {K_S_DOWN,		"\033O2B"},
    {K_S_RIGHT,		"\033O2C"},
    {K_S_LEFT,		"\033O2D"},
    /* An extra set of function keys for vt100 mode */
    {K_XF1,		"\033OP"},
    {K_XF2,		"\033OQ"},
    {K_XF3,		"\033OR"},
    {K_XF4,		"\033OS"},
    {K_F1,		"\033[11~"},
    {K_F2,		"\033[12~"},
    {K_F3,		"\033[13~"},
    {K_F4,		"\033[14~"},
    {K_F5,		"\033[15~"},
    {K_F6,		"\033[17~"},
    {K_F7,		"\033[18~"},
    {K_F8,		"\033[19~"},
    {K_F9,		"\033[20~"},
    {K_F10,		"\033[21~"},
    {K_F11,		"\033[23~"},
    {K_F12,		"\033[24~"},
    {K_S_XF1,		"\033O2P"},
    {K_S_XF2,		"\033O2Q"},
    {K_S_XF3,		"\033O2R"},
    {K_S_XF4,		"\033O2S"},
    {K_S_F1,		"\033[11;2~"},
    {K_S_F2,		"\033[12;2~"},
    {K_S_F3,		"\033[13;2~"},
    {K_S_F4,		"\033[14;2~"},
    {K_S_F5,		"\033[15;2~"},
    {K_S_F6,		"\033[17;2~"},
    {K_S_F7,		"\033[18;2~"},
    {K_S_F8,		"\033[19;2~"},
    {K_S_F9,		"\033[20;2~"},
    {K_S_F10,		"\033[21;2~"},
    {K_S_F11,		"\033[23;2~"},
    {K_S_F12,		"\033[24;2~"},
    {K_HELP,		"\033[28~"},
    {K_UNDO,		"\033[26~"},
    {K_INS,		"\033[2~"},
    {K_HOME,		"\033[7~"},
    {K_S_HOME,		"\033O2H"},
    {K_KHOME,		"\033[1~"},
    {K_XHOME,		"\033OH"},	/* alternate Home */
    {K_END,		"\033[8~"},
    {K_S_END,		"\033O2F"},
    {K_KEND,		"\033[4~"},
    {K_XEND,		"\033OF"},	/* alternate End */
    {K_PAGEUP,		"\033[5~"},
    {K_PAGEDOWN,	"\033[6~"},
    {K_KPLUS,		"\033Ok"},	/* keypad plus */
    {K_KMINUS,		"\033Om"},	/* keypad minus */
    {K_KDIVIDE,		"\033Oo"},	/* keypad / */
    {K_KMULTIPLY,	"\033Oj"},	/* keypad * */
    {K_KENTER,		"\033OM"},	/* keypad Enter */
    {K_KDEL,		"\033[3~"},	/* keypad Del */

    {BT_EXTRA_KEYS,   ""},
    {TERMCAP2KEY('k', '0'),	"\033[10~"},	/* F0 */
    {TERMCAP2KEY('F', '3'),	"\033[25~"},	/* F13 */
    {TERMCAP2KEY('F', '6'),	"\033[29~"},	/* F16 */
    {TERMCAP2KEY('F', '7'),	"\033[31~"},	/* F17 */
    {TERMCAP2KEY('F', '8'),	"\033[32~"},	/* F18 */
    {TERMCAP2KEY('F', '9'),	"\033[33~"},	/* F19 */
    {TERMCAP2KEY('F', 'A'),	"\033[34~"},	/* F20 */
# endif

# if defined(UNIX) || defined(ALL_BUILTIN_TCAPS)
/*
 * iris-ansi for Silicon Graphics machines.
 */
    {(int)KS_NAME,	"iris-ansi"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
#if 0	/* The scroll region is not working as Vim expects. */
#  ifdef TERMINFO
    {(int)KS_CS,	"\033[%i%p1%d;%p2%dr"},
#  else
    {(int)KS_CS,	"\033[%i%d;%dr"},
#  endif
#endif
    {(int)KS_CL,	"\033[H\033[2J"},
    {(int)KS_VE,	"\033[9/y\033[12/y"},
    {(int)KS_VS,	"\033[10/y\033[=1h\033[=2l"},
    {(int)KS_TI,	"\033[=6h"},
    {(int)KS_TE,	"\033[=6l"},
    {(int)KS_SE,	"\033[m"},
    {(int)KS_SO,	"\033[1;7m"},
    {(int)KS_ME,	"\033[m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MD,	"\033[1m"},
    {(int)KS_UE,	"\033[m"},
    {(int)KS_CCO,	"8"},			/* allow 8 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},	/* set background color */
    {(int)KS_CAF,	"\033[3%p1%dm"},	/* set foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},		/* set background color */
    {(int)KS_CAF,	"\033[3%dm"},		/* set foreground color */
#  endif
    {(int)KS_US,	"\033[4m"},
    {(int)KS_MS,	"y"},	/* does this really work? */
    {(int)KS_LE,	"\010"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
    {(int)KS_SR,	"\033M"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {(int)KS_CIS,	"\033P3.y"},
    {(int)KS_CIE,	"\234"},
    {(int)KS_TS,	"\033P1.y"},
    {(int)KS_FS,	"\234"},
#  ifdef TERMINFO
    {(int)KS_CWS,	"\033[203;%p1%d;%p2%d/y"},
#  else
    {(int)KS_CWS,	"\033[203;%d;%d/y"},
#  endif
    {K_UP,		"\033[A"},
    {K_DOWN,		"\033[B"},
    {K_LEFT,		"\033[D"},
    {K_RIGHT,		"\033[C"},
    {K_S_UP,		"\033[161q"},
    {K_S_DOWN,		"\033[164q"},
    {K_S_LEFT,		"\033[158q"},
    {K_S_RIGHT,		"\033[167q"},
    {K_F1,		"\033[001q"},
    {K_F2,		"\033[002q"},
    {K_F3,		"\033[003q"},
    {K_F4,		"\033[004q"},
    {K_F5,		"\033[005q"},
    {K_F6,		"\033[006q"},
    {K_F7,		"\033[007q"},
    {K_F8,		"\033[008q"},
    {K_F9,		"\033[009q"},
    {K_F10,		"\033[010q"},
    {K_F11,		"\033[011q"},
    {K_F12,		"\033[012q"},
    {K_S_F1,		"\033[013q"},
    {K_S_F2,		"\033[014q"},
    {K_S_F3,		"\033[015q"},
    {K_S_F4,		"\033[016q"},
    {K_S_F5,		"\033[017q"},
    {K_S_F6,		"\033[018q"},
    {K_S_F7,		"\033[019q"},
    {K_S_F8,		"\033[020q"},
    {K_S_F9,		"\033[021q"},
    {K_S_F10,		"\033[022q"},
    {K_S_F11,		"\033[023q"},
    {K_S_F12,		"\033[024q"},
    {K_INS,		"\033[139q"},
    {K_HOME,		"\033[H"},
    {K_END,		"\033[146q"},
    {K_PAGEUP,		"\033[150q"},
    {K_PAGEDOWN,	"\033[154q"},
# endif

# if defined(DEBUG) || defined(ALL_BUILTIN_TCAPS)
/*
 * for debugging
 */
    {(int)KS_NAME,	"debug"},
    {(int)KS_CE,	"[CE]"},
    {(int)KS_CD,	"[CD]"},
    {(int)KS_AL,	"[AL]"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"[CAL%p1%d]"},
#  else
    {(int)KS_CAL,	"[CAL%d]"},
#  endif
    {(int)KS_DL,	"[DL]"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"[CDL%p1%d]"},
#  else
    {(int)KS_CDL,	"[CDL%d]"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CS,	"[%dCS%p1%d]"},
#  else
    {(int)KS_CS,	"[%dCS%d]"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CAB,	"[CAB%p1%d]"},
    {(int)KS_CAF,	"[CAF%p1%d]"},
    {(int)KS_CSB,	"[CSB%p1%d]"},
    {(int)KS_CSF,	"[CSF%p1%d]"},
#  else
    {(int)KS_CAB,	"[CAB%d]"},
    {(int)KS_CAF,	"[CAF%d]"},
    {(int)KS_CSB,	"[CSB%d]"},
    {(int)KS_CSF,	"[CSF%d]"},
#  endif
    {(int)KS_OP,	"[OP]"},
    {(int)KS_LE,	"[LE]"},
    {(int)KS_CL,	"[CL]"},
    {(int)KS_VI,	"[VI]"},
    {(int)KS_VE,	"[VE]"},
    {(int)KS_VS,	"[VS]"},
    {(int)KS_ME,	"[ME]"},
    {(int)KS_MR,	"[MR]"},
    {(int)KS_MB,	"[MB]"},
    {(int)KS_MD,	"[MD]"},
    {(int)KS_SE,	"[SE]"},
    {(int)KS_SO,	"[SO]"},
    {(int)KS_UE,	"[UE]"},
    {(int)KS_US,	"[US]"},
    {(int)KS_MS,	"[MS]"},
#  ifdef TERMINFO
    {(int)KS_CM,	"[%p1%dCM%p2%d]"},
#  else
    {(int)KS_CM,	"[%dCM%d]"},
#  endif
    {(int)KS_SR,	"[SR]"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"[CRI%p1%d]"},
#  else
    {(int)KS_CRI,	"[CRI%d]"},
#  endif
    {(int)KS_VB,	"[VB]"},
    {(int)KS_KS,	"[KS]"},
    {(int)KS_KE,	"[KE]"},
    {(int)KS_TI,	"[TI]"},
    {(int)KS_TE,	"[TE]"},
    {(int)KS_CIS,	"[CIS]"},
    {(int)KS_CIE,	"[CIE]"},
    {(int)KS_TS,	"[TS]"},
    {(int)KS_FS,	"[FS]"},
#  ifdef TERMINFO
    {(int)KS_CWS,	"[%p1%dCWS%p2%d]"},
    {(int)KS_CWP,	"[%p1%dCWP%p2%d]"},
#  else
    {(int)KS_CWS,	"[%dCWS%d]"},
    {(int)KS_CWP,	"[%dCWP%d]"},
#  endif
    {(int)KS_CRV,	"[CRV]"},
    {K_UP,		"[KU]"},
    {K_DOWN,		"[KD]"},
    {K_LEFT,		"[KL]"},
    {K_RIGHT,		"[KR]"},
    {K_S_UP,		"[S-KU]"},
    {K_S_DOWN,		"[S-KD]"},
    {K_S_LEFT,		"[S-KL]"},
    {K_S_RIGHT,		"[S-KR]"},
    {K_F1,		"[F1]"},
    {K_XF1,		"[xF1]"},
    {K_F2,		"[F2]"},
    {K_XF2,		"[xF2]"},
    {K_F3,		"[F3]"},
    {K_XF3,		"[xF3]"},
    {K_F4,		"[F4]"},
    {K_XF4,		"[xF4]"},
    {K_F5,		"[F5]"},
    {K_F6,		"[F6]"},
    {K_F7,		"[F7]"},
    {K_F8,		"[F8]"},
    {K_F9,		"[F9]"},
    {K_F10,		"[F10]"},
    {K_F11,		"[F11]"},
    {K_F12,		"[F12]"},
    {K_S_F1,		"[S-F1]"},
    {K_S_XF1,		"[S-xF1]"},
    {K_S_F2,		"[S-F2]"},
    {K_S_XF2,		"[S-xF2]"},
    {K_S_F3,		"[S-F3]"},
    {K_S_XF3,		"[S-xF3]"},
    {K_S_F4,		"[S-F4]"},
    {K_S_XF4,		"[S-xF4]"},
    {K_S_F5,		"[S-F5]"},
    {K_S_F6,		"[S-F6]"},
    {K_S_F7,		"[S-F7]"},
    {K_S_F8,		"[S-F8]"},
    {K_S_F9,		"[S-F9]"},
    {K_S_F10,		"[S-F10]"},
    {K_S_F11,		"[S-F11]"},
    {K_S_F12,		"[S-F12]"},
    {K_HELP,		"[HELP]"},
    {K_UNDO,		"[UNDO]"},
    {K_BS,		"[BS]"},
    {K_INS,		"[INS]"},
    {K_KINS,		"[KINS]"},
    {K_DEL,		"[DEL]"},
    {K_KDEL,		"[KDEL]"},
    {K_HOME,		"[HOME]"},
    {K_KHOME,		"[KHOME]"},
    {K_XHOME,		"[XHOME]"},
    {K_END,		"[END]"},
    {K_KEND,		"[KEND]"},
    {K_XEND,		"[XEND]"},
    {K_PAGEUP,		"[PAGEUP]"},
    {K_PAGEDOWN,	"[PAGEDOWN]"},
    {K_KPAGEUP,		"[KPAGEUP]"},
    {K_KPAGEDOWN,	"[KPAGEDOWN]"},
    {K_MOUSE,		"[MOUSE]"},
    {K_KPLUS,		"[KPLUS]"},
    {K_KMINUS,		"[KMINUS]"},
    {K_KDIVIDE,		"[KDIVIDE]"},
    {K_KMULTIPLY,	"[KMULTIPLY]"},
    {K_KENTER,		"[KENTER]"},
# endif

#endif /* NO_BUILTIN_TCAPS */

/*
 * The most minimal terminal: only clear screen and cursor positioning
 * Always included.
 */
    {(int)KS_NAME,	"dumb"},
    {(int)KS_CL,	"\014"},
#ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#endif

/*
 * end marker
 */
    {(int)KS_NAME,	NULL}

};	/* end of builtin_termcaps */

/*
 * DEFAULT_TERM is used, when no terminal is specified with -T option or $TERM.
 */
#ifdef RISCOS
# define DEFAULT_TERM	(char_u *)"riscos"
#endif

#ifdef AMIGA
# define DEFAULT_TERM	(char_u *)"amiga"
#endif

#ifdef MSWIN
# define DEFAULT_TERM	(char_u *)"win32"
#endif

#ifdef MSDOS
# define DEFAULT_TERM	(char_u *)"pcterm"
#endif

#if defined(UNIX) && !defined(__MINT__)
# define DEFAULT_TERM	(char_u *)"ansi"
#endif

#ifdef __MINT__
# define DEFAULT_TERM	(char_u *)"vt52"
#endif

#ifdef __EMX__
# define DEFAULT_TERM	(char_u *)"os2ansi"
#endif

#ifdef VMS
# define DEFAULT_TERM	(char_u *)"vt320"
#endif

#ifdef __BEOS__
# undef DEFAULT_TERM
# define DEFAULT_TERM	(char_u *)"beos-ansi"
#endif

#ifdef macintosh
# define DEFAULT_TERM	(char_u *)"mac-ansi"
#endif

/*
 * Term_strings contains currently used terminal output strings.
 * It is initialized with the default values by parse_builtin_tcap().
 * The values can be changed by setting the option with the same name.
 */
char_u *(term_strings[(int)KS_LAST + 1]);

static int	need_gather = FALSE;		/* need to fill termleader[] */
static char_u	termleader[256 + 1];		/* for check_termcode() */

    static struct builtin_term *
find_builtin_term(term)
    char_u	*term;
{
    struct builtin_term *p;

    p = builtin_termcaps;
    while (p->bt_string != NULL)
    {
	if (p->bt_entry == (int)KS_NAME)
	{
#ifdef UNIX
	    if (STRCMP(p->bt_string, "iris-ansi") == 0 && vim_is_iris(term))
		return p;
	    else if (STRCMP(p->bt_string, "xterm") == 0 && vim_is_xterm(term))
		return p;
	    else
#endif
#ifdef VMS
		if (STRCMP(p->bt_string, "vt320") == 0 && vim_is_vt300(term))
		    return p;
		else
#endif
		  if (STRCMP(term, p->bt_string) == 0)
		    return p;
	}
	++p;
    }
    return p;
}

/*
 * Parsing of the builtin termcap entries.
 * Caller should check if 'name' is a valid builtin term.
 * The terminal's name is not set, as this is already done in termcapinit().
 */
    static void
parse_builtin_tcap(term)
    char_u  *term;
{
    struct builtin_term	    *p;
    char_u		    name[2];
    int			    term_8bit;

    p = find_builtin_term(term);
    term_8bit = term_is_8bit(term);

    for (++p; p->bt_entry != (int)KS_NAME && p->bt_entry != BT_EXTRA_KEYS; ++p)
    {
	if ((int)p->bt_entry < 0x100)	/* KS_xx entry */
	{
	    /* Only set the value if it wasn't set yet. */
	    if (term_strings[p->bt_entry] == NULL
				 || term_strings[p->bt_entry] == empty_option)
	    {
		/* 8bit terminal: use CSI instead of <Esc>[ */
		if (term_8bit && term_7to8bit((char_u *)p->bt_string) != 0)
		{
		    char_u  *s, *t;

		    s = vim_strsave((char_u *)p->bt_string);
		    if (s != NULL)
		    {
			for (t = s; *t; ++t)
			    if (term_7to8bit(t))
			    {
				*t = term_7to8bit(t);
				STRCPY(t + 1, t + 2);
			    }
			term_strings[p->bt_entry] = s;
			set_term_option_alloced(&term_strings[p->bt_entry]);
		    }
		}
		else
		    term_strings[p->bt_entry] = (char_u *)p->bt_string;
	    }
	}
	else
	{
	    name[0] = KEY2TERMCAP0((int)p->bt_entry);
	    name[1] = KEY2TERMCAP1((int)p->bt_entry);
	    if (find_termcode(name) == NULL)
		add_termcode(name, (char_u *)p->bt_string, term_8bit);
	}
    }
}

/*
 * Set terminal options for terminal "term".
 * Return OK if terminal 'term' was found in a termcap, FAIL otherwise.
 *
 * While doing this, until ttest(), some options may be NULL, be careful.
 */
    int
set_termname(term)
    char_u *term;
{
    struct builtin_term *termp;
#ifdef HAVE_TGETENT
    int		builtin_first = p_tbi;
    int		try;
    int		termcap_cleared = FALSE;
    static char	nr_colors[15];	    /* string for number of colors */
#endif
    int		width = 0, height = 0;
    char_u	*error_msg = NULL;
    char_u	*bs_p, *del_p;

    if (term_is_builtin(term))
    {
	term += 8;
#ifdef HAVE_TGETENT
	builtin_first = 1;
#endif
    }

/*
 * If HAVE_TGETENT is not defined, only the builtin termcap is used, otherwise:
 *   If builtin_first is TRUE:
 *     0. try builtin termcap
 *     1. try external termcap
 *     2. if both fail default to a builtin terminal
 *   If builtin_first is FALSE:
 *     1. try external termcap
 *     2. try builtin termcap, if both fail default to a builtin terminal
 */
#ifdef HAVE_TGETENT
    for (try = builtin_first ? 0 : 1; try < 3; ++try)
    {
	/*
	 * Use external termcap
	 */
	if (try == 1)
	{
	    char_u	    *p;
	    static char_u   tstrbuf[TBUFSZ];
	    int		    i;
	    char_u	    tbuf[TBUFSZ];
	    char_u	    *tp;
	    static char	    *(key_names[]) =
			    {
			    "ku", "kd", "kr",	/* "kl" is a special case */
# ifdef ARCHIE
			    "su", "sd",		/* Termcap code made up! */
# endif
			    "#4", "%i",
			    "k1", "k2", "k3", "k4", "k5", "k6",
			    "k7", "k8", "k9", "k;", "F1", "F2",
			    "%1", "&8", "kb", "kI", "kD", "kh",
			    "@7", "kP", "kN", "K1", "K3", "K4", "K5",
			    NULL
			    };
	    static struct {
			    enum SpecialKey dest; /* index in term_strings[] */
			    char *name;		  /* termcap name for string */
			  } string_names[] =
			    {	{KS_CE, "ce"}, {KS_AL, "al"}, {KS_CAL,"AL"},
				{KS_DL, "dl"}, {KS_CDL,"DL"}, {KS_CS, "cs"},
				{KS_CL, "cl"}, {KS_CD, "cd"},
				{KS_VI, "vi"}, {KS_VE, "ve"}, {KS_MB, "mb"},
				{KS_VS, "vs"}, {KS_ME, "me"}, {KS_MR, "mr"},
				{KS_MD, "md"}, {KS_SE, "se"}, {KS_SO, "so"},
				{KS_CZH,"ZH"}, {KS_CZR,"ZR"}, {KS_UE, "ue"},
				{KS_US, "us"}, {KS_CM, "cm"}, {KS_SR, "sr"},
				{KS_CRI,"RI"}, {KS_VB, "vb"}, {KS_KS, "ks"},
				{KS_KE, "ke"}, {KS_TI, "ti"}, {KS_TE, "te"},
				{KS_BC, "bc"}, {KS_CSB,"Sb"}, {KS_CSF,"Sf"},
				{KS_CAB,"AB"}, {KS_CAF,"AF"}, {KS_LE, "le"},
				{KS_ND, "nd"}, {KS_OP, "op"}, {KS_CRV, "RV"},
				{KS_CIS, "IS"}, {KS_CIE, "IE"},
				{KS_TS, "ts"}, {KS_FS, "fs"},
				{KS_CWP, "WP"}, {KS_CWS, "WS"},
				{(enum SpecialKey)0, NULL}
			    };

	    /*
	     * If the external termcap does not have a matching entry, try the
	     * builtin ones.
	     */
	    if ((error_msg = tgetent_error(tbuf, term)) == NULL)
	    {
		tp = tstrbuf;
		if (!termcap_cleared)
		{
		    clear_termoptions();	/* clear old options */
		    termcap_cleared = TRUE;
		}

	    /* get output strings */
		for (i = 0; string_names[i].name != NULL; ++i)
		{
		    if (term_str(string_names[i].dest) == NULL
			    || term_str(string_names[i].dest) == empty_option)
			term_str(string_names[i].dest) =
					   TGETSTR(string_names[i].name, &tp);
		}

		if ((T_MS == NULL || T_MS == empty_option) && tgetflag("ms"))
		    T_MS = (char_u *)"y";
		if ((T_XS == NULL || T_XS == empty_option) && tgetflag("xs"))
		    T_XS = (char_u *)"y";
		if ((T_DB == NULL || T_DB == empty_option) && tgetflag("db"))
		    T_DB = (char_u *)"y";
		if ((T_DA == NULL || T_DA == empty_option) && tgetflag("da"))
		    T_DA = (char_u *)"y";


	    /* get key codes */

		for (i = 0; key_names[i] != NULL; ++i)
		{
		    if (find_termcode((char_u *)key_names[i]) == NULL)
			add_termcode((char_u *)key_names[i],
					   TGETSTR(key_names[i], &tp), FALSE);
		}

		    /* if cursor-left == backspace, ignore it (televideo 925) */
		if (find_termcode((char_u *)"kl") == NULL)
		{
		    p = TGETSTR("kl", &tp);
		    if (p != NULL && *p != Ctrl('H'))
			add_termcode((char_u *)"kl", p, FALSE);
		}

		if (height == 0)
		    height = tgetnum("li");
		if (width == 0)
		    width = tgetnum("co");

		/*
		 * Get number of colors.  If non-zero, store as string in
		 * nr_colors[].
		 */
		i = tgetnum("Co");
		if (i > 0)
		{
		    sprintf(nr_colors, "%d", i);
		    T_CCO = (char_u *)nr_colors;
		}

# ifndef hpux
		BC = (char *)TGETSTR("bc", &tp);
		UP = (char *)TGETSTR("up", &tp);
		p = TGETSTR("pc", &tp);
		if (p)
		    PC = *p;
# endif /* hpux */
	    }
	}
	else	    /* try == 0 || try == 2 */
#endif /* HAVE_TGETENT */
	/*
	 * Use builtin termcap
	 */
	{
#ifdef HAVE_TGETENT
	    /*
	     * If builtin termcap was already used, there is no need to search
	     * for the builtin termcap again, quit now.
	     */
	    if (try == 2 && builtin_first && termcap_cleared)
		break;
#endif
	    /*
	     * search for 'term' in builtin_termcaps[]
	     */
	    termp = find_builtin_term(term);
	    if (termp->bt_string == NULL)	/* did not find it */
	    {
#ifdef HAVE_TGETENT
		/*
		 * If try == 0, first try the external termcap. If that is not
		 * found we'll get back here with try == 2.
		 * If termcap_cleared is set we used the external termcap,
		 * don't complain about not finding the term in the builtin
		 * termcap.
		 */
		if (try == 0)			/* try external one */
		    continue;
		if (termcap_cleared)		/* found in external termcap */
		    break;
#endif

#ifdef USE_GUI_GTK
		/*
		 * Skip the error message when the GUI is about to start.  Can
		 * happen when using GTK version of gvim when $term is
		 * invalid.
		 */
		if (!gui.starting)
		{
#endif
		mch_errmsg("\r\n");
		if (error_msg != NULL)
		{
		    mch_errmsg((char *)error_msg);
		    mch_errmsg("\r\n");
		}
		mch_errmsg("'");
		mch_errmsg((char *)term);
		mch_errmsg("' not known. Available builtin terminals are:\r\n");
		for (termp = &(builtin_termcaps[0]); termp->bt_string != NULL;
								      ++termp)
		{
		    if (termp->bt_entry == (int)KS_NAME)
		    {
#ifdef HAVE_TGETENT
			mch_errmsg("    builtin_");
#else
			mch_errmsg("    ");
#endif
			mch_errmsg(termp->bt_string);
			mch_errmsg("\r\n");
		    }
		}
#ifdef USE_GUI_GTK
		}
#endif
		/* when user typed :set term=xxx, quit here */
		if (starting != NO_SCREEN)
		{
		    screen_start();	/* don't know where cursor is now */
		    wait_return(TRUE);
		    return FAIL;
		}
		term = DEFAULT_TERM;
#ifdef USE_GUI_GTK
		if (!gui.starting)
		{
#endif
		mch_errmsg("defaulting to '");
		mch_errmsg((char *)term);
		mch_errmsg("'\r\n");
		screen_start();		/* don't know where cursor is now */
		out_flush();
		ui_delay(2000L, TRUE);
#ifdef USE_GUI_GTK
		}
#endif
		set_string_option_direct((char_u *)"term", -1, term, TRUE);
#ifdef USE_GUI_GTK
		if (!gui.starting)
#endif
		    mch_display_error();
	    }
	    out_flush();
#ifdef HAVE_TGETENT
	    if (!termcap_cleared)
	    {
#endif
		clear_termoptions();	    /* clear old options */
#ifdef HAVE_TGETENT
		termcap_cleared = TRUE;
	    }
#endif
	    parse_builtin_tcap(term);
#ifdef USE_GUI
	    if (term_is_gui(term))
	    {
		out_flush();
		gui_init();
		/* If starting the GUI failed, don't do any of the other
		 * things for this terminal */
		if (!gui.in_use)
		    return FAIL;
#ifdef HAVE_TGETENT
		break;		/* don't try using external termcap */
#endif
	    }
#endif /* USE_GUI */
	}
#ifdef HAVE_TGETENT
    }
#endif

/*
 * special: There is no info in the termcap about whether the cursor
 * positioning is relative to the start of the screen or to the start of the
 * scrolling region.  We just guess here. Only msdos pcterm is known to do it
 * relative.
 */
    if (STRCMP(term, "pcterm") == 0)
	T_CCS = (char_u *)"yes";
    else
	T_CCS = empty_option;

#ifdef UNIX
/*
 * Any "stty" settings override the default for t_kb from the termcap.
 * This is in os_unix.c, because it depends a lot on the version of unix that
 * is being used.
 * Don't do this when the GUI is active, it uses "t_kb" and "t_kD" directly.
 */
#ifdef USE_GUI
    if (!gui.in_use)
#endif
	get_stty();
#endif

/*
 * If the termcap has no entry for 'bs' and/or 'del' and the ioctl() also
 * didn't work, use the default CTRL-H
 * The default for t_kD is DEL, unless t_kb is DEL.
 * The vim_strsave'd strings are probably lost forever, well it's only two
 * bytes.  Don't do this when the GUI is active, it uses "t_kb" and "t_kD"
 * directly.
 */
#ifdef USE_GUI
    if (!gui.in_use)
#endif
    {
	bs_p = find_termcode((char_u *)"kb");
	del_p = find_termcode((char_u *)"kD");
	if (bs_p == NULL || *bs_p == NUL)
	    add_termcode((char_u *)"kb", (bs_p = (char_u *)"\010"), FALSE);
	if ((del_p == NULL || *del_p == NUL) &&
					    (bs_p == NULL || *bs_p != '\177'))
	    add_termcode((char_u *)"kD", (char_u *)"\177", FALSE);
    }

#ifdef USE_MOUSE
# if defined(UNIX) || defined(VMS)
    /*
     * For Unix, set the 'ttymouse' option to the type of mouse to be used.
     * The termcode for the mouse is added as a side effect in option.c.
     */
    {
	char_u	*p;

	p = (char_u *)"";
#  ifdef NETTERM_MOUSE
	p = (char_u *)"netterm";
#  endif
#  ifdef DEC_MOUSE
	p = (char_u *)"dec";
#  endif
#  ifdef XTERM_MOUSE
#   ifdef USE_CLIPBOARD
#    ifdef USE_GUI
	if (!term_is_gui(term))
#    endif
	    clip_init(FALSE);
#   endif
	if (vim_is_xterm(term))
	{
	    if (use_xterm_mouse())
		p = NULL;	/* keep existing value, might be "xterm2" */
	    else
		p = (char_u *)"xterm";
#    ifdef XTERM_CLIP
	    setup_xterm_clip();
#    endif
	}
#  endif
	if (p != NULL)
	    set_option_value((char_u *)"ttym", 0L, p);
	else
	    check_mouse_termcode();	/* set mouse termcode anyway */
    }
# else
    set_mouse_termcode(KS_MOUSE, (char_u *)"\233M");
# endif
#endif	/* USE_MOUSE */

#ifdef USE_SNIFF
    {
	char_u	name[2];

	name[0] = (int)KS_EXTRA;
	name[1] = (int)KE_SNIFF;
	add_termcode(name, (char_u *)"\233sniff", FALSE);
    }
#endif

#ifdef USE_TERM_CONSOLE
	/* DEFAULT_TERM indicates that it is the machine console. */
    if (STRCMP(term, DEFAULT_TERM))
	term_console = FALSE;
    else
    {
	term_console = TRUE;
# ifdef AMIGA
	win_resize_on();	/* enable window resizing reports */
# endif
    }
#endif

#if defined(UNIX) || defined(VMS)
/*
 * 'ttyfast' is default on for xterm, iris-ansi and a few others.
 */
    if (vim_is_fastterm(term))
	p_tf = TRUE;
#endif
#ifdef USE_TERM_CONSOLE
/*
 * 'ttyfast' is default on consoles
 */
    if (term_console)
	p_tf = TRUE;
#endif

    ttest(TRUE);	/* make sure we have a valid set of terminal codes */

    full_screen = TRUE;		/* we can use termcap codes from now on */
    set_term_defaults();	/* use current values as defaults */
#ifdef HAVE_TGETENT
    need_get_crv = TRUE;	/* Get terminal version later */
#endif

    /*
     * Initialize the terminal with the appropriate termcap codes.
     * Set the mouse and window title if possible.
     * Don't do this when starting, need to parse the .vimrc first, because it
     * may redefine t_TI etc.
     */
    if (starting != NO_SCREEN)
    {
	starttermcap();		/* may change terminal mode */
#ifdef USE_MOUSE
	setmouse();		/* may start using the mouse */
#endif
#ifdef WANT_TITLE
	maketitle();		/* may display window title */
#endif
    }

	/* display initial screen after ttest() checking. jw. */
    if (width <= 0 || height <= 0)
    {
	/* termcap failed to report size */
	/* set defaults, in case ui_get_winsize also fails */
	width = 80;
#if defined MSDOS  ||  defined WIN32
	height = 25;	    /* console is often 25 lines */
#else
	height = 24;	    /* most terminals are 24 lines */
#endif
    }
    set_winsize(width, height, FALSE);	/* may change Rows */
    if (starting != NO_SCREEN)
    {
	if (scroll_region)
	    scroll_region_reset();	    /* In case Rows changed */
	check_map_keycodes();	/* check mappings for terminal codes used */

#ifdef AUTOCMD
	{
	    BUF		*old_curbuf;

	    /*
	     * Execute the TermChanged autocommands for each buffer that is
	     * loaded.
	     */
	    old_curbuf = curbuf;
	    for (curbuf = firstbuf; curbuf != NULL; curbuf = curbuf->b_next)
	    {
		if (curbuf->b_ml.ml_mfp != NULL)
		    apply_autocmds(EVENT_TERMCHANGED, NULL, NULL, FALSE,
								      curbuf);
	    }
	    if (buf_valid(old_curbuf))
		curbuf = old_curbuf;
	}
#endif
    }

    return OK;
}

#if defined(USE_MOUSE) || defined(PROTO)
#define HMT_NORMAL	1
#define HMT_NETTERM	2
#define HMT_DEC		4
static int has_mouse_termcode = 0;

# if (!defined(UNIX) || defined(XTERM_MOUSE) \
	    || defined(NETTERM_MOUSE) || defined(DEC_MOUSE)) || defined(PROTO)
    void
set_mouse_termcode(n, s)
    int		n;	/* KS_MOUSE, KS_NETTERM_MOUSE or KS_DEC_MOUSE */
    char_u	*s;
{
    char_u	name[2];

    name[0] = n;
    name[1] = K_FILLER;
    add_termcode(name, s, FALSE);
#  ifdef NETTERM_MOUSE
    if (n == KS_NETTERM_MOUSE)
	has_mouse_termcode |= HMT_NETTERM;
    else
#  endif
#  ifdef DEC_MOUSE
	if (n == KS_DEC_MOUSE)
	    has_mouse_termcode |= HMT_DEC;
	else
#  endif
	    has_mouse_termcode |= HMT_NORMAL;
}
# endif

# if ((defined(UNIX) || defined(VMS) || defined(OS2)) && (defined(XTERM_MOUSE) \
	    || defined(DEC_MOUSE) || defined(GPM_MOUSE))) || defined(PROTO)
    void
del_mouse_termcode(n)
    int		n;	/* KS_MOUSE, KS_NETTERM_MOUSE or KS_DEC_MOUSE */
{
    char_u	name[2];

    name[0] = n;
    name[1] = K_FILLER;
    del_termcode(name);
#  ifdef NETTERM_MOUSE
    if (n == KS_NETTERM_MOUSE)
	has_mouse_termcode &= ~HMT_NETTERM;
    else
#  endif
#  ifdef DEC_MOUSE
	if (n == KS_DEC_MOUSE)
	    has_mouse_termcode &= ~HMT_DEC;
	else
#  endif
	    has_mouse_termcode &= ~HMT_NORMAL;
}
# endif
#endif

#ifdef HAVE_TGETENT
/*
 * Call tgetent()
 * Return error message if it fails, NULL if it's OK.
 */
    static char_u *
tgetent_error(tbuf, term)
    char_u  *tbuf;
    char_u  *term;
{
    int	    i;

    i = TGETENT(tbuf, term);
    if (i < 1)
    {
	/* On FreeBSD tputs() gets a SEGV after a tgetent() which fails.  Call
	 * tgetent() with the always existing "dumb" entry to avoid a crash or
	 * hang. */
	(void)TGETENT(tbuf, "dumb");

	if (i == -1)
	    return (char_u *)"Cannot open termcap file";
	if (i == 0)
#ifdef TERMINFO
	    return (char_u *)"Terminal entry not found in terminfo";
#else
	    return (char_u *)"Terminal entry not found in termcap";
#endif
    }
    return NULL;
}

/*
 * Some versions of tgetstr() have been reported to return -1 instead of NULL.
 * Fix that here.
 */
    static char_u *
vim_tgetstr(s, pp)
    char	*s;
    char_u	**pp;
{
    char	*p;

    p = tgetstr(s, (char **)pp);
    if (p == (char *)-1)
	p = NULL;
    return (char_u *)p;
}
#endif /* HAVE_TGETENT */

#if defined(HAVE_TGETENT) && (defined(UNIX) || defined(__EMX__))
/*
 * Get Columns and Rows from the termcap. Used after a window signal if the
 * ioctl() fails. It doesn't make sense to call tgetent each time if the "co"
 * and "li" entries never change. But on some systems this works.
 * Errors while getting the entries are ignored.
 */
    void
getlinecol()
{
    char_u	    tbuf[TBUFSZ];

    if (T_NAME != NULL && *T_NAME != NUL && tgetent_error(tbuf, T_NAME) == NULL)
    {
	if (Columns == 0)
	    Columns = tgetnum("co");
	if (Rows == 0)
	    Rows = tgetnum("li");
    }
}
#endif /* defined(HAVE_TGETENT) && defined(UNIX) */

/*
 * Get a string entry from the termcap and add it to the list of termcodes.
 * Used for <t_xx> special keys.
 * Give an error message for failure when not sourcing.
 * If force given, replace an existing entry.
 * Return FAIL if the entry was not found, OK if the entry was added.
 */
    int
add_termcap_entry(name, force)
    char_u  *name;
    int	    force;
{
    char_u  *term;
    int	    key;
    struct builtin_term *termp;
#ifdef HAVE_TGETENT
    char_u  *string;
    int	    i;
    int	    builtin_first;
    char_u  tbuf[TBUFSZ];
    char_u  tstrbuf[TBUFSZ];
    char_u  *tp = tstrbuf;
    char_u  *error_msg = NULL;
#endif

/*
 * If the GUI is running or will start in a moment, we only support the keys
 * that the GUI can produce.
 */
#ifdef USE_GUI
    if (gui.in_use || gui.starting)
	return gui_mch_haskey(name);
#endif

    if (!force && find_termcode(name) != NULL)	    /* it's already there */
	return OK;

    term = T_NAME;
    if (term == NULL || *term == NUL)	    /* 'term' not defined yet */
	return FAIL;

    if (term_is_builtin(term))		    /* name starts with "builtin_" */
    {
	term += 8;
#ifdef HAVE_TGETENT
	builtin_first = TRUE;
#endif
    }
#ifdef HAVE_TGETENT
    else
	builtin_first = p_tbi;
#endif

#ifdef HAVE_TGETENT
/*
 * We can get the entry from the builtin termcap and from the external one.
 * If 'ttybuiltin' is on or the terminal name starts with "builtin_", try
 * builtin termcap first.
 * If 'ttybuiltin' is off, try external termcap first.
 */
    for (i = 0; i < 2; ++i)
    {
	if (!builtin_first == i)
#endif
	/*
	 * Search in builtin termcap
	 */
	{
	    termp = find_builtin_term(term);
	    if (termp->bt_string != NULL)	/* found it */
	    {
		key = TERMCAP2KEY(name[0], name[1]);
		while (termp->bt_entry != (int)KS_NAME)
		{
		    if ((int)termp->bt_entry == key)
		    {
			add_termcode(name, (char_u *)termp->bt_string,
							  term_is_8bit(term));
			return OK;
		    }
		    ++termp;
		}
	    }
	}
#ifdef HAVE_TGETENT
	else
	/*
	 * Search in external termcap
	 */
	{
	    error_msg = tgetent_error(tbuf, term);
	    if (error_msg == NULL)
	    {
		string = TGETSTR((char *)name, &tp);
		if (string != NULL && *string != NUL)
		{
		    add_termcode(name, string, FALSE);
		    return OK;
		}
	    }
	}
    }
#endif

    if (sourcing_name == NULL)
    {
#ifdef HAVE_TGETENT
	if (error_msg != NULL)
	    EMSG(error_msg);
	else
#endif
	    EMSG2("No \"%s\" entry in termcap", name);
    }
    return FAIL;
}

    static int
term_is_builtin(name)
    char_u  *name;
{
    return (STRNCMP(name, "builtin_", (size_t)8) == 0);
}

/*
 * Return TRUE if terminal "name" uses CSI instead of <Esc>[.
 * Assume that the terminal is using 8-bit controls when the name contains
 * "8bit", like in "xterm-8bit".
 */
    int
term_is_8bit(name)
    char_u  *name;
{
    return (strstr((char *)name, "8bit") != NULL);
}

/*
 * Translate terminal control chars from 7-bit to 8-bit:
 * <Esc>[ -> CSI
 * <Esc>] -> <M-C-]>
 * <Esc>O -> <M-C-O>
 */
    static int
term_7to8bit(p)
    char_u  *p;
{
    if (*p == ESC)
    {
	if (p[1] == '[')
	    return CSI;
	if (p[1] == ']')
	    return 0x9d;
	if (p[1] == 'O')
	    return 0x8f;
    }
    return 0;
}

#ifdef USE_GUI
    int
term_is_gui(name)
    char_u  *name;
{
    return (STRCMP(name, "builtin_gui") == 0 || STRCMP(name, "gui") == 0);
}
#endif

#if !defined(HAVE_TGETENT) || defined(AMIGA) || defined(PROTO)

    char_u *
tltoa(i)
    unsigned long i;
{
    static char_u buf[16];
    char_u	*p;

    p = buf + 15;
    *p = '\0';
    do
    {
	--p;
	*p = (char_u) (i % 10 + '0');
	i /= 10;
    }
    while (i > 0 && p > buf);
    return p;
}
#endif

#ifndef HAVE_TGETENT

/*
 * minimal tgoto() implementation.
 * no padding and we only parse for %i %d and %+char
 */
char *tgoto __ARGS((char *, int, int));

    char *
tgoto(cm, x, y)
    char *cm;
    int x, y;
{
    static char buf[30];
    char *p, *s, *e;

    if (!cm)
	return "OOPS";
    e = buf + 29;
    for (s = buf; s < e && *cm; cm++)
    {
	if (*cm != '%')
	{
	    *s++ = *cm;
	    continue;
	}
	switch (*++cm)
	{
	case 'd':
	    p = (char *)tltoa((unsigned long)y);
	    y = x;
	    while (*p)
		*s++ = *p++;
	    break;
	case 'i':
	    x++;
	    y++;
	    break;
	case '+':
	    *s++ = (char)(*++cm + y);
	    y = x;
	    break;
	case '%':
	    *s++ = *cm;
	    break;
	default:
	    return "OOPS";
	}
    }
    *s = '\0';
    return buf;
}

#endif /* HAVE_TGETENT */

/*
 * Set the terminal name and initialize the terminal options.
 * If "name" is NULL or empty, get the terminal name from the environment.
 * If that fails, use the default terminal name.
 */
    void
termcapinit(name)
    char_u *name;
{
    char_u	*term;

    if (name != NULL && *name == NUL)
	name = NULL;	    /* empty name is equal to no name */
    term = name;

#ifdef __BEOS__
    /*
     * TERM environment variable is normally set to 'ansi' on the Bebox;
     * Since the BeBox doesn't quite support full ANSI yet, we use our
     * own custom 'ansi-beos' termcap instead, unless the -T option has
     * been given on the command line.
     */
    if (term == NULL
		 && strcmp((char *)mch_getenv((char_u *)"TERM"), "ansi") == 0)
	term = DEFAULT_TERM;
#endif
#ifndef MSWIN
    if (term == NULL)
	term = mch_getenv((char_u *)"TERM");
#endif
    if (term == NULL || *term == NUL)
	term = DEFAULT_TERM;
    set_string_option_direct((char_u *)"term", -1, term, TRUE);

    /* Set the default terminal name. */
    set_string_default("term", term);
    set_string_default("ttytype", term);

    /*
     * Avoid using "term" here, because the next mch_getenv() may overwrite it.
     */
    set_termname(T_NAME != NULL ? T_NAME : term);
}

/*
 * the number of calls to ui_write is reduced by using the buffer "out_buf"
 */
#ifdef DOS16
# define OUT_SIZE	255		/* only have 640K total... */
#else
# define OUT_SIZE	2047
#endif
	    /* Add one to allow mch_write() in os_win32.c to append a NUL */
static char_u		out_buf[OUT_SIZE + 1];
static int		out_pos = 0;	/* number of chars in out_buf */

/*
 * out_flush(): flush the output buffer
 */
    void
out_flush()
{
    int	    len;

    if (out_pos != 0)
    {
	/* set out_pos to 0 before ui_write, to avoid recursiveness */
	len = out_pos;
	out_pos = 0;
	ui_write(out_buf, len);
    }
}

#ifdef USE_GUI
/*
 * out_trash(): Throw away the contents of the output buffer
 */
    void
out_trash()
{
    out_pos = 0;
}
#endif

/*
 * out_char(c): put a character into the output buffer.
 *		Flush it if it becomes full.
 * This should not be used for outputting text on the screen (use functions
 * like msg_puts() and screen_putchar() for that).
 */
    void
out_char(c)
    unsigned	c;
{
#if defined(UNIX) || defined(VMS) || defined(AMIGA)
    if (c == '\n')	/* turn LF into CR-LF (CRMOD doesn't seem to do this) */
	out_char('\r');
#endif

    out_buf[out_pos++] = c;

    /* For testing we flush each time. */
    if (out_pos >= OUT_SIZE || p_wd)
	out_flush();
}

static void out_char_nf __ARGS((unsigned));

/*
 * out_char_nf(c): like out_char(), but don't flush when p_wd is set
 */
    static void
out_char_nf(c)
    unsigned	c;
{
#if defined(UNIX) || defined(VMS) || defined(AMIGA)
    if (c == '\n')	/* turn LF into CR-LF (CRMOD doesn't seem to do this) */
	out_char_nf('\r');
#endif

    out_buf[out_pos++] = c;

    if (out_pos >= OUT_SIZE)
	out_flush();
}

/*
 * A never-padding out_str.
 * use this whenever you don't want to run the string through tputs.
 * tputs above is harmless, but tputs from the termcap library
 * is likely to strip off leading digits, that it mistakes for padding
 * information. (jw)
 * This should only be used for writing terminal codes, not for outputting
 * normal text (use functions like msg_puts() and screen_putchar() for that).
 */
    void
out_str_nf(s)
    char_u *s;
{
    if (out_pos > OUT_SIZE - 20)  /* avoid terminal strings being split up */
	out_flush();
    while (*s)
	out_char_nf(*s++);

    /* For testing we write one string at a time. */
    if (p_wd)
	out_flush();
}

/*
 * out_str(s): Put a string character at a time into the output buffer.
 * If HAVE_TGETENT is defined use the termcap parser. (jw)
 * This should only be used for writing terminal codes, not for outputting
 * normal text (use functions like msg_puts() and screen_putchar() for that).
 */
    void
out_str(s)
    char_u	 *s;
{
    if (s != NULL && *s)
    {
#ifdef USE_GUI
	/* Don't use tputs() when GUI is used, ncurses crashes. */
	if (gui.in_use)
	{
	    out_str_nf(s);
	    return;
	}
#endif
	/* avoid terminal strings being split up */
	if (out_pos > OUT_SIZE - 20)
	    out_flush();
#ifdef HAVE_TGETENT
	tputs((char *)s, 1, TPUTSFUNCAST out_char_nf);
#else
	while (*s)
	    out_char_nf(*s++);
#endif

	/* For testing we write one string at a time. */
	if (p_wd)
	    out_flush();
    }
}

/*
 * cursor positioning using termcap parser. (jw)
 */
    void
term_windgoto(row, col)
    int	    row;
    int	    col;
{
    OUT_STR(tgoto((char *)T_CM, col, row));
}

    void
term_cursor_right(i)
    int	    i;
{
    OUT_STR(tgoto((char *)T_CRI, 0, i));
}

    void
term_append_lines(line_count)
    int	    line_count;
{
    OUT_STR(tgoto((char *)T_CAL, 0, line_count));
}

    void
term_delete_lines(line_count)
    int	    line_count;
{
    OUT_STR(tgoto((char *)T_CDL, 0, line_count));
}

#if defined(HAVE_TGETENT) || defined(PROTO)
    void
term_set_winpos(x, y)
    int	    x;
    int	    y;
{
    /* Can't handle a negative value here */
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;
    OUT_STR(tgoto((char *)T_CWP, y, x));
}

    void
term_set_winsize(width, height)
    int	    width;
    int	    height;
{
    OUT_STR(tgoto((char *)T_CWS, height, width));
}
#endif

    void
term_fg_color(n)
    int	    n;
{
    /* Use "AF" termcap entry if present, "Sf" entry otherwise */
    if (*T_CAF)
	term_color(T_CAF, n);
    else if (*T_CSF)
	term_color(T_CSF, n);
}

    void
term_bg_color(n)
    int	    n;
{
    /* Use "AB" termcap entry if present, "Sb" entry otherwise */
    if (*T_CAB)
	term_color(T_CAB, n);
    else if (*T_CSB)
	term_color(T_CSB, n);
}

    static void
term_color(s, n)
    char_u  *s;
    int	    n;
{
    int i = 2;	/* index in s[] just after <Esc>[ or CSI */

    /* Special handling of 16 colors, because termcap can't handle it */
    /* Also accept "\e[3%dm" for TERMINFO, it is sometimes used */
    /* Also accept CSI instead of <Esc>[ */
    if (n > 7 && atoi((char *)T_CCO) == 16
	      && ((s[0] == ESC && s[1] == '[') || (s[0] == CSI && (i = 1) == 1))
	      && s[i] != NUL
	      && (STRCMP(s + i + 1, "%p1%dm") == 0
		  || STRCMP(s + i + 1, "%dm") == 0))
    {
	if (s[i] == '3')	/* foreground */
	{
#ifdef TERMINFO
	    OUT_STR(tgoto(i == 2 ? "\033[9%p1%dm"
					       : "\233\071%p1%dm", 0, n - 8));
#else
	    OUT_STR(tgoto(i == 2 ? "\033[9%dm" : "\233\071%dm", 0, n - 8));
#endif
	    return;
	}
	if (s[i] == '4')	/* background */
	{
#ifdef TERMINFO
	    OUT_STR(tgoto(i == 2 ? "\033[10%p1%dm"
					   : "\233\061\060%p1%dm", 0, n - 8));
#else
	    OUT_STR(tgoto(i == 2 ? "\033[10%dm"
					      : "\233\061\060%dm", 0, n - 8));
#endif
	    return;
	}

    }
    OUT_STR(tgoto((char *)s, 0, n));
}

#if (defined(WANT_TITLE) && (defined(UNIX) || defined(OS2) || defined(VMS))) || defined(PROTO)
/*
 * Generic function to set window title, using t_ts and t_fs.
 */
    void
term_settitle(title)
    char_u	*title;
{
    /* t_ts takes one argument: column in status line */
    OUT_STR(tgoto((char *)T_TS, 0, 0));	/* set title start */
    out_str_nf(title);
    out_str(T_FS);			/* set title end */
    out_flush();
}
#endif

/*
 * Make sure we have a valid set or terminal options.
 * Replace all entries that are NULL by empty_option
 */
    void
ttest(pairs)
    int	pairs;
{
    char    *t = NULL;

    check_options();		    /* make sure no options are NULL */

  /* hard requirements */
    if (*T_CL == NUL)		    /* erase display */
	t = "cl";
    if (*T_CM == NUL)		    /* cursor motion */
	t = "cm";

    if (t != NULL)
	EMSG2("terminal capability %s required", t);

/*
 * if "cs" defined, use a scroll region, it's faster.
 */
    if (*T_CS != NUL)
	scroll_region = TRUE;
    else
	scroll_region = FALSE;

    if (pairs)
    {
	/*
	 * optional pairs
	 */
	/* TP goes to normal mode for TI (invert) and TB (bold) */
	if (*T_ME == NUL)
	    T_ME = T_MR = T_MD = T_MB = empty_option;
	if (*T_SO == NUL || *T_SE == NUL)
	    T_SO = T_SE = empty_option;
	if (*T_US == NUL || *T_UE == NUL)
	    T_US = T_UE = empty_option;
	if (*T_CZH == NUL || *T_CZR == NUL)
	    T_CZH = T_CZR = empty_option;

	/* T_VE is needed even though T_VI is not defined */
	if (*T_VE == NUL)
	    T_VI = empty_option;

	/* if 'mr' or 'me' is not defined use 'so' and 'se' */
	if (*T_ME == NUL)
	{
	    T_ME = T_SE;
	    T_MR = T_SO;
	    T_MD = T_SO;
	}

	/* if 'so' or 'se' is not defined use 'mr' and 'me' */
	if (*T_SO == NUL)
	{
	    T_SE = T_ME;
	    if (*T_MR == NUL)
		T_SO = T_MD;
	    else
		T_SO = T_MR;
	}

	/* if 'ZH' or 'ZR' is not defined use 'mr' and 'me' */
	if (*T_CZH == NUL)
	{
	    T_CZR = T_ME;
	    if (*T_MR == NUL)
		T_CZH = T_MD;
	    else
		T_CZH = T_MR;
	}

	/* "Sb" and "Sf" come in pairs */
	if (*T_CSB == NUL || *T_CSF == NUL)
	{
	    T_CSB = empty_option;
	    T_CSF = empty_option;
	}

	/* "AB" and "AF" come in pairs */
	if (*T_CAB == NUL || *T_CAF == NUL)
	{
	    T_CAB = empty_option;
	    T_CAF = empty_option;
	}

	/* if 'Sb' and 'AB' are not defined, reset "Co" */
	if (*T_CSB == NUL && *T_CAB == NUL)
	    T_CCO = empty_option;

	/* Set 'weirdinvert' according to value of 't_xs' */
	p_wiv = (*T_XS != NUL);
    }
    need_gather = TRUE;
}

#if defined(USE_GUI) || defined(PROTO)
/*
 * Represent the given long_u as individual bytes, with the most significant
 * byte first, and store them in dst.
 */
    void
add_long_to_buf(val, dst)
    long_u  val;
    char_u  *dst;
{
    int	    i;
    int	    shift;

    for (i = 1; i <= sizeof(long_u); i++)
    {
	shift = 8 * (sizeof(long_u) - i);
	dst[i - 1] = (char_u) ((val >> shift) & 0xff);
    }
}

/*
 * Interpret the next string of bytes in buf as a long integer, with the most
 * significant byte first.  Note that it is assumed that buf has been through
 * inchar(), so that NUL and K_SPECIAL will be represented as three bytes each.
 * Puts result in val, and returns the number of bytes read from buf
 * (between sizeof(long_u) and 2 * sizeof(long_u)), or -1 if not enough bytes
 * were present.
 */
    int
get_long_from_buf(buf, val)
    char_u  *buf;
    long_u  *val;
{
    int	    len;
    char_u  bytes[sizeof(long_u)];
    int	    i;
    int	    shift;

    *val = 0;
    len = get_bytes_from_buf(buf, bytes, (int)sizeof(long_u));
    if (len != -1)
    {
	for (i = 0; i < sizeof(long_u); i++)
	{
	    shift = 8 * (sizeof(long_u) - 1 - i);
	    *val += (long_u)bytes[i] << shift;
	}
    }
    return len;
}
#endif

#if defined(USE_GUI) \
    || (defined(USE_MOUSE) && (!defined(UNIX) || defined(XTERM_MOUSE)))
/*
 * Read the next num_bytes bytes from buf, and store them in bytes.  Assume
 * that buf has been through inchar().	Returns the actual number of bytes used
 * from buf (between num_bytes and num_bytes*2), or -1 if not enough bytes were
 * available.
 */
    static int
get_bytes_from_buf(buf, bytes, num_bytes)
    char_u  *buf;
    char_u  *bytes;
    int	    num_bytes;
{
    int	    len = 0;
    int	    i;
    char_u  c;

    for (i = 0; i < num_bytes; i++)
    {
	if ((c = buf[len++]) == NUL)
	    return -1;
	if (c == K_SPECIAL)
	{
	    if (buf[len] == NUL || buf[len + 1] == NUL)	    /* cannot happen? */
		return -1;
	    if (buf[len++] == (int)KS_ZERO)
		c = NUL;
	    ++len;	/* skip K_FILLER */
	    /* else it should be KS_SPECIAL, and c already equals K_SPECIAL */
	}
	bytes[i] = c;
    }
    return len;
}
#endif

    void
check_winsize()
{
    static int	old_Rows = 0;

    if (Columns < MIN_COLUMNS)
	Columns = MIN_COLUMNS;
    if (Rows < min_rows())	/* need room for one window and command line */
	Rows = min_rows();

    if (old_Rows != Rows)
    {
	old_Rows = Rows;
	screen_new_rows();	/* may need to update window sizes */
    }
}

/*
 * Set window size.
 * If 'mustset' is TRUE, we must set Rows and Columns, do not get real
 * window size (this is used for the :win command).
 * If 'mustset' is FALSE, we may try to get the real window size and if
 * it fails use 'width' and 'height'.
 */
    void
set_winsize(width, height, mustset)
    int	    width, height;
    int	    mustset;
{
    static int		busy = FALSE;

    /*
     * Avoid recursiveness, can happen when setting the window size causes
     * another window-changed signal.
     */
    if (busy)
	return;

    if (width < 0 || height < 0)    /* just checking... */
	return;

    if (State == HITRETURN || State == SETWSIZE) /* postpone the resizing */
    {
	State = SETWSIZE;
	return;
    }

    ++busy;

    if (State != ASKMORE && State != EXTERNCMD && State != CONFIRM)
	screenclear();
    else
	screen_start();	    /* don't know where cursor is now */
#ifdef AMIGA
    out_flush();	    /* must do this before mch_get_winsize() for some
			       obscure reason */
#endif

    if (mustset || (ui_get_winsize() == FAIL && height != 0))
    {
	Rows = height;
	Columns = width;
	check_winsize();	/* always check, to get p_scroll right */
	ui_set_winsize();
    }
    else
	check_winsize();	/* always check, to get p_scroll right */

    if (starting != NO_SCREEN)
    {
#ifdef WANT_TITLE
	maketitle();
#endif
	changed_line_abv_curs();
	invalidate_botline();

	/*
	 * We only redraw when it's needed:
	 * - While at the more prompt or executing an external command, don't
	 *   redraw, but position the cursor.
	 * - While editing the command line, only redraw that.
	 * - in Ex mode, don't redraw anything.
	 * - Otherwise, redraw right now, and position the cursor.
	 * Always need to call update_screen() or screenalloc(), to make
	 * sure Rows/Columns and the size of NextScreen is correct!
	 */
	if (State == ASKMORE || State == EXTERNCMD || State == CONFIRM
							     || exmode_active)
	{
	    screenalloc(FALSE);
	    repeat_message();
	}
	else if (State == CMDLINE)
	{
	    update_screen(NOT_VALID);
	    redrawcmdline();
	}
	else
	{
	    update_topline();
	    update_screen(NOT_VALID);
	    if (redrawing())
		setcursor();
	}
	cursor_on();	    /* redrawing may have switched it off */
    }
    out_flush();
    --busy;
}

static int	cur_tmode = TMODE_COOK;

/*
 * Set the terminal to TMODE_RAW (for Normal mode) or TMODE_COOK (for external
 * commands and Ex mode).
 */
    void
settmode(tmode)
    int	 tmode;
{
#ifdef USE_GUI
    /* don't set the term where gvim was started to any mode */
    if (gui.in_use)
	return;
#endif

    if (full_screen)
    {
	/* In Ex mode, never set to RAW */
	if (exmode_active)
	    tmode = TMODE_COOK;

	/*
	 * When returning after calling a shell we want to really set the
	 * terminal to raw mode, even though we think it already is, because
	 * the shell program may have reset the terminal mode.
	 * When we think the terminal is normal, don't try to set it to
	 * normal again, because that causes problems (logout!) on some
	 * machines.
	 */
	if (tmode != TMODE_COOK || cur_tmode != TMODE_COOK)
	{
#ifdef USE_MOUSE
	    if (tmode != TMODE_RAW)
		mch_setmouse(FALSE);		/* switch mouse off */
#endif
	    out_flush();
	    mch_settmode(tmode);    /* machine specific function */
	    cur_tmode = tmode;
#ifdef USE_MOUSE
	    if (tmode == TMODE_RAW)
		setmouse();			/* may switch mouse on */
#endif
	    out_flush();
	}

#ifdef HAVE_TGETENT
	/* Request version string (for xterm) just after switching to raw mode
	 * (otherwise the result will be echoed).  The result is caught in
	 * check_termcode(). */
	if (cur_tmode == TMODE_RAW && need_get_crv && *T_CRV)
	{
	    out_str(T_CRV);
	    need_get_crv = FALSE;
	    /* check for the characters now, otherwise they might be eaten by
	     * get_keystroke() */
	    (void)vpeekc();
	}
#endif

    }
}

    void
starttermcap()
{
    screen_stop_highlight();
    if (full_screen && !termcap_active)
    {
	out_str(T_TI);			/* start termcap mode */
	out_str(T_KS);			/* start "keypad transmit" mode */
	out_flush();
	termcap_active = TRUE;
	screen_start();			/* don't know where cursor is now */
    }
}

    void
stoptermcap()
{
    screen_stop_highlight();
    reset_cterm_colors();
    if (termcap_active)
    {
	out_str(T_KE);			/* stop "keypad transmit" mode */
	out_flush();
	termcap_active = FALSE;
	cursor_on();			/* just in case it is still off */
	out_str(T_TE);			/* stop termcap mode */
	screen_start();			/* don't know where cursor is now */
	out_flush();
    }
}

/*
 * Return TRUE when saving and restoring the screen.
 */
    int
swapping_screen()
{
    return (*T_TI != NUL);
}

#ifdef USE_MOUSE
/*
 * setmouse() - switch mouse on/off depending on current mode and 'mouse'
 */
    void
setmouse()
{
    int	    checkfor;

# ifdef USE_GUI
    if (gui.in_use)
	return;
# endif
    /* be quick when mouse is off */
    if (*p_mouse == NUL || has_mouse_termcode == 0)
	return;

    /* don't switch mouse on when not in raw mode (Ex mode) */
    if (cur_tmode != TMODE_RAW)
    {
	mch_setmouse(FALSE);
	return;
    }

    if (VIsual_active)
	checkfor = MOUSE_VISUAL;
    else if (State == HITRETURN || State == ASKMORE || State == SETWSIZE)
	checkfor = MOUSE_RETURN;
    else if (State & INSERT)
	checkfor = MOUSE_INSERT;
    else if (State & CMDLINE)
	checkfor = MOUSE_COMMAND;
    else if (State == CONFIRM || State == EXTERNCMD)
	checkfor = ' '; /* don't use mouse for ":confirm" or ":!cmd" */
    else
	checkfor = MOUSE_NORMAL;    /* assume normal mode */

    if (mouse_has(checkfor))
	mch_setmouse(TRUE);
    else
	mch_setmouse(FALSE);
}

/*
 * Return TRUE if
 * - "c" is in 'mouse', or
 * - 'a' is in 'mouse' and "c" is in MOUSE_A, or
 * - the current buffer is a help file and 'h' is in 'mouse' and we are in a
 *   normal editing mode (not at hit-return message).
 */
    int
mouse_has(c)
    int	    c;
{
    char_u	*p;

    for (p = p_mouse; *p; ++p)
	switch (*p)
	{
	    case 'a': if (vim_strchr((char_u *)MOUSE_A, c) != NULL)
			  return TRUE;
		      break;
	    case MOUSE_HELP: if (c != MOUSE_RETURN && curbuf->b_help)
				 return TRUE;
			     break;
	    default: if (c == *p) return TRUE; break;
	}
    return FALSE;
}

/*
 * Return TRUE when 'mousemodel' is set to "popup".
 */
    int
mouse_model_popup()
{
    return (p_mousem[0] == 'p');
}
#endif

/*
 * By outputting the 'cursor very visible' termcap code, for some windowed
 * terminals this makes the screen scrolled to the correct position.
 * Used when starting Vim or returning from a shell.
 */
    void
scroll_start()
{
    if (*T_VS != NUL)
    {
	out_str(T_VS);
	out_str(T_VE);
	screen_start();			/* don't know where cursor is now */
    }
}

/*
 * enable cursor, unless in Visual mode.
 */
static int cursor_is_off = FALSE;

    void
cursor_on()
{
    if (cursor_is_off)
    {
	out_str(T_VE);
	cursor_is_off = FALSE;
    }
}

    void
cursor_off()
{
    if (full_screen)
    {
	if (!cursor_is_off)
	    out_str(T_VI);	    /* disable cursor */
	cursor_is_off = TRUE;
    }
}

/*
 * Set scrolling region for window 'wp'.
 * The region starts 'off' lines from the start of the window.
 */
    void
scroll_region_set(wp, off)
    WIN	    *wp;
    int	    off;
{
    OUT_STR(tgoto((char *)T_CS, wp->w_winpos + wp->w_height - 1,
							 wp->w_winpos + off));
    screen_start();		    /* don't know where cursor is now */
}

/*
 * Reset scrolling region to the whole screen.
 */
    void
scroll_region_reset()
{
    OUT_STR(tgoto((char *)T_CS, (int)Rows - 1, 0));
    screen_start();		    /* don't know where cursor is now */
}


/*
 * List of terminal codes that are currently recognized.
 */

struct termcode
{
    char_u  name[2];	    /* termcap name of entry */
    char_u  *code;	    /* terminal code (in allocated memory) */
    int	    len;	    /* STRLEN(code) */
} *termcodes = NULL;

static int  tc_max_len = 0; /* number of entries that termcodes[] can hold */
static int  tc_len = 0;	    /* current number of entries in termcodes[] */

    void
clear_termcodes()
{
    while (tc_len > 0)
	vim_free(termcodes[--tc_len].code);
    vim_free(termcodes);
    termcodes = NULL;
    tc_max_len = 0;

#ifdef HAVE_TGETENT
    BC = (char *)empty_option;
    UP = (char *)empty_option;
    PC = NUL;			/* set pad character to NUL */
    ospeed = 0;
#endif

    need_gather = TRUE;		/* need to fill termleader[] */
}

/*
 * Add a new entry to the list of terminal codes.
 * The list is kept alphabetical for ":set termcap"
 */
    void
add_termcode(name, string, use_8bit)
    char_u	*name;
    char_u	*string;
    int		use_8bit;	/* replace 7-bit control by 8-bit one */
{
    struct termcode *new_tc;
    int		    i, j;
    char_u	    *s;

    if (string == NULL || *string == NUL)
    {
	del_termcode(name);
	return;
    }

    s = vim_strsave(string);
    if (s == NULL)
	return;

    /* Change leading <Esc>[ to CSI, change <Esc>O to <M-O>. */
    if (use_8bit && term_7to8bit(string) != 0)
    {
	mch_memmove(s, s + 1, STRLEN(s));
	s[0] = term_7to8bit(string);
    }

    need_gather = TRUE;		/* need to fill termleader[] */

    /*
     * need to make space for more entries
     */
    if (tc_len == tc_max_len)
    {
	tc_max_len += 20;
	new_tc = (struct termcode *)alloc(
			    (unsigned)(tc_max_len * sizeof(struct termcode)));
	if (new_tc == NULL)
	{
	    tc_max_len -= 20;
	    return;
	}
	for (i = 0; i < tc_len; ++i)
	    new_tc[i] = termcodes[i];
	vim_free(termcodes);
	termcodes = new_tc;
    }

    /*
     * Look for existing entry with the same name, it is replaced.
     * Look for an existing entry that is alphabetical higher, the new entry
     * is inserted in front of it.
     */
    for (i = 0; i < tc_len; ++i)
    {
	if (termcodes[i].name[0] < name[0])
	    continue;
	if (termcodes[i].name[0] == name[0])
	{
	    if (termcodes[i].name[1] < name[1])
		continue;
	    /*
	     * Exact match: Replace old code.
	     */
	    if (termcodes[i].name[1] == name[1])
	    {
		vim_free(termcodes[i].code);
		--tc_len;
		break;
	    }
	}
	/*
	 * Found alphabetical larger entry, move rest to insert new entry
	 */
	for (j = tc_len; j > i; --j)
	    termcodes[j] = termcodes[j - 1];
	break;
    }

    termcodes[i].name[0] = name[0];
    termcodes[i].name[1] = name[1];
    termcodes[i].code = s;
    termcodes[i].len = STRLEN(s);
    ++tc_len;
}

    char_u  *
find_termcode(name)
    char_u  *name;
{
    int	    i;

    for (i = 0; i < tc_len; ++i)
	if (termcodes[i].name[0] == name[0] && termcodes[i].name[1] == name[1])
	    return termcodes[i].code;
    return NULL;
}

#if defined(CMDLINE_COMPL) || defined(PROTO)
    char_u *
get_termcode(i)
    int	    i;
{
    if (i >= tc_len)
	return NULL;
    return &termcodes[i].name[0];
}
#endif

    void
del_termcode(name)
    char_u  *name;
{
    int	    i;

    if (termcodes == NULL)	/* nothing there yet */
	return;

    need_gather = TRUE;		/* need to fill termleader[] */

    for (i = 0; i < tc_len; ++i)
	if (termcodes[i].name[0] == name[0] && termcodes[i].name[1] == name[1])
	{
	    vim_free(termcodes[i].code);
	    --tc_len;
	    while (i < tc_len)
	    {
		termcodes[i] = termcodes[i + 1];
		++i;
	    }
	    return;
	}
    /* not found. Give error message? */
}

/*
 * Check if typebuf[] contains a terminal key code.
 * Check from typebuf[typeoff] to typebuf[typeoff + max_offset].
 * Return 0 for no match, -1 for partial match, > 0 for full match.
 * With a match, the match is removed, the replacement code is inserted in
 * typebuf[] and the number of characters in typebuf[] is returned.
 * When "buf" is not NULL, it is used instead of typebuf[]. "buflen" is then
 * the length of the string in buf[].
 */
    int
check_termcode(max_offset, buf, buflen)
    int		max_offset;
    char_u	*buf;
    int		buflen;
{
    char_u	*tp;
    char_u	*p;
    int		slen = 0;	/* init for GCC */
    int		len;
    int		offset;
    char_u	key_name[2];
    int		new_slen;
    int		extra;
    char_u	string[MAX_KEY_CODE_LEN + 1];
    int		i, j;
#ifdef USE_GUI
    long_u	val;
#endif
#ifdef USE_MOUSE
# if !defined(UNIX) || defined(XTERM_MOUSE) || defined(USE_GUI)
    char_u	bytes[3];
    int		num_bytes;
# endif
    int		mouse_code = 0;	    /* init for GCC */
    int		modifiers;
    int		is_click, is_drag;
    int		wheel_code = 0;
    int		current_button;
    static int	held_button = MOUSE_RELEASE;
    static int	orig_num_clicks = 1;
    static int	orig_mouse_code = 0x0;
# if defined(UNIX) && defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)
    static int	orig_mouse_col = 0;
    static int	orig_mouse_row = 0;
    static linenr_t orig_topline = 0;
    static struct timeval  orig_mouse_time = {0, 0};
					/* time of previous mouse click */
    struct timeval  mouse_time;		/* time of current mouse click */
    long	timediff;		/* elapsed time in msec */
# endif
#endif
    int		cpo_koffset;

    cpo_koffset = (vim_strchr(p_cpo, CPO_KOFFSET) != NULL);

    /*
     * Speed up the checks for terminal codes by gathering all first bytes
     * used in termleader[].  Often this is just a single <Esc>.
     */
    if (need_gather)
	gather_termleader();

    /*
     * Check at several positions in typebuf[], to catch something like
     * "x<Up>" that can be mapped. Stop at max_offset, because characters
     * after that cannot be used for mapping, and with @r commands typebuf[]
     * can become very long.
     * This is used often, KEEP IT FAST!
     */
    for (offset = 0; offset < max_offset; ++offset)
    {
	if (buf == NULL)
	{
	    if (offset >= typelen)
		break;
	    tp = typebuf + typeoff + offset;
	    len = typelen - offset;	/* length of the input */
	}
	else
	{
	    if (offset >= buflen)
		break;
	    tp = buf + offset;
	    len = buflen - offset;
	}

	/*
	 * Don't check characters after K_SPECIAL, those are already
	 * translated terminal chars (avoid translating ~@^Hx).
	 */
	if (*tp == K_SPECIAL)
	{
	    offset += 2;	/* there are always 2 extra characters */
	    continue;
	}

	/*
	 * Skip this position if the character does not appear as the first
	 * character in term_strings. This speeds up a lot, since most
	 * termcodes start with the same character (ESC or CSI).
	 */
	i = *tp;
	for (p = termleader; *p && *p != i; ++p)
	    ;
	if (*p == NUL)
	    continue;

	/*
	 * Skip this position if p_ek is not set and tp[0] is an ESC and we
	 * are in Insert mode.
	 */
	if (*tp == ESC && !p_ek && (State & INSERT))
	    continue;

	new_slen = 0;		/* Length of what will replace the termcode */
	key_name[0] = NUL;	/* no key name found yet */

#ifdef USE_GUI
	if (gui.in_use)
	{
	    /*
	     * GUI special key codes are all of the form [CSI xx].
	     */
	    if (*tp == CSI)	    /* Special key from GUI */
	    {
		if (len < 3)
		    return -1;	    /* Shouldn't happen */
		slen = 3;
		key_name[0] = tp[1];
		key_name[1] = tp[2];
	    }
	}
	else
#endif /* USE_GUI */
	{
	    for (i = 0; i < tc_len; ++i)
	    {
		/*
		 * Ignore the entry if we are not at the start of typebuf[]
		 * and there are not enough characters to make a match.
		 * But only when the 'K' flag is in 'cpoptions'.
		 */
		slen = termcodes[i].len;
		if (cpo_koffset && offset && len < slen)
		    continue;
		if (STRNCMP(termcodes[i].code, tp,
				     (size_t)(slen > len ? len : slen)) == 0)
		{
		    if (len < slen)		/* got a partial sequence */
			return -1;		/* need to get more chars */

		    /*
		     * When found a keypad key, check if there is another key
		     * that matches and use that one.  This makes <Home> to be
		     * found instead of <kHome> when they produce the same
		     * key code.
		     */
		    if (termcodes[i].name[0] == 'K' &&
						isdigit(termcodes[i].name[1]))
		    {
			for (j = i + 1; j < tc_len; ++j)
			    if (termcodes[j].len == slen &&
				    STRNCMP(termcodes[i].code,
					    termcodes[j].code, slen) == 0)
			    {
				i = j;
				break;
			    }
		    }

		    key_name[0] = termcodes[i].name[0];
		    key_name[1] = termcodes[i].name[1];

		    break;
		}
	    }
	}

	if (key_name[0] == NUL)
	{
	    /* Check for xterm version string: "<Esc>[><x>;<vers>;<y>c".  Also
	     * eat other possible responses to t_RV, rxvt returns
	     * "<Esc>[?1;2c".  Also accept CSI instead of <Esc>[. */
	    if (*T_CRV != NUL && ((tp[0] == ESC && tp[1] == '[' && len >= 3)
					       || (tp[0] == CSI && len >= 2)))
	    {
		j = 0;
		extra = 0;
		for (i = 2 + (tp[0] != CSI);
			     i < len && (isdigit(tp[i]) || tp[i] == ';'); ++i)
		    if (tp[i] == ';' && ++j == 1)
			extra = atoi((char *)tp + i + 1);
		if (i == len)
		    return -1;		/* not enough characters */

		/* eat it when at least one digit and ending in 'c' */
		if (i > 2 + (tp[0] != CSI) && tp[i] == 'c')
		{
		    /* If xterm version is >= 95, we can use mouse dragging */
		    if (tp[1 + (tp[0] != CSI)] == '>' && j == 2 && extra >= 95)
			set_option_value((char_u *)"ttymouse", 0L,
							  (char_u *)"xterm2");
		    key_name[0] = (int)KS_EXTRA;
		    key_name[1] = (int)KE_IGNORE;
		    slen = i + 1;
		}
		else
		    continue;
	    }
	    else
		continue;    /* No match at this position, try next one */
	}

	/* We only get here when we have a complete termcode match */

#ifdef USE_MOUSE
	/*
	 * If it is a mouse click, get the coordinates.
	 */
	if (key_name[0] == (int)KS_MOUSE
# ifdef NETTERM_MOUSE
		|| key_name[0] == (int)KS_NETTERM_MOUSE
# endif
# ifdef DEC_MOUSE
		|| key_name[0] == (int)KS_DEC_MOUSE
# endif
		)
	{
	    is_click = is_drag = FALSE;

# if !defined(UNIX) || defined(XTERM_MOUSE) || defined(USE_GUI)
	    if (key_name[0] == (int)KS_MOUSE)
	    {
		/*
		 * For xterm and MSDOS we get "<t_mouse>scr", where
		 *  s == encoded button state:
		 *	   0x20 = left button down
		 *	   0x21 = middle button down
		 *	   0x22 = right button down
		 *	   0x23 = any button release
		 *	   0x60 = button 4 down (scroll wheel down)
		 *	   0x61 = button 5 down (scroll wheel up)
		 *	add 0x04 for SHIFT
		 *	add 0x08 for ALT
		 *	add 0x10 for CTRL
		 *	add 0x20 for mouse drag (0x40 is drag with left button)
		 *  c == column + ' ' + 1 == column + 33
		 *  r == row + ' ' + 1 == row + 33
		 *
		 * The coordinates are passed on through global variables.
		 * Ugly, but this avoids trouble with mouse clicks at an
		 * unexpected moment and allows for mapping them.
		 */
		num_bytes = get_bytes_from_buf(tp + slen, bytes, 3);
		if (num_bytes == -1)	/* not enough coordinates */
		    return -1;
		mouse_code = bytes[0];
		mouse_col = bytes[1] - ' ' - 1;
		mouse_row = bytes[2] - ' ' - 1;
		slen += num_bytes;

#  if !defined(MSWIN) && !defined(MSDOS)
		/*
		 * Handle mouse events.
		 * Recognize the xterm mouse wheel, but not in the GUI
		 * and the MS-DOS or Win32 console (multi-clicks use >= 0x60).
		 */
		if (mouse_code >= MOUSEWHEEL_LOW
#   ifdef USE_GUI
			&& !gui.in_use
#   endif
			)
		{
		    /* Keep the mouse_code before it's changed, so that we
		     * remember that it was a mouse wheel click. */
		    wheel_code = mouse_code;
		}
#   ifdef UNIX
		else if (use_xterm_mouse() > 1)
		{
		    if (mouse_code & MOUSE_DRAG_XTERM)
			mouse_code |= MOUSE_DRAG;
		}
#   endif
#   ifdef XTERM_CLIP
		else if (!(mouse_code & MOUSE_DRAG & ~MOUSE_CLICK_MASK))
		{
		    if ((mouse_code & MOUSE_RELEASE) == MOUSE_RELEASE)
			stop_xterm_trace();
		    else
			start_xterm_trace(mouse_code);
		}
#   endif
#  endif
	    }
# endif /* !UNIX || XTERM_MOUSE */
# ifdef NETTERM_MOUSE
	    if (key_name[0] == (int)KS_NETTERM_MOUSE)
	    {
		int mc, mr;

		/* expect a rather limited sequence like: balancing {
		 * \033}6,45\r
		 * '6' is the row, 45 is the column
		 */
		p = tp + slen;
		mr = getdigits(&p);
		if (*p++ != ',')
		    return -1;
		mc = getdigits(&p);
		if (*p++ != '\r')
		    return -1;

		mouse_col = mc - 1;
		mouse_row = mr - 1;
		mouse_code = MOUSE_LEFT;
		slen += (p - (tp + slen));
	    }
# endif	/* NETTERM_MOUSE */
# ifdef DEC_MOUSE
	    if (key_name[0] == (int)KS_DEC_MOUSE)
	    {
	       /* The DEC Locator Input Model
		* Netterm delivers the code sequence:
		*  \033[2;4;24;80&w  (left button down)
		*  \033[3;0;24;80&w  (left button up)
		*  \033[6;1;24;80&w  (right button down)
		*  \033[7;0;24;80&w  (right button up)
		* CSI Pe ; Pb ; Pr ; Pc ; Pp & w
		* Pe is the event code
		* Pb is the button code
		* Pr is the row coordinate
		* Pc is the column coordinate
		* Pp is the third coordinate (page number)
		* Pe, the event code indicates what event caused this report
		*    The following event codes are defined:
		*    0 - request, the terminal received an explicit request
		*	 for a locator report, but the locator is unavailable
		*    1 - request, the terminal received an explicit request
		*	 for a locator report
		*    2 - left button down
		*    3 - left button up
		*    4 - middle button down
		*    5 - middle button up
		*    6 - right button down
		*    7 - right button up
		*    8 - fourth button down
		*    9 - fourth button up
		*    10 - locator outside filter rectangle
		* Pb, the button code, ASCII decimal 0-15 indicating which
		*   buttons are down if any. The state of the four buttons
		*   on the locator correspond to the low four bits of the
		*   decimal value,
		*   "1" means button depressed
		*   0 - no buttons down,
		*   1 - right,
		*   2 - middle,
		*   4 - left,
		*   8 - fourth
		* Pr is the row coordinate of the locator position in the page,
		*   encoded as an ASCII decimal value.
		*   If Pr is omitted, the locator position is undefined
		*   (outside the terminal window for example).
		* Pc is the column coordinate of the locator position in the
		*   page, encoded as an ASCII decimal value.
		*   If Pc is omitted, the locator position is undefined
		*   (outside the terminal window for example).
		* Pp is the page coordinate of the locator position
		*   encoded as an ASCII decimal value.
		*   The page coordinate may be omitted if the locator is on
		*   page one (the default).
		*/
		int Pe, Pb, Pr, Pc /* , Pp */;

		p = tp + slen;

		/* get event status */
		Pe = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* get button status */
		Pb = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* get row status */
		Pr = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* get column status */
		Pc = getdigits(&p);

		/* the page parameter is optional */
		if (*p == ';')
		{
		    p++;
		    /* Pp = getdigits(&p); */
		}
		/* else
		    Pp = 0; */
		if (*p++ != '&')
		    return -1;
		if (*p++ != 'w')
		    return -1;

		mouse_code = 0;
		switch (Pe)
		{
		case  0: return -1; /* position request while unavailable */
		case  1: /* a response to a locator position request includes
			    the status of all buttons */
			 Pb &= 7;   /* mask off and ignore fourth button */
			 if (Pb & 4)
			     mouse_code  = MOUSE_LEFT;
			 if (Pb & 2)
			     mouse_code  = MOUSE_MIDDLE;
			 if (Pb & 1)
			     mouse_code  = MOUSE_RIGHT;
			 if (Pb)
			 {
			     held_button = mouse_code;
			     mouse_code |= MOUSE_DRAG;
			     WantQueryMouse = 1;
			 }
			 is_drag = TRUE;
			 showmode();
			 break;
		case  2: mouse_code = MOUSE_LEFT;
			 WantQueryMouse = 1;
			 break;
		case  3: mouse_code = MOUSE_RELEASE | MOUSE_LEFT;
			 break;
		case  4: mouse_code = MOUSE_MIDDLE;
			 WantQueryMouse = 1;
			 break;
		case  5: mouse_code = MOUSE_RELEASE | MOUSE_MIDDLE;
			 break;
		case  6: mouse_code = MOUSE_RIGHT;
			 WantQueryMouse = 1;
			 break;
		case  7: mouse_code = MOUSE_RELEASE | MOUSE_RIGHT;
			 break;
		case  8: return -1; /* fourth button down */
		case  9: return -1; /* fourth button up */
		case 10: return -1; /* mouse outside of filter rectangle */
		default: return -1; /* should never occur */
		}

		/* we ignore the Pp word */
		mouse_col = Pc - 1;
		mouse_row = Pr - 1;

		slen += (p - (tp + slen));
	    }
# endif /* DEC_MOUSE */

	    /* Interpret the mouse code */
	    current_button = (mouse_code & MOUSE_CLICK_MASK);
	    if (current_button == MOUSE_RELEASE)
	    {
		/*
		 * If we get a mouse drag or release event when
		 * there is no mouse button held down (held_button ==
		 * MOUSE_RELEASE), produce a K_IGNORE below.
		 * (can happen when you hold down two buttons
		 * and then let them go, or click in the menu bar, but not
		 * on a menu, and drag into the text).
		 */
		if ((mouse_code & MOUSE_DRAG) == MOUSE_DRAG)
		    is_drag = TRUE;
		current_button = held_button;
	    }
	    else if (wheel_code == 0)
	    {
# if defined(UNIX) && defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)
#  ifdef GPM_MOUSE
		extern int gpm_flag; /* gpm library variable */
#   ifdef USE_GUI
		/*
		 * Only for Unix, when GUI or gpm is not active, we handle
		 * multi-clicks here.
		 */
		if (gpm_flag == 0 && !gui.in_use)
#   else
		if (gpm_flag == 0)
#   endif
#  else
#   ifdef USE_GUI
		if (!gui.in_use)
#   endif
#  endif
		{
		    /*
		     * Compute the time elapsed since the previous mouse click.
		     */
		    gettimeofday(&mouse_time, NULL);
		    timediff = (mouse_time.tv_usec -
					      orig_mouse_time.tv_usec) / 1000;
		    if (timediff < 0)
			--orig_mouse_time.tv_sec;
		    timediff += (mouse_time.tv_sec -
					       orig_mouse_time.tv_sec) * 1000;
		    orig_mouse_time = mouse_time;
		    if (mouse_code == orig_mouse_code &&
			    timediff < p_mouset &&
			    orig_num_clicks != 4 &&
			    orig_mouse_col == mouse_col &&
			    orig_mouse_row == mouse_row &&
			    orig_topline == curwin->w_topline)
			++orig_num_clicks;
		    else
			orig_num_clicks = 1;
		    orig_mouse_col = mouse_col;
		    orig_mouse_row = mouse_row;
		    orig_topline = curwin->w_topline;
		}
#  if defined(USE_GUI) || defined(GPM_MOUSE)
		else
		    orig_num_clicks = NUM_MOUSE_CLICKS(mouse_code);
#  endif
# else
		orig_num_clicks = NUM_MOUSE_CLICKS(mouse_code);
# endif
		is_click = TRUE;
		orig_mouse_code = mouse_code;
	    }
	    if (!is_drag)
		held_button = mouse_code & MOUSE_CLICK_MASK;

	    /*
	     * Translate the actual mouse event into a pseudo mouse event.
	     * First work out what modifiers are to be used.
	     */
	    modifiers = 0x0;
	    if (orig_mouse_code & MOUSE_SHIFT)
		modifiers |= MOD_MASK_SHIFT;
	    if (orig_mouse_code & MOUSE_CTRL)
		modifiers |= MOD_MASK_CTRL;
	    if (orig_mouse_code & MOUSE_ALT)
		modifiers |= MOD_MASK_ALT;
	    if (orig_num_clicks == 2)
		modifiers |= MOD_MASK_2CLICK;
	    else if (orig_num_clicks == 3)
		modifiers |= MOD_MASK_3CLICK;
	    else if (orig_num_clicks == 4)
		modifiers |= MOD_MASK_4CLICK;

	    /* Add the modifier codes to our string */
	    if (modifiers != 0)
	    {
		string[new_slen++] = K_SPECIAL;
		string[new_slen++] = (int)KS_MODIFIER;
		string[new_slen++] = modifiers;
	    }

	    /* Work out our pseudo mouse event */
	    key_name[0] = (int)KS_EXTRA;
	    if (wheel_code != 0)
		key_name[1] = (wheel_code & 1)
					? (int)KE_MOUSEUP : (int)KE_MOUSEDOWN;
	    else
		key_name[1] = get_pseudo_mouse_code(current_button,
							   is_click, is_drag);
	}
#endif /* USE_MOUSE */

#ifdef USE_GUI
	/*
	 * If using the GUI, then we get menu and scrollbar events.
	 *
	 * A menu event is encoded as K_SPECIAL, KS_MENU, K_FILLER followed by
	 * four bytes which are to be taken as a pointer to the VimMenu
	 * structure.
	 *
	 * A scrollbar event is K_SPECIAL, KS_SCROLLBAR, K_FILLER followed by
	 * one byte representing the scrollbar number, and then four bytes
	 * representing a long_u which is the new value of the scrollbar.
	 *
	 * A horizontal scrollbar event is K_SPECIAL, KS_HORIZ_SCROLLBAR,
	 * K_FILLER followed by four bytes representing a long_u which is the
	 * new value of the scrollbar.
	 */
	else if (key_name[0] == (int)KS_MENU)
	{
	    num_bytes = get_long_from_buf(tp + slen, &val);
	    if (num_bytes == -1)
		return -1;
	    current_menu = (VimMenu *)val;
	    slen += num_bytes;
	}
	else if (key_name[0] == (int)KS_SCROLLBAR)
	{
	    /* Get the last scrollbar event in the queue of the same type */
	    j = 0;
	    for (i = 0; tp[j] == CSI && tp[j + 1] == KS_SCROLLBAR
						     && tp[j + 2] != NUL; ++i)
	    {
		j += 3;
		num_bytes = get_bytes_from_buf(tp + j, bytes, 1);
		if (num_bytes == -1)
		    break;
		if (i == 0)
		    current_scrollbar = (int)bytes[0];
		else if (current_scrollbar != (int)bytes[0])
		    break;
		j += num_bytes;
		num_bytes = get_long_from_buf(tp + j, &val);
		if (num_bytes == -1)
		    break;
		scrollbar_value = val;
		j += num_bytes;
		slen = j;
	    }
	    if (i == 0)		/* not enough characters to make one */
		return -1;
	}
	else if (key_name[0] == (int)KS_HORIZ_SCROLLBAR)
	{
	    /* Get the last horiz. scrollbar event in the queue */
	    j = 0;
	    for (i = 0; tp[j] == CSI && tp[j + 1] == KS_HORIZ_SCROLLBAR
						     && tp[j + 2] != NUL; ++i)
	    {
		j += 3;
		num_bytes = get_long_from_buf(tp + j, &val);
		if (num_bytes == -1)
		    break;
		scrollbar_value = val;
		j += num_bytes;
		slen = j;
	    }
	    if (i == 0)		/* not enough characters to make one */
		return -1;
	}
#endif /* USE_GUI */
	/* Finally, add the special key code to our string */
	if (key_name[1] == NUL)
	    string[new_slen++] = key_name[0];	/* from ":set <M-b>=xx" */
	else
	{
	    string[new_slen++] = K_SPECIAL;
	    string[new_slen++] = key_name[0];
	    string[new_slen++] = key_name[1];
	}
	string[new_slen] = NUL;
	extra = new_slen - slen;
	if (buf == NULL)
	{
	    if (extra < 0)
		/* remove matched chars, taking care of noremap */
		del_typebuf(-extra, offset);
	    else if (extra > 0)
		/* insert the extra space we need */
		ins_typebuf(string + slen, FALSE, offset, FALSE);

	    /*
	     * Careful: del_typebuf() and ins_typebuf() may have
	     * reallocated typebuf[]
	     */
	    mch_memmove(typebuf + typeoff + offset, string, (size_t)new_slen);
	}
	else
	{
	    if (extra < 0)
		/* remove matched characters */
		mch_memmove(buf + offset, buf + offset - extra,
					   (size_t)(buflen + offset + extra));
	    else if (extra > 0)
		/* insert the extra space we need */
		mch_memmove(buf + offset + extra, buf + offset,
						   (size_t)(buflen - offset));
	    mch_memmove(buf + offset, string, (size_t)new_slen);
	}
	return (len + extra + offset);
    }
    return 0;			    /* no match found */
}

/*
 * Replace any terminal code strings in from[] with the equivalent internal
 * vim representation.	This is used for the "from" and "to" part of a
 * mapping, and the "to" part of a menu command.
 * Any strings like "<C-UP>" are also replaced, unless 'cpoptions' contains
 * '<'.
 * K_SPECIAL by itself is replaced by K_SPECIAL KS_SPECIAL K_FILLER.
 *
 * The replacement is done in result[] and finally copied into allocated
 * memory. If this all works well *bufp is set to the allocated memory and a
 * pointer to it is returned. If something fails *bufp is set to NULL and from
 * is returned.
 *
 * CTRL-V characters are removed.  When "from_part" is TRUE, a trailing CTRL-V
 * is included, otherwise it is removed (for ":map xx ^V", maps xx to
 * nothing).  When 'cpoptions' does not contain 'B', a backslash can be used
 * instead of a CTRL-V.
 */
    char_u  *
replace_termcodes(from, bufp, from_part, do_lt)
    char_u	*from;
    char_u	**bufp;
    int		from_part;
    int		do_lt;		/* also translate <lt> */
{
    int		i;
    int		slen;
    int		key;
    int		dlen = 0;
    char_u	*src;
    int		do_backslash;	/* backslash is a special character */
    int		do_special;	/* recognize <> key codes */
    int		do_key_code;	/* recognize raw key codes */
    char_u	*result;	/* buffer for resulting string */

    do_backslash = (vim_strchr(p_cpo, CPO_BSLASH) == NULL);
    do_special = (vim_strchr(p_cpo, CPO_SPECI) == NULL);
    do_key_code = (vim_strchr(p_cpo, CPO_KEYCODE) == NULL);

    /*
     * Allocate space for the translation.  Worst case a single character is
     * replaced by 6 bytes (shifted special key), plus a NUL at the end.
     */
    result = alloc((unsigned)STRLEN(from) * 6 + 1);
    if (result == NULL)		/* out of memory */
    {
	*bufp = NULL;
	return from;
    }

    src = from;

    /*
     * Check for #n at start only: function key n
     */
    if (from_part && src[0] == '#' && isdigit(src[1]))	    /* function key */
    {
	result[dlen++] = K_SPECIAL;
	result[dlen++] = 'k';
	if (src[1] == '0')
	    result[dlen++] = ';';	/* #0 is F10 is "k;" */
	else
	    result[dlen++] = src[1];	/* #3 is F3 is "k3" */
	src += 2;
    }

    /*
     * Copy each byte from *from to result[dlen]
     */
    while (*src != NUL)
    {
	/*
	 * If 'cpoptions' does not contain '<', check for special key codes,
	 * like "<C-S-MouseLeft>"
	 */
	if (do_special && (do_lt || STRNCMP(src, "<lt>", 4)))
	{
	    slen = trans_special(&src, result + dlen, TRUE);
	    if (slen)
	    {
		dlen += slen;
		continue;
	    }
	}

	/*
	 * If 'cpoptions' does not contain 'k', see if it's an actual key-code.
	 * Note that this is also checked after replacing the <> form.
	 * Single character codes are NOT replaced (e.g. ^H or DEL), because
	 * it could be a character in the file.
	 */
	if (do_key_code)
	{
	    i = find_term_bykeys(src);
	    if (i >= 0)
	    {
		result[dlen++] = K_SPECIAL;
		result[dlen++] = termcodes[i].name[0];
		result[dlen++] = termcodes[i].name[1];
		src += termcodes[i].len;
		/* If terminal code matched, continue after it. */
		continue;
	    }
	}

	/*
	 * If the character is K_SPECIAL, replace it with K_SPECIAL KS_SPECIAL
	 * K_FILLER.
	 */
	if (*src == K_SPECIAL)
	{
	    result[dlen++] = K_SPECIAL;
	    result[dlen++] = (int)KS_SPECIAL;
	    result[dlen++] = K_FILLER;
	    ++src;
	    continue;
	}

	/*
	 * Remove CTRL-V and ignore the next character.
	 * For "from" side the CTRL-V at the end is included, for the "to"
	 * part it is removed.
	 * If 'cpoptions' does not contain 'B', also accept a backslash.
	 */
	key = *src;
	if (key == Ctrl('V') || (do_backslash && key == '\\'))
	{
	    ++src;				/* skip CTRL-V or backslash */
	    if (*src == NUL)
	    {
		if (from_part)
		    result[dlen++] = key;
		break;
	    }
	}

#ifdef MULTI_BYTE
	/* skip multibyte char correctly */
	if (is_dbcs && IsLeadByte(*src) && src[1] != NUL)
	    result[dlen++] = *src++;
#endif
	result[dlen++] = *src++;
    }
    result[dlen] = NUL;

    /*
     * Copy the new string to allocated memory.
     * If this fails, just return from.
     */
    if ((*bufp = vim_strsave(result)) != NULL)
	from = *bufp;
    vim_free(result);
    return from;
}

/*
 * Find a termcode with keys 'src' (must be NUL terminated).
 * Return the index in termcodes[], or -1 if not found.
 */
    int
find_term_bykeys(src)
    char_u	*src;
{
    int		i;
    int		slen;

    for (i = 0; i < tc_len; ++i)
    {
	slen = termcodes[i].len;
	if (slen > 1 && STRNCMP(termcodes[i].code, src, (size_t)slen) == 0)
	    return i;
    }
    return -1;
}

/*
 * Gather the first characters in the terminal key codes into a string.
 * Used to speed up check_termcode().
 */
    static void
gather_termleader()
{
    int	    i;
    int	    len = 0;

#ifdef USE_GUI
    if (gui.in_use)
	termleader[len++] = CSI;    /* the GUI codes are not in termcodes[] */
#endif
    termleader[len] = NUL;

    for (i = 0; i < tc_len; ++i)
	if (vim_strchr(termleader, termcodes[i].code[0]) == NULL)
	{
	    termleader[len++] = termcodes[i].code[0];
	    termleader[len] = NUL;
	}

    need_gather = FALSE;
}

/*
 * Show all termcodes (for ":set termcap")
 * This code looks a lot like showoptions(), but is different.
 */
    void
show_termcodes()
{
    int		    col;
    int		    *items;
    int		    item_count;
    int		    run;
    int		    row, rows;
    int		    cols;
    int		    i;
    int		    len;

#define INC 27	    /* try to make three columns */
#define GAP 2	    /* spaces between columns */

    if (tc_len == 0)	    /* no terminal codes (must be GUI) */
	return;
    items = (int *)alloc((unsigned)(sizeof(int) * tc_len));
    if (items == NULL)
	return;

    /* Highlight title */
    MSG_PUTS_TITLE("\n--- Terminal keys ---");

    /*
     * do the loop two times:
     * 1. display the short items (non-strings and short strings)
     * 2. display the long items (strings)
     */
    for (run = 1; run <= 2 && !got_int; ++run)
    {
	/*
	 * collect the items in items[]
	 */
	item_count = 0;
	for (i = 0; i < tc_len; i++)
	{
	    len = show_one_termcode(termcodes[i].name,
						    termcodes[i].code, FALSE);
	    if ((len <= INC - GAP && run == 1) || (len > INC - GAP && run == 2))
		items[item_count++] = i;
	}

	/*
	 * display the items
	 */
	if (run == 1)
	{
	    cols = (Columns + GAP) / INC;
	    if (cols == 0)
		cols = 1;
	    rows = (item_count + cols - 1) / cols;
	}
	else	/* run == 2 */
	    rows = item_count;
	for (row = 0; row < rows && !got_int; ++row)
	{
	    msg_putchar('\n');			/* go to next line */
	    if (got_int)			/* 'q' typed in more */
		break;
	    col = 0;
	    for (i = row; i < item_count; i += rows)
	    {
		msg_col = col;			/* make columns */
		show_one_termcode(termcodes[items[i]].name,
					      termcodes[items[i]].code, TRUE);
		col += INC;
	    }
	    out_flush();
	    ui_breakcheck();
	}
    }
    vim_free(items);
}

/*
 * Show one termcode entry.
 * Output goes into IObuff[]
 */
    int
show_one_termcode(name, code, printit)
    char_u  *name;
    char_u  *code;
    int	    printit;
{
    char_u	*p;
    int		len;

    if (name[0] > '~')
    {
	IObuff[0] = ' ';
	IObuff[1] = ' ';
	IObuff[2] = ' ';
	IObuff[3] = ' ';
    }
    else
    {
	IObuff[0] = 't';
	IObuff[1] = '_';
	IObuff[2] = name[0];
	IObuff[3] = name[1];
    }
    IObuff[4] = ' ';

    p = get_special_key_name(TERMCAP2KEY(name[0], name[1]), 0);
    if (p[1] != 't')
	STRCPY(IObuff + 5, p);
    else
	IObuff[5] = NUL;
    len = STRLEN(IObuff);
    do
	IObuff[len++] = ' ';
    while (len < 17);
    IObuff[len] = NUL;
    if (code == NULL)
	len += 4;
    else
	len += vim_strsize(code);

    if (printit)
    {
	msg_puts(IObuff);
	if (code == NULL)
	    msg_puts((char_u *)"NULL");
	else
	    msg_outtrans(code);
    }
    return len;
}

#if defined(CMDLINE_COMPL) || defined(PROTO)
/*
 * Translate an internal mapping/abbreviation representation into the
 * corresponding external one recognized by :map/:abbrev commands;
 * respects the current B/k/< settings of 'cpoption'.
 *
 * This function is called when expanding mappings/abbreviations on the
 * command-line, and for building the "Ambiguous mapping..." error messæge.
 *
 * It uses a growarray to build the translation string since the
 * latter can be wider than the original description. The caller has to
 * free the string afterwards.
 *
 * Returns NULL when there is a problem.
 */
    char_u *
translate_mapping(str, expmap)
    char_u	*str;
    int		expmap;  /* TRUE when expanding mappings on command-line */
{
    struct growarray	ga;
    int			c;
    int			modifiers;
    int			cpo_bslash;
    int			cpo_special;
    int			cpo_keycode;

    ga_init(&ga);
    ga.ga_itemsize = 1;
    ga.ga_growsize = 40;

    cpo_bslash = (vim_strchr(p_cpo, CPO_BSLASH) != NULL);
    cpo_special = (vim_strchr(p_cpo, CPO_SPECI) != NULL);
    cpo_keycode = (vim_strchr(p_cpo, CPO_KEYCODE) == NULL);

    for (; *str; ++str)
    {
	c = *str;
	if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
	{
	    modifiers = 0;
	    if (str[1] == KS_MODIFIER)
	    {
		str++;
		modifiers = *++str;
		c = *++str;
	    }
	    if (cpo_special && cpo_keycode && c == K_SPECIAL && !modifiers)
	    {
		int	i;

		/* try to find special key in termcodes */
		for (i = 0; i < tc_len; ++i)
		    if (termcodes[i].name[0] == str[1]
					    && termcodes[i].name[1] == str[2])
			break;
		if (i < tc_len)
		{
		    ga_concat(&ga, termcodes[i].code);
		    str += 2;
		    continue; /* for (str) */
		}
	    }
	    if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
	    {
		if (expmap && cpo_special)
		{
		    ga_clear(&ga);
		    return NULL;
		}
		c = TO_SPECIAL(str[1], str[2]);
		if (c == K_ZERO)	/* display <Nul> as ^@ */
		    c = NUL;
		str += 2;
	    }
	    if (IS_SPECIAL(c) || modifiers)	/* special key */
	    {
		if (expmap && cpo_special)
		{
		    ga_clear(&ga);
		    return NULL;
		}
		ga_concat(&ga, get_special_key_name(c, modifiers));
		continue; /* for (str) */
	    }
	}
	if (c == ' ' || c == '\t' || c == Ctrl('J') || c == Ctrl('V')
	    || (c == '<' && !cpo_special) || (c == '\\' && !cpo_bslash))
	    ga_append(&ga, cpo_bslash ? Ctrl('V') : '\\');
	if (c)
	    ga_append(&ga, c);
    }
    ga_append(&ga, NUL);
    return (char_u *)(ga.ga_data);
}
#endif

#if (defined(WIN32) && !defined(USE_GUI)) || defined(PROTO)
static char ksme_str[20];
static char ksmr_str[20];
static char ksmd_str[20];

/*
 * For Win32 console: update termcap codes for existing console attributes.
 */
    void
update_tcap(attr)
    int attr;
{
    struct builtin_term *p;

    p = find_builtin_term(DEFAULT_TERM);
    sprintf(ksme_str, "\033|%dm", attr);
    sprintf(ksmd_str, "\033|%dm", attr | 0x08);  /* FOREGROUND_INTENSITY */
    sprintf(ksmr_str, "\033|%dm", ((attr & 0x0F) << 4) | ((attr & 0xF0) >> 4));

    while (p->bt_string != NULL)
    {
      if (p->bt_entry == (int)KS_ME)
	  p->bt_string = &ksme_str[0];
      else if (p->bt_entry == (int)KS_MR)
	  p->bt_string = &ksmr_str[0];
      else if (p->bt_entry == (int)KS_MD)
	  p->bt_string = &ksmd_str[0];
      ++p;
    }
}
#endif
