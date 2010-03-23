/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * message.c: functions for displaying messages on the command line
 */

#define MESSAGE_FILE		/* don't include prototype for smsg() */

#include "vim.h"

#ifdef __QNX__
# include <stdarg.h>
#endif

static void reset_last_sourcing __ARGS((void));
static void add_msg_hist __ARGS((char_u *s, int len, int attr));
static void hit_return_msg __ARGS((void));
static void msg_home_replace_attr __ARGS((char_u *fname, int attr));
static int  msg_use_printf __ARGS((void));
static void msg_screen_putchar __ARGS((int c, int attr));
static int  msg_check_screen __ARGS((void));
static void redir_write __ARGS((char_u *s));
#ifdef CON_DIALOG
static char_u *msg_show_console_dialog __ARGS((char_u *message, char_u *buttons, int dfltbutton));
static int msg_noquit_more = FALSE; /* quit not allowed at more prompt */
#endif

struct msg_hist
{
    struct msg_hist	*next;
    char_u		*msg;
    int			attr;
};

static struct msg_hist *first_msg_hist = NULL;
static struct msg_hist *last_msg_hist = NULL;
static int msg_hist_len = 0;
static int msg_hist_off = FALSE;	/* don't add messages to history */

/*
 * When writing messages to the screen, there are many different situations.
 * A number of variables is used to remember the current state:
 * msg_didany	    TRUE when messages were written since the last time the
 *		    user reacted to a prompt.
 *		    Reset: After hitting a key for the hit-return prompt,
 *		    hitting <CR> for the command line or input().
 *		    Set: When any message is written to the screen.
 * msg_didout	    TRUE when something was written to the current line.
 *		    Reset: When advancing to the next line, when the current
 *		    text can be overwritten.
 *		    Set: When any message is written to the screen.
 * msg_nowait	    No extra delay for the last drawn message.
 *		    Used in normal_cmd() before the mode message is drawn.
 * emsg_on_display  There was an error message recently.  Indicates that there
 *		    should be a delay before redrawing.
 * msg_scroll	    The next message should not overwrite the current one.
 * msg_scrolled	    How many lines the screen has been scrolled (because of
 *		    messages).  Used in update_screen() to scroll the screen
 *		    back.  Incremented each time the screen scrolls a line.
 * lines_left	    Number of lines available for messages before the
 *		    more-prompt is to be given.
 * need_wait_return TRUE when the hit-return prompt is needed.
 *		    Reset: After giving the hit-return prompt, when the user
 *		    has answered some other prompt.
 *		    Set: When the ruler or typeahead display is overwritten,
 *		    scrolling the screen for some message.
 * keep_msg	    Message to be displayed after redrawing the screen.
 */

/*
 * msg(s) - displays the string 's' on the status line
 * When terminal not initialized (yet) mch_errmsg(..) is used.
 * return TRUE if wait_return not called
 */
    int
msg(s)
    char_u	    *s;
{
#ifdef WANT_EVAL
    set_vim_var_string(VV_STATUSMSG, s);
#endif
    return msg_attr(s, 0);
}

    int
msg_attr(s, attr)
    char_u	*s;
    int		attr;
{
    static int	entered = 0;
    int		retval;
    char_u	*buf = NULL;

    /*
     * It is possible that displaying a messages causes a problem (e.g.,
     * when redrawing the window), which causes another message, etc..	To
     * break this loop, limit the recursiveness to 3 levels.
     */
    if (entered >= 3)
	return TRUE;
    ++entered;

    /* Add message to history (unless it's a repeated kept message or a
     * truncated message) */
    if (s != keep_msg
	    || (*s != '<'
		&& last_msg_hist != NULL
		&& last_msg_hist->msg != NULL
		&& STRCMP(s, last_msg_hist->msg)))
	add_msg_hist(s, -1, attr);

    /* Truncate the message if needed. */
    buf = msg_strtrunc(s);
    if (buf != NULL)
	s = buf;

    msg_start();
    msg_outtrans_attr(s, attr);
    msg_clr_eos();
    retval = msg_end();

    vim_free(buf);
    --entered;
    return retval;
}

/*
 * Truncate a string such that it can be printed without causing a scroll.
 */
    char_u *
msg_strtrunc(s)
    char_u	*s;
{
    char_u	*buf = NULL;
    int		len;
    int		room;
    int		half;
    int		i;

    /* May truncate message to avoid a hit-return prompt */
    if (!msg_scroll && !need_wait_return && shortmess(SHM_TRUNCALL)
							    && !exmode_active)
    {
	len = vim_strsize(s);
	room = (int)(Rows - cmdline_row - 1) * Columns + sc_col - 1;
	if (len > room)
	{
	    buf = alloc(room + 1);
	    if (buf != NULL)
	    {
		room -= 3;
		half = room / 2;
		len = 0;
		for (i = 0; len < half; ++i)
		{
		    len += charsize(s[i]);
		    buf[i] = s[i];
		}
		--i;
		len -= charsize(s[i]);
		STRCPY(buf + i, "...");
		for (i = STRLEN(s) - 1; len < room; --i)
		    len += charsize(s[i]);
		if (len > room)
		    ++i;
		STRCAT(buf, s + i + 1);
	    }
	}
    }
    return buf;
}

/*
 * automatic prototype generation does not understand this function
 */
#ifndef PROTO
# ifndef __QNX__

int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg __ARGS((char_u *, long, long, long,
			long, long, long, long, long, long, long));
int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg_attr __ARGS((int, char_u *, long, long, long,
			long, long, long, long, long, long, long));

/* VARARGS */
    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
    char_u	*s;
    long	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
    int ret = smsg_attr(0, s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
#ifdef WANT_EVAL
    set_vim_var_string(VV_STATUSMSG, IObuff);
#endif
    return ret;
}

/* VARARGS */
    int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
smsg_attr(attr, s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
    int		attr;
    char_u	*s;
    long	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
    sprintf((char *)IObuff, (char *)s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    return msg_attr(IObuff, attr);
}

# else /* __QNX__ */

    int
smsg(char_u *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vsprintf((char *)IObuff, (char *)s, arglist);
    va_end(arglist);
    return msg(IObuff);
}

    int
smsg_attr(int attr, char_u *s, ...)
{
    va_list arglist;

    va_start(arglist, s);
    vsprintf((char *)IObuff, (char *)s, arglist);
    va_end(arglist);
    return msg_attr(IObuff, attr);
}

# endif /* __QNX__ */
#endif

/*
 * Remember the last sourcing name/lnum used in an error message, so that it
 * isn't printed each time when it didn't change.
 */
static int	last_sourcing_lnum = 0;
static char_u   *last_sourcing_name = NULL;

/*
 * Reset the last used sourcing name/lnum.  Makes sure it is displayed again
 * for the next error message;
 */
    static void
reset_last_sourcing()
{
    vim_free(last_sourcing_name);
    last_sourcing_name = NULL;
    last_sourcing_lnum = 0;
}

/*
 * emsg() - display an error message
 *
 * Rings the bell, if appropriate, and calls message() to do the real work
 * When terminal not initialized (yet) mch_errmsg(..) is used.
 *
 * return TRUE if wait_return not called
 */
    int
emsg(s)
    char_u	   *s;
{
    char_u	    *Buf;
    int		    attr;
    int		    other_sourcing_name;

    if (emsg_off)		/* no error messages at the moment */
	return TRUE;

    if (global_busy)		/* break :global command */
	++global_busy;

    if (p_eb)
	beep_flush();		/* also includes flush_buffers() */
    else
	flush_buffers(FALSE);	/* flush internal buffers */
    did_emsg = TRUE;		/* flag for DoOneCmd() */

#ifdef WANT_EVAL
    set_vim_var_string(VV_ERRMSG, s);
#endif

#ifdef VIMBUDDY
    if (sourcing_name == NULL)
    {
	VimBuddyText(s, 2);
	return TRUE;
    }
#endif
    emsg_on_display = TRUE;	/* remember there is an error message */
    ++msg_scroll;		/* don't overwrite a previous message */
    attr = hl_attr(HLF_E);	/* set highlight mode for error messages */
    if (msg_scrolled)
	need_wait_return = TRUE;    /* needed in case emsg() is called after
				     * wait_return has reset need_wait_return
				     * and a redraw is expected because
				     * msg_scrolled is non-zero */

/*
 * First output name and line number of source of error message
 */
    if (sourcing_name != NULL)
    {
	if (last_sourcing_name != NULL)
	    other_sourcing_name = STRCMP(sourcing_name, last_sourcing_name);
	else
	    other_sourcing_name = TRUE;
    }
    else
	other_sourcing_name = FALSE;

    if (sourcing_name != NULL
	    && (other_sourcing_name || sourcing_lnum != last_sourcing_lnum)
	    && (Buf = alloc((unsigned)STRLEN(sourcing_name) + 35)) != NULL)
    {
	++no_wait_return;
	if (other_sourcing_name)
	{
	    sprintf((char *)Buf, "Error detected while processing %s:",
					    sourcing_name);
	    msg_attr(Buf, attr);
	}
	    /* lnum is 0 when executing a command from the command line
	     * argument, we don't want a line number then */
	if (sourcing_lnum != 0)
	{
	    sprintf((char *)Buf, "line %4ld:", (long)sourcing_lnum);
	    msg_attr(Buf, hl_attr(HLF_N));
	}
	--no_wait_return;
	last_sourcing_lnum = sourcing_lnum;  /* only once for each line */
	vim_free(Buf);
    }

    /* remember the last sourcing name printed, also when it's empty */
    if (sourcing_name == NULL || other_sourcing_name)
    {
	vim_free(last_sourcing_name);
	if (sourcing_name == NULL)
	    last_sourcing_name = NULL;
	else
	    last_sourcing_name = vim_strsave(sourcing_name);
    }
    msg_nowait = FALSE;			/* wait for this msg */

    return msg_attr(s, attr);
}

    int
emsg2(s, a1)
    char_u *s, *a1;
{
    if (emsg_off)		/* no error messages at the moment */
	return TRUE;

    /* Check for NULL strings (just in case) */
    if (a1 == NULL)
	a1 = (char_u *)"[NULL]";
    /* Check for very long strings (can happen with ":help ^A<CR>") */
    if (STRLEN(s) + STRLEN(a1) >= (size_t)IOSIZE)
	a1 = (char_u *)"[string too long]";
    sprintf((char *)IObuff, (char *)s, (char *)a1);
    return emsg(IObuff);
}

    int
emsgn(s, n)
    char_u *s;
    long    n;
{
    if (emsg_off)		/* no error messages at the moment */
	return TRUE;
    sprintf((char *)IObuff, (char *)s, n);
    return emsg(IObuff);
}

/*
 * Like msg(), but truncate to a single line if p_shm contains 't', or when
 * "force" is TRUE.  This truncates in another way as for normal messages.
 * Careful: The string may be changed by msg_may_trunc()!
 * Returns a pointer to the printed message, if wait_return() not called.
 */
    char_u *
msg_trunc_attr(s, force, attr)
    char_u	*s;
    int		force;
    int		attr;
{
    int		n;

    /* Add message to history before truncating */
    add_msg_hist(s, -1, attr);

    s = msg_may_trunc(force, s);

    msg_hist_off = TRUE;
    n = msg_attr(s, attr);
    msg_hist_off = FALSE;

    if (n)
	return s;
    return NULL;
}

/*
 * Check if message "s" should be truncated at the start (for filenames).
 * Return a pointer to where the truncated message starts.
 * Note: May change the message by replacing a character with '<'.
 */
    char_u *
msg_may_trunc(force, s)
    int		force;
    char_u	*s;
{
    int		n;

    if ((force || (shortmess(SHM_TRUNC) && !exmode_active))
	    && (n = (int)STRLEN(s) -
		    (int)(Rows - cmdline_row - 1) * Columns - sc_col + 1) > 0)
    {
	s += n;
	*s = '<';
    }
    return s;
}

    static void
add_msg_hist(s, len, attr)
    char_u	*s;
    int		len;		/* -1 for undetermined length */
    int		attr;
{
    struct msg_hist *p;

    if (msg_hist_off)
	return;

    /* Don't let the message history get too big */
    while (msg_hist_len > 20)
    {
	p = first_msg_hist;
	first_msg_hist = p->next;
	vim_free(p->msg);
	vim_free(p);
	--msg_hist_len;
    }
    /* allocate an entry and add the message at the end of the history */
    p = (struct msg_hist *)alloc((int)sizeof(struct msg_hist));
    if (p != NULL)
    {
	if (len < 0)
	    len = STRLEN(s);
	/* remove leading and trailing newlines */
	while (len > 0 && *s == '\n')
	{
	    ++s;
	    --len;
	}
	while (len > 0 && s[len - 1] == '\n')
	    --len;
	p->msg = vim_strnsave(s, len);
	p->next = NULL;
	p->attr = attr;
	if (last_msg_hist != NULL)
	    last_msg_hist->next = p;
	last_msg_hist = p;
	if (first_msg_hist == NULL)
	    first_msg_hist = last_msg_hist;
	++msg_hist_len;
    }
}

/*
 * ":messages" command.
 */
    void
ex_messages()
{
    struct msg_hist *p;

    msg_hist_off = TRUE;
    for (p = first_msg_hist; p != NULL; p = p->next)
	if (p->msg != NULL)
	    msg_attr(p->msg, p->attr);
    msg_hist_off = FALSE;
}

#if defined(CON_DIALOG) || defined(PROTO)
static void msg_end_prompt __ARGS((void));

/*
 * Call this after prompting the user.  This will avoid a hit-return message
 * and a delay.
 */
    static void
msg_end_prompt()
{
    need_wait_return = FALSE;
    emsg_on_display = FALSE;
    cmdline_row = msg_row;
    msg_col = 0;
    msg_clr_eos();
}
#endif

/*
 * wait for the user to hit a key (normally a return)
 * if 'redraw' is TRUE, clear and redraw the screen
 * if 'redraw' is FALSE, just redraw the screen
 * if 'redraw' is -1, don't redraw at all
 */
    void
wait_return(redraw)
    int	    redraw;
{
    int		    c;
    int		    oldState;
    int		    tmpState;

    if (redraw == TRUE)
	must_redraw = CLEAR;

/*
 * With the global command (and some others) we only need one return at the
 * end. Adjust cmdline_row to avoid the next message overwriting the last one.
 * When inside vgetc(), we can't wait for a typed character at all.
 */
    if (vgetc_busy)
	return;
    if (no_wait_return)
    {
	need_wait_return = TRUE;
	if (!exmode_active)
	    cmdline_row = msg_row;
	return;
    }

    redir_off = TRUE;		    /* don't redirect this message */
    oldState = State;
    if (quit_more)
    {
	c = CR;			    /* just pretend CR was hit */
	quit_more = FALSE;
	got_int = FALSE;
    }
    else if (exmode_active)
    {
	MSG_PUTS(" ");	  /* make sure the cursor is on the right line */
	c = CR;			    /* no need for a return in ex mode */
	got_int = FALSE;
    }
    else
    {
	State = HITRETURN;
#ifdef USE_MOUSE
	setmouse();
#endif
#ifdef USE_ON_FLY_SCROLL
	dont_scroll = TRUE;		/* disallow scrolling here */
#endif
	hit_return_msg();

#ifdef ORG_HITRETURN
	do
	{
	    c = safe_vgetc();
	} while (vim_strchr((char_u *)"\r\n: ", c) == NULL);
	if (c == ':')			/* this can vi too (but not always!) */
	    stuffcharReadbuff(c);
#else
	do
	{
	    c = safe_vgetc();
	    if (!global_busy)
		got_int = FALSE;
	} while (c == Ctrl('C')
#ifdef USE_GUI
				|| c == K_SCROLLBAR || c == K_HORIZ_SCROLLBAR
#endif
#ifdef USE_MOUSE
				|| c == K_LEFTDRAG   || c == K_LEFTRELEASE
				|| c == K_MIDDLEDRAG || c == K_MIDDLERELEASE
				|| c == K_RIGHTDRAG  || c == K_RIGHTRELEASE
				|| c == K_MOUSEDOWN  || c == K_MOUSEUP
				|| c == K_IGNORE     ||
				(!mouse_has(MOUSE_RETURN) &&
				     (c == K_LEFTMOUSE ||
				      c == K_MIDDLEMOUSE ||
				      c == K_RIGHTMOUSE))
#endif
				);
	ui_breakcheck();
#ifdef USE_MOUSE
	/*
	 * Avoid that the mouse-up event causes visual mode to start.
	 */
	if (c == K_LEFTMOUSE || c == K_MIDDLEMOUSE || c == K_RIGHTMOUSE)
	    jump_to_mouse(MOUSE_SETPOS, NULL);
	else
#endif
	    if (vim_strchr((char_u *)"\r\n ", c) == NULL)
	{
	    stuffcharReadbuff(c);
	    do_redraw = TRUE;	    /* need a redraw even though there is
				       something in the stuff buffer */
	}
#endif
    }
    redir_off = FALSE;

    /*
     * If the user hits ':', '?' or '/' we get a command line from the next
     * line.
     */
    if (c == ':' || c == '?' || c == '/')
    {
	if (!exmode_active)
	    cmdline_row = msg_row;
	skip_redraw = TRUE;	    /* skip redraw once */
	do_redraw = FALSE;
    }

/*
 * If the window size changed set_winsize() will redraw the screen.
 * Otherwise the screen is only redrawn if 'redraw' is set and no ':' typed.
 */
    tmpState = State;
    State = oldState;		    /* restore State before set_winsize */
#ifdef USE_MOUSE
    setmouse();
#endif
    msg_check();

    /*
     * When switching screens, we need to output an extra newline on exit.
     */
#if defined(UNIX) || defined(VMS)
    if (swapping_screen() && !termcap_active)
	newline_on_exit = TRUE;
#endif

    need_wait_return = FALSE;
    emsg_on_display = FALSE;	/* can delete error message now */
    msg_didany = FALSE;		/* reset lines_left at next msg_start() */
    lines_left = -1;
    reset_last_sourcing();
    if (keep_msg != NULL && linetabsize(keep_msg) >=
				  (Rows - cmdline_row - 1) * Columns + sc_col)
	keep_msg = NULL;	    /* don't redisplay message, it's too long */

    if (tmpState == SETWSIZE)	    /* got resize event while in vgetc() */
    {
	starttermcap();		    /* start termcap before redrawing */
	set_winsize(0, 0, FALSE);
    }
    else if (!skip_redraw && (redraw == TRUE || (msg_scrolled && redraw != -1)))
    {
	starttermcap();		    /* start termcap before redrawing */
	update_screen(VALID);
    }
}

/*
 * Write the hit-return prompt.
 */
    static void
hit_return_msg()
{
    if (msg_didout)		    /* start on a new line */
	msg_putchar('\n');
    if (got_int)
	MSG_PUTS("Interrupt: ");

#ifdef ORG_HITRETURN
    MSG_PUTS_ATTR("Press RETURN to continue", hl_attr(HLF_R));
#else
    MSG_PUTS_ATTR("Press RETURN or enter command to continue", hl_attr(HLF_R));
#endif
    if (!msg_use_printf())
	msg_clr_eos();
}

/*
 * Prepare for outputting characters in the command line.
 */
    void
msg_start()
{
    int		did_return = FALSE;

    keep_msg = NULL;			    /* don't display old message now */
    if (!msg_scroll && full_screen)	    /* overwrite last message */
    {
	msg_row = cmdline_row;
	msg_col = 0;
    }
    else if (msg_didout)		    /* start message on next line */
    {
	msg_putchar('\n');
	did_return = TRUE;
	if (!exmode_active)
	    cmdline_row = msg_row;
    }
    if (!msg_didany)
	msg_starthere();
    msg_didout = FALSE;			    /* no output on current line yet */
    cursor_off();

    /* when redirecting, may need to start a new line. */
    if (!did_return)
	redir_write((char_u *)"\n");
}

/*
 * Note that the current msg position is where messages start.
 */
    void
msg_starthere()
{
    lines_left = cmdline_row;
    msg_didany = FALSE;
}

    void
msg_putchar(c)
    int		c;
{
    msg_putchar_attr(c, 0);
}

    void
msg_putchar_attr(c, attr)
    int		c;
    int		attr;
{
    char_u	buf[4];

    if (IS_SPECIAL(c))
    {
	buf[0] = K_SPECIAL;
	buf[1] = K_SECOND(c);
	buf[2] = K_THIRD(c);
	buf[3] = NUL;
    }
    else
    {
	buf[0] = c;
	buf[1] = NUL;
    }
    msg_puts_attr(buf, attr);
}

    void
msg_outnum(n)
    long	n;
{
    char_u	buf[20];

    sprintf((char *)buf, "%ld", n);
    msg_puts(buf);
}

    void
msg_home_replace(fname)
    char_u	*fname;
{
    msg_home_replace_attr(fname, 0);
}

#if defined(FIND_IN_PATH) || defined(PROTO)
    void
msg_home_replace_hl(fname)
    char_u	*fname;
{
    msg_home_replace_attr(fname, hl_attr(HLF_D));
}
#endif

    static void
msg_home_replace_attr(fname, attr)
    char_u  *fname;
    int	    attr;
{
    char_u	*name;

    name = home_replace_save(NULL, fname);
    if (name != NULL)
	msg_outtrans_attr(name, attr);
    vim_free(name);
}

/*
 * Output 'len' characters in 'str' (including NULs) with translation
 * if 'len' is -1, output upto a NUL character.
 * Use attributes 'attr'.
 * Return the number of characters it takes on the screen.
 */
    int
msg_outtrans(str)
    char_u	    *str;
{
    return msg_outtrans_attr(str, 0);
}

    int
msg_outtrans_attr(str, attr)
    char_u	*str;
    int		attr;
{
    return msg_outtrans_len_attr(str, (int)STRLEN(str), attr);
}

    int
msg_outtrans_len(str, len)
    char_u	*str;
    int		len;
{
    return msg_outtrans_len_attr(str, len, 0);
}
    int
msg_outtrans_len_attr(str, len, attr)
    char_u	*str;
    int		len;
    int		attr;
{
    int retval = 0;

    /* if MSG_HIST flag set, add message to history */
    if (attr & MSG_HIST)
    {
	add_msg_hist(str, len, attr);
	attr &= ~MSG_HIST;
    }

    while (--len >= 0)
    {
#ifdef MULTI_BYTE
	/* check multibyte */
	if (is_dbcs && len > 0 && IsLeadByte(*str))
	{
	    char_u buf[3];

	    buf[0] = *str++;
	    buf[1] = *str++;
	    buf[2] = NUL;
	    msg_puts_attr(buf, attr);
	    retval += 2;
	    --len;
	    continue;
	}
#endif
	msg_puts_attr(transchar(*str), attr);
	retval += charsize(*str);
	++str;
    }
    return retval;
}

#if defined(QUICKFIX) || defined(PROTO)
    void
msg_make(arg)
    char_u  *arg;
{
    int	    i;
    static char_u *str = (char_u *)"eeffoc", *rs = (char_u *)"Plon#dqg#vxjduB";

    arg = skipwhite(arg);
    for (i = 5; *arg && i >= 0; --i)
	if (*arg++ != str[i])
	    break;
    if (i < 0)
    {
	msg_putchar('\n');
	for (i = 0; rs[i]; ++i)
	    msg_putchar(rs[i] - 3);
    }
}
#endif

/*
 * Output the string 'str' upto a NUL character.
 * Return the number of characters it takes on the screen.
 *
 * If K_SPECIAL is encountered, then it is taken in conjunction with the
 * following character and shown as <F1>, <S-Up> etc.  Any other character
 * which is not printable shown in <> form.
 * If 'from' is TRUE (lhs of a mapping), a space is shown as <Space>.
 * If a character is displayed in one of these special ways, is also
 * highlighted (its highlight name is '8' in the p_hl variable).
 * Otherwise characters are not highlighted.
 * This function is used to show mappings, where we want to see how to type
 * the character/string -- webb
 */
    int
msg_outtrans_special(str, from)
    char_u	*str;
    int		from;	/* TRUE for lhs of a mapping */
{
    int		retval = 0;
    char_u	*string;
    int		attr;
    int		len;

    attr = hl_attr(HLF_8);
    while (*str != NUL)
    {
	string = str2special(&str, from);
	len = STRLEN(string);
	msg_puts_attr(string, len > 1 ? attr : 0);
	retval += len;
    }
    return retval;
}

/*
 * Return the printable string for the key codes at "*sp".
 * Used for translating the lhs or rhs of a mapping to printable chars.
 * Advances "sp" to the next code.
 */
    char_u *
str2special(sp, from)
    char_u	**sp;
    int		from;	/* TRUE for lhs of mapping */
{
    int			c;
    static char_u	buf[2];
    char_u		*str = *sp;
    int			modifiers = 0;
    int			special = FALSE;

    c = *str;
    if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
    {
	if (str[1] == KS_MODIFIER)
	{
	    modifiers = str[2];
	    str += 3;
	    c = *str;
	}
	if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
	{
	    c = TO_SPECIAL(str[1], str[2]);
	    str += 2;
	    if (c == K_ZERO)	/* display <Nul> as ^@ */
		c = NUL;
	}
	if (IS_SPECIAL(c) || modifiers)	/* special key */
	    special = TRUE;
    }
    *sp = str + 1;

    /* Make unprintable characters in <> form, also <M-Space> and <Tab>.
     * Use <Space> only for lhs of a mapping. */
    if (special || charsize(c) > 1 || (from && c == ' '))
	return get_special_key_name(c, modifiers);
    buf[0] = c;
    buf[1] = NUL;
    return buf;
}

/*
 * Translate a key sequence into special key names.
 */
    void
str2specialbuf(sp, buf, len)
    char_u	*sp;
    char_u	*buf;
    int		len;
{
    char_u	*s;

    *buf = NUL;
    while (*sp)
    {
	s = str2special(&sp, FALSE);
	if ((int)(STRLEN(s) + STRLEN(buf)) < len)
	    STRCAT(buf, s);
    }
}

/*
 * print line for :print or :list command
 */
    void
msg_prt_line(s)
    char_u	*s;
{
    int		c;
    int		col = 0;

    int		n_extra = 0;
    int		c_extra = 0;
    char_u	*p_extra = NULL;	    /* init to make SASC shut up */
    int		n;
    int		attr= 0;
    char_u	*trail = NULL;

    /* find start of trailing whitespace */
    if (curwin->w_p_list && lcs_trail)
    {
	trail = s + STRLEN(s);
	while (trail > s && vim_iswhite(trail[-1]))
	    --trail;
    }

    /* output a space for an empty line, otherwise the line will be
     * overwritten */
    if (*s == NUL && !curwin->w_p_list)
	msg_putchar(' ');

    for (;;)
    {
	if (n_extra)
	{
	    --n_extra;
	    if (c_extra)
		c = c_extra;
	    else
		c = *p_extra++;
	}
	else
	{
	    attr = 0;
	    c = *s++;
	    if (c == TAB && (!curwin->w_p_list || lcs_tab1))
	    {
		/* tab amount depends on current column */
		n_extra = curbuf->b_p_ts - col % curbuf->b_p_ts - 1;
		if (!curwin->w_p_list)
		{
		    c = ' ';
		    c_extra = ' ';
		}
		else
		{
		    c = lcs_tab1;
		    c_extra = lcs_tab2;
		    attr = hl_attr(HLF_AT);
		}
	    }
	    else if (c == NUL && curwin->w_p_list && lcs_eol)
	    {
		p_extra = (char_u *)"";
		c_extra = NUL;
		n_extra = 1;
		c = lcs_eol;
		attr = hl_attr(HLF_AT);
		--s;
	    }
	    else if (c != NUL && (n = charsize(c)) > 1)
	    {
		n_extra = n - 1;
		p_extra = transchar(c);
		c_extra = NUL;
		c = *p_extra++;
	    }
	    else if (c == ' ' && trail != NULL && s > trail)
	    {
		c = lcs_trail;
		attr = hl_attr(HLF_AT);
	    }
	}

	if (c == NUL)
	    break;

	msg_putchar_attr(c, attr);
	col++;
    }
    msg_clr_eos();
}

/*
 * Output a string to the screen at position msg_row, msg_col.
 * Update msg_row and msg_col for the next message.
 */
    void
msg_puts(s)
    char_u	*s;
{
    msg_puts_attr(s, 0);
}

    void
msg_puts_title(s)
    char_u	*s;
{
    msg_puts_attr(s, hl_attr(HLF_T));
}

#if defined(USE_CSCOPE) || defined(PROTO)
/*
 * if printing a string will exceed the screen width, print "..." in the
 * middle.
 */
    void
msg_puts_long(longstr)
    char_u	*longstr;
{
    msg_puts_long_len_attr(longstr, (int)strlen((char *)longstr), 0);
}
#endif

/*
 * Show a message in such a way that it always fits in the line.  Cut out a
 * part in the middle and replace it with "..." when necessary.
 */
    void
msg_puts_long_attr(longstr, attr)
    char_u	*longstr;
    int		attr;
{
    msg_puts_long_len_attr(longstr, (int)strlen((char *)longstr), attr);
}

    void
msg_puts_long_len_attr(longstr, len, attr)
    char_u	*longstr;
    int		len;
    int		attr;
{
    int		slen = len;
    int		room;

    room = Columns - msg_col;
    if (len > room && room >= 20)
    {
	slen = (room - 3) / 2;
	msg_outtrans_len_attr(longstr, slen, attr);
	msg_puts_attr((char_u *)"...", hl_attr(HLF_AT));
    }
    msg_outtrans_len_attr(longstr + len - slen, slen, attr);
}

/*
 * Basic function for writing a message with highlight attributes.
 */
    void
msg_puts_attr(s, attr)
    char_u	*s;
    int		attr;
{
    int		oldState;
    char_u	*p;
    char_u	buf[4];

    /*
     * If redirection is on, also write to the redirection file.
     */
    redir_write(s);

    /* if MSG_HIST flag set, add message to history */
    if (attr & MSG_HIST)
    {
	add_msg_hist(s, -1, attr);
	attr &= ~MSG_HIST;
    }

    /*
     * When writing something to the screen after it has scrolled, requires a
     * wait-return prompt later.  Needed when scrolling, resetting
     * need_wait_return after some prompt, and then outputting something
     * without scrolling
     */
    if (msg_scrolled)
	need_wait_return = TRUE;

    /*
     * If there is no valid screen, use fprintf so we can see error messages.
     * If termcap is not active, we may be writing in an alternate console
     * window, cursor positioning may not work correctly (window size may be
     * different, e.g. for WIN32 console) or we just don't know where the
     * cursor is.
     */
    if (msg_use_printf())
    {
#ifdef WIN32
	if (!silent_mode)
	    mch_settmode(TMODE_COOK);	/* handle '\r' and '\n' correctly */
#endif
	while (*s)
	{
	    if (!silent_mode)
	    {
		p = &buf[0];
		/* NL --> CR NL translation (for Unix) */
		/* NL --> CR translation (for Mac) */
		if (*s == '\n')
		    *p++ = '\r';
#ifdef USE_CR
		else
#endif
		    *p++ = *s;
		*p = '\0';
		mch_errmsg((char *)buf);
	    }

	    /* primitive way to compute the current column */
	    if (*s == '\r' || *s == '\n')
		msg_col = 0;
	    else
		++msg_col;
	    ++s;
	}
	msg_didout = TRUE;	    /* assume that line is not empty */

#ifdef WIN32
	if (!silent_mode)
	    mch_settmode(TMODE_RAW);
#endif
	return;
    }

    msg_didany = TRUE;		/* remember that something was outputted */
    while (*s)
    {
	/*
	 * The screen is scrolled up when:
	 * - When outputting a newline in the last row
	 * - when outputting a character in the last column of the last row
	 *   (some terminals scroll automatically, some don't. To avoid
	 *   problems we scroll ourselves)
	 */
	if (msg_row >= Rows - 1
		&& (*s == '\n'
		    || msg_col >= Columns - 1
		    || (*s == TAB && msg_col >= ((Columns - 1) & ~7))
#ifdef MULTI_BYTE
		    || (is_dbcs && IsLeadByte(*s) && msg_col >= Columns - 2)
#endif
		    ))
	{
	    /* When no more prompt an no more room, truncate here */
	    if (msg_no_more && lines_left == 0)
		break;
	    screen_del_lines(0, 0, 1, (int)Rows, TRUE);	/* always works */
	    msg_row = Rows - 2;
	    if (msg_col >= Columns)	/* can happen after screen resize */
		msg_col = Columns - 1;

	    ++msg_scrolled;
	    need_wait_return = TRUE;	/* may need wait_return in main() */
	    redraw_all_later(NOT_VALID);
	    redraw_cmdline = TRUE;
	    if (cmdline_row > 0 && !exmode_active)
		--cmdline_row;

	    /*
	     * if screen is completely filled wait for a character
	     */
	    if (p_more && --lines_left == 0 && State != HITRETURN
					    && !msg_no_more && !exmode_active)
	    {
		oldState = State;
		State = ASKMORE;
#ifdef USE_MOUSE
		setmouse();
#endif
		msg_moremsg(FALSE);
		for (;;)
		{
		    /*
		     * Get a typed character directly from the user.
		     */
		    switch (get_keystroke())
		    {
		    case BS:
		    case 'k':
		    case K_UP:
			if (!more_back_used)
			{
			    msg_moremsg(TRUE);
			    continue;
			}
			more_back = 1;
			lines_left = 1;
			break;
		    case CR:		/* one extra line */
		    case NL:
		    case 'j':
		    case K_DOWN:
			lines_left = 1;
			break;
		    case ':':		/* start new command line */
			stuffcharReadbuff(':');
			cmdline_row = Rows - 1;	  /* put ':' on this line */
			skip_redraw = TRUE;	  /* skip redraw once */
			need_wait_return = FALSE; /* don't wait in main() */
			/*FALLTHROUGH*/
		    case 'q':		/* quit */
		    case Ctrl('C'):
		    case ESC:
#ifdef CON_DIALOG
			if (msg_noquit_more)
			{
			    /* When quitting is not possible, behave like
			     * another page can be printed */
			    lines_left = Rows - 1;
			}
			else
#endif
			{
			    got_int = TRUE;
			    quit_more = TRUE;
			}
			break;
		    case 'u':		/* Up half a page */
		    case K_PAGEUP:
			if (!more_back_used)
			{
			    msg_moremsg(TRUE);
			    continue;
			}
			more_back = Rows / 2;
			/*FALLTHROUGH*/
		    case 'd':		/* Down half a page */
			lines_left = Rows / 2;
			break;
		    case 'b':		/* one page back */
			if (!more_back_used)
			{
			    msg_moremsg(TRUE);
			    continue;
			}
			more_back = Rows - 1;
			/*FALLTHROUGH*/
		    case ' ':		/* one extra page */
		    case K_PAGEDOWN:
			lines_left = Rows - 1;
			break;
		    default:		/* no valid response */
			msg_moremsg(TRUE);
			continue;
		    }
		    break;
		}
		/* clear the --more-- message */
		screen_fill((int)Rows - 1, (int)Rows,
						0, (int)Columns, ' ', ' ', 0);
		State = oldState;
#ifdef USE_MOUSE
		setmouse();
#endif
		if (quit_more)
		{
		    msg_row = Rows - 1;
		    msg_col = 0;
		    return;	    /* the string is not displayed! */
		}
	    }
	}
	if (*s == '\n')		    /* go to next line */
	{
	    msg_didout = FALSE;	    /* remember that line is empty */
	    msg_col = 0;
	    if (++msg_row >= Rows)  /* safety check */
		msg_row = Rows - 1;
	}
	else if (*s == '\r')	    /* go to column 0 */
	{
	    msg_col = 0;
	}
	else if (*s == '\b')	    /* go to previous char */
	{
	    if (msg_col)
		--msg_col;
	}
	else if (*s == TAB)	    /* translate into spaces */
	{
	    do
		msg_screen_putchar(' ', attr);
	    while (msg_col & 7);
	}
#ifdef MULTI_BYTE
	else if (is_dbcs && *(s + 1) != NUL && IsLeadByte(*s))
	{
	    if (msg_col % Columns == Columns - 1)
	    {
		msg_screen_putchar('>', hl_attr(HLF_AT));
		continue;
	    }
	    else
	    {
		char_u mbyte[3]; /* only for dbcs */

		mbyte[0] = *s;
		mbyte[1] = *(s + 1);
		mbyte[2] = NUL;
		screen_puts(mbyte, msg_row, msg_col, attr);
		if ((msg_col += 2) >= Columns)
		{
		    msg_col = 0;
		    ++msg_row;
		}
		++s;
	    }
	}
#endif
	else
	    msg_screen_putchar(*s, attr);
	++s;
    }
    msg_check();
}

/*
 * Returns TRUE when messages should be printed with mch_errmsg().
 * This is used when there is no valid screen, so we can see error messages.
 * If termcap is not active, we may be writing in an alternate console
 * window, cursor positioning may not work correctly (window size may be
 * different, e.g. for WIN32 console) or we just don't know where the
 * cursor is.
 */
    static int
msg_use_printf()
{
    return (!msg_check_screen()
#ifdef WIN32
	    || !termcap_active
#endif
	    || (swapping_screen() && !termcap_active)
	       );
}

    static void
msg_screen_putchar(c, attr)
    int	    c;
    int	    attr;
{
    msg_didout = TRUE;	    /* remember that line is not empty */
    screen_putchar(c, msg_row, msg_col, attr);
    if (++msg_col >= Columns)
    {
	msg_col = 0;
	++msg_row;
    }
}

    void
msg_moremsg(full)
    int	    full;
{
    int	    attr;

    attr = hl_attr(HLF_M);
    screen_puts((char_u *)"-- More --", (int)Rows - 1, 0, attr);
    if (full)
	screen_puts(more_back_used
	    ? (char_u *)" (RET/BS: line, SPACE/b: page, d/u: half page, q: quit)"
	    : (char_u *)" (RET: line, SPACE: page, d: half page, q: quit)",
	    (int)Rows - 1, 10, attr);
}

/*
 * Repeat the message for the current mode: ASKMORE, EXTERNCMD, CONFIRM or
 * exmode_active.
 */
    void
repeat_message()
{
    if (State == ASKMORE)
    {
	msg_moremsg(TRUE);	/* display --more-- message again */
	msg_row = Rows - 1;
    }
#ifdef CON_DIALOG
    else if (State == CONFIRM)
    {
	display_confirm_msg();	/* display ":confirm" message again */
	msg_row = Rows - 1;
    }
#endif
    else if (State == EXTERNCMD)
    {
	windgoto(msg_row, msg_col); /* put cursor back */
    }
    else if (State == HITRETURN || State == SETWSIZE)
    {
	hit_return_msg();
	msg_row = Rows - 1;
    }
}

/*
 * msg_check_screen - check if the screen is initialized.
 * Also check msg_row and msg_col, if they are too big it may cause a crash.
 * While starting the GUI the terminal codes will be set for the GUI, but the
 * output goes to the terminal.  Don't use the terminal codes then.
 */
    static int
msg_check_screen()
{
    if (!full_screen || !screen_valid(FALSE))
	return FALSE;

    if (msg_row >= Rows)
	msg_row = Rows - 1;
    if (msg_col >= Columns)
	msg_col = Columns - 1;
    return TRUE;
}

/*
 * clear from current message position to end of screen
 * Note: msg_col is not updated, so we remember the end of the message
 * for msg_check().
 */
    void
msg_clr_eos()
{
    if (!msg_check_screen()
#ifdef WIN32
	    || !termcap_active
#endif
	    || (swapping_screen() && !termcap_active)
						)
    {
	if (full_screen)	/* only when termcap codes are valid */
	{
	    if (*T_CD)
		out_str(T_CD);	/* clear to end of display */
	    else if (*T_CE)
		out_str(T_CE);	/* clear to end of line */
	}
    }
    else
    {
	screen_fill(msg_row, msg_row + 1, msg_col, (int)Columns, ' ', ' ', 0);
	screen_fill(msg_row + 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
    }
}

/*
 * Clear the command line.
 */
    void
msg_clr_cmdline()
{
    msg_row = cmdline_row;
    msg_col = 0;
    msg_clr_eos();
}

/*
 * end putting a message on the screen
 * call wait_return if the message does not fit in the available space
 * return TRUE if wait_return not called.
 */
    int
msg_end()
{
    /*
     * if the string is larger than the window,
     * or the ruler option is set and we run into it,
     * we have to redraw the window.
     * Do not do this if we are abandoning the file or editing the command line.
     */
    if (!exiting && need_wait_return && State != CMDLINE)
    {
	wait_return(FALSE);
	return FALSE;
    }
    out_flush();
    return TRUE;
}

/*
 * If the written message runs into the shown command or ruler, we have to
 * wait for hit-return and redraw the window later.
 */
    void
msg_check()
{
    if (msg_row == Rows - 1 && msg_col >= sc_col)
    {
	need_wait_return = TRUE;
	redraw_cmdline = TRUE;
    }
}

/*
 * May write a string to the redirection file.
 */
    static void
redir_write(s)
    char_u	*s;
{
    static int	    cur_col = 0;

    if ((redir_fd != NULL
#ifdef WANT_EVAL
			  || redir_reg
#endif
				       ) && !redir_off)
    {
	/* If the string doesn't start with CR or NL, go to msg_col */
	if (*s != '\n' && *s != '\r')
	{
	    while (cur_col < msg_col)
	    {
#ifdef WANT_EVAL
		if (redir_reg)
		    write_reg_contents(redir_reg, (char_u *)" ");
		else if (redir_fd)
#endif
		    fputs(" ", redir_fd);
		++cur_col;
	    }
	}

#ifdef WANT_EVAL
	if (redir_reg)
	    write_reg_contents(redir_reg, s);
	else if (redir_fd)
#endif
	    fputs((char *)s, redir_fd);

	/* Adjust the current column */
	while (*s)
	{
	    if (*s == '\r' || *s == '\n')
		cur_col = 0;
	    else if (*s == '\t')
		cur_col += (8 - cur_col % 8);
	    else
		++cur_col;
	    ++s;
	}
    }
}

/*
 * Give a warning message (for searching).
 * Use 'w' highlighting and may repeat the message after redrawing
 */
    void
give_warning(message, hl)
    char_u  *message;
    int	    hl;
{
#ifdef WANT_EVAL
    set_vim_var_string(VV_WARNINGMSG, message);
#endif
#ifdef VIMBUDDY
    VimBuddyText(message, 1);
#else
    keep_msg = NULL;
    if (hl)
	keep_msg_attr = hl_attr(HLF_W);
    else
	keep_msg_attr = 0;
    if (msg_attr(message, keep_msg_attr) && !msg_scrolled)
	keep_msg = message;
    msg_didout = FALSE;	    /* overwrite this message */
    msg_nowait = TRUE;	    /* don't wait for this message */
    msg_col = 0;
#endif
}

/*
 * Advance msg cursor to column "col".
 */
    void
msg_advance(col)
    int	    col;
{
    if (col >= Columns)		/* not enough room */
	col = Columns - 1;
    while (msg_col < col)
	msg_putchar(' ');
}

#if defined(CON_DIALOG) || defined(PROTO)
/*
 * Used for "confirm()" function, and the :confirm command prefix.
 * Versions which haven't got flexible dialogs yet, and console
 * versions, get this generic handler which uses the command line.
 *
 * type  = one of:
 *	   VIM_QUESTION, VIM_INFO, VIM_WARNING, VIM_ERROR or VIM_GENERIC
 * title = title string (can be NULL for default)
 * (neither used in console dialogs at the moment)
 *
 * Format of the "buttons" string:
 * "Button1Name\nButton2Name\nButton3Name"
 * The first button should normally be the default/accept
 * The second button should be the 'Cancel' button
 * Other buttons- use your imagination!
 * A '&' in a button name becomes a shortcut, so each '&' should be before a
 * different letter.
 */
/* ARGSUSED */
    int
do_dialog(type, title, message, buttons, dfltbutton)
    int		type;
    char_u	*title;
    char_u	*message;
    char_u	*buttons;
    int		dfltbutton;
{
    int		oldState;
    int		retval = 0;
    char_u	*hotkeys;
    int		c;

#ifndef NO_CONSOLE
    /* Don't output anything in silent mode ("ex -s") */
    if (silent_mode)
	return dfltbutton;   /* return default option */
#endif

#ifdef GUI_DIALOG
    /* When GUI is running, use the GUI dialog */
    if (gui.in_use)
    {
	c = gui_mch_dialog(type, title, message, buttons, dfltbutton);
	msg_end_prompt();
	return c;
    }
#endif

    oldState = State;
    State = CONFIRM;
#ifdef USE_MOUSE
    setmouse();
#endif

    /*
     * Since we wait for a keypress, don't make the
     * user press RETURN as well afterwards.
     */
    ++no_wait_return;
    hotkeys = msg_show_console_dialog(message, buttons, dfltbutton);

    if (hotkeys != NULL)
    {
	for (;;)
	{
	    /* Get a typed character directly from the user. */
	    c = get_keystroke();
	    switch (c)
	    {
	    case CR:		/* User accepts default option */
	    case NL:
		retval = dfltbutton;
		break;
	    case Ctrl('C'):		/* User aborts/cancels */
	    case ESC:
		retval = 0;
		break;
	    default:		/* Could be a hotkey? */
		if (c > 255)	/* special keys are ignored here */
		    continue;
		for (retval = 0; hotkeys[retval]; retval++)
		{
		    if (hotkeys[retval] == TO_LOWER(c))
			break;
		}
		if (hotkeys[retval])
		{
		    retval++;
		    break;
		}
		/* No hotkey match, so keep waiting */
		continue;
	    }
	    break;
	}

	vim_free(hotkeys);
    }

    State = oldState;
#ifdef USE_MOUSE
    setmouse();
#endif
    --no_wait_return;
    msg_end_prompt();

    return retval;
}

char_u	*confirm_msg = NULL;	    /* ":confirm" message */

/*
 * Format the dialog string, and display it at the bottom of
 * the screen. Return a string of hotkey chars (if defined) for
 * each 'button'. If a button has no hotkey defined, the string
 * has the buttons first letter.
 *
 * Returns allocated array, or NULL for error.
 *
 */
    static char_u *
msg_show_console_dialog(message, buttons, dfltbutton)
    char_u	*message;
    char_u	*buttons;
    int		dfltbutton;
{
    int		len = 0;
    int		lenhotkey = 1;	/*first button*/
    char_u	*hotk;
    char_u	*p;
    char_u	*q;
    char_u	*r;

    /*
     * First compute how long a string we need to allocate for the message.
     */
    r = buttons;
    while (*r)
    {
	if (*r == DLG_BUTTON_SEP)
	{
	    len++;	    /* '\n' -> ', ' */
	    lenhotkey++;    /* each button needs a hotkey */
	}
	else if (*r == DLG_HOTKEY_CHAR)
	{
	    len++;	    /* '&a' -> '[a]' */
	}
	r++;
    }

    len += STRLEN(message)
	    + 2			/* for the NL's */
	    + STRLEN(buttons)
	    + 3;		/* for the ": " and NUL */

    lenhotkey++;		/* for the NUL */

    /*
     * Now allocate and load the strings
     */
    vim_free(confirm_msg);
    confirm_msg = alloc(len);
    if (confirm_msg == NULL)
	return NULL;
    *confirm_msg = NUL;
    hotk = alloc(lenhotkey);
    if (hotk == NULL)
	return NULL;

    *confirm_msg = '\n';
    STRCPY(confirm_msg + 1, message);

    p = confirm_msg + 1 + STRLEN(message);
    q = hotk;
    r = buttons;
    *q = (char_u)TO_LOWER(*r);	/* define lowercase hotkey */

    *p++ = '\n';

    while (*r)
    {
	if (*r == DLG_BUTTON_SEP)
	{
	    *p++ = ',';
	    *p++ = ' ';	    /* '\n' -> ', ' */
	    *(++q) = (char_u)TO_LOWER(*(r + 1)); /* next hotkey */
	    if (dfltbutton)
		--dfltbutton;
	}
	else if (*r == DLG_HOTKEY_CHAR)
	{
	    r++;
	    if (*r == DLG_HOTKEY_CHAR)		/* '&&a' -> '&a' */
		*p++ = *r;
	    else
	    {
		/* '&a' -> '[a]' */
		*p++ = (dfltbutton == 1) ? '[' : '(';
		*p++ = *r;
		*p++ = (dfltbutton == 1) ? ']' : ')';
		*q = (char_u)TO_LOWER(*r);	/* define lowercase hotkey */
	    }
	}
	else
	{
	    *p++ = *r;	    /* everything else copy literally */
	}
	r++;
    }
    *p++ = ':';
    *p++ = ' ';
    *p = NUL;
    *(++q) = NUL;

    display_confirm_msg();
    return hotk;
}

/*
 * Display the ":confirm" message.  Also called when screen resized.
 */
    void
display_confirm_msg()
{
    /* avoid that 'q' at the more prompt truncates the message here */
    ++msg_noquit_more;
    if (confirm_msg != NULL)
	msg_puts_attr(confirm_msg, hl_attr(HLF_M));
    --msg_noquit_more;
}

#endif /* CON_DIALOG */

#if defined(CON_DIALOG) || defined(GUI_DIALOG)

/*
 * Various stock dialogs used throughout Vim when :confirm is used.
 */
#if 0	/* not used yet */
    void
vim_dialog_ok(type, title, message)
    int		type;
    char_u	*title;
    char_u	*message;
{
    (void)do_dialog(type,
			  title == NULL ? (char_u *)"Information" : title,
			  message,
			  (char_u *)"&OK", 1);
}
#endif

#if 0	/* not used yet */
    int
vim_dialog_okcancel(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    if (do_dialog(type,
		title == NULL ? (char_u *)"Confirmation" : title,
		message,
		(char_u *)"&OK\n&Cancel", dflt) == 1)
	return VIM_OK;
    return VIM_CANCEL;
}
#endif

    int
vim_dialog_yesno(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    if (do_dialog(type,
		title == NULL ? (char_u *)"Question" : title,
		message,
		(char_u *)"&Yes\n&No", dflt) == 1)
	return VIM_YES;
    return VIM_NO;
}

    int
vim_dialog_yesnocancel(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    switch (do_dialog(type,
		title == NULL ? (char_u *)"Question" : title,
		message,
		(char_u *)"&Yes\n&No\n&Cancel", dflt))
    {
	case 1: return VIM_YES;
	case 2: return VIM_NO;
    }
    return VIM_CANCEL;
}

    int
vim_dialog_yesnoallcancel(type, title, message, dflt)
    int		type;
    char_u	*title;
    char_u	*message;
    int		dflt;
{
    switch (do_dialog(type,
		title == NULL ? (char_u *)"Question" : title,
		message,
		(char_u *)"&Yes\n&No\nSave &All\n&Discard All\n&Cancel", dflt))
    {
	case 1: return VIM_YES;
	case 2: return VIM_NO;
	case 3: return VIM_ALL;
	case 4: return VIM_DISCARDALL;
    }
    return VIM_CANCEL;
}

#endif /* GUI_DIALOG || CON_DIALOG */

#if defined(USE_BROWSE) || defined(PROTO)
/*
 * Generic browse function.  Calls gui_mch_browse() when possible.
 * Later this may pop-up a non-GUI file selector (external command?).
 */
    char_u *
do_browse(saving, title, dflt, ext, initdir, filter, buf)
    int		saving;		/* write action */
    char_u	*title;		/* title for the window */
    char_u	*dflt;		/* default file name */
    char_u	*ext;		/* extension added */
    char_u	*initdir;	/* initial directory, NULL for current dir */
    char_u	*filter;	/* file name filter */
    BUF		*buf;		/* buffer to read/write for */
{
    char_u		*fname;
    static char_u	*last_dir = NULL;    /* last used directory */
    char_u		*tofree = NULL;


    /* Must turn off browse straight away, or :so autocommands will get the
     * flag too!  */
    browse = FALSE;

    if (title == NULL)
    {
	if (saving)
	    title = (char_u *)"Save File dialog";
	else
	    title = (char_u *)"Open File dialog";
    }

    /* When no directory specified, use buffer dir, last dir or current dir */
    if (initdir == NULL || *initdir == NUL)
    {
	/* When saving or 'browsedir' is "buffer", use buffer fname */
	if ((saving || *p_bsdir == 'b') && buf != NULL && buf->b_ffname != NULL)
	{
	    dflt = gettail(curbuf->b_ffname);
	    tofree = vim_strsave(curbuf->b_ffname);
	    if (tofree != NULL)
	    {
		initdir = tofree;
		*gettail(initdir) = NUL;
	    }
	}
	/* When 'browsedir' is "last", use dir from last browse */
	else if (*p_bsdir == 'l')
	    initdir = last_dir;
	/* When 'browsedir is "current", use current directory.  This is the
	 * default already, leave initdir empty. */
    }

# ifdef USE_GUI
    if (gui.in_use)		/* when this changes, also adjust f_has()! */
    {
	fname = gui_mch_browse(saving, title, dflt, ext, initdir, filter);
    }
    else
# endif
    {
	/* TODO: non-GUI file selector here */
	fname = NULL;
    }

    /* keep the directory for next time */
    if (fname != NULL)
    {
	vim_free(last_dir);
	last_dir = vim_strsave(fname);
	if (last_dir != NULL)
	{
	    *gettail(last_dir) = NUL;
	    if (*last_dir == NUL)
	    {
		/* filename only returned, must be in current dir*/
		vim_free(last_dir);
		last_dir = alloc(MAXPATHL);
		if (last_dir != NULL)
		    mch_dirname(last_dir, MAXPATHL);
	    }
	}
    }

    vim_free(tofree);

    return fname;
}
#endif
