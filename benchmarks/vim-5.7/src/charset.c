/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include "vim.h"

#ifdef LINEBREAK
static int win_chartabsize __ARGS((WIN *wp, int c, colnr_t col));
#endif

/*
 * chartab[] is used
 * - to quickly recognize ID characters
 * - to quickly recognize file name characters
 * - to quickly recognize printable characters
 * - to store the size of a character on the screen: Printable is 1 position,
 *   2 otherwise
 */
static int    chartab_initialized = FALSE;

/*
 * init_chartab(): Fill chartab[] with flags for ID and file name characters
 * and the size of characters on the screen (1 or 2 positions).
 * Also fills b_chartab[] with flags for keyword characters for current
 * buffer.
 *
 * Return FAIL if 'iskeyword', 'isident', 'isfname' or 'isprint' option has an
 * error, OK otherwise.
 */
    int
init_chartab()
{
    int	    c;
    int	    c2;
    char_u  *p;
    int	    i;
    int	    tilde;
    int	    do_isalpha;

    /*
     * Set the default size for printable characters:
     * From <Space> to '~' is 1 (printable), others are 2 (not printable).
     * This also inits all 'isident' and 'isfname' flags to FALSE.
     */
    c = 0;
    while (c < ' ')
	chartab[c++] = 2;
    while (c <= '~')
	chartab[c++] = 1 + CHAR_IP;
#ifdef FKMAP
    if (p_altkeymap)
    {
	while (c < YE)
	    chartab[c++] = 1 + CHAR_IP;
    }
#endif
    while (c < 256)
	chartab[c++] = 2;

    /*
     * Init word char flags all to FALSE
     */
    if (curbuf != NULL)
	for (c = 0; c < 256; ++c)
	    curbuf->b_chartab[c] = FALSE;

#ifdef LISPINDENT
    /*
     * In lisp mode the '-' character is included in keywords.
     */
    if (curbuf->b_p_lisp)
	curbuf->b_chartab['-'] = TRUE;
#endif

    /* Walk through the 'isident', 'iskeyword', 'isfname' and 'isprint'
     * options Each option is a list of characters, character numbers or
     * ranges, separated by commas, e.g.: "200-210,x,#-178,-"
     */
    for (i = 0; i < 4; ++i)
    {
	if (i == 0)
	    p = p_isi;		    /* first round: 'isident' */
	else if (i == 1)
	    p = p_isp;		    /* second round: 'isprint' */
	else if (i == 2)
	    p = p_isf;		    /* third round: 'isfname' */
	else	/* i == 3 */
	    p = curbuf->b_p_isk;    /* fourth round: 'iskeyword' */

	while (*p)
	{
	    tilde = FALSE;
	    do_isalpha = FALSE;
	    if (*p == '^' && p[1] != NUL)
	    {
		tilde = TRUE;
		++p;
	    }
	    if (isdigit(*p))
		c = getdigits(&p);
	    else
		c = *p++;
	    c2 = -1;
	    if (*p == '-' && p[1] != NUL)
	    {
		++p;
		if (isdigit(*p))
		    c2 = getdigits(&p);
		else
		    c2 = *p++;
	    }
	    if (c <= 0 || (c2 < c && c2 != -1) || c2 >= 256 ||
						    !(*p == NUL || *p == ','))
		return FAIL;

	    if (c2 == -1)	/* not a range */
	    {
		/*
		 * A single '@' (not "@-@"):
		 * Decide on letters being ID/printable/keyword chars with
		 * standard function isalpha(). This takes care of locale.
		 */
		if (c == '@')
		{
		    do_isalpha = TRUE;
		    c = 1;
		    c2 = 255;
		}
		else
		    c2 = c;
	    }
	    while (c <= c2)
	    {
		if (!do_isalpha || isalpha(c)
#ifdef FKMAP
			|| (p_altkeymap && (F_isalpha(c) || F_isdigit(c)))
#endif
			    )
		{
		    if (i == 0)			/* (re)set ID flag */
		    {
			if (tilde)
			    chartab[c] &= ~CHAR_ID;
			else
			    chartab[c] |= CHAR_ID;
		    }
		    else if (i == 1)		/* set printable to 1 or 2 */
		    {
			if (c < ' ' || c > '~'
#ifdef FKMAP
				|| (p_altkeymap
					    && (F_isalpha(c) || F_isdigit(c)))
#endif
				)
			{
			    if (tilde)
			    {
				chartab[c] = (chartab[c] & ~CHAR_MASK) + 2;
				chartab[c] &= ~CHAR_IP;
			    }
			    else
			    {
				chartab[c] = (chartab[c] & ~CHAR_MASK) + 1;
				chartab[c] |= CHAR_IP;
			    }
			}
		    }
		    else if (i == 2)		/* (re)set fname flag */
		    {
			if (tilde)
			    chartab[c] &= ~CHAR_IF;
			else
			    chartab[c] |= CHAR_IF;
		    }
		    else /* i == 3 */		/* (re)set keyword flag */
			curbuf->b_chartab[c] = !tilde;
		}
		++c;
	    }
	    p = skip_to_option_part(p);
	}
    }
    chartab_initialized = TRUE;
    return OK;
}

/*
 * Translate any special characters in buf[bufsize].
 * If there is not enough room, not all characters will be translated.
 */
    void
trans_characters(buf, bufsize)
    char_u  *buf;
    int	    bufsize;
{
    int	    len;	    /* length of string needing translation */
    int	    room;	    /* room in buffer after string */
    char_u  *new;	    /* translated character */
    int	    new_len;	    /* length of new[] */

    len = STRLEN(buf);
    room = bufsize - len;
    while (*buf)
    {
	new = transchar(*buf);
	new_len = STRLEN(new);
	if (new_len > 1)
	{
	    room -= new_len - 1;
	    if (room <= 0)
		return;
	    mch_memmove(buf + new_len, buf + 1, (size_t)len);
	}
	mch_memmove(buf, new, (size_t)new_len);
	buf += new_len;
	--len;
    }
}

#if defined(WANT_EVAL) || defined(PROTO)
/*
 * Translate a string into allocated memory, replacing special chars with
 * printable chars.  Returns NULL when out of memory.
 */
    char_u *
transstr(s)
    char_u	*s;
{
    char_u	*res;

    res = alloc((unsigned)(vim_strsize(s) + 1));
    if (res != NULL)
    {
	*res = NUL;
	while (*s)
	    STRCAT(res, transchar(*s++));
    }
    return res;
}
#endif

/*
 * Catch 22: chartab[] can't be initialized before the options are
 * initialized, and initializing options may cause transchar() to be called!
 * When chartab_initialized == FALSE don't use chartab[].
 */
    char_u *
transchar(c)
    int	 c;
{
    static char_u   buf[5];
    int		    i;

    i = 0;
    if (IS_SPECIAL(c))	    /* special key code, display as ~@ char */
    {
	buf[0] = '~';
	buf[1] = '@';
	i = 2;
	c = K_SECOND(c);
    }

    if ((!chartab_initialized && ((c >= ' ' && c <= '~')
#ifdef FKMAP
			|| F_ischar(c)
#endif
		)) || (chartab[c] & CHAR_IP))	    /* printable character */
    {
	buf[i] = c;
	buf[i + 1] = NUL;
    }
    else
	transchar_nonprint(buf + i, c);
    return buf;
}

    void
transchar_nonprint(buf, c)
    char_u	*buf;
    int		c;
{
    if (c <= 0x7f)				    /* 0x00 - 0x1f and 0x7f */
    {
	if (c == NL)
	    c = NUL;			/* we use newline in place of a NUL */
	else if (c == CR && get_fileformat(curbuf) == EOL_MAC)
	    c = NL;		/* we use CR in place of  NL in this case */
	buf[0] = '^';
	buf[1] = c ^ 0x40;		/* DEL displayed as ^? */
	buf[2] = NUL;
    }
    else if (c >= ' ' + 0x80 && c <= '~' + 0x80)    /* 0xa0 - 0xfe */
    {
	buf[0] = '|';
	buf[1] = c - 0x80;
	buf[2] = NUL;
    }
    else					    /* 0x80 - 0x9f and 0xff */
    {
	buf[0] = '~';
	buf[1] = (c - 0x80) ^ 0x40;	/* 0xff displayed as ~? */
	buf[2] = NUL;
    }
}

/*
 * return the number of characters 'c' will take on the screen
 * This is used very often, keep it fast!!!
 */
    int
charsize(c)
    int c;
{
    if (IS_SPECIAL(c))
	return (chartab[K_SECOND(c)] & CHAR_MASK) + 2;
    return (chartab[c] & CHAR_MASK);
}

/*
 * Return the number of characters string 's' will take on the screen,
 * counting TABs as two characters: "^I".
 */
    int
vim_strsize(s)
    char_u *s;
{
    int	    len = 0;

    while (*s)
	len += charsize(*s++);
    return len;
}

/*
 * Return the number of characters 'c' will take on the screen, taking
 * into account the size of a tab.
 * Use a define to make it fast, this is used very often!!!
 * Also see getvcol() below.
 */

#define RET_WIN_BUF_CHARTABSIZE(wp, buf, c, col) \
    if ((c) == TAB && (!(wp)->w_p_list || lcs_tab1)) \
    { \
	int ts; \
	ts = (buf)->b_p_ts; \
	return (int)(ts - (col % ts)); \
    } \
    else \
	return charsize(c);

    int
chartabsize(c, col)
    int		c;
    colnr_t	col;
{
    RET_WIN_BUF_CHARTABSIZE(curwin, curbuf, c, col)
}

#ifdef LINEBREAK
    static int
win_chartabsize(wp, c, col)
    WIN		*wp;
    int		c;
    colnr_t	col;
{
    RET_WIN_BUF_CHARTABSIZE(wp, wp->w_buffer, c, col)
}
#endif

/*
 * return the number of characters the string 's' will take on the screen,
 * taking into account the size of a tab
 */
    int
linetabsize(s)
    char_u	*s;
{
    colnr_t	col = 0;

    while (*s != NUL)
	col += lbr_chartabsize(s++, col);
    return (int)col;
}

/*
 * Like linetabsize(), but for a given window instead of the current one.
 */
    int
win_linetabsize(wp, s)
    WIN		*wp;
    char_u	*s;
{
    colnr_t	col = 0;

    while (*s != NUL)
	col += win_lbr_chartabsize(wp, s++, col, NULL);
    return (int)col;
}

/*
 * return TRUE if 'c' is a normal identifier character
 * letters and characters from 'isident' option.
 */
    int
vim_isIDc(c)
    int c;
{
    return (c < 0x100 && (chartab[c] & CHAR_ID));
}

/*
 * return TRUE if 'c' is a keyword character: Letters and characters from
 * 'iskeyword' option for current buffer.
 */
    int
vim_iswordc(c)
    int c;
{
    return (c < 0x100 && curbuf->b_chartab[c]);
}

#if defined(SYNTAX_HL) || defined(PROTO)
    int
vim_iswordc_buf(c, buf)
    int c;
    BUF	*buf;
{
    return (c < 0x100 && buf->b_chartab[c]);
}
#endif

/*
 * return TRUE if 'c' is a valid file-name character
 */
    int
vim_isfilec(c)
    int	c;
{
    return ((c < 0x100 && (chartab[c] & CHAR_IF))
#ifdef MULTI_BYTE
	    /* assume that every leading byte is a filename character */
	    || IsLeadByte(c)
#endif
	   );
}

/*
 * return TRUE if 'c' is a printable character
 */
    int
vim_isprintc(c)
    int c;
{
    return (c < 0x100 && (chartab[c] & CHAR_IP));
}

/*
 * return TRUE if 'c' is a printable character
 *
 * No check for (c < 0x100) to make it a bit faster.
 * This is the most often used function, keep it fast!
 */
    int
safe_vim_isprintc(c)
    int c;
{
    return (chartab[c] & CHAR_IP);
}

/*
 * like chartabsize(), but also check for line breaks on the screen
 */
    int
lbr_chartabsize(s, col)
    unsigned char   *s;
    colnr_t	    col;
{
#ifdef LINEBREAK
    if (!curwin->w_p_lbr && *p_sbr == NUL)
    {
#endif
	RET_WIN_BUF_CHARTABSIZE(curwin, curbuf, *s, col)
#ifdef LINEBREAK
    }
    return win_lbr_chartabsize(curwin, s, col, NULL);
#endif
}

/*
 * This function is used very often, keep it fast!!!!
 *
 * If "head" not NULL, set *head to the size of what we for 'showbreak' string
 * at start of line.  Warning: *head is only set if it's a non-zero value,
 * init to 0 before calling.
 */
/*ARGSUSED*/
    int
win_lbr_chartabsize(wp, s, col, head)
    WIN			*wp;
    unsigned char	*s;
    colnr_t		col;
    int			*head;
{
    int		c = *s;
#ifdef LINEBREAK
    int		size;
    colnr_t	col2;
    colnr_t	colmax;
    int		added;
    int		numberextra;

    /*
     * No 'linebreak' and 'showbreak': return quickly.
     */
    if (!wp->w_p_lbr && *p_sbr == NUL)
    {
#endif
	RET_WIN_BUF_CHARTABSIZE(wp, wp->w_buffer, c, col)
#ifdef LINEBREAK
    }

    /*
     * First get normal size, without 'linebreak'
     */
    size = win_chartabsize(wp, c, col);

    /*
     * If 'linebreak' set check at a blank before a non-blank if the line
     * needs a break here
     */
    if (wp->w_p_lbr && vim_isbreak(c) && !vim_isbreak(s[1])
					     && !wp->w_p_list && wp->w_p_wrap)
    {
	numberextra = wp->w_p_nu? 8: 0;
	/* count all characters from first non-blank after a blank up to next
	 * non-blank after a blank */
	col2 = col;
	colmax = (((col + numberextra) / Columns) + 1) * Columns;
	while ((c = *++s) != NUL && (vim_isbreak(c)
		|| (!vim_isbreak(c) && (col2 == col || !vim_isbreak(s[-1])))))
	{
	    col2 += win_chartabsize(wp, c, col2);
	    if (col2 + numberextra >= colmax)		/* doesn't fit */
	    {
		size = Columns - ((col + numberextra) % Columns);
		break;
	    }
	}
    }

    /*
     * May have to add something for 'showbreak' string at start of line
     * Set *head to the size of what we add.
     */
    added = 0;
    if (*p_sbr != NUL && wp->w_p_wrap && col)
    {
	numberextra = wp->w_p_nu ? 8 : 0;
	col = (col + numberextra) % Columns;
	if (col == 0 || col + size > (colnr_t)Columns)
	{
	    added = STRLEN(p_sbr);
	    size += added;
	    if (col != 0)
		added = 0;
	}
    }
    if (head != NULL)
	*head = added;
    return size;
#endif
}

/*
 * Get virtual column number of pos.
 *  start: on the first position of this character (TAB, ctrl)
 * cursor: where the cursor is on this character (first char, except for TAB)
 *    end: on the last position of this character (TAB, ctrl)
 *
 * This is used very often, keep it fast!
 */
    void
getvcol(wp, pos, start, cursor, end)
    WIN		*wp;
    FPOS	*pos;
    colnr_t	*start;
    colnr_t	*cursor;
    colnr_t	*end;
{
    int		    col;
    colnr_t	    vcol;
    char_u	   *ptr;
#ifdef MULTI_BYTE
    char_u	   *bptr;
#endif
    int		    incr;
    int		    head;
    int		    ts = wp->w_buffer->b_p_ts;
    int		    c;

    vcol = 0;
#ifdef MULTI_BYTE
    bptr =
#endif
    ptr = ml_get_buf(wp->w_buffer, pos->lnum, FALSE);

    /*
     * This function is used very often, do some speed optimizations.
     * When 'list', 'linebreak' and 'showbreak' are not set use a simple loop.
     */
    if ((!wp->w_p_list || lcs_tab1)
#ifdef LINEBREAK
	    && !wp->w_p_lbr && *p_sbr == NUL
#endif
       )
    {
	head = 0;
	for (col = pos->col; ; --col, ++ptr)
	{
	    c = *ptr;
	    /* make sure we don't go past the end of the line */
	    if (c == NUL)
	    {
		incr = 1;	/* NUL at end of line only takes one column */
		break;
	    }
	    /* A tab gets expanded, depending on the current column */
	    if (c == TAB)
		incr = ts - (vcol % ts);
	    else
		incr = CHARSIZE(c);
#ifdef MULTI_BYTE
	    if (wp->w_p_wrap
		    && is_dbcs
		    && ptr - bptr > 0
		    && (int)(vcol % Columns) == (int)Columns - 2
		    && mb_isbyte1(bptr, (int)(ptr - bptr + 1)))
		++incr;
#endif

	    if (col == 0)	/* character at pos.col */
		break;

	    vcol += incr;
	}
    }
    else
    {
	for (col = pos->col; ; --col, ++ptr)
	{
	    /* A tab gets expanded, depending on the current column */
	    head = 0;
	    incr = win_lbr_chartabsize(wp, ptr, vcol, &head);
	    /* make sure we don't go past the end of the line */
	    if (*ptr == NUL)
	    {
		incr = 1;	/* NUL at end of line only takes one column */
		break;
	    }

#ifdef MULTI_BYTE
	    if (wp->w_p_wrap
		    && is_dbcs
		    && ptr - bptr > 0
		    && (int)(vcol % Columns) == (int)Columns - 2
		    && mb_isbyte1(bptr, (int)(ptr - bptr + 1)))
		++incr;
#endif
	    if (col == 0)	/* character at pos.col */
		break;

	    vcol += incr;
	}
    }
    if (start != NULL)
	*start = vcol + head;
    if (end != NULL)
	*end = vcol + incr - 1;
    if (cursor != NULL)
    {
	if (*ptr == TAB && (State & NORMAL) && !wp->w_p_list
					 && !(VIsual_active && *p_sel == 'e'))
	    *cursor = vcol + incr - 1;	    /* cursor at end */
	else
	    *cursor = vcol + head;	    /* cursor at start */
    }
}

/*
 * Get the most left and most right virtual column of pos1 and pos2.
 * Used for Visual block mode.
 */
    void
getvcols(pos1, pos2, left, right)
    FPOS    *pos1, *pos2;
    colnr_t *left, *right;
{
    colnr_t	l1, l2, r1, r2;

    getvcol(curwin, pos1, &l1, NULL, &r1);
    getvcol(curwin, pos2, &l2, NULL, &r2);
    if (l1 < l2)
	*left = l1;
    else
	*left = l2;
    if (r1 > r2)
	*right = r1;
    else
	*right = r2;
}

/*
 * skipwhite: skip over ' ' and '\t'.
 */
    char_u *
skipwhite(p)
    char_u	*p;
{
    while (vim_iswhite(*p)) /* skip to next non-white */
	++p;
    return p;
}

/*
 * skipdigits: skip over digits;
 */
    char_u *
skipdigits(p)
    char_u	*p;
{
    while (isdigit(*p))	/* skip to next non-digit */
	++p;
    return p;
}

/*
 * vim_isdigit: version of isdigit() that can handle characters > 0x100.
 */
    int
vim_isdigit(c)
    int	    c;
{
    return (c > 0 && c < 0x100 && isdigit(c));
}

/*
 * skiptowhite: skip over text until ' ' or '\t' or NUL.
 */
    char_u *
skiptowhite(p)
    char_u	*p;
{
    while (*p != ' ' && *p != '\t' && *p != NUL)
	++p;
    return p;
}

/*
 * skiptowhite_esc: Like skiptowhite(), but also skip escaped chars
 */
    char_u *
skiptowhite_esc(p)
    char_u	*p;
{
    while (*p != ' ' && *p != '\t' && *p != NUL)
    {
	if ((*p == '\\' || *p == Ctrl('V')) && *(p + 1) != NUL)
	    ++p;
	++p;
    }
    return p;
}

/*
 * Getdigits: Get a number from a string and skip over it.
 * Note: the argument is a pointer to a char_u pointer!
 */

    long
getdigits(pp)
    char_u **pp;
{
    char_u	*p;
    long	retval;

    p = *pp;
    retval = atol((char *)p);
    if (*p == '-')		/* skip negative sign */
	++p;
    p = skipdigits(p);		/* skip to next non-digit */
    *pp = p;
    return retval;
}

/*
 * Return TRUE if "lbuf" is empty or only contains blanks.
 */
    int
vim_isblankline(lbuf)
    char_u	*lbuf;
{
    char_u	*p;

    p = skipwhite(lbuf);
    return (*p == NUL || *p == '\r' || *p == '\n');
}

/*
 * Convert a string into a long and/or unsigned long, taking care of
 * hexadecimal and octal numbers.
 * If "hexp" is not NULL, returns a flag to indicate the type of the number:
 *  0	    decimal
 *  '0'	    octal
 *  'X'	    hex
 *  'x'	    hex
 * If "len" is not NULL, the length of the number in characters is returned.
 * If "nptr" is not NULL, the signed result is returned in it.
 * If "unptr" is not NULL, the unsigned result is returned in it.
 */
    void
vim_str2nr(start, hexp, len, dooct, dohex, nptr, unptr)
    char_u		*start;
    int			*hexp;	    /* return: type of number 0 = decimal, 'x'
				       or 'X' is hex, '0' = octal */
    int			*len;	    /* return: detected length of number */
    int			dooct;	    /* recognize octal number */
    int			dohex;	    /* recognize hex number */
    long		*nptr;	    /* return: signed result */
    unsigned long	*unptr;	    /* return: unsigned result */
{
    char_u	    *ptr = start;
    int		    hex = 0;		/* default is decimal */
    int		    negative = FALSE;
    long	    n = 0;
    unsigned long   un = 0;

    if (ptr[0] == '-')
    {
	negative = TRUE;
	++ptr;
    }

    if (ptr[0] == '0')			/* could be hex or octal */
    {
	hex = ptr[1];
	if (dohex && (hex == 'X' || hex == 'x') && isxdigit(ptr[2]))
	    ptr += 2;			/* hexadecimal */
	else
	{
	    if (dooct && isdigit(hex))
		hex = '0';		/* octal */
	    else
		hex = 0;		/* 0 by itself is decimal */
	}
    }

    /*
     * Do the string-to-numeric conversion "manually" to avoid sscanf quirks.
     */
    if (hex)
    {
	if (hex == '0')
	{
	    /* octal */
	    while ('0' <= *ptr && *ptr <= '7')
	    {
		n = 8 * n + (long)(*ptr - '0');
		un = 8 * un + (unsigned long)(*ptr - '0');
		++ptr;
	    }
	}
	else
	{
	    /* hex */
	    while (isxdigit(*ptr))
	    {
		n = 16 * n + (long)hex2nr(*ptr);
		un = 16 * un + (unsigned long)hex2nr(*ptr);
		++ptr;
	    }
	}
    }
    else
    {
	/* decimal */
	while (isdigit(*ptr))
	{
	    n = 10 * n + (long)(*ptr - '0');
	    un = 10 * un + (unsigned long)(*ptr - '0');
	    ++ptr;
	}
    }

    if (!hex && negative)   /* account for leading '-' for decimal numbers */
	n = -n;

    if (hexp != NULL)
	*hexp = hex;
    if (len != NULL)
	*len = ptr - start;
    if (nptr != NULL)
	*nptr = n;
    if (unptr != NULL)
	*unptr = un;
}

/*
 * Return the value of a single hex character.
 * Only valid when the argument is '0' - '9', 'A' - 'F' or 'a' - 'f'.
 */
    int
hex2nr(c)
    int	    c;
{
    if (c >= 'a')
	return c - 'a' + 10;
    if (c >= 'A')
	return c - 'A' + 10;
    return c - '0';
}
