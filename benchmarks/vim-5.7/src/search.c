/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
 * search.c: code for normal mode searching commands
 */

#include "vim.h"

static void save_re_pat __ARGS((int idx, char_u *pat, int magic));
static int inmacro __ARGS((char_u *, char_u *));
static int check_linecomment __ARGS((char_u *line));
static int cls __ARGS((void));
static int skip_chars __ARGS((int, int));
#ifdef TEXT_OBJECTS
static void back_in_line __ARGS((void));
static void find_first_blank __ARGS((FPOS *));
static void findsent_forward __ARGS((long count, int at_start_sent));
#endif
#ifdef FIND_IN_PATH
static void show_pat_in_path __ARGS((char_u *, int,
					 int, int, FILE *, linenr_t *, long));
#endif
#ifdef VIMINFO
static void wvsp_one __ARGS((FILE *fp, int idx, char *s, int sc));
#endif

static char_u *top_bot_msg = (char_u *)"search hit TOP, continuing at BOTTOM";
static char_u *bot_top_msg = (char_u *)"search hit BOTTOM, continuing at TOP";

/*
 * This file contains various searching-related routines. These fall into
 * three groups:
 * 1. string searches (for /, ?, n, and N)
 * 2. character searches within a single line (for f, F, t, T, etc)
 * 3. "other" kinds of searches like the '%' command, and 'word' searches.
 */

/*
 * String searches
 *
 * The string search functions are divided into two levels:
 * lowest:  searchit(); uses an FPOS for starting position and found match.
 * Highest: do_search(); uses curwin->w_cursor; calls searchit().
 *
 * The last search pattern is remembered for repeating the same search.
 * This pattern is shared between the :g, :s, ? and / commands.
 * This is in search_regcomp().
 *
 * The actual string matching is done using a heavily modified version of
 * Henry Spencer's regular expression library.  See regexp.c.
 */

/* The offset for a search command is store in a soff struct */
/* Note: only spats[0].off is really used */
struct soffset
{
    int		dir;		/* search direction */
    int		line;		/* search has line offset */
    int		end;		/* search set cursor at end */
    long	off;		/* line or char offset */
};

/* A search pattern and its attributes are stored in a spat struct */
struct spat
{
    char_u	    *pat;	/* the pattern (in allocated memory) or NULL */
    int		    magic;	/* magicness of the pattern */
    int		    no_scs;	/* no smarcase for this pattern */
    struct soffset  off;
};

/*
 * Two search patterns are remembered: One for the :substitute command and
 * one for other searches.  last_idx points to the one that was used the last
 * time.
 */
static struct spat spats[2] =
{
    {NULL, TRUE, FALSE, {'/', 0, 0, 0L}},	/* last used search pat */
    {NULL, TRUE, FALSE, {'/', 0, 0, 0L}}	/* last used substitute pat */
};

static int last_idx = 0;	/* index in spats[] for RE_LAST */

#if defined(AUTOCMD) || defined(WANT_EVAL) || defined(PROTO)
/* copy of spats[], for keeping the search patterns while executing autocmds */
static struct spat  saved_spats[2];
static int	    saved_last_idx = 0;
# ifdef EXTRA_SEARCH
static int	    saved_no_hlsearch = 0;
# endif
#endif

static char_u	    *mr_pattern = NULL;	/* pattern used by search_regcomp() */

#ifdef FIND_IN_PATH
/*
 * Type used by find_pattern_in_path() to remember which included files have
 * been searched already.
 */
typedef struct SearchedFile
{
    FILE	*fp;		/* File pointer */
    char_u	*name;		/* Full name of file */
    linenr_t	lnum;		/* Line we were up to in file */
    int		matched;	/* Found a match in this file */
} SearchedFile;
#endif

/*
 * translate search pattern for vim_regcomp()
 *
 * pat_save == RE_SEARCH: save pat in spats[RE_SEARCH].pat (normal search cmd)
 * pat_save == RE_SUBST: save pat in spats[RE_SUBST].pat (:substitute command)
 * pat_save == RE_BOTH: save pat in both patterns (:global command)
 * pat_use  == RE_SEARCH: use previous search pattern if "pat" is NULL
 * pat_use  == RE_SUBST: use previous sustitute pattern if "pat" is NULL
 * pat_use  == RE_LAST: use last used pattern if "pat" is NULL
 * options & SEARCH_HIS: put search string in history
 * options & SEARCH_KEEP: keep previous search pattern
 *
 */
    vim_regexp *
search_regcomp(pat, pat_save, pat_use, options)
    char_u  *pat;
    int	    pat_save;
    int	    pat_use;
    int	    options;
{
    int	    magic;
    int	    i;

    rc_did_emsg = FALSE;
    magic = p_magic;

    /*
     * If no pattern given, use a previously defined pattern.
     */
    if (pat == NULL || *pat == NUL)
    {
	if (pat_use == RE_LAST)
	    i = last_idx;
	else
	    i = pat_use;
	if (spats[i].pat == NULL)	/* pattern was never defined */
	{
	    if (pat_use == RE_SUBST)
		emsg(e_nopresub);
	    else
		emsg(e_noprevre);
	    rc_did_emsg = TRUE;
	    return (vim_regexp *)NULL;
	}
	pat = spats[i].pat;
	magic = spats[i].magic;
	no_smartcase = spats[i].no_scs;
    }
    else if (options & SEARCH_HIS)	/* put new pattern in history */
	add_to_history(HIST_SEARCH, pat, TRUE);

    mr_pattern = pat;

    /*
     * Save the currently used pattern in the appropriate place,
     * unless the pattern should not be remembered.
     */
    if (!(options & SEARCH_KEEP))
    {
	/* search or global command */
	if (pat_save == RE_SEARCH || pat_save == RE_BOTH)
	    save_re_pat(RE_SEARCH, pat, magic);
	/* substitute or global command */
	if (pat_save == RE_SUBST || pat_save == RE_BOTH)
	    save_re_pat(RE_SUBST, pat, magic);
    }

    set_reg_ic(pat);		/* tell the vim_regexec routine how to search */
    return vim_regcomp(pat, magic);
}

/*
 * Get search pattern used by search_regcomp().
 */
    char_u *
get_search_pat()
{
    return mr_pattern;
}

    static void
save_re_pat(idx, pat, magic)
    int		idx;
    char_u	*pat;
    int		magic;
{
    if (spats[idx].pat != pat)
    {
	vim_free(spats[idx].pat);
	spats[idx].pat = vim_strsave(pat);
	spats[idx].magic = magic;
	spats[idx].no_scs = no_smartcase;
	last_idx = idx;
#ifdef EXTRA_SEARCH
	/* If 'hlsearch' set and search pat changed: need redraw. */
	if (p_hls)
	    redraw_all_later(NOT_VALID);
	no_hlsearch = FALSE;
#endif
    }
}

#if defined(AUTOCMD) || defined(WANT_EVAL) || defined(PROTO)
/*
 * Save the search patterns, so they can be restored later.
 * Used before/after executing autocommands and user functions.
 */
static int save_level = 0;

    void
save_search_patterns()
{
    if (save_level++ == 0)
    {
	saved_spats[0] = spats[0];
	if (spats[0].pat != NULL)
	    saved_spats[0].pat = vim_strsave(spats[0].pat);
	saved_spats[1] = spats[1];
	if (spats[1].pat != NULL)
	    saved_spats[1].pat = vim_strsave(spats[1].pat);
	saved_last_idx = last_idx;
# ifdef EXTRA_SEARCH
	saved_no_hlsearch = no_hlsearch;
# endif
    }
}

    void
restore_search_patterns()
{
    if (--save_level == 0)
    {
	vim_free(spats[0].pat);
	spats[0] = saved_spats[0];
	vim_free(spats[1].pat);
	spats[1] = saved_spats[1];
	last_idx = saved_last_idx;
# ifdef EXTRA_SEARCH
	no_hlsearch = saved_no_hlsearch;
# endif
    }
}
#endif

/*
 * Set reg_ic according to p_ic, p_scs and the search pattern.
 */
    void
set_reg_ic(pat)
    char_u  *pat;
{
    char_u *p;

    reg_ic = p_ic;
    if (reg_ic && !no_smartcase && p_scs
#ifdef INSERT_EXPAND
				&& !(ctrl_x_mode && curbuf->b_p_inf)
#endif
								    )
    {
	/* don't ignore case if pattern has uppercase */
	for (p = pat; *p; )
	{
#ifdef MULTI_BYTE
	    if (is_dbcs && IsLeadByte(*p))
	    {
		if (*++p == NUL)
		    break;
		++p;
	    }
	    else
#endif
		if (isupper(*p++))
		    reg_ic = FALSE;
	}
    }
    no_smartcase = FALSE;
}

    char_u *
last_search_pat()
{
    return spats[last_idx].pat;
}

#if defined(WANT_EVAL) || defined(VIMINFO)
/*
 * Set the last search pattern.  For ":let @/ =" and viminfo.
 * Also set the saved search pattern, so that this works in an autocommand.
 */
    void
set_last_search_pat(s, idx, magic, setlast)
    char_u	*s;
    int		idx;
    int		magic;
    int		setlast;
{
    vim_free(spats[idx].pat);
    spats[idx].pat = vim_strsave(s);
    spats[idx].magic = magic;
    spats[idx].no_scs = FALSE;
    spats[idx].off.dir = '/';
    spats[idx].off.line = FALSE;
    spats[idx].off.end = FALSE;
    spats[idx].off.off = 0;
    if (setlast)
	last_idx = idx;
    if (save_level)
    {
	vim_free(saved_spats[idx].pat);
	saved_spats[idx] = spats[0];
	if (spats[idx].pat != NULL)
	    saved_spats[idx].pat = vim_strsave(spats[idx].pat);
	saved_last_idx = last_idx;
    }
#ifdef EXTRA_SEARCH
    /* If 'hlsearch' set and search pat changed: need redraw. */
    if (p_hls && idx == last_idx && !no_hlsearch)
	redraw_all_later(NOT_VALID);
#endif
}
#endif

#ifdef EXTRA_SEARCH
/*
 * Get a regexp program for the last used search pattern.
 * This is used for highlighting all matches in a window.
 */
    vim_regexp *
last_pat_prog()
{
    vim_regexp *prog;

    if (spats[last_idx].pat == NULL)
	return NULL;
    ++emsg_off;		/* So it doesn't beep if bad expr */
    prog = search_regcomp((char_u *)"", 0, last_idx, SEARCH_KEEP);
    --emsg_off;
    return prog;
}
#endif

/*
 * lowest level search function.
 * Search for 'count'th occurrence of 'str' in direction 'dir'.
 * Start at position 'pos' and return the found position in 'pos'.
 *
 * if (options & SEARCH_MSG) == 0 don't give any messages
 * if (options & SEARCH_MSG) == SEARCH_NFMSG don't give 'notfound' messages
 * if (options & SEARCH_MSG) == SEARCH_MSG give all messages
 * if (options & SEARCH_HIS) put search pattern in history
 * if (options & SEARCH_END) return position at end of match
 * if (options & SEARCH_START) accept match at pos itself
 * if (options & SEARCH_KEEP) keep previous search pattern
 *
 * Return OK for success, FAIL for failure.
 */
    int
searchit(buf, pos, dir, str, count, options, pat_use)
    BUF	    *buf;
    FPOS    *pos;
    int	    dir;
    char_u  *str;
    long    count;
    int	    options;
    int	    pat_use;
{
    int			found;
    linenr_t		lnum;		/* no init to shut up Apollo cc */
    vim_regexp		*prog;
    char_u		*ptr;
    char_u		*match = NULL, *matchend = NULL;    /* init for GCC */
    int			loop;
    FPOS		start_pos;
    int			at_first_line;
    int			extra_col;
    int			match_ok;
    char_u		*p;

    if ((prog = search_regcomp(str, RE_SEARCH, pat_use,
			     (options & (SEARCH_HIS + SEARCH_KEEP)))) == NULL)
    {
	if ((options & SEARCH_MSG) && !rc_did_emsg)
	    emsg2((char_u *)"Invalid search string: %s", mr_pattern);
	return FAIL;
    }

    if (options & SEARCH_START)
	extra_col = 0;
    else
	extra_col = 1;


/*
 * find the string
 */
    do	/* loop for count */
    {
	start_pos = *pos;	/* remember start pos for detecting no match */
	found = 0;		/* default: not found */
	at_first_line = TRUE;	/* default: start in first line */
	if (pos->lnum == 0)	/* correct lnum for when starting in line 0 */
	{
	    pos->lnum = 1;
	    pos->col = 0;
	    at_first_line = FALSE;  /* not in first line now */
	}

	/*
	 * Start searching in current line, unless searching backwards and
	 * we're in column 0.
	 */
	if (dir == BACKWARD && start_pos.col == 0)
	{
	    lnum = pos->lnum - 1;
	    at_first_line = FALSE;
	}
	else
	    lnum = pos->lnum;

	for (loop = 0; loop <= 1; ++loop)   /* loop twice if 'wrapscan' set */
	{
	    for ( ; lnum > 0 && lnum <= buf->b_ml.ml_line_count;
					   lnum += dir, at_first_line = FALSE)
	    {
		/*
		 * Look for a match somewhere in the line.
		 */
		ptr = ml_get_buf(buf, lnum, FALSE);
		if (vim_regexec(prog, ptr, TRUE))
		{
		    match = prog->startp[0];
		    matchend = prog->endp[0];

		    /*
		     * Forward search in the first line: match should be after
		     * the start position. If not, continue at the end of the
		     * match (this is vi compatible).
		     */
		    if (dir == FORWARD && at_first_line)
		    {
			match_ok = TRUE;
			/*
			 * When *match == NUL the cursor will be put one back
			 * afterwards, compare with that position, otherwise
			 * "/$" will get stuck on end of line.
			 */
			while ((options & SEARCH_END) ?
						 ((int)(matchend - ptr) - 1  <
					     (int)start_pos.col + extra_col) :
				  ((int)(match - ptr) - (int)(*match == NUL) <
					      (int)start_pos.col + extra_col))
			{
			    /*
			     * If vi-compatible searching, continue at the end
			     * of the match, otherwise continue one position
			     * forward.
			     */
			    if (vim_strchr(p_cpo, CPO_SEARCH) != NULL)
			    {
				p = matchend;
				if (match == p && *p != NUL)
				    ++p;
			    }
			    else
			    {
				p = match;
				if (*p != NUL)
				    ++p;
			    }
			    if (*p != NUL && vim_regexec(prog, p, FALSE))
			    {
				match = prog->startp[0];
				matchend = prog->endp[0];
			    }
			    else
			    {
				match_ok = FALSE;
				break;
			    }
			}
			if (!match_ok)
			    continue;
		    }
		    if (dir == BACKWARD)
		    {
			/*
			 * Now, if there are multiple matches on this line,
			 * we have to get the last one. Or the last one before
			 * the cursor, if we're on that line.
			 * When putting the new cursor at the end, compare
			 * relative to the end of the match.
			 */
			match_ok = FALSE;
			for (;;)
			{
			    if (!at_first_line || ((options & SEARCH_END) ?
					((prog->endp[0] - ptr) - 1 + extra_col
						      <= (int)start_pos.col) :
					  ((prog->startp[0] - ptr) + extra_col
						      <= (int)start_pos.col)))
			    {
				match_ok = TRUE;
				match = prog->startp[0];
				matchend = prog->endp[0];
			    }
			    else
				break;
			    /*
			     * If vi-compatible searching, continue at the end
			     * of the match, otherwise continue one position
			     * forward.
			     */
			    if (vim_strchr(p_cpo, CPO_SEARCH) != NULL)
			    {
				p = matchend;
				if (p == match && *p != NUL)
				    ++p;
			    }
			    else
			    {
				p = match;
				if (*p != NUL)
				    ++p;
			    }
			    if (*p == NUL || !vim_regexec(prog, p, (int)FALSE))
				break;
			}

			/*
			 * If there is only a match after the cursor, skip
			 * this match.
			 */
			if (!match_ok)
			    continue;
		    }

		    pos->lnum = lnum;
		    if (options & SEARCH_END && !(options & SEARCH_NOOF))
			pos->col = (int) (matchend - ptr - 1);
		    else
			pos->col = (int) (match - ptr);
		    found = 1;
		    break;
		}
		line_breakcheck();	/* stop if ctrl-C typed */
		if (got_int)
		    break;

		if (loop && lnum == start_pos.lnum)
		    break;	    /* if second loop, stop where started */
	    }
	    at_first_line = FALSE;

	    /*
	     * stop the search if wrapscan isn't set, after an interrupt and
	     * after a match
	     */
	    if (!p_ws || got_int || found)
		break;

	    /*
	     * If 'wrapscan' is set we continue at the other end of the file.
	     * If 'shortmess' does not contain 's', we give a message.
	     * This message is also remembered in keep_msg for when the screen
	     * is redrawn. The keep_msg is cleared whenever another message is
	     * written.
	     */
	    if (dir == BACKWARD)    /* start second loop at the other end */
	    {
		lnum = buf->b_ml.ml_line_count;
		if (!shortmess(SHM_SEARCH) && (options & SEARCH_MSG))
		    give_warning(top_bot_msg, TRUE);
	    }
	    else
	    {
		lnum = 1;
		if (!shortmess(SHM_SEARCH) && (options & SEARCH_MSG))
		    give_warning(bot_top_msg, TRUE);
	    }
	}
	if (got_int)
	    break;
    }
    while (--count > 0 && found);   /* stop after count matches or no match */

    vim_free(prog);

    if (!found)		    /* did not find it */
    {
	if (got_int)
	    emsg(e_interr);
	else if ((options & SEARCH_MSG) == SEARCH_MSG)
	{
	    if (p_ws)
		emsg2(e_patnotf2, mr_pattern);
	    else if (lnum == 0)
		EMSG2("search hit TOP without match for: %s", mr_pattern);
	    else
		EMSG2("search hit BOTTOM without match for: %s", mr_pattern);
	}
	return FAIL;
    }
    search_match_len = matchend - match;

    return OK;
}

/*
 * Highest level string search function.
 * Search for the 'count'th occurence of string 'str' in direction 'dirc'
 *		  If 'dirc' is 0: use previous dir.
 *    If 'str' is NULL or empty : use previous string.
 *    If 'options & SEARCH_REV' : go in reverse of previous dir.
 *    If 'options & SEARCH_ECHO': echo the search command and handle options
 *    If 'options & SEARCH_MSG' : may give error message
 *    If 'options & SEARCH_OPT' : interpret optional flags
 *    If 'options & SEARCH_HIS' : put search pattern in history
 *    If 'options & SEARCH_NOOF': don't add offset to position
 *    If 'options & SEARCH_MARK': set previous context mark
 *    If 'options & SEARCH_KEEP': keep previous search pattern
 *    If 'options & SEARCH_START': accept match at curpos itself
 *
 * Careful: If spats[0].off.line == TRUE and spats[0].off.off == 0 this
 * makes the movement linewise without moving the match position.
 *
 * return 0 for failure, 1 for found, 2 for found and line offset added
 */
    int
do_search(oap, dirc, str, count, options)
    OPARG	    *oap;
    int		    dirc;
    char_u	   *str;
    long	    count;
    int		    options;
{
    FPOS	    pos;	/* position of the last match */
    char_u	    *searchstr;
    struct soffset  old_off;
    int		    retval;	/* Return value */
    char_u	    *p;
    long	    c;
    char_u	    *dircp;

    /*
     * A line offset is not remembered, this is vi compatible.
     */
    if (spats[0].off.line && vim_strchr(p_cpo, CPO_LINEOFF) != NULL)
    {
	spats[0].off.line = FALSE;
	spats[0].off.off = 0;
    }

    /*
     * Save the values for when (options & SEARCH_KEEP) is used.
     * (there is no "if ()" around this because gcc wants them initialized)
     */
    old_off = spats[0].off;

    pos = curwin->w_cursor;	/* start searching at the cursor position */

    /*
     * Find out the direction of the search.
     */
    if (dirc == 0)
	dirc = spats[0].off.dir;
    else
	spats[0].off.dir = dirc;
    if (options & SEARCH_REV)
    {
#ifdef WIN32
	/* There is a bug in the Visual C++ 2.2 compiler which means that
	 * dirc always ends up being '/' */
	dirc = (dirc == '/')  ?  '?'  :  '/';
#else
	if (dirc == '/')
	    dirc = '?';
	else
	    dirc = '/';
#endif
    }

#ifdef EXTRA_SEARCH
    /*
     * Turn 'hlsearch' highlighting back on.
     */
    if (no_hlsearch && !(options & SEARCH_KEEP))
    {
	redraw_all_later(NOT_VALID);
	no_hlsearch = FALSE;
    }
#endif

    /*
     * Repeat the search when pattern followed by ';', e.g. "/foo/;?bar".
     */
    for (;;)
    {
	searchstr = str;
	dircp = NULL;
					    /* use previous pattern */
	if (str == NULL || *str == NUL || *str == dirc)
	{
	    if (spats[RE_SEARCH].pat == NULL)	    /* no previous pattern */
	    {
		emsg(e_noprevre);
		retval = 0;
		goto end_do_search;
	    }
	    /* make search_regcomp() use spats[RE_SEARCH].pat */
	    searchstr = (char_u *)"";
	}

	if (str != NULL && *str != NUL)	/* look for (new) offset */
	{
	    /*
	     * Find end of regular expression.
	     * If there is a matching '/' or '?', toss it.
	     */
	    p = skip_regexp(str, dirc, (int)p_magic);
	    if (*p == dirc)
	    {
		dircp = p;	/* remember where we put the NUL */
		*p++ = NUL;
	    }
	    spats[0].off.line = FALSE;
	    spats[0].off.end = FALSE;
	    spats[0].off.off = 0;
	    /*
	     * Check for a line offset or a character offset.
	     * For get_address (echo off) we don't check for a character
	     * offset, because it is meaningless and the 's' could be a
	     * substitute command.
	     */
	    if (*p == '+' || *p == '-' || isdigit(*p))
		spats[0].off.line = TRUE;
	    else if ((options & SEARCH_OPT) &&
					(*p == 'e' || *p == 's' || *p == 'b'))
	    {
		if (*p == 'e')		/* end */
		    spats[0].off.end = SEARCH_END;
		++p;
	    }
	    if (isdigit(*p) || *p == '+' || *p == '-')	   /* got an offset */
	    {
					    /* 'nr' or '+nr' or '-nr' */
		if (isdigit(*p) || isdigit(*(p + 1)))
		    spats[0].off.off = atol((char *)p);
		else if (*p == '-')	    /* single '-' */
		    spats[0].off.off = -1;
		else			    /* single '+' */
		    spats[0].off.off = 1;
		++p;
		while (isdigit(*p))	    /* skip number */
		    ++p;
	    }
	    searchcmdlen = p - str;	    /* compute length of search command
							    for get_address() */
	    str = p;			    /* put str after search command */
	}

	if ((options & SEARCH_ECHO) && messaging())
	{
	    char_u	*msgbuf;
	    char_u	*trunc;

	    if (*searchstr == NUL)
		p = spats[last_idx].pat;
	    else
		p = searchstr;
	    msgbuf = alloc((unsigned)(STRLEN(p) + 40));
	    if (msgbuf != NULL)
	    {
		msgbuf[0] = dirc;
		STRCPY(msgbuf + 1, p);
		if (spats[0].off.line || spats[0].off.end || spats[0].off.off)
		{
		    p = msgbuf + STRLEN(msgbuf);
		    *p++ = dirc;
		    if (spats[0].off.end)
			*p++ = 'e';
		    else if (!spats[0].off.line)
			*p++ = 's';
		    if (spats[0].off.off > 0 || spats[0].off.line)
			*p++ = '+';
		    if (spats[0].off.off != 0 || spats[0].off.line)
			sprintf((char *)p, "%ld", spats[0].off.off);
		    else
			*p = NUL;
		}

		msg_start();
		trunc = msg_strtrunc(msgbuf);
		if (trunc != NULL)
		{
		    msg_outtrans(trunc);
		    vim_free(trunc);
		}
		else
		    msg_outtrans(msgbuf);
		msg_clr_eos();
		msg_check();
		vim_free(msgbuf);

		gotocmdline(FALSE);
		out_flush();
		msg_nowait = TRUE;	    /* don't wait for this message */
	    }
	}

	/*
	 * If there is a character offset, subtract it from the current
	 * position, so we don't get stuck at "?pat?e+2" or "/pat/s-2".
	 * This is not done for a line offset, because then we would not be vi
	 * compatible.
	 */
	if (!spats[0].off.line && spats[0].off.off)
	{
	    if (spats[0].off.off > 0)
	    {
		for (c = spats[0].off.off; c; --c)
		    if (decl(&pos) == -1)
			break;
		if (c)			/* at start of buffer */
		{
		    pos.lnum = 0;	/* allow lnum == 0 here */
		    pos.col = MAXCOL;
		}
	    }
	    else
	    {
		for (c = spats[0].off.off; c; ++c)
		    if (incl(&pos) == -1)
			break;
		if (c)			/* at end of buffer */
		{
		    pos.lnum = curbuf->b_ml.ml_line_count + 1;
		    pos.col = 0;
		}
	    }
	}

#ifdef FKMAP	    /* when in Farsi mode, reverse the character flow */
	if (p_altkeymap && curwin->w_p_rl)
	     lrFswap(searchstr,0);
#endif

	c = searchit(curbuf, &pos, dirc == '/' ? FORWARD : BACKWARD,
		searchstr, count, spats[0].off.end + (options &
		       (SEARCH_KEEP + SEARCH_HIS + SEARCH_MSG + SEARCH_START +
			   ((str != NULL && *str == ';') ? 0 : SEARCH_NOOF))),
		RE_LAST);
	if (dircp != NULL)
	    *dircp = dirc;	/* restore second '/' or '?' for normal_cmd() */
	if (c == FAIL)
	{
	    retval = 0;
	    goto end_do_search;
	}
	if (spats[0].off.end && oap != NULL)
	    oap->inclusive = TRUE;  /* 'e' includes last character */

	retval = 1;		    /* pattern found */

	/*
	 * Add character and/or line offset
	 */
	if (!(options & SEARCH_NOOF) || *str == ';')
	{
	    if (spats[0].off.line)	/* Add the offset to the line number. */
	    {
		c = pos.lnum + spats[0].off.off;
		if (c < 1)
		    pos.lnum = 1;
		else if (c > curbuf->b_ml.ml_line_count)
		    pos.lnum = curbuf->b_ml.ml_line_count;
		else
		    pos.lnum = c;
		pos.col = 0;

		retval = 2;	    /* pattern found, line offset added */
	    }
	    else
	    {
		/* to the right, check for end of file */
		if (spats[0].off.off > 0)
		{
		    for (c = spats[0].off.off; c; --c)
			if (incl(&pos) == -1)
			    break;
		}
		/* to the left, check for start of file */
		else
		{
		    if ((c = pos.col + spats[0].off.off) >= 0)
			pos.col = c;
		    else
			for (c = spats[0].off.off; c; ++c)
			    if (decl(&pos) == -1)
				break;
		}
	    }
	}

	/*
	 * The search command can be followed by a ';' to do another search.
	 * For example: "/pat/;/foo/+3;?bar"
	 * This is like doing another search command, except:
	 * - The remembered direction '/' or '?' is from the first search.
	 * - When an error happens the cursor isn't moved at all.
	 * Don't do this when called by get_address() (it handles ';' itself).
	 */
	if (!(options & SEARCH_OPT) || str == NULL || *str != ';')
	    break;

	dirc = *++str;
	if (dirc != '?' && dirc != '/')
	{
	    retval = 0;
	    EMSG("Expected '?' or '/'  after ';'");
	    goto end_do_search;
	}
	++str;
    }

    if (options & SEARCH_MARK)
	setpcmark();
    curwin->w_cursor = pos;
    curwin->w_set_curswant = TRUE;

end_do_search:
    if (options & SEARCH_KEEP)
	spats[0].off = old_off;
    return retval;
}

#if defined(INSERT_EXPAND) || defined(PROTO)
/*
 * search_for_exact_line(buf, pos, dir, pat)
 *
 * Search for a line starting with the given pattern (ignoring leading
 * white-space), starting from pos and going in direction dir.	pos will
 * contain the position of the match found.    Blank lines match only if
 * ADDING is set.  if p_ic is set then the pattern must be in lowercase.
 * Return OK for success, or FAIL if no line found.
 */
    int
search_for_exact_line(buf, pos, dir, pat)
    BUF		*buf;
    FPOS	*pos;
    int		dir;
    char_u	*pat;
{
    linenr_t	start = 0;
    char_u	*ptr;
    char_u	*p;

    if (buf->b_ml.ml_line_count == 0)
	return FAIL;
    for (;;)
    {
	pos->lnum += dir;
	if (pos->lnum < 1)
	{
	    if (p_ws)
	    {
		pos->lnum = buf->b_ml.ml_line_count;
		if (!shortmess(SHM_SEARCH))
		    give_warning(top_bot_msg, TRUE);
	    }
	    else
	    {
		pos->lnum = 1;
		break;
	    }
	}
	else if (pos->lnum > buf->b_ml.ml_line_count)
	{
	    if (p_ws)
	    {
		pos->lnum = 1;
		if (!shortmess(SHM_SEARCH))
		    give_warning(bot_top_msg, TRUE);
	    }
	    else
	    {
		pos->lnum = 1;
		break;
	    }
	}
	if (pos->lnum == start)
	    break;
	if (start == 0)
	    start = pos->lnum;
	ptr = ml_get_buf(buf, pos->lnum, FALSE);
	p = skipwhite(ptr);
	pos->col = p - ptr;

	/* when adding lines the matching line may be empty but it is not
	 * ignored because we are interested in the next line -- Acevedo */
	if ((continue_status & CONT_ADDING) && !(continue_status & CONT_SOL))
	{
	    if (p_ic)
	    {
		/*
		if (STRICMP(p, pat) == 0)
		*/
		for (ptr = pat; TO_LOWER(*p) == *ptr && *p; p++, ptr++)
		    ;
		if (*p == *ptr)	/* only possible if both NUL, exact match */
		    return OK;
	    }
	    else if (STRCMP(p, pat) == 0)
		return OK;
	}
	else if (*p)	/* ignore empty lines */
	{	/* expanding lines or words */
	    if (p_ic)
	    {
		/*
		if (STRNICMP(p, pat, completion_length) == 0)
		*/
		for (ptr = pat; TO_LOWER(*p) == *ptr && *p; p++, ptr++)
		    ;
		if (*ptr == NUL)
		    return OK;
	    }
	    else if (STRNCMP(p, pat, completion_length) == 0)
		return OK;
	}
    }
    return FAIL;
}
#endif /* INSERT_EXPAND */

/*
 * Character Searches
 */

/*
 * searchc(c, dir, type, count)
 *
 * Search for character 'c', in direction 'dir'. If 'type' is 0, move to the
 * position of the character, otherwise move to just before the char.
 * Repeat this 'count' times.
 */
    int
searchc(c, dir, type, count)
    int		    c;
    int		    dir;
    int		    type;
    long	    count;
{
    static int	    lastc = NUL;    /* last character searched for */
    static int	    lastcdir;	    /* last direction of character search */
    static int	    lastctype;	    /* last type of search ("find" or "to") */
    int		    col;
    char_u	    *p;
    int		    len;
#ifdef MULTI_BYTE
    int		    c2 = NUL;	    /* 2nd byte for DBCS */
    static int	    lastc2 = NUL;   /* 2nd last character searched */
    int		    char_bytes;	    /* 1: normal char, 2: DBCS char */
#endif

    if (c != NUL)	/* normal search: remember args for repeat */
    {
	if (!KeyStuffed)    /* don't remember when redoing */
	{
	    lastc = c;
	    lastcdir = dir;
	    lastctype = type;
#ifdef MULTI_BYTE
	    if (is_dbcs && IsLeadByte(c))
		lastc2 = c2 = (char_u)safe_vgetc();
#endif
	}
    }
    else		/* repeat previous search */
    {
	if (lastc == NUL)
	    return FALSE;
	if (dir)	/* repeat in opposite direction */
	    dir = -lastcdir;
	else
	    dir = lastcdir;
	type = lastctype;
	c = lastc;
#ifdef MULTI_BYTE
	c2 = lastc2;
#endif
    }

    p = ml_get_curline();
    col = curwin->w_cursor.col;
    len = STRLEN(p);

#ifdef MULTI_BYTE
    if (is_dbcs && IsLeadByte(c))
	char_bytes = 2;
    else
	char_bytes = 1;
#endif

    while (count--)
    {
	for (;;)
	{
#ifdef MULTI_BYTE
	    if (is_dbcs && dir > 0 && IsLeadByte(p[col]))
		++col;		/* advance two bytes for multibyte char */
#endif
	    if ((col += dir) < 0 || col >= len)
		return FALSE;
#ifdef MULTI_BYTE
	    if (is_dbcs && dir < 0 && IsTrailByte(p, &p[col]))
		continue;	/* skip multibyte's trail byte */

	    if (is_dbcs && char_bytes == 2)
	    {
		if (p[col] == c && p[col + 1] == c2)
		    break;
	    }
	    else
#endif
		if (p[col] == c)
		    break;
	}
    }
    if (type)
    {
	/* backup to before the character (possibly double-byte) */
	col -= dir;
#ifdef MULTI_BYTE
	if (is_dbcs
		&& ((dir < 0 && char_bytes == 2)
		    || (dir > 0 && IsTrailByte(p, &p[col]))))
	    col -= dir;
#endif
    }
    curwin->w_cursor.col = col;
    return TRUE;
}

/*
 * "Other" Searches
 */

/*
 * findmatch - find the matching paren or brace
 *
 * Improvement over vi: Braces inside quotes are ignored.
 */
    FPOS *
findmatch(oap, initc)
    OPARG   *oap;
    int	    initc;
{
    return findmatchlimit(oap, initc, 0, 0);
}

/*
 * findmatchlimit -- find the matching paren or brace, if it exists within
 * maxtravel lines of here.  A maxtravel of 0 means search until falling off
 * the edge of the file.
 *
 * "initc" is the character to find a match for.  NUL means to find the
 * character at or after the cursor.
 *
 * flags: FM_BACKWARD	search backwards (when initc is '/', '*' or '#')
 *	  FM_FORWARD	search forwards (when initc is '/', '*' or '#')
 *	  FM_BLOCKSTOP	stop at start/end of block ({ or } in column 0)
 *	  FM_SKIPCOMM	skip comments (not implemented yet!)
 */

    FPOS *
findmatchlimit(oap, initc, flags, maxtravel)
    OPARG   *oap;
    int	    initc;
    int	    flags;
    int	    maxtravel;
{
    static FPOS	    pos;		/* current search position */
    int		    findc = 0;		/* matching brace */
    int		    c;
    int		    count = 0;		/* cumulative number of braces */
    int		    backwards = FALSE;	/* init for gcc */
    int		    inquote = FALSE;	/* TRUE when inside quotes */
    char_u	    *linep;		/* pointer to current line */
    char_u	    *ptr;
    int		    do_quotes;		/* check for quotes in current line */
    int		    at_start;		/* do_quotes value at start position */
    int		    hash_dir = 0;	/* Direction searched for # things */
    int		    comment_dir = 0;	/* Direction searched for comments */
    FPOS	    match_pos;		/* Where last slash-star was found */
    int		    start_in_quotes;	/* start position is in quotes */
    int		    traveled = 0;	/* how far we've searched so far */
    int		    ignore_cend = FALSE;    /* ignore comment end */
    int		    cpo_match;		/* vi compatible matching */
    int		    dir;		/* Direction to search */
    int		    comment_col = MAXCOL;   /* start of / / comment */

    pos = curwin->w_cursor;
    linep = ml_get(pos.lnum);

    cpo_match = (vim_strchr(p_cpo, CPO_MATCH) != NULL);

    /* Direction to search when initc is '/', '*' or '#' */
    if (flags & FM_BACKWARD)
	dir = BACKWARD;
    else if (flags & FM_FORWARD)
	dir = FORWARD;
    else
	dir = 0;

    /*
     * if initc given, look in the table for the matching character
     * '/' and '*' are special cases: look for start or end of comment.
     * When '/' is used, we ignore running backwards into an star-slash, for
     * "[*" command, we just want to find any comment.
     */
    if (initc == '/' || initc == '*')
    {
	comment_dir = dir;
	if (initc == '/')
	    ignore_cend = TRUE;
	backwards = (dir == FORWARD) ? FALSE : TRUE;
	initc = NUL;
    }
    else if (initc != '#' && initc != NUL)
    {
	/* 'matchpairs' is "x:y,x:y" */
	for (ptr = curbuf->b_p_mps; *ptr; ptr += 2)
	{
	    if (*ptr == initc)
	    {
		findc = initc;
		initc = ptr[2];
		backwards = TRUE;
		break;
	    }
	    ptr += 2;
	    if (*ptr == initc)
	    {
		findc = initc;
		initc = ptr[-2];
		backwards = FALSE;
		break;
	    }
	}
	if (!findc)		/* invalid initc! */
	    return NULL;
    }
    /*
     * Either initc is '#', or no initc was given and we need to look under the
     * cursor.
     */
    else
    {
	if (initc == '#')
	{
	    hash_dir = dir;
	}
	else
	{
	    /*
	     * initc was not given, must look for something to match under
	     * or near the cursor.
	     * Only check for special things when 'cpo' doesn't have '%'.
	     */
	    if (!cpo_match)
	    {
		/* Are we before or at #if, #else etc.? */
		ptr = skipwhite(linep);
		if (*ptr == '#' && pos.col <= (colnr_t)(ptr - linep))
		{
		    ptr = skipwhite(ptr + 1);
		    if (   STRNCMP(ptr, "if", 2) == 0
			|| STRNCMP(ptr, "endif", 5) == 0
			|| STRNCMP(ptr, "el", 2) == 0)
			hash_dir = 1;
		}

		/* Are we on a comment? */
		else if (linep[pos.col] == '/')
		{
		    if (linep[pos.col + 1] == '*')
		    {
			comment_dir = FORWARD;
			backwards = FALSE;
			pos.col++;
		    }
		    else if (pos.col > 0 && linep[pos.col - 1] == '*')
		    {
			comment_dir = BACKWARD;
			backwards = TRUE;
			pos.col--;
		    }
		}
		else if (linep[pos.col] == '*')
		{
		    if (linep[pos.col + 1] == '/')
		    {
			comment_dir = BACKWARD;
			backwards = TRUE;
		    }
		    else if (pos.col > 0 && linep[pos.col - 1] == '/')
		    {
			comment_dir = FORWARD;
			backwards = FALSE;
		    }
		}
	    }

	    /*
	     * If we are not on a comment or the # at the start of a line, then
	     * look for brace anywhere on this line after the cursor.
	     */
	    if (!hash_dir && !comment_dir)
	    {
		/*
		 * Find the brace under or after the cursor.
		 * If beyond the end of the line, use the last character in
		 * the line.
		 */
		if (linep[pos.col] == NUL && pos.col)
		    --pos.col;
		for (;;)
		{
		    initc = linep[pos.col];
		    if (initc == NUL)
			break;

		    for (ptr = curbuf->b_p_mps; *ptr; ++ptr)
		    {
			if (*ptr == initc)
			{
			    findc = ptr[2];
			    backwards = FALSE;
			    break;
			}
			ptr += 2;
			if (*ptr == initc)
			{
			    findc = ptr[-2];
			    backwards = TRUE;
			    break;
			}
			if (!*++ptr)
			    break;
		    }
		    if (findc)
			break;
		    ++pos.col;
		}
		if (!findc)
		{
		    /* no brace in the line, maybe use "  #if" then */
		    if (!cpo_match && *skipwhite(linep) == '#')
			hash_dir = 1;
		    else
			return NULL;
		}
	    }
	}
	if (hash_dir)
	{
	    /*
	     * Look for matching #if, #else, #elif, or #endif
	     */
	    if (oap != NULL)
		oap->motion_type = MLINE;   /* Linewise for this case only */
	    if (initc != '#')
	    {
		ptr = skipwhite(skipwhite(linep) + 1);
		if (STRNCMP(ptr, "if", 2) == 0 || STRNCMP(ptr, "el", 2) == 0)
		    hash_dir = 1;
		else if (STRNCMP(ptr, "endif", 5) == 0)
		    hash_dir = -1;
		else
		    return NULL;
	    }
	    pos.col = 0;
	    while (!got_int)
	    {
		if (hash_dir > 0)
		{
		    if (pos.lnum == curbuf->b_ml.ml_line_count)
			break;
		}
		else if (pos.lnum == 1)
		    break;
		pos.lnum += hash_dir;
		linep = ml_get(pos.lnum);
		line_breakcheck();	/* check for CTRL-C typed */
		ptr = skipwhite(linep);
		if (*ptr != '#')
		    continue;
		pos.col = ptr - linep;
		ptr = skipwhite(ptr + 1);
		if (hash_dir > 0)
		{
		    if (STRNCMP(ptr, "if", 2) == 0)
			count++;
		    else if (STRNCMP(ptr, "el", 2) == 0)
		    {
			if (count == 0)
			    return &pos;
		    }
		    else if (STRNCMP(ptr, "endif", 5) == 0)
		    {
			if (count == 0)
			    return &pos;
			count--;
		    }
		}
		else
		{
		    if (STRNCMP(ptr, "if", 2) == 0)
		    {
			if (count == 0)
			    return &pos;
			count--;
		    }
		    else if (initc == '#' && STRNCMP(ptr, "el", 2) == 0)
		    {
			if (count == 0)
			    return &pos;
		    }
		    else if (STRNCMP(ptr, "endif", 5) == 0)
			count++;
		}
	    }
	    return NULL;
	}
    }

#ifdef RIGHTLEFT
    if (curwin->w_p_rl)
	backwards = !backwards;
#endif

    do_quotes = -1;
    start_in_quotes = MAYBE;
    /* backward search: Check if this line contains a single-line comment */
    if (backwards && comment_dir)
	comment_col = check_linecomment(linep);
    while (!got_int)
    {
	/*
	 * Go to the next position, forward or backward. We could use
	 * inc() and dec() here, but that is much slower
	 */
	if (backwards)
	{
	    if (pos.col == 0)		/* at start of line, go to prev. one */
	    {
		if (pos.lnum == 1)	/* start of file */
		    break;
		--pos.lnum;

		if (maxtravel && traveled++ > maxtravel)
		    break;

		linep = ml_get(pos.lnum);
		pos.col = STRLEN(linep);    /* put pos.col on trailing NUL */
		do_quotes = -1;
		line_breakcheck();

		/* Check if this line contains a single-line comment */
		if (comment_dir)
		    comment_col = check_linecomment(linep);
	    }
	    else
	    {
		--pos.col;
#ifdef MULTI_BYTE
		if (is_dbcs && IsTrailByte(linep, linep + pos.col))
		    --pos.col;
#endif
	    }
	}
	else				/* forward search */
	{
	    if (linep[pos.col] == NUL)	/* at end of line, go to next one */
	    {
		if (pos.lnum == curbuf->b_ml.ml_line_count) /* end of file */
		    break;
		++pos.lnum;

		if (maxtravel && traveled++ > maxtravel)
		    break;

		linep = ml_get(pos.lnum);
		pos.col = 0;
		do_quotes = -1;
		line_breakcheck();
	    }
	    else
	    {
#ifdef MULTI_BYTE
		if (is_dbcs && IsLeadByte(linep[pos.col])
						 && linep[pos.col + 1] != NUL)
		    ++pos.col;
#endif
		++pos.col;
	    }
	}

	/*
	 * If FM_BLOCKSTOP given, stop at a '{' or '}' in column 0.
	 */
	if (pos.col == 0 && (flags & FM_BLOCKSTOP) &&
					 (linep[0] == '{' || linep[0] == '}'))
	{
	    if (linep[0] == findc && count == 0)	/* match! */
		return &pos;
	    break;					/* out of scope */
	}

	if (comment_dir)
	{
	    /* Note: comments do not nest, and we ignore quotes in them */
	    /* TODO: ignore comment brackets inside strings */
	    if (comment_dir == FORWARD)
	    {
		if (linep[pos.col] == '*' && linep[pos.col + 1] == '/')
		{
		    pos.col++;
		    return &pos;
		}
	    }
	    else    /* Searching backwards */
	    {
		/*
		 * A comment may contain / * or / /, it may also start or end
		 * with / * /.	Ignore a / * after / /.
		 */
		if (pos.col == 0)
		    continue;
		else if (  linep[pos.col - 1] == '/'
			&& linep[pos.col] == '*'
			&& (int)pos.col < comment_col)
		{
		    count++;
		    match_pos = pos;
		    match_pos.col--;
		}
		else if (linep[pos.col - 1] == '*' && linep[pos.col] == '/')
		{
		    if (count > 0)
			pos = match_pos;
		    else if (pos.col > 1 && linep[pos.col - 2] == '/')
			pos.col -= 2;
		    else if (ignore_cend)
			continue;
		    else
			return NULL;
		    return &pos;
		}
	    }
	    continue;
	}

	/*
	 * If smart matching ('cpoptions' does not contain '%'), braces inside
	 * of quotes are ignored, but only if there is an even number of
	 * quotes in the line.
	 */
	if (cpo_match)
	    do_quotes = 0;
	else if (do_quotes == -1)
	{
	    /*
	     * count the number of quotes in the line, skipping \" and '"'
	     */
	    at_start = do_quotes;
	    for (ptr = linep; *ptr; ++ptr)
	    {
		if (ptr == linep + pos.col + backwards)
		    at_start = (do_quotes & 1);
		if (*ptr == '"' && (ptr == linep || ptr[-1] != '\\') &&
			    (ptr == linep || ptr[-1] != '\'' || ptr[1] != '\''))
		    ++do_quotes;
	    }
	    do_quotes &= 1;	    /* result is 1 with even number of quotes */

	    /*
	     * If we find an uneven count, check current line and previous
	     * one for a '\' at the end.
	     */
	    if (!do_quotes)
	    {
		inquote = FALSE;
		if (ptr[-1] == '\\')
		{
		    do_quotes = 1;
		    if (start_in_quotes == MAYBE)
		    {
			inquote = !at_start;
			if (inquote)
			    start_in_quotes = TRUE;
		    }
		    else if (backwards)
			inquote = TRUE;
		}
		if (pos.lnum > 1)
		{
		    ptr = ml_get(pos.lnum - 1);
		    if (*ptr && *(ptr + STRLEN(ptr) - 1) == '\\')
		    {
			do_quotes = 1;
			if (start_in_quotes == MAYBE)
			{
			    inquote = at_start;
			    if (inquote)
				start_in_quotes = TRUE;
			}
			else if (!backwards)
			    inquote = TRUE;
		    }
		}
	    }
	}
	if (start_in_quotes == MAYBE)
	    start_in_quotes = FALSE;

	/*
	 * If 'smartmatch' is set:
	 *   Things inside quotes are ignored by setting 'inquote'.  If we
	 *   find a quote without a preceding '\' invert 'inquote'.  At the
	 *   end of a line not ending in '\' we reset 'inquote'.
	 *
	 *   In lines with an uneven number of quotes (without preceding '\')
	 *   we do not know which part to ignore. Therefore we only set
	 *   inquote if the number of quotes in a line is even, unless this
	 *   line or the previous one ends in a '\'.  Complicated, isn't it?
	 */
	switch (c = linep[pos.col])
	{
	case NUL:
		/* at end of line without trailing backslash, reset inquote */
	    if (pos.col == 0 || linep[pos.col - 1] != '\\')
	    {
		inquote = FALSE;
		start_in_quotes = FALSE;
	    }
	    break;

	case '"':
		/* a quote that is preceded with a backslash is ignored */
	    if (do_quotes && (pos.col == 0 || linep[pos.col - 1] != '\\'))
	    {
		inquote = !inquote;
		start_in_quotes = FALSE;
	    }
	    break;

	/*
	 * If smart matching ('cpoptions' does not contain '%'):
	 *   Skip things in single quotes: 'x' or '\x'.  Be careful for single
	 *   single quotes, eg jon's.  Things like '\233' or '\x3f' are not
	 *   skipped, there is never a brace in them.
	 *   Ignore this when finding matches for `'.
	 */
	case '\'':
	    if (!cpo_match && initc != '\'' && findc != '\'')
	    {
		if (backwards)
		{
		    if (pos.col > 1)
		    {
			if (linep[pos.col - 2] == '\'')
			{
			    pos.col -= 2;
			    break;
			}
			else if (linep[pos.col - 2] == '\\' &&
				    pos.col > 2 && linep[pos.col - 3] == '\'')
			{
			    pos.col -= 3;
			    break;
			}
		    }
		}
		else if (linep[pos.col + 1])	/* forward search */
		{
		    if (linep[pos.col + 1] == '\\' &&
			    linep[pos.col + 2] && linep[pos.col + 3] == '\'')
		    {
			pos.col += 3;
			break;
		    }
		    else if (linep[pos.col + 2] == '\'')
		    {
			pos.col += 2;
			break;
		    }
		}
	    }
	    /* FALLTHROUGH */

	default:
	    /* Check for match outside of quotes, and inside of
	     * quotes when the start is also inside of quotes */
	    if (!inquote || start_in_quotes == TRUE)
	    {
		if (c == initc)
		    count++;
		else if (c == findc)
		{
		    if (count == 0)
			return &pos;
		    count--;
		}
	    }
	}
    }

    if (comment_dir == BACKWARD && count > 0)
    {
	pos = match_pos;
	return &pos;
    }
    return (FPOS *)NULL;	/* never found it */
}

/*
 * Check if line[] contains a / / comment.
 * Return MAXCOL if not, otherwise return the column.
 * TODO: skip strings.
 */
    static int
check_linecomment(line)
    char_u	*line;
{
    char_u  *p;

    p = line;
    while ((p = vim_strchr(p, '/')) != NULL)
    {
	if (p[1] == '/')
	    break;
	++p;
    }

    if (p == NULL)
	return MAXCOL;
    return (int)(p - line);
}

/*
 * Move cursor briefly to character matching the one under the cursor.
 * Show the match only if it is visible on the screen.
 * If there isn't a match, then beep.
 */
    void
showmatch()
{
    FPOS	   *lpos, save_cursor;
    FPOS	    mpos;
    colnr_t	    vcol;
    long	    save_so;
#ifdef CURSOR_SHAPE
    int		    save_state;
#endif

    if ((lpos = findmatch(NULL, NUL)) == NULL)	    /* no match, so beep */
	vim_beep();
    else if (lpos->lnum >= curwin->w_topline)
    {
	if (!curwin->w_p_wrap)
	    getvcol(curwin, lpos, NULL, &vcol, NULL);
	if (curwin->w_p_wrap || (vcol >= curwin->w_leftcol &&
					  vcol < curwin->w_leftcol + Columns))
	{
	    mpos = *lpos;    /* save the pos, update_screen() may change it */
	    update_screen(VALID_TO_CURSCHAR); /* show the new char first */
	    save_cursor = curwin->w_cursor;
	    save_so = p_so;

#ifdef CURSOR_SHAPE
	    save_state = State;
	    State = SHOWMATCH;
	    ui_cursor_shape();		/* may show different cursor shape */
#endif
	    curwin->w_cursor = mpos;	/* move to matching char */
	    p_so = 0;			/* don't use 'scrolloff' here */
	    showruler(FALSE);
	    setcursor();
	    cursor_on();		/* make sure that the cursor is shown */
	    out_flush();

	    /*
	     * brief pause, unless 'm' is present in 'cpo' and a character is
	     * available.
	     */
	    if (vim_strchr(p_cpo, CPO_SHOWMATCH) != NULL)
		ui_delay(p_mat * 100L, TRUE);
	    else if (!char_avail())
		ui_delay(p_mat * 100L, FALSE);
	    curwin->w_cursor = save_cursor;	/* restore cursor position */
	    p_so = save_so;
#ifdef CURSOR_SHAPE
	    State = save_state;
	    ui_cursor_shape();		/* may show different cursor shape */
#endif
	}
    }
}

/*
 * findsent(dir, count) - Find the start of the next sentence in direction
 * 'dir' Sentences are supposed to end in ".", "!" or "?" followed by white
 * space or a line break. Also stop at an empty line.
 * Return OK if the next sentence was found.
 */
    int
findsent(dir, count)
    int	    dir;
    long    count;
{
    FPOS	    pos, tpos;
    int		    c;
    int		    (*func) __ARGS((FPOS *));
    int		    startlnum;
    int		    noskip = FALSE;	    /* do not skip blanks */
    int		    cpo_J;

    pos = curwin->w_cursor;
    if (dir == FORWARD)
	func = incl;
    else
	func = decl;

    while (count--)
    {
	/*
	 * if on an empty line, skip upto a non-empty line
	 */
	if (gchar_pos(&pos) == NUL)
	{
	    do
		if ((*func)(&pos) == -1)
		    break;
	    while (gchar_pos(&pos) == NUL);
	    if (dir == FORWARD)
		goto found;
	}
	/*
	 * if on the start of a paragraph or a section and searching forward,
	 * go to the next line
	 */
	else if (dir == FORWARD && pos.col == 0 &&
						startPS(pos.lnum, NUL, FALSE))
	{
	    if (pos.lnum == curbuf->b_ml.ml_line_count)
		return FAIL;
	    ++pos.lnum;
	    goto found;
	}
	else if (dir == BACKWARD)
	    decl(&pos);

	/* go back to the previous non-blank char */
	while ((c = gchar_pos(&pos)) == ' ' || c == '\t' ||
	     (dir == BACKWARD && vim_strchr((char_u *)".!?)]\"'", c) != NULL))
	{
	    if (decl(&pos) == -1)
		break;
	    /* when going forward: Stop in front of empty line */
	    if (lineempty(pos.lnum) && dir == FORWARD)
	    {
		incl(&pos);
		goto found;
	    }
	}

	/* remember the line where the search started */
	startlnum = pos.lnum;
	cpo_J = vim_strchr(p_cpo, CPO_ENDOFSENT) != NULL;

	for (;;)		/* find end of sentence */
	{
	    c = gchar_pos(&pos);
	    if (c == NUL || (pos.col == 0 && startPS(pos.lnum, NUL, FALSE)))
	    {
		if (dir == BACKWARD && pos.lnum != startlnum)
		    ++pos.lnum;
		break;
	    }
	    if (c == '.' || c == '!' || c == '?')
	    {
		tpos = pos;
		do
		    if ((c = inc(&tpos)) == -1)
			break;
		while (vim_strchr((char_u *)")]\"'", c = gchar_pos(&tpos))
			!= NULL);
		if (c == -1  || (!cpo_J && (c == ' ' || c == '\t')) || c == NUL
		    || (cpo_J && (c == ' ' && inc(&tpos) >= 0
			      && gchar_pos(&tpos) == ' ')))
		{
		    pos = tpos;
		    if (gchar_pos(&pos) == NUL) /* skip NUL at EOL */
			inc(&pos);
		    break;
		}
	    }
	    if ((*func)(&pos) == -1)
	    {
		if (count)
		    return FAIL;
		noskip = TRUE;
		break;
	    }
	}
found:
	    /* skip white space */
	while (!noskip && ((c = gchar_pos(&pos)) == ' ' || c == '\t'))
	    if (incl(&pos) == -1)
		break;
    }

    setpcmark();
    curwin->w_cursor = pos;
    return OK;
}

/*
 * findpar(dir, count, what) - Find the next paragraph in direction 'dir'
 * Paragraphs are currently supposed to be separated by empty lines.
 * Return TRUE if the next paragraph was found.
 * If 'what' is '{' or '}' we go to the next section.
 * If 'both' is TRUE also stop at '}'.
 */
    int
findpar(oap, dir, count, what, both)
    OPARG	    *oap;
    int		    dir;
    long	    count;
    int		    what;
    int		    both;
{
    linenr_t	curr;
    int		did_skip;   /* TRUE after separating lines have been skipped */
    int		first;	    /* TRUE on first line */

    curr = curwin->w_cursor.lnum;

    while (count--)
    {
	did_skip = FALSE;
	for (first = TRUE; ; first = FALSE)
	{
	    if (*ml_get(curr) != NUL)
		did_skip = TRUE;

	    if (!first && did_skip && startPS(curr, what, both))
		break;

	    if ((curr += dir) < 1 || curr > curbuf->b_ml.ml_line_count)
	    {
		if (count)
		    return FALSE;
		curr -= dir;
		break;
	    }
	}
    }
    setpcmark();
    if (both && *ml_get(curr) == '}')	/* include line with '}' */
	++curr;
    curwin->w_cursor.lnum = curr;
    if (curr == curbuf->b_ml.ml_line_count)
    {
	if ((curwin->w_cursor.col = STRLEN(ml_get(curr))) != 0)
	{
	    --curwin->w_cursor.col;
	    oap->inclusive = TRUE;
	}
    }
    else
	curwin->w_cursor.col = 0;
    return TRUE;
}

/*
 * check if the string 's' is a nroff macro that is in option 'opt'
 */
    static int
inmacro(opt, s)
    char_u	*opt;
    char_u	*s;
{
    char_u	*macro;

    for (macro = opt; macro[0]; ++macro)
    {
	if (macro[0] == s[0] && (((s[1] == NUL || s[1] == ' ') &&
		   (macro[1] == NUL || macro[1] == ' ')) || macro[1] == s[1]))
	    break;
	++macro;
	if (macro[0] == NUL)
	    break;
    }
    return (macro[0] != NUL);
}

/*
 * startPS: return TRUE if line 'lnum' is the start of a section or paragraph.
 * If 'para' is '{' or '}' only check for sections.
 * If 'both' is TRUE also stop at '}'
 */
    int
startPS(lnum, para, both)
    linenr_t	lnum;
    int		para;
    int		both;
{
    char_u	*s;

    s = ml_get(lnum);
    if (*s == para || *s == '\f' || (both && *s == '}'))
	return TRUE;
    if (*s == '.' && (inmacro(p_sections, s + 1) ||
					   (!para && inmacro(p_para, s + 1))))
	return TRUE;
    return FALSE;
}

/*
 * The following routines do the word searches performed by the 'w', 'W',
 * 'b', 'B', 'e', and 'E' commands.
 */

/*
 * To perform these searches, characters are placed into one of three
 * classes, and transitions between classes determine word boundaries.
 *
 * The classes are:
 *
 * 0 - white space
 * 1 - keyword charactes (letters, digits and underscore)
 * 2 - everything else
 */

static int	stype;		/* type of the word motion being performed */

/*
 * cls() - returns the class of character at curwin->w_cursor
 *
 * The 'type' of the current search modifies the classes of characters if a
 * 'W', 'B', or 'E' motion is being done. In this case, chars. from class 2
 * are reported as class 1 since only white space boundaries are of interest.
 */
    static int
cls()
{
    int	    c;

    c = gchar_cursor();
#ifdef FKMAP	/* when 'akm' (Farsi mode), take care of Farsi blank */
    if (p_altkeymap && c == F_BLANK)
	return 0;
#endif
    if (c == ' ' || c == '\t' || c == NUL)
	return 0;
#ifdef MULTI_BYTE
    if (is_dbcs && c > 0xff)
    {
	/* process code leading/trailing bytes */
	unsigned char pcode_lb = c & 0xff;
	unsigned char pcode_tb = (unsigned)c >> 8;

	if (is_dbcs == (int)DBCS_JPN)
	{
	    /* JIS code classification */
	    unsigned char lb;
	    unsigned char tb;

	    lb = pcode_lb;
	    tb = pcode_tb;
	    /* convert process code to JIS */
# if defined(WIN32) || defined(macintosh)
	    /* process code is SJIS */
	    if (lb <= 0x9f)
		lb = (lb - 0x81) * 2 + 0x21;
	    else
		lb = (lb - 0xc1) * 2 + 0x21;
	    if (tb <= 0x7e)
		tb -= 0x1f;
	    else if (tb <= 0x9e)
		tb -= 0x20;
	    else
	    {
		tb -= 0x7e;
		lb += 1;
	    }
# else
	    /*
	     * XXX:  Code page identification can not use with all
	     *       system! So, some other encoding information
	     *       will be needed.
	     *       In japanese: SJIS,EUC,UNICODE,(JIS)
	     *	     Note that JIS-code system don't use as
	     *       process code in most system because it uses
	     *       escape sequences(JIS is context depend encoding).
	     */
	    /* assume process code is JAPANESE-EUC */
	    lb &= 0x7f;
	    tb &= 0x7f;
# endif
	    /* exceptions */
	    switch (lb << 8 | tb)
	    {
		case 0x213c: /* prolongedsound handled as KATAKANA */
		    return 13;
	    }
	    /* sieved by KU code */
	    switch (lb)
	    {
		case 0x21:
		case 0x22:
		    /* special symbols */
		    return 10;
		case 0x23:
		    /* alpha-numeric */
		    return 11;
		case 0x24:
		    /* hiragana */
		    return 12;
		case 0x25:
		    /* katakana */
		    return 13;
		case 0x26:
		    /* greek */
		    return 14;
		case 0x27:
		    /* russian */
		    return 15;
		case 0x28:
		    /* lines */
		    return 16;
		default:
		    /* kanji */
		    return 17;
	    }
	}
	return 3;
    }
#endif

    if (vim_iswordc(c))
	return 1;

    /*
     * If stype is non-zero, report these as class 1.
     */
    return (stype == 0) ? 2 : 1;
}


/*
 * fwd_word(count, type, eol) - move forward one word
 *
 * Returns FAIL if the cursor was already at the end of the file.
 * If eol is TRUE, last word stops at end of line (for operators).
 */
    int
fwd_word(count, type, eol)
    long	count;
    int		type;
    int		eol;
{
    int		sclass;	    /* starting class */
    int		i;
    int		last_line;

    stype = type;
    while (--count >= 0)
    {
	sclass = cls();

	/*
	 * We always move at least one character, unless on the last character
	 * in the buffer.
	 */
	last_line = (curwin->w_cursor.lnum == curbuf->b_ml.ml_line_count);
	i = inc_cursor();
	if (i == -1 || (i == 1 && last_line)) /* started at last char in file */
	    return FAIL;
	if (i == 1 && eol && count == 0)      /* started at last char in line */
	    return OK;

	/*
	 * Go one char past end of current word (if any)
	 */
	if (sclass != 0)
	    while (cls() == sclass)
	    {
		i = inc_cursor();
		if (i == -1 || (i == 1 && eol && count == 0))
		    return OK;
	    }

	/*
	 * go to next non-white
	 */
	while (cls() == 0)
	{
	    /*
	     * We'll stop if we land on a blank line
	     */
	    if (curwin->w_cursor.col == 0 && *ml_get_curline() == NUL)
		break;

	    i = inc_cursor();
	    if (i == -1 || (i == 1 && eol && count == 0))
		return OK;
	}
    }
    return OK;
}

/*
 * bck_word() - move backward 'count' words
 *
 * If stop is TRUE and we are already on the start of a word, move one less.
 *
 * Returns FAIL if top of the file was reached.
 */
    int
bck_word(count, type, stop)
    long	count;
    int		type;
    int		stop;
{
    int		sclass;	    /* starting class */

    stype = type;
    while (--count >= 0)
    {
	sclass = cls();
	if (dec_cursor() == -1)     /* started at start of file */
	    return FAIL;

	if (!stop || sclass == cls() || sclass == 0)
	{
	    /*
	     * Skip white space before the word.
	     * Stop on an empty line.
	     */
	    while (cls() == 0)
	    {
		if (curwin->w_cursor.col == 0 &&
					     lineempty(curwin->w_cursor.lnum))
		    goto finished;
		if (dec_cursor() == -1)	     /* hit start of file, stop here */
		    return OK;
	    }

	    /*
	     * Move backward to start of this word.
	     */
	    if (skip_chars(cls(), BACKWARD))
		return OK;
	}

	inc_cursor();			 /* overshot - forward one */
finished:
	stop = FALSE;
    }
    return OK;
}

/*
 * end_word() - move to the end of the word
 *
 * There is an apparent bug in the 'e' motion of the real vi. At least on the
 * System V Release 3 version for the 80386. Unlike 'b' and 'w', the 'e'
 * motion crosses blank lines. When the real vi crosses a blank line in an
 * 'e' motion, the cursor is placed on the FIRST character of the next
 * non-blank line. The 'E' command, however, works correctly. Since this
 * appears to be a bug, I have not duplicated it here.
 *
 * Returns FAIL if end of the file was reached.
 *
 * If stop is TRUE and we are already on the end of a word, move one less.
 * If empty is TRUE stop on an empty line.
 */
    int
end_word(count, type, stop, empty)
    long	count;
    int		type;
    int		stop;
    int		empty;
{
    int		sclass;	    /* starting class */

    stype = type;
    while (--count >= 0)
    {
	sclass = cls();
	if (inc_cursor() == -1)
	    return FAIL;

	/*
	 * If we're in the middle of a word, we just have to move to the end
	 * of it.
	 */
	if (cls() == sclass && sclass != 0)
	{
	    /*
	     * Move forward to end of the current word
	     */
	    if (skip_chars(sclass, FORWARD))
		return FAIL;
	}
	else if (!stop || sclass == 0)
	{
	    /*
	     * We were at the end of a word. Go to the end of the next word.
	     * First skip white space, if 'empty' is TRUE, stop at empty line.
	     */
	    while (cls() == 0)
	    {
		if (empty && curwin->w_cursor.col == 0 &&
					     lineempty(curwin->w_cursor.lnum))
		    goto finished;
		if (inc_cursor() == -1)	    /* hit end of file, stop here */
		    return FAIL;
	    }

	    /*
	     * Move forward to the end of this word.
	     */
	    if (skip_chars(cls(), FORWARD))
		return FAIL;
	}
	dec_cursor();			/* overshot - one char backward */
finished:
	stop = FALSE;			/* we move only one word less */
    }
    return OK;
}

/*
 * bckend_word(count, type) - move back to the end of the word
 *
 * If 'eol' is TRUE, stop at end of line.
 *
 * Returns FAIL if start of the file was reached.
 */
    int
bckend_word(count, type, eol)
    long	count;
    int		type;
    int		eol;
{
    int		sclass;	    /* starting class */
    int		i;

    stype = type;
    while (--count >= 0)
    {
	sclass = cls();
	if ((i = dec_cursor()) == -1)
	    return FAIL;
	if (eol && i == 1)
	    return OK;

	/*
	 * Move backward to before the start of this word.
	 */
	if (sclass != 0)
	{
	    while (cls() == sclass)
		if ((i = dec_cursor()) == -1 || (eol && i == 1))
		    return OK;
	}

	/*
	 * Move backward to end of the previous word
	 */
	while (cls() == 0)
	{
	    if (curwin->w_cursor.col == 0 && lineempty(curwin->w_cursor.lnum))
		break;
	    if ((i = dec_cursor()) == -1 || (eol && i == 1))
		return OK;
	}
    }
    return OK;
}

/*
 * Skip a row of characters of the same class.
 * Return TRUE when end-of-file reached, FALSE otherwise.
 */
    static int
skip_chars(cclass, dir)
    int	    cclass;
    int	    dir;
{
    while (cls() == cclass)
	if ((dir == FORWARD ? inc_cursor() : dec_cursor()) == -1)
	    return TRUE;
    return FALSE;
}

#ifdef TEXT_OBJECTS
/*
 * Go back to the start of the word or the start of white space
 */
    static void
back_in_line()
{
    int		sclass;		    /* starting class */

    sclass = cls();
    for (;;)
    {
	if (curwin->w_cursor.col == 0)	    /* stop at start of line */
	    break;
	--curwin->w_cursor.col;
	if (cls() != sclass)		    /* stop at start of word */
	{
	    ++curwin->w_cursor.col;
	    break;
	}
    }
}

    static void
find_first_blank(posp)
    FPOS    *posp;
{
    int	    c;

    while (decl(posp) != -1)
    {
	c = gchar_pos(posp);
	if (!vim_iswhite(c))
	{
	    incl(posp);
	    break;
	}
    }
}

/*
 * Skip count/2 sentences and count/2 separating white spaces.
 */
    static void
findsent_forward(count, at_start_sent)
    long    count;
    int	    at_start_sent;	/* cursor is at start of sentence */
{
    while (count--)
    {
	findsent(FORWARD, 1L);
	if (at_start_sent)
	    find_first_blank(&curwin->w_cursor);
	if (count == 0 || at_start_sent)
	    decl(&curwin->w_cursor);
	at_start_sent = !at_start_sent;
    }
}

/*
 * Find word under cursor, cursor at end.
 * Used while an operator is pending, and in Visual mode.
 */
    int
current_word(oap, count, include, type)
    OPARG	*oap;
    long	count;
    int		include;    /* TRUE: include word and white space */
    int		type;	    /* FALSE == word, TRUE == WORD */
{
    FPOS	start_pos;
    FPOS	pos;
    int		inclusive = TRUE;

    stype = type;

    /* Correct cursor when 'selection' is exclusive */
    if (VIsual_active && *p_sel == 'e' && lt(VIsual, curwin->w_cursor))
	dec_cursor();

    /*
     * When Visual mode is not active, or when the VIsual area is only one
     * character, select the word and/or white space under the cursor.
     */
    if (!VIsual_active || equal(curwin->w_cursor, VIsual))
    {
	/*
	 * Go to start of current word or white space.
	 */
	back_in_line();
	start_pos = curwin->w_cursor;

	/*
	 * If the start is on white space, and white space should be included
	 * ("	word"), or start is not on white space, and white space should
	 * not be included ("word"), find end of word.
	 */
	if ((cls() == 0) == include)
	{
	    if (end_word(1L, type, TRUE, TRUE) == FAIL)
		return FAIL;
	}
	else
	{
	    /*
	     * If the start is not on white space, and white space should be
	     * included ("word	 "), or start is on white space and white
	     * space should not be included ("	 "), find start of word.
	     */
	    if (fwd_word(1L, type, TRUE) == FAIL)
		return FAIL;
	    /*
	     * If end is just past a new-line, we don't want to include the
	     * first character on the line
	     */
	    if (oneleft() == FAIL)	/* put cursor on last char of area */
		inclusive = FALSE;
	    else if (include)
	    {
		/*
		 * If we don't include white space at the end, move the start
		 * to include some white space there. This makes "daw" work
		 * better on the last word in a sentence. Don't delete white
		 * space at start of line (indent).
		 */
		if (cls() != 0)
		{
		    pos = curwin->w_cursor;	/* save cursor position */
		    curwin->w_cursor = start_pos;
		    if (oneleft() == OK)
		    {
			back_in_line();
			if (cls() == 0 && curwin->w_cursor.col > 0)
			    start_pos = curwin->w_cursor;
		    }
		    curwin->w_cursor = pos;	/* put cursor back at end */
		}
	    }
	}

	if (VIsual_active)
	{
	    /* should do something when inclusive == FALSE ! */
	    VIsual = start_pos;
	    VIsual_mode = 'v';
	    update_curbuf(NOT_VALID);	    /* update the inversion */
	}
	else
	{
	    oap->start = start_pos;
	    oap->motion_type = MCHAR;
	}
	--count;
    }

    /*
     * When count is still > 0, extend with more objects.
     */
    while (count > 0)
    {
	inclusive = TRUE;
	if (VIsual_active && lt(curwin->w_cursor, VIsual))
	{
	    /*
	     * In Visual mode, with cursor at start: move cursor back.
	     */
	    if (decl(&curwin->w_cursor) == -1)
		return FAIL;
	    if (include != (cls() != 0))
	    {
		if (bck_word(1L, type, TRUE) == FAIL)
		    return FAIL;
	    }
	    else
	    {
		if (bckend_word(1L, type, TRUE) == FAIL)
		    return FAIL;
		(void)incl(&curwin->w_cursor);
	    }
	}
	else
	{
	    /*
	     * Move cursor forward one word and/or white area.
	     */
	    if (incl(&curwin->w_cursor) == -1)
		return FAIL;
	    if (include != (cls() == 0))
	    {
		if (fwd_word(1L, type, TRUE) == FAIL)
		    return FAIL;
		/*
		 * If end is just past a new-line, we don't want to include
		 * the first character on the line
		 */
		if (oneleft() == FAIL)	/* put cursor on last char of white */
		    inclusive = FALSE;
	    }
	    else
	    {
		if (end_word(1L, type, TRUE, TRUE) == FAIL)
		    return FAIL;
	    }
	}
	--count;
    }
    if (VIsual_active)
    {
	if (*p_sel == 'e' && inclusive && lt(VIsual, curwin->w_cursor))
	    inc_cursor();
    }
    else
	oap->inclusive = inclusive;

    return OK;
}

/*
 * Find sentence(s) under the cursor, cursor at end.
 * When Visual active, extend it by one or more sentences.
 */
    int
current_sent(oap, count, include)
    OPARG   *oap;
    long    count;
    int	    include;
{
    FPOS    start_pos;
    FPOS    pos;
    int	    start_blank;
    int	    c;
    int	    at_start_sent;
    long    ncount;

    start_pos = curwin->w_cursor;
    pos = start_pos;
    findsent(FORWARD, 1L);	/* Find start of next sentence. */

    /*
     * When visual area is bigger than one character: Extend it.
     */
    if (VIsual_active && !equal(start_pos, VIsual))
    {
extend:
	if (lt(start_pos, VIsual))
	{
	    /*
	     * Cursor at start of Visual area.
	     * Find out where we are:
	     * - in the white space before a sentence
	     * - in a sentence or just after it
	     * - at the start of a sentence
	     */
	    at_start_sent = TRUE;
	    decl(&pos);
	    while (lt(pos, curwin->w_cursor))
	    {
		c = gchar_pos(&pos);
		if (!vim_iswhite(c))
		{
		    at_start_sent = FALSE;
		    break;
		}
		incl(&pos);
	    }
	    if (!at_start_sent)
	    {
		findsent(BACKWARD, 1L);
		if (equal(curwin->w_cursor, start_pos))
		    at_start_sent = TRUE;  /* exactly at start of sentence */
		else
		    /* inside a sentence, go to its end (start of next) */
		    findsent(FORWARD, 1L);
	    }
	    if (include)	/* "as" gets twice as much as "is" */
		count *= 2;
	    while (count--)
	    {
		if (at_start_sent)
		    find_first_blank(&curwin->w_cursor);
		c = gchar_cursor();
		if (!at_start_sent || (!include && !vim_iswhite(c)))
		    findsent(BACKWARD, 1L);
		at_start_sent = !at_start_sent;
	    }
	}
	else
	{
	    /*
	     * Cursor at end of Visual area.
	     * Find out where we are:
	     * - just before a sentence
	     * - just before or in the white space before a sentence
	     * - in a sentence
	     */
	    incl(&pos);
	    at_start_sent = TRUE;
	    if (!equal(pos, curwin->w_cursor))	/* not just before a sentence */
	    {
		at_start_sent = FALSE;
		while (lt(pos, curwin->w_cursor))
		{
		    c = gchar_pos(&pos);
		    if (!vim_iswhite(c))
		    {
			at_start_sent = TRUE;
			break;
		    }
		    incl(&pos);
		}
		if (at_start_sent)	/* in the sentence */
		    findsent(BACKWARD, 1L);
		else		/* in/before white before a sentence */
		    curwin->w_cursor = start_pos;
	    }

	    if (include)	/* "as" gets twice as much as "is" */
		count *= 2;
	    findsent_forward(count, at_start_sent);
	}
	return OK;
    }

    /*
     * If cursor started on blank, check if it is just before the start of the
     * next sentence.
     */
    while (c = gchar_pos(&pos), vim_iswhite(c))	/* vim_iswhite() is a macro */
	incl(&pos);
    if (equal(pos, curwin->w_cursor))
    {
	start_blank = TRUE;
	find_first_blank(&start_pos);	/* go back to first blank */
    }
    else
    {
	start_blank = FALSE;
	findsent(BACKWARD, 1L);
	start_pos = curwin->w_cursor;
    }
    if (include)
	ncount = count * 2;
    else
	ncount = count;
    if (!include && start_blank)
	--ncount;
    if (ncount)
	findsent_forward(ncount, TRUE);

    if (include)
    {
	/*
	 * If the blank in front of the sentence is included, exclude the
	 * blanks at the end of the sentence, go back to the first blank.
	 * If there are no trailing blanks, try to include leading blanks.
	 */
	if (start_blank)
	    find_first_blank(&curwin->w_cursor);
	else if (c = gchar_cursor(), !vim_iswhite(c))
	    find_first_blank(&start_pos);
    }

    if (VIsual_active)
    {
	/* avoid getting stuck with "is" on a single space before a sent. */
	if (equal(start_pos, curwin->w_cursor))
	    goto extend;
	VIsual = start_pos;
	VIsual_mode = 'v';
	update_curbuf(NOT_VALID);	/* update the inversion */
    }
    else
    {
	/* include a newline after the sentence, if there is one */
	if (incl(&curwin->w_cursor) == -1)
	    oap->inclusive = TRUE;
	else
	    oap->inclusive = FALSE;
	oap->start = start_pos;
	oap->motion_type = MCHAR;
    }
    return OK;
}

    int
current_block(oap, count, include, what, other)
    OPARG	*oap;
    long	count;
    int		include;	/* TRUE == include white space */
    int		what;		/* '(', '{', etc. */
    int		other;		/* ')', '}', etc. */
{
    FPOS	old_pos;
    FPOS	*pos = NULL;
    FPOS	start_pos;
    FPOS	*end_pos;
    FPOS	old_start, old_end;
    char_u	*save_cpo;

    old_pos = curwin->w_cursor;
    old_end = curwin->w_cursor;		    /* remember where we started */
    old_start = old_end;

    /*
     * If we start on '(', '{', ')', '}', etc., use the whole block inclusive.
     */
    if (!VIsual_active || equal(VIsual, curwin->w_cursor))
    {
	setpcmark();
	if (what == '{')		    /* ignore indent */
	    while (inindent(1))
		if (inc_cursor() != 0)
		    break;
	if (gchar_cursor() == what)	    /* cursor on '(' or '{' */
	    ++curwin->w_cursor.col;
    }
    else if (lt(VIsual, curwin->w_cursor))
    {
	old_start = VIsual;
	curwin->w_cursor = VIsual;	    /* cursor at low end of Visual */
    }
    else
	old_end = VIsual;

    /*
     * Search backwards for unclosed '(', '{', etc..
     * Put this position in start_pos.
     * Ignory quotes here.
     */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"%";
    while (count-- > 0)
    {
	if ((pos = findmatch(NULL, what)) == NULL)
	    break;
	curwin->w_cursor = *pos;
	start_pos = *pos;   /* the findmatch for end_pos will overwrite *pos */
    }
    p_cpo = save_cpo;

    /*
     * Search for matching ')', '}', etc.
     * Put this position in curwin->w_cursor.
     */
    if (pos == NULL || (end_pos = findmatch(NULL, other)) == NULL)
    {
	curwin->w_cursor = old_pos;
	return FAIL;
    }
    curwin->w_cursor = *end_pos;

    /*
     * Try to exclude the '(', '{', ')', '}', etc. when "include" is FALSE.
     * If the ending '}' is only preceded by indent, skip that indent.
     * But only if the resulting area is not smaller than what we started with.
     */
    while (!include)
    {
	incl(&start_pos);
	decl(&curwin->w_cursor);
	if (what == '{')
	    while (inindent(1))
		if (decl(&curwin->w_cursor) != 0)
		    break;
	/*
	 * In Visual mode, when the resulting area is not bigger than what we
	 * started with, extend it to the next block, and then exclude again.
	 */
	if (!lt(start_pos, old_start) && !lt(old_end, curwin->w_cursor)
		&& VIsual_active)
	{
	    curwin->w_cursor = old_start;
	    decl(&curwin->w_cursor);
	    if ((pos = findmatch(NULL, what)) == NULL)
	    {
		curwin->w_cursor = old_pos;
		return FAIL;
	    }
	    start_pos = *pos;
	    curwin->w_cursor = *pos;
	    if ((end_pos = findmatch(NULL, other)) == NULL)
	    {
		curwin->w_cursor = old_pos;
		return FAIL;
	    }
	    curwin->w_cursor = *end_pos;
	}
	else
	    break;
    }

    if (VIsual_active)
    {
	if (*p_sel == 'e')
	    ++curwin->w_cursor.col;
	VIsual = start_pos;
	VIsual_mode = 'v';
	update_curbuf(NOT_VALID);	/* update the inversion */
	showmode();
    }
    else
    {
	oap->start = start_pos;
	oap->motion_type = MCHAR;
	oap->inclusive = TRUE;
    }

    return OK;
}

    int
current_par(oap, count, include, type)
    OPARG   *oap;
    long    count;
    int	    include;	    /* TRUE == include white space */
    int	    type;	    /* 'p' for paragraph, 'S' for section */
{
    linenr_t	start_lnum;
    linenr_t	end_lnum;
    int		white_in_front;
    int		dir;
    int		start_is_white;
    int		prev_start_is_white;
    int		retval = OK;
    int		do_white = FALSE;
    int		t;
    int		i;

    if (type == 'S')	    /* not implemented yet */
	return FAIL;

    start_lnum = curwin->w_cursor.lnum;

    /*
     * When visual area is more than one line: extend it.
     */
    if (VIsual_active && start_lnum != VIsual.lnum)
    {
extend:
	if (start_lnum < VIsual.lnum)
	    dir = BACKWARD;
	else
	    dir = FORWARD;
	for (i = count; --i >= 0; )
	{
	    if (start_lnum ==
			   (dir == BACKWARD ? 1 : curbuf->b_ml.ml_line_count))
	    {
		retval = FAIL;
		break;
	    }

	    prev_start_is_white = -1;
	    for (t = 0; t < 2; ++t)
	    {
		start_lnum += dir;
		start_is_white = linewhite(start_lnum);
		if (prev_start_is_white == start_is_white)
		{
		    start_lnum -= dir;
		    break;
		}
		for (;;)
		{
		    if (start_lnum == (dir == BACKWARD
					    ? 1 : curbuf->b_ml.ml_line_count))
			break;
		    if (start_is_white != linewhite(start_lnum + dir)
			    || (!start_is_white
				    && startPS(start_lnum + (dir > 0
							     ? 1 : 0), 0, 0)))
			break;
		    start_lnum += dir;
		}
		if (!include)
		    break;
		if (start_lnum == (dir == BACKWARD
					    ? 1 : curbuf->b_ml.ml_line_count))
		    break;
		prev_start_is_white = start_is_white;
	    }
	}
	curwin->w_cursor.lnum = start_lnum;
	curwin->w_cursor.col = 0;
	return retval;
    }

    /*
     * First move back to the start_lnum of the paragraph or white lines
     */
    white_in_front = linewhite(start_lnum);
    while (start_lnum > 1)
    {
	if (white_in_front)	    /* stop at first white line */
	{
	    if (!linewhite(start_lnum - 1))
		break;
	}
	else		/* stop at first non-white line of start of paragraph */
	{
	    if (linewhite(start_lnum - 1) || startPS(start_lnum, 0, 0))
		break;
	}
	--start_lnum;
    }

    /*
     * Move past the end of any white lines.
     */
    end_lnum = start_lnum;
    while (linewhite(end_lnum) && end_lnum < curbuf->b_ml.ml_line_count)
	    ++end_lnum;

    --end_lnum;
    i = count;
    if (!include && white_in_front)
	--i;
    while (i--)
    {
	if (end_lnum == curbuf->b_ml.ml_line_count)
	    return FAIL;

	if (!include)
	    do_white = linewhite(end_lnum + 1);

	if (include || !do_white)
	{
	    ++end_lnum;
	    /*
	     * skip to end of paragraph
	     */
	    while (end_lnum < curbuf->b_ml.ml_line_count
		    && !linewhite(end_lnum + 1)
		    && !startPS(end_lnum + 1, 0, 0))
		++end_lnum;
	}

	if (i == 0 && white_in_front)
	    break;

	/*
	 * skip to end of white lines after paragraph
	 */
	if (include || do_white)
	    while (end_lnum < curbuf->b_ml.ml_line_count
						   && linewhite(end_lnum + 1))
		++end_lnum;
    }

    /*
     * If there are no empty lines at the end, try to find some empty lines at
     * the start (unless that has been done already).
     */
    if (!white_in_front && !linewhite(end_lnum) && include)
	while (start_lnum > 1 && linewhite(start_lnum - 1))
	    --start_lnum;

    if (VIsual_active)
    {
	/* Problem: when doing "Vipipip" nothing happens in a single white
	 * line, we get stuck there.  Trap this here. */
	if (VIsual_mode == 'V' && start_lnum == curwin->w_cursor.lnum)
	    goto extend;
	VIsual.lnum = start_lnum;
	VIsual_mode = 'V';
	update_curbuf(NOT_VALID);	/* update the inversion */
	showmode();
    }
    else
    {
	oap->start.lnum = start_lnum;
	oap->motion_type = MLINE;
    }
    curwin->w_cursor.lnum = end_lnum;
    curwin->w_cursor.col = 0;

    return OK;
}
#endif

#if defined(LISPINDENT) || defined(CINDENT) || defined(TEXT_OBJECTS) \
	|| defined(PROTO)
/*
 * return TRUE if line 'lnum' is empty or has white chars only.
 */
    int
linewhite(lnum)
    linenr_t	lnum;
{
    char_u  *p;

    p = skipwhite(ml_get(lnum));
    return (*p == NUL);
}
#endif

#if defined(FIND_IN_PATH) || defined(PROTO)
/*
 * Find identifiers or defines in included files.
 * if p_ic && (continue_status & CONT_SOL) then ptr must be in lowercase.
 */
/*ARGSUSED*/
    void
find_pattern_in_path(ptr, dir, len, whole, skip_comments,
				    type, count, action, start_lnum, end_lnum)
    char_u	*ptr;		/* pointer to search pattern */
    int		dir;		/* direction of expansion */
    int		len;		/* length of search pattern */
    int		whole;		/* match whole words only */
    int		skip_comments;	/* don't match inside comments */
    int		type;		/* Type of search; are we looking for a type?
				   a macro? */
    long	count;
    int		action;		/* What to do when we find it */
    linenr_t	start_lnum;	/* first line to start searching */
    linenr_t	end_lnum;	/* last line for searching */
{
    SearchedFile *files;		/* Stack of included files */
    SearchedFile *bigger;		/* When we need more space */
    int		max_path_depth = 50;
    long	match_count = 1;

    char_u	*pat;
    int		pat_reg_ic = FALSE;
    char_u	*new_fname;
    char_u	*curr_fname = curbuf->b_fname;
    char_u	*prev_fname = NULL;
    linenr_t	lnum;
    int		depth;
    int		depth_displayed;	/* For type==CHECK_PATH */
    int		old_files;
    int		already_searched;
    char_u	*file_line;
    char_u	*line;
    char_u	*p;
    char_u	save_char;
    int		define_matched;
    vim_regexp	*prog = NULL;
    vim_regexp	*include_prog = NULL;
    vim_regexp	*define_prog = NULL;
    int		matched = FALSE;
    int		did_show = FALSE;
    int		found = FALSE;
    int		i;
    char_u	*already = NULL;
    char_u	*startp = NULL;
#ifdef RISCOS
    int		previous_munging = __uname_control;
#endif

    file_line = alloc(LSIZE);
    if (file_line == NULL)
	return;

#ifdef RISCOS
    /* UnixLib knows best how to munge c file names - turn munging back on. */
    __uname_control = __UNAME_LONG_TRUNC;
#endif

    if (type != CHECK_PATH && type != FIND_DEFINE
#ifdef INSERT_EXPAND
	/* when CONT_SOL is set compare "ptr" with the beginning of the line
	 * is faster than quote_meta/regcomp/regexep "ptr" -- Acevedo */
	    && !(continue_status & CONT_SOL)
#endif
       )
    {
	pat = alloc(len + 5);
	if (pat == NULL)
	    goto fpip_end;
	sprintf((char *)pat, whole ? "\\<%.*s\\>" : "%.*s", len, ptr);
	set_reg_ic(pat);    /* set reg_ic according to p_ic, p_scs and pat */
	pat_reg_ic = reg_ic;
	prog = vim_regcomp(pat, (int)p_magic);
	vim_free(pat);
	if (prog == NULL)
	    goto fpip_end;
    }
    if (*p_inc != NUL)
    {
	include_prog = vim_regcomp(p_inc, (int)p_magic);
	if (include_prog == NULL)
	    goto fpip_end;
    }
    if (type == FIND_DEFINE && *p_def != NUL)
    {
	define_prog = vim_regcomp(p_def, (int)p_magic);
	if (define_prog == NULL)
	    goto fpip_end;
    }
    files = (SearchedFile *)lalloc((long_u)
			       (max_path_depth * sizeof(SearchedFile)), TRUE);
    if (files == NULL)
	goto fpip_end;
    for (i = 0; i < max_path_depth; i++)
    {
	files[i].fp = NULL;
	files[i].name = NULL;
	files[i].lnum = 0;
	files[i].matched = FALSE;
    }
    old_files = max_path_depth;
    depth = depth_displayed = -1;

    lnum = start_lnum;
    if (end_lnum > curbuf->b_ml.ml_line_count)
	end_lnum = curbuf->b_ml.ml_line_count;
    if (lnum > end_lnum)		/* do at least one line */
	lnum = end_lnum;
    line = ml_get(lnum);

    for (;;)
    {
	reg_ic = FALSE;	/* don't ignore case in include pattern */
	if (include_prog != NULL && vim_regexec(include_prog, line, TRUE))
	{
	    new_fname = get_file_name_in_path(include_prog->endp[0] + 1,
							    0, FNAME_EXP, 1L);
	    already_searched = FALSE;
	    if (new_fname != NULL)
	    {
		/* Check whether we have already searched in this file */
		for (i = 0;; i++)
		{
		    if (i == depth + 1)
			i = old_files;
		    if (i == max_path_depth)
			break;
		    if (STRCMP(new_fname, files[i].name) == 0)
		    {
			if (type != CHECK_PATH &&
				action == ACTION_SHOW_ALL && files[i].matched)
			{
			    msg_putchar('\n');	    /* cursor below last one */
			    if (!got_int)	    /* don't display if 'q'
						       typed at "--more--"
						       mesage */
			    {
				msg_home_replace_hl(new_fname);
				MSG_PUTS(" (includes previously listed match)");
				prev_fname = NULL;
			    }
			}
			vim_free(new_fname);
			new_fname = NULL;
			already_searched = TRUE;
			break;
		    }
		}
	    }

	    if (type == CHECK_PATH && (action == ACTION_SHOW_ALL ||
				    (new_fname == NULL && !already_searched)))
	    {
		if (did_show)
		    msg_putchar('\n');	    /* cursor below last one */
		else
		{
		    gotocmdline(TRUE);	    /* cursor at status line */
		    MSG_PUTS_TITLE("--- Included files ");
		    if (action != ACTION_SHOW_ALL)
			MSG_PUTS_TITLE("not found ");
		    MSG_PUTS_TITLE("in path ---\n");
		}
		did_show = TRUE;
		while (depth_displayed < depth && !got_int)
		{
		    ++depth_displayed;
		    for (i = 0; i < depth_displayed; i++)
			MSG_PUTS("  ");
		    msg_home_replace(files[depth_displayed].name);
		    MSG_PUTS(" -->\n");
		}
		if (!got_int)		    /* don't display if 'q' typed
					       for "--more--" message */
		{
		    for (i = 0; i <= depth_displayed; i++)
			MSG_PUTS("  ");
		    /*
		     * Isolate the file name.
		     * Include the surrounding "" or <> if present.
		     */
		    for (p = include_prog->endp[0] + 1; !vim_isfilec(*p); p++)
			;
		    for (i = 0; vim_isfilec(p[i]); i++)
			;
		    if (p[-1] == '"' || p[-1] == '<')
		    {
			--p;
			++i;
		    }
		    if (p[i] == '"' || p[i] == '>')
			++i;
		    save_char = p[i];
		    p[i] = NUL;
				/* Same highlighting as for directories */
		    msg_outtrans_attr(p, hl_attr(HLF_D));
		    p[i] = save_char;
		    if (new_fname == NULL && action == ACTION_SHOW_ALL)
		    {
			if (already_searched)
			    MSG_PUTS("  (Already listed)");
			else
			    MSG_PUTS("  NOT FOUND");
		    }
		}
		out_flush();	    /* output each line directly */
	    }

	    if (new_fname != NULL)
	    {
		/* Push the new file onto the file stack */
		if (depth + 1 == old_files)
		{
		    bigger = (SearchedFile *)lalloc((long_u)(
			    max_path_depth * 2 * sizeof(SearchedFile)), TRUE);
		    if (bigger != NULL)
		    {
			for (i = 0; i <= depth; i++)
			    bigger[i] = files[i];
			for (i = depth + 1; i < old_files + max_path_depth; i++)
			{
			    bigger[i].fp = NULL;
			    bigger[i].name = NULL;
			    bigger[i].lnum = 0;
			    bigger[i].matched = FALSE;
			}
			for (i = old_files; i < max_path_depth; i++)
			    bigger[i + max_path_depth] = files[i];
			old_files += max_path_depth;
			max_path_depth *= 2;
			vim_free(files);
			files = bigger;
		    }
		}
		if ((files[depth + 1].fp = mch_fopen((char *)new_fname, "r"))
								    == NULL)
		    vim_free(new_fname);
		else
		{
		    if (++depth == old_files)
		    {
			/*
			 * lalloc() for 'bigger' must have failed above.  We
			 * will forget one of our already visited files now.
			 */
			vim_free(files[old_files].name);
			++old_files;
		    }
		    files[depth].name = curr_fname = new_fname;
		    files[depth].lnum = 0;
		    files[depth].matched = FALSE;
#ifdef INSERT_EXPAND
		    if (action == ACTION_EXPAND)
		    {
			sprintf((char*)IObuff, "Scanning included file: %s",
			    (char *)new_fname);
			msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
		    }
#endif
		}
	    }
	}
	else
	{
	    /*
	     * Check if the line is a define (type == FIND_DEFINE)
	     */
	    p = line;
search_line:
	    define_matched = FALSE;
	    reg_ic = FALSE;	/* don't ignore case in define patterns */
	    if (define_prog != NULL && vim_regexec(define_prog, line, TRUE))
	    {
		/*
		 * Pattern must be first identifier after 'define', so skip
		 * to that position before checking for match of pattern.  Also
		 * don't let it match beyond the end of this identifier.
		 */
		p = define_prog->endp[0] + 1;
		while (*p && !vim_isIDc(*p))
		    p++;
		define_matched = TRUE;
	    }

	    /*
	     * Look for a match.  Don't do this if we are looking for a
	     * define and this line didn't match define_prog above.
	     */
	    if (define_prog == NULL || define_matched)
	    {
		reg_ic = pat_reg_ic;
		if (define_matched
#ifdef INSERT_EXPAND
			|| (continue_status & CONT_SOL)
#endif
		    )
		{
		    /* compare the first "len" chars from "ptr" */
		    startp = skipwhite(p);
		    if (p_ic)
			matched = !STRNICMP(startp, ptr, len);
		    else
			matched = !STRNCMP(startp, ptr, len);
		    if (matched && define_matched && whole
						    && vim_isIDc(startp[len]))
			matched = FALSE;
		}
		else if (prog && vim_regexec(prog, p, p == line))
		{
		    matched = TRUE;
		    startp = prog->startp[0];
		    /*
		     * Check if the line is not a comment line (unless we are
		     * looking for a define).  A line starting with "# define"
		     * is not considered to be a comment line.
		     */
		    if (!define_matched && skip_comments)
		    {
#ifdef COMMENTS
			fo_do_comments = TRUE;
			if ((*line != '#' ||
				STRNCMP(skipwhite(line + 1), "define", 6) != 0)
				&& get_leader_len(line, NULL, FALSE)
				)
			    matched = FALSE;

			/*
			 * Also check for a "/ *" or "/ /" before the match.
			 * Skips lines like "int backwards;  / * normal index
			 * * /" when looking for "normal".
			 * Note: Doesn't skip "/ *" in comments.
			 */
			p = skipwhite(line);
			if (matched
				|| (p[0] == '/' && p[1] == '*') || p[0] == '*')
#endif
			    for (p = line; *p && p < startp; ++p)
			    {
				if (matched
					&& p[0] == '/'
					&& (p[1] == '*' || p[1] == '/'))
				{
				    matched = FALSE;
				    /* After "//" all text is comment */
				    if (p[1] == '/')
					break;
				    ++p;
				}
				else if (!matched && p[0] == '*' && p[1] == '/')
				{
				    /* Can find match after "* /". */
				    matched = TRUE;
				    ++p;
				}
			    }
#ifdef COMMENTS
			fo_do_comments = FALSE;
#endif
		    }
		}
	    }
	}
	if (matched)
	{
#ifdef INSERT_EXPAND
	    if (action == ACTION_EXPAND)
	    {
		int	reuse = 0;
		int	add_r;
		char_u	*aux;

		if (depth == -1 && lnum == curwin->w_cursor.lnum)
		    break;
		found = TRUE;
		aux = p = startp;
		if (continue_status & CONT_ADDING)
		{
		    p += completion_length;
		    if (vim_iswordc(*p))
			goto exit_matched;
		    while (*p && *p != '\n' && !vim_iswordc(*p++))
			;
		}
		while (vim_iswordc(*p))
		    ++p;
		i = p - aux;

		if ((continue_status & CONT_ADDING) && i == completion_length)
		{
		    /* get the next line */
		    /* IOSIZE > completion_length, so the STRNCPY works */
		    STRNCPY(IObuff, aux, i);
		    if (!(     depth < 0
			    && lnum < end_lnum
			    && (line = ml_get(++lnum)) != NULL)
			&& !(	depth >= 0
			    && !vim_fgets(line = file_line,
						     LSIZE, files[depth].fp)))
			goto exit_matched;

		    /* we read a line, set "already" to check this "line" later
		     * if depth >= 0 we'll increase files[depth].lnum far
		     * bellow  -- Acevedo */
		    already = aux = p = skipwhite(line);
		    while (*p && *p != '\n' && !vim_iswordc(*p++))
			;
		    while (vim_iswordc(*p))
			p++;
		    if (p > aux)
		    {
			if (*aux != ')' && IObuff[i-1] != TAB)
			{
			    if (IObuff[i-1] != ' ')
				IObuff[i++] = ' ';
			    /* IObuf =~ "\(\k\|\i\).* ", thus i >= 2*/
			    if (p_js
				&& (IObuff[i-2] == '.'
				    || (vim_strchr(p_cpo, CPO_JOINSP) == NULL
					&& (IObuff[i-2] == '?'
					    || IObuff[i-2] == '!'))))
				IObuff[i++] = ' ';
			}
			/* copy as much as posible of the new word */
			if (p - aux >= IOSIZE - i)
			    p = aux + IOSIZE - i - 1;
			STRNCPY(IObuff + i, aux, p - aux);
			i += p - aux;
			reuse |= CONT_S_IPOS;
		    }
		    IObuff[i] = NUL;
		    aux = IObuff;

		    if (i == completion_length)
			goto exit_matched;
		}

		add_r = ins_compl_add_infercase(aux, i,
			curr_fname == curbuf->b_fname ? NULL : curr_fname,
			dir, reuse);
		if (add_r == OK)
		    /* if dir was BACKWARD then honor it just once */
		    dir = FORWARD;
		else if (add_r == RET_ERROR)
		    break;
	    }
	    else
#endif
		 if (action == ACTION_SHOW_ALL)
	    {
		found = TRUE;
		if (!did_show)
		    gotocmdline(TRUE);		/* cursor at status line */
		if (curr_fname != prev_fname)
		{
		    if (did_show)
			msg_putchar('\n');	/* cursor below last one */
		    if (!got_int)		/* don't display if 'q' typed
						    at "--more--" mesage */
			msg_home_replace_hl(curr_fname);
		    prev_fname = curr_fname;
		}
		did_show = TRUE;
		if (!got_int)
		    show_pat_in_path(line, type, TRUE, action,
			    (depth == -1) ? NULL : files[depth].fp,
			    (depth == -1) ? &lnum : &files[depth].lnum,
			    match_count++);

		/* Set matched flag for this file and all the ones that
		 * include it */
		for (i = 0; i <= depth; ++i)
		    files[i].matched = TRUE;
	    }
	    else if (--count <= 0)
	    {
		found = TRUE;
		if (depth == -1 && lnum == curwin->w_cursor.lnum)
		    EMSG("Match is on current line");
		else if (action == ACTION_SHOW)
		{
		    show_pat_in_path(line, type, did_show, action,
			(depth == -1) ? NULL : files[depth].fp,
			(depth == -1) ? &lnum : &files[depth].lnum, 1L);
		    did_show = TRUE;
		}
		else
		{
#ifdef USE_GUI
		    need_mouse_correct = TRUE;
#endif
		    if (action == ACTION_SPLIT)
		    {
			if (win_split(0, FALSE, FALSE) == FAIL)
			    break;
		    }
		    if (depth == -1)
		    {
			setpcmark();
			curwin->w_cursor.lnum = lnum;
		    }
		    else
		    {
			if (getfile(0, files[depth].name, NULL, TRUE,
						files[depth].lnum, FALSE) > 0)
			    break;	/* failed to jump to file */
			/* autocommands may have changed the lnum, we don't
			 * want that here */
			curwin->w_cursor.lnum = files[depth].lnum;
		    }
		}
		if (action != ACTION_SHOW)
		{
		    curwin->w_cursor.col = startp - line;
		    curwin->w_set_curswant = TRUE;
		}
		break;
	    }
#ifdef INSERT_EXPAND
exit_matched:
#endif
	    matched = FALSE;
	    /* look for other matches in the rest of the line if we
	     * are not at the end of it already */
	    if (define_prog == NULL
#ifdef INSERT_EXPAND
		    && action == ACTION_EXPAND
		    && !(continue_status & CONT_SOL)
#endif
		    && *(p = startp + 1))
		goto search_line;
	}
	line_breakcheck();
#ifdef INSERT_EXPAND
	if (action == ACTION_EXPAND)
	    ins_compl_check_keys();
	if (got_int || completion_interrupted)
#else
	if (got_int)
#endif
	    break;
	while (depth >= 0 && !already &&
	       vim_fgets(line = file_line, LSIZE, files[depth].fp))
	{
	    fclose(files[depth].fp);
	    --old_files;
	    files[old_files].name = files[depth].name;
	    files[old_files].matched = files[depth].matched;
	    --depth;
	    curr_fname = (depth == -1) ? curbuf->b_fname
				       : files[depth].name;
	    if (depth < depth_displayed)
		depth_displayed = depth;
	}
	if (depth >= 0)		/* we could read the line */
	    files[depth].lnum++;
	else if (!already)
	{
	    if (++lnum > end_lnum)
		break;
	    line = ml_get(lnum);
	}
	already = NULL;
    }
    for (i = 0; i <= depth; i++)
    {
	fclose(files[i].fp);
	vim_free(files[i].name);
    }
    for (i = old_files; i < max_path_depth; i++)
	vim_free(files[i].name);
    vim_free(files);

    if (type == CHECK_PATH)
    {
	if (!did_show)
	{
	    if (action != ACTION_SHOW_ALL)
		MSG("All included files were found");
	    else
		MSG("No included files");
	}
    }
    else if (!found
#ifdef INSERT_EXPAND
		    && action != ACTION_EXPAND
#endif
						)
    {
#ifdef INSERT_EXPAND
	if (got_int || completion_interrupted)
#else
	if (got_int)
#endif
	    emsg(e_interr);
	else if (type == FIND_DEFINE)
	    EMSG("Couldn't find definition");
	else
	    EMSG("Couldn't find pattern");
    }
    if (action == ACTION_SHOW || action == ACTION_SHOW_ALL)
	msg_end();

fpip_end:
    vim_free(file_line);
#ifdef INSERT_EXPAND
    if (!(continue_status & CONT_SOL))
#endif
	vim_free(prog);
    vim_free(include_prog);
    vim_free(define_prog);

#ifdef RISCOS
   /* Restore previous file munging state. */
    __uname_control = previous_munging;
#endif
}

    static void
show_pat_in_path(line, type, did_show, action, fp, lnum, count)
    char_u  *line;
    int	    type;
    int	    did_show;
    int	    action;
    FILE    *fp;
    linenr_t *lnum;
    long    count;
{
    char_u  *p;

    if (did_show)
	msg_putchar('\n');	/* cursor below last one */
    else
	gotocmdline(TRUE);	/* cursor at status line */
    if (got_int)		/* 'q' typed at "--more--" message */
	return;
    for (;;)
    {
	p = line + STRLEN(line) - 1;
	if (fp != NULL)
	{
	    /* We used fgets(), so get rid of newline at end */
	    if (p >= line && *p == '\n')
		--p;
	    if (p >= line && *p == '\r')
		--p;
	    *(p + 1) = NUL;
	}
	if (action == ACTION_SHOW_ALL)
	{
	    sprintf((char *)IObuff, "%3ld: ", count);	/* show match nr */
	    msg_puts(IObuff);
	    sprintf((char *)IObuff, "%4ld", *lnum);	/* show line nr */
						/* Highlight line numbers */
	    msg_puts_attr(IObuff, hl_attr(HLF_N));
	    MSG_PUTS(" ");
	}
	msg_prt_line(line);
	out_flush();			/* show one line at a time */

	/* Definition continues until line that doesn't end with '\' */
	if (got_int || type != FIND_DEFINE || p < line || *p != '\\')
	    break;

	if (fp != NULL)
	{
	    if (vim_fgets(line, LSIZE, fp)) /* end of file */
		break;
	    ++*lnum;
	}
	else
	{
	    if (++*lnum > curbuf->b_ml.ml_line_count)
		break;
	    line = ml_get(*lnum);
	}
	msg_putchar('\n');
    }
}
#endif

#ifdef VIMINFO
    int
read_viminfo_search_pattern(line, fp, force)
    char_u	*line;
    FILE	*fp;
    int		force;
{
    char_u	*lp;
    int		idx = -1;
    int		magic = FALSE;
    int		no_scs = FALSE;
    int		off_line = FALSE;
    int		off_end = FALSE;
    long	off = 0;
    int		setlast = FALSE;
#ifdef EXTRA_SEARCH
    static int	hlsearch_on = FALSE;
#endif
    char_u	*val;

    /*
     * Old line types:
     * "/pat", "&pat": search/subst. pat
     * "~/pat", "~&pat": last used search/subst. pat
     * New line types:
     * "~h", "~H": hlsearch highlighting off/on
     * "~<magic><smartcase><line><end><off><last><which>pat"
     * <magic>: 'm' off, 'M' on
     * <smartcase>: 's' off, 'S' on
     * <line>: 'L' line offset, 'l' char offset
     * <end>: 'E' from end, 'e' from start
     * <off>: decimal, offset
     * <last>: '~' last used pattern
     * <which>: '/' search pat, '&' subst. pat
     */
    lp = line;
    if (lp[0] == '~' && (lp[1] == 'm' || lp[1] == 'M'))	/* new line type */
    {
	if (lp[1] == 'M')		/* magic on */
	    magic = TRUE;
	if (lp[2] == 's')
	    no_scs = TRUE;
	if (lp[3] == 'L')
	    off_line = TRUE;
	if (lp[4] == 'E')
	    off_end = TRUE;
	lp += 5;
	off = getdigits(&lp);
    }
    if (lp[0] == '~')		/* use this pattern for last-used pattern */
    {
	setlast = TRUE;
	lp++;
    }
    if (lp[0] == '/')
	idx = RE_SEARCH;
    else if (lp[0] == '&')
	idx = RE_SUBST;
#ifdef EXTRA_SEARCH
    else if (lp[0] == 'h')	/* ~h: 'hlsearch' highlighting off */
	hlsearch_on = FALSE;
    else if (lp[0] == 'H')	/* ~H: 'hlsearch' highlighting on */
	hlsearch_on = TRUE;
#endif
    if (idx >= 0)
    {
	if (force || spats[idx].pat == NULL)
	{
	    val = viminfo_readstring(lp + 1, fp);
	    if (val != NULL)
	    {
		set_last_search_pat(val, idx, magic, setlast);
		vim_free(val);
		spats[idx].no_scs = no_scs;
		spats[idx].off.line = off_line;
		spats[idx].off.end = off_end;
		spats[idx].off.off = off;
#ifdef EXTRA_SEARCH
		if (setlast)
		    no_hlsearch = !hlsearch_on;
#endif
	    }
	}
    }
    return vim_fgets(line, LSIZE, fp);
}

    void
write_viminfo_search_pattern(fp)
    FILE    *fp;
{
    if (get_viminfo_parameter('/') != 0)
    {
#ifdef EXTRA_SEARCH
	fprintf(fp, "\n# hlsearch on (H) or off (h):\n~%c",
	    (no_hlsearch || find_viminfo_parameter('h') != NULL) ? 'h' : 'H');
#endif
	wvsp_one(fp, RE_SEARCH, "", '/');
	wvsp_one(fp, RE_SUBST, "Substitute ", '&');
    }
}

    static void
wvsp_one(fp, idx, s, sc)
    FILE	*fp;	/* file to write to */
    int		idx;	/* spats[] index */
    char	*s;	/* search pat */
    int		sc;	/* dir char */
{
    if (spats[idx].pat != NULL)
    {
	fprintf(fp, "\n# Last %sSearch Pattern:\n~", s);
	/* off.dir is not stored, it's reset to forward */
	fprintf(fp, "%c%c%c%c%ld%s%c",
		spats[idx].magic    ? 'M' : 'm',	/* magic */
		spats[idx].no_scs   ? 's' : 'S',	/* smartcase */
		spats[idx].off.line ? 'L' : 'l',	/* line offset */
		spats[idx].off.end  ? 'E' : 'e',	/* offset from end */
		spats[idx].off.off,			/* offset */
		last_idx == idx	    ? "~" : "",		/* last used pat */
		sc);
	viminfo_writestring(fp, spats[idx].pat);
    }
}
#endif /* VIMINFO */
