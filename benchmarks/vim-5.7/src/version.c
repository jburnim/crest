/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include "vim.h"

/*
 * Vim originated from Stevie version 3.6 (Fish disk 217) by GRWalter (Fred)
 * It has been changed beyond recognition since then.
 *
 * Differences between version 4.x and 5.0, 5.1, etc. can be
 * found with ":help version5".
 * Differences between version 3.0 and 4.x can be found with ":help version4".
 * All the remarks about older versions have been removed, they are not very
 * interesting.
 */

#include "version.h"

char	*Version = VIM_VERSION_SHORT;
char	*mediumVersion = VIM_VERSION_MEDIUM;

#if defined(HAVE_DATE_TIME) || defined(PROTO)
# if (defined(VMS) && defined(VAXC)) || defined(PROTO)
char	longVersion[sizeof(VIM_VERSION_LONG_DATE) + sizeof(__DATE__)
						      + sizeof(__TIME__) + 3];
    void
make_version()
{
    /*
     * Construct the long version string.  Necessary because
     * VAX C can't catenate strings in the preprocessor.
     */
    strcpy(longVersion, VIM_VERSION_LONG_DATE);
    strcat(longVersion, __DATE__);
    strcat(longVersion, " ");
    strcat(longVersion, __TIME__);
    strcat(longVersion, ")");
}
# else
char	*longVersion = VIM_VERSION_LONG_DATE __DATE__ " " __TIME__ ")";
# endif
#else
char	*longVersion = VIM_VERSION_LONG;
#endif

static void version_msg __ARGS((char *s));

static char *(features[]) =
{
#ifdef AMIGA		/* only for Amiga systems */
# ifdef NO_ARP
	"-ARP",
# else
	"+ARP",
# endif
#endif
#ifdef AUTOCMD
	"+autocmd",
#else
	"-autocmd",
#endif
#ifdef USE_BROWSE
	"+browse",
#else
	"-browse",
#endif
#ifdef NO_BUILTIN_TCAPS
	"-builtin_terms",
#endif
#ifdef SOME_BUILTIN_TCAPS
	"+builtin_terms",
#endif
#ifdef ALL_BUILTIN_TCAPS
	"++builtin_terms",
#endif
#ifdef BYTE_OFFSET
	"+byte_offset",
#else
	"-byte_offset",
#endif
#ifdef CINDENT
	"+cindent",
#else
	"-cindent",
#endif
#ifdef CMDLINE_COMPL
	"+cmdline_compl",
#else
	"-cmdline_compl",
#endif
#ifdef CMDLINE_INFO
	"+cmdline_info",
#else
	"-cmdline_info",
#endif
#ifdef COMMENTS
	"+comments",
#else
	"-comments",
#endif
#ifdef CRYPTV
	"+cryptv",
#else
	"-cryptv",
#endif
#ifdef USE_CSCOPE
	"+cscope",
#else
	"-cscope",
#endif
#if defined(CON_DIALOG) && defined(GUI_DIALOG)
	"+dialog_con_gui",
#else
# if defined(CON_DIALOG)
	"+dialog_con",
# else
#  if defined(GUI_DIALOG)
	"+dialog_gui",
#  else
	"-dialog",
#  endif
# endif
#endif
#ifdef DIGRAPHS
	"+digraphs",
#else
	"-digraphs",
#endif
#ifdef EMACS_TAGS
	"+emacs_tags",
#else
	"-emacs_tags",
#endif
#ifdef WANT_EVAL
	"+eval",
#else
	"-eval",
#endif
#ifdef EX_EXTRA
	"+ex_extra",
#else
	"-ex_extra",
#endif
#ifdef EXTRA_SEARCH
	"+extra_search",
#else
	"-extra_search",
#endif
#ifdef FKMAP
	"+farsi",
#else
	"-farsi",
#endif
#ifdef FILE_IN_PATH
	"+file_in_path",
#else
	"-file_in_path",
#endif
#ifdef WANT_OSFILETYPE
	"+osfiletype",
#else
	"-osfiletype",
#endif
#ifdef FIND_IN_PATH
	"+find_in_path",
#else
	"-find_in_path",
#endif
	    /* only interesting on Unix systems */
#if !defined(USE_SYSTEM) && defined(UNIX)
	"+fork()",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef USE_GUI_GTK
	"+GUI_GTK",
# else
#  ifdef USE_GUI_MOTIF
	"+GUI_Motif",
#  else
#   ifdef USE_GUI_ATHENA
	"+GUI_Athena",
#   else
#    ifdef USE_GUI_BEOS
	"+GUI_BeOS",
#     else
	"-GUI",
#    endif
#   endif
#  endif
# endif
#endif
#ifdef HANGUL_INPUT
	"+hangul_input",
#else
	"-hangul_input",
#endif
#ifdef INSERT_EXPAND
	"+insert_expand",
#else
	"-insert_expand",
#endif
#ifdef HAVE_LANGMAP
	"+langmap",
#else
	"-langmap",
#endif
#ifdef LINEBREAK
	"+linebreak",
#else
	"-linebreak",
#endif
#ifdef LISPINDENT
	"+lispindent",
#else
	"-lispindent",
#endif
#ifdef WANT_MENU
	"+menu",
#else
	"-menu",
#endif
#ifdef MKSESSION
	"+mksession",
#else
	"-mksession",
#endif
#ifdef WANT_MODIFY_FNAME
	"+modify_fname",
#else
	"-modify_fname",
#endif
#ifdef USE_MOUSE
	"+mouse",
# else
	"-mouse",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef DEC_MOUSE
	"+mouse_dec",
# else
	"-mouse_dec",
# endif
# ifdef GPM_MOUSE
	"+mouse_gpm",
# else
	"-mouse_gpm",
# endif
# ifdef NETTERM_MOUSE
	"+mouse_netterm",
# else
	"-mouse_netterm",
# endif
# ifdef XTERM_MOUSE
	"+mouse_xterm",
# else
	"-mouse_xterm",
# endif
#endif
#ifdef MULTI_BYTE_IME
	"+multi_byte_ime",
#else
# ifdef MULTI_BYTE
	"+multi_byte",
# else
	"-multi_byte",
# endif
#endif
#ifdef USE_GUI_WIN32
# ifdef HAVE_OLE
	"+ole",
# else
	"-ole",
# endif
#endif
#ifdef HAVE_PERL_INTERP
	"+perl",
#else
	"-perl",
#endif
#ifdef HAVE_PYTHON
	"+python",
#else
	"-python",
#endif
#ifdef QUICKFIX
	"+quickfix",
#else
	"-quickfix",
#endif
#ifdef RIGHTLEFT
	"+rightleft",
#else
	"-rightleft",
#endif
#ifdef SCROLLBIND
	"+scrollbind",
#else
	"-scrollbind",
#endif
#ifdef SMARTINDENT
	"+smartindent",
#else
	"-smartindent",
#endif
#ifdef USE_SNIFF
	"+sniff",
#else
	"-sniff",
#endif
#ifdef STATUSLINE
	"+statusline",
#else
	"-statusline",
#endif
#ifdef SYNTAX_HL
	"+syntax",
#else
	"-syntax",
#endif
	    /* only interesting on Unix systems */
#if defined(USE_SYSTEM) && (defined(UNIX) || defined(__EMX__))
	"+system()",
#endif
#ifdef BINARY_TAGS
	"+tag_binary",
#else
	"-tag_binary",
#endif
#ifdef OLD_STATIC_TAGS
	"+tag_old_static",
#else
	"-tag_old_static",
#endif
#ifdef TAG_ANY_WHITE
	"+tag_any_white",
#else
	"-tag_any_white",
#endif
#ifdef HAVE_TCL
	"+tcl",
#else
	"-tcl",
#endif
#if defined(UNIX) || defined(__EMX__)
/* only Unix (or OS/2 with EMX!) can have terminfo instead of termcap */
# ifdef TERMINFO
	"+terminfo",
# else
	"-terminfo",
# endif
#else		    /* unix always includes termcap support */
# ifdef HAVE_TGETENT
	"+tgetent",
# else
	"-tgetent",
# endif
#endif
#ifdef TEXT_OBJECTS
	"+textobjects",
#else
	"-textobjects",
#endif
#ifdef WANT_TITLE
	"+title",
#else
	"-title",
#endif
#ifdef USER_COMMANDS
	"+user_commands",
#else
	"-user_commands",
#endif
#ifdef VISUALEXTRA
	"+visualextra",
#else
	"-visualextra",
#endif
#ifdef VIMINFO
	"+viminfo",
#else
	"-viminfo",
#endif
#ifdef WILDIGNORE
	"+wildignore",
#else
	"-wildignore",
#endif
#ifdef WILDMENU
	"+wildmenu",
#else
	"-wildmenu",
#endif
#ifdef WRITEBACKUP
	"+writebackup",
#else
	"-writebackup",
#endif
#if defined(UNIX) || defined(VMS)
# if defined(WANT_X11) && defined(HAVE_X11)
	"+X11",
# else
	"-X11",
# endif
#endif
#ifdef USE_FONTSET
	"+xfontset",
#else
	"-xfontset",
#endif
#ifdef USE_XIM
	"+xim",
#else
	"-xim",
#endif
#ifdef UNIX
#ifdef BROKEN_LOCALE
	"+brokenlocale",
#endif
# ifdef XTERM_CLIP
	"+xterm_clipboard",
# else
	"-xterm_clipboard",
# endif
#endif
#ifdef SAVE_XTERM_SCREEN
	"+xterm_save",
#else
	"-xterm_save",
#endif
	NULL
};

static int included_patches[] =
{   /* Add new patch number below this line */
/**/
    0
};

    int
highest_patch()
{
    int		i;
    int		h = 0;

    for (i = 0; included_patches[i] != 0; ++i)
	if (included_patches[i] > h)
	    h = included_patches[i];
    return h;
}

    void
do_version(arg)
    char_u  *arg;
{
    /*
     * Ignore a ":version 9.99" command.
     */
    if (*arg == NUL)
    {
	msg_putchar('\n');
	list_version();
    }
}

    void
list_version()
{
    int		i;
    int		first;
    char	*s = "";

    /*
     * When adding features here, don't forget to update the list of
     * internal variables in eval.c!
     */
    MSG(longVersion);
#ifdef WIN32
# ifdef USE_GUI_WIN32
#  if (_MSC_VER <= 1010)    /* Only MS VC 4.1 and earlier can do Win32s */
    MSG_PUTS("\nMS-Windows 16/32 bit GUI version");
#  else
    MSG_PUTS("\nMS-Windows 32 bit GUI version");
#  endif
    if (gui_is_win32s())
	MSG_PUTS(" in Win32s mode");
# ifdef HAVE_OLE
    MSG_PUTS(" with OLE support");
# endif
# else
    MSG_PUTS("\nMS-Windows 32 bit console version");
# endif
#endif
#ifdef WIN16
    MSG_PUTS("\nMS-Windows 16 bit version");
#endif
#ifdef MSDOS
# ifdef DJGPP
    MSG_PUTS("\n32 bit MS-DOS version");
# else
    MSG_PUTS("\n16 bit MS-DOS version");
# endif
#endif
#ifdef macintosh
    MSG_PUTS("\nMacOS version");
#endif
#ifdef RISCOS
    MSG_PUTS("\nRISC OS version");
#endif
#ifdef VMS
    MSG_PUTS("\nOpenVMS version");
#endif

    /* Print the list of patch numbers if there is at least one. */
    /* Print a range when patches are consecutive: "1-10, 12, 15-40, 42-45" */
    if (included_patches[0] != 0)
    {
	MSG_PUTS("\nIncluded patches: ");
	first = -1;
	/* find last one */
	for (i = 0; included_patches[i] != 0; ++i)
	    ;
	while (--i >= 0)
	{
	    if (first < 0)
		first = included_patches[i];
	    if (i == 0 || included_patches[i - 1] != included_patches[i] + 1)
	    {
		MSG_PUTS(s);
		s = ", ";
		msg_outnum((long)first);
		if (first != included_patches[i])
		{
		    MSG_PUTS("-");
		    msg_outnum((long)included_patches[i]);
		}
		first = -1;
	    }
	}
    }

#if defined(UNIX) || defined(VMS)
    if (*compiled_user != NUL)
    {
	MSG_PUTS("\nCompiled by ");
	MSG_PUTS(compiled_user);
	if (*compiled_sys != NUL)
	{
	    MSG_PUTS("@");
	    MSG_PUTS(compiled_sys);
	}
	MSG_PUTS(", with (+) or without (-):\n");
    }
    else
#endif
	MSG_PUTS("\nCompiled with (+) or without (-):\n");

    /* print all the features */
    for (i = 0; features[i] != NULL; ++i)
    {
	version_msg(features[i]);
	if (msg_col > 0)
	    msg_putchar(' ');
    }

    msg_putchar('\n');
#ifdef SYS_VIMRC_FILE
    version_msg("   system vimrc file: \"");
    version_msg(SYS_VIMRC_FILE);
    version_msg("\"\n");
#endif
#ifdef USR_VIMRC_FILE
    version_msg("     user vimrc file: \"");
    version_msg(USR_VIMRC_FILE);
    version_msg("\"\n");
#endif
#ifdef USR_VIMRC_FILE2
    version_msg(" 2nd user vimrc file: \"");
    version_msg(USR_VIMRC_FILE2);
    version_msg("\"\n");
#endif
#ifdef USR_VIMRC_FILE3
    version_msg("  3d user vimrc file: \"");
    version_msg(USR_VIMRC_FILE3);
    version_msg("\"\n");
#endif
#ifdef USR_EXRC_FILE
    version_msg("      user exrc file: \"");
    version_msg(USR_EXRC_FILE);
    version_msg("\"\n");
#endif
#ifdef USR_EXRC_FILE2
    version_msg("  2nd user exrc file: \"");
    version_msg(USR_EXRC_FILE2);
    version_msg("\"\n");
#endif
#ifdef USE_GUI
# ifdef SYS_GVIMRC_FILE
    version_msg("  system gvimrc file: \"");
    version_msg(SYS_GVIMRC_FILE);
    MSG_PUTS("\"\n");
# endif
    version_msg("    user gvimrc file: \"");
    version_msg(USR_GVIMRC_FILE);
    version_msg("\"\n");
# ifdef USR_GVIMRC_FILE2
    version_msg("2nd user gvimrc file: \"");
    version_msg(USR_GVIMRC_FILE2);
    version_msg("\"\n");
# endif
# ifdef USR_GVIMRC_FILE3
    version_msg(" 3d user gvimrc file: \"");
    version_msg(USR_GVIMRC_FILE3);
    version_msg("\"\n");
# endif
#endif
#ifdef USE_GUI
# ifdef SYS_MENU_FILE
    version_msg("    system menu file: \"");
    version_msg(SYS_MENU_FILE);
    MSG_PUTS("\"\n");
# endif
#endif
#ifdef HAVE_PATHDEF
    if (*default_vim_dir != NUL)
    {
	version_msg("  fall-back for $VIM: \"");
	version_msg((char *)default_vim_dir);
	MSG_PUTS("\"\n");
    }
    if (*default_vimruntime_dir != NUL)
    {
	version_msg(" f-b for $VIMRUNTIME: \"");
	version_msg((char *)default_vimruntime_dir);
	MSG_PUTS("\"\n");
    }
    version_msg("Compilation: ");
    version_msg((char *)all_cflags);
    msg_putchar('\n');
#ifdef VMS
    if (*compiler_version != NUL)
    {
	version_msg("Compiler: ");
	version_msg((char *)compiler_version);
	msg_putchar('\n');
    }
#endif
    version_msg("Linking: ");
    version_msg((char *)all_lflags);
#endif
#ifdef DEBUG
    msg_putchar('\n');
    version_msg("  DEBUG BUILD");
#endif
}

/*
 * Output a string for the version message.  If it's going to wrap, output a
 * newline, unless the message is too long to fit on the screen anyway.
 */
    static void
version_msg(s)
    char	*s;
{
    int		len = strlen(s);

    if (len < (int)Columns && msg_col + len >= (int)Columns)
	msg_putchar('\n');
    MSG_PUTS(s);
}
