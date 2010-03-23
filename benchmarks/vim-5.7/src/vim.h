/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#ifndef VIM__H
# define VIM__H

/* use fastcall for Borland, when compiling for Win32 (not for DOS16) */
#if defined(__BORLANDC__) && defined(WIN32) && !defined(DEBUG)
# pragma option -pr
#endif

/* ============ the header file puzzle (ca. 50-100 pieces) ========= */

#ifdef HAVE_CONFIG_H	/* GNU autoconf (or something else) was here */
# include "config.h"
# define HAVE_PATHDEF

/*
 * Check if configure correcly managed to find sizeof(int).  If this failed,
 * it becomes zero.  This is likely a problem of not being able to run the
 * test program.  Other items from configure may also be wrong then!
 */
# if (SIZEOF_INT == 0)
    Error: configure did not run properly.  Check config.log.
# endif
#endif

#ifdef __EMX__		/* hand-edited config.h for OS/2 with EMX */
# include "os_os2_cfg.h"
#endif

#if defined macintosh
# define USE_GUI_MAC	    /* mandatory */
#endif
#if defined(USE_GUI_MOTIF) \
    || defined(USE_GUI_GTK) \
    || defined(USE_GUI_ATHENA) \
    || defined(USE_GUI_MAC) \
    || defined(USE_GUI_WIN32) \
    || defined(USE_GUI_BEOS) \
    || defined(USE_GUI_AMIGA)
# ifndef USE_GUI
#  define USE_GUI
# endif
#endif

#if defined(USE_GUI_WIN32) || defined(USE_GUI_WIN16)
# define USE_GUI_MSWIN
#endif
#if defined(WIN32) || defined(WIN16)
# define MSWIN
#endif

#include "feature.h"	/* #defines for optionals and features */

/*
 * Find out if function definitions should include argument types
 */
#ifdef AZTEC_C
# include <functions.h>
# define __ARGS(x)  x
#endif

#ifdef SASC
# include <clib/exec_protos.h>
# define __ARGS(x)  x
#endif

#ifdef _DCC
# include <clib/exec_protos.h>
# define __ARGS(x)  x
#endif

#ifdef __TURBOC__
# define __ARGS(x) x
#endif

#ifdef __BEOS__
# include "os_beos.h"
# define __ARGS(x)  x
#endif

#if defined(UNIX) || defined(__EMX__)
# include "os_unix.h"	    /* bring lots of system header files */
#endif

#ifdef VMS
# include "os_vms.h"
#endif

#ifndef __ARGS
# if defined(__STDC__) || defined(__GNUC__) || defined(WIN32)
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

/* __ARGS and __PARMS are the same thing. */
#ifndef __PARMS
# define __PARMS(x) __ARGS(x)
#endif

#ifdef UNIX
# include "osdef.h"	/* bring missing declarations in */
#endif

#ifdef __EMX__
# define    getcwd  _getcwd2
# define    chdir   _chdir2
# undef	    CHECK_INODE
#endif

#ifdef AMIGA
# include "os_amiga.h"
#endif

#ifdef MSDOS
# include "os_msdos.h"
#endif

#ifdef WIN16
# include "os_win16.h"
#endif

#ifdef WIN32
# include "os_win32.h"
#endif

#ifdef __MINT__
# include "os_mint.h"
#endif

#ifdef macintosh
# include "os_mac.h"
#endif

#ifdef RISCOS
# include "os_riscos.h"
#endif

/*
 * Maximum length of a path (for non-unix systems) Make it a bit long, to stay
 * on the safe side.  But not too long to put on the stack.
 */
#ifndef MAXPATHL
# ifdef MAXPATHLEN
#  define MAXPATHL  MAXPATHLEN
# else
#  define MAXPATHL  256
# endif
#endif

#define NUMBUFLEN 30	    /* length of a buffer to store a number in ASCII */

/*
 * Shorthand for unsigned variables. Many systems, but not all, have u_char
 * already defined, so we use char_u to avoid trouble.
 */
typedef unsigned char	char_u;
typedef unsigned short	short_u;
typedef unsigned int	int_u;
typedef unsigned long	long_u;

#ifndef UNIX		    /* For Unix this is included in os_unix.h */
# include <stdio.h>
# include <ctype.h>
#endif

#include "ascii.h"
#include "keymap.h"
#include "term.h"
#include "macros.h"

#ifdef LATTICE
# include <sys/types.h>
# include <sys/stat.h>
#endif
#ifdef _DCC
# include <sys/stat.h>
#endif
#if defined MSDOS  ||  defined MSWIN
# include <sys/stat.h>
#endif

/*
 * Allow other (non-unix) systems to configure themselves now
 * These are also in os_unix.h, because osdef.sh needs them there.
 */
#ifndef UNIX
/* Note: Some systems need both string.h and strings.h (Savage).  If the
 * system can't handle this, define NO_STRINGS_WITH_STRING_H. */
# ifdef HAVE_STRING_H
#  include <string.h>
# endif
# if defined(HAVE_STRINGS_H) && !defined(NO_STRINGS_WITH_STRING_H)
#   include <strings.h>
# endif
# ifdef HAVE_STAT_H
#  include <stat.h>
# endif
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif /* NON-UNIX */

/* ================ end of the header file puzzle =============== */

/*
 * flags for update_screen()
 * The higher the value, the higher the priority
 */
#define VALID			10  /* buffer not changed */
#define INVERTED		20  /* redisplay inverted part */
#define VALID_TO_CURSCHAR	30  /* line at/below cursor changed */
#define VALID_BEF_CURSCHAR	35  /* line just above cursor changed */
#define NOT_VALID		40  /* buffer changed somewhere */
#define CLEAR			50  /* screen messed up, clear it */

/*
 * Hints used to optimize screen updating.
 */
#define HINT_NONE	0	    /* no current hint */
#define HINT_DEL_CHAR	1	    /* delete character */
#define HINT_INS_CHAR	2	    /* insert character */

/*
 * Terminal highlighting attribute bits.
 * Attibutes above HL_ALL are used for syntax highlighting.
 */
#define HL_NORMAL		0x00
#define HL_INVERSE		0x01
#define HL_BOLD			0x02
#define HL_ITALIC		0x04
#define HL_UNDERLINE		0x08
#define HL_STANDOUT		0x10
#define HL_ALL			0x1f

/* special attribute addition: Put message in history */
#define MSG_HIST		0x1000

/*
 * values for State
 *
 * The lower byte is used to distinguish normal/visual/op_pending and cmdline/
 * insert+replace mode.  This is used for mapping.  If none of these bits are
 * set, no mapping is done.
 * The upper byte is used to distinguish between other states.
 */
#define NORMAL		0x01	/* Normal mode, command expected */
#define VISUAL		0x02	/* Visual mode - use get_real_state() */
#define OP_PENDING	0x04	/* Normal mode, operator is pending - use
				   get_real_state() */
#define CMDLINE		0x08	/* Editing command line */
#define INSERT		0x10	/* Insert mode */

#define NORMAL_BUSY	(0x100 + NORMAL) /* Normal mode, busy with a command */
#define REPLACE		(0x200 + INSERT) /* Replace mode */
#define VREPLACE	(0x300 + INSERT) /* Virtual replace mode */
#define HITRETURN	(0x600 + NORMAL) /* waiting for return or command */
#define ASKMORE		0x700	/* Asking if you want --more-- */
#define SETWSIZE	0x800	/* window size has changed */
#define ABBREV		0x900	/* abbreviation instead of mapping */
#define EXTERNCMD	0xa00	/* executing an external command */
#define SHOWMATCH	(0xb00 + INSERT) /* show matching paren */
#define CONFIRM		0xc00	/* ":confirm" prompt */

/* directions */
#define FORWARD			1
#define BACKWARD		(-1)
#define FORWARD_FILE		3

/* return values for functions */
#define OK			1
#define FAIL			0

/* flags for b_flags */
#define BF_RECOVERED	0x01	/* buffer has been recovered */
#define BF_CHECK_RO	0x02	/* need to check readonly when loading file
				   into buffer (set by ":e", may be reset by
				   ":buf" */
#define BF_NEVERLOADED	0x04	/* file has never been loaded into buffer,
				   many variables still need to be set */
#define BF_NOTEDITED	0x08	/* Set when file name is changed after
				   starting to edit, reset when file is
				   written out. */
#define BF_NEW		0x10	/* file didn't exist when editing started */
#define BF_NEW_W	0x20	/* Warned for BF_NEW and file created */
#define BF_READERR	0x40	/* got errors while reading the file */

/* Mask to check for flags that prevent normal writing */
#define BF_WRITE_MASK	(BF_NOTEDITED + BF_NEW + BF_READERR)

/*
 * values for command line completion
 */
#define CONTEXT_UNKNOWN		(-2)
#define EXPAND_UNSUCCESSFUL	(-1)
#define EXPAND_NOTHING		0
#define EXPAND_COMMANDS		1
#define EXPAND_FILES		2
#define EXPAND_DIRECTORIES	3
#define EXPAND_SETTINGS		4
#define EXPAND_BOOL_SETTINGS	5
#define EXPAND_TAGS		6
#define EXPAND_OLD_SETTING	7
#define EXPAND_HELP		8
#define EXPAND_BUFFERS		9
#define EXPAND_EVENTS		10
#define EXPAND_MENUS		11
#define EXPAND_SYNTAX		12
#define EXPAND_HIGHLIGHT	13
#define EXPAND_AUGROUP		14
#define EXPAND_USER_VARS	15
#define EXPAND_MAPPINGS		16
#define EXPAND_TAGS_LISTFILES	17
#define EXPAND_FUNCTIONS	18
#define EXPAND_USER_FUNC	19
#define EXPAND_EXPRESSION	20
#define EXPAND_MENUNAMES	21
#define EXPAND_USER_COMMANDS	22
#define EXPAND_USER_CMD_FLAGS	23
#define EXPAND_USER_NARGS	24
#define EXPAND_USER_COMPLETE	25

/* Values for nextwild() and ExpandOne().  See ExpandOne() for meaning. */
#define WILD_FREE		1
#define WILD_EXPAND_FREE	2
#define WILD_EXPAND_KEEP	3
#define WILD_NEXT		4
#define WILD_PREV		5
#define WILD_ALL		6
#define WILD_LONGEST		7

#define WILD_LIST_NOTFOUND	1
#define WILD_HOME_REPLACE	2
#define WILD_USE_NL		4
#define WILD_NO_BEEP		8
#define WILD_ADD_SLASH		16
#define WILD_KEEP_ALL		32
#define WILD_SILENT		64
#define WILD_ESCAPE		128

/* Flags for expand_wildcards() */
#define EW_DIR		1	/* include directory names */
#define EW_FILE		2	/* include file names */
#define EW_NOTFOUND	4	/* include not found names */
#define EW_ADDSLASH	8	/* append slash to directory name */
#define EW_KEEPALL	16	/* keep all matches */
#define EW_SILENT	32	/* don't print "1 returned" from shell */
/* Note: mostly EW_NOTFOUND and EW_SILENT are mutually exclusive: EW_NOTFOUND
 * is used when executing commands and EW_SILENT for interactive expanding. */

#ifdef NO_EXPANDPATH
# define gen_expand_wildcards mch_expand_wildcards
#endif

/* Values for the find_pattern_in_path() function args 'type' and 'action': */
#define FIND_ANY	1
#define FIND_DEFINE	2
#define CHECK_PATH	3

#define ACTION_SHOW	1
#define ACTION_GOTO	2
#define ACTION_SPLIT	3
#define ACTION_SHOW_ALL	4
#ifdef INSERT_EXPAND
# define ACTION_EXPAND	5
#endif

/* Values for 'options' argument in do_search() and searchit() */
#define SEARCH_REV    0x01  /* go in reverse of previous dir. */
#define SEARCH_ECHO   0x02  /* echo the search command and handle options */
#define SEARCH_MSG    0x0c  /* give messages (yes, it's not 0x04) */
#define SEARCH_NFMSG  0x08  /* give all messages except not found */
#define SEARCH_OPT    0x10  /* interpret optional flags */
#define SEARCH_HIS    0x20  /* put search pattern in history */
#define SEARCH_END    0x40  /* put cursor at end of match */
#define SEARCH_NOOF   0x80  /* don't add offset to position */
#define SEARCH_START 0x100  /* start search without col offset */
#define SEARCH_MARK  0x200  /* set previous context mark */
#define SEARCH_KEEP  0x400  /* keep previous search pattern */

/* Values for find_ident_under_cursor() */
#define FIND_IDENT	1	/* find identifier (word) */
#define FIND_STRING	2	/* find any string (WORD) */

/* Values for get_file_name_in_path() */
#define FNAME_MESS	1	/* give error message */
#define FNAME_EXP	2	/* expand to path */
#define FNAME_HYP	4	/* check for hypertext link */

/* Values for buflist_getfile() */
#define GETF_SETMARK	0x01	/* set pcmark before jumping */
#define GETF_ALT	0x02	/* jumping to alternate file (not buf num) */
#define GETF_SWITCH	0x04	/* respect 'switchbuf' settings when jumping */

/* Values for in_indentkeys() */
#define KEY_OPEN_FORW	0x101
#define KEY_OPEN_BACK	0x102

/* Values for mch_call_shell() second argument */
#define SHELL_FILTER	1	/* filtering text */
#define SHELL_EXPAND	2	/* expanding wildcards */
#define SHELL_COOKED	4	/* set term to cooked mode */
#define SHELL_DOOUT	8	/* redirecting output */
#define SHELL_SILENT	16	/* don't print error returned by command */

/* Values for readfile() flags */
#define READ_NEW	0x01	/* read a file into a new buffer */
#define READ_FILTER	0x02	/* read filter output */
#define READ_STDIN	0x04	/* read from stdin */

/* Values for change_indent() */
#define INDENT_SET	1	/* set indent */
#define INDENT_INC	2	/* increase indent */
#define INDENT_DEC	3	/* decrease indent */

/* Values for flags argument for findmatchlimit() */
#define FM_BACKWARD	0x01	/* search backwards */
#define FM_FORWARD	0x02	/* search forwards */
#define FM_BLOCKSTOP	0x04	/* stop at start/end of block */
#define FM_SKIPCOMM	0x08	/* skip comments */

/* Values for action argument for do_buffer() */
#define DOBUF_GOTO	0	/* go to specified buffer */
#define DOBUF_SPLIT	1	/* split window and go to specified buffer */
#define DOBUF_UNLOAD	2	/* unload specified buffer(s) */
#define DOBUF_DEL	3	/* delete specified buffer(s) */
#define DOBUF_SWITCH	4	/* switch to open window containing buffer  */
				/* or split current window and go to buffer */

/* Values for start argument for do_buffer() */
#define DOBUF_CURRENT	0	/* "count" buffer from current buffer */
#define DOBUF_FIRST	1	/* "count" buffer from first buffer */
#define DOBUF_LAST	2	/* "count" buffer from last buffer */
#define DOBUF_MOD	3	/* "count" mod. buffer from current buffer */

/* Values for sub_cmd and which_pat argument for search_regcomp() */
/* Also used for which_pat argument for searchit() */
#define RE_SEARCH   0		/* save/use pat in/from search_pattern */
#define RE_SUBST    1		/* save/use pat in/from subst_pattern */
#define RE_BOTH	    2		/* save pat in both patterns */
#define RE_LAST	    2		/* use last used pattern if "pat" is NULL */

/* Return values for fullpathcmp() */
/* Note: can use (fullpathcmp() & FPC_SAME) to check for equal files */
#define FPC_SAME	1	/* both exist and are the same file. */
#define FPC_DIFF	2	/* both exist and are different files. */
#define FPC_NOTX	4	/* both don't exist. */
#define FPC_DIFFX	6	/* one of them doesn't exist. */
#define FPC_SAMEX	7	/* both don't exist and file names are same. */

/* flags for do_ecmd() */
#define ECMD_HIDE	0x01	/* don't free the current buffer */
#define ECMD_SET_HELP	0x02	/* set b_help flag of (new) buffer before
				   opening file */
#define ECMD_OLDBUF	0x04	/* use existing buffer if it exists */
#define ECMD_FORCEIT	0x08	/* ! used in Ex command */
#define ECMD_ADDBUF	0x10	/* don't edit, just add to buffer list */

/* for lnum argument in do_ecmd() */
#define ECMD_LASTL	(linenr_t)0	/* use last position in loaded file */
#define ECMD_LAST	(linenr_t)-1	/* use last position in all files */
#define ECMD_ONE	(linenr_t)1	/* use first line */

/* flags for do_cmdline() */
#define DOCMD_VERBOSE	0x01	/* included command in error message */
#define DOCMD_NOWAIT	0x02	/* don't call wait_return() and friends */
#define DOCMD_REPEAT	0x04	/* repeat exec. until getline() returns NULL */

/* flags for beginline() */
#define BL_WHITE	1	/* cursor on first non-white in the line */
#define BL_SOL		2	/* use 'sol' option */
#define BL_FIX		4	/* don't leave cursor on a NUL */

/* flags for mf_sync() */
#define MFS_ALL		1	/* also sync blocks with negative numbers */
#define MFS_STOP	2	/* stop syncing when a character is available */
#define MFS_FLUSH	4	/* flushed file to disk */
#define MFS_ZERO	8	/* only write block 0 */

/* flags for buf_copy_options() */
#define BCO_ENTER	1	/* going to enter the buffer */
#define BCO_ALWAYS	2	/* always copy the options */
#define BCO_NOHELP	4	/* don't touch the help related options */

/* flags for do_put() */
#define PUT_FIXINDENT	1	/* make indent look nice */
#define PUT_CURSEND	2	/* leave cursor after end of new text */

/*
 * There are four history tables:
 */
#define HIST_CMD    0		/* colon commands */
#define HIST_SEARCH 1		/* search commands */
#define HIST_EXPR   2		/* expressions (from entering = register) */
#define HIST_INPUT  3		/* input() lines */
#define HIST_COUNT  4		/* number of history tables */

/*
 * Flags for chartab[].
 */
#define CHAR_MASK	0x03	/* low two bits for size */
#define CHAR_IP		0x04	/* third bit set for printable chars */
#define CHAR_ID		0x08	/* fourth bit set for ID chars */
#define CHAR_IF		0x10	/* fifth bit set for file name chars */

/*
 * Values for do_tag().
 */
#define DT_TAG		1	/* jump to newer position or same tag again */
#define DT_POP		2	/* jump to older position */
#define DT_NEXT		3	/* jump to next match of same tag */
#define DT_PREV		4	/* jump to previous match of same tag */
#define DT_FIRST	5	/* jump to first match of same tag */
#define DT_LAST		6	/* jump to first match of same tag */
#define DT_SELECT	7	/* jump to selection from list */
#define DT_HELP		8	/* like DT_TAG, but no wildcards */
#define DT_JUMP		9	/* jump to new tag or selection from list */
#define DT_CSCOPE	10	/* cscope find command (like tjump) */

/*
 * flags for find_tags().
 */
#define TAG_HELP	1	/* only search for help tags */
#define TAG_NAMES	2	/* only return name of tag */
#define	TAG_REGEXP	4	/* use tag pattern as regexp */
#define	TAG_NOIC	8	/* don't always ignore case */
#ifdef USE_CSCOPE
# define TAG_CSCOPE	16	/* cscope tag */
#endif
#define TAG_VERBOSE	32	/* message verbosity */
#define TAG_INS_COMP	64	/* Currently doing insert completion */
#define TAG_MANY	200	/* When finding many tags (for completion),
				   find up to this many tags */

/*
 * Types of dialogs passed to do_vim_dialog().
 */
#define VIM_GENERIC	0
#define VIM_ERROR	1
#define VIM_WARNING	2
#define VIM_INFO	3
#define VIM_QUESTION	4
#define VIM_LAST_TYPE	4	/* sentinel value */

/*
 * Return values for functions like gui_yesnocancel()
 */
#define VIM_OK		1
#define VIM_YES		2
#define VIM_NO		3
#define VIM_CANCEL	4
#define VIM_ALL		5
#define VIM_DISCARDALL  6

/* Magic chars used in confirm dialog strings */
#define DLG_BUTTON_SEP	'\n'
#define DLG_HOTKEY_CHAR	'&'

/* Values for "starting" */
#define NO_SCREEN	2	/* no screen updating yet */
#define NO_BUFFERS	1	/* not all buffers loaded yet */
/*			0	   not starting anymore */

/* Values for swap_exists_action: what to do when swap file already exists */
#define SEA_NONE	0	/* don't use dialog */
#define SEA_DIALOG	1	/* use dialog when */
#define SEA_QUIT	2	/* quit editing the file */
#define SEA_RECOVER	3	/* recover the file */

/*
 * Minimal size for block 0 of a swap file.
 * NOTE: This depends on size of struct block0! It's not done with a sizeof(),
 * because struct block0 is defined in memline.c (Sorry).
 * The maximal block size is arbitrary.
 */
#define MIN_SWAP_PAGE_SIZE 1048
#define MAX_SWAP_PAGE_SIZE 50000

/*
 * Events for autocommands.
 */
enum auto_event
{
    EVENT_BUFCREATE = 0,	/* just after creating a buffer */
    EVENT_BUFDELETE,		/* just before deleting a buffer */
    EVENT_BUFENTER,		/* after entering a buffer */
    EVENT_BUFFILEPOST,		/* after renaming a buffer */
    EVENT_BUFFILEPRE,		/* before renaming a buffer */
    EVENT_BUFLEAVE,		/* before leaving a buffer */
    EVENT_BUFNEWFILE,		/* when creating a buffer for a new file */
    EVENT_BUFREADPOST,		/* after reading a buffer */
    EVENT_BUFREADPRE,		/* before reading a buffer */
    EVENT_BUFUNLOAD,		/* just before unloading a buffer */
    EVENT_BUFHIDDEN,		/* just after buffer becomes hidden */
    EVENT_BUFWRITEPOST,		/* after writing a buffer */
    EVENT_BUFWRITEPRE,		/* before writing a buffer */
    EVENT_FILEAPPENDPOST,	/* after appending to a file */
    EVENT_FILEAPPENDPRE,	/* before appending to a file */
    EVENT_FILECHANGEDSHELL,	/* after shell command that changed file */
    EVENT_FILEREADPOST,		/* after reading a file */
    EVENT_FILEREADPRE,		/* before reading a file */
    EVENT_FILETYPE,		/* new file type detected (user defined) */
    EVENT_FILEWRITEPOST,	/* after writing a file */
    EVENT_FILEWRITEPRE,		/* before writing a file */
    EVENT_FILTERREADPOST,	/* after reading from a filter */
    EVENT_FILTERREADPRE,	/* before reading from a filter */
    EVENT_FILTERWRITEPOST,	/* after writing to a filter */
    EVENT_FILTERWRITEPRE,	/* before writing to a filter */
    EVENT_FOCUSGAINED,		/* got the focus */
    EVENT_FOCUSLOST,		/* lost the focus to another app */
    EVENT_GUIENTER,		/* after starting the GUI */
    EVENT_STDINREADPOST,	/* after reading from stdin */
    EVENT_STDINREADPRE,		/* before reading from stdin */
    EVENT_SYNTAX,		/* syntax selected */
    EVENT_TERMCHANGED,		/* after changing 'term' */
    EVENT_USER,			/* user defined autocommand */
    EVENT_VIMENTER,		/* after starting Vim */
    EVENT_VIMLEAVE,		/* before exiting Vim */
    EVENT_VIMLEAVEPRE,		/* before exiting Vim and writing .viminfo */
    EVENT_WINENTER,		/* after entering a window */
    EVENT_WINLEAVE,		/* before leaving a window */
    EVENT_FILEENCODING,		/* after changing the file-encoding (set fe=) */
    EVENT_CURSORHOLD,
    NUM_EVENTS			/* MUST be the last one */
};

typedef enum auto_event EVENT_T;

/*
 * Values for index in highlight_attr[].
 * When making changes, also update the table in highlight_changed()!
 */
enum hlf_value
{
    HLF_8 = 0,	    /* Meta & special keys listed with ":map" */
    HLF_AT,	    /* @ and ~ characters at end of screen */
    HLF_D,	    /* directories in CTRL-D listing */
    HLF_E,	    /* error messages */
    HLF_H,	    /* obsolete, ignored */
    HLF_I,	    /* incremental search */
    HLF_L,	    /* last search string */
    HLF_M,	    /* "--More--" message */
    HLF_CM,	    /* Mode (e.g., "-- INSERT --") */
    HLF_N,	    /* line number for ":number" and ":#" commands */
    HLF_R,	    /* return to continue message and yes/no questions */
    HLF_S,	    /* status lines */
    HLF_SNC,	    /* status lines of not-current windows */
    HLF_T,	    /* Titles for output from ":set all", ":autocmd" etc. */
    HLF_V,	    /* Visual mode */
    HLF_VNC,	    /* Visual mode, autoselecting and not clipboard owner */
    HLF_W,	    /* warning messages */
    HLF_WM,	    /* Wildmenu highlight */
    HLF_COUNT	    /* MUST be the last one */
};

/* the HL_FLAGS must be in the same order as the HLF_ enums! */
#define HL_FLAGS {'8', '@', 'd', 'e', 'h', 'i', 'l', 'm', 'M', \
		  'n', 'r', 's', 'S', 't', 'v', 'V', 'w', 'W'}

/*
 * Boolean constants
 */
#ifndef TRUE
# define FALSE	0	    /* note: this is an int, not a long! */
# define TRUE	1
#endif

#define MAYBE	2	    /* sometimes used for a variant on TRUE */

/* May be returned by add_new_completion(): */
#define RET_ERROR		(-1)

/*
 * Operator IDs; The order must correspond to opchars[] in ops.c!
 */
#define OP_NOP		0	/* no pending operation */
#define OP_DELETE	1	/* "d"  delete operator */
#define OP_YANK		2	/* "y"  yank operator */
#define OP_CHANGE	3	/* "c"  change operator */
#define OP_LSHIFT	4	/* "<"  left shift operator */
#define OP_RSHIFT	5	/* ">"  right shift operator */
#define OP_FILTER	6	/* "!"  filter operator */
#define OP_TILDE	7	/* "g~" switch case operator */
#define OP_INDENT	8	/* "="  indent operator */
#define OP_FORMAT	9	/* "gq" format operator */
#define OP_COLON	10	/* ":"  colon operator */
#define OP_UPPER	11	/* "gU" make upper case operator */
#define OP_LOWER	12	/* "gu" make lower case operator */
#define OP_JOIN		13	/* "J"  join operator, only for Visual mode */
#define OP_JOIN_NS	14	/* "gJ"  join operator, only for Visual mode */
#define OP_ROT13	15	/* "g?" rot-13 encoding */
#define OP_REPLACE	16	/* "r"  replace chars, only for Visual mode */
#define OP_INSERT	17	/* "I"  Insert column, only for Visual mode */
#define OP_APPEND	18	/* "A"  Append column, only for Visual mode */

/*
 * Motion types, used for operators and for yank/delete registers.
 */
#define MCHAR	0		/* character-wise movement/register */
#define MLINE	1		/* line-wise movement/register */
#define MBLOCK	2		/* block-wise register */

/*
 * Minimum screen size
 */
#define MIN_COLUMNS	12	/* minimal columns for screen */
#define MIN_LINES	2	/* minimal lines for screen */
#define STATUS_HEIGHT	1	/* height of a status line under a window */

/*
 * Buffer sizes
 */
#ifndef CMDBUFFSIZE
# define CMDBUFFSIZE	256	/* size of the command processing buffer */
#endif

#define LSIZE	    512		/* max. size of a line in the tags file */

#define IOSIZE	   (1024+1)	/* file i/o and sprintf buffer size */
#define MSG_BUF_LEN 80		/* length of buffer for small messages */

#if defined(AMIGA) || defined(__linux__) || defined(__QNX__) || defined(__CYGWIN32__) || defined(_AIX)
# define TBUFSZ 2048		/* buffer size for termcap entry */
#else
# define TBUFSZ 1024		/* buffer size for termcap entry */
#endif

/*
 * Maximum length of key sequence to be mapped.
 * Must be able to hold an Amiga resize report.
 */
#define MAXMAPLEN   50

#ifdef BINARY_FILE_IO
# define WRITEBIN   "wb"	/* no CR-LF translation */
# define READBIN    "rb"
# define APPENDBIN  "ab"
#else
# define WRITEBIN   "w"
# define READBIN    "r"
# define APPENDBIN  "a"
#endif

/*
 * EMX doesn't have a global way of making open() use binary I/O.
 * Use O_BINARY for all open() calls.
 */
#if defined(__EMX__) || defined(__CYGWIN32__)
# define O_EXTRA    O_BINARY
#else
# define O_EXTRA    0
#endif

/*
 * defines to avoid typecasts from (char_u *) to (char *) and back
 * (vim_strchr() and vim_strrchr() are now in alloc.c)
 */
#define STRLEN(s)	    strlen((char *)(s))
#define STRCPY(d, s)	    strcpy((char *)(d), (char *)(s))
#define STRNCPY(d, s, n)    strncpy((char *)(d), (char *)(s), (size_t)(n))
#define STRCMP(d, s)	    strcmp((char *)(d), (char *)(s))
#define STRNCMP(d, s, n)    strncmp((char *)(d), (char *)(s), (size_t)(n))
#ifdef HAVE_STRCASECMP
# define STRICMP(d, s)	    strcasecmp((char *)(d), (char *)(s))
#else
# ifdef HAVE_STRICMP
#  define STRICMP(d, s)	    stricmp((char *)(d), (char *)(s))
# else
#  define STRICMP(d, s)	    vim_stricmp((char *)(d), (char *)(s))
# endif
#endif
#ifdef HAVE_STRNCASECMP
# define STRNICMP(d, s, n)  strncasecmp((char *)(d), (char *)(s), (size_t)(n))
#else
# ifdef HAVE_STRNICMP
#  define STRNICMP(d, s, n) strnicmp((char *)(d), (char *)(s), (size_t)(n))
# else
#  define STRNICMP(d, s, n) vim_strnicmp((char *)(d), (char *)(s), (size_t)(n))
# endif
#endif
#define STRCAT(d, s)	    strcat((char *)(d), (char *)(s))
#define STRNCAT(d, s, n)    strncat((char *)(d), (char *)(s), (size_t)(n))

#ifdef HAVE_STRPBRK
# define vim_strpbrk(s, cs) (char_u *)strpbrk((char *)(s), (char *)(cs))
#endif

#define MSG(s)			    msg((char_u *)(s))
#define MSG_ATTR(s, attr)	    msg_attr((char_u *)(s), (attr))
#define EMSG(s)			    emsg((char_u *)(s))
#define EMSG2(s, p)		    emsg2((char_u *)(s), (char_u *)(p))
#define EMSGN(s, n)		    emsgn((char_u *)(s), (long)(n))
#define OUT_STR(s)		    out_str((char_u *)(s))
#define OUT_STR_NF(s)		    out_str_nf((char_u *)(s))
#define MSG_PUTS(s)		    msg_puts((char_u *)(s))
#define MSG_PUTS_ATTR(s, a)	    msg_puts_attr((char_u *)(s), (a))
#define MSG_PUTS_TITLE(s)	    msg_puts_title((char_u *)(s))
#define MSG_PUTS_LONG(s)	    msg_puts_long((char_u *)(s))
#define MSG_PUTS_LONG_ATTR(s, a)    msg_puts_long_attr((char_u *)(s), (a))

typedef long	    linenr_t;	    /* line number type */
typedef unsigned    colnr_t;	    /* column number type */

#define MAXLNUM (0x7fffffff)	    /* maximum (invalid) line number */

#if SIZEOF_INT >= 4
# define MAXCOL	(0x7fffffff)	    /* maximum column number, 31 bits */
#else
# define MAXCOL	(0x7fff)	    /* maximum column number, 15 bits */
#endif

#define SHOWCMD_COLS 10		    /* columns needed by shown command */
#define STL_MAX_ITEM 50		    /* max count of %<flag> in statusline*/

/*
 * Include a prototype for mch_memmove(), it may not be in alloc.pro.
 */
#ifdef VIM_MEMMOVE
void mch_memmove __ARGS((void *, void *, size_t));
#else
# ifndef mch_memmove
#  define mch_memmove(to, from, len) memmove(to, from, len)
# endif
#endif

/*
 * fnamecmp() is used to compare file names.
 * On some systems case in a file name does not matter, on others it does.
 * (this does not account for maximum name lengths and things like "../dir",
 * thus it is not 100% accurate!)
 */
#ifdef CASE_INSENSITIVE_FILENAME
# ifdef BACKSLASH_IN_FILENAME
#  define fnamecmp(x, y) vim_fnamecmp((x), (y))
#  define fnamencmp(x, y, n) vim_fnamencmp((x), (y), (size_t)(n))
# else
#  define fnamecmp(x, y) STRICMP((x), (y))
#  define fnamencmp(x, y, n) STRNICMP((x), (y), (n))
# endif
#else
# define fnamecmp(x, y) strcmp((char *)(x), (char *)(y))
# define fnamencmp(x, y, n) strncmp((char *)(x), (char *)(y), (size_t)(n))
#endif

#ifdef HAVE_MEMSET
# define vim_memset(ptr, c, size)   memset((ptr), (c), (size))
#else
void *vim_memset __ARGS((void *, int, size_t));
#endif

#ifdef HAVE_MEMCMP
# define vim_memcmp(p1, p2, len)   memcmp((p1), (p2), (len))
#else
# ifdef HAVE_BCMP
#  define vim_memcmp(p1, p2, len)   bcmp((p1), (p2), (len))
# else
int vim_memcmp __ARGS((void *, void *, size_t));
#  define VIM_MEMCMP
# endif
#endif

/*
 * Enums need a typecast to be used as array index (for Ultrix).
 */
#define hl_attr(n)	highlight_attr[(int)(n)]
#define term_str(n)	term_strings[(int)(n)]

/*
 * vim_iswhite() is used for "^" and the like. It differs from isspace()
 * because it doesn't include <CR> and <LF> and the like.
 */
#define vim_iswhite(x)	((x) == ' ' || (x) == '\t')

/* Note that gui.h is included by structs.h */

#include "regexp.h"	    /* for struct regexp */
#include "structs.h"	    /* file that defines many structures */

#ifdef USE_MOUSE

/* Codes for mouse button events in lower three bits: */
# define MOUSE_LEFT	0x00
# define MOUSE_MIDDLE	0x01
# define MOUSE_RIGHT	0x02
# define MOUSE_RELEASE	0x03

/* bit masks for modifiers: */
# define MOUSE_SHIFT	0x04
# define MOUSE_ALT	0x08
# define MOUSE_CTRL	0x10

/* mouse buttons that are handled like a key press (GUI only) */
# define MOUSE_4	0x100	/* scroll wheel down */
# define MOUSE_5	0x200	/* scroll wheel up */

/* 0x20 is reserved by xterm */
# define MOUSE_DRAG_XTERM   0x40

# define MOUSE_DRAG	(0x40 | MOUSE_RELEASE)

/* Lowest button code for using the mouse wheel (xterm only) */
# define MOUSEWHEEL_LOW		0x60

# define MOUSE_CLICK_MASK	0x03

# define NUM_MOUSE_CLICKS(code) \
    (((unsigned)((code) & 0xC0) >> 6) + 1)

# define SET_NUM_MOUSE_CLICKS(code, num) \
    (code) = ((code) & 0x3f) | ((((num) - 1) & 3) << 6)

/*
 * jump_to_mouse() returns one of these values, possibly with
 * CURSOR_MOVED added
 */
# define IN_UNKNOWN	1
# define IN_BUFFER	2
# define IN_STATUS_LINE	3	    /* Or in command line */
# define CURSOR_MOVED	0x100
/* In addition get_fpos_of_mouse() may specify these */
# define BEFORE_TEXTLINE	0x200
# define AFTER_TEXTLINE		0x300

/* flags for jump_to_mouse() */
# define MOUSE_FOCUS		0x01	/* need to stay in this window */
# define MOUSE_MAY_VIS		0x02	/* may start Visual mode */
# define MOUSE_DID_MOVE		0x04	/* only act when mouse has moved */
# define MOUSE_SETPOS		0x08	/* only set current mouse position */
# define MOUSE_MAY_STOP_VIS	0x10	/* may stop Visual mode */

#endif /* USE_MOUSE */

/* defines for eval_vars() */
#define VALID_PATH		1
#define VALID_HEAD		2

/* defines for Vim variables (don't change them!) */
#define VV_COUNT	0
#define VV_COUNT1	1
#define VV_ERRMSG	2
#define VV_WARNINGMSG	3
#define VV_STATUSMSG	4
#define VV_SHELL_ERROR	5
#define VV_THIS_SESSION	6
#define VV_VERSION	7
#define VV_LEN		8	/* number of v: vars */

#ifdef USE_CLIPBOARD

/* Selection states for modeless selection */
# define SELECT_CLEARED		0
# define SELECT_IN_PROGRESS	1
# define SELECT_DONE		2

# define SELECT_MODE_CHAR	0
# define SELECT_MODE_WORD	1
# define SELECT_MODE_LINE	2

# ifdef USE_GUI_WIN32
#  ifdef HAVE_OLE
#   define WM_OLE (WM_APP+0)
#  endif
# endif

/* Info about selected text */
typedef struct VimClipboard
{
    int		available;	    /* Is clipboard available? */
    int		owned;		    /* Flag: do we own the selection? */
    FPOS	start;		    /* Start of selected area */
    FPOS	end;		    /* End of selected area */
    int		vmode;		    /* Visual mode character */

    /* Fields for selection that doesn't use Visual mode */
    short_u	origin_row;
    short_u	origin_start_col;
    short_u	origin_end_col;
    short_u	word_start_col;
    short_u	word_end_col;

    FPOS	prev;		    /* Previous position */
    short_u	state;		    /* Current selection state */
    short_u	mode;		    /* Select by char, word, or line. */

# if defined(USE_GUI_X11) || defined(XTERM_CLIP)
    Atom	xatom;		    /* Vim's own special selection format */
    Atom	xa_targets;
    Atom	xa_text;
    Atom	xa_compound_text;
# endif

# ifdef USE_GUI_GTK
    GdkAtom	atom;		    /* Vim's own special selection format */
# endif
# ifdef MSWIN
    int_u	format;		    /* Vim's own special clipboard format */
# endif
# ifdef USE_GUI_BEOS
				    /* no clipboard at the moment */
# endif
} VimClipboard;
#endif /* USE_CLIPBOARD */

#ifdef __BORLANDC__
/* work around a bug in the Borland 'stat' function: */
# include <io.h>	    /* for access() */

# define stat(a,b) (access(a,0) ? -1 : stat(a,b))
#endif

#include "globals.h"	    /* global variables and messages */
#include "option.h"	    /* option variables and defines */
#include "ex_cmds.h"	    /* Ex command defines */
#include "proto.h"	    /* function prototypes */

#ifdef USE_SNIFF
# include "if_sniff.h"
#endif

/* This has to go after the include of proto.h, as proto/os_win32.pro declares
 * functions of these names. The declarations would break if the defines had
 * been seen at that stage.
 */
#if !defined(USE_GUI_WIN32) && !defined(macintosh)
# define mch_errmsg(str)	fprintf(stderr, "%s", (str))
# define mch_display_error()	fflush(stderr)
# define mch_msg(str)		printf("%s", (str))
#else
# define mch_msg(str)		mch_errmsg((str))
#endif

/*
 * If console dialog not supported, but GUI dialog is, use the GUI one.
 */
#if defined(GUI_DIALOG) && !defined(CON_DIALOG)
# define do_dialog gui_mch_dialog
#endif

/*
 * The filters used in file browsers are too machine specific to
 * be defined usefully in a generic fashion.
 * So instead the filter strings are defined here for each architecture
 * that supports gui_mch_browse()
 */
#ifdef USE_BROWSE
# ifdef USE_GUI_WIN32
#  define BROWSE_FILTER_MACROS \
		(char_u *)"Vim macro files (*.vim)\0*.vim\0All Files (*.*)\0*.*\0\0"
#  define BROWSE_FILTER_ALL_FILES (char_u *)"All Files (*.*)\0*.*\0\0"
# else
#  define BROWSE_FILTER_MACROS \
		(char_u *)"Vim macro files (*.vim)\0*.vim\0All Files (*)\0*\0\0"
#  define BROWSE_FILTER_ALL_FILES (char_u *)"All Files (*)\0*\0\0"
# endif
#endif

/* stop using fastcall for Borland */
#if defined(__BORLANDC__) && defined(WIN32) && !defined(DEBUG)
# pragma option -p.
#endif

#if defined(MEM_PROFILE)
# define vim_realloc(ptr, size)  mem_realloc((ptr), (size))
#else
# define vim_realloc(ptr, size)  realloc((ptr), (size))
#endif

#endif /* VIM__H */
