/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * ex_getln.c: Functions for entering and editing an Ex command line.
 */

#include "vim.h"

/*
 * Variables shared between getcmdline(), redrawcmdline() and others.
 * These need to be saved when using CTRL-R |, that's why they are in a
 * structure.
 */
struct cmdline_info
{
    char_u	*cmdbuff;	/* pointer to command line buffer */
    int		cmdbufflen;	/* length of cmdbuff */
    int		cmdlen;		/* number of chars on command line */
    int		cmdpos;		/* current cursor position */
    int		cmdspos;	/* cursor column on screen */
    int		cmdfirstc;	/* ':', '/', '?', '=' or NUL */
    int		cmdindent;	/* number of spaces before cmdline */
    char_u	*cmdprompt;	/* message in front of cmdline */
    int		cmdattr;	/* attributes for prompt */
    int		overstrike;	/* Typing mode on the command line.  Shared by
				   getcmdline() and put_on_cmdline(). */
};

static struct cmdline_info ccline;	/* current cmdline_info */

static int	cmd_numfiles = -1;	/* number of files found by
						    file name completion */
static char_u	**cmd_files = NULL;	/* list of files */

struct hist_entry
{
    int		hisnum;		/* identifying number */
    char_u	*hisstr;	/* actual entry */
};

/*static char_u	**(history[HIST_COUNT]) = {NULL, NULL, NULL, NULL};*/
static struct hist_entry *(history[HIST_COUNT]) = {NULL, NULL, NULL, NULL};
static int	hisidx[HIST_COUNT] = {-1, -1, -1, -1};  /* last entered entry */
static int	hisnum[HIST_COUNT] = {0, 0, 0, 0};
		    /* identifying (unique) number of newest history entry */
static int	hislen = 0;		/* actual length of history tables */

#ifdef RIGHTLEFT
static int	cmd_hkmap = 0;	/* Hebrew mapping during command line */
#endif

#ifdef FKMAP
static int	cmd_fkmap = 0;	/* Farsi mapping during command line */
#endif

static int	hist_char2type __ARGS((int c));
static void	init_history __ARGS((void));

static int	in_history __ARGS((int, char_u *, int));
#ifdef WANT_EVAL
static int	calc_hist_idx __ARGS((int histype, int num));
#endif
static int	cmdline_charsize __ARGS((int idx));
static void	set_cmdspos __ARGS((void));
static void	set_cmdspos_cursor __ARGS((void));
static void	alloc_cmdbuff __ARGS((int len));
static int	realloc_cmdbuff __ARGS((int len));
static void	putcmdline __ARGS((int));
#ifdef WILDMENU
static void	cmdline_del __ARGS((int from));
#endif
static void	redrawcmdprompt __ARGS((void));
static void	redrawcmd __ARGS((void));
static void	cursorcmd __ARGS((void));
static int	ccheck_abbr __ARGS((int));
static int	nextwild __ARGS((int, int));
static int	showmatches __ARGS((int wildmenu));
static void	set_expand_context __ARGS((void));
static int	ExpandFromContext __ARGS((char_u *, int *, char_u ***, int, int));

/*
 * getcmdline() - accept a command line starting with firstc.
 *
 * firstc == ':'	    get ":" command line.
 * firstc == '/' or '?'	    get search pattern
 * firstc == '='	    get expression
 * firstc == '@'	    get text for input() function
 * firstc == NUL	    get text for :insert command
 * firstc == -1		    like NUL, and break on CTRL-C
 *
 * The line is collected in ccline.cmdbuff, which is reallocated to fit the
 * command line.
 *
 * Careful: getcmdline() can be called recursively!
 *
 * Return pointer to allocated string if there is a commandline, NULL
 * otherwise.
 */
/*ARGSUSED*/
    char_u *
getcmdline(firstc, count, indent)
    int		firstc;
    long	count;		/* only used for incremental search */
    int		indent;		/* indent for inside conditionals */
{
    int		c;
#ifdef DIGRAPHS
    int		cc;
#endif
    int		i;
    int		j;
    char_u	*p;
    int		hiscnt;			/* current history line in use */
    char_u	*lookfor = NULL;	/* string to match */
    int		gotesc = FALSE;		/* TRUE when <ESC> just typed */
    int		do_abbr;		/* when TRUE check for abbr. */
    int		histype;		/* history type to be used */
#ifdef EXTRA_SEARCH
    FPOS	old_cursor;
    colnr_t	old_curswant;
    colnr_t	old_leftcol;
    linenr_t	old_topline;
    linenr_t	old_botline;
    int		did_incsearch = FALSE;
    int		incsearch_postponed = FALSE;
#endif
    int		did_wild_list = FALSE;	/* did wild_list() recently */
    int		wim_index = 0;		/* index in wim_flags[] */
    int		res;
    int		save_msg_scroll = msg_scroll;
    int		save_State = State;	/* remember State when called */
    int		some_key_typed = FALSE;	/* one of the keys was typed */
#ifdef USE_MOUSE
    /* mouse drag and release events are ignored, unless they are
     * preceded with a mouse down event */
    int		ignore_drag_release = TRUE;
#endif
#ifdef WANT_EVAL
    int		break_ctrl_c = FALSE;
#endif

#ifdef USE_SNIFF
    want_sniff_request = 0;
#endif
#ifdef WANT_EVAL
    if (firstc == -1)
    {
	firstc = NUL;
	break_ctrl_c = TRUE;
    }
#endif
#ifdef RIGHTLEFT
    /* start without Hebrew mapping for a command line */
    if (firstc == ':' || firstc == '=')
	cmd_hkmap = 0;
#endif

    ccline.overstrike = FALSE;		    /* always start in insert mode */
#ifdef EXTRA_SEARCH
    old_cursor = curwin->w_cursor;	    /* needs to be restored later */
    old_curswant = curwin->w_curswant;
    old_leftcol = curwin->w_leftcol;
    old_topline = curwin->w_topline;
    old_botline = curwin->w_botline;
#endif

    /*
     * set some variables for redrawcmd()
     */
    ccline.cmdfirstc = (firstc == '@' ? 0 : firstc);
    ccline.cmdindent = indent;
    alloc_cmdbuff(exmode_active ? 250 : 0); /* alloc initial ccline.cmdbuff */
    if (ccline.cmdbuff == NULL)
	return NULL;			    /* out of memory */
    ccline.cmdlen = ccline.cmdpos = 0;

    redir_off = TRUE;		/* don't redirect the typed command */
    i = msg_scrolled;
    msg_scrolled = 0;		/* avoid wait_return message */
    gotocmdline(TRUE);
    msg_scrolled += i;
    redrawcmdprompt();		/* draw prompt or indent */
    set_cmdspos();
    expand_context = EXPAND_NOTHING;

    /*
     * Avoid scrolling when called by a recursive do_cmdline(), e.g. when doing
     * ":@0" when register 0 doesn't contain a CR.
     */
    msg_scroll = FALSE;

    State = CMDLINE;
#ifdef USE_MOUSE
    setmouse();
#endif

    init_history();
    hiscnt = hislen;		/* set hiscnt to impossible history value */
    histype = hist_char2type(firstc);

#ifdef DIGRAPHS
    do_digraph(-1);		/* init digraph typahead */
#endif

    /* collect the command string, handling editing keys */
    for (;;)
    {
#ifdef USE_ON_FLY_SCROLL
	dont_scroll = FALSE;	/* allow scrolling here */
#endif
	quit_more = FALSE;	/* reset after CTRL-D which had a more-prompt */

	cursorcmd();		/* set the cursor on the right spot */
	c = safe_vgetc();
	if (KeyTyped)
	{
	    some_key_typed = TRUE;
#ifdef RIGHTLEFT
	    if (cmd_hkmap)
		c = hkmap(c);
# ifdef FKMAP
	    if (cmd_fkmap)
		c = cmdl_fkmap(c);
# endif
#endif
	}

	/*
	 * Ignore got_int when CTRL-C was typed here.
	 * Don't ignore it in :global, we really need to break then, e.g., for
	 * ":g/pat/normal /pat" (without the <CR>).
	 * Don't ignore it for the input() function.
	 */
	if ((c == Ctrl('C')
#ifdef UNIX
		|| c == intr_char
#endif
				)
#if defined(WANT_EVAL) || defined(CRYPTV)
		&& firstc != '@'
#endif
#ifdef WANT_EVAL
		&& !break_ctrl_c
#endif
		&& !global_busy)
	    got_int = FALSE;

	/* free old command line when finished moving around in the history
	 * list */
	if (lookfor
		&& c != K_S_DOWN && c != K_S_UP && c != K_DOWN && c != K_UP
		&& c != K_PAGEDOWN && c != K_PAGEUP
		&& c != K_KPAGEDOWN && c != K_KPAGEUP
		&& c != K_LEFT && c != K_RIGHT
		&& (cmd_numfiles > 0 || (c != Ctrl('P') && c != Ctrl('N'))))
	{
	    vim_free(lookfor);
	    lookfor = NULL;
	}

	/*
	 * <S-Tab> works like CTRL-P (unless 'wc' is <S-Tab>).
	 */
	if (c != p_wc && c == K_S_TAB && cmd_numfiles != -1)
	    c = Ctrl('P');

#ifdef WILDMENU
	/* Special translations for 'wildmenu' */
	if (did_wild_list && p_wmnu)
	{
	    if (c == K_LEFT)
		c = Ctrl('P');
	    else if (c == K_RIGHT)
		c = Ctrl('N');
	}
	/* Hitting CR after "emenu Name.": complete submenu */
	if (expand_context == EXPAND_MENUNAMES && p_wmnu
		&& ccline.cmdpos > 1
		&& ccline.cmdbuff[ccline.cmdpos - 1] == '.'
		&& ccline.cmdbuff[ccline.cmdpos - 2] != '\\'
		&& (c == '\n' || c == '\r' || c == K_KENTER))
	    c = K_DOWN;
#endif

	/* free expanded names when finished walking through matches */
	if (cmd_numfiles != -1 && !(c == p_wc && KeyTyped)
		&& c != p_wcm
		&& c != Ctrl('N') && c != Ctrl('P') && c != Ctrl('A')
		&& c != Ctrl('L'))
	{
	    (void)ExpandOne(NULL, NULL, 0, WILD_FREE);
	    did_wild_list = FALSE;
#ifdef WILDMENU
	    if (!p_wmnu || (c != K_UP && c != K_DOWN))
#endif
		expand_context = EXPAND_NOTHING;
	    wim_index = 0;
#ifdef WILDMENU
	    if (p_wmnu && wild_menu_showing)
	    {
		int skt = KeyTyped;

		if (wild_menu_showing == WM_SCROLLED)
		{
		    /* Entered command line, move it up */
		    cmdline_row--;
		    redrawcmd();
		}
		else if (save_p_ls != -1)
		{
		    /* restore 'laststatus' if it was changed */
		    p_ls = save_p_ls;
		    last_status();
		    update_screen(VALID);	/* redraw the screen NOW */
		    redrawcmd();
		    save_p_ls = -1;
		}
		else
		    win_redr_status(lastwin);
		KeyTyped = skt;
		wild_menu_showing = 0;
	    }
#endif
	}

#ifdef WILDMENU
	/* Special translations for 'wildmenu' */
	if (expand_context == EXPAND_MENUNAMES && p_wmnu)
	{
	    /* Hitting <Down> after "emenu Name.": complete submenu */
	    if (ccline.cmdbuff[ccline.cmdpos - 1] == '.' && c == K_DOWN)
		c = p_wc;
	    else if (c == K_UP)
	    {
		/* Hitting <Up>: Remove one submenu name in front of the
		 * cursor */
		int found = FALSE;

		j = expand_pattern - ccline.cmdbuff;
		i = 0;
		while (--j > 0)
		{
		    /* check for start of menu name */
		    if (ccline.cmdbuff[j] == ' '
			    && ccline.cmdbuff[j - 1] != '\\')
		    {
			i = j + 1;
			break;
		    }
		    /* check for start of submenu name */
		    if (ccline.cmdbuff[j] == '.'
			    && ccline.cmdbuff[j - 1] != '\\')
		    {
			if (found)
			{
			    i = j + 1;
			    break;
			}
			else
			    found = TRUE;
		    }
		}
		if (i > 0)
		    cmdline_del(i);
		c = p_wc;
		expand_context = EXPAND_NOTHING;
	    }
	}
	if (expand_context == EXPAND_FILES && p_wmnu)
	{
	    char_u upseg[5];

	    upseg[0] = PATHSEP;
	    upseg[1] = '.';
	    upseg[2] = '.';
	    upseg[3] = PATHSEP;
	    upseg[4] = NUL;

	    if (ccline.cmdbuff[ccline.cmdpos - 1] == PATHSEP
		    && c == K_DOWN
		    && (ccline.cmdbuff[ccline.cmdpos - 2] != '.'
		        || ccline.cmdbuff[ccline.cmdpos - 3] != '.'))
	    {
		/* go down a directory */
		c = p_wc;
	    }
	    else if (STRNCMP(expand_pattern, upseg + 1, 3) == 0 && c == K_DOWN)
	    {
		/* If in a direct ancestor, strip off one ../ to go down */
		int found = FALSE;

		j = ccline.cmdpos;
		i = expand_pattern - ccline.cmdbuff;
		while (--j > i)
		{
		    if (vim_ispathsep(ccline.cmdbuff[j]))
		    {
			found = TRUE;
			break;
		    }
		}
		if (found
			&& ccline.cmdbuff[j - 1] == '.'
			&& ccline.cmdbuff[j - 2] == '.'
			&& (vim_ispathsep(ccline.cmdbuff[j - 3]) || j == i + 2))
		{
		    cmdline_del(j - 2);
		    c = p_wc;
		}
	    }
	    else if (c == K_UP)
	    {
		/* go up a directory */
		int found = FALSE;

		j = ccline.cmdpos - 1;
		i = expand_pattern - ccline.cmdbuff;
		while (--j > i)
		{
		    if (vim_ispathsep(ccline.cmdbuff[j])
#ifdef BACKSLASH_IN_FILENAME
			    && vim_strchr(" *?[{`$%#", ccline.cmdbuff[j + 1])
			       == NULL
#endif
		       )
		    {
			if (found)
			{
			    i = j + 1;
			    break;
			}
			else
			    found = TRUE;
		    }
		}

		if (!found)
		    j = i;
		else if (STRNCMP(ccline.cmdbuff + j, upseg, 4) == 0)
		    j += 4;
		else if (STRNCMP(ccline.cmdbuff + j, upseg + 1, 3) == 0
			     && j == i)
		    j += 3;
		else
		    j = 0;
		if (j > 0)
		{
		    /* TODO this is only for DOS/UNIX systems - need to put in
		     * machine-specific stuff here and in upseg init */
		    cmdline_del(j);
		    put_on_cmdline(upseg + 1, 3, FALSE);
		}
		else if (ccline.cmdpos > i)
		    cmdline_del(i);
		c = p_wc;
	    }
	}
#if 0 /* If enabled <Down> on a file takes you _completely_ out of wildmenu */
	if (p_wmnu
		&& (expand_context == EXPAND_FILES
		    || expand_context == EXPAND_MENUNAMES)
		&& (c == K_UP || c == K_DOWN))
	    expand_context = EXPAND_NOTHING;
#endif
#endif

	/* CTRL-\ CTRL-N goes to Normal mode */
	if (c == Ctrl('\\'))
	{
	    c = safe_vgetc();
	    if (c != Ctrl('N'))
	    {
		vungetc(c);
		c = Ctrl('\\');
	    }
	    else
	    {
		gotesc = TRUE;	/* will free ccline.cmdbuff after putting it
				   in history */
		goto returncmd;	/* back to Normal mode */
	    }
	}

#ifdef DIGRAPHS
	c = do_digraph(c);
#endif

	if (c == '\n' || c == '\r' || c == K_KENTER || (c == ESC
			&& (!KeyTyped || vim_strchr(p_cpo, CPO_ESC) != NULL)))
	{
	    gotesc = FALSE;	/* Might have typed ESC previously, don't
				   truncate the cmdline now. */
	    if (ccheck_abbr(c + ABBR_OFF))
		goto cmdline_changed;
	    windgoto(msg_row, 0);
	    out_flush();
	    break;
	}

	/*
	 * Completion for 'wildchar' key.
	 * - hitting <ESC> twice means: abandon command line.
	 * - wildcard expansion is only done when the key is really typed, not
	 *   when it comes from a macro
	 */
	if ((c == p_wc && !gotesc && KeyTyped) || c == p_wcm)
	{
	    if (cmd_numfiles > 0)   /* typed p_wc at least twice */
	    {
		/* if 'wildmode' contains "list" may still need to list */
		if (cmd_numfiles > 1
			&& !did_wild_list
			&& (wim_flags[wim_index] & WIM_LIST))
		{
		    showmatches(FALSE);
		    redrawcmd();
		    did_wild_list = TRUE;
		}
		if (wim_flags[wim_index] & WIM_LONGEST)
		    res = nextwild(WILD_LONGEST, WILD_NO_BEEP);
		else if (wim_flags[wim_index] & WIM_FULL)
		    res = nextwild(WILD_NEXT, WILD_NO_BEEP);
		else
		    res = OK;	    /* don't insert 'wildchar' now */
	    }
	    else		    /* typed p_wc first time */
	    {
		wim_index = 0;
		j = ccline.cmdpos;
		/* if 'wildmode' first contains "longest", get longest
		 * common part */
		if (wim_flags[0] & WIM_LONGEST)
		    res = nextwild(WILD_LONGEST, WILD_NO_BEEP);
		else
		    res = nextwild(WILD_EXPAND_KEEP, WILD_NO_BEEP);

		/* if interrupted while completing, behave like it failed */
		if (got_int)
		{
		    (void)vpeekc();	/* remove <C-C> from input stream */
		    got_int = FALSE;	/* don't abandon the command line */
		    (void)ExpandOne(NULL, NULL, 0, WILD_FREE);
#ifdef WILDMENU
		    expand_context = EXPAND_NOTHING;
#endif
		    goto cmdline_changed;
		}

		/* when more than one match, and 'wildmode' first contains
		 * "list", or no change and 'wildmode' contains "longest,list",
		 * list all matches */
		if (res == OK && cmd_numfiles > 1)
		{
		    /* a "longest" that didn't do anything is skipped (but not
		     * "list:longest") */
		    if (wim_flags[0] == WIM_LONGEST && ccline.cmdpos == j)
			wim_index = 1;
		    if ((wim_flags[wim_index] & WIM_LIST)
#ifdef WILDMENU
			    || (p_wmnu && (wim_flags[wim_index] & WIM_FULL) != 0)
#endif
			    )
		    {
			if (!(wim_flags[0] & WIM_LONGEST))
			{
#ifdef WILDMENU
			    int p_wmnu_save = p_wmnu;
			    p_wmnu = 0;
#endif
			    nextwild(WILD_PREV, 0);	/* remove match */
#ifdef WILDMENU
			    p_wmnu = p_wmnu_save;
#endif
			}
#ifdef WILDMENU
			showmatches(p_wmnu && ((wim_flags[wim_index] & WIM_LIST) == 0));
#else
			showmatches(FALSE);
#endif
			redrawcmd();
			did_wild_list = TRUE;
			if (wim_flags[wim_index] & WIM_LONGEST)
			    nextwild(WILD_LONGEST, WILD_NO_BEEP);
			else if (wim_flags[wim_index] & WIM_FULL)
			    nextwild(WILD_NEXT, WILD_NO_BEEP);
		    }
		    else
			vim_beep();
		}
#ifdef WILDMENU
		else if (cmd_numfiles == -1)
		    expand_context = EXPAND_NOTHING;
#endif
	    }
	    if (wim_index < 3)
		++wim_index;
	    if (c == ESC)
		gotesc = TRUE;
	    if (res == OK)
		goto cmdline_changed;
	}
	gotesc = FALSE;

	/* <S-Tab> goes to last match, in a clumsy way */
	if (c == K_S_TAB && KeyTyped)
	{
	    if (nextwild(WILD_EXPAND_KEEP, 0) == OK
		    && nextwild(WILD_PREV, 0) == OK
		    && nextwild(WILD_PREV, 0) == OK)
		goto cmdline_changed;
	}

	if (c == NUL || c == K_ZERO)	    /* NUL is stored as NL */
	    c = NL;

	do_abbr = TRUE;		/* default: check for abbreviation */

	/*
	 * Big switch for a typed command line character.
	 */
	switch (c)
	{
	case K_BS:
	case Ctrl('H'):
	case K_DEL:
	case K_KDEL:
	case Ctrl('W'):
#ifdef FKMAP
		if (cmd_fkmap && c == K_BS)
		    c = K_DEL;
#endif
		if (c == K_KDEL)
		    c = K_DEL;

		/*
		 * delete current character is the same as backspace on next
		 * character, except at end of line
		 */
		if (c == K_DEL && ccline.cmdpos != ccline.cmdlen)
		    ++ccline.cmdpos;
#ifdef MULTI_BYTE
		if (is_dbcs && c == K_DEL
		    && mb_isbyte2(ccline.cmdbuff, ccline.cmdpos))
		    ++ccline.cmdpos;
#endif
		if (ccline.cmdpos > 0)
		{
		    j = ccline.cmdpos;
		    if (c == Ctrl('W'))
		    {
			while (ccline.cmdpos &&
			       vim_isspace(ccline.cmdbuff[ccline.cmdpos - 1]))
			    --ccline.cmdpos;
			i = vim_iswordc(ccline.cmdbuff[ccline.cmdpos - 1]);
			while (ccline.cmdpos && !vim_isspace(
					 ccline.cmdbuff[ccline.cmdpos - 1]) &&
				vim_iswordc(
				      ccline.cmdbuff[ccline.cmdpos - 1]) == i)
			    --ccline.cmdpos;
		    }
		    else
			--ccline.cmdpos;
#ifdef MULTI_BYTE
		    if (c != Ctrl('W'))
		    {
			if (is_dbcs
			    && mb_isbyte2(ccline.cmdbuff, ccline.cmdpos))
			    --ccline.cmdpos;
		    }
#endif
		    ccline.cmdlen -= j - ccline.cmdpos;
		    i = ccline.cmdpos;
		    while (i < ccline.cmdlen)
			ccline.cmdbuff[i++] = ccline.cmdbuff[j++];
		    redrawcmd();
		}
		else if (ccline.cmdlen == 0 && c != Ctrl('W')
				   && ccline.cmdprompt == NULL && indent == 0)
		{
		    vim_free(ccline.cmdbuff);	/* no commandline to return */
		    ccline.cmdbuff = NULL;
		    msg_col = 0;
		    msg_putchar(' ');		/* delete ':' */
		    redraw_cmdline = TRUE;
		    goto returncmd;		/* back to cmd mode */
		}
		goto cmdline_changed;

	case K_INS:
	case K_KINS:
#ifdef FKMAP
		/* if Farsi mode set, we are in reverse insert mode -
		   Do not change the mode */
		if (cmd_fkmap)
		    beep_flush();
		else
#endif
		ccline.overstrike = !ccline.overstrike;
#ifdef CURSOR_SHAPE
		ui_cursor_shape();	/* may show different cursor shape */
#endif
		goto cmdline_not_changed;

/*	case '@':   only in very old vi */
	case Ctrl('U'):
		ccline.cmdpos = 0;
		ccline.cmdlen = 0;
		redrawcmd();
		goto cmdline_changed;

	case ESC:	/* get here if p_wc != ESC or when ESC typed twice */
	case Ctrl('C'):
		gotesc = TRUE;	    /* will free ccline.cmdbuff after putting
				       it in history */
		goto returncmd;	    /* back to cmd mode */

	case Ctrl('R'):		    /* insert register */
#ifdef USE_ON_FLY_SCROLL
		dont_scroll = TRUE; /* disallow scrolling here */
#endif
		putcmdline('"');
		++no_mapping;
		i = c = safe_vgetc(); /* CTRL-R <char> */
		if (c == Ctrl('R'))
		    c = safe_vgetc(); /* CTRL-R CTRL-R <char> */
		--no_mapping;
#ifdef WANT_EVAL
		/*
		 * Insert the result of an expression.
		 * Need to save the current command line, to be able to enter
		 * a new one...
		 */
		if (c == '=')
		{
		    struct cmdline_info	    save_ccline;

		    if (ccline.cmdfirstc == '=')/* can't do this recursively */
		    {
			beep_flush();
			c = ESC;
		    }
		    else
		    {
			save_ccline = ccline;
			ccline.cmdbuff = NULL;
			ccline.cmdprompt = NULL;
			c = get_expr_register();
			ccline = save_ccline;
		    }
		}
#endif
		if (c != ESC)	    /* use ESC to cancel inserting register */
		    cmdline_paste(c, i == Ctrl('R'));
		redrawcmd();
		goto cmdline_changed;

	case Ctrl('D'):
		if (showmatches(FALSE) == FAIL)
		    break;	/* Use ^D as normal char instead */

		redrawcmd();
		continue;	/* don't do incremental search now */

	case K_RIGHT:
	case K_S_RIGHT:
		do
		{
		    if (ccline.cmdpos >= ccline.cmdlen)
			break;
		    i = cmdline_charsize(ccline.cmdpos);
		    if (KeyTyped && ccline.cmdspos + i >= Columns * Rows)
			break;
		    ccline.cmdspos += i;
#ifdef MULTI_BYTE
		    if (is_dbcs && mb_isbyte1(ccline.cmdbuff, ccline.cmdpos))
		    {
			++ccline.cmdspos;
			++ccline.cmdpos;
		    }
#endif
		    ++ccline.cmdpos;
		}
		while ((c == K_S_RIGHT || (mod_mask & MOD_MASK_CTRL))
			&& ccline.cmdbuff[ccline.cmdpos] != ' ')
		    ;
		goto cmdline_not_changed;

	case K_LEFT:
	case K_S_LEFT:
		do
		{
		    if (ccline.cmdpos <= 0)
			break;
		    --ccline.cmdpos;
#ifdef MULTI_BYTE
		    if (is_dbcs && mb_isbyte2(ccline.cmdbuff, ccline.cmdpos))
		    {
			--ccline.cmdpos;
			--ccline.cmdspos;
		    }
#endif
		    ccline.cmdspos -= cmdline_charsize(ccline.cmdpos);
		}
		while ((c == K_S_LEFT || (mod_mask & MOD_MASK_CTRL))
			&& ccline.cmdbuff[ccline.cmdpos - 1] != ' ');
		goto cmdline_not_changed;

#if defined(USE_MOUSE) || defined(FKMAP)
	case K_IGNORE:
#endif
#ifdef USE_MOUSE
	case K_MIDDLEDRAG:
	case K_MIDDLERELEASE:
		goto cmdline_not_changed;   /* Ignore mouse */

	case K_MIDDLEMOUSE:
# ifdef USE_GUI
		/* When GUI is active, also paste when 'mouse' is empty */
		if (!gui.in_use)
# endif
		    if (!mouse_has(MOUSE_COMMAND))
			goto cmdline_not_changed;   /* Ignore mouse */
# ifdef USE_GUI
		if (gui.in_use)
		    cmdline_paste('*', TRUE);
		else
# endif
		    cmdline_paste(0, TRUE);
		redrawcmd();
		goto cmdline_changed;

	case K_LEFTDRAG:
	case K_LEFTRELEASE:
	case K_RIGHTDRAG:
	case K_RIGHTRELEASE:
		if (ignore_drag_release)
		    goto cmdline_not_changed;
		/* FALLTHROUGH */
	case K_LEFTMOUSE:
	case K_RIGHTMOUSE:
		if (c == K_LEFTRELEASE || c == K_RIGHTRELEASE)
		    ignore_drag_release = TRUE;
		else
		    ignore_drag_release = FALSE;
# ifdef USE_GUI
		/* When GUI is active, also move when 'mouse' is empty */
		if (!gui.in_use)
# endif
		    if (!mouse_has(MOUSE_COMMAND))
			goto cmdline_not_changed;   /* Ignore mouse */
		set_cmdspos();
		for (ccline.cmdpos = 0; ccline.cmdpos < ccline.cmdlen;
							      ++ccline.cmdpos)
		{
		    i = cmdline_charsize(ccline.cmdpos);
		    if (mouse_row <= cmdline_row + ccline.cmdspos / Columns
				  && mouse_col < ccline.cmdspos % Columns + i)
			break;
		    ccline.cmdspos += i;
		}
		goto cmdline_not_changed;

	/* Mouse scroll wheel: ignored here */
	case K_MOUSEDOWN:
	case K_MOUSEUP:
		goto cmdline_not_changed;

#endif	/* USE_MOUSE */

#ifdef USE_GUI
	case K_LEFTMOUSE_NM:	/* mousefocus click, ignored */
	case K_LEFTRELEASE_NM:
		goto cmdline_not_changed;

	case K_SCROLLBAR:
		if (!msg_scrolled)
		{
		    gui_do_scroll();
		    redrawcmd();
		}
		goto cmdline_not_changed;

	case K_HORIZ_SCROLLBAR:
		if (!msg_scrolled)
		{
		    gui_do_horiz_scroll();
		    redrawcmd();
		}
		goto cmdline_not_changed;
#endif
	case K_SELECT:	    /* end of Select mode mapping - ignore */
		goto cmdline_not_changed;

	case Ctrl('B'):	    /* begin of command line */
	case K_HOME:
	case K_KHOME:
	case K_XHOME:
	case K_S_HOME:
		ccline.cmdpos = 0;
		set_cmdspos();
		goto cmdline_not_changed;

	case Ctrl('E'):	    /* end of command line */
	case K_END:
	case K_KEND:
	case K_XEND:
	case K_S_END:
		ccline.cmdpos = ccline.cmdlen;
		ccline.cmdbuff[ccline.cmdlen] = NUL;
		set_cmdspos_cursor();
		goto cmdline_not_changed;

	case Ctrl('A'):	    /* all matches */
		if (nextwild(WILD_ALL, 0) == FAIL)
		    break;
		goto cmdline_changed;

	case Ctrl('L'):	    /* longest common part */
		if (nextwild(WILD_LONGEST, 0) == FAIL)
		    break;
		goto cmdline_changed;

	case Ctrl('N'):	    /* next match */
	case Ctrl('P'):	    /* previous match */
		if (cmd_numfiles > 0)
		{
		    if (nextwild((c == Ctrl('P')) ? WILD_PREV : WILD_NEXT, 0)
								      == FAIL)
			break;
		    goto cmdline_changed;
		}

	case K_UP:
	case K_DOWN:
	case K_S_UP:
	case K_S_DOWN:
	case K_PAGEUP:
	case K_KPAGEUP:
	case K_PAGEDOWN:
	case K_KPAGEDOWN:
		if (hislen == 0 || firstc == NUL)	/* no history */
		    goto cmdline_not_changed;

		i = hiscnt;

		/* save current command string so it can be restored later */
		ccline.cmdbuff[ccline.cmdlen] = NUL;
		if (lookfor == NULL)
		{
		    if ((lookfor = vim_strsave(ccline.cmdbuff)) == NULL)
			goto cmdline_not_changed;
		    lookfor[ccline.cmdpos] = NUL;
		}

		j = STRLEN(lookfor);
		for (;;)
		{
		    /* one step backwards */
		    if (c == K_UP || c == K_S_UP || c == Ctrl('P') ||
			    c == K_PAGEUP || c == K_KPAGEUP)
		    {
			if (hiscnt == hislen)	/* first time */
			    hiscnt = hisidx[histype];
			else if (hiscnt == 0 && hisidx[histype] != hislen - 1)
			    hiscnt = hislen - 1;
			else if (hiscnt != hisidx[histype] + 1)
			    --hiscnt;
			else			/* at top of list */
			{
			    hiscnt = i;
			    break;
			}
		    }
		    else    /* one step forwards */
		    {
			/* on last entry, clear the line */
			if (hiscnt == hisidx[histype])
			{
			    hiscnt = hislen;
			    break;
			}

			/* not on a history line, nothing to do */
			if (hiscnt == hislen)
			    break;
			if (hiscnt == hislen - 1)   /* wrap around */
			    hiscnt = 0;
			else
			    ++hiscnt;
		    }
		    if (hiscnt < 0 || history[histype][hiscnt].hisstr == NULL)
		    {
			hiscnt = i;
			break;
		    }
		    if ((c != K_UP && c != K_DOWN) || hiscnt == i ||
			    STRNCMP(history[histype][hiscnt].hisstr,
						    lookfor, (size_t)j) == 0)
			break;
		}

		if (hiscnt != i)	/* jumped to other entry */
		{
		    vim_free(ccline.cmdbuff);
		    if (hiscnt == hislen)
			p = lookfor;	/* back to the old one */
		    else
			p = history[histype][hiscnt].hisstr;

		    alloc_cmdbuff((int)STRLEN(p));
		    if (ccline.cmdbuff == NULL)
			goto returncmd;
		    STRCPY(ccline.cmdbuff, p);

		    ccline.cmdpos = ccline.cmdlen = STRLEN(ccline.cmdbuff);
		    redrawcmd();
		    goto cmdline_changed;
		}
		beep_flush();
		goto cmdline_not_changed;

	case Ctrl('V'):
	case Ctrl('Q'):
#ifdef USE_MOUSE
		ignore_drag_release = TRUE;
#endif
		putcmdline('^');
		c = get_literal();	    /* get next (two) character(s) */
		do_abbr = FALSE;	    /* don't do abbreviation now */
		break;

#ifdef DIGRAPHS
	case Ctrl('K'):
#ifdef USE_MOUSE
		ignore_drag_release = TRUE;
#endif
		putcmdline('?');
#ifdef USE_ON_FLY_SCROLL
		dont_scroll = TRUE;	    /* disallow scrolling here */
#endif
		++no_mapping;
		++allow_keys;
		c = safe_vgetc();
		--no_mapping;
		--allow_keys;
		if (c != ESC)		    /* ESC cancels CTRL-K */
		{
		    if (IS_SPECIAL(c))	    /* insert special key code */
			break;
		    if (charsize(c) == 1)
			putcmdline(c);
		    ++no_mapping;
		    ++allow_keys;
		    cc = safe_vgetc();
		    --no_mapping;
		    --allow_keys;
		    if (cc != ESC)	    /* ESC cancels CTRL-K */
		    {
			c = getdigraph(c, cc, TRUE);
			break;
		    }
		}
		redrawcmd();
		goto cmdline_not_changed;
#endif /* DIGRAPHS */

#ifdef RIGHTLEFT
	case Ctrl('_'):	    /* CTRL-_: switch language mode */
		if (!p_ari)
		    break;
#ifdef FKMAP
		if (p_altkeymap)
		{
		    cmd_fkmap = !cmd_fkmap;
		    if (cmd_fkmap)	/* in Farsi always in Insert mode */
			ccline.overstrike = FALSE;
		}
		else			    /* Hebrew is default */
#endif
		    cmd_hkmap = !cmd_hkmap;
		goto cmdline_not_changed;
#endif

	    /* keypad keys: When not mapped they produce a normal char */
	case K_KPLUS:		c = '+'; break;
	case K_KMINUS:		c = '-'; break;
	case K_KDIVIDE:		c = '/'; break;
	case K_KMULTIPLY:	c = '*'; break;

	default:
#ifdef UNIX
		if (c == intr_char)
		{
		    gotesc = TRUE;	/* will free ccline.cmdbuff after
					   putting it in history */
		    goto returncmd;	/* back to Normal mode */
		}
#endif
		/*
		 * Normal character with no special meaning.  Just set mod_mask
		 * to 0x0 so that typing Shift-Space in the GUI doesn't enter
		 * the string <S-Space>.  This should only happen after ^V.
		 */
		if (!IS_SPECIAL(c))
		    mod_mask = 0x0;
		break;
	}
	/*
	 * End of switch on command line character.
	 * We come here if we have a normal character.
	 */

	if (do_abbr && (IS_SPECIAL(c) || !vim_iswordc(c)) && ccheck_abbr(c))
	    goto cmdline_changed;

	/*
	 * put the character in the command line
	 */
	if (IS_SPECIAL(c) || mod_mask != 0x0)
	    put_on_cmdline(get_special_key_name(c, mod_mask), -1, TRUE);
	else
	{
	    IObuff[0] = c;

#ifdef MULTI_BYTE
	    /* prevent from splitting a multi-byte character into two
	       indivisual characters in command line. */
	    if (is_dbcs && IsLeadByte(c))
	    {
		IObuff[1] = safe_vgetc();
		put_on_cmdline(IObuff, 2, TRUE);
	    }
	    else
		put_on_cmdline(IObuff, 1, TRUE);
#else
	    put_on_cmdline(IObuff, 1, TRUE);
#endif
	}
	goto cmdline_changed;

/*
 * This part implements incremental searches for "/" and "?"
 * Jump to cmdline_not_changed when a character has been read but the command
 * line did not change. Then we only search and redraw if something changed in
 * the past.
 * Jump to cmdline_changed when the command line did change.
 * (Sorry for the goto's, I know it is ugly).
 */
cmdline_not_changed:
#ifdef EXTRA_SEARCH
	if (!incsearch_postponed)
	    continue;
#endif

cmdline_changed:
#ifdef EXTRA_SEARCH
	if (p_is && (firstc == '/' || firstc == '?'))
	{
	    /* if there is a character waiting, search and redraw later */
	    if (char_avail())
	    {
		incsearch_postponed = TRUE;
		continue;
	    }
	    incsearch_postponed = FALSE;
	    curwin->w_cursor = old_cursor;  /* start at old position */

	    /* If there is no command line, don't do anything */
	    if (ccline.cmdlen == 0)
		i = 0;
	    else
	    {
		ccline.cmdbuff[ccline.cmdlen] = NUL;
		++emsg_off;    /* So it doesn't beep if bad expr */
		i = do_search(NULL, firstc, ccline.cmdbuff, count,
				      SEARCH_KEEP + SEARCH_OPT + SEARCH_NOOF);
		--emsg_off;
	    }
	    if (i)
		highlight_match = TRUE;		/* highlight position */
	    else
		highlight_match = FALSE;	    /* don't highlight */

	    /* first restore the old curwin values, so the screen is
	     * positioned in the same way as the actual search command */
	    curwin->w_leftcol = old_leftcol;
	    curwin->w_topline = old_topline;
	    curwin->w_botline = old_botline;
	    update_topline();
	    /*
	     * First move cursor to end of match, then to start.  This moves
	     * the whole match onto the screen when 'nowrap' is set.
	     */
	    curwin->w_cursor.col += search_match_len;
	    validate_cursor();
	    curwin->w_cursor.col -= search_match_len;
	    validate_cursor();

	    update_screen(NOT_VALID);
	    redrawcmdline();
	    did_incsearch = TRUE;
	}
#else
	;
#endif
    }

returncmd:

#ifdef FKMAP
    cmd_fkmap = 0;
#endif

#ifdef EXTRA_SEARCH
    if (did_incsearch)
    {
	curwin->w_cursor = old_cursor;
	curwin->w_curswant = old_curswant;
	curwin->w_leftcol = old_leftcol;
	curwin->w_topline = old_topline;
	curwin->w_botline = old_botline;
	highlight_match = FALSE;
	validate_cursor();	/* needed for TAB */
	redraw_later(NOT_VALID);
    }
#endif

    if (ccline.cmdbuff != NULL)
    {
	/*
	 * Put line in history buffer (":" and "=" only when it was typed).
	 */
	ccline.cmdbuff[ccline.cmdlen] = NUL;
	if (ccline.cmdlen && firstc
				&& (some_key_typed || histype == HIST_SEARCH))
	{
	    add_to_history(histype, ccline.cmdbuff, TRUE);
	    if (firstc == ':')
	    {
		vim_free(new_last_cmdline);
		new_last_cmdline = vim_strsave(ccline.cmdbuff);
	    }
	}

	if (gotesc)	    /* abandon command line */
	{
	    vim_free(ccline.cmdbuff);
	    ccline.cmdbuff = NULL;
	    MSG("");
	    redraw_cmdline = TRUE;
	}
    }

    /*
     * If the screen was shifted up, redraw the whole screen (later).
     * If the line is too long, clear it, so ruler and shown command do
     * not get printed in the middle of it.
     */
    msg_check();
    msg_scroll = save_msg_scroll;
    redir_off = FALSE;

    /* When the command line was typed, no need for a wait-return prompt. */
    if (some_key_typed)
	need_wait_return = FALSE;

    State = save_State;
#ifdef USE_MOUSE
    setmouse();
#endif

    return ccline.cmdbuff;
}

#if (defined(CRYPTV) || defined(WANT_EVAL)) || defined(PROTO)
/*
 * Get a command line with a prompt.
 * This is prepared to be called recursively from getcmdline() (e.g. by
 * f_input() when evaluating an expression from CTRL-R =).
 * Returns the command line in allocated memory, or NULL.
 */
    char_u *
getcmdline_prompt(firstc, prompt, attr)
    int		firstc;
    char_u	*prompt;	/* command line prompt */
    int		attr;		/* attributes for prompt */
{
    char_u		*s;
    struct cmdline_info	save_ccline;

    save_ccline = ccline;
    ccline.cmdbuff = NULL;
    ccline.cmdprompt = prompt;
    ccline.cmdattr = attr;
    s = getcmdline(firstc, 1L, 0);
    ccline = save_ccline;

    return s;
}
#endif

    static int
cmdline_charsize(idx)
    int		idx;
{
#ifdef CRYPTV
    if (cmdline_crypt)	    /* showing '*', always 1 position */
	return 1;
#endif
    return charsize(ccline.cmdbuff[idx]);
}

/*
 * Compute the offset of the cursor on the command line for the prompt and
 * indent.
 */
    static void
set_cmdspos()
{
    if (ccline.cmdfirstc)
	ccline.cmdspos = 1 + ccline.cmdindent;
    else
	ccline.cmdspos = 0 + ccline.cmdindent;
}

/*
 * Compute the screen position for the cursor on the command line.
 */
    static void
set_cmdspos_cursor()
{
    int		i;
    int		m;
    int		c;

    set_cmdspos();
    if (KeyTyped)
	m = Columns * Rows;
    else
	m = MAXCOL;
    for (i = 0; i < ccline.cmdlen && i < ccline.cmdpos; ++i)
    {
#ifdef CRYPTV
	if (cmdline_crypt)
	    c = 1;
	else
#endif
	    c = charsize(ccline.cmdbuff[i]);
#ifdef MULTI_BYTE
	/* multibyte wrap */
	if (is_dbcs && IsLeadByte(ccline.cmdbuff[i])
				 && (ccline.cmdspos % Columns + c == Columns))
	    ccline.cmdspos++;
#endif
	/* If the cmdline doesn't fit, put cursor on last visible char. */
	if ((ccline.cmdspos += c) >= m)
	{
	    ccline.cmdpos = i - 1;
	    ccline.cmdspos -= c;
	    break;
	}
    }
}

/*
 * Get an Ex command line for the ":" command.
 */
/* ARGSUSED */
    char_u *
getexline(c, dummy, indent)
    int		c;		/* normally ':', NUL for ":append" */
    void	*dummy;		/* cookie not used */
    int		indent;		/* indent for inside conditionals */
{
    /* When executing a register, remove ':' that's in front of each line. */
    if (exec_from_reg && vpeekc() == ':')
	(void)vgetc();
    return getcmdline(c, 1L, indent);
}

/*
 * Get an Ex command line for Ex mode.
 * In Ex mode we only use the OS supplied line editing features and no
 * mappings or abbreviations.
 */
/* ARGSUSED */
    char_u *
getexmodeline(c, dummy, indent)
    int		c;		/* normally ':', NUL for ":append" */
    void	*dummy;		/* cookie not used */
    int		indent;		/* indent for inside conditionals */
{
    struct growarray	line_ga;
    int			len;
    int			off = 0;
    char_u		*p;
    int			finished = FALSE;
#if defined(USE_GUI) || defined(NO_COOKED_INPUT)
    int			startcol = 0;
    int			c1;
    int			escaped = FALSE;	/* CTRL-V typed */
    int			vcol = 0;
#endif

    /* always start in column 0; write a newline if necessary */
    compute_cmdrow();
    if (msg_col)
	msg_putchar('\n');
    if (c == ':')
    {
	msg_putchar(':');
	while (indent-- > 0)
	    msg_putchar(' ');
#if defined(USE_GUI) || defined(NO_COOKED_INPUT)
	startcol = msg_col;
#endif
    }

    ga_init2(&line_ga, 1, 30);

    /*
     * Get the line, one character at a time.
     */
    got_int = FALSE;
    while (!got_int && !finished)
    {
	if (ga_grow(&line_ga, 40) == FAIL)
	    break;
	p = (char_u *)line_ga.ga_data + line_ga.ga_len;

	/* Get one character (inchar gets a third of maxlen characters!) */
	len = inchar(p + off, 3, -1L);
	if (len < 0)
	    continue;	    /* end of input script reached */
	/* for a special character, we need at least three characters */
	if ((*p == K_SPECIAL || *p == CSI) && off + len < 3)
	{
	    off += len;
	    continue;
	}
	len += off;
	off = 0;

	/*
	 * When using the GUI, and for systems that don't have cooked input,
	 * handle line editing here.
	 */
#if defined(USE_GUI) || defined(NO_COOKED_INPUT)
# ifndef NO_COOKED_INPUT
	if (gui.in_use)
# endif
	{
	    if (got_int)
	    {
		msg_putchar('\n');
		break;
	    }

	    while (len > 0)
	    {
		c1 = *p++;
		--len;
# ifndef NO_COOKED_INPUT
		if ((c1 == K_SPECIAL || c1 == CSI) && len >= 2)
# else
#  ifdef USE_GUI
		if ((c1 == K_SPECIAL || (c1 == CSI && gui.in_use)) && len >= 2)
#  else
		if (c1 == K_SPECIAL && len >= 2)
#  endif
# endif
		{
		    c1 = TO_SPECIAL(p[0], p[1]);
		    p += 2;
		    len -= 2;
		}

		if (!escaped)
		{
		    /* CR typed means "enter", which is NL */
		    if (c1 == '\r')
			c1 = '\n';

		    if (c1 == BS || c1 == K_BS
				  || c1 == DEL || c1 == K_DEL || c1 == K_KDEL)
		    {
			if (line_ga.ga_len > 0)
			{
			    int		i, v;
			    char_u	*q;

			    --line_ga.ga_len;
			    ++line_ga.ga_room;
			    /* compute column that cursor should be in */
			    v = 0;
			    q = ((char_u *)line_ga.ga_data);
			    for (i = 0; i < line_ga.ga_len; ++i)
			    {
				if (*q == TAB)
				    v += 8 - v % 8;
				else
				    v += charsize(*q);
				++q;
			    }
			    /* erase characters to position cursor */
			    while (vcol > v)
			    {
				msg_putchar('\b');
				msg_putchar(' ');
				msg_putchar('\b');
				--vcol;
			    }
			}
			continue;
		    }

		    if (c1 == Ctrl('U'))
		    {
			msg_col = startcol;
			msg_clr_eos();
			line_ga.ga_room += line_ga.ga_len;
			line_ga.ga_len = 0;
			continue;
		    }

		    if (c1 == Ctrl('V'))
		    {
			escaped = TRUE;
			continue;
		    }
		}

		((char_u *)line_ga.ga_data)[line_ga.ga_len] = c1;
		if (c1 == '\n')
		    msg_putchar('\n');
		else if (c1 == TAB)
		{
		    /* Don't use chartabsize(), 'ts' can be different */
		    do
		    {
			msg_putchar(' ');
		    } while (++vcol % 8);
		}
		else
		{
		    msg_outtrans_len(
			     ((char_u *)line_ga.ga_data) + line_ga.ga_len, 1);
		    vcol += charsize(c1);
		}
		++line_ga.ga_len;
		--line_ga.ga_room;
		escaped = FALSE;
	    }
	    windgoto(msg_row, msg_col);
	}
# ifndef NO_COOKED_INPUT
	else
# endif
#endif
#ifndef NO_COOKED_INPUT
	{
	    line_ga.ga_len += len;
	    line_ga.ga_room -= len;
	}
#endif
	p = (char_u *)(line_ga.ga_data) + line_ga.ga_len;
	if (line_ga.ga_len && p[-1] == '\n')
	{
	    finished = TRUE;
	    --line_ga.ga_len;
	    --p;
	    *p = NUL;
	}
    }

    /* note that cursor has moved, because of the echoed <CR> */
    screen_down();
    /* make following messages go to the next line */
    msg_didout = FALSE;
    msg_col = 0;
    if (msg_row < Rows - 1)
	++msg_row;
    emsg_on_display = FALSE;		/* don't want ui_delay() */

    if (got_int)
	ga_clear(&line_ga);

    return (char_u *)line_ga.ga_data;
}

#ifdef CURSOR_SHAPE
/*
 * Return TRUE if ccline.overstrike is on.
 */
    int
cmdline_overstrike()
{
    return ccline.overstrike;
}

/*
 * Return TRUE if the cursor is at the end of the cmdline.
 */
    int
cmdline_at_end()
{
    return (ccline.cmdpos >= ccline.cmdlen);
}
#endif

/*
 * Allocate a new command line buffer.
 * Assigns the new buffer to ccline.cmdbuff and ccline.cmdbufflen.
 * Returns the new value of ccline.cmdbuff and ccline.cmdbufflen.
 */
    static void
alloc_cmdbuff(len)
    int	    len;
{
    /*
     * give some extra space to avoid having to allocate all the time
     */
    if (len < 80)
	len = 100;
    else
	len += 20;

    ccline.cmdbuff = alloc(len);    /* caller should check for out-of-memory */
    ccline.cmdbufflen = len;
}

/*
 * Re-allocate the command line to length len + something extra.
 * return FAIL for failure, OK otherwise
 */
    static int
realloc_cmdbuff(len)
    int	    len;
{
    char_u	*p;

    p = ccline.cmdbuff;
    alloc_cmdbuff(len);			/* will get some more */
    if (ccline.cmdbuff == NULL)		/* out of memory */
    {
	ccline.cmdbuff = p;		/* keep the old one */
	return FAIL;
    }
    mch_memmove(ccline.cmdbuff, p, (size_t)ccline.cmdlen);
    vim_free(p);
    return OK;
}

/*
 * put a character on the command line.
 * Used for CTRL-V and CTRL-K
 */
    static void
putcmdline(c)
    int	    c;
{
    char_u  buf[1];

    msg_no_more = TRUE;
    buf[0] = c;
    msg_outtrans_len(buf, 1);
    msg_outtrans_len(ccline.cmdbuff + ccline.cmdpos,
					       ccline.cmdlen - ccline.cmdpos);
    msg_no_more = FALSE;
    cursorcmd();
}

/*
 * Put the given string, of the given length, onto the command line.
 * If len is -1, then STRLEN() is used to calculate the length.
 * If 'redraw' is TRUE then the new part of the command line, and the remaining
 * part will be redrawn, otherwise it will not.  If this function is called
 * twice in a row, then 'redraw' should be FALSE and redrawcmd() should be
 * called afterwards.
 */
    int
put_on_cmdline(str, len, redraw)
    char_u	*str;
    int		len;
    int		redraw;
{
    int		retval;
    int		i;
    int		m;
    int		c;

    if (len < 0)
	len = STRLEN(str);

    /* Check if ccline.cmdbuff needs to be longer */
    if (ccline.cmdlen + len + 1 >= ccline.cmdbufflen)
	retval = realloc_cmdbuff(ccline.cmdlen + len);
    else
	retval = OK;
    if (retval == OK)
    {
	if (!ccline.overstrike)
	{
	    mch_memmove(ccline.cmdbuff + ccline.cmdpos + len,
					       ccline.cmdbuff + ccline.cmdpos,
				     (size_t)(ccline.cmdlen - ccline.cmdpos));
	    ccline.cmdlen += len;
	}
	else if (ccline.cmdpos + len > ccline.cmdlen)
	    ccline.cmdlen = ccline.cmdpos + len;
	mch_memmove(ccline.cmdbuff + ccline.cmdpos, str, (size_t)len);
	if (redraw)
	{
#ifdef CRYPTV
	    if (cmdline_crypt)		/* only write '*' characters */
		for (i = ccline.cmdpos; i < ccline.cmdlen; ++i)
		    msg_putchar('*');
	    else
#endif
	    {
		msg_no_more = TRUE;
		msg_outtrans_len(ccline.cmdbuff + ccline.cmdpos,
					       ccline.cmdlen - ccline.cmdpos);
		msg_no_more = FALSE;
	    }
	}
#ifdef FKMAP
	/*
	 * If we are in Farsi command mode, the character input must be in
	 * Insert mode. So do not advance the cmdpos.
	 */
	if (!cmd_fkmap)
#endif
	{
	    if (KeyTyped)
		m = Columns * Rows;
	    else
		m = MAXCOL;
	    for (i = 0; i < len; ++i)
	    {
#ifdef CRYPTV
		if (cmdline_crypt)
		    c = 1;
		else
#endif
		    c = charsize(str[i]);
#ifdef MULTI_BYTE
		/* multibyte wrap */
		if (is_dbcs && IsLeadByte(str[i])
				   && ccline.cmdspos % Columns + c == Columns)
		    ccline.cmdspos++;
#endif
		/* Stop cursor at the end of the screen */
		if (ccline.cmdspos + c >= m)
		    break;
		++ccline.cmdpos;
		ccline.cmdspos += c;
	    }
	}
    }
    if (redraw)
	msg_check();
    return retval;
}

#ifdef WILDMENU
/*
 * Delete characters on the command line, from "from" to the current
 * position.
 */
    static void
cmdline_del(from)
    int from;
{
    mch_memmove(ccline.cmdbuff + from, ccline.cmdbuff + ccline.cmdpos,
	    (size_t)(ccline.cmdlen - ccline.cmdpos));
    ccline.cmdlen -= ccline.cmdpos - from;
    ccline.cmdpos = from;
}
#endif

/*
 * this fuction is called when the screen size changes and with incremental
 * search
 */
    void
redrawcmdline()
{
    need_wait_return = FALSE;
    compute_cmdrow();
    redrawcmd();
    cursorcmd();
}

    static void
redrawcmdprompt()
{
    int		i;

    if (ccline.cmdfirstc)
	msg_putchar(ccline.cmdfirstc);
    if (ccline.cmdprompt != NULL)
    {
	msg_puts_attr(ccline.cmdprompt, ccline.cmdattr);
	ccline.cmdindent = msg_col;
    }
    else
	for (i = ccline.cmdindent; i > 0; --i)
	    msg_putchar(' ');
}

/*
 * Redraw what is currently on the command line.
 */
    static void
redrawcmd()
{
#ifdef CRYPTV
    int		i;
#endif

    msg_start();
    redrawcmdprompt();
#ifdef CRYPTV
    if (cmdline_crypt)
    {
	/* Show '*' for every character typed */
	for (i = 0; i < ccline.cmdlen; ++i)
	    msg_putchar('*');
	msg_clr_eos();
    }
    else
#endif
    {
	/* Don't use more prompt, truncate the cmdline if it doesn't fit. */
	msg_no_more = TRUE;
	msg_outtrans_len(ccline.cmdbuff, ccline.cmdlen);
	msg_clr_eos();
	msg_no_more = FALSE;
    }
    set_cmdspos_cursor();

    /*
     * An emsg() before may have set msg_scroll. This is used in normal mode,
     * in cmdline mode we can reset them now.
     */
    msg_scroll = FALSE;		/* next message overwrites cmdline */

    /* Typing ':' at the more prompt may set skip_redraw.  We don't want this
     * in cmdline mode */
    skip_redraw = FALSE;
}

    void
compute_cmdrow()
{
    if (exmode_active || msg_scrolled)
	cmdline_row = Rows - 1;
    else
	cmdline_row = lastwin->w_winpos + lastwin->w_height +
					lastwin->w_status_height;
}

    static void
cursorcmd()
{
    msg_row = cmdline_row + (ccline.cmdspos / (int)Columns);
    msg_col = ccline.cmdspos % (int)Columns;
    if (msg_row >= Rows)
	msg_row = Rows - 1;
    windgoto(msg_row, msg_col);
#ifdef MCH_CURSOR_SHAPE
    mch_update_cursor();
#endif
}

    void
gotocmdline(clr)
    int		    clr;
{
    msg_start();
    msg_col = 0;	    /* always start in column 0 */
    if (clr)		    /* clear the bottom line(s) */
	msg_clr_eos();	    /* will reset clear_cmdline */
    windgoto(cmdline_row, 0);
}

/*
 * Check the word in front of the cursor for an abbreviation.
 * Called when the non-id character "c" has been entered.
 * When an abbreviation is recognized it is removed from the text with
 * backspaces and the replacement string is inserted, followed by "c".
 */
    static int
ccheck_abbr(c)
    int c;
{
    if (p_paste || no_abbr)	    /* no abbreviations or in paste mode */
	return FALSE;

    return check_abbr(c, ccline.cmdbuff, ccline.cmdpos, 0);
}

/*
 * Return FAIL if this is not an appropriate context in which to do
 * completion of anything, return OK if it is (even if there are no matches).
 * For the caller, this means that the character is just passed through like a
 * normal character (instead of being expanded).  This allows :s/^I^D etc.
 */
    static int
nextwild(type, options)
    int	    type;
    int	    options;	    /* extra options for ExpandOne() */
{
    int	    i, j;
    char_u  *p1;
    char_u  *p2;
    int	    oldlen;
    int	    difflen;
    int	    v;

    if (cmd_numfiles == -1)
	set_expand_context();

    if (expand_context == EXPAND_UNSUCCESSFUL)
    {
	beep_flush();
	return OK;  /* Something illegal on command line */
    }
    if (expand_context == EXPAND_NOTHING)
    {
	/* Caller can use the character as a normal char instead */
	return FAIL;
    }

    MSG_PUTS("...");	    /* show that we are busy */
    out_flush();

    i = expand_pattern - ccline.cmdbuff;
    oldlen = ccline.cmdpos - i;

    if (type == WILD_NEXT || type == WILD_PREV)
    {
	/*
	 * Get next/previous match for a previous expanded pattern.
	 */
	p2 = ExpandOne(NULL, NULL, 0, type);
    }
    else
    {
	/*
	 * Translate string into pattern and expand it.
	 */
	if ((p1 = addstar(&ccline.cmdbuff[i], oldlen, expand_context)) == NULL)
	    p2 = NULL;
	else
	{
	    p2 = ExpandOne(p1, vim_strnsave(&ccline.cmdbuff[i], oldlen),
		    WILD_HOME_REPLACE|WILD_ADD_SLASH|WILD_SILENT|WILD_ESCAPE
							      |options, type);
	    vim_free(p1);
	    /* longest match: make sure it is not shorter (happens with :help */
	    if (p2 != NULL && type == WILD_LONGEST)
	    {
		for (j = 0; j < oldlen; ++j)
		     if (ccline.cmdbuff[i + j] == '*'
			     || ccline.cmdbuff[i + j] == '?')
			 break;
		if ((int)STRLEN(p2) < j)
		{
		    vim_free(p2);
		    p2 = NULL;
		}
	    }
	}
    }

    if (p2 != NULL && !got_int)
    {
	if (ccline.cmdlen + (difflen = STRLEN(p2) - oldlen) >
							ccline.cmdbufflen - 4)
	{
	    v = realloc_cmdbuff(ccline.cmdlen + difflen);
	    expand_pattern = ccline.cmdbuff + i;
	}
	else
	    v = OK;
	if (v == OK)
	{
	    vim_strncpy(&ccline.cmdbuff[ccline.cmdpos + difflen],
					       &ccline.cmdbuff[ccline.cmdpos],
		    ccline.cmdlen - ccline.cmdpos);
	    STRNCPY(&ccline.cmdbuff[i], p2, STRLEN(p2));
	    ccline.cmdlen += difflen;
	    ccline.cmdpos += difflen;
	}
    }
    vim_free(p2);

    redrawcmd();

    /* When expanding a ":map" command and no matches are found, assume that
     * the key is supposed to be inserted literally */
    if (expand_context == EXPAND_MAPPINGS && p2 == NULL)
	return FAIL;

    if (cmd_numfiles <= 0 && p2 == NULL)
	beep_flush();
    else if (cmd_numfiles == 1)
	(void)ExpandOne(NULL, NULL, 0, WILD_FREE);  /* free expanded pattern */

    return OK;
}

/*
 * Do wildcard expansion on the string 'str'.
 * Chars that should not be expanded must be preceded with a backslash.
 * Return a pointer to alloced memory containing the new string.
 * Return NULL for failure.
 *
 * mode = WILD_FREE:	    just free previously expanded matches
 * mode = WILD_EXPAND_FREE: normal expansion, do not keep matches
 * mode = WILD_EXPAND_KEEP: normal expansion, keep matches
 * mode = WILD_NEXT:	    use next match in multiple match, wrap to first
 * mode = WILD_PREV:	    use previous match in multiple match, wrap to first
 * mode = WILD_ALL:	    return all matches concatenated
 * mode = WILD_LONGEST:	    return longest matched part
 *
 * options = WILD_LIST_NOTFOUND:    list entries without a match
 * options = WILD_HOME_REPLACE:	    do home_replace() for buffer names
 * options = WILD_USE_NL:	    Use '\n' for WILD_ALL
 * options = WILD_NO_BEEP:	    Don't beep for multiple matches
 * options = WILD_ADD_SLASH:	    add a slash after directory names
 * options = WILD_KEEP_ALL:	    don't remove 'wildignore' entries
 * options = WILD_SILENT:	    don't print warning messages
 * options = WILD_ESCAPE:	    put backslash before special chars
 *
 * The global variable expand_context must have been set!
 */
    char_u *
ExpandOne(str, orig, options, mode)
    char_u	*str;
    char_u	*orig;	    /* allocated copy of original of expanded string */
    int		options;
    int		mode;
{
    char_u	*ss = NULL;
    static int	findex;
    static char_u *orig_save = NULL;	/* kept value of orig */
    int		i;
    int		non_suf_match;		/* number without matching suffix */
    long_u	len;
    char_u	*p;

/*
 * first handle the case of using an old match
 */
    if (mode == WILD_NEXT || mode == WILD_PREV)
    {
	if (cmd_numfiles > 0)
	{
	    if (mode == WILD_PREV)
	    {
		if (findex == -1)
		    findex = cmd_numfiles;
		--findex;
	    }
	    else    /* mode == WILD_NEXT */
		++findex;

	    /*
	     * When wrapping around, return the original string, set findex to
	     * -1.
	     */
	    if (findex < 0)
	    {
		if (orig_save == NULL)
		    findex = cmd_numfiles - 1;
		else
		    findex = -1;
	    }
	    if (findex >= cmd_numfiles)
	    {
		if (orig_save == NULL)
		    findex = 0;
		else
		    findex = -1;
	    }
#ifdef WILDMENU
	    if (p_wmnu)
		win_redr_status_matches(cmd_numfiles, cmd_files, findex);
#endif
	    if (findex == -1)
		return vim_strsave(orig_save);
	    return vim_strsave(cmd_files[findex]);
	}
	else
	    return NULL;
    }

/* free old names */
    if (cmd_numfiles != -1 && mode != WILD_ALL && mode != WILD_LONGEST)
    {
	FreeWild(cmd_numfiles, cmd_files);
	cmd_numfiles = -1;
	vim_free(orig_save);
	orig_save = NULL;
    }
    findex = 0;

    if (mode == WILD_FREE)	/* only release file name */
	return NULL;

    if (cmd_numfiles == -1)
    {
	vim_free(orig_save);
	orig_save = orig;

	/*
	 * Do the expansion.
	 */
	if (ExpandFromContext(str, &cmd_numfiles, &cmd_files, FALSE,
							     options) == FAIL)
	{
#ifdef FNAME_ILLEGAL
	    /* Illegal file name has been silently skipped.  But when there
	     * are wildcards, the real problem is that there was no match,
	     * causing the pattern to be added, which has illegal characters.
	     */
	    if (!(options & WILD_SILENT) && (options & WILD_LIST_NOTFOUND))
		emsg2(e_nomatch2, str);
#endif
	}
	else if (cmd_numfiles == 0)
	{
	    if (!(options & WILD_SILENT))
		emsg2(e_nomatch2, str);
	}
	else
	{
	    /*
	     * May change home directory back to "~"
	     */
	    if (options & WILD_HOME_REPLACE)
		tilde_replace(str, cmd_numfiles, cmd_files);

	    if (options & WILD_ESCAPE)
	    {
	      if (expand_context == EXPAND_FILES
			|| expand_context == EXPAND_BUFFERS
			|| expand_context == EXPAND_DIRECTORIES)
	      {
		/*
		 * Insert a backslash into a file name before a space, \, %, #
		 * and wildmatch characters, except '~'.
		 */
		for (i = 0; i < cmd_numfiles; ++i)
		{
		    /* for ":set path=" we need to escape spaces twice */
		    if (expand_set_path)
		    {
			p = vim_strsave_escaped(cmd_files[i], (char_u *)" ");
			if (p != NULL)
			{
			    vim_free(cmd_files[i]);
			    cmd_files[i] = p;
#if defined(BACKSLASH_IN_FILENAME) || defined(COLON_AS_PATHSEP)
			    p = vim_strsave_escaped(cmd_files[i],
							       (char_u *)" ");
			    if (p != NULL)
			    {
				vim_free(cmd_files[i]);
				cmd_files[i] = p;
			    }
#endif
			}
		    }
		    p = vim_strsave_escaped(cmd_files[i],
#ifdef BACKSLASH_IN_FILENAME
						    (char_u *)" *?[{`$%#"
#else
# ifdef COLON_AS_PATHSEP
						    (char_u *)" *?[{`$%#/"
# else
						    (char_u *)" *?[{`$\\%#'\"|"
# endif
#endif
									    );
		    if (p != NULL)
		    {
			vim_free(cmd_files[i]);
			cmd_files[i] = p;
		    }

		    /* If 'str' starts with "\~", replace "~" at start of
		     * cmd_files[i] with "\~". */
		    if (str[0] == '\\' && str[1] == '~'
						    && cmd_files[i][0] == '~')
		    {
			p = alloc((unsigned)(STRLEN(cmd_files[i]) + 2));
			if (p != NULL)
			{
			    p[0] = '\\';
			    STRCPY(p + 1, cmd_files[i]);
			    vim_free(cmd_files[i]);
			    cmd_files[i] = p;
			}
		    }
		}
		expand_set_path = FALSE;
	      }
	      else if (expand_context == EXPAND_TAGS)
	      {
		/*
		 * Insert a backslash before characters in a tag name that
		 * would terminate the ":tag" command.
		 */
		for (i = 0; i < cmd_numfiles; ++i)
		{
		    p = vim_strsave_escaped(cmd_files[i], (char_u *)"\\|\"");
		    if (p != NULL)
		    {
			vim_free(cmd_files[i]);
			cmd_files[i] = p;
		    }
		}
	      }
	    }

	    /*
	     * Check for matching suffixes in file names.
	     */
	    if (mode != WILD_ALL && mode != WILD_LONGEST)
	    {
		if (cmd_numfiles)
		    non_suf_match = cmd_numfiles;
		else
		    non_suf_match = 1;
		if ((expand_context == EXPAND_FILES
			    || expand_context == EXPAND_DIRECTORIES)
			&& cmd_numfiles > 1)	/* more than one match; check suffix */
		{
		    /*
		     * The files will have been sorted on matching suffix in
		     * expand_wildcards, only need to check the first two.
		     */
		    non_suf_match = 0;
		    for (i = 0; i < 2; ++i)
			if (match_suffix(cmd_files[i]))
			    ++non_suf_match;
		}
		if (non_suf_match != 1)
		{
		    /* Can we ever get here unless it's while expanding
		     * interactively?  If not, we can get rid of this all
		     * together. Don't really want to wait for this message
		     * (and possibly have to hit return to continue!).
		     */
		    if (!(options & WILD_SILENT))
			emsg(e_toomany);
		    else if (!(options & WILD_NO_BEEP))
			beep_flush();
		}
		if (!(non_suf_match != 1 && mode == WILD_EXPAND_FREE))
		    ss = vim_strsave(cmd_files[0]);
	    }
	}
    }

    /* Find longest common part */
    if (mode == WILD_LONGEST && cmd_numfiles > 0)
    {
	for (len = 0; cmd_files[0][len]; ++len)
	{
	    for (i = 0; i < cmd_numfiles; ++i)
	    {
#ifdef CASE_INSENSITIVE_FILENAME
		if (expand_context == EXPAND_DIRECTORIES
			|| expand_context == EXPAND_FILES
			|| expand_context == EXPAND_BUFFERS)
		{
		    if (TO_LOWER(cmd_files[i][len]) !=
						   TO_LOWER(cmd_files[0][len]))
			break;
		}
		else
#endif
		     if (cmd_files[i][len] != cmd_files[0][len])
		    break;
	    }
	    if (i < cmd_numfiles)
	    {
		if (!(options & WILD_NO_BEEP))
		    vim_beep();
		break;
	    }
	}
	ss = alloc((unsigned)len + 1);
	if (ss)
	{
	    STRNCPY(ss, cmd_files[0], len);
	    ss[len] = NUL;
	}
	findex = -1;			    /* next p_wc gets first one */
    }

    /* Concatenate all matching names */
    if (mode == WILD_ALL && cmd_numfiles > 0)
    {
	len = 0;
	for (i = 0; i < cmd_numfiles; ++i)
	    len += STRLEN(cmd_files[i]) + 1;
	ss = lalloc(len, TRUE);
	if (ss != NULL)
	{
	    *ss = NUL;
	    for (i = 0; i < cmd_numfiles; ++i)
	    {
		STRCAT(ss, cmd_files[i]);
		if (i != cmd_numfiles - 1)
		    STRCAT(ss, (options & WILD_USE_NL) ? "\n" : " ");
	    }
	}
    }

    if (mode == WILD_EXPAND_FREE || mode == WILD_ALL)
    {
	FreeWild(cmd_numfiles, cmd_files);
	cmd_numfiles = -1;
    }

    return ss;
}

/*
 * For each file name in files[num_files]:
 * If 'orig_pat' starts with "~/", replace the home directory with "~".
 */
    void
tilde_replace(orig_pat, num_files, files)
    char_u  *orig_pat;
    int	    num_files;
    char_u  **files;
{
    int	    i;
    char_u  *p;

    if (orig_pat[0] == '~' && vim_ispathsep(orig_pat[1]))
    {
	for (i = 0; i < num_files; ++i)
	{
	    p = home_replace_save(NULL, files[i]);
	    if (p != NULL)
	    {
		vim_free(files[i]);
		files[i] = p;
	    }
	}
    }
}

/*
 * show all matches for completion on the command line
 */
/*ARGSUSED*/
    static int
showmatches(wildmenu)
    int		wildmenu;
{
    char_u	*file_str = NULL;
    int		num_files;
    char_u	**files_found;
    int		i, j, k;
    int		maxlen;
    int		lines;
    int		columns;
    char_u	*p;
    int		lastlen;
    int		attr;

    if (cmd_numfiles == -1)
    {
	set_expand_context();
	if (expand_context == EXPAND_UNSUCCESSFUL)
	{
	    beep_flush();
	    return OK;  /* Something illegal on command line */
	}
	if (expand_context == EXPAND_NOTHING)
	{
	    /* Caller can use the character as a normal char instead */
	    return FAIL;
	}

	/* add star to file name, or convert to regexp if not exp. files. */
	file_str = addstar(expand_pattern,
		      (int)(ccline.cmdbuff + ccline.cmdpos - expand_pattern),
							      expand_context);
	if (file_str == NULL)
	    return OK;

	/* find all files that match the description */
	if (ExpandFromContext(file_str, &num_files, &files_found, FALSE,
					  WILD_ADD_SLASH|WILD_SILENT) == FAIL)
	{
	    num_files = 0;
	    files_found = (char_u **)"";
	}
    }
    else
    {
	num_files = cmd_numfiles;
	files_found = cmd_files;
    }

#ifdef WILDMENU
    if (!wildmenu)
    {
#endif
	msg_didany = FALSE;		/* lines_left will be set */
	msg_start();			/* prepare for paging */
	msg_putchar('\n');
	out_flush();
	cmdline_row = msg_row;
	msg_didany = FALSE;		/* lines_left will be set again */
	msg_start();			/* prepare for paging */
#ifdef WILDMENU
    }
#endif

    if (got_int)
	got_int = FALSE;	/* only int. the completion, not the cmd line */
#ifdef WILDMENU
    else if (wildmenu)
	win_redr_status_matches(num_files, files_found, 0);
#endif
    else
    {
	/* find the length of the longest file name */
	maxlen = 0;
	for (i = 0; i < num_files; ++i)
	{
	    if (expand_context == EXPAND_FILES
					  || expand_context == EXPAND_BUFFERS)
	    {
		home_replace(NULL, files_found[i], NameBuff, MAXPATHL, TRUE);
		j = vim_strsize(NameBuff);
	    }
	    else
		j = vim_strsize(files_found[i]);
	    if (j > maxlen)
		maxlen = j;
	}

	if (expand_context == EXPAND_TAGS_LISTFILES)
	    lines = num_files;
	else
	{
	    /* compute the number of columns and lines for the listing */
	    maxlen += 2;    /* two spaces between file names */
	    columns = ((int)Columns + 2) / maxlen;
	    if (columns < 1)
		columns = 1;
	    lines = (num_files + columns - 1) / columns;
	}

	attr = hl_attr(HLF_D);	/* find out highlighting for directories */

	if (expand_context == EXPAND_TAGS_LISTFILES)
	{
	    MSG_PUTS_ATTR("tagname", hl_attr(HLF_T));
	    msg_clr_eos();
	    msg_advance(maxlen - 3);
	    MSG_PUTS_ATTR(" kind file\n", hl_attr(HLF_T));
	}

	/* list the files line by line */
	for (i = 0; i < lines; ++i)
	{
	    lastlen = 999;
	    for (k = i; k < num_files; k += lines)
	    {
		if (expand_context == EXPAND_TAGS_LISTFILES)
		{
		    msg_outtrans_attr(files_found[k], hl_attr(HLF_D));
		    p = files_found[k] + STRLEN(files_found[k]) + 1;
		    msg_advance(maxlen + 1);
		    msg_puts(p);
		    msg_advance(maxlen + 3);
		    msg_puts_long_attr(p + 2, hl_attr(HLF_D));
		    break;
		}
		for (j = maxlen - lastlen; --j >= 0; )
		    msg_putchar(' ');
		if (expand_context == EXPAND_FILES
					  || expand_context == EXPAND_BUFFERS)
		{
			    /* highlight directories */
		    j = (mch_isdir(files_found[k]));
		    home_replace(NULL, files_found[k], NameBuff, MAXPATHL,
									TRUE);
		    p = NameBuff;
		}
		else
		{
		    j = FALSE;
		    p = files_found[k];
		}
		lastlen = msg_outtrans_attr(p, j ? attr : 0);
	    }
	    if (msg_col > 0)	/* when not wrapped around */
	    {
		msg_clr_eos();
		msg_putchar('\n');
	    }
	    out_flush();		    /* show one line at a time */
	    if (got_int)
	    {
		got_int = FALSE;
		break;
	    }
	}

	/*
	 * we redraw the command below the lines that we have just listed
	 * This is a bit tricky, but it saves a lot of screen updating.
	 */
	cmdline_row = msg_row;	/* will put it back later */
    }

    if (cmd_numfiles == -1)
    {
	vim_free(file_str);
	FreeWild(num_files, files_found);
    }

    return OK;
}

/*
 * Prepare a string for expansion.
 * When expanding file names: The string will be used with expand_wildcards().
 * Copy the file name into allocated memory and add a '*' at the end.
 * When expanding other names: The string will be used with regcomp().  Copy
 * the name into allocated memory and add ".*" at the end.
 */
    char_u *
addstar(fname, len, context)
    char_u	*fname;
    int		len;
    int		context;	/* EXPAND_FILES etc. */
{
    char_u	*retval;
    int		i, j;
    int		new_len;
    char_u	*tail;

    if (context != EXPAND_FILES && context != EXPAND_DIRECTORIES)
    {
	/*
	 * Matching will be done internally (on something other than files).
	 * So we convert the file-matching-type wildcards into our kind for
	 * use with vim_regcomp().  First work out how long it will be:
	 */

	/* for help tags the translation is done in find_help_tags() */
	if (context == EXPAND_HELP)
	    retval = vim_strnsave(fname, len);
	else
	{
	    new_len = len + 2;		/* +2 for '^' at start, NUL at end */
	    for (i = 0; i < len; i++)
	    {
		if (fname[i] == '*' || fname[i] == '~')
		    new_len++;		/* '*' needs to be replaced by ".*"
					   '~' needs to be replaced by "\~" */

		/* Buffer names are like file names.  "." should be literal */
		if (context == EXPAND_BUFFERS && fname[i] == '.')
		    new_len++;		/* "." becomes "\." */
	    }
	    retval = alloc(new_len);
	    if (retval != NULL)
	    {
		retval[0] = '^';
		j = 1;
		for (i = 0; i < len; i++, j++)
		{
		    if (fname[i] == '\\' && ++i == len)	/* skip backslash */
			break;

		    switch (fname[i])
		    {
			case '*':   retval[j++] = '.';
				    break;
			case '~':   retval[j++] = '\\';
				    break;
			case '?':   retval[j] = '.';
				    continue;
			case '.':   if (context == EXPAND_BUFFERS)
					retval[j++] = '\\';
				    break;
		    }
		    retval[j] = fname[i];
		}
		retval[j] = NUL;
	    }
	}
    }
    else
    {
	retval = alloc(len + 4);
	if (retval != NULL)
	{
	    STRNCPY(retval, fname, len);
	    retval[len] = NUL;

	    /*
	     * Don't add a star to ~, ~user, $var or `cmd`.
	     * ~ would be at the start of the tail.
	     * $ could be anywhere in the tail.
	     * ` could be anywhere in the file name.
	     */
	    tail = gettail(retval);
	    if (*tail != '~' && vim_strchr(tail, '$') == NULL
					   && vim_strchr(retval, '`') == NULL)
	    {
#ifdef MSDOS
		/*
		 * if there is no dot in the file name, add "*.*" instead of
		 * "*".
		 */
		for (i = len - 1; i >= 0; --i)
		    if (vim_strchr((char_u *)".\\/:", retval[i]) != NULL)
			break;
		if (i < 0 || retval[i] != '.')
		{
		    retval[len++] = '*';
		    retval[len++] = '.';
		}
#endif
		retval[len++] = '*';
	    }
	    retval[len] = NUL;
	}
    }
    return retval;
}

/*
 * Must parse the command line so far to work out what context we are in.
 * Completion can then be done based on that context.
 * This routine sets two global variables:
 *  char_u *expand_pattern  The start of the pattern to be expanded within
 *				the command line (ends at the cursor).
 *  int expand_context	    The type of thing to expand.  Will be one of:
 *
 *  EXPAND_UNSUCCESSFUL	    Used sometimes when there is something illegal on
 *			    the command line, like an unknown command.	Caller
 *			    should beep.
 *  EXPAND_NOTHING	    Unrecognised context for completion, use char like
 *			    a normal char, rather than for completion.	eg
 *			    :s/^I/
 *  EXPAND_COMMANDS	    Cursor is still touching the command, so complete
 *			    it.
 *  EXPAND_BUFFERS	    Complete file names for :buf and :sbuf commands.
 *  EXPAND_FILES	    After command with XFILE set, or after setting
 *			    with P_EXPAND set.	eg :e ^I, :w>>^I
 *  EXPAND_DIRECTORIES	    In some cases this is used instead of the latter
 *			    when we know only directories are of interest.  eg
 *			    :set dir=^I
 *  EXPAND_SETTINGS	    Complete variable names.  eg :set d^I
 *  EXPAND_BOOL_SETTINGS    Complete boolean variables only,  eg :set no^I
 *  EXPAND_TAGS		    Complete tags from the files in p_tags.  eg :ta a^I
 *  EXPAND_TAGS_LISTFILES   As above, but list filenames on ^D, after :tselect
 *  EXPAND_HELP		    Complete tags from the file 'helpfile'/tags
 *  EXPAND_EVENTS	    Complete event names
 *  EXPAND_SYNTAX	    Complete :syntax command arguments
 *  EXPAND_HIGHLIGHT	    Complete highlight (syntax) group names
 *  EXPAND_AUGROUP	    Complete autocommand group names
 *  EXPAND_USER_VARS	    Complete user defined variable names, eg :unlet a^I
 *  EXPAND_MAPPINGS	    Complete mapping and abbreviation names,
 *			      eg :unmap a^I , :cunab x^I
 *  EXPAND_FUNCTIONS	    Complete internal or user defined function names,
 *			      eg :call sub^I
 *  EXPAND_USER_FUNC	    Complete user defined function names, eg :delf F^I
 *  EXPAND_EXPRESSION	    Complete internal or user defined function/variable
 *			    names in expressions, eg :while s^I
 *
 * -- webb.
 */
    static void
set_expand_context()
{
    char_u	*nextcomm;
    int		old_char = NUL;

    if (ccline.cmdfirstc != ':')	/* only expansion for ':' commands */
    {
	expand_context = EXPAND_NOTHING;
	return;
    }

    /*
     * Avoid a UMR warning from Purify, only save the character if it has been
     * written before.
     */
    if (ccline.cmdpos < ccline.cmdlen)
	old_char = ccline.cmdbuff[ccline.cmdpos];
    ccline.cmdbuff[ccline.cmdpos] = NUL;
    nextcomm = ccline.cmdbuff;
    while (nextcomm != NULL)
	nextcomm = set_one_cmd_context(nextcomm);
    ccline.cmdbuff[ccline.cmdpos] = old_char;
}

/*
 * Do the expansion based on the global variables expand_context and
 * expand_pattern -- webb.
 */
    static int
ExpandFromContext(pat, num_file, file, files_only, options)
    char_u  *pat;
    int	    *num_file;
    char_u  ***file;
    int	    files_only;
    int	    options;
{
#ifdef CMDLINE_COMPL
    vim_regexp	*prog;
#endif
    int		ret;
    int		flags = 0;

    if (!files_only)
	flags |= EW_DIR;
    if (options & WILD_LIST_NOTFOUND)
	flags |= EW_NOTFOUND;
    if (options & WILD_ADD_SLASH)
	flags |= EW_ADDSLASH;
    if (options & WILD_KEEP_ALL)
	flags |= EW_KEEPALL;
    if (options & WILD_SILENT)
	flags |= EW_SILENT;

    if (expand_context == EXPAND_FILES)
    {
	/*
	 * Expand file names.
	 */
	return expand_wildcards(1, &pat, num_file, file, flags|EW_FILE);
    }
    else if (expand_context == EXPAND_DIRECTORIES)
    {
	/*
	 * Expand directory names.
	 */
	int	free_pat = FALSE;
	int	i;

	/* for ":set path=" we need to remove backslashes for escaped space */
	if (expand_set_path)
	{
	    free_pat = TRUE;
	    pat = vim_strsave(pat);
	    for (i = 0; pat[i]; ++i)
		if (pat[i] == '\\'
			&& pat[i + 1] == '\\'
			&& pat[i + 2] == '\\'
			&& pat[i + 3] == ' ')
		    STRCPY(pat + i, pat + i + 3);
	}

	ret = expand_wildcards(1, &pat, num_file, file,
						 (flags | EW_DIR) & ~EW_FILE);
	if (free_pat)
	    vim_free(pat);
	return ret;
    }

    *file = (char_u **)"";
    *num_file = 0;
    if (expand_context == EXPAND_HELP)
	return find_help_tags(pat, num_file, file);

#ifndef CMDLINE_COMPL
    return FAIL;
#else
    if (expand_context == EXPAND_OLD_SETTING)
	return ExpandOldSetting(num_file, file);

    set_reg_ic(pat);	    /* set reg_ic according to p_ic, p_scs and pat */

    if (expand_context == EXPAND_BUFFERS)
	return ExpandBufnames(pat, num_file, file, options);
    else if (expand_context == EXPAND_TAGS
	    || expand_context == EXPAND_TAGS_LISTFILES)
	return expand_tags(expand_context == EXPAND_TAGS, pat, num_file, file);

    prog = vim_regcomp(pat, (int)p_magic);
    if (prog == NULL)
	return FAIL;

    if (expand_context == EXPAND_COMMANDS)
	ret = ExpandGeneric(prog, num_file, file, get_command_name);
    else if (expand_context == EXPAND_SETTINGS ||
				       expand_context == EXPAND_BOOL_SETTINGS)
	ret = ExpandSettings(prog, num_file, file);
#ifdef USER_COMMANDS
    else if (expand_context == EXPAND_USER_COMMANDS)
	ret = ExpandGeneric(prog, num_file, file, get_user_commands);
    else if (expand_context == EXPAND_USER_CMD_FLAGS)
	ret = ExpandGeneric(prog, num_file, file, get_user_cmd_flags);
    else if (expand_context == EXPAND_USER_NARGS)
	ret = ExpandGeneric(prog, num_file, file, get_user_cmd_nargs);
    else if (expand_context == EXPAND_USER_COMPLETE)
	ret = ExpandGeneric(prog, num_file, file, get_user_cmd_complete);
#endif
#ifdef WANT_EVAL
    else if (expand_context == EXPAND_USER_VARS)
	ret = ExpandGeneric(prog, num_file, file, get_user_var_name);
    else if (expand_context == EXPAND_FUNCTIONS)
	ret = ExpandGeneric(prog, num_file, file, get_function_name);
    else if (expand_context == EXPAND_USER_FUNC)
	ret = ExpandGeneric(prog, num_file, file, get_user_func_name);
    else if (expand_context == EXPAND_EXPRESSION)
	ret = ExpandGeneric(prog, num_file, file, get_expr_name);
#endif
    else if (expand_context == EXPAND_MAPPINGS)
	ret = ExpandMappings(prog, num_file, file);
#ifdef WANT_MENU
    else if (expand_context == EXPAND_MENUS)
	ret = ExpandGeneric(prog, num_file, file, get_menu_name);
    else if (expand_context == EXPAND_MENUNAMES)
	ret = ExpandGeneric(prog, num_file, file, get_menu_names);
#endif
#ifdef SYNTAX_HL
    else if (expand_context == EXPAND_SYNTAX)
    {
	reg_ic = TRUE;
	ret = ExpandGeneric(prog, num_file, file, get_syntax_name);
    }
#endif
    else if (expand_context == EXPAND_HIGHLIGHT)
    {
	reg_ic = TRUE;
	ret = ExpandGeneric(prog, num_file, file, get_highlight_name);
    }
#ifdef AUTOCMD
    else if (expand_context == EXPAND_EVENTS)
    {
	reg_ic = TRUE;
	ret = ExpandGeneric(prog, num_file, file, get_event_name);
    }
    else if (expand_context == EXPAND_AUGROUP)
    {
	reg_ic = TRUE;
	ret = ExpandGeneric(prog, num_file, file, get_augroup_name);
    }
#endif
    else
	ret = FAIL;

    vim_free(prog);
    return ret;
#endif /* CMDLINE_COMPL */
}

#if defined(CMDLINE_COMPL) || defined(PROTO)
/*
 * Expand a list of names.
 *
 * Generic function for command line completion.  It calls a function to
 * obtain strings, one by one.	The strings are matched against a regexp
 * program.  Matching strings are copied into an array, which is returned.
 *
 * Returns OK when no problems encountered, FAIL for error (out of memory).
 */
    int
ExpandGeneric(prog, num_file, file, func)
    vim_regexp	*prog;
    int		*num_file;
    char_u	***file;
    char_u	*((*func)__ARGS((int))); /* returns a string from the list */
{
    int i;
    int count = 0;
    int	loop;
    char_u  *str;

    /* do this loop twice:
     * loop == 0: count the number of matching names
     * loop == 1: copy the matching names into allocated memory
     */
    for (loop = 0; loop <= 1; ++loop)
    {
	for (i = 0; ; ++i)
	{
	    str = (*func)(i);
	    if (str == NULL)	    /* end of list */
		break;
	    if (*str == NUL)	    /* skip empty strings */
		continue;

	    if (vim_regexec(prog, str, TRUE))
	    {
		if (loop)
		{
		    str = vim_strsave_escaped(str, (char_u *)" \t\\.");
		    (*file)[count] = str;
#ifdef WANT_MENU
		    if (func == get_menu_names && str != NULL)
		    {
			/* test for separator added by get_menu_names() */
			str += STRLEN(str) - 1;
			if (*str == '\001')
			    *str = '.';
		    }
#endif
		}
		++count;
	    }
	}
	if (loop == 0)
	{
	    if (count == 0)
		return OK;
	    *num_file = count;
	    *file = (char_u **)alloc((unsigned)(count * sizeof(char_u *)));
	    if (*file == NULL)
	    {
		*file = (char_u **)"";
		return FAIL;
	    }
	    count = 0;
	}
    }
    return OK;
}
#endif

/*********************************
 *  Command line history stuff	 *
 *********************************/

/*
 * Translate a history character to the associated type number.
 */
    static int
hist_char2type(c)
    int	    c;
{
    if (c == ':')
	return HIST_CMD;
    if (c == '=')
	return HIST_EXPR;
    if (c == '@')
	return HIST_INPUT;
    return HIST_SEARCH;	    /* must be '?' or '/' */
}

/*
 * Table of history names.
 * These names are used in :history and various hist...() functions.
 * It is sufficient to give the significant prefix of a history name.
 */

static char *(history_names[]) =
{
    "cmd",
    "search",
    "expr",
    "input",
    NULL
};

/*
 * init_history() - Initialize the command line history.
 * Also used to re-allocate the history when the size changes.
 */
    static void
init_history()
{
    int			newlen;	    /* new length of history table */
    struct hist_entry	*temp;
    int			i;
    int			j;
    int			type;

    /*
     * If size of history table changed, reallocate it
     */
    newlen = (int)p_hi;
    if (newlen != hislen)			/* history length changed */
    {
	for (type = 0; type < HIST_COUNT; ++type)   /* adjust the tables */
	{
	    if (newlen)
	    {
		temp = (struct hist_entry *)lalloc( (long_u)(newlen
					* sizeof(struct hist_entry)), TRUE);
		if (temp == NULL)   /* out of memory! */
		{
		    if (type == 0)  /* first one: just keep the old length */
		    {
			newlen = hislen;
			break;
		    }
		    /* Already changed one table, now we can only have zero
		     * length for all tables. */
		    newlen = 0;
		    type = -1;
		    continue;
		}
	    }
	    else
		temp = NULL;
	    if (newlen == 0 || temp != NULL)
	    {
		if (hisidx[type] < 0)		/* there are no entries yet */
		{
		    for (i = 0; i < newlen; ++i)
		    {
			temp[i].hisnum = 0;
			temp[i].hisstr = NULL;
		    }
		}
		else if (newlen > hislen)	/* array becomes bigger */
		{
		    for (i = 0; i <= hisidx[type]; ++i)
			temp[i] = history[type][i];
		    j = i;
		    for ( ; i <= newlen - (hislen - hisidx[type]); ++i)
		    {
			temp[i].hisnum = 0;
			temp[i].hisstr = NULL;
		    }
		    for ( ; j < hislen; ++i, ++j)
			temp[i] = history[type][j];
		}
		else				/* array becomes smaller or 0 */
		{
		    j = hisidx[type];
		    for (i = newlen - 1; ; --i)
		    {
			if (i >= 0)		/* copy newest entries */
			    temp[i] = history[type][j];
			else			/* remove older entries */
			    vim_free(history[type][j].hisstr);
			if (--j < 0)
			    j = hislen - 1;
			if (j == hisidx[type])
			    break;
		    }
		    hisidx[type] = newlen - 1;
		}
		vim_free(history[type]);
		history[type] = temp;
	    }
	}
	hislen = newlen;
    }
}

/*
 * Check if command line 'str' is already in history.
 * If 'move_to_front' is TRUE, matching entry is moved to end of history.
 */
    static int
in_history(type, str, move_to_front)
    int	    type;
    char_u  *str;
    int	    move_to_front;	/* Move the entry to the front if it exists */
{
    int	    i;
    int	    last_i = -1;

    if (hisidx[type] < 0)
	return FALSE;
    i = hisidx[type];
    do
    {
	if (history[type][i].hisstr == NULL)
	    return FALSE;
	if (STRCMP(str, history[type][i].hisstr) == 0)
	{
	    if (!move_to_front)
		return TRUE;
	    last_i = i;
	    break;
	}
	if (--i < 0)
	    i = hislen - 1;
    } while (i != hisidx[type]);

    if (last_i >= 0)
    {
	str = history[type][i].hisstr;
	while (i != hisidx[type])
	{
	    if (++i >= hislen)
		i = 0;
	    history[type][last_i] = history[type][i];
	    last_i = i;
	}
	history[type][i].hisstr = str;
	history[type][i].hisnum = ++hisnum[type];
	return TRUE;
    }
    return FALSE;
}

/*
 * Convert history name (from table above) to its number equivalent:
 * HIST_CMD, HIST_SEARCH, HIST_EXPR, or HIST_INPUT.
 * Returns -1 for unknown history name.
 */
    int
get_histtype(name)
    char_u	*name;
{
    int		i;
    int		len = STRLEN(name);

    for (i = 0; history_names[i] != NULL; ++i)
	if (STRNICMP(name, history_names[i], len) == 0)
	    return i;

    if (vim_strchr((char_u *)":=@?/", name[0]) != NULL && name[1] == NUL)
	return hist_char2type(name[0]);

    return -1;
}

static int	last_maptick = -1;	/* last seen maptick */

/*
 * Add the given string to the given history.  If the string is already in the
 * history then it is moved to the front.  "histype" may be HIST_CMD,
 * HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    void
add_to_history(histype, new_entry, in_map)
    int		histype;
    char_u	*new_entry;
    int		in_map;		/* consider maptick when inside a mapping */
{
    struct hist_entry	*hisptr;

    if (hislen == 0)		/* no history */
	return;

    /*
     * Searches inside the same mapping overwrite each other, so that only
     * the last line is kept.  Be careful not to remove a line that was moved
     * down, only lines that were added.
     */
    if (histype == HIST_SEARCH && in_map)
    {
	if (maptick == last_maptick)
	{
	    /* Current line is from the same mapping, remove it */
	    hisptr = &history[HIST_SEARCH][hisidx[HIST_SEARCH]];
	    vim_free(hisptr->hisstr);
	    hisptr->hisstr = NULL;
	    hisptr->hisnum = 0;
	    --hisnum[histype];
	    if (--hisidx[HIST_SEARCH] < 0)
		hisidx[HIST_SEARCH] = hislen - 1;
	}
	last_maptick = -1;
    }
    if (!in_history(histype, new_entry, TRUE))
    {
	if (++hisidx[histype] == hislen)
	    hisidx[histype] = 0;
	hisptr = &history[histype][hisidx[histype]];
	vim_free(hisptr->hisstr);
	hisptr->hisstr = vim_strsave(new_entry);
	hisptr->hisnum = ++hisnum[histype];
	if (histype == HIST_SEARCH && in_map)
	    last_maptick = maptick;
    }
}

#if defined(WANT_EVAL) || defined(PROTO)

/*
 * Get identifier of newest history entry.
 * "histype" may be HIST_CMD, HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    int
get_history_idx(histype)
    int	    histype;
{
    if (hislen == 0 || histype < 0 || histype >= HIST_COUNT
		    || hisidx[histype] < 0)
	return -1;

    return history[histype][hisidx[histype]].hisnum;
}

/*
 * Calculate history index from a number:
 *   num > 0: seen as identifying number of a history entry
 *   num < 0: relative position in history wrt newest entry
 * "histype" may be HIST_CMD, HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    static int
calc_hist_idx(histype, num)
    int	    histype;
    int	    num;
{
    int			i;
    struct hist_entry	*hist;

    if (hislen == 0 || histype < 0 || histype >= HIST_COUNT
		    || (i = hisidx[histype]) < 0 || num == 0)
	return -1;

    hist = history[histype];
    if (num > 0)
    {
	while (hist[i].hisnum > num)
	    if (--i < 0)
		i += hislen;
	if (hist[i].hisnum == num
		&& hist[i].hisstr != NULL)
	    return i;
    }
    else if (-num <= hislen)
    {
	i += num + 1;
	if (i < 0)
	    i += hislen;
	if (hist[i].hisstr != NULL)
	    return i;
    }
    return -1;
}

/*
 * Get a history entry by its index.
 * "histype" may be HIST_CMD, HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    char_u*
get_history_entry(histype, idx)
    int	    histype;
    int	    idx;
{
    idx = calc_hist_idx(histype, idx);
    if (idx >= 0)
	return history[histype][idx].hisstr;
    else
	return (char_u *)"";
}

/*
 * Clear all entries of a history.
 * "histype" may be HIST_CMD, HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    int
clr_history(histype)
    int	    histype;
{
    int			i;
    struct hist_entry	*hisptr;

    if (hislen != 0 && histype >= 0 && histype < HIST_COUNT)
    {
	hisptr = history[histype];
	for (i = hislen; i--;)
	{
	    vim_free(hisptr->hisstr);
	    hisptr->hisnum = 0;
	    hisptr++->hisstr = NULL;
	}
	hisidx[histype] = -1;	/* mark history as cleared */
	hisnum[histype] = 0;	/* reset identifier counter */
	return OK;
    }
    return FAIL;
}

/*
 * Remove all entries matching {str} from a history.
 * "histype" may be HIST_CMD, HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    int
del_history_entry(histype, str)
    int		histype;
    char_u	*str;
{
    vim_regexp		*prog = NULL;
    struct hist_entry	*hisptr;
    int			idx;
    int			i;
    int			last;
    int			found = FALSE;

    if (hislen != 0 && histype >= 0 && histype < HIST_COUNT && *str != NUL
	&& (idx = hisidx[histype]) >= 0 && (prog = vim_regcomp(str, TRUE)))
    {
	i = last = idx;
	do
	{
	    hisptr = &history[histype][i];
	    if (hisptr->hisstr == NULL)
		break;
	    if (vim_regexec(prog, hisptr->hisstr, TRUE))
	    {
		found = TRUE;
		vim_free(hisptr->hisstr);
		hisptr->hisstr = NULL;
		hisptr->hisnum = 0;
	    }
	    else
	    {
		if (i != last)
		{
		    history[histype][last] = *hisptr;
		    hisptr->hisstr = NULL;
		    hisptr->hisnum = 0;
		}
		if (--last < 0)
		    last += hislen;
	    }
	    if (--i < 0)
		i += hislen;
	} while (i != idx);
	if (history[histype][idx].hisstr == NULL)
	    hisidx[histype] = -1;
    }
    if (prog)
	vim_free(prog);
    return found;
}

/*
 * Remove an indexed entry from a history.
 * "histype" may be HIST_CMD, HIST_SEARCH, HIST_EXPR or HIST_INPUT.
 */
    int
del_history_idx(histype, idx)
    int	    histype;
    int	    idx;
{
    int	    i, j;

    i = calc_hist_idx(histype, idx);
    if (i < 0)
	return FALSE;
    idx = hisidx[histype];
    vim_free(history[histype][i].hisstr);

    /* When deleting the last added search string in a mapping, reset
     * last_maptick, so that the last added search string isn't deleted again.
     */
    if (histype == HIST_SEARCH && maptick == last_maptick && i == idx)
	last_maptick = -1;

    while (i != idx)
    {
	j = (i + 1) % hislen;
	history[histype][i] = history[histype][j];
	i = j;
    }
    history[histype][i].hisstr = NULL;
    history[histype][i].hisnum = 0;
    if (--i < 0)
	i += hislen;
    hisidx[histype] = i;
    return TRUE;
}

#endif /* WANT_EVAL */

#if defined(CRYPTV) || defined(PROTO)
/*
 * Very specific function to remove the value in ":set key=val" from the
 * history.
 */
    void
remove_key_from_history()
{
    char_u	*p;
    int		i;

    i = hisidx[HIST_CMD];
    if (i < 0)
	return;
    p = history[HIST_CMD][i].hisstr;
    if (p != NULL)
	for ( ; *p; ++p)
	    if (STRNCMP(p, "key", 3) == 0 && !islower(p[3]))
	    {
		p = vim_strchr(p + 3, '=');
		if (p == NULL)
		    break;
		++p;
		for (i = 0; p[i] && !vim_iswhite(p[i]); ++i)
		    if (p[i] == '\\' && p[i + 1])
			++i;
		mch_memmove(p, p + i, STRLEN(p + i) + 1);
		--p;
	    }
}
#endif

/*
 * Get indices "num1,num2" that specify a range within a list (not a range of
 * text lines in a buffer!) from a string.  Used for ":history" and ":clist".
 * Returns OK if parsed successfully, otherwise FAIL.
 */
    int
get_list_range(str, num1, num2)
    char_u	**str;
    int		*num1;
    int		*num2;
{
    int		len;
    int		first = FALSE;
    long	num;

    *str = skipwhite(*str);
    if (**str == '-' || isdigit(**str))	/* parse "from" part of range */
    {
	vim_str2nr(*str, NULL, &len, FALSE, FALSE, &num, NULL);
	*str += len;
	*num1 = (int)num;
	first = TRUE;
    }
    *str = skipwhite(*str);
    if (**str == ',')			/* parse "to" part of range */
    {
	*str = skipwhite(*str + 1);
	vim_str2nr(*str, NULL, &len, FALSE, FALSE, &num, NULL);
	if (len > 0)
	{
	    *num2 = (int)num;
	    *str = skipwhite(*str + len);
	}
	else if (!first)		/* no number given at all */
	    return FAIL;
    }
    else if (first)			/* only one number given */
	*num2 = *num1;
    return OK;
}

/*
 * :history command - print a history
 */
    void
do_history(arg)
    char_u  *arg;
{
    struct hist_entry	*hist;
    int			histype1 = HIST_CMD;
    int			histype2 = HIST_CMD;
    int			hisidx1 = 1;
    int			hisidx2 = -1;
    int			idx;
    int			i, j, k;
    char_u		*end;

    if (hislen == 0)
    {
	MSG("'history' option is zero");
	return;
    }

    if (!(isdigit(*arg) || *arg == '-' || *arg == ','))
    {
	end = arg;
	while (isalpha(*end) || vim_strchr((char_u *)":=@/?", *end) != NULL)
	    end++;
	i = *end;
	*end = NUL;
	histype1 = get_histtype(arg);
	if (histype1 == -1)
	{
	    if (STRNICMP(arg, "all", STRLEN(arg)) == 0)
	    {
		histype1 = 0;
		histype2 = HIST_COUNT-1;
	    }
	    else
	    {
		*end = i;
		emsg(e_trailing);
		return;
	    }
	}
	else
	    histype2 = histype1;
	*end = i;
    }
    else
	end = arg;
    if (!get_list_range(&end, &hisidx1, &hisidx2) || *end != NUL)
    {
	emsg(e_trailing);
	return;
    }

    for (; !got_int && histype1 <= histype2; ++histype1)
    {
	STRCPY(IObuff, "\n      #  ");
	STRCAT(STRCAT(IObuff, history_names[histype1]), " history");
	MSG_PUTS_TITLE(IObuff);
	idx = hisidx[histype1];
	hist = history[histype1];
	j = hisidx1;
	k = hisidx2;
	if (j < 0)
	    j = (-j > hislen) ? 0 : hist[(hislen+j+idx+1) % hislen].hisnum;
	if (k < 0)
	    k = (-k > hislen) ? 0 : hist[(hislen+k+idx+1) % hislen].hisnum;
	if (idx >= 0 && j <= k)
	    for (i = idx + 1; !got_int; ++i)
	    {
		if (i == hislen)
		    i = 0;
		if (hist[i].hisstr != NULL
		    && hist[i].hisnum >= j && hist[i].hisnum <= k)
		{
		    msg_putchar('\n');
		    sprintf((char *)IObuff, "%c%6d  %s", i == idx ? '>' : ' ',
					    hist[i].hisnum, hist[i].hisstr);
		    msg_outtrans(IObuff);
		    out_flush();
		}
		if (i == idx)
		    break;
	    }
    }
}

#ifdef VIMINFO
static char_u **viminfo_history[HIST_COUNT] = {NULL, NULL, NULL, NULL};
static int	viminfo_hisidx[HIST_COUNT] = {0, 0, 0, 0};
static int	viminfo_hislen[HIST_COUNT] = {0, 0, 0, 0};
static int	viminfo_add_at_front = FALSE;

static int	hist_type2char __ARGS((int type, int use_question));

/*
 * Translate a history type number to the associated character.
 */
    static int
hist_type2char(type, use_question)
    int	    type;
    int	    use_question;	    /* use '?' instead of '/' */
{
    if (type == HIST_CMD)
	return ':';
    if (type == HIST_SEARCH)
    {
	if (use_question)
	    return '?';
	else
	    return '/';
    }
    if (type == HIST_EXPR)
	return '=';
    return '@';
}

/*
 * Prepare for reading the history from the viminfo file.
 * This allocates history arrays to store the read history lines.
 */
    void
prepare_viminfo_history(asklen)
    int	    asklen;
{
    int	    i;
    int	    num;
    int	    type;
    int	    len;

    init_history();
    viminfo_add_at_front = (asklen != 0);
    if (asklen > hislen)
	asklen = hislen;

    for (type = 0; type < HIST_COUNT; ++type)
    {
	/*
	 * Count the number of empty spaces in the history list.  If there are
	 * more spaces available than we request, then fill them up.
	 */
	for (i = 0, num = 0; i < hislen; i++)
	    if (history[type][i].hisstr == NULL)
		num++;
	len = asklen;
	if (num > len)
	    len = num;
	if (len <= 0)
	    viminfo_history[type] = NULL;
	else
	    viminfo_history[type] =
		   (char_u **)lalloc((long_u)(len * sizeof(char_u *)), FALSE);
	if (viminfo_history[type] == NULL)
	    len = 0;
	viminfo_hislen[type] = len;
	viminfo_hisidx[type] = 0;
    }
}

/*
 * Accept a line from the viminfo, store it in the history array when it's
 * new.
 */
    int
read_viminfo_history(line, fp)
    char_u  *line;
    FILE    *fp;
{
    int	    type;
    char_u  *val;

    type = hist_char2type(line[0]);
    if (viminfo_hisidx[type] < viminfo_hislen[type])
    {
	val = viminfo_readstring(line + 1, fp);
	if (val != NULL)
	{
	    if (!in_history(type, val, viminfo_add_at_front))
		viminfo_history[type][viminfo_hisidx[type]++] = val;
	    else
		vim_free(val);
	}
    }
    return vim_fgets(line, LSIZE, fp);
}

    void
finish_viminfo_history()
{
    int idx;
    int i;
    int	type;

    for (type = 0; type < HIST_COUNT; ++type)
    {
	if (history[type] == NULL)
	    return;
	idx = hisidx[type] + viminfo_hisidx[type];
	if (idx >= hislen)
	    idx -= hislen;
	else if (idx < 0)
	    idx = hislen - 1;
	if (viminfo_add_at_front)
	    hisidx[type] = idx;
	else
	{
	    if (hisidx[type] == -1)
		hisidx[type] = hislen - 1;
	    do
	    {
		if (history[type][idx].hisstr != NULL)
		    break;
		if (++idx == hislen)
		    idx = 0;
	    } while (idx != hisidx[type]);
	    if (idx != hisidx[type] && --idx < 0)
		idx = hislen - 1;
	}
	for (i = 0; i < viminfo_hisidx[type]; i++)
	{
	    vim_free(history[type][idx].hisstr);
	    history[type][idx].hisstr = viminfo_history[type][i];
	    if (--idx < 0)
		idx = hislen - 1;
	}
	idx += 1;
	idx %= hislen;
	for (i = 0; i < viminfo_hisidx[type]; i++)
	{
	    history[type][idx++].hisnum = ++hisnum[type];
	    idx %= hislen;
	}
	vim_free(viminfo_history[type]);
	viminfo_history[type] = NULL;
    }
}

    void
write_viminfo_history(fp)
    FILE    *fp;
{
    int	    i;
    int	    type;
    int	    num_saved;

    init_history();
    if (hislen == 0)
	return;
    for (type = 0; type < HIST_COUNT; ++type)
    {
	num_saved = get_viminfo_parameter(hist_type2char(type, FALSE));
	if (num_saved == 0)
	    continue;
	if (num_saved < 0)  /* Use default */
	    num_saved = hislen;
	fprintf(fp, "\n# %s History (newest to oldest):\n",
			    type == HIST_CMD ? "Command Line" :
			    type == HIST_SEARCH ? "Search String" :
			    type == HIST_EXPR ?  "Expression" :
					"Input Line");
	if (num_saved > hislen)
	    num_saved = hislen;
	i = hisidx[type];
	if (i >= 0)
	    while (num_saved--)
	    {
		if (history[type][i].hisstr != NULL)
		{
		    putc(hist_type2char(type, TRUE), fp);
		    viminfo_writestring(fp, history[type][i].hisstr);
		}
		if (--i < 0)
		    i = hislen - 1;
	    }
    }
}
#endif /* VIMINFO */

#if defined(FKMAP) || defined(PROTO)
/*
 * Write a character at the current cursor+offset position.
 * It is directly written into the command buffer block.
 */
    void
cmd_pchar(c, offset)
    int	    c, offset;
{
    if (ccline.cmdpos + offset >= ccline.cmdlen || ccline.cmdpos + offset < 0)
    {
	EMSG("cmd_pchar beyond the command length");
	return;
    }
    ccline.cmdbuff[ccline.cmdpos + offset] = (char_u)c;
}

    int
cmd_gchar(offset)
    int	    offset;
{
    if (ccline.cmdpos + offset >= ccline.cmdlen || ccline.cmdpos + offset < 0)
    {
	/*  EMSG("cmd_gchar beyond the command length"); */
	return NUL;
    }
    return (int)ccline.cmdbuff[ccline.cmdpos + offset];
}
#endif
