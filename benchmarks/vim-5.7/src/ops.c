/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * ops.c: implementation of various operators: op_shift, op_delete, op_tilde,
 *	  op_change, op_yank, do_put, do_join
 */

#include "vim.h"

/*
 * Number of registers.
 *	0 = unnamed register, for normal yanks and puts
 *   1..9 = number registers, for deletes
 * 10..35 = named registers
 *     36 = delete register (-)
 *     37 = Clipboard register (*). Only if USE_CLIPBOARD defined
 */
#ifdef USE_CLIPBOARD
# define NUM_REGISTERS		38
#else
# define NUM_REGISTERS		37
#endif

/*
 * Symbolic names for some registers.
 */
#define DELETION_REGISTER	36
#ifdef USE_CLIPBOARD
# define CLIPBOARD_REGISTER	37
#endif

/*
 * Each yank register is an array of pointers to lines.
 */
static struct yankreg
{
    char_u	**y_array;	/* pointer to array of line pointers */
    linenr_t	y_size;		/* number of lines in y_array */
    char_u	y_type;		/* MLINE, MCHAR or MBLOCK */
} y_regs[NUM_REGISTERS];

static struct yankreg	*y_current;	    /* ptr to current yankreg */
static int		y_append;	    /* TRUE when appending */
static struct yankreg	*y_previous = NULL; /* ptr to last written yankreg */

/*
 * structure used by block_prep, op_delete and op_yank for blockwise operators
 * also op_change, op_shift, op_insert, op_replace - AKelly
 */
struct block_def
{
    int		startspaces;	/* 'extra' cols of first char */
    int		endspaces;	/* 'extra' cols of first char */
    int		textlen;	/* chars in block */
    char_u	*textstart;	/* pointer to 1st char in block */
    colnr_t	textcol;	/* cols of chars (at least part.) in block */
    colnr_t	start_vcol;	/* start col of 1st char wholly inside block */
    colnr_t	end_vcol;	/* start col of 1st char wholly after block */
#ifdef VISUALEXTRA
    int		is_short;	/* TRUE if line is too short to fit in block */
    int		is_MAX;		/* TRUE if curswant==MAXCOL when starting */
    int		is_EOL;		/* TRUE if cursor is on NUL when starting */
    int		is_oneChar;	/* TRUE if block within one character */
    int		pre_whitesp;	/* screen cols of ws before block */
    int		pre_whitesp_c;	/* chars of ws before block */
    colnr_t	end_char_vcols;	/* number of vcols of post-block char */
#endif
    colnr_t	start_char_vcols; /* number of vcols of pre-block char */
};

#ifdef VISUALEXTRA
static void shift_block __ARGS((OPARG *oap, int amount));
static void block_insert __ARGS((OPARG *oap, char_u *s, int b_insert, struct block_def*bdp));
#endif
#ifdef WANT_EVAL
static char_u	*get_expr_line __ARGS((void));
#endif
static void	get_yank_register __ARGS((int regname, int writing));
static int	stuff_yank __ARGS((int, char_u *));
static int	put_in_typebuf __ARGS((char_u *s, int colon));
static void	stuffescaped __ARGS((char_u *arg));
static int	get_spec_reg __ARGS((int regname, char_u **argp, int *allocated, int errmsg));
static void	cmdline_paste_str __ARGS((char_u *s, int literally));
static void	free_yank __ARGS((long));
static void	free_yank_all __ARGS((void));
static void	block_prep __ARGS((OPARG *oap, struct block_def *, linenr_t, int));
#if defined(USE_CLIPBOARD) || defined(WANT_EVAL)
static void	str_to_reg __ARGS((struct yankreg *y_ptr, int type, char_u *str, long len));
#endif
#ifdef COMMENTS
static int	same_leader __ARGS((int, char_u *, int, char_u *));
static int	fmt_check_par __ARGS((linenr_t, int *, char_u **));
#else
static int	fmt_check_par __ARGS((linenr_t));
#endif

/*
 * The names of operators.  Index must correspond with defines in vim.h!!!
 * The third field indicates whether the operator always works on lines.
 */
static char opchars[][3] =
{
    {NUL, NUL, FALSE},	/* OP_NOP */
    {'d', NUL, FALSE},	/* OP_DELETE */
    {'y', NUL, FALSE},	/* OP_YANK */
    {'c', NUL, FALSE},	/* OP_CHANGE */
    {'<', NUL, TRUE},	/* OP_LSHIFT */
    {'>', NUL, TRUE},	/* OP_RSHIFT */
    {'!', NUL, TRUE},	/* OP_FILTER */
    {'g', '~', FALSE},	/* OP_TILDE */
    {'=', NUL, TRUE},	/* OP_INDENT */
    {'g', 'q', TRUE},	/* OP_FORMAT */
    {':', NUL, TRUE},	/* OP_COLON */
    {'g', 'U', FALSE},	/* OP_UPPER */
    {'g', 'u', FALSE},	/* OP_LOWER */
    {'J', NUL, TRUE},	/* DO_JOIN */
    {'g', 'J', TRUE},	/* DO_JOIN_NS */
    {'g', '?', FALSE},	/* OP_ROT13 */
    {'r', NUL, FALSE},	/* OP_REPLACE */
    {'I', NUL, FALSE},	/* OP_INSERT */
    {'A', NUL, FALSE}	/* OP_APPEND */
};

/*
 * Translate a command name into an operator type.
 * Must only be called with a valid operator name!
 */
    int
get_op_type(char1, char2)
    int		char1;
    int		char2;
{
    int		i;

    if (char1 == 'r')		/* ignore second character */
	return OP_REPLACE;
    if (char1 == '~')		/* when tilde is an operator */
	return OP_TILDE;
    for (i = 0; ; ++i)
	if (opchars[i][0] == char1 && opchars[i][1] == char2)
	    break;
    return i;
}

/*
 * Return TRUE if operator "op" always works on whole lines.
 */
    int
op_on_lines(op)
    int op;
{
    return opchars[op][2];
}

/*
 * Get first operator command character.
 * Returns 'g' if there is another command character.
 */
    int
get_op_char(optype)
    int		optype;
{
    return opchars[optype][0];
}

/*
 * Get second operator command character.
 */
    int
get_extra_op_char(optype)
    int		optype;
{
    return opchars[optype][1];
}

/*
 * op_shift - handle a shift operation
 */
    void
op_shift(oap, curs_top, amount)
    OPARG	    *oap;
    int		    curs_top;
    int		    amount;
{
    long	    i;
    int		    first_char;
    int		    block_col = 0;

    if (u_save((linenr_t)(curwin->w_cursor.lnum - 1),
		 (linenr_t)(curwin->w_cursor.lnum + oap->line_count)) == FAIL)
	return;

#ifdef SYNTAX_HL
    /* recompute syntax hl., starting with current line */
    syn_changed(curwin->w_cursor.lnum);
#endif

    if (oap->block_mode)
	block_col = curwin->w_cursor.col;

    for (i = oap->line_count; --i >= 0; )
    {
	first_char = *ml_get_curline();
	if (first_char == NUL)				/* empty line */
	    curwin->w_cursor.col = 0;
#ifdef VISUALEXTRA
	else if (oap->block_mode)
	    shift_block(oap, amount);
#endif
	else
	    /* Move the line right if it doesn't start with '#', 'smartindent'
	     * isn't set or 'cindent' isn't set or '#' isn't in 'cino'. */
#if defined(SMARTINDENT) || defined(CINDENT)
	    if (first_char != '#' || (
# ifdef SMARTINDENT
#  ifdef CINDENT
			 (!curbuf->b_p_si || curbuf->b_p_cin) &&
#  else
			 !curbuf->b_p_si
#  endif
# endif
# ifdef CINDENT
			 (!curbuf->b_p_cin || !in_cinkeys('#', ' ', TRUE))
# endif
					))
#endif
	{
	    shift_line(oap->op_type == OP_LSHIFT, p_sr, amount);
	}
	++curwin->w_cursor.lnum;
    }

    if (oap->block_mode)
    {
	curwin->w_cursor.lnum -= oap->line_count;
	curwin->w_cursor.col = block_col;
    }
    else if (curs_top)	    /* put cursor on first line, for ">>" */
    {
	curwin->w_cursor.lnum -= oap->line_count;
	beginline(BL_SOL | BL_FIX);   /* shift_line() may have set cursor.col */
    }
    else
	--curwin->w_cursor.lnum;	/* put cursor on last line, for ":>" */
    update_topline();
    update_screen(NOT_VALID);

    if (oap->line_count > p_report)
       smsg((char_u *)"%ld line%s %ced %d time%s", oap->line_count,
	      plural(oap->line_count), (oap->op_type == OP_RSHIFT) ? '>' : '<',
			 amount, plural((long)amount));

    /*
     * Set "'[" and "']" marks.
     */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;
}

/*
 * shift the current line one shiftwidth left (if left != 0) or right
 * leaves cursor on first blank in the line
 */
    void
shift_line(left, round, amount)
    int	left;
    int	round;
    int	amount;
{
    int		count;
    int		i, j;
    int		p_sw = (int)curbuf->b_p_sw;

    count = get_indent();	/* get current indent */

    if (round)			/* round off indent */
    {
	i = count / p_sw;	/* number of p_sw rounded down */
	j = count % p_sw;	/* extra spaces */
	if (j && left)		/* first remove extra spaces */
	    --amount;
	if (left)
	{
	    i -= amount;
	    if (i < 0)
		i = 0;
	}
	else
	    i += amount;
	count = i * p_sw;
    }
    else		/* original vi indent */
    {
	if (left)
	{
	    count -= p_sw * amount;
	    if (count < 0)
		count = 0;
	}
	else
	    count += p_sw * amount;
    }

    /* Set new indent */
    if (State == VREPLACE)
	change_indent(INDENT_SET, count, FALSE, NUL);
    else
	set_indent(count, TRUE);
}

#if defined(VISUALEXTRA) || defined(PROTO)
/*
 * shift the current block one shiftwidth right or left
 * leaves cursor on first character in block
 */
    static void
shift_block(oap, amount)
    OPARG	*oap;
    int		amount;
{
    int			left = (oap->op_type == OP_LSHIFT);
    int			oldstate = State;
    int			total, split;
    char_u		*newp, *oldp, *midp, *ptr;
    int			oldcol = curwin->w_cursor.col;
    int			p_sw = (int)curbuf->b_p_sw;
    int			p_ts = (int)curbuf->b_p_ts;
    struct block_def	bd;
    int			internal = 0;
    int			incr;
    colnr_t		vcol, col = 0, ws_vcol;
    int			i = 0, j = 0;

#ifdef RIGHTLEFT
    int			old_p_ri = p_ri;

    p_ri = 0;			/* don't want revins in ident */
#endif

    State = INSERT;		/* don't want REPLACE for State */
    block_prep(oap, &bd, curwin->w_cursor.lnum, TRUE);
    if (bd.is_short)
	return;

    /* total is number of screen columns to be inserted/removed */
    total = amount * p_sw;
    oldp = ml_get_curline();

    if (!left)
    {
	/*
	 *  1. Get start vcol
	 *  2. Total ws vcols
	 *  3. Divvy into TABs & spp
	 *  4. Construct new string
	 */
	total += bd.pre_whitesp; /* all virtual WS upto & incl a split TAB */
	ws_vcol = bd.start_vcol - bd.pre_whitesp;
	if (bd.startspaces)
	    ++bd.textstart;
	for ( ; vim_iswhite(*bd.textstart); bd.textstart++)
	{
	    incr = lbr_chartabsize(bd.textstart, (colnr_t)(bd.start_vcol));
	    total += incr;
	    bd.start_vcol += incr;
	}
	/* OK, now total=all the VWS reqd, and textstart points at the 1st
	 * non-ws char in the block. */
	if (!curbuf->b_p_et)
	    i = ((ws_vcol % p_ts) + total) / p_ts; /* number of tabs */
	if (i)
	    j = ((ws_vcol % p_ts) + total) % p_ts; /* number of spp */
	else
	    j = total;
	/* if we're splitting a TAB, allow for it */
	bd.textcol -= bd.pre_whitesp_c - (bd.startspaces != 0);
	newp = alloc_check(bd.textcol + i + j
				       + (unsigned)STRLEN(bd.textstart) + 1);
	if (newp == NULL)
	    return;
	vim_memset(newp, NUL, (size_t)(bd.textcol + i + j
						 + STRLEN(bd.textstart) + 1));
	mch_memmove(newp, oldp, (size_t)bd.textcol);
	copy_chars(newp + bd.textcol, (size_t)i, TAB);
	copy_spaces(newp + bd.textcol + i, (size_t)j);
	/* the end */
	mch_memmove(newp + bd.textcol + i + j,
			      bd.textstart, (size_t)STRLEN(bd.textstart) + 1);
    }
    else /* left */
    {
	vcol = oap->start_vcol;
	/* walk vcol past ws to be removed */
	for (midp = oldp + bd.textcol;
	      vcol < (oap->start_vcol + total) && vim_iswhite(*midp);
	      ++midp)
	{
	    incr = lbr_chartabsize(midp, (colnr_t)vcol);
	    vcol += incr;
	}
	/* internal is the block-internal ws replacing a split TAB */
	if (vcol > (oap->start_vcol + total))
	{
	    /* we have to split the TAB *(midp-1) */
	    internal = vcol - (oap->start_vcol + total);
	}
	/* if 'expandtab' is not set, use TABs */

	split = bd.startspaces + internal;
	if (split > 0)
	{
	    if (!curbuf->b_p_et)
	    {
		for (ptr = oldp, col = 0; ptr < oldp+bd.textcol; ptr++)
		{
		    col += lbr_chartabsize(ptr, (colnr_t)col);
		}
		/* col+1 now equals the start col of the first char of the
		 * block (may be < oap.start_vcol if we're splitting a TAB) */
		i = ((col % p_ts) + split) / p_ts; /* number of tabs */
	    }
	    if (i)
		j = ((col % p_ts) + split) % p_ts; /* number of spp */
	    else
		j = split;
	}

	newp = alloc_check(bd.textcol + i + j + (unsigned)STRLEN(midp) + 1);
	if (newp == NULL)
	    return;
	vim_memset(newp, NUL, (size_t)(bd.textcol + i + j + STRLEN(midp) + 1));

	/* copy first part we want to keep */
	mch_memmove(newp, oldp, (size_t)bd.textcol);
	/* Now copy any TABS and spp to ensure correct alignment! */
	while (vim_iswhite(*midp))
	{
	    if (*midp == TAB)
		i++;
	    else /*space */
		j++;
	    midp++;
	}
	/* We might have an extra TAB worth of spp now! */
	if (j / p_ts && !curbuf->b_p_et)
	{
	    i++;
	    j -= p_ts;
	}
	copy_chars(newp + bd.textcol, (size_t)i, TAB);
	copy_spaces(newp + bd.textcol + i, (size_t)j);

	/* the end */
	mch_memmove(newp + STRLEN(newp), midp, (size_t)STRLEN(midp) + 1);
    }
    /* replace the line */
    ml_replace(curwin->w_cursor.lnum, newp, FALSE);
    changed();
    State = oldstate;
    curwin->w_cursor.col = oldcol;
#ifdef RIGHTLEFT
    p_ri = old_p_ri;
#endif
}
#endif

#ifdef VISUALEXTRA
/* Insert string s (b_insert ? before : after) block :AKelly */
    static void
block_insert(oap, s, b_insert, bdp)
    OPARG		*oap;
    char_u		*s;
    int			b_insert;
    struct block_def	*bdp;
{
    int		p_ts;
    int		count = 0;	/* extra spaces to replace a cut TAB */
    int		spaces = 0;	/* non-zero if cutting a TAB */
    colnr_t	offset;		/* pointer along new line */
    unsigned	s_len;		/* STRLEN(s) */
    char_u	*newp, *oldp;	/* new, old lines */
    linenr_t	lnum;		/* loop var */
    int		oldstate = State;

    State = INSERT;		/* don't want REPLACE for State */
    s_len = STRLEN(s);

    for (lnum = oap->start.lnum + 1; lnum <= oap->end.lnum; lnum++)
    {
	block_prep(oap, bdp, lnum, TRUE);
	if (bdp->is_short && b_insert)
	    continue;	/* OP_INSERT, line ends before block start */

	oldp = ml_get(lnum);

	if (b_insert)
	{
	    p_ts = bdp->start_char_vcols;
	    if ((spaces = bdp->startspaces) != 0)
		count = p_ts - 1; /* we're cutting a TAB */
	    offset = bdp->textcol;
	}
	else /* append */
	{
	    p_ts =  bdp->end_char_vcols;
	    if (!bdp->is_short) /* spaces = padding after block */
	    {
		if ((spaces = (bdp->endspaces ? p_ts - bdp->endspaces : 0 ))
									 != 0)
		    count = p_ts - 1; /* we're cutting a TAB */
		offset = bdp->textcol + bdp->textlen - (spaces != 0);
	    }
	    else /* spaces = padding to block edge */
	    {
		/* if $ used, just append to EOL (ie spaces==0)
		 * else if cursor was on NUL, we need 1 less padding */
		if (!bdp->is_MAX)
		    spaces = bdp->endspaces + !bdp->is_EOL;
		count = spaces;
		offset = bdp->textcol + bdp->textlen;
	    }
	}

	newp = alloc_check((unsigned)(STRLEN(oldp)) + s_len + count + 1);
	if (newp == NULL)
	    continue;

	/* copy up to shifted part */
	mch_memmove(newp, oldp, (size_t)(offset));

	/* insert pre-padding */
	copy_spaces(newp + offset, (size_t)spaces);

	/* copy the new text */
	mch_memmove(newp + offset + spaces, s, (size_t)s_len);

	oldp += offset;
	if (spaces && !bdp->is_short)
	{
	    /* insert post-padding */
	    copy_spaces(newp + offset + spaces + s_len,
						     (size_t)(p_ts - spaces));
	    /* We're splitting a TAB, don't copy it. */
	    oldp++;
	    /* We allowed for that TAB, remember this now */
	    count++;
	}

	mch_memmove(newp + offset + s_len + (spaces ? count : 0),
					    oldp, (size_t)(STRLEN(oldp) + 1));

	ml_replace(lnum, newp, FALSE);
    } /* for all lnum */

    State = oldstate;
}
#endif

#if defined(LISPINDENT) || defined(CINDENT)
/*
 * op_reindent - handle reindenting a block of lines for C or lisp.
 *
 * mechanism copied from op_shift, above
 */
    void
op_reindent(oap, how)
    OPARG	*oap;
    int		(*how) __ARGS((void));
{
    long	i;
    char_u	*l;
    int		count;

    if (u_save((linenr_t)(curwin->w_cursor.lnum - 1),
		 (linenr_t)(curwin->w_cursor.lnum + oap->line_count)) == FAIL)
	return;

#ifdef SYNTAX_HL
    /* recompute syntax hl., starting with current line */
    syn_changed(curwin->w_cursor.lnum);
#endif

    for (i = oap->line_count; --i >= 0 && !got_int; )
    {
	/* it's a slow thing to do, so give feedback so there's no worry that
	 * the computer's just hung. */

	if (	   (i % 50 == 0
		    || i == oap->line_count - 1)
		&& oap->line_count > p_report)
	    smsg((char_u *)"%ld line%s to indent... ", i, plural(i));

	/*
	 * Be vi-compatible: For lisp indenting the first line is not
	 * indented, unless there is only one line.
	 */
#ifdef LISPINDENT
	if (i != oap->line_count - 1 || oap->line_count == 1 ||
						       how != get_lisp_indent)
#endif
	{
	    l = skipwhite(ml_get_curline());
	    if (*l == NUL)		    /* empty or blank line */
		count = 0;
	    else
		count = how();		    /* get the indent for this line */

	    set_indent(count, TRUE);
	}
	++curwin->w_cursor.lnum;
    }

    /* put cursor on first non-blank of indented line */
    curwin->w_cursor.lnum -= oap->line_count;
    beginline(BL_SOL | BL_FIX);

    update_topline();
    update_screen(NOT_VALID);

    if (oap->line_count > p_report)
    {
	i = oap->line_count - (i + 1);
	smsg((char_u *)"%ld line%s indented ", i, plural(i));
    }
    /* set '[ and '] marks */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;
}
#endif /* defined(LISPINDENT) || defined(CINDENT) */

#ifdef WANT_EVAL
/*
 * Keep the last expression line here, for repeating.
 */
static char_u	*expr_line = NULL;

/*
 * Get an expression for the "\"=expr1" or "CTRL-R =expr1"
 * Returns '=' when OK, NUL otherwise.
 */
    int
get_expr_register()
{
    char_u	*new_line;

    new_line = getcmdline('=', 0L, 0);
    if (new_line == NULL)
	return NUL;
    if (*new_line == NUL)	/* use previous line */
	vim_free(new_line);
    else
	set_expr_line(new_line);
    return '=';
}

/*
 * Set the expression for the '=' register.
 * Argument must be an allocated string.
 */
    void
set_expr_line(new_line)
    char_u	*new_line;
{
    vim_free(expr_line);
    expr_line = new_line;
}

/*
 * Get the result of the '=' register expression.
 * Returns a pointer to allocated memory, or NULL for failure.
 */
    static char_u *
get_expr_line()
{
    if (expr_line == NULL)
	return NULL;
    return eval_to_string(expr_line, NULL);
}
#endif /* WANT_EVAL */

/*
 * Check if 'regname' is a valid name of a yank register.
 * Note: There is no check for 0 (default register), caller should do this
 */
    int
valid_yank_reg(regname, writing)
    int	    regname;
    int	    writing;	    /* if TRUE check for writable registers */
{
    if (regname > '~')
	return FALSE;
    if (       isalnum(regname)
	    || (!writing && vim_strchr((char_u *)
#ifdef WANT_EVAL
				    "/.%#:="
#else
				    "/.%#:"
#endif
					, regname) != NULL)
	    || regname == '"'
	    || regname == '-'
	    || regname == '_'
#ifdef USE_CLIPBOARD
	    || regname == '*'
#endif
							)
	return TRUE;
    return FALSE;
}

/*
 * Set y_current and y_append, according to the value of "regname".
 * Cannot handle the '_' register.
 *
 * If regname is 0 and writing, use register 0
 * If regname is 0 and reading, use previous register
 */
    static void
get_yank_register(regname, writing)
    int	    regname;
    int	    writing;
{
    int	    i;

    y_append = FALSE;
    if (((regname == 0 && !writing) || regname == '"') && y_previous != NULL)
    {
	y_current = y_previous;
	return;
    }
    i = regname;
    if (isdigit(i))
	i -= '0';
    else if (islower(i))
	i -= 'a' - 10;
    else if (isupper(i))
    {
	i -= 'A' - 10;
	y_append = TRUE;
    }
    else if (regname == '-')
	i = DELETION_REGISTER;
#ifdef USE_CLIPBOARD
    /* When clipboard is not available, use register 0 instead of '*' */
    else if (clipboard.available && regname == '*')
	i = CLIPBOARD_REGISTER;
#endif
    else		/* not 0-9, a-z, A-Z or '-': use register 0 */
	i = 0;
    y_current = &(y_regs[i]);
    if (writing)	/* remember the register we write into for do_put() */
	y_previous = y_current;
}

#if defined(USE_MOUSE) || defined(PROTO)
/*
 * return TRUE if the current yank register has type MLINE
 */
    int
yank_register_mline(regname)
    int	    regname;
{
    if (regname != 0 && !valid_yank_reg(regname, FALSE))
	return FALSE;
    if (regname == '_')		/* black hole is always empty */
	return FALSE;
    get_yank_register(regname, FALSE);
    return (y_current->y_type == MLINE);
}
#endif

/*
 * start or stop recording into a yank register
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_record(c)
    int c;
{
    char_u	*p;
    static int	regname;
    struct yankreg *old_y_previous, *old_y_current;
    int		retval;

    if (Recording == FALSE)	    /* start recording */
    {
			/* registers 0-9, a-z and " are allowed */
	if (c > '~' || (!isalnum(c) && c != '"'))
	    retval = FAIL;
	else
	{
	    Recording = TRUE;
	    showmode();
	    regname = c;
	    retval = OK;
	}
    }
    else			    /* stop recording */
    {
	Recording = FALSE;
	MSG("");
	p = get_recorded();
	if (p == NULL)
	    retval = FAIL;
	else
	{
	    /*
	     * We don't want to change the default register here, so save and
	     * restore the current register name.
	     */
	    old_y_previous = y_previous;
	    old_y_current = y_current;

	    retval = stuff_yank(regname, p);

	    y_previous = old_y_previous;
	    y_current = old_y_current;
	}
    }
    return retval;
}

/*
 * Stuff string 'p' into yank register 'regname' as a single line (append if
 * uppercase).	'p' must have been alloced.
 *
 * return FAIL for failure, OK otherwise
 */
    static int
stuff_yank(regname, p)
    int		regname;
    char_u	*p;
{
    char_u	*lp;
    char_u	**pp;

    /* check for read-only register */
    if (regname != 0 && !valid_yank_reg(regname, TRUE))
    {
	vim_free(p);
	return FAIL;
    }
    if (regname == '_')		    /* black hole: don't do anything */
    {
	vim_free(p);
	return OK;
    }
    get_yank_register(regname, TRUE);
    if (y_append && y_current->y_array != NULL)
    {
	pp = &(y_current->y_array[y_current->y_size - 1]);
	lp = lalloc((long_u)(STRLEN(*pp) + STRLEN(p) + 1), TRUE);
	if (lp == NULL)
	{
	    vim_free(p);
	    return FAIL;
	}
	STRCPY(lp, *pp);
	STRCAT(lp, p);
	vim_free(p);
	vim_free(*pp);
	*pp = lp;
    }
    else
    {
	free_yank_all();
	if ((y_current->y_array =
			(char_u **)alloc((unsigned)sizeof(char_u *))) == NULL)
	{
	    vim_free(p);
	    return FAIL;
	}
	y_current->y_array[0] = p;
	y_current->y_size = 1;
	y_current->y_type = MCHAR;  /* used to be MLINE, why? */
    }
    return OK;
}

/*
 * execute a yank register: copy it into the stuff buffer
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_execreg(regname, colon, addcr)
    int	    regname;
    int	    colon;		/* insert ':' before each line */
    int	    addcr;		/* always add '\n' to end of line */
{
    static int	lastc = NUL;
    long	i;
    char_u	*p;
    int		retval = OK;
    int		remap;

    if (regname == '@')			/* repeat previous one */
	regname = lastc;
					/* check for valid regname */
    if (regname == '%' || regname == '#' || !valid_yank_reg(regname, FALSE))
	return FAIL;
    lastc = regname;

    if (regname == '_')			/* black hole: don't stuff anything */
	return OK;

    if (regname == ':')			/* use last command line */
    {
	if (last_cmdline == NULL)
	{
	    EMSG(e_nolastcmd);
	    return FAIL;
	}
	vim_free(new_last_cmdline); /* don't keep the cmdline containing @: */
	new_last_cmdline = NULL;
	retval = put_in_typebuf(last_cmdline, TRUE);
    }
#ifdef WANT_EVAL
    else if (regname == '=')
    {
	p = get_expr_line();
	if (p == NULL)
	    return FAIL;
	retval = put_in_typebuf(p, colon);
	vim_free(p);
    }
#endif
    else if (regname == '.')		/* use last inserted text */
    {
	p = get_last_insert_save();
	if (p == NULL)
	{
	    EMSG(e_noinstext);
	    return FAIL;
	}
	retval = put_in_typebuf(p, colon);
	vim_free(p);
    }
    else
    {
	get_yank_register(regname, FALSE);
	if (y_current->y_array == NULL)
	    return FAIL;

	/* Disallow remaping for ":@r". */
	remap = colon ? -1 : 0;

	/*
	 * Insert lines into typeahead buffer, from last one to first one.
	 */
	for (i = y_current->y_size; --i >= 0; )
	{
	/* insert newline between lines and after last line if type is MLINE */
	    if (y_current->y_type == MLINE || i < y_current->y_size - 1
								     || addcr)
	    {
		if (ins_typebuf((char_u *)"\n", remap, 0, TRUE) == FAIL)
		    return FAIL;
	    }
	    if (ins_typebuf(y_current->y_array[i], remap, 0, TRUE) == FAIL)
		return FAIL;
	    if (colon && ins_typebuf((char_u *)":", remap, 0, TRUE) == FAIL)
		return FAIL;
	}
	Exec_reg = TRUE;	/* disable the 'q' command */
    }
    return retval;
}

    static int
put_in_typebuf(s, colon)
    char_u	*s;
    int		colon;	    /* add ':' before the line */
{
    int		retval = OK;

    if (colon)
	retval = ins_typebuf((char_u *)"\n", FALSE, 0, TRUE);
    if (retval == OK)
	retval = ins_typebuf(s, FALSE, 0, TRUE);
    if (colon && retval == OK)
	retval = ins_typebuf((char_u *)":", FALSE, 0, TRUE);
    return retval;
}

/*
 * Insert a yank register: copy it into the Read buffer.
 * Used by CTRL-R command and middle mouse button in insert mode.
 *
 * return FAIL for failure, OK otherwise
 */
    int
insert_reg(regname, literally)
    int		regname;
    int		literally;	/* insert literally, not as if typed */
{
    long	i;
    int		retval = OK;
    char_u	*arg;
    int		allocated;

    /*
     * It is possible to get into an endless loop by having CTRL-R a in
     * register a and then, in insert mode, doing CTRL-R a.
     * If you hit CTRL-C, the loop will be broken here.
     */
    ui_breakcheck();
    if (got_int)
	return FAIL;

    /* check for valid regname */
    if (regname != NUL && !valid_yank_reg(regname, FALSE))
	return FAIL;

#ifdef USE_CLIPBOARD
    if (regname == '*')
    {
	if (!clipboard.available)
	    regname = 0;
	else
	    clip_get_selection();	/* may fill * register */
    }
#endif

    if (regname == '.')			/* insert last inserted text */
	retval = stuff_inserted(NUL, 1L, TRUE);
    else if (get_spec_reg(regname, &arg, &allocated, TRUE))
    {
	if (arg == NULL)
	    return FAIL;
	if (literally)
	    stuffescaped(arg);
	else
	    stuffReadbuff(arg);
	if (allocated)
	    vim_free(arg);
    }
    else				/* name or number register */
    {
	get_yank_register(regname, FALSE);
	if (y_current->y_array == NULL)
	    retval = FAIL;
	else
	{
	    for (i = 0; i < y_current->y_size; ++i)
	    {
		if (literally)
		    stuffescaped(y_current->y_array[i]);
		else
		    stuffReadbuff(y_current->y_array[i]);
		/*
		 * Insert a newline between lines and after last line if
		 * y_type is MLINE.
		 */
		if (y_current->y_type == MLINE || i < y_current->y_size - 1)
		    stuffcharReadbuff('\n');
	    }
	}
    }

    return retval;
}

/*
 * Stuff a string into the typeahead buffer, such that edit() will insert it
 * literally.
 */
    static void
stuffescaped(arg)
    char_u	*arg;
{
    while (*arg)
    {
	if ((*arg < ' ' && *arg != TAB) || *arg > '~')
	    stuffcharReadbuff(Ctrl('V'));
	stuffcharReadbuff(*arg++);
    }
}

/*
 * If "regname" is a special register, return a pointer to its value.
 */
    static int
get_spec_reg(regname, argp, allocated, errmsg)
    int		regname;
    char_u	**argp;
    int		*allocated;
    int		errmsg;		/* give error message when failing */
{
    int		cnt;

    *argp = NULL;
    *allocated = FALSE;
    switch (regname)
    {
	case '%':		/* file name */
	    if (errmsg)
		check_fname();	/* will give emsg if not set */
	    *argp = curbuf->b_fname;
	    return TRUE;

	case '#':		/* alternate file name */
	    *argp = getaltfname(errmsg);	/* may give emsg if not set */
	    return TRUE;

#ifdef WANT_EVAL
	case '=':		/* result of expression */
	    *argp = get_expr_line();
	    *allocated = TRUE;
	    return TRUE;
#endif

	case ':':		/* last command line */
	    if (last_cmdline == NULL && errmsg)
		EMSG(e_nolastcmd);
	    *argp = last_cmdline;
	    return TRUE;

	case '/':		/* last search-pattern */
	    if (last_search_pat() == NULL && errmsg)
		EMSG(e_noprevre);
	    *argp = last_search_pat();
	    return TRUE;

	case '.':		/* last inserted text */
	    *argp = get_last_insert_save();
	    *allocated = TRUE;
	    if (*argp == NULL && errmsg)
		EMSG(e_noinstext);
	    return TRUE;

#ifdef FILE_IN_PATH
	case Ctrl('F'):		/* Filename under cursor */
	case Ctrl('P'):		/* Path under cursor, expand via "path" */
	    if (!errmsg)
		return FALSE;
	    *argp = file_name_at_cursor(FNAME_MESS | FNAME_HYP
				| (regname == Ctrl('P') ? FNAME_EXP : 0), 1L);
	    *allocated = TRUE;
	    return TRUE;
#endif

	case Ctrl('W'):		/* word under cursor */
	case Ctrl('A'):		/* WORD (mnemonic All) under cursor */
	    if (!errmsg)
		return FALSE;
	    cnt = find_ident_under_cursor(argp, regname == Ctrl('W')
				   ?  (FIND_IDENT|FIND_STRING) : FIND_STRING);
	    *argp = cnt ? vim_strnsave(*argp, cnt) : NULL;
	    *allocated = TRUE;
	    return TRUE;

	case '_':		/* black hole: always empty */
	    *argp = (char_u *)"";
	    return TRUE;
    }

    return FALSE;
}

/*
 * paste a yank register into the command line.
 * used by CTRL-R command in command-line mode
 * insert_reg() can't be used here, because special characters from the
 * register contents will be interpreted as commands.
 *
 * return FAIL for failure, OK otherwise
 */
    int
cmdline_paste(regname, literally)
    int regname;
    int literally;	/* Insert text literally instead of "as typed" */
{
    long	i;
    char_u	*arg;
    int		allocated;

    /* check for valid regname; also accept special characters for CTRL-R in
     * the command line */
    if (regname != Ctrl('F') && regname != Ctrl('P') && regname != Ctrl('W')
	    && regname != Ctrl('A') && !valid_yank_reg(regname, FALSE))
	return FAIL;

#ifdef USE_CLIPBOARD
    if (regname == '*')
    {
	if (!clipboard.available)
	    regname = 0;
	else
	    clip_get_selection();
    }
#endif

    if (get_spec_reg(regname, &arg, &allocated, TRUE))
    {
	if (arg == NULL)
	    return FAIL;
	cmdline_paste_str(arg, literally);
	if (allocated)
	    vim_free(arg);
	return (int)OK;
    }

    get_yank_register(regname, FALSE);
    if (y_current->y_array == NULL)
	return FAIL;

    for (i = 0; i < y_current->y_size; ++i)
    {
	cmdline_paste_str(y_current->y_array[i], literally);

	/* insert ^M between lines and after last line if type is MLINE */
	if (y_current->y_type == MLINE || i < y_current->y_size - 1)
	    cmdline_paste_str((char_u *)"\r", literally);
    }
    return OK;
}

/*
 * Put a string on the command line.
 * When "literally" is TRUE, insert literally.
 * When "literally" is FALSE, insert as typed, but don't leave the command
 * line.
 */
    static void
cmdline_paste_str(s, literally)
    char_u	*s;
    int		literally;
{
    if (literally)
	put_on_cmdline(s, -1, TRUE);
    else
	while (*s)
	{
	    if (*s == Ctrl('V') && s[1])
		stuffcharReadbuff(*s++);
	    else if (*s == ESC || *s == Ctrl('C') || *s == CR || *s == NL
#ifdef UNIX
		    || *s == intr_char
#endif
		    || (*s == Ctrl('\\') && s[1] == Ctrl('N')))
		stuffcharReadbuff(Ctrl('V'));
	    stuffcharReadbuff(*s++);
	}
}

/*
 * op_delete - handle a delete operation
 *
 * return FAIL if undo failed, OK otherwise.
 */
    int
op_delete(oap)
    OPARG   *oap;
{
    int			n;
    linenr_t		lnum;
    char_u		*ptr;
    char_u		*newp, *oldp;
    linenr_t		old_lcount = curbuf->b_ml.ml_line_count;
    int			did_yank = FALSE;
    struct block_def	bd;

    if (curbuf->b_ml.ml_flags & ML_EMPTY)	    /* nothing to do */
	return OK;

    /* Nothing to delete, return here.	Do prepare undo, for op_change(). */
    if (oap->empty)
    {
	return u_save_cursor();
    }

#ifdef USE_CLIPBOARD
    /* If no register specified, and "unnamed" in 'clipboard', use * register */
    if (oap->regname == 0 && vim_strchr(p_cb, 'd') != NULL)
        oap->regname = '*';
    if (!clipboard.available && oap->regname == '*')
	oap->regname = 0;
#endif

#ifdef MULTI_BYTE
    if (is_dbcs)
    {
	char_u *p;

	if (oap->end.col >= oap->start.col)
	{
	    p = ml_get(oap->end.lnum);
	    if (IsTrailByte(p, p + oap->end.col + oap->inclusive))
		oap->end.col++;
	}
	else
	{
	    p = ml_get(oap->start.lnum);
	    if (IsTrailByte(p, p + oap->start.col + oap->inclusive))
		oap->start.col++;
	}
    }
#endif

/*
 * Imitate the strange Vi behaviour: If the delete spans more than one line
 * and motion_type == MCHAR and the result is a blank line, make the delete
 * linewise.  Don't do this for the change command or Visual mode.
 */
    if (       oap->motion_type == MCHAR
	    && !oap->is_VIsual
	    && oap->line_count > 1
	    && oap->op_type == OP_DELETE)
    {
	ptr = ml_get(oap->end.lnum) + oap->end.col + oap->inclusive;
	ptr = skipwhite(ptr);
	if (*ptr == NUL && inindent(0))
	    oap->motion_type = MLINE;
    }

/*
 * Check for trying to delete (e.g. "D") in an empty line.
 * Note: For the change operator it is ok.
 */
    if (       oap->motion_type == MCHAR
	    && oap->line_count == 1
	    && oap->op_type == OP_DELETE
	    && *ml_get(oap->start.lnum) == NUL)
    {
	/*
	 * It's an error to operate on an empty region, when 'E' inclucded in
	 * 'cpoptions' (Vi compatible).
	 */
	if (vim_strchr(p_cpo, CPO_EMPTYREGION) != NULL)
	    beep_flush();
	return OK;
    }

/*
 * Do a yank of whatever we're about to delete.
 * If a yank register was specified, put the deleted text into that register.
 * For the black hole register '_' don't yank anything.
 */
    if (oap->regname != '_')
    {
	if (oap->regname != 0)
	{
	    /* check for read-only register */
	    if (!valid_yank_reg(oap->regname, TRUE))
	    {
		beep_flush();
		return OK;
	    }
	    get_yank_register(oap->regname, TRUE); /* yank into specif'd reg. */
	    if (op_yank(oap, TRUE, FALSE) == OK)   /* yank without message */
		did_yank = TRUE;
	}

	/*
	 * Put deleted text into register 1 and shift number registers if the
	 * delete contains a line break, or when a regname has been specified!
	 */
	if (oap->regname != 0 || oap->motion_type == MLINE
						       || oap->line_count > 1)
	{
	    y_current = &y_regs[9];
	    free_yank_all();			/* free register nine */
	    for (n = 9; n > 1; --n)
		y_regs[n] = y_regs[n - 1];
	    y_previous = y_current = &y_regs[1];
	    y_regs[1].y_array = NULL;		/* set register one to empty */
	    oap->regname = 0;
	}
	else if (oap->regname == 0)		/* yank into unnamed register */
	{
	    oap->regname = '-';		/* use special delete register */
	    get_yank_register(oap->regname, TRUE);
	    oap->regname = 0;
	}

	if (oap->regname == 0 && op_yank(oap, TRUE, FALSE) == OK)
	    did_yank = TRUE;

	/*
	 * If there's too much stuff to fit in the yank register, then get a
	 * confirmation before doing the delete. This is crude, but simple.
	 * And it avoids doing a delete of something we can't put back if we
	 * want.
	 */
	if (!did_yank)
	{
	    if (ask_yesno((char_u *)"cannot yank; delete anyway", TRUE) != 'y')
	    {
		emsg(e_abort);
		return FAIL;
	    }
	}
    }

/*
 * block mode delete
 */
    if (oap->block_mode)
    {
	if (u_save((linenr_t)(oap->start.lnum - 1),
			       (linenr_t)(oap->end.lnum + 1)) == FAIL)
	    return FAIL;

#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with this line */
	syn_changed(curwin->w_cursor.lnum);
#endif
	for (lnum = curwin->w_cursor.lnum;
			       curwin->w_cursor.lnum <= oap->end.lnum;
						      ++curwin->w_cursor.lnum)
	{
	    block_prep(oap, &bd, curwin->w_cursor.lnum, TRUE);
	    if (bd.textlen == 0)	/* nothing to delete */
		continue;

	    /* n == number of chars deleted
	     * If we delete a TAB, it may be replaced by several characters.
	     * Thus the number of characters may increase!
	     */
	    n = bd.textlen - bd.startspaces - bd.endspaces;
	    oldp = ml_get_curline();
	    newp = alloc_check((unsigned)STRLEN(oldp) + 1 - n);
	    if (newp == NULL)
		continue;
	    /* copy up to deleted part */
	    mch_memmove(newp, oldp, (size_t)bd.textcol);
	    /* insert spaces */
	    copy_spaces(newp + bd.textcol,
				     (size_t)(bd.startspaces + bd.endspaces));
	    /* copy the part after the deleted part */
	    oldp += bd.textcol + bd.textlen;
	    mch_memmove(newp + bd.textcol + bd.startspaces + bd.endspaces,
						      oldp, STRLEN(oldp) + 1);
	    /* replace the line */
	    ml_replace(curwin->w_cursor.lnum, newp, FALSE);
	}

	curwin->w_cursor.lnum = lnum;
	changed_cline_bef_curs();	/* recompute cursor pos. on screen */
	approximate_botline();		/* w_botline may be wrong now */
	adjust_cursor();

	changed();
	update_screen(VALID_TO_CURSCHAR);
	oap->line_count = 0;	    /* no lines deleted */
    }
    else if (oap->motion_type == MLINE)
    {
	if (oap->op_type == OP_CHANGE)
	{
	    /* Delete the lines except the first one.  Temporarily move the
	     * cursor to the next line.  Save the current line number, if the
	     * last line is deleted it may be changed.
	     */
	    if (oap->line_count > 1)
	    {
		lnum = curwin->w_cursor.lnum;
		++curwin->w_cursor.lnum;
		del_lines((long)(oap->line_count - 1), TRUE, TRUE);
		curwin->w_cursor.lnum = lnum;
	    }
	    if (u_save_cursor() == FAIL)
		return FAIL;
	    if (curbuf->b_p_ai)		    /* don't delete indent */
	    {
		beginline(BL_WHITE);	    /* cursor on first non-white */
		did_ai = TRUE;		    /* delete the indent when ESC hit */
		ai_col = curwin->w_cursor.col;
	    }
	    else
		beginline(0);		    /* cursor in column 0 */
#ifdef SYNTAX_HL
	    /* recompute syntax hl., starting with current line */
	    syn_changed(curwin->w_cursor.lnum);
#endif
	    truncate_line(FALSE);   /* delete the rest of the line */
				    /* leave cursor past last char in line */
	    if (oap->line_count > 1)
		u_clearline();	    /* "U" command not possible after "2cc" */
	}
	else
	{
	    del_lines(oap->line_count, TRUE, TRUE);
	    beginline(BL_WHITE | BL_FIX);
	    u_clearline();	/* "U" command not possible after "dd" */
	}
    }
    else if (oap->line_count == 1)	/* delete characters within one line */
    {
	if (u_save_cursor() == FAIL)
	    return FAIL;
	/* if 'cpoptions' contains '$', display '$' at end of change */
	if (	   vim_strchr(p_cpo, CPO_DOLLAR) != NULL
		&& oap->op_type == OP_CHANGE
		&& oap->end.lnum == curwin->w_cursor.lnum
		&& !oap->is_VIsual)
	    display_dollar(oap->end.col - !oap->inclusive);
	n = oap->end.col - oap->start.col + 1 - !oap->inclusive;
	(void)del_chars((long)n, restart_edit == NUL);
    }
    else				/* delete characters between lines */
    {
	if (u_save_cursor() == FAIL)	/* save first line for undo */
	    return FAIL;
#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with current line */
	syn_changed(curwin->w_cursor.lnum);
#endif
	truncate_line(TRUE);		/* delete from cursor to end of line */

	oap->start = curwin->w_cursor;	/* remember curwin->w_cursor */
	++curwin->w_cursor.lnum;
					/* includes save for undo */
	del_lines((long)(oap->line_count - 2), TRUE, TRUE);

	if (u_save_cursor() == FAIL)	/* save last line for undo */
	    return FAIL;
	u_clearline();			/* "U" should not be possible now */
	/* delete from start of line until op_end */
	curwin->w_cursor.col = 0;
	(void)del_chars((long)(oap->end.col + 1 - !oap->inclusive),
							 restart_edit == NUL);
	curwin->w_cursor = oap->start;	/* restore curwin->w_cursor */
	(void)do_join(FALSE, TRUE);
    }

    /*
     * For a change within one line, the screen is updated differently (to
     * take care of 'dollar').
     */
    if (oap->motion_type == MCHAR && oap->line_count == 1)
    {
	if (dollar_vcol)
	    must_redraw = 0;	    /* don't want a redraw now */
	else
	    update_screenline();
    }
    else if (!global_busy)	    /* no need to update screen for :global */
    {
	update_topline();
	update_screen(NOT_VALID);
    }

    msgmore(curbuf->b_ml.ml_line_count - old_lcount);

    /*
     * Set "'[" and "']" marks.
     */
    curbuf->b_op_start = oap->start;
    if (oap->block_mode)
    {
	curbuf->b_op_end.lnum = oap->end.lnum;
	curbuf->b_op_end.col = oap->start.col;
    }
    else
	curbuf->b_op_end = oap->start;

    return OK;
}

#if defined(VISUALEXTRA) || defined(PROTO)
/*
 * Replace a whole area with one character.
 */
    int
op_replace(oap, c)
    OPARG   *oap;
    int     c;
{
    int			n;
    linenr_t		lnum;
    char_u		*newp, *oldp;
    linenr_t		old_lcount = curbuf->b_ml.ml_line_count;
    struct block_def	bd;

    if ((curbuf->b_ml.ml_flags & ML_EMPTY ) || oap->empty)
	return OK;	    /* nothing to do */

#ifdef MULTI_BYTE
    if (is_dbcs)
    {
	char_u *p;

	if (oap->end.col >= oap->start.col)
	{
	    p = ml_get(oap->end.lnum);
	    if (IsTrailByte(p, p + oap->end.col + oap->inclusive))
		oap->end.col++;
	}
	else
	{
	    p = ml_get(oap->start.lnum);
	    if (IsTrailByte(p, p + oap->start.col + oap->inclusive))
		oap->start.col++;
	}
    }
#endif

/*
 * block mode replace
 */
    if (oap->block_mode)
    {
	if (u_save((linenr_t)(oap->start.lnum - 1),
			       (linenr_t)(oap->end.lnum + 1)) == FAIL)
	    return FAIL;

#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with this line */
	syn_changed(curwin->w_cursor.lnum);
#endif
	for (lnum = curwin->w_cursor.lnum;
			       curwin->w_cursor.lnum <= oap->end.lnum;
						      ++curwin->w_cursor.lnum)
	{
	    block_prep(oap, &bd, curwin->w_cursor.lnum, TRUE);
	    if (bd.textlen == 0)	/* nothing to delete */
		continue;

	    /* n == number of extra chars required
	     * If we split a TAB, it may be replaced by several characters.
	     * Thus the number of characters may increase!
	     */
	    /* allow for pre spaces */
	    n = (bd.startspaces ? bd.start_char_vcols - 1 : 0);
	    /* allow for post spp */
	    n += (bd.endspaces && !bd.is_oneChar ? bd.end_char_vcols - 1 : 0);
	    n += (oap->end_vcol - oap->start_vcol) - bd.textlen + 1;

	    oldp = ml_get_curline();
	    newp = alloc_check((unsigned)STRLEN(oldp) + 1 + n);
	    if (newp == NULL)
		continue;
	    vim_memset(newp, NUL, (size_t)(STRLEN(oldp) + 1 + n));
	    /* copy up to deleted part */
	    mch_memmove(newp, oldp, (size_t)bd.textcol);
	    oldp += bd.textcol + bd.textlen;
	    /* insert pre-spaces */
	    copy_spaces(newp + bd.textcol, (size_t)bd.startspaces);
	    /* insert replacement chars CHECK FOR ALLOCATED SPACE */
	    copy_chars(newp + STRLEN(newp), (size_t)bd.textlen, c);
	    if (!bd.is_short)
	    {
		/* insert post-spaces */
		copy_spaces(newp + STRLEN(newp), (size_t)bd.endspaces);
		/* copy the part after the changed part */
		mch_memmove(newp + STRLEN(newp), oldp, STRLEN(oldp) + 1);
	    }
	    /* replace the line */
	    ml_replace(curwin->w_cursor.lnum, newp, FALSE);
	}

	curwin->w_cursor.lnum = lnum;
	changed_cline_bef_curs();	/* recompute cursor pos. on screen */
	approximate_botline();		/* w_botline may be wrong now */
	adjust_cursor();

	changed();
	update_screen(VALID_TO_CURSCHAR);
	oap->line_count = 0;	    /* no lines deleted */
    }

    /*
     * For a change within one line, the screen is updated differently (to
     * take care of 'dollar').
     */
    if (oap->motion_type == MCHAR && oap->line_count == 1)
    {
	if (dollar_vcol)
	    must_redraw = 0;	    /* don't want a redraw now */
	else
	    update_screenline();
    }
    else if (!global_busy)	    /* no need to update screen for :global */
    {
	update_topline();
	update_screen(NOT_VALID);
    }

    msgmore(curbuf->b_ml.ml_line_count - old_lcount);

    /*
     * Set "'[" and "']" marks.
     */
    curbuf->b_op_start = oap->start;
    if (oap->block_mode)
    {
	curbuf->b_op_end.lnum = oap->end.lnum;
	curbuf->b_op_end.col = oap->start.col;
    }
    else
	curbuf->b_op_end = oap->start;

    return OK;
}
#endif

/*
 * op_tilde - handle the (non-standard vi) tilde operator
 */
    void
op_tilde(oap)
    OPARG	*oap;
{
    FPOS		pos;
    struct block_def	bd;

    if (u_save((linenr_t)(oap->start.lnum - 1),
				       (linenr_t)(oap->end.lnum + 1)) == FAIL)
	return;

    /*
     * Set '[ and '] marks.
     */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;

    pos = oap->start;
    if (oap->block_mode)		    /* Visual block mode */
    {
	for (; pos.lnum <= oap->end.lnum; ++pos.lnum)
	{
	    block_prep(oap, &bd, pos.lnum, FALSE);
	    pos.col = bd.textcol;
	    while (--bd.textlen >= 0)
	    {
		swapchar(oap->op_type, &pos);
		if (inc(&pos) == -1)	    /* at end of file */
		    break;
	    }
	}
    }
    else				    /* not block mode */
    {
	if (oap->motion_type == MLINE)
	{
	    pos.col = 0;
	    oap->end.col = STRLEN(ml_get(oap->end.lnum));
	    if (oap->end.col)
		--oap->end.col;
	}
	else if (!oap->inclusive)
	    dec(&(oap->end));

	while (ltoreq(pos, oap->end))
	{
	    swapchar(oap->op_type, &pos);
	    if (inc(&pos) == -1)    /* at end of file */
		break;
	}
    }

    if (oap->motion_type == MCHAR && oap->line_count == 1 && !oap->block_mode)
	update_screenline();
    else
    {
#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with current line */
	syn_changed(oap->start.lnum);
#endif
	update_topline();
	update_screen(NOT_VALID);
    }

    if (oap->line_count > p_report)
	smsg((char_u *)"%ld line%s ~ed",
				    oap->line_count, plural(oap->line_count));
}

/*
 * If op_type == OP_UPPER: make uppercase,
 * if op_type == OP_LOWER: make lowercase,
 * else swap case of character at 'pos'
 */
    void
swapchar(op_type, pos)
    int	    op_type;
    FPOS    *pos;
{
    int	    c;
    int	    nc;

    c = gchar_pos(pos);
#ifdef MULTI_BYTE
    if (c & 0xff00)	/* No lower/uppercase letter */
	return;
#endif
    nc = c;
    if (islower(c))
    {
	if (op_type == OP_ROT13)
	    nc = (((c - 'a') + 13) % 26) + 'a';
	else if (op_type != OP_LOWER)
	    nc = TO_UPPER(c);
    }
    else if (isupper(c))
    {
	if (op_type == OP_ROT13)
	    nc = (((c - 'A') + 13) % 26) + 'A';
	else if (op_type != OP_UPPER)
	    nc = TO_LOWER(c);
    }
    if (nc != c)
    {
	pchar(*pos, nc);
	changed();
    }
}

#if defined(VISUALEXTRA) || defined(PROTO)
/*
 * op_insert - Insert and append operators for Visual mode.
 */
    void
op_insert(oap, count1)
    OPARG	*oap;
    long	count1;
{
    long		ins_len, pre_textlen = 0;
    char_u		*firstline, *ins_text;
    struct block_def	bd;
    int			i;

    if (u_save((linenr_t)(oap->start.lnum - 1),
	       (linenr_t)(oap->end.lnum + 1)) == FAIL)
	return;

    /* edit() changes this - record it for OP_APPEND */
    bd.is_MAX = (curwin->w_curswant == MAXCOL);
    /* beyond EOL allowed in VIsual mode */
    bd.is_EOL = (linetabsize(ml_get_curline()) == (int)oap->end_vcol);

    /* vis block is still marked. Get rid of it now. */
    curwin->w_cursor.lnum = oap->start.lnum;
    update_screen(VALID_TO_CURSCHAR);

    if (oap->block_mode)
    {
	/* Get the info about the block before entering the text */
	block_prep(oap, &bd, oap->start.lnum, TRUE);
	firstline = ml_get(oap->start.lnum) + bd.textcol;
	if (oap->op_type == OP_APPEND)
	    firstline += bd.textlen;
	pre_textlen = STRLEN(firstline);
    }

    if (oap->op_type == OP_APPEND)
    {
	if (oap->block_mode)
	{
	    /* this lil bit if code adapted from nv_append() */
	    curwin->w_set_curswant = TRUE;
	    while (inc_cursor() == 0
		    && (curwin->w_cursor.col < bd.textcol + bd.textlen))
		;
	    if (bd.is_short && !bd.is_MAX)
	    {
		/* First line was too short, make it longer and adjust the
		 * values in "bd". */
		for (i = 0; i < bd.endspaces; ++i)
		    ins_char(' ');
		bd.textlen += bd.endspaces;
	    }
	}
	else
	{
	    curwin->w_cursor = oap->end;

	    /* Works just like an 'i'nsert on the next character. */
	    if (!lineempty(curwin->w_cursor.lnum)
		    && oap->start_vcol != oap->end_vcol)
		inc_cursor();
	}
    }

    edit(NUL, FALSE, (linenr_t)count1);

    /* if user has moved off this line, we don't know what to do, so do
     * nothing */
    if (curwin->w_cursor.lnum != oap->start.lnum)
	return;


    if (oap->block_mode)
    {
	struct block_def	bd2;

	/*
	 * Spaces and tabs in the indent may have changed to other spaces and
	 * tabs.  Get the starting column again and correct the lenght.
	 * Don't do this when "$" used, end-of-line will have changed.
	 */
	block_prep(oap, &bd2, oap->start.lnum, TRUE);
	if (!bd.is_MAX || bd2.textlen < bd.textlen)
	{
	    if (oap->op_type == OP_APPEND)
	    {
		pre_textlen += bd2.textlen - bd.textlen;
		if (bd2.endspaces)
		    --bd2.textlen;
	    }
	    bd.textcol = bd2.textcol;
	    bd.textlen = bd2.textlen;
	}

	/*
	 * Subsequent calls to ml_get() flush the firstline data - take a
	 * copy of the required string.
	 */
	firstline = ml_get(oap->start.lnum) + bd.textcol;
	if (oap->op_type == OP_APPEND)
	    firstline += bd.textlen;
	if ((ins_len = STRLEN(firstline) - pre_textlen) > 0)
	{
	    if ((ins_text = alloc_check((unsigned)(ins_len + 1))) != 0)
	    {

		STRNCPY(ins_text, firstline, ins_len);
		*(ins_text + ins_len) = NUL;

		/* block handled here */
		block_insert(oap, ins_text, (oap->op_type == OP_INSERT), &bd);

		curwin->w_cursor.col = oap->start.col;
		changed_cline_bef_curs();	/* recompute cursor posn. */
		approximate_botline();		/* w_botline may be wrong now */
		adjust_cursor();

		changed();
		update_screen(VALID_TO_CURSCHAR);
	    }
	    vim_free(ins_text);
	}
    }
#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with this line */
	syn_changed(oap->start.lnum);
#endif

#if defined(LISPINDENT) || defined(CINDENT)
    if (oap->motion_type == MLINE)
    {
# ifdef LISPINDENT
	if (curbuf->b_p_lisp && curbuf->b_p_ai)
	    fixthisline(get_lisp_indent);
# endif
# if defined(LISPINDENT) && defined(CINDENT)
	else
# endif
# ifdef CINDENT
	if (curbuf->b_p_cin)
	    fixthisline(get_c_indent);
# endif
    }
#endif
}
#endif

/*
 * op_change - handle a change operation
 *
 * return TRUE if edit() returns because of a CTRL-O command
 */
    int
op_change(oap)
    OPARG	*oap;
{
    colnr_t		l;
    int			retval;
#ifdef VISUALEXTRA
    long		offset;
    linenr_t		linenr;
    long		ins_len, pre_textlen = 0;
    char_u		*firstline;
    char_u		*ins_text, *newp, *oldp;
    struct block_def	bd;
#endif

    l = oap->start.col;
    if (oap->motion_type == MLINE)
    {
	l = 0;
#ifdef SMARTINDENT
	if (curbuf->b_p_si
# ifdef CINDENT
		&& !curbuf->b_p_cin
# endif
		)
	    can_si = TRUE;	/* It's like opening a new line, do si */
#endif
    }

    /* First delete the text in the region.  In an empty buffer only need to
     * save for undo */
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
    {
	if (u_save_cursor() == FAIL)
	    return FALSE;
    }
    else if (op_delete(oap) == FAIL)
	return FALSE;

    if ((l > curwin->w_cursor.col) && !lineempty(curwin->w_cursor.lnum))
	inc_cursor();

#ifdef VISUALEXTRA
    /* check for still on same line (<CR> in inserted text meaningless) */
    /* skip blank lines too */
    if (oap->block_mode)
    {
	firstline = ml_get(oap->start.lnum);
	pre_textlen = STRLEN(firstline);
	block_prep(oap, &bd, oap->start.lnum, TRUE);
    }
#endif

#if defined(LISPINDENT) || defined(CINDENT)
    if (oap->motion_type == MLINE)
    {
# ifdef LISPINDENT
	if (curbuf->b_p_lisp && curbuf->b_p_ai)
	    fixthisline(get_lisp_indent);
# endif
# if defined(LISPINDENT) && defined(CINDENT)
	else
# endif
# ifdef CINDENT
	if (curbuf->b_p_cin)
	    fixthisline(get_c_indent);
# endif
    }
#endif

    retval = edit(NUL, FALSE, (linenr_t)1);

#ifdef VISUALEXTRA
    /*
     * In Visual block mode, handle copying the next text to all lines of the
     * block.
     */
    if (oap->block_mode && oap->start.lnum != oap->end.lnum)
    {
	firstline = ml_get(oap->start.lnum);
	/*
	 * Subsequent calls to ml_get() flush the firstline data - take a
	 * copy of the required bit.
	 */
	if ((ins_len = STRLEN(firstline) - pre_textlen) > 0)
	{
	    if ((ins_text = alloc_check((unsigned)(ins_len + 1))) != 0)
	    {
		/* put cursor at end of changed text */
		curwin->w_cursor = oap->end;

		STRNCPY(ins_text, firstline + bd.textcol, ins_len);
		*(ins_text + ins_len) = NUL;
		for (linenr = oap->start.lnum + 1;
		     linenr <= oap->end.lnum;
		     linenr++)
		{
		    block_prep(oap, &bd, linenr, TRUE);
		    if (!bd.is_short)
		    {
			oldp = ml_get(linenr);
			newp = alloc_check((unsigned)(STRLEN(oldp)
							      + ins_len + 1));
			if (newp == NULL)
			    continue;
			/* copy up to block start */
			mch_memmove(newp, oldp, (size_t)bd.textcol);
			for (offset = 0; offset < ins_len; offset++)
			    *(newp + bd.textcol + offset) = *(ins_text
								    + offset);
			oldp += bd.textcol;
			mch_memmove(newp + bd.textcol + offset, oldp,
							    STRLEN(oldp) + 1);
			ml_replace(linenr, newp, FALSE);
			if (linenr == oap->end.lnum)
			    curwin->w_cursor.col = bd.textcol + ins_len - 1;
		    }
		}
		changed_line_abv_curs();	/* recompute cursor posn. */
		approximate_botline();		/* w_botline may be wrong now */
		adjust_cursor();

		changed();
		update_screen(NOT_VALID);
	    }
	    vim_free(ins_text);
	}
#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with this line */
	syn_changed(oap->start.lnum);
#endif
    }
#endif

    return retval;
}

/*
 * set all the yank registers to empty (called from main())
 */
    void
init_yank()
{
    int		i;

    for (i = 0; i < NUM_REGISTERS; ++i)
	y_regs[i].y_array = NULL;
}

/*
 * Free "n" lines from the current yank register.
 * Called for normal freeing and in case of error.
 */
    static void
free_yank(n)
    long	n;
{
    if (y_current->y_array != NULL)
    {
	long	    i;

	for (i = n; --i >= 0; )
	{
#ifdef AMIGA	    /* only for very slow machines */
	    if ((i & 1023) == 1023)  /* this may take a while */
	    {
		/*
		 * This message should never cause a hit-return message.
		 * Overwrite this message with any next message.
		 */
		++no_wait_return;
		smsg((char_u *)"freeing %ld lines", i + 1);
		--no_wait_return;
		msg_didout = FALSE;
		msg_col = 0;
	    }
#endif
	    vim_free(y_current->y_array[i]);
	}
	vim_free(y_current->y_array);
	y_current->y_array = NULL;
#ifdef AMIGA
	if (n >= 1000)
	    MSG("");
#endif
    }
}

    static void
free_yank_all()
{
    free_yank(y_current->y_size);
}

/*
 * Yank the text between curwin->w_cursor and startpos into a yank register.
 * If we are to append (uppercase register), we first yank into a new yank
 * register and then concatenate the old and the new one (so we keep the old
 * one in case of out-of-memory).
 *
 * return FAIL for failure, OK otherwise
 */
    int
op_yank(oap, deleting, mess)
    OPARG   *oap;
    int	    deleting;
    int	    mess;
{
    long		y_idx;		/* index in y_array[] */
    struct yankreg	*curr;		/* copy of y_current */
    struct yankreg	newreg;		/* new yank register when appending */
    char_u		**new_ptr;
    linenr_t		lnum;		/* current line number */
    long		j;
    long		len;
    int			yanktype = oap->motion_type;
    long		yanklines = oap->line_count;
    linenr_t		yankendlnum = oap->end.lnum;
    char_u		*p;
    char_u		*pnew;
    struct block_def	bd;

				    /* check for read-only register */
    if (oap->regname != 0 && !valid_yank_reg(oap->regname, TRUE))
    {
	beep_flush();
	return FAIL;
    }
    if (oap->regname == '_')	    /* black hole: nothing to do */
	return OK;

#ifdef USE_CLIPBOARD
    /* If no register specified, and "unnamed" in 'clipboard', use * register */
    if (!deleting && oap->regname == 0 && vim_strchr(p_cb, 'd') != NULL)
        oap->regname = '*';
    if (!clipboard.available && oap->regname == '*')
	oap->regname = 0;
#endif

    if (!deleting)		    /* op_delete() already set y_current */
	get_yank_register(oap->regname, TRUE);

    curr = y_current;
				    /* append to existing contents */
    if (y_append && y_current->y_array != NULL)
	y_current = &newreg;
    else
	free_yank_all();	    /* free previously yanked lines */

/*
 * If the cursor was in column 1 before and after the movement, and the
 * operator is not inclusive, the yank is always linewise.
 */
    if (       oap->motion_type == MCHAR
	    && oap->start.col == 0
	    && !oap->inclusive
	    && (!oap->is_VIsual || *p_sel == 'o')
	    && oap->end.col == 0
	    && yanklines > 1)
    {
	yanktype = MLINE;
	--yankendlnum;
	--yanklines;
    }

    y_current->y_size = yanklines;
    y_current->y_type = yanktype;   /* set the yank register type */
    y_current->y_array = (char_u **)lalloc_clear((long_u)(sizeof(char_u *) *
							    yanklines), TRUE);

    if (y_current->y_array == NULL)
    {
	y_current = curr;
	return FAIL;
    }

    y_idx = 0;
    lnum = oap->start.lnum;

/*
 * Visual block mode
 */
    if (oap->block_mode)
    {
	y_current->y_type = MBLOCK;	    /* set the yank register type */
	for ( ; lnum <= yankendlnum; ++lnum)
	{
	    block_prep(oap, &bd, lnum, FALSE);

	    if ((pnew = alloc(bd.startspaces + bd.endspaces +
					  bd.textlen + 1)) == NULL)
		goto fail;
	    y_current->y_array[y_idx++] = pnew;

	    copy_spaces(pnew, (size_t)bd.startspaces);
	    pnew += bd.startspaces;

	    mch_memmove(pnew, bd.textstart, (size_t)bd.textlen);
	    pnew += bd.textlen;

	    copy_spaces(pnew, (size_t)bd.endspaces);
	    pnew += bd.endspaces;

	    *pnew = NUL;
	}
    }
    else
    {
	/*
	 * there are three parts for non-block mode:
	 * 1. if yanktype != MLINE yank last part of the top line
	 * 2. yank the lines between op_start and op_end, inclusive when
	 *    yanktype == MLINE
	 * 3. if yanktype != MLINE yank first part of the bot line
	 */
	if (yanktype != MLINE)
	{
	    if (yanklines == 1)	    /* op_start and op_end on same line */
	    {
		j = oap->end.col - oap->start.col + 1 - !oap->inclusive;
		/* Watch out for very big endcol (MAXCOL) */
		p = ml_get(lnum) + oap->start.col;
		len = STRLEN(p);
		if (j > len || j < 0)
		    j = len;
		if ((y_current->y_array[0] = vim_strnsave(p, (int)j)) == NULL)
		{
fail:
		    /* free the allocated lines */
		    free_yank(y_idx);
		    y_current = curr;
		    return FAIL;
		}
		goto success;
	    }
	    if ((y_current->y_array[0] =
			vim_strsave(ml_get(lnum++) + oap->start.col)) == NULL)
		goto fail;
	    ++y_idx;
	}

	while (yanktype == MLINE ? (lnum <= yankendlnum) : (lnum < yankendlnum))
	{
	    if ((y_current->y_array[y_idx] =
					 vim_strsave(ml_get(lnum++))) == NULL)
		goto fail;
	    ++y_idx;
	}
	if (yanktype != MLINE)
	{
	    if ((y_current->y_array[y_idx] = vim_strnsave(ml_get(yankendlnum),
				 oap->end.col + 1 - !oap->inclusive)) == NULL)
		goto fail;
	}
    }

success:
    if (curr != y_current)	/* append the new block to the old block */
    {
	new_ptr = (char_u **)lalloc((long_u)(sizeof(char_u *) *
				   (curr->y_size + y_current->y_size)), TRUE);
	if (new_ptr == NULL)
	    goto fail;
	for (j = 0; j < curr->y_size; ++j)
	    new_ptr[j] = curr->y_array[j];
	vim_free(curr->y_array);
	curr->y_array = new_ptr;

	if (yanktype == MLINE)	/* MLINE overrides MCHAR and MBLOCK */
	    curr->y_type = MLINE;

	/* concatenate the last line of the old block with the first line of
	 * the new block */
	if (curr->y_type == MCHAR)
	{
	    pnew = lalloc((long_u)(STRLEN(curr->y_array[curr->y_size - 1])
			      + STRLEN(y_current->y_array[0]) + 1), TRUE);
	    if (pnew == NULL)
	    {
		    y_idx = y_current->y_size - 1;
		    goto fail;
	    }
	    STRCPY(pnew, curr->y_array[--j]);
	    STRCAT(pnew, y_current->y_array[0]);
	    vim_free(curr->y_array[j]);
	    vim_free(y_current->y_array[0]);
	    curr->y_array[j++] = pnew;
	    y_idx = 1;
	}
	else
	    y_idx = 0;
	while (y_idx < y_current->y_size)
	    curr->y_array[j++] = y_current->y_array[y_idx++];
	curr->y_size = j;
	vim_free(y_current->y_array);
	y_current = curr;
    }
    if (mess)			/* Display message about yank? */
    {
	if (yanktype == MCHAR && !oap->block_mode && yanklines == 1)
	    yanklines = 0;
	/* Some versions of Vi use ">=" here, some don't...  */
	if (yanklines > p_report)
	{
	    /* redisplay now, so message is not deleted */
	    update_topline_redraw();
	    smsg((char_u *)"%ld line%s yanked", yanklines, plural(yanklines));
	}
    }

    /*
     * Set "'[" and "']" marks.
     */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;

#ifdef USE_CLIPBOARD
    /*
     * If we were yanking to the clipboard register, send result to clipboard.
     */
    if (curr == &(y_regs[CLIPBOARD_REGISTER]) && clipboard.available)
    {
	clip_own_selection();
	clip_gen_set_selection();
    }
#endif

    return OK;
}

/*
 * put contents of register "regname" into the text
 * For ":put" command count == -1.
 * flags: PUT_FIXINDENT	make indent look nice
 *	  PUT_CURSEND	leave cursor after end of new text
 */
    void
do_put(regname, dir, count, flags)
    int		regname;
    int		dir;		/* BACKWARD for 'P', FORWARD for 'p' */
    long	count;
    int		flags;
{
    char_u	*ptr;
    char_u	*newp, *oldp;
    int		yanklen;
    int		oldlen;
    int		totlen = 0;		/* init for gcc */
    linenr_t	lnum;
    colnr_t	col;
    long	i;			/* index in y_array[] */
    int		y_type;
    long	y_size;
    char_u	**y_array = NULL;
    long	nr_lines = 0;
    colnr_t	vcol;
    int		delcount;
    int		incr = 0;
    long	j;
    FPOS	new_cursor;
    int		indent;
    int		orig_indent = 0;	/* init for gcc */
    int		indent_diff = 0;	/* init for gcc */
    int		first_indent = TRUE;
    int		lendiff = 0;
    FPOS	old_pos;
    struct block_def bd;
    char_u	*insert_string = NULL;
    int		allocated = FALSE;
#ifdef MULTI_BYTE
    int		bMultiByteCode = 0;
#endif

#ifdef USE_CLIPBOARD
    /* If no register specified, and "unnamed" in 'clipboard', use * register */
    if (regname == 0 && vim_strchr(p_cb, 'd') != NULL)
        regname = '*';
    if (regname == '*')
    {
	if (!clipboard.available)
	    regname = 0;
	else
	    clip_get_selection();
    }
#endif

    if (flags & PUT_FIXINDENT)
	orig_indent = get_indent();

    curbuf->b_op_start = curwin->w_cursor;	/* default for '[ mark */

    if (dir == FORWARD)
    {
#ifdef MULTI_BYTE
	/* put it on the next of the multi-byte character. */
	if (is_dbcs)
	{
	    ptr = ml_get(curwin->w_cursor.lnum) + curwin->w_cursor.col;
	    if (IsLeadByte(*ptr) && (*(ptr + 1) != '\0'))
	    {
		bMultiByteCode = 1;
		curbuf->b_op_start.col++;
	    }
	}
#endif
	curbuf->b_op_start.col++;
    }

    curbuf->b_op_end = curwin->w_cursor;	/* default for '] mark */

    /*
     * Using inserted text works differently, because the register includes
     * special characters (newlines, etc.).
     */
    if (regname == '.')
    {
	(void)stuff_inserted((dir == FORWARD ? (count == -1 ? 'o' : 'a') :
				    (count == -1 ? 'O' : 'i')), count, FALSE);
	/* Putting the text is done later, so can't really move the cursor to
	 * the nex character.  Use "l" to simulate it. */
	if ((flags & PUT_CURSEND) && gchar_cursor() != NUL)
	    stuffcharReadbuff('l');
	return;
    }

    /*
     * For special registers '%' (file name), '#' (alternate file name) and
     * ':' (last command line), etc. we have to create a fake yank register.
     */
    if (get_spec_reg(regname, &insert_string, &allocated, TRUE))
    {
	if (insert_string == NULL)
	    return;
    }

    if (insert_string != NULL)
    {
	y_type = MCHAR;
	if (regname == '=')
	{
	    /* For the = register we need to split the string at NL
	     * characters. */
	    /* Loop twice: count the number of lines and save them. */
	    for (;;)
	    {
		y_size = 0;
		ptr = insert_string;
		while (ptr != NULL)
		{
		    if (y_array != NULL)
			y_array[y_size] = ptr;
		    ++y_size;
		    ptr = vim_strchr(ptr, '\n');
		    if (ptr != NULL)
		    {
			if (y_array != NULL)
			    *ptr = NUL;
			++ptr;
			/* A trailing '\n' makes the string linewise */
			if (*ptr == NUL)
			{
			    y_type = MLINE;
			    break;
			}
		    }
		}
		if (y_array != NULL)
		    break;
		y_array = (char_u **)alloc((unsigned)
						 (y_size * sizeof(char_u *)));
		if (y_array == NULL)
		    goto end;
	    }
	}
	else
	{
	    y_size = 1;		/* use fake one-line yank register */
	    y_array = &insert_string;
	}
    }
    else
    {
	get_yank_register(regname, FALSE);

	y_type = y_current->y_type;
	y_size = y_current->y_size;
	y_array = y_current->y_array;
    }

    if (count == -1)	    /* :put command */
    {
	y_type = MLINE;
	count = 1;
    }

    if (y_size == 0 || y_array == NULL)
    {
	EMSG2("Nothing in register %s",
		  regname == 0 ? (char_u *)"\"" : transchar(regname));
	goto end;
    }

    if (y_type == MBLOCK)
    {
	lnum = curwin->w_cursor.lnum + y_size + 1;
	if (lnum > curbuf->b_ml.ml_line_count)
	    lnum = curbuf->b_ml.ml_line_count + 1;
	if (u_save(curwin->w_cursor.lnum - 1, lnum) == FAIL)
	    goto end;
    }
    else if (u_save_cursor() == FAIL)
	goto end;

    yanklen = STRLEN(y_array[0]);
    changed();

    lnum = curwin->w_cursor.lnum;
    col = curwin->w_cursor.col;
    approximate_botline();	    /* w_botline might not be valid now */
    changed_cline_bef_curs();	    /* cursor posn on screen may change */

/*
 * block mode
 */
    if (y_type == MBLOCK)
    {
	if (dir == FORWARD && gchar_cursor() != NUL)
	{
	    getvcol(curwin, &curwin->w_cursor, NULL, NULL, &col);

#ifdef MULTI_BYTE
	    if (is_dbcs)
	    {
		/* put it on the next of the multi-byte character. */
		col += bMultiByteCode + 1;
		curwin->w_cursor.col += bMultiByteCode + 1;
	    }
	    else
#endif
	    {
		++col;
		++curwin->w_cursor.col;
	    }
	}
	else
	    getvcol(curwin, &curwin->w_cursor, &col, NULL, NULL);
#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with current line */
	syn_changed(curwin->w_cursor.lnum);
#endif
	for (i = 0; i < y_size; ++i)
	{
	    bd.startspaces = 0;
	    bd.endspaces = 0;
	    bd.textcol = 0;
	    vcol = 0;
	    delcount = 0;

	    /* add a new line */
	    if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
	    {
		ml_append(curbuf->b_ml.ml_line_count, (char_u *)"",
							   (colnr_t)1, FALSE);
		++nr_lines;
	    }
	    /* get the old line and advance to the position to insert at */
	    oldp = ml_get_curline();
	    oldlen = STRLEN(oldp);
	    for (ptr = oldp; vcol < col && *ptr; ++ptr)
	    {
		/* Count a tab for what it's worth (if list mode not on) */
		incr = lbr_chartabsize(ptr, (colnr_t)vcol);
		vcol += incr;
		++bd.textcol;
	    }
	    if (vcol < col) /* line too short, padd with spaces */
	    {
		bd.startspaces = col - vcol;
	    }
	    else if (vcol > col)
	    {
		bd.endspaces = vcol - col;
		bd.startspaces = incr - bd.endspaces;
		--bd.textcol;
		delcount = 1;
	    }

	    /* insert the new text */
	    yanklen = STRLEN(y_array[i]);
	    totlen = count * yanklen + bd.startspaces + bd.endspaces;
	    newp = alloc_check((unsigned)totlen + oldlen + 1);
	    if (newp == NULL)
		break;
	    /* copy part up to cursor to new line */
	    ptr = newp;
	    mch_memmove(ptr, oldp, (size_t)bd.textcol);
	    ptr += bd.textcol;
	    /* may insert some spaces before the new text */
	    copy_spaces(ptr, (size_t)bd.startspaces);
	    ptr += bd.startspaces;
	    /* insert the new text */
	    for (j = 0; j < count; ++j)
	    {
		mch_memmove(ptr, y_array[i], (size_t)yanklen);
		ptr += yanklen;
	    }
	    /* may insert some spaces after the new text */
	    copy_spaces(ptr, (size_t)bd.endspaces);
	    ptr += bd.endspaces;
	    /* move the text after the cursor to the end of the line. */
	    mch_memmove(ptr, oldp + bd.textcol + delcount,
				(size_t)(oldlen - bd.textcol - delcount + 1));
	    ml_replace(curwin->w_cursor.lnum, newp, FALSE);

	    ++curwin->w_cursor.lnum;
	    if (i == 0)
		curwin->w_cursor.col += bd.startspaces;
	}
						/* adjust '] mark */
	curbuf->b_op_end.lnum = curwin->w_cursor.lnum - 1;
	curbuf->b_op_end.col = bd.textcol + totlen - 1;
	if (flags & PUT_CURSEND)
	{
	    curwin->w_cursor = curbuf->b_op_end;
	    curwin->w_cursor.col++;
	}
	else
	    curwin->w_cursor.lnum = lnum;
	update_topline();
	if (flags & PUT_CURSEND)
	    update_screen(NOT_VALID);
	else
	    update_screen(VALID_TO_CURSCHAR);
    }
    else	/* not block mode */
    {
	if (y_type == MCHAR)
	{
    /* if type is MCHAR, FORWARD is the same as BACKWARD on the next char */
	    if (dir == FORWARD && gchar_cursor() != NUL)
	    {
#ifdef MULTI_BYTE
		if (is_dbcs)
		{
		    /* put it on the next of the multi-byte character. */
		    col += bMultiByteCode + 1;
		    if (yanklen)
		    {
			curwin->w_cursor.col += bMultiByteCode + 1;
			curbuf->b_op_end.col += bMultiByteCode + 1;
		    }
		}
		else
#endif
		{
		    ++col;
		    if (yanklen)
		    {
			++curwin->w_cursor.col;
			++curbuf->b_op_end.col;
		    }
		}
	    }
	    new_cursor = curwin->w_cursor;
	}
	else if (dir == BACKWARD)
    /* if type is MLINE, BACKWARD is the same as FORWARD on the previous line */
	    --lnum;

/*
 * simple case: insert into current line
 */
	if (y_type == MCHAR && y_size == 1)
	{
	    totlen = count * yanklen;
	    if (totlen)
	    {
		oldp = ml_get(lnum);
		newp = alloc_check((unsigned)(STRLEN(oldp) + totlen + 1));
		if (newp == NULL)
		    goto end;		/* alloc() will give error message */
		mch_memmove(newp, oldp, (size_t)col);
		ptr = newp + col;
		for (i = 0; i < count; ++i)
		{
		    mch_memmove(ptr, y_array[0], (size_t)yanklen);
		    ptr += yanklen;
		}
		mch_memmove(ptr, oldp + col, STRLEN(oldp + col) + 1);
		ml_replace(lnum, newp, FALSE);
		/* Put cursor on last putted char. */
		curwin->w_cursor.col += (colnr_t)(totlen - 1);
	    }
	    curbuf->b_op_end = curwin->w_cursor;
	    /* For "CTRL-O p" in Insert mode, put cursor after last char */
	    if (totlen && (restart_edit || (flags & PUT_CURSEND)))
		++curwin->w_cursor.col;
	    update_screenline();
	}
	else
	{
#ifdef SYNTAX_HL
	    /* recompute syntax hl., starting with current line */
	    syn_changed(lnum);
#endif
	    while (--count >= 0)
	    {
		i = 0;
		if (y_type == MCHAR)
		{
		    /*
		     * Split the current line in two at the insert position.
		     * First insert y_array[size - 1] in front of second line.
		     * Then append y_array[0] to first line.
		     */
		    ptr = ml_get(lnum) + col;
		    totlen = STRLEN(y_array[y_size - 1]);
		    newp = alloc_check((unsigned)(STRLEN(ptr) + totlen + 1));
		    if (newp == NULL)
			goto error;
		    STRCPY(newp, y_array[y_size - 1]);
		    STRCAT(newp, ptr);
			/* insert second line */
		    ml_append(lnum, newp, (colnr_t)0, FALSE);
		    vim_free(newp);

		    oldp = ml_get(lnum);
		    newp = alloc_check((unsigned)(col + yanklen + 1));
		    if (newp == NULL)
			goto error;
					    /* copy first part of line */
		    mch_memmove(newp, oldp, (size_t)col);
					    /* append to first line */
		    mch_memmove(newp + col, y_array[0], (size_t)(yanklen + 1));
		    ml_replace(lnum, newp, FALSE);

		    curwin->w_cursor.lnum = lnum;
		    i = 1;
		}

		while (i < y_size)
		{
		    if ((y_type != MCHAR || i < y_size - 1) &&
			ml_append(lnum, y_array[i], (colnr_t)0, FALSE) == FAIL)
			    goto error;
		    lnum++;
		    i++;
		    if (flags & PUT_FIXINDENT)
		    {
			old_pos = curwin->w_cursor;
			curwin->w_cursor.lnum = lnum;
			ptr = ml_get(lnum);
			if (count == 0 && i == y_size - 1)
			    lendiff = STRLEN(ptr);
#if defined(SMARTINDENT) || defined(CINDENT)
			if (*ptr == '#' && (
# ifdef SMARTINDENT
#  ifdef CINDENT
			   (curbuf->b_p_si && !curbuf->b_p_cin) ||
#  else
			   curbuf->b_p_si
#  endif
# endif
# ifdef CINDENT
			   (curbuf->b_p_cin && in_cinkeys('#', ' ', TRUE))
# endif
					    ))
			    indent = 0;     /* Leave # lines at start */
			else
#endif
			     if (*ptr == NUL)
			    indent = 0;     /* Ignore empty lines */
			else if (first_indent)
			{
			    indent_diff = orig_indent - get_indent();
			    indent = orig_indent;
			    first_indent = FALSE;
			}
			else if ((indent = get_indent() + indent_diff) < 0)
			    indent = 0;
			set_indent(indent, TRUE);
			curwin->w_cursor = old_pos;
			/* remember how many chars were removed */
			if (count == 0 && i == y_size - 1)
			    lendiff -= STRLEN(ml_get(lnum));
		    }
		    ++nr_lines;
		}
	    }

	    /* put '] mark at last inserted character */
	    curbuf->b_op_end.lnum = lnum;
	    /* correct length for change in indent */
	    col = STRLEN(y_array[y_size - 1]) - lendiff;
	    if (col > 1)
		curbuf->b_op_end.col = col - 1;
	    else
		curbuf->b_op_end.col = 0;

	    if (flags & PUT_CURSEND)
	    {
		/* put cursor after inserted text */
		if (y_type == MLINE)
		{
		    if (lnum >= curbuf->b_ml.ml_line_count)
			curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
		    else
			curwin->w_cursor.lnum = lnum + 1;
		    curwin->w_cursor.col = 0;
		}
		else
		{
		    curwin->w_cursor.lnum = lnum;
		    curwin->w_cursor.col = col;
		}
		/* the text before the cursor needs redrawing */
		redraw_curbuf_later(NOT_VALID);
	    }
	    else if (y_type == MLINE)
	    {
		/* put cursor onfirst non-blank in first inserted line */
		curwin->w_cursor.col = 0;
		if (dir == FORWARD)
		    ++curwin->w_cursor.lnum;
		beginline(BL_WHITE | BL_FIX);
	    }
	    else	/* put cursor on first inserted character */
		curwin->w_cursor = new_cursor;

error:
	    if (y_type == MLINE)	/* adjust '[ mark */
	    {
		curbuf->b_op_start.col = 0;
		if (dir == FORWARD)
		    curbuf->b_op_start.lnum++;
	    }
	    mark_adjust(curbuf->b_op_start.lnum + (y_type == MCHAR),
					     (linenr_t)MAXLNUM, nr_lines, 0L);
	    update_topline();
	    update_screen(VALID_TO_CURSCHAR);
	}
    }

    msgmore(nr_lines);
    curwin->w_set_curswant = TRUE;

end:
    if (allocated)
    {
	vim_free(insert_string);
	if (regname == '=')
	    vim_free(y_array);
    }
    if ((flags & PUT_CURSEND) && gchar_cursor() == NUL && curwin->w_cursor.col
				       && !(restart_edit || (State & INSERT)))
	--curwin->w_cursor.col;
}

/* Return the character name of the register with the given number */
    int
get_register_name(num)
    int num;
{
    if (num == -1)
	return '"';
    else if (num < 10)
	return num + '0';
    else if (num == DELETION_REGISTER)
	return '-';
#ifdef USE_CLIPBOARD
    else if (num == CLIPBOARD_REGISTER)
	return '*';
#endif
    else
	return num + 'a' - 10;
}

/*
 * display the contents of the yank registers
 */
    void
do_dis(arg)
    char_u *arg;
{
    int		    i, n;
    long	    j;
    char_u	    *p;
    struct yankreg  *yb;
    char_u	    name;
    int		    attr;

    if (arg != NULL && *arg == NUL)
	arg = NULL;
    attr = hl_attr(HLF_8);

    /* Highlight title */
    MSG_PUTS_TITLE("\n--- Registers ---");
    for (i = -1; i < NUM_REGISTERS; ++i)
    {
	if (i == -1)
	{
	    if (y_previous != NULL)
		yb = y_previous;
	    else
		yb = &(y_regs[0]);
	}
	else
	    yb = &(y_regs[i]);
	name = get_register_name(i);
	if (yb->y_array != NULL
		&& (arg == NULL || vim_strchr(arg, name) != NULL))
	{
	    msg_putchar('\n');
	    msg_putchar('"');
	    msg_putchar(name);
	    MSG_PUTS("   ");

	    n = (int)Columns - 6;
	    for (j = 0; j < yb->y_size && n > 1; ++j)
	    {
		if (j)
		{
		    MSG_PUTS_ATTR("^J", attr);
		    n -= 2;
		}
		for (p = yb->y_array[j]; *p && (n -= charsize(*p)) >= 0; ++p)
		    msg_outtrans_len(p, 1);
	    }
	    if (n > 1 && yb->y_type == MLINE)
		MSG_PUTS_ATTR("^J", attr);
	    out_flush();		    /* show one line at a time */
	}
    }

    /*
     * display last inserted text
     */
    if ((p = get_last_insert()) != NULL
	    && (arg == NULL || vim_strchr(arg, '.') != NULL))
    {
	MSG_PUTS("\n\".   ");
	dis_msg(p, TRUE);
    }

    /*
     * display last command line
     */
    if (last_cmdline != NULL && (arg == NULL || vim_strchr(arg, ':') != NULL))
    {
	MSG_PUTS("\n\":   ");
	dis_msg(last_cmdline, FALSE);
    }

    /*
     * display current file name
     */
    if (curbuf->b_fname != NULL
	    && (arg == NULL || vim_strchr(arg, '%') != NULL))
    {
	MSG_PUTS("\n\"%   ");
	dis_msg(curbuf->b_fname, FALSE);
    }

    /*
     * display alternate file name
     */
    if (arg == NULL || vim_strchr(arg, '%') != NULL)
    {
	char_u	    *fname;
	linenr_t    dummy;

	if (buflist_name_nr(0, &fname, &dummy) != FAIL)
	{
	    MSG_PUTS("\n\"#   ");
	    dis_msg(fname, FALSE);
	}
    }

    /*
     * display last search pattern
     */
    if (last_search_pat() != NULL
			     && (arg == NULL || vim_strchr(arg, '/') != NULL))
    {
	MSG_PUTS("\n\"/   ");
	dis_msg(last_search_pat(), FALSE);
    }

#ifdef WANT_EVAL
    /*
     * display last used expression
     */
    if (expr_line != NULL && (arg == NULL || vim_strchr(arg, '=') != NULL))
    {
	MSG_PUTS("\n\"=   ");
	dis_msg(expr_line, FALSE);
    }
#endif
}

/*
 * display a string for do_dis()
 * truncate at end of screen line
 */
    void
dis_msg(p, skip_esc)
    char_u	*p;
    int		skip_esc;	    /* if TRUE, ignore trailing ESC */
{
    int	    n;

    n = (int)Columns - 6;
    while (*p && !(*p == ESC && skip_esc && *(p + 1) == NUL) &&
			(n -= charsize(*p)) >= 0)
	msg_outtrans_len(p++, 1);
}

/*
 * join 'count' lines (minimal 2), including u_save()
 */
    void
do_do_join(count, insert_space, redraw)
    long    count;
    int	    insert_space;
    int	    redraw;		    /* can redraw, curwin->w_wcol valid */
{
    if (u_save((linenr_t)(curwin->w_cursor.lnum - 1),
		    (linenr_t)(curwin->w_cursor.lnum + count)) == FAIL)
	return;

    if (count > 10)
	redraw = FALSE;		    /* don't redraw each small change */
    while (--count > 0)
    {
	line_breakcheck();
	if (got_int || do_join(insert_space, redraw) == FAIL)
	{
	    beep_flush();
	    break;
	}
    }
    redraw_later(VALID_TO_CURSCHAR);

    /*
     * Need to update the screen if the line where the cursor is became too
     * long to fit on the screen.
     */
    update_topline_redraw();
}

/*
 * Join two lines at the cursor position.
 * "redraw" is TRUE when the screen should be updated.
 *
 * return FAIL for failure, OK ohterwise
 */
    int
do_join(insert_space, redraw)
    int		insert_space;
    int		redraw;
{
    char_u	*curr;
    char_u	*next;
    char_u	*newp;
    int		endcurr1, endcurr2;
    int		currsize;	/* size of the current line */
    int		nextsize;	/* size of the next line */
    int		spaces;		/* number of spaces to insert */
    int		rows_to_del = 0;/* number of rows on screen to delete */
    linenr_t	t;

    if (curwin->w_cursor.lnum == curbuf->b_ml.ml_line_count)
	return FAIL;		/* can't join on last line */

    if (redraw)
    {
	/*
	 * Check if we can really redraw:  w_cline_row and w_cline_height need
	 * to be valid.  Try to make them valid by calling may_validate_crow()
	 */
	if (may_validate_crow() == OK)
	    rows_to_del = plines_m(curwin->w_cursor.lnum,
						   curwin->w_cursor.lnum + 1);
	else
	    redraw = FALSE;
    }

    curr = ml_get_curline();
    currsize = STRLEN(curr);
    endcurr1 = endcurr2 = NUL;
    if (currsize > 0)
    {
	endcurr1 = *(curr + currsize - 1);
	if (currsize > 1)
	    endcurr2 = *(curr + currsize - 2);
    }

    next = ml_get((linenr_t)(curwin->w_cursor.lnum + 1));
    spaces = 0;
    if (insert_space)
    {
	next = skipwhite(next);
	if (*next != ')' && currsize != 0 && endcurr1 != TAB)
	{
	    /* don't add a space if the line is inding in a space */
	    if (endcurr1 == ' ')
		endcurr1 = endcurr2;
	    else
		++spaces;
	    /* extra space when 'joinspaces' set and line ends in '.' */
	    if (       p_js
		    && (endcurr1 == '.'
			|| (vim_strchr(p_cpo, CPO_JOINSP) == NULL
			    && (endcurr1 == '?' || endcurr1 == '!'))))
		++spaces;
	}
    }
    nextsize = STRLEN(next);

    newp = alloc_check((unsigned)(currsize + nextsize + spaces + 1));
    if (newp == NULL)
	return FAIL;

    /*
     * Insert the next line first, because we already have that pointer.
     * Curr has to be obtained again, because getting next will have
     * invalidated it.
     */
    mch_memmove(newp + currsize + spaces, next, (size_t)(nextsize + 1));

    curr = ml_get_curline();
    mch_memmove(newp, curr, (size_t)currsize);

    copy_spaces(newp + currsize, (size_t)spaces);

    ml_replace(curwin->w_cursor.lnum, newp, FALSE);

#ifdef SYNTAX_HL
    /* recompute syntax hl. for current line */
    syn_changed(curwin->w_cursor.lnum);
#endif

    /*
     * Delete the following line. To do this we move the cursor there
     * briefly, and then move it back. After del_lines() the cursor may
     * have moved up (last line deleted), so the current lnum is kept in t.
     */
    t = curwin->w_cursor.lnum;
    ++curwin->w_cursor.lnum;
    del_lines(1L, FALSE, FALSE);
    curwin->w_cursor.lnum = t;

    /*
     * the number of rows on the screen is reduced by the difference
     * in number of rows of the two old lines and the one new line
     */
    if (redraw)
    {
	rows_to_del -= plines(curwin->w_cursor.lnum);
	if (rows_to_del > 0)
	    win_del_lines(curwin, curwin->w_cline_row + curwin->w_cline_height,
						     rows_to_del, TRUE, TRUE);
    }

    /*
     * go to first character of the joined line
     */
    if (currsize == 0)
	curwin->w_cursor.col = 0;
    else
    {
	curwin->w_cursor.col = currsize - 1;
	(void)oneright();
    }
    changed();

    return OK;
}

#ifdef COMMENTS
/*
 * Return TRUE if the two comment leaders given are the same.  The cursor is
 * in the first line.  White-space is ignored.	Note that the whole of
 * 'leader1' must match 'leader2_len' characters from 'leader2' -- webb
 */
    static int
same_leader(leader1_len, leader1_flags, leader2_len, leader2_flags)
    int	    leader1_len;
    char_u  *leader1_flags;
    int	    leader2_len;
    char_u  *leader2_flags;
{
    int	    idx1 = 0, idx2 = 0;
    char_u  *p;
    char_u  *line1;
    char_u  *line2;

    if (leader1_len == 0)
	return (leader2_len == 0);

    /*
     * If first leader has 'f' flag, the lines can be joined only if the
     * second line does not have a leader.
     * If first leader has 'e' flag, the lines can never be joined.
     * If fist leader has 's' flag, the lines can only be joined if there is
     * some text after it and the second line has the 'm' flag.
     */
    if (leader1_flags != NULL)
    {
	for (p = leader1_flags; *p && *p != ':'; ++p)
	{
	    if (*p == COM_FIRST)
		return (leader2_len == 0);
	    if (*p == COM_END)
		return FALSE;
	    if (*p == COM_START)
	    {
		if (*(ml_get_curline() + leader1_len) == NUL)
		    return FALSE;
		if (leader2_flags == NULL || leader2_len == 0)
		    return FALSE;
		for (p = leader2_flags; *p && *p != ':'; ++p)
		    if (*p == COM_MIDDLE)
			return TRUE;
		return FALSE;
	    }
	}
    }

    /*
     * Get current line and next line, compare the leaders.
     * The first line has to be saved, only one line can be locked at a time.
     */
    line1 = vim_strsave(ml_get_curline());
    if (line1 != NULL)
    {
	for (idx1 = 0; vim_iswhite(line1[idx1]); ++idx1)
	    ;
	line2 = ml_get(curwin->w_cursor.lnum + 1);
	for (idx2 = 0; idx2 < leader2_len; ++idx2)
	{
	    if (!vim_iswhite(line2[idx2]))
	    {
		if (line1[idx1++] != line2[idx2])
		    break;
	    }
	    else
		while (vim_iswhite(line1[idx1]))
		    ++idx1;
	}
	vim_free(line1);
    }
    return (idx2 == leader2_len && idx1 == leader1_len);
}
#endif

/*
 * implementation of the format operator 'gq'
 */
    void
op_format(oap)
    OPARG	*oap;
{
    long	old_line_count = curbuf->b_ml.ml_line_count;
    int		is_not_par;		/* current line not part of parag. */
    int		next_is_not_par;	/* next line not part of paragraph */
    int		is_end_par;		/* at end of paragraph */
    int		prev_is_end_par = FALSE;/* prev. line not part of parag. */
#ifdef COMMENTS
    int		leader_len = 0;		/* leader len of current line */
    int		next_leader_len;	/* leader len of next line */
    char_u	*leader_flags = NULL;	/* flags for leader of current line */
    char_u	*next_leader_flags;	/* flags for leader of next line */
#endif
    int		advance = TRUE;
    int		second_indent = -1;
    int		do_second_indent;
    int		first_par_line = TRUE;
    int		smd_save;
    long	count;
    int		need_set_indent = TRUE;	/* set indent of next paragraph */
    int		force_format = FALSE;
    int		max_len;
    int		screenlines = -1;

    if (u_save((linenr_t)(oap->start.lnum - 1),
				       (linenr_t)(oap->end.lnum + 1)) == FAIL)
	return;

    /* When formatting less than a screenfull, will try to speed up redrawing
     * by inserting/deleting screen lines */
    if (oap->end.lnum - oap->start.lnum < Rows)
	screenlines = plines_m(oap->start.lnum, oap->end.lnum);

    /* length of a line to force formatting: 3 * 'tw' */
    max_len = comp_textwidth(TRUE) * 3;

    /* Set '[ mark at the start of the formatted area */
    curbuf->b_op_start = oap->start;

    /* check for 'q' and '2' in 'formatoptions' */
#ifdef COMMENTS
    fo_do_comments = has_format_option(FO_Q_COMS);
#endif
    do_second_indent = has_format_option(FO_Q_SECOND);

    /*
     * Get info about the previous and current line.
     */
    if (curwin->w_cursor.lnum > 1)
	is_not_par = fmt_check_par(curwin->w_cursor.lnum - 1
#ifdef COMMENTS
				   , &leader_len, &leader_flags
#endif
				  );
    else
	is_not_par = TRUE;
    next_is_not_par = fmt_check_par(curwin->w_cursor.lnum
#ifdef COMMENTS
					, &next_leader_len, &next_leader_flags
#endif
					);
    is_end_par = (is_not_par || next_is_not_par);

    curwin->w_cursor.lnum--;
    for (count = oap->line_count; count > 0 && !got_int; --count)
    {
	/*
	 * Advance to next paragraph.
	 */
	if (advance)
	{
	    curwin->w_cursor.lnum++;
	    prev_is_end_par = is_end_par;
	    is_not_par = next_is_not_par;
#ifdef COMMENTS
	    leader_len = next_leader_len;
	    leader_flags = next_leader_flags;
#endif
	}

	/*
	 * The last line to be formatted.
	 */
	if (count == 1)
	{
	    next_is_not_par = TRUE;
#ifdef COMMENTS
	    next_leader_len = 0;
	    next_leader_flags = NULL;
#endif
	}
	else
	    next_is_not_par = fmt_check_par(curwin->w_cursor.lnum + 1
#ifdef COMMENTS
					, &next_leader_len, &next_leader_flags
#endif
					);
	advance = TRUE;
	is_end_par = (is_not_par || next_is_not_par);

	/*
	 * Skip lines that are not in a paragraph.
	 */
	if (!is_not_par)
	{
	    /*
	     * For the first line of a paragraph, check indent of second line.
	     * Don't do this for comments and empty lines.
	     */
	    if (first_par_line
		    && do_second_indent
		    && prev_is_end_par
		    && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count
#ifdef COMMENTS
		    && leader_len == 0
		    && next_leader_len == 0
#endif
		    && !lineempty(curwin->w_cursor.lnum + 1))
		second_indent = get_indent_lnum(curwin->w_cursor.lnum + 1);

	    /*
	     * When the comment leader changes, it's the end of the paragraph.
	     */
	    if (curwin->w_cursor.lnum >= curbuf->b_ml.ml_line_count
#ifdef COMMENTS
		    || !same_leader(leader_len, leader_flags,
					  next_leader_len, next_leader_flags)
#endif
		    )
		is_end_par = TRUE;

	    /*
	     * If we have got to the end of a paragraph, or the line is
	     * getting long, format it.
	     */
	    if (is_end_par || force_format)
	    {
		if (need_set_indent)
		    /* replace indent in first line with minimal number of
		     * tabs and spaces, according to current options */
		    set_indent(get_indent(), TRUE);

		/* put cursor on last non-space */
		coladvance(MAXCOL);
		while (curwin->w_cursor.col && vim_isspace(gchar_cursor()))
		    dec_cursor();

#ifdef SYNTAX_HL
		/* recompute syntax hl., starting with current line */
		syn_changed(curwin->w_cursor.lnum);
#endif
		/* do the formatting, without 'showmode' */
		State = INSERT;	/* for open_line() */
		smd_save = p_smd;
		p_smd = FALSE;
		insertchar(NUL, TRUE, second_indent, FALSE);
		State = NORMAL;
		p_smd = smd_save;
		second_indent = -1;
		/* at end of par.: need to set indent of next par. */
		need_set_indent = is_end_par;
		if (is_end_par)
		    first_par_line = TRUE;
		force_format = FALSE;
	    }

	    /*
	     * When still in same paragraph, join the lines together.  But
	     * first delete the comment leader from the second line.
	     */
	    if (!is_end_par)
	    {
		advance = FALSE;
		curwin->w_cursor.lnum++;
		curwin->w_cursor.col = 0;
#ifdef COMMENTS
		(void)del_chars((long)next_leader_len, FALSE);
#endif
		curwin->w_cursor.lnum--;
		if (do_join(TRUE, FALSE) == FAIL)
		{
		    beep_flush();
		    break;
		}
		first_par_line = FALSE;
		/* If the line is getting long, format it next time */
		if (STRLEN(ml_get_curline()) > (size_t)max_len)
		    force_format = TRUE;
		else
		    force_format = FALSE;
	    }
	}
	line_breakcheck();
    }
#ifdef COMMENTS
    fo_do_comments = FALSE;
#endif

    /*
     * Try to correct the number of lines on the screen, to speed up
     * displaying.
     */
    if (screenlines > 0)
    {
	screenlines -= plines_m(oap->start.lnum, curwin->w_cursor.lnum);
	if (screenlines > 0)
	    win_del_lines(curwin, curwin->w_cline_row, screenlines,
								 FALSE, TRUE);
	else if (screenlines < 0)
	    win_ins_lines(curwin, curwin->w_cline_row, screenlines,
								 FALSE, TRUE);
    }

    /*
     * Leave the cursor at the first non-blank of the last formatted line.
     * If the cursor was moved one line back (e.g. with "Q}") go to the next
     * line, so "." will do the next lines.
     */
    if (oap->end_adjusted && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
	++curwin->w_cursor.lnum;
    beginline(BL_WHITE | BL_FIX);
    update_screen(NOT_VALID);
    msgmore(curbuf->b_ml.ml_line_count - old_line_count);

    /* put '] mark on the end of the formatted area */
    curbuf->b_op_end = curwin->w_cursor;
}

/*
 * Blank lines, and lines containing only the comment leader, are left
 * untouched by the formatting.  The function returns TRUE in this
 * case.  It also returns TRUE when a line starts with the end of a comment
 * ('e' in comment flags), so that this line is skipped, and not joined to the
 * previous line.  A new paragraph starts after a blank line, or when the
 * comment leader changes -- webb.
 */
#ifdef COMMENTS
    static int
fmt_check_par(lnum, leader_len, leader_flags)
    linenr_t	lnum;
    int		*leader_len;
    char_u	**leader_flags;
{
    char_u	*flags = NULL;	    /* init for GCC */
    char_u	*ptr;

    ptr = ml_get(lnum);
    *leader_len = get_leader_len(ptr, leader_flags, FALSE);

    if (*leader_len > 0)
    {
	/*
	 * Search for 'e' flag in comment leader flags.
	 */
	flags = *leader_flags;
	while (*flags && *flags != ':' && *flags != COM_END)
	    ++flags;
    }

    return (ptr[*leader_len] == NUL
	    || (*leader_len > 0 && *flags == COM_END)
	    || startPS(lnum, NUL, FALSE));
}
#else
    static int
fmt_check_par(lnum)
    linenr_t	lnum;
{
    return (*ml_get(lnum) == NUL || startPS(lnum, NUL, FALSE));
}
#endif

/*
 * prepare a few things for block mode yank/delete/tilde
 *
 * for delete:
 * - textlen includes the first/last char to be (partly) deleted
 * - start/endspaces is the number of columns that are taken by the
 *   first/last deleted char minus the number of columns that have to be
 *   deleted.  for yank and tilde:
 * - textlen includes the first/last char to be wholly yanked
 * - start/endspaces is the number of columns of the first/last yanked char
 *   that are to be yanked.
 */
    static void
block_prep(oap, bdp, lnum, is_del)
    OPARG		*oap;
    struct block_def	*bdp;
    linenr_t		lnum;
    int			is_del;
{
    int		incr = 0;
    char_u	*pend;
    char_u	*pstart;

    bdp->startspaces = 0;
    bdp->endspaces = 0;
    bdp->textlen = 0;
    bdp->textcol = 0;
    bdp->start_vcol = 0;
    bdp->end_vcol = 0;
#ifdef VISUALEXTRA
    bdp->is_short = FALSE;
    bdp->is_oneChar = FALSE;
    bdp->pre_whitesp = 0;
    bdp->pre_whitesp_c = 0;
#endif
    pstart = ml_get(lnum);
    while (bdp->start_vcol < oap->start_vcol && *pstart)
    {
	/* Count a tab for what it's worth (if list mode not on) */
	incr = lbr_chartabsize(pstart, (colnr_t)bdp->start_vcol);
	bdp->start_vcol += incr;
	++bdp->textcol;
#ifdef VISUALEXTRA
	if (vim_iswhite(*pstart))
	{
	    bdp->pre_whitesp += incr;
	    bdp->pre_whitesp_c++;
	}
	else
	{
	    bdp->pre_whitesp = 0;
	    bdp->pre_whitesp_c = 0;
	}
#endif
	++pstart;
    }
    bdp->start_char_vcols = incr;
    if (bdp->start_vcol < oap->start_vcol)	/* line too short */
    {
#ifdef VISUALEXTRA
	bdp->is_short = TRUE;
#endif
	if (!is_del || oap->op_type == OP_APPEND)
	    bdp->endspaces = oap->end_vcol - oap->start_vcol + 1;
    }
    else
    {
	bdp->startspaces = bdp->start_vcol - oap->start_vcol;
	if (is_del && bdp->startspaces)
	    bdp->startspaces = bdp->start_char_vcols - bdp->startspaces;
	pend = pstart;
	bdp->end_vcol = bdp->start_vcol;
	if (bdp->end_vcol > oap->end_vcol)	/* it's all in one character */
	{
#ifdef VISUALEXTRA
	    bdp->is_oneChar = TRUE;
#endif
	    if (oap->op_type == OP_INSERT)
		bdp->endspaces = bdp->start_char_vcols - bdp->startspaces;
	    else if (oap->op_type == OP_APPEND)
	    {
		bdp->startspaces += oap->end_vcol - oap->start_vcol + 1;
		bdp->endspaces = bdp->start_char_vcols - bdp->startspaces;
	    }
	    else
	    {
		bdp->startspaces = oap->end_vcol - oap->start_vcol + 1;
		if (is_del && oap->op_type != OP_LSHIFT)
		    bdp->startspaces = bdp->start_char_vcols - bdp->startspaces;
	    }
	}
	else
	{
	    while (bdp->end_vcol <= oap->end_vcol && *pend)
	    {
		/* Count a tab for what it's worth (if list mode not on) */
		incr = lbr_chartabsize(pend, (colnr_t)bdp->end_vcol);
		bdp->end_vcol += incr;
		++pend;
	    }
	    if (bdp->end_vcol <= oap->end_vcol
		    && (!is_del
			|| oap->op_type == OP_APPEND
			|| oap->op_type == OP_REPLACE)) /* line too short */
	    {
#ifdef VISUALEXTRA
		bdp->is_short = TRUE;
#endif
		if (oap->op_type == OP_APPEND)
		    bdp->endspaces = oap->end_vcol - bdp->end_vcol + 1;
		else
		    bdp->endspaces = 0;
	    }
	    else if (bdp->end_vcol > oap->end_vcol)
	    {
		bdp->endspaces = bdp->end_vcol - oap->end_vcol - 1;
		if (!is_del && pend != pstart && bdp->endspaces)
		    --pend;
	    }
	}
#ifdef VISUALEXTRA
	bdp->end_char_vcols = incr;
#endif
	if (is_del && bdp->startspaces)
	{
	    --pstart;
	    --bdp->textcol;
	}
	bdp->textlen = (int)(pend - pstart);
    }
    bdp->textstart = pstart;
}

#ifdef RIGHTLEFT
static void reverse_line __ARGS((char_u *s));

    static void
reverse_line(s)
    char_u *s;
{
    int	    i, j;
    char_u  c;

    if ((i = STRLEN(s) - 1) <= 0)
	return;

    curwin->w_cursor.col = i - curwin->w_cursor.col;
    for (j = 0; j < i; j++, i--)
    {
	c = s[i]; s[i] = s[j]; s[j] = c;
    }
}

# define RLADDSUBFIX() if (curwin->w_p_rl) reverse_line(ptr);
#else
# define RLADDSUBFIX()
#endif

/*
 * add or subtract 'Prenum1' from a number in a line
 * 'command' is CTRL-A for add, CTRL-X for subtract
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_addsub(command, Prenum1)
    int		command;
    linenr_t	Prenum1;
{
    int		    col;
    char_u	    buf1[NUMBUFLEN];
    char_u	    buf2[NUMBUFLEN];
    int		    hex;		/* 'X' or 'x': hex; '0': octal */
    static int	    hexupper = FALSE;	/* 0xABC */
    unsigned long   n;
    unsigned long   oldn;
    char_u	    *ptr;
    int		    c;
    int		    length = 0;		/* character length of the number */
    int		    todel;
    int		    dohex;
    int		    dooct;
    int		    firstdigit;
    int		    negative;
    int		    subtract;

    dohex = (vim_strchr(curbuf->b_p_nf, 'x') != NULL);
    dooct = (vim_strchr(curbuf->b_p_nf, 'o') != NULL);

    ptr = ml_get_curline();
    RLADDSUBFIX();

    /*
     * First check if we are on a hexadecimal number, after the "0x".
     */
    col = curwin->w_cursor.col;
    if (dohex)
	while (col > 0 && isxdigit(ptr[col]))
	    --col;
    if (       dohex
	    && col > 0
	    && (ptr[col] == 'X'
		|| ptr[col] == 'x')
	    && ptr[col - 1] == '0'
	    && isxdigit(ptr[col + 1]))
    {
	/*
	 * Found hexadecimal number, move to its start.
	 */
	--col;
    }
    else
    {
	/*
	 * Search forward and then backward to find the start of number.
	 */
	col = curwin->w_cursor.col;

	while (ptr[col] != NUL && !isdigit(ptr[col]))
	    ++col;

	while (col > 0 && isdigit(ptr[col - 1]))
	    --col;
    }

    /*
     * If a number was found, and saving for undo works, replace the number.
     */
    firstdigit = ptr[col];
    RLADDSUBFIX();
    if (!isdigit(firstdigit) || u_save_cursor() != OK)
    {
	beep_flush();
	return FAIL;
    }

    /* get ptr again, because u_save() may have changed it */
    ptr = ml_get_curline();
    RLADDSUBFIX();

    negative = FALSE;
    if (col > 0 && ptr[col - 1] == '-')	    /* negative number */
    {
	--col;
	negative = TRUE;
    }

    /* get the number value (unsigned) */
    vim_str2nr(ptr + col, &hex, &length, dooct, dohex, NULL, &n);

    /* ignore leading '-' for hex and octal numbers */
    if (hex && negative)
    {
	++col;
	--length;
	negative = FALSE;
    }

    /* add or subtract */
    subtract = FALSE;
    if (command == Ctrl('X'))
	subtract ^= TRUE;
    if (negative)
	subtract ^= TRUE;

    oldn = n;
    if (subtract)
	n -= (unsigned long)Prenum1;
    else
	n += (unsigned long)Prenum1;

    /* handle wraparound for decimal numbers */
    if (!hex)
    {
	if (subtract)
	{
	    if (n > oldn)
	    {
		n = 1 + (n ^ (unsigned long)-1);
		negative ^= TRUE;
	    }
	}
	else /* add */
	{
	    if (n < oldn)
	    {
		n = (n ^ (unsigned long)-1);
		negative ^= TRUE;
	    }
	}
	if (n == 0)
	    negative = FALSE;
    }

    /*
     * Delete the old number.
     */
    curwin->w_cursor.col = col;
    todel = length;
    c = gchar_cursor();
    /*
     * Don't include the '-' in the length, only the length of the part
     * after it is kept the same.
     */
    if (c == '-')
	--length;
    while (todel-- > 0)
    {
	if (isalpha(c))
	{
	    if (isupper(c))
		hexupper = TRUE;
	    else
		hexupper = FALSE;
	}
	(void)del_char(FALSE);
	c = gchar_cursor();
    }

    /*
     * Prepare the leading characters in buf1[].
     */
    ptr = buf1;
    if (negative)
    {
	*ptr++ = '-';
    }
    if (hex)
    {
	*ptr++ = '0';
	--length;
    }
    if (hex == 'x' || hex == 'X')
    {
	*ptr++ = hex;
	--length;
    }

    /*
     * Put the number characters in buf2[].
     */
    if (hex == 0)
	sprintf((char *)buf2, "%lu", n);
    else if (hex == '0')
	sprintf((char *)buf2, "%lo", n);
    else if (hex && hexupper)
	sprintf((char *)buf2, "%lX", n);
    else
	sprintf((char *)buf2, "%lx", n);
    length -= STRLEN(buf2);

    /*
     * adjust number of zeros to the new number of digits, so the
     * total length of the number remains the same
     */
    if (firstdigit == '0')
	while (length-- > 0)
	    *ptr++ = '0';
    *ptr = NUL;
    STRCAT(buf1, buf2);
    ins_str(buf1);		    /* insert the new number */
    --curwin->w_cursor.col;
    curwin->w_set_curswant = TRUE;
#ifdef RIGHTLEFT
    ptr = ml_get_buf(curbuf, curwin->w_cursor.lnum, TRUE);
    RLADDSUBFIX();
#endif
    update_screenline();
    return OK;
}

#ifdef VIMINFO
    int
read_viminfo_register(line, fp, force)
    char_u  *line;
    FILE    *fp;
    int	    force;
{
    int	    eof;
    int	    do_it = TRUE;
    int	    size;
    int	    limit;
    int	    i;
    int	    set_prev = FALSE;
    char_u  *str;
    char_u  **array = NULL;

    /* We only get here (hopefully) if line[0] == '"' */
    str = line + 1;
    if (*str == '"')
    {
	set_prev = TRUE;
	str++;
    }
    if (!isalnum(*str) && *str != '-')
    {
	if (viminfo_error("Illegal register name", line))
	    return TRUE;	/* too many errors, pretend end-of-file */
	do_it = FALSE;
    }
    get_yank_register(*str++, FALSE);
    if (!force && y_current->y_array != NULL)
	do_it = FALSE;
    size = 0;
    limit = 100;	/* Optimized for registers containing <= 100 lines */
    if (do_it)
    {
	if (set_prev)
	    y_previous = y_current;
	vim_free(y_current->y_array);
	array = y_current->y_array =
		       (char_u **)alloc((unsigned)(limit * sizeof(char_u *)));
	str = skipwhite(str);
	if (STRNCMP(str, "CHAR", 4) == 0)
	    y_current->y_type = MCHAR;
	else if (STRNCMP(str, "BLOCK", 5) == 0)
	    y_current->y_type = MBLOCK;
	else
	    y_current->y_type = MLINE;
    }
    while (!(eof = vim_fgets(line, LSIZE, fp))
					&& (line[0] == TAB || line[0] == '<'))
    {
	if (do_it)
	{
	    if (size >= limit)
	    {
		y_current->y_array = (char_u **)
			      alloc((unsigned)(limit * 2 * sizeof(char_u *)));
		for (i = 0; i < limit; i++)
		    y_current->y_array[i] = array[i];
		vim_free(array);
		limit *= 2;
		array = y_current->y_array;
	    }
	    str = viminfo_readstring(line + 1, fp);
	    if (str != NULL)
		array[size++] = str;
	    else
		do_it = FALSE;
	}
    }
    if (do_it)
    {
	if (size == 0)
	{
	    vim_free(array);
	    y_current->y_array = NULL;
	}
	else if (size < limit)
	{
	    y_current->y_array =
			(char_u **)alloc((unsigned)(size * sizeof(char_u *)));
	    for (i = 0; i < size; i++)
		y_current->y_array[i] = array[i];
	    vim_free(array);
	}
	y_current->y_size = size;
    }
    return eof;
}

    void
write_viminfo_registers(fp)
    FILE    *fp;
{
    int	    i, j;
    char_u  *type;
    char_u  c;
    int	    num_lines;
    int	    max_num_lines;

    fprintf(fp, "\n# Registers:\n");

    max_num_lines = get_viminfo_parameter('"');
    if (max_num_lines == 0)
	return;
    for (i = 0; i < NUM_REGISTERS; i++)
    {
	if (y_regs[i].y_array == NULL)
	    continue;
#ifdef USE_CLIPBOARD
	/* Skip '*' register, we don't want it back next time */
	if (i == CLIPBOARD_REGISTER)
	    continue;
#endif
	switch (y_regs[i].y_type)
	{
	    case MLINE:
		type = (char_u *)"LINE";
		break;
	    case MCHAR:
		type = (char_u *)"CHAR";
		break;
	    case MBLOCK:
		type = (char_u *)"BLOCK";
		break;
	    default:
		sprintf((char *)IObuff, "Unknown register type %d",
		    y_regs[i].y_type);
		emsg(IObuff);
		type = (char_u *)"LINE";
		break;
	}
	if (y_previous == &y_regs[i])
	    fprintf(fp, "\"");
	if (i == DELETION_REGISTER)
	    c = '-';
	else if (i < 10)
	    c = '0' + i;
	else
	    c = 'a' + i - 10;
	fprintf(fp, "\"%c\t%s\n", c, type);
	num_lines = y_regs[i].y_size;

	/* If max_num_lines < 0, then we save ALL the lines in the register */
	if (max_num_lines > 0 && num_lines > max_num_lines)
	    num_lines = max_num_lines;
	for (j = 0; j < num_lines; j++)
	{
	    putc('\t', fp);
	    viminfo_writestring(fp, y_regs[i].y_array[j]);
	}
    }
}
#endif /* VIMINFO */

#if defined(USE_CLIPBOARD) || defined(PROTO)
/*
 * Text selection stuff that uses the GUI selection register '*'.  When using a
 * GUI this may be text from another window, otherwise it is the last text we
 * had highlighted with VIsual mode.  With mouse support, clicking the middle
 * button performs the paste, otherwise you will need to do <"*p>.
 */

    void
clip_free_selection()
{
    struct yankreg *y_ptr = y_current;

    y_current = &y_regs[CLIPBOARD_REGISTER];	    /* '*' register */
    free_yank_all();
    y_current->y_size = 0;
    y_current = y_ptr;
}

/*
 * Get the selected text and put it in the gui text register '*'.
 */
    void
clip_get_selection()
{
    struct yankreg *old_y_previous, *old_y_current;
    FPOS    old_cursor, old_visual;
    int	    old_visual_mode;
    colnr_t old_curswant;
    int	    old_set_curswant;
    OPARG   oa;
    CMDARG  ca;

    if (clipboard.owned)
    {
	if (y_regs[CLIPBOARD_REGISTER].y_array != NULL)
	    return;

	/* Get the text between clipboard.start & clipboard.end */
	old_y_previous = y_previous;
	old_y_current = y_current;
	old_cursor = curwin->w_cursor;
	old_curswant = curwin->w_curswant;
	old_set_curswant = curwin->w_set_curswant;
	old_visual = VIsual;
	old_visual_mode = VIsual_mode;
	clear_oparg(&oa);
	oa.regname = '*';
	oa.op_type = OP_YANK;
	vim_memset(&ca, 0, sizeof(ca));
	ca.oap = &oa;
	ca.cmdchar = 'y';
	ca.count1 = 1;
	do_pending_operator(&ca, NULL, NULL, 0, TRUE, TRUE);
	y_previous = old_y_previous;
	y_current = old_y_current;
	curwin->w_cursor = old_cursor;
	curwin->w_curswant = old_curswant;
	curwin->w_set_curswant = old_set_curswant;
	VIsual = old_visual;
	VIsual_mode = old_visual_mode;
    }
    else
    {
	clip_free_selection();

	/* Try to get selected text from another window */
	clip_gen_request_selection();
    }
}

/* Convert from the GUI selection string into the '*' register */
    void
clip_yank_selection(type, str, len)
    int	    type;
    char_u  *str;
    long    len;
{
    struct yankreg *y_ptr = &y_regs[CLIPBOARD_REGISTER];    /* '*' register */

    clip_free_selection();

    str_to_reg(y_ptr, type, str, len);
}

/*
 * Convert the '*' register into a GUI selection string returned in *str with
 * length *len.
 * Returns the motion type, or -1 for failure.
 */
    int
clip_convert_selection(str, len)
    char_u	**str;
    long_u	*len;
{
    struct yankreg *y_ptr = &y_regs[CLIPBOARD_REGISTER];    /* '*' register */
    char_u	*p;
    int		lnum;
    int		i, j;
    int_u	eolsize;

#ifdef USE_CRNL
    eolsize = 2;
#else
    eolsize = 1;
#endif

    *str = NULL;
    *len = 0;
    if (y_ptr->y_array == NULL)
	return -1;

    for (i = 0; i < y_ptr->y_size; i++)
	*len += STRLEN(y_ptr->y_array[i]) + eolsize;

    /*
     * Don't want newline character at end of last line if we're in MCHAR mode.
     */
    if (y_ptr->y_type == MCHAR && *len > eolsize)
	*len -= eolsize;

    p = *str = lalloc(*len, TRUE);
    if (p == NULL)
	return -1;
    lnum = 0;
    for (i = 0, j = 0; i < (int)*len; i++, j++)
    {
	if (y_ptr->y_array[lnum][j] == '\n')
	    p[i] = NUL;
	else if (y_ptr->y_array[lnum][j] == NUL)
	{
#ifdef USE_CRNL
	    p[i++] = '\r';
#endif
#ifdef USE_CR
	    p[i] = '\r';
#else
	    p[i] = '\n';
#endif
	    lnum++;
	    j = -1;
	}
	else
	    p[i] = y_ptr->y_array[lnum][j];
    }
    return y_ptr->y_type;
}
#endif /* USE_CLIPBOARD || PROTO */

#ifdef WANT_EVAL
/*
 * Return the contents of a register as a single allocated string.
 * Used for "@r" in expressions.
 * Returns NULL for error.
 */
    char_u *
get_reg_contents(regname)
    int	    regname;
{
    long    i;
    char_u  *retval;
    int	    allocated;
    long    len;

    /* Don't allow using an expression register inside an expression */
    if (regname == '=')
	return NULL;

    if (regname == '@')	    /* "@@" is used for unnamed register */
	regname = '"';

    /* check for valid regname */
    if (regname != NUL && !valid_yank_reg(regname, FALSE))
	return NULL;

#ifdef USE_CLIPBOARD
    if (regname == '*')
    {
	if (!clipboard.available)
	    regname = 0;
	else
	    clip_get_selection();		/* may fill * register */
    }
#endif

    if (get_spec_reg(regname, &retval, &allocated, FALSE))
    {
	if (retval == NULL)
	    return NULL;
	if (!allocated)
	    retval = vim_strsave(retval);
	return retval;
    }

    get_yank_register(regname, FALSE);
    if (y_current->y_array == NULL)
	return NULL;

    /*
     * Compute length of resulting string.
     */
    len = 0;
    for (i = 0; i < y_current->y_size; ++i)
    {
	len += STRLEN(y_current->y_array[i]);
	/*
	 * Insert a newline between lines and after last line if
	 * y_type is MLINE.
	 */
	if (y_current->y_type == MLINE || i < y_current->y_size - 1)
	    ++len;
    }

    retval = lalloc(len + 1, TRUE);

    /*
     * Copy the lines of the yank register into the string.
     */
    if (retval != NULL)
    {
	len = 0;
	for (i = 0; i < y_current->y_size; ++i)
	{
	    STRCPY(retval + len, y_current->y_array[i]);
	    len += STRLEN(retval + len);

	    /*
	     * Insert a NL between lines and after the last line if y_type is
	     * MLINE.
	     */
	    if (y_current->y_type == MLINE || i < y_current->y_size - 1)
		retval[len++] = '\n';
	}
	retval[len] = NUL;
    }

    return retval;
}

/*
 * Store string 'str' in register 'name'.
 * Careful: 'str' is modified, you may have to use a copy!
 * If 'str' ends in '\n' or '\r', use linewise, otherwise use characterwise.
 */
    void
write_reg_contents(name, str)
    int	    name;
    char_u  *str;
{
    struct yankreg  *old_y_previous, *old_y_current;
    long	    len;

    /* Special case: '/' search pattern */
    if (name == '/')
    {
	set_last_search_pat(str, RE_SEARCH, TRUE, TRUE);
	return;
    }

    if (!valid_yank_reg(name, TRUE))	    /* check for valid reg name */
    {
	EMSG2("Invalid register name: '%s'", transchar(name));
	return;
    }

    if (name == '_')	    /* black hole: nothing to do */
	return;

    /* Don't want to change the current (unnamed) register */
    old_y_previous = y_previous;
    old_y_current = y_current;

    get_yank_register(name, TRUE);
    if (!y_append)
	free_yank_all();
    len = STRLEN(str);
    str_to_reg(y_current,
	    (len > 0 && (str[len - 1] == '\n' || str[len -1] == '\r'))
	     ? MLINE : MCHAR, str, len);

#ifdef USE_CLIPBOARD
    /*
     * If we are writing to the clipboard register, send result to clipboard.
     */
    if (y_current == &(y_regs[CLIPBOARD_REGISTER]) && clipboard.available)
    {
	clip_own_selection();
	clip_gen_set_selection();
    }
#endif

    y_previous = old_y_previous;
    y_current = old_y_current;
}
#endif	/* WANT_EVAL */

#if defined(USE_CLIPBOARD) || defined(WANT_EVAL)
/*
 * Put a string into a register.  When the register is not empty, the string
 * is appended.
 */
    static void
str_to_reg(y_ptr, type, str, len)
    struct yankreg	*y_ptr;		/* pointer to yank register */
    int			type;		/* MCHAR or MLINE */
    char_u		*str;		/* string to put in register */
    long		len;		/* lenght of string */
{
    int	    lnum;
    long    start;
    long    i;
    int	    extra;
    int	    newlines;			/* number of lines added */
    int	    extraline = 0;		/* extra line at the end */
    int	    append = FALSE;		/* append to last line in register */
    char_u  *s;
    char_u  **pp;

    if (y_ptr->y_array == NULL)		/* NULL means emtpy register */
	y_ptr->y_size = 0;

    /*
     * Count the number of lines within the string
     */
    newlines = 0;
    for (i = 0; i < len; i++)
	if (str[i] == '\n')
	    ++newlines;
    if (type == MCHAR || len == 0 || str[len - 1] != '\n')
    {
	extraline = 1;
	++newlines;	/* count extra newline at the end */
    }
    if (y_ptr->y_size > 0 && y_ptr->y_type == MCHAR)
    {
	append = TRUE;
	--newlines;	/* uncount newline when appending first line */
    }

    /*
     * Allocate an array to hold the pointers to the new register lines.
     * If the register was not empty, move the existing lines to the new array.
     */
    pp = (char_u **)lalloc_clear((y_ptr->y_size + newlines)
						    * sizeof(char_u *), TRUE);
    if (pp == NULL)	/* out of memory */
	return;
    for (lnum = 0; lnum < y_ptr->y_size; ++lnum)
	pp[lnum] = y_ptr->y_array[lnum];
    vim_free(y_ptr->y_array);
    y_ptr->y_array = pp;

    /*
     * Find the end of each line and save it into the array.
     */
    for (start = 0; start < len + extraline; start += i + 1)
    {
	for (i = start; i < len; ++i)	/* find the end of the line */
	    if (str[i] == '\n')
		break;
	i -= start;			/* i is now length of line */
	if (append)
	{
	    --lnum;
	    extra = STRLEN(y_ptr->y_array[lnum]);
	}
	else
	    extra = 0;
	s = alloc((unsigned)(i + extra + 1));
	if (s == NULL)
	    break;
	if (extra)
	{
	    mch_memmove(s, y_ptr->y_array[lnum], (size_t)extra);
	    vim_free(y_ptr->y_array[lnum]);
	}
	if (i)
	    mch_memmove(s + extra, str + start, (size_t)i);
	extra += i;
	s[extra] = NUL;
	y_ptr->y_array[lnum++] = s;
	while (--extra >= 0)
	{
	    if (*s == NUL)
		*s = '\n';	    /* replace NUL with newline */
	    ++s;
	}
	append = FALSE;		    /* only first line is appended */
    }
    y_ptr->y_type = type;
    y_ptr->y_size = lnum;
}
#endif /* USE_CLIPBOARD || WANT_EVAL || PROTO */

    void
clear_oparg(oap)
    OPARG	*oap;
{
    vim_memset(oap, 0, sizeof(OPARG));
}
