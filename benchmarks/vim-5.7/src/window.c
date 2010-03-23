/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include "vim.h"

#ifdef FILE_IN_PATH
static char_u *find_file_in_wildcard_path __ARGS((char_u *path_so_far, char_u *wildcards, int level, long *countptr));
static int path_is_url __ARGS((char_u *p));
#endif
static void reset_VIsual __ARGS((void));
static int win_comp_pos __ARGS((void));
static void win_exchange __ARGS((long));
static void win_rotate __ARGS((int, int));
static void win_goto __ARGS((WIN *wp));
static void win_enter_ext __ARGS((WIN *wp, int undo_sync, int no_curwin));
static void win_append __ARGS((WIN *, WIN *));
static void win_remove __ARGS((WIN *));
static void win_new_height __ARGS((WIN *, int));

static WIN	*prevwin = NULL;	/* previous window */

#define URL_SLASH	1		/* path_is_url() has found "://" */
#define URL_BACKSLASH	2		/* path_is_url() has found ":\\" */

/*
 * all CTRL-W window commands are handled here, called from normal_cmd().
 */
    void
do_window(nchar, Prenum)
    int		nchar;
    long	Prenum;
{
    long	Prenum1;
    WIN		*wp;
    int		xchar;
#if defined(FILE_IN_PATH) || defined(FIND_IN_PATH)
    char_u	*ptr;
#endif
#ifdef FIND_IN_PATH
    int		type = FIND_DEFINE;
    int		len;
#endif

    if (Prenum == 0)
	Prenum1 = 1;
    else
	Prenum1 = Prenum;

    switch (nchar)
    {
/* split current window in two parts */
    case 'S':
    case Ctrl('S'):
    case 's':	reset_VIsual();			/* stop Visual mode */
#ifdef USE_GUI
		need_mouse_correct = TRUE;
#endif
		win_split((int)Prenum, TRUE, FALSE);
		break;

/* split current window and edit alternate file */
    case K_CCIRCM:
    case '^':
		reset_VIsual();			/* stop Visual mode */
		stuffReadbuff((char_u *)":split #");
		if (Prenum)
		    stuffnumReadbuff(Prenum);	/* buffer number */
		stuffcharReadbuff('\n');
		break;

/* open new window */
    case Ctrl('N'):
    case 'n':	reset_VIsual();			/* stop Visual mode */
		stuffcharReadbuff(':');
		if (Prenum)
		    stuffnumReadbuff(Prenum);	/* window height */
		stuffReadbuff((char_u *)"new\n");
		break;

/* quit current window */
    case Ctrl('Q'):
    case 'q':	reset_VIsual();			/* stop Visual mode */
		stuffReadbuff((char_u *)":quit\n");
		break;

/* close current window */
    case Ctrl('C'):
    case 'c':	reset_VIsual();			/* stop Visual mode */
		stuffReadbuff((char_u *)":close\n");
		break;

/* close preview window */
    case Ctrl('Z'):
    case 'z':	reset_VIsual();			/* stop Visual mode */
		stuffReadbuff((char_u *)":pclose\n");
		break;

/* close all but current window */
    case Ctrl('O'):
    case 'o':	reset_VIsual();			/* stop Visual mode */
		stuffReadbuff((char_u *)":only\n");
		break;

/* cursor to next window */
    case 'j':
    case K_DOWN:
    case Ctrl('J'):
		for (wp = curwin; wp->w_next != NULL && Prenum1-- > 0;
							    wp = wp->w_next)
		    ;
		win_goto(wp);
		break;

/* cursor to next window with wrap around */
    case Ctrl('W'):
    case 'w':
/* cursor to previous window with wrap around */
    case 'W':
		if (lastwin == firstwin)	/* just one window */
		    beep_flush();
		else
		{
		    if (Prenum)			/* go to specified window */
		    {
			for (wp = firstwin; --Prenum > 0; )
			{
			    if (wp->w_next == NULL)
				break;
			    else
				wp = wp->w_next;
			}
		    }
		    else
		    {
			if (nchar == 'W')	    /* go to previous window */
			{
			    wp = curwin->w_prev;
			    if (wp == NULL)
				wp = lastwin;	    /* wrap around */
			}
			else			    /* go to next window */
			{
			    wp = curwin->w_next;
			    if (wp == NULL)
				wp = firstwin;	    /* wrap around */
			}
		    }
		    win_goto(wp);
		}
		break;

/* cursor to window above */
    case 'k':
    case K_UP:
    case Ctrl('K'):
		for (wp = curwin; wp->w_prev != NULL && Prenum1-- > 0;
							    wp = wp->w_prev)
		    ;
		win_goto(wp);
		break;

/* cursor to top window */
    case 't':
    case Ctrl('T'):
		wp = firstwin;
		win_goto(wp);
		break;

/* cursor to bottom window */
    case 'b':
    case Ctrl('B'):
		wp = lastwin;
		win_goto(wp);
		break;

/* cursor to last accessed (previous) window */
    case 'p':
    case Ctrl('P'):
		if (prevwin == NULL)
		    beep_flush();
		else
		{
		    wp = prevwin;
		    win_goto(wp);
		}
		break;

/* exchange current and next window */
    case 'x':
    case Ctrl('X'):
		win_exchange(Prenum);
		break;

/* rotate windows downwards */
    case Ctrl('R'):
    case 'r':	reset_VIsual();			/* stop Visual mode */
		win_rotate(FALSE, (int)Prenum1);    /* downwards */
		break;

/* rotate windows upwards */
    case 'R':	reset_VIsual();			/* stop Visual mode */
		win_rotate(TRUE, (int)Prenum1);	    /* upwards */
		break;

/* make all windows the same height */
    case '=':
#ifdef USE_GUI
		need_mouse_correct = TRUE;
#endif
		win_equal(NULL, TRUE);
		break;

/* increase current window height */
    case '+':
#ifdef USE_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(curwin->w_height + (int)Prenum1);
		break;

/* decrease current window height */
    case '-':
#ifdef USE_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(curwin->w_height - (int)Prenum1);
		break;

/* set current window height */
    case Ctrl('_'):
    case '_':
#ifdef USE_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(Prenum ? (int)Prenum : 9999);
		break;

/* jump to tag and split window if tag exists */
    case '}':
		if (Prenum)
		    g_do_tagpreview = Prenum;
		else
		    g_do_tagpreview = p_pvh;
		/*FALLTHROUGH*/
    case ']':
    case Ctrl(']'):
		reset_VIsual();			/* stop Visual mode */
		if (Prenum)
		    postponed_split = Prenum;
		else
		    postponed_split = -1;
		stuffcharReadbuff(Ctrl(']'));
		break;

#ifdef FILE_IN_PATH
/* edit file name under cursor in a new window */
    case 'f':
    case Ctrl('F'):
		reset_VIsual();			/* stop Visual mode */
		ptr = file_name_at_cursor(FNAME_MESS|FNAME_HYP|FNAME_EXP,
								     Prenum1);
		if (ptr != NULL)
		{
#ifdef USE_GUI
		    need_mouse_correct = TRUE;
#endif
		    setpcmark();
		    if (win_split(0, FALSE, FALSE) == OK)
			(void)do_ecmd(0, ptr, NULL, NULL, ECMD_LASTL,
								   ECMD_HIDE);
		    vim_free(ptr);
		}
		break;
#endif

#ifdef FIND_IN_PATH
/* Go to the first occurence of the identifier under cursor along path in a
 * new window -- webb
 */
    case 'i':			    /* Go to any match */
    case Ctrl('I'):
		type = FIND_ANY;
		/* FALLTHROUGH */
    case 'd':			    /* Go to definition, using p_def */
    case Ctrl('D'):
		if ((len = find_ident_under_cursor(&ptr, FIND_IDENT)) == 0)
		    break;
		find_pattern_in_path(ptr, 0, len, TRUE,
			Prenum == 0 ? TRUE : FALSE, type,
			Prenum1, ACTION_SPLIT, (linenr_t)1, (linenr_t)MAXLNUM);
		curwin->w_set_curswant = TRUE;
		break;
#endif

/* CTRL-W g  extended commands */
    case 'g':
    case Ctrl('G'):
#ifdef USE_ON_FLY_SCROLL
		dont_scroll = TRUE;		/* disallow scrolling here */
#endif
		++no_mapping;
		++allow_keys;   /* no mapping for xchar, but allow key codes */
		xchar = safe_vgetc();
#ifdef HAVE_LANGMAP
		LANGMAP_ADJUST(xchar, TRUE);
#endif
		--no_mapping;
		--allow_keys;
#ifdef CMDLINE_INFO
		(void)add_to_showcmd(xchar);
#endif
		switch (xchar)
		{
		    case '}':
			xchar = Ctrl(']');
			if (Prenum)
			    g_do_tagpreview = Prenum;
			else
			    g_do_tagpreview = p_pvh;
			/*FALLTHROUGH*/
		    case ']':
		    case Ctrl(']'):
			reset_VIsual();			/* stop Visual mode */
			if (Prenum)
			    postponed_split = Prenum;
			else
			    postponed_split = -1;
			stuffcharReadbuff('g');
			stuffcharReadbuff(xchar);
			break;

		    default:
			beep_flush();
			break;
		}
		break;

    default:	beep_flush();
		break;
    }
}

    static void
reset_VIsual()
{
    if (VIsual_active)
    {
	end_visual_mode();
	update_curbuf(NOT_VALID);	/* delete the inversion */
    }
    VIsual_reselect = FALSE;
}

/*
 * split the current window, implements CTRL-W s and :split
 *
 * new_height is the height for the new window, 0 to make half of current
 * height
 * redraw is TRUE when redraw now
 *
 * return FAIL for failure, OK otherwise
 */
    int
win_split(new_height, redraw, req_room)
    int	    new_height;
    int	    redraw;
    int	    req_room;	    /* require enough room for new window */
{
    WIN		*wp;
    int		i;
    int		need_status;
    int		do_equal = (p_ea && new_height == 0);
    int		needed;
    int		available;
    int		curwin_height;

    /* add a status line when p_ls == 1 and splitting the first window */
    if (lastwin == firstwin && p_ls == 1 && curwin->w_status_height == 0)
	need_status = STATUS_HEIGHT;
    else
	need_status = 0;

/*
 * check if we are able to split the current window and compute its height
 */
    available = curwin->w_height;
    needed = 2 * p_wmh + STATUS_HEIGHT + need_status;
    if (req_room)
	needed += p_wh - p_wmh;
    if (p_ea)
    {
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	    if (wp != curwin)
	    {
		available += wp->w_height;
		needed += p_wmh;
	    }
    }
    if (available < needed)
    {
	EMSG(e_noroom);
	return FAIL;
    }
    curwin_height = curwin->w_height;
    if (need_status)
    {
	curwin->w_status_height = STATUS_HEIGHT;
	curwin_height -= STATUS_HEIGHT;
    }
    if (new_height == 0)
	new_height = curwin_height / 2;

    if (new_height > curwin_height - p_wmh - STATUS_HEIGHT)
	new_height = curwin_height - p_wmh - STATUS_HEIGHT;

    if (new_height < p_wmh)
	new_height = p_wmh;

    /* if it doesn't fit in the current window, need win_equal() */
    if (curwin_height - new_height - STATUS_HEIGHT < p_wmh)
	do_equal = TRUE;
/*
 * allocate new window structure and link it in the window list
 */
    if (p_sb)	    /* new window below current one */
	wp = win_alloc(curwin);
    else
	wp = win_alloc(curwin->w_prev);
    if (wp == NULL)
	return FAIL;
/*
 * compute the new screen positions
 */
    win_new_height(wp, new_height);
    win_new_height(curwin, curwin_height - (new_height + STATUS_HEIGHT));
    if (p_sb)	    /* new window below current one */
    {
	wp->w_winpos = curwin->w_winpos + curwin->w_height + STATUS_HEIGHT;
	wp->w_status_height = curwin->w_status_height;
	curwin->w_status_height = STATUS_HEIGHT;
    }
    else	    /* new window above current one */
    {
	wp->w_winpos = curwin->w_winpos;
	wp->w_status_height = STATUS_HEIGHT;
	curwin->w_winpos = wp->w_winpos + wp->w_height + STATUS_HEIGHT;
    }
/*
 * make the contents of the new window the same as the current one
 */
    wp->w_buffer = curbuf;
    curbuf->b_nwindows++;
    wp->w_cursor = curwin->w_cursor;
    wp->w_valid = 0;
    wp->w_curswant = curwin->w_curswant;
    wp->w_set_curswant = curwin->w_set_curswant;
    wp->w_topline = curwin->w_topline;
    wp->w_leftcol = curwin->w_leftcol;
    wp->w_pcmark = curwin->w_pcmark;
    wp->w_prev_pcmark = curwin->w_prev_pcmark;
    wp->w_alt_fnum = curwin->w_alt_fnum;
    wp->w_fraction = curwin->w_fraction;
    wp->w_prev_fraction_row = curwin->w_prev_fraction_row;

    wp->w_arg_idx = curwin->w_arg_idx;
    /*
     * copy tagstack and options from existing window
     */
    for (i = 0; i < curwin->w_tagstacklen; i++)
    {
	wp->w_tagstack[i] = curwin->w_tagstack[i];
	if (wp->w_tagstack[i].tagname != NULL)
	    wp->w_tagstack[i].tagname = vim_strsave(wp->w_tagstack[i].tagname);
    }
    wp->w_tagstackidx = curwin->w_tagstackidx;
    wp->w_tagstacklen = curwin->w_tagstacklen;
    win_copy_options(curwin, wp);
/*
 * Both windows need redrawing
 */
    wp->w_redr_type = NOT_VALID;
    wp->w_redr_status = TRUE;
    curwin->w_redr_type = NOT_VALID;
    curwin->w_redr_status = TRUE;

    if (need_status)
    {
	msg_row = Rows - 1;
	msg_col = sc_col;
	msg_clr_eos();	/* Old command/ruler may still be there -- webb */
	comp_col();
	msg_row = Rows - 1;
	msg_col = 0;	/* put position back at start of line */
    }
/*
 * make the new window the current window and redraw
 */
    if (do_equal)
	win_equal(wp, FALSE);
    win_enter(wp, FALSE);

    if (redraw)
	update_screen(NOT_VALID);
    else
	redraw_later(NOT_VALID);

    return OK;
}

/*
 * Check if "win" is a pointer to an existing window.
 */
    int
win_valid(win)
    WIN	    *win;
{
    WIN	    *wp;

    if (win == NULL)
	return FALSE;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (wp == win)
	    return TRUE;
    return FALSE;
}

/*
 * Return the number of windows.
 */
    int
win_count()
{
    WIN	    *wp;
    int	    count = 0;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	++count;
    return count;
}

/*
 * Make 'count' windows on the screen.
 * Return actual number of windows on the screen.
 * Must be called when there is just one window, filling the whole screen
 * (excluding the command line).
 */
    int
make_windows(count)
    int	    count;
{
    int	    maxcount;
    int	    todo;
    int	    p_sb_save;

/*
 * Each window needs at least 'winminheight' lines and a status line.
 * Add 4 lines for one window, otherwise we may end up with all zero-line
 * windows. Use value of 'winheight' if it is set
 */
    maxcount = (curwin->w_height + curwin->w_status_height
				  - (p_wh - p_wmh)) / (p_wmh + STATUS_HEIGHT);
    if (maxcount < 2)
	maxcount = 2;
    if (count > maxcount)
	count = maxcount;

    /*
     * add status line now, otherwise first window will be too big
     */
    if ((p_ls == 2 || (count > 1 && p_ls == 1)) && curwin->w_status_height == 0)
    {
	curwin->w_status_height = STATUS_HEIGHT;
	win_new_height(curwin, curwin->w_height - STATUS_HEIGHT);
    }

#ifdef AUTOCMD
/*
 * Don't execute autocommands while creating the windows.  Must do that
 * when putting the buffers in the windows.
 */
    ++autocmd_busy;
#endif

/*
 * set 'splitbelow' off for a moment, don't want that now
 */
    p_sb_save = p_sb;
    p_sb = FALSE;
	/* todo is number of windows left to create */
    for (todo = count - 1; todo > 0; --todo)
	if (win_split(curwin->w_height - (curwin->w_height - todo
				* STATUS_HEIGHT) / (todo + 1) - STATUS_HEIGHT,
							FALSE, FALSE) == FAIL)
	    break;
    p_sb = p_sb_save;

#ifdef AUTOCMD
    --autocmd_busy;
#endif

	/* return actual number of windows */
    return (count - todo);
}

/*
 * Exchange current and next window
 */
    static void
win_exchange(Prenum)
    long	Prenum;
{
    WIN	    *wp;
    WIN	    *wp2;
    int	    temp;

    if (lastwin == firstwin)	    /* just one window */
    {
	beep_flush();
	return;
    }

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

/*
 * find window to exchange with
 */
    if (Prenum)
    {
	wp = firstwin;
	while (wp != NULL && --Prenum > 0)
	    wp = wp->w_next;
    }
    else if (curwin->w_next != NULL)	/* Swap with next */
	wp = curwin->w_next;
    else    /* Swap last window with previous */
	wp = curwin->w_prev;

    if (wp == curwin || wp == NULL)
	return;

/*
 * 1. remove curwin from the list. Remember after which window it was in wp2
 * 2. insert curwin before wp in the list
 * if wp != wp2
 *    3. remove wp from the list
 *    4. insert wp after wp2
 * 5. exchange the status line height
 */
    wp2 = curwin->w_prev;
    win_remove(curwin);
    win_append(wp->w_prev, curwin);
    if (wp != wp2)
    {
	win_remove(wp);
	win_append(wp2, wp);
    }
    temp = curwin->w_status_height;
    curwin->w_status_height = wp->w_status_height;
    wp->w_status_height = temp;

    win_comp_pos();		/* recompute window positions */

    win_enter(wp, TRUE);
    update_screen(CLEAR);
}

/*
 * rotate windows: if upwards TRUE the second window becomes the first one
 *		   if upwards FALSE the first window becomes the second one
 */
    static void
win_rotate(upwards, count)
    int	    upwards;
    int	    count;
{
    WIN		 *wp;
    int		 height;

    if (firstwin == lastwin)		/* nothing to do */
    {
	beep_flush();
	return;
    }

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

    while (count--)
    {
	if (upwards)		/* first window becomes last window */
	{
	    wp = firstwin;
	    win_remove(wp);
	    win_append(lastwin, wp);
	    wp = lastwin->w_prev;	    /* previously last window */
	}
	else			/* last window becomes first window */
	{
	    wp = lastwin;
	    win_remove(lastwin);
	    win_append(NULL, wp);
	    wp = firstwin;		    /* previously last window */
	}
	    /* exchange status height of old and new last window */
	height = lastwin->w_status_height;
	lastwin->w_status_height = wp->w_status_height;
	wp->w_status_height = height;

	    /* recompute w_winpos for all windows */
	(void)win_comp_pos();
    }

    update_screen(CLEAR);
}

/*
 * Move window "win1" to below "win2" and make "win1" the current window.
 */
    void
win_move_after(win1, win2)
    WIN	*win1, *win2;
{
    int	    height;

    /* check if the arguments are reasonable */
    if (win1 == win2)
	return;

    /* check if there is something to do */
    if (win2->w_next != win1)
    {
	/* may need move the status line of the last window */
	if (win1 == lastwin)
	{
	    height = win1->w_prev->w_status_height;
	    win1->w_prev->w_status_height = win1->w_status_height;
	    win1->w_status_height = height;
	}
	else if (win2 == lastwin)
	{
	    height = win1->w_status_height;
	    win1->w_status_height = win2->w_status_height;
	    win2->w_status_height = height;
	}
	win_remove(win1);
	win_append(win2, win1);

	(void)win_comp_pos();	/* recompute w_winpos for all windows */
	redraw_later(NOT_VALID);
    }
    win_enter(win1, FALSE);
}

/*
 * Make all windows the same height.
 * 'next_curwin' will soon be the current window, make sure it has enough
 * rows.
 */
    void
win_equal(next_curwin, redraw)
    WIN	    *next_curwin;	    /* pointer to current window to be */
    int	    redraw;
{
    int	    total;
    int	    less;
    int	    wincount;
    int	    winpos;
    int	    temp;
    WIN	    *wp;
    int	    new_height;

/*
 * count the number of lines available
 */
    total = 0;
    wincount = 0;
    for (wp = firstwin; wp; wp = wp->w_next)
    {
	total += wp->w_height - p_wmh;
	wincount++;
    }

/*
 * If next_curwin given and 'winheight' set, make next_curwin p_wh lines.
 */
    less = 0;
    if (next_curwin != NULL)
    {
	if (p_wh - p_wmh > total)    /* all lines go to current window */
	    less = total;
	else
	{
	    less = p_wh - p_wmh - total / wincount;
	    if (less < 0)
		less = 0;
	}
    }

/*
 * spread the available lines over the windows
 */
    winpos = 0;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	if (wp == next_curwin && less)
	{
	    less = 0;
	    temp = p_wh - p_wmh;
	    if (temp > total)
		temp = total;
	}
	else
	    temp = (total - less + ((unsigned)wincount >> 1)) / wincount;
	new_height = p_wmh + temp;
	if (wp->w_winpos != winpos || wp->w_height != new_height)
	{
	    wp->w_redr_type = NOT_VALID;
	    wp->w_redr_status = TRUE;
	}
	wp->w_winpos = winpos;
	win_new_height(wp, new_height);
	total -= temp;
	--wincount;
	winpos += wp->w_height + wp->w_status_height;
    }
    if (redraw)
	must_redraw = CLEAR;
}

/*
 * close all windows for buffer 'buf'
 */
    void
close_windows(buf)
    BUF	    *buf;
{
    WIN	    *win;

    ++RedrawingDisabled;
    for (win = firstwin; win != NULL && lastwin != firstwin; )
    {
	if (win->w_buffer == buf)
	{
	    close_window(win, FALSE);
	    win = firstwin;	    /* go back to the start */
	}
	else
	    win = win->w_next;
    }
    --RedrawingDisabled;
}

/*
 * close window "win"
 * If "free_buf" is TRUE related buffer may be freed.
 *
 * called by :quit, :close, :xit, :wq and findtag()
 */
    void
close_window(win, free_buf)
    WIN	    *win;
    int	    free_buf;
{
    WIN	    *wp;
#ifdef AUTOCMD
    int	    other_buffer = FALSE;
#endif
    int	    close_curwin = FALSE;

    if (lastwin == firstwin)
    {
	EMSG("Cannot close last window");
	return;
    }

#ifdef AUTOCMD
    if (win == curwin)
    {
	/*
	 * Guess which window is going to be the new current window.
	 * This may change because of the autocommands (sigh).
	 */
	if ((!p_sb && win->w_next != NULL) || win->w_prev == NULL)
	    wp = win->w_next;
	else
	    wp = win->w_prev;

	/*
	 * Be careful: If autocommands delete the window, return now.
	 */
	if (wp->w_buffer != curbuf)
	{
	    other_buffer = TRUE;
	    apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf);
	    if (!win_valid(win))
		return;
	}
	apply_autocmds(EVENT_WINLEAVE, NULL, NULL, FALSE, curbuf);
	if (!win_valid(win))
	    return;
    }
#endif

/*
 * Remove the window.
 * if 'splitbelow' the free space goes to the window above it.
 * if 'nosplitbelow' the free space goes to the window below it.
 * This makes opening a window and closing it immediately keep the same window
 * layout.
 */
				    /* freed space goes to next window */
    if ((!p_sb && win->w_next != NULL) || win->w_prev == NULL)
    {
	wp = win->w_next;
	wp->w_winpos = win->w_winpos;
    }
    else			    /* freed space goes to previous window */
	wp = win->w_prev;

/*
 * Close the link to the buffer.
 */
    close_buffer(win, win->w_buffer, free_buf, FALSE);
    /* autocommands may have closed the window already */
    if (!win_valid(win))
	return;

    win_new_height(wp, wp->w_height + win->w_height + win->w_status_height);
    win_free(win);

    /* Make sure curwin isn't invalid.  It can cause severe trouble when
     * printing an error message.  For win_equal() curbuf needs to be valid
     * too. */
    if (win == curwin)
    {
	curwin = wp;
	curbuf = wp->w_buffer;
	close_curwin = TRUE;
    }
    if (p_ea)
	win_equal(wp, FALSE);
    if (close_curwin)
    {
	win_enter_ext(wp, FALSE, TRUE);
#ifdef AUTOCMD
	if (other_buffer)
	    /* careful: after this wp and win may be invalid! */
	    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif
    }

    /*
     * if last window has status line now and we don't want one,
     * remove the status line
     */
    if (lastwin->w_status_height &&
			(p_ls == 0 || (p_ls == 1 && firstwin == lastwin)))
    {
	win_new_height(lastwin, lastwin->w_height + lastwin->w_status_height);
	lastwin->w_status_height = 0;
	comp_col();
    }

    update_screen(NOT_VALID);
}

/*
 * Close all windows except current one.
 * Buffers in the other windows become hidden if 'hidden' is set, or '!' is
 * used and the buffer was modified.
 *
 * Used by ":bdel" and ":only".
 */
    void
close_others(message, forceit)
    int	    message;
    int	    forceit;	    /* always hide all other windows */
{
    WIN	    *wp;
    WIN	    *nextwp;

    if (lastwin == firstwin)
    {
	if (message
#ifdef AUTOCMD
		    && !autocmd_busy
#endif
				    )
	    MSG("Already only one window");
	return;
    }

    for (wp = firstwin; win_valid(wp); wp = nextwp)
    {
	nextwp = wp->w_next;
	if (wp == curwin)		/* don't close current window */
	    continue;

	/* Check if it's allowed to abandon this window */
	if (!can_abandon(wp->w_buffer, forceit))
	{
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	    if (message && (p_confirm || confirm))
		dialog_changed(wp->w_buffer, FALSE);
	    if (buf_changed(wp->w_buffer))
#endif
		continue;
	}

	/* Close the link to the buffer. */
	close_buffer(wp, wp->w_buffer,
				 !p_hid && !buf_changed(wp->w_buffer), FALSE);
	/* autocommands may have closed the window already */
	if (!win_valid(wp))
	    continue;

	/* Remove the window.  All lines go to previous or next window. */
	if (wp->w_prev != NULL)
	    win_new_height(wp->w_prev,
		   wp->w_prev->w_height + wp->w_height + wp->w_status_height);
	else
	{
	    win_new_height(wp->w_next,
		   wp->w_next->w_height + wp->w_height + wp->w_status_height);
	    wp->w_next->w_winpos = wp->w_winpos;
	}
	win_free(wp);
    }

    /*
     * If current window has a status line and we don't want one,
     * remove the status line.
     */
    if (lastwin != firstwin)
	EMSG("Other window contains changes");
    else if (curwin->w_status_height && p_ls != 2)
    {
	win_new_height(curwin, curwin->w_height + curwin->w_status_height);
	curwin->w_status_height = 0;
	comp_col();
    }
    if (message)
	update_screen(NOT_VALID);
}

/*
 * init the cursor in the window
 *
 * called when a new file is being edited
 */
    void
win_init(wp)
    WIN	    *wp;
{
    wp->w_redr_type = NOT_VALID;
    wp->w_cursor.lnum = 1;
    wp->w_curswant = wp->w_cursor.col = 0;
    wp->w_pcmark.lnum = 1;	/* pcmark not cleared but set to line 1 */
    wp->w_pcmark.col = 0;
    wp->w_prev_pcmark.lnum = 0;
    wp->w_prev_pcmark.col = 0;
    wp->w_topline = 1;
    wp->w_botline = 2;
#ifdef FKMAP
    if (curwin->w_p_rl)
	wp->w_p_pers = W_CONV + W_R_L;
    else
	wp->w_p_pers = W_CONV;
#endif
}

/*
 * Go to another window.
 * When jumping to another buffer, stop visual mode.  Do this before
 * changing windows so we can yank the selection into the '*' register.
 */
    static void
win_goto(wp)
    WIN	    *wp;
{
    if (wp->w_buffer != curbuf && VIsual_active)
    {
	end_visual_mode();
	redraw_curbuf_later(NOT_VALID);
    }
    VIsual_reselect = FALSE;
#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif
    win_enter(wp, TRUE);
}

#if defined(HAVE_PERL_INTERP) || defined(PROTO)
/*
 * Go to window nr "winnr" (counting top to bottom).
 */
    WIN *
win_goto_nr(winnr)
    int	    winnr;
{
    WIN	    *wp;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (--winnr == 0)
	    break;
    return wp;
}
#endif

/*
 * Make window wp the current window.
 */
    void
win_enter(wp, undo_sync)
    WIN		*wp;
    int		undo_sync;
{
    win_enter_ext(wp, undo_sync, FALSE);
}

/*
 * Make window wp the current window.
 * Can be called with "curwin_invalid" TRUE, which means that curwin has just
 * been closed and isn't valid.
 */
    static void
win_enter_ext(wp, undo_sync, curwin_invalid)
    WIN		*wp;
    int		undo_sync;
    int		curwin_invalid;
{
#ifdef AUTOCMD
    int		other_buffer = FALSE;
#endif

    if (wp == curwin && !curwin_invalid)	/* nothing to do */
	return;

#ifdef AUTOCMD
    if (!curwin_invalid)
    {
	/*
	 * Be careful: If autocommands delete the window, return now.
	 */
	if (wp->w_buffer != curbuf)
	{
	    apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf);
	    other_buffer = TRUE;
	    if (!win_valid(wp))
		return;
	}
	apply_autocmds(EVENT_WINLEAVE, NULL, NULL, FALSE, curbuf);
	if (!win_valid(wp))
	    return;
    }
#endif

	/* sync undo before leaving the current buffer */
    if (undo_sync && curbuf != wp->w_buffer)
	u_sync();
	/* may have to copy the buffer options when 'cpo' contains 'S' */
    if (wp->w_buffer != curbuf)
	buf_copy_options(curbuf, wp->w_buffer, BCO_ENTER | BCO_NOHELP);
    if (!curwin_invalid)
    {
	prevwin = curwin;	/* remember for CTRL-W p */
	curwin->w_redr_status = TRUE;
    }
    curwin = wp;
    curbuf = wp->w_buffer;
    adjust_cursor();
    changed_line_abv_curs();	/* assume cursor position needs updating */

#ifdef AUTOCMD
    apply_autocmds(EVENT_WINENTER, NULL, NULL, FALSE, curbuf);
    if (other_buffer)
	apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif

#ifdef WANT_TITLE
    maketitle();
#endif
    curwin->w_redr_status = TRUE;
    if (restart_edit)
	redraw_later(VALID);	/* causes status line redraw */

    /* set window height to desired minimal value */
    if (curwin->w_height < p_wh)
	win_setheight((int)p_wh);
#ifdef USE_MOUSE
    setmouse();			/* in case jumped to/from help buffer */
#endif
}

/*
 * Jump to the first open window that contains buffer buf if one exists
 * TODO: Alternatively jump to last open window? Dependent from 'splitbelow'?
 * Returns pointer to window if it exists, otherwise NULL.
 */
    WIN *
buf_jump_open_win(buf)
    BUF	    *buf;
{
    WIN	    *wp;

    for (wp = firstwin; wp; wp = wp->w_next)
	if (wp->w_buffer == buf)
	    break;
    if (wp)
	win_enter(wp, FALSE);
    return wp;
}

/*
 * allocate a window structure and link it in the window list
 */
    WIN *
win_alloc(after)
    WIN	    *after;
{
    WIN	    *newwin;

    /*
     * allocate window structure and linesizes arrays
     */
    newwin = (WIN *)alloc_clear((unsigned)sizeof(WIN));
    if (newwin != NULL && win_alloc_lsize(newwin) == FAIL)
    {
	vim_free(newwin);
	newwin = NULL;
    }

    if (newwin != NULL)
    {
	/*
	 * link the window in the window list
	 */
	win_append(after, newwin);

	/* position the display and the cursor at the top of the file. */
	newwin->w_topline = 1;
	newwin->w_botline = 2;
	newwin->w_cursor.lnum = 1;
#ifdef SCROLLBIND
	newwin->w_scbind_pos = 1;
#endif

	/* We won't calculate w_fraction until resizing the window */
	newwin->w_fraction = 0;
	newwin->w_prev_fraction_row = -1;

#ifdef USE_GUI
	if (gui.in_use)
	{
	    gui_create_scrollbar(&newwin->w_scrollbars[SBAR_LEFT], newwin);
	    gui_create_scrollbar(&newwin->w_scrollbars[SBAR_RIGHT], newwin);
	}
#endif
#ifdef WANT_EVAL
	var_init(&newwin->w_vars);	    /* init internal variables */
#endif
    }
    return newwin;
}

/*
 * remove window 'wp' from the window list and free the structure
 */
    void
win_free(wp)
    WIN	    *wp;
{
    int	    i;

#ifdef HAVE_PERL_INTERP
    perl_win_free(wp);
#endif

#ifdef HAVE_PYTHON
    python_window_free(wp);
#endif

#ifdef HAVE_TCL
    tcl_window_free(wp);
#endif

#ifdef WANT_EVAL
    var_clear(&wp->w_vars);	    /* free all internal variables */
#endif

    if (prevwin == wp)
	prevwin = NULL;
    win_free_lsize(wp);

    for (i = 0; i < wp->w_tagstacklen; ++i)
	vim_free(wp->w_tagstack[i].tagname);

#ifdef USE_GUI
    if (gui.in_use)
    {
	gui_mch_destroy_scrollbar(&wp->w_scrollbars[SBAR_LEFT]);
	gui_mch_destroy_scrollbar(&wp->w_scrollbars[SBAR_RIGHT]);
    }
#endif /* USE_GUI */

    win_remove(wp);
    vim_free(wp);
}

    static void
win_append(after, wp)
    WIN	    *after, *wp;
{
    WIN	    *before;

    if (after == NULL)	    /* after NULL is in front of the first */
	before = firstwin;
    else
	before = after->w_next;

    wp->w_next = before;
    wp->w_prev = after;
    if (after == NULL)
	firstwin = wp;
    else
	after->w_next = wp;
    if (before == NULL)
	lastwin = wp;
    else
	before->w_prev = wp;
}

/*
 * remove window from the window list
 */
    static void
win_remove(wp)
    WIN	    *wp;
{
    if (wp->w_prev)
	wp->w_prev->w_next = wp->w_next;
    else
	firstwin = wp->w_next;
    if (wp->w_next)
	wp->w_next->w_prev = wp->w_prev;
    else
	lastwin = wp->w_prev;
}

/*
 * allocate lsize arrays for a window
 * return FAIL for failure, OK for success
 */
    int
win_alloc_lsize(wp)
    WIN	    *wp;
{
    wp->w_lsize_valid = 0;
    wp->w_lsize_lnum = (linenr_t *)alloc((unsigned)(Rows * sizeof(linenr_t)));
    wp->w_lsize = alloc((unsigned)Rows);
    if (wp->w_lsize_lnum == NULL || wp->w_lsize == NULL)
    {
	win_free_lsize(wp);	/* one of the two may have worked */
	return FAIL;
    }
    return OK;
}

/*
 * free lsize arrays for a window
 */
    void
win_free_lsize(wp)
    WIN	    *wp;
{
    vim_free(wp->w_lsize_lnum);
    vim_free(wp->w_lsize);
    wp->w_lsize_lnum = NULL;
    wp->w_lsize = NULL;
}

/*
 * call this fuction whenever Rows changes value
 */
    void
screen_new_rows()
{
    WIN	    *wp;
    int	    extra_lines;

    if (firstwin == NULL)	/* not initialized yet */
	return;
/*
 * the number of extra lines is the difference between the position where
 * the command line should be and where it is now
 */
    extra_lines = Rows - p_ch -
	   (lastwin->w_winpos + lastwin->w_height + lastwin->w_status_height);
    if (extra_lines < 0)			/* reduce windows height */
    {
	for (wp = lastwin; wp; wp = wp->w_prev)
	{
	    if (wp->w_height - p_wmh < -extra_lines)
	    {
		extra_lines += wp->w_height - p_wmh;
		win_new_height(wp, (int)p_wmh);
	    }
	    else
	    {
		win_new_height(wp, wp->w_height + extra_lines);
		break;
	    }
	}
	(void)win_comp_pos();		    /* compute w_winpos */
    }
    else if (extra_lines > 0)		    /* increase height of last window */
	win_new_height(lastwin, lastwin->w_height + extra_lines);

    compute_cmdrow();

    if (p_ea)
	win_equal(curwin, FALSE);
}

/*
 * update the w_winpos field for all windows
 * returns the row just after the last window
 */
    static int
win_comp_pos()
{
    WIN	    *wp;
    int	    row;

    row = 0;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	if (wp->w_winpos != row)	/* if position changes, redraw */
	{
	    wp->w_winpos = row;
	    wp->w_redr_type = NOT_VALID;
	    wp->w_redr_status = TRUE;
	}
	row += wp->w_height + wp->w_status_height;
    }
    return row;
}

/*
 * set current window height
 */
    void
win_setheight(height)
    int	    height;
{
    WIN	    *wp;
    int	    room;		/* total number of lines available */
    int	    take;		/* number of lines taken from other windows */
    int	    room_cmdline;	/* lines available from cmdline */
    int	    row;
    int	    run;

    if (p_wmh == 0)
    {
	/* Always keep current window at least one line high, even when
	 * 'winminheight' is zero */
	if (height <= 0)	/* need at least one line */
	{
	    height = 1;
	    room = 1;
	}
	else
	    room = p_wmh;	/* count 'winminheight' for the curr. window */
    }
    else
    {
	if (height < p_wmh)	/* need at least some lines */
	    height = p_wmh;
	room = p_wmh;		/* count 'winminheight' for the curr. window */
    }

/*
 * compute the room we have from all the windows
 */
    room_cmdline = Rows - p_ch;
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	room += wp->w_height - p_wmh;
	room_cmdline -= wp->w_height + wp->w_status_height;
    }
/*
 * limit new height to the room available
 */
    if (height > room + room_cmdline)	    /* can't make it that large */
	height = room + room_cmdline;	    /* use all available room */
/*
 * compute the number of lines we will take from the windows (can be negative)
 */
    take = height - curwin->w_height;
    if (take == 0)			    /* no change, nothing to do */
	return;

    if (take > 0)
    {
	take -= room_cmdline;		    /* use lines from cmdline first */
	if (take < 0)
	    take = 0;
    }
/*
 * set the current window to the new height
 */
    win_new_height(curwin, height);

/*
 * First take lines from the windows below the current window.
 * If that is not enough, takes lines from windows above the current window.
 */
    for (run = 0; run < 2; ++run)
    {
	if (run == 0)
	    wp = curwin->w_next;	/* 1st run: start with next window */
	else
	    wp = curwin->w_prev;	/* 2nd run: start with prev window */
	while (wp != NULL && take != 0)
	{
	    if (wp->w_height - take < p_wmh)
	    {
		take -= wp->w_height - p_wmh;
		win_new_height(wp, (int)p_wmh);
	    }
	    else
	    {
		win_new_height(wp, wp->w_height - take);
		take = 0;
	    }
	    if (run == 0)
		wp = wp->w_next;
	    else
		wp = wp->w_prev;
	}
    }

/* recompute the window positions */
    row = win_comp_pos();

/*
 * If there is extra space created between the last window and the command line,
 * clear it.
 */
    if (full_screen && msg_scrolled == 0)
	screen_fill(row, cmdline_row, 0, (int)Columns, ' ', ' ', 0);
    cmdline_row = row;
    msg_row = row;
    msg_col = 0;

    update_screen(NOT_VALID);
}

/*
 * Check 'winminheight' for a valid value.
 */
    void
win_setminheight()
{
    int		room;
    int		first = TRUE;
    WIN		*wp;

    /* loop until there is a 'winminheight' that is possible */
    while (p_wmh > 0)
    {
	room = -p_wh;
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	    room += wp->w_height - p_wmh;
	if (room >= 0)
	    break;
	--p_wmh;
	if (first)
	{
	    EMSG(e_noroom);
	    first = FALSE;
	}
    }
}

#ifdef USE_MOUSE

    void
win_drag_status_line(offset)
    int	    offset;
{
    WIN	    *wp;
    int	    room;
    int	    row;
    int	    up;		/* if TRUE, drag status line up, otherwise down */

    if (offset < 0)
    {
	up = TRUE;
	offset = -offset;
    }
    else
	up = FALSE;

    if (up) /* drag up */
    {
	if (p_wmh == 0)
	    room = -1;	/* current window should be at least one line */
	else
	    room = 0;
	for (wp = curwin; wp != NULL && room < offset; wp = wp->w_prev)
	    room += wp->w_height - p_wmh;
	wp = curwin->w_next;		    /* put wp at window that grows */
    }
    else    /* drag down */
    {
	/*
	 * Only dragging the last status line can reduce p_ch.
	 */
	room = Rows - cmdline_row;
	if (curwin->w_next == NULL)
	    room -= 1;
	else
	    room -= p_ch;
	for (wp = curwin->w_next; wp != NULL && room < offset; wp = wp->w_next)
	    room += wp->w_height - p_wmh;
	wp = curwin;			    /* put wp at window that grows */
    }

    if (room < offset)	    /* Not enough room */
	offset = room;	    /* Move as far as we can */
    if (offset <= 0)
	return;

    if (wp != NULL)	    /* grow window wp by offset lines */
	win_new_height(wp, wp->w_height + offset);

    if (up)
	wp = curwin;		    /* current window gets smaller */
    else
	wp = curwin->w_next;	    /* next window gets smaller */

    while (wp != NULL && offset > 0)
    {
	if (wp->w_height - offset <= p_wmh)
	{
	    offset -= wp->w_height - p_wmh;
	    if (wp == curwin && p_wmh == 0)
	    {
		win_new_height(wp, 1);
		offset += 1;
	    }
	    else
		win_new_height(wp, (int)p_wmh);
	}
	else
	{
	    win_new_height(wp, wp->w_height - offset);
	    offset = 0;
	}
	if (up)
	    wp = wp->w_prev;
	else
	    wp = wp->w_next;
    }
    row = win_comp_pos();
    screen_fill(row, cmdline_row, 0, (int)Columns, ' ', ' ', 0);
    cmdline_row = row;
    p_ch = Rows - cmdline_row;
    update_screen(NOT_VALID);
    showmode();
}
#endif /* USE_MOUSE */

/*
 * Set new window height.
 */
    static void
win_new_height(wp, height)
    WIN	    *wp;
    int	    height;
{
    linenr_t	lnum;
    int		sline, line_size;
#define FRACTION_MULT	16384L

    if (wp->w_wrow != wp->w_prev_fraction_row && wp->w_height > 0)
	wp->w_fraction = ((long)wp->w_wrow * FRACTION_MULT
				    + FRACTION_MULT / 2) / (long)wp->w_height;

    wp->w_height = height;
    wp->w_skipcol = 0;

    lnum = wp->w_cursor.lnum;
    if (lnum < 1)		/* can happen when starting up */
	lnum = 1;
    wp->w_wrow = ((long)wp->w_fraction * (long)height - 1L) / FRACTION_MULT;
    line_size = plines_win_col(wp, lnum, (long)(wp->w_cursor.col)) - 1;
    sline = wp->w_wrow - line_size;
    if (sline < 0)
    {
	/*
	 * Cursor line would go off top of screen if w_wrow was this high.
	 */
	wp->w_wrow = line_size;
    }
    else
    {
	while (sline > 0 && lnum > 1)
	    sline -= (line_size = plines_win(wp, --lnum));
	if (sline < 0)
	{
	    /*
	     * Line we want at top would go off top of screen.	Use next line
	     * instead.
	     */
	    lnum++;
	    wp->w_wrow -= line_size + sline;
	}
	else if (sline > 0)
	{
	    /* First line of file reached, use that as topline. */
	    lnum = 1;
	    wp->w_wrow -= sline;
	}
    }
    set_topline(wp, lnum);
    if (wp == curwin)
    {
	if (p_so)
	    update_topline();
	curs_columns(FALSE);	    /* validate w_wrow */
    }
    wp->w_prev_fraction_row = wp->w_wrow;

    win_comp_scroll(wp);
    if (wp->w_redr_type < NOT_VALID)
	wp->w_redr_type = NOT_VALID;
    wp->w_redr_status = TRUE;
    invalidate_botline_win(wp);
}

    void
win_comp_scroll(wp)
    WIN	    *wp;
{
    wp->w_p_scroll = ((unsigned)wp->w_height >> 1);
    if (wp->w_p_scroll == 0)
	wp->w_p_scroll = 1;
}

/*
 * command_height: called whenever p_ch has been changed
 */
    void
command_height(old_p_ch)
    long    old_p_ch;
{
    WIN	    *wp;
    int	    h;

    if (starting != NO_SCREEN)
    {
	cmdline_row = Rows - p_ch;
	if (p_ch > old_p_ch)		    /* p_ch got bigger */
	{
	    for (wp = lastwin; p_ch > old_p_ch; wp = wp->w_prev)
	    {
		if (wp == NULL)
		{
		    emsg(e_noroom);
		    p_ch = old_p_ch;
		    break;
		}
		h = wp->w_height - (p_ch - old_p_ch);
		if (p_wmh == 0)
		{
		    /* don't make current window zero lines */
		    if (wp == curwin && h < 1)
			h = 1;
		}
		else if (h < p_wmh)
		    h = p_wmh;
		old_p_ch += wp->w_height - h;
		win_new_height(wp, h);
	    }
	    win_comp_pos();
	    /* clear the lines added to cmdline */
	    if (full_screen)
		screen_fill((int)(cmdline_row), (int)Rows, 0,
						   (int)Columns, ' ', ' ', 0);
	    msg_row = cmdline_row;
	    redraw_cmdline = TRUE;
	    return;
	}

	if (msg_row < cmdline_row)
	    msg_row = cmdline_row;
	redraw_cmdline = TRUE;
    }
    win_new_height(lastwin, (int)(lastwin->w_height + old_p_ch - p_ch));
}

    void
last_status()
{
    WIN		*wp;

    if (lastwin->w_status_height)
    {
	/* remove status line */
	if (p_ls == 0 || (p_ls == 1 && firstwin == lastwin))
	{
	    win_new_height(lastwin, lastwin->w_height + 1);
	    lastwin->w_status_height = 0;
	    comp_col();
	}
    }
    else if (p_ls == 2 || (p_ls == 1 && firstwin != lastwin))
    {
	/* go to first window with enough room for a win_new_height(-1) */
	for (wp = lastwin; wp->w_height <= p_wmh; wp = wp->w_prev)
	    if (wp == NULL)
	    {
		emsg(e_noroom);
		return;
	    }
	win_new_height(wp, wp->w_height - 1);
	win_comp_pos();
	lastwin->w_status_height = 1;
	comp_col();
	redraw_all_later(NOT_VALID);
    }
}

#if defined(FILE_IN_PATH) || defined(PROTO)
/*
 * file_name_at_cursor()
 *
 * Return the name of the file under (or to the right of) the cursor.
 *
 * get_file_name_in_path()
 *
 * Return the name of the file at (or to the right of) ptr[col].
 *
 * The p_path variable is searched if the file name does not start with '/'.
 * The string returned has been alloc'ed and should be freed by the caller.
 * NULL is returned if the file name or file is not found.
 *
 * options:
 * FNAME_MESS	    give error messages
 * FNAME_EXP	    expand to path
 * FNAME_HYP	    check for hypertext link
 */
    char_u *
file_name_at_cursor(options, count)
    int	    options;
    long    count;
{
    return get_file_name_in_path(ml_get_curline(),
					curwin->w_cursor.col, options, count);
}

    char_u *
get_file_name_in_path(line, col, options, count)
    char_u  *line;
    int	    col;
    int	    options;
    long    count;
{
    char_u  *ptr;
    char_u  *file_name;
    char_u  *path;
    int	    len;

    /*
     * search forward for what could be the start of a file name
     */
    ptr = line + col;
    while (*ptr != NUL && !vim_isfilec(*ptr))
	++ptr;
    if (*ptr == NUL)		/* nothing found */
    {
	if (options & FNAME_MESS)
	    EMSG("No file name under cursor");
	return NULL;
    }

    /*
     * search backward for first char of the file name
     */
    while (ptr > line && vim_isfilec(ptr[-1]))
	--ptr;

    /*
     * Go one char back to ":" before "//" even when ':' is not in 'isfname'.
     */
    if ((options & FNAME_HYP) && ptr > line && path_is_url(ptr - 1))
	--ptr;

    /*
     * Search forward for the last char of the file name.
     * Also allow "://" when ':' is not in 'isfname'.
     */
    len = 0;
    while (vim_isfilec(ptr[len])
			 || ((options & FNAME_HYP) && path_is_url(ptr + len)))
	++len;

    if (options & FNAME_HYP)
    {
	/* For hypertext links, ignore the name of the machine.
	 * Such a link looks like "type://machine/path". Only "/path" is used.
	 * First search for the string "://", then for the extra '/'
	 */
	if ((file_name = vim_strchr(ptr, ':')) != NULL
		&& ((path_is_url(file_name) == URL_SLASH
			&& (path = vim_strchr(file_name + 3, '/')) != NULL)
		    || (path_is_url(file_name) == URL_BACKSLASH
			&& (path = vim_strchr(file_name + 3, '\\')) != NULL))
		&& path < ptr + len)
	{
	    len -= path - ptr;
	    ptr = path;
	    if (ptr[1] == '~')	    /* skip '/' for /~user/path */
	    {
		++ptr;
		--len;
	    }
	}
    }

    if (!(options & FNAME_EXP))
	return vim_strnsave(ptr, len);

    return find_file_in_path(ptr, len, options, count);
}

/*
 * Find the file name "ptr[len]" in the path.
 *
 * options:
 * FNAME_MESS	    give error message when not found
 *
 * Uses NameBuff[]!
 *
 * Returns an allocated string for the file name.  NULL for error.
 */
    char_u *
find_file_in_path(ptr, len, options, count)
    char_u	*ptr;		/* file name */
    int		len;		/* length of file name */
    int		options;
    long	count;		/* use count'th matching file name */
{
    char_u	save_char;
    char_u	*file_name;
    char_u	*curr_path = NULL;
    char_u	*dir;
    int		curr_path_len;
    char_u	*p;
    char_u	*head;


    /* copy file name into NameBuff, expanding environment variables */
    save_char = ptr[len];
    ptr[len] = NUL;
    expand_env(ptr, NameBuff, MAXPATHL);
    ptr[len] = save_char;

    if (mch_isFullName(NameBuff))
    {
	/*
	 * Absolute path, no need to use 'path'.
	 */
	if ((file_name = vim_strsave(NameBuff)) == NULL)
	    return NULL;
	if (mch_getperm(file_name) >= 0)
	    return file_name;
	if (options & FNAME_MESS)
	    EMSG2("Can't find file \"%s\"", NameBuff);
    }
    else
    {
	/*
	 * Relative path, use 'path' option.
	 */
	if (curbuf->b_fname != NULL)
	{
	    curr_path = curbuf->b_fname;
	    ptr = gettail(curr_path);
	    curr_path_len = ptr - curr_path;
	}
	else
	    curr_path_len = 0;
	if ((file_name = alloc((int)(curr_path_len + STRLEN(p_path) +
					    STRLEN(NameBuff) + 3))) == NULL)
	    return NULL;

	for (dir = p_path; *dir && !got_int; )
	{
	    len = copy_option_part(&dir, file_name, 31000, " ,");
	    /* len == 0 means: use current directory */
	    if (len != 0)
	    {
		/* Look for file relative to current file */
		if (file_name[0] == '.' && curr_path_len > 0
				 && (len == 1 || vim_ispathsep(file_name[1])))
		{
		    if (len == 1)	/* just a "." */
			len = 0;
		    else		/* "./path": move "path" */
		    {
			len -= 2;
			mch_memmove(file_name + curr_path_len, file_name + 2,
								 (size_t)len);
		    }
		    STRNCPY(file_name, curr_path, curr_path_len);
		    len += curr_path_len;
		}
		if (!vim_ispathsep(file_name[len - 1]))
		    file_name[len++] = PATHSEP;
		file_name[len] = NUL;

		/*
		 * Handle "**" in the path: 'wildcard in path'.
		 */
		if (mch_has_wildcard(file_name))
		{
		    p = get_past_head(file_name);
		    if (p == file_name)	    /* no absolute file name */
			p = find_file_in_wildcard_path((char_u *)"",
							file_name, 0, &count);
		    else    /* absolute file name, separate head */
		    {
			head = vim_strnsave(file_name,
						   (unsigned)(p - file_name));
			if (head != NULL)
			{
			    p = find_file_in_wildcard_path(head, p, 0, &count);
			    vim_free(head);
			}
		    }
		    if (p != NULL)
		    {
			vim_free(file_name);
			return p;
		    }
		    continue;
		}
	    }
	    STRCPY(file_name + len, NameBuff);

	    /*
	     * Translate names like "src/a/../b/file.c" into "src/b/file.c".
	     */
	    simplify_filename(file_name);
	    if (mch_getperm(file_name) >= 0 && --count == 0)
		return file_name;
	}
	if (options & FNAME_MESS)
	    EMSG2("Can't find file \"%s\" in path", NameBuff);
    }

    /* get here when file doesn't exist */
    vim_free(file_name);
    return NULL;
}

/*
 * find_file_in_wildcard_path(): expand path recursively while searching
 *				     files in path
 *
 * The syntax '**' means the whole subtree.
 * To avoid endless recursion, a counter restricts the depth to 100 levels.
 * In the following pseudo code '+/' will mean '*' followed by '/'
 *
 * in the case of 'set path=,/foo/bar/+/+/,'
 * the function call hierarchy will be
 *   find_file_in_wildcard_path("/", "foo/bar/+/+/", NameBuff, 0);
 *   find_file_in_wildcard_path("/foo/", "bar/+/+/", NameBuff, 0);
 *   find_file_in_wildcard_path("/foo/bar/", "+/+/", NameBuff, 0);
 * which in turn will call
 *   find_file_in_wildcard_path("/foo/bar/dir/", "+/", NameBuff, 1);
 * for each directory 'dir' in '/foo/bar/+'.  It's the next call,
 *   find_file_in_wildcard_path("/foo/bar/dir/dir2/", "", NameBuff, 2);
 * that will try to find the file 'NameBuff' in the given directory.
 *
 * pseudo code:
 *
 *  find_file_in_wildcard_path(path_so_far, wildcards, level)
 *  {
 *    if (level > 100)
 *	return NULL;
 *
 *    file_name = path_so_far + first_segment(wildcards);
 *    rest_of_wildcards = all_but_first_segment(wildcards);
 *
 *    result = expand(file_name);
 *
 *    if (!rest_of_wildcards) {
 *	foreach_path_in(result) {
 *	  if (exists&readable(path + NameBuff))
 *	    return path+NameBuff;
 *	}
 *    } else {
 *	foreach_path_in(result) {
 *	  c = find_file_in_wildcard_path(path, rest_of_wildcards, level+1);
 *	  if (c)
 *	    return c;
 *	}
 *    }
 *    if (infinite_recursion(wildcards)) {
 *	foreach_path_in(result) {
 *	  c = find_file_in_wildcard_path(path, wildcards, level+1);
 *	  if (c)
 *	    return c;
 *	}
 *    }
 *    return NULL;
 *  }
 */
    static char_u *
find_file_in_wildcard_path(path_so_far, wildcards, level, countptr)
    char_u  *path_so_far;
    char_u  *wildcards;
    int	    level;
    long    *countptr;
{
    char_u  *file_name;
    int	    len;
    char_u  *rest_of_wildcards;
    int	    nFiles = 0;
    char_u  **ppFiles;
    int	    i;
    char_u  *c;

    ui_breakcheck();
    if (level > 100 || got_int)
	return NULL;

    if ((file_name = alloc((int)MAXPATHL)) == NULL)
	return NULL;

    STRCPY(file_name, path_so_far);
    len = STRLEN(file_name);
    if (!vim_ispathsep(file_name[len-1]))
    {
	file_name[len++] = PATHSEP;
	file_name[len] = NUL;
    }
    rest_of_wildcards = wildcards;
    if (rest_of_wildcards)
    {
	if (STRNCMP(rest_of_wildcards, "**", 2) == 0)
	    rest_of_wildcards++;
	while (*rest_of_wildcards && !vim_ispathsep(*rest_of_wildcards))
	    file_name[len++] = *rest_of_wildcards++;
	/* file_name[len++] = *rest_of_wildcards++; */
	rest_of_wildcards++;
	file_name[len] = NUL;
    }

    expand_wildcards(1, &file_name, &nFiles, &ppFiles,
					EW_FILE|EW_DIR|EW_ADDSLASH|EW_SILENT);

    if (!*rest_of_wildcards)
    {
	for (i = 0; i < nFiles; ++i)
	{
	    if (!mch_isdir(ppFiles[i]))
		continue;   /* not a directory */
	    STRCPY(file_name, ppFiles[i]);
	    add_pathsep(file_name);
	    STRCAT(file_name, NameBuff);
	    if (mch_getperm(file_name) >= 0 && --*countptr == 0)
	    {
		FreeWild(nFiles, ppFiles);
		return file_name;
	    }
	}
    }
    else
    {
	for (i = 0; i < nFiles; ++i)
	{
	    if (!mch_isdir(ppFiles[i]))
		continue;   /* not a directory */
	    c = find_file_in_wildcard_path(ppFiles[i],
					rest_of_wildcards, level+1, countptr);
	    if (c)
	    {
		FreeWild(nFiles, ppFiles);
		vim_free(file_name);
		return c;
	    }
	}
    }

    if (STRNCMP(wildcards, "**", 2) == 0)
    {
	for (i = 0; i < nFiles; ++i)
	{
	    if (!mch_isdir(ppFiles[i]))
		continue;   /* not a directory */
	    c = find_file_in_wildcard_path(ppFiles[i],
						wildcards, level+1, countptr);
	    if (c)
	    {
		FreeWild(nFiles, ppFiles);
		vim_free(file_name);
		return c;
	    }
	}
    }

    FreeWild(nFiles, ppFiles);

    vim_free(file_name);
    return NULL;
}

/*
 * Check if the "://" of a URL is at the pointer, return URL_SLASH.
 * Also check for ":\\", which MS Internet Explorer accepts, return
 * URL_BACKSLASH.
 */
    static int
path_is_url(p)
    char_u  *p;
{
    if (STRNCMP(p, "://", (size_t)3) == 0)
	return URL_SLASH;
    else if (STRNCMP(p, ":\\\\", (size_t)3) == 0)
	return URL_BACKSLASH;
    return 0;
}
#endif /* FILE_IN_PATH */

/*
 * Return the minimal number of rows that is needed on the screen to display
 * the current number of windows.
 */
    int
min_rows()
{
    WIN	    *wp;
    int	    total;

    if (firstwin == NULL)	/* not initialized yet */
	return MIN_LINES;

    total = p_ch;	/* count the room for the status line */
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	total += p_wmh + wp->w_status_height;
    if (p_wmh == 0)
	total += 1;	/* at least one window should have a line! */
    return total;
}

/*
 * Return TRUE if there is only one window, not counting a help window, unless
 * it is the current window.
 */
    int
only_one_window()
{
    int	    count = 0;
    WIN	    *wp;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (!wp->w_buffer->b_help || wp == curwin)
	    ++count;
    return (count <= 1);
}

/*
 * Correct the cursor line number in other windows.  Used after changing the
 * current buffer, and before applying autocommands.
 * When "do_curwin" is TRUE, also check current window.
 */
    void
check_lnums(do_curwin)
    int		do_curwin;
{
    WIN		*wp;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if ((do_curwin || wp != curwin) && wp->w_buffer == curbuf)
	{
	    if (wp->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		wp->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    if (wp->w_topline > curbuf->b_ml.ml_line_count)
		wp->w_topline = curbuf->b_ml.ml_line_count;
	}
}
