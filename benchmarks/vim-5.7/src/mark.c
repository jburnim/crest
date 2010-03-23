/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * mark.c: functions for setting marks and jumping to them
 */

#include "vim.h"

/*
 * This file contains routines to maintain and manipulate marks.
 */

/*
 * If a named file mark's lnum is non-zero, it is valid.
 * If a named file mark's fnum is non-zero, it is for an existing buffer,
 * otherwise it is from .viminfo and namedfm_names[n] is the file name.
 * There are marks 'A - 'Z (set by user) and '0 to '9 (set when writing
 * viminfo).
 */
#define EXTRA_MARKS 10					/* marks 0-9 */
static struct filemark namedfm[NMARKS + EXTRA_MARKS];	/* marks with file nr */
static char_u *namedfm_names[NMARKS + EXTRA_MARKS];	/* name for namedfm[] */

static char_u *mark_line __ARGS((FPOS *mp, int lead_len));
static void show_one_mark __ARGS((int, char_u *, FPOS *, char_u *, int current));
static void cleanup_jumplist __ARGS((void));

/*
 * setmark(c) - set named mark 'c' at current cursor position
 *
 * Returns OK on success, FAIL if bad name given.
 */
    int
setmark(c)
    int		c;
{
    int		i;

    if (c == '\'' || c == '`')
    {
	setpcmark();
	/* keep it even when the cursor doesn't move */
	curwin->w_prev_pcmark = curwin->w_pcmark;
	return OK;
    }

    if (c > 'z')	    /* some islower() and isupper() cannot handle
				characters above 127 */
	return FAIL;
    if (islower(c))
    {
	i = c - 'a';
	curbuf->b_namedm[i] = curwin->w_cursor;
	return OK;
    }
    if (isupper(c))
    {
	i = c - 'A';
	namedfm[i].mark = curwin->w_cursor;
	namedfm[i].fnum = curbuf->b_fnum;
	return OK;
    }
    return FAIL;
}

/*
 * setpcmark() - set the previous context mark to the current position
 *		 and add it to the jump list
 */
    void
setpcmark()
{
    int i;
#ifdef ROTATE
    struct filemark tempmark;
#endif

    /* for :global the mark is set only once */
    if (global_busy)
	return;

    curwin->w_prev_pcmark = curwin->w_pcmark;
    curwin->w_pcmark = curwin->w_cursor;

#ifdef ROTATE
    /*
     * If last used entry is not at the top, put it at the top by rotating
     * the stack until it is (the newer entries will be at the bottom).
     * Keep one entry (the last used one) at the top.
     */
    if (curwin->w_jumplistidx < curwin->w_jumplistlen)
	++curwin->w_jumplistidx;
    while (curwin->w_jumplistidx < curwin->w_jumplistlen)
    {
	tempmark = curwin->w_jumplist[curwin->w_jumplistlen - 1];
	for (i = curwin->w_jumplistlen - 1; i > 0; --i)
	    curwin->w_jumplist[i] = curwin->w_jumplist[i - 1];
	curwin->w_jumplist[0] = tempmark;
	++curwin->w_jumplistidx;
    }
#endif

    /* If jumplist is full: remove oldest entry */
    if (++curwin->w_jumplistlen > JUMPLISTSIZE)
    {
	curwin->w_jumplistlen = JUMPLISTSIZE;
	for (i = 1; i < JUMPLISTSIZE; ++i)
	    curwin->w_jumplist[i - 1] = curwin->w_jumplist[i];
    }
    curwin->w_jumplistidx = curwin->w_jumplistlen - 1;

#ifdef ARCHIE
    /* Workaround for a bug in gcc 2.4.5 R2 on the Archimedes
     * Should be fixed in 2.5.x.
     */
    curwin->w_jumplist[curwin->w_jumplistidx].mark.ptr = curwin->w_pcmark.ptr;
    curwin->w_jumplist[curwin->w_jumplistidx].mark.col = curwin->w_pcmark.col;
#else
    curwin->w_jumplist[curwin->w_jumplistidx].mark = curwin->w_pcmark;
#endif
    curwin->w_jumplist[curwin->w_jumplistidx].fnum = curbuf->b_fnum;
    ++curwin->w_jumplistidx;
}

/*
 * checkpcmark() - To change context, call setpcmark(), then move the current
 *		   position to where ever, then call checkpcmark().  This
 *		   ensures that the previous context will only be changed if
 *		   the cursor moved to a different line. -- webb.
 *		   If pcmark was deleted (with "dG") the previous mark is
 *		   restored.
 */
    void
checkpcmark()
{
    if (curwin->w_prev_pcmark.lnum != 0
	    && (equal(curwin->w_pcmark, curwin->w_cursor)
		|| curwin->w_pcmark.lnum == 0))
    {
	curwin->w_pcmark = curwin->w_prev_pcmark;
	curwin->w_prev_pcmark.lnum = 0;		/* Show it has been checked */
    }
}

/*
 * move "count" positions in the jump list (count may be negative)
 */
    FPOS *
movemark(count)
    int count;
{
    FPOS	*pos;
    struct filemark *jmp;

    cleanup_jumplist();

    if (curwin->w_jumplistlen == 0)	    /* nothing to jump to */
	return (FPOS *)NULL;

    for (;;)
    {
	if (curwin->w_jumplistidx + count < 0
		|| curwin->w_jumplistidx + count >= curwin->w_jumplistlen)
	    return (FPOS *)NULL;

	/*
	 * if first CTRL-O or CTRL-I command after a jump, add cursor position
	 * to list.  Careful: If there are duplicates (CTRL-O immidiately after
	 * starting Vim on a file), another entry may have been removed.
	 */
	if (curwin->w_jumplistidx == curwin->w_jumplistlen)
	{
	    setpcmark();
	    --curwin->w_jumplistidx;	/* skip the new entry */
	    if (curwin->w_jumplistidx + count < 0)
		return (FPOS *)NULL;
	}

	curwin->w_jumplistidx += count;

	jmp = curwin->w_jumplist + curwin->w_jumplistidx;
	if (jmp->fnum != curbuf->b_fnum)
	{					     /* jump to other file */
	    if (buflist_findnr(jmp->fnum) == NULL)
	    {					     /* Skip this one .. */
		count += count < 0 ? -1 : 1;
		continue;
	    }
	    if (buflist_getfile(jmp->fnum, jmp->mark.lnum, 0, FALSE) == FAIL)
		return (FPOS *)NULL;
	    /* Set lnum again, autocommands my have changed it */
	    curwin->w_cursor.lnum = jmp->mark.lnum;
	    curwin->w_cursor.col = jmp->mark.col;
	    pos = (FPOS *)-1;
	}
	else
	    pos = &(jmp->mark);
	return pos;
    }
}

/*
 * getmark(c) - find mark for char 'c'
 *
 * Return pointer to FPOS if found (caller needs to check lnum!)
 *	  NULL if there is no mark called 'c'.
 *	  -1 if mark is in other file (only if changefile is TRUE)
 */
    FPOS *
getmark(c, changefile)
    int		c;
    int		changefile;		/* allowed to edit another file */
{
    FPOS	    *posp;
    FPOS	    *startp, *endp;
    static  FPOS    pos_copy;
    char_u	    *p;

    posp = NULL;
    if (c > '~')			/* check for islower()/isupper() */
	;
    else if (c == '\'' || c == '`')	/* previous context mark */
    {
	pos_copy = curwin->w_pcmark;	/* need to make a copy because */
	posp = &pos_copy;		/*   w_pcmark may be changed soon */
    }
    else if (c == '"')			/* to pos when leaving buffer */
	posp = &(curbuf->b_last_cursor);
    else if (c == '[')			/* to start of previous operator */
	posp = &(curbuf->b_op_start);
    else if (c == ']')			/* to end of previous operator */
	posp = &(curbuf->b_op_end);
    else if (c == '<' || c == '>')	/* start/end of visual area */
    {
	startp = &curbuf->b_visual_start;
	endp = &curbuf->b_visual_end;
	if ((c == '<') == lt(*startp, *endp))
	    posp = startp;
	else
	    posp = endp;
	/*
	 * For Visual line mode, set mark at begin or end of line
	 */
	if (curbuf->b_visual_mode == 'V')
	{
	    pos_copy = *posp;
	    posp = &pos_copy;
	    if (c == '<')
		pos_copy.col = 0;
	    else
		pos_copy.col = MAXCOL;
	}
    }
    else if (islower(c))		/* normal named mark */
	posp = &(curbuf->b_namedm[c - 'a']);
    else if (isupper(c) || vim_isdigit(c))	/* named file mark */
    {
	if (vim_isdigit(c))
	    c = c - '0' + NMARKS;
	else
	    c -= 'A';
	posp = &(namedfm[c].mark);

	if (namedfm[c].fnum == 0 && namedfm_names[c] != NULL)
	{
	    /*
	     * First expand "~/" in the file name to the home directory.
	     * Try to shorten the file name.
	     */
	    expand_env(namedfm_names[c], NameBuff, MAXPATHL);
	    mch_dirname(IObuff, IOSIZE);
	    p = shorten_fname(NameBuff, IObuff);

	    /* buflist_new will call fmarks_check_names() */
	    (void)buflist_new(NameBuff, p, (linenr_t)1, FALSE);
	}

	if (namedfm[c].fnum != curbuf->b_fnum)	    /* mark is in other file */
	{
	    if (namedfm[c].mark.lnum != 0 && changefile && namedfm[c].fnum)
	    {
		if (buflist_getfile(namedfm[c].fnum,
			     namedfm[c].mark.lnum, GETF_SETMARK, FALSE) == OK)
		{
		    /* Set the lnum again, autocommands may have changed it */
		    curwin->w_cursor.lnum = namedfm[c].mark.lnum;
		    curwin->w_cursor.col = namedfm[c].mark.col;
		    return (FPOS *)-1;
		}
	    }
	    posp = &pos_copy;		/* mark exists, but is not valid in
					    current buffer */
	    pos_copy.lnum = 0;
	}
    }
    return posp;
}

/*
 * Check all file marks for a name that matches the file name in buf.
 * May replace the name with an fnum.
 */
    void
fmarks_check_names(buf)
    BUF	    *buf;
{
    char_u	*name;
    int		i;

    if (buf->b_ffname == NULL)
	return;

    name = home_replace_save(buf, buf->b_ffname);
    if (name == NULL)
	return;

    for (i = 0; i < NMARKS + EXTRA_MARKS; ++i)
    {
	if (namedfm[i].fnum == 0 && namedfm_names[i] != NULL
				     && fnamecmp(name, namedfm_names[i]) == 0)
	{
	    namedfm[i].fnum = buf->b_fnum;
	    vim_free(namedfm_names[i]);
	    namedfm_names[i] = NULL;
	}
    }
    vim_free(name);
}

/*
 * Check a if a position from a mark is valid.
 * Give and error message and return FAIL if not.
 */
    int
check_mark(pos)
    FPOS    *pos;
{
    if (pos == NULL)
    {
	emsg(e_umark);
	return FAIL;
    }
    if (pos->lnum == 0)
    {
	emsg(e_marknotset);
	return FAIL;
    }
    if (pos->lnum > curbuf->b_ml.ml_line_count)
    {
	emsg(e_markinval);
	return FAIL;
    }
    return OK;
}

/*
 * clrallmarks() - clear all marks in the buffer 'buf'
 *
 * Used mainly when trashing the entire buffer during ":e" type commands
 */
    void
clrallmarks(buf)
    BUF	    *buf;
{
    static int		i = -1;

    if (i == -1)	/* first call ever: initialize */
	for (i = 0; i < NMARKS + 1; i++)
	{
	    namedfm[i].mark.lnum = 0;
	    namedfm_names[i] = NULL;
	}

    for (i = 0; i < NMARKS; i++)
	buf->b_namedm[i].lnum = 0;
    buf->b_op_start.lnum = 0;		/* start/end op mark cleared */
    buf->b_op_end.lnum = 0;
    buf->b_last_cursor.lnum = 1;	/* '" mark cleared */
    buf->b_last_cursor.col = 0;
}

/*
 * Get name of file from a filemark.
 * When it's in the current buffer, return the text at the mark.
 * Returns an allocated string.
 */
    char_u *
fm_getname(fmark, lead_len)
    struct filemark *fmark;
    int		lead_len;
{
    if (fmark->fnum == curbuf->b_fnum)		    /* current buffer */
	return mark_line(&(fmark->mark), lead_len);
    return buflist_nr2name(fmark->fnum, FALSE, TRUE);
}

/*
 * Return the line at mark "mp".  Truncate to fit in window.
 * The returned string has been allocated.
 */
    static char_u *
mark_line(mp, lead_len)
    FPOS	*mp;
    int		lead_len;
{
    char_u	*s, *p;
    int		len;

    if (mp->lnum == 0 || mp->lnum > curbuf->b_ml.ml_line_count)
	return vim_strsave((char_u *)"-invalid-");
    s = vim_strnsave(skipwhite(ml_get(mp->lnum)), (int)Columns);
    if (s == NULL)
	return NULL;
    /* Truncate the line to fit it in the window */
    len = 0;
    for (p = s; *p != NUL; ++p)
    {
	len += charsize(*p);
	if (len >= Columns - lead_len)
	    break;
    }
    *p = NUL;
    return s;
}

/*
 * print the marks
 */
    void
do_marks(arg)
    char_u	*arg;
{
    int		i;
    char_u	*name;

    if (arg != NULL && *arg == NUL)
	arg = NULL;

    show_one_mark('\'', arg, &curwin->w_pcmark, NULL, TRUE);
    for (i = 0; i < NMARKS; ++i)
	show_one_mark(i + 'a', arg, &curbuf->b_namedm[i], NULL, TRUE);
    for (i = 0; i < NMARKS + EXTRA_MARKS; ++i)
    {
	name = namedfm[i].fnum ? fm_getname(&namedfm[i], 15) : namedfm_names[i];
	if (name != NULL)
	{
	    show_one_mark(i >= NMARKS ? i - NMARKS + '0' : i + 'A',
		    arg, &namedfm[i].mark, name,
		    namedfm[i].fnum == curbuf->b_fnum);
	    if (namedfm[i].fnum)
		vim_free(name);
	}
    }
    show_one_mark('"', arg, &curbuf->b_last_cursor, NULL, TRUE);
    show_one_mark('[', arg, &curbuf->b_op_start, NULL, TRUE);
    show_one_mark(']', arg, &curbuf->b_op_end, NULL, TRUE);
    show_one_mark('<', arg, &curbuf->b_visual_start, NULL, TRUE);
    show_one_mark('>', arg, &curbuf->b_visual_end, NULL, TRUE);
    show_one_mark(-1, arg, NULL, NULL, FALSE);
}

    static void
show_one_mark(c, arg, p, name, current)
    int		c;
    char_u	*arg;
    FPOS	*p;
    char_u	*name;
    int		current;	/* in current file */
{
    static int	did_title = FALSE;
    int		mustfree = FALSE;

    if (c == -1)			    /* finish up */
    {
	if (did_title)
	    did_title = FALSE;
	else
	{
	    if (arg == NULL)
		MSG("No marks set");
	    else
		EMSG2("No marks matching \"%s\"", arg);
	}
    }
    /* don't output anything if 'q' typed at --more-- prompt */
    else if (!got_int
	    && (arg == NULL || vim_strchr(arg, c) != NULL)
	    && p->lnum != 0)
    {
	if (!did_title)
	{
	    /* Highlight title */
	    MSG_PUTS_TITLE("\nmark line  col file/text");
	    did_title = TRUE;
	}
	msg_putchar('\n');
	if (!got_int)
	{
	    sprintf((char *)IObuff, " %c %6ld %4d ", c, p->lnum, p->col);
	    msg_outtrans(IObuff);
	    if (name == NULL && current)
	    {
		name = mark_line(p, 15);
		mustfree = TRUE;
	    }
	    if (name != NULL)
	    {
		msg_outtrans_attr(name, current ? hl_attr(HLF_D) : 0);
		if (mustfree)
		    vim_free(name);
	    }
	}
	out_flush();		    /* show one line at a time */
    }
}

/*
 * print the jumplist
 */
    void
do_jumps()
{
    int		i;
    char_u	*name;

    cleanup_jumplist();
    /* Highlight title */
    MSG_PUTS_TITLE("\n jump line  col file/text");
    for (i = 0; i < curwin->w_jumplistlen; ++i)
    {
	if (curwin->w_jumplist[i].mark.lnum != 0)
	{
	    name = fm_getname(&curwin->w_jumplist[i], 16);
	    if (name == NULL)	    /* file name not available */
		continue;

	    msg_putchar('\n');
	    sprintf((char *)IObuff, "%c %2d %5ld %4d ",
		i == curwin->w_jumplistidx ? '>' : ' ',
		i > curwin->w_jumplistidx ? i - curwin->w_jumplistidx
					  : curwin->w_jumplistidx - i,
		curwin->w_jumplist[i].mark.lnum,
		curwin->w_jumplist[i].mark.col);
	    msg_outtrans(IObuff);
	    msg_outtrans_attr(name,
				 curwin->w_jumplist[i].fnum == curbuf->b_fnum
							? hl_attr(HLF_D) : 0);
	    vim_free(name);
	}
	out_flush();
    }
    if (curwin->w_jumplistidx == curwin->w_jumplistlen)
	MSG_PUTS("\n>");
}

/*
 * adjust marks between line1 and line2 (inclusive) to move 'amount' lines
 * If 'amount' is MAXLNUM the mark is made invalid.
 * If 'amount_after' is non-zero adjust marks after line 2.
 */

#define one_adjust(add) \
    { \
	lp = add; \
	if (*lp >= line1 && *lp <= line2) \
	{ \
	    if (amount == MAXLNUM) \
		*lp = 0; \
	    else \
		*lp += amount; \
	} \
	else if (amount_after && *lp > line2) \
	    *lp += amount_after; \
    }

/* don't delete the line, just put at first deleted line */
#define one_adjust_nodel(add) \
    { \
	lp = add; \
	if (*lp >= line1 && *lp <= line2) \
	{ \
	    if (amount == MAXLNUM) \
		*lp = line1; \
	    else \
		*lp += amount; \
	} \
	else if (amount_after && *lp > line2) \
	    *lp += amount_after; \
    }

    void
mark_adjust(line1, line2, amount, amount_after)
    linenr_t	line1;
    linenr_t	line2;
    long	amount;
    long	amount_after;
{
    int		i;
    int		fnum = curbuf->b_fnum;
    linenr_t	*lp;
    WIN		*win;

    if (line2 < line1 && amount_after == 0L)	    /* nothing to do */
	return;

/* named marks, lower case and upper case */
    for (i = 0; i < NMARKS; i++)
    {
	one_adjust(&(curbuf->b_namedm[i].lnum));
	if (namedfm[i].fnum == fnum)
	    one_adjust(&(namedfm[i].mark.lnum));
    }
    for (i = NMARKS; i < NMARKS + EXTRA_MARKS; i++)
    {
	if (namedfm[i].fnum == fnum)
	    one_adjust(&(namedfm[i].mark.lnum));
    }

/* previous context mark */
    one_adjust(&(curwin->w_pcmark.lnum));

/* previous pcmark */
    one_adjust(&(curwin->w_prev_pcmark.lnum));

/* Visual area */
    one_adjust_nodel(&(curbuf->b_visual_start.lnum));
    one_adjust_nodel(&(curbuf->b_visual_end.lnum));

/* marks in the tag stack */
    for (win = firstwin; win != NULL; win = win->w_next)
	if (win->w_buffer == curbuf)
	    for (i = 0; i < win->w_tagstacklen; i++)
		if (win->w_tagstack[i].fmark.fnum == fnum)
		    one_adjust_nodel(&(win->w_tagstack[i].fmark.mark.lnum));

#ifdef QUICKFIX
/* quickfix marks */
    qf_mark_adjust(line1, line2, amount, amount_after);
#endif

/* jumplist marks */
    for (win = firstwin; win != NULL; win = win->w_next)
    {
	/*
	 * When deleting lines, this may create duplicate marks in the
	 * jumplist. They will be removed later.
	 */
	for (i = 0; i < win->w_jumplistlen; ++i)
	    if (win->w_jumplist[i].fnum == fnum)
		one_adjust_nodel(&(win->w_jumplist[i].mark.lnum));
	/*
	 * also adjust the line at the top of the window and the cursor
	 * position for windows with the same buffer.
	 */
	if (win != curwin && win->w_buffer == curbuf)
	{
	    if (win->w_topline >= line1 && win->w_topline <= line2)
	    {
		if (amount == MAXLNUM)	    /* topline is deleted */
		{
		    if (line1 <= 1)
			win->w_topline = 1;
		    else
			win->w_topline = line1 - 1;
		}
		else			/* keep topline on the same line */
		    win->w_topline += amount;
	    }
	    else if (amount_after && win->w_topline > line2)
		win->w_topline += amount_after;
	    if (win->w_cursor.lnum >= line1 && win->w_cursor.lnum <= line2)
	    {
		if (amount == MAXLNUM)	    /* line with cursor is deleted */
		{
		    if (line1 <= 1)
			win->w_cursor.lnum = 1;
		    else
			win->w_cursor.lnum = line1 - 1;
		    win->w_cursor.col = 0;
		}
		else			/* keep cursor on the same line */
		    win->w_cursor.lnum += amount;
	    }
	    else if (amount_after && win->w_cursor.lnum > line2)
		win->w_cursor.lnum += amount_after;
	}
    }
}

/*
 * When deleting lines, this may create duplicate marks in the
 * jumplist. They will be removed here for the current window.
 */
    static void
cleanup_jumplist()
{
    int	    i;
    int	    from, to;

    to = 0;
    for (from = 0; from < curwin->w_jumplistlen; ++from)
    {
	if (curwin->w_jumplistidx == from)
	    curwin->w_jumplistidx = to;
	for (i = from + 1; i < curwin->w_jumplistlen; ++i)
	    if (curwin->w_jumplist[i].fnum == curwin->w_jumplist[from].fnum &&
		curwin->w_jumplist[i].mark.lnum ==
					   curwin->w_jumplist[from].mark.lnum)
		break;
	if (i >= curwin->w_jumplistlen)	    /* no duplicate */
	    curwin->w_jumplist[to++] = curwin->w_jumplist[from];
    }
    if (curwin->w_jumplistidx == curwin->w_jumplistlen)
	curwin->w_jumplistidx = to;
    curwin->w_jumplistlen = to;
}

    void
set_last_cursor(win)
    WIN	    *win;
{
    win->w_buffer->b_last_cursor = win->w_cursor;
}

#ifdef VIMINFO
    int
read_viminfo_filemark(line, fp, force)
    char_u  *line;
    FILE    *fp;
    int	    force;
{
    int	    idx;
    char_u  *str;

    /* We only get here (hopefully) if line[0] == '\'' */
    str = line + 1;
    if (*str > 127 || (!isdigit(*str) && !isupper(*str)))
    {
	if (viminfo_error("Illegal file mark name", line))
	    return TRUE;	/* Too many errors, pretend end-of-file */
    }
    else
    {
	if (isdigit(*str))
	    idx = *str - '0' + NMARKS;
	else
	    idx = *str - 'A';
	if (namedfm[idx].mark.lnum == 0 || force)
	{
	    str = skipwhite(str + 1);
	    namedfm[idx].mark.lnum = getdigits(&str);
	    str = skipwhite(str);
	    namedfm[idx].mark.col = getdigits(&str);
	    str = skipwhite(str);
	    namedfm_names[idx] = viminfo_readstring(str, fp);
	}
    }
    return vim_fgets(line, LSIZE, fp);
}

    void
write_viminfo_filemarks(fp)
    FILE    *fp;
{
    int	    i;
    char_u  *name;

    if (get_viminfo_parameter('\'') == 0)
	return;

    fprintf(fp, "\n# File marks:\n");

    /*
     * Find a mark that is the same file and position as the cursor.
     * That one, or else the last one is deleted.
     * Move '0 to '1, '1 to '2, etc. until the matching one or '9
     * Set '0 mark to current cursor position.
     */
    if (curbuf->b_ffname != NULL && !removable(curbuf->b_ffname))
    {
	name = buflist_nr2name(curbuf->b_fnum, TRUE, FALSE);
	for (i = NMARKS; i < NMARKS + EXTRA_MARKS - 1; ++i)
	    if (namedfm[i].mark.lnum == curwin->w_cursor.lnum
		    && (namedfm_names[i] == NULL
			    ? namedfm[i].fnum == curbuf->b_fnum
			    : (name != NULL
				    && STRCMP(name, namedfm_names[i]) == 0)))
		break;
	vim_free(name);

	vim_free(namedfm_names[i]);
	for ( ; i > NMARKS; --i)
	{
	    namedfm[i] = namedfm[i - 1];
	    namedfm_names[i] = namedfm_names[i - 1];
	}
	namedfm[NMARKS].mark = curwin->w_cursor;
	namedfm[NMARKS].fnum = curbuf->b_fnum;
	namedfm_names[NMARKS] = NULL;
    }

    for (i = 0; i < NMARKS + EXTRA_MARKS; i++)
    {
	if (namedfm[i].mark.lnum == 0)		/* not set */
	    continue;

	if (namedfm[i].fnum)			/* there is a buffer */
	    name = buflist_nr2name(namedfm[i].fnum, TRUE, FALSE);
	else
	    name = namedfm_names[i];		/* use name from .viminfo */
	if (name == NULL)
	    continue;

	fprintf(fp, "'%c  %ld  %ld  ",
		    i < NMARKS ? i + 'A' : i - NMARKS + '0',
		    (long)namedfm[i].mark.lnum,
		    (long)namedfm[i].mark.col);
	viminfo_writestring(fp, name);
	if (namedfm[i].fnum)
	    vim_free(name);
    }
}

/*
 * Return TRUE if "name" is on removable media (depending on 'viminfo').
 */
    int
removable(name)
    char_u  *name;
{
    char_u  *p;
    char_u  part[51];
    int	    retval = FALSE;

    name = home_replace_save(NULL, name);
    if (name != NULL)
    {
	for (p = p_viminfo; *p; )
	{
	    copy_option_part(&p, part, 51, ", ");
	    if (part[0] == 'r'
			   && STRNICMP(part + 1, name, STRLEN(part + 1)) == 0)
	    {
		retval = TRUE;
		break;
	    }
	}
	vim_free(name);
    }
    return retval;
}

/*
 * Write all the named marks for all buffers.
 * Return the number of buffers for which marks have been written.
 */
    int
write_viminfo_marks(fp_out)
    FILE    *fp_out;
{
    int	    count;
    BUF	    *buf;
    WIN	    *win;
    int	    is_mark_set;
    int	    i;

    /*
     * Set b_last_cursor for the all buffers that have a window.
     */
    for (win = firstwin; win != NULL; win = win->w_next)
	set_last_cursor(win);

    fprintf(fp_out, "\n# History of marks within files (newest to oldest):\n");
    count = 0;
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
    {
	/*
	 * Only write something if buffer has been loaded and at least one
	 * mark is set.
	 */
	if (buf->b_marks_read)
	{
	    if (buf->b_last_cursor.lnum != 0)
		is_mark_set = TRUE;
	    else
	    {
		is_mark_set = FALSE;
		for (i = 0; i < NMARKS; i++)
		    if (buf->b_namedm[i].lnum != 0)
		    {
			is_mark_set = TRUE;
			break;
		    }
	    }
	    if (is_mark_set && buf->b_ffname != NULL &&
		 buf->b_ffname[0] != NUL && !removable(buf->b_ffname))
	    {
		home_replace(NULL, buf->b_ffname, IObuff, IOSIZE, TRUE);
		fprintf(fp_out, "\n> ");
		viminfo_writestring(fp_out, IObuff);
		if (buf->b_last_cursor.lnum != 0)
		    fprintf(fp_out, "\t\"\t%ld\t%d\n",
			    buf->b_last_cursor.lnum, buf->b_last_cursor.col);
		for (i = 0; i < NMARKS; i++)
		    if (buf->b_namedm[i].lnum != 0)
			fprintf(fp_out, "\t%c\t%ld\t%d\n", 'a' + i,
				buf->b_namedm[i].lnum, buf->b_namedm[i].col);
		count++;
	    }
	}
    }

    return count;
}

/*
 * Handle marks in the viminfo file:
 * fp_out == NULL   read marks for current buffer only
 * fp_out != NULL   copy marks for buffers not in buffer list
 */
    void
copy_viminfo_marks(line, fp_in, fp_out, count, eof)
    char_u	*line;
    FILE	*fp_in;
    FILE	*fp_out;
    int		count;
    int		eof;
{
    BUF		*buf;
    int		num_marked_files;
    int		load_marks;
    int		copy_marks_out;
    char_u	*str;
    int		i;
    char_u	*p;
    char_u	*name_buf;
    long	lnum;
    int		col;

    if ((name_buf = alloc(LSIZE)) == NULL)
	return;
    num_marked_files = get_viminfo_parameter('\'');
    while (!eof && (count < num_marked_files || fp_out == NULL))
    {
	if (line[0] != '>')
	{
	    if (line[0] != '\n' && line[0] != '\r' && line[0] != '#')
	    {
		if (viminfo_error("Missing '>'", line))
		    break;	/* too many errors, return now */
	    }
	    eof = vim_fgets(line, LSIZE, fp_in);
	    continue;		/* Skip this dud line */
	}

	/*
	 * Handle long line and translate escaped characters.
	 * Find file name, set str to start.
	 * Ignore leading and trailing white space.
	 */
	str = skipwhite(line + 1);
	str = viminfo_readstring(str, fp_in);
	if (str == NULL)
	    continue;
	p = str + STRLEN(str);
	while (p != str && (*p == NUL || vim_isspace(*p)))
	    p--;
	if (*p)
	    p++;
	*p = NUL;

	/*
	 * If fp_out == NULL, load marks for current buffer.
	 * If fp_out != NULL, copy marks for buffers not in buflist.
	 */
	load_marks = copy_marks_out = FALSE;
	if (fp_out == NULL)
	{
	    if (curbuf->b_ffname != NULL)
	    {
		home_replace(NULL, curbuf->b_ffname, name_buf, LSIZE, TRUE);
		if (fnamecmp(str, name_buf) == 0)
		    load_marks = TRUE;
	    }
	}
	else /* fp_out != NULL */
	{
	    /* This is slow if there are many buffers!! */
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
		if (buf->b_ffname != NULL)
		{
		    home_replace(NULL, buf->b_ffname, name_buf, LSIZE, TRUE);
		    if (fnamecmp(str, name_buf) == 0)
			break;
		}

	    /*
	     * copy marks if the buffer has not been loaded
	     */
	    if (buf == NULL || !buf->b_marks_read)
	    {
		copy_marks_out = TRUE;
		fputs("\n> ", fp_out);
		viminfo_writestring(fp_out, str);
		count++;
	    }
	}
	vim_free(str);

	while (!(eof = vim_fgets(line, LSIZE, fp_in)) && line[0] == TAB)
	{
	    if (load_marks)
	    {
		if (line[1] != NUL)
		    sscanf((char *)line + 2, "%ld %d", &lnum, &col);
		if (line[1] == '"')
		{
		    curbuf->b_last_cursor.lnum = lnum;
		    curbuf->b_last_cursor.col = col;
		}
		else if ((i = line[1] - 'a') >= 0 && i < NMARKS)
		{
		    curbuf->b_namedm[i].lnum = lnum;
		    curbuf->b_namedm[i].col = col;
		}
	    }
	    else if (copy_marks_out)
		fputs((char *)line, fp_out);
	}
	if (load_marks)
	    break;
    }
    vim_free(name_buf);
}
#endif /* VIMINFO */
