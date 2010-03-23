/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Code to handle user-settable options. This is all pretty much table-
 * driven. Checklist for adding a new option:
 * - Put it in the options array below (copy an existing entry).
 * - For a global option: Add a variable for it in option.h.
 * - For a buffer or window local option:
 *   - Add a PV_XX entry to the enum below.
 *   - Add a variable to the window or buffer struct in structs.h.
 *   - For a window option, add some code to win_copy_options().
 *   - For a buffer option, add some code to buf_copy_options().
 *   - For a buffer string option, add code to check_buf_options().
 * - If it's a numeric option, add any necessary bounds checks to do_set().
 * - If it's a list of flags, add some code in do_set(), search for WW_ALL.
 * - When adding an option with expansion (P_EXPAND), but with a different
 *   default for Vi and Vim (no P_VI_DEF), add some code at VIMEXP.
 * - Add documentation!  One line in doc/help.txt, full description in
 *   options.txt, and any other related places.
 * - Add an entry in runtime/optwin.vim.
 * When making changes:
 * - Adjust the help for the option in doc/option.txt.
 * - When an entry has the P_VIM flag, or is lacking the P_VI_DEF flag, add a
 *   comment at the help for the 'compatible' option.
 */

#include "vim.h"

struct vimoption
{
    char	*fullname;	/* full option name */
    char	*shortname;	/* permissible abbreviation */
    short_u	flags;		/* see below */
    char_u	*var;		/* pointer to variable */
    char_u	*def_val[2];	/* default values for variable (vi and vim) */
};

#define VI_DEFAULT  0	    /* def_val[VI_DEFAULT] is Vi default value */
#define VIM_DEFAULT 1	    /* def_val[VIM_DEFAULT] is Vim default value */

/*
 * Flags
 *
 * Note: P_EXPAND and P_IND can never be used at the same time.
 * Note: P_IND cannot be used for a terminal option.
 */
#define P_BOOL		0x01	/* the option is boolean */
#define P_NUM		0x02	/* the option is numeric */
#define P_STRING	0x04	/* the option is a string */
#define P_ALLOCED	0x08	/* the string option is in allocated memory,
				    must use vim_free() when assigning new
				    value. Not set if default is the same. */
#define P_EXPAND	0x10	/* environment expansion */
#define P_IND		0x20	/* indirect, is in curwin or curbuf */
#define P_NODEFAULT	0x40	/* has no default value */
#define P_DEF_ALLOCED	0x80	/* default value is in allocated memory, must
				    use vim_free() when assigning new value */
#define P_WAS_SET	0x100	/* option has been set/reset */
#define P_NO_MKRC	0x200	/* don't include in :mkvimrc output */
#define P_VI_DEF	0x400	/* Use Vi default for Vim */
#define P_VIM		0x800	/* Vim option, reset when 'cp' set */

#define P_RSTAT		0x1000	/* when changed, redraw status lines */
#define P_RBUF		0x2000	/* when changed, redraw current buffer */
#define P_RALL		0x4000	/* when changed, redraw all */
#define P_RCLR		0x7000	/* when changed, clear and redraw all */
#define P_COMMA		0x8000	/* comma separated list */

/*
 * The options that are in curwin or curbuf have P_IND set and a var field
 * that contains one of the enum values below.
 */
enum indirect_options
{
    PV_AI = 1,
    PV_BIN,
    PV_CIN,
    PV_CINK,
    PV_CINO,
    PV_CINW,
    PV_COM,
    PV_CPT,
    PV_EOL,
    PV_ET,
    PV_FE,
    PV_FF,
    PV_FO,
    PV_FT,
    PV_INF,
    PV_ISK,
    PV_KEY,
    PV_LBR,
    PV_LISP,
    PV_LIST,
    PV_ML,
    PV_MPS,
    PV_MOD,
    PV_NF,
    PV_NU,
    PV_OFT,
    PV_RL,
    PV_RO,
    PV_SCROLL,
    PV_SI,
    PV_SN,
    PV_STS,
    PV_SWF,
    PV_SYN,
    PV_SCBIND,
    PV_SW,
    PV_TS,
    PV_TW,
    PV_TX,
    PV_WM,
    PV_WRAP
};

/*
 * options[] is initialized here.
 * The order of the options MUST be alphabetic for ":set all" and findoption().
 * All option names MUST start with a lowercase letter (for findoption()).
 * Exception: "t_" options are at the end.
 * The options with a NULL variable are 'hidden': a set command for them is
 * ignored and they are not printed.
 */
static struct vimoption options[] =
{
    {"aleph",	    "al",   P_NUM|P_VI_DEF,
#ifdef RIGHTLEFT
			    (char_u *)&p_aleph,
#else
			    (char_u *)NULL,
#endif
			    {
#if (defined(MSDOS) || defined(WIN32) || defined(OS2)) && !defined(USE_GUI_WIN32)
			    (char_u *)128L,
#else
			    (char_u *)224L,
#endif
					    (char_u *)0L}},
    {"allowrevins", "ari",  P_BOOL|P_VI_DEF|P_VIM,
#ifdef RIGHTLEFT
			    (char_u *)&p_ari,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"altkeymap",   "akm",  P_BOOL|P_VI_DEF,
#ifdef FKMAP
			    (char_u *)&p_altkeymap,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"autoindent",  "ai",   P_BOOL|P_IND|P_VI_DEF,
			    (char_u *)PV_AI,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"autoprint",   "ap",   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"autowrite",   "aw",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_aw,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"background",  "bg",   P_STRING|P_VI_DEF|P_RCLR,
			    (char_u *)&p_bg,
			    {
#if (defined(MSDOS) || defined(OS2) || defined(WIN32)) && !defined(USE_GUI)
			    (char_u *)"dark",
#else
			    (char_u *)"light",
#endif
					    (char_u *)0L}},
    {"backspace",   "bs",   P_STRING|P_VI_DEF|P_VIM|P_COMMA,
			    (char_u *)&p_bs,
			    {(char_u *)"", (char_u *)0L}},
    {"backup",	    "bk",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_bk,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"backupdir",   "bdir", P_STRING|P_EXPAND|P_VI_DEF|P_COMMA,
			    (char_u *)&p_bdir,
			    {(char_u *)DEF_BDIR, (char_u *)0L}},
    {"backupext",   "bex",  P_STRING|P_VI_DEF,
			    (char_u *)&p_bex,
			    {
#ifdef VMS
			    (char_u *)"_",
#else
			    (char_u *)"~",
#endif
					    (char_u *)0L}},
    {"beautify",    "bf",   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"binary",	    "bin",  P_BOOL|P_IND|P_VI_DEF|P_RSTAT,
			    (char_u *)PV_BIN,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"bioskey",	    "biosk",P_BOOL|P_VI_DEF,
#ifdef MSDOS
			    (char_u *)&p_biosk,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)TRUE, (char_u *)0L}},
    {"breakat",	    "brk",  P_STRING|P_VI_DEF|P_RALL,
#ifdef LINEBREAK
			    (char_u *)&p_breakat,
			    {(char_u *)" \t!@*-+_;:,./?", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"browsedir",   "bsdir",P_STRING|P_VI_DEF,
#ifdef USE_BROWSE
			    (char_u *)&p_bsdir,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"last", (char_u *)0L}},
    {"cindent",	    "cin",  P_BOOL|P_IND|P_VI_DEF|P_VIM,
#ifdef CINDENT
			    (char_u *)PV_CIN,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"cinkeys",	    "cink", P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
#ifdef CINDENT
			    (char_u *)PV_CINK,
			    {(char_u *)"0{,0},:,0#,!^F,o,O,e", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"cinoptions",  "cino", P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
#ifdef CINDENT
			    (char_u *)PV_CINO,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"cinwords",    "cinw", P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
#if defined(SMARTINDENT) || defined(CINDENT)
			    (char_u *)PV_CINW,
			    {(char_u *)"if,else,while,do,for,switch",
				(char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"clipboard",   "cb",   P_STRING|P_VI_DEF|P_COMMA,
#ifdef USE_CLIPBOARD
                            (char_u *)&p_cb,
#else
			    (char_u *)NULL,
#endif
                            {(char_u *)"", (char_u *)0L}},
    {"cmdheight",   "ch",   P_NUM|P_VI_DEF|P_RALL,
			    (char_u *)&p_ch,
			    {(char_u *)1L, (char_u *)0L}},
    {"columns",	    "co",   P_NUM|P_NODEFAULT|P_NO_MKRC|P_VI_DEF|P_RCLR,
			    (char_u *)&Columns,
			    {(char_u *)80L, (char_u *)0L}},
    {"comments",    "com",  P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
#ifdef COMMENTS
			    (char_u *)PV_COM,
			    {(char_u *)"s1:/*,mb:*,ex:*/,://,b:#,:%,:XCOMM,n:>,fb:-",
#else
			    (char_u *)NULL,
			    {(char_u *)0L,
#endif
				(char_u *)0L}},
    {"compatible",  "cp",   P_BOOL|P_RALL,
			    (char_u *)&p_cp,
			    {(char_u *)TRUE, (char_u *)FALSE}},
    {"complete",    "cpt",  P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
#ifdef INSERT_EXPAND
			    (char_u *)PV_CPT,
			    {(char_u *)".,w,b,u,t,i", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"confirm",     "cf",   P_BOOL|P_VI_DEF,
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
			    (char_u *)&p_confirm,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"conskey",	    "consk",P_BOOL|P_VI_DEF,
#ifdef MSDOS
			    (char_u *)&p_consk,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"cpoptions",   "cpo",  P_STRING|P_VIM|P_RALL,
			    (char_u *)&p_cpo,
			    {(char_u *)CPO_ALL, (char_u *)CPO_DEFAULT}},
    {"cscopeprg",   "csprg", P_STRING|P_EXPAND|P_VI_DEF,
#ifdef USE_CSCOPE
			    (char_u *)&p_csprg,
			    {(char_u *)"cscope", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"cscopetag",   "cst",  P_BOOL|P_VI_DEF|P_VIM,
#ifdef USE_CSCOPE
			    (char_u *)&p_cst,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)0L, (char_u *)0L}},
    {"cscopetagorder", "csto", P_NUM|P_VI_DEF|P_VIM,
#ifdef USE_CSCOPE
			    (char_u *)&p_csto,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)0L, (char_u *)0L}},
    {"cscopeverbose", "csverb", P_BOOL|P_VI_DEF|P_VIM,
#ifdef USE_CSCOPE
			    (char_u *)&p_csverbose,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)0L, (char_u *)0L}},
    {"define",	    "def",  P_STRING|P_VI_DEF,
			    (char_u *)&p_def,
			    {(char_u *)"^#\\s*define", (char_u *)0L}},
    {"dictionary",  "dict", P_STRING|P_EXPAND|P_VI_DEF|P_COMMA,
#ifdef INSERT_EXPAND
			    (char_u *)&p_dict,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"digraph",	    "dg",   P_BOOL|P_VI_DEF|P_VIM,
#ifdef DIGRAPHS
			    (char_u *)&p_dg,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"directory",   "dir",  P_STRING|P_EXPAND|P_VI_DEF|P_COMMA,
			    (char_u *)&p_dir,
			    {(char_u *)DEF_DIR, (char_u *)0L}},
    {"display",	    "dy",   P_STRING|P_VI_DEF|P_COMMA|P_RALL,
			    (char_u *)&p_dy,
			    {(char_u *)"", (char_u *)0L}},
    {"edcompatible","ed",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_ed,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"endofline",   "eol",  P_BOOL|P_IND|P_NO_MKRC|P_VI_DEF|P_RSTAT,
			    (char_u *)PV_EOL,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"equalalways", "ea",   P_BOOL|P_VI_DEF|P_RALL,
			    (char_u *)&p_ea,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"equalprg",    "ep",   P_STRING|P_EXPAND|P_VI_DEF,
			    (char_u *)&p_ep,
			    {(char_u *)"", (char_u *)0L}},
    {"errorbells",  "eb",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_eb,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"errorfile",   "ef",   P_STRING|P_EXPAND|P_VI_DEF,
#ifdef QUICKFIX
			    (char_u *)&p_ef,
			    {(char_u *)ERRORFILE, (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"errorformat", "efm",  P_STRING|P_VI_DEF|P_COMMA,
#ifdef QUICKFIX
			    (char_u *)&p_efm,
			    {(char_u *)EFM_DFLT, (char_u *)0L},
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"esckeys",	    "ek",   P_BOOL|P_VIM,
			    (char_u *)&p_ek,
			    {(char_u *)FALSE, (char_u *)TRUE}},
    {"eventignore", "ei",   P_STRING|P_VI_DEF|P_COMMA,
#ifdef AUTOCMD
			    (char_u *)&p_ei,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"expandtab",   "et",   P_BOOL|P_IND|P_VI_DEF|P_VIM,
			    (char_u *)PV_ET,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"exrc",	    "ex",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_exrc,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"fileencoding", "fe",   P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_RSTAT,
#ifdef MULTI_BYTE
			    (char_u *)PV_FE,
			    {(char_u *)FE_DFLT, (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"fileformat",  "ff",   P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_RSTAT|P_NO_MKRC,
			    (char_u *)PV_FF,
			    {(char_u *)FF_DFLT, (char_u *)0L}},
    {"fileformats", "ffs",  P_STRING|P_VIM|P_COMMA,
			    (char_u *)&p_ffs,
			    {(char_u *)FFS_VI, (char_u *)FFS_DFLT}},
    {"filetype",    "ft",   P_STRING|P_IND|P_ALLOCED|P_VI_DEF,
#ifdef AUTOCMD
			    (char_u *)PV_FT,
			    {(char_u *)"", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"fkmap",	    "fk",   P_BOOL|P_VI_DEF,
#ifdef FKMAP
			    (char_u *)&p_fkmap,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"flash",	    "fl",   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"formatoptions","fo",  P_STRING|P_IND|P_ALLOCED|P_VIM,
			    (char_u *)PV_FO,
			    {(char_u *)FO_DFLT_VI, (char_u *)FO_DFLT}},
    {"formatprg",   "fp",   P_STRING|P_EXPAND|P_VI_DEF,
			    (char_u *)&p_fp,
			    {(char_u *)"", (char_u *)0L}},
    {"gdefault",    "gd",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_gd,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"graphic",	    "gr",   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"grepformat",  "gfm",  P_STRING|P_VI_DEF|P_COMMA,
#ifdef QUICKFIX
			    (char_u *)&p_gefm,
			    {(char_u *)GEFM_DFLT, (char_u *)0L},
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"grepprg",	    "gp",   P_STRING|P_EXPAND|P_VI_DEF,
#ifdef QUICKFIX
			    (char_u *)&p_gp,
			    {
# ifdef WIN32
			    /* may be changed to "grep -n" in os_win32.c */
			    (char_u *)"findstr /n",
# else
			    (char_u *)"grep -n",
# endif
			    (char_u *)0L},
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"guicursor",    "gcr",  P_STRING|P_VI_DEF|P_COMMA,
#ifdef CURSOR_SHAPE
			    (char_u *)&p_guicursor,
			    {
# ifdef USE_GUI
				(char_u *)"n-v-c:block-Cursor,ve:ver35-Cursor,o:hor50-Cursor,i-ci:ver25-Cursor,r-cr:hor20-Cursor,sm:block-Cursor-blinkwait175-blinkoff150-blinkon175",
# else	/* MSDOS or Win32 console */
				(char_u *)"n-v-c:block,o:hor50,i-ci:hor10,r-cr:hor30,sm:block",
# endif
				    (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
				    },
    {"guifont",	    "gfn",  P_STRING|P_VI_DEF|P_RCLR|P_COMMA,
#ifdef USE_GUI
			    (char_u *)&p_guifont,
			    {(char_u *)"", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
				    },
    {"guifontset",  "gfs",  P_STRING|P_VI_DEF|P_RCLR|P_COMMA,
#if defined(USE_GUI) && defined(USE_FONTSET)
			    (char_u *)&p_guifontset,
			    {(char_u *)"", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
				    },
    {"guiheadroom", "ghr",  P_NUM|P_VI_DEF,
#if defined(USE_GUI_GTK) || defined(USE_GUI_X11)
			    (char_u *)&p_ghr,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)50L, (char_u *)0L}},
    {"guioptions",  "go",   P_STRING|P_VI_DEF|P_RALL,
#if defined(USE_GUI)
			    (char_u *)&p_go,
# if defined(USE_GUI_GTK) || defined(USE_GUI_WIN32)
			    {(char_u *)"agimrtT", (char_u *)0L}
# else
#  ifdef UNIX
			    {(char_u *)"agimrt", (char_u *)0L}
#  else
			    {(char_u *)"gmrt", (char_u *)0L}
#  endif
# endif
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
				    },
    {"guipty",	    NULL,   P_BOOL|P_VI_DEF,
#if defined(USE_GUI)
			    (char_u *)&p_guipty,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)TRUE, (char_u *)0L}},
    {"hardtabs",    "ht",   P_NUM|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}},
    {"helpfile",    "hf",   P_STRING|P_EXPAND|P_VI_DEF,
			    (char_u *)&p_hf,
			    {(char_u *)VIM_HLP, (char_u *)0L}},
    {"helpheight",  "hh",   P_NUM|P_VI_DEF,
			    (char_u *)&p_hh,
			    {(char_u *)20L, (char_u *)0L}},
    {"hidden",	    "hid",  P_BOOL|P_VI_DEF,
			    (char_u *)&p_hid,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"highlight",   "hl",   P_STRING|P_VI_DEF|P_RCLR|P_COMMA,
			    (char_u *)&p_hl,
			    {(char_u *)"8:SpecialKey,@:NonText,d:Directory,e:ErrorMsg,i:IncSearch,l:Search,m:MoreMsg,M:ModeMsg,n:LineNr,r:Question,s:StatusLine,S:StatusLineNC,t:Title,v:Visual,V:VisualNOS,w:WarningMsg,W:WildMenu",
				(char_u *)0L}},
    {"history",	    "hi",   P_NUM|P_VIM,
			    (char_u *)&p_hi,
			    {(char_u *)0L, (char_u *)20L}},
    {"hkmap",	    "hk",   P_BOOL|P_VI_DEF|P_VIM,
#ifdef RIGHTLEFT
			    (char_u *)&p_hkmap,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"hkmapp",	    "hkp",  P_BOOL|P_VI_DEF|P_VIM,
#ifdef RIGHTLEFT
			    (char_u *)&p_hkmapp,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"hlsearch",    "hls",  P_BOOL|P_VI_DEF|P_VIM|P_RALL,
			    (char_u *)&p_hls,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"icon",	    NULL,   P_BOOL|P_VI_DEF,
#ifdef WANT_TITLE
			    (char_u *)&p_icon,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"iconstring",  NULL,   P_STRING|P_VI_DEF,
#ifdef WANT_TITLE
			    (char_u *)&p_iconstring,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"ignorecase",  "ic",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_ic,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"include",	    "inc",  P_STRING|P_VI_DEF,
			    (char_u *)&p_inc,
			    {(char_u *)"^#\\s*include", (char_u *)0L}},
    {"incsearch",   "is",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_is,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"infercase",   "inf",  P_BOOL|P_IND|P_VI_DEF,
			    (char_u *)PV_INF,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"insertmode",  "im",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_im,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"isfname",	    "isf",  P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_isf,
			    {
#ifdef BACKSLASH_IN_FILENAME
			    (char_u *)"@,48-57,/,.,-,_,+,,,$,:,@-@,!,\\,~",
#else
# ifdef AMIGA
			    (char_u *)"@,48-57,/,.,-,_,+,,,$,:",
# else /* UNIX et al. */
			    (char_u *)"@,48-57,/,.,-,_,+,,,$,~",
# endif
#endif
				(char_u *)0L}},
    {"isident",	    "isi",  P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_isi,
			    {
#if defined(MSDOS) || defined(MSWIN) || defined(OS2)
			    (char_u *)"@,48-57,_,128-167,224-235",
#else
			    (char_u *)"@,48-57,_,192-255",
#endif
				(char_u *)0L}},
    {"iskeyword",   "isk",  P_STRING|P_IND|P_ALLOCED|P_VIM|P_COMMA,
			    (char_u *)PV_ISK,
			    {(char_u *)"@,48-57,_",
# if defined(MSDOS) || defined(MSWIN) || defined(OS2)
				(char_u *)"@,48-57,_,128-167,224-235"
# else
				(char_u *)"@,48-57,_,192-255"
# endif
				}},
    {"isprint",	    "isp",  P_STRING|P_VI_DEF|P_RALL|P_COMMA,
			    (char_u *)&p_isp,
			    {
#if defined(MSDOS) || defined(MSWIN) || defined(OS2) || defined(macintosh)
			    (char_u *)"@,~-255",
#else
			    (char_u *)"@,161-255",
#endif
				(char_u *)0L}},
    {"joinspaces",  "js",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_js,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"key",	    NULL,    P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_NO_MKRC,
#ifdef CRYPTV
			    (char_u *)PV_KEY,
			    {(char_u *)"", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"keymodel",    "km",   P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_km,
			    {(char_u *)"", (char_u *)0L}},
    {"keywordprg",  "kp",   P_STRING|P_EXPAND|P_VI_DEF,
			    (char_u *)&p_kp,
			    {
#if defined(MSDOS) || defined(MSWIN)
			    (char_u *)"",
#else
#ifdef VMS
			    (char_u *)"help",
#else
# if defined(OS2)
			    (char_u *)"view /",
# else
#  ifdef USEMAN_S
			    (char_u *)"man -s",
#  else
			    (char_u *)"man",
#  endif
# endif
#endif
#endif
				(char_u *)0L}},
    {"langmap",     "lmap", P_STRING|P_VI_DEF|P_COMMA,
#ifdef HAVE_LANGMAP
			    (char_u *)&p_langmap,
			    {(char_u *)"",	/* unmatched } */
#else
			    (char_u *)NULL,
			    {(char_u *)NULL,
#endif
				(char_u *)0L}},
    {"laststatus",  "ls",   P_NUM|P_VI_DEF|P_RALL,
			    (char_u *)&p_ls,
			    {(char_u *)1L, (char_u *)0L}},
    {"lazyredraw",  "lz",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_lz,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"linebreak",   "lbr",  P_BOOL|P_IND|P_VI_DEF|P_RBUF,
#ifdef LINEBREAK
			    (char_u *)PV_LBR,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"lines",	    NULL,   P_NUM|P_NODEFAULT|P_NO_MKRC|P_VI_DEF|P_RCLR,
			    (char_u *)&Rows,
			    {
#if defined(MSDOS) || defined(WIN32) || defined(OS2)
			    (char_u *)25L,
#else
			    (char_u *)24L,
#endif
					    (char_u *)0L}},
    {"lisp",	    NULL,   P_BOOL|P_IND|P_VI_DEF,
			    (char_u *)PV_LISP,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"list",	    NULL,   P_BOOL|P_IND|P_VI_DEF|P_RBUF,
			    (char_u *)PV_LIST,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"listchars",   "lcs",  P_STRING|P_VI_DEF|P_RALL|P_COMMA,
			    (char_u *)&p_lcs,
			    {(char_u *)"eol:$", (char_u *)0L}},
    {"magic",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_magic,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"makeef",	    "mef",   P_STRING|P_EXPAND|P_VI_DEF,
#ifdef QUICKFIX
			    (char_u *)&p_mef,
			    {(char_u *)MAKEEF, (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"makeprg",	    "mp",   P_STRING|P_EXPAND|P_VI_DEF,
#ifdef QUICKFIX
			    (char_u *)&p_mp,
			    {(char_u *)"make", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"matchpairs",  "mps",  P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
			    (char_u *)PV_MPS,
			    {(char_u *)"(:),{:},[:]", (char_u *)0L}},
    {"matchtime",   "mat",  P_NUM|P_VI_DEF,
			    (char_u *)&p_mat,
			    {(char_u *)5L, (char_u *)0L}},
    {"maxfuncdepth", "mfd", P_NUM|P_VI_DEF,
#ifdef WANT_EVAL
			    (char_u *)&p_mfd,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)100L, (char_u *)0L}},
    {"maxmapdepth", "mmd",  P_NUM|P_VI_DEF,
			    (char_u *)&p_mmd,
			    {(char_u *)1000L, (char_u *)0L}},
    {"maxmem",	    "mm",   P_NUM|P_VI_DEF,
			    (char_u *)&p_mm,
			    {(char_u *)MAXMEM, (char_u *)0L}},
    {"maxmemtot",   "mmt",  P_NUM|P_VI_DEF,
			    (char_u *)&p_mmt,
			    {(char_u *)MAXMEMTOT, (char_u *)0L}},
    {"mesg",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"modeline",    "ml",   P_BOOL|P_IND|P_VIM,
			    (char_u *)PV_ML,
			    {(char_u *)FALSE, (char_u *)TRUE}},
    {"modelines",   "mls",  P_NUM|P_VI_DEF,
			    (char_u *)&p_mls,
			    {(char_u *)5L, (char_u *)0L}},
    {"modified",    "mod",  P_BOOL|P_IND|P_NO_MKRC|P_VI_DEF|P_RSTAT,
			    (char_u *)PV_MOD,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"more",	    NULL,   P_BOOL|P_VIM,
			    (char_u *)&p_more,
			    {(char_u *)FALSE, (char_u *)TRUE}},
    {"mouse",	    NULL,   P_STRING|P_VI_DEF,
			    (char_u *)&p_mouse,
			    {
#if defined(MSDOS) || defined(WIN32)
				(char_u *)"a",
#else
				(char_u *)"",
#endif
				(char_u *)0L}},
    {"mousefocus",   "mousef", P_BOOL|P_VI_DEF,
#ifdef USE_GUI
			    (char_u *)&p_mousef,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"mousehide",   "mh",   P_BOOL|P_VI_DEF,
#ifdef USE_GUI
			    (char_u *)&p_mh,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"mousemodel",  "mousem", P_STRING|P_VI_DEF,
			    (char_u *)&p_mousem,
			    {
#if defined(MSDOS) || defined(MSWIN)
				(char_u *)"popup",
#else
# if defined(macintosh)
				(char_u *)"popup_setpos",
# else
				(char_u *)"extend",
# endif
#endif
				(char_u *)0L}},
    {"mousetime",   "mouset",	P_NUM|P_VI_DEF,
			    (char_u *)&p_mouset,
			    {(char_u *)500L, (char_u *)0L}},
    {"novice",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"nrformats",   "nf",   P_STRING|P_IND|P_ALLOCED|P_VI_DEF|P_COMMA,
			    (char_u *)PV_NF,
			    {(char_u *)"octal,hex", (char_u *)0L}},
    {"number",	    "nu",   P_BOOL|P_IND|P_VI_DEF|P_RBUF,
			    (char_u *)PV_NU,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"open",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"optimize",    "opt",  P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"osfiletype",  "oft",  P_STRING|P_IND|P_ALLOCED|P_VI_DEF,
#ifdef WANT_OSFILETYPE
			    (char_u *)PV_OFT,
			    {(char_u *)OFT_DFLT, (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"paragraphs",  "para", P_STRING|P_VI_DEF,
			    (char_u *)&p_para,
			    {(char_u *)"IPLPPPQPP LIpplpipbp", (char_u *)0L}},
    {"paste",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_paste,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"pastetoggle", "pt",   P_STRING|P_VI_DEF,
			    (char_u *)&p_pt,
			    {(char_u *)"", (char_u *)0L}},
    {"patchmode",   "pm",   P_STRING|P_VI_DEF,
			    (char_u *)&p_pm,
			    {(char_u *)"", (char_u *)0L}},
    {"path",	    "pa",   P_STRING|P_EXPAND|P_VI_DEF|P_COMMA,
			    (char_u *)&p_path,
			    {
#if defined AMIGA || defined MSDOS || defined MSWIN
			    (char_u *)".,,",
#else
# if defined(__EMX__)
			    (char_u *)".,/emx/include,,",
# else /* Unix, probably */
			    (char_u *)".,/usr/include,,",
# endif
#endif
				(char_u *)0L}},
    {"previewheight", "pvh",P_NUM|P_VI_DEF,
			    (char_u *)&p_pvh,
			    {(char_u *)12L, (char_u *)0L}},
    {"prompt",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"readonly",    "ro",   P_BOOL|P_IND|P_VI_DEF|P_RSTAT,
			    (char_u *)PV_RO,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"redraw",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"remap",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_remap,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"report",	    NULL,   P_NUM|P_VI_DEF,
			    (char_u *)&p_report,
			    {(char_u *)2L, (char_u *)0L}},
    {"restorescreen", "rs", P_BOOL|P_VI_DEF,
#ifdef WIN32
			    (char_u *)&p_rs,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)TRUE, (char_u *)0L}},
    {"revins",	    "ri",   P_BOOL|P_VI_DEF|P_VIM,
#ifdef RIGHTLEFT
			    (char_u *)&p_ri,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"rightleft",   "rl",   P_BOOL|P_IND|P_VI_DEF|P_RBUF,
#ifdef RIGHTLEFT
			    (char_u *)PV_RL,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"ruler",	    "ru",   P_BOOL|P_VI_DEF|P_VIM|P_RSTAT,
#ifdef CMDLINE_INFO
			    (char_u *)&p_ru,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"rulerformat", "ruf",  P_STRING|P_VI_DEF|P_ALLOCED|P_RSTAT,
#ifdef STATUSLINE
			    (char_u *)&p_ruf,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"scroll",	    "scr",  P_NUM|P_IND|P_NO_MKRC|P_VI_DEF,
			    (char_u *)PV_SCROLL,
			    {(char_u *)12L, (char_u *)0L}},
    {"scrollbind",  "scb",  P_BOOL|P_IND|P_VI_DEF,
#ifdef SCROLLBIND
			    (char_u *)PV_SCBIND,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"scrolljump",  "sj",   P_NUM|P_VI_DEF|P_VIM,
			    (char_u *)&p_sj,
			    {(char_u *)1L, (char_u *)0L}},
    {"scrolloff",   "so",   P_NUM|P_VI_DEF|P_VIM|P_RALL,
			    (char_u *)&p_so,
			    {(char_u *)0L, (char_u *)0L}},
    {"scrollopt",   "sbo",  P_STRING|P_VI_DEF|P_COMMA,
#ifdef SCROLLBIND
			    (char_u *)&p_sbo,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"ver,jump", (char_u *)0L}},
    {"sections",    "sect", P_STRING|P_VI_DEF,
			    (char_u *)&p_sections,
			    {(char_u *)"SHNHH HUnhsh", (char_u *)0L}},
    {"secure",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_secure,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"selection",   "sel",  P_STRING|P_VI_DEF,
			    (char_u *)&p_sel,
			    {(char_u *)"inclusive", (char_u *)0L}},
    {"selectmode",  "slm",  P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_slm,
			    {(char_u *)"", (char_u *)0L}},
    {"sessionoptions", "ssop", P_STRING|P_VI_DEF|P_COMMA,
#ifdef MKSESSION
			    (char_u *)&p_sessopt,
			    {(char_u *)"buffers,winsize,options,help,blank",
							       (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"shell",	    "sh",   P_STRING|P_EXPAND|P_VI_DEF,
			    (char_u *)&p_sh,
			    {
#ifdef VMS
			    (char_u *)"",
#else
# if defined(MSDOS)
			    (char_u *)"command",
# else
#  if defined(WIN16)
			    (char_u *)"command.com",
#  else
#   if defined(WIN32)
			    (char_u *)"",	/* set in set_init_1() */
#   else
#    if defined(OS2)
			    (char_u *)"cmd.exe",
#    else
#     if defined(ARCHIE)
			    (char_u *)"gos",
#     else
			    (char_u *)"sh",
#     endif
#    endif
#   endif
#  endif
# endif
#endif /* VMS */
				(char_u *)0L}},
    {"shellcmdflag","shcf", P_STRING|P_VI_DEF,
			    (char_u *)&p_shcf,
			    {
#if defined(MSDOS) || defined(MSWIN)
			    (char_u *)"/c",
#else
# if defined(OS2)
			    (char_u *)"/c",
# else
			    (char_u *)"-c",
# endif
#endif
				(char_u *)0L}},
    {"shellpipe",   "sp",   P_STRING|P_VI_DEF,
#ifdef QUICKFIX
			    (char_u *)&p_sp,
			    {
#if defined(UNIX) || defined(OS2)
# ifdef ARCHIE
			    (char_u *)"2>",
# else
			    (char_u *)"| tee",
# endif
#else
			    (char_u *)">",
#endif
				(char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
    },
    {"shellquote",  "shq",  P_STRING|P_VI_DEF,
			    (char_u *)&p_shq,
			    {(char_u *)"", (char_u *)0L}},
    {"shellredir",  "srr",  P_STRING|P_VI_DEF,
			    (char_u *)&p_srr,
			    {(char_u *)">", (char_u *)0L}},
    {"shellslash",  "ssl",   P_BOOL|P_VI_DEF,
#ifdef BACKSLASH_IN_FILENAME
			    (char_u *)&p_ssl,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"shelltype",   "st",   P_NUM|P_VI_DEF,
#ifdef AMIGA
			    (char_u *)&p_st,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)0L, (char_u *)0L}},
    {"shellxquote", "sxq",  P_STRING|P_VI_DEF,
			    (char_u *)&p_sxq,
			    {
#if defined(UNIX) && defined(USE_SYSTEM) && !defined(__EMX__)
			    (char_u *)"\"",
#else
			    (char_u *)"",
#endif
				(char_u *)0L}},
    {"shiftround",  "sr",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_sr,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"shiftwidth",  "sw",   P_NUM|P_IND|P_VI_DEF,
			    (char_u *)PV_SW,
			    {(char_u *)8L, (char_u *)0L}},
    {"shortmess",   "shm",  P_STRING|P_VIM,
			    (char_u *)&p_shm,
			    {(char_u *)"", (char_u *)"filnxtToO"}},
    {"shortname",   "sn",   P_BOOL|P_IND|P_VI_DEF,
#ifdef SHORT_FNAME
			    (char_u *)NULL,
#else
			    (char_u *)PV_SN,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"showbreak",   "sbr",  P_STRING|P_VI_DEF|P_RALL,
#ifdef LINEBREAK
			    (char_u *)&p_sbr,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"showcmd",	    "sc",   P_BOOL|P_VIM,
#ifdef CMDLINE_INFO
			    (char_u *)&p_sc,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE,
#ifdef UNIX
				(char_u *)FALSE
#else
				(char_u *)TRUE
#endif
				}},
    {"showfulltag", "sft",  P_BOOL|P_VI_DEF,
			    (char_u *)&p_sft,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"showmatch",   "sm",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_sm,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"showmode",    "smd",  P_BOOL|P_VIM,
			    (char_u *)&p_smd,
			    {(char_u *)FALSE, (char_u *)TRUE}},
    {"sidescroll",  "ss",   P_NUM|P_VI_DEF,
			    (char_u *)&p_ss,
			    {(char_u *)0L, (char_u *)0L}},
    {"slowopen",    "slow", P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"smartcase",   "scs",  P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_scs,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"smartindent", "si",   P_BOOL|P_IND|P_VI_DEF|P_VIM,
#ifdef SMARTINDENT
			    (char_u *)PV_SI,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"smarttab",    "sta",  P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_sta,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"softtabstop", "sts",  P_NUM|P_IND|P_VI_DEF|P_VIM,
			    (char_u *)PV_STS,
			    {(char_u *)0L, (char_u *)0L}},
    {"sourceany",   NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"splitbelow",  "sb",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_sb,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"startofline", "sol",  P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_sol,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"statusline"  ,"stl",  P_STRING|P_VI_DEF|P_ALLOCED|P_RSTAT,
#ifdef STATUSLINE
			    (char_u *)&p_stl,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"suffixes",    "su",   P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_su,
			    {(char_u *)".bak,~,.o,.h,.info,.swp,.obj",
				(char_u *)0L}},
    {"swapfile",    "swf",  P_BOOL|P_IND|P_VI_DEF|P_RSTAT,
			    (char_u *)PV_SWF,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"swapsync",    "sws",  P_STRING|P_VI_DEF,
			    (char_u *)&p_sws,
			    {(char_u *)"fsync", (char_u *)0L}},
    {"switchbuf",   "swb",  P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_swb,
			    {(char_u *)"", (char_u *)0L}},
    {"syntax",	    "syn",  P_STRING|P_IND|P_ALLOCED|P_VI_DEF,
#ifdef SYNTAX_HL
			    (char_u *)PV_SYN,
			    {(char_u *)"", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}
#endif
			    },
    {"tabstop",	    "ts",   P_NUM|P_IND|P_VI_DEF|P_RBUF,
			    (char_u *)PV_TS,
			    {(char_u *)8L, (char_u *)0L}},
    {"tagbsearch",  "tbs",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_tbs,
#ifdef VMS	/* binary searching doesn't appear to work on VMS */
			    {(char_u *)0L, (char_u *)0L}},
#else
			    {(char_u *)TRUE, (char_u *)0L}},
#endif
    {"taglength",   "tl",   P_NUM|P_VI_DEF,
			    (char_u *)&p_tl,
			    {(char_u *)0L, (char_u *)0L}},
    {"tagrelative", "tr",   P_BOOL|P_VIM,
			    (char_u *)&p_tr,
			    {(char_u *)FALSE, (char_u *)TRUE}},
    {"tags",	    "tag",  P_STRING|P_EXPAND|P_VI_DEF|P_COMMA,
			    (char_u *)&p_tags,
			    {
#ifdef EMACS_TAGS
			    (char_u *)"./tags,./TAGS,tags,TAGS",
#else
			    (char_u *)"./tags,tags",
#endif
				(char_u *)0L}},
    {"tagstack",    "tgst", P_BOOL|P_VI_DEF,
			    (char_u *)&p_tgst,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"term",	    NULL,   P_STRING|P_EXPAND|P_NODEFAULT|P_NO_MKRC|P_VI_DEF|P_RALL,
			    (char_u *)&T_NAME,
			    {(char_u *)"", (char_u *)0L}},
    {"terse",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_terse,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"textauto",    "ta",   P_BOOL|P_VIM,
			    (char_u *)&p_ta,
			    {(char_u *)TA_DFLT, (char_u *)TRUE}},
    {"textmode",    "tx",   P_BOOL|P_IND|P_VI_DEF|P_NO_MKRC,
			    (char_u *)PV_TX,
			    {
#ifdef USE_CRNL
			    (char_u *)TRUE,
#else
			    (char_u *)FALSE,
#endif
				(char_u *)0L}},
    {"textwidth",   "tw",   P_NUM|P_IND|P_VI_DEF|P_VIM,
			    (char_u *)PV_TW,
			    {(char_u *)0L, (char_u *)0L}},
    {"tildeop",	    "top",  P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_to,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"timeout",	    "to",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_timeout,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"timeoutlen",  "tm",   P_NUM|P_VI_DEF,
			    (char_u *)&p_tm,
			    {(char_u *)1000L, (char_u *)0L}},
    {"title",	    NULL,   P_BOOL|P_VI_DEF,
#ifdef WANT_TITLE
			    (char_u *)&p_title,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"titlelen",    NULL,   P_NUM|P_VI_DEF,
#ifdef WANT_TITLE
			    (char_u *)&p_titlelen,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)85L, (char_u *)0L}},
    {"titleold",    NULL,   P_STRING|P_VI_DEF,
#ifdef WANT_TITLE
			    (char_u *)&p_titleold,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"Thanks for flying Vim", (char_u *)0L}},
    {"titlestring", NULL,   P_STRING|P_VI_DEF,
#ifdef WANT_TITLE
			    (char_u *)&p_titlestring,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
#if defined(USE_GUI_GTK) && defined(USE_TOOLBAR)
    {"toolbar",     "tb",   P_STRING|P_COMMA|P_VI_DEF,
			    (char_u *)&p_toolbar,
			    {(char_u *)"icons,tooltips", (char_u *)0L}},
#endif
    {"ttimeout",    NULL,   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_ttimeout,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"ttimeoutlen", "ttm",  P_NUM|P_VI_DEF,
			    (char_u *)&p_ttm,
			    {(char_u *)-1L, (char_u *)0L}},
    {"ttybuiltin",  "tbi",  P_BOOL|P_VI_DEF,
			    (char_u *)&p_tbi,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"ttyfast",	    "tf",   P_BOOL|P_NO_MKRC|P_VI_DEF,
			    (char_u *)&p_tf,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"ttymouse",    "ttym", P_STRING|P_NODEFAULT|P_NO_MKRC|P_VI_DEF,
			    (char_u *)&p_ttym,
			    {(char_u *)"", (char_u *)0L}},
    {"ttyscroll",   "tsl",  P_NUM|P_VI_DEF,
			    (char_u *)&p_ttyscroll,
			    {(char_u *)999L, (char_u *)0L}},
    {"ttytype",	    "tty",  P_STRING|P_EXPAND|P_NODEFAULT|P_NO_MKRC|P_VI_DEF|P_RALL,
			    (char_u *)&T_NAME,
			    {(char_u *)"", (char_u *)0L}},
    {"undolevels",  "ul",   P_NUM|P_VI_DEF,
			    (char_u *)&p_ul,
			    {
#if defined(UNIX) || defined(WIN32) || defined(OS2) || defined(VMS)
			    (char_u *)1000L,
#else
			    (char_u *)100L,
#endif
				(char_u *)0L}},
    {"updatecount", "uc",   P_NUM|P_VI_DEF,
			    (char_u *)&p_uc,
			    {(char_u *)200L, (char_u *)0L}},
    {"updatetime",  "ut",   P_NUM|P_VI_DEF,
			    (char_u *)&p_ut,
			    {(char_u *)4000L, (char_u *)0L}},
    {"verbose",	    "vbs",  P_NUM|P_VI_DEF,
			    (char_u *)&p_verbose,
			    {(char_u *)0L, (char_u *)0L}},
    {"viminfo",	    "vi",   P_STRING|P_VI_DEF|P_COMMA,
#ifdef VIMINFO
			    (char_u *)&p_viminfo,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"visualbell",  "vb",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_vb,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"w300",	    NULL,   P_NUM|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}},
    {"w1200",	    NULL,   P_NUM|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}},
    {"w9600",	    NULL,   P_NUM|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}},
    {"warn",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_warn,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"weirdinvert", "wiv",  P_BOOL|P_VI_DEF|P_RCLR,
			    (char_u *)&p_wiv,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"whichwrap",   "ww",   P_STRING|P_VIM|P_COMMA,
			    (char_u *)&p_ww,
			    {(char_u *)"", (char_u *)"b,s"}},
    {"wildchar",    "wc",   P_NUM|P_VIM,
			    (char_u *)&p_wc,
			    {(char_u *)(long)Ctrl('E'), (char_u *)(long)TAB}},
    {"wildcharm",   "wcm",   P_NUM|P_VI_DEF,
			    (char_u *)&p_wcm,
			    {(char_u *)0L, (char_u *)0L}},
    {"wildignore",  "wig",  P_STRING|P_VI_DEF|P_COMMA,
#ifdef WILDIGNORE
			    (char_u *)&p_wig,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)"", (char_u *)0L}},
    {"wildmenu",    "wmnu", P_BOOL|P_VI_DEF,
#ifdef WILDMENU
			    (char_u *)&p_wmnu,
#else
			    (char_u *)NULL,
#endif
			    {(char_u *)FALSE, (char_u *)0L}},
    {"wildmode",    "wim",  P_STRING|P_VI_DEF|P_COMMA,
			    (char_u *)&p_wim,
			    {(char_u *)"full", (char_u *)0L}},
    {"winaltkeys",  "wak",  P_STRING|P_VI_DEF,
#ifdef HAS_WAK
			    (char_u *)&p_wak,
			    {(char_u *)"menu", (char_u *)0L}
#else
			    (char_u *)NULL,
			    {(char_u *)NULL, (char_u *)0L}
#endif
			    },
    {"window",	    "wi",   P_NUM|P_VI_DEF,
			    (char_u *)NULL,
			    {(char_u *)0L, (char_u *)0L}},
    {"winheight",   "wh",   P_NUM|P_VI_DEF,
			    (char_u *)&p_wh,
			    {(char_u *)1L, (char_u *)0L}},
    {"winminheight", "wmh", P_NUM|P_VI_DEF,
			    (char_u *)&p_wmh,
			    {(char_u *)1L, (char_u *)0L}},
    {"wrap",	    NULL,   P_BOOL|P_IND|P_VI_DEF|P_RBUF,
			    (char_u *)PV_WRAP,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"wrapmargin",  "wm",   P_NUM|P_IND|P_VI_DEF,
			    (char_u *)PV_WM,
			    {(char_u *)0L, (char_u *)0L}},
    {"wrapscan",    "ws",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_ws,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"write",	    NULL,   P_BOOL|P_VI_DEF,
			    (char_u *)&p_write,
			    {(char_u *)TRUE, (char_u *)0L}},
    {"writeany",    "wa",   P_BOOL|P_VI_DEF,
			    (char_u *)&p_wa,
			    {(char_u *)FALSE, (char_u *)0L}},
    {"writebackup", "wb",   P_BOOL|P_VI_DEF|P_VIM,
			    (char_u *)&p_wb,
			    {
#ifdef WRITEBACKUP
			    (char_u *)TRUE,
#else
			    (char_u *)FALSE,
#endif
				(char_u *)0L}},
    {"writedelay",  "wd",   P_NUM|P_VI_DEF,
			    (char_u *)&p_wd,
			    {(char_u *)0L, (char_u *)0L}},

/* terminal output codes */
#define p_term(sss, vvv)   {sss, NULL, P_STRING|P_VI_DEF|P_RALL, \
			    (char_u *)&vvv, \
			    {(char_u *)"", (char_u *)0L}},

    p_term("t_AB", T_CAB)
    p_term("t_AF", T_CAF)
    p_term("t_AL", T_CAL)
    p_term("t_al", T_AL)
    p_term("t_bc", T_BC)
    p_term("t_cd", T_CD)
    p_term("t_ce", T_CE)
    p_term("t_cl", T_CL)
    p_term("t_cm", T_CM)
    p_term("t_Co", T_CCO)
    p_term("t_CS", T_CCS)
    p_term("t_cs", T_CS)
    p_term("t_da", T_DA)
    p_term("t_db", T_DB)
    p_term("t_DL", T_CDL)
    p_term("t_dl", T_DL)
    p_term("t_fs", T_FS)
    p_term("t_IE", T_CIE)
    p_term("t_IS", T_CIS)
    p_term("t_ke", T_KE)
    p_term("t_ks", T_KS)
    p_term("t_le", T_LE)
    p_term("t_mb", T_MB)
    p_term("t_md", T_MD)
    p_term("t_me", T_ME)
    p_term("t_mr", T_MR)
    p_term("t_ms", T_MS)
    p_term("t_nd", T_ND)
    p_term("t_op", T_OP)
    p_term("t_RI", T_CRI)
    p_term("t_RV", T_CRV)
    p_term("t_Sb", T_CSB)
    p_term("t_Sf", T_CSF)
    p_term("t_se", T_SE)
    p_term("t_so", T_SO)
    p_term("t_sr", T_SR)
    p_term("t_ts", T_TS)
    p_term("t_te", T_TE)
    p_term("t_ti", T_TI)
    p_term("t_ue", T_UE)
    p_term("t_us", T_US)
    p_term("t_vb", T_VB)
    p_term("t_ve", T_VE)
    p_term("t_vi", T_VI)
    p_term("t_vs", T_VS)
    p_term("t_WP", T_CWP)
    p_term("t_WS", T_CWS)
    p_term("t_xs", T_XS)
    p_term("t_ZH", T_CZH)
    p_term("t_ZR", T_CZR)

/* terminal key codes are not in here */

    {NULL, NULL, 0, NULL, {NULL, NULL}}		/* end marker */
};

#define PARAM_COUNT (sizeof(options) / sizeof(struct vimoption))

static char *(p_bg_values[]) = {"light", "dark", NULL};
static char *(p_nf_values[]) = {"octal", "hex", NULL};
static char *(p_ff_values[]) = {FF_UNIX, FF_DOS, FF_MAC, NULL};
#ifdef MULTI_BYTE
static char *(p_fe_values[]) = {FE_ANSI, FE_UNICODE, FE_DBJPN, FE_DBKOR,
    FE_DBCHT, FE_DBCHS, FE_HEBREW, FE_FARSI , NULL};
#endif
#ifdef HAS_WAK
static char *(p_wak_values[]) = {"yes", "menu", "no", NULL};
#endif
#ifdef MKSESSION
static char *(p_sessopt_values[]) = {"buffers", "winpos", "resize", "winsize", "options", "help", "blank", "globals", "slash", "unix", NULL};
#endif
static char *(p_mousem_values[]) = {"extend", "popup", "popup_setpos", "mac", NULL};
static char *(p_slm_values[]) = {"mouse", "key", "cmd", NULL};
static char *(p_sel_values[]) = {"inclusive", "exclusive", "old", NULL};
#if defined(USE_MOUSE) && (defined(UNIX) || defined(VMS))
static char *(p_ttym_values[]) = {"xterm", "xterm2", "dec", "netterm", NULL};
#endif
static char *(p_km_values[]) = {"startsel", "stopsel", NULL};
static char *(p_bsdir_values[]) = {"current", "last", "buffer", NULL};
#ifdef SCROLLBIND
static char *(p_scbopt_values[]) = {"ver", "hor", "jump", NULL};
#endif
static char *(p_swb_values[]) = {"useopen", "split", NULL};
static char *(p_dy_values[]) = {"lastline", NULL};
#ifdef USE_CLIPBOARD
static char *(p_cb_values[]) = {"unnamed", "autoselect", NULL};
#endif
static char *(p_bs_values[]) = {"indent", "eol", "start", NULL};

static void set_option_default __ARGS((int, int));
static void set_options_default __ARGS((int dofree));
static char_u *illegal_char __ARGS((char_u *, int));
#ifdef WANT_TITLE
static void did_set_title __ARGS((int icon));
#endif
static char_u *option_expand __ARGS((int));
static void set_string_option __ARGS((int opt_idx, char_u *value));
static char_u *did_set_string_option __ARGS((int opt_idx, char_u **varp, int new_value_alloced, char_u *oldval, char_u *errbuf));
static char_u *set_bool_option __ARGS((int opt_idx, char_u *varp, int value));
static char_u *set_num_option __ARGS((int opt_idx, char_u *varp, long value, char_u *errbuf));
static void check_redraw __ARGS((int flags));
static int findoption __ARGS((char_u *));
static int find_key_option __ARGS((char_u *));
static void showoptions __ARGS((int));
static int option_not_default __ARGS((struct vimoption *));
static void showoneopt __ARGS((struct vimoption *));
static int  istermoption __ARGS((struct vimoption *));
static char_u *get_varp __ARGS((struct vimoption *));
static void option_value2string __ARGS((struct vimoption *));
#ifdef HAVE_LANGMAP
static void langmap_init __ARGS((void));
static void langmap_set __ARGS((void));
#endif
static void paste_option_changed __ARGS((void));
static void compatible_set __ARGS((void));
#ifdef LINEBREAK
static void fill_breakat_flags __ARGS((void));
#endif
static int check_opt_strings __ARGS((char_u *val, char **values, int));
static int check_opt_wim __ARGS((void));

/*
 * Initialize the options, first part.
 *
 * Called only once from main(), just after creating the first buffer.
 */
    void
set_init_1()
{
    char_u  *p;
    int	    opt_idx;
    long    n;

#ifdef HAVE_LANGMAP
    langmap_init();
#endif

    /* Be Vi compatible by default */
    p_cp = TRUE;

    /*
     * Find default value for 'shell' option.
     */
    if ((p = mch_getenv((char_u *)"SHELL")) != NULL
#if defined(MSDOS) || defined(MSWIN) || defined(OS2)
# ifdef __EMX__
	    || (p = mch_getenv((char_u *)"EMXSHELL")) != NULL
# endif
	    || (p = mch_getenv((char_u *)"COMSPEC")) != NULL
# ifdef WIN32
	    || (p = default_shell()) != NULL
# endif
#endif
							    )
    {
	set_string_default("sh", p);
    }

    /*
     * 'maxmemtot' and 'maxmem' may have to be adjusted for available memory
     */
    opt_idx = findoption((char_u *)"maxmemtot");
    if (options[opt_idx].def_val[VI_DEFAULT] == (char_u *)0L)
    {
#ifdef HAVE_AVAIL_MEM
	n = (mch_avail_mem(FALSE) >> 11);
#else
	n = (0x7fffffff >> 11);
#endif
	options[opt_idx].def_val[VI_DEFAULT] = (char_u *)n;
	opt_idx = findoption((char_u *)"maxmem");
	if ((long)options[opt_idx].def_val[VI_DEFAULT] > n
			  || (long)options[opt_idx].def_val[VI_DEFAULT] == 0L)
	    options[opt_idx].def_val[VI_DEFAULT] = (char_u *)n;
    }

#ifdef USE_GUI_WIN32
    /* force 'shortname' for Win32s */
    if (gui_is_win32s())
	options[findoption((char_u *)"shortname")].def_val[VI_DEFAULT] =
							       (char_u *)TRUE;
#endif

    /*
     * set all the options (except the terminal options) to their default value
     */
    set_options_default(FALSE);

#ifdef USE_GUI
    if (found_reverse_arg)
	set_option_value((char_u *)"bg", 0L, (char_u *)"dark");
#endif

    curbuf->b_p_initialized = TRUE;
    check_buf_options(curbuf);
    check_options();

    /*
     * initialize the table for 'iskeyword' et.al.
     * Must be before option_expand(), because that one needs vim_isIDc()
     */
    init_chartab();

#ifdef LINEBREAK
    /*
     * initialize the table for 'breakat'.
     */
    fill_breakat_flags();
#endif

    /*
     * Expand environment variables and things like "~" for the defaults.
     * If option_expand() returns non-NULL the variable is expanded. This can
     * only happen for non-indirect options.
     * Also set the default to the expanded value, so ":set" does not list
     * them. Don't set the P_ALLOCED flag, because we don't want to free the
     * default.
     */
    for (opt_idx = 0; !istermoption(&options[opt_idx]); opt_idx++)
    {
	p = option_expand(opt_idx);
	if (p != NULL)
	{
	    *(char_u **)options[opt_idx].var = p;
	    /* VIMEXP
	     * Defaults for all expanded options are currently the same for Vi
	     * and Vim.  When this changes, add some code here!  Also need to
	     * split P_DEF_ALLOCED in two.
	     */
	    options[opt_idx].def_val[VI_DEFAULT] = p;
	    options[opt_idx].flags |= P_DEF_ALLOCED;
	}
    }

    /* Initialize the highlight_attr[] table. */
    highlight_changed();

    curbuf->b_start_ffc = *curbuf->b_p_ff;    /* Buffer is unchanged */

    /* Parse default for 'wildmode'  */
    check_opt_wim();

}

/*
 * Set an option to its default value.
 */
    static void
set_option_default(opt_idx, dofree)
    int	    opt_idx;
    int	    dofree;	/* TRUE when old value can be freed */
{
    char_u	*varp;		/* pointer to variable for current option */
    int		dvi;		/* index in def_val[] */
    int		flags;

    varp = get_varp(&(options[opt_idx]));
    flags = options[opt_idx].flags;
    if (varp != NULL)	    /* nothing to do for hidden option */
    {
	if ((flags & P_VI_DEF) || p_cp)
	    dvi = VI_DEFAULT;
	else
	    dvi = VIM_DEFAULT;
	if (flags & P_STRING)
	{
	    /* indirect options are always in allocated memory */
	    if (flags & P_IND)
		set_string_option_direct(NULL, opt_idx,
				      options[opt_idx].def_val[dvi], dofree);
	    else
	    {
		if (dofree && (flags & P_ALLOCED))
		    free_string_option(*(char_u **)(varp));
		*(char_u **)varp = options[opt_idx].def_val[dvi];
		options[opt_idx].flags &= ~P_ALLOCED;
	    }
	}
	else if (flags & P_NUM)
	{
	    if (varp == (char_u *)PV_SCROLL)
		win_comp_scroll(curwin);
	    else
		*(long *)varp = (long)options[opt_idx].def_val[dvi];
	}
	else	/* P_BOOL */
	    /* the cast to long is required for Manx C */
	    *(int *)varp = (int)(long)options[opt_idx].def_val[dvi];
    }
}

/*
 * Set all options (except terminal options) to their default value.
 */
    static void
set_options_default(dofree)
    int	    dofree;		/* may free old value */
{
    int	    i;
    WIN	    *wp;

    for (i = 0; !istermoption(&options[i]); i++)
	if (!(options[i].flags & P_NODEFAULT))
	    set_option_default(i, dofree);

    /* The 'scroll' option must be computed for each window, not only the
     * current one */
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	win_comp_scroll(wp);
}

/*
 * Set the Vi-default value of a string option.
 * Used for 'sh' and 'term'.
 */
    void
set_string_default(name, val)
    char    *name;
    char_u  *val;
{
    char_u	*p;
    int		opt_idx;

    p = vim_strsave(val);
    if (p != NULL)		/* we don't want a NULL */
    {
	opt_idx = findoption((char_u *)name);
	if (options[opt_idx].flags & P_DEF_ALLOCED)
	    vim_free(options[opt_idx].def_val[VI_DEFAULT]);
	options[opt_idx].def_val[VI_DEFAULT] = p;
	options[opt_idx].flags |= P_DEF_ALLOCED;
    }
}

/*
 * Set the Vi-default value of a number option.
 * Used for 'lines' and 'columns'.
 */
    void
set_number_default(name, val)
    char	*name;
    long	val;
{
    options[findoption((char_u *)name)].def_val[VI_DEFAULT] = (char_u *)val;
}

/*
 * Initialize the options, part two: After getting Rows and Columns and
 * setting 'term'.
 */
    void
set_init_2()
{
    /*
     * 'scroll' defaults to half the window height. Note that this default is
     * wrong when the window height changes.
     */
    options[findoption((char_u *)"scroll")].def_val[VI_DEFAULT]
					      = (char_u *)((long_u)Rows >> 1);
    comp_col();

#if !((defined(MSDOS) || defined(OS2) || defined(WIN32)) && !defined(USE_GUI))
    {
	int	idx4;

	/*
	 * Try guessing the value of 'background', depending on the terminal
	 * name.  Only need to check for terminals with a dark background,
	 * that can handle color.
	 */
	idx4 = findoption((char_u *)"bg");
	if (!(options[idx4].flags & P_WAS_SET))
	{
	    if (STRCMP(T_NAME, "linux") == 0)	    /* linux console */
		set_string_option_direct(NULL, idx4, (char_u *)"dark", TRUE);
	}
    }
#endif
}

/*
 * Initialize the options, part three: After reading the .vimrc
 */
    void
set_init_3()
{
#if defined(UNIX) || defined(OS2)
/*
 * Set 'shellpipe' and 'shellredir', depending on the 'shell' option.
 * This is done after other initializations, where 'shell' might have been
 * set, but only if they have not been set before.
 */
    char_u  *p;
    int	    idx_srr;
    int	    do_srr;
#ifdef QUICKFIX
    int	    idx_sp;
    int	    do_sp;
#endif

    idx_srr = findoption((char_u *)"srr");
    do_srr = !(options[idx_srr].flags & P_WAS_SET);
#ifdef QUICKFIX
    idx_sp = findoption((char_u *)"sp");
    do_sp = !(options[idx_sp].flags & P_WAS_SET);
#endif

    /*
     * Isolate the name of the shell:
     * - Skip beyond any path.  E.g., "/usr/bin/csh -f" -> "csh -f".
     * - Remove any argument.  E.g., "csh -f" -> "csh".
     */
    p = gettail(p_sh);
    p = vim_strnsave(p, skiptowhite(p) - p);
    if (p != NULL)
    {
	/*
	 * Default for p_sp is "| tee", for p_srr is ">".
	 * For known shells it is changed here to include stderr.
	 */
	if (	   fnamecmp(p, "csh") == 0
		|| fnamecmp(p, "tcsh") == 0
# ifdef OS2	    /* also check with .exe extension */
		|| fnamecmp(p, "csh.exe") == 0
		|| fnamecmp(p, "tcsh.exe") == 0
# endif
	   )
	{
#ifdef QUICKFIX
	    if (do_sp)
	    {
		p_sp = (char_u *)"|& tee";
		options[idx_sp].def_val[VI_DEFAULT] = p_sp;
	    }
#endif
	    if (do_srr)
	    {
		p_srr = (char_u *)">&";
		options[idx_srr].def_val[VI_DEFAULT] = p_srr;
	    }
	}
	else
# ifndef OS2	/* Always use bourne shell style redirection if we reach this */
	    if (       STRCMP(p, "sh") == 0
		    || STRCMP(p, "ksh") == 0
		    || STRCMP(p, "zsh") == 0
		    || STRCMP(p, "bash") == 0)
# endif
	    {
#ifdef QUICKFIX
		if (do_sp)
		{
		    p_sp = (char_u *)"2>&1| tee";
		    options[idx_sp].def_val[VI_DEFAULT] = p_sp;
		}
#endif
		if (do_srr)
		{
		    p_srr = (char_u *)">%s 2>&1";
		    options[idx_srr].def_val[VI_DEFAULT] = p_srr;
		}
	    }
	vim_free(p);
    }
#endif

#if defined(MSDOS) || defined(WIN32) || defined(OS2)
    /*
     * Set 'shellcmdflag and 'shellquote' depending on the 'shell' option.
     * This is done after other initializations, where 'shell' might have been
     * set, but only if they have not been set before.  Default for p_shcf is
     * "/c", for p_shq is "".  For "sh" like  shells it is changed here to
     * "-c" and "\"", but not for DJGPP, because it starts the shell without
     * command.com.  And for Win32 we need to set p_sxq instead.
     */
    if (strstr((char *)p_sh, "sh") != NULL)
    {
	int	idx3;

	idx3 = findoption((char_u *)"shcf");
	if (!(options[idx3].flags & P_WAS_SET))
	{
	    p_shcf = (char_u *)"-c";
	    options[idx3].def_val[VI_DEFAULT] = p_shcf;
	}

# ifndef DJGPP
#  ifdef WIN32
	/* Somehow Win32 requires the quotes around the redirection too */
	idx3 = findoption((char_u *)"sxq");
	if (!(options[idx3].flags & P_WAS_SET))
	{
	    p_sxq = (char_u *)"\"";
	    options[idx3].def_val[VI_DEFAULT] = p_sxq;
	}
#  else
	idx3 = findoption((char_u *)"shq");
	if (!(options[idx3].flags & P_WAS_SET))
	{
	    p_shq = (char_u *)"\"";
	    options[idx3].def_val[VI_DEFAULT] = p_shq;
	}
#  endif
# endif
    }
#endif

#ifdef WANT_TITLE
    set_title_defaults();
#endif

#ifdef BACKSLASH_IN_FILENAME
    /* If 'shellslash' was set in a vimrc file, need to adjust the file name
     * arguments. */
    if (p_ssl)
    {
	int i;

	for (i = 0; i < arg_file_count; ++i)
	    if (arg_files[i] != NULL)
		slash_adjust(arg_files[i]);
    }
#endif
}

#ifdef USE_GUI
/*
 * Option initializations that can only be done after opening the GUI window.
 */
    void
init_gui_options()
{
    /* Set the 'background' option according to the lightness of the
     * background color. */
    if (!option_was_set((char_u *)"bg")
			       && gui_mch_get_lightness(gui.back_pixel) < 127)
    {
	set_option_value((char_u *)"bg", 0L, (char_u *)"dark");
	highlight_changed();
    }
}
#endif

#ifdef WANT_TITLE
/*
 * 'title' and 'icon' only default to true if they have not been set or reset
 * in .vimrc and we can read the old value.
 * When 'title' and 'icon' have been reset in .vimrc, we won't even check if
 * they can be reset.  This reduces startup time when using X on a remote
 * machine.
 */
    void
set_title_defaults()
{
    int	    idx1;
    long    val;

    /*
     * If GUI is (going to be) used, we can always set the window title and
     * icon name.  Saves a bit of time, because the X11 display server does
     * not need to be contacted.
     */
    idx1 = findoption((char_u *)"title");
    if (!(options[idx1].flags & P_WAS_SET))
    {
#ifdef USE_GUI
	if (gui.starting || gui.in_use)
	    val = TRUE;
	else
#endif
	    val = mch_can_restore_title();
	options[idx1].def_val[VI_DEFAULT] = (char_u *)val;
	p_title = val;
    }
    idx1 = findoption((char_u *)"icon");
    if (!(options[idx1].flags & P_WAS_SET))
    {
#ifdef USE_GUI
	if (gui.starting || gui.in_use)
	    val = TRUE;
	else
#endif
	    val = mch_can_restore_icon();
	options[idx1].def_val[VI_DEFAULT] = (char_u *)val;
	p_icon = val;
    }
}
#endif

/*
 * Parse 'arg' for option settings.
 *
 * 'arg' may be IObuff, but only when no errors can be present and option
 * does not need to be expanded with option_expand().
 *
 * returns FAIL if an error is detected, OK otherwise
 */
    int
do_set(arg, modeline)
    char_u	*arg;		/* option string (may be written to!) */
    int		modeline;	/* TRUE when called for modeline */
{
    int		opt_idx;
    char_u	*errmsg;
    char_u	errbuf[80];
    char_u	*startarg;
    int		prefix;	/* 1: nothing, 0: "no", 2: "inv" in front of name */
    int		nextchar;	    /* next non-white char after option name */
    int		afterchar;	    /* character just after option name */
    int		len;
    int		i;
    long	value;
    int		key;
    int		flags;		    /* flags for current option */
    char_u	*varp = NULL;	    /* pointer to variable for current option */
    int		did_show = FALSE;   /* already showed one value */
    int		adding;		    /* "opt+=arg" */
    int		prepending;	    /* "opt^=arg" */
    int		removing;	    /* "opt-=arg" */

    if (*arg == NUL)
    {
	showoptions(0);
	return OK;
    }

    while (*arg)	/* loop to process all options */
    {
	errmsg = NULL;
	startarg = arg;	    /* remember for error message */

	if (STRNCMP(arg, "all", 3) == 0 && !isalpha(arg[3]))
	{
	    /*
	     * ":set all"  show all options.
	     * ":set all&" set all options to their default value.
	     */
	    arg += 3;
	    if (*arg == '&')
	    {
		++arg;
		set_options_default(TRUE);
	    }
	    else
		showoptions(1);
	}
	else if (STRNCMP(arg, "termcap", 7) == 0)
	{
	    showoptions(2);
	    show_termcodes();
	    arg += 7;
	}
	else
	{
	    prefix = 1;
	    if (STRNCMP(arg, "no", 2) == 0)
	    {
		prefix = 0;
		arg += 2;
	    }
	    else if (STRNCMP(arg, "inv", 3) == 0)
	    {
		prefix = 2;
		arg += 3;
	    }

	    /* find end of name */
	    key = 0;
	    if (*arg == '<')
	    {
		opt_idx = -1;
		/* look out for <t_>;> */
		if (arg[1] == 't' && arg[2] == '_' && arg[3] && arg[4])
		    len = 5;
		else
		{
		    len = 1;
		    while (arg[len] != NUL && arg[len] != '>')
			++len;
		}
		if (arg[len] != '>')
		{
		    errmsg = e_invarg;
		    goto skip;
		}
		arg[len] = NUL;			    /* put NUL after name */
		if (arg[1] == 't' && arg[2] == '_') /* could be term code */
		    opt_idx = findoption(arg + 1);
		arg[len++] = '>';		    /* restore '>' */
		if (opt_idx == -1)
		    key = find_key_option(arg + 1);
	    }
	    else
	    {
		len = 0;
		/*
		 * The two characters after "t_" may not be alphanumeric.
		 */
		if (arg[0] == 't' && arg[1] == '_' && arg[2] && arg[3])
		{
		    len = 4;
		}
		else
		{
		    while (isalnum(arg[len]) || arg[len] == '_')
			++len;
		}
		nextchar = arg[len];
		arg[len] = NUL;			    /* put NUL after name */
		opt_idx = findoption(arg);
		arg[len] = nextchar;		    /* restore nextchar */
		if (opt_idx == -1)
		    key = find_key_option(arg);
	    }

	    if (opt_idx == -1 && key == 0)	/* found a mismatch: skip */
	    {
		errmsg = (char_u *)"Unknown option";
		goto skip;
	    }

	    if (opt_idx >= 0)
	    {
		if (options[opt_idx].var == NULL)   /* hidden option: skip */
		    goto skip;

		flags = options[opt_idx].flags;
		varp = get_varp(&(options[opt_idx]));
	    }
	    else
		flags = P_STRING;

	    /* Disallow changing some options from modelines */
	    if (modeline)
	    {
		if (varp == (char_u *)&p_exrc
			|| varp == (char_u *)&p_secure
			|| varp == (char_u *)&p_shcf
#ifdef QUICKFIX
			|| varp == (char_u *)&p_sp
#endif
			|| varp == (char_u *)&p_shq
			|| varp == (char_u *)&p_srr
			|| varp == (char_u *)&p_sxq
			|| varp == (char_u *)&p_sh)
		{
		    errmsg = (char_u *)"Not allowed in a modeline";
		    goto skip;
		}
		if (p_secure && (0
#ifdef USE_CSCOPE
			    || varp == (char_u *)&p_csprg
#endif
			    || varp == (char_u *)&p_ep
			    || varp == (char_u *)&p_fp
#ifdef QUICKFIX
			    || varp == (char_u *)&p_mp
			    || varp == (char_u *)&p_gp
#endif
			    || varp == (char_u *)&p_kp
				))
		{
		    smsg((char_u *)"Warning: %s option changed from modeline",
			    options[opt_idx].fullname);
		}
	    }

	    /* remember character after option name */
	    afterchar = arg[len];

	    /* skip white space, allow ":set ai  ?" */
	    while (vim_iswhite(arg[len]))
		++len;

	    adding = FALSE;
	    prepending = FALSE;
	    removing = FALSE;
	    if (arg[len] == '+' && arg[len + 1] == '=')
	    {
		adding = TRUE;		/* "+=" */
		++len;
	    }
	    else if (arg[len] == '^' && arg[len + 1] == '=')
	    {
		prepending = TRUE;	/* "^=" */
		++len;
	    }
	    else if (arg[len] == '-' && arg[len + 1] == '=')
	    {
		removing = TRUE;	/* "-=" */
		++len;
	    }

	    nextchar = arg[len];
	    if (vim_strchr((char_u *)"?=:!&", nextchar) != NULL)
	    {
		arg += len;
		len = 0;
		if (vim_strchr((char_u *)"?!&", nextchar) != NULL
			&& arg[1] != NUL && !vim_iswhite(arg[1]))
		{
		    errmsg = e_trailing;
		    goto skip;
		}
	    }

	    /*
	     * allow '=' and ':' as MSDOS command.com allows only one
	     * '=' character per "set" command line. grrr. (jw)
	     */
	    if (nextchar == '?'
		    || (prefix == 1
			&& vim_strchr((char_u *)"=:&", nextchar) == NULL
			&& !(flags & P_BOOL)))
	    {
		/*
		 * print value
		 */
		if (did_show)
		    msg_putchar('\n');	    /* cursor below last one */
		else
		{
		    gotocmdline(TRUE);	    /* cursor at status line */
		    did_show = TRUE;	    /* remember that we did a line */
		}
		if (opt_idx >= 0)
		    showoneopt(&options[opt_idx]);
		else
		{
		    char_u	    name[2];
		    char_u	    *p;

		    name[0] = KEY2TERMCAP0(key);
		    name[1] = KEY2TERMCAP1(key);
		    p = find_termcode(name);
		    if (p == NULL)
		    {
			errmsg = (char_u *)"Unknown option";
			goto skip;
		    }
		    else
			(void)show_one_termcode(name, p, TRUE);
		}
		if (nextchar != '?'
			&& nextchar != NUL && !vim_iswhite(afterchar))
		    errmsg = e_trailing;
	    }
	    else
	    {
		if (flags & P_BOOL)		    /* boolean */
		{
		    if (nextchar == '=' || nextchar == ':')
		    {
			errmsg = e_invarg;
			goto skip;
		    }

		    /*
		     * ":set opt!": invert
		     * ":set opt&": reset to default value
		     */
		    if (nextchar == '!')
			value = *(int *)(varp) ^ 1;
		    else if (nextchar == '&')
		    {
			/* Use a trick here to get the default value,
			 * without setting the actual value yet. */
			i = *(int *)varp;
			set_option_default(opt_idx, FALSE);
			value = *(int *)varp;
			*(int *)varp = i;
		    }
		    else
		    {
			/*
			 * ":set invopt": invert
			 * ":set opt" or ":set noopt": set or reset
			 */
			if (nextchar != NUL && !vim_iswhite(afterchar))
			{
			    errmsg = e_trailing;
			    goto skip;
			}
			if (prefix == 2)	/* inv */
			    value = *(int *)(varp) ^ 1;
			else
			    value = prefix;
		    }

		    errmsg = set_bool_option(opt_idx, varp, (int)value);
		}
		else				    /* numeric or string */
		{
		    if (vim_strchr((char_u *)"=:&", nextchar) == NULL
							       || prefix != 1)
		    {
			errmsg = e_invarg;
			goto skip;
		    }

		    if (flags & P_NUM)		    /* numeric */
		    {
			/*
			 * Different ways to set a number option:
			 * &	    set to default value
			 * <xx>	    accept special key codes for 'wildchar'
			 * c	    accept any non-digit for 'wildchar'
			 * [-]0-9   set number
			 * other    error
			 */
			arg += len + 1;
			if (nextchar == '&')
			{
			    long	temp;

			    /* Use a trick here to get the default value,
			     * without setting the actual value yet. */
			    temp = *(long *)varp;
			    set_option_default(opt_idx, FALSE);
			    value = *(long *)varp;
			    *(long *)varp = temp;
			}
			else if (((long *)varp == &p_wc
				    || (long *)varp == &p_wcm)
				&& (*arg == '<'
				    || *arg == '^'
				    || ((!arg[1] || vim_iswhite(arg[1]))
					&& !isdigit(*arg))))
			{
			    if (*arg == '<')
				value = find_key_option(arg + 1);
			    else if (*arg == '^')
				value = arg[1] ^ 0x40;
			    else
				value = *arg;
			    if ((value == 0) && ((long *)varp != &p_wcm))
			    {
				errmsg = e_invarg;
				goto skip;
			    }
			}
				/* allow negative numbers (for 'undolevels') */
			else if (*arg == '-' || isdigit(*arg))
			{
			    i = 0;
			    if (*arg == '-')
				i = 1;
#ifdef HAVE_STRTOL
			    value = strtol((char *)arg, NULL, 0);
			    if (arg[i] == '0' && TO_LOWER(arg[i + 1]) == 'x')
				i += 2;
#else
			    value = atol((char *)arg);
#endif
			    while (isdigit(arg[i]))
				++i;
			    if (arg[i] != NUL && !vim_iswhite(arg[i]))
			    {
				errmsg = e_invarg;
				goto skip;
			    }
			}
			else
			{
			    errmsg = (char_u *)"Number required after =";
			    goto skip;
			}

			if (adding)
			    value = *(long *)varp + value;
			if (prepending)
			    value = *(long *)varp * value;
			if (removing)
			    value = *(long *)varp - value;
			errmsg = set_num_option(opt_idx, varp, value, errbuf);
		    }
		    else if (opt_idx >= 0)		    /* string */
		    {
			char_u	    *save_arg = NULL;
			char_u	    *s;
			char_u	    *oldval;	/* previous value if *varp */
			char_u	    *newval;
			unsigned    newlen;
			int	    comma;
			int	    bs;
			int	    new_value_alloced;	/* new string option
							   was allocated */

			/* The old value is kept until we are sure that the new
			 * value is valid.  set_option_default() is therefore
			 * called with FALSE
			 */
			oldval = *(char_u **)(varp);
			if (nextchar == '&')	    /* set to default val */
			{
			    /* The old value will be freed in
			     * did_set_string_option().  Don't change the
			     * flags now, but do remember if the new value is
			     * allocated. */
			    set_option_default(opt_idx, FALSE);
			    new_value_alloced =
					 (options[opt_idx].flags & P_ALLOCED);
			    options[opt_idx].flags = flags;
			}
			else
			{
			    arg += len + 1; /* jump to after the '=' or ':' */

			    /*
			     * Convert 'whichwrap' number to string, for
			     * backwards compatibility with Vim 3.0.
			     * Misuse errbuf[] for the resulting string.
			     */
			    if (varp == (char_u *)&p_ww && isdigit(*arg))
			    {
				*errbuf = NUL;
				i = getdigits(&arg);
				if (i & 1)
				    STRCAT(errbuf, "b,");
				if (i & 2)
				    STRCAT(errbuf, "s,");
				if (i & 4)
				    STRCAT(errbuf, "h,l,");
				if (i & 8)
				    STRCAT(errbuf, "<,>,");
				if (i & 16)
				    STRCAT(errbuf, "[,],");
				if (*errbuf != NUL)	/* remove trailing , */
				    errbuf[STRLEN(errbuf) - 1] = NUL;
				save_arg = arg;
				arg = errbuf;
			    }
			    /*
			     * Remove '>' before 'dir' and 'bdir', for
			     * backwards compatibility with version 3.0
			     */
			    else if (  *arg == '>'
				    && (varp == (char_u *)&p_dir
					    || varp == (char_u *)&p_bdir))
			    {
				++arg;
			    }

			    /*
			     * Copy the new string into allocated memory.
			     * Can't use set_string_option_direct(), because
			     * we need to remove the backslashes.
			     */
			    /* get a bit too much */
			    newlen = STRLEN(arg) + 1;
			    if (adding || prepending || removing)
				newlen += STRLEN(oldval) + 1;
			    newval = alloc(newlen);
			    if (newval == NULL)  /* out of mem, don't change */
				break;
			    s = newval;

			    /*
			     * Copy the string, skip over escaped chars.
			     * For MS-DOS and WIN32 backslashes before normal
			     * file name characters are not removed, and keep
			     * backslash at start, for "\\machine\path", but
			     * do remove it for "\\\\machine\\path".
			     */
			    while (*arg && !vim_iswhite(*arg))
			    {
				if (*arg == '\\' && arg[1] != NUL
#ifdef BACKSLASH_IN_FILENAME
					&& !((flags & P_EXPAND)
						&& vim_isfilec(arg[1])
						&& (arg[1] != '\\'
						    || (s == newval
							&& arg[2] != '\\')))
#endif
								    )
				    ++arg;
				*s++ = *arg++;
			    }
			    *s = NUL;

			    /* concatenate the two strings; add a ',' if
			     * needed */
			    if (adding || prepending)
			    {
				comma = ((flags & P_COMMA) && *oldval);
				if (adding)
				{
				    i = STRLEN(oldval);
				    mch_memmove(newval + i + comma, newval,
							  STRLEN(newval) + 1);
				    mch_memmove(newval, oldval, (size_t)i);
				}
				else
				{
				    i = STRLEN(newval);
				    mch_memmove(newval + i + comma, oldval,
							  STRLEN(oldval) + 1);
				}
				if (comma)
				    newval[i] = ',';
			    }

			    /* locate newval[] in oldval[] and remove it */
			    if (removing)
			    {
				i = STRLEN(newval);
				bs = 0;
				for (s = oldval; *s; ++s)
				{
				    if ((!(flags & P_COMMA)
						|| s == oldval
						|| (s[-1] == ',' && !(bs & 1)))
					    && STRNCMP(s, newval, i) == 0
					    && (!(flags & P_COMMA)
						|| s[i] == ','
						|| s[i] == NUL))
					break;
				    /* Count backspaces.  Only a comma with an
				     * even number of backspaces before it is
				     * recognized as a separator */
				    if (s > oldval && s[-1] == '\\')
					++bs;
				    else
					bs = 0;
				}
				STRCPY(newval, oldval);
				if (*s)
				{
				    /* may need to remove a comma */
				    if (flags & P_COMMA)
				    {
					if (s == oldval)
					{
					    /* include comma after string */
					    if (s[i] == ',')
						++i;
					}
					else
					{
					    /* include comma before string */
					    --s;
					    ++i;
					}
				    }
				    mch_memmove(newval + (s - oldval), s + i,
							   STRLEN(s + i) + 1);
				}
			    }

			    *(char_u **)(varp) = newval;

			    if (save_arg != NULL)   /* number for 'whichwrap' */
				arg = save_arg;
			    new_value_alloced = TRUE;
			}

			/* expand environment variables and ~ */
			s = option_expand(opt_idx);
			if (s != NULL)
			{
			    if (new_value_alloced)
				vim_free(*(char_u **)(varp));
			    *(char_u **)(varp) = s;
			    new_value_alloced = TRUE;
			}

			errmsg = did_set_string_option(opt_idx, (char_u **)varp,
					   new_value_alloced, oldval, errbuf);
			/*
			 * If error detected, print the error message.
			 */
			if (errmsg != NULL)
			    goto skip;
		    }
		    else	    /* key code option */
		    {
			char_u	    name[2];
			char_u	    *p;

			name[0] = KEY2TERMCAP0(key);
			name[1] = KEY2TERMCAP1(key);
			if (nextchar == '&')
			{
			    if (add_termcap_entry(name, TRUE) == FAIL)
				errmsg = (char_u *)"Not found in termcap";
			}
			else
			{
			    arg += len + 1; /* jump to after the '=' or ':' */
			    for(p = arg; *p && !vim_iswhite(*p); ++p)
			    {
				if (*p == '\\' && *(p + 1))
				    ++p;
			    }
			    nextchar = *p;
			    *p = NUL;
			    add_termcode(name, arg, FALSE);
			    *p = nextchar;
			}
			if (full_screen)
			    ttest(FALSE);
			redraw_all_later(CLEAR);
		    }
		}
		if (opt_idx >= 0)
		    options[opt_idx].flags |= P_WAS_SET;
	    }

skip:
	    /*
	     * Advance to next argument.
	     * - skip until a blank found, taking care of backslashes
	     * - skip blanks
	     * - skip one "=val" argument (for hidden options ":set gfn =xx")
	     */
	    for (i = 0; i < 2 ; ++i)
	    {
		while (*arg != NUL && !vim_iswhite(*arg))
		    if (*arg++ == '\\' && *arg != NUL)
			++arg;
		arg = skipwhite(arg);
		if (*arg != '=')
		    break;
	    }
	}
	arg = skipwhite(arg);

	if (errmsg)
	{
	    ++no_wait_return;	/* wait_return done below */
	    emsg(errmsg);	/* show error highlighted */
	    MSG_PUTS(": ");
				/* show argument normal */
	    while (startarg < arg)
		msg_puts(transchar(*startarg++));
	    msg_end();		/* check for scrolling */
	    --no_wait_return;

	    return FAIL;
	}
    }

    return OK;
}

    static char_u *
illegal_char(errbuf, c)
    char_u	*errbuf;
    int		c;
{
    if (errbuf == NULL)
	return (char_u *)"";
    sprintf((char *)errbuf, "Illegal character <%s>", (char *)transchar(c));
    return errbuf;
}

#ifdef WANT_TITLE
/*
 * When changing 'title', 'titlestring', 'icon' or 'iconstring', call
 * maketitle() to create and display it.
 * When switching the title or icon off, call mch_restore_title() to get
 * the old value back.
 */
    static void
did_set_title(icon)
    int	    icon;	    /* Did set icon instead of title */
{
    if (starting != NO_SCREEN
#ifdef USE_GUI
	    && !gui.starting
#endif
				)
    {
	maketitle();
	if (icon)
	{
	    if (!p_icon)
		mch_restore_title(2);
	}
	else
	{
	    if (!p_title)
		mch_restore_title(1);
	}
    }
}
#endif

/*
 * set_options_bin -  called when 'bin' changes value.
 */
    void
set_options_bin(oldval, newval)
    int	    oldval;
    int	    newval;
{
    /*
     * The option values that are changed when 'bin' changes are
     * copied when 'bin is set and restored when 'bin' is reset.
     */
    if (newval)
    {
	if (!oldval)		/* switched on */
	{
	    curbuf->b_p_tw_nobin = curbuf->b_p_tw;
	    curbuf->b_p_wm_nobin = curbuf->b_p_wm;
	    curbuf->b_p_ml_nobin = curbuf->b_p_ml;
	    curbuf->b_p_et_nobin = curbuf->b_p_et;
	}

	curbuf->b_p_tw = 0;	/* no automatic line wrap */
	curbuf->b_p_wm = 0;	/* no automatic line wrap */
	curbuf->b_p_ml = 0;	/* no modelines */
	curbuf->b_p_et = 0;	/* no expandtab */
    }
    else if (oldval)		/* switched off */
    {
	curbuf->b_p_tw = curbuf->b_p_tw_nobin;
	curbuf->b_p_wm = curbuf->b_p_wm_nobin;
	curbuf->b_p_ml = curbuf->b_p_ml_nobin;
	curbuf->b_p_et = curbuf->b_p_et_nobin;
    }
}

#ifdef VIMINFO
/*
 * Find the parameter represented by the given character (eg ', :, ", or /),
 * and return its associated value in the 'viminfo' string.
 * Only works for number parameters, not for 'r' or 'n'.
 * If the parameter is not specified in the string, return -1.
 */
    int
get_viminfo_parameter(type)
    int	    type;
{
    char_u  *p;

    p = find_viminfo_parameter(type);
    if (p != NULL && isdigit(*p))
	return atoi((char *)p);
    return -1;
}

/*
 * Find the parameter represented by the given character (eg ', :, ", or /) in
 * the 'viminfo' option and return a pointer to the string after it.
 * Return NULL if the parameter is not specified in the string.
 */
    char_u *
find_viminfo_parameter(type)
    int	    type;
{
    char_u  *p;

    for (p = p_viminfo; *p; ++p)
    {
	if (*p == type)
	    return p + 1;
	if (*p == 'n')		    /* 'n' is always the last one */
	    break;
	p = vim_strchr(p, ',');	    /* skip until next ',' */
	if (p == NULL)		    /* hit the end without finding parameter */
	    break;
    }
    return NULL;
}
#endif

/*
 * Expand environment variables for some string options.
 * These string options cannot be indirect!
 * Return pointer to allocated memory, or NULL when not expanded.
 */
    static char_u *
option_expand(opt_idx)
    int	    opt_idx;
{
    char_u	*p;

	/* if option doesn't need expansion or is hidden: nothing to do */
    if (!(options[opt_idx].flags & P_EXPAND) || options[opt_idx].var == NULL)
	return NULL;

    p = *(char_u **)(options[opt_idx].var);

    /*
     * Expanding this with NameBuff, expand_env() must not be passed IObuff.
     */
    expand_env(p, NameBuff, MAXPATHL);
    if (STRCMP(NameBuff, p) == 0)   /* they are the same */
	return NULL;

    return vim_strsave(NameBuff);
}

/*
 * Check for string options that are NULL (normally only termcap options).
 */
    void
check_options()
{
    int	    opt_idx;
    char_u  **p;

    for (opt_idx = 0; options[opt_idx].fullname != NULL; opt_idx++)
	if ((options[opt_idx].flags & P_STRING) && options[opt_idx].var != NULL)
	{
	    p = (char_u **)get_varp(&(options[opt_idx]));
	    if (*p == NULL)
		*p = empty_option;
	}
}

/*
 * Check string options in a buffer for NULL value.
 */
    void
check_buf_options(buf)
    BUF	    *buf;
{
#ifdef MULTI_BYTE
    if (buf->b_p_fe == NULL)
	buf->b_p_fe = empty_option;
#endif
    if (buf->b_p_ff == NULL)
	buf->b_p_ff = empty_option;
#ifdef CRYPTV
    if (buf->b_p_key == NULL)
	buf->b_p_key = empty_option;
#endif
    if (buf->b_p_mps == NULL)
	buf->b_p_mps = empty_option;
    if (buf->b_p_fo == NULL)
	buf->b_p_fo = empty_option;
    if (buf->b_p_isk == NULL)
	buf->b_p_isk = empty_option;
#ifdef COMMENTS
    if (buf->b_p_com == NULL)
	buf->b_p_com = empty_option;
#endif
    if (buf->b_p_nf == NULL)
	buf->b_p_nf = empty_option;
#ifdef SYNTAX_HL
    if (buf->b_p_syn == NULL)
	buf->b_p_syn = empty_option;
#endif
#ifdef CINDENT
    if (buf->b_p_cink == NULL)
	buf->b_p_cink = empty_option;
    if (buf->b_p_cino == NULL)
	buf->b_p_cino = empty_option;
#endif
#ifdef AUTOCMD
    if (buf->b_p_ft == NULL)
	buf->b_p_ft = empty_option;
#endif
#ifdef WANT_OSFILETYPE
    if (buf->b_p_oft == NULL)
	buf->b_p_oft = empty_option;
#endif
#if defined(SMARTINDENT) || defined(CINDENT)
    if (buf->b_p_cinw == NULL)
	buf->b_p_cinw = empty_option;
#endif
#ifdef INSERT_EXPAND
    if (buf->b_p_cpt == NULL)
	buf->b_p_cpt = empty_option;
#endif
}

/*
 * Free the string allocated for an option.
 * Checks for the string being empty_option. This may happen if we're out of
 * memory, vim_strsave() returned NULL, which was replaced by empty_option by
 * check_options().
 * Does NOT check for P_ALLOCED flag!
 */
    void
free_string_option(p)
    char_u	*p;
{
    if (p != empty_option)
	vim_free(p);
}

/*
 * Mark a terminal option as allocated, found by a pointer into term_strings[].
 */
    void
set_term_option_alloced(p)
    char_u **p;
{
    int		opt_idx;

    for (opt_idx = 1; options[opt_idx].fullname != NULL; opt_idx++)
	if (options[opt_idx].var == (char_u *)p)
	{
	    options[opt_idx].flags |= P_ALLOCED;
	    return;
	}
    return; /* cannot happen: didn't find it! */
}

/*
 * Set a string option to a new value (without checking the effect).
 * The string is copied into allocated memory.
 * If 'dofree' is set, the old value may be freed.
 * if (opt_idx == -1) name is used, otherwise opt_idx is used.
 */
    void
set_string_option_direct(name, opt_idx, val, dofree)
    char_u  *name;
    int	    opt_idx;
    char_u  *val;
    int	    dofree;
{
    char_u  *s;
    char_u  **varp;

    if (opt_idx == -1)		/* use name */
    {
	opt_idx = findoption(name);
	if (opt_idx == -1)	/* not found (should not happen) */
	    return;
    }

    if (options[opt_idx].var == NULL)	/* can't set hidden option */
	return;

    s = vim_strsave(val);
    if (s != NULL)
    {
	varp = (char_u **)get_varp(&(options[opt_idx]));
	if (dofree && (options[opt_idx].flags & P_ALLOCED))
	    free_string_option(*varp);
	*varp = s;
	options[opt_idx].flags |= P_ALLOCED;
    }
}

/*
 * Set a string option to a new value, and handle the effects.
 */
    static void
set_string_option(opt_idx, value)
    int	    opt_idx;
    char_u  *value;
{
    char_u  *s;
    char_u  **varp;
    char_u  *oldval;

    if (options[opt_idx].var == NULL)	/* don't set hidden option */
	return;

    s = vim_strsave(value);
    if (s != NULL)
    {
	varp = (char_u **)get_varp(&(options[opt_idx]));
	oldval = *varp;
	*varp = s;
	options[opt_idx].flags |= P_WAS_SET;
	(void)did_set_string_option(opt_idx, varp, TRUE, oldval, NULL);
    }
}

/*
 * Handle string options that need some action to perform when changed.
 * Returns NULL for success, or an error message for an error.
 */
    static char_u *
did_set_string_option(opt_idx, varp, new_value_alloced, oldval, errbuf)
    int		opt_idx;		/* index in options[] table */
    char_u	**varp;			/* pointer to the option variable */
    int		new_value_alloced;	/* new value was allocated */
    char_u	*oldval;		/* previous value of the option */
    char_u	*errbuf;		/* buffer for errors, or NULL */
{
    char_u	*errmsg = NULL;
    char_u	*s, *p;
    int		did_chartab = FALSE;

    /* 'term' */
    if (varp == &T_NAME)
    {
	if (T_NAME[0] == NUL)
	    errmsg = (char_u *)"Cannot set 'term' to empty string";
#ifdef USE_GUI
	if (gui.in_use)
	    errmsg = (char_u *)"Cannot change term in GUI";
	else if (term_is_gui(T_NAME))
	    errmsg = (char_u *)"Use \":gui\" to start the GUI";
#endif
	else if (set_termname(T_NAME) == FAIL)
	    errmsg = (char_u *)"Not found in termcap";
	else
	{
	    /* Screen colors may have changed. */
	    out_str(T_ME);
	    redraw_later(CLEAR);
	}
    }

    /* 'backupext' and 'patchmode' */
    else if ((varp == &p_bex || varp == &p_pm))
    {
	if (STRCMP(*p_bex == '.' ? p_bex + 1 : p_bex,
		     *p_pm == '.' ? p_pm + 1 : p_pm) == 0)
	    errmsg = (char_u *)"'backupext' and 'patchmode' are equal";
    }

    /*
     * 'isident', 'iskeyword', 'isprint or 'isfname' option: refill chartab[]
     * If the new option is invalid, use old value.  'lisp' option: refill
     * chartab[] for '-' char
     */
    else if (  varp == &p_isi
	    || varp == &(curbuf->b_p_isk)
	    || varp == &p_isp
	    || varp == &p_isf)
    {
	if (init_chartab() == FAIL)
	{
	    did_chartab = TRUE;	    /* need to restore it below */
	    errmsg = e_invarg;	    /* error in value */
	}
    }

    /* 'helpfile' */
    else if (varp == &p_hf)
    {
	/* May compute new values for $VIM and $VIMRUNTIME */
	if (didset_vim)
	{
	    vim_setenv((char_u *)"VIM", (char_u *)"");
	    didset_vim = FALSE;
	}
	if (didset_vimruntime)
	{
	    vim_setenv((char_u *)"VIMRUNTIME", (char_u *)"");
	    didset_vimruntime = FALSE;
	}
    }

    /* 'highlight' */
    else if (varp == &p_hl)
    {
	if (highlight_changed() == FAIL)
	    errmsg = e_invarg;	/* invalid flags */
    }

    /* 'nrformats' */
    else if (varp == &(curbuf->b_p_nf))
    {
	if (check_opt_strings(curbuf->b_p_nf, p_nf_values, TRUE) != OK)
	    errmsg = e_invarg;
    }

#ifdef MKSESSION
    /* 'sessionoptions' */
    else if (varp == &(p_sessopt))
    {
	if (check_opt_strings(p_sessopt, p_sessopt_values, TRUE) != OK)
	    errmsg = e_invarg;
    }
#endif

    /* 'scrollopt' */
#ifdef SCROLLBIND
    else if (varp == &(p_sbo))
    {
	if (check_opt_strings(p_sbo, p_scbopt_values, TRUE) != OK)
	    errmsg = e_invarg;
    }
#endif

    /* 'background' */
    else if (varp == &p_bg)
    {
	if (check_opt_strings(p_bg, p_bg_values, FALSE) == OK)
	    init_highlight(FALSE);
	else
	    errmsg = e_invarg;
    }

    /* 'wildmode' */
    else if (varp == &p_wim)
    {
	if (check_opt_wim() == FAIL)
	    errmsg = e_invarg;
    }

#ifdef HAS_WAK
    /* 'winaltkeys' */
    else if (varp == &p_wak)
    {
	if (*p_wak == NUL
		|| check_opt_strings(p_wak, p_wak_values, FALSE) != OK)
	    errmsg = e_invarg;
# ifdef WANT_MENU
#  ifdef USE_GUI_MOTIF
	else if (gui.in_use)
	    gui_motif_set_mnemonics(p_wak[0] == 'y' || p_wak[0] == 'm');
#  else
#   ifdef USE_GUI_GTK
	else if (gui.in_use)
	    gui_gtk_set_mnemonics(p_wak[0] == 'y' || p_wak[0] == 'm');
#   endif
#  endif
# endif
    }
#endif

#ifdef AUTOCMD
    /* 'eventignore' */
    else if (varp == &p_ei)
    {
	if (check_ei() == FAIL)
	    errmsg = e_invarg;
    }
#endif

#ifdef MULTI_BYTE
    /* 'fileencoding' */
    else if (varp == &(curbuf->b_p_fe))
    {
	if (check_opt_strings(curbuf->b_p_fe, p_fe_values, FALSE) != OK)
	    errmsg = e_invarg;
	else
	{
	    int is_dbcs_new;

	   /* set flags for various items */
	    is_dbcs_new = 0;
	    is_unicode = 0;
	    if (STRCMP(curbuf->b_p_fe, FE_DBJPN) == 0)
		is_dbcs_new = DBCS_JPN;
	    else if (STRCMP(curbuf->b_p_fe, FE_DBKOR) == 0)
		is_dbcs_new = DBCS_KOR;
	    else if (STRCMP(curbuf->b_p_fe, FE_DBCHT) == 0)
		is_dbcs_new = DBCS_CHT;
	    else if (STRCMP(curbuf->b_p_fe, FE_DBCHS) == 0)
		is_dbcs_new = DBCS_CHS;
	    else if (STRCMP(curbuf->b_p_fe, FE_UNICODE) == 0)
	    {
		is_unicode = 1;
	    }
	    /* if a DBCS locale, make sure it is valid */
# ifdef USE_GUI_WIN32
	    if (is_dbcs_new)
	    {
		if (!IsValidCodePage(is_dbcs_new))
		{
		    errmsg = (char_u *)"Not a valid codepage";
		}
		else
		{
		    /* ok code page, so let's set is_dbcs and set pgm
		     * language
		     * */
		    is_dbcs = is_dbcs_new;
		}
	    }
	    else
# endif
		is_dbcs = is_dbcs_new;
# ifdef USE_GUI_WIN32
	    is_funky_dbcs = is_dbcs && ((int)GetACP() != is_dbcs);
# endif
# ifdef AUTOCMD
	    /* fire off an autocommand to let people do custom font setup,
	     * whatever */
	    if (errmsg == NULL)
		apply_autocmds(EVENT_FILEENCODING, NULL, (char_u *)"",
							       FALSE, curbuf);
# endif
	}
    }
#endif /* MULTI_BYTE */

    /* 'fileformat' */
    else if (varp == &(curbuf->b_p_ff))
    {
	if (check_opt_strings(curbuf->b_p_ff, p_ff_values, FALSE) != OK)
	    errmsg = e_invarg;
	else
	{
	    /* also change 'textmode' */
	    if (get_fileformat(curbuf) == EOL_DOS)
		curbuf->b_p_tx = TRUE;
	    else
		curbuf->b_p_tx = FALSE;
	}
    }

    /* 'fileformats' */
    else if (varp == &p_ffs)
    {
	if (check_opt_strings(p_ffs, p_ff_values, TRUE) != OK)
	    errmsg = e_invarg;
	else
	{
	    /* also change 'textauto' */
	    if (*p_ffs == NUL)
		p_ta = FALSE;
	    else
		p_ta = TRUE;
	}
    }

#ifdef CRYPTV
    /* 'cryptkey' */
    else if (varp == &(curbuf->b_p_key))
    {
	/* Make sure the ":set" command doesn't show the new value in the
	 * history. */
	remove_key_from_history();
    }
#endif

    /* 'matchpairs' */
    else if (varp == &(curbuf->b_p_mps))
    {
	/* Check for "x:y,x:y" */
	for (p = curbuf->b_p_mps; *p; p += 4)
	{
	    if (!p[0] || p[1] != ':' || !p[2] || (p[3] && p[3] != ','))
	    {
		errmsg = e_invarg;
		break;
	    }
	    if (!p[3])
		break;
	}
    }

#ifdef COMMENTS
    /* 'comments' */
    else if (varp == &(curbuf->b_p_com))
    {
	for (s = curbuf->b_p_com; *s; )
	{
	    while (*s && *s != ':')
	    {
		if (vim_strchr((char_u *)COM_ALL, *s) == NULL
						 && !isdigit(*s) && *s != '-')
		{
		    errmsg = illegal_char(errbuf, *s);
		    break;
		}
		++s;
	    }
	    if (*s++ == NUL)
		errmsg = (char_u *)"Missing colon";
	    else if (*s == ',' || *s == NUL)
		errmsg = (char_u *)"Zero length string";
	    if (errmsg != NULL)
		break;
	    while (*s && *s != ',')
	    {
		if (*s == '\\' && s[1] != NUL)
		    ++s;
		++s;
	    }
	    s = skip_to_option_part(s);
	}
    }
#endif

    /* 'listchars' */
    else if (varp == &p_lcs)
    {
	int	round, i, len;

	/* first round: check for valid value, second round: assign values */
	for (round = 0; round <= 1 && errmsg == NULL; ++round)
	{
	    static struct lcstab
	    {
		int	*lcsp;
		char	*name;
	    } lcstab[] =
	    {
		{&lcs_eol,	"eol"},
		{&lcs_ext,	"extends"},
		{&lcs_tab2,	"tab"},
		{&lcs_trail,	"trail"},
	    };

	    if (round)
	    {
		for (i = 0; i < sizeof(lcstab) / sizeof(struct lcstab); ++i)
		    *(lcstab[i].lcsp) = NUL;
		lcs_tab1 = NUL;
	    }
	    p = p_lcs;
	    while (*p)
	    {
		for (i = 0; i < sizeof(lcstab) / sizeof(struct lcstab); ++i)
		{
		    len = STRLEN(lcstab[i].name);
		    if (STRNCMP(p, lcstab[i].name, len) == 0
			    && p[len] == ':'
			    && p[len + 1] != NUL)
		    {
			if (lcstab[i].lcsp == &lcs_tab2)
			    ++len;
			if (p[len + 1] != NUL
			    && (p[len + 2] == ',' || p[len + 2] == NUL))
			{
			    if (round)
			    {
				*(lcstab[i].lcsp) = p[len + 1];
				if (lcstab[i].lcsp == &lcs_tab2)
				    lcs_tab1 = p[len];
			    }
			    p += len + 2;
			    break;
			}
		    }
		}

		if (i == sizeof(lcstab) / sizeof(struct lcstab))
		{
		    errmsg = e_invarg;
		    break;
		}
		if (*p == ',')
		    ++p;
	    }
	}
    }

#ifdef VIMINFO
    /* 'viminfo' */
    else if (varp == &(p_viminfo))
    {
	for (s = p_viminfo; *s;)
	{
	    /* Check it's a valid character */
	    if (vim_strchr((char_u *)"\"'%!fhrn:/", *s) == NULL)
	    {
		errmsg = illegal_char(errbuf, *s);
		break;
	    }
	    if (*s == 'n')	/* name is always last one */
	    {
		break;
	    }
	    else if (*s == 'r') /* skip until next ',' */
	    {
		while (*++s && *s != ',')
		    ;
	    }
	    else if (*s == '%' || *s == '!' || *s == 'h') /* no extra chars */
		++s;
	    else		/* must have a number */
	    {
		while (isdigit(*++s))
		    ;

		if (!isdigit(*(s - 1)))
		{
		    if (errbuf != NULL)
		    {
			sprintf((char *)errbuf, "Missing number after <%s>",
							 transchar(*(s - 1)));
			errmsg = errbuf;
		    }
		    else
			errmsg = (char_u *)"";
		    break;
		}
	    }
	    if (*s == ',')
		++s;
	    else if (*s)
	    {
		if (errbuf != NULL)
		    errmsg = (char_u *)"Missing comma";
		else
		    errmsg = (char_u *)"";
		break;
	    }
	}
	if (*p_viminfo && errmsg == NULL && get_viminfo_parameter('\'') < 0)
	    errmsg = (char_u *)"Must specify a ' value";
    }
#endif /* VIMINFO */

    /* terminal options */
    else if (istermoption(&options[opt_idx]) && full_screen)
    {
	/* ":set t_Co=0" does ":set t_Co=" */
	if (varp == &T_CCO && atoi((char *)T_CCO) == 0)
	{
	    if (new_value_alloced)
		vim_free(T_CCO);
	    T_CCO = empty_option;
	}
	ttest(FALSE);
	if (varp == &T_ME)
	{
	    out_str(T_ME);
	    redraw_later(CLEAR);
#if defined(MSDOS) || (defined(WIN32) && !defined(USE_GUI_WIN32))
	    /* Since t_me has been set, this probably means that the user
	     * wants to use this as default colors.  Need to reset default
	     * background/foreground colors. */
	    mch_set_normal_colors();
#endif
	}
    }

#ifdef LINEBREAK
    /* 'showbreak' */
    else if (varp == &p_sbr)
    {
	for (s = p_sbr; *s; ++s)
	    if (charsize(*s) != 1)
		errmsg = (char_u *)"contains unprintable character";
    }
#endif

#ifdef USE_GUI
    /* 'guifont' */
    else if (varp == &p_guifont)
    {
	if (gui.in_use && gui_init_font(p_guifont) != OK
# if defined(USE_GUI_MSWIN) || defined(USE_GUI_GTK)
		&& *p_guifont != '*'
# endif
		)
	    errmsg = (char_u *)"Cannot set font(s)";
    }
# ifdef USE_FONTSET
    else if (varp == &p_guifontset)
    {
	if (gui.in_use && gui.fontset && gui_init_font(p_guifontset) != OK)
	    errmsg = (char_u *)"Cannot set fontset";
    }
# endif
#endif

#ifdef CURSOR_SHAPE
    /* 'guicursor' */
    else if (varp == &p_guicursor)
	errmsg = parse_guicursor();
#endif

#ifdef HAVE_LANGMAP
    /* 'langmap' */
    else if (varp == &p_langmap)
	langmap_set();
#endif

#ifdef LINEBREAK
    /* 'breakat' */
    else if (varp == &p_breakat)
	fill_breakat_flags();
#endif

#ifdef WANT_TITLE
    /* 'titlestring' and 'iconstring' */
    else if (varp == &p_titlestring
	         || varp == &p_iconstring)
    {
# ifdef STATUSLINE
	int	flagval = (varp == &p_titlestring) ? STL_IN_TITLE : STL_IN_ICON;

	/* NULL => statusline syntax */
	if (vim_strchr(*varp, '%') && check_stl_option(*varp) == NULL)
	    stl_syntax |= flagval;
	else
	    stl_syntax &= ~flagval;
# endif
	did_set_title(varp == &p_iconstring);

    }
#endif

#ifdef USE_GUI
    /* 'guioptions' */
    else if (varp == &p_go)
	gui_init_which_components(oldval);
#endif

#if defined(USE_MOUSE) && (defined(UNIX) || defined(VMS))
    /* 'ttymouse' */
    else if (varp == &p_ttym)
    {
	if (check_opt_strings(p_ttym, p_ttym_values, FALSE) != OK)
	    errmsg = e_invarg;
	else
	    check_mouse_termcode();
    }
#endif

    /* 'selection' */
    else if (varp == &p_sel)
    {
	if (*p_sel == NUL
		|| check_opt_strings(p_sel, p_sel_values, FALSE) != OK)
	    errmsg = e_invarg;
    }

    /* 'selectmode' */
    else if (varp == &p_slm)
    {
	if (check_opt_strings(p_slm, p_slm_values, TRUE) != OK)
	    errmsg = e_invarg;
    }

    /* 'browsedir' */
    else if (varp == &p_bsdir)
    {
	if (check_opt_strings(p_bsdir, p_bsdir_values, FALSE) != OK)
	    errmsg = e_invarg;
    }

    /* 'keymodel' */
    else if (varp == &p_km)
    {
	if (check_opt_strings(p_km, p_km_values, TRUE) != OK)
	    errmsg = e_invarg;
    }

    /* 'mousemodel' */
    else if (varp == &p_mousem)
    {
	if (check_opt_strings(p_mousem, p_mousem_values, FALSE) != OK)
	    errmsg = e_invarg;
    }

    /* 'switchbuf' */
    else if (varp == &(p_swb))
    {
	if (check_opt_strings(p_swb, p_swb_values, TRUE) != OK)
	    errmsg = e_invarg;
    }

    /* 'lastline' */
    else if (varp == &(p_dy))
    {
	if (check_opt_strings(p_dy, p_dy_values, TRUE) != OK)
	    errmsg = e_invarg;
    }

#ifdef USE_CLIPBOARD
    /* 'clipboard' */
    else if (varp == &(p_cb))
    {
	if (check_opt_strings(p_cb, p_cb_values, TRUE) != OK)
	    errmsg = e_invarg;
    }
#endif


#ifdef AUTOCMD
# ifdef SYNTAX_HL
    /* When 'syntax' is set, load the syntax of that name */
    else if (varp == &(curbuf->b_p_syn))
    {
	apply_autocmds(EVENT_SYNTAX, curbuf->b_p_syn,
					     curbuf->b_fname, TRUE, curbuf);
    }
# endif

    /* When 'filetype' is set, trigger the FileType autocommands of that name */
    else if (varp == &(curbuf->b_p_ft))
    {
	apply_autocmds(EVENT_FILETYPE, curbuf->b_p_ft,
					     curbuf->b_fname, TRUE, curbuf);
    }
#endif

#ifdef STATUSLINE
    /* 'statusline' or 'rulerformat' */
    else if (varp == &p_stl || varp == &p_ruf)
    {
	int wid;

	if (varp == &p_ruf)	/* reset ru_wid first */
	    ru_wid = 0;
	s = *varp;
	if (varp == &p_ruf && *s == '%')
	{
	    /* set ru_wid if 'ruf' starts with "%99(" */
	    if (*++s == '-')	/* ignore a '-' */
		s++;
	    wid = getdigits(&s);
	    if (wid && *s == '(' && (errmsg = check_stl_option(p_ruf)) == NULL)
		ru_wid = wid;
	    else
		errmsg = check_stl_option(p_ruf);
	}
	else
	    errmsg = check_stl_option(s);
	if (varp == &(p_ruf) && errmsg == NULL)
	    comp_col();
    }
#endif

#ifdef INSERT_EXPAND
    /* check if it is a valid value for 'complete' -- Acevedo */
    else if (varp == &(curbuf->b_p_cpt))
    {
	for (s = curbuf->b_p_cpt; *s;)
	{
	    while(*s == ',' || *s == ' ')
		s++;
	    if (!*s)
		break;
	    if (vim_strchr((char_u *)".wbukid]t", *s) == NULL)
	    {
		errmsg = illegal_char(errbuf, *s);
		break;
	    }
	    if (*++s != NUL && *s != ',' && *s != ' ')
	    {
		if (*(s-1) == 'k')
		{
		    /* skip optional filename after 'k' */
		    while (*s && *s != ',' && *s != ' ')
		    {
			if (*s == '\\')
			    ++s;
			++s;
		    }
		}
		else
		{
		    if (errbuf != NULL)
		    {
			sprintf((char *)errbuf,
					"Illegal character after <%c>", *--s);
			errmsg = errbuf;
		    }
		    else
			errmsg = (char_u *)"";
		    break;
		}
	    }
	}
    }
#endif /* INSERT_EXPAND */


#if defined(USE_GUI_GTK) && defined(USE_TOOLBAR)
    else if (varp == &p_toolbar)
    {
	if (p_toolbar && (strstr((const char *)p_toolbar, "text")
				 || strstr((const char *)p_toolbar, "icons")))
	    gui_mch_show_toolbar(TRUE);
	else
	    gui_mch_show_toolbar(FALSE);
    }
#endif

    /* 'pastetoggle': translate key codes like in a mapping */
    else if (varp == &p_pt)
    {
	if (*p_pt)
	{
	    (void)replace_termcodes(p_pt, &p, TRUE, TRUE);
	    if (p != NULL)
	    {
		if (new_value_alloced)
		    vim_free(p_pt);
		p_pt = p;
		new_value_alloced = TRUE;
	    }
	}
    }

    /* 'backspace' */
    else if (varp == &p_bs)
    {
	if (isdigit(*p_bs))
	{
	    if (*p_bs >'2' || p_bs[1] != NUL)
		errmsg = e_invarg;
	}
	else if (check_opt_strings(p_bs, p_bs_values, TRUE) != OK)
	    errmsg = e_invarg;
    }

    /* Options that are a list of flags. */
    else
    {
	p = NULL;
	if (varp == &p_ww)
	    p = (char_u *)WW_ALL;
	if (varp == &p_shm)
	    p = (char_u *)SHM_ALL;
	else if (varp == &(p_cpo))
	    p = (char_u *)CPO_ALL;
	else if (varp == &(curbuf->b_p_fo))
	    p = (char_u *)FO_ALL;
	else if (varp == &p_mouse)
	{
#ifdef USE_MOUSE
	    p = (char_u *)MOUSE_ALL;
#else
	    if (*p_mouse != NUL)
		errmsg = (char_u *)"No mouse support";
#endif
	}
#if defined(USE_GUI)
	else if (varp == &p_go)
	    p = (char_u *)GO_ALL;
#endif
	if (p != NULL)
	{
	    for (s = *varp; *s; ++s)
		if (vim_strchr(p, *s) == NULL)
		{
		    errmsg = illegal_char(errbuf, *s);
		    break;
		}
	}
    }

    /*
     * If error detected, restore the previous value.
     */
    if (errmsg != NULL)
    {
	if (new_value_alloced)
	    vim_free(*varp);
	*varp = oldval;
	/*
	 * When resetting some values, need to act on it.
	 */
	if (did_chartab)
	    (void)init_chartab();
	if (varp == &p_hl)
	    (void)highlight_changed();
    }
    else
    {
	/*
	 * Free string options that are in allocated memory.
	 */
	if (options[opt_idx].flags & P_ALLOCED)
	    free_string_option(oldval);
	if (new_value_alloced)
	    options[opt_idx].flags |= P_ALLOCED;
	else
	    options[opt_idx].flags &= ~P_ALLOCED;
    }

#ifdef USE_MOUSE
    if (varp == &p_mouse)
    {
	if (*p_mouse == NUL)
	    mch_setmouse(FALSE);    /* switch mouse off */
	else
	    setmouse();		    /* in case 'mouse' changed */
    }
#endif

    curwin->w_set_curswant = TRUE;  /* in case 'showbreak' changed */
    check_redraw(options[opt_idx].flags);

    return errmsg;
}

#ifdef STATUSLINE
/*
 * Check validity of options with the 'statusline' format.
 * Return error message or NULL.
 */
    char_u *
check_stl_option(s)
    char_u	*s;
{
    int		itemcnt = 0;
    int         groupdepth = 0;
    static char_u   errbuf[80];

    while (*s && itemcnt < STL_MAX_ITEM)
    {
	/* Check for valid keys after % sequences */
	while (*s && *s != '%')
	    s++;
	if (!*s)
	    break;
	s++;
	if (*s == '%' || *s == STL_TRUNCMARK || *s == STL_MIDDLEMARK)
	{
	    s++;
	    continue;
	}
	if (*s == ')')
	{
	    s++;
	    groupdepth--;
	    continue;
	}
	if (*s == '-')
	    s++;
	while (isdigit(*s))
	    s++;
	if (*s == STL_HIGHLIGHT)
	    continue;
	if (*s == '.')
	{
	    s++;
	    while (*s && isdigit(*s))
		s++;
	}
	if (*s == '(')
	{
	    groupdepth++;
	    continue;
	}
	if (vim_strchr(STL_ALL, *s) == NULL)
	{
	    return illegal_char(errbuf, *s);
	}
	if (*s == '{')
	{
	    s++;
	    while (*s != '}' && *s)
		s++;
	    if (*s != '}')
		return (char_u *) "Unclosed expression sequence";
	}
    }
    if (itemcnt >= STL_MAX_ITEM)
	return (char_u *) "too many items";
    if (groupdepth != 0)
	return (char_u *) "unbalanced groups";
    return NULL;
}
#endif

/*
 * Set the value of a boolean option, and take care of side effects.
 * Returns NULL for success, or an error message for an error.
 */
    static char_u *
set_bool_option(opt_idx, varp, value)
    int	    opt_idx;		    /* index in options[] table */
    char_u  *varp;		    /* pointer to the option variable */
    int	    value;		    /* new value */
{
    int	    old_p_bin = curbuf->b_p_bin;    /* remember old 'bin' */
    int	    old_p_ea = p_ea;		    /* remember old 'equalalways' */
    int	    old_p_wiv = p_wiv;		    /* remember old 'weirdinvert' */
#ifdef FKMAP
    int	    old_akm = p_altkeymap;	    /* previous value if p_altkeymap*/
#endif

    /*
     * in secure mode, setting of the secure option is not
     * allowed
     */
    if (secure && (int *)varp == &p_secure)
	return (char_u *)"not allowed here";

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

    *(int *)varp = value;	    /* set the new value */

    /* handle the setting of the compatible option */
    if ((int *)varp == &p_cp)
    {
	compatible_set();
    }

    /* when 'readonly' is reset, also reset readonlymode */
    else if ((int *)varp == &curbuf->b_p_ro && !curbuf->b_p_ro)
	readonlymode = FALSE;

    /* when 'bin' is set also set some other options */
    else if ((int *)varp == &curbuf->b_p_bin)
    {
	set_options_bin(old_p_bin, curbuf->b_p_bin);
    }

    /* when 'swf' is set create swapfile, when reset remove swapfile */
    else if ((int *)varp == &curbuf->b_p_swf)
    {
	if (curbuf->b_p_swf && p_uc)
	    ml_open_file(curbuf);		/* create the swap file */
	else
	    mf_close_file(curbuf, TRUE);	/* remove the swap file */
    }

    /* when 'terse' is set change 'shortmess' */
    else if ((int *)varp == &p_terse)
    {
	char_u	*p;

	p = vim_strchr(p_shm, SHM_SEARCH);

	/* insert 's' in p_shm */
	if (p_terse && p == NULL)
	{
	    STRCPY(IObuff, p_shm);
	    STRCAT(IObuff, "s");
	    set_string_option_direct((char_u *)"shm", -1, IObuff, TRUE);
	}
	/* remove 's' from p_shm */
	else if (!p_terse && p != NULL)
	    mch_memmove(p, p + 1, STRLEN(p));
    }
    /* when 'paste' is set or reset also change other options */
    else if ((int *)varp == &p_paste)
    {
	paste_option_changed();
    }
    /* when 'ignorecase' is set or reset and 'hlsearch' is set, redraw */
    else if ((int *)varp == &p_ic && p_hls)
    {
	redraw_all_later(NOT_VALID);
    }
#ifdef EXTRA_SEARCH
    /* when 'hlsearch' is set or reset reset no_hlsearch */
    else if ((int *)varp == &p_hls)
    {
	no_hlsearch = FALSE;
    }
#endif
    /* when 'textmode' is set or reset also change 'fileformat' */
    else if ((int *)varp == &curbuf->b_p_tx)
    {
	set_fileformat(curbuf->b_p_tx ? EOL_DOS : EOL_UNIX);
    }
    /* when 'textauto' is set or reset also change 'fileformats' */
    else if ((int *)varp == &p_ta)
    {
	set_string_option_direct((char_u *)"ffs", -1,
			      p_ta ? (char_u *)FFS_DFLT : (char_u *)"", TRUE);
    }
    /*
     * When 'lisp' option changes include/exclude '-' in
     * keyword characters.
     */
#ifdef LISPINDENT
    else if (varp == (char_u *)&(curbuf->b_p_lisp))
	init_chartab();	    /* ignore errors */
#endif
#ifdef WANT_TITLE
    /* when 'title' changed, may need to change the title; same for 'icon' */
    else if ((int *)varp == &p_title)
	did_set_title(FALSE);
    else if ((int *)varp == &p_icon)
	did_set_title(TRUE);
#endif
    else if ((int *)varp == &curbuf->b_changed)
    {
	if (!value)
	    curbuf->b_start_ffc = *curbuf->b_p_ff;    /* Buffer is unchanged */
#ifdef AUTOCMD
	modified_was_set = value;
#endif
    }

#ifdef BACKSLASH_IN_FILENAME
    else if ((int *)varp == &p_ssl)
    {
	if (p_ssl)
	{
	    psepc = '/';
	    psepcN = '\\';
	    pseps[0] = '/';
	    psepsN[0] = '\\';
	}
	else
	{
	    psepc = '\\';
	    psepcN = '/';
	    pseps[0] = '\\';
	    psepsN[0] = '/';
	}
    }
#endif

    if (p_ea && !old_p_ea)
	win_equal(curwin, FALSE);

    /*
     * When 'weirdinvert' changed, set/reset 't_xs'.
     * Then set 'weirdinvert' according to value of 't_xs'.
     */
    if (p_wiv && !old_p_wiv)
	T_XS = (char_u *)"y";
    else if (!p_wiv && old_p_wiv)
	T_XS = empty_option;
    p_wiv = (*T_XS != NUL);

#ifdef FKMAP
    /*
     * In case some second language keymapping options have changed, check
     * and correct the setting in a consistent way.
     */
    if (old_akm != p_altkeymap)
    {
	if (!p_altkeymap)
	{
	    p_hkmap = p_fkmap;
	    p_fkmap = 0;
	    init_chartab();
	}
	else
	{
	    p_fkmap = p_hkmap;
	    p_hkmap = 0;
	    init_chartab();
	}
    }

    /*
     * If hkmap set, reset Farsi keymapping.
     */
    if (p_hkmap && p_altkeymap)
    {
	p_altkeymap = 0;
	p_fkmap = 0;
	init_chartab();
    }

    /*
     * If fkmap set, reset Hebrew keymapping.
     */
    if (p_fkmap && !p_altkeymap)
    {
	p_altkeymap = 1;
	p_hkmap = 0;
	init_chartab();
    }
#endif

    options[opt_idx].flags |= P_WAS_SET;

    comp_col();			    /* in case 'ruler' or 'showcmd' changed */
    curwin->w_set_curswant = TRUE;  /* in case 'list' changed */
    check_redraw(options[opt_idx].flags);

    return NULL;
}

/*
 * Set the value of a number option, and take care of side effects.
 * Returns NULL for success, or an error message for an error.
 */
    static char_u *
set_num_option(opt_idx, varp, value, errbuf)
    int	    opt_idx;		    /* index in options[] table */
    char_u  *varp;		    /* pointer to the option variable */
    long    value;		    /* new value */
    char_u  *errbuf;		    /* buffer for error messages */
{
    char_u  *errmsg = NULL;
    long    old_Rows = Rows;		/* remember old Rows */
    long    old_Columns = Columns;	/* remember old Columns */
    long    old_p_ch = p_ch;		/* remember old command line height */
    long    old_p_uc = p_uc;		/* remember old 'updatecount' */
#ifdef WANT_TITLE
    long    old_titlelen = p_titlelen;	/* remember old 'titlelen' */
#endif

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

    *(long *)varp = value;

    /*
     * Number options that need some action when changed
     */
    if ((long *)varp == &p_wh || (long *)varp == &p_hh)
    {
	if (p_wh < 1)
	{
	    errmsg = e_positive;
	    p_wh = 1;
	}
	if (p_wmh > p_wh)
	{
	    errmsg = (char_u *)"'winheight' cannot be smaller than 'winminheight'";
	    p_wh = p_wmh;
	}
	if (p_hh < 0)
	{
	    errmsg = e_positive;
	    p_hh = 0;
	}

	/* Change window height NOW */
	if (lastwin != firstwin)
	{
	    if ((long *)varp == &p_wh && curwin->w_height < p_wh)
		win_setheight((int)p_wh);
	    if ((long *)varp == &p_hh && curbuf->b_help
						   && curwin->w_height < p_hh)
		win_setheight((int)p_hh);
	}
    }

    /* 'winminheight' */
    else if ((long *)varp == &p_wmh)
    {
	if (p_wmh < 0)
	{
	    errmsg = e_positive;
	    p_wh = 0;
	}
	if (p_wmh > p_wh)
	{
	    errmsg = (char_u *)"'winheight' cannot be smaller than 'winminheight'";
	    p_wmh = p_wh;
	}
	win_setminheight();
    }

    /* (re)set last window status line */
    else if ((long *)varp == &p_ls)
    {
	last_status();
    }

    /*
     * Check the bounds for numeric options here
     */
    if (Rows < min_rows() && full_screen)
    {
	if (errbuf != NULL)
	{
	    sprintf((char *)errbuf, "Need at least %d lines", min_rows());
	    errmsg = errbuf;
	}
	Rows = min_rows();
    }
    if (Columns < MIN_COLUMNS && full_screen)
    {
	if (errbuf != NULL)
	{
	    sprintf((char *)errbuf, "Need at least %d columns", MIN_COLUMNS);
	    errmsg = errbuf;
	}
	Columns = MIN_COLUMNS;
    }

#ifdef DJGPP
    /* avoid a crash by checking for a too large value of 'columns' */
    if (old_Columns != Columns && full_screen && term_console)
	mch_check_columns();
#endif

    /*
     * If the screenheight has been changed, assume it is the physical
     * screenheight.
     */
    if ((old_Rows != Rows || old_Columns != Columns) && full_screen)
    {
	ui_set_winsize();	    /* try to change the window size */
	check_winsize();	    /* in case 'columns' changed */
#ifdef MSDOS
	set_window();	    /* active window may have changed */
#endif
    }

    if (curbuf->b_p_sts < 0)
    {
	errmsg = e_positive;
	curbuf->b_p_sts = 0;
    }
    if (curbuf->b_p_ts <= 0)
    {
	errmsg = e_positive;
	curbuf->b_p_ts = 8;
    }
    if (curbuf->b_p_sw <= 0)
    {
	errmsg = e_positive;
	curbuf->b_p_sw = curbuf->b_p_ts;
    }
    if (curbuf->b_p_tw < 0)
    {
	errmsg = e_positive;
	curbuf->b_p_tw = 0;
    }
    if (p_tm < 0)
    {
	errmsg = e_positive;
	p_tm = 0;
    }
#ifdef WANT_TITLE
    if (p_titlelen < 0)
    {
	errmsg = e_positive;
	p_titlelen = 85;
    }
#endif
    if ((curwin->w_p_scroll <= 0
		|| (curwin->w_p_scroll > curwin->w_height
		    && curwin->w_height > 0))
	    && full_screen)
    {
	if ((long *)varp == &(curwin->w_p_scroll))
	{
	    if (curwin->w_p_scroll != 0)
		errmsg = e_scroll;
	    win_comp_scroll(curwin);
	}
	/* If 'scroll' became invalid because of a side effect silently adjust
	 * it. */
	else if (curwin->w_p_scroll <= 0)
	    curwin->w_p_scroll = 1;
	else /* curwin->w_p_scroll > curwin->w_height */
	    curwin->w_p_scroll = curwin->w_height;
    }
    if (p_report < 0)
    {
	errmsg = e_positive;
	p_report = 1;
    }
    if ((p_sj < 0 || p_sj >= Rows) && full_screen)
    {
	if (Rows != old_Rows)	    /* Rows changed, just adjust p_sj */
	    p_sj = Rows / 2;
	else
	{
	    errmsg = e_scroll;
	    p_sj = 1;
	}
    }
    if (p_so < 0 && full_screen)
    {
	errmsg = e_scroll;
	p_so = 0;
    }
    if (p_uc < 0)
    {
	errmsg = e_positive;
	p_uc = 100;
    }
    if (p_ch < 1)
    {
	errmsg = e_positive;
	p_ch = 1;
    }
    if (p_ut < 0)
    {
	errmsg = e_positive;
	p_ut = 2000;
    }
    if (p_ss < 0)
    {
	errmsg = e_positive;
	p_ss = 0;
    }

    /* when 'updatecount' changes from zero to non-zero, open swap files */
    if (p_uc && !old_p_uc)
	ml_open_files();

    /* if p_ch changed value, change the command line height */
    if (p_ch != old_p_ch)
	command_height(old_p_ch);

#ifdef WANT_TITLE
    /* if 'titlelen' has changed, redraw the title */
    if (old_titlelen != p_titlelen && starting != NO_SCREEN)
	maketitle();
#endif

    options[opt_idx].flags |= P_WAS_SET;

    comp_col();			    /* in case 'columns' or 'ls' changed */
    curwin->w_set_curswant = TRUE;  /* in case 'tabstop' changed */
    check_redraw(options[opt_idx].flags);

    return errmsg;
}

/*
 * Called after an option changed: check if something needs to be redrawn.
 */
    static void
check_redraw(flags)
    int	    flags;
{
    if (flags & (P_RSTAT | P_RALL))	/* mark all status lines dirty */
	status_redraw_all();

    if (flags & (P_RBUF | P_RALL))
    {
	/* Update cursor position and botline (wrapping my have changed). */
	changed_line_abv_curs();
	invalidate_botline();
	update_topline();
    }

    if (flags & P_RBUF)
	redraw_curbuf_later(NOT_VALID);
    if (flags & P_RALL)
	redraw_all_later(NOT_VALID);
    if ((flags & P_RCLR) == P_RCLR)
	redraw_all_later(CLEAR);
}

/*
 * Find index for option 'arg'.
 * Return -1 if not found.
 */
    static int
findoption(arg)
    char_u *arg;
{
    int		    opt_idx;
    char	    *s, *p;
    static short    quick_tab[27] = {0, 0};	/* quick access table */
    int		    is_term_opt;

    /*
     * For first call: Initialize the quick-access table.
     * It contains the index for the first option that starts with a certain
     * letter.  There are 26 letters, plus the first "t_" option.
     */
    if (quick_tab[1] == 0)
    {
	p = options[0].fullname;
	for (opt_idx = 1; (s = options[opt_idx].fullname) != NULL; opt_idx++)
	{
	    if (s[0] != p[0])
	    {
		if (s[0] == 't' && s[1] == '_')
		    quick_tab[26] = opt_idx;
		else
		    quick_tab[s[0] - 'a'] = opt_idx;
	    }
	    p = s;
	}
    }

    /*
     * Check for name starting with an illegal character.
     */
    if (arg[0] < 'a' || arg[0] > 'z')
	return -1;

    is_term_opt = (arg[0] == 't' && arg[1] == '_');
    if (is_term_opt)
	opt_idx = quick_tab[26];
    else
	opt_idx = quick_tab[arg[0] - 'a'];
    for ( ; (s = options[opt_idx].fullname) != NULL; opt_idx++)
    {
	if (STRCMP(arg, s) == 0)		    /* match full name */
	    break;
    }
    if (s == NULL && !is_term_opt)
    {
	opt_idx = quick_tab[arg[0] - 'a'];
	for ( ; options[opt_idx].fullname != NULL; opt_idx++)
	{
	    s = options[opt_idx].shortname;
	    if (s != NULL && STRCMP(arg, s) == 0)   /* match short name */
		break;
	    s = NULL;
	}
    }
    if (s == NULL)
	opt_idx = -1;
    return opt_idx;
}

#if defined(WANT_EVAL) || defined(HAVE_TCL)
/*
 * Get the value for an option.
 *
 * Returns:
 * Number or Toggle option: 1, *numval gets value.
 *	     String option: 0, *stringval gets allocated string.
 *	     hidden option: -1.
 *	    unknown option: -2.
 */
    int
get_option_value(name, numval, stringval)
    char_u	*name;
    long	*numval;
    char_u	**stringval;	    /* NULL when only checking existance */
{
    int	    opt_idx;
    char_u  *varp;

    opt_idx = findoption(name);
    if (opt_idx < 0)		    /* unknown option */
	return -2;

    varp = get_varp(&(options[opt_idx]));
    if (varp == NULL)		    /* hidden option */
	return -1;

    if (options[opt_idx].flags & P_STRING)
    {
	if (stringval != NULL)
	{
#ifdef CRYPTV
	    /* never return the value of the crypt key */
	    if ((char_u **)varp == &curbuf->b_p_key)
		*stringval = vim_strsave((char_u *)"*****");
	    else
#endif
		*stringval = vim_strsave(*(char_u **)(varp));
	}
	return 0;
    }
    if (options[opt_idx].flags & P_NUM)
	*numval = *(long *)varp;
    else
	*numval = *(int *)varp;
    return 1;
}
#endif

/*
 * Set the value of option "name".
 * Use "string" for string options, use "number" for other options.
 */
    void
set_option_value(name, number, string)
    char_u	*name;
    long	number;
    char_u	*string;
{
    int	    opt_idx;
    char_u  *varp;

    opt_idx = findoption(name);
    if (opt_idx == -1)
	EMSG2("Unknown option: %s", name);
    else if (options[opt_idx].flags & P_STRING)
	set_string_option(opt_idx, string);
    else
    {
	varp = get_varp(&options[opt_idx]);
	if (varp != NULL)	/* hidden option is not changed */
	{
	    if (options[opt_idx].flags & P_NUM)
		(void)set_num_option(opt_idx, varp, number, NULL);
	    else
		(void)set_bool_option(opt_idx, varp, (int)number);
	}
    }
}

/*
 * Get the terminal code for a terminal option.
 * Returns NULL when not found.
 */
    char_u *
get_term_code(tname)
    char_u	*tname;
{
    int	    opt_idx;
    char_u  *varp;

    if (tname[0] != 't' || tname[1] != '_' ||
	    tname[2] == NUL || tname[3] == NUL)
	return NULL;
    if ((opt_idx = findoption(tname)) >= 0)
    {
	varp = get_varp(&(options[opt_idx]));
	if (varp != NULL)
	    varp = *(char_u **)(varp);
	return varp;
    }
    return find_termcode(tname + 2);
}

    char_u *
get_highlight_default()
{
    int i;

    i = findoption((char_u *)"hl");
    if (i >= 0)
	return options[i].def_val[VI_DEFAULT];
    return (char_u *)NULL;
}

/*
 * Translate a string like "t_xx", "<t_xx>" or "<S-Tab>" to a key number.
 */
    static int
find_key_option(arg)
    char_u *arg;
{
    int		key;
    int		modifiers;

    /*
     * Don't use get_special_key_code() for t_xx, we don't want it to call
     * add_termcap_entry().
     */
    if (arg[0] == 't' && arg[1] == '_' && arg[2] && arg[3])
	key = TERMCAP2KEY(arg[2], arg[3]);
    else
    {
	--arg;			    /* put arg at the '<' */
	key = find_special_key(&arg, &modifiers, TRUE);
	if (modifiers)		    /* can't handle modifiers here */
	    key = 0;
    }
    return key;
}

/*
 * if 'all' == 0: show changed options
 * if 'all' == 1: show all normal options
 * if 'all' == 2: show all terminal options
 */
    static void
showoptions(all)
    int		all;
{
    struct vimoption   *p;
    int		    col;
    int		    isterm;
    char_u	    *varp;
    struct vimoption	**items;
    int		    item_count;
    int		    run;
    int		    row, rows;
    int		    cols;
    int		    i;
    int		    len;

#define INC 20
#define GAP 3

    items = (struct vimoption **)alloc((unsigned)(sizeof(struct vimoption *) *
								PARAM_COUNT));
    if (items == NULL)
	return;

    /* Highlight title */
    if (all == 2)
	MSG_PUTS_TITLE("\n--- Terminal codes ---");
    else
	MSG_PUTS_TITLE("\n--- Options ---");

    /*
     * do the loop two times:
     * 1. display the short items
     * 2. display the long items (only strings and numbers)
     */
    for (run = 1; run <= 2 && !got_int; ++run)
    {
	/*
	 * collect the items in items[]
	 */
	item_count = 0;
	for (p = &options[0]; p->fullname != NULL; p++)
	{
	    isterm = istermoption(p);
	    varp = get_varp(p);
	    if (varp != NULL
		    && ((all == 2 && isterm)
			|| (all == 1 && !isterm)
			|| (all == 0 && option_not_default(p))))
	    {
		if (p->flags & P_BOOL)
		    len = 1;		/* a toggle option fits always */
		else
		{
		    option_value2string(p);
		    len = STRLEN(p->fullname) + vim_strsize(NameBuff) + 1;
		}
		if ((len <= INC - GAP && run == 1) ||
						(len > INC - GAP && run == 2))
		    items[item_count++] = p;
	    }
	}

	/*
	 * display the items
	 */
	if (run == 1)
	{
	    cols = (Columns + GAP - 3) / INC;
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
		showoneopt(items[i]);
		col += INC;
	    }
	    out_flush();
	    ui_breakcheck();
	}
    }
    vim_free(items);
}

/*
 * Return TRUE if option is different from the default value
 */
    static int
option_not_default(p)
    struct vimoption	*p;
{
    char_u  *varp;
    int	    dvi;

    varp = get_varp(p);
    if (varp == NULL)
	return FALSE;	/* hidden option is never changed */

    if ((p->flags & P_VI_DEF) || p_cp)
	dvi = VI_DEFAULT;
    else
	dvi = VIM_DEFAULT;
    if (p->flags & P_NUM)
	return (*(long *)varp != (long)p->def_val[dvi]);
    if (p->flags & P_BOOL)
			/* the cast to long is required for Manx C */
	return (*(int *)varp != (int)(long)p->def_val[dvi]);
    /* P_STRING */
    return STRCMP(*(char_u **)varp, p->def_val[dvi]);
}

/*
 * showoneopt: show the value of one option
 * must not be called with a hidden option!
 */
    static void
showoneopt(p)
    struct vimoption	*p;
{
    char_u		*varp;

    varp = get_varp(p);

    if ((p->flags & P_BOOL) && !*(int *)varp)
	MSG_PUTS("no");
    else
	MSG_PUTS("  ");
    MSG_PUTS(p->fullname);
    if (!(p->flags & P_BOOL))
    {
	msg_putchar('=');
	option_value2string(p);	    /* put string of option value in NameBuff */
	msg_outtrans(NameBuff);
    }
}

/*
 * Write modified options as set command to a file.
 * Return FAIL on error, OK otherwise.
 */
    int
makeset(fd)
    FILE	*fd;
{
    struct vimoption	*p;
    char_u		*s;
    char_u		*varp;

    /*
     * The options that don't have a default (terminal name, columns, lines)
     * are never written. Terminal options are also not written.
     */
    for (p = &options[0]; !istermoption(p); p++)
	if (!(p->flags & P_NO_MKRC) && !istermoption(p)
						   && (option_not_default(p)))
	{
	    varp = get_varp(p);
	    if (p->flags & P_BOOL)
	    {
		if (fprintf(fd, "set %s%s", *(int *)(varp) ? "" : "no",
							     p->fullname) < 0)
		    return FAIL;
	    }
	    else if (p->flags & P_NUM)
	    {
		if (fprintf(fd, "set %s=%ld", p->fullname, *(long *)(varp)) < 0)
		    return FAIL;
	    }
	    else    /* P_STRING */
	    {
		if (fprintf(fd, "set %s=", p->fullname) < 0)
		    return FAIL;
		s = *(char_u **)(varp);
		/* some characters have to be escaped with CTRL-V or
		 * backslash */
		if (s != NULL && putescstr(fd, s, TRUE) == FAIL)
		    return FAIL;
	    }
	    if (put_eol(fd) < 0)
		return FAIL;
	}
    return OK;
}

/*
 * Clear all the terminal options.
 * If the option has been allocated, free the memory.
 * Terminal options are never hidden or indirect.
 */
    void
clear_termoptions()
{
    struct vimoption   *p;

    /*
     * Reset a few things before clearing the old options. This may cause
     * outputting a few things that the terminal doesn't understand, but the
     * screen will be cleared later, so this is OK.
     */
#ifdef USE_MOUSE
    mch_setmouse(FALSE);	    /* switch mouse off */
#endif
#ifdef XTERM_CLIP
    clear_xterm_clip();
#endif
#ifdef WANT_TITLE
    mch_restore_title(3);	    /* restore window titles */
#endif
#ifdef WIN32
    /*
     * Check if this is allowed now.
     */
    if (can_end_termcap_mode(FALSE) == TRUE)
#endif
	stoptermcap();			/* stop termcap mode */

    for (p = &options[0]; p->fullname != NULL; p++)
	if (istermoption(p))
	{
	    if (p->flags & P_ALLOCED)
		free_string_option(*(char_u **)(p->var));
	    if (p->flags & P_DEF_ALLOCED)
		free_string_option(p->def_val[VI_DEFAULT]);
	    *(char_u **)(p->var) = empty_option;
	    p->def_val[VI_DEFAULT] = empty_option;
	    p->flags &= ~(P_ALLOCED|P_DEF_ALLOCED);
	}
    clear_termcodes();
}

/*
 * Set the terminal option defaults to the current value.
 * Used after setting the terminal name.
 */
    void
set_term_defaults()
{
    struct vimoption   *p;

    for (p = &options[0]; p->fullname != NULL; p++)
    {
	if (istermoption(p) && p->def_val[VI_DEFAULT] != *(char_u **)(p->var))
	{
	    if (p->flags & P_DEF_ALLOCED)
	    {
		free_string_option(p->def_val[VI_DEFAULT]);
		p->flags &= ~P_DEF_ALLOCED;
	    }
	    p->def_val[VI_DEFAULT] = *(char_u **)(p->var);
	    if (p->flags & P_ALLOCED)
	    {
		p->flags |= P_DEF_ALLOCED;
		p->flags &= ~P_ALLOCED;	 /* don't free the value now */
	    }
	}
    }
}

/*
 * return TRUE if 'p' starts with 't_'
 */
    static int
istermoption(p)
    struct vimoption *p;
{
    return (p->fullname[0] == 't' && p->fullname[1] == '_');
}

/*
 * Compute columns for ruler and shown command. 'sc_col' is also used to
 * decide what the maximum length of a message on the status line can be.
 * If there is a status line for the last window, 'sc_col' is independent
 * of 'ru_col'.
 */

#define COL_RULER 17	    /* columns needed by standard ruler */

    void
comp_col()
{
#ifdef CMDLINE_INFO
    int last_has_status = (p_ls == 2 || (p_ls == 1 && firstwin != lastwin));

    sc_col = 0;
    ru_col = 0;
    if (p_ru)
    {
#ifdef STATUSLINE
	ru_col = (ru_wid ? ru_wid : COL_RULER) + 1;
#else
	ru_col = COL_RULER + 1;
#endif
	/* no last status line, adjust sc_col */
	if (!last_has_status)
	    sc_col = ru_col;
    }
    if (p_sc)
    {
	sc_col += SHOWCMD_COLS;
	if (!p_ru || last_has_status)	    /* no need for separating space */
	    ++sc_col;
    }
    sc_col = Columns - sc_col;
    ru_col = Columns - ru_col;
    if (sc_col <= 0)		/* screen too narrow, will become a mess */
	sc_col = 1;
    if (ru_col <= 0)
	ru_col = 1;
#else
    sc_col = Columns;
    ru_col = Columns;
#endif
}

    static char_u *
get_varp(p)
    struct vimoption	*p;
{
    if (!(p->flags & P_IND) || p->var == NULL)
	return p->var;

    switch ((long)(p->var))
    {
	case PV_LIST:	return (char_u *)&(curwin->w_p_list);
	case PV_NU:	return (char_u *)&(curwin->w_p_nu);
#ifdef RIGHTLEFT
	case PV_RL:	return (char_u *)&(curwin->w_p_rl);
#endif
	case PV_SCROLL:	return (char_u *)&(curwin->w_p_scroll);
	case PV_WRAP:	return (char_u *)&(curwin->w_p_wrap);
#ifdef LINEBREAK
	case PV_LBR:	return (char_u *)&(curwin->w_p_lbr);
#endif
#ifdef SCROLLBIND
	case PV_SCBIND: return (char_u *)&(curwin->w_p_scb);
#endif

	case PV_AI:	return (char_u *)&(curbuf->b_p_ai);
	case PV_BIN:	return (char_u *)&(curbuf->b_p_bin);
#ifdef CINDENT
	case PV_CIN:	return (char_u *)&(curbuf->b_p_cin);
	case PV_CINK:	return (char_u *)&(curbuf->b_p_cink);
	case PV_CINO:	return (char_u *)&(curbuf->b_p_cino);
#endif
#if defined(SMARTINDENT) || defined(CINDENT)
	case PV_CINW:	return (char_u *)&(curbuf->b_p_cinw);
#endif
#ifdef COMMENTS
	case PV_COM:	return (char_u *)&(curbuf->b_p_com);
#endif
#ifdef INSERT_EXPAND
	case PV_CPT:	return (char_u *)&(curbuf->b_p_cpt);
#endif
	case PV_EOL:	return (char_u *)&(curbuf->b_p_eol);
	case PV_ET:	return (char_u *)&(curbuf->b_p_et);
#ifdef MULTI_BYTE
	case PV_FE:	return (char_u *)&(curbuf->b_p_fe);
#endif
	case PV_FF:	return (char_u *)&(curbuf->b_p_ff);
#ifdef AUTOCMD
	case PV_FT:	return (char_u *)&(curbuf->b_p_ft);
#endif
	case PV_FO:	return (char_u *)&(curbuf->b_p_fo);
	case PV_INF:	return (char_u *)&(curbuf->b_p_inf);
	case PV_ISK:	return (char_u *)&(curbuf->b_p_isk);
#ifdef CRYPTV
	case PV_KEY:	return (char_u *)&(curbuf->b_p_key);
#endif
	case PV_LISP:	return (char_u *)&(curbuf->b_p_lisp);
	case PV_ML:	return (char_u *)&(curbuf->b_p_ml);
	case PV_MPS:	return (char_u *)&(curbuf->b_p_mps);
	case PV_MOD:	return (char_u *)&(curbuf->b_changed);
	case PV_NF:	return (char_u *)&(curbuf->b_p_nf);
#ifdef WANT_OSFILETYPE
	case PV_OFT:	return (char_u *)&(curbuf->b_p_oft);
#endif
	case PV_RO:	return (char_u *)&(curbuf->b_p_ro);
#ifdef SMARTINDENT
	case PV_SI:	return (char_u *)&(curbuf->b_p_si);
#endif
#ifndef SHORT_FNAME
	case PV_SN:	return (char_u *)&(curbuf->b_p_sn);
#endif
	case PV_STS:	return (char_u *)&(curbuf->b_p_sts);
	case PV_SWF:	return (char_u *)&(curbuf->b_p_swf);
#ifdef SYNTAX_HL
	case PV_SYN:	return (char_u *)&(curbuf->b_p_syn);
#endif
	case PV_SW:	return (char_u *)&(curbuf->b_p_sw);
	case PV_TS:	return (char_u *)&(curbuf->b_p_ts);
	case PV_TW:	return (char_u *)&(curbuf->b_p_tw);
	case PV_TX:	return (char_u *)&(curbuf->b_p_tx);
	case PV_WM:	return (char_u *)&(curbuf->b_p_wm);
	default:	EMSG("get_varp ERROR");
    }
    /* always return a valid pointer to avoid a crash! */
    return (char_u *)&(curbuf->b_p_wm);
}

/*
 * Copy options from one window to another.
 * Used when creating a new window.
 * The 'scroll' option is not copied, because it depends on the window height.
 */
    void
win_copy_options(wp_from, wp_to)
    WIN	    *wp_from;
    WIN	    *wp_to;
{
    wp_to->w_p_list = wp_from->w_p_list;
    wp_to->w_p_nu = wp_from->w_p_nu;
#ifdef RIGHTLEFT
    wp_to->w_p_rl = wp_from->w_p_rl;
# ifdef FKMAP
    wp_to->w_p_pers = wp_from->w_p_pers;
# endif
#endif
    wp_to->w_p_wrap = wp_from->w_p_wrap;
#ifdef LINEBREAK
    wp_to->w_p_lbr = wp_from->w_p_lbr;
#endif
#ifdef SCROLLBIND
    wp_to->w_p_scb = wp_from->w_p_scb;
#endif
}

/*
 * Copy options from one buffer to another.
 * Used when creating a new buffer and sometimes when entering a buffer.
 * flags:
 * BCO_ENTER	We will enter the bp_to buffer.
 * BCO_ALWAYS	Always copy the options, but only set b_p_initialized when
 *		appropriate.
 * BCO_NOHELP	Don't copy the help settings from or to a help buffer.
 */
    void
buf_copy_options(bp_from, bp_to, flags)
    BUF	    *bp_from;
    BUF	    *bp_to;
    int	    flags;
{
    int	    should_copy = TRUE;
    char_u  *save_p_isk = NULL;	    /* init for GCC */
    int	    dont_do_help;

    /*
     * Don't do anything of the "to" buffer is invalid.
     */
    if (bp_to == NULL || !buf_valid(bp_to))
	return;

    /*
     * Only copy if the "from" buffer is valid and "to" and "from" are
     * different.
     */
    if (bp_from != NULL && buf_valid(bp_from) && bp_from != bp_to)
    {
	/*
	 * Always copy when entering and 'cpo' contains 'S'.
	 * Don't copy when already initialized.
	 * Don't copy when 'cpo' contains 's' and not entering.
	 * 'S'	BCO_ENTER  initialized	's'  should_copy
	 * yes	  yes	       X	 X	TRUE
	 * yes	  no	      yes	 X	FALSE
	 * no	   X	      yes	 X	FALSE
	 *  X	  no	      no	yes	FALSE
	 *  X	  no	      no	no	TRUE
	 * no	  yes	      no	 X	TRUE
	 */
	if ((vim_strchr(p_cpo, CPO_BUFOPTGLOB) == NULL || !(flags & BCO_ENTER))
		&& (bp_to->b_p_initialized
		    || (!(flags & BCO_ENTER)
			&& vim_strchr(p_cpo, CPO_BUFOPT) != NULL)))
	    should_copy = FALSE;

	if (should_copy || (flags & BCO_ALWAYS))
	{
	    dont_do_help = (flags & BCO_NOHELP)
					&& (bp_to->b_help || bp_from->b_help);
	    if (dont_do_help)		/* don't free b_p_isk */
	    {
		save_p_isk = bp_to->b_p_isk;
		bp_to->b_p_isk = NULL;
	    }
	    /*
	     * Always free the allocated strings.
	     * If not already initialized, set 'readonly' and copy 'fileformat'.
	     */
	    if (!bp_to->b_p_initialized)
	    {
		free_buf_options(bp_to, TRUE);
		bp_to->b_p_ro = FALSE;		/* don't copy readonly */
		bp_to->b_p_tx = bp_from->b_p_tx;
		bp_to->b_p_ff = vim_strsave(bp_from->b_p_ff);
#ifdef MULTI_BYTE
		bp_to->b_p_fe = vim_strsave(bp_from->b_p_fe);
#endif
	    }
	    else
		free_buf_options(bp_to, FALSE);

	    bp_to->b_p_ai = bp_from->b_p_ai;
	    bp_to->b_p_ai_save = bp_from->b_p_ai_save;
	    bp_to->b_p_sw = bp_from->b_p_sw;
	    bp_to->b_p_tw = bp_from->b_p_tw;
	    bp_to->b_p_tw_save = bp_from->b_p_tw_save;
	    bp_to->b_p_tw_nobin = bp_from->b_p_tw_nobin;
	    bp_to->b_p_wm = bp_from->b_p_wm;
	    bp_to->b_p_wm_save = bp_from->b_p_wm_save;
	    bp_to->b_p_wm_nobin = bp_from->b_p_wm_nobin;
	    bp_to->b_p_bin = bp_from->b_p_bin;
	    bp_to->b_p_et = bp_from->b_p_et;
	    bp_to->b_p_et_nobin = bp_from->b_p_et_nobin;
	    bp_to->b_p_ml = bp_from->b_p_ml;
	    bp_to->b_p_ml_nobin = bp_from->b_p_ml_nobin;
	    bp_to->b_p_inf = bp_from->b_p_inf;
	    bp_to->b_p_swf = bp_from->b_p_swf;
#ifdef INSERT_EXPAND
	    bp_to->b_p_cpt = vim_strsave(bp_from->b_p_cpt);
#endif
	    bp_to->b_p_sts = bp_from->b_p_sts;
#ifndef SHORT_FNAME
	    bp_to->b_p_sn = bp_from->b_p_sn;
#endif
#ifdef COMMENTS
	    bp_to->b_p_com = vim_strsave(bp_from->b_p_com);
#endif
	    bp_to->b_p_fo = vim_strsave(bp_from->b_p_fo);
	    bp_to->b_p_nf = vim_strsave(bp_from->b_p_nf);
	    bp_to->b_p_mps = vim_strsave(bp_from->b_p_mps);
#ifdef SMARTINDENT
	    bp_to->b_p_si = bp_from->b_p_si;
	    bp_to->b_p_si_save = bp_from->b_p_si_save;
#endif
#ifdef CINDENT
	    bp_to->b_p_cin = bp_from->b_p_cin;
	    bp_to->b_p_cin_save = bp_from->b_p_cin_save;
	    bp_to->b_p_cink = vim_strsave(bp_from->b_p_cink);
	    bp_to->b_p_cino = vim_strsave(bp_from->b_p_cino);
#endif
#ifdef AUTOCMD
	    /* Don't copy 'filetype', it must be detected */
	    bp_to->b_p_ft = empty_option;
#endif
#ifdef WANT_OSFILETYPE
	    bp_to->b_p_oft = vim_strsave(bp_from->b_p_oft);
#endif
#if defined(SMARTINDENT) || defined(CINDENT)
	    bp_to->b_p_cinw = vim_strsave(bp_from->b_p_cinw);
#endif
#ifdef LISPINDENT
	    bp_to->b_p_lisp = bp_from->b_p_lisp;
	    bp_to->b_p_lisp_save = bp_from->b_p_lisp_save;
#endif
#ifdef SYNTAX_HL
	    /* Don't copy 'syntax', it must be set */
	    bp_to->b_p_syn = empty_option;
#endif
#ifdef CRYPTV
	    bp_to->b_p_key = vim_strsave(bp_from->b_p_key);
#endif

	    /*
	     * Don't copy the options set by do_help(), use the saved values,
	     * when going from a help buffer to a non-help buffer.
	     * Don't touch these at all when BCO_NOHELP is used and going from
	     * or to a help buffer.
	     */
	    if (dont_do_help)
		bp_to->b_p_isk = save_p_isk;
	    else
	    {
		if (!keep_help_flag && bp_from->b_help && !bp_to->b_help
						     && help_save_isk != NULL)
		{
		    bp_to->b_p_isk = vim_strsave(help_save_isk);
		    if (bp_to->b_p_isk != NULL)
			init_chartab();
		    bp_to->b_p_ts = help_save_ts;
		    bp_to->b_help = FALSE;
		}
		else
		{
		    bp_to->b_p_isk = vim_strsave(bp_from->b_p_isk);
		    mch_memmove(bp_to->b_chartab, bp_from->b_chartab,
								 (size_t)256);
		    bp_to->b_p_ts = bp_from->b_p_ts;
		    bp_to->b_help = bp_from->b_help;
		}
	    }
	}

	/*
	 * When the options should be copied (ignoring BCO_ALWAYS), set the
	 * flag that indicates that the options have been initialized.
	 */
	if (should_copy)
	    bp_to->b_p_initialized = TRUE;
    }

    check_buf_options(bp_to);	    /* make sure we don't have NULLs */
}


#if defined(CMDLINE_COMPL) || defined(PROTO)
static int expand_option_idx = -1;
static char_u expand_option_name[5] = {'t', '_', NUL, NUL, NUL};

    void
set_context_in_set_cmd(arg)
    char_u *arg;
{
    int		nextchar;
    int		flags = 0;	/* init for GCC */
    int		opt_idx = 0;	/* init for GCC */
    char_u	*p;
    char_u	*s;
    char_u	*after_blank = NULL;
    int		is_term_option = FALSE;
    int		key;

    expand_context = EXPAND_SETTINGS;
    if (*arg == NUL)
    {
	expand_pattern = arg;
	return;
    }
    p = arg + STRLEN(arg) - 1;
    if (*p == ' ' && *(p - 1) != '\\')
    {
	expand_pattern = p + 1;
	return;
    }
    while (p > arg)
    {
	s = p;
	/* count number of backslashes before ' ' or ',' */
	if (*p == ' ' || *p == ',')
	{
	    while (s > arg && *(s - 1) == '\\')
		--s;
	}
	/* break at a space with an even number of backslashes */
	if (*p == ' ' && ((p - s) & 1) == 0)
	{
	    ++p;
	    break;
	}
	/* remember possible start of file name to expand */
	if (after_blank == NULL
		&& ((*p == ' ' && (p - s) < 2)
		    || (*p == ',' && p == s)))
	    after_blank = p + 1;
	--p;
    }
    if (STRNCMP(p, "no", 2) == 0)
    {
	expand_context = EXPAND_BOOL_SETTINGS;
	p += 2;
    }
    if (STRNCMP(p, "inv", 3) == 0)
    {
	expand_context = EXPAND_BOOL_SETTINGS;
	p += 3;
    }
    expand_pattern = arg = p;
    if (*arg == '<')
    {
	while (*p != '>')
	    if (*p++ == NUL)	    /* expand terminal option name */
		return;
	key = get_special_key_code(arg + 1);
	if (key == 0)		    /* unknown name */
	{
	    expand_context = EXPAND_NOTHING;
	    return;
	}
	nextchar = *++p;
	is_term_option = TRUE;
	expand_option_name[2] = KEY2TERMCAP0(key);
	expand_option_name[3] = KEY2TERMCAP1(key);
    }
    else
    {
	if (p[0] == 't' && p[1] == '_')
	{
	    p += 2;
	    if (*p != NUL)
		++p;
	    if (*p == NUL)
		return;		/* expand option name */
	    nextchar = *++p;
	    is_term_option = TRUE;
	    expand_option_name[2] = p[-2];
	    expand_option_name[3] = p[-1];
	}
	else
	{
	    while (isalnum(*p) || *p == '_' || *p == '*')   /* Allow * wildcard */
		p++;
	    if (*p == NUL)
		return;
	    nextchar = *p;
	    *p = NUL;
	    opt_idx = findoption(arg);
	    *p = nextchar;
	    if (opt_idx == -1 || options[opt_idx].var == NULL)
	    {
		expand_context = EXPAND_NOTHING;
		return;
	    }
	    flags = options[opt_idx].flags;
	    if (flags & P_BOOL)
	    {
		expand_context = EXPAND_NOTHING;
		return;
	    }
	}
    }
    /* handle "-=" and "+=" */
    if ((nextchar == '-' || nextchar == '+') && p[1] == '=')
    {
	++p;
	nextchar = '=';
    }
    if ((nextchar != '=' && nextchar != ':')
				    || expand_context == EXPAND_BOOL_SETTINGS)
    {
	expand_context = EXPAND_UNSUCCESSFUL;
	return;
    }
    if (expand_context != EXPAND_BOOL_SETTINGS && p[1] == NUL)
    {
	expand_context = EXPAND_OLD_SETTING;
	if (is_term_option)
	    expand_option_idx = -1;
	else
	    expand_option_idx = opt_idx;
	expand_pattern = p + 1;
	return;
    }
    expand_context = EXPAND_NOTHING;
    if (is_term_option || (flags & P_NUM))
	return;
    if (after_blank != NULL)
	expand_pattern = after_blank;
    else
	expand_pattern = p + 1;
    if (flags & P_EXPAND)
    {
	p = options[opt_idx].var;
	if (p == (char_u *)&p_bdir || p == (char_u *)&p_dir ||
						       p == (char_u *)&p_path)
	{
	    expand_context = EXPAND_DIRECTORIES;
	    if (p == (char_u *)&p_path)
		expand_set_path = TRUE;
	}
	else
	    expand_context = EXPAND_FILES;
    }
    return;
}

    int
ExpandSettings(prog, num_file, file)
    vim_regexp	*prog;
    int		*num_file;
    char_u	***file;
{
    int num_normal = 0;	    /* Number of matching non-term-code settings */
    int num_term = 0;	    /* Number of matching terminal code settings */
    int opt_idx;
    int match;
    int count = 0;
    char_u *str;
    int	loop;
    int is_term_opt;
    char_u  name_buf[MAX_KEY_NAME_LEN];
    int	save_reg_ic;

    /* do this loop twice:
     * loop == 0: count the number of matching options
     * loop == 1: copy the matching options into allocated memory
     */
    for (loop = 0; loop <= 1; ++loop)
    {
	if (expand_context != EXPAND_BOOL_SETTINGS)
	{
	    if (vim_regexec(prog, (char_u *)"all", TRUE))
	    {
		if (loop == 0)
		    num_normal++;
		else
		    (*file)[count++] = vim_strsave((char_u *)"all");
	    }
	    if (vim_regexec(prog, (char_u *)"termcap", TRUE))
	    {
		if (loop == 0)
		    num_normal++;
		else
		    (*file)[count++] = vim_strsave((char_u *)"termcap");
	    }
	}
	for (opt_idx = 0; (str = (char_u *)options[opt_idx].fullname) != NULL;
								    opt_idx++)
	{
	    if (options[opt_idx].var == NULL)
		continue;
	    if (expand_context == EXPAND_BOOL_SETTINGS
	      && !(options[opt_idx].flags & P_BOOL))
		continue;
	    is_term_opt = istermoption(&options[opt_idx]);
	    if (is_term_opt && num_normal > 0)
		continue;
	    match = FALSE;
	    if (vim_regexec(prog, str, TRUE) ||
					(options[opt_idx].shortname != NULL &&
			 vim_regexec(prog,
				 (char_u *)options[opt_idx].shortname, TRUE)))
		match = TRUE;
	    else if (is_term_opt)
	    {
		name_buf[0] = '<';
		name_buf[1] = 't';
		name_buf[2] = '_';
		name_buf[3] = str[2];
		name_buf[4] = str[3];
		name_buf[5] = '>';
		name_buf[6] = NUL;
		if (vim_regexec(prog, name_buf, TRUE))
		{
		    match = TRUE;
		    str = name_buf;
		}
	    }
	    if (match)
	    {
		if (loop == 0)
		{
		    if (is_term_opt)
			num_term++;
		    else
			num_normal++;
		}
		else
		    (*file)[count++] = vim_strsave(str);
	    }
	}
	/*
	 * Check terminal key codes, these are not in the option table
	 */
	if (expand_context != EXPAND_BOOL_SETTINGS  && num_normal == 0)
	{
	    for (opt_idx = 0; (str = get_termcode(opt_idx)) != NULL; opt_idx++)
	    {
		if (!isprint(str[0]) || !isprint(str[1]))
		    continue;

		name_buf[0] = 't';
		name_buf[1] = '_';
		name_buf[2] = str[0];
		name_buf[3] = str[1];
		name_buf[4] = NUL;

		match = FALSE;
		if (vim_regexec(prog, name_buf, TRUE))
		    match = TRUE;
		else
		{
		    name_buf[0] = '<';
		    name_buf[1] = 't';
		    name_buf[2] = '_';
		    name_buf[3] = str[0];
		    name_buf[4] = str[1];
		    name_buf[5] = '>';
		    name_buf[6] = NUL;

		    if (vim_regexec(prog, name_buf, TRUE))
			match = TRUE;
		}
		if (match)
		{
		    if (loop == 0)
			num_term++;
		    else
			(*file)[count++] = vim_strsave(name_buf);
		}
	    }
	    /*
	     * Check special key names.
	     */
	    for (opt_idx = 0; (str = get_key_name(opt_idx)) != NULL; opt_idx++)
	    {
		name_buf[0] = '<';
		STRCPY(name_buf + 1, str);
		STRCAT(name_buf, ">");

		save_reg_ic = reg_ic;
		reg_ic = TRUE;			/* ignore case here */
		if (vim_regexec(prog, name_buf, TRUE))
		{
		    if (loop == 0)
			num_term++;
		    else
			(*file)[count++] = vim_strsave(name_buf);
		}
		reg_ic = save_reg_ic;
	    }
	}
	if (loop == 0)
	{
	    if (num_normal > 0)
		*num_file = num_normal;
	    else if (num_term > 0)
		*num_file = num_term;
	    else
		return OK;
	    *file = (char_u **) alloc((unsigned)(*num_file * sizeof(char_u *)));
	    if (*file == NULL)
	    {
		*file = (char_u **)"";
		return FAIL;
	    }
	}
    }
    return OK;
}

    int
ExpandOldSetting(num_file, file)
    int	    *num_file;
    char_u  ***file;
{
    char_u  *var = NULL;	/* init for GCC */
    char_u  *buf;

    *num_file = 0;
    *file = (char_u **)alloc((unsigned)sizeof(char_u *));
    if (*file == NULL)
	return FAIL;

    /*
     * For a terminal key code expand_option_idx is < 0.
     */
    if (expand_option_idx < 0)
    {
	var = find_termcode(expand_option_name + 2);
	if (var == NULL)
	    expand_option_idx = findoption(expand_option_name);
    }

    if (expand_option_idx >= 0)
    {
	/* put string of option value in NameBuff */
	option_value2string(&options[expand_option_idx]);
	var = NameBuff;
    }
    else if (var == NULL)
	var = (char_u *)"";

    /* A backslash is required before some characters */
    buf = vim_strsave_escaped(var, escape_chars);

    if (buf == NULL)
    {
	vim_free(*file);
	*file = NULL;
	return FAIL;
    }

    *file[0] = buf;
    *num_file = 1;
    return OK;
}
#endif

/*
 * Get the value for the numeric or string option *opp in a nice format into
 * NameBuff[].  Must not be called with a hidden option!
 */
    static void
option_value2string(opp)
    struct vimoption	*opp;
{
    char_u	*varp;

    varp = get_varp(opp);
    if (opp->flags & P_NUM)
    {
	if (((long *)varp == &p_wc) || ((long *)varp == &p_wcm))
	{
	    long wc = *(long *)varp;
	    if (IS_SPECIAL(wc) || find_special_key_in_table((int)wc) >= 0)
		STRCPY(NameBuff, get_special_key_name((int)wc, 0));
	    else
		STRCPY(NameBuff, transchar((int)wc));
	}
	else
	    sprintf((char *)NameBuff, "%ld", *(long *)varp);
    }
    else    /* P_STRING */
    {
	varp = *(char_u **)(varp);
	if (varp == NULL)		    /* just in case */
	    NameBuff[0] = NUL;
#ifdef CRYPTV
	/* don't show the actual value of 'key', only that it's set */
	if (opp->var == (char_u *)PV_KEY && *varp)
	    STRCPY(NameBuff, "*****");
#endif
	else if (opp->flags & P_EXPAND)
	    home_replace(NULL, varp, NameBuff, MAXPATHL, FALSE);
	/* Translate 'pastetoggle' into special key names */
	else if ((char_u **)opp->var == &p_pt)
	    str2specialbuf(p_pt, NameBuff, MAXPATHL);
	else
	    STRNCPY(NameBuff, varp, MAXPATHL);
    }
}

#ifdef HAVE_LANGMAP
/*
 * Any character has an equivalent character.  This is used for keyboards that
 * have a special language mode that sends characters above 128 (although
 * other characters can be translated too).
 */

/*
 * char_u langmap_mapchar[256];
 * Normally maps each of the 128 upper chars to an <128 ascii char; used to
 * "translate" native lang chars in normal mode or some cases of
 * insert mode without having to tediously switch lang mode back&forth.
 */

    static void
langmap_init()
{
    int i;

    for (i = 0; i < 256; i++)		/* we init with a-one-to one map */
	langmap_mapchar[i] = i;
}

/*
 * Called when langmap option is set; the language map can be
 * changed at any time!
 */
    static void
langmap_set()
{
    char_u  *p;
    char_u  *p2;
    int	    from, to;

    langmap_init();			    /* back to one-to-one map first */

    for (p = p_langmap; p[0]; )
    {
	for (p2 = p; p2[0] && p2[0] != ',' && p2[0] != ';'; ++p2)
	    if (p2[0] == '\\' && p2[1])
		++p2;
	if (p2[0] == ';')
	    ++p2;	    /* abcd;ABCD form, p2 points to A */
	else
	    p2 = NULL;	    /* aAbBcCdD form, p2 is NULL */
	while (p[0])
	{
	    if (p[0] == '\\' && p[1])
		++p;
	    from = p[0];
	    if (p2 == NULL)
	    {
		if (p[1] == '\\')
		    ++p;
		to = p[1];
	    }
	    else
	    {
		if (p2[0] == '\\')
		    ++p2;
		to = p2[0];
	    }
	    if (to == NUL)
	    {
		EMSG2("'langmap': Matching character missing for %s",
							     transchar(from));
		return;
	    }
	    langmap_mapchar[from] = to;

	    /* Advance to next pair */
	    if (p2 == NULL)
	    {
		p += 2;
		if (p[0] == ',')
		{
		    ++p;
		    break;
		}
	    }
	    else
	    {
		++p;
		++p2;
		if (*p == ';')
		{
		    p = p2;
		    if (p[0])
		    {
			if (p[0] != ',')
			{
			    EMSG2("'langmap': Extra characters after semicolon: %s", p);
			    return;
			}
			++p;
		    }
		    break;
		}
	    }
	}
    }
}
#endif

/*
 * Return TRUE if format option 'x' is in effect.
 * Take care of no formatting when 'paste' is set.
 */
    int
has_format_option(x)
    int	    x;
{
    if (p_paste)
	return FALSE;
    return (vim_strchr(curbuf->b_p_fo, x) != NULL);
}

/*
 * Return TRUE if "x" is present in 'shortmess' option, or
 * 'shortmess' contains 'a' and "x" is present in SHM_A.
 */
    int
shortmess(x)
    int	    x;
{
    return (   vim_strchr(p_shm, x) != NULL
	    || (vim_strchr(p_shm, 'a') != NULL
		&& vim_strchr((char_u *)SHM_A, x) != NULL));
}

/*
 * set_paste_option() - Called after p_paste was set or reset.
 */
    static void
paste_option_changed()
{
    static int	    old_p_paste = FALSE;
    static int	    save_sm = 0;
#ifdef CMDLINE_INFO
    static int	    save_ru = 0;
#endif
#ifdef RIGHTLEFT
    static int	    save_ri = 0;
    static int	    save_hkmap = 0;
#endif
    BUF		    *buf;

    if (p_paste)
    {
	/*
	 * Paste switched from off to on.
	 * Save the current values, so they can be restored later.
	 */
	if (!old_p_paste)
	{
	    /* save options for each buffer */
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	    {
		buf->b_p_tw_save = buf->b_p_tw;
		buf->b_p_wm_save = buf->b_p_wm;
		buf->b_p_sts_save = buf->b_p_sts;
		buf->b_p_ai_save = buf->b_p_ai;
#ifdef SMARTINDENT
		buf->b_p_si_save = buf->b_p_si;
#endif
#ifdef CINDENT
		buf->b_p_cin_save = buf->b_p_cin;
#endif
#ifdef LISPINDENT
		buf->b_p_lisp_save = buf->b_p_lisp;
#endif
	    }

	    /* save global options */
	    save_sm = p_sm;
#ifdef CMDLINE_INFO
	    save_ru = p_ru;
#endif
#ifdef RIGHTLEFT
	    save_ri = p_ri;
	    save_hkmap = p_hkmap;
#endif
	}

	/*
	 * Always set the option values, also when 'paste' is set when it is
	 * already on.
	 */
	/* set options for each buffer */
	for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	{
	    buf->b_p_tw = 0;	    /* textwidth is 0 */
	    buf->b_p_wm = 0;	    /* wrapmargin is 0 */
	    buf->b_p_sts = 0;	    /* softtabstop is 0 */
	    buf->b_p_ai = 0;	    /* no auto-indent */
#ifdef SMARTINDENT
	    buf->b_p_si = 0;	    /* no smart-indent */
#endif
#ifdef CINDENT
	    buf->b_p_cin = 0;	    /* no c indenting */
#endif
#ifdef LISPINDENT
	    buf->b_p_lisp = 0;	    /* no lisp indenting */
#endif
	}

	/* set global options */
	p_sm = 0;		    /* no showmatch */
#ifdef CMDLINE_INFO
	if (p_ru)
	    status_redraw_all();    /* redraw to remove the ruler */
	p_ru = 0;		    /* no ruler */
#endif
#ifdef RIGHTLEFT
	p_ri = 0;		    /* no reverse insert */
	p_hkmap = 0;		    /* no Hebrew keyboard */
#endif
    }

    /*
     * Paste switched from on to off: Restore saved values.
     */
    else if (old_p_paste)
    {
	/* restore options for each buffer */
	for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	{
	    buf->b_p_tw = buf->b_p_tw_save;
	    buf->b_p_wm = buf->b_p_wm_save;
	    buf->b_p_sts = buf->b_p_sts_save;
	    buf->b_p_ai = buf->b_p_ai_save;
#ifdef SMARTINDENT
	    buf->b_p_si = buf->b_p_si_save;
#endif
#ifdef CINDENT
	    buf->b_p_cin = buf->b_p_cin_save;
#endif
#ifdef LISPINDENT
	    buf->b_p_lisp = buf->b_p_lisp_save;
#endif
	}

	/* restore global options */
	p_sm = save_sm;
#ifdef CMDLINE_INFO
	if (p_ru != save_ru)
	    status_redraw_all();    /* redraw to draw the ruler */
	p_ru = save_ru;
#endif
#ifdef RIGHTLEFT
	p_ri = save_ri;
	p_hkmap = save_hkmap;
#endif
    }

    old_p_paste = p_paste;
}

/*
 * vimrc_found() - Called when a ".vimrc" or "VIMINIT" has been found.
 *
 * Reset 'compatible' and set the values for options that didn't get set yet
 * to the Vim defaults.
 * Don't do this if the 'compatible' option has been set or reset before.
 */
    void
vimrc_found()
{
    int	    opt_idx;

    if (!option_was_set((char_u *)"cp"))
    {
	p_cp = FALSE;
	for (opt_idx = 0; !istermoption(&options[opt_idx]); opt_idx++)
	    if (!(options[opt_idx].flags & (P_WAS_SET|P_VI_DEF)))
		set_option_default(opt_idx, TRUE);
	init_chartab();		    /* make b_p_isk take effect */
    }
}

/*
 * Set 'compatible' on or off.  Called for "-C" and "-N" command line arg.
 */
    void
change_compatible(on)
    int	    on;
{
    if (p_cp != on)
    {
	p_cp = on;
	compatible_set();
    }
    options[findoption((char_u *)"cp")].flags |= P_WAS_SET;
}

/*
 * Return TRUE when option "name" has been set.
 */
    int
option_was_set(name)
    char_u	*name;
{
    int idx;

    idx = findoption(name);
    if (idx < 0)	/* unknown option */
	return FALSE;
    if (options[idx].flags & P_WAS_SET)
	return TRUE;
    return FALSE;
}

/*
 * compatible_set() - Called when 'compatible' has been set or unset.
 *
 * When 'compatible' set: Set all relevant options (those that have the P_VIM)
 * flag) to a Vi compatible value.
 * When 'compatible' is unset: Set all options that have a different default
 * for Vim (without the P_VI_DEF flag) to that default.
 */
    static void
compatible_set()
{
    int	    opt_idx;

    for (opt_idx = 0; !istermoption(&options[opt_idx]); opt_idx++)
	if (	   ((options[opt_idx].flags & P_VIM) && p_cp)
		|| (!(options[opt_idx].flags & P_VI_DEF) && !p_cp))
	    set_option_default(opt_idx, TRUE);
    init_chartab();		    /* make b_p_isk take effect */
}

#ifdef LINEBREAK

# if defined(__BORLANDC__) && (__BORLANDC__ < 0x500)
   /* Borland C++ screws up loop optimisation here (negri) */
#  pragma option -O-l
# endif

/*
 * fill_breakat_flags() -- called when 'breakat' changes value.
 */
    static void
fill_breakat_flags()
{
    char_u	*c;
    int		i;

    for (i = 0; i < 256; i++)
	breakat_flags[i] = FALSE;

    if (p_breakat != NULL)
	for (c = p_breakat; *c; c++)
	    breakat_flags[*c] = TRUE;
}

# if defined(__BORLANDC__) && (__BORLANDC__ < 0x500)
#  pragma option -O.l
# endif

#endif

/*
 * Check an option that can be a range of string values.
 *
 * Return OK for correct value, FAIL otherwise.
 */
    static int
check_opt_strings(val, values, list)
    char_u  *val;
    char    **values;
    int	    list;	    /* when TRUE: accept a list of values */
{
    int	    i;
    int	    len;

    while (*val)
    {
	for (i = 0; ; ++i)
	{
	    if (values[i] == NULL)	/* val not found in values[] */
		return FAIL;

	    len = STRLEN(values[i]);
	    if (STRNCMP(values[i], val, len) == 0)
	    {
		if (list && val[len] == ',')
		{
		    val += len + 1;
		    break;		/* check next item in val list */
		}
		else if (val[len] == NUL)
		    return OK;
	    }
	}
    }
    return OK;
}

/*
 * Read the 'wildmode' option, fill wim_flags[].
 */
    static int
check_opt_wim()
{
    char_u	new_wim_flags[4];
    char_u	*p;
    int		i;
    int		idx = 0;

    for (i = 0; i < 4; ++i)
	new_wim_flags[i] = 0;

    for (p = p_wim; *p; ++p)
    {
	for (i = 0; isalpha(p[i]); ++i)
	    ;
	if (p[i] != NUL && p[i] != ',' && p[i] != ':')
	    return FAIL;
	if (i == 7 && STRNCMP(p, "longest", 7) == 0)
	    new_wim_flags[idx] |= WIM_LONGEST;
	else if (i == 4 && STRNCMP(p, "full", 4) == 0)
	    new_wim_flags[idx] |= WIM_FULL;
	else if (i == 4 && STRNCMP(p, "list", 4) == 0)
	    new_wim_flags[idx] |= WIM_LIST;
	else
	    return FAIL;
	p += i;
	if (*p == NUL)
	    break;
	if (*p == ',')
	{
	    if (idx == 3)
		return FAIL;
	    ++idx;
	}
    }

    /* fill remaining entries with last flag */
    while (idx < 3)
    {
	new_wim_flags[idx + 1] = new_wim_flags[idx];
	++idx;
    }

    /* only when there are no errors, wim_flags[] is changed */
    for (i = 0; i < 4; ++i)
	wim_flags[i] = new_wim_flags[i];
    return OK;
}

/*
 * Check if backspacing over something is allowed.
 */
    int
can_bs(what)
    int		what;	    /* BS_INDENT, BS_EOL or BS_START */
{
    switch (*p_bs)
    {
	case '2':	return TRUE;
	case '1':	return (what != BS_START);
	case '0':	return FALSE;
    }
    return vim_strchr(p_bs, what) != NULL;
}
