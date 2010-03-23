/* vi:set ts=8 sts=0 sw=8:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
 * feature.h: Defines for optional code and preferences
 *
 * Edit this file to include/exclude parts of Vim, before compiling.
 * The only other file that may be edited is Makefile, it contains machine
 * specific options.
 *
 * To include specific options, change the "#if*" and "#endif" into comments,
 * or uncomment the "#define".
 * To exclude specific options, change the "#define" into a comment.
 */

/*
 * When adding a new feature:
 * - Add a #define below.
 * - Add a message in the table above do_version().
 * - Add a string to f_has().
 * - Add a feature to ":help feature-list" in doc/eval.txt.
 * - Add feature to ":help +feature-list" in doc/various.txt.
 * - Add comment for the documentation of commands that use the feature.
 * - When the feature is not included when MIN_FEAT is defined, add it to the
 *   list at ":help minimal-features".
 */

/*
 * Basic choices:
 * ==============
 *
 * MIN_FEAT		minimal features enabled, as basic as possible (DOS16)
 * MAX_FEAT		maximal features enabled, as rich as possible.
 * default		A selection of features enabled.
 *
 * These executables are made available with MAX_FEAT defined, because they
 * are supposed to have enough RAM: Win32 (console & GUI), dos32, OS/2 and VMS.
 * The dos16 version has very little RAM available, use MIN_FEAT.
 */
#if !defined(MIN_FEAT) && !defined(MAX_FEAT)
/* #define MIN_FEAT */
/* #define MAX_FEAT */
# if defined(MSWIN) || defined(DJGPP) || defined(OS2) || defined(VMS)
#  define MAX_FEAT
# else
#  ifdef MSDOS
#   define MIN_FEAT
#  endif
# endif
#endif

/*
 * Optional code (see ":help +feature-list")
 * =============
 */

/*
 * +digraphs		When DIGRAPHS defined: Include digraph support.
 *			In insert mode and on the command line you will be
 *			able to use digraphs. The CTRL-K command will work.
 */
#ifndef MIN_FEAT
# define DIGRAPHS
#endif

/*
 * +langmap		When HAVE_LANGMAP defined: Include support for
 *			'langmap' option.  Only useful when you put your
 *			keyboard in a special language mode, e.g. for typing
 *			greek.
 */
#ifdef MAX_FEAT
# define HAVE_LANGMAP
#endif

/*
 * +insert_expand	When INSERT_EXPAND defined: Support for
 *			CTRL-N/CTRL-P/CTRL-X in insert mode. Takes about
 *			4Kbyte of code.
 */
#ifndef MIN_FEAT
# define INSERT_EXPAND
#endif

/*
 * +cmdline_compl	When CMDLINE_COMPL defined: Support for
 *			completion of mappings/abbreviations in cmdline mode.
 *			Takes a few Kbyte of code.
 */
#ifndef MIN_FEAT
# define CMDLINE_COMPL
#endif

/*
 * +textobjects		When TEXT_OBJECTS defined: Support for text objects:
 *			"vaw", "das", etc.
 */
#ifndef MIN_FEAT
# define TEXT_OBJECTS
#endif

/*
 * +visualextra		Extra features for Visual mode (mostly block operators).
 */
#ifndef MIN_FEAT
# define VISUALEXTRA
#endif

/*
 * +cmdline_info	When CMDLINE_INFO defined: Support for 'showcmd' and
 *			'ruler' options.
 */
#ifndef MIN_FEAT
# define CMDLINE_INFO
#endif

/*
 * +linebreak		When LINEBREAK defined: Support for 'showbreak',
 *			'breakat'  and 'linebreak' options.
 */
#ifndef MIN_FEAT
# define LINEBREAK
#endif

/*
 * +ex_extra		When EX_EXTRA defined: Support for ":retab", ":right",
 *			":left", ":center", ":normal".
 */
#ifndef MIN_FEAT
# define EX_EXTRA
#endif

/*
 * +extra_search	When EXTRA_SEARCH defined: Support for 'hlsearch' and
 *			'incsearch'.
 */
#ifndef MIN_FEAT
# define EXTRA_SEARCH
#endif

/*
 * +quickfix		When QUICKFIX defined: Support for quickfix commands.
 */
#ifndef MIN_FEAT
# define QUICKFIX
#endif

/*
 * +file_in_path	When FILE_IN_PATH defined: Support for "gf" and
 *			"<cfile>".
 */
#ifndef MIN_FEAT
# define FILE_IN_PATH
#endif

/*
 * +find_in_path	When FIND_IN_PATH defined: Support for "[I" ":isearch"
 *			"^W^I", ":checkpath", etc.
 */
#ifndef MIN_FEAT
# ifdef FILE_IN_PATH	/* FILE_IN_PATH is required */
#  define FIND_IN_PATH
# endif
#endif

/*
 * +rightleft		When RIGHTLEFT defined: Right-to-left typing and
 *			Hebrew support.  Takes some code.
 */
#ifdef MAX_FEAT
# define RIGHTLEFT
#endif

/*
 * +farsi		When FKMAP defined: Farsi (Persian language) Keymap
 *			support.  Takes some code.  Needs RIGHTLEFT.
 */
#ifdef MAX_FEAT
# ifndef RIGHTLEFT
#  define RIGHTLEFT
# endif
# define FKMAP
#endif

/*
 * +emacs_tags		When EMACS_TAGS defined: Include support for emacs
 *			style TAGS file.  Takes some code.
 */
#ifdef MAX_FEAT
# define EMACS_TAGS
#endif

/*
 * +tag_binary		When BINARY_TAGS defined: Use a binary search instead
 *			of a linear search when search a tags file.
 */
#ifndef MIN_FEAT
# define BINARY_TAGS
#endif

/*
 * +tag_old_static	When OLD_STATIC_TAGS defined: Include support for old
 *			style static tags: "file:tag  file  ..".  Slows down
 *			tag searching a bit.
 */
#ifndef MIN_FEAT
# define OLD_STATIC_TAGS
#endif

/*
 * +tag_any_white	When TAG_ANY_WHITE defined: Allow any white space to
 *			separate the fields in a tags file.	When not
 *			defined, only a TAB is allowed.
 */
/* #define TAG_ANY_WHITE */

/*
 * +cscope		Unix only.  When USE_CSCOPE defined, enable interface
 *			to support cscope.
 */
#if defined(UNIX) && defined(MAX_FEAT) && !defined(USE_CSCOPE)
# define USE_CSCOPE
#endif

/*
 * +eval		When WANT_EVAL defined: Include built-in script
 *			language and expression evaluation, ":let", ":if",
 *			etc.
 */
#ifndef MIN_FEAT
# define WANT_EVAL
#endif

/*
 * +user_commands	When USER_COMMANDS defined: Allow the user to define
 *			his own commands.
 */
#ifndef MIN_FEAT
# define USER_COMMANDS
#endif

/*
 * +modify_fname	When WANT_MODIFY_FNAME defined: Include modifiers for
 *			file name.  E.g., "%:p:h".
 */
#ifndef MIN_FEAT
# define WANT_MODIFY_FNAME
#endif

/*
 * +autocmd		When defined: Include support for ":autocmd"
 */
#ifndef MIN_FEAT
# define AUTOCMD
#endif

/*
 * +title		When defined: Include support for 'title' and 'icon'
 * +statusline		When defined: Include support for 'statusline',
 *			'rulerformat' and special format of 'titlestring' and
 *			'iconstring'.
 * +byte_offset		When defined: Include support for '%o' in 'statusline'
 *                      and builtin functions line2byte() and byte2line().
 */
#if !defined(MIN_FEAT) && !defined(MSDOS)
# define WANT_TITLE
#endif

#ifndef MIN_FEAT
# define STATUSLINE
# ifndef CMDLINE_INFO
#  define CMDLINE_INFO	/* 'ruler' is required for 'statusline' */
# endif
#endif

#ifndef MIN_FEAT
# define BYTE_OFFSET
#endif

/*
 * +wildignore		When defined: Include support for 'wildignore'
 */
#ifndef MIN_FEAT
# define WILDIGNORE
#endif

/*
 * +wildmenu		When defined: Include support for 'wildmenu'
 */
#ifndef MIN_FEAT
# define WILDMENU
#endif

/*
 * +osfiletype		When WANT_OSFILETYPE defined: Include support for
 *			filetype checking in autocommands. Eg:
 *			*.html,*.htm,<html>,*.shtml
 *			Only on systems that support filetypes (RISC OS).
 */
#if 0
# define WANT_OSFILETYPE
# define OFT_DFLT "Text"
#endif

/*
 * +viminfo		When VIMINFO defined: Include support for
 *			reading/writing the viminfo file. Takes about 8Kbyte
 *			of code.
 * VIMINFO_FILE		Location of user .viminfo file (should start with $).
 * VIMINFO_FILE2	Location of alternate user .viminfo file.
 */
#ifndef MIN_FEAT
# define VIMINFO
/* #define VIMINFO_FILE	"$HOME/foo/.viminfo" */
/* #define VIMINFO_FILE2 "~/bar/.viminfo" */
#endif

/*
 * +syntax		When SYNTAX_HL defined: Include support for syntax
 *			highlighting.  When using this, it's a good idea to
 *			have AUTOCMD too.
 */
#if !defined(MIN_FEAT) || defined(PROTO)
# define SYNTAX_HL
#endif

/*
 * +sniff		When USE_SNIFF defined: Include support for Sniff
 *			interface.  This needs to be defined in the Makefile.
 */

/*
 * +builtin_terms	Choose one out of the following four:
 *
 * NO_BUILTIN_TCAPS	When defined: Do not include any builtin termcap
 *			entries (used only with HAVE_TGETENT defined).
 *
 * (nothing)		Machine specific termcap entries will be included.
 *
 * SOME_BUILTIN_TCAPS	When defined: Include most useful builtin termcap
 *			entries (used only with NO_BUILTIN_TCAPS not defined).
 *			This is the default.
 *
 * ALL_BUILTIN_TCAPS	When defined: Include all builtin termcap entries
 *			(used only with NO_BUILTIN_TCAPS not defined).
 */
#ifdef HAVE_TGETENT
/* #define NO_BUILTIN_TCAPS */
#endif

#ifndef NO_BUILTIN_TCAPS
# ifdef MAX_FEAT
#  define ALL_BUILTIN_TCAPS
# else
#  define SOME_BUILTIN_TCAPS		/* default */
# endif
#endif

/*
 * +lispindent		When LISPINDENT defined: Include lisp indenting (From
 *			Eric Fischer). Doesn't completely work like Vi (yet).
 * +cindent		When CINDENT defined: Include C code indenting (From
 *			Eric Fischer).
 * +smartindent		When SMARTINDENT defined: Do smart C code indenting
 *			when the 'si' option is set. It's not as good as
 *			CINDENT, only included to keep the old code.
 *
 * These two need to be defined when making prototypes.
 */
#if !defined(MIN_FEAT) || defined(PROTO)
# define LISPINDENT
#endif

#if !defined(MIN_FEAT) || defined(PROTO)
# define CINDENT
#endif

#ifndef MIN_FEAT
# define SMARTINDENT
#endif

/*
 * +comments		'comments' option.
 */
#ifndef MIN_FEAT
# define COMMENTS
#endif

/*
 * +cryptv		Optional encryption by Mohsin Ahmed <mosh@sasi.com>.
 */
#if !defined(MIN_FEAT) || defined(PROTO)
# define CRYPTV
#endif

/*
 * +browse		Enable :browse command.
 */
#if !defined(MIN_FEAT) && (defined(USE_GUI_MSWIN) || defined(USE_GUI_MOTIF) || defined(USE_GUI_ATHENA) || defined(USE_GUI_GTK))
# define USE_BROWSE
#endif

/*
 * +mksession		Enable :mksession command.
 */
#ifndef MIN_FEAT
# define MKSESSION
#endif

/*
 * +multi_byte		Enable generic multi-byte character handling.
 */
#if defined(MAX_FEAT) && !defined(MULTI_BYTE) && !defined(WIN16)
# define MULTI_BYTE
#endif

/*
 * +multi_byte_ime	Win32 IME input method.  Requires +multi_byte.
 *			Only for far-east Windows, so IME can be used to input
 *			chars.  Not tested much!
 */
#if defined(USE_GUI_WIN32) && !defined(MULTI_BYTE_IME)
/* #  define MULTI_BYTE_IME */
# endif

#if defined(MULTI_BYTE_IME) && !defined(MULTI_BYTE)
# define MULTI_BYTE
#endif

/*
 * +xim			X Input Method.  For entering special languages like
 *			chinese and Japanese.
 * +hangul_input	Internal Hangul input method.  Must be included
 *			through configure: "--enable-hangulin"
 * Both are for Unix only.  Works for VMS too.
 */
#ifndef USE_XIM
/* #define USE_XIM */
#endif

#ifdef HANGUL_INPUT
# define HANGUL_DEFAULT_KEYBOARD 2	/* 2 or 3 bulsik keyboard */
# define ESC_CHG_TO_ENG_MODE		/* if defined, when ESC pressed,
					 * turn to english mode
					 */
# if defined(USE_XIM) && !defined(LINT)
   Error: You should select only ONE of XIM and HANGUL_INPUT
# endif
#endif
#if defined(HANGUL_INPUT) || defined(USE_XIM)
/* # define X_LOCALE */			/* for OS with the incomplete locale
					 * support like linux
					 */
/* # define SLOW_XSERVER */		/* for extremely slow X server */
#endif

/*
 * +xfontset		X fontset support.  For outputting special languages.
 *			Required for XIM.
 */
#ifndef USE_FONTSET
# ifdef USE_XIM
#  define USE_FONTSET
# else
/* #  define USE_FONTSET */
# endif
#endif

/*
 *			When WINDOWS_ALT_KEYS defined, let Windows handle ALT
 *			keys, they are not available to Vim.  Allows
 *			selectiong menu items.
 */
/* #define WINDOWS_ALT_KEYS */

/*
 * +menu		:menu.
 */
#ifndef MIN_FEAT
# define WANT_MENU
#endif

/*
 * BROWSE_CURRBUF	When defined: Open file browser in the directory of
 *			the current buffer, instead of the current directory.
 *
 * USE_TOOLBAR		Include code for a toolbar (for the Win32 GUI, GTK
 *			always has it).  But only if menus are enabled.
 */
#if defined(USE_GUI_MSWIN) && !defined(MIN_FEAT)
# define BROWSE_CURRBUF
#endif
#if (defined(USE_GUI_GTK) || defined(USE_GUI_WIN32)) \
	&& !defined(MIN_FEAT) && defined(WANT_MENU)
# define USE_TOOLBAR
#endif
#if defined(USE_TOOLBAR) && !defined(WANT_MENU)
# define WANT_MENU
#endif

/*
 * +scrollbind		Enable synchronization of split windows
 */
#ifndef MIN_FEAT
# define SCROLLBIND
#endif

/*
 * +dialog_gui		When GUI_DIALOG defined, use GUI dialog.
 * +dialog_con		When CON_DIALOG defined, may use Console dialog.
 *			When none of these defined, no dialog support.
 */
#ifndef MIN_FEAT
# if defined(USE_GUI_MSWIN) || defined(macintosh)
#  define GUI_DIALOG
# else
#  if defined(USE_GUI_ATHENA) || defined(USE_GUI_MOTIF) || defined(USE_GUI_GTK)
#   define CON_DIALOG
#   define GUI_DIALOG
#  else
#   define CON_DIALOG
#  endif
# endif
#endif

/*
 * Preferences:
 * ============
 */

/*
 * +writebackup		When WRITEBACKUP defined: 'writebackup' is default on:
 *			Use a backup file while overwriting a file.  But it's
 *			deleted again when 'backup' is not set.  Changing this
 *			is strongly discouraged: You can loose all your
 *			changes when the computer crashes while writing the
 *			file.
 *			VMS note: It does work on VMS as well, but because of
 *			version handling it does not have any purpose.
 *			Overwrite will write to the new version.
 */
#ifndef VMS
# define WRITEBACKUP
#endif

/*
 * +xterm_save		When SAVE_XTERM_SCREEN defined: The t_ti and t_te
 *			entries for the builtin xterm will be set to save the
 *			screen when starting Vim and restoring it when
 *			exiting.
 */
/* #define SAVE_XTERM_SCREEN */

/*
 * DEBUG		When defined: Output a lot of debugging garbage.
 */
/* #define DEBUG */

/*
 * VIMRC_FILE		Name of the .vimrc file in current dir.
 */
/* #define VIMRC_FILE	".vimrc" */

/*
 * EXRC_FILE		Name of the .exrc file in current dir.
 */
/* #define EXRC_FILE	".exrc" */

/*
 * GVIMRC_FILE		Name of the .gvimrc file in current dir.
 */
/* #define GVIMRC_FILE	".gvimrc" */

/*
 * SESSION_FILE		Name of the default ":mksession" file.
 */
#define SESSION_FILE	"Session.vim"

/*
 * USR_VIMRC_FILE	Name of the user .vimrc file.
 * USR_VIMRC_FILE2	Name of alternate user .vimrc file.
 * USR_VIMRC_FILE3	Name of alternate user .vimrc file.
 */
/* #define USR_VIMRC_FILE	"~/foo/.vimrc" */
/* #define USR_VIMRC_FILE2	"~/bar/.vimrc" */
/* #define USR_VIMRC_FILE3	"$VIM/.vimrc" */

/*
 * USR_EXRC_FILE	Name of the user .exrc file.
 * USR_EXRC_FILE2	Name of the alternate user .exrc file.
 */
/* #define USR_EXRC_FILE	"~/foo/.exrc" */
/* #define USR_EXRC_FILE2	"~/bar/.exrc" */

/*
 * USR_GVIMRC_FILE	Name of the user .gvimrc file.
 * USR_GVIMRC_FILE2	Name of the alternate user .gvimrc file.
 */
/* #define USR_GVIMRC_FILE	"~/foo/.gvimrc" */
/* #define USR_GVIMRC_FILE2	"~/bar/.gvimrc" */
/* #define USR_GVIMRC_FILE3	"$VIM/.gvimrc" */

/*
 * SYS_VIMRC_FILE	Name of the system-wide .vimrc file.
 */
/* #define SYS_VIMRC_FILE	"/etc/vimrc" */

/*
 * SYS_GVIMRC_FILE	Name of the system-wide .gvimrc file.
 */
/* #define SYS_GVIMRC_FILE	"/etc/gvimrc" */

/*
 * VIM_HLP		Name of the help file.
 */
/* # define VIM_HLP	"$VIMRUNTIME/doc/help.txt.gz" */

/*
 * FILETYPE_FILE	Name of the file type detection file.
 */
/* # define FILETYPE_FILE	"$VIMRUNTIME/filetype.vim" */

/*
 * FTOFF_FILE		Name of the file to switch off file type detection.
 */
/* # define FTOFF_FILE	"$VIMRUNTIME/ftoff.vim" */

/*
 * SYS_MENU_FILE	Name of the default menu.vim file.
 */
/* # define SYS_MENU_FILE	"$VIMRUNTIME/menu.vim" */

/*
 * SYS_OPTWIN_FILE	Name of the default optwin.vim file.
 */
#ifndef SYS_OPTWIN_FILE
# define SYS_OPTWIN_FILE	"$VIMRUNTIME/optwin.vim"
#endif

/*
 * SYNTAX_FNAME		Name of a syntax file, where %s is the syntax name.
 */
/* #define SYNTAX_FNAME	"/foo/%s.vim" */

/*
 * RUNTIME_DIRNAME	Generic name for the directory of the runtime files.
 */
#ifndef RUNTIME_DIRNAME
# define RUNTIME_DIRNAME "runtime"
#endif

/*
 * Machine dependent:
 * ==================
 */

/*
 * +fork		Unix only.  When USE_SYSTEM defined: Use system()
 * +system		instead of fork/exec for starting a shell.  Doesn't
 *			work for the GUI!
 */
/* #define USE_SYSTEM */

/*
 * +X11			Unix only.  When WANT_X11 defined: Include code for
 *			xterm title saving.  Only works if HAVE_X11 is also
 *			defined.
 */
#if !defined(MIN_FEAT) || defined(USE_GUI_MOTIF) || defined(USE_GUI_ATHENA)
# define WANT_X11
#endif

/*
 * +mouse_xterm		Unix only. When XTERM_MOUSE defined: Include code for
 *			xterm mouse handling.
 * +xterm_clipboard	Unix only. When XTERM_CLIP defined: Include code for
 *			handling the clipboard like in gui mode
 * +mouse_netterm	idem, NETTERM_MOUSE, for Netterm mouse handling.
 * +mouse_dec		idem, DEC_MOUSE, for Dec mouse handling.
 * (none)		MS-DOS mouse support.
 * +mouse_gpm		Unix only: when GPM_MOUSE defined: Include code for
 *			Linux console mouse handling.
 * +mouse		Any mouse support (any of the above enabled).
 */
#if !defined(AMIGA) || defined(USE_GUI_AMIGA)	/* Amiga console has no mouse */
# ifndef MIN_FEAT
#  define XTERM_MOUSE
# endif
# ifdef MAX_FEAT
#  define NETTERM_MOUSE
# endif
# ifdef MAX_FEAT
#  define DEC_MOUSE
# endif
# if (defined(MSDOS) || defined(WIN32)) && !defined(MIN_FEAT)
#  define DOS_MOUSE
# endif
#endif

#if !defined(MIN_FEAT) && defined(UNIX) && defined(WANT_X11) && defined(HAVE_X11)
# define XTERM_CLIP
# ifndef USE_CLIPBOARD
#  define USE_CLIPBOARD
# endif
#endif
#if defined(HAVE_GPM) && !defined(MIN_FEAT)
# define GPM_MOUSE
#endif
/* Define USE_MOUSE when any of the above is defined */
/* It's also defined in gui.h, the GUI always has a mouse. */
#if !defined(USE_MOUSE) && (defined(XTERM_MOUSE) || defined(NETTERM_MOUSE) \
	|| defined(DEC_MOUSE) || defined(DOS_MOUSE) || defined(GPM_MOUSE))
# define USE_MOUSE		/* include mouse support */
#endif

/*
 * cursor shape		Adjust the shape of the cursor to the mode.
 */
#ifndef MIN_FEAT
/* MS-DOS console and Win32 console can change cursor shape */
# if defined(MSDOS) || (defined(WIN32) && !defined(USE_GUI_WIN32))
#  define MCH_CURSOR_SHAPE
# endif
#endif

/* GUI and some consoles can change the shape of the cursor */
#if defined(USE_GUI) || defined(MCH_CURSOR_SHAPE)
# define CURSOR_SHAPE
#endif

/*
 * +ARP			Amiga only. When defined: Do not use arp.library, DOS
 *			2.0 required.
 */
/* #define NO_ARP */

/*
 * +GUI_Athena		To compile Vim with or without the GUI (gvim) you have
 * +GUI_BeOS		to edit the Makefile.
 * +GUI_Motif
 */

/*
 * +ole			Win32 OLE automation: Use if_ole_vc.mak.
 */

/*
 * These features can only be included by using a configure argument.  See the
 * Makefile for a line to uncomment.
 * +perl		Perl interface: "--enable-perlinterp"
 * +python		Python interface: "--enable-pythoninterp"
 * +tcl			TCL interface: "--enable-tclinterp"
 */

/*
 * These features are automatically detected:
 * +terminfo
 * +tgetent
 */
