/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include "vim.h"

/* Structure containing all the GUI information */
Gui gui;

static void gui_check_screen __ARGS((void));
static void gui_position_components __ARGS((int));
static void gui_outstr __ARGS((char_u *, int));
static void gui_delete_lines __ARGS((int row, int count));
static void gui_insert_lines __ARGS((int row, int count));
static void gui_update_scrollbars __ARGS((int));
static void gui_update_horiz_scrollbar __ARGS((int));
static WIN *y2win __ARGS((int y));

/*
 * The Athena scrollbars can move the thumb to after the end of the scrollbar,
 * this makes the thumb indicate the part of the text that is shown.  Motif
 * can't do this.
 */
#if defined(USE_GUI_ATHENA) || defined(macintosh)
# define SCROLL_PAST_END
#endif

/*
 * gui_start -- Called when user wants to start the GUI.
 *
 * Careful: This function can be called recursively when there is a ":gui"
 * command in the .gvimrc file.  Only the first call should fork, not the
 * recursive call.
 */
    void
gui_start()
{
    char_u	*old_term;
#if defined(UNIX) && !defined(__BEOS__)
    pid_t	pid = -1;
    int		dofork = TRUE;
#endif
#ifdef USE_GUI_GTK
    int	    pfd[2];
#endif
    static int	recursive = 0;

    old_term = vim_strsave(T_NAME);

    /*
     * Set_termname() will call gui_init() to start the GUI.
     * Set the "starting" flag, to indicate that the GUI will start.
     *
     * We don't want to open the GUI window until after we've read .gvimrc,
     * otherwise we don't know what font we will use, and hence we don't know
     * what size the window should be.	So if there are errors in the .gvimrc
     * file, they will have to go to the terminal: Set full_screen to FALSE.
     * full_screen will be set to TRUE again by a successful termcapinit().
     */
    settmode(TMODE_COOK);		/* stop RAW mode */
    if (full_screen)
	cursor_on();			/* needed for ":gui" in .vimrc */
    gui.starting = TRUE;
    full_screen = FALSE;

#if defined(UNIX) && !defined(__BEOS__)
    if (!gui.dofork || vim_strchr(p_go, GO_FORG) || recursive)
	dofork = FALSE;
#endif
    ++recursive;

#ifdef USE_GUI_GTK
    /*
     * for GTK, we need to fork a little earlier, before we setup any signals,
     * since signals are not inherited from parent->child, and this will screw
     * up GTK's signal handling.  what we do is create a pipe between the
     * parent and child.  the child then tries to start the GUI.  it writes the
     * value of gui.in_use to the pipe, so that the parent will know whether or
     * not it was successful.
     */

    if (dofork && pipe(pfd) != 0)
    {
	EMSG("pipe() error");
	dofork = FALSE;
    }
    if (dofork && (pid = fork()) < 0)
    {
	EMSG("fork() failed");
	dofork = FALSE;
	(void)close(pfd[0]);
	(void)close(pfd[1]);
    }
    if (dofork)
    {
	if (pid == 0)	/* child */
	{
	    (void)close(pfd[0]);	/* child closes its read end */

	    /* Do the setsid() first, otherwise the exit() of the parent may
	     * kill the child too (when starting gvim from inside a gvim). */
# if defined(HAVE_SETSID)
	    (void)setsid();
# else
	    (void)setpgid(0, 0);
# endif
	    termcapinit((char_u *)"builtin_gui");   /* goes to gui_init() */
	    gui.starting = recursive - 1;
	    (void)write(pfd[1], &gui.in_use, sizeof(int));
	    (void)close(pfd[1]);
	    if (!gui.in_use)
		exit(1);

#ifdef AUTOCMD
	    apply_autocmds(EVENT_GUIENTER, NULL, NULL, FALSE, curbuf);
#endif
	    --recursive;
	    return; /* child successfully started gui */
	}
	else		/* parent */
	{
	    int gui_in_use;
	    int i;

	    (void)close(pfd[1]);	/* parent closes its write end */

	    /*
	     * Repeat the read() when interrupted by a signal.  Could be a
	     * harmless SIGWINCH.
	     */
	    for (;;)
	    {
		i = read(pfd[0], &gui_in_use, sizeof(int));
		if (i != sizeof(int))
		    gui_in_use = 0;
#ifdef EINTR
		if (i >= 0 || errno != EINTR)
#endif
		    break;
	    }
	    if (gui_in_use)
		exit(0);	/* child was successful, parent can exit */

	    EMSG("child process failed to start gui");
	    (void)close(pfd[0]);
	    /* terminate the child process, in case it's still around */
	    (void)kill(pid, SIGTERM);
	}
    }
    else

#endif	/* USE_GUI_GTK */

    {
	termcapinit((char_u *)"builtin_gui");
	gui.starting = recursive - 1;
    }


    if (!gui.in_use)			/* failed to start GUI */
    {
	termcapinit(old_term);		/* back to old term settings */
	settmode(TMODE_RAW);		/* restart RAW mode */
#ifdef WANT_TITLE
	set_title_defaults();		/* set 'title' and 'icon' again */
#endif
    }

    vim_free(old_term);

#if defined(UNIX) && !defined(__BEOS__) && !defined(USE_GUI_GTK)
    /*
     * Quit the current process and continue in the child.
     * Makes "gvim file" disconnect from the shell it was started in.
     * Don't do this when Vim was started with "-f" or the 'f' flag is present
     * in 'guioptions'.
     */
    if (gui.in_use && dofork)
    {
	pid = fork();
	if (pid > 0)	    /* Parent */
	{
	    /* Give the child some time to do the setsid(), otherwise the
	     * exit() may kill the child too (when starting gvim from inside a
	     * gvim). */
	    ui_delay(100L, TRUE);
	    exit(0);
	}

# if defined(HAVE_SETSID) || defined(HAVE_SETPGID)
	/*
	 * Change our process group.  On some systems/shells a CTRL-C in the
	 * shell where Vim was started would otherwise kill gvim!
	 */
	if (pid == 0)	    /* child */
#  if defined(HAVE_SETSID)
	    (void)setsid();
#  else
	    (void)setpgid(0, 0);
#  endif
# endif
    }
#endif

#ifdef AUTOCMD
    /* If the GUI started successfully, trigger the GUIEnter event */
    if (gui.in_use)
	apply_autocmds(EVENT_GUIENTER, NULL, NULL, FALSE, curbuf);
#endif
    --recursive;
}

/*
 * Call this when vim starts up, whether or not the GUI is started
 */
    void
gui_prepare(argc, argv)
    int	    *argc;
    char    **argv;
{
    gui.in_use = FALSE;		    /* No GUI yet (maybe later) */
    gui.starting = FALSE;	    /* No GUI yet (maybe later) */
    gui.dofork = TRUE;		    /* default is to use fork() */
    gui_mch_prepare(argc, argv);
}

/*
 * Try initializing the GUI and check if it can be started.
 * Used from main() to check early if "vim -g" can start the GUI.
 * Used from gui_init() to prepare for starting the GUI.
 * Returns FAIL or OK.
 */
    int
gui_init_check()
{
    static int result = MAYBE;

    if (result != MAYBE)
    {
	if (result == FAIL)
	    EMSG("Cannot start the GUI");
	return result;
    }

    gui.window_created = FALSE;
    gui.dying = FALSE;
    gui.in_focus = TRUE;		/* so the guicursor setting works */
    gui.dragged_sb = SBAR_NONE;
    gui.dragged_wp = NULL;
    gui.pointer_hidden = FALSE;
    gui.col = 0;
    gui.row = 0;
    gui.num_cols = Columns;
    gui.num_rows = Rows;

    gui.cursor_is_valid = FALSE;
    gui.scroll_region_top = 0;
    gui.scroll_region_bot = Rows - 1;
    gui.highlight_mask = HL_NORMAL;
    gui.char_width = 1;
    gui.char_height = 1;
    gui.char_ascent = 0;
    gui.border_width = 0;

    gui.norm_font = (GuiFont)NULL;
    gui.bold_font = (GuiFont)NULL;
    gui.ital_font = (GuiFont)NULL;
    gui.boldital_font = (GuiFont)NULL;
#ifdef USE_FONTSET
    gui.fontset = (GuiFont)NULL;
#endif

#ifdef WANT_MENU
    gui.menu_is_active = TRUE;	    /* default: include menu */
# ifndef USE_GUI_GTK
    gui.menu_height = MENU_DEFAULT_HEIGHT;
    gui.menu_width = 0;
# endif
#endif

    gui.scrollbar_width = gui.scrollbar_height = SB_DEFAULT_WIDTH;
    gui.prev_wrap = -1;

#ifdef ALWAYS_USE_GUI
    result = OK;
#else
    result = gui_mch_init_check();
#endif
    return result;
}

/*
 * This is the call which starts the GUI.
 */
    void
gui_init()
{
    WIN		*wp;
    static int	recursive = 0;

    /*
     * It's possible to use ":gui" in a .gvimrc file.  The first halve of this
     * function will then be executed at the first call, the rest by the
     * recursive call.  This allow the window to be opened halfway reading a
     * gvimrc file.
     */
    if (!recursive)
    {
	++recursive;

	clip_init(TRUE);

	/* If can't initialize, don't try doing the rest */
	if (gui_init_check() == FAIL)
	{
	    --recursive;
	    clip_init(FALSE);
	    return;
	}

	/*
	 * Set up system-wide default menus.
	 */
#if defined(SYS_MENU_FILE) && defined(WANT_MENU)
	if (vim_strchr(p_go, GO_NOSYSMENU) == NULL)
	{
	    sys_menu = TRUE;
	    do_source((char_u *)SYS_MENU_FILE, FALSE, FALSE);
	    sys_menu = FALSE;
	}
#endif

	/*
	 * Switch on the mouse by default, unless the user changed it already.
	 * This can then be changed in the .gvimrc.
	 */
	if (!option_was_set((char_u *)"mouse"))
	    set_string_option_direct((char_u *)"mouse", -1,
							 (char_u *)"a", TRUE);

	/*
	 * If -U option given, use only the initializations from that file and
	 * nothing else.  Skip all initializations for "-U NONE".
	 */
	if (use_gvimrc != NULL)
	{
	    if (STRCMP(use_gvimrc, "NONE") != 0
		    && do_source(use_gvimrc, FALSE, FALSE) != OK)
		EMSG2("Cannot read from \"%s\"", use_gvimrc);
	}
	else
	{
	    /*
	     * Get system wide defaults for gvim, only when file name defined.
	     */
#ifdef SYS_GVIMRC_FILE
	    do_source((char_u *)SYS_GVIMRC_FILE, FALSE, FALSE);
#endif

	    /*
	     * Try to read GUI initialization commands from the following
	     * places:
	     * - environment variable GVIMINIT
	     * - the user gvimrc file (~/.gvimrc)
	     * - the second user gvimrc file ($VIM/.gvimrc for Dos)
	     * - the third user gvimrc file ($VIM/.gvimrc for Amiga)
	     * The first that exists is used, the rest is ignored.
	     */
	    if (process_env((char_u *)"GVIMINIT", FALSE) == FAIL
		 && do_source((char_u *)USR_GVIMRC_FILE, TRUE, FALSE) == FAIL
#ifdef USR_GVIMRC_FILE2
		 && do_source((char_u *)USR_GVIMRC_FILE2, TRUE, FALSE) == FAIL
#endif
				)
	    {
#ifdef USR_GVIMRC_FILE3
		(void)do_source((char_u *)USR_GVIMRC_FILE3, TRUE, FALSE);
#endif
	    }

	    /*
	     * Read initialization commands from ".gvimrc" in current
	     * directory.  This is only done if the 'exrc' option is set.
	     * Because of security reasons we disallow shell and write
	     * commands now, except for unix if the file is owned by the user
	     * or 'secure' option has been reset in environment of global
	     * ".gvimrc".
	     * Only do this if GVIMRC_FILE is not the same as USR_GVIMRC_FILE,
	     * USR_GVIMRC_FILE2, USR_GVIMRC_FILE3 or SYS_GVIMRC_FILE.
	     */
	    if (p_exrc)
	    {
#ifdef UNIX
		{
		    struct stat s;

		    /* if ".gvimrc" file is not owned by user, set 'secure'
		     * mode */
		    if (mch_stat(GVIMRC_FILE, &s) || s.st_uid != getuid())
			secure = p_secure;
		}
#else
		secure = p_secure;
#endif

		if (       fullpathcmp((char_u *)USR_GVIMRC_FILE,
				     (char_u *)GVIMRC_FILE, FALSE) != FPC_SAME
#ifdef SYS_GVIMRC_FILE
			&& fullpathcmp((char_u *)SYS_GVIMRC_FILE,
				     (char_u *)GVIMRC_FILE, FALSE) != FPC_SAME
#endif
#ifdef USR_GVIMRC_FILE2
			&& fullpathcmp((char_u *)USR_GVIMRC_FILE2,
				     (char_u *)GVIMRC_FILE, FALSE) != FPC_SAME
#endif
#ifdef USR_GVIMRC_FILE3
			&& fullpathcmp((char_u *)USR_GVIMRC_FILE3,
				     (char_u *)GVIMRC_FILE, FALSE) != FPC_SAME
#endif
			)
		    do_source((char_u *)GVIMRC_FILE, TRUE, FALSE);

		if (secure == 2)
		    need_wait_return = TRUE;
		secure = 0;
	    }
	}

	if (need_wait_return || msg_didany)
	    wait_return(TRUE);

	--recursive;
    }

    /* If recursive call opened the window, return here from the first call */
    if (gui.in_use)
	return;

    /*
     * Create the GUI windows ready for opening.
     */
    gui.in_use = TRUE;		/* Must be set after menus have been set up */
    if (gui_mch_init() == FAIL)
	goto error;

    /*
     * Check validity of any generic resources that may have been loaded.
     */
    if (gui.border_width < 0)
	gui.border_width = 0;

    /*
     * Set up the fonts.
     */
    if (font_opt)
	set_option_value((char_u *)"gfn", 0L, (char_u *)font_opt);
    if (gui_init_font(p_guifont) == FAIL)
	goto error2;

    gui.num_cols = Columns;
    gui.num_rows = Rows;
    gui_reset_scroll_region();

    /* Create initial scrollbars */
    for (wp = firstwin; wp; wp = wp->w_next)
    {
	gui_create_scrollbar(&wp->w_scrollbars[SBAR_LEFT], wp);
	gui_create_scrollbar(&wp->w_scrollbars[SBAR_RIGHT], wp);
    }
    gui_create_scrollbar(&gui.bottom_sbar, NULL);

#ifdef WANT_MENU
    gui_create_initial_menus(root_menu, NULL);
#endif

    /* Configure the desired menu and scrollbars */
    gui_init_which_components(NULL);

    /* All components of the window have been created now */
    gui.window_created = TRUE;

#ifndef USE_GUI_GTK
    /* Set the window size, adjusted for the screen size.  For GTK this only
     * works after the window has been opened, thus it is further down. */
    gui_set_winsize(TRUE);
#endif

    /*
     * Actually open the GUI window.
     */
    if (gui_mch_open() != FAIL)
    {
#ifdef WANT_TITLE
	maketitle();
	resettitle();
#endif
	init_gui_options();
#ifdef USE_GUI_GTK
	/* Give GTK+ a chance to put all widget's into place. */
	gui_mch_update();
	/* Now make sure the window fits on the screen. */
	gui_set_winsize(TRUE);
#endif
	return;
    }

error2:
#ifdef USE_GUI_X11
    /* undo gui_mch_init() */
    gui_mch_uninit();
#endif

error:
    gui.in_use = FALSE;
    clip_init(FALSE);
}


    void
gui_exit(rc)
    int		rc;
{
#ifndef __BEOS__
    /* don't free the fonts, it leads to a BUS error
     * richard@whitequeen.com Jul 99 */
    free_highlight_fonts();
#endif
    gui.in_use = FALSE;
    gui_mch_exit(rc);
}

#if defined(USE_GUI_GTK) || defined(USE_GUI_X11) || defined(USE_GUI_MSWIN) || defined(PROTO)
/*
 * Called when the GUI window is closed by the user.  If there are no changed
 * files Vim exits, otherwise there will be a dialog to ask the user what to
 * do.
 * When this function returns, Vim should NOT exit!
 */
    void
gui_window_closed()
{
# ifdef USE_BROWSE
    int save_browse = browse;
# endif
# if defined(GUI_DIALOG) || defined(CON_DIALOG)
    int	save_confirm = confirm;
# endif

    /* Only exit when there are no changed files */
    exiting = TRUE;
# ifdef USE_BROWSE
    browse = TRUE;
# endif
# if defined(GUI_DIALOG) || defined(CON_DIALOG)
    confirm = TRUE;
# endif
    /* If there are changed buffers, present the user with a dialog if
     * possible, otherwise give an error message. */
    if (!check_changed_any(FALSE))
	getout(0);

    exiting = FALSE;
# ifdef USE_BROWSE
    browse = save_browse;
# endif
# if defined(GUI_DIALOG) || defined(CON_DIALOG)
    confirm = save_confirm;
# endif
    setcursor();		/* position cursor */
    out_flush();
}
#endif

/*
 * Set the font. Uses the 'font' option. The first font name that works is
 * used. If none is found, use the default font.
 * Return OK when able to set the font.
 */
    int
gui_init_font(font_list)
    char_u	*font_list;
{
#define FONTLEN 320
    char_u	font_name[FONTLEN];
    int		font_list_empty = FALSE;
    int		ret = FAIL;

    if (!gui.in_use)
	return FAIL;

    font_name[0] = NUL;
    if (*font_list == NUL)
	font_list_empty = TRUE;
    else
	while (*font_list != NUL)
	{
#ifdef USE_FONTSET
	    /* When using a fontset, use the whole list of fonts */
	    if (gui.fontset)
	    {
		STRNCPY(font_name, font_list, FONTLEN);
		font_list += STRLEN(font_list);
	    }
	    else
#endif
	    {
		/* Not using fontset: Isolate one comma separated font name */
		(void)copy_option_part(&font_list, font_name, FONTLEN, ",");
	    }
	    if (gui_mch_init_font(font_name) == OK)
	    {
		ret = OK;
		break;
	    }
	}

    if (ret != OK
	    && STRCMP(font_name, "*") != 0
	    && (font_list_empty || gui.norm_font == (GuiFont)0))
    {
	/*
	 * Couldn't load any font in 'font_list', keep the current font if
	 * there is one.  If 'font_list' is empty, or if there is no current
	 * font, tell gui_mch_init_font() to try to find a font we can load.
	 */
	ret = gui_mch_init_font(NULL);
    }

    if (ret == OK)
    {
	/* Set normal font as current font */
	gui_mch_set_font(gui.norm_font);
	gui_set_winsize(
#ifdef MSWIN
		TRUE
#else
		FALSE
#endif
		);
    }

    return ret;
}

    void
gui_set_cursor(row, col)
    int	    row;
    int	    col;
{
    gui.row = row;
    gui.col = col;
}

/*
 * gui_check_screen - check if the cursor is on the screen.
 */
    static void
gui_check_screen()
{
    if (gui.row >= Rows)
	gui.row = Rows - 1;
    if (gui.col >= Columns)
	gui.col = Columns - 1;
    if (gui.cursor_row >= Rows || gui.cursor_col >= Columns)
	gui.cursor_is_valid = FALSE;
}

/*
 * Redraw the cursor if necessary or when forced.
 * Careful: The contents of LinePointers[] must match what is on the screen,
 * otherwise this goes wrong.  May need to call out_flush() first.
 */
    void
gui_update_cursor(force, clear_selection)
    int		force;		/* when TRUE, update even when not moved */
    int		clear_selection;/* clear selection under cursor */
{
    int		cur_width = 0;
    int		cur_height = 0;
    long_u	old_hl_mask;
    int		idx;
    int		id;
    GuiColor	cfg, cbg, cc;	/* cursor fore-/background color */
    int		cattr;		/* cursor attributes */
    int		attr;
    struct attr_entry *aep = NULL;

    gui_check_screen();
    if (!gui.cursor_is_valid || force
		    || gui.row != gui.cursor_row || gui.col != gui.cursor_col)
    {
	gui_undraw_cursor();
	if (gui.row <0)
	    return;
#ifdef MULTI_BYTE_IME
	if (gui.row != gui.cursor_row || gui.col != gui.cursor_col)
	    ImeSetCompositionWindow();
#endif
	gui.cursor_row = gui.row;
	gui.cursor_col = gui.col;
	gui.cursor_is_valid = TRUE;

	/* Only write to the screen after LinePointers[] has been initialized */
	if (!screen_cleared || NextScreen == NULL)
	    return;

	/* Clear the selection if we are about to write over it */
	if (clear_selection)
	    clip_may_clear_selection(gui.row, gui.row);
	/* Check that the cursor is inside the window (resizing may have made
	 * it invalid) */
	if (gui.row >= screen_Rows || gui.col >= screen_Columns)
	    return;

	/*
	 * How the cursor is drawn depends on the current mode.
	 */
	idx = get_cursor_idx();
	id = cursor_table[idx].id;

	/* get the colors and attributes for the cursor.  Default is inverted */
	cfg = (GuiColor)-1;
	cbg = (GuiColor)-1;
	cattr = HL_INVERSE;
	gui_mch_set_blinking(cursor_table[idx].blinkwait,
			     cursor_table[idx].blinkon,
			     cursor_table[idx].blinkoff);
	if (id > 0)
	{
	    cattr = syn_id2colors(id, &cfg, &cbg);
	    --cbg;
	    --cfg;
	}

	/*
	 * Get the attributes for the character under the cursor.
	 * When no cursor color was given, use the character color.
	 */
	attr = *(LinePointers[gui.row] + gui.col + screen_Columns);
	if (attr > HL_ALL)
	    aep = syn_gui_attr2entry(attr);
	if (aep != NULL)
	{
	    attr = aep->ae_attr;
	    if (cfg < 0)
		cfg = ((attr & HL_INVERSE)  ? aep->ae_u.gui.bg_color
					    : aep->ae_u.gui.fg_color) - 1;
	    if (cbg < 0)
		cbg = ((attr & HL_INVERSE)  ? aep->ae_u.gui.fg_color
					    : aep->ae_u.gui.bg_color) - 1;
	}
	if (cfg < 0)
	    cfg = (attr & HL_INVERSE) ? gui.back_pixel : gui.norm_pixel;
	if (cbg < 0)
	    cbg = (attr & HL_INVERSE) ? gui.norm_pixel : gui.back_pixel;

#ifdef USE_XIM
	if (aep != NULL)
	{
	    xim_bg_color = ((attr & HL_INVERSE) ? aep->ae_u.gui.fg_color
						: aep->ae_u.gui.bg_color) - 1;
	    xim_fg_color = ((attr & HL_INVERSE) ? aep->ae_u.gui.bg_color
						: aep->ae_u.gui.fg_color) - 1;
	    if (xim_bg_color < 0)
		xim_bg_color = (attr & HL_INVERSE) ? gui.norm_pixel
						   : gui.back_pixel;
	    if (xim_fg_color < 0)
		xim_fg_color = (attr & HL_INVERSE) ? gui.back_pixel
						   : gui.norm_pixel;
	}
	else
	{
	    xim_bg_color = (attr & HL_INVERSE) ? gui.norm_pixel
					       : gui.back_pixel;
	    xim_fg_color = (attr & HL_INVERSE) ? gui.back_pixel
					       : gui.norm_pixel;
	}
#endif

	attr &= ~HL_INVERSE;
	if (cattr & HL_INVERSE)
	{
	    cc = cbg;
	    cbg = cfg;
	    cfg = cc;
	}
	cattr &= ~HL_INVERSE;

	/*
	 * When we don't have window focus, draw a hollow cursor.
	 */
	if (!gui.in_focus)
	{
	    gui_mch_draw_hollow_cursor(cbg);
	    return;
	}

	old_hl_mask = gui.highlight_mask;
	if (cursor_table[idx].shape == SHAPE_BLOCK
#ifdef HANGUL_INPUT
	    || composing_hangul
#endif
	    )
	{
	    /*
	     * Draw the text character with the cursor colors.	Use the
	     * character attributes plus the cursor attributes.
	     */
	    gui.highlight_mask = (cattr | attr);
#ifdef HANGUL_INPUT
	    gui_outstr_nowrap((composing_hangul ? composing_hangul_buffer
		: LinePointers[gui.row] + gui.col ), 1,
		    GUI_MON_IS_CURSOR | GUI_MON_NOCLEAR, cfg, cbg, 0);
#else
	    gui_outstr_nowrap(LinePointers[gui.row] + gui.col, 1,
			    GUI_MON_IS_CURSOR | GUI_MON_NOCLEAR, cfg, cbg, 0);
#endif
	}
	else
	{
	    /*
	     * First draw the partial cursor, then overwrite with the text
	     * character, using a transparant background.
	     */
	    if (cursor_table[idx].shape == SHAPE_VER)
	    {
		cur_height = gui.char_height;
		cur_width = (gui.char_width * cursor_table[idx].percentage
								  + 99) / 100;
	    }
	    else
	    {
		cur_height = (gui.char_height * cursor_table[idx].percentage
								  + 99) / 100;
		cur_width = gui.char_width;
#ifdef HANGUL_INPUT
		if (*(LinePointers[gui.row] + gui.col) & 0x80)
		    cur_width += gui.char_width;
#endif
	    }
	    gui_mch_draw_part_cursor(cur_width, cur_height, cbg);

#ifndef USE_GUI_MSWIN	    /* doesn't seem to work for MSWindows */
	    gui.highlight_mask = *(LinePointers[gui.row] + gui.col
							    + screen_Columns);
	    gui_outstr_nowrap(LinePointers[gui.row] + gui.col, 1,
		    GUI_MON_TRS_CURSOR | GUI_MON_NOCLEAR,
		    (GuiColor)0, (GuiColor)0, 0);
#endif
	}
	gui.highlight_mask = old_hl_mask;
    }
#ifdef USE_XIM
    xim_set_preedit();
#endif
}

#if defined(WANT_MENU) || defined(PROTO)
    void
gui_position_menu()
{
# ifndef USE_GUI_GTK
    if (gui.menu_is_active && gui.in_use)
	gui_mch_set_menu_pos(0, 0, gui.menu_width, gui.menu_height);
# endif
}
#endif

/*
 * Position the various GUI components (text area, menu).  The vertical
 * scrollbars are NOT handled here.  See gui_update_scrollbars().
 */
/*ARGSUSED*/
    static void
gui_position_components(total_width)
    int	    total_width;
{
    int	    text_area_x;
    int	    text_area_y;
    int	    text_area_width;
    int	    text_area_height;

    /* avoid that moving components around generates events */
    ++hold_gui_events;

    text_area_x = 0;
    if (gui.which_scrollbars[SBAR_LEFT])
	text_area_x += gui.scrollbar_width;

#if defined(USE_GUI_MSWIN) && defined(USE_TOOLBAR)
    if (vim_strchr(p_go, GO_TOOLBAR) != NULL)
	text_area_y = TOOLBAR_BUTTON_HEIGHT + TOOLBAR_BORDER_HEIGHT;
    else
#endif
	text_area_y = 0;

#if defined(WANT_MENU) && !defined(USE_GUI_GTK)
    gui.menu_width = total_width;
    if (gui.menu_is_active)
	text_area_y += gui.menu_height;
#endif
    text_area_width = gui.num_cols * gui.char_width + gui.border_offset * 2;
    text_area_height = gui.num_rows * gui.char_height + gui.border_offset * 2;

    gui_mch_set_text_area_pos(text_area_x,
			      text_area_y,
			      text_area_width,
			      text_area_height
#ifdef USE_XIM
				  + xim_get_status_area_height()
#endif
			      );
#ifdef WANT_MENU
    gui_position_menu();
#endif
    if (gui.which_scrollbars[SBAR_BOTTOM])
	gui_mch_set_scrollbar_pos(&gui.bottom_sbar,
				  text_area_x,
				  text_area_y + text_area_height,
				  text_area_width,
				  gui.scrollbar_height);
    gui.left_sbar_x = 0;
    gui.right_sbar_x = text_area_x + text_area_width;

    --hold_gui_events;
}

    int
gui_get_base_width()
{
    int	    base_width;

    base_width = 2 * gui.border_offset;
    if (gui.which_scrollbars[SBAR_LEFT])
	base_width += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_RIGHT])
	base_width += gui.scrollbar_width;
    return base_width;
}

    int
gui_get_base_height()
{
    int	    base_height;

    base_height = 2 * gui.border_offset;
    if (gui.which_scrollbars[SBAR_BOTTOM])
	base_height += gui.scrollbar_height;
#ifdef USE_GUI_GTK
    /* We can't take the sizes properly into acount until anuthing is realized.
     * Therefore we recalculate all the values here just before setting the
     * size. (--mdcki) */
#else
# ifdef WANT_MENU
    if (gui.menu_is_active)
	base_height += gui.menu_height;
# endif
# if defined(USE_GUI_MSWIN) && defined(USE_TOOLBAR)
    if (vim_strchr(p_go, GO_TOOLBAR) != NULL)
	base_height += (TOOLBAR_BUTTON_HEIGHT + TOOLBAR_BORDER_HEIGHT);
# endif
#endif
    return base_height;
}

/*
 * Should be called after the GUI window has been resized.  Its arguments are
 * the new width and height of the window in pixels.
 */
    void
gui_resize_window(pixel_width, pixel_height)
    int		pixel_width;
    int		pixel_height;
{
    static int	busy = FALSE;

    if (!gui.window_created)	    /* ignore when still initializing */
	return;

    /*
     * Can't resize the screen while it is being redrawn.  Remember the new
     * size and handle it later.
     */
    if (updating_screen || busy)
    {
	new_pixel_width = pixel_width;
	new_pixel_height = pixel_height;
	return;
    }

again:
    busy = TRUE;

#ifdef USE_GUI_BEOS
    vim_lock_screen();
#endif

    /* Flush pending output before redrawing */
    out_flush();

    gui.num_cols = (pixel_width - gui_get_base_width()) / gui.char_width;
    gui.num_rows = (pixel_height - gui_get_base_height()
				   + (gui.char_height / 2)) / gui.char_height;

    gui_position_components(pixel_width);

    gui_reset_scroll_region();
    /*
     * At the "more" and ":confirm" prompt there is no redraw, put the cursor
     * at the last line here (why does it have to be one row too low?).
     */
    if (State == ASKMORE || State == CONFIRM)
	gui.row = gui.num_rows;

    if (gui.num_rows != screen_Rows || gui.num_cols != screen_Columns)
	set_winsize(0, 0, FALSE);

#ifdef USE_GUI_BEOS
    vim_unlock_screen();
#endif

    gui_update_scrollbars(TRUE);
    gui_update_cursor(FALSE, TRUE);
#ifdef USE_XIM
    xim_set_status_area();
#endif

    busy = FALSE;
    /*
     * We could have been called again while redrawing the screen.
     * Need to do it all again with the latest size then.
     */
    if (new_pixel_height)
    {
	pixel_width = new_pixel_width;
	pixel_height = new_pixel_height;
	new_pixel_width = 0;
	new_pixel_height = 0;
	goto again;
    }
}

/*
 * Check if gui_resize_window() must be called.
 */
    void
gui_may_resize_window()
{
    int		h, w;

    if (new_pixel_height)
    {
	/* careful: gui_resize_window() may postpone the resize again if we
	 * were called indirectly by it */
	w = new_pixel_width;
	h = new_pixel_height;
	new_pixel_width = 0;
	new_pixel_height = 0;
	gui_resize_window(w, h);
    }
}

    int
gui_get_winsize()
{
    Rows = gui.num_rows;
    Columns = gui.num_cols;
    return OK;
}

/*
 * Set the size of the window according to Rows and Columns.
 */
    void
gui_set_winsize(fit_to_display)
    int	    fit_to_display;
{
    int	    base_width;
    int	    base_height;
    int	    width;
    int	    height;
    int	    min_width;
    int	    min_height;
    int	    screen_w;
    int	    screen_h;

    if (!gui.window_created)
	return;

    base_width = gui_get_base_width();
    base_height = gui_get_base_height();
    width = Columns * gui.char_width + base_width;
    height = Rows * gui.char_height + base_height;

    if (fit_to_display)
    {
	gui_mch_get_screen_dimensions(&screen_w, &screen_h);
	if (width > screen_w)
	{
	    Columns = (screen_w - base_width) / gui.char_width;
	    if (Columns < MIN_COLUMNS)
		Columns = MIN_COLUMNS;
	    gui.num_cols = Columns;
	    gui_reset_scroll_region();
	    width = Columns * gui.char_width + base_width;
	}
	if (height > screen_h)
	{
	    Rows = (screen_h - base_height) / gui.char_height;
	    check_winsize();
	    gui.num_rows = Rows;
	    gui_reset_scroll_region();
	    height = Rows * gui.char_height + base_height;
	}
    }

    min_width = base_width + MIN_COLUMNS * gui.char_width;
    min_height = base_height + MIN_LINES * gui.char_height;

    gui_mch_set_winsize(width, height, min_width, min_height,
			base_width, base_height);
    gui_position_components(width);
    gui_update_scrollbars(TRUE);
}

/*
 * Make scroll region cover whole screen.
 */
    void
gui_reset_scroll_region()
{
    gui.scroll_region_top = 0;
    gui.scroll_region_bot = gui.num_rows - 1;
}

    void
gui_start_highlight(mask)
    int	    mask;
{
    if (mask > HL_ALL)		    /* highlight code */
	gui.highlight_mask = mask;
    else			    /* mask */
	gui.highlight_mask |= mask;
}

    void
gui_stop_highlight(mask)
    int	    mask;
{
    if (mask > HL_ALL)		    /* highlight code */
	gui.highlight_mask = HL_NORMAL;
    else			    /* mask */
	gui.highlight_mask &= ~mask;
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_clear_block(row1, col1, row2, col2)
    int	    row1;
    int	    col1;
    int	    row2;
    int	    col2;
{
    /* Clear the selection if we are about to write over it */
    clip_may_clear_selection(row1, row2);

    gui_mch_clear_block(row1, col1, row2, col2);

    /* Invalidate cursor if it was in this block */
    if (       gui.cursor_row >= row1 && gui.cursor_row <= row2
	    && gui.cursor_col >= col1 && gui.cursor_col <= col2)
	gui.cursor_is_valid = FALSE;
}

/*
 * Write code to update cursor shape later.
 */
    void
gui_upd_cursor_shape()
{
    OUT_STR("\033|s");
}

    void
gui_write(s, len)
    char_u	*s;
    int		len;
{
    char_u	*p;
    int		arg1 = 0, arg2 = 0;
#if defined(RISCOS) || defined(WIN16)
    int		force = TRUE;	/* JK230798, stop Vim being smart or our
				   redraw speed will suffer */
#else
    int		force = FALSE;	/* force cursor update */
#endif
    int		force_scrollbar = FALSE;

/* #define DEBUG_GUI_WRITE */
#ifdef DEBUG_GUI_WRITE
    {
	int i;
	char_u *str;

	printf("gui_write(%d):\n    ", len);
	for (i = 0; i < len; i++)
	    if (s[i] == ESC)
	    {
		if (i != 0)
		    printf("\n    ");
		printf("<ESC>");
	    }
	    else
	    {
		str = transchar(s[i]);
		if (str[0] && str[1])
		    printf("<%s>", (char *)str);
		else
		    printf("%s", (char *)str);
	    }
	printf("\n");
    }
#endif
    while (len)
    {
	if (s[0] == ESC && s[1] == '|')
	{
	    p = s + 2;
	    if (isdigit(*p))
	    {
		arg1 = getdigits(&p);
		if (p > s + len)
		    break;
		if (*p == ';')
		{
		    ++p;
		    arg2 = getdigits(&p);
		    if (p > s + len)
			break;
		}
	    }
	    switch (*p)
	    {
		case 'C':	/* Clear screen */
		    clip_scroll_selection(9999);
		    gui_mch_clear_all();
		    gui.cursor_is_valid = FALSE;
		    force_scrollbar = TRUE;
		    break;
		case 'M':	/* Move cursor */
		    gui_set_cursor(arg1, arg2);
		    break;
		case 's':	/* force cursor (shape) update */
		    force = TRUE;
		    break;
		case 'R':	/* Set scroll region */
		    if (arg1 < arg2)
		    {
			gui.scroll_region_top = arg1;
			gui.scroll_region_bot = arg2;
		    }
		    else
		    {
			gui.scroll_region_top = arg2;
			gui.scroll_region_bot = arg1;
		    }
		    break;
		case 'd':	/* Delete line */
		    gui_delete_lines(gui.row, 1);
		    break;
		case 'D':	/* Delete lines */
		    gui_delete_lines(gui.row, arg1);
		    break;
		case 'i':	/* Insert line */
		    gui_insert_lines(gui.row, 1);
		    break;
		case 'I':	/* Insert lines */
		    gui_insert_lines(gui.row, arg1);
		    break;
		case '$':	/* Clear to end-of-line */
		    gui_clear_block(gui.row, gui.col, gui.row,
							    (int)Columns - 1);
		    break;
		case 'h':	/* Turn on highlighting */
		    gui_start_highlight(arg1);
		    break;
		case 'H':	/* Turn off highlighting */
		    gui_stop_highlight(arg1);
		    break;
		case 'f':	/* flash the window (visual bell) */
		    gui_mch_flash(arg1 == 0 ? 20 : arg1);
		    break;
		default:
		    p = s + 1;	/* Skip the ESC */
		    break;
	    }
	    len -= ++p - s;
	    s = p;
	}
	else if (s[0] < 0x20)		/* Ctrl character */
	{
	    if (s[0] == '\n')		/* NL */
	    {
		gui.col = 0;
		if (gui.row < gui.scroll_region_bot)
		    gui.row++;
		else
		    gui_delete_lines(gui.scroll_region_top, 1);
	    }
	    else if (s[0] == '\r')	/* CR */
	    {
		gui.col = 0;
	    }
	    else if (s[0] == '\b')	/* Backspace */
	    {
		if (gui.col)
		    --gui.col;
	    }
	    else if (s[0] == Ctrl('L'))	/* cursor-right */
	    {
		++gui.col;
	    }
	    else if (s[0] == Ctrl('G'))	/* Beep */
	    {
		gui_mch_beep();
	    }
	    /* Other Ctrl character: shouldn't happen! */

	    --len;	/* Skip this char */
	    ++s;
	}
	else
	{
	    p = s;
	    while (len && *p >= 0x20)
	    {
		len--;
		p++;
	    }
	    gui_outstr(s, p - s);
	    s = p;
	}
    }
    gui_update_cursor(force, TRUE);
    gui_update_scrollbars(force_scrollbar);

    /*
     * We need to make sure this is cleared since Athena doesn't tell us when
     * he is done dragging.
     */
#ifdef USE_GUI_ATHENA
    gui.dragged_sb = SBAR_NONE;
#endif

    gui_mch_flush();		    /* In case vim decides to take a nap */
}

    static void
gui_outstr(s, len)
    char_u  *s;
    int	    len;
{
    int	    this_len;

    if (len == 0)
	return;

    if (len < 0)
	len = STRLEN(s);

    while (gui.col + len > Columns)
    {
	this_len = Columns - gui.col;
	gui_outstr_nowrap(s, this_len, GUI_MON_WRAP_CURSOR, (GuiColor)0,
							      (GuiColor)0, 0);
	s += this_len;
	len -= this_len;
    }
    gui_outstr_nowrap(s, len, GUI_MON_WRAP_CURSOR, (GuiColor)0, (GuiColor)0, 0);
}

/*
 * Output the given string at the current cursor position.  If the string is
 * too long to fit on the line, then it is truncated.
 * "flags":
 * GUI_MON_WRAP_CURSOR may be used if the cursor position should be wrapped
 * when the end of the line is reached, however the string will still be
 * truncated and not continue on the next line.
 * GUI_MON_IS_CURSOR should only be used when this function is being called to
 * actually draw (an inverted) cursor.
 * GUI_MON_TRS_CURSOR is used to draw the cursor text with a transparant
 * background.
 */
    void
gui_outstr_nowrap(s, len, flags, fg, bg, back)
    char_u	*s;
    int		len;
    int		flags;
    GuiColor	fg, bg;	    /* colors for cursor */
    int		back;	    /* backup this many chars when using bold trick */
{
    long_u	    highlight_mask;
    GuiColor	    fg_color;
    GuiColor	    bg_color;
    GuiFont	    font;
    struct attr_entry *aep = NULL;
    int		    draw_flags;
    int		    col = gui.col;

    if (len == 0)
	return;

    if (len < 0)
	len = STRLEN(s);

    if (gui.highlight_mask > HL_ALL)
    {
	aep = syn_gui_attr2entry(gui.highlight_mask);
	if (aep == NULL)	    /* highlighting not set */
	    highlight_mask = 0;
	else
	    highlight_mask = aep->ae_attr;
    }
    else
	highlight_mask = gui.highlight_mask;

#ifndef MSWIN16_FASTTEXT
    /* Set the font */
    if (aep != NULL && aep->ae_u.gui.font != 0)
	font = aep->ae_u.gui.font;
    else
    {
	if ((highlight_mask & (HL_BOLD | HL_STANDOUT)) && gui.bold_font != 0)
	{
	    if ((highlight_mask & HL_ITALIC) && gui.boldital_font != 0)
		font = gui.boldital_font;
	    else
		font = gui.bold_font;
	}
	else if ((highlight_mask & HL_ITALIC) && gui.ital_font != 0)
	    font = gui.ital_font;
	else
	    font = gui.norm_font;
    }
    gui_mch_set_font(font);
#endif

    draw_flags = 0;

    /* Set the color */
    bg_color = gui.back_pixel;
    if ((flags & GUI_MON_IS_CURSOR) && gui.in_focus)
    {
	draw_flags |= DRAW_CURSOR;
	fg_color = fg;
	bg_color = bg;
    }
    else if (aep != NULL)
    {
	fg_color = aep->ae_u.gui.fg_color;
	if (fg_color == 0)
	    fg_color = gui.norm_pixel;
	else
	    --fg_color;
	bg_color = aep->ae_u.gui.bg_color;
	if (bg_color == 0)
	    bg_color = gui.back_pixel;
	else
	    --bg_color;
    }
    else
	fg_color = gui.norm_pixel;

    if (highlight_mask & (HL_INVERSE | HL_STANDOUT))
    {
#if defined(AMIGA) || defined(RISCOS)
	gui_mch_set_colors(bg_color, fg_color);
#else
	gui_mch_set_fg_color(bg_color);
	gui_mch_set_bg_color(fg_color);
#endif
    }
    else
    {
#if defined(AMIGA) || defined(RISCOS)
	gui_mch_set_colors(fg_color, bg_color);
#else
	gui_mch_set_fg_color(fg_color);
	gui_mch_set_bg_color(bg_color);
#endif
    }

    /* Clear the selection if we are about to write over it */
    if (!(flags & GUI_MON_NOCLEAR))
	clip_may_clear_selection(gui.row, gui.row);


    /* If there's no bold font, then fake it */
    if ((highlight_mask & (HL_BOLD | HL_STANDOUT)) &&
	    (gui.bold_font == 0 || (aep != NULL && aep->ae_u.gui.font != 0)))
	draw_flags |= DRAW_BOLD;

    /*
     * When drawing bold or italic characters the spill-over from the left
     * neighbor may be destroyed.  Backup to start redrawing just after a
     * blank.
     */
    if ((draw_flags & DRAW_BOLD) || (highlight_mask & HL_ITALIC))
    {
	s -= back;
	len += back;
	col -= back;
    }

#ifdef RISCOS
    /* If there's no italic font, then fake it */
    if ((highlight_mask & HL_ITALIC) && gui.ital_font == 0)
	draw_flags |= DRAW_ITALIC;

    /* Do we underline the text? */
    if (highlight_mask & HL_UNDERLINE)
	draw_flags |= DRAW_UNDERL;
#else
    /* Do we underline the text? */
    if ((highlight_mask & HL_UNDERLINE) ||
	    ((highlight_mask & HL_ITALIC) && gui.ital_font == 0))
	draw_flags |= DRAW_UNDERL;
#endif

    /* Do we draw transparantly? */
    if ((flags & GUI_MON_TRS_CURSOR))
	draw_flags |= DRAW_TRANSP;

    /* Draw the text */
    gui_mch_draw_string(gui.row, col, s, len, draw_flags);

    /* May need to invert it when it's part of the selection (assumes len==1) */
    if (flags & GUI_MON_NOCLEAR)
	clip_may_redraw_selection(gui.row, col);

    if (!(flags & (GUI_MON_IS_CURSOR | GUI_MON_TRS_CURSOR)))
    {
	/* Invalidate the old physical cursor position if we wrote over it */
	if (gui.cursor_row == gui.row && gui.cursor_col >= col
		&& gui.cursor_col < col + len)
	    gui.cursor_is_valid = FALSE;

	/* Update the cursor position */
	gui.col = col + len;
	if ((flags & GUI_MON_WRAP_CURSOR) && gui.col >= Columns)
	{
	    gui.col = 0;
	    gui.row++;
	}
    }
}

/*
 * Un-draw the cursor.	Actually this just redraws the character at the given
 * position.  The character just before it too, for when it was in bold.
 */
    void
gui_undraw_cursor()
{
    if (gui.cursor_is_valid)
    {
#ifdef HANGUL_INPUT
	if (composing_hangul && (gui.col == gui.cursor_col
				 && gui.row == gui.cursor_row))
	    gui_outstr_nowrap(composing_hangul_buffer, 1,
		    GUI_MON_IS_CURSOR | GUI_MON_NOCLEAR,
		    gui.norm_pixel, gui.back_pixel, 0);
	else
	{
#endif
	if (gui_redraw_block(gui.cursor_row, gui.cursor_col,
			      gui.cursor_row, gui.cursor_col, GUI_MON_NOCLEAR)
		&& gui.cursor_col > 0)
	    (void)gui_redraw_block(gui.cursor_row, gui.cursor_col - 1,
			 gui.cursor_row, gui.cursor_col - 1, GUI_MON_NOCLEAR);
#ifdef HANGUL_INPUT
	    if (composing_hangul)
		gui_redraw_block(gui.cursor_row, gui.cursor_col + 1,
			gui.cursor_row, gui.cursor_col + 1, GUI_MON_NOCLEAR);
	}
#endif
    }
}

    void
gui_redraw(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    int	    row1, col1, row2, col2;

    row1 = Y_2_ROW(y);
    col1 = X_2_COL(x);
    row2 = Y_2_ROW(y + h - 1);
    col2 = X_2_COL(x + w - 1);

    (void)gui_redraw_block(row1, col1, row2, col2, 0);

    /*
     * We may need to redraw the cursor, but don't take it upon us to change
     * its location after a scroll.
     * (maybe be more strict even and test col too?)
     * These things may be outside the update/clipping region and reality may
     * not reflect Vims internal ideas if these operations are clipped away.
     */
    if (gui.row == gui.cursor_row)
	gui_update_cursor(FALSE, TRUE);

    if (clipboard.state != SELECT_CLEARED)
	clip_redraw_selection(x, y, w, h);
}

/*
 * Draw a rectangular block of characters, from row1 to row2 (inclusive) and
 * from col1 to col2 (inclusive).
 * Return TRUE when the character before the first drawn character has
 * different attributes (may have to be redrawn too).
 */
    int
gui_redraw_block(row1, col1, row2, col2, flags)
    int	    row1;
    int	    col1;
    int	    row2;
    int	    col2;
    int	    flags;	/* flags for gui_outstr_nowrap() */
{
    int	    old_row, old_col;
    long_u  old_hl_mask;
    char_u  *screenp, *attrp, first_attr;
    int	    idx, len;
    int	    back;
    int	    retval = FALSE;
#ifdef MULTI_BYTE
    int	    orig_col1, orig_col2;
#endif

    /* Don't try to update when NextScreen is not valid */
    if (!screen_cleared || NextScreen == NULL)
	return retval;

    /* Don't try to draw outside the window! */
    /* Check everything, strange values may be caused by big border width */
    col1 = check_col(col1);
    col2 = check_col(col2);
    row1 = check_row(row1);
    row2 = check_row(row2);

    /* Remember where our cursor was */
    old_row = gui.row;
    old_col = gui.col;
    old_hl_mask = gui.highlight_mask;
#ifdef MULTI_BYTE
    orig_col1 = col1;
    orig_col2 = col2;
#endif

    for (gui.row = row1; gui.row <= row2; gui.row++)
    {
#ifdef MULTI_BYTE
	col1 = orig_col1;
	col2 = orig_col2;
	if (is_dbcs && mb_isbyte2(LinePointers[gui.row], col1))
	    col1--;
	if (is_dbcs && mb_isbyte1(LinePointers[gui.row], col2))
	    col2++;
#endif
	gui.col = col1;
	screenp = LinePointers[gui.row] + gui.col;
	attrp = screenp + screen_Columns;
	len = col2 - col1 + 1;

	/* Find how many chars back this highlighting starts, or where a space
	 * is.  Needed for when the bold trick is used */
	for (back = 0; back < col1; ++back)
	    if (attrp[-1 - back] != attrp[0] || screenp[-1 - back] == ' ')
		break;
	retval = (col1 && attrp[-1] && back == 0);

	/* break it up in strings of characters with the same attributes */
	while (len > 0)
	{
	    first_attr = attrp[0];
	    for (idx = 0; len > 0 && attrp[idx] == first_attr; idx++)
		--len;
	    gui.highlight_mask = first_attr;
	    gui_outstr_nowrap(screenp, idx, flags,
					      (GuiColor)0, (GuiColor)0, back);
	    screenp += idx;
	    attrp += idx;
	    back = 0;
	}
    }

    /* Put the cursor back where it was */
    gui.row = old_row;
    gui.col = old_col;
    gui.highlight_mask = old_hl_mask;

    return retval;
}

    static void
gui_delete_lines(row, count)
    int	    row;
    int	    count;
{
    if (row == 0)
	clip_scroll_selection(count);
    gui_mch_delete_lines(row, count);
}

    static void
gui_insert_lines(row, count)
    int	    row;
    int	    count;
{
    if (row == 0)
	clip_scroll_selection(-count);
    gui_mch_insert_lines(row, count);
}

/*
 * The main GUI input routine.	Waits for a character from the keyboard.
 * wtime == -1	    Wait forever.
 * wtime == 0	    Don't wait.
 * wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_wait_for_chars(wtime)
    long    wtime;
{
    int	    retval;
#ifdef AUTOCMD
    static int once_already = 0;
#endif

    /*
     * If we're going to wait a bit, update the menus for the current
     * State.
     */
#ifdef WANT_MENU
    if (wtime != 0)
	gui_update_menus(0);
#endif
    gui_mch_update();
    if (!vim_is_input_buf_empty())	/* Got char, return immediately */
    {
#ifdef AUTOCMD
	once_already = 0;
#endif
	return OK;
    }
    if (wtime == 0)	/* Don't wait for char */
    {
#ifdef AUTOCMD
	once_already = 0;
#endif
	return FAIL;
    }

    if (wtime > 0)
    {
	/* Blink when waiting for a character.	Probably only does something
	 * for showmatch() */
	gui_mch_start_blink();
	retval = gui_mch_wait_for_chars(wtime);
	gui_mch_stop_blink();
#ifdef AUTOCMD
	once_already = 0;
#endif
	return retval;
    }

    /*
     * While we are waiting indefenitely for a character, blink the cursor.
     */
    gui_mch_start_blink();


#ifdef AUTOCMD
    /* If there is no character available within 2 seconds (default),
     * write the autoscript file to disk */
    if (once_already == 2)
    {
	updatescript(0);
	retval = gui_mch_wait_for_chars(-1L);
	once_already = 0;
    }
    else if (once_already == 1)
    {
	setcursor();
	once_already = 2;
	retval = 0;
    }
    else
#endif
	if (gui_mch_wait_for_chars(p_ut) != OK)
    {
#ifdef AUTOCMD
	if (has_cursorhold() && get_real_state() == NORMAL_BUSY)
	{
	    apply_autocmds(EVENT_CURSORHOLD, NULL, NULL, FALSE, curbuf);
	    update_screen(VALID);

	    once_already = 1;
	    retval = 0;
	}
	else
#endif
	{
	    updatescript(0);
	    retval = gui_mch_wait_for_chars(-1L);
#ifdef AUTOCMD
	    once_already = 0;
#endif
	}
    }
    else
	retval = OK;

    gui_mch_stop_blink();
    return retval;
}

/*
 * Generic mouse support function.  Add a mouse event to the input buffer with
 * the given properties.
 *  button	    --- may be any of MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT,
 *			MOUSE_DRAG, or MOUSE_RELEASE.
 *			MOUSE_4 and MOUSE_5 are used for a scroll wheel.
 *  x, y	    --- Coordinates of mouse in pixels.
 *  repeated_click  --- TRUE if this click comes only a short time after a
 *			previous click.
 *  modifiers	    --- Bit field which may be any of the following modifiers
 *			or'ed together: MOUSE_SHIFT | MOUSE_CTRL | MOUSE_ALT.
 * This function will ignore drag events where the mouse has not moved to a new
 * character.
 */
    void
gui_send_mouse_event(button, x, y, repeated_click, modifiers)
    int	    button;
    int	    x;
    int	    y;
    int	    repeated_click;
    int_u   modifiers;
{
    static int	    prev_row = 0, prev_col = 0;
    static int	    prev_button = -1;
    static int	    num_clicks = 1;
    char_u	    string[6];
    int		    row, col;
#ifdef USE_CLIPBOARD
    int		    checkfor;
    int		    did_clip = FALSE;
#endif

    /*
     * Scrolling may happen at any time, also while a selection is present.
     */
    if (button == MOUSE_4 || button == MOUSE_5)
    {
	/* Don't put events in the input queue now. */
	if (hold_gui_events)
	    return;

	string[3] = CSI;
	string[4] = KS_EXTRA;
	string[5] = (int)(button == MOUSE_4 ? KE_MOUSEDOWN : KE_MOUSEUP);
	if (modifiers == 0)
	    add_to_input_buf(string + 3, 3);
	else
	{
	    string[0] = CSI;
	    string[1] = KS_MODIFIER;
	    string[2] = 0;
	    if (modifiers & MOUSE_SHIFT)
		string[2] |= MOD_MASK_SHIFT;
	    if (modifiers & MOUSE_CTRL)
		string[2] |= MOD_MASK_CTRL;
	    if (modifiers & MOUSE_ALT)
		string[2] |= MOD_MASK_ALT;
	    add_to_input_buf(string, 6);
	}
	return;
    }

#ifdef USE_CLIPBOARD
    /* If a clipboard selection is in progress, handle it */
    if (clipboard.state == SELECT_IN_PROGRESS)
    {
	clip_process_selection(button, x, y, repeated_click, modifiers);
	return;
    }

    /* Determine which mouse settings to look for based on the current mode */
    switch (get_real_state())
    {
	case NORMAL_BUSY:
	case OP_PENDING:
	case NORMAL:	    checkfor = MOUSE_NORMAL;	break;
	case VISUAL:	    checkfor = MOUSE_VISUAL;	break;
	case REPLACE:
	case VREPLACE:
	case INSERT:	    checkfor = MOUSE_INSERT;	break;
	case HITRETURN:	    checkfor = MOUSE_RETURN;	break;

	    /*
	     * On the command line, use the clipboard selection on all lines
	     * but the command line.  But not when pasting.
	     */
	case CMDLINE:
	    if (Y_2_ROW(y) < cmdline_row && button != MOUSE_MIDDLE)
		checkfor = ' ';
	    else
		checkfor = MOUSE_COMMAND;
	    break;

	default:
	    checkfor = ' ';
	    break;
    };

    /*
     * Allow clipboard selection of text on the command line in "normal"
     * modes.  Don't do this when dragging the status line, or extending a
     * Visual selection.
     */
    if ((State == NORMAL || State == NORMAL_BUSY
		|| State == INSERT || State == REPLACE || State == VREPLACE)
	    && Y_2_ROW(y) >= gui.num_rows - p_ch
	    && button != MOUSE_DRAG)
	checkfor = ' ';

    /*
     * In Ex mode, always use non-Visual selection.
     */
    if (exmode_active)
	checkfor = ' ';

    /*
     * If the mouse settings say to not use the mouse, use the non-Visual mode
     * selection.  But if Visual is active, assume that only the Visual area
     * will be selected.
     * Exception: On the command line, both the selection is used and a mouse
     * key is send.
     */
    if (!mouse_has(checkfor) || checkfor == MOUSE_COMMAND)
    {
	/* Don't do non-visual selection in Visual mode. */
	if (VIsual_active)
	    return;

	/*
	 * When 'mousemodel' is "popup", shift-left is translated to right.
	 */
	if (mouse_model_popup())
	{
	    if (button == MOUSE_LEFT && (modifiers & MOUSE_SHIFT))
	    {
		button = MOUSE_RIGHT;
		modifiers &= ~ MOUSE_SHIFT;
	    }
	}

	/* If the selection is done, allow the right button to extend it.
	 * If the selection is cleared, allow the right button to start it
	 * from the cursor position. */
	if (button == MOUSE_RIGHT)
	{
	    if (clipboard.state == SELECT_CLEARED)
	    {
		if (State == CMDLINE)
		{
		    col = msg_col;
		    row = msg_row;
		}
		else
		{
		    col = curwin->w_wcol;
		    row = curwin->w_wrow + curwin->w_winpos;
		}
		clip_start_selection(MOUSE_LEFT, FILL_X(col), FILL_Y(row),
								    FALSE, 0);
	    }
	    clip_process_selection(button, x, y, repeated_click, modifiers);
	    did_clip = TRUE;
	}
	/* Allow the left button to start the selection */
	else if (button ==
# ifdef RISCOS
		/* Only start a drag on a drag event. Otherwise
		 * we don't get a release event.
		 */
		    MOUSE_DRAG
# else
		    MOUSE_LEFT
# endif
				)
	{
	    clip_start_selection(button, x, y, repeated_click, modifiers);
	    did_clip = TRUE;
	}
# ifdef RISCOS
	else if (button == MOUSE_LEFT)
	{
	    clip_clear_selection();
	    did_clip = TRUE;
	}
# endif

	/* Always allow pasting */
	if (button != MOUSE_MIDDLE)
	{
	    if (!mouse_has(checkfor) || button == MOUSE_RELEASE)
		return;
	    button = MOUSE_LEFT;
	}
	repeated_click = FALSE;
    }

    if (clipboard.state != SELECT_CLEARED && !did_clip)
	clip_clear_selection();
#endif

    /* Don't put events in the input queue now. */
    if (hold_gui_events)
	return;

    row = check_row(Y_2_ROW(y));
    col = check_col(X_2_COL(x));
#ifdef MULTI_BYTE
    if (is_dbcs && prev_col > col && mb_isbyte1(LinePointers[row], col))
	col--;
#endif

    /*
     * If we are dragging and the mouse hasn't moved far enough to be on a
     * different character, then don't send an event to vim.
     */
    if (button == MOUSE_DRAG && row == prev_row && col == prev_col)
	return;

    /*
     * If topline has changed (window scrolled) since the last click, reset
     * repeated_click, because we don't want starting Visual mode when
     * clicking on a different character in the text.
     */
    if (curwin->w_topline != gui_prev_topline)
	repeated_click = FALSE;

    string[0] = CSI;	/* this sequence is recognized by check_termcode() */
    string[1] = KS_MOUSE;
    string[2] = K_FILLER;
    if (button != MOUSE_DRAG && button != MOUSE_RELEASE)
    {
	if (repeated_click)
	{
	    /*
	     * Handle multiple clicks.	They only count if the mouse is still
	     * pointing at the same character.
	     */
	    if (button != prev_button || row != prev_row || col != prev_col)
		num_clicks = 1;
	    else if (++num_clicks > 4)
		num_clicks = 1;
	}
	else
	    num_clicks = 1;
	prev_button = button;
	gui_prev_topline = curwin->w_topline;

	string[3] = (char_u)(button | 0x20);
	SET_NUM_MOUSE_CLICKS(string[3], num_clicks);
    }
    else
	string[3] = (char_u)button;

    string[3] |= modifiers;
    string[4] = (char_u)(col + ' ' + 1);
    string[5] = (char_u)(row + ' ' + 1);
    add_to_input_buf(string, 6);

    prev_row = row;
    prev_col = col;

    /*
     * We need to make sure this is cleared since Athena doesn't tell us when
     * he is done dragging.
     */
#ifdef USE_GUI_ATHENA
    gui.dragged_sb = SBAR_NONE;
#endif
}

#if defined(WANT_MENU) || defined(PROTO)
/*
 * Callback function for when a menu entry has been selected.
 */
    void
gui_menu_cb(menu)
    VimMenu *menu;
{
    char_u  bytes[3 + sizeof(long_u)];

    /* Don't put events in the input queue now. */
    if (hold_gui_events)
	return;

    bytes[0] = CSI;
    bytes[1] = KS_MENU;
    bytes[2] = K_FILLER;
    add_long_to_buf((long_u)menu, bytes + 3);
    add_to_input_buf(bytes, 3 + sizeof(long_u));
}
#endif

/*
 * Set which components are present.
 * If "oldval" is not NULL, "oldval" is the previous value, the new * value is
 * in p_go.
 */
/*ARGSUSED*/
    void
gui_init_which_components(oldval)
    char_u  *oldval;
{
    static int prev_which_scrollbars[3] = {-1, -1, -1};
#ifdef WANT_MENU
    static int prev_menu_is_active = -1;
#endif
#ifdef USE_TOOLBAR
    static int	prev_toolbar = -1;
    int		using_toolbar = FALSE;
#endif
#if defined(WANT_MENU) && !defined(WIN16)
    static int	prev_tearoff = -1;
    int		using_tearoff = FALSE;
#endif

    char_u  *p;
    int	    i;
#ifdef WANT_MENU
    int	    grey_old, grey_new;
    char_u  *temp;
#endif
    WIN	    *wp;
    int	    need_winsize;

#ifdef WANT_MENU
    if (oldval != NULL && gui.in_use)
    {
	/*
	 * Check if the menu's go from grey to non-grey or vise versa.
	 */
	grey_old = (vim_strchr(oldval, GO_GREY) != NULL);
	grey_new = (vim_strchr(p_go, GO_GREY) != NULL);
	if (grey_old != grey_new)
	{
	    temp = p_go;
	    p_go = oldval;
	    gui_update_menus(MENU_ALL_MODES);
	    p_go = temp;
	}
    }
    gui.menu_is_active = FALSE;
#endif

    for (i = 0; i < 3; i++)
	gui.which_scrollbars[i] = FALSE;
    for (p = p_go; *p; p++)
	switch (*p)
	{
	    case GO_LEFT:
		gui.which_scrollbars[SBAR_LEFT] = TRUE;
		break;
	    case GO_RIGHT:
		gui.which_scrollbars[SBAR_RIGHT] = TRUE;
		break;
	    case GO_BOT:
		gui.which_scrollbars[SBAR_BOTTOM] = TRUE;
		break;
#ifdef WANT_MENU
	    case GO_MENUS:
		gui.menu_is_active = TRUE;
		break;
#endif
	    case GO_GREY:
		/* make menu's have grey items, ignored here */
		break;
#ifdef USE_TOOLBAR
	    case GO_TOOLBAR:
		using_toolbar = TRUE;
		break;
#endif
	    case GO_TEAROFF:
#if defined(WANT_MENU) && !defined(WIN16)
		using_tearoff = TRUE;
#endif
		break;
	    default:
		/* Ignore options that are not supported */
		break;
	}
    if (gui.in_use)
    {
	need_winsize = FALSE;
	for (i = 0; i < 3; i++)
	{
	    if (gui.which_scrollbars[i] != prev_which_scrollbars[i])
	    {
		if (i == SBAR_BOTTOM)
		{
		    gui_mch_enable_scrollbar(&gui.bottom_sbar,
					     gui.which_scrollbars[i]);
		}
		else
		{
		    for (wp = firstwin; wp != NULL; wp = wp->w_next)
			gui_mch_enable_scrollbar(&wp->w_scrollbars[i],
						 gui.which_scrollbars[i]);
		}
		need_winsize = TRUE;
	    }
	    prev_which_scrollbars[i] = gui.which_scrollbars[i];
	}

#ifdef WANT_MENU
	if (gui.menu_is_active != prev_menu_is_active)
	{
	    gui_mch_enable_menu(gui.menu_is_active);
	    prev_menu_is_active = gui.menu_is_active;
	    need_winsize = TRUE;
	}
#endif

#ifdef USE_TOOLBAR
	if (using_toolbar != prev_toolbar)
	{
	    gui_mch_show_toolbar(using_toolbar);
	    prev_toolbar = using_toolbar;
	    need_winsize = TRUE;
	}
#endif
#if defined(WANT_MENU) && !defined(WIN16)
	if (using_tearoff != prev_tearoff)
	{
	    gui_mch_toggle_tearoffs(using_tearoff);
	    prev_tearoff = using_tearoff;
	}
#endif
	if (need_winsize)
	    gui_set_winsize(FALSE);
    }
}


/*
 * Scrollbar stuff:
 */

    void
gui_create_scrollbar(sb, wp)
    GuiScrollbar    *sb;
    WIN		    *wp;
{
    static int	sbar_ident = 0;
    int	    which;

    sb->ident = sbar_ident++;	/* No check for too big, but would it happen? */
    sb->wp = wp;
    sb->value = 0;
    sb->pixval = 0;
    sb->size = 1;
    sb->max = 1;
    sb->top = 1;
    sb->height = 0;
    sb->status_height = 0;
    gui_mch_create_scrollbar(sb, (wp == NULL) ? SBAR_HORIZ : SBAR_VERT);
    if (wp != NULL)
    {
	which = (sb == &wp->w_scrollbars[SBAR_LEFT]) ? SBAR_LEFT : SBAR_RIGHT;
	gui_mch_enable_scrollbar(sb, gui.which_scrollbars[which]);
    }
}

/*
 * Find the scrollbar with the given index.
 */
    GuiScrollbar *
gui_find_scrollbar(ident)
    long	ident;
{
    WIN		    *wp;

    if (gui.bottom_sbar.ident == ident)
	return &gui.bottom_sbar;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	if (wp->w_scrollbars[SBAR_LEFT].ident == ident)
	    return &wp->w_scrollbars[SBAR_LEFT];
	if (wp->w_scrollbars[SBAR_RIGHT].ident == ident)
	    return &wp->w_scrollbars[SBAR_RIGHT];
    }
    return NULL;
}

/*
 * For most systems: Put a code in the input buffer for a dragged scrollbar.
 *
 * For Win32 and Macintosh:
 * Scrollbars seem to grab focus and vim doesn't read the input queue until
 * you stop dragging the scrollbar.  We get here each time the scrollbar is
 * dragged another pixel, but as far as the rest of vim goes, it thinks
 * we're just hanging in the call to DispatchMessage() in
 * process_message().  The DispatchMessage() call that hangs was passed a
 * mouse button click event in the scrollbar window. -- webb.
 *
 * Solution: Do the scrolling right here.  But only when allowed.
 * Ignore the scrollbars while executing an external command or when there
 * are still characters to be processed.
 */
    void
gui_drag_scrollbar(sb, value, still_dragging)
    GuiScrollbar    *sb;
    long	    value;
    int		    still_dragging;
{
    WIN		*wp;
    int		sb_num;
#ifdef USE_ON_FLY_SCROLL
    colnr_t	old_leftcol = curwin->w_leftcol;
    linenr_t	old_topline = curwin->w_topline;
#else
    char_u	bytes[4 + sizeof(long_u)];
    int		byte_count;
#endif

    if (sb == NULL)
	return;

    /* Don't put events in the input queue now. */
    if (hold_gui_events)
	return;

    if (still_dragging)
    {
	if (sb->wp == NULL)
	    gui.dragged_sb = SBAR_BOTTOM;
	else if (sb == &sb->wp->w_scrollbars[SBAR_LEFT])
	    gui.dragged_sb = SBAR_LEFT;
	else
	    gui.dragged_sb = SBAR_RIGHT;
	gui.dragged_wp = sb->wp;
    }
    else
	gui.dragged_sb = SBAR_NONE;

    /* Vertical sbar info is kept in the first sbar (the left one) */
    if (sb->wp != NULL)
	sb = &sb->wp->w_scrollbars[0];

    /*
     * Check validity of value
     */
    if (value < 0)
	value = 0;
#ifdef SCROLL_PAST_END
    else if (value > sb->max)
	value = sb->max;
#else
    if (value > sb->max - sb->size + 1)
	value = sb->max - sb->size + 1;
#endif

    sb->value = value;

#ifdef USE_ON_FLY_SCROLL
    /* When not allowed to do the scrolling right now, return. */
    if (dont_scroll || !vim_is_input_buf_empty())
	return;
#endif

#ifdef RIGHTLEFT
    if (sb->wp == NULL && curwin->w_p_rl)
    {
	value = sb->max + 1 - sb->size - value;
	if (value < 0)
	    value = 0;
    }
#endif

    if (sb->wp != NULL)		/* vertical scrollbar */
    {
	sb_num = 0;
	for (wp = firstwin; wp != sb->wp && wp != NULL; wp = wp->w_next)
	    sb_num++;

	if (wp == NULL)
	    return;

#ifdef USE_ON_FLY_SCROLL
	current_scrollbar = sb_num;
	scrollbar_value = value;
	if (State & NORMAL)
	{
	    gui_do_scroll();
	    setcursor();
	}
	else if (State & INSERT)
	{
	    ins_scroll();
	    setcursor();
	}
	else if (State & CMDLINE)
	{
	    if (!msg_scrolled)
	    {
		gui_do_scroll();
		redrawcmdline();
	    }
	}
#else
	bytes[0] = CSI;
	bytes[1] = KS_SCROLLBAR;
	bytes[2] = K_FILLER;
	bytes[3] = (char_u)sb_num;
	byte_count = 4;
#endif
    }
    else
    {
#ifdef USE_ON_FLY_SCROLL
	scrollbar_value = value;

	if (State & NORMAL)
	    gui_do_horiz_scroll();
	else if (State & INSERT)
	    ins_horscroll();
	else if (State & CMDLINE)
	{
	    if (!msg_scrolled)
	    {
		gui_do_horiz_scroll();
		redrawcmdline();
	    }
	}
	if (old_leftcol != curwin->w_leftcol)
	{
	    updateWindow(curwin);   /* update window, status and cmdline */
	    setcursor();
	}
#else
	bytes[0] = CSI;
	bytes[1] = KS_HORIZ_SCROLLBAR;
	bytes[2] = K_FILLER;
	byte_count = 3;
#endif
    }

#ifdef USE_ON_FLY_SCROLL
# ifdef SCROLLBIND
    /*
     * synchronize other windows, as necessary according to 'scrollbind'
     */
    if (curwin->w_p_scb
	    && ((sb->wp == NULL && curwin->w_leftcol != old_leftcol)
		|| (sb->wp == curwin && curwin->w_topline != old_topline)))
    {
	do_check_scrollbind(TRUE);
	if (sb->wp == NULL)
	    setcursor();
    }
# endif
    out_flush();
#else
    add_long_to_buf((long)value, bytes + byte_count);
    add_to_input_buf(bytes, byte_count + sizeof(long_u));
#endif
}

/*
 * Scrollbar stuff:
 */

    static void
gui_update_scrollbars(force)
    int		    force;	    /* Force all scrollbars to get updated */
{
    WIN		    *wp;
    GuiScrollbar    *sb;
    int		    val, size, max;
    int		    which_sb;
    int		    h, y;

    /* Update the horizontal scrollbar */
    gui_update_horiz_scrollbar(force);

    /* Return straight away if there is neither a left nor right scrollbar */
    if (!gui.which_scrollbars[SBAR_LEFT] && !gui.which_scrollbars[SBAR_RIGHT])
	return;

    /*
     * Don't want to update a scrollbar while we're dragging it.  But if we
     * have both a left and right scrollbar, and we drag one of them, we still
     * need to update the other one.
     */
    if (       (gui.dragged_sb == SBAR_LEFT
		|| gui.dragged_sb == SBAR_RIGHT)
	    && (!gui.which_scrollbars[SBAR_LEFT]
		|| !gui.which_scrollbars[SBAR_RIGHT])
	    && !force)
	return;

    if (!force && (gui.dragged_sb == SBAR_LEFT || gui.dragged_sb == SBAR_RIGHT))
    {
	/*
	 * If we have two scrollbars and one of them is being dragged, just
	 * copy the scrollbar position from the dragged one to the other one.
	 */
	which_sb = SBAR_LEFT + SBAR_RIGHT - gui.dragged_sb;
	if (gui.dragged_wp != NULL)
	    gui_mch_set_scrollbar_thumb(
		    &gui.dragged_wp->w_scrollbars[which_sb],
		    gui.dragged_wp->w_scrollbars[0].value,
		    gui.dragged_wp->w_scrollbars[0].size,
		    gui.dragged_wp->w_scrollbars[0].max);
	return;
    }

    /* avoid that moving components around generates events */
    ++hold_gui_events;

    for (wp = firstwin; wp; wp = wp->w_next)
    {
	if (wp->w_buffer == NULL)	/* just in case */
	    continue;
#ifdef SCROLL_PAST_END
	max = wp->w_buffer->b_ml.ml_line_count - 1;
#else
	max = wp->w_buffer->b_ml.ml_line_count + wp->w_height - 2;
#endif
	if (max < 0)			/* empty buffer */
	    max = 0;
	val = wp->w_topline - 1;
	size = wp->w_height;
#ifdef SCROLL_PAST_END
	if (val > max)			/* just in case */
	    val = max;
#else
	if (size > max + 1)		/* just in case */
	    size = max + 1;
	if (val > max - size + 1)
	    val = max - size + 1;
#endif
	if (val < 0)			/* minimal value is 0 */
	    val = 0;

	/*
	 * Scrollbar at index 0 (the left one) contains all the information.
	 * It would be the same info for left and right so we just store it for
	 * one of them.
	 */
	sb = &wp->w_scrollbars[0];

	/*
	 * Note: no check for valid w_botline.	If it's not valid the
	 * scrollbars will be updated later anyway.
	 */
	if (size < 1 || wp->w_botline - 2 > max)
	{
	    /*
	     * This can happen during changing files.  Just don't update the
	     * scrollbar for now.
	     */
	    sb->height = 0;	    /* Force update next time */
	    if (gui.which_scrollbars[SBAR_LEFT])
		gui_mch_enable_scrollbar(&wp->w_scrollbars[SBAR_LEFT], FALSE);
	    if (gui.which_scrollbars[SBAR_RIGHT])
		gui_mch_enable_scrollbar(&wp->w_scrollbars[SBAR_RIGHT], FALSE);
	    continue;
	}
	if (force || sb->height != wp->w_height
	    || sb->top != wp->w_winpos
	    || sb->status_height != wp->w_status_height)
	{
	    /* Height or position of scrollbar has changed */
	    sb->top = wp->w_winpos;
	    sb->height = wp->w_height;
	    sb->status_height = wp->w_status_height;

	    /* Calculate height and position in pixels */
	    h = (sb->height + sb->status_height) * gui.char_height;
	    y = sb->top * gui.char_height + gui.border_offset;
#if defined(WANT_MENU) && !defined(USE_GUI_GTK)
	    if (gui.menu_is_active)
		y += gui.menu_height;
#endif

#if defined(USE_GUI_MSWIN) && defined(USE_TOOLBAR)
	    if (vim_strchr(p_go, GO_TOOLBAR) != NULL)
		y += TOOLBAR_BUTTON_HEIGHT + TOOLBAR_BORDER_HEIGHT;
#endif

	    if (wp == firstwin)
	    {
		/* Height of top scrollbar includes width of top border */
		h += gui.border_offset;
		y -= gui.border_offset;
	    }
	    if (gui.which_scrollbars[SBAR_LEFT])
	    {
		gui_mch_set_scrollbar_pos(&wp->w_scrollbars[SBAR_LEFT],
					  gui.left_sbar_x, y,
					  gui.scrollbar_width, h);
		gui_mch_enable_scrollbar(&wp->w_scrollbars[SBAR_LEFT], TRUE);
	    }
	    if (gui.which_scrollbars[SBAR_RIGHT])
	    {
		gui_mch_set_scrollbar_pos(&wp->w_scrollbars[SBAR_RIGHT],
					  gui.right_sbar_x, y,
					  gui.scrollbar_width, h);
		gui_mch_enable_scrollbar(&wp->w_scrollbars[SBAR_RIGHT], TRUE);
	    }
	}

	/* reduce the number of calls to gui_mch_set_scrollbar_thumb() by
	 * checking if the thumb moved at least a pixel */
	if (max == 0)
	    y = 0;
	else
	    y = (val * (sb->height + 2) * gui.char_height + max / 2) / max;
#ifndef RISCOS
	/* RISCOS does scrollbars differently - if the position through the
	 * file has changed then we must update the scrollbar regardless of
	 * how far we think it has moved. */
	if (force || sb->pixval != y || sb->size != size || sb->max != max)
#endif
	{
	    /* Thumb of scrollbar has moved */
	    sb->value = val;
	    sb->pixval = y;
	    sb->size = size;
	    sb->max = max;
	    if (gui.which_scrollbars[SBAR_LEFT] && gui.dragged_sb != SBAR_LEFT)
		gui_mch_set_scrollbar_thumb(&wp->w_scrollbars[SBAR_LEFT],
					    val, size, max);
	    if (gui.which_scrollbars[SBAR_RIGHT]
					&& gui.dragged_sb != SBAR_RIGHT)
		gui_mch_set_scrollbar_thumb(&wp->w_scrollbars[SBAR_RIGHT],
					    val, size, max);
	}
    }
    --hold_gui_events;
}

/*
 * Scroll a window according to the values set in the globals current_scrollbar
 * and scrollbar_value.  Return TRUE if the cursor in the current window moved
 * or FALSE otherwise.
 */
    int
gui_do_scroll()
{
    WIN		*wp, *old_wp;
    int		i;
    long	nlines;
    FPOS	old_cursor;
    linenr_t	old_topline;

    for (wp = firstwin, i = 0; i < current_scrollbar; i++)
    {
	if (wp == NULL)
	    break;
	wp = wp->w_next;
    }
    if (wp == NULL)
    {
	/* Couldn't find window */
	return FALSE;
    }

    /*
     * Compute number of lines to scroll.  If zero, nothing to do.
     */
    nlines = (long)scrollbar_value + 1 - (long)wp->w_topline;
    if (nlines == 0)
	return FALSE;

    old_cursor = curwin->w_cursor;
    old_wp = curwin;
    old_topline = wp->w_topline;
    curwin = wp;
    curbuf = wp->w_buffer;
    if (nlines < 0)
	scrolldown(-nlines);
    else
	scrollup(nlines);
    if (old_topline != wp->w_topline)
    {
	if (p_so)
	{
	    cursor_correct();		/* fix window for 'so' */
	    update_topline();		/* avoid up/down jump */
	}
	coladvance(curwin->w_curswant);
#ifdef SCROLLBIND
	curwin->w_scbind_pos = curwin->w_topline;
#endif
    }

    curwin = old_wp;
    curbuf = old_wp->w_buffer;

    /*
     * Don't call updateWindow() when nothing has changed (it will overwrite
     * the status line!).
     */
    if (old_topline != wp->w_topline)
    {
	wp->w_redr_type = VALID;
	updateWindow(wp);   /* update window, status line, and cmdline */
    }

    return !equal(curwin->w_cursor, old_cursor);
}


/*
 * Horizontal scrollbar stuff:
 */

    static void
gui_update_horiz_scrollbar(force)
    int	    force;
{
    int	    value, size, max;
    char_u  *p;

    if (!gui.which_scrollbars[SBAR_BOTTOM])
	return;

    if (!force && gui.dragged_sb == SBAR_BOTTOM)
	return;

    if (!force && curwin->w_p_wrap && gui.prev_wrap)
	return;

    /*
     * It is possible for the cursor to be invalid if we're in the middle of
     * something (like changing files).  If so, don't do anything for now.
     */
    if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
    {
	gui.bottom_sbar.value = -1;
	return;
    }

    size = Columns;
    if (curwin->w_p_wrap)
    {
	value = 0;
#ifdef SCROLL_PAST_END
	max = 0;
#else
	max = Columns - 1;
#endif
    }
    else
    {
	value = curwin->w_leftcol;

	/* Calculate max for horizontal scrollbar */
	p = ml_get_curline();
	max = 0;
	if (p != NULL && p[0] != NUL)
	    while (p[1] != NUL)		    /* Don't count last character */
		max += chartabsize(*p++, (colnr_t)max);
#ifndef SCROLL_PAST_END
	max += Columns - 1;
#endif
	/* The line number isn't scrolled, thus there is less space when
	 * 'number' is set. */
	if (curwin->w_p_nu)
	{
	    size -= 8;
#ifndef SCROLL_PAST_END
	    max -= 8;
#endif
	}
    }

#ifndef SCROLL_PAST_END
    if (value > max - size + 1)
	value = max - size + 1;	    /* limit the value to allowable range */
#endif

#ifdef RIGHTLEFT
    if (curwin->w_p_rl)
    {
	value = max + 1 - size - value;
	if (value < 0)
	{
	    size += value;
	    value = 0;
	}
    }
#endif
    if (!force && value == gui.bottom_sbar.value && size == gui.bottom_sbar.size
						&& max == gui.bottom_sbar.max)
	return;

    gui.bottom_sbar.value = value;
    gui.bottom_sbar.size = size;
    gui.bottom_sbar.max = max;
    gui.prev_wrap = curwin->w_p_wrap;

    gui_mch_set_scrollbar_thumb(&gui.bottom_sbar, value, size, max);
}

/*
 * Do a horizontal scroll.  Return TRUE if the cursor moved, FALSE otherwise.
 */
    int
gui_do_horiz_scroll()
{
    /* no wrapping, no scrolling */
    if (curwin->w_p_wrap)
	return FALSE;

    if (curwin->w_leftcol == scrollbar_value)
	return FALSE;

    curwin->w_leftcol = scrollbar_value;
    return leftcol_changed();
}

/*
 * Check that none of the colors are the same as the background color
 */
    void
gui_check_colors()
{
    if (gui.norm_pixel == gui.back_pixel || gui.norm_pixel == (GuiColor)-1)
    {
	gui_set_bg_color((char_u *)"White");
	if (gui.norm_pixel == gui.back_pixel || gui.norm_pixel == (GuiColor)-1)
	    gui_set_fg_color((char_u *)"Black");
    }
}

    void
gui_set_fg_color(name)
    char_u	*name;
{
    gui.norm_pixel = gui_mch_get_color(name);
    hl_set_fg_color_name(vim_strsave(name));
}

    void
gui_set_bg_color(name)
    char_u	*name;
{
    gui.back_pixel = gui_mch_get_color(name);
    hl_set_bg_color_name(vim_strsave(name));
}

#if defined(USE_GUI_X11) || defined(PROTO)
    void
gui_new_scrollbar_colors()
{
    WIN	    *wp;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	gui_mch_set_scrollbar_colors(&(wp->w_scrollbars[SBAR_LEFT]));
	gui_mch_set_scrollbar_colors(&(wp->w_scrollbars[SBAR_RIGHT]));
    }
    gui_mch_set_scrollbar_colors(&gui.bottom_sbar);
}
#endif

/*
 * Call this when focus has changed.
 */
    void
gui_focus_change(in_focus)
    int	    in_focus;
{
    gui.in_focus = in_focus;
    out_flush();		/* make sure output has been written */
    gui_update_cursor(TRUE, FALSE);

#ifdef AUTOCMD
    /*
     * Fire the focus gained/lost autocommand.
     * When something was executed, make sure the cursor it put back where it
     * belongs.
     */
    if (apply_autocmds(in_focus ? EVENT_FOCUSGAINED : EVENT_FOCUSLOST,
						   NULL, NULL, FALSE, curbuf))
    {
	if (State == CMDLINE)
	    redrawcmdline();
	else if (State == HITRETURN || State == SETWSIZE || State == ASKMORE
		|| State == EXTERNCMD || State == CONFIRM || exmode_active)
	    repeat_message();
	else if (((State & NORMAL) || (State & INSERT)) && redrawing())
	    setcursor();
	cursor_on();	    /* redrawing may have switched it off */
	out_flush();
    }
#endif
}

/*
 * Called when the mouse moved (but not when dragging).
 */
    void
gui_mouse_moved(y)
    int		y;
{
    WIN		*wp;
    char_u	st[6];
    int		x;

    /* Only handle this when 'mousefocus' set and ... */
    if (p_mousef
	    && !hold_gui_events		/* not holding events */
	    && (State & (NORMAL|INSERT))/* Normal/Visual/Insert mode */
	    && State != HITRETURN	/* but not hit-return prompt */
	    && msg_scrolled == 0	/* no scrolled message */
	    && !need_mouse_correct	/* not moving the pointer */
	    && gui.in_focus)		/* gvim in focus */
    {
	x = gui_mch_get_mouse_x();
	/* Don't move the mouse when it's left or right of the Vim window */
	if (x < 0 || x > Columns * gui.char_width)
	    return;
	wp = y2win(y);
	if (wp == curwin || wp == NULL)
	    return;	/* still in the same old window, or none at all */

	/*
	 * format a mouse click on status line input
	 * ala gui_send_mouse_event(0, x, y, 0, 0);
	 * Trick: Use a column of -1, so that check_termcode will generate a
	 * K_LEFTMOUSE_NM key code.
	 */
	if (finish_op)
	{
	    /* abort the current operator first */
	    st[0] = ESC;
	    add_to_input_buf(st, 1);
	}
	st[0] = CSI;
	st[1] = KS_MOUSE;
	st[2] = K_FILLER;
	st[3] = (char_u)MOUSE_LEFT;
	st[4] = (char_u)(' ');		/* column -1 */
	st[5] = (char_u)(wp->w_height + wp->w_winpos + ' ' + 1);
	add_to_input_buf(st, 6);
	st[3] = (char_u)MOUSE_RELEASE;
	add_to_input_buf(st, 6);

#ifdef USE_GUI_GTK
	/* Need to wake up the main loop */
	if (gtk_main_level() > 0)
	    gtk_main_quit();
#endif
    }
}

/*
 * Called when mouse should be moved to window with focus.
 */
    void
gui_mouse_correct()
{
    int		x, y;
    WIN		*wp = NULL;

    need_mouse_correct = FALSE;
    if (gui.in_use && p_mousef)
    {
	x = gui_mch_get_mouse_x();
	/* Don't move the mouse when it's left or right of the Vim window */
	if (x < 0 || x > Columns * gui.char_width)
	    return;
	y = gui_mch_get_mouse_y();
	if (y >= 0)
	    wp = y2win(y);
	if (wp != curwin && wp != NULL)	/* If in other than current window */
	{
	    validate_cline_row();
	    gui_mch_setmouse((int)Columns * gui.char_width - 3,
			 (curwin->w_winpos + curwin->w_wrow) * gui.char_height
						     + (gui.char_height) / 2);
	}
    }
}

/*
 * Find window where the mouse pointer "y" coordinate is in.
 */
    static WIN *
y2win(y)
    int		y;
{
    int		row;
    WIN		*wp;

    row = Y_2_ROW(y);
    if (row < firstwin->w_winpos)    /* before first window (Toolbar) */
	return NULL;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (row < wp->w_winpos + wp->w_height + wp->w_status_height)
	    break;
    return wp;
}
