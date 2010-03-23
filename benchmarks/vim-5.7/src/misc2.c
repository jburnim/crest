/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * misc2.c: Various functions.
 */

#include "vim.h"

#ifdef HAVE_FCNTL_H
# include <fcntl.h>	    /* for chdir() */
#endif

/*
 * coladvance(col)
 *
 * Try to advance the Cursor to the specified column.
 *
 * return OK if desired column is reached, FAIL if not
 */
    int
coladvance(wcol)
    colnr_t	    wcol;
{
    int		idx;
    char_u	*ptr;
    colnr_t	col;

    ptr = ml_get_curline();

    /* try to advance to the specified column */
    idx = -1;
    col = 0;
    while (col <= wcol && *ptr)
    {
	++idx;
	/* Count a tab for what it's worth (if list mode not on) */
	col += lbr_chartabsize(ptr, col);
	++ptr;
    }
    /*
     * In Insert mode it is allowed to be one char beyond the end of the line.
     * Also in Visual mode, when 'selection' is not "old".
     */
    if (((State & INSERT) || (VIsual_active && *p_sel != 'o')) && col <= wcol)
	++idx;
    if (idx < 0)
	curwin->w_cursor.col = 0;
    else
	curwin->w_cursor.col = idx;
#ifdef MULTI_BYTE
    /* prevent cursor from moving on the trail byte */
    if (is_dbcs)
	AdjustCursorForMultiByteChar();
#endif

    if (col <= wcol)
	return FAIL;	    /* Couldn't reach column */
    else
	return OK;	    /* Reached column */
}

/*
 * inc(p)
 *
 * Increment the line pointer 'p' crossing line boundaries as necessary.
 * Return 1 when crossing a line, -1 when at end of file, 0 otherwise.
 */
    int
inc_cursor()
{
    return inc(&curwin->w_cursor);
}

    int
inc(lp)
    FPOS  *lp;
{
    char_u  *p = ml_get_pos(lp);

    if (*p != NUL)	/* still within line, move to next char (may be NUL) */
    {
#ifdef MULTI_BYTE
	if (is_dbcs && IsLeadByte(p[0]) && p[1] != NUL)
	{
	    lp->col += 2;
	    return ((p[2] != NUL) ? 0 : 1);
	}
#endif
	lp->col++;
	return ((p[1] != NUL) ? 0 : 1);
    }
    if (lp->lnum != curbuf->b_ml.ml_line_count)     /* there is a next line */
    {
	lp->col = 0;
	lp->lnum++;
	return 1;
    }
    return -1;
}

/*
 * incl(lp): same as inc(), but skip the NUL at the end of non-empty lines
 */
    int
incl(lp)
    FPOS    *lp;
{
    int	    r;

    if ((r = inc(lp)) == 1 && lp->col)
	r = inc(lp);
    return r;
}

/*
 * dec(p)
 *
 * Decrement the line pointer 'p' crossing line boundaries as necessary.
 * Return 1 when crossing a line, -1 when at start of file, 0 otherwise.
 */
    int
dec_cursor()
{
#ifdef MULTI_BYTE
    return (is_dbcs ? mb_dec(&curwin->w_cursor) : dec(&curwin->w_cursor));
#else
    return dec(&curwin->w_cursor);
#endif
}

    int
dec(lp)
    FPOS  *lp;
{
    if (lp->col > 0)
    {		/* still within line */
	lp->col--;
	return 0;
    }
    if (lp->lnum > 1)
    {		/* there is a prior line */
	lp->lnum--;
	lp->col = STRLEN(ml_get(lp->lnum));
	return 1;
    }
    return -1;			/* at start of file */
}

/*
 * decl(lp): same as dec(), but skip the NUL at the end of non-empty lines
 */
    int
decl(lp)
    FPOS    *lp;
{
    int	    r;

    if ((r = dec(lp)) == 1 && lp->col)
	r = dec(lp);
    return r;
}

/*
 * Make sure curwin->w_cursor.lnum is valid.
 */
    void
check_cursor_lnum()
{
    if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
	curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    if (curwin->w_cursor.lnum <= 0)
	curwin->w_cursor.lnum = 1;
}

/*
 * Make sure curwin->w_cursor.col is valid.
 */
    void
check_cursor_col()
{
    colnr_t len;

    len = STRLEN(ml_get_curline());
    if (len == 0)
	curwin->w_cursor.col = 0;
    else if (curwin->w_cursor.col >= len)
    {
	/* Allow cursor past end-of-line in Insert mode, restarting Insert
	 * mode or when in Visual mode and 'selection' isn't "old" */
	if (State & INSERT || restart_edit || (VIsual_active && *p_sel != 'o'))
	    curwin->w_cursor.col = len;
	else
	    curwin->w_cursor.col = len - 1;
    }
}

/*
 * make sure curwin->w_cursor in on a valid character
 */
    void
adjust_cursor()
{
    check_cursor_lnum();
    check_cursor_col();
}

#if defined(TEXT_OBJECTS) || defined(PROTO)
/*
 * Make sure curwin->w_cursor is not on the NUL at the end of the line.
 * Allow it when in Visual mode and 'selection' is not "old".
 */
    void
adjust_cursor_col()
{
    if ((!VIsual_active || *p_sel == 'o')
	    && curwin->w_cursor.col && gchar_cursor() == NUL)
	--curwin->w_cursor.col;
}
#endif

/*
 * When curwin->w_leftcol has changed, adjust the cursor position.
 * Return TRUE if the cursor was moved.
 */
    int
leftcol_changed()
{
    long	lastcol;
    colnr_t	s, e;
    int		retval = FALSE;

    changed_cline_bef_curs();
    lastcol = curwin->w_leftcol + Columns - (curwin->w_p_nu ? 8 : 0) - 1;
    validate_virtcol();

    /*
     * If the cursor is right or left of the screen, move it to last or first
     * character.
     */
    if (curwin->w_virtcol > (colnr_t)lastcol)
    {
	retval = TRUE;
	coladvance((colnr_t)lastcol);
    }
    else if (curwin->w_virtcol < curwin->w_leftcol)
    {
	retval = TRUE;
	(void)coladvance(curwin->w_leftcol);
    }

    /*
     * If the start of the character under the cursor is not on the screen,
     * advance the cursor one more char.  If this fails (last char of the
     * line) adjust the scrolling.
     */
    getvcol(curwin, &curwin->w_cursor, &s, NULL, &e);
    if (e > (colnr_t)lastcol)
    {
	retval = TRUE;
	coladvance(s - 1);
    }
    else if (s < curwin->w_leftcol)
    {
	retval = TRUE;
	if (coladvance(e + 1) == FAIL)	/* there isn't another character */
	{
	    curwin->w_leftcol = s;	/* adjust w_leftcol instead */
	    changed_cline_bef_curs();
	}
    }

    if (retval)
	curwin->w_set_curswant = TRUE;
    redraw_later(NOT_VALID);
    return retval;
}

/**********************************************************************
 * Various routines dealing with allocation and deallocation of memory.
 */

#if defined(MEM_PROFILE) || defined(PROTO)

# define MEM_SIZES  8200
static long_u mem_allocs[MEM_SIZES];
static long_u mem_frees[MEM_SIZES];
static long_u mem_allocated;
static long_u mem_freed;
static long_u mem_peak;
static long_u num_alloc;
static long_u num_freed;

static void mem_pre_alloc_s __ARGS((size_t *sizep));
static void mem_pre_alloc_l __ARGS((long_u *sizep));
static void mem_post_alloc __ARGS((void **pp, size_t size));
static void mem_pre_free __ARGS((void **pp));

    static void
mem_pre_alloc_s(sizep)
    size_t *sizep;
{
    *sizep += sizeof(size_t);
}

    static void
mem_pre_alloc_l(sizep)
    long_u *sizep;
{
    *sizep += sizeof(size_t);
}

    static void
mem_post_alloc(pp, size)
    void **pp;
    size_t size;
{
    if (*pp == NULL)
	return;
    size -= sizeof(size_t);
    *(long_u *)*pp = size;
    if (size <= MEM_SIZES-1)
	mem_allocs[size-1]++;
    else
	mem_allocs[MEM_SIZES-1]++;
    mem_allocated += size;
    if (mem_allocated - mem_freed > mem_peak)
	mem_peak = mem_allocated - mem_freed;
    num_alloc++;
    *pp = (void *)((char *)*pp + sizeof(size_t));
}

    static void
mem_pre_free(pp)
    void **pp;
{
    long_u size;

    *pp = (void *)((char *)*pp - sizeof(size_t));
    size = *(size_t *)*pp;
    if (size <= MEM_SIZES-1)
	mem_frees[size-1]++;
    else
	mem_frees[MEM_SIZES-1]++;
    mem_freed += size;
    num_freed++;
}

/*
 * called on exit via atexit()
 */
    void
vim_mem_profile_dump()
{
    int i, j;

    printf("\r\n");
    j = 0;
    for (i = 0; i < MEM_SIZES - 1; i++)
    {
	if (mem_allocs[i] || mem_frees[i])
	{
	    if (mem_frees[i] > mem_allocs[i])
		printf("\r\nERROR: ");
	    printf("[%4d / %4lu-%-4lu] ", i + 1, mem_allocs[i], mem_frees[i]);
	    j++;
	    if (j > 3)
	    {
		j = 0;
		printf("\r\n");
	    }
	}
    }

    i = MEM_SIZES - 1;
    if (mem_allocs[i])
    {
	printf("\r\n");
	if (mem_frees[i] > mem_allocs[i])
	    printf("ERROR: ");
	printf("[>%d / %4lu-%-4lu]", i, mem_allocs[i], mem_frees[i]);
    }

    printf("\r\n\n[bytes] total alloc-freed %lu-%lu, in use %lu, peak use %lu\r\n",
	    mem_allocated, mem_freed, mem_allocated - mem_freed, mem_peak);
    printf("[calls] total re/malloc()'s %lu, total free()'s %lu\r\n\n",
	    num_alloc, num_freed);
}

#endif /* MEM_PROFILE */

/*
 * Some memory is reserved for error messages and for being able to
 * call mf_release_all(), which needs some memory for mf_trans_add().
 */
#if defined(MSDOS) && !defined(DJGPP)
# define SMALL_MEM
# define KEEP_ROOM 8192L
#else
# define KEEP_ROOM (2 * 8192L)
#endif

static void vim_strup __ARGS((char_u *p));

/*
 * Note: if unsinged is 16 bits we can only allocate up to 64K with alloc().
 * Use lalloc for larger blocks.
 */
    char_u *
alloc(size)
    unsigned	    size;
{
    return (lalloc((long_u)size, TRUE));
}

/*
 * Allocate memory and set all bytes to zero.
 */
    char_u *
alloc_clear(size)
    unsigned	    size;
{
    char_u *p;

    p = (lalloc((long_u)size, TRUE));
    if (p != NULL)
	(void)vim_memset(p, 0, (size_t)size);
    return p;
}

/*
 * alloc() with check for maximum line length
 */
    char_u *
alloc_check(size)
    unsigned	    size;
{
#if !defined(UNIX) && !defined(__EMX__)
    if (sizeof(int) == 2 && size > 0x7fff)
    {
	EMSG("Line is becoming too long");
	return NULL;
    }
#endif
    return (lalloc((long_u)size, TRUE));
}

/*
 * Allocate memory like lalloc() and set all bytes to zero.
 */
    char_u *
lalloc_clear(size, message)
    long_u	size;
    int		message;
{
    char_u *p;

    p = (lalloc(size, message));
    if (p != NULL)
	(void)vim_memset(p, 0, (size_t)size);
    return p;
}

/*
 * Low level memory allocation function.
 * This is used often, KEEP IT FAST!
 */
    char_u *
lalloc(size, message)
    long_u	size;
    int		message;
{
    char_u	*p;		    /* pointer to new storage space */
    static int	releasing = FALSE;  /* don't do mf_release_all() recursive */
    int		try_again;
#if defined(HAVE_AVAIL_MEM) && !defined(SMALL_MEM)
    static long_u allocated = 0;    /* allocated since last avail check */
#endif

    /* Safety check for allocating zero bytes */
    if (size <= 0)
    {
	EMSGN("Internal error: lalloc(%ld, )", size);
	return NULL;
    }

#ifdef MEM_PROFILE
    mem_pre_alloc_l(&size);
#endif

#if defined(MSDOS) && !defined(DJGPP)
    if (size >= 0xfff0)		/* in MSDOS we can't deal with >64K blocks */
	p = NULL;
    else
#endif

    /*
     * Loop when out of memory: Try to release some memfile blocks and
     * if some blocks are released call malloc again.
     */
    for (;;)
    {
	/*
	 * Handle three kind of systems:
	 * 1. No check for available memory: Just return.
	 * 2. Slow check for available memory: call mch_avail_mem() after
	 *    allocating KEEP_ROOM amount of memory.
	 * 3. Strict check for available memory: call mch_avail_mem()
	 */
	if ((p = (char_u *)malloc((size_t)size)) != NULL)
	{
#ifndef HAVE_AVAIL_MEM
	    /* 1. No check for available memory: Just return. */
	    goto theend;
#else
# ifndef SMALL_MEM
	    /* 2. Slow check for available memory: call mch_avail_mem() after
	     *    allocating (KEEP_ROOM / 2) amount of memory. */
	    allocated += size;
	    if (allocated < KEEP_ROOM / 2)
		goto theend;
	    allocated = 0;
# endif
	    /* 3. check for available memory: call mch_avail_mem() */
	    if (mch_avail_mem(TRUE) < KEEP_ROOM && !releasing)
	    {
		vim_free((char *)p);	/* System is low... no go! */
		p = NULL;
	    }
	    else
		goto theend;
#endif
	}
	/*
	 * Remember that mf_release_all() is being called to avoid an endless
	 * loop, because mf_release_all() may call alloc() recursively.
	 */
	if (releasing)
	    break;
	releasing = TRUE;
	try_again = mf_release_all();
	releasing = FALSE;
	if (!try_again)
	    break;
    }

    if (message && p == NULL)
	do_outofmem_msg();

theend:
#ifdef MEM_PROFILE
    mem_post_alloc((void **)&p, (size_t)size);
#endif
    return p;
}

#if defined(MEM_PROFILE) || defined(PROTO)
/*
 * realloc() with memory profiling.
 */
    void *
mem_realloc(ptr, size)
    void *ptr;
    size_t size;
{
    void *p;

    mem_pre_free(&ptr);
    mem_pre_alloc_s(&size);

    p = realloc(ptr, size);

    mem_post_alloc(&p, size);

    return p;
}
#endif

/*
* Avoid repeating the error message many times (they take 1 second each).
* Did_outofmem_msg is reset when a character is read.
*/
    void
do_outofmem_msg()
{
    if (!did_outofmem_msg)
    {
	emsg(e_outofmem);
	did_outofmem_msg = TRUE;
    }
}

/*
 * copy a string into newly allocated memory
 */
    char_u *
vim_strsave(string)
    char_u	*string;
{
    char_u	*p;
    unsigned	len;

    len = STRLEN(string) + 1;
    p = alloc(len);
    if (p != NULL)
	mch_memmove(p, string, (size_t)len);
    return p;
}

    char_u *
vim_strnsave(string, len)
    char_u	*string;
    int		len;
{
    char_u	*p;

    p = alloc((unsigned)(len + 1));
    if (p != NULL)
    {
	STRNCPY(p, string, len);
	p[len] = NUL;
    }
    return p;
}

#if 0	/* not used */
/*
 * like vim_strnsave(), but remove backslashes from the string.
 */
    char_u *
vim_strnsave_esc(string, len)
    char_u	*string;
    int		len;
{
    char_u *p1, *p2;

    p1 = alloc((unsigned) (len + 1));
    if (p1 != NULL)
    {
	STRNCPY(p1, string, len);
	p1[len] = NUL;
	for (p2 = p1; *p2; ++p2)
	    if (*p2 == '\\' && *(p2 + 1) != NUL)
		STRCPY(p2, p2 + 1);
    }
    return p1;
}
#endif

/*
 * Same as vim_strsave(), but any characters found in esc_chars are preceded
 * by a backslash.
 */
    char_u *
vim_strsave_escaped(string, esc_chars)
    char_u	*string;
    char_u	*esc_chars;
{
    char_u	*p;
    char_u	*p2;
    char_u	*escaped_string;
    unsigned	length;

    /*
     * First count the number of backslashes required.
     * Then allocate the memory and insert them.
     */
    length = 1;				/* count the trailing '/' and NUL */
    for (p = string; *p; p++)
    {
#ifdef MULTI_BYTE
	if (is_dbcs && *(p + 1) != NUL && IsLeadByte(*p))
	{
	    length += 2;
	    ++p;	/* skip multibyte */
	    continue;
	}
#endif
	if (vim_strchr(esc_chars, *p) != NULL)
	    ++length;			/* count a backslash */
	++length;			/* count an ordinary char */
    }
    escaped_string = alloc(length);
    if (escaped_string != NULL)
    {
	p2 = escaped_string;
	for (p = string; *p; p++)
	{
#ifdef MULTI_BYTE
	    if (is_dbcs && *(p + 1) != NUL && IsLeadByte(*p))
	    {
		*p2++ = *p++;	/* skip multibyte lead  */
		*p2++ = *p;	/* skip multibyte trail */
		continue;
	    }
#endif
	    if (vim_strchr(esc_chars, *p) != NULL)
		*p2++ = '\\';
	    *p2++ = *p;
	}
	*p2 = NUL;
    }
    return escaped_string;
}

/*
 * Like vim_strsave(), but make all characters uppercase.
 * This uses ASCII lower-to-upper case translation, language independent.
 */
    char_u *
vim_strsave_up(string)
    char_u	*string;
{
    char_u *p1;

    p1 = vim_strsave(string);
    vim_strup(p1);
    return p1;
}

/*
 * Like vim_strnsave(), but make all characters uppercase.
 * This uses ASCII lower-to-upper case translation, language independent.
 */
    char_u *
vim_strnsave_up(string, len)
    char_u	*string;
    int		len;
{
    char_u *p1;

    p1 = vim_strnsave(string, len);
    vim_strup(p1);
    return p1;
}

/*
 * ASCII lower-to-upper case translation, language independent.
 */
    static void
vim_strup(p)
    char_u	*p;
{
    char_u  *p2;
    int	    c;

    if (p != NULL)
    {
	p2 = p;
	while ((c = *p2) != NUL)
	    *p2++ = (c < 'a' || c > 'z') ? c : (c - 0x20);
    }
}

/*
 * copy a space a number of times
 */
    void
copy_spaces(ptr, count)
    char_u	*ptr;
    size_t	count;
{
    size_t	i = count;
    char_u	*p = ptr;

    while (i--)
	*p++ = ' ';
}

#if defined(VISUALEXTRA) || defined(PROTO)
/*
 * copy a character a number of times
 */
    void
copy_chars(ptr, count, c)
    char_u	*ptr;
    size_t	count;
    int		c;
{
    size_t	i = count;
    char_u	*p = ptr;

    while (i--)
	*p++ = c;
}
#endif

/*
 * delete spaces at the end of a string
 */
    void
del_trailing_spaces(ptr)
    char_u	*ptr;
{
    char_u	*q;

    q = ptr + STRLEN(ptr);
    while (--q > ptr && vim_iswhite(q[0]) && q[-1] != '\\' &&
							   q[-1] != Ctrl('V'))
	*q = NUL;
}

/*
 * vim_strncpy()
 *
 * This is here because strncpy() does not guarantee successful results when
 * the to and from strings overlap.  It is only currently called from nextwild()
 * which copies part of the command line to another part of the command line.
 * This produced garbage when expanding files etc in the middle of the command
 * line (on my terminal, anyway) -- webb.
 */
    void
vim_strncpy(to, from, len)
    char_u *to;
    char_u *from;
    int len;
{
    int i;

    if (to <= from)
    {
	while (len-- && *from)
	    *to++ = *from++;
	if (len >= 0)
	    *to = *from;    /* Copy NUL */
    }
    else
    {
	for (i = 0; i < len; i++)
	{
	    to++;
	    if (*from++ == NUL)
	    {
		i++;
		break;
	    }
	}
	for (; i > 0; i--)
	    *--to = *--from;
    }
}

/*
 * Isolate one part of a string option where parts are separated with commas.
 * The part is copied into buf[maxlen].
 * "*option" is advanced to the next part.
 * The length is returned.
 */
    int
copy_option_part(option, buf, maxlen, sep_chars)
    char_u	**option;
    char_u	*buf;
    int		maxlen;
    char	*sep_chars;
{
    int	    len = 0;
    char_u  *p = *option;

    /* skip '.' at start of option part, for 'suffixes' */
    if (*p == '.')
	buf[len++] = *p++;
    while (*p && vim_strchr((char_u *)sep_chars, *p) == NULL)
    {
	/*
	 * Skip backslash before a separator character and space.
	 */
	if (p[0] == '\\' && vim_strchr((char_u *)sep_chars, p[1]) != NULL)
	    ++p;
	if (len < maxlen - 1)
	    buf[len++] = *p;
	++p;
    }
    buf[len] = NUL;

    p = skip_to_option_part(p);	/* p points to next file name */

    *option = p;
    return len;
}

/*
 * replacement for free() that ignores NULL pointers
 */
    void
vim_free(x)
    void *x;
{
    if (x != NULL)
    {
#ifdef MEM_PROFILE
	mem_pre_free(&x);
#endif
	free(x);
    }
}

#ifndef HAVE_MEMSET
    void *
vim_memset(ptr, c, size)
    void    *ptr;
    int	    c;
    size_t  size;
{
    char *p = ptr;

    while (size-- > 0)
	*p++ = c;
    return ptr;
}
#endif

#ifdef VIM_MEMCMP
/*
 * Return zero when "b1" and "b2" are the same for "len" bytes.
 * Return non-zero otherwise.
 */
    int
vim_memcmp(b1, b2, len)
    void    *b1;
    void    *b2;
    size_t  len;
{
    char_u  *p1 = (char_u *)b1, *p2 = (char_u *)b2;

    for ( ; len > 0; --len)
    {
	if (*p1 != *p2)
	    return 1;
	++p1;
	++p2;
    }
    return 0;
}
#endif

#ifdef VIM_MEMMOVE
/*
 * Version of memmove that handles overlapping source and destination.
 * For systems that don't have a function that is guaranteed to do that (SYSV).
 */
    void
mch_memmove(dst_arg, src_arg, len)
    void    *src_arg, *dst_arg;
    size_t  len;
{
    /*
     * A void doesn't have a size, we use char pointers.
     */
    char *dst = dst_arg, *src = src_arg;

					/* overlap, copy backwards */
    if (dst > src && dst < src + len)
    {
	src +=len;
	dst +=len;
	while (len-- > 0)
	    *--dst = *--src;
    }
    else				/* copy forwards */
	while (len-- > 0)
	    *dst++ = *src++;
}
#endif

#if (!defined(HAVE_STRCASECMP) && !defined(HAVE_STRICMP)) || defined(PROTO)
/*
 * Compare two strings, ignoring case.
 * return 0 for match, < 0 for smaller, > 0 for bigger
 */
    int
vim_stricmp(s1, s2)
    char	*s1;
    char	*s2;
{
    int		i;

    for (;;)
    {
	i = (int)TO_LOWER(*s1) - (int)TO_LOWER(*s2);
	if (i != 0)
	    return i;			    /* this character different */
	if (*s1 == NUL)
	    break;			    /* strings match until NUL */
	++s1;
	++s2;
    }
    return 0;				    /* strings match */
}
#endif

#if (!defined(HAVE_STRNCASECMP) && !defined(HAVE_STRNICMP)) || defined(PROTO)
/*
 * Compare two strings, for length "len", ignoring case.
 * return 0 for match, < 0 for smaller, > 0 for bigger
 */
    int
vim_strnicmp(s1, s2, len)
    char	*s1;
    char	*s2;
    size_t	len;
{
    int		i;

    while (len > 0)
    {
	i = (int)TO_LOWER(*s1) - (int)TO_LOWER(*s2);
	if (i != 0)
	    return i;			    /* this character different */
	if (*s1 == NUL)
	    break;			    /* strings match until NUL */
	++s1;
	++s2;
	--len;
    }
    return 0;				    /* strings match */
}
#endif

/*
 * Version of strchr() and strrchr() that handle unsigned char strings
 * with characters above 128 correctly. Also it doesn't return a pointer to
 * the NUL at the end of the string.
 */
    char_u  *
vim_strchr(string, n)
    char_u  *string;
    int	    n;
{
    char_u	*p;
    int		c;

    p = string;
    while ((c = *p) != NUL)
    {
	if (c == n)
	    return p;
	++p;
    }
    return NULL;
}

    char_u  *
vim_strrchr(string, n)
    char_u  *string;
    int	    n;
{
    char_u  *retval = NULL;

    while (*string)
    {
	if (*string == n)
	    retval = string;
	++string;
    }
    return retval;
}

/*
 * Vim's version of strpbrk(), in case it's missing.
 * Don't generate a prototype for this, causes problems when it's not used.
 */
#ifndef PROTO
# ifndef HAVE_STRPBRK
#  ifdef vim_strpbrk
#   undef vim_strpbrk
#  endif
    char_u *
vim_strpbrk(s, charset)
    char_u  *s;
    char_u  *charset;
{
    while (*s)
    {
	if (vim_strchr(charset, *s) != NULL)
	    return s;
	++s;
    }
    return NULL;
}
# endif
#endif

/*
 * Vim has its own isspace() function, because on some machines isspace()
 * can't handle characters above 128.
 */
    int
vim_isspace(x)
    int	    x;
{
    return ((x >= 9 && x <= 13) || x == ' ');
}

/************************************************************************
 * Functions for hanlding growing arrays.
 */

/*
 * Clear an allocated growing array.
 */
    void
ga_clear(gap)
    struct growarray *gap;
{
    vim_free(gap->ga_data);
    ga_init(gap);
}

#if defined(WANT_EVAL) || defined(PROTO)
/*
 * Clear a growing array that contains a list of strings.
 */
    void
ga_clear_strings(gap)
    struct growarray *gap;
{
    int		i;

    for (i = 0; i < gap->ga_len; ++i)
	vim_free(((char_u **)(gap->ga_data))[i]);
    ga_clear(gap);
}
#endif

/*
 * Initialize a growing array.	Don't forget to set ga_itemsize and
 * ga_growsize!  Or use ga_init2().
 */
    void
ga_init(gap)
    struct growarray *gap;
{
    gap->ga_data = NULL;
    gap->ga_room = 0;
    gap->ga_len = 0;
}

    void
ga_init2(gap, itemsize, growsize)
    struct growarray	*gap;
    int			itemsize;
    int			growsize;
{
    ga_init(gap);
    gap->ga_itemsize = itemsize;
    gap->ga_growsize = growsize;
}

/*
 * Make room in growing array "gap" for at least "n" items.
 * Return FAIL for failure, OK otherwise.
 */
    int
ga_grow(gap, n)
    struct growarray	*gap;
    int			n;
{
    size_t	    len;
    char_u	    *pp;

    if (gap->ga_room < n)
    {
	if (n < gap->ga_growsize)
	    n = gap->ga_growsize;
	len = gap->ga_itemsize * (gap->ga_len + n);
	pp = alloc_clear((unsigned)len);
	if (pp == NULL)
	    return FAIL;
	gap->ga_room = n;
	if (gap->ga_data != NULL)
	{
	    mch_memmove(pp, gap->ga_data,
				      (size_t)(gap->ga_itemsize * gap->ga_len));
	    vim_free(gap->ga_data);
	}
	gap->ga_data = pp;
    }
    return OK;
}

#if defined(WANT_EVAL) || defined(CMDLINE_COMPL) || defined(PROTO)
/*
 * Concatenate a string to a growarray which contains characters.
 * Note: Does NOT copy the NUL at the end!
 */
    void
ga_concat(gap, s)
    struct growarray	*gap;
    char_u		*s;
{
    size_t    len = STRLEN(s);

    if (ga_grow(gap, (int)len) == OK)
    {
	mch_memmove((char *)gap->ga_data + gap->ga_len, s, len);
	gap->ga_len += len;
	gap->ga_room -= len;
    }
}

/*
 * Append a character to a growarray which contains characters.
 */
    void
ga_append(gap, c)
    struct growarray	*gap;
    int			c;
{
    if (ga_grow(gap, 1) == OK)
    {
	*((char *)gap->ga_data + gap->ga_len) = c;
	++gap->ga_len;
	--gap->ga_room;
    }
}
#endif

/************************************************************************
 * functions that use lookup tables for various things, generally to do with
 * special key codes.
 */

/*
 * Some useful tables.
 */

static struct modmasktable
{
    int	    mod_mask;	    /* Bit-mask for particular key modifier */
    char_u  name;	    /* Single letter name of modifier */
} mod_mask_table[] =
{
    {MOD_MASK_ALT,	(char_u)'M'},
    {MOD_MASK_CTRL,	(char_u)'C'},
    {MOD_MASK_SHIFT,	(char_u)'S'},
    {MOD_MASK_2CLICK,	(char_u)'2'},
    {MOD_MASK_3CLICK,	(char_u)'3'},
    {MOD_MASK_4CLICK,	(char_u)'4'},
#ifdef macintosh
    {MOD_MASK_CMD,      (char_u)'D'},
#endif
    {0x0,		NUL}
};

/*
 * Shifted key terminal codes and their unshifted equivalent.
 * Don't add mouse codes here, they are handled seperately!
 */
static char_u shifted_keys_table[] =
{
/*  shifted			unshifted  */
    '&', '9',			'@', '1',		/* begin */
    '&', '0',			'@', '2',		/* cancel */
    '*', '1',			'@', '4',		/* command */
    '*', '2',			'@', '5',		/* copy */
    '*', '3',			'@', '6',		/* create */
    '*', '4',			'k', 'D',		/* delete char */
    '*', '5',			'k', 'L',		/* delete line */
    '*', '7',			'@', '7',		/* end */
    '*', '9',			'@', '9',		/* exit */
    '*', '0',			'@', '0',		/* find */
    '#', '1',			'%', '1',		/* help */
    '#', '2',			'k', 'h',		/* home */
    '#', '3',			'k', 'I',		/* insert */
    '#', '4',			'k', 'l',		/* left arrow */
    '%', 'a',			'%', '3',		/* message */
    '%', 'b',			'%', '4',		/* move */
    '%', 'c',			'%', '5',		/* next */
    '%', 'd',			'%', '7',		/* options */
    '%', 'e',			'%', '8',		/* previous */
    '%', 'f',			'%', '9',		/* print */
    '%', 'g',			'%', '0',		/* redo */
    '%', 'h',			'&', '3',		/* replace */
    '%', 'i',			'k', 'r',		/* right arrow */
    '%', 'j',			'&', '5',		/* resume */
    '!', '1',			'&', '6',		/* save */
    '!', '2',			'&', '7',		/* suspend */
    '!', '3',			'&', '8',		/* undo */
    KS_EXTRA, (int)KE_S_UP,	'k', 'u',		/* up arrow */
    KS_EXTRA, (int)KE_S_DOWN,	'k', 'd',		/* down arrow */

    KS_EXTRA, (int)KE_S_XF1,	KS_EXTRA, (int)KE_XF1,	/* vt100 F1 */
    KS_EXTRA, (int)KE_S_XF2,	KS_EXTRA, (int)KE_XF2,
    KS_EXTRA, (int)KE_S_XF3,	KS_EXTRA, (int)KE_XF3,
    KS_EXTRA, (int)KE_S_XF4,	KS_EXTRA, (int)KE_XF4,

    KS_EXTRA, (int)KE_S_F1,	'k', '1',		/* F1 */
    KS_EXTRA, (int)KE_S_F2,	'k', '2',
    KS_EXTRA, (int)KE_S_F3,	'k', '3',
    KS_EXTRA, (int)KE_S_F4,	'k', '4',
    KS_EXTRA, (int)KE_S_F5,	'k', '5',
    KS_EXTRA, (int)KE_S_F6,	'k', '6',
    KS_EXTRA, (int)KE_S_F7,	'k', '7',
    KS_EXTRA, (int)KE_S_F8,	'k', '8',
    KS_EXTRA, (int)KE_S_F9,	'k', '9',
    KS_EXTRA, (int)KE_S_F10,	'k', ';',		/* F10 */

    KS_EXTRA, (int)KE_S_F11,	'F', '1',
    KS_EXTRA, (int)KE_S_F12,	'F', '2',
    KS_EXTRA, (int)KE_S_F13,	'F', '3',
    KS_EXTRA, (int)KE_S_F14,	'F', '4',
    KS_EXTRA, (int)KE_S_F15,	'F', '5',
    KS_EXTRA, (int)KE_S_F16,	'F', '6',
    KS_EXTRA, (int)KE_S_F17,	'F', '7',
    KS_EXTRA, (int)KE_S_F18,	'F', '8',
    KS_EXTRA, (int)KE_S_F19,	'F', '9',
    KS_EXTRA, (int)KE_S_F20,	'F', 'A',

    KS_EXTRA, (int)KE_S_F21,	'F', 'B',
    KS_EXTRA, (int)KE_S_F22,	'F', 'C',
    KS_EXTRA, (int)KE_S_F23,	'F', 'D',
    KS_EXTRA, (int)KE_S_F24,	'F', 'E',
    KS_EXTRA, (int)KE_S_F25,	'F', 'F',
    KS_EXTRA, (int)KE_S_F26,	'F', 'G',
    KS_EXTRA, (int)KE_S_F27,	'F', 'H',
    KS_EXTRA, (int)KE_S_F28,	'F', 'I',
    KS_EXTRA, (int)KE_S_F29,	'F', 'J',
    KS_EXTRA, (int)KE_S_F30,	'F', 'K',

    KS_EXTRA, (int)KE_S_F31,	'F', 'L',
    KS_EXTRA, (int)KE_S_F32,	'F', 'M',
    KS_EXTRA, (int)KE_S_F33,	'F', 'N',
    KS_EXTRA, (int)KE_S_F34,	'F', 'O',
    KS_EXTRA, (int)KE_S_F35,	'F', 'P',

    KS_EXTRA, (int)KE_S_TAB,	KS_EXTRA, (int)KE_TAB,	/* TAB pseudo code*/

    NUL
};

static struct key_name_entry
{
    int	    key;	/* Special key code or ascii value */
    char_u  *name;	/* Name of key */
} key_names_table[] =
{
    {' ',		(char_u *)"Space"},
    {TAB,		(char_u *)"Tab"},
    {K_TAB,		(char_u *)"Tab"},
    {NL,		(char_u *)"NL"},
    {NL,		(char_u *)"NewLine"},	/* Alternative name */
    {NL,		(char_u *)"LineFeed"},	/* Alternative name */
    {NL,		(char_u *)"LF"},	/* Alternative name */
    {CR,		(char_u *)"CR"},
    {CR,		(char_u *)"Return"},	/* Alternative name */
    {K_BS,		(char_u *)"BS"},
    {K_BS,		(char_u *)"BackSpace"},	/* Alternative name */
    {ESC,		(char_u *)"Esc"},
    {CSI,		(char_u *)"CSI"},
    {K_CSI,		(char_u *)"xCSI"},
    {'|',		(char_u *)"Bar"},
    {'\\',		(char_u *)"Bslash"},
    {K_DEL,		(char_u *)"Del"},
    {K_DEL,		(char_u *)"Delete"},	/* Alternative name */
    {K_KDEL,		(char_u *)"kDel"},
    {K_UP,		(char_u *)"Up"},
    {K_DOWN,		(char_u *)"Down"},
    {K_LEFT,		(char_u *)"Left"},
    {K_RIGHT,		(char_u *)"Right"},

    {K_F1,		(char_u *)"F1"},
    {K_F2,		(char_u *)"F2"},
    {K_F3,		(char_u *)"F3"},
    {K_F4,		(char_u *)"F4"},
    {K_F5,		(char_u *)"F5"},
    {K_F6,		(char_u *)"F6"},
    {K_F7,		(char_u *)"F7"},
    {K_F8,		(char_u *)"F8"},
    {K_F9,		(char_u *)"F9"},
    {K_F10,		(char_u *)"F10"},

    {K_F11,		(char_u *)"F11"},
    {K_F12,		(char_u *)"F12"},
    {K_F13,		(char_u *)"F13"},
    {K_F14,		(char_u *)"F14"},
    {K_F15,		(char_u *)"F15"},
    {K_F16,		(char_u *)"F16"},
    {K_F17,		(char_u *)"F17"},
    {K_F18,		(char_u *)"F18"},
    {K_F19,		(char_u *)"F19"},
    {K_F20,		(char_u *)"F20"},

    {K_F21,		(char_u *)"F21"},
    {K_F22,		(char_u *)"F22"},
    {K_F23,		(char_u *)"F23"},
    {K_F24,		(char_u *)"F24"},
    {K_F25,		(char_u *)"F25"},
    {K_F26,		(char_u *)"F26"},
    {K_F27,		(char_u *)"F27"},
    {K_F28,		(char_u *)"F28"},
    {K_F29,		(char_u *)"F29"},
    {K_F30,		(char_u *)"F30"},

    {K_F31,		(char_u *)"F31"},
    {K_F32,		(char_u *)"F32"},
    {K_F33,		(char_u *)"F33"},
    {K_F34,		(char_u *)"F34"},
    {K_F35,		(char_u *)"F35"},

    {K_XF1,		(char_u *)"xF1"},
    {K_XF2,		(char_u *)"xF2"},
    {K_XF3,		(char_u *)"xF3"},
    {K_XF4,		(char_u *)"xF4"},

    {K_HELP,		(char_u *)"Help"},
    {K_UNDO,		(char_u *)"Undo"},
    {K_INS,		(char_u *)"Insert"},
    {K_INS,		(char_u *)"Ins"},	/* Alternative name */
    {K_KINS,		(char_u *)"kInsert"},
    {K_HOME,		(char_u *)"Home"},
    {K_KHOME,		(char_u *)"kHome"},
    {K_XHOME,		(char_u *)"xHome"},
    {K_END,		(char_u *)"End"},
    {K_KEND,		(char_u *)"kEnd"},
    {K_XEND,		(char_u *)"xEnd"},
    {K_PAGEUP,		(char_u *)"PageUp"},
    {K_PAGEDOWN,	(char_u *)"PageDown"},
    {K_KPAGEUP,		(char_u *)"kPageUp"},
    {K_KPAGEDOWN,	(char_u *)"kPageDown"},
    {K_KPLUS,		(char_u *)"kPlus"},
    {K_KMINUS,		(char_u *)"kMinus"},
    {K_KDIVIDE,		(char_u *)"kDivide"},
    {K_KMULTIPLY,	(char_u *)"kMultiply"},
    {K_KENTER,		(char_u *)"kEnter"},

    {'<',		(char_u *)"lt"},

    {K_MOUSE,		(char_u *)"Mouse"},
    {K_LEFTMOUSE,	(char_u *)"LeftMouse"},
    {K_LEFTMOUSE_NM,	(char_u *)"LeftMouseNM"},
    {K_LEFTDRAG,	(char_u *)"LeftDrag"},
    {K_LEFTRELEASE,	(char_u *)"LeftRelease"},
    {K_LEFTRELEASE_NM,	(char_u *)"LeftReleaseNM"},
    {K_MIDDLEMOUSE,	(char_u *)"MiddleMouse"},
    {K_MIDDLEDRAG,	(char_u *)"MiddleDrag"},
    {K_MIDDLERELEASE,	(char_u *)"MiddleRelease"},
    {K_RIGHTMOUSE,	(char_u *)"RightMouse"},
    {K_RIGHTDRAG,	(char_u *)"RightDrag"},
    {K_RIGHTRELEASE,	(char_u *)"RightRelease"},
    {K_MOUSEDOWN,	(char_u *)"MouseDown"},
    {K_MOUSEUP,		(char_u *)"MouseUp"},
    {K_ZERO,		(char_u *)"Nul"},
    {0,			NULL}
};

#define KEY_NAMES_TABLE_LEN (sizeof(key_names_table) / sizeof(struct key_name_entry))

#ifdef USE_MOUSE
static struct mousetable
{
    int	    pseudo_code;	/* Code for pseudo mouse event */
    int	    button;		/* Which mouse button is it? */
    int	    is_click;		/* Is it a mouse button click event? */
    int	    is_drag;		/* Is it a mouse drag event? */
} mouse_table[] =
{
    {(int)KE_LEFTMOUSE,		MOUSE_LEFT,	TRUE,	FALSE},
#ifdef USE_GUI
    {(int)KE_LEFTMOUSE_NM,	MOUSE_LEFT,	TRUE,	FALSE},
#endif
    {(int)KE_LEFTDRAG,		MOUSE_LEFT,	FALSE,	TRUE},
    {(int)KE_LEFTRELEASE,	MOUSE_LEFT,	FALSE,	FALSE},
#ifdef USE_GUI
    {(int)KE_LEFTRELEASE_NM,	MOUSE_LEFT,	FALSE,	FALSE},
#endif
    {(int)KE_MIDDLEMOUSE,	MOUSE_MIDDLE,	TRUE,	FALSE},
    {(int)KE_MIDDLEDRAG,	MOUSE_MIDDLE,	FALSE,	TRUE},
    {(int)KE_MIDDLERELEASE,	MOUSE_MIDDLE,	FALSE,	FALSE},
    {(int)KE_RIGHTMOUSE,	MOUSE_RIGHT,	TRUE,	FALSE},
    {(int)KE_RIGHTDRAG,		MOUSE_RIGHT,	FALSE,	TRUE},
    {(int)KE_RIGHTRELEASE,	MOUSE_RIGHT,	FALSE,	FALSE},
    /* DRAG without CLICK */
    {(int)KE_IGNORE,		MOUSE_RELEASE,	FALSE,	TRUE},
    /* RELEASE without CLICK */
    {(int)KE_IGNORE,		MOUSE_RELEASE,	FALSE,	FALSE},
    {0,				0,		0,	0},
};
#endif /* USE_MOUSE */

/*
 * Return the modifier mask bit (MOD_MASK_*) which corresponds to the given
 * modifier name ('S' for Shift, 'C' for Ctrl etc).
 */
    int
name_to_mod_mask(c)
    int	    c;
{
    int	    i;

    if (c <= 255)	/* avoid TO_LOWER() with number > 255 */
    {
	for (i = 0; mod_mask_table[i].mod_mask; i++)
	    if (TO_LOWER(c) == TO_LOWER(mod_mask_table[i].name))
		return mod_mask_table[i].mod_mask;
    }
    return 0x0;
}

#if 0 /* not used */
/*
 * Decide whether the given key code (K_*) is a shifted special
 * key (by looking at mod_mask).  If it is, then return the appropriate shifted
 * key code, otherwise just return the character as is.
 */
    int
check_shifted_spec_key(c)
    int	    c;
{
    return simplify_key(c, &mod_mask);
}
#endif

/*
 * Check if if there is a special key code for "key" that includes the
 * modifiers specified.
 */
    int
simplify_key(key, modifiers)
    int	    key;
    int	    *modifiers;
{
    int	    i;
    int	    key0;
    int	    key1;

    if (*modifiers & MOD_MASK_SHIFT)
    {
	if (key == TAB)		/* TAB is a special case */
	{
	    *modifiers &= ~MOD_MASK_SHIFT;
	    return K_S_TAB;
	}
	key0 = KEY2TERMCAP0(key);
	key1 = KEY2TERMCAP1(key);
	for (i = 0; shifted_keys_table[i] != NUL; i += 4)
	    if (key0 == shifted_keys_table[i + 2] &&
					    key1 == shifted_keys_table[i + 3])
	    {
		*modifiers &= ~MOD_MASK_SHIFT;
		return TERMCAP2KEY(shifted_keys_table[i],
						   shifted_keys_table[i + 1]);
	    }
    }
    return key;
}

/*
 * Return a string which contains the name of the given key when the given
 * modifiers are down.
 */
    char_u *
get_special_key_name(c, modifiers)
    int	    c;
    int	    modifiers;
{
    static char_u string[MAX_KEY_NAME_LEN + 1];

    int	    i, idx;
    int	    table_idx;
    char_u  *s;

    string[0] = '<';
    idx = 1;

    /*
     * Translate shifted special keys into unshifted keys and set modifier.
     */
    if (IS_SPECIAL(c))
    {
	for (i = 0; shifted_keys_table[i]; i += 4)
	    if (       KEY2TERMCAP0(c) == shifted_keys_table[i]
		    && KEY2TERMCAP1(c) == shifted_keys_table[i + 1])
	    {
		modifiers |= MOD_MASK_SHIFT;
		c = TERMCAP2KEY(shifted_keys_table[i + 2],
						   shifted_keys_table[i + 3]);
		break;
	    }
    }

    /* try to find the key in the special key table */
    table_idx = find_special_key_in_table(c);

    /*
     * When not a known special key, and not a printable character, try to
     * extract modifiers.
     */
    if (table_idx < 0 && (!vim_isprintc(c) || (c & 0x7f) == ' ') && (c & 0x80))
    {
	c &= 0x7f;
	modifiers |= MOD_MASK_ALT;
	/* try again, to find the un-alted key in the special key table */
	table_idx = find_special_key_in_table(c);
    }
    if (table_idx < 0 && !vim_isprintc(c) && c < ' ')
    {
	c += '@';
	modifiers |= MOD_MASK_CTRL;
    }

    /* translate the modifier into a string */
    for (i = 0; mod_mask_table[i].mod_mask; i++)
	if (modifiers & mod_mask_table[i].mod_mask)
	{
	    string[idx++] = mod_mask_table[i].name;
	    string[idx++] = (char_u)'-';
	}

    if (table_idx < 0)		/* unknown special key, output t_xx */
    {
	if (IS_SPECIAL(c))
	{
	    string[idx++] = 't';
	    string[idx++] = '_';
	    string[idx++] = KEY2TERMCAP0(c);
	    string[idx++] = KEY2TERMCAP1(c);
	}
	/* Not a special key, only modifiers, output directly */
	else
	{
	    if (vim_isprintc(c))
		string[idx++] = c;
	    else
	    {
		s = transchar(c);
		while (*s)
		    string[idx++] = *s++;
	    }
	}
    }
    else		/* use name of special key */
    {
	STRCPY(string + idx, key_names_table[table_idx].name);
	idx = STRLEN(string);
    }
    string[idx++] = '>';
    string[idx] = NUL;
    return string;
}

/*
 * Try translating a <> name at (*srcp)[] to dst[].
 * Return the number of characters added to dst[], zero for no match.
 * If there is a match, srcp is advanced to after the <> name.
 * dst[] must be big enough to hold the result (up to six characters)!
 */
    int
trans_special(srcp, dst, keycode)
    char_u	**srcp;
    char_u	*dst;
    int		keycode; /* prefer key code, e.g. K_DEL instead of DEL */
{
    int	    modifiers;
    int	    key;
    int	    dlen = 0;

    key = find_special_key(srcp, &modifiers, keycode);
    if (key == 0)
	return 0;

    /* Put the appropriate modifier in a string */
    if (modifiers != 0)
    {
	dst[dlen++] = K_SPECIAL;
	dst[dlen++] = KS_MODIFIER;
	dst[dlen++] = modifiers;
    }

    if (IS_SPECIAL(key))
    {
	dst[dlen++] = K_SPECIAL;
	dst[dlen++] = KEY2TERMCAP0(key);
	dst[dlen++] = KEY2TERMCAP1(key);
    }
    else
	dst[dlen++] = key;

    return dlen;
}

/*
 * Try translating a <> name at (*srcp)[], return the key and modifiers.
 * srcp is advanced to after the <> name.
 * returns 0 if there is no match.
 */
    int
find_special_key(srcp, modp, keycode)
    char_u	**srcp;
    int		*modp;
    int		keycode; /* prefer key code, e.g. K_DEL instead of DEL */
{
    char_u  *last_dash;
    char_u  *end_of_name;
    char_u  *src;
    char_u  *bp;
    int	    modifiers;
    int	    bit;
    int	    key;

    src = *srcp;
    if (src[0] != '<')
	return 0;

    /* Find end of modifier list */
    last_dash = src;
    for (bp = src + 1; *bp == '-' || vim_isIDc(*bp); bp++)
    {
	if (*bp == '-')
	{
	    last_dash = bp;
	    if (bp[1] != NUL && bp[2] == '>')
		++bp;	/* anything accepted, like <C-?> */
	}
	if (bp[0] == 't' && bp[1] == '_' && bp[2] && bp[3])
	    bp += 3;	/* skip t_xx, xx may be '-' or '>' */
    }

    if (*bp == '>')	/* found matching '>' */
    {
	end_of_name = bp + 1;

	/* Which modifiers are given? */
	modifiers = 0x0;
	for (bp = src + 1; bp < last_dash; bp++)
	{
	    if (*bp != '-')
	    {
		bit = name_to_mod_mask(*bp);
		if (bit == 0x0)
		    break;	/* Illegal modifier name */
		modifiers |= bit;
	    }
	}

	/*
	 * Legal modifier name.
	 */
	if (bp >= last_dash)
	{
	    /*
	     * Modifier with single letter, or special key name.
	     */
	    if (modifiers != 0 && last_dash[2] == '>')
		key = last_dash[1];
	    else
		key = get_special_key_code(last_dash + 1);

	    /*
	     * get_special_key_code() may return NUL for invalid
	     * special key name.
	     */
	    if (key != NUL)
	    {
		/*
		 * Only use a modifier when there is no special key code that
		 * includes the modifier.
		 */
		key = simplify_key(key, &modifiers);

		if (!keycode)
		{
		    /* don't want keycode, use single byte code */
		    if (key == K_BS)
			key = BS;
		    else if (key == K_DEL || key == K_KDEL)
			key = DEL;
		}

		/*
		 * Normal Key with modifier: Try to make a single byte code.
		 */
		if (!IS_SPECIAL(key))
		{
		    if ((modifiers & MOD_MASK_SHIFT) && isalpha(key))
		    {
			key = TO_UPPER(key);
			modifiers &= ~MOD_MASK_SHIFT;
		    }
		    if ((modifiers & MOD_MASK_CTRL)
			    && ((key >= '?' && key <= '_') || isalpha(key)))
		    {
			if (key == '?')
			    key = DEL;
			else
			    key &= 0x1f;
			modifiers &= ~MOD_MASK_CTRL;
		    }
		    if ((modifiers & MOD_MASK_ALT) && key < 0x80)
		    {
			key |= 0x80;
			modifiers &= ~MOD_MASK_ALT;
		    }
		}

		*modp = modifiers;
		*srcp = end_of_name;
		return key;
	    }
	}
    }
    return 0;
}

/*
 * Try to find key "c" in the special key table.
 * Return the index when found, -1 when not found.
 */
    int
find_special_key_in_table(c)
    int	    c;
{
    int	    i;

    for (i = 0; key_names_table[i].name != NULL; i++)
	if (c == key_names_table[i].key)
	    break;
    if (key_names_table[i].name == NULL)
	i = -1;
    return i;
}

/*
 * Find the special key with the given name (the given string does not have to
 * end with NUL, the name is assumed to end before the first non-idchar).
 * If the name starts with "t_" the next two characters are interpreted as a
 * termcap name.
 * Return the key code, or 0 if not found.
 */
    int
get_special_key_code(name)
    char_u  *name;
{
    char_u  *table_name;
    char_u  string[3];
    int	    i, j;

    /*
     * If it's <t_xx> we get the code for xx from the termcap
     */
    if (name[0] == 't' && name[1] == '_' && name[2] != NUL && name[3] != NUL)
    {
	string[0] = name[2];
	string[1] = name[3];
	string[2] = NUL;
	if (add_termcap_entry(string, FALSE) == OK)
	    return TERMCAP2KEY(name[2], name[3]);
    }
    else
	for (i = 0; key_names_table[i].name != NULL; i++)
	{
	    table_name = key_names_table[i].name;
	    for (j = 0; vim_isIDc(name[j]) && table_name[j] != NUL; j++)
		if (TO_LOWER(table_name[j]) != TO_LOWER(name[j]))
		    break;
	    if (!vim_isIDc(name[j]) && table_name[j] == NUL)
		return key_names_table[i].key;
	}
    return 0;
}

#ifdef CMDLINE_COMPL
    char_u *
get_key_name(i)
    int	    i;
{
    if (i >= KEY_NAMES_TABLE_LEN)
	return NULL;
    return  key_names_table[i].name;
}
#endif

#ifdef USE_MOUSE
/*
 * Look up the given mouse code to return the relevant information in the other
 * arguments.  Return which button is down or was released.
 */
    int
get_mouse_button(code, is_click, is_drag)
    int	    code;
    int	    *is_click;
    int	    *is_drag;
{
    int	    i;

    for (i = 0; mouse_table[i].pseudo_code; i++)
	if (code == mouse_table[i].pseudo_code)
	{
	    *is_click = mouse_table[i].is_click;
	    *is_drag = mouse_table[i].is_drag;
	    return mouse_table[i].button;
	}
    return 0;	    /* Shouldn't get here */
}

/*
 * Return the appropriate pseudo mouse event token (KE_LEFTMOUSE etc) based on
 * the given information about which mouse button is down, and whether the
 * mouse was clicked, dragged or released.
 */
    int
get_pseudo_mouse_code(button, is_click, is_drag)
    int	    button;	/* eg MOUSE_LEFT */
    int	    is_click;
    int	    is_drag;
{
    int	    i;

    for (i = 0; mouse_table[i].pseudo_code; i++)
	if (button == mouse_table[i].button
	    && is_click == mouse_table[i].is_click
	    && is_drag == mouse_table[i].is_drag)
	{
#ifdef USE_GUI
	    /* Trick: a non mappable left click and release has mouse_col < 0.
	     * Used for 'mousefocus' in gui_mouse_moved() */
	    if (mouse_col < 0)
	    {
		mouse_col = 0;
		if (mouse_table[i].pseudo_code == (int)KE_LEFTMOUSE)
		    return (int)KE_LEFTMOUSE_NM;
		if (mouse_table[i].pseudo_code == (int)KE_LEFTRELEASE)
		    return (int)KE_LEFTRELEASE_NM;
	    }
#endif
	    return mouse_table[i].pseudo_code;
	}
    return (int)KE_IGNORE;	    /* not recongnized, ignore it */
}
#endif /* USE_MOUSE */

/*
 * Return the current end-of-line type: EOL_DOS, EOL_UNIX or EOL_MAC.
 */
    int
get_fileformat(buf)
    BUF	    *buf;
{
    int	    c = *buf->b_p_ff;

    if (buf->b_p_bin || c == 'u')
	return EOL_UNIX;
    if (c == 'm')
	return EOL_MAC;
    return EOL_DOS;
}

/*
 * Set the current end-of-line type to EOL_DOS, EOL_UNIX or EOL_MAC.
 * Sets both 'textmode' and 'fileformat'.
 */
    void
set_fileformat(t)
    int		t;
{
    switch (t)
    {
    case EOL_DOS:
	set_string_option_direct((char_u *)"ff", -1, (char_u *)FF_DOS, TRUE);
	curbuf->b_p_tx = TRUE;
	break;
    case EOL_UNIX:
	set_string_option_direct((char_u *)"ff", -1, (char_u *)FF_UNIX, TRUE);
	curbuf->b_p_tx = FALSE;
	break;
    case EOL_MAC:
	set_string_option_direct((char_u *)"ff", -1, (char_u *)FF_MAC, TRUE);
	curbuf->b_p_tx = FALSE;
	break;
    }
    check_status(curbuf);
}

/*
 * Return the default fileformat from 'fileformats'.
 */
    int
default_fileformat()
{
    switch (*p_ffs)
    {
	case 'm':   return EOL_MAC;
	case 'd':   return EOL_DOS;
    }
    return EOL_UNIX;
}

/*
 * Call shell.	Calls mch_call_shell, with 'shellxquote' added.
 */
    int
call_shell(cmd, opt)
    char_u	*cmd;
    int		opt;
{
    char_u	*ncmd;
    int		retval;

#ifdef USE_GUI_MSWIN
    /* Don't hide the pointer while executing a shell command. */
    gui_mch_mousehide(FALSE);
#endif
#ifdef USE_GUI
    ++hold_gui_events;
#endif
    /* The external command may update a tags file, clear cached tags. */
    tag_freematch();

    if (cmd == NULL || *p_sxq == NUL)
	retval = mch_call_shell(cmd, opt);
    else
    {
	ncmd = alloc((unsigned)(STRLEN(cmd) + STRLEN(p_sxq) * 2 + 1));
	if (ncmd != NULL)
	{
	    STRCPY(ncmd, p_sxq);
	    STRCAT(ncmd, cmd);
	    STRCAT(ncmd, p_sxq);
	    retval = mch_call_shell(ncmd, opt);
	    vim_free(ncmd);
	}
	else
	    retval = -1;
    }
#ifdef USE_GUI
    --hold_gui_events;
#endif

#ifdef WANT_EVAL
    set_vim_var_nr(VV_SHELL_ERROR, (long)retval);
#endif
    return retval;
}

/*
 * VISUAL and OP_PENDING State are never set, they are equal to NORMAL State
 * with a condition.  This function returns the real State.
 */
    int
get_real_state()
{
    if ((State & NORMAL))
    {
	if (VIsual_active)
	    return VISUAL;
	else if (finish_op)
	    return OP_PENDING;
    }
    return State;
}

#if defined(MKSESSION) || defined(MSWIN) || defined(USE_GUI_GTK) || defined(PROTO)
/*
 * Change to a file's directory.
 */
    int
vim_chdirfile(fname)
    char_u	*fname;
{
    char_u	temp_string[MAXPATHL];
    char_u	*p;
    char_u	*t;

    STRCPY(temp_string, fname);
    p = get_past_head(temp_string);
    t = gettail(temp_string);
    while (t > p && vim_ispathsep(t[-1]))
	--t;
    *t = NUL; /* chop off end of string */

    return mch_chdir((char *)temp_string);
}
#endif

#ifdef CURSOR_SHAPE

/*
 * Handling of cursor shapes in various modes.
 */

struct cursor_entry cursor_table[SHAPE_COUNT] =
{
    /* The values will be filled in from the guicursor' default when the GUI
     * starts. */
    {0,	0, 700L, 400L, 250L, 0, "n"},
    {0,	0, 700L, 400L, 250L, 0, "v"},
    {0,	0, 700L, 400L, 250L, 0, "i"},
    {0,	0, 700L, 400L, 250L, 0, "r"},
    {0,	0, 700L, 400L, 250L, 0, "c"},
    {0,	0, 700L, 400L, 250L, 0, "ci"},
    {0,	0, 700L, 400L, 250L, 0, "cr"},
    {0,	0, 100L, 100L, 100L, 0, "sm"},
    {0,	0, 700L, 400L, 250L, 0, "o"},
    {0,	0, 700L, 400L, 250L, 0, "ve"}
};

/*
 * Parse the 'guicursor' option.
 * Returns error message for an illegal option, NULL otherwise.
 */
    char_u *
parse_guicursor()
{
    char_u	*modep;
    char_u	*colonp;
    char_u	*commap;
    char_u	*p, *endp;
    int		idx = 0;		/* init for GCC */
    int		all_idx;
    int		len;
    int		i;
    long	n;
    int		found_ve = FALSE;	/* found "ve" flag */

    /*
     * Repeat for all comma separated parts.
     */
    modep = p_guicursor;
    while (*modep)
    {
	colonp = vim_strchr(modep, ':');
	if (colonp == NULL)
	    return (char_u *)"Missing colon";
	commap = vim_strchr(modep, ',');

	/*
	 * Repeat for all mode's before the colon.
	 * For the 'a' mode, we loop to handle all the modes.
	 */
	all_idx = -1;
	while (modep < colonp || all_idx >= 0)
	{
	    if (all_idx < 0)
	    {
		/* Find the mode. */
		if (modep[1] == '-' || modep[1] == ':')
		    len = 1;
		else
		    len = 2;
		if (len == 1 && TO_LOWER(modep[0]) == 'a')
		    all_idx = SHAPE_COUNT - 1;
		else
		{
		    for (idx = 0; idx < SHAPE_COUNT; ++idx)
			if (STRNICMP(modep, cursor_table[idx].name, len) == 0)
			    break;
		    if (idx == SHAPE_COUNT)
			return (char_u *)"Illegal mode";
		    if (len == 2 && modep[0] == 'v' && modep[1] == 'e')
			found_ve = TRUE;
		}
		modep += len + 1;
	    }

	    if (all_idx >= 0)
		idx = all_idx--;
	    else
	    {
		/* Set the defaults, for the missing parts */
		cursor_table[idx].shape = SHAPE_BLOCK;
		cursor_table[idx].blinkwait = 700L;
		cursor_table[idx].blinkon = 400L;
		cursor_table[idx].blinkoff = 250L;
	    }

	    /* Parse the part after the colon */
	    for (p = colonp + 1; *p && *p != ','; )
	    {
		/*
		 * First handle the ones with a number argument.
		 */
		i = *p;
		len = 0;
		if (STRNICMP(p, "ver", 3) == 0)
		    len = 3;
		else if (STRNICMP(p, "hor", 3) == 0)
		    len = 3;
		else if (STRNICMP(p, "blinkwait", 9) == 0)
		    len = 9;
		else if (STRNICMP(p, "blinkon", 7) == 0)
		    len = 7;
		else if (STRNICMP(p, "blinkoff", 8) == 0)
		    len = 8;
		if (len)
		{
		    p += len;
		    if (!isdigit(*p))
			return (char_u *)"digit expected";
		    n = getdigits(&p);
		    if (len == 3)   /* "ver" or "hor" */
		    {
			if (n == 0)
			    return (char_u *)"Illegal percentage";
			if (TO_LOWER(i) == 'v')
			    cursor_table[idx].shape = SHAPE_VER;
			else
			    cursor_table[idx].shape = SHAPE_HOR;
			cursor_table[idx].percentage = n;
		    }
		    else if (len == 9)
			cursor_table[idx].blinkwait = n;
		    else if (len == 7)
			cursor_table[idx].blinkon = n;
		    else
			cursor_table[idx].blinkoff = n;
		}
		else if (STRNICMP(p, "block", 5) == 0)
		{
		    cursor_table[idx].shape = SHAPE_BLOCK;
		    p += 5;
		}
		else	/* must be a highlight group name then */
		{
		    endp = vim_strchr(p, '-');
		    if (commap == NULL)		    /* last part */
		    {
			if (endp == NULL)
			    endp = p + STRLEN(p);   /* find end of part */
		    }
		    else if (endp > commap || endp == NULL)
			endp = commap;
		    cursor_table[idx].id = syn_check_group(p, (int)(endp - p));
		    p = endp;
		}
		if (*p == '-')
		    ++p;
	    }
	}
	modep = p;
	if (*modep == ',')
	    ++modep;
    }

    /* If the 's' flag is not given, use the 'v' cursor for 's' */
    if (!found_ve)
    {
	cursor_table[SHAPE_VE].shape = cursor_table[SHAPE_V].shape;
	cursor_table[SHAPE_VE].percentage = cursor_table[SHAPE_V].percentage;
	cursor_table[SHAPE_VE].blinkwait = cursor_table[SHAPE_V].blinkwait;
	cursor_table[SHAPE_VE].blinkon = cursor_table[SHAPE_V].blinkon;
	cursor_table[SHAPE_VE].blinkoff = cursor_table[SHAPE_V].blinkoff;
	cursor_table[SHAPE_VE].id = cursor_table[SHAPE_V].id;
    }

    return NULL;
}

/*
 * Return the index into cursor_table[] for the current mode.
 */
    int
get_cursor_idx()
{
    if (State == SHOWMATCH)
	return SHAPE_SM;
    if (State == INSERT)
	return SHAPE_I;
    if (State == REPLACE)
	return SHAPE_R;
    if (State == VREPLACE)
	return SHAPE_R;
    if (State == CMDLINE)
    {
	if (cmdline_at_end())
	    return SHAPE_C;
	if (cmdline_overstrike())
	    return SHAPE_CR;
	return SHAPE_CI;
    }
    if (finish_op)
	return SHAPE_O;
    if (VIsual_active)
    {
	if (*p_sel == 'e')
	    return SHAPE_VE;
	else
	    return SHAPE_V;
    }
    return SHAPE_N;
}

#endif /* CURSOR_SHAPE */


#ifdef CRYPTV
/*
 * Optional encryption suypport.
 * Mohsin Ahmed, mosh@sasi.com, 98-09-24
 * Based on zip/crypt sources.
 */

/*
 * NOTE FOR USA: It is unclear if exporting this code from the USA is allowed.
 * If you do not want to take any risk, remove this bit of the code (from
 * #ifdef CRYPTV to the matching #endif) and disable the crypt feature.
 * This code was originally created in Europe and India.
 */

/* from zip.h */

typedef unsigned short ush;	/* unsigned 16-bit value */
typedef unsigned long  ulg;	/* unsigned 32-bit value */

/*
 * from util.c
 * Table of CRC-32's of all single byte values (made by makecrc.c)
 */

ulg crc_32_tab[] =
{
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};

#define CRC32(c, b) (crc_32_tab[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))


static ulg keys[3]; /* keys defining the pseudo-random sequence */

/*
 * Return the next byte in the pseudo-random sequence
 */
    int
decrypt_byte()
{
    ush temp;

    temp = (ush)keys[2] | 2;
    return (int)(((unsigned)(temp * (temp ^ 1)) >> 8) & 0xff);
}

/*
 * Update the encryption keys with the next byte of plain text
 */
    int
update_keys(c)
    int c;			/* byte of plain text */
{
    keys[0] = CRC32(keys[0], c);
    keys[1] += keys[0] & 0xff;
    keys[1] = keys[1] * 134775813L + 1;
    keys[2] = CRC32(keys[2], (int)(keys[1] >> 24));
    return c;
}

/*
 * Initialize the encryption keys and the random header according to
 * the given password.
 * If "passwd" is NULL or empty, don't do anything.
 */
    void
crypt_init_keys(passwd)
    char_u *passwd;		/* password string with which to modify keys */
{
    if (passwd && *passwd)
    {
	keys[0] = 305419896L;
	keys[1] = 591751049L;
	keys[2] = 878082192L;
	while (*passwd != '\0')
	    update_keys((int)*passwd++);
    }
}

/*
 * Ask the user for a crypt key.
 * When "store" is TRUE, the new key in stored in the 'key' option, and the
 * 'key' option value is returned: Don't free it.
 * When "store" is FALSE, the typed key is returned in allocated memory.
 * Returns NULL on failure.
 */
    char_u *
get_crypt_key(store)
    int		store;
{
    char_u	*p;

    cmdline_crypt = TRUE;
    cmdline_row = msg_row;
    p = getcmdline_prompt(NUL, (char_u *)"Enter encryption key: ", 0);
    cmdline_crypt = FALSE;

    /* since the user typed this, no need to wait for return */
    need_wait_return = FALSE;
    msg_didout = FALSE;
    if (p != NULL && store)
    {
	set_option_value((char_u *)"key", 0L, p);
	return curbuf->b_p_key;
    }
    return p;
}

#endif /* CRYPTV */

/*
 * Get user name from machine-specific function and cache it.
 * Returns the user name in "buf[len]".
 * Some systems are quite slow in obtaining the user name (Windows NT).
 * Returns OK or FAIL.
 */
    int
get_user_name(buf, len)
    char_u	*buf;
    int		len;
{
    static char_u	*name = NULL;

    if (name == NULL)
    {
	if (mch_get_user_name(buf, len) == FAIL)
	    return FAIL;
	name = vim_strsave(buf);
    }
    else
	STRNCPY(buf, name, len);
    return OK;
}
