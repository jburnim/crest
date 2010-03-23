/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * buffer.c: functions for dealing with the buffer structure
 */

/*
 * The buffer list is a double linked list of all buffers.
 * Each buffer can be in one of these states:
 * never loaded: BF_NEVERLOADED is set, only the file name is valid
 *   not loaded: b_ml.ml_mfp == NULL, no memfile allocated
 *	 hidden: b_nwindows == 0, loaded but not displayed in a window
 *	 normal: loaded and displayed in a window
 *
 * Instead of storing file names all over the place, each file name is
 * stored in the buffer list. It can be referenced by a number.
 *
 * The current implementation remembers all file names ever used.
 */


#include "vim.h"

static char_u	*buflist_match __ARGS((vim_regexp *prog, BUF *buf));
static char_u	*buflist_match_try __ARGS((vim_regexp *prog, char_u *name));
static void	buflist_setfpos __ARGS((BUF *, linenr_t, colnr_t));
#ifdef UNIX
static BUF	*buflist_findname_stat __ARGS((char_u *ffname, struct stat *st));
static int	otherfile_buf __ARGS((BUF *buf, char_u *ffname, struct stat *stp));
static void	buf_setino __ARGS((BUF *buf));
static int	buf_same_ino __ARGS((BUF *buf, struct stat *stp));
#else
static int	otherfile_buf __ARGS((BUF *buf, char_u *ffname));
#endif
#ifdef WANT_TITLE
static int	ti_change __ARGS((char_u *str, char_u **last));
#endif
static void	free_buffer __ARGS((BUF *));

/*
 * Open current buffer, that is: open the memfile and read the file into memory
 * return FAIL for failure, OK otherwise
 */
    int
open_buffer(read_stdin)
    int	    read_stdin;	    /* read file from stdin */
{
    int	    retval = OK;
#ifdef AUTOCMD
    BUF	    *old_curbuf;
    BUF	    *new_curbuf;
#endif

    /*
     * The 'readonly' flag is only set when BF_NEVERLOADED is being reset.
     * When re-entering the same buffer, it should not change, because the
     * user may have reset the flag by hand.
     */
    if (readonlymode && curbuf->b_ffname != NULL
					&& (curbuf->b_flags & BF_NEVERLOADED))
	curbuf->b_p_ro = TRUE;

    if (ml_open() == FAIL)
    {
	/*
	 * There MUST be a memfile, otherwise we can't do anything
	 * If we can't create one for the current buffer, take another buffer
	 */
	close_buffer(NULL, curbuf, FALSE, FALSE);
	for (curbuf = firstbuf; curbuf != NULL; curbuf = curbuf->b_next)
	    if (curbuf->b_ml.ml_mfp != NULL)
		break;
	/*
	 * if there is no memfile at all, exit
	 * This is OK, since there are no changes to loose.
	 */
	if (curbuf == NULL)
	{
	    EMSG("Cannot allocate buffer, exiting...");
	    getout(2);
	}
	EMSG("Cannot allocate buffer, using other one...");
	enter_buffer(curbuf);
	return FAIL;
    }

#ifdef AUTOCMD
    /* The autocommands in readfile() may change the buffer, but only AFTER
     * reading the file. */
    old_curbuf = curbuf;
    modified_was_set = FALSE;
#endif
    if (curbuf->b_ffname != NULL)
	retval = readfile(curbuf->b_ffname, curbuf->b_fname,
		       (linenr_t)0, (linenr_t)0, (linenr_t)MAXLNUM, READ_NEW);
    else if (read_stdin)
	retval = readfile(NULL, NULL, (linenr_t)0,
		       (linenr_t)0, (linenr_t)MAXLNUM, READ_NEW + READ_STDIN);

    /* if first time loading this buffer, init chartab */
    if (curbuf->b_flags & BF_NEVERLOADED)
	init_chartab();

    /*
     * Set/reset the Changed flag first, autocmds may change the buffer.
     * Apply the automatic commands, before processing the modelines.
     * So the modelines have priority over auto commands.
     */
    /* When reading stdin, the buffer contents always needs writing, so set
     * the changed flag.  Unless in readonly mode: "ls | gview -". */
    if ((read_stdin && !readonlymode)
#ifdef AUTOCMD
		|| modified_was_set	/* ":set modified" used in autocmd */
#endif
		)
	changed();
    else if (retval != FAIL)
	unchanged(curbuf, FALSE);
    curbuf->b_start_ffc = *curbuf->b_p_ff;    /* keep this fileformat */

    /* require "!" to overwrite the file, because it wasn't read completely */
    if (got_int)
	curbuf->b_flags |= BF_READERR;

#ifdef AUTOCMD
    curwin->w_topline = 1;
    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif

    if (retval != FAIL)
    {
#ifdef AUTOCMD
	/*
	 * The autocommands may have changed the current buffer.  Apply the
	 * modelines to the correct buffer, if it still exists.
	 */
	if (buf_valid(old_curbuf))
	{
	    new_curbuf = curbuf;
	    curbuf = old_curbuf;
	    curwin->w_buffer = old_curbuf;
#endif
	    do_modelines();
	    curbuf->b_flags &= ~(BF_CHECK_RO | BF_NEVERLOADED);
#ifdef AUTOCMD
	    curbuf = new_curbuf;
	    curwin->w_buffer = new_curbuf;
	}
#endif
    }

    return retval;
}

/*
 * Return TRUE if "buf" points to a valid buffer (in the buffer list).
 */
    int
buf_valid(buf)
    BUF	    *buf;
{
    BUF	    *bp;

    for (bp = firstbuf; bp != NULL; bp = bp->b_next)
	if (bp == buf)
	    return TRUE;
    return FALSE;
}

/*
 * Close the link to a buffer. If "free_buf" is TRUE free the buffer if it
 * becomes unreferenced. The caller should get a new buffer very soon!
 * If 'del_buf' is TRUE, remove the buffer from the buffer list.
 */
    void
close_buffer(win, buf, free_buf, del_buf)
    WIN	    *win;	    /* if not NULL, set b_last_cursor */
    BUF	    *buf;
    int	    free_buf;
    int	    del_buf;
{
#ifdef AUTOCMD
    int		is_curbuf;
#endif

    if (buf->b_nwindows > 0)
	--buf->b_nwindows;
    if (buf->b_nwindows == 0 && win != NULL)
    {
	set_last_cursor(win);	/* may set b_last_cursor */
	if (win == curwin && curwin->w_cursor.lnum != 1)
				/* and remember last cursor position */
	    buflist_setfpos(buf, curwin->w_cursor.lnum, curwin->w_cursor.col);
    }
#ifdef AUTOCMD
    /* When the buffer becomes hidden, but is not unloaded, trigger BufHidden */
    if (buf->b_nwindows == 0 && !free_buf)
    {
	apply_autocmds(EVENT_BUFHIDDEN, buf->b_fname, buf->b_fname, FALSE, buf);
	if (!buf_valid(buf))	    /* autocommands may delete the buffer */
	    return;
    }
#endif
    if (buf->b_nwindows > 0 || !free_buf)
    {
	if (buf == curbuf)
	    u_sync();	    /* sync undo before going to another buffer */
	return;
    }

    /* Always remove the buffer when there is no file name. */
    if (buf->b_ffname == NULL)
	del_buf = TRUE;

    /*
     * Free all things allocated for this buffer.
     * Also calls the "BufDelete" autocommands when del_buf is TRUE.
     */
#ifdef AUTOCMD
    is_curbuf = (buf == curbuf);
#endif
    buf_freeall(buf, del_buf);
#ifdef AUTOCMD
    /*
     * Autocommands may have deleted the buffer.
     * It's possible that autocommands change curbuf to the one being deleted.
     * This might cause the previous curbuf to be deleted unexpectedly.  But
     * in some cases it's OK to delete the curbuf, because a new one is
     * obtained anyway.  Therefore only return if curbuf changed to the
     * deleted buffer.
     */
    if (!buf_valid(buf) || (buf == curbuf && !is_curbuf))
	return;
#endif

    /*
     * Remove the buffer from the list.
     */
    if (del_buf)
    {
	vim_free(buf->b_ffname);
	vim_free(buf->b_sfname);
	if (buf->b_prev == NULL)
	    firstbuf = buf->b_next;
	else
	    buf->b_prev->b_next = buf->b_next;
	if (buf->b_next == NULL)
	    lastbuf = buf->b_prev;
	else
	    buf->b_next->b_prev = buf->b_prev;
	free_buffer(buf);
    }
    else
	buf_clear(buf);
}

/*
 * buf_clear() - make buffer empty
 */
    void
buf_clear(buf)
    BUF	    *buf;
{
    buf->b_ml.ml_line_count = 1;
    unchanged(buf, TRUE);
#ifndef SHORT_FNAME
    buf->b_shortname = FALSE;
#endif
    buf->b_p_eol = TRUE;
    buf->b_ml.ml_mfp = NULL;
    buf->b_ml.ml_flags = ML_EMPTY;		/* empty buffer */
}

/*
 * buf_freeall() - free all things allocated for a buffer that are related to
 * the file.
 */
/*ARGSUSED*/
    void
buf_freeall(buf, del_buf)
    BUF		*buf;
    int		del_buf;	/* buffer is going to be deleted */
{
#ifdef AUTOCMD
    int		is_curbuf = (buf == curbuf);

    apply_autocmds(EVENT_BUFUNLOAD, buf->b_fname, buf->b_fname, FALSE, buf);
    if (!buf_valid(buf))	    /* autocommands may delete the buffer */
	return;
    if (del_buf)
    {
	apply_autocmds(EVENT_BUFDELETE, buf->b_fname, buf->b_fname, FALSE, buf);
	if (!buf_valid(buf))	    /* autocommands may delete the buffer */
	    return;
    }
    /*
     * It's possible that autocommands change curbuf to the one being deleted.
     * This might cause curbuf to be deleted unexpectedly.  But in some cases
     * it's OK to delete the curbuf, because a new one is obtained anyway.
     * Therefore only return if curbuf changed to the deleted buffer.
     */
    if (buf == curbuf && !is_curbuf)
	return;
#endif
#ifdef HAVE_TCL
    tcl_buffer_free(buf);
#endif
    u_blockfree(buf);		    /* free the memory allocated for undo */
    ml_close(buf, TRUE);	    /* close and delete the memline/memfile */
    buf->b_ml.ml_line_count = 0;    /* no lines in buffer */
    u_clearall(buf);		    /* reset all undo information */
#ifdef SYNTAX_HL
    syntax_clear(buf);		    /* reset syntax info */
#endif
}

/*
 * Free a buffer structure and the things it contains related to the buffer
 * itself (not the file).
 */
    static void
free_buffer(buf)
    BUF		*buf;
{
    WINFPOS	*wlp;

    /* Free the b_winfpos list for buffer "buf". */
    while (buf->b_winfpos != NULL)
    {
	wlp = buf->b_winfpos;
	buf->b_winfpos = wlp->wl_next;
	vim_free(wlp);
    }

#ifdef HAVE_PERL_INTERP
    perl_buf_free(buf);
#endif
#ifdef HAVE_PYTHON
    python_buffer_free(buf);
#endif
#ifdef WANT_EVAL
    var_clear(&buf->b_vars);	    /* free all internal variables */
#endif
    free_buf_options(buf, TRUE);
    vim_free(buf);
}

/*
 * do_bufdel() - delete or unload buffer(s)
 *
 * addr_count == 0: ":bdel" - delete current buffer
 * addr_count == 1: ":N bdel" or ":bdel N [N ..] - first delete
 *		    buffer "end_bnr", then any other arguments.
 * addr_count == 2: ":N,N bdel" - delete buffers in range
 *
 * command can be DOBUF_UNLOAD (":bunload") or DOBUF_DEL (":bdel")
 *
 * Returns error message or NULL
 */
    char_u *
do_bufdel(command, arg, addr_count, start_bnr, end_bnr, forceit)
    int	    command;
    char_u  *arg;	/* pointer to extra arguments */
    int	    addr_count;
    int	    start_bnr;	/* first buffer number in a range */
    int	    end_bnr;	/* buffer number or last buffer number in a range */
    int	    forceit;
{
    int	    do_current = 0;	/* delete current buffer? */
    int	    deleted = 0;	/* number of buffers deleted */
    char_u  *errormsg = NULL;	/* return value */
    int	    bnr;		/* buffer number */
    char_u  *p;

    if (addr_count == 0)
    {
	(void)do_buffer(command, DOBUF_CURRENT, FORWARD, 0, forceit);
    }
    else
    {
	if (addr_count == 2)
	{
	    if (*arg)		/* both range and argument is not allowed */
		return e_trailing;
	    bnr = start_bnr;
	}
	else	/* addr_count == 1 */
	    bnr = end_bnr;

	for ( ;!got_int; ui_breakcheck())
	{
	    /*
	     * delete the current buffer last, otherwise when the
	     * current buffer is deleted, the next buffer becomes
	     * the current one and will be loaded, which may then
	     * also be deleted, etc.
	     */
	    if (bnr == curbuf->b_fnum)
		do_current = bnr;
	    else if (do_buffer(command, DOBUF_FIRST, FORWARD, (int)bnr,
							       forceit) == OK)
		++deleted;

	    /*
	     * find next buffer number to delete/unload
	     */
	    if (addr_count == 2)
	    {
		if (++bnr > end_bnr)
		    break;
	    }
	    else    /* addr_count == 1 */
	    {
		arg = skipwhite(arg);
		if (*arg == NUL)
		    break;
		if (!isdigit(*arg))
		{
		    p = skiptowhite_esc(arg);
		    bnr = buflist_findpat(arg, p);
		    if (bnr < 0)	    /* failed */
			break;
		    arg = p;
		}
		else
		    bnr = getdigits(&arg);
	    }
	}
	if (!got_int && do_current && do_buffer(command, DOBUF_FIRST,
					  FORWARD, do_current, forceit) == OK)
	    ++deleted;

	if (deleted == 0)
	{
	    sprintf((char *)IObuff, "No buffers were %s",
		    command == DOBUF_UNLOAD ? "unloaded" : "deleted");
	    errormsg = IObuff;
	}
	else /* should 'report' be used here? */
	    smsg((char_u *)"%d buffer%s %s", deleted,
		    plural((long)deleted),
		    command == DOBUF_UNLOAD ? "unloaded" : "deleted");
    }

    return errormsg;
}

/*
 * Implementation of the command for the buffer list
 *
 * action == DOBUF_GOTO	    go to specified buffer
 * action == DOBUF_SPLIT    split window and go to specified buffer
 * action == DOBUF_UNLOAD   unload specified buffer(s)
 * action == DOBUF_DEL	    delete specified buffer(s)
 *
 * start == DOBUF_CURRENT   go to "count" buffer from current buffer
 * start == DOBUF_FIRST	    go to "count" buffer from first buffer
 * start == DOBUF_LAST	    go to "count" buffer from last buffer
 * start == DOBUF_MOD	    go to "count" modified buffer from current buffer
 *
 * Return FAIL or OK.
 */
    int
do_buffer(action, start, dir, count, forceit)
    int		action;
    int		start;
    int		dir;		/* FORWARD or BACKWARD */
    int		count;		/* buffer number or number of buffers */
    int		forceit;	/* TRUE for :...! */
{
    BUF		*buf;
    BUF		*delbuf;
    int		retval;
    int		forward;

    switch (start)
    {
	case DOBUF_FIRST:   buf = firstbuf; break;
	case DOBUF_LAST:    buf = lastbuf;  break;
	default:	    buf = curbuf;   break;
    }
    if (start == DOBUF_MOD)	    /* find next modified buffer */
    {
	while (count-- > 0)
	{
	    do
	    {
		buf = buf->b_next;
		if (buf == NULL)
		    buf = firstbuf;
	    }
	    while (buf != curbuf && !buf_changed(buf));
	}
	if (!buf_changed(buf))
	{
	    EMSG("No modified buffer found");
	    return FAIL;
	}
    }
    else if (start == DOBUF_FIRST && count) /* find specified buffer number */
    {
	while (buf != NULL && buf->b_fnum != count)
	    buf = buf->b_next;
    }
    else
    {
	while (count-- > 0)
	{
	    if (dir == FORWARD)
	    {
		buf = buf->b_next;
		if (buf == NULL)
		    buf = firstbuf;
	    }
	    else
	    {
		buf = buf->b_prev;
		if (buf == NULL)
		    buf = lastbuf;
	    }
	    /* in non-help buffer, skip help buffers, and vv */
	    if ((buf->b_help) !=
			     (start == DOBUF_LAST ? lastbuf : curbuf)->b_help)
		 count++;
	}
    }

    if (buf == NULL)	    /* could not find it */
    {
	if (start == DOBUF_FIRST)
	{
					    /* don't warn when deleting */
	    if (action != DOBUF_UNLOAD && action != DOBUF_DEL)
		EMSGN("Cannot go to buffer %ld", count);
	}
	else if (dir == FORWARD)
	    EMSG("Cannot go beyond last buffer");
	else
	    EMSG("Cannot go before first buffer");
	return FAIL;
    }

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

    /*
     * delete buffer buf from memory and/or the list
     */
    if (action == DOBUF_UNLOAD || action == DOBUF_DEL)
    {
	if (!forceit && buf_changed(buf))
	{
	    EMSGN("No write since last change for buffer %ld (use ! to override)",
			buf->b_fnum);
	    return FAIL;
	}

	/*
	 * If deleting last buffer, make it empty.
	 * The last buffer cannot be unloaded.
	 */
	if (firstbuf->b_next == NULL)
	{
	    if (action == DOBUF_UNLOAD)
	    {
		EMSG("Cannot unload last buffer");
		return FAIL;
	    }

	    /* Close any other windows on this buffer, then make it empty. */
	    close_others(FALSE, TRUE);
	    buf = curbuf;
	    setpcmark();
	    retval = do_ecmd(0, NULL, NULL, NULL, ECMD_ONE,
						  forceit ? ECMD_FORCEIT : 0);

	    /*
	     * do_ecmd() may create a new buffer, then we have to delete
	     * the old one.  But do_ecmd() may have done that already, check
	     * if the buffer still exists.
	     */
	    if (buf != curbuf && buf_valid(buf))
		close_buffer(NULL, buf, TRUE, TRUE);
	    return retval;
	}

	/*
	 * If the deleted buffer is the current one, close the current window
	 * (unless it's the only window).
	 */
	while (buf == curbuf && firstwin != lastwin)
	    close_window(curwin, FALSE);

	/*
	 * If the buffer to be deleted is not current one, delete it here.
	 */
	if (buf != curbuf)
	{
	    close_windows(buf);
	    if (buf_valid(buf))
		close_buffer(NULL, buf, TRUE, action == DOBUF_DEL);
	    return OK;
	}

	/*
	 * Deleting the current buffer: Need to find another buffer to go to.
	 * There must be another, otherwise it would have been handled above.
	 * First use au_new_curbuf, if it is valid.
	 * Then prefer the buffer we most recently visited.
	 * Else try to find one that is loaded, after the current buffer,
	 * then before the current buffer.
	 * Finally use any buffer.
	 */
	buf = NULL;
#ifdef AUTOCMD
	if (au_new_curbuf != NULL && buf_valid(au_new_curbuf))
	    buf = au_new_curbuf;
	else
#endif
	    if (curwin->w_jumplistlen > 0)
	{
	    int     jumpidx;

	    jumpidx = curwin->w_jumplistidx - 1;
	    if (jumpidx < 0)
		jumpidx = curwin->w_jumplistlen - 1;

	    forward = jumpidx;
	    while (jumpidx != curwin->w_jumplistidx)
	    {
		buf = buflist_findnr(curwin->w_jumplist[jumpidx].fnum);
		if (buf == curbuf || (buf != NULL && buf->b_ml.ml_mfp == NULL))
		    buf = NULL;	/* Must be open and not current */
		/* found a valid buffer: stop searching */
		if (buf != NULL)
		    break;
		/* advance to older entry in jump list */
		if (!jumpidx && curwin->w_jumplistidx == curwin->w_jumplistlen)
		    break;
		if (--jumpidx < 0)
		    jumpidx = curwin->w_jumplistlen - 1;
		if (jumpidx == forward)       /* List exhausted for sure */
		    break;
	    }
	}

	if (buf == NULL)	/* No previous buffer, Try 2'nd approach */
	{
	    forward = TRUE;
	    buf = curbuf->b_next;
	    for (;;)
	    {
		if (buf == NULL)
		{
		    if (!forward)	/* tried both directions */
			break;
		    buf = curbuf->b_prev;
		    forward = FALSE;
		    continue;
		}
		/* in non-help buffer, try to skip help buffers, and vv */
		if (buf->b_ml.ml_mfp != NULL && buf->b_help == curbuf->b_help)
		    break;
		if (forward)
		    buf = buf->b_next;
		else
		    buf = buf->b_prev;
	    }
	}
	if (buf == NULL)	/* Still no loaded buffers, just take one */
	{
	    if (curbuf->b_next != NULL)
		buf = curbuf->b_next;
	    else
		buf = curbuf->b_prev;
	}
    }

    /*
     * make buf current buffer
     */
    if (action == DOBUF_SPLIT)	    /* split window first */
    {
	/* jump to first window containing buf if one exists ("useopen") */
	if (vim_strchr(p_swb, 'u') && buf_jump_open_win(buf))
	    return OK;
	if (win_split(0, FALSE, FALSE) == FAIL)
	    return FAIL;
    }

    /* go to current buffer - nothing to do */
    if (buf == curbuf)
	return OK;

    /*
     * Check if the current buffer may be abandoned.
     */
    if (action == DOBUF_GOTO && !can_abandon(curbuf, forceit))
    {
	EMSG(e_nowrtmsg);
	return FAIL;
    }

    setpcmark();
    curwin->w_alt_fnum = curbuf->b_fnum; /* remember alternate file */
    buflist_altfpos();			 /* remember curpos */

    /* close_windows() or apply_autocmds() may change curbuf */
    delbuf = curbuf;

#ifdef AUTOCMD
    apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf);
    if (buf_valid(delbuf))
#endif
    {
	if (action == DOBUF_UNLOAD || action == DOBUF_DEL)
	    close_windows(delbuf);
	if (buf_valid(delbuf))
	    close_buffer(delbuf == curwin->w_buffer ? curwin : NULL, delbuf,
		    (action == DOBUF_GOTO && !p_hid && !buf_changed(delbuf))
			     || action == DOBUF_UNLOAD || action == DOBUF_DEL,
		      action == DOBUF_DEL);
    }
#ifdef AUTOCMD
    if (buf_valid(buf))	    /* an autocommand may have deleted buf! */
#endif
	enter_buffer(buf);
    return OK;
}

/*
 * enter a new current buffer.
 * (old curbuf must have been freed already)
 */
    void
enter_buffer(buf)
    BUF	    *buf;
{
    buf_copy_options(curbuf, buf, BCO_ENTER | BCO_NOHELP);
    curwin->w_buffer = buf;
    curbuf = buf;
    ++curbuf->b_nwindows;
    if (curbuf->b_ml.ml_mfp == NULL)	/* need to load the file */
	open_buffer(FALSE);
    else
    {
	need_fileinfo = TRUE;		/* display file info after redraw */
	(void)buf_check_timestamp(curbuf, FALSE); /* check if file changed */
#ifdef AUTOCMD
	curwin->w_topline = 1;
	apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
#endif
    }
    buflist_getfpos();			/* restore curpos.lnum and possibly
					 * curpos.col */
    check_arg_idx(curwin);		/* check for valid arg_idx */
#ifdef WANT_TITLE
    maketitle();
#endif
#ifdef AUTOCMD
    if (curwin->w_topline == 1)		/* when autocmds didn't change it */
#endif
	scroll_cursor_halfway(FALSE);	/* redisplay at correct position */
    update_screen(NOT_VALID);
}

/*
 * functions for dealing with the buffer list
 */

/*
 * Add a file name to the buffer list.  Return a pointer to the buffer.
 * If the same file name already exists return a pointer to that buffer.
 * If it does not exist, or if fname == NULL, a new entry is created.
 * If use_curbuf is TRUE, may use current buffer.
 * This is the ONLY way to create a new buffer.
 */
static int  top_file_num = 1;		/* highest file number */

    BUF *
buflist_new(ffname, sfname, lnum, use_curbuf)
    char_u	*ffname;
    char_u	*sfname;
    linenr_t	lnum;
    int		use_curbuf;
{
    BUF		*buf;
#ifdef UNIX
    struct stat	st;
#endif

    fname_expand(&ffname, &sfname);	/* will allocate ffname */

#ifdef UNIX
    if (sfname == NULL || mch_stat((char *)sfname, &st) < 0)
	st.st_dev = (unsigned)-1;
#endif

    /*
     * If file name already exists in the list, update the entry.
     */
    if (ffname != NULL && (buf =
#ifdef UNIX
		buflist_findname_stat(ffname, &st)
#else
		buflist_findname(ffname)
#endif
		) != NULL)
    {
	vim_free(ffname);
	if (lnum != 0)
	    buflist_setfpos(buf, lnum, (colnr_t)0);
	/* copy the options now, if 'cpo' doesn't have 's' and not done
	 * already */
	buf_copy_options(curbuf, buf, 0);
	return buf;
    }

    /*
     * If the current buffer has no name and no contents, use the current
     * buffer.	Otherwise: Need to allocate a new buffer structure.
     *
     * This is the ONLY place where a new buffer structure is allocated!
     */
    if (use_curbuf
	    && curbuf != NULL
	    && curbuf->b_ffname == NULL
	    && curbuf->b_nwindows <= 1
	    && (curbuf->b_ml.ml_mfp == NULL || bufempty()))
    {
	buf = curbuf;
#ifdef AUTOCMD
	/* It's like this buffer is deleted. */
	apply_autocmds(EVENT_BUFDELETE, NULL, NULL, FALSE, curbuf);
#endif
    }
    else
    {
	buf = (BUF *)alloc_clear((unsigned)sizeof(BUF));
	if (buf == NULL)
	{
	    vim_free(ffname);
	    return NULL;
	}
    }

    if (ffname != NULL)
    {
	buf->b_ffname = ffname;
	buf->b_sfname = vim_strsave(sfname);
    }
    if (buf->b_winfpos == NULL)
	buf->b_winfpos = (WINFPOS *)alloc((unsigned)sizeof(WINFPOS));
    if ((ffname != NULL && (buf->b_ffname == NULL || buf->b_sfname == NULL))
	    || buf->b_winfpos == NULL)
    {
	vim_free(buf->b_ffname);
	buf->b_ffname = NULL;
	vim_free(buf->b_sfname);
	buf->b_sfname = NULL;
	if (buf != curbuf)
	    free_buffer(buf);
	return NULL;
    }

    if (buf == curbuf)
    {
	buf_freeall(buf, FALSE); /* free all things allocated for this buffer */
	if (buf != curbuf)	 /* autocommands deleted the buffer! */
	    return NULL;
	buf->b_nwindows = 0;
    }
    else
    {
	/*
	 * put new buffer at the end of the buffer list
	 */
	buf->b_next = NULL;
	if (firstbuf == NULL)		/* buffer list is empty */
	{
	    buf->b_prev = NULL;
	    firstbuf = buf;
	}
	else				/* append new buffer at end of list */
	{
	    lastbuf->b_next = buf;
	    buf->b_prev = lastbuf;
	}
	lastbuf = buf;

	buf->b_fnum = top_file_num++;
	if (top_file_num < 0)		/* wrap around (may cause duplicates) */
	{
	    EMSG("Warning: List of file names overflow");
	    out_flush();
	    ui_delay(3000L, TRUE);	/* make sure it is noticed */
	    top_file_num = 1;
	}

	buf->b_winfpos->wl_fpos.lnum = lnum;
	buf->b_winfpos->wl_fpos.col = 0;
	buf->b_winfpos->wl_next = NULL;
	buf->b_winfpos->wl_prev = NULL;
	buf->b_winfpos->wl_win = curwin;

#ifdef WANT_EVAL
	var_init(&buf->b_vars);	    /* init internal variables */
#endif
	/*
	 * Always copy the options from the current buffer.
	 */
	buf_copy_options(curbuf, buf, BCO_ALWAYS);
    }

    buf->b_fname = buf->b_sfname;
#ifdef UNIX
    if (st.st_dev == (unsigned)-1)
	buf->b_dev = -1;
    else
    {
	buf->b_dev = st.st_dev;
	buf->b_ino = st.st_ino;
    }
#endif
    buf->b_u_synced = TRUE;
    buf->b_flags = BF_CHECK_RO | BF_NEVERLOADED;
    buf_clear(buf);
    clrallmarks(buf);		    /* clear marks */
    fmarks_check_names(buf);	    /* check file marks for this file */
#ifdef AUTOCMD
    apply_autocmds(EVENT_BUFCREATE, NULL, NULL, FALSE, buf);
#endif

    return buf;
}

#if 0 /* not used */
/*
 * Get the highest buffer number.  Note that some buffers may have been
 * deleted.
 */
    int
buflist_maxbufnr()
{
    return (top_file_num - 1);
}
#endif

/*
 * Free the memory for the options of a buffer.
 */
    void
free_buf_options(buf, free_p_ff)
    BUF	    *buf;
    int	    free_p_ff;	    /* also free 'fileformat'? */
{
    if (free_p_ff)
    {
	free_string_option(buf->b_p_ff);
#ifdef MULTI_BYTE
	free_string_option(buf->b_p_fe);
#endif
    }
#ifdef CRYPTV
    free_string_option(buf->b_p_key);
#endif
    free_string_option(buf->b_p_mps);
    free_string_option(buf->b_p_fo);
    free_string_option(buf->b_p_isk);
#ifdef COMMENTS
    free_string_option(buf->b_p_com);
#endif
    free_string_option(buf->b_p_nf);
#ifdef SYNTAX_HL
    free_string_option(buf->b_p_syn);
#endif
#ifdef AUTOCMD
    free_string_option(buf->b_p_ft);
#endif
#ifdef WANT_OSFILETYPE
    free_string_option(buf->b_p_oft);
#endif
#ifdef CINDENT
    free_string_option(buf->b_p_cink);
    free_string_option(buf->b_p_cino);
#endif
#if defined(CINDENT) || defined(SMARTINDENT)
    free_string_option(buf->b_p_cinw);
#endif
#ifdef INSERT_EXPAND
    free_string_option(buf->b_p_cpt);
#endif
}

/*
 * get alternate file n
 * set linenr to lnum or altfpos.lnum if lnum == 0
 *	also set cursor column to altfpos.col if 'startofline' is not set.
 * if (options & GETF_SETMARK) call setpcmark()
 * if (options & GETF_ALT) we are jumping to an alternate file.
 * if (options & GETF_SWITCH) respect 'switchbuf' settings when jumping
 *
 * return FAIL for failure, OK for success
 */
    int
buflist_getfile(n, lnum, options, forceit)
    int		n;
    linenr_t	lnum;
    int		options;
    int		forceit;
{
    BUF		*buf;
    WIN		*wp = NULL;
    FPOS	*fpos;
    colnr_t	col;

    buf = buflist_findnr(n);
    if (buf == NULL)
    {
	if ((options & GETF_ALT) && n == 0)
	    emsg(e_noalt);
	else
	    EMSGN("buffer %ld not found", n);
	return FAIL;
    }

    /* if alternate file is the current buffer, nothing to do */
    if (buf == curbuf)
	return OK;

    /* altfpos may be changed by getfile(), get it now */
    if (lnum == 0)
    {
	fpos = buflist_findfpos(buf);
	lnum = fpos->lnum;
	col = fpos->col;
    }
    else
	col = 0;

    if (options & GETF_SWITCH)
    {
	/* use existing open window for buffer if wanted */
	if (vim_strchr(p_swb, 'u'))     /* useopen */
	    wp = buf_jump_open_win(buf);
	/* split window if wanted ("split") */
	if (wp == NULL && vim_strchr(p_swb, 't') && !win_split(0, FALSE, FALSE))
	    return FAIL;
    }

    ++RedrawingDisabled;
    if (getfile(buf->b_fnum, NULL, NULL, (options & GETF_SETMARK),
							  lnum, forceit) <= 0)
    {
	--RedrawingDisabled;

	/* cursor is at to BOL and w_cursor.lnum is checked due to getfile() */
	if (!p_sol && col != 0)
	{
	    curwin->w_cursor.col = col;
	    check_cursor_col();
	}
	return OK;
    }
    --RedrawingDisabled;
    return FAIL;
}

/*
 * go to the last know line number for the current buffer
 */
    void
buflist_getfpos()
{
    FPOS	*fpos;

    fpos = buflist_findfpos(curbuf);

    curwin->w_cursor.lnum = fpos->lnum;
    check_cursor_lnum();

    if (p_sol)
	curwin->w_cursor.col = 0;
    else
    {
	curwin->w_cursor.col = fpos->col;
	check_cursor_col();
    }
}

/*
 * find file in buffer list by name (it has to be for the current window)
 * 'ffname' must have a full path.
 */
    BUF *
buflist_findname(ffname)
    char_u	*ffname;
{
#ifdef UNIX
    struct stat st;

    if (mch_stat((char *)ffname, &st) < 0)
	st.st_dev = (unsigned)-1;
    return buflist_findname_stat(ffname, &st);
}

/*
 * Same as buflist_findname(), but pass the stat structure to avoid getting it
 * twice for the same file.
 */
    static BUF *
buflist_findname_stat(ffname, stp)
    char_u	*ffname;
    struct stat	*stp;
{
#endif
    BUF		*buf;

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (!otherfile_buf(buf, ffname
#ifdef UNIX
		    , stp
#endif
		    ))
	    return buf;
    return NULL;
}

/*
 * Find file in buffer list by a regexppattern.
 * Return fnum of the found buffer, < 0 for error.
 */
    int
buflist_findpat(pattern, pattern_end)
    char_u	*pattern;
    char_u	*pattern_end;	/* pointer to first char after pattern */
{
    BUF		*buf;
    vim_regexp	*prog;
    int		fnum = -1;
    char_u	*pat;
    char_u	*patend;
    char_u	*match;
    int		attempt;
    char_u	*p;
    int		toggledollar;

    if (pattern_end == pattern + 1 && (*pattern == '%' || *pattern == '#'))
    {
	if (*pattern == '%')
	    fnum = curbuf->b_fnum;
	else
	    fnum = curwin->w_alt_fnum;
    }

    /*
     * Try four ways of matching:
     * attempt == 0: without '^' or '$' (at any position)
     * attempt == 1: with '^' at start (only at postion 0)
     * attempt == 2: with '$' at end (only match at end)
     * attempt == 3: with '^' at start and '$' at end (only full match)
     */
    else
    {
	pat = file_pat_to_reg_pat(pattern, pattern_end, NULL, FALSE);
	if (pat == NULL)
	    return -1;
	patend = pat + STRLEN(pat) - 1;
	toggledollar = (patend > pat && *patend == '$');

	for (attempt = 0; attempt <= 3; ++attempt)
	{
	    /* may add '^' and '$' */
	    if (toggledollar)
		*patend = (attempt < 2) ? NUL : '$';	/* add/remove '$' */
	    p = pat;
	    if (*p == '^' && !(attempt & 1))		/* add/remove '^' */
		++p;
	    prog = vim_regcomp(p, (int)p_magic);
	    if (prog == NULL)
	    {
		vim_free(pat);
		return -1;
	    }

	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	    {
		match = buflist_match(prog, buf);
		if (match != NULL)
		{
		    if (fnum >= 0)		/* already found a match */
		    {
			fnum = -2;
			break;
		    }
		    fnum = buf->b_fnum;	/* remember first match */
		}
	    }
	    vim_free(prog);
	    if (fnum >= 0)			/* found one match */
		break;
	}
	vim_free(pat);
    }

    if (fnum == -2)
	EMSG2("More than one match for %s", pattern);
    else if (fnum < 1)
	EMSG2("No matching buffer for %s", pattern);
    return fnum;
}

#ifdef CMDLINE_COMPL

/*
 * Find all buffer names that match.
 * For command line expansion of ":buf" and ":sbuf".
 * Return OK if matches found, FAIL otherwise.
 */
    int
ExpandBufnames(pat, num_file, file, options)
    char_u	*pat;
    int		*num_file;
    char_u	***file;
    int		options;
{
    int		count = 0;
    BUF		*buf;
    int		round;
    char_u	*p;
    int		attempt;
    vim_regexp	*prog;

    *num_file = 0;		    /* return values in case of FAIL */
    *file = NULL;

    /*
     * attempt == 1: try match with    '^', match at start
     * attempt == 2: try match without '^', match anywhere
     */
    for (attempt = 1; attempt <= 2; ++attempt)
    {
	if (attempt == 2)
	{
	    if (*pat != '^')	    /* there's no '^', no need to try again */
		break;
	    ++pat;		    /* skip the '^' */
	}
	prog = vim_regcomp(pat, (int)p_magic);
	if (prog == NULL)
	    return FAIL;

	/*
	 * round == 1: Count the matches.
	 * round == 2: Build the array to keep the matches.
	 */
	for (round = 1; round <= 2; ++round)
	{
	    count = 0;
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	    {
		p = buflist_match(prog, buf);
		if (p != NULL)
		{
		    if (round == 1)
			++count;
		    else
		    {
			if (options & WILD_HOME_REPLACE)
			    p = home_replace_save(buf, p);
			else
			    p = vim_strsave(p);
			(*file)[count++] = p;
		    }
		}
	    }
	    if (count == 0)	/* no match found, break here */
		break;
	    if (round == 1)
	    {
		*file = (char_u **)alloc((unsigned)(count * sizeof(char_u *)));
		if (*file == NULL)
		{
		    vim_free(prog);
		    return FAIL;
		}
	    }
	}
	vim_free(prog);
	if (count)		/* match(es) found, break here */
	    break;
    }

    *num_file = count;
    return (count == 0 ? FAIL : OK);
}

#endif /* CMDLINE_COMPL */

/*
 * Check for a match on the file name for buffer "buf" with regex prog "prog".
 */
    static char_u *
buflist_match(prog, buf)
    vim_regexp	*prog;
    BUF		*buf;
{
    char_u  *match;
    int	    save_reg_ic = reg_ic;

#ifdef CASE_INSENSITIVE_FILENAME
    reg_ic = TRUE;		/* Always ignore case */
#else
    reg_ic = FALSE;		/* Never ignore case */
#endif

    /* First try the short file name, then the long file name. */
    match = buflist_match_try(prog, buf->b_sfname);
    if (match == NULL)
	match = buflist_match_try(prog, buf->b_ffname);

    reg_ic = save_reg_ic;

    return match;
}

    static char_u *
buflist_match_try(prog, name)
    vim_regexp	*prog;
    char_u	*name;
{
    char_u  *match = NULL;
    char_u  *p;

    if (name != NULL)
    {
	if (vim_regexec(prog, name, TRUE) != 0)
	    match = name;
	else
	{
	    /* Replace $(HOME) with '~' and try matching again. */
	    p = home_replace_save(NULL, name);
	    if (p != NULL && vim_regexec(prog, p, TRUE) != 0)
		match = name;
	    vim_free(p);
	}
    }
    return match;
}

/*
 * find file in buffer name list by number
 */
    BUF	*
buflist_findnr(nr)
    int		nr;
{
    BUF		*buf;
    WIN		*wp;

    if (nr == 0)
	nr = curwin->w_alt_fnum;
    if (vim_strchr(p_swb, 'u'))		/* "useopen" if possible */
	for (wp = firstwin; wp; wp = wp->w_next)
	    if (wp->w_buffer->b_fnum == nr)
		return wp->w_buffer;
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (buf->b_fnum == nr)
	    return (buf);
    return NULL;
}

/*
 * Get name of file 'n' in the buffer list.
 * home_replace() is used to shorten the file name (used for marks).
 * Returns a pointer to allocated memory, of NULL when failed.
 */
    char_u *
buflist_nr2name(n, fullname, helptail)
    int n;
    int fullname;
    int helptail;	    /* for help buffers return tail only */
{
    BUF	    *buf;

    buf = buflist_findnr(n);
    if (buf == NULL)
	return NULL;
    return home_replace_save(helptail ? buf : NULL,
				     fullname ? buf->b_ffname : buf->b_fname);
}

/*
 * set the lnum and col for the buffer 'buf' and the current window
 */
    static void
buflist_setfpos(buf, lnum, col)
    BUF		*buf;
    linenr_t	lnum;
    colnr_t	col;
{
    WINFPOS	*wlp;

    for (wlp = buf->b_winfpos; wlp != NULL; wlp = wlp->wl_next)
	if (wlp->wl_win == curwin)
	    break;
    if (wlp == NULL)		/* make new entry */
    {
	wlp = (WINFPOS *)alloc((unsigned)sizeof(WINFPOS));
	if (wlp == NULL)
	    return;
	wlp->wl_win = curwin;
    }
    else			/* remove entry from list */
    {
	if (wlp->wl_prev)
	    wlp->wl_prev->wl_next = wlp->wl_next;
	else
	    buf->b_winfpos = wlp->wl_next;
	if (wlp->wl_next)
	    wlp->wl_next->wl_prev = wlp->wl_prev;
    }
    wlp->wl_fpos.lnum = lnum;
    wlp->wl_fpos.col = col;

    /* insert entry in front of the list */
    wlp->wl_next = buf->b_winfpos;
    buf->b_winfpos = wlp;
    wlp->wl_prev = NULL;
    if (wlp->wl_next)
	wlp->wl_next->wl_prev = wlp;

    return;
}

/*
 * find the position (lnum and col) for the buffer 'buf' for the current window
 * returns a pointer to no_position if no position is found
 */
    FPOS *
buflist_findfpos(buf)
    BUF		*buf;
{
    WINFPOS	*wlp;
    static FPOS no_position = {1, 0};

    for (wlp = buf->b_winfpos; wlp != NULL; wlp = wlp->wl_next)
	if (wlp->wl_win == curwin)
	    break;

    if (wlp == NULL)	/* if no fpos for curwin, use the first in the list */
	wlp = buf->b_winfpos;

    if (wlp != NULL)
	return &(wlp->wl_fpos);
    else
	return &no_position;
}

/*
 * find the lnum for the buffer 'buf' for the current window
 */
    linenr_t
buflist_findlnum(buf)
    BUF	    *buf;
{
    return buflist_findfpos(buf)->lnum;
}

/*
 * list all know file names (for :files and :buffers command)
 */
    void
buflist_list()
{
    BUF		*buf;
    int		len;
    int		i;

    for (buf = firstbuf; buf != NULL && !got_int; buf = buf->b_next)
    {
	msg_putchar('\n');
	if (buf->b_fname == NULL)
	    STRCPY(NameBuff, "No File");
	else
	    home_replace(buf, buf->b_fname, NameBuff, MAXPATHL, TRUE);

	sprintf((char *)IObuff, "%3d %c%c%c \"",
		buf->b_fnum,
		buf == curbuf ? '%' :
			(curwin->w_alt_fnum == buf->b_fnum ? '#' : ' '),
		buf->b_ml.ml_mfp == NULL ? '-' :
			(buf->b_nwindows == 0 ? 'h' : ' '),
		buf_changed(buf) ? '+' : ' ');

	len = STRLEN(IObuff);
	STRNCPY(IObuff + len, NameBuff, IOSIZE - 20 - len);

	len = STRLEN(IObuff);
	IObuff[len++] = '"';

	/* put "line 999" in column 40 or after the file name */
	IObuff[len] = NUL;
	i = 40 - vim_strsize(IObuff);
	do
	{
	    IObuff[len++] = ' ';
	} while (--i > 0 && len < IOSIZE - 18);
	sprintf((char *)IObuff + len, "line %ld",
		buf == curbuf ? curwin->w_cursor.lnum :
				(long)buflist_findlnum(buf));
	msg_outtrans(IObuff);
	out_flush();	    /* output one line at a time */
	ui_breakcheck();
    }
}


/*
 * Get file name and line number for file 'fnum'.
 * Used by DoOneCmd() for translating '%' and '#'.
 * Used by insert_reg() and cmdline_paste() for '#' register.
 * Return FAIL if not found, OK for success.
 */
    int
buflist_name_nr(fnum, fname, lnum)
    int		fnum;
    char_u	**fname;
    linenr_t	*lnum;
{
    BUF		*buf;

    buf = buflist_findnr(fnum);
    if (buf == NULL || buf->b_fname == NULL)
	return FAIL;

    *fname = buf->b_fname;
    *lnum = buflist_findlnum(buf);

    return OK;
}

/*
 * Set the current file name to 'ffname', short file name to 'sfname'.
 * The file name with the full path is also remembered, for when :cd is used.
 * Returns FAIL for failure (file name already in use by other buffer)
 *	OK otherwise.
 */
    int
setfname(ffname, sfname, message)
    char_u *ffname, *sfname;
    int	    message;
{
    BUF		*buf;
#ifdef UNIX
    struct stat st;
#endif

    if (ffname == NULL || *ffname == NUL)
    {
	vim_free(curbuf->b_ffname);
	vim_free(curbuf->b_sfname);
	curbuf->b_ffname = NULL;
	curbuf->b_sfname = NULL;
#ifdef UNIX
	st.st_dev = (unsigned)-1;
#endif
    }
    else
    {
	fname_expand(&ffname, &sfname);	    /* will allocate ffname */
	if (ffname == NULL)		    /* out of memory */
	    return FAIL;

#ifdef USE_FNAME_CASE
# ifdef USE_LONG_FNAME
	if (USE_LONG_FNAME)
# endif
	    fname_case(sfname);	    /* set correct case for short file name */
#endif
	/*
	 * if the file name is already used in another buffer:
	 * - if the buffer is loaded, fail
	 * - if the buffer is not loaded, delete it from the list
	 */
#ifdef UNIX
	if (mch_stat((char *)ffname, &st) < 0)
	    st.st_dev = (unsigned)-1;
	buf = buflist_findname_stat(ffname, &st);
#else
	buf = buflist_findname(ffname);
#endif
	if (buf != NULL && buf != curbuf)
	{
	    if (buf->b_ml.ml_mfp != NULL)	/* it's loaded, fail */
	    {
		if (message)
		    EMSG("Buffer with this name already exists");
		vim_free(ffname);
		return FAIL;
	    }
	    close_buffer(NULL, buf, TRUE, TRUE);    /* delete from the list */
	}
	sfname = vim_strsave(sfname);
	if (ffname == NULL || sfname == NULL)
	{
	    vim_free(sfname);
	    vim_free(ffname);
	    return FAIL;
	}
	vim_free(curbuf->b_ffname);
	vim_free(curbuf->b_sfname);
	curbuf->b_ffname = ffname;
	curbuf->b_sfname = sfname;
    }
    curbuf->b_fname = curbuf->b_sfname;
#ifdef UNIX
    if (st.st_dev == (unsigned)-1)
	curbuf->b_dev = -1;
    else
    {
	curbuf->b_dev = st.st_dev;
	curbuf->b_ino = st.st_ino;
    }
#endif

#ifndef SHORT_FNAME
    curbuf->b_shortname = FALSE;
#endif
    /*
     * If the file name changed, also change the name of the swapfile
     */
    if (curbuf->b_ml.ml_mfp != NULL)
	ml_setname();

    check_arg_idx(curwin);	/* check file name for arg list */
#ifdef WANT_TITLE
    maketitle();		/* set window title */
#endif
    status_redraw_all();	/* status lines need to be redrawn */
    fmarks_check_names(curbuf);	/* check named file marks */
    ml_timestamp(curbuf);	/* reset timestamp */
    return OK;
}

/*
 * set alternate file name for current window
 *
 * Used by do_one_cmd(), do_write() and do_ecmd().
 */
    void
setaltfname(ffname, sfname, lnum)
    char_u	*ffname;
    char_u	*sfname;
    linenr_t	lnum;
{
    BUF	    *buf;

    buf = buflist_new(ffname, sfname, lnum, FALSE);
    if (buf != NULL)
	curwin->w_alt_fnum = buf->b_fnum;
}

/*
 * Get alternate file name for current window.
 * Return NULL if there isn't any, and give error message if requested.
 */
    char_u  *
getaltfname(errmsg)
    int		errmsg;		/* give error message */
{
    char_u	*fname;
    linenr_t	dummy;

    if (buflist_name_nr(0, &fname, &dummy) == FAIL)
    {
	if (errmsg)
	    emsg(e_noalt);
	return NULL;
    }
    return fname;
}

/*
 * add a file name to the buflist and return its number
 *
 * used by qf_init(), main() and doarglist()
 */
    int
buflist_add(fname)
    char_u	*fname;
{
    BUF	    *buf;

    buf = buflist_new(fname, NULL, (linenr_t)0, FALSE);
    if (buf != NULL)
	return buf->b_fnum;
    return 0;
}

/*
 * set alternate cursor position for current window
 */
    void
buflist_altfpos()
{
    buflist_setfpos(curbuf, curwin->w_cursor.lnum, curwin->w_cursor.col);
}

/*
 * Return TRUE if 'ffname' is not the same file as current file.
 * Fname must have a full path (expanded by mch_FullName).
 */
    int
otherfile(ffname)
    char_u	*ffname;
{
    return otherfile_buf(curbuf, ffname
#ifdef UNIX
	    , NULL
#endif
	    );
}

    static int
otherfile_buf(buf, ffname
#ifdef UNIX
	, stp
#endif
	)
    BUF		*buf;
    char_u	*ffname;
#ifdef UNIX
    struct stat	*stp;
#endif
{
    /* no name is different */
    if (ffname == NULL || *ffname == NUL || buf->b_ffname == NULL)
	return TRUE;
    if (fnamecmp(ffname, buf->b_ffname) == 0)
	return FALSE;
#ifdef UNIX
    {
	struct stat	st;

	/* If no struct stat given, get it now */
	if (stp == NULL)
	{
	    if (buf->b_dev < 0 || mch_stat((char *)ffname, &st) < 0)
		st.st_dev = (unsigned)-1;
	    stp = &st;
	}
	/* Use dev/ino to check if the files are the same, even when the names
	 * are different (possible with links).  Still need to compare the
	 * name above, for when the file doesn't exist yet.
	 * Problem: The dev/ino changes when a file is deleted (and created
	 * again) and remains the same when renamed/moved.  We don't want to
	 * mch_stat() each buffer each time, that would be too slow.  Get the
	 * dev/ino again when they appear to match, but not when they appear
	 * to be different: Could skip a buffer when it's actually the same
	 * file. */
	if (buf_same_ino(buf, stp))
	{
	    buf_setino(buf);
	    if (buf_same_ino(buf, stp))
		return FALSE;
	}
    }
#endif
    return TRUE;
}

#ifdef UNIX
/*
 * Set inode and device number for a buffer.
 * Must always be called when b_fname is changed!.
 */
    static void
buf_setino(buf)
    BUF		*buf;
{
    struct stat	st;

    if (buf->b_fname != NULL && mch_stat((char *)buf->b_fname, &st) >= 0)
    {
	buf->b_dev = st.st_dev;
	buf->b_ino = st.st_ino;
    }
    else
	buf->b_dev = -1;
}

/*
 * Return TRUE if dev/ino in buffer "buf" matches with "stp".
 */
    static int
buf_same_ino(buf, stp)
    BUF		*buf;
    struct stat *stp;
{
    return (buf->b_dev >= 0
	    && stp->st_dev == buf->b_dev
	    && stp->st_ino == buf->b_ino);
}
#endif

    void
fileinfo(fullname, shorthelp, dont_truncate)
    int fullname;
    int shorthelp;
    int	dont_truncate;
{
    char_u	*name;
    int		n;
    char_u	*p;
    char_u	*buffer;

    buffer = alloc(IOSIZE);
    if (buffer == NULL)
	return;

    if (fullname > 1)	    /* 2 CTRL-G: include buffer number */
    {
	sprintf((char *)buffer, "buf %d: ", curbuf->b_fnum);
	p = buffer + STRLEN(buffer);
    }
    else
	p = buffer;

    *p++ = '"';
    if (curbuf->b_ffname == NULL)
	STRCPY(p, "No File");
    else
    {
	if (!fullname && curbuf->b_fname != NULL)
	    name = curbuf->b_fname;
	else
	    name = curbuf->b_ffname;
	home_replace(shorthelp ? curbuf : NULL, name, p,
					  (int)(IOSIZE - (p - buffer)), TRUE);
    }

    sprintf((char *)buffer + STRLEN(buffer),
	    "\"%s%s%s%s%s%s",
	    curbuf_changed() ? (shortmess(SHM_MOD) ?
						" [+]" : " [Modified]") : " ",
	    (curbuf->b_flags & BF_NOTEDITED) ? "[Not edited]" : "",
	    (curbuf->b_flags & BF_NEW) ? "[New file]" : "",
	    (curbuf->b_flags & BF_READERR) ? "[Read errors]" : "",
	    curbuf->b_p_ro ? (shortmess(SHM_RO) ? "[RO]" : "[readonly]") : "",
	    (curbuf_changed() || (curbuf->b_flags & BF_WRITE_MASK)
							  || curbuf->b_p_ro) ?
								    " " : "");
    n = (int)(((long)curwin->w_cursor.lnum * 100L) /
					    (long)curbuf->b_ml.ml_line_count);
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
    {
	STRCPY(buffer + STRLEN(buffer), no_lines_msg);
    }
#ifdef CMDLINE_INFO
    else if (p_ru)
    {
	/* Current line and column are already on the screen -- webb */
	sprintf((char *)buffer + STRLEN(buffer),
	    "%ld line%s --%d%%--",
	    (long)curbuf->b_ml.ml_line_count,
	    plural((long)curbuf->b_ml.ml_line_count),
	    n);
    }
#endif
    else
    {
	sprintf((char *)buffer + STRLEN(buffer),
	    "line %ld of %ld --%d%%-- col ",
	    (long)curwin->w_cursor.lnum,
	    (long)curbuf->b_ml.ml_line_count,
	    n);
	validate_virtcol();
	col_print(buffer + STRLEN(buffer),
		   (int)curwin->w_cursor.col + 1, (int)curwin->w_virtcol + 1);
    }

    (void)append_arg_number(curwin, buffer, !shortmess(SHM_FILE), IOSIZE);

    if (dont_truncate)
    {
	/* Temporarily set msg_scroll to avoid the message being truncated */
	n = msg_scroll;
	msg_scroll = TRUE;
	msg(buffer);
	msg_scroll = n;
    }
    else
	msg_trunc_attr(buffer, FALSE, 0);

    vim_free(buffer);
}

/*
 * Give some info about the position of the cursor (for "g CTRL-G").
 */
    void
cursor_pos_info()
{
    char_u	*p;
    char_u	buf1[20];
    char_u	buf2[20];
    linenr_t	lnum;
    long	char_count = 0;
    long	char_count_cursor = 0;
    int		eol_size;
    long	last_check = 100000;

    /*
     * Compute the length of the file in characters.
     */
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
    {
	MSG(no_lines_msg);
    }
    else
    {
	if (get_fileformat(curbuf) == EOL_DOS)
	    eol_size = 2;
	else
	    eol_size = 1;
	for (lnum = 1; lnum <= curbuf->b_ml.ml_line_count; ++lnum)
	{
	    if (lnum == curwin->w_cursor.lnum)
		char_count_cursor = char_count + curwin->w_cursor.col + 1;
	    char_count += STRLEN(ml_get(lnum)) + eol_size;
	    /* Check for a CTRL-C every 100000 characters */
	    if (char_count > last_check)
	    {
		ui_breakcheck();
		if (got_int)
		    return;
		last_check = char_count + 100000;
	    }
	}
	if (!curbuf->b_p_eol && curbuf->b_p_bin)
	    char_count -= eol_size;

	p = ml_get_curline();
	validate_virtcol();
	col_print(buf1, (int)curwin->w_cursor.col + 1,
						  (int)curwin->w_virtcol + 1);
	col_print(buf2, (int)STRLEN(p), linetabsize(p));

	sprintf((char *)IObuff, "Col %s of %s; Line %ld of %ld; Char %ld of %ld",
		(char *)buf1, (char *)buf2,
		(long)curwin->w_cursor.lnum, (long)curbuf->b_ml.ml_line_count,
		char_count_cursor, char_count);
	msg(IObuff);
    }
}

    void
col_print(buf, col, vcol)
    char_u  *buf;
    int	    col;
    int	    vcol;
{
    if (col == vcol)
	sprintf((char *)buf, "%d", col);
    else
	sprintf((char *)buf, "%d-%d", col, vcol);
}

#ifdef WANT_TITLE
/*
 * put file name in title bar of window and in icon title
 */

static char_u *lasttitle = NULL;
static char_u *lasticon = NULL;

    void
maketitle()
{
    char_u	*t_name;
    char_u	*t_str = NULL;
    char_u	*i_name;
    char_u	*i_str = NULL;
    int		maxlen = 0;
    int		len;
    int         mustset;
    char_u	buf[IOSIZE];

    if (p_title)
    {
	if (p_titlelen > 0)
	{
	    maxlen = p_titlelen * Columns / 100;
	    if (maxlen < 10)
		maxlen = 10;
	}

	t_str = buf;
	if (*p_titlestring != NUL)
	{
#ifdef STATUSLINE
	    if (stl_syntax & STL_IN_TITLE)
		build_stl_str(curwin, t_str, p_titlestring, 0, maxlen);
	    else
#endif
		t_str = p_titlestring;
	}
	else
	{
	    if (curbuf->b_ffname == NULL)
		t_name = (char_u *)"VIM -";
	    else
	    {
		t_name = buf + 100; /* No more than a hundred unprintables */
		STRCPY(t_name, "VIM - ");
		home_replace(curbuf, curbuf->b_ffname,
			     t_name + 6, IOSIZE - 106, TRUE);
		append_arg_number(curwin, t_name, FALSE, IOSIZE - 100);
		if (maxlen)
		{
		    len = STRLEN(t_name);
		    if (len > maxlen)
		    {
			mch_memmove(t_name + 6, t_name + 6 + len - maxlen,
							  (size_t)maxlen - 5);
			t_name[5] = '<';
		    }
		}
	    }
	    *t_str = NUL;
	    while (*t_name)
		STRCAT(t_str, transchar(*t_name++));
	}
    }
    mustset = ti_change(t_str, &lasttitle);

    if (p_icon)
    {
	i_str = buf;
	if (*p_iconstring != NUL)
	{
#ifdef STATUSLINE
	    if (stl_syntax & STL_IN_ICON)
		build_stl_str(curwin, i_str, p_iconstring, 0, 0);
	    else
#endif
		i_str = p_iconstring;
	}
	else
	{
	    if (curbuf->b_ffname == NULL)
		i_name = (char_u *)"No File";
	    else		    /* use file name only in icon */
		i_name = gettail(curbuf->b_ffname);
	    *i_str = NUL;
	    while (*i_name)
		STRCAT(i_str, transchar(*i_name++));
	}
    }

    mustset |= ti_change(i_str, &lasticon);

    if (mustset)
	resettitle();
}

/*
 * Used for title and icon: Check if "str" differs from "*last".  Set "*last"
 * from "str" if it does.
 * Return TRUE when "*last" changed.
 */
    static int
ti_change(str, last)
    char_u	*str;
    char_u	**last;
{
    if ((str == NULL) != (*last == NULL)
	    || (str != NULL && *last != NULL && STRCMP(str, *last) != 0))
    {
	vim_free(*last);
	if (str == NULL)
	    *last = NULL;
	else
	    *last = vim_strsave(str);
	return TRUE;
    }
    return FALSE;
}

/*
 * Put current window title back (used after calling a shell)
 */
    void
resettitle()
{
    mch_settitle(lasttitle, lasticon);
}
#endif /* WANT_TITLE */

/*
 * Append (file 2 of 8) to 'buf', if editing more than one file.
 * Return TRUE if it was appended.
 */
    int
append_arg_number(wp, buf, add_file, maxlen)
    WIN	    *wp;
    char_u  *buf;
    int	    add_file;		/* Add "file" before the arg number */
    int	    maxlen;		/* maximum nr of chars in buf or zero*/
{
    char_u	*p;

    if (arg_file_count <= 1)		/* nothing to do */
	return FALSE;

    p = buf + STRLEN(buf);		/* go to the end of the buffer */
    if (maxlen && p - buf + 35 >= maxlen) /* getting too long */
	return FALSE;
    *p++ = ' ';
    *p++ = '(';
    if (add_file)
    {
	STRCPY(p, "file ");
	p += 5;
    }
    sprintf((char *)p, wp->w_arg_idx_invalid ? "(%d) of %d)"
			    : "%d of %d)", wp->w_arg_idx + 1, arg_file_count);
    return TRUE;
}

/*
 * If fname is not a full path, make it a full path.
 * Returns pointer to allocated memory (NULL for failure).
 */
    char_u  *
fix_fname(fname)
    char_u  *fname;
{
    /*
     * Force expanding the path always for Unix, because symbolic links may
     * mess up the full path name, even though it starts with a '/'.
     */
#ifdef UNIX
    return FullName_save(fname, TRUE);
#else
    if (!mch_isFullName(fname))
	return FullName_save(fname, FALSE);

    fname = vim_strsave(fname);

#ifdef USE_FNAME_CASE
# ifdef USE_LONG_FNAME
    if (USE_LONG_FNAME)
# endif
    {
	if (fname != NULL)
	    fname_case(fname);		/* set correct case for file name */
    }
#endif

    return fname;
#endif
}

/*
 * make ffname a full file name, set sfname to ffname if not NULL
 * ffname becomes a pointer to allocated memory (or NULL).
 */
    void
fname_expand(ffname, sfname)
    char_u	**ffname;
    char_u	**sfname;
{
    if (*ffname == NULL)	/* if no file name given, nothing to do */
	return;
    if (*sfname == NULL)	/* if no short file name given, use ffname */
	*sfname = *ffname;
    *ffname = fix_fname(*ffname);   /* expand to full path */
}

/*
 * do_arg_all(): Open up to 'count' windows, one for each argument.
 */
    void
do_arg_all(count, forceit)
    int	count;
    int	forceit;		/* hide buffers in current windows */
{
    int		i;
    WIN		*wp, *wpnext;
    char_u	*opened;	/* array of flags for which args are open */
    int		opened_len;	/* lenght of opened[] */
    int		use_firstwin = FALSE;	/* use first window for arglist */
    int		split_ret = OK;
    int		p_sb_save;
    int		p_ea_save;

    if (arg_file_count <= 0)
    {
	/* Don't give an error message.  We don't want it when the ":all"
	 * command is in the .vimrc. */
	return;
    }
    setpcmark();

    opened_len = arg_file_count;
    opened = alloc_clear((unsigned)opened_len);
    if (opened == NULL)
	return;

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

    /*
     * Try closing all windows that are not in the argument list.
     * When 'hidden' or "forceit" set the buffer becomes hidden.
     * Windows that have a changed buffer and can't be hidden won't be closed.
     */
    for (wp = firstwin; wp != NULL; wp = wpnext)
    {
	wpnext = wp->w_next;
	if (wp->w_buffer->b_ffname == NULL || wp->w_buffer->b_nwindows > 1)
	    i = arg_file_count;
	else
	{
	    /* check if the buffer in this window is in the arglist */
	    for (i = 0; i < arg_file_count; ++i)
	    {
		if (fullpathcmp(arg_files[i],
				     wp->w_buffer->b_ffname, TRUE) & FPC_SAME)
		{
		    if (i < opened_len)
			opened[i] = TRUE;
		    break;
		}
	    }
	}
	wp->w_arg_idx = i;

	if (i == arg_file_count)		/* close this window */
	{
	    if (p_hid || forceit || wp->w_buffer->b_nwindows > 1
						|| !buf_changed(wp->w_buffer))
	    {
		/* If the buffer was changed, and we would like to hide it,
		 * try autowriting. */
		if (!p_hid && wp->w_buffer->b_nwindows <= 1
						 && buf_changed(wp->w_buffer))
		{
		    autowrite(wp->w_buffer, FALSE);
#ifdef AUTOCMD
		    /* check if autocommands removed the window */
		    if (!win_valid(wp))
		    {
			wpnext = firstwin;	/* start all over... */
			continue;
		    }
#endif
		}
		if (firstwin == lastwin)	/* can't close last window */
		    use_firstwin = TRUE;
		else
		{
		    close_window(wp, !p_hid && !buf_changed(wp->w_buffer));
#ifdef AUTOCMD
		    /* check if autocommands removed the next window */
		    if (!win_valid(wpnext))
			wpnext = firstwin;	/* start all over... */
#endif
		}
	    }
	}
    }

    /*
     * Open a window for files in the argument list that don't have one.
     * arg_file_count may change while doing this, because of autocommands.
     */
    if (count > arg_file_count || count <= 0)
	count = arg_file_count;

#ifdef AUTOCMD
    /* Don't execute Win/Buf Enter/Leave autocommands here. */
    ++autocmd_no_enter;
    ++autocmd_no_leave;
#endif
    win_enter(lastwin, FALSE);
    for (i = 0; i < count && i < arg_file_count && !got_int; ++i)
    {
	if (i == arg_file_count - 1)
	    arg_had_last = TRUE;
	if (i < opened_len && opened[i])
	{
	    /* Move the already present window to below the current window */
	    if (curwin->w_arg_idx != i)
	    {
		for (wpnext = firstwin; wpnext != NULL; wpnext = wpnext->w_next)
		{
		    if (wpnext->w_arg_idx == i)
		    {
			win_move_after(wpnext, curwin);
			break;
		    }
		}
	    }
	}
	else if (split_ret == OK)
	{
	    if (!use_firstwin)		/* split current window */
	    {
		p_sb_save = p_sb;
		p_ea_save = p_ea;
		p_sb = TRUE;		/* put windows in order of arglist */
		p_ea = TRUE;		/* use space from all windows */
		split_ret = win_split(0, FALSE, TRUE);
		p_sb = p_sb_save;
		p_ea = p_ea_save;
		if (split_ret == FAIL)
		    continue;
	    }
#ifdef AUTOCMD
	    else    /* first window: do autocmd for leaving this buffer */
		--autocmd_no_leave;
#endif

	    curwin->w_arg_idx = i;
	    /* edit file i */
	    (void)do_ecmd(0, arg_files[i], NULL, NULL, ECMD_ONE,
		   ((p_hid || buf_changed(curwin->w_buffer)) ? ECMD_HIDE : 0)
							       + ECMD_OLDBUF);
#ifdef AUTOCMD
	    if (use_firstwin)
		++autocmd_no_leave;
#endif
	    use_firstwin = FALSE;
	}
	ui_breakcheck();
    }
#ifdef AUTOCMD
    --autocmd_no_enter;
#endif
    win_enter(firstwin, FALSE);			/* back to first window */
#ifdef AUTOCMD
    --autocmd_no_leave;
#endif
}

/*
 * do_buffer_all: Open a window for each buffer.
 *
 * 'count' is the maximum number of windows to open.
 * When 'all' is TRUE, also load inactive buffers.
 */
    void
do_buffer_all(count, all)
    int	    count;
    int	    all;
{
    BUF		*buf;
    WIN		*wp, *wpnext;
    int		split_ret = OK;
    int		p_sb_save;
    int		p_ea_save;
    int		open_wins = 0;

    setpcmark();

#ifdef USE_GUI
    need_mouse_correct = TRUE;
#endif

    /*
     * Close superfluous windows (two windows for the same buffer).
     */
    for (wp = firstwin; wp != NULL; wp = wpnext)
    {
	wpnext = wp->w_next;
	if (wp->w_buffer->b_nwindows > 1)
	{
	    close_window(wp, FALSE);
#ifdef AUTOCMD
	    wpnext = firstwin;	    /* just in case an autocommand does
				       something strange with windows */
	    open_wins = 0;
#endif
	}
	else
	    ++open_wins;
    }

    /*
     * Go through the buffer list.  When a buffer doesn't have a window yet,
     * open one.  Otherwise move the window to the right position.
     * Watch out for autocommands that delete buffers or windows!
     */
#ifdef AUTOCMD
    /* Don't execute Win/Buf Enter/Leave autocommands here. */
    ++autocmd_no_enter;
#endif
    win_enter(lastwin, FALSE);
#ifdef AUTOCMD
    ++autocmd_no_leave;
#endif
    for (buf = firstbuf; buf != NULL && open_wins < count; buf = buf->b_next)
    {
	/* Check if this buffer needs a window */
	if (!all && buf->b_ml.ml_mfp == NULL)
	    continue;

	/* Check if this buffer already has a window */
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	    if (wp->w_buffer == buf)
		break;
	/* If the buffer already has a window, move it */
	if (wp != NULL)
	    win_move_after(wp, curwin);
	else if (split_ret == OK)
	{
	    /* Split the window and put the buffer in it */
	    p_sb_save = p_sb;
	    p_ea_save = p_ea;
	    p_sb = TRUE;		/* put windows in order of arglist */
	    p_ea = TRUE;		/* use space from all windows */
	    split_ret = win_split(0, FALSE, TRUE);
	    ++open_wins;
	    p_sb = p_sb_save;
	    p_ea = p_ea_save;
	    if (split_ret == FAIL)
		continue;

	    /* Open the buffer in this window. */
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	    swap_exists_action = SEA_DIALOG;
#endif
	    (void)do_buffer(DOBUF_GOTO, DOBUF_FIRST, FORWARD,
							 (int)buf->b_fnum, 0);
#ifdef AUTOCMD
	    if (!buf_valid(buf))	/* autocommands deleted the buffer!!! */
	    {
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
		swap_exists_action = SEA_NONE;
#endif
		break;
	    }
#endif
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
	    if (swap_exists_action == SEA_QUIT)
	    {
		/* User selected Quit at ATTENTION prompt; close this window. */
		close_window(curwin, TRUE);
		--open_wins;
	    }
	    else if (swap_exists_action == SEA_RECOVER)
	    {
		/* User selected Recover at ATTENTION prompt. */
		ml_recover();
		MSG_PUTS("\n");	/* don't overwrite the last message */
		cmdline_row = msg_row;
		do_modelines();
	    }
	    swap_exists_action = SEA_NONE;
#endif
	}

	ui_breakcheck();
	if (got_int)
	{
	    (void)vgetc();	/* only break the file loading, not the rest */
	    break;
	}
    }
#ifdef AUTOCMD
    --autocmd_no_enter;
#endif
    win_enter(firstwin, FALSE);		/* back to first window */
#ifdef AUTOCMD
    --autocmd_no_leave;
#endif

    /*
     * Close superfluous windows.
     */
    for (wp = lastwin; open_wins > count; )
    {
	if (p_hid || !buf_changed(wp->w_buffer)
				      || autowrite(wp->w_buffer, FALSE) == OK)
	{
	    close_window(wp, !p_hid);
	    --open_wins;
	    wp = lastwin;
	}
	else
	{
	    wp = wp->w_prev;
	    if (wp == NULL)
		break;
	}
    }
}

/*
 * do_modelines() - process mode lines for the current file
 *
 * Returns immediately if the "ml" option isn't set.
 */
static int  chk_modeline __ARGS((linenr_t));

    void
do_modelines()
{
    linenr_t	    lnum;
    int		    nmlines;
    static int	    entered = 0;

    if (!curbuf->b_p_ml || (nmlines = (int)p_mls) == 0)
	return;

    /* Disallow recursive entry here.  Can happen when executing a modeline
     * triggers an autocommand, which reloads modelines with a ":do". */
    if (entered)
	return;

    ++entered;
    for (lnum = 1; lnum <= curbuf->b_ml.ml_line_count && lnum <= nmlines;
								       ++lnum)
	if (chk_modeline(lnum) == FAIL)
	    nmlines = 0;

    for (lnum = curbuf->b_ml.ml_line_count; lnum > 0 && lnum > nmlines &&
			  lnum > curbuf->b_ml.ml_line_count - nmlines; --lnum)
	if (chk_modeline(lnum) == FAIL)
	    nmlines = 0;
    --entered;
}

/*
 * chk_modeline() - check a single line for a mode string
 * Return FAIL if an error encountered.
 */
    static int
chk_modeline(lnum)
    linenr_t	lnum;
{
    char_u	*s;
    char_u	*e;
    char_u	*linecopy;		/* local copy of any modeline found */
    int		prev;
    int		end;
    int		retval = OK;
    char_u	*save_sourcing_name;
    linenr_t	save_sourcing_lnum;

    prev = -1;
    for (s = ml_get(lnum); *s != NUL; ++s)
    {
	if (prev == -1 || vim_isspace(prev))
	{
	    if ((prev != -1 && STRNCMP(s, "ex:", (size_t)3) == 0) ||
			       STRNCMP(s, "vi:", (size_t)3) == 0 ||
			       STRNCMP(s, "vim:", (size_t)4) == 0)
		break;
	}
	prev = *s;
    }

    if (*s)
    {
	do				/* skip over "ex:", "vi:" or "vim:" */
	    ++s;
	while (s[-1] != ':');

	s = linecopy = vim_strsave(s);	/* copy the line, it will change */
	if (linecopy == NULL)
	    return FAIL;

	save_sourcing_lnum = sourcing_lnum;
	save_sourcing_name = sourcing_name;
	sourcing_lnum = lnum;		/* prepare for emsg() */
	sourcing_name = (char_u *)"modelines";

	end = FALSE;
	while (end == FALSE)
	{
	    s = skipwhite(s);
	    if (*s == NUL)
		break;

	    /*
	     * Find end of set command: ':' or end of line.
	     * Skip over "\:", replacing it with ":".
	     */
	    for (e = s; *e != ':' && *e != NUL; ++e)
		if (e[0] == '\\' && e[1] == ':')
		    STRCPY(e, e + 1);
	    if (*e == NUL)
		end = TRUE;

	    /*
	     * If there is a "set" command, require a terminating ':' and
	     * ignore the stuff after the ':'.
	     * "vi:set opt opt opt: foo" -- foo not interpreted
	     * "vi:opt opt opt: foo" -- foo interpreted
	     */
	    if (STRNCMP(s, "set ", (size_t)4) == 0)
	    {
		if (*e != ':')		/* no terminating ':'? */
		    break;
		end = TRUE;
		s += 4;
	    }

	    *e = NUL;			/* truncate the set command */
	    if (do_set(s, TRUE) == FAIL)/* stop if error found */
	    {
		retval = FAIL;
		break;
	    }
	    s = e + 1;			/* advance to next part */
	}

	sourcing_lnum = save_sourcing_lnum;
	sourcing_name = save_sourcing_name;

	vim_free(linecopy);
    }
    return retval;
}

#ifdef VIMINFO
    int
read_viminfo_bufferlist(line, fp, writing)
    char_u	*line;
    FILE	*fp;
    int		writing;
{
    char_u	*tab;
    linenr_t	lnum;
    colnr_t	col;
    BUF		*buf;
    char_u	*sfname;
    char_u	*xline;

    /* Handle long line and escaped characters. */
    xline = viminfo_readstring(line + 1, fp);

    /* don't read in if there are files on the command-line or if writing: */
    if (xline != NULL && !writing && arg_file_count == 0
				       && find_viminfo_parameter('%') != NULL)
    {
	/* Format is: <fname> Tab <lnum> Tab <col>.
	 * Watch out for a Tab in the file name, work from the end. */
	lnum = 0;
	col = 0;
	tab = vim_strrchr(xline, '\t');
	if (tab != NULL)
	{
	    *tab++ = '\0';
	    col = atoi((char *)tab);
	    tab = vim_strrchr(xline, '\t');
	    if (tab != NULL)
	    {
		*tab++ = '\0';
		lnum = atol((char *)tab);
	    }
	}

	/* Expand "~/" in the file name at "line + 1" to a full path.
	 * Then try shortening it by comparing with the current directory */
	expand_env(xline, NameBuff, MAXPATHL);
	mch_dirname(IObuff, IOSIZE);
	sfname = shorten_fname(NameBuff, IObuff);
	if (sfname == NULL)
	    sfname = NameBuff;

	buf = buflist_new(NameBuff, sfname, (linenr_t)0, FALSE);
	if (buf != NULL)	/* just in case... */
	{
	    buf->b_last_cursor.lnum = lnum;
	    buf->b_last_cursor.col = col;
	    buflist_setfpos(buf, lnum, col);
	}
    }
    vim_free(xline);

    return vim_fgets(line, LSIZE, fp);
}

    void
write_viminfo_bufferlist(fp)
    FILE    *fp;
{
    BUF		*buf;
    WIN		*win;
    char_u	*line;

    if (find_viminfo_parameter('%') == NULL)
	return;

    /* Allocate room for the file name, lnum and col. */
    line = alloc(MAXPATHL + 30);
    if (line == NULL)
	return;

    for (win = firstwin; win != NULL; win = win->w_next)
	set_last_cursor(win);

    fprintf(fp, "\n# Buffer list:\n");
    for (buf = firstbuf; buf != NULL ; buf = buf->b_next)
    {
	if (buf->b_fname == NULL || buf->b_help || removable(buf->b_ffname))
	    continue;

	putc('%', fp);
	home_replace(NULL, buf->b_ffname, line, MAXPATHL, TRUE);
	sprintf((char *)line + STRLEN(line), "\t%ld\t%d",
			(long)buf->b_last_cursor.lnum,
			buf->b_last_cursor.col);
	viminfo_writestring(fp, line);
    }
    vim_free(line);
}
#endif
