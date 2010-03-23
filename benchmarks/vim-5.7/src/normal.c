/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Contains the main routine for processing characters in command mode.
 * Communicates closely with the code in ops.c to handle the operators.
 */

#include "vim.h"

/*
 * The Visual area is remembered for reselection.
 */
static int	resel_VIsual_mode = NUL;	/* 'v', 'V', or Ctrl-V */
static linenr_t	resel_VIsual_line_count;	/* number of lines */
static colnr_t	resel_VIsual_col;		/* nr of cols or end col */

static void	op_colon __ARGS((OPARG *oap));
#ifdef USE_MOUSE
static void	find_start_of_word __ARGS((FPOS *));
static void	find_end_of_word __ARGS((FPOS *));
static int	get_mouse_class __ARGS((int));
#endif
static void	prep_redo_cmd __ARGS((CMDARG *cap));
static void	prep_redo __ARGS((int regname, long, int, int, int, int));
static int	checkclearop __ARGS((OPARG *oap));
static int	checkclearopq __ARGS((OPARG *oap));
static void	clearop __ARGS((OPARG *oap));
static void	clearopbeep __ARGS((OPARG *oap));
#ifdef CMDLINE_INFO
static void	del_from_showcmd __ARGS((int));
#endif

/*
 * nv_*(): functions called to handle Normal and Visual mode commands.
 * n_*(): functions called to handle Normal mode commands.
 * v_*(): functions called to handle Visual mode commands.
 */
static void	nv_gd __ARGS((OPARG *oap, int nchar));
static int	nv_screengo __ARGS((OPARG *oap, int dir, long dist));
static void	nv_scroll_line __ARGS((CMDARG *cap, int is_ctrl_e));
static void	nv_zet __ARGS((CMDARG *cap));
static void	nv_colon __ARGS((CMDARG *cap));
static void	nv_ctrlg __ARGS((CMDARG *cap));
static void	nv_zzet __ARGS((CMDARG *cap));
static void	nv_ident __ARGS((CMDARG *cap, char_u **searchp));
static void	nv_scroll __ARGS((CMDARG *cap));
static void	nv_right __ARGS((CMDARG *cap));
static int	nv_left __ARGS((CMDARG *cap));
#ifdef FILE_IN_PATH
static void	nv_gotofile __ARGS((CMDARG *cap));
#endif
static void	nv_dollar __ARGS((CMDARG *cap));
static void	nv_search __ARGS((CMDARG *cap, char_u **searchp, int dont_set_mark));
static void	nv_next __ARGS((CMDARG *cap, int flag));
static void	nv_csearch __ARGS((CMDARG *cap, int dir, int type));
static void	nv_brackets __ARGS((CMDARG *cap, int dir));
static void	nv_percent __ARGS((CMDARG *cap));
static void	nv_brace __ARGS((CMDARG *cap, int dir));
static void	nv_findpar __ARGS((CMDARG *cap, int dir));
static int	nv_Replace __ARGS((CMDARG *cap));
static int	nv_VReplace __ARGS((CMDARG *cap));
static int	nv_vreplace __ARGS((CMDARG *cap));
static void	v_swap_corners __ARGS((CMDARG *cap));
static int	nv_replace __ARGS((CMDARG *cap));
static void	n_swapchar __ARGS((CMDARG *cap));
static void	nv_cursormark __ARGS((CMDARG *cap, int flag, FPOS *pos));
static void	v_visop __ARGS((CMDARG *cap));
static void	nv_optrans __ARGS((CMDARG *cap));
static void	nv_gomark __ARGS((CMDARG *cap, int flag));
static void	nv_pcmark __ARGS((CMDARG *cap));
static void	nv_regname __ARGS((CMDARG *cap, linenr_t *opnump));
static void	nv_visual __ARGS((CMDARG *cap, int selectmode));
static void	n_start_visual_mode __ARGS((int c));
static int	nv_g_cmd __ARGS((CMDARG *cap, char_u **searchp));
static int	n_opencmd __ARGS((CMDARG *cap));
static void	nv_Undo __ARGS((CMDARG *cap));
static void	nv_operator __ARGS((CMDARG *cap));
static void	nv_lineop __ARGS((CMDARG *cap));
static void	nv_pipe __ARGS((CMDARG *cap));
static void	nv_bck_word __ARGS((CMDARG *cap, int type));
static void	nv_wordcmd __ARGS((CMDARG *cap, int type));
static void	adjust_for_sel __ARGS((CMDARG *cap));
static void	unadjust_for_sel __ARGS((void));
static void	nv_goto __ARGS((CMDARG *cap, linenr_t lnum));
static void	nv_select __ARGS((CMDARG *cap));
static void	nv_normal __ARGS((CMDARG *cap));
static void	nv_esc __ARGS((CMDARG *oap, linenr_t opnum));
static int	nv_edit __ARGS((CMDARG *cap));
#ifdef TEXT_OBJECTS
static void	nv_object __ARGS((CMDARG *cap));
#endif
static void	nv_q __ARGS((CMDARG *cap));
static void	nv_at __ARGS((CMDARG *cap));
static void	nv_halfpage __ARGS((CMDARG *cap));
static void	nv_join __ARGS((CMDARG *cap));
static void	nv_put __ARGS((CMDARG *cap));

/*
 * Execute a command in Normal mode.
 *
 * This is basically a big switch with the cases arranged in rough categories
 * in the following order:
 *
 *    0. Macros (q, @)
 *    1. Screen positioning commands (^U, ^D, ^F, ^B, ^E, ^Y, z)
 *    2. Control commands (:, <help>, ^L, ^G, ^^, ZZ, *, ^], ^T)
 *    3. Cursor motions (G, H, M, L, l, K_RIGHT,  , h, K_LEFT, ^H, k, K_UP,
 *	 ^P, +, CR, LF, j, K_DOWN, ^N, _, |, B, b, W, w, E, e, $, ^, 0)
 *    4. Searches (?, /, n, N, T, t, F, f, ,, ;, ], [, %, (, ), {, })
 *    5. Edits (., u, K_UNDO, ^R, U, r, J, p, P, ^A, ^S)
 *    6. Inserts (A, a, I, i, o, O, R)
 *    7. Operators (~, d, c, y, >, <, !, =)
 *    8. Abbreviations (x, X, D, C, s, S, Y, &)
 *    9. Marks (m, ', `, ^O, ^I)
 *   10. Register name setting ('"')
 *   11. Visual (v, V, ^V)
 *   12. Suspend (^Z)
 *   13. Window commands (^W)
 *   14. extended commands (starting with 'g')
 *   15. mouse click
 *   16. scrollbar movement
 *   17. The end (ESC)
 */
/*ARGSUSED*/
    void
normal_cmd(oap, toplevel)
    OPARG	*oap;
    int		toplevel;	/* TRUE when called from main() */
{
    static linenr_t opnum = 0;		    /* count before an operator */
    CMDARG	    ca;			    /* command arguments */
    int		    c;
    int		    flag = FALSE;
    int		    type = 0;		    /* type of operation */
    int		    dir = FORWARD;	    /* search direction */
    char_u	    *searchbuff = NULL;	    /* buffer for search string */
    int		    command_busy = FALSE;
    int		    ctrl_w = FALSE;	    /* got CTRL-W command */
    int		    old_col = curwin->w_curswant;
    int		    dont_adjust_op_end = FALSE;
    FPOS	    old_pos;		    /* cursor position before command */
#ifdef CMDLINE_INFO
    int		    need_flushbuf;	    /* need to call out_flush() */
#endif
    static int	    restart_VIsual_select = 0;
    int		    mapped_len;
    static int	    old_mapped_len = 0;

#ifdef SCROLLBIND
    do_check_scrollbind(FALSE);
#endif

    vim_memset(&ca, 0, sizeof(ca));
    ca.oap = oap;

#ifdef USE_SNIFF
    want_sniff_request = sniff_connected;
#endif

    /*
     * If there is an operator pending, then the command we take this time
     * will terminate it. Finish_op tells us to finish the operation before
     * returning this time (unless the operation was cancelled).
     */
#ifdef CURSOR_SHAPE
    c = finish_op;
#endif
    finish_op = (oap->op_type != OP_NOP);
#ifdef CURSOR_SHAPE
    if (finish_op != c)
	ui_cursor_shape();		/* may show different cursor shape */
#endif

    if (!finish_op && !oap->regname)
	opnum = 0;

    mapped_len = typebuf_maplen();

    State = NORMAL_BUSY;
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = FALSE;	/* allow scrolling here */
#endif

    /*
     * Get the command character from the user.
     */
#if defined(WIN32) && defined(USE_SNIFF)
    if (sniff_request_waiting)
    {
	c = K_SNIFF;
	sniff_request_waiting = 0;
	want_sniff_request = 0;
    }
    else
#endif
	c = safe_vgetc();

#ifdef HAVE_LANGMAP
    LANGMAP_ADJUST(c, TRUE);
#endif

    /*
     * If a mapping was started in Visual or Select mode, remember the length
     * of the mapping.  This is used below to not return to Insert mode for as
     * long as the mapping is being executed.
     */
    if (!restart_edit)
	old_mapped_len = 0;
    else if (old_mapped_len
	    || (VIsual_active && mapped_len == 0 && typebuf_maplen() > 0))
	old_mapped_len = typebuf_maplen();

    if (c == NUL)
	c = K_ZERO;
    else if (c == K_KMULTIPLY)  /* K_KMULTIPLY is same as '*' */
	c = '*';
    else if (c == K_KMINUS)	/* K_KMINUS is same as '-' */
	c = '-';
    else if (c == K_KPLUS)	/* K_KPLUS is same as '+' */
	c = '+';
    else if (c == K_KDIVIDE)	/* K_KDIVIDE is same as '/' */
	c = '/';

    /*
     * In Visual/Select mode, a few keys are handled in a special way.
     */
    if (VIsual_active)
    {
	/* In Select mode, typed text replaces the selection */
	if (VIsual_select)
	{
	    if (vim_isprintc(c) || c == NL || c == CR || c == K_KENTER)
	    {
		stuffcharReadbuff(c);
		c = 'c';
	    }
	}

	/* when 'keymodel' contains "stopsel" may stop Select/Visual mode */
	if (vim_strchr(p_km, 'o') != NULL)
	{
	    switch (c)
	    {
		case K_UP:
		case Ctrl('P'):
		case K_DOWN:
		case Ctrl('N'):
		case K_LEFT:
		case K_RIGHT:
		case Ctrl('B'):
		case K_PAGEUP:
		case K_KPAGEUP:
		case Ctrl('F'):
		case K_PAGEDOWN:
		case K_KPAGEDOWN:
		case K_HOME:
		case K_KHOME:
		case K_XHOME:
		case K_END:
		case K_KEND:
		case K_XEND:
		    if (!(mod_mask & MOD_MASK_SHIFT))
		    {
			end_visual_mode();
			redraw_curbuf_later(NOT_VALID);
		    }
		    break;
	    }
	}

	/* Keys that work different when 'keymodel' contains "startsel" */
	if (vim_strchr(p_km, 'a') != NULL)
	{
	    switch (c)
	    {
		case K_S_UP:	c = K_UP; break;
		case K_S_DOWN:	c = K_DOWN; break;
		case K_S_LEFT:	c = K_LEFT; break;
		case K_S_RIGHT:	c = K_RIGHT; break;
		case K_S_HOME:	c = K_HOME; break;
		case K_S_END:	c = K_END; break;

		case K_PAGEUP:
		case K_KPAGEUP:
		case K_PAGEDOWN:
		case K_KPAGEDOWN:
		case K_KHOME:
		case K_XHOME:
		case K_KEND:
		case K_XEND:
				mod_mask &= !MOD_MASK_SHIFT; break;
	    }
	}
    }

#ifdef CMDLINE_INFO
    need_flushbuf = add_to_showcmd(c);
#endif

getcount:
    if (!(VIsual_active && VIsual_select))
    {
	/* Pick up any leading digits and compute ca.count0 */
	while (    (c >= '1' && c <= '9')
		|| (ca.count0 != 0 && (c == K_DEL || c == K_KDEL || c == '0')))
	{
	    if (c == K_DEL || c == K_KDEL)
	    {
		ca.count0 /= 10;
#ifdef CMDLINE_INFO
		del_from_showcmd(4);	/* delete the digit and ~@% */
#endif
	    }
	    else
		ca.count0 = ca.count0 * 10 + (c - '0');
	    if (ca.count0 < 0)	    /* got too large! */
		ca.count0 = 999999999;
	    if (ctrl_w)
	    {
		++no_mapping;
		++allow_keys;		/* no mapping for nchar, but keys */
	    }
	    c = safe_vgetc();
#ifdef HAVE_LANGMAP
	    LANGMAP_ADJUST(c, TRUE);
#endif
	    if (ctrl_w)
	    {
		--no_mapping;
		--allow_keys;
	    }
#ifdef CMDLINE_INFO
	    need_flushbuf |= add_to_showcmd(c);
#endif
	}

    /*
     * If we got CTRL-W there may be a/another count
     */
	if (c == Ctrl('W') && !ctrl_w && oap->op_type == OP_NOP)
	{
	    ctrl_w = TRUE;
	    opnum = ca.count0;		/* remember first count */
	    ca.count0 = 0;
	    ++no_mapping;
	    ++allow_keys;		/* no mapping for nchar, but keys */
	    c = safe_vgetc();		/* get next character */
#ifdef HAVE_LANGMAP
	    LANGMAP_ADJUST(c, TRUE);
#endif
	    --no_mapping;
	    --allow_keys;
#ifdef CMDLINE_INFO
	    need_flushbuf |= add_to_showcmd(c);
#endif
	    goto getcount;		/* jump back */
	}
    }

    ca.cmdchar = c;

    /*
     * If we're in the middle of an operator (including after entering a yank
     * buffer with '"') AND we had a count before the
     * operator, then that count overrides the current value of ca.count0.
     * What * this means effectively, is that commands like "3dw" get turned
     * into "d3w" which makes things fall into place pretty neatly.
     * If you give a count before AND after the operator, they are multiplied.
     */
    if (opnum != 0)
    {
	    if (ca.count0)
		ca.count0 *= opnum;
	    else
		ca.count0 = opnum;
    }

    /*
     * Always remember the count.  It will be set to zero (on the next call,
     * above) when there is no pending operator.
     * When called from main(), save the count for use by the "count" built-in
     * variable.
     */
    opnum = ca.count0;
    ca.count1 = (ca.count0 == 0 ? 1 : ca.count0);

#ifdef WANT_EVAL
    if (toplevel)
    {
	set_vim_var_nr(VV_COUNT, ca.count0);
	set_vim_var_nr(VV_COUNT1, ca.count1);
    }
#endif

    /*
     * Get an additional character if we need one.
     * For CTRL-W we already got it when looking for a count.
     */
    if (ctrl_w)
    {
	ca.nchar = ca.cmdchar;
	ca.cmdchar = Ctrl('W');
    }
    else if (  (oap->op_type == OP_NOP
		&& vim_strchr((char_u *)"@zm\"", ca.cmdchar) != NULL)
	    || (oap->op_type == OP_NOP
		&& (ca.cmdchar == 'r'
		    || (!VIsual_active && ca.cmdchar == 'Z')))
	    || vim_strchr((char_u *)"tTfF[]g'`", ca.cmdchar) != NULL
	    || (ca.cmdchar == 'q'
		&& oap->op_type == OP_NOP
		&& !Recording
		&& !Exec_reg)
	    || ((ca.cmdchar == 'a' || ca.cmdchar == 'i')
		&& (oap->op_type != OP_NOP || VIsual_active)))
    {
	++no_mapping;
	++allow_keys;		/* no mapping for nchar, but allow key codes */
	if (ca.cmdchar == 'g')
	    ca.nchar = safe_vgetc();
#ifdef CURSOR_SHAPE
	if (ca.cmdchar == 'r' || (ca.cmdchar == 'g' && ca.nchar == 'r'))
	{
	    State = REPLACE;	/* pretend Replace mode, for cursor shape */
	    ui_cursor_shape();	/* show different cursor shape */
	}
#endif
	/* For 'g' commands, already got next char above, "gr" still needs an
	 * extra one though.
	 */
	if (ca.cmdchar != 'g')
	    ca.nchar = safe_vgetc();
	else if (ca.nchar == 'r')
	    ca.extra_char = safe_vgetc();
#ifdef CURSOR_SHAPE
	State = NORMAL_BUSY;
#endif
#ifdef HAVE_LANGMAP
	/* adjust chars > 127, except after tTfFr or "gr" command */
	LANGMAP_ADJUST(ca.nchar,
			   vim_strchr((char_u *)"tTfFrv", ca.cmdchar) == NULL);
#endif
#ifdef RIGHTLEFT
	/* adjust Hebrew mapped char */
	if (p_hkmap && vim_strchr((char_u *)"tTfFrv", ca.cmdchar) && KeyTyped)
	    ca.nchar = hkmap(ca.nchar);
# ifdef FKMAP
	/* adjust Farsi mapped char */
	if (p_fkmap && strchr("tTfFrv", ca.cmdchar) && KeyTyped)
	    ca.nchar = fkmap(ca.nchar);
# endif
#endif
	--no_mapping;
	--allow_keys;
#ifdef CMDLINE_INFO
	need_flushbuf |= add_to_showcmd(ca.nchar);
	if (ca.cmdchar == 'g' && ca.nchar == 'r')
	    need_flushbuf |= add_to_showcmd(ca.extra_char);
#endif
    }

#ifdef CMDLINE_INFO
    /*
     * Flush the showcmd characters onto the screen so we can see them while
     * the command is being executed.  Only do this when the shown command was
     * actually displayed, otherwise this will slow down a lot when executing
     * mappings.
     */
    if (need_flushbuf)
	out_flush();
#endif

    State = NORMAL;
    if (ca.nchar == ESC)
    {
	clearop(oap);
	if (p_im && !restart_edit)
	    restart_edit = 'a';
	goto normal_end;
    }

#ifdef RIGHTLEFT
    if (curwin->w_p_rl && KeyTyped)	/* invert horizontal operations */
	switch (ca.cmdchar)
	{
	    case 'l':	    ca.cmdchar = 'h'; break;
	    case K_RIGHT:   ca.cmdchar = K_LEFT; break;
	    case K_S_RIGHT: ca.cmdchar = K_S_LEFT; break;
	    case 'h':	    ca.cmdchar = 'l'; break;
	    case K_LEFT:    ca.cmdchar = K_RIGHT; break;
	    case K_S_LEFT:  ca.cmdchar = K_S_RIGHT; break;
	    case '>':	    ca.cmdchar = '<'; break;
	    case '<':	    ca.cmdchar = '>'; break;
	}
#endif

    /* when 'keymodel' contains "startsel" some keys start Select/Visual mode */
    if (!VIsual_active && vim_strchr(p_km, 'a') != NULL)
    {
	static int seltab[] =
	{
	    /* key		unshifted	shift included */
	    K_S_RIGHT,		K_RIGHT,	TRUE,
	    K_S_LEFT,		K_LEFT,		TRUE,
	    K_S_UP,		K_UP,		TRUE,
	    K_S_DOWN,		K_DOWN,		TRUE,
	    K_S_HOME,		K_HOME,		TRUE,
	    K_S_END,		K_END,		TRUE,
	    K_KHOME,		K_KHOME,	FALSE,
	    K_XHOME,		K_XHOME,	FALSE,
	    K_KEND,		K_KEND,		FALSE,
	    K_XEND,		K_KEND,		FALSE,
	    K_PAGEUP,		K_PAGEUP,	FALSE,
	    K_KPAGEUP,		K_KPAGEUP,	FALSE,
	    K_PAGEDOWN,		K_PAGEDOWN,	FALSE,
	    K_KPAGEDOWN,	K_KPAGEDOWN,	FALSE,
#ifdef macintosh
	    K_RIGHT,		K_RIGHT,	FALSE,
	    K_LEFT,		K_LEFT,		FALSE,
	    K_UP,		K_UP,		FALSE,
	    K_DOWN,		K_DOWN,		FALSE,
	    K_HOME,		K_HOME,		FALSE,
	    K_END,		K_END,		FALSE,
#endif
	};
	int	i;

	for (i = 0; i < sizeof(seltab) / sizeof(int); i += 3)
	{
	    if (seltab[i] == ca.cmdchar
		    && (seltab[i + 2] || (mod_mask & MOD_MASK_SHIFT)))
	    {
		ca.cmdchar = seltab[i + 1];
		start_selection();
		break;
	    }
	}
    }

    msg_didout = FALSE;	    /* don't scroll screen up for normal command */
    msg_col = 0;
    old_pos = curwin->w_cursor;		/* remember where cursor was */

/*
 * Generally speaking, every command below should either clear any pending
 * operator (with *clearop*()), or set the motion type variable
 * oap->motion_type.
 *
 * When a cursor motion command is made, it is marked as being a character or
 * line oriented motion.  Then, if an operator is in effect, the operation
 * becomes character or line oriented accordingly.
 */
/*
 * Variables available here:
 * ca.cmdchar	command character
 * ca.nchar	extra command character
 * ca.count0	count before command (0 if no count given)
 * ca.count1	count before command (1 if no count given)
 * oap		Operator Arguments (same as ca.oap)
 * flag		is FALSE, use as you like.
 * dir		is FORWARD, use as you like.
 */
    switch (ca.cmdchar)
    {
/*
 * 0: Macros
 */
    case 'q':
	nv_q(&ca);
	break;

    case '@':		/* execute a named register */
	nv_at(&ca);
	break;

/*
 * 1: Screen positioning commands
 */
    case Ctrl('D'):
    case Ctrl('U'):
	nv_halfpage(&ca);
	break;

    case Ctrl('B'):
    case K_S_UP:
    case K_PAGEUP:
    case K_KPAGEUP:
	dir = BACKWARD;

    case Ctrl('F'):
    case K_S_DOWN:
    case K_PAGEDOWN:
    case K_KPAGEDOWN:
	if (checkclearop(oap))
	    break;
	(void)onepage(dir, ca.count1);
	break;

    case Ctrl('E'):
	flag = TRUE;
	/* FALLTHROUGH */

    case Ctrl('Y'):
	nv_scroll_line(&ca, flag);
	break;

#ifdef USE_MOUSE
    /* Mouse scroll wheel: default action is to scroll three lines, or one
     * page when Shift is used. */
    case K_MOUSEUP:
	flag = TRUE;
    case K_MOUSEDOWN:
	if (mod_mask & MOD_MASK_SHIFT)
	{
	    (void)onepage(flag ? FORWARD : BACKWARD, 1L);
	}
	else
	{
	    ca.count1 = 3;
	    ca.count0 = 3;
	    nv_scroll_line(&ca, flag);
	}
	break;
#endif

    case 'z':
	if (!checkclearop(oap))
	    nv_zet(&ca);
	break;

/*
 * 2: Control commands
 */
    case ':':
	   nv_colon(&ca);
	   break;

    case 'Q':
	/*
	 * Ignore 'Q' in Visual mode, just give a beep.
	 */
	if (VIsual_active)
	    vim_beep();
	else if (!checkclearop(oap))
	    do_exmode();
	break;

    case K_HELP:
    case K_F1:
    case K_XF1:
	if (!checkclearopq(oap))
	    do_help(NULL);
	break;

    case Ctrl('L'):
	if (!checkclearop(oap))
	{
#if defined(__BEOS__) && !USE_THREAD_FOR_INPUT_WITH_TIMEOUT
	    /*
	     * Right now, the BeBox doesn't seem to have an easy way to detect
	     * window resizing, so we cheat and make the user detect it
	     * manually with CTRL-L instead
	     */
	    ui_get_winsize();
#endif
	    update_screen(CLEAR);
	}
	break;

    case Ctrl('G'):
	nv_ctrlg(&ca);
	break;

    case K_CCIRCM:	    /* CTRL-^, short for ":e #" */
	if (!checkclearopq(oap))
	    (void)buflist_getfile((int)ca.count0, (linenr_t)0,
						GETF_SETMARK|GETF_ALT, FALSE);
	break;

    case 'Z':
	nv_zzet(&ca);
	break;

    case 163:			/* the pound sign, '#' for English keyboards */
	ca.cmdchar = '#';
	/*FALLTHROUGH*/

    case Ctrl(']'):		/* :ta to current identifier */
    case 'K':			/* run program for current identifier */
    case '*':			/* / to current identifier or string */
    case K_KMULTIPLY:		/* same as '*' */
    case '#':			/* ? to current identifier or string */
	if (ca.cmdchar == K_KMULTIPLY)
	    ca.cmdchar = '*';
	nv_ident(&ca, &searchbuff);
	break;

    case Ctrl('T'):		/* backwards in tag stack */
	if (!checkclearopq(oap))
	    do_tag((char_u *)"", DT_POP, (int)ca.count1, FALSE, TRUE);
	break;

/*
 * 3. Cursor motions
 */
    case 'G':
	nv_goto(&ca, curbuf->b_ml.ml_line_count);
	break;

    case 'H':
    case 'M':
    case 'L':
	nv_scroll(&ca);
	break;

    case K_RIGHT:
	if (mod_mask & MOD_MASK_CTRL)
	{
	    oap->inclusive = FALSE;
	    nv_wordcmd(&ca, 1);
	    break;
	}
    case 'l':
    case ' ':
	nv_right(&ca);
	break;

    case K_LEFT:
	if (mod_mask & MOD_MASK_CTRL)
	{
	    nv_bck_word(&ca, 1);
	    break;
	}
    case 'h':
	dont_adjust_op_end = nv_left(&ca);
	break;

    case K_BS:
    case Ctrl('H'):
	if (VIsual_active && VIsual_select)
	{
	    ca.cmdchar = 'x';	/* BS key behaves like 'x' in Select mode */
	    v_visop(&ca);
	}
	else
	    dont_adjust_op_end = nv_left(&ca);
	break;

    case '-':
    case K_KMINUS:
	flag = TRUE;
	/* FALLTHROUGH */

    case 'k':
    case K_UP:
    case Ctrl('P'):
	oap->motion_type = MLINE;
	if (cursor_up(ca.count1, oap->op_type == OP_NOP) == FAIL)
	    clearopbeep(oap);
	else if (flag)
	    beginline(BL_WHITE | BL_FIX);
	break;

    case '+':
    case K_KPLUS:
    case CR:
    case K_KENTER:
	flag = TRUE;
	/* FALLTHROUGH */

    case 'j':
    case K_DOWN:
    case Ctrl('N'):
    case NL:
	oap->motion_type = MLINE;
	if (cursor_down(ca.count1, oap->op_type == OP_NOP) == FAIL)
	    clearopbeep(oap);
	else if (flag)
	    beginline(BL_WHITE | BL_FIX);
	break;

	/*
	 * This is a strange motion command that helps make operators more
	 * logical. It is actually implemented, but not documented in the
	 * real Vi. This motion command actually refers to "the current
	 * line". Commands like "dd" and "yy" are really an alternate form of
	 * "d_" and "y_". It does accept a count, so "d3_" works to delete 3
	 * lines.
	 */
    case '_':
	nv_lineop(&ca);
	break;

    case K_HOME:
    case K_KHOME:
    case K_XHOME:
    case K_S_HOME:
	if ((mod_mask & MOD_MASK_CTRL))	    /* CTRL-HOME = goto line 1 */
	{
	    nv_goto(&ca, (linenr_t)1);
	    break;
	}
	ca.count0 = 1;
	/* FALLTHROUGH */

    case '|':
	nv_pipe(&ca);
	break;

    /*
     * Word Motions
     */
    case 'B':
	type = 1;
	/* FALLTHROUGH */

    case 'b':
    case K_S_LEFT:
	nv_bck_word(&ca, type);
	break;

    case 'E':
	type = TRUE;
	/* FALLTHROUGH */

    case 'e':
	oap->inclusive = TRUE;
	nv_wordcmd(&ca, type);
	break;

    case 'W':
	type = TRUE;
	/* FALLTHROUGH */

    case 'w':
    case K_S_RIGHT:
	oap->inclusive = FALSE;
	nv_wordcmd(&ca, type);
	break;

    case K_END:
    case K_KEND:
    case K_XEND:
    case K_S_END:
	if ((mod_mask & MOD_MASK_CTRL))	    /* CTRL-END = goto last line */
	{
	    nv_goto(&ca, curbuf->b_ml.ml_line_count);
	    ca.count1 = 1;	/* to end of current line */
	}
	/* FALLTHROUGH */

    case '$':
	nv_dollar(&ca);
	break;

    case '^':
	flag = BL_WHITE | BL_FIX;
	/* FALLTHROUGH */

    case '0':
	oap->motion_type = MCHAR;
	oap->inclusive = FALSE;
	beginline(flag);
	break;

/*
 * 4: Searches
 */
    case K_KDIVIDE:
	ca.cmdchar = '/';
	/* FALLTHROUGH */
    case '?':
    case '/':
	nv_search(&ca, &searchbuff, FALSE);
	break;

    case 'N':
	flag = SEARCH_REV;
	/* FALLTHROUGH */

    case 'n':
	nv_next(&ca, flag);
	break;

	/*
	 * Character searches
	 */
    case 'T':
	dir = BACKWARD;
	/* FALLTHROUGH */

    case 't':
	nv_csearch(&ca, dir, TRUE);
	break;

    case 'F':
	dir = BACKWARD;
	/* FALLTHROUGH */

    case 'f':
	nv_csearch(&ca, dir, FALSE);
	break;

    case ',':
	flag = 1;
	/* FALLTHROUGH */

    case ';':
	/* ca.nchar == NUL, thus repeat previous search */
	nv_csearch(&ca, flag, FALSE);
	break;

	/*
	 * section or C function searches
	 */
    case '[':
	dir = BACKWARD;
	/* FALLTHROUGH */

    case ']':
	nv_brackets(&ca, dir);
	break;

    case '%':
	nv_percent(&ca);
	break;

    case '(':
	dir = BACKWARD;
	/* FALLTHROUGH */

    case ')':
	nv_brace(&ca, dir);
	break;

    case '{':
	dir = BACKWARD;
	/* FALLTHROUGH */

    case '}':
	nv_findpar(&ca, dir);
	break;

/*
 * 5: Edits
 */
    case '.':		    /* redo command */
	if (!checkclearopq(oap))
	{
	    /*
	     * if restart_edit is TRUE, the last but one command is repeated
	     * instead of the last command (inserting text). This is used for
	     * CTRL-O <.> in insert mode
	     */
	    if (start_redo(ca.count0, restart_edit && !arrow_used) == FAIL)
		clearopbeep(oap);
	}
	break;

    case 'u':		    /* undo */
	if (VIsual_active || oap->op_type == OP_LOWER)
	{
	    /* translate "<Visual>u" to "<Visual>gu" and "guu" to "gugu" */
	    ca.cmdchar = 'g';
	    ca.nchar = 'u';
	    nv_operator(&ca);
	    break;
	}
	/* FALLTHROUGH */

    case K_UNDO:
	if (!checkclearopq(oap))
	{
	    u_undo((int)ca.count1);
	    curwin->w_set_curswant = TRUE;
	}
	break;

    case Ctrl('R'):	/* undo undo */
	if (!checkclearopq(oap))
	{
	    u_redo((int)ca.count1);
	    curwin->w_set_curswant = TRUE;
	}
	break;

    case 'U':		/* Undo line */
	nv_Undo(&ca);
	break;

    case 'r':		/* replace character */
	command_busy = nv_replace(&ca);
	break;

    case 'J':
	nv_join(&ca);
	break;

    case 'P':
    case 'p':
	nv_put(&ca);
	break;

    case Ctrl('A'):	    /* add to number */
    case Ctrl('X'):	    /* subtract from number */
	if (!checkclearopq(oap) && do_addsub((int)ca.cmdchar, ca.count1) == OK)
	    prep_redo_cmd(&ca);
	break;

/*
 * 6: Inserts
 */
    case 'A':
    case 'a':
    case 'I':
    case 'i':
    case K_INS:
    case K_KINS:
	command_busy = nv_edit(&ca);
	break;

    case 'o':
    case 'O':
	if (VIsual_active)  /* switch start and end of visual */
	    v_swap_corners(&ca);
	else
	    command_busy = n_opencmd(&ca);
	break;

    case 'R':
	command_busy = nv_Replace(&ca);
	break;

/*
 * 7: Operators
 */
    case '~':	    /* swap case */
	/*
	 * if tilde is not an operator and Visual is off: swap case
	 * of a single character
	 */
	if (	   !p_to
		&& !VIsual_active
		&& oap->op_type != OP_TILDE)
	{
	    n_swapchar(&ca);
	    break;
	}
	/*FALLTHROUGH*/

    case 'd':
    case 'c':
    case 'y':
    case '>':
    case '<':
    case '!':
    case '=':
	nv_operator(&ca);
	break;

/*
 * 8: Abbreviations
 */

    case 'S':
    case 's':
	if (VIsual_active)	/* "vs" and "vS" are the same as "vc" */
	{
	    if (ca.cmdchar == 'S')
		VIsual_mode = 'V';
	    ca.cmdchar = 'c';
	    nv_operator(&ca);
	    break;
	}
	/* FALLTHROUGH */
    case K_DEL:
    case K_KDEL:
    case 'Y':
    case 'D':
    case 'C':
    case 'x':
    case 'X':
	if (ca.cmdchar == K_DEL || ca.cmdchar == K_KDEL)
	    ca.cmdchar = 'x';		/* DEL key behaves like 'x' */

	/* with Visual these commands are operators */
	if (VIsual_active)
	{
	    v_visop(&ca);
	    break;
	}
	/* FALLTHROUGH */

    case '&':
	nv_optrans(&ca);
	opnum = 0;
	break;

/*
 * 9: Marks
 */
    case 'm':
	if (!checkclearop(oap))
	{
	    if (setmark(ca.nchar) == FAIL)
		clearopbeep(oap);
	}
	break;

    case '\'':
	flag = TRUE;
	/* FALLTHROUGH */

    case '`':
	nv_gomark(&ca, flag);
	break;

    case Ctrl('O'):
	/* switch from Select to Visual mode for one command */
	if (VIsual_active && VIsual_select)
	{
	    VIsual_select = FALSE;
	    showmode();
	    restart_VIsual_select = 2;
	    break;
	}
	ca.count1 = -ca.count1;	/* goto older pcmark */
	/* FALLTHROUGH */

    case Ctrl('I'):		/* goto newer pcmark */
	nv_pcmark(&ca);
	break;

/*
 * 10. Register name setting
 */
    case '"':
	nv_regname(&ca, &opnum);
	break;

/*
 * 11. Visual
 */
    case 'v':
    case 'V':
    case Ctrl('V'):
	if (!checkclearop(oap))
	    nv_visual(&ca, FALSE);
	break;

/*
 * 12. Suspend
 */

    case Ctrl('Z'):
	clearop(oap);
	if (VIsual_active)
	    end_visual_mode();		    /* stop Visual */
	stuffReadbuff((char_u *)":st\r");   /* with autowrite */
	break;

/*
 * 13. Window commands
 */

    case Ctrl('W'):
	if (!checkclearop(oap))
	    do_window(ca.nchar, ca.count0);	/* everything is in window.c */
	break;

/*
 *   14. extended commands (starting with 'g')
 */
    case 'g':
	command_busy = nv_g_cmd(&ca, &searchbuff);
	break;

/*
 * 15. mouse click
 */
#ifdef USE_MOUSE
    case K_LEFTMOUSE:
    case K_LEFTMOUSE_NM:
    case K_LEFTDRAG:
    case K_LEFTRELEASE:
    case K_LEFTRELEASE_NM:
    case K_MIDDLEMOUSE:
    case K_MIDDLEDRAG:
    case K_MIDDLERELEASE:
    case K_RIGHTMOUSE:
    case K_RIGHTDRAG:
    case K_RIGHTRELEASE:
	(void)do_mouse(oap, ca.cmdchar, BACKWARD, ca.count1, 0);
	break;

    case K_IGNORE:
	break;
#endif

#ifdef USE_GUI
/*
 * 16. scrollbar movement
 */
    case K_SCROLLBAR:
	if (oap->op_type != OP_NOP)
	    clearopbeep(oap);

	/* Even if an operator was pending, we still want to scroll */
	gui_do_scroll();
	break;

    case K_HORIZ_SCROLLBAR:
	if (oap->op_type != OP_NOP)
	    clearopbeep(oap);

	/* Even if an operator was pending, we still want to scroll */
	gui_do_horiz_scroll();
	break;
#endif

    case K_SELECT:	    /* end of Select mode mapping */
	nv_select(&ca);
	break;

#ifdef FKMAP
    case K_F8:
    case K_F9:
	farsi_fkey(ca.cmdchar);
	break;
#endif

#ifdef USE_SNIFF
    case K_SNIFF:
	ProcessSniffRequests();
	break;
#endif

/*
 * 17. The end
 */
	/* CTRL-\ CTRL-N goes to Normal mode: a no-op */
    case Ctrl('\\'):
	nv_normal(&ca);
	break;

    case Ctrl('C'):
	restart_edit = 0;
	/*FALLTHROUGH*/

    case ESC:
	nv_esc(&ca, opnum);
	break;

    default:			/* not a known command */
	clearopbeep(oap);
	break;

    }	/* end of switch on command character */

/*
 * if we didn't start or finish an operator, reset oap->regname, unless we
 * need it later.
 */
    if (!finish_op && !oap->op_type &&
		       vim_strchr((char_u *)"\"DCYSsXx.", ca.cmdchar) == NULL)
	oap->regname = 0;

/*
 * If an operation is pending, handle it...
 */
    do_pending_operator(&ca, searchbuff,
			   &command_busy, old_col, FALSE, dont_adjust_op_end);

    /*
     * Wait when a message is displayed that will be overwritten by the mode
     * message.
     * In Visual mode and with "^O" in Insert mode, a short message will be
     * overwritten by the mode message.  Wait a bit, until a key is hit.
     * In Visual mode, it's more important to keep the Visual area updated
     * than keeping a message (e.g. from a /pat search).
     * Only do this if the command was typed, not from a mapping.
     * Also wait a bit after an error message, e.g. for "^O:".
     * Don't redraw the screen, it would remove the message.
     */
    if (       ((p_smd
		    && ((VIsual_active
			    && old_pos.lnum == curwin->w_cursor.lnum
			    && old_pos.col == curwin->w_cursor.col)
			|| restart_edit)
		    && (clear_cmdline
			|| redraw_cmdline)
		    && msg_didany
		    && !msg_nowait
		    && KeyTyped)
		|| (restart_edit
		    && !VIsual_active
		    && (msg_scroll
			|| emsg_on_display)))
	    && oap->regname == 0
	    && !command_busy
	    && stuff_empty()
	    && typebuf_typed()
	    && oap->op_type == OP_NOP)
    {
	int	save_State = State;

	/* Draw the cursor with the right shape here */
	if (restart_edit)
	    State = INSERT;

	/* If need to redraw, and there is a "keep_msg", redraw before the
	 * delay */
	if (must_redraw && keep_msg != NULL && !emsg_on_display)
	{
	    char_u	*kmsg = keep_msg;

	    /* showmode() will clear keep_msg, but we want to use it anyway */
	    update_screen(must_redraw);
	    msg_attr(kmsg, keep_msg_attr);
	}
	setcursor();
	cursor_on();
	out_flush();
	if (msg_scroll || emsg_on_display)
	    ui_delay(1000L, TRUE);	/* wait at least one second */
	ui_delay(3000L, FALSE);		/* wait up to three seconds */
	State = save_State;

	msg_scroll = FALSE;
	emsg_on_display = FALSE;
    }

    /*
     * Finish up after executing a Normal mode command.
     */
normal_end:

    msg_nowait = FALSE;

    /* Reset finish_op, in case it was set */
#ifdef USE_GUI
    c = finish_op;
#endif
    finish_op = FALSE;
#ifdef CURSOR_SHAPE
    /* Redraw the cursor with another shape, if we were in Operator-pending
     * mode or did a replace command. */
    if ((c && !finish_op) || ca.cmdchar == 'r')
	ui_cursor_shape();		/* may show different cursor shape */
#endif

#ifdef CMDLINE_INFO
    if (oap->op_type == OP_NOP && oap->regname == 0)
	clear_showcmd();
#endif

    /*
     * Update the other windows for the current buffer if modified has been
     * set in set_Changed() (This should be done more efficiently)
     */
    if (modified)
    {
	update_other_win();
	modified = FALSE;
    }

    checkpcmark();		/* check if we moved since setting pcmark */
    vim_free(searchbuff);

#ifdef SCROLLBIND
    if (curwin->w_p_scb)
    {
	validate_cursor();	/* may need to update w_leftcol */
	do_check_scrollbind(TRUE);
    }
#endif

    /*
     * May restart edit(), if we got here with CTRL-O in Insert mode (but not
     * if still inside a mapping that started in Visual mode).
     * May switch from Visual to Select mode after CTRL-O command.
     */
    if (      ((restart_edit && !VIsual_active && old_mapped_len == 0)
		|| restart_VIsual_select == 1)
	    && oap->op_type == OP_NOP
	    && !command_busy
	    && stuff_empty()
	    && oap->regname == 0)
    {
	if (restart_VIsual_select == 1)
	{
	    VIsual_select = TRUE;
	    showmode();
	    restart_VIsual_select = 0;
	}
	if (restart_edit && !VIsual_active && old_mapped_len == 0)
	{
	    if (must_redraw)
		update_screen(must_redraw);
	    (void)edit(restart_edit, FALSE, 1L);
	}
    }

    if (restart_VIsual_select == 2)
	restart_VIsual_select = 1;

#ifdef MULTI_BYTE
    if (is_dbcs)
	AdjustCursorForMultiByteChar();
#endif
}

/*
 * Handle an operator after visual mode or when the movement is finished
 */
    void
do_pending_operator(cap, searchbuff,
			  command_busy, old_col, gui_yank, dont_adjust_op_end)
    CMDARG	*cap;
    char_u	*searchbuff;
    int		*command_busy;
    int		old_col;
    int		gui_yank;
    int		dont_adjust_op_end;
{
    OPARG	*oap = cap->oap;
    FPOS	old_cursor;
    int		empty_region_error;

    /* The visual area is remembered for redo */
    static int	    redo_VIsual_mode = NUL; /* 'v', 'V', or Ctrl-V */
    static linenr_t redo_VIsual_line_count; /* number of lines */
    static colnr_t  redo_VIsual_col;	    /* number of cols or end column */
    static long	    redo_VIsual_count;	    /* count for Visual operator */

#if defined(USE_CLIPBOARD)
    /*
     * Yank the visual area into the GUI selection register before we operate
     * on it and lose it forever.
     * Don't do it if a specific register was specified, so that ""x"*P works.
     * This could call do_pending_operator() recursively, but that's OK
     * because gui_yank will be TRUE for the nested call.
     */
    if (clipboard.available
	    && oap->op_type != OP_NOP
	    && !gui_yank
	    && VIsual_active
	    && oap->regname == 0
	    && !redo_VIsual_busy)
	clip_auto_select();
#endif
    old_cursor = curwin->w_cursor;

    /*
     * If an operation is pending, handle it...
     */
    if ((VIsual_active || finish_op) && oap->op_type != OP_NOP)
    {
	oap->is_VIsual = VIsual_active;

	/* only redo yank when 'y' flag is in 'cpoptions' */
	if ((vim_strchr(p_cpo, CPO_YANK) != NULL || oap->op_type != OP_YANK)
		&& !VIsual_active)
	{
	    prep_redo(oap->regname, cap->count0,
		    get_op_char(oap->op_type), get_extra_op_char(oap->op_type),
		    cap->cmdchar, cap->nchar);
	    if (cap->cmdchar == '/' || cap->cmdchar == '?') /* was a search */
	    {
		/*
		 * If 'cpoptions' does not contain 'r', insert the search
		 * pattern to really repeat the same command.
		 */
		if (vim_strchr(p_cpo, CPO_REDO) == NULL)
		    AppendToRedobuff(searchbuff);
		AppendToRedobuff(NL_STR);
	    }
	}

	if (redo_VIsual_busy)
	{
	    oap->start = curwin->w_cursor;
	    curwin->w_cursor.lnum += redo_VIsual_line_count - 1;
	    if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    VIsual_mode = redo_VIsual_mode;
	    if (VIsual_mode == 'v')
	    {
		if (redo_VIsual_line_count <= 1)
		    curwin->w_cursor.col += redo_VIsual_col - 1;
		else
		    curwin->w_cursor.col = redo_VIsual_col;
	    }
	    if (redo_VIsual_col == MAXCOL)
	    {
		curwin->w_curswant = MAXCOL;
		coladvance(MAXCOL);
	    }
	    cap->count0 = redo_VIsual_count;
	    if (redo_VIsual_count != 0)
		cap->count1 = redo_VIsual_count;
	    else
		cap->count1 = 1;
	}
	else if (VIsual_active)
	{
	    /* In Select mode, a linewise selection is operated upon like a
	     * characterwise selection. */
	    if (VIsual_select && VIsual_mode == 'V')
	    {
		if (lt(VIsual, curwin->w_cursor))
		{
		    VIsual.col = 0;
		    curwin->w_cursor.col =
					STRLEN(ml_get(curwin->w_cursor.lnum));
		}
		else
		{
		    curwin->w_cursor.col = 0;
		    VIsual.col = STRLEN(ml_get(VIsual.lnum));
		}
		VIsual_mode = 'v';
	    }
	    /* If 'selection' is "exclusive", backup one character for
	     * charwise selections. */
	    else if (VIsual_mode == 'v')
		unadjust_for_sel();

	    /* Save the current VIsual area for '< and '> marks, and "gv" */
	    curbuf->b_visual_start = VIsual;
	    curbuf->b_visual_end = curwin->w_cursor;
	    curbuf->b_visual_mode = VIsual_mode;

	    oap->start = VIsual;
	    if (VIsual_mode == 'V')
		oap->start.col = 0;
	}

	/*
	 * Set oap->start to the first position of the operated text, oap->end
	 * to the end of the operated text.  w_cursor is equal to oap->start.
	 */
	if (lt(oap->start, curwin->w_cursor))
	{
	    oap->end = curwin->w_cursor;
	    curwin->w_cursor = oap->start;
	}
	else
	{
	    oap->end = oap->start;
	    oap->start = curwin->w_cursor;
	}
#ifdef MULTI_BYTE
	if (is_dbcs && (VIsual_active || oap->inclusive))
	{
	    char_u *p;

	    p = ml_get(oap->end.lnum);
	    if (IsTrailByte(p, p + oap->end.col + 1))
		oap->end.col++;
	}
#endif
	oap->line_count = oap->end.lnum - oap->start.lnum + 1;

	if (VIsual_active || redo_VIsual_busy)
	{
	    if (VIsual_mode == Ctrl('V'))	/* block mode */
	    {
		colnr_t	    start, end;

		oap->block_mode = TRUE;

		getvcol(curwin, &(oap->start),
				      &oap->start_vcol, NULL, &oap->end_vcol);
		if (!redo_VIsual_busy)
		{
		    getvcol(curwin, &(oap->end), &start, NULL, &end);
		    if (start < oap->start_vcol)
			oap->start_vcol = start;
		    if (end > oap->end_vcol)
		    {
			if (*p_sel == 'e' && start - 1 >= oap->end_vcol)
			    oap->end_vcol = start - 1;
			else
			    oap->end_vcol = end;
		    }
		}

		/* if '$' was used, get oap->end_vcol from longest line */
		if (curwin->w_curswant == MAXCOL)
		{
		    curwin->w_cursor.col = MAXCOL;
		    oap->end_vcol = 0;
		    for (curwin->w_cursor.lnum = oap->start.lnum;
			    curwin->w_cursor.lnum <= oap->end.lnum;
						      ++curwin->w_cursor.lnum)
		    {
			getvcol(curwin, &curwin->w_cursor, NULL, NULL, &end);
			if (end > oap->end_vcol)
			    oap->end_vcol = end;
		    }
		}
		else if (redo_VIsual_busy)
		    oap->end_vcol = oap->start_vcol + redo_VIsual_col - 1;
		/*
		 * Correct oap->end.col and oap->start.col to be the
		 * upper-left and lower-right corner of the block area.
		 */
		curwin->w_cursor.lnum = oap->end.lnum;
		coladvance(oap->end_vcol);
		oap->end = curwin->w_cursor;
		curwin->w_cursor = oap->start;
		coladvance(oap->start_vcol);
		oap->start = curwin->w_cursor;
	    }

	    if (!redo_VIsual_busy && !gui_yank)
	    {
		/*
		 * Prepare to reselect and redo Visual: this is based on the
		 * size of the Visual text
		 */
		resel_VIsual_mode = VIsual_mode;
		if (curwin->w_curswant == MAXCOL)
		    resel_VIsual_col = MAXCOL;
		else if (VIsual_mode == Ctrl('V'))
		    resel_VIsual_col = oap->end_vcol - oap->start_vcol + 1;
		else if (oap->line_count > 1)
		    resel_VIsual_col = oap->end.col;
		else
		    resel_VIsual_col = oap->end.col - oap->start.col + 1;
		resel_VIsual_line_count = oap->line_count;
	    }

	    /* can't redo yank (unless 'y' is in 'cpoptions') and ":" */
	    if ((vim_strchr(p_cpo, CPO_YANK) != NULL || oap->op_type != OP_YANK)
		    && oap->op_type != OP_COLON)
	    {
		prep_redo(oap->regname, 0L, NUL, 'v', get_op_char(oap->op_type),
					     get_extra_op_char(oap->op_type));
		redo_VIsual_mode = resel_VIsual_mode;
		redo_VIsual_col = resel_VIsual_col;
		redo_VIsual_line_count = resel_VIsual_line_count;
		redo_VIsual_count = cap->count0;
	    }

	    /*
	     * oap->inclusive defaults to TRUE.
	     * If oap->end is on a NUL (empty line) oap->inclusive becomes
	     * FALSE.  This makes "d}P" and "v}dP" work the same.
	     */
	    oap->inclusive = TRUE;
	    if (VIsual_mode == 'V')
		oap->motion_type = MLINE;
	    else
	    {
		oap->motion_type = MCHAR;
		if (VIsual_mode != Ctrl('V') && *ml_get_pos(&(oap->end)) == NUL)
		{
		    oap->inclusive = FALSE;
		    /* Try to include the newline, unless it's an operator
		     * that works on lines only */
		    if (*p_sel != 'o'
			    && !op_on_lines(oap->op_type)
			    && oap->end.lnum < curbuf->b_ml.ml_line_count)
		    {
			++oap->end.lnum;
			oap->end.col = 0;
			++oap->line_count;
		    }
		}
	    }

	    redo_VIsual_busy = FALSE;
	    /*
	     * Switch Visual off now, so screen updating does
	     * not show inverted text when the screen is redrawn.
	     * With OP_YANK and sometimes with OP_COLON and OP_FILTER there is
	     * no screen redraw, so it is done here to remove the inverted
	     * part.
	     */
	    if (!gui_yank)
	    {
		VIsual_active = FALSE;
#ifdef USE_MOUSE
		setmouse();
		mouse_dragging = 0;
#endif
		if (p_smd)
		    clear_cmdline = TRUE;   /* unshow visual mode later */
		if (oap->op_type == OP_YANK || oap->op_type == OP_COLON ||
						     oap->op_type == OP_FILTER)
		    update_curbuf(NOT_VALID);
	    }
	}

	curwin->w_set_curswant = TRUE;

	/*
	 * oap->empty is set when start and end are the same.  The inclusive
	 * flag affects this too, unless yanking and the end is on a NUL.
	 */
	oap->empty = (oap->motion_type == MCHAR
		    && (!oap->inclusive
			|| (oap->op_type == OP_YANK && gchar_pos(&oap->end) == NUL))
		    && equal(oap->start, oap->end));
	/*
	 * For delete, change and yank, it's an error to operate on an
	 * empty region, when 'E' inclucded in 'cpoptions' (Vi compatible).
	 */
	empty_region_error = (oap->empty
				&& vim_strchr(p_cpo, CPO_EMPTYREGION) != NULL);

	/* Force a redraw when operating on an empty Visual region */
	if (oap->is_VIsual && oap->empty)
	    redraw_curbuf_later(NOT_VALID);

	/*
	 * If the end of an operator is in column one while oap->motion_type
	 * is MCHAR and oap->inclusive is FALSE, we put op_end after the last
	 * character in the previous line. If op_start is on or before the
	 * first non-blank in the line, the operator becomes linewise
	 * (strange, but that's the way vi does it).
	 */
	if (	   oap->motion_type == MCHAR
		&& oap->inclusive == FALSE
		&& !dont_adjust_op_end
		&& oap->end.col == 0
		&& (!oap->is_VIsual || *p_sel == 'o')
		&& oap->line_count > 1)
	{
	    oap->end_adjusted = TRUE;	    /* remember that we did this */
	    --oap->line_count;
	    --oap->end.lnum;
	    if (inindent(0))
		oap->motion_type = MLINE;
	    else
	    {
		oap->end.col = STRLEN(ml_get(oap->end.lnum));
		if (oap->end.col)
		{
		    --oap->end.col;
		    oap->inclusive = TRUE;
		}
	    }
	}
	else
	    oap->end_adjusted = FALSE;

	switch (oap->op_type)
	{
	case OP_LSHIFT:
	case OP_RSHIFT:
	    op_shift(oap, TRUE, oap->is_VIsual ? (int)cap->count1 : 1);
	    break;

	case OP_JOIN_NS:
	case OP_JOIN:
	    if (oap->line_count < 2)
		oap->line_count = 2;
	    if (curwin->w_cursor.lnum + oap->line_count - 1 >
						   curbuf->b_ml.ml_line_count)
		beep_flush();
	    else
		do_do_join(oap->line_count, oap->op_type == OP_JOIN, TRUE);
	    break;

	case OP_DELETE:
	    VIsual_reselect = FALSE;	    /* don't reselect now */
	    if (empty_region_error)
		vim_beep();
	    else
		(void)op_delete(oap);
	    break;

	case OP_YANK:
	    if (empty_region_error)
	    {
		if (!gui_yank)
		    vim_beep();
	    }
	    else
		(void)op_yank(oap, FALSE, !gui_yank);
	    check_cursor_col();
	    break;

	case OP_CHANGE:
	    VIsual_reselect = FALSE;	    /* don't reselect now */
	    if (empty_region_error)
		vim_beep();
	    else
	    {
		/* This is a new edit command, not a restart.  We don't edit
		 * recursively. */
		restart_edit = 0;
		*command_busy = op_change(oap);	/* will call edit() */
	    }
	    break;

	case OP_FILTER:
	    if (vim_strchr(p_cpo, CPO_FILTER) != NULL)
		AppendToRedobuff((char_u *)"!\r");  /* use any last used !cmd */
	    else
		bangredo = TRUE;    /* do_bang() will put cmd in redo buffer */

	case OP_INDENT:
	case OP_COLON:

#if defined(LISPINDENT) || defined(CINDENT)
	    /*
	     * If 'equalprg' is empty, do the indenting internally.
	     */
	    if (oap->op_type == OP_INDENT && *p_ep == NUL)
	    {
# ifdef LISPINDENT
		if (curbuf->b_p_lisp)
		{
		    op_reindent(oap, get_lisp_indent);
		    break;
		}
# endif
# ifdef CINDENT
		op_reindent(oap, get_c_indent);
		break;
# endif
	    }
#endif /* defined(LISPINDENT) || defined(CINDENT) */

	    op_colon(oap);
	    break;

	case OP_TILDE:
	case OP_UPPER:
	case OP_LOWER:
	case OP_ROT13:
	    if (empty_region_error)
		vim_beep();
	    else
		op_tilde(oap);
	    check_cursor_col();
	    break;

	case OP_FORMAT:
	    if (*p_fp != NUL)
		op_colon(oap);		/* use external command */
	    else
		op_format(oap);		/* use internal function */
	    break;

	case OP_INSERT:
	case OP_APPEND:
	    VIsual_reselect = FALSE;	/* don't reselect now */
#ifdef VISUALEXTRA
	    if (empty_region_error)
#endif
		vim_beep();
#ifdef VISUALEXTRA
	    else
	    {
		/* This is a new edit command, not a restart.  We don't edit
		 * recursively. */
		restart_edit = 0;
		op_insert(oap, cap->count1);/* handles insert & append
					     * will call edit() */
	    }
#endif
	    break;

	case OP_REPLACE:
	    VIsual_reselect = FALSE;	/* don't reselect now */
#ifdef VISUALEXTRA
	    if (empty_region_error)
#endif
		vim_beep();
#ifdef VISUALEXTRA
	    else
		op_replace(oap, cap->nchar);
#endif
	    break;

	default:
	    clearopbeep(oap);
	}
	if (!gui_yank)
	{
	    /*
	     * if 'sol' not set, go back to old column for some commands
	     */
	    if (!p_sol && oap->motion_type == MLINE && !oap->end_adjusted
		    && (oap->op_type == OP_LSHIFT || oap->op_type == OP_RSHIFT
						|| oap->op_type == OP_DELETE))
		coladvance(curwin->w_curswant = old_col);
	    oap->op_type = OP_NOP;
	}
	else
	    curwin->w_cursor = old_cursor;
	oap->block_mode = FALSE;
	oap->regname = 0;
    }
}

/*
 * Handle indent and format operators and visual mode ":".
 */
    static void
op_colon(oap)
    OPARG	*oap;
{
    stuffcharReadbuff(':');
    if (oap->is_VIsual)
	stuffReadbuff((char_u *)"'<,'>");
    else
    {
	/*
	 * Make the range look nice, so it can be repeated.
	 */
	if (oap->start.lnum == curwin->w_cursor.lnum)
	    stuffcharReadbuff('.');
	else
	    stuffnumReadbuff((long)oap->start.lnum);
	if (oap->end.lnum != oap->start.lnum)
	{
	    stuffcharReadbuff(',');
	    if (oap->end.lnum == curwin->w_cursor.lnum)
		stuffcharReadbuff('.');
	    else if (oap->end.lnum == curbuf->b_ml.ml_line_count)
		stuffcharReadbuff('$');
	    else if (oap->start.lnum == curwin->w_cursor.lnum)
	    {
		stuffReadbuff((char_u *)".+");
		stuffnumReadbuff((long)oap->line_count - 1);
	    }
	    else
		stuffnumReadbuff((long)oap->end.lnum);
	}
    }
    if (oap->op_type != OP_COLON)
	stuffReadbuff((char_u *)"!");
    if (oap->op_type == OP_INDENT)
    {
#ifndef CINDENT
	if (*p_ep == NUL)
	    stuffReadbuff((char_u *)"indent");
	else
#endif
	    stuffReadbuff(p_ep);
	stuffReadbuff((char_u *)"\n");
    }
    else if (oap->op_type == OP_FORMAT)
    {
	if (*p_fp == NUL)
	    stuffReadbuff((char_u *)"fmt");
	else
	    stuffReadbuff(p_fp);
	stuffReadbuff((char_u *)"\n");
    }

    /*
     * do_cmdline() does the rest
     */
}

#if defined(USE_MOUSE) || defined(PROTO)
/*
 * Do the appropriate action for the current mouse click in the current mode.
 *
 * Normal Mode:
 * event	 modi-	position      visual	   change   action
 *		 fier	cursor			   window
 * left press	  -	yes	    end		    yes
 * left press	  C	yes	    end		    yes	    "^]" (2)
 * left press	  S	yes	    end		    yes	    "*" (2)
 * left drag	  -	yes	start if moved	    no
 * left relse	  -	yes	start if moved	    no
 * middle press	  -	yes	 if not active	    no	    put register
 * middle press	  -	yes	 if active	    no	    yank and put
 * right press	  -	yes	start or extend	    yes
 * right press	  S	yes	no change	    yes	    "#" (2)
 * right drag	  -	yes	extend		    no
 * right relse	  -	yes	extend		    no
 *
 * Insert or Replace Mode:
 * event	 modi-	position      visual	   change   action
 *		 fier	cursor			   window
 * left press	  -	yes	(cannot be active)  yes
 * left press	  C	yes	(cannot be active)  yes	    "CTRL-O^]" (2)
 * left press	  S	yes	(cannot be active)  yes	    "CTRL-O*" (2)
 * left drag	  -	yes	start or extend (1) no	    CTRL-O (1)
 * left relse	  -	yes	start or extend (1) no	    CTRL-O (1)
 * middle press	  -	no	(cannot be active)  no	    put register
 * right press	  -	yes	start or extend	    yes	    CTRL-O
 * right press	  S	yes	(cannot be active)  yes	    "CTRL-O#" (2)
 *
 * (1) only if mouse pointer moved since press
 * (2) only if click is in same buffer
 *
 * Return TRUE if start_arrow() should be called for edit mode.
 */
    int
do_mouse(oap, c, dir, count, fix_indent)
    OPARG	*oap;		/* operator argument, can be NULL */
    int		c;		/* K_LEFTMOUSE, etc */
    int		dir;		/* Direction to 'put' if necessary */
    long	count;
    int		fix_indent;	/* PUT_FIXINDENT if fixing indent necessary */
{
    static FPOS	orig_cursor;
    static int	do_always = FALSE;	/* ignore 'mouse' setting next time */
    static int	got_click = FALSE;	/* got a click some time back */

    int		which_button;	/* MOUSE_LEFT, _MIDDLE or _RIGHT */
    int		is_click;	/* If FALSE it's a drag or release event */
    int		is_drag;	/* If TRUE it's a drag event */
    int		jump_flags = 0;	/* flags for jump_to_mouse() */
    FPOS	start_visual;
    FPOS	end_visual;
    colnr_t	leftcol, rightcol;
    int		diff;
    int		moved;		/* Has cursor moved? */
    int		in_status_line;	/* mouse in status line */
    int		c1, c2;
    int		VIsual_was_active = VIsual_active;
    int		regname;

    /*
     * When GUI is active, always recognize mouse events, otherwise:
     * - Ignore mouse event in normal mode if 'mouse' doesn't include 'n'.
     * - Ignore mouse event in visual mode if 'mouse' doesn't include 'v'.
     * - For command line and insert mode 'mouse' is checked before calling
     *	 do_mouse().
     */
    if (do_always)
	do_always = FALSE;
    else
#ifdef USE_GUI
	if (!gui.in_use)
#endif
	{
	    if (VIsual_active)
	    {
		if (!mouse_has(MOUSE_VISUAL))
		    return FALSE;
	    }
	    else if (State == NORMAL && !mouse_has(MOUSE_NORMAL))
		return FALSE;
	}

    which_button = get_mouse_button(KEY2TERMCAP1(c), &is_click, &is_drag);

    /*
     * Ignore drag and release events if we didn't get a click.
     */
    if (is_click)
	got_click = TRUE;
    else
    {
	if (!got_click)			/* didn't get click, ignore */
	    return FALSE;
	if (!is_drag)			/* release, reset got_click */
	    got_click = FALSE;
    }

    /*
     * ALT is currently ignored
     */
    if ((mod_mask & MOD_MASK_ALT))
	return FALSE;

    /*
     * CTRL right mouse button does CTRL-T
     */
    if (is_click && (mod_mask & MOD_MASK_CTRL) && which_button == MOUSE_RIGHT)
    {
	if (State & INSERT)
	    stuffcharReadbuff(Ctrl('O'));
	stuffcharReadbuff(Ctrl('T'));
	got_click = FALSE;		/* ignore drag&release now */
	return FALSE;
    }

    /*
     * CTRL only works with left mouse button
     */
    if ((mod_mask & MOD_MASK_CTRL) && which_button != MOUSE_LEFT)
	return FALSE;

    /*
     * When a modifier is down, ignore drag and release events, as well as
     * multiple clicks and the middle mouse button.
     * Accept shift-leftmouse drags when 'mousemodel' is "popup.*".
     */
    if ((mod_mask & (MOD_MASK_SHIFT | MOD_MASK_CTRL | MOD_MASK_ALT))
	    && (!is_click
		|| (mod_mask & MOD_MASK_MULTI_CLICK)
		|| which_button == MOUSE_MIDDLE)
	    && !((mod_mask & MOD_MASK_SHIFT)
		&& mouse_model_popup()
		&& which_button == MOUSE_LEFT)
	    )
	return FALSE;

    /*
     * If the button press was used as the movement command for an operator
     * (eg "d<MOUSE>"), or it is the middle button that is held down, ignore
     * drag/release events.
     */
    if (!is_click && which_button == MOUSE_MIDDLE)
	return FALSE;

    if (oap != NULL)
	regname = oap->regname;
    else
	regname = 0;

    /*
     * Middle mouse button does a 'put' of the selected text
     */
    if (which_button == MOUSE_MIDDLE)
    {
	if (State == NORMAL)
	{
	    /*
	     * If an operator was pending, we don't know what the user wanted
	     * to do. Go back to normal mode: Clear the operator and beep().
	     */
	    if (oap != NULL && oap->op_type != OP_NOP)
	    {
		clearopbeep(oap);
		return FALSE;
	    }

	    /*
	     * If visual was active, yank the highlighted text and put it
	     * before the mouse pointer position.
	     */
	    if (VIsual_active)
	    {
		stuffcharReadbuff('y');
		stuffcharReadbuff(K_MIDDLEMOUSE);
		do_always = TRUE;	/* ignore 'mouse' setting next time */
		return FALSE;
	    }
	    /*
	     * The rest is below jump_to_mouse()
	     */
	}

	/*
	 * Middle click in insert mode doesn't move the mouse, just insert the
	 * contents of a register.  '.' register is special, can't insert that
	 * with do_put().
	 */
	else if (State & INSERT)
	{
	    if (regname == '.')
		insert_reg(regname, TRUE);
	    else
	    {
#ifdef USE_CLIPBOARD
		if (clipboard.available && regname == 0)
		    regname = '*';
#endif
		if ((State == REPLACE || State == VREPLACE)
					    && !yank_register_mline(regname))
		    insert_reg(regname, TRUE);
		else
		{
		    do_put(regname, BACKWARD, 1L, fix_indent | PUT_CURSEND);

		    /* Repeat it with CTRL-R CTRL-O r or CTRL-R CTRL-P r */
		    AppendCharToRedobuff(Ctrl('R'));
		    AppendCharToRedobuff(fix_indent ? Ctrl('P') : Ctrl('O'));
		    AppendCharToRedobuff(regname == 0 ? '"' : regname);
		}
	    }
	    return FALSE;
	}
	else
	    return FALSE;
    }

    if (!is_click)
	jump_flags |= MOUSE_FOCUS | MOUSE_DID_MOVE;

    start_visual.lnum = 0;

    /*
     * When 'mousemodel' is "popup" or "popup_setpos", translate mouse events:
     * right button up   -> pop-up menu
     * shift-left button -> right button
     */
    if (mouse_model_popup())
    {
	if (which_button == MOUSE_RIGHT
			    && !(mod_mask & (MOD_MASK_SHIFT | MOD_MASK_CTRL)))
	{
	    /*
	     * NOTE: Ignore right button down and drag mouse events.
	     * Windows only shows the popup menu on the button up event.
	     */
#if defined(USE_GUI_MOTIF) || defined(USE_GUI_GTK)
	    if (!is_click)
		return FALSE;
#endif
#if defined(USE_GUI_ATHENA) || defined(USE_GUI_MSWIN)
	    if (is_click || is_drag)
		return FALSE;
#endif
#if defined(USE_GUI_MOTIF) || defined(USE_GUI_GTK) \
	    || defined(USE_GUI_ATHENA) || defined(USE_GUI_MSWIN) \
	    || defined(USE_GUI_MAC)
	    if (gui.in_use)
	    {
		jump_flags = 0;
		if (STRCMP(p_mousem, "popup_setpos") == 0)
		{
		    if (VIsual_active)
		    {
			FPOS    m_pos;

			/*
			 * set MOUSE_MAY_STOP_VIS if we are outside the
			 * selection or the current window (might have false
			 * negative here)
			 */
			if (mouse_row < curwin->w_winpos
			     || mouse_row
				      > (curwin->w_winpos + curwin->w_height))
			    jump_flags = MOUSE_MAY_STOP_VIS;
			else if (get_fpos_of_mouse(&m_pos, NULL) != IN_BUFFER)
			    jump_flags = MOUSE_MAY_STOP_VIS;
			else
			{
			    if ((lt(curwin->w_cursor, VIsual)
					&& (lt(m_pos, curwin->w_cursor)
					    || lt(VIsual, m_pos)))
				    || (lt(VIsual, curwin->w_cursor)
					&& (lt(m_pos, VIsual)
					    || lt(curwin->w_cursor, m_pos))))
			    {
				jump_flags = MOUSE_MAY_STOP_VIS;
			    }
			    else if (VIsual_mode == Ctrl('V'))
			    {
				getvcols(&curwin->w_cursor, &VIsual,
							 &leftcol, &rightcol);
				getvcol(curwin, &m_pos, NULL, &m_pos.col, NULL);
				if (m_pos.col < leftcol || m_pos.col > rightcol)
				    jump_flags = MOUSE_MAY_STOP_VIS;
			    }
			}
		    }
		    else
			jump_flags = MOUSE_MAY_STOP_VIS;
		}
		if (jump_flags)
		{
		    jump_flags = jump_to_mouse(jump_flags, NULL);
		    update_curbuf(NOT_VALID);
		    setcursor();
		    out_flush();    /* Update before showing popup menu */
		}
# ifdef WANT_MENU
		gui_show_popupmenu();
# endif
		return (jump_flags & CURSOR_MOVED) != 0;
	    }
	    else
		return FALSE;
#else
	    return FALSE;
#endif
	}
	if (which_button == MOUSE_LEFT && (mod_mask & MOD_MASK_SHIFT))
	{
	    which_button = MOUSE_RIGHT;
	    mod_mask &= ~ MOD_MASK_SHIFT;
	}
    }

    if ((State & (NORMAL | INSERT))
			    && !(mod_mask & (MOD_MASK_SHIFT | MOD_MASK_CTRL)))
    {
	if (which_button == MOUSE_LEFT)
	{
	    if (is_click)
	    {
		/* stop Visual mode for a left click in a window, but not when
		 * on a status line */
		if (VIsual_active)
		    jump_flags |= MOUSE_MAY_STOP_VIS;
	    }
	    else
		jump_flags |= MOUSE_MAY_VIS;
	}
	else if (which_button == MOUSE_RIGHT)
	{
	    if (is_click && VIsual_active)
	    {
		/*
		 * Remember the start and end of visual before moving the
		 * cursor.
		 */
		if (lt(curwin->w_cursor, VIsual))
		{
		    start_visual = curwin->w_cursor;
		    end_visual = VIsual;
		}
		else
		{
		    start_visual = VIsual;
		    end_visual = curwin->w_cursor;
		}
	    }
	    jump_flags |= MOUSE_MAY_VIS;
	}
    }

    /*
     * If an operator is pending, ignore all drags and releases until the
     * next mouse click.
     */
    if (!is_drag && oap != NULL && oap->op_type != OP_NOP)
    {
	got_click = FALSE;
	oap->motion_type = MCHAR;
    }

    /*
     * Jump!
     */
    jump_flags = jump_to_mouse(jump_flags,
				      oap == NULL ? NULL : &(oap->inclusive));
    moved = (jump_flags & CURSOR_MOVED);
    in_status_line = ((jump_flags & IN_STATUS_LINE) == IN_STATUS_LINE);

    /* Set global flag that we are extending the Visual area with mouse
     * dragging; temporarily mimimize 'scrolloff'. */
    if (moved && VIsual_active && is_drag && p_so)
    {
	/* In the very first line, allow scrolling one line */
	if (mouse_row == 0)
	    mouse_dragging = 2;
	else
	    mouse_dragging = 1;
    }

    if (start_visual.lnum)		/* right click in visual mode */
    {
	/*
	 * In Visual-block mode, divide the area in four, pick up the corner
	 * that is in the quarter that the cursor is in.
	 */
	if (VIsual_mode == Ctrl('V'))
	{
	    getvcols(&start_visual, &end_visual, &leftcol, &rightcol);
	    if (curwin->w_curswant > (leftcol + rightcol) / 2)
		end_visual.col = leftcol;
	    else
		end_visual.col = rightcol;
	    if (curwin->w_cursor.lnum <
				    (start_visual.lnum + end_visual.lnum) / 2)
		end_visual.lnum = end_visual.lnum;
	    else
		end_visual.lnum = start_visual.lnum;

	    /* move VIsual to the right column */
	    start_visual = curwin->w_cursor;	    /* save the cursor pos */
	    curwin->w_cursor = end_visual;
	    coladvance(end_visual.col);
	    VIsual = curwin->w_cursor;
	    curwin->w_cursor = start_visual;	    /* restore the cursor */
	}
	else
	{
	    /*
	     * If the click is before the start of visual, change the start.
	     * If the click is after the end of visual, change the end.  If
	     * the click is inside the visual, change the closest side.
	     */
	    if (lt(curwin->w_cursor, start_visual))
		VIsual = end_visual;
	    else if (lt(end_visual, curwin->w_cursor))
		VIsual = start_visual;
	    else
	    {
		/* In the same line, compare column number */
		if (end_visual.lnum == start_visual.lnum)
		{
		    if (curwin->w_cursor.col - start_visual.col >
				    end_visual.col - curwin->w_cursor.col)
			VIsual = start_visual;
		    else
			VIsual = end_visual;
		}

		/* In different lines, compare line number */
		else
		{
		    diff = (curwin->w_cursor.lnum - start_visual.lnum) -
				(end_visual.lnum - curwin->w_cursor.lnum);

		    if (diff > 0)		/* closest to end */
			VIsual = start_visual;
		    else if (diff < 0)	/* closest to start */
			VIsual = end_visual;
		    else			/* in the middle line */
		    {
			if (curwin->w_cursor.col <
					(start_visual.col + end_visual.col) / 2)
			    VIsual = end_visual;
			else
			    VIsual = start_visual;
		    }
		}
	    }
	}
    }
    /*
     * If Visual mode started in insert mode, execute "CTRL-O"
     */
    else if ((State & INSERT) && VIsual_active)
	stuffcharReadbuff(Ctrl('O'));

    /*
     * When the cursor has moved in insert mode, and something was inserted,
     * and there are several windows, need to redraw.
     */
    if (moved && (State & INSERT) && modified && firstwin->w_next != NULL)
    {
	update_curbuf(NOT_VALID);
	modified = FALSE;
    }

    /*
     * Middle mouse click: Put text before cursor.
     */
    if (which_button == MOUSE_MIDDLE)
    {
#ifdef USE_CLIPBOARD
	if (clipboard.available && regname == 0)
	    regname = '*';
#endif
	if (yank_register_mline(regname))
	{
	    if (mouse_past_bottom)
		dir = FORWARD;
	}
	else if (mouse_past_eol)
	    dir = FORWARD;

	if (fix_indent)
	{
	    c1 = (dir == BACKWARD) ? '[' : ']';
	    c2 = 'p';
	}
	else
	{
	    c1 = (dir == FORWARD) ? 'p' : 'P';
	    c2 = NUL;
	}
	prep_redo(regname, count, NUL, c1, c2, NUL);

	/*
	 * Remember where the paste started, so in edit() Insstart can be set
	 * to this position
	 */
	if (restart_edit)
	    where_paste_started = curwin->w_cursor;
	do_put(regname, dir, count, fix_indent | PUT_CURSEND);
    }

    /*
     * Ctrl-Mouse click (or double click in a help window) jumps to the tag
     * under the mouse pointer.
     */
    else if ((mod_mask & MOD_MASK_CTRL)
			  || (curbuf->b_help && (mod_mask & MOD_MASK_2CLICK)))
    {
	if (State & INSERT)
	    stuffcharReadbuff(Ctrl('O'));
	stuffcharReadbuff(Ctrl(']'));
	got_click = FALSE;		/* ignore drag&release now */
    }

    /*
     * Shift-Mouse click searches for the next occurrence of the word under
     * the mouse pointer
     */
    else if ((mod_mask & MOD_MASK_SHIFT))
    {
	if (State & INSERT || (VIsual_active && VIsual_select))
	    stuffcharReadbuff(Ctrl('O'));
	if (which_button == MOUSE_LEFT)
	    stuffcharReadbuff('*');
	else	/* MOUSE_RIGHT */
	    stuffcharReadbuff('#');
    }

    /* Handle double clicks, unless on status line */
    else if (in_status_line)
	;
    else if ((mod_mask & MOD_MASK_MULTI_CLICK) && (State & (NORMAL | INSERT)))
    {
	if (is_click || !VIsual_active)
	{
	    if (VIsual_active)
		orig_cursor = VIsual;
	    else
	    {
		check_visual_highlight();
		VIsual = curwin->w_cursor;
		orig_cursor = VIsual;
		VIsual_active = TRUE;
		VIsual_reselect = TRUE;
		/* start Select mode if 'selectmode' contains "mouse" */
		may_start_select('o');
		setmouse();
		if (p_smd)
		    redraw_cmdline = TRUE;  /* show visual mode later */
	    }
	    if (mod_mask & MOD_MASK_2CLICK)
		VIsual_mode = 'v';
	    else if (mod_mask & MOD_MASK_3CLICK)
		VIsual_mode = 'V';
	    else if (mod_mask & MOD_MASK_4CLICK)
		VIsual_mode = Ctrl('V');
#ifdef USE_CLIPBOARD
	    /* Make sure the clipboard gets updated.  Needed because start and
	     * end may still be the same, and the selection needs to be owned */
	    clipboard.vmode = NUL;
#endif
	}
	if (mod_mask & MOD_MASK_2CLICK)
	{
	    if (lt(curwin->w_cursor, orig_cursor))
	    {
		find_start_of_word(&curwin->w_cursor);
		find_end_of_word(&VIsual);
	    }
	    else
	    {
		find_start_of_word(&VIsual);
		find_end_of_word(&curwin->w_cursor);
	    }
	    curwin->w_set_curswant = TRUE;
	}
	if (is_click)
	    update_curbuf(NOT_VALID);	    /* update the inversion */
    }
    else if (VIsual_active && VIsual_was_active != VIsual_active)
	VIsual_mode = 'v';

    return moved;
}

    static void
find_start_of_word(pos)
    FPOS    *pos;
{
    char_u  *ptr;
    int	    cclass;

    ptr = ml_get(pos->lnum);
    cclass = get_mouse_class(ptr[pos->col]);

    /* Can't test pos->col >= 0 because pos->col is unsigned */
    while (pos->col > 0 && get_mouse_class(ptr[pos->col]) == cclass)
	pos->col--;
    if (pos->col != 0 || get_mouse_class(ptr[0]) != cclass)
	pos->col++;
}

    static void
find_end_of_word(pos)
    FPOS    *pos;
{
    char_u  *ptr;
    int	    cclass;

    ptr = ml_get(pos->lnum);
    if (*p_sel == 'e' && pos->col)
	pos->col--;
    cclass = get_mouse_class(ptr[pos->col]);
    while (ptr[pos->col] && get_mouse_class(ptr[pos->col]) == cclass)
	pos->col++;
    if (*p_sel != 'e' && pos->col)
	pos->col--;
}

    static int
get_mouse_class(c)
    int	    c;
{
    if (c == ' ' || c == '\t')
	return ' ';

    if (vim_isIDc(c))
	return 'a';

    /*
     * There are a few special cases where we want certain combinations of
     * characters to be considered as a single word.  These are things like
     * "->", "/ *", "*=", "+=", "&=", "<=", ">=", "!=" etc.  Otherwise, each
     * character is in it's own class.
     */
    if (c != NUL && vim_strchr((char_u *)"-+*/%<>&|^!=", c) != NULL)
	return '=';
    return c;
}
#endif /* USE_MOUSE */

/*
 * Check if  highlighting for visual mode is possible, give a warning message
 * if not.
 */
    void
check_visual_highlight()
{
    static int	    did_check = FALSE;

    if (!did_check && hl_attr(HLF_V) == 0)
	MSG("Warning: terminal cannot highlight");
    did_check = TRUE;
}

/*
 * End visual mode.
 * This function should ALWAYS be called to end visual mode, except from
 * do_pending_operator().
 */
    void
end_visual_mode()
{
#ifdef USE_CLIPBOARD
    /*
     * If we are using the clipboard, then remember what was selected in case
     * we need to paste it somewhere while we still own the selection.
     * Only do this when the clipboard is already owned.  Don't want to grab
     * the selection when hitting ESC.
     */
    if (clipboard.available && clipboard.owned)
	clip_auto_select();
#endif

    VIsual_active = FALSE;
#ifdef USE_MOUSE
    setmouse();
    mouse_dragging = 0;
#endif

    /* Save the current VIsual area for '< and '> marks, and "gv" */
    curbuf->b_visual_start = VIsual;
    curbuf->b_visual_end = curwin->w_cursor;
    curbuf->b_visual_mode = VIsual_mode;

    if (p_smd)
	clear_cmdline = TRUE;		/* unshow visual mode later */

    /* Don't leave the cursor past the end of the line */
    if (curwin->w_cursor.col > 0 && *ml_get_cursor() == NUL)
	--curwin->w_cursor.col;
}

/*
 * Find the identifier under or to the right of the cursor.  If none is
 * found and find_type has FIND_STRING, then find any non-white string.  The
 * length of the string is returned, or zero if no string is found.  If a
 * string is found, a pointer to the string is put in *string, but note that
 * the caller must use the length returned as this string may not be NUL
 * terminated.
 */
    int
find_ident_under_cursor(string, find_type)
    char_u  **string;
    int	    find_type;
{
    char_u  *ptr;
    int	    col = 0;	    /* init to shut up GCC */
    int	    i;

    /*
     * if i == 0: try to find an identifier
     * if i == 1: try to find any string
     */
    ptr = ml_get_curline();
    for (i = (find_type & FIND_IDENT) ? 0 : 1;	i < 2; ++i)
    {
	/*
	 * skip to start of identifier/string
	 */
	col = curwin->w_cursor.col;
	while (ptr[col] != NUL &&
		    (i == 0 ? !vim_iswordc(ptr[col]) : vim_iswhite(ptr[col])))
	    ++col;

	/*
	 * Back up to start of identifier/string. This doesn't match the
	 * real vi but I like it a little better and it shouldn't bother
	 * anyone.
	 * When FIND_IDENT isn't defined, we backup until a blank.
	 */
	while (col > 0 && (i == 0 ? vim_iswordc(ptr[col - 1]) :
		    (!vim_iswhite(ptr[col - 1]) &&
		   (!(find_type & FIND_IDENT) || !vim_iswordc(ptr[col - 1])))))
	    --col;

	/*
	 * if we don't want just any old string, or we've found an identifier,
	 * stop searching.
	 */
	if (!(find_type & FIND_STRING) || vim_iswordc(ptr[col]))
	    break;
    }
    /*
     * didn't find an identifier or string
     */
    if (ptr[col] == NUL || (!vim_iswordc(ptr[col]) && i == 0))
    {
	if (find_type & FIND_STRING)
	    EMSG("No string under cursor");
	else
	    EMSG("No identifier under cursor");
	return 0;
    }
    ptr += col;
    *string = ptr;
    col = 0;
    while (i == 0 ? vim_iswordc(*ptr) : (*ptr != NUL && !vim_iswhite(*ptr)))
    {
	++ptr;
	++col;
    }
    return col;
}

/*
 * Prepare for redo of a normal command.
 */
    static void
prep_redo_cmd(cap)
    CMDARG  *cap;
{
    prep_redo(cap->oap->regname, cap->count0,
					  NUL, cap->cmdchar, NUL, cap->nchar);
}

/*
 * Prepare for redo of any command.
 */
    static void
prep_redo(regname, num, cmd1, cmd2, cmd3, cmd4)
    int	    regname;
    long    num;
    int	    cmd1;
    int	    cmd2;
    int	    cmd3;
    int	    cmd4;
{
    ResetRedobuff();
    if (regname != 0)	/* yank from specified buffer */
    {
	AppendCharToRedobuff('"');
	AppendCharToRedobuff(regname);
    }
    if (num)
	AppendNumberToRedobuff(num);

    if (cmd1 != NUL)
	AppendCharToRedobuff(cmd1);
    if (cmd2 != NUL)
	AppendCharToRedobuff(cmd2);
    if (cmd3 != NUL)
	AppendCharToRedobuff(cmd3);
    if (cmd4 != NUL)
	AppendCharToRedobuff(cmd4);
}

/*
 * check for operator active and clear it
 *
 * return TRUE if operator was active
 */
    static int
checkclearop(oap)
    OPARG	*oap;
{
    if (oap->op_type == OP_NOP)
	return FALSE;
    clearopbeep(oap);
    return TRUE;
}

/*
 * check for operator or Visual active and clear it
 *
 * return TRUE if operator was active
 */
    static int
checkclearopq(oap)
    OPARG	*oap;
{
    if (oap->op_type == OP_NOP && !VIsual_active)
	return FALSE;
    clearopbeep(oap);
    return TRUE;
}

    static void
clearop(oap)
    OPARG	*oap;
{
    oap->op_type = OP_NOP;
    oap->regname = 0;
}

    static void
clearopbeep(oap)
    OPARG	*oap;
{
    clearop(oap);
    beep_flush();
}

#ifdef CMDLINE_INFO
/*
 * Routines for displaying a partly typed command
 */

static char_u	showcmd_buf[SHOWCMD_COLS + 1];
static char_u	old_showcmd_buf[SHOWCMD_COLS + 1];  /* For push_showcmd() */
static int	showcmd_is_clear = TRUE;

static void display_showcmd __ARGS((void));

    void
clear_showcmd()
{
    if (!p_sc)
	return;

    showcmd_buf[0] = NUL;

    /*
     * Don't actually display something if there is nothing to clear.
     */
    if (showcmd_is_clear)
	return;

    display_showcmd();
}

/*
 * Add 'c' to string of shown command chars.
 * Return TRUE if output has been written (and setcursor() has been called).
 */
    int
add_to_showcmd(c)
    int	    c;
{
    char_u  *p;
    int	    old_len;
    int	    extra_len;
    int	    overflow;
#if defined(USE_MOUSE)
    int	    i;
    static int	    ignore[] =
	       {
#ifdef USE_GUI
		K_SCROLLBAR, K_HORIZ_SCROLLBAR,
		K_LEFTMOUSE_NM, K_LEFTRELEASE_NM,
#endif
		K_IGNORE,
		K_LEFTMOUSE, K_LEFTDRAG, K_LEFTRELEASE,
		K_MIDDLEMOUSE, K_MIDDLEDRAG, K_MIDDLERELEASE,
		K_RIGHTMOUSE, K_RIGHTDRAG, K_RIGHTRELEASE,
		K_MOUSEDOWN, K_MOUSEUP,
		0};
#endif

    if (!p_sc)
	return FALSE;

#if defined(USE_MOUSE)
    /* Ignore keys that are scrollbar updates and mouse clicks */
    for (i = 0; ignore[i]; ++i)
	if (ignore[i] == c)
	    return FALSE;
#endif

    p = transchar(c);
    old_len = STRLEN(showcmd_buf);
    extra_len = STRLEN(p);
    overflow = old_len + extra_len - SHOWCMD_COLS;
    if (overflow > 0)
	STRCPY(showcmd_buf, showcmd_buf + overflow);
    STRCAT(showcmd_buf, p);

    if (char_avail())
	return FALSE;

    display_showcmd();

    return TRUE;
}

    void
add_to_showcmd_c(c)
    int		c;
{
    if (!add_to_showcmd(c))
	setcursor();
}

/*
 * Delete 'len' characters from the end of the shown command.
 */
    static void
del_from_showcmd(len)
    int	    len;
{
    int	    old_len;

    if (!p_sc)
	return;

    old_len = STRLEN(showcmd_buf);
    if (len > old_len)
	len = old_len;
    showcmd_buf[old_len - len] = NUL;

    if (!char_avail())
	display_showcmd();
}

    void
push_showcmd()
{
    if (p_sc)
	STRCPY(old_showcmd_buf, showcmd_buf);
}

    void
pop_showcmd()
{
    if (!p_sc)
	return;

    STRCPY(showcmd_buf, old_showcmd_buf);

    display_showcmd();
}

    static void
display_showcmd()
{
    int	    len;

    cursor_off();

    len = STRLEN(showcmd_buf);
    if (len == 0)
	showcmd_is_clear = TRUE;
    else
    {
	screen_puts(showcmd_buf, (int)Rows - 1, sc_col, 0);
	showcmd_is_clear = FALSE;
    }

    /*
     * clear the rest of an old message by outputing up to SHOWCMD_COLS spaces
     */
    screen_puts((char_u *)"          " + len, (int)Rows - 1, sc_col + len, 0);

    setcursor();	    /* put cursor back where it belongs */
}
#endif

#ifdef SCROLLBIND
/*
 * When "check" is FALSE, prepare for commands that scroll the window.
 * When "check" is TRUE, take care of scroll-binding after the window has
 * scrolled.  Called from normal_cmd() and edit().
 */
    void
do_check_scrollbind(check)
    int		check;
{
    static WIN		*old_curwin = NULL;
    static linenr_t	old_topline = 0;
    static BUF		*old_buf = NULL;
    static colnr_t	old_leftcol = 0;

    if (check && curwin->w_p_scb)
    {
	if (did_syncbind)
	    did_syncbind = FALSE;
	else if (curwin == old_curwin)
	{
	    /*
	     * synchronize other windows, as necessary according to
	     * 'scrollbind'
	     */
	    if (curwin->w_buffer == old_buf
		    && (curwin->w_topline != old_topline
			|| curwin->w_leftcol != old_leftcol))
	    {
		check_scrollbind(curwin->w_topline - old_topline,
			(long)(curwin->w_leftcol - old_leftcol));
	    }
	}
	else if (vim_strchr(p_sbo, 'j')) /* jump flag set in 'scrollopt' */
	{
	    /*
	     * When switching between windows, make sure that the relative
	     * vertical offset is valid for the new window.  The relative
	     * offset is invalid whenever another 'scrollbind' window has
	     * scrolled to a point that would force the current window to
	     * scroll past the beginning or end of its buffer.  When the
	     * resync is performed, some of the other 'scrollbind' windows may
	     * need to jump so that the current window's relative position is
	     * visible on-screen.
	     */
	    check_scrollbind(curwin->w_topline - curwin->w_scbind_pos, 0L);
	}
	curwin->w_scbind_pos = curwin->w_topline;
    }

    old_curwin = curwin;
    old_topline = curwin->w_topline;
    old_buf = curwin->w_buffer;
    old_leftcol = curwin->w_leftcol;
}

/*
 * Synchronize any windows that have "scrollbind" set, based on the
 * number of rows by which the current window has changed
 * (1998-11-02 16:21:01  R. Edward Ralston <eralston@computer.org>)
 */
    void
check_scrollbind(topline_diff, leftcol_diff)
    linenr_t	topline_diff;
    long	leftcol_diff;
{
    int		want_ver;
    int		want_hor;
    WIN		*old_curwin = curwin;
    BUF		*old_curbuf = curbuf;
    int		update = 0;
    int		old_VIsual_select = VIsual_select;
    int		old_VIsual_active = VIsual_active;
    colnr_t	tgt_leftcol = curwin->w_leftcol;
    long	topline;
    long	y;

    /*
     * check 'scrollopt' string for vertical and horizontal scroll options
     */
    want_ver = (vim_strchr(p_sbo, 'v') && topline_diff);
    want_hor = (vim_strchr(p_sbo, 'h') && (leftcol_diff || topline_diff));

    /*
     * loop through the scrollbound windows and scroll accordingly
     */
    VIsual_select = VIsual_active = 0;
    for (curwin = firstwin; curwin; curwin = curwin->w_next)
    {
	curbuf = curwin->w_buffer;
	if (curwin != old_curwin  /* don't scroll in original window */
		&& curwin->w_p_scb)
	{
	    /*
	     * do the vertical scroll
	     */
	    if (want_ver)
	    {
		curwin->w_scbind_pos += topline_diff;
		topline = curwin->w_scbind_pos;
		if (topline > curbuf->b_ml.ml_line_count - p_so)
		    topline = curbuf->b_ml.ml_line_count - p_so;
		if (topline < 1)
		    topline = 1;

		y = topline - curwin->w_topline;
		if (y > 0)
		    scrollup(y);
		else
		    scrolldown(-y);

		redraw_later(VALID);
		cursor_correct();
		curwin->w_redr_status = TRUE;
	    }

	    /*
	     * do the horizontal scroll
	     */
	    if (want_hor && curwin->w_leftcol != tgt_leftcol)
	    {
		curwin->w_leftcol = tgt_leftcol;
		leftcol_changed();
	    }
	    update = 1;
	}
    }

    /*
     * reset current-window
     */
    VIsual_select = old_VIsual_select;
    VIsual_active = old_VIsual_active;
    curwin = old_curwin;
    curbuf = old_curbuf;

    out_flush();
    if (update)
	update_screen(NOT_VALID);
}
#endif /* #ifdef SCROLLBIND */

/*
 * Implementation of "gd" and "gD" command.
 */
    static void
nv_gd(oap, nchar)
    OPARG   *oap;
    int	    nchar;
{
    int		len;
    char_u	*pat;
    FPOS	old_pos;
    int		t;
    int		save_p_ws;
    int		save_p_scs;
    char_u	*ptr;

    if ((len = find_ident_under_cursor(&ptr, FIND_IDENT)) == 0 ||
					       (pat = alloc(len + 5)) == NULL)
    {
	clearopbeep(oap);
	return;
    }
    sprintf((char *)pat, vim_iswordc(*ptr) ? "\\<%.*s\\>" :
	    "%.*s", len, ptr);
    old_pos = curwin->w_cursor;
    save_p_ws = p_ws;
    save_p_scs = p_scs;
    p_ws = FALSE;	/* don't wrap around end of file now */
    p_scs = FALSE;	/* don't switch ignorecase off now */
#ifdef COMMENTS
    fo_do_comments = TRUE;
#endif

    /*
     * With "gD" go to line 1.
     * With "gd" Search back for the start of the current function, then go
     * back until a blank line.  If this fails go to line 1.
     */
    if (nchar == 'D' || !findpar(oap, BACKWARD, 1L, '{', FALSE))
    {
	setpcmark();			/* Set in findpar() otherwise */
	curwin->w_cursor.lnum = 1;
    }
    else
    {
	while (curwin->w_cursor.lnum > 1 && *skipwhite(ml_get_curline()) != NUL)
	    --curwin->w_cursor.lnum;
    }

    /* Search forward for the identifier, ignore comment lines. */
    while ((t = searchit(curbuf, &curwin->w_cursor, FORWARD, pat, 1L, 0,
							       RE_LAST)) == OK
#ifdef COMMENTS
	    && get_leader_len(ml_get_curline(), NULL, FALSE)
#endif
	    && old_pos.lnum > curwin->w_cursor.lnum)
	++curwin->w_cursor.lnum;
    if (t == FAIL || old_pos.lnum <= curwin->w_cursor.lnum)
    {
	clearopbeep(oap);
	curwin->w_cursor = old_pos;
    }
    else
	curwin->w_set_curswant = TRUE;

    vim_free(pat);
    p_ws = save_p_ws;
    p_scs = save_p_scs;
#ifdef COMMENTS
    fo_do_comments = FALSE;
#endif
}

/*
 * Move 'dist' lines in direction 'dir', counting lines by *screen*
 * lines rather than lines in the file.
 * 'dist' must be positive.
 *
 * Return OK if able to move cursor, FAIL otherwise.
 */
    static int
nv_screengo(oap, dir, dist)
    OPARG   *oap;
    int	    dir;
    long    dist;
{
    int		linelen = linetabsize(ml_get_curline());
    int		retval = OK;
    int		atend = FALSE;
    int		n;

    oap->motion_type = MCHAR;
    oap->inclusive = FALSE;

    /*
     * Instead of sticking at the last character of the line in the file we
     * try to stick in the last column of the screen
     */
    if (curwin->w_curswant == MAXCOL)
    {
	atend = TRUE;
	validate_virtcol();
	curwin->w_curswant = ((curwin->w_virtcol +
		       (curwin->w_p_nu ? 8 : 0)) / Columns + 1) * Columns - 1;
	if (curwin->w_p_nu && curwin->w_curswant > 8)
	    curwin->w_curswant -= 8;
    }
    else
	while (curwin->w_curswant >= (colnr_t)(linelen + Columns))
	    curwin->w_curswant -= Columns;

    while (dist--)
    {
	if (dir == BACKWARD)
	{
						/* move back within line */
	    if ((long)curwin->w_curswant >= Columns)
		curwin->w_curswant -= Columns;
	    else				/* to previous line */
	    {
		if (curwin->w_cursor.lnum == 1)
		{
		    retval = FAIL;
		    break;
		}
		--curwin->w_cursor.lnum;
		linelen = linetabsize(ml_get_curline());
		n = ((linelen + (curwin->w_p_nu ? 8 : 0) - 1) / Columns)
								    * Columns;
		if (curwin->w_p_nu &&
				 (long)curwin->w_curswant >= Columns - 8 && n)
		    n -= Columns;
		curwin->w_curswant += n;
	    }
	}
	else /* dir == FORWARD */
	{
	    n = ((linelen + (curwin->w_p_nu ? 8 : 0) - 1) / Columns) * Columns;
	    if (curwin->w_p_nu && n > 8)
		n -= 8;
						/* move forward within line */
	    if (curwin->w_curswant < (colnr_t)n)
		curwin->w_curswant += Columns;
	    else				/* to next line */
	    {
		if (curwin->w_cursor.lnum == curbuf->b_ml.ml_line_count)
		{
		    retval = FAIL;
		    break;
		}
		curwin->w_cursor.lnum++;
		linelen = linetabsize(ml_get_curline());
		curwin->w_curswant %= Columns;
	    }
	}
    }
    coladvance(curwin->w_curswant);
    if (atend)
	curwin->w_curswant = MAXCOL;	    /* stick in the last column */

    return retval;
}

/*
 * Handle CTRL-E and CTRL-Y commands: scroll a line up or down.
 */
    static void
nv_scroll_line(cap, is_ctrl_e)
    CMDARG	*cap;
    int		is_ctrl_e;	    /* TRUE for CTRL-E command */
{
    if (checkclearop(cap->oap))
	return;
    scroll_redraw(is_ctrl_e, cap->count1);
}

/*
 * Scroll "count" lines up or down, and redraw.
 */
    void
scroll_redraw(up, count)
    int		up;
    long	count;
{
    linenr_t	prev_topline = curwin->w_topline;
    linenr_t	prev_lnum = curwin->w_cursor.lnum;

    if (up)
	scrollup(count);
    else
	scrolldown(count);
    if (p_so)
    {
	cursor_correct();
	update_topline();
	/* If moved back to where we were, at least move the cursor, otherwise
	 * we get stuck at one position.  Don't move the cursor up if the
	 * first line of the buffer is already on the screen */
	if (curwin->w_topline == prev_topline)
	{
	    if (up)
		cursor_down(1L, FALSE);
	    else if (prev_topline != 1L)
		cursor_up(1L, FALSE);
	}
    }
    if (curwin->w_cursor.lnum != prev_lnum)
	coladvance(curwin->w_curswant);
    update_screen(VALID);
}

    static void
nv_zet(cap)
    CMDARG  *cap;
{
    long	n;
    colnr_t	col;
    int		nchar = cap->nchar;


    if (vim_isdigit(nchar))
    {
	n = nchar - '0';
	for (;;)
	{
#ifdef USE_ON_FLY_SCROLL
	    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
	    ++no_mapping;
	    ++allow_keys;   /* no mapping for nchar, but allow key codes */
	    nchar = safe_vgetc();
#ifdef HAVE_LANGMAP
	    LANGMAP_ADJUST(nchar, TRUE);
#endif
	    --no_mapping;
	    --allow_keys;
#ifdef CMDLINE_INFO
	    (void)add_to_showcmd(nchar);
#endif
	    if (nchar == K_DEL || nchar == K_KDEL)
		n /= 10;
	    else if (vim_isdigit(nchar))
		n = n * 10 + (nchar - '0');
	    else if (nchar == CR)
	    {
#ifdef USE_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight((int)n);
		break;
	    }
	    else if (nchar == 'l' || nchar == 'h' ||
					  nchar == K_LEFT || nchar == K_RIGHT)
	    {
		cap->count1 = n ? n * cap->count1 : cap->count1;
		goto dozet;
	    }
	    else
	    {
		clearopbeep(cap->oap);
		break;
	    }
	}
	cap->oap->op_type = OP_NOP;
	return;
    }
dozet:
    /*
     * If line number given, set cursor, except for "zh", "zl", "ze" and
     * "zs"
     */
    if (       vim_strchr((char_u *)"hles", nchar) == NULL
	    && nchar != K_LEFT
	    && nchar != K_RIGHT
	    && cap->count0
	    && cap->count0 != curwin->w_cursor.lnum)
    {
	setpcmark();
	if (cap->count0 > curbuf->b_ml.ml_line_count)
	    curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	else
	    curwin->w_cursor.lnum = cap->count0;
    }

    switch (nchar)
    {
	/* put cursor at top of screen */
    case '+':
    case K_KPLUS:
	if (cap->count0 == 0)
	{
	    /* No count given: put cursor at the line below screen */
	    validate_botline();	/* using w_botline, make sure it's valid */
	    if (curwin->w_botline > curbuf->b_ml.ml_line_count)
		curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    else
		curwin->w_cursor.lnum = curwin->w_botline;
	}
	/* FALLTHROUGH */
    case NL:
    case CR:
    case K_KENTER:
	beginline(BL_WHITE | BL_FIX);
	/* FALLTHROUGH */
    case 't':
	scroll_cursor_top(0, TRUE);
	break;

	/* put cursor in middle of screen */
    case '.':
	beginline(BL_WHITE | BL_FIX);
	/* FALLTHROUGH */
    case 'z':
	scroll_cursor_halfway(TRUE);
	break;

	/* put cursor at bottom of screen */
    case '^':
	/* Strange Vi behavior: <count>z^ finds line at top of window when
	 * <count> is at bottom of window, and puts that one at bottom of
	 * window. */
	if (cap->count0 != 0)
	{
	    scroll_cursor_bot(0, TRUE);
	    curwin->w_cursor.lnum = curwin->w_topline;
	}
	else if (curwin->w_topline == 1)
	    curwin->w_cursor.lnum = 1;
	else
	    curwin->w_cursor.lnum = curwin->w_topline - 1;
	/* FALLTHROUGH */
    case '-':
    case K_KMINUS:
	beginline(BL_WHITE | BL_FIX);
	/* FALLTHROUGH */
    case 'b':
	scroll_cursor_bot(0, TRUE);
	break;

	/* "zH" - scroll screen right half-page */
    case 'H':
	cap->count1 *= Columns / 2;
	/* FALLTHROUGH */

	/* "zh" - scroll screen to the right */
    case 'h':
    case K_LEFT:
	if (!curwin->w_p_wrap)
	{
	    if ((colnr_t)cap->count1 > curwin->w_leftcol)
		curwin->w_leftcol = 0;
	    else
		curwin->w_leftcol -= (colnr_t)cap->count1;
	    leftcol_changed();
	}
	break;

	/* "zL" - scroll screen left half-page */
    case 'L':
	cap->count1 *= Columns / 2;
	/* FALLTHROUGH */

	/* "zl" - scroll screen to the left */
    case 'l':
    case K_RIGHT:
	if (!curwin->w_p_wrap)
	{
	    /* scroll the window left */
	    curwin->w_leftcol += (colnr_t)cap->count1;
	    leftcol_changed();
	}
	break;

	/* "zs" - scroll screen, cursor at the start */
    case 's':
	if (!curwin->w_p_wrap)
	{
	    getvcol(curwin, &curwin->w_cursor, &col, NULL, NULL);
	    curwin->w_leftcol = col;
	    redraw_later(NOT_VALID);
	}
	break;

	/* "ze" - scroll screen, cursor at the end */
    case 'e':
	if (!curwin->w_p_wrap)
	{
	    getvcol(curwin, &curwin->w_cursor, NULL, NULL, &col);
	    n = Columns;
	    if (curwin->w_p_nu)
		n -= 8;
	    if ((long)col < n)
		curwin->w_leftcol = 0;
	    else
		curwin->w_leftcol = col - n + 1;
	    redraw_later(NOT_VALID);
	}
	break;

    case Ctrl('S'): /* ignore CTRL-S and CTRL-Q to avoid problems */
    case Ctrl('Q'): /* with terminals that use xon/xoff */
	break;

    default:
	clearopbeep(cap->oap);
    }
    update_screen(VALID);
}

/*
 * Handle a ":" command.
 */
    static void
nv_colon(cap)
    CMDARG  *cap;
{
    int	    old_p_im;

    if (VIsual_active)
	nv_operator(cap);
    else if (!checkclearop(cap->oap))
    {
	/* translate "count:" into ":.,.+(count - 1)" */
	if (cap->count0)
	{
	    stuffcharReadbuff('.');
	    if (cap->count0 > 1)
	    {
		stuffReadbuff((char_u *)",.+");
		stuffnumReadbuff((long)cap->count0 - 1L);
	    }
	}

	/* When typing, don't type below an old message */
	if (KeyTyped)
	    compute_cmdrow();

	old_p_im = p_im;

	/* get a command line and execute it */
	do_cmdline(NULL, getexline, NULL, 0);

	/* If 'insertmode' changed, enter or exit Insert mode */
	if (p_im != old_p_im)
	{
	    if (p_im)
		restart_edit = 'i';
	    else
		restart_edit = 0;
	}
    }
}

/*
 * Handle CTRL-G command.
 */
    static void
nv_ctrlg(cap)
    CMDARG *cap;
{
    if (VIsual_active)	/* toggle Selection/Visual mode */
    {
	VIsual_select = !VIsual_select;
	showmode();
    }
    else if (!checkclearop(cap->oap))
	/* print full name if count given or :cd used */
	fileinfo((int)cap->count0, FALSE, TRUE);
}

/*
 * "ZZ": write if changed, and exit window
 * "ZQ": quit window (Elvis compatible)
 */
    static void
nv_zzet(cap)
    CMDARG *cap;
{
    if (!checkclearopq(cap->oap))
    {
	if (cap->nchar == 'Z')
	    stuffReadbuff((char_u *)":x\n");
	else if (cap->nchar == 'Q')
	    stuffReadbuff((char_u *)":q!\n");
	else
	    clearopbeep(cap->oap);
    }
}

/*
 * Handle the commands that use the word under the cursor.
 *
 * Returns TRUE for "*" and "#" commands, indicating that the next search
 * should not set the pcmark.
 */
    static void
nv_ident(cap, searchp)
    CMDARG	*cap;
    char_u	**searchp;
{
    char_u	*ptr = NULL;
    int		n = 0;		/* init for GCC */
    int		cmdchar;
    int		g_cmd;		/* "g" command */
    char_u	*aux_ptr;
    int		isman;
    int		isman_s;

    if (cap->cmdchar == 'g')	/* "g*", "g#", "g]" and "gCTRL-]" */
    {
	cmdchar = cap->nchar;
	g_cmd = TRUE;
    }
    else
    {
	cmdchar = cap->cmdchar;
	g_cmd = FALSE;
    }

    /*
     * The "]", "CTRL-]" and "K" commands accept an argument in Visual mode.
     */
    if (cmdchar == ']' || cmdchar == Ctrl(']') || cmdchar == 'K')
    {
	if (VIsual_active)	/* :ta to visual highlighted text */
	{
	    if (VIsual_mode != 'V')
		unadjust_for_sel();
	    if (VIsual.lnum != curwin->w_cursor.lnum)
	    {
		clearopbeep(cap->oap);
		return;
	    }
	    if (lt(curwin->w_cursor, VIsual))
	    {
		ptr = ml_get_pos(&curwin->w_cursor);
		n = VIsual.col - curwin->w_cursor.col + 1;
	    }
	    else
	    {
		ptr = ml_get_pos(&VIsual);
		n = curwin->w_cursor.col - VIsual.col + 1;
	    }
	    end_visual_mode();
	    VIsual_reselect = FALSE;
	    redraw_curbuf_later(NOT_VALID);    /* update the inversion later */
	}
	if (checkclearopq(cap->oap))
	    return;
    }

    if (ptr == NULL && (n = find_ident_under_cursor(&ptr,
		    (cmdchar == '*' || cmdchar == '#')
				 ? FIND_IDENT|FIND_STRING : FIND_IDENT)) == 0)
    {
	clearop(cap->oap);
	return;
    }

    isman = (STRCMP(p_kp, "man") == 0);
    isman_s = (STRCMP(p_kp, "man -s") == 0);
    if (cap->count0 && !(cmdchar == 'K' && (isman || isman_s))
	    && !(cmdchar == '*' || cmdchar == '#'))
	stuffnumReadbuff(cap->count0);
    switch (cmdchar)
    {
	case '*':
	case '#':
	    /*
	     * Put cursor at start of word, makes search skip the word
	     * under the cursor.
	     * Call setpcmark() first, so "*``" puts the cursor back where
	     * it was.
	     */
	    setpcmark();
	    curwin->w_cursor.col = ptr - ml_get_curline();

	    if (!g_cmd && vim_iswordc(*ptr))
		stuffReadbuff((char_u *)"\\<");
	    no_smartcase = TRUE;	/* don't use 'smartcase' now */
	    break;

	case 'K':
	    if (*p_kp == NUL)
		stuffReadbuff((char_u *)":he ");
	    else
	    {
		stuffReadbuff((char_u *)":! ");
		if (!cap->count0 && isman_s)
		    stuffReadbuff((char_u *)"man");
		else
		    stuffReadbuff(p_kp);
		stuffReadbuff((char_u *)" ");
		if (cap->count0 && (isman || isman_s))
		{
		    stuffnumReadbuff(cap->count0);
		    stuffReadbuff((char_u *)" ");
		}
	    }
	    break;

	case ']':
#ifdef USE_CSCOPE
	    if (p_cst)
		stuffReadbuff((char_u *)":cstag ");
	    else
#endif
	    stuffReadbuff((char_u *)":ts ");
	    break;

	default:
	    if (curbuf->b_help)
		stuffReadbuff((char_u *)":he ");
	    else if (g_cmd)
		stuffReadbuff((char_u *)":tj ");
	    else
		stuffReadbuff((char_u *)":ta ");
    }

    /*
     * Now grab the chars in the identifier
     */
    if (cmdchar == '*' || cmdchar == '#')
	aux_ptr = (char_u *)(p_magic ? "/?.*~[^$\\" : "/?^$\\");
    else if (cmdchar == 'K' && *p_kp != NUL)
	aux_ptr = escape_chars;
    else
	/* Don't escape spaces and Tabs in a tag with a backslash */
	aux_ptr = (char_u *)"\\|\"";
    while (n--)
    {
	/* put a backslash before \ and some others */
	if (vim_strchr(aux_ptr, *ptr) != NULL)
	    stuffcharReadbuff('\\');
	/* don't interpret the characters as edit commands */
	else if (*ptr < ' ' || *ptr > '~')
	    stuffcharReadbuff(Ctrl('V'));
	stuffcharReadbuff(*ptr++);
    }

    if (       !g_cmd
	    && (cmdchar == '*' || cmdchar == '#')
	    && vim_iswordc(ptr[-1]))
	stuffReadbuff((char_u *)"\\>");
    stuffReadbuff((char_u *)"\n");

    /*
     * The search commands may be given after an operator.  Therefore they
     * must be executed right now.
     */
    if (cmdchar == '*' || cmdchar == '#')
    {
	if (cmdchar == '*')
	    cap->cmdchar = '/';
	else
	    cap->cmdchar = '?';
	nv_search(cap, searchp, TRUE);
    }
}

/*
 * Handle scrolling command 'H', 'L' and 'M'.
 */
    static void
nv_scroll(cap)
    CMDARG  *cap;
{
    int	    used = 0;
    long    n;

    cap->oap->motion_type = MLINE;
    setpcmark();

    if (cap->cmdchar == 'L')
    {
	validate_botline();	    /* make sure curwin->w_botline is valid */
	curwin->w_cursor.lnum = curwin->w_botline - 1;
	if (cap->count1 - 1 >= curwin->w_cursor.lnum)
	    curwin->w_cursor.lnum = 1;
	else
	    curwin->w_cursor.lnum -= cap->count1 - 1;
    }
    else
    {
	if (cap->cmdchar == 'M')
	{
	    validate_botline();	    /* make sure w_empty_rows is valid */
	    for (n = 0; curwin->w_topline + n < curbuf->b_ml.ml_line_count; ++n)
		if ((used += plines(curwin->w_topline + n)) >=
			    (curwin->w_height - curwin->w_empty_rows + 1) / 2)
		    break;
	    if (n && used > curwin->w_height)
		--n;
	}
	else
	    n = cap->count1 - 1;
	curwin->w_cursor.lnum = curwin->w_topline + n;
	if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
	    curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    }

    cursor_correct();	/* correct for 'so' */
    beginline(BL_SOL | BL_FIX);
}

/*
 * Cursor right commands.
 */
    static void
nv_right(cap)
    CMDARG	*cap;
{
    long	n;
    int		past_line;

    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    past_line = (VIsual_active && *p_sel != 'o');
    for (n = cap->count1; n > 0; --n)
    {
	if ((!past_line && oneright() == FAIL)
		|| (past_line && *ml_get_cursor() == NUL))
	{
	    /*
	     *	  <Space> wraps to next line if 'whichwrap' bit 1 set.
	     *	      'l' wraps to next line if 'whichwrap' bit 2 set.
	     * CURS_RIGHT wraps to next line if 'whichwrap' bit 3 set
	     */
	    if (       ((cap->cmdchar == ' '
			    && vim_strchr(p_ww, 's') != NULL)
			|| (cap->cmdchar == 'l'
			    && vim_strchr(p_ww, 'l') != NULL)
			|| (cap->cmdchar == K_RIGHT
			    && vim_strchr(p_ww, '>') != NULL))
		    && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
	    {
		/* When deleting we also count the NL as a character.
		 * Set cap->oap->inclusive when last char in the line is
		 * included, move to next line after that */
		if (	   (cap->oap->op_type == OP_DELETE
			    || cap->oap->op_type == OP_CHANGE)
			&& !cap->oap->inclusive
			&& !lineempty(curwin->w_cursor.lnum))
		    cap->oap->inclusive = TRUE;
		else
		{
		    ++curwin->w_cursor.lnum;
		    curwin->w_cursor.col = 0;
		    curwin->w_set_curswant = TRUE;
		    cap->oap->inclusive = FALSE;
		}
		continue;
	    }
	    if (cap->oap->op_type == OP_NOP)
	    {
		/* Only beep and flush if not moved at all */
		if (n == cap->count1)
		    beep_flush();
	    }
	    else
	    {
		if (!lineempty(curwin->w_cursor.lnum))
		    cap->oap->inclusive = TRUE;
	    }
	    break;
	}
	else if (past_line)
	{
	    curwin->w_set_curswant = TRUE;
	    ++curwin->w_cursor.col;
#ifdef MULTI_BYTE
	    if (is_dbcs && mb_isbyte2(ml_get(curwin->w_cursor.lnum),
							curwin->w_cursor.col))
		++curwin->w_cursor.col;
#endif
	}
    }
}

/*
 * Cursor left commands.
 *
 * Returns TRUE when operator end should not be adjusted.
 */
    static int
nv_left(cap)
    CMDARG	*cap;
{
    long	n;
    int		retval = FALSE;

    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    for (n = cap->count1; n > 0; --n)
    {
	if (oneleft() == FAIL)
	{
	    /* <BS> and <Del> wrap to previous line if 'whichwrap' has 'b'.
	     *		 'h' wraps to previous line if 'whichwrap' has 'h'.
	     *	   CURS_LEFT wraps to previous line if 'whichwrap' has '<'.
	     */
	    if (       (((cap->cmdchar == K_BS
				|| cap->cmdchar == Ctrl('H'))
			    && vim_strchr(p_ww, 'b') != NULL)
			|| (cap->cmdchar == 'h'
			    && vim_strchr(p_ww, 'h') != NULL)
			|| (cap->cmdchar == K_LEFT
			    && vim_strchr(p_ww, '<') != NULL))
		    && curwin->w_cursor.lnum > 1)
	    {
		--(curwin->w_cursor.lnum);
		coladvance(MAXCOL);
		curwin->w_set_curswant = TRUE;

		/* When the NL before the first char has to be deleted we
		 * put the cursor on the NUL after the previous line.
		 * This is a very special case, be careful!
		 * don't adjust op_end now, otherwise it won't work */
		if (	   (cap->oap->op_type == OP_DELETE
			    || cap->oap->op_type == OP_CHANGE)
			&& !lineempty(curwin->w_cursor.lnum))
		{
		    ++curwin->w_cursor.col;
		    retval = TRUE;
		}
		continue;
	    }
	    /* Only beep and flush if not moved at all */
	    else if (cap->oap->op_type == OP_NOP && n == cap->count1)
		beep_flush();
	    break;
	}
    }

    return retval;
}

#ifdef FILE_IN_PATH
/*
 * Grab the file name under the cursor and edit it.
 */
    static void
nv_gotofile(cap)
    CMDARG	*cap;
{
    char_u	*ptr;

    ptr = file_name_at_cursor(FNAME_MESS|FNAME_HYP|FNAME_EXP, cap->count1);
    if (ptr != NULL)
    {
	/* do autowrite if necessary */
	if (curbuf_changed() && curbuf->b_nwindows <= 1 && !p_hid)
	    autowrite(curbuf, FALSE);
	setpcmark();
	(void)do_ecmd(0, ptr, NULL, NULL, ECMD_LAST, p_hid ? ECMD_HIDE : 0);
	vim_free(ptr);
    }
    else
	clearop(cap->oap);
}
#endif

/*
 * Handle the "$" command.
 */
    static void
nv_dollar(cap)
    CMDARG	*cap;
{
    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = TRUE;
    curwin->w_curswant = MAXCOL;	/* so we stay at the end */
    if (cursor_down((long)(cap->count1 - 1),
					 cap->oap->op_type == OP_NOP) == FAIL)
	clearopbeep(cap->oap);
}

/*
 * Implementation of '?' and '/' commands.
 */
    static void
nv_search(cap, searchp, dont_set_mark)
    CMDARG	    *cap;
    char_u	    **searchp;
    int		    dont_set_mark;	/* don't set PC mark */
{
    OPARG	*oap = cap->oap;
    int		i;

    if (cap->cmdchar == '?' && cap->oap->op_type == OP_ROT13)
    {
	/* Translate "g??" to "g?g?" */
	cap->cmdchar = 'g';
	cap->nchar = '?';
	nv_operator(cap);
	return;
    }

    if ((*searchp = getcmdline(cap->cmdchar, cap->count1, 0)) == NULL)
    {
	clearop(oap);
	return;
    }
    oap->motion_type = MCHAR;
    oap->inclusive = FALSE;
    curwin->w_set_curswant = TRUE;

    i = do_search(oap, cap->cmdchar, *searchp, cap->count1,
	    (dont_set_mark ? 0 : SEARCH_MARK) |
	    SEARCH_OPT | SEARCH_ECHO | SEARCH_MSG);
    if (i == 0)
	clearop(oap);
    else if (i == 2)
	oap->motion_type = MLINE;

    /* "/$" will put the cursor after the end of the line, may need to
     * correct that here */
    adjust_cursor();
}

/*
 * Handle "N" and "n" commands.
 */
    static void
nv_next(cap, flag)
    CMDARG	*cap;
    int		flag;
{
    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    curwin->w_set_curswant = TRUE;
    if (!do_search(cap->oap, 0, NULL, cap->count1,
	      SEARCH_MARK | SEARCH_OPT | SEARCH_ECHO | SEARCH_MSG | flag))
	clearop(cap->oap);

    /* "/$" will put the cursor after the end of the line, may need to
     * correct that here */
    adjust_cursor();
}

    static void
nv_csearch(cap, dir, type)
    CMDARG	*cap;
    int		dir;
    int		type;
{
    cap->oap->motion_type = MCHAR;
    if (dir == BACKWARD)
	cap->oap->inclusive = FALSE;
    else
	cap->oap->inclusive = TRUE;
    if (cap->nchar >= 0x100 || !searchc(cap->nchar, dir, type, cap->count1))
	clearopbeep(cap->oap);
    else
    {
	curwin->w_set_curswant = TRUE;
	adjust_for_sel(cap);
    }
}

    static void
nv_brackets(cap, dir)
    CMDARG	*cap;
    int		dir;		    /* BACKWARD or FORWARD */
{
    FPOS	new_pos;
    FPOS	prev_pos;
    FPOS	*pos = NULL;	    /* init for GCC */
    FPOS	old_pos;	    /* cursor position before command */
    int		flag;
    long	n;
    int		findc;
    int		c;

    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    old_pos = curwin->w_cursor;

#ifdef FILE_IN_PATH
    /*
     * "[f" or "]f" : Edit file under the cursor (same as "gf")
     */
    if (cap->nchar == 'f')
	nv_gotofile(cap);
    else
#endif

#ifdef FIND_IN_PATH
    /*
     * Find the occurence(s) of the identifier or define under cursor
     * in current and included files or jump to the first occurence.
     *
     *			search	     list	    jump
     *		      fwd   bwd    fwd	 bwd	 fwd	bwd
     * identifier     "]i"  "[i"   "]I"  "[I"	"]^I"  "[^I"
     * define	      "]d"  "[d"   "]D"  "[D"	"]^D"  "[^D"
     */
    if (vim_strchr((char_u *)"iI\011dD\004", cap->nchar) != NULL)
    {
	char_u	*ptr;
	int	len;

	if ((len = find_ident_under_cursor(&ptr, FIND_IDENT)) == 0)
	    clearop(cap->oap);
	else
	{
	    find_pattern_in_path(ptr, 0, len, TRUE,
		cap->count0 == 0 ? !isupper(cap->nchar) : FALSE,
		((cap->nchar & 0xf) == ('d' & 0xf)) ?  FIND_DEFINE : FIND_ANY,
		cap->count1,
		isupper(cap->nchar) ? ACTION_SHOW_ALL :
			    islower(cap->nchar) ? ACTION_SHOW : ACTION_GOTO,
		cap->cmdchar == ']' ? curwin->w_cursor.lnum : (linenr_t)1,
		(linenr_t)MAXLNUM);
	    curwin->w_set_curswant = TRUE;
	}
    }
    else
#endif

    /*
     * "[{", "[(", "]}" or "])": go to Nth unclosed '{', '(', '}' or ')'
     * "[#", "]#": go to start/end of Nth innermost #if..#endif construct.
     * "[/", "[*", "]/", "]*": go to Nth comment start/end.
     * "[m" or "]m" search for prev/next start of (Java) method.
     * "[M" or "]M" search for prev/next end of (Java) method.
     */
    if (  (cap->cmdchar == '['
		&& vim_strchr((char_u *)"{(*/#mM", cap->nchar) != NULL)
	    || (cap->cmdchar == ']'
		&& vim_strchr((char_u *)"})*/#mM", cap->nchar) != NULL))
    {
	if (cap->nchar == '*')
	    cap->nchar = '/';
	new_pos.lnum = 0;
	prev_pos.lnum = 0;
	if (cap->nchar == 'm' || cap->nchar == 'M')
	{
	    if (cap->cmdchar == '[')
		findc = '{';
	    else
		findc = '}';
	    n = 9999;
	}
	else
	{
	    findc = cap->nchar;
	    n = cap->count1;
	}
	for ( ; n > 0; --n)
	{
	    if ((pos = findmatchlimit(cap->oap, findc,
		(cap->cmdchar == '[') ? FM_BACKWARD : FM_FORWARD, 0)) == NULL)
	    {
		if (new_pos.lnum == 0)	/* nothing found */
		{
		    if (cap->nchar != 'm' && cap->nchar != 'M')
			clearopbeep(cap->oap);
		}
		else
		    pos = &new_pos;	/* use last one found */
		break;
	    }
	    prev_pos = new_pos;
	    curwin->w_cursor = *pos;
	    new_pos = *pos;
	}
	curwin->w_cursor = old_pos;

	/*
	 * Handle "[m", "]m", "[M" and "[M".  The findmatchlimit() only
	 * brought us to the match for "[m" and "]M" when inside a method.
	 * Try finding the '{' or '}' we want to be at.
	 * Also repeat for the given count.
	 */
	if (cap->nchar == 'm' || cap->nchar == 'M')
	{
	    /* norm is TRUE for "]M" and "[m" */
	    int	    norm = ((findc == '{') == (cap->nchar == 'm'));

	    n = cap->count1;
	    /* found a match: we were inside a method */
	    if (prev_pos.lnum != 0)
	    {
		pos = &prev_pos;
		curwin->w_cursor = prev_pos;
		if (norm)
		    --n;
	    }
	    else
		pos = NULL;
	    while (n > 0)
	    {
		for (;;)
		{
		    if ((findc == '{' ? dec_cursor() : inc_cursor()) < 0)
		    {
			/* if not found anything, that's an error */
			if (pos == NULL)
			    clearopbeep(cap->oap);
			n = 0;
			break;
		    }
		    c = gchar_cursor();
		    if (c == '{' || c == '}')
		    {
			/* Must have found end/start of class: use it.
			 * Or found the place to be at. */
			if ((c == findc && norm) || (n == 1 && !norm))
			{
			    new_pos = curwin->w_cursor;
			    pos = &new_pos;
			    n = 0;
			}
			/* if no match found at all, we started outside of the
			 * class and we're inside now.  Just go on. */
			else if (new_pos.lnum == 0)
			{
			    new_pos = curwin->w_cursor;
			    pos = &new_pos;
			}
			/* found start/end of other method: go to match */
			else if ((pos = findmatchlimit(cap->oap, findc,
			    (cap->cmdchar == '[') ? FM_BACKWARD : FM_FORWARD,
								  0)) == NULL)
			    n = 0;
			else
			    curwin->w_cursor = *pos;
			break;
		    }
		}
		--n;
	    }
	    curwin->w_cursor = old_pos;
	    if (pos == NULL && new_pos.lnum != 0)
		clearopbeep(cap->oap);
	}
	if (pos != NULL)
	{
	    setpcmark();
	    curwin->w_cursor = *pos;
	    curwin->w_set_curswant = TRUE;
	}
    }

    /*
     * "[[", "[]", "]]" and "][": move to start or end of function
     */
    else if (cap->nchar == '[' || cap->nchar == ']')
    {
	if (cap->nchar == cap->cmdchar)		    /* "]]" or "[[" */
	    flag = '{';
	else
	    flag = '}';		    /* "][" or "[]" */

	curwin->w_set_curswant = TRUE;
	/*
	 * Imitate strange Vi behaviour: When using "]]" with an operator
	 * we also stop at '}'.
	 */
	if (!findpar(cap->oap, dir, cap->count1, flag,
	      (cap->oap->op_type != OP_NOP && dir == FORWARD && flag == '{')))
	    clearopbeep(cap->oap);
	else if (cap->oap->op_type == OP_NOP)
	    beginline(BL_WHITE | BL_FIX);
    }

    /*
     * "[p", "[P", "]P" and "]p": put with indent adjustment
     */
    else if (cap->nchar == 'p' || cap->nchar == 'P')
    {
	if (!checkclearopq(cap->oap))
	{
	    prep_redo_cmd(cap);
	    do_put(cap->oap->regname,
	      (cap->cmdchar == ']' && cap->nchar == 'p') ? FORWARD : BACKWARD,
						  cap->count1, PUT_FIXINDENT);
	}
    }

#ifdef USE_MOUSE
    /*
     * [ or ] followed by a middle mouse click: put selected text with
     * indent adjustment.  Any other button just does as usual.
     */
    else if (cap->nchar >= K_LEFTMOUSE && cap->nchar <= K_RIGHTRELEASE)
    {
	(void)do_mouse(cap->oap, cap->nchar,
		       (cap->cmdchar == ']') ? FORWARD : BACKWARD,
		       cap->count1, PUT_FIXINDENT);
    }
#endif /* USE_MOUSE */

    /* Not a valid cap->nchar. */
    else
	clearopbeep(cap->oap);
}

/*
 * Handle Normal mode "%" command.
 */
    static void
nv_percent(cap)
    CMDARG	*cap;
{
    FPOS	*pos;

    cap->oap->inclusive = TRUE;
    if (cap->count0)	    /* {cnt}% : goto {cnt} percentage in file */
    {
	if (cap->count0 > 100)
	    clearopbeep(cap->oap);
	else
	{
	    cap->oap->motion_type = MLINE;
	    setpcmark();
			    /* round up, so CTRL-G will give same value */
	    curwin->w_cursor.lnum = (curbuf->b_ml.ml_line_count *
						      cap->count0 + 99) / 100;
	    beginline(BL_SOL | BL_FIX);
	}
    }
    else		    /* "%" : go to matching paren */
    {
	cap->oap->motion_type = MCHAR;
	if ((pos = findmatch(cap->oap, NUL)) == NULL)
	    clearopbeep(cap->oap);
	else
	{
	    setpcmark();
	    curwin->w_cursor = *pos;
	    curwin->w_set_curswant = TRUE;
	    adjust_for_sel(cap);
	}
    }
}

/*
 * Handle "(" and ")" commands.
 */
    static void
nv_brace(cap, dir)
    CMDARG	*cap;
    int		dir;
{
    cap->oap->motion_type = MCHAR;
    if (cap->cmdchar == ')')
	cap->oap->inclusive = FALSE;
    else
	cap->oap->inclusive = TRUE;
    curwin->w_set_curswant = TRUE;

    if (findsent(dir, cap->count1) == FAIL)
	clearopbeep(cap->oap);
}

/*
 * Handle the "{" and "}" commands.
 */
    static void
nv_findpar(cap, dir)
    CMDARG	*cap;
    int		dir;
{
    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    curwin->w_set_curswant = TRUE;
    if (!findpar(cap->oap, dir, cap->count1, NUL, FALSE))
	clearopbeep(cap->oap);
}

/*
 * Handle the "r" command.
 */
    static int
nv_replace(cap)
    CMDARG	*cap;
{
    char_u	*ptr;
    int		had_ctrl_v;
    int		command_busy = FALSE;
    long	n;

    /* Visual mode "r" */
    if (VIsual_active)
    {
	nv_operator(cap);
	return command_busy;
    }

    if (checkclearop(cap->oap))
	return command_busy;

    ptr = ml_get_cursor();
    /* special key or not enough characters to replace */
#ifdef MULTI_BYTE
    if (
	/* SB case: */
	(!is_dbcs
	     && ((cap->nchar >= 0x100 || STRLEN(ptr) < (unsigned)cap->count1)))
	/* DBCS case: */
	|| (is_dbcs && ((cap->nchar >= 0x100
	    /* does not support multi-replacement for multi-byte */
	    || (IsLeadByte(cap->nchar) && cap->count1 > 1)
	    || ((MultiStrLen(ptr) < cap->count1)
		/* support single-replacement for multi-byte */
		&& !(STRLEN(ptr) >= 2 && (unsigned)cap->count1 == 1))))))
#else
	if (cap->nchar >= 0x100 || STRLEN(ptr) < (unsigned)cap->count1)
#endif
    {
	clearopbeep(cap->oap);
	return FALSE;
    }

    /*
     * Replacing with a TAB is done by edit(), because it is complicated when
     * 'expandtab', 'smarttab' or 'softtabstop' is set.
     * Other characters are done below to avoid problems with things like
     * CTRL-V 048 (for edit() this would be R CTRL-V 0 ESC).
     */
    if (cap->nchar == '\t' && (curbuf->b_p_et || p_sta))
    {
	stuffnumReadbuff(cap->count1);
	stuffcharReadbuff('R');
	stuffcharReadbuff('\t');
	stuffcharReadbuff(ESC);
	return FALSE;
    }

    if (cap->nchar == Ctrl('V'))		/* get another character */
    {
	had_ctrl_v = Ctrl('V');
	cap->nchar = get_literal();
    }
    else
	had_ctrl_v = NUL;
    if (u_save_cursor() == FAIL)	/* save line for undo */
	return FALSE;

    if (had_ctrl_v != Ctrl('V') && (cap->nchar == '\r' || cap->nchar == '\n'))
    {
	/*
	 * Replace character(s) by a single newline.
	 * Strange vi behaviour: Only one newline is inserted.
	 * Delete the characters here.
	 * Insert the newline with an insert command, takes care of
	 * autoindent.	The insert command depends on being on the last
	 * character of a line or not.
	 */
	(void)del_chars(cap->count1, FALSE);	/* delete the characters */
	stuffcharReadbuff('\r');
	stuffcharReadbuff(ESC);
	/*
	 * Give 'r' to edit(), to get the redo command right.
	 */
	command_busy = edit('r', FALSE, cap->count1);
    }
    else
    {
#ifdef MULTI_BYTE
	int	trailbyte = NUL;
	int	prechar = NUL;		/* init for GCC */

	if (is_dbcs && IsLeadByte(cap->nchar))
	    trailbyte = (char_u)safe_vgetc();
#endif
	prep_redo(cap->oap->regname, cap->count1,
					    NUL, 'r', had_ctrl_v, cap->nchar);
	for (n = cap->count1; n > 0; --n)	/* replace the characters */
	{
	    /*
	     * Replace a 'normal' character.
	     * Get ptr again, because u_save and/or showmatch() will have
	     * released the line.  At the same time we let know that the line
	     * will be changed.
	     */
	    ptr = ml_get_buf(curbuf, curwin->w_cursor.lnum, TRUE);
#ifdef MULTI_BYTE
	    if (is_dbcs)
		prechar = ptr[curwin->w_cursor.col];
#endif
	    ptr[curwin->w_cursor.col] = cap->nchar;
	    if (       p_sm
		    && (cap->nchar == ')'
			|| cap->nchar == '}'
			|| cap->nchar == ']'))
		showmatch();
	    ++curwin->w_cursor.col;
#ifdef MULTI_BYTE
	    if (is_dbcs)
	    {
		/* Handle three situations:
		 * 1. replace double-byte with double-byte: set trailbyte.
		 * 2. replace single-byte with double-byte: insert trailbyte.
		 * 3. replace double-byte with single-byte: delete char.
		 */
		if (trailbyte != NUL)
		{
		    if (IsLeadByte(prechar))
		    {
			ptr[curwin->w_cursor.col] = trailbyte;
			++curwin->w_cursor.col;
		    }
		    else
		    {
			(void)ins_char(trailbyte);
			if (n > 1)
			    /* pointer will have changed, get it again */
			   ptr = ml_get_buf(curbuf, curwin->w_cursor.lnum,
									TRUE);
		    }
		}
		else if (IsLeadByte(prechar))
		{
		    mch_memmove(ptr + curwin->w_cursor.col,
			    ptr + curwin->w_cursor.col + 1,
			    STRLEN(ptr + curwin->w_cursor.col));
		}
	    }
#endif
	}
	--curwin->w_cursor.col;	    /* cursor on the last replaced char */
	curwin->w_set_curswant = TRUE;
	set_last_insert(cap->nchar);
	changed_cline_bef_curs();   /* update cursor screen pos. later */
	/* w_botline might change a bit when replacing special characters */
	approximate_botline();
	update_screenline();
    }
    changed();

    return command_busy;
}

/*
 * 'o': Exchange start and end of Visual area.
 * 'O': same, but in block mode exchange left and right corners.
 */
    static void
v_swap_corners(cap)
    CMDARG	*cap;
{
    FPOS	old_cursor;
    colnr_t	left, right;

    if ((cap->cmdchar == 'O') && VIsual_mode == Ctrl('V'))
    {
	old_cursor = curwin->w_cursor;
	getvcols(&old_cursor, &VIsual, &left, &right);
	curwin->w_cursor.lnum = VIsual.lnum;
	coladvance(left);
	VIsual = curwin->w_cursor;
	curwin->w_cursor.lnum = old_cursor.lnum;
	coladvance(right);
	curwin->w_curswant = right;
	if (curwin->w_cursor.col == old_cursor.col)
	{
	    curwin->w_cursor.lnum = VIsual.lnum;
	    coladvance(right);
	    VIsual = curwin->w_cursor;
	    curwin->w_cursor.lnum = old_cursor.lnum;
	    coladvance(left);
	    curwin->w_curswant = left;
	}
    }
    if (cap->cmdchar != 'O' || VIsual_mode != Ctrl('V'))
    {
	old_cursor = curwin->w_cursor;
	curwin->w_cursor = VIsual;
	VIsual = old_cursor;
	curwin->w_set_curswant = TRUE;
    }
}

/*
 * "R".
 */
    static int
nv_Replace(cap)
    CMDARG	    *cap;
{
    int		    command_busy = FALSE;

    if (VIsual_active)		/* "R" is replace lines */
    {
	cap->cmdchar = 'c';
	VIsual_mode = 'V';
	nv_operator(cap);
    }
    else if (!checkclearopq(cap->oap))
    {
	if (u_save_cursor() == OK)
	{
	    /* This is a new edit command, not a restart.  We don't edit
	     * recursively. */
	    restart_edit = 0;
	    command_busy = edit('R', FALSE, cap->count1);
	}
    }
    return command_busy;
}

/*
 * "gR".
 */
    static int
nv_VReplace(cap)
    CMDARG	    *cap;
{
    int		    command_busy = FALSE;

    if (VIsual_active)
    {
	cap->cmdchar = 'R';
	cap->nchar = NUL;
	return nv_Replace(cap);	/* Do same as "R" in Visual mode for now */
    }
    else if (!checkclearopq(cap->oap))
    {
	if (u_save_cursor() == OK)
	{
	    /* This is a new edit command, not a restart.  We don't edit
	     * recursively. */
	    restart_edit = 0;
	    command_busy = edit('V', FALSE, cap->count1);
	}
    }
    return command_busy;
}

/*
 * "gr".
 */
    static int
nv_vreplace(cap)
    CMDARG	    *cap;
{
    int		    command_busy = FALSE;

    if (VIsual_active)
    {
	cap->cmdchar = 'r';
	cap->nchar = cap->extra_char;
	return nv_replace(cap);	/* Do same as "r" in Visual mode for now */
    }
    else if (!checkclearopq(cap->oap))
    {
	if (u_save_cursor() == OK)
	{
	    if (cap->extra_char == Ctrl('V'))	/* get another character */
		cap->extra_char = get_literal();
	    stuffcharReadbuff(cap->extra_char);
	    stuffcharReadbuff(ESC);
	    /* This is a new edit command, not a restart.  We don't edit
	     * recursively. */
	    restart_edit = 0;
	    command_busy = edit('v', FALSE, cap->count1);
	}
    }
    return command_busy;
}

/*
 * Swap case for "~" command, when it does not work like an operator.
 */
    static void
n_swapchar(cap)
    CMDARG	*cap;
{
    long	n;

    if (checkclearopq(cap->oap))
	return;

    if (lineempty(curwin->w_cursor.lnum) && vim_strchr(p_ww, '~') == NULL)
    {
	clearopbeep(cap->oap);
	return;
    }

    prep_redo_cmd(cap);

    if (u_save_cursor() == FAIL)
	return;

    for (n = cap->count1; n > 0; --n)
    {
	swapchar(cap->oap->op_type, &curwin->w_cursor);
	inc_cursor();
	if (gchar_cursor() == NUL)
	{
	    if (vim_strchr(p_ww, '~') != NULL
		    && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
	    {
		++curwin->w_cursor.lnum;
		curwin->w_cursor.col = 0;
		redraw_curbuf_later(NOT_VALID);
		if (n > 1)
		{
		    if (u_savesub(curwin->w_cursor.lnum) == FAIL)
			break;
		    u_clearline();
		}
	    }
	    else
		break;
	}
    }

    adjust_cursor();
    curwin->w_set_curswant = TRUE;
    changed();

    /* assume that the length of the line doesn't change, so w_botline
     * remains valid */
    update_screenline();
}

/*
 * Move cursor to mark.
 */
    static void
nv_cursormark(cap, flag, pos)
    CMDARG	*cap;
    int		flag;
    FPOS	*pos;
{
    if (check_mark(pos) == FAIL)
	clearop(cap->oap);
    else
    {
	if (cap->cmdchar == '\'' || cap->cmdchar == '`')
	    setpcmark();
	curwin->w_cursor = *pos;
	if (flag)
	    beginline(BL_WHITE | BL_FIX);
	else
	    adjust_cursor();
    }
    cap->oap->motion_type = flag ? MLINE : MCHAR;
    cap->oap->inclusive = FALSE;		/* ignored if not MCHAR */
    curwin->w_set_curswant = TRUE;
}

/*
 * Handle commands that are operators in Visual mode.
 */
    static void
v_visop(cap)
    CMDARG	*cap;
{
    static char_u trans[] = "YyDdCcxdXdAAIIrr";

    /* Uppercase means linewise, except in block mode, then "D" deletes till
     * the end of the line, and "C" replaces til EOL */
    if (isupper(cap->cmdchar))
    {
	if (VIsual_mode != Ctrl('V'))
	    VIsual_mode = 'V';
	else if (cap->cmdchar == 'C' || cap->cmdchar == 'D')
	    curwin->w_curswant = MAXCOL;
    }
    cap->cmdchar = *(vim_strchr(trans, cap->cmdchar) + 1);
    nv_operator(cap);
}

/*
 * Translate a command into another command.
 */
    static void
nv_optrans(cap)
    CMDARG	*cap;
{
    static char_u *(ar[8]) = {(char_u *)"dl", (char_u *)"dh",
			      (char_u *)"d$", (char_u *)"c$",
			      (char_u *)"cl", (char_u *)"cc",
			      (char_u *)"yy", (char_u *)":s\r"};
    static char_u *str = (char_u *)"xXDCsSY&";

    if (!checkclearopq(cap->oap))
    {
	if (cap->count0)
	    stuffnumReadbuff(cap->count0);
	stuffReadbuff(ar[(int)(vim_strchr(str, cap->cmdchar) - str)]);
    }
}

/*
 * Handle "'" and "`" commands.
 */
    static void
nv_gomark(cap, flag)
    CMDARG	*cap;
    int		flag;
{
    FPOS	*pos;

    pos = getmark(cap->nchar, (cap->oap->op_type == OP_NOP));
    if (pos == (FPOS *)-1)	    /* jumped to other file */
    {
	if (flag)
	{
	    check_cursor_lnum();
	    beginline(BL_WHITE | BL_FIX);
	}
	else
	    adjust_cursor();
    }
    else
	nv_cursormark(cap, flag, pos);
}

/*
 * Handle CTRL-O and CTRL-I commands.
 */
    static void
nv_pcmark(cap)
    CMDARG	*cap;
{
    FPOS	*pos;

    if (!checkclearopq(cap->oap))
    {
	pos = movemark((int)cap->count1);
	if (pos == (FPOS *)-1)		/* jump to other file */
	{
	    curwin->w_set_curswant = TRUE;
	    adjust_cursor();
	}
	else if (pos != NULL)		    /* can jump */
	    nv_cursormark(cap, FALSE, pos);
	else
	    clearopbeep(cap->oap);
    }
}

/*
 * Handle '"' command.
 */
    static void
nv_regname(cap, opnump)
    CMDARG	*cap;
    linenr_t	*opnump;
{
    if (checkclearop(cap->oap))
	return;
#ifdef WANT_EVAL
    if (cap->nchar == '=')
	cap->nchar = get_expr_register();
#endif
    if (cap->nchar != NUL && valid_yank_reg(cap->nchar, FALSE))
    {
	cap->oap->regname = cap->nchar;
	*opnump = cap->count0;	    /* remember count before '"' */
    }
    else
	clearopbeep(cap->oap);
}

/*
 * Handle "v", "V" and "CTRL-V" commands.
 * Also for "gh", "gH" and "g^H" commands.
 */
    static void
nv_visual(cap, selectmode)
    CMDARG	*cap;
    int		selectmode;	    /* Always start selectmode */
{
    VIsual_select = selectmode;
    if (VIsual_active)	    /* change Visual mode */
    {
	if (VIsual_mode == cap->cmdchar)    /* stop visual mode */
	    end_visual_mode();
	else				    /* toggle char/block mode */
	{				    /*	   or char/line mode */
	    VIsual_mode = cap->cmdchar;
	    showmode();
	}
	update_curbuf(NOT_VALID);	    /* update the inversion */
    }
    else		    /* start Visual mode */
    {
	check_visual_highlight();
	if (cap->count0)		    /* use previously selected part */
	{
	    if (resel_VIsual_mode == NUL)   /* there is none */
	    {
		beep_flush();
		return;
	    }
	    VIsual = curwin->w_cursor;
	    VIsual_active = TRUE;
	    VIsual_reselect = TRUE;
	    if (!selectmode)
		/* start Select mode when 'selectmode' contains "cmd" */
		may_start_select('c');
#ifdef USE_MOUSE
	    setmouse();
#endif
	    if (p_smd)
		redraw_cmdline = TRUE;	    /* show visual mode later */
	    /*
	     * For V and ^V, we multiply the number of lines even if there
	     * was only one -- webb
	     */
	    if (resel_VIsual_mode != 'v' || resel_VIsual_line_count > 1)
	    {
		curwin->w_cursor.lnum +=
				    resel_VIsual_line_count * cap->count0 - 1;
		if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		    curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    }
	    VIsual_mode = resel_VIsual_mode;
	    if (VIsual_mode == 'v')
	    {
		if (resel_VIsual_line_count <= 1)
		    curwin->w_cursor.col += resel_VIsual_col * cap->count0 - 1;
		else
		    curwin->w_cursor.col = resel_VIsual_col;
	    }
	    if (resel_VIsual_col == MAXCOL)
	    {
		curwin->w_curswant = MAXCOL;
		coladvance(MAXCOL);
	    }
	    else if (VIsual_mode == Ctrl('V'))
	    {
		validate_virtcol();
		curwin->w_curswant = curwin->w_virtcol +
					   resel_VIsual_col * cap->count0 - 1;
		coladvance(curwin->w_curswant);
	    }
	    else
		curwin->w_set_curswant = TRUE;
	    update_curbuf(NOT_VALID);	/* show the inversion */
	}
	else
	{
	    if (!selectmode)
		/* start Select mode when 'selectmode' contains "cmd" */
		may_start_select('c');
	    n_start_visual_mode(cap->cmdchar);
	}
    }
}

/*
 * Start selection for Shift-movement keys.
 */
    void
start_selection()
{
    /* if 'selectmode' contains "key", start Select mode */
    may_start_select('k');
    n_start_visual_mode('v');
}

/*
 * Start Select mode, if "c" is in 'selectmode' and not in a mapping or menu.
 */
    void
may_start_select(c)
    int		c;
{
    VIsual_select = (stuff_empty() && typebuf_typed()
		    && (vim_strchr(p_slm, c) != NULL));
}

/*
 * Start Visual mode "c".
 * Should set VIsual_select before calling this.
 */
    static void
n_start_visual_mode(c)
    int		c;
{
    VIsual = curwin->w_cursor;
    VIsual_mode = c;
    VIsual_active = TRUE;
    VIsual_reselect = TRUE;
#ifdef USE_MOUSE
    setmouse();
#endif
    if (p_smd)
	redraw_cmdline = TRUE;	/* show visual mode later */
#ifdef USE_CLIPBOARD
    /* Make sure the clipboard gets updated.  Needed because start and
     * end may still be the same, and the selection needs to be owned */
    clipboard.vmode = NUL;
#endif
    update_screenline();	/* start the inversion */
}

/*
 * Handle the "g" command.
 */
    static int
nv_g_cmd(cap, searchp)
    CMDARG	*cap;
    char_u	**searchp;
{
    OPARG	*oap = cap->oap;
    FPOS	tpos;
    int		i;
    int		flag = FALSE;
    int		command_busy = FALSE;

    switch (cap->nchar)
    {
#ifdef MEM_PROFILE
    /*
     * "g^A": dump log of used memory.
     */
    case Ctrl('A'):
	vim_mem_profile_dump();
	break;
#endif

    /*
     * "gR": Enter virtual replace mode.
     */
    case 'R':
	command_busy = nv_VReplace(cap);
	break;

    case 'r':
	command_busy = nv_vreplace(cap);
	break;

    /*
     * "gv": Reselect the previous Visual area.  If Visual already active,
     *	     exchange previous and current Visual area.
     */
    case 'v':
	if (checkclearop(oap))
	    break;

	if (	   curbuf->b_visual_start.lnum == 0
		|| curbuf->b_visual_start.lnum > curbuf->b_ml.ml_line_count
		|| curbuf->b_visual_end.lnum == 0)
	    beep_flush();
	else
	{
	    /* set w_cursor to the start of the Visual area, tpos to the end */
	    if (VIsual_active)
	    {
		i = VIsual_mode;
		VIsual_mode = curbuf->b_visual_mode;
		curbuf->b_visual_mode = i;
		tpos = curbuf->b_visual_end;
		curbuf->b_visual_end = curwin->w_cursor;
		curwin->w_cursor = curbuf->b_visual_start;
		curbuf->b_visual_start = VIsual;
	    }
	    else
	    {
		VIsual_mode = curbuf->b_visual_mode;
		tpos = curbuf->b_visual_end;
		curwin->w_cursor = curbuf->b_visual_start;
	    }

	    VIsual_active = TRUE;
	    VIsual_reselect = TRUE;

	    /* Set Visual to the start and w_cursor to the end of the Visual
	     * area.  Make sure they are on an existing character. */
	    adjust_cursor();
	    VIsual = curwin->w_cursor;
	    curwin->w_cursor = tpos;
	    adjust_cursor();
	    update_topline();
	    /*
	     * When called from normal "g" command: start Select mode when
	     * 'selectmode' contains "cmd".  When called for K_SELECT, always
	     * start Select mode
	     */
	    if (searchp == NULL)
		VIsual_select = TRUE;
	    else
		may_start_select('c');
#ifdef USE_MOUSE
	    setmouse();
#endif
#ifdef USE_CLIPBOARD
	    /* Make sure the clipboard gets updated.  Needed because start and
	     * end are still the same, and the selection needs to be owned */
	    clipboard.vmode = NUL;
#endif
	    update_curbuf(NOT_VALID);
	    showmode();
	}
	break;
    /*
     * "gV": Don't reselect the previous Visual area after a Select mode
     *	     mapping of menu.
     */
    case 'V':
	VIsual_reselect = FALSE;
	break;

    /*
     * "gh":  start Select mode.
     * "gH":  start Select line mode.
     * "g^H": start Select block mode.
     */
    case K_BS:
	cap->nchar = Ctrl('H');
	/* FALLTHROUGH */
    case 'h':
    case 'H':
    case Ctrl('H'):
	if (!checkclearop(oap))
	{
	    cap->cmdchar = cap->nchar + ('v' - 'h');
	    nv_visual(cap, TRUE);
	}
	break;

    /*
     * "gj" and "gk" two new funny movement keys -- up and down
     * movement based on *screen* line rather than *file* line.
     */
    case 'j':
    case K_DOWN:
	/* with 'nowrap' it works just like the normal "j" command */
	if (!curwin->w_p_wrap)
	{
	    oap->motion_type = MLINE;
	    i = cursor_down(cap->count1, oap->op_type == OP_NOP);
	}
	else
	    i = nv_screengo(oap, FORWARD, cap->count1);
	if (i == FAIL)
	    clearopbeep(oap);
	break;

    case 'k':
    case K_UP:
	/* with 'nowrap' it works just like the normal "k" command */
	if (!curwin->w_p_wrap)
	{
	    oap->motion_type = MLINE;
	    i = cursor_up(cap->count1, oap->op_type == OP_NOP);
	}
	else
	    i = nv_screengo(oap, BACKWARD, cap->count1);
	if (i == FAIL)
	    clearopbeep(oap);
	break;

    /*
     * "gJ": join two lines without inserting a space.
     */
    case 'J':
	nv_join(cap);
	break;

    /*
     * "g0", "g^" and "g$": Like "0", "^" and "$" but for screen lines.
     * "gm": middle of "g0" and "g$".
     */
    case '^':
	flag = TRUE;
	/* FALLTHROUGH */

    case '0':
    case 'm':
    case K_HOME:
    case K_KHOME:
    case K_XHOME:
	oap->motion_type = MCHAR;
	oap->inclusive = FALSE;
	if (curwin->w_p_wrap)
	{
	    validate_virtcol();
	    i = ((curwin->w_virtcol + (curwin->w_p_nu ? 8 : 0)) /
							   Columns) * Columns;
	    if (curwin->w_p_nu && i > 8)
		i -= 8;
	}
	else
	    i = curwin->w_leftcol;
	if (cap->nchar == 'm')
	    i += Columns / 2;
	coladvance((colnr_t)i);
	if (flag)
	{
	    do
		i = gchar_cursor();
	    while (vim_iswhite(i) && oneright() == OK);
	}
	curwin->w_set_curswant = TRUE;
	break;

    case '$':
    case K_END:
    case K_KEND:
    case K_XEND:
	oap->motion_type = MCHAR;
	oap->inclusive = TRUE;
	if (curwin->w_p_wrap)
	{
	    curwin->w_curswant = MAXCOL;    /* so we stay at the end */
	    if (cap->count1 == 1)
	    {
		validate_virtcol();
		i = ((curwin->w_virtcol + (curwin->w_p_nu ? 8 : 0)) /
						   Columns + 1) * Columns - 1;
		if (curwin->w_p_nu && i > 8)
		    i -= 8;
		coladvance((colnr_t)i);
	    }
	    else if (nv_screengo(oap, FORWARD, cap->count1 - 1) == FAIL)
		clearopbeep(oap);
	}
	else
	{
	    i = curwin->w_leftcol + Columns - 1;
	    if (curwin->w_p_nu)
		i -= 8;
	    coladvance((colnr_t)i);
	    curwin->w_set_curswant = TRUE;
	}
	break;

    /*
     * "g*" and "g#", like "*" and "#" but without using "\<" and "\>"
     */
    case '*':
    case '#':
	nv_ident(cap, searchp);
	break;

    /*
     * ge and gE: go back to end of word
     */
    case 'e':
    case 'E':
	oap->motion_type = MCHAR;
	curwin->w_set_curswant = TRUE;
	oap->inclusive = TRUE;
	if (bckend_word(cap->count1, cap->nchar == 'E', FALSE) == FAIL)
	    clearopbeep(oap);
	break;

    /*
     * "g CTRL-G": display info about cursor position
     */
    case Ctrl('G'):
	cursor_pos_info();
	break;

    /*
     * "gI": Start insert in column 1.
     */
    case 'I':
	beginline(0);
	if (!checkclearopq(oap))
	{
	    if (u_save_cursor() == OK)
	    {
		/* This is a new edit command, not a restart.  We don't edit
		 * recursively. */
		restart_edit = 0;
		command_busy = edit('g', FALSE, cap->count1);
	    }
	}
	break;

#ifdef FILE_IN_PATH
    /*
     * "gf": goto file, edit file under cursor
     * "]f" and "[f": can also be used.
     */
    case 'f':
	nv_gotofile(cap);
	break;
#endif

    /*
     * "gs": Goto sleep, but keep on checking for CTRL-C
     */
    case 's':
	out_flush();
	while (cap->count1-- && !got_int)
	{
	    ui_delay(1000L, TRUE);
	    ui_breakcheck();
	}
	break;

    /*
     * "ga": Display the ascii value of the character under the
     * cursor.	It is displayed in decimal, hex, and octal. -- webb
     */
    case 'a':
	do_ascii();
	break;

    /*
     * "gg": Goto the first line in file.  With a count it goes to
     * that line number like for "G". -- webb
     */
    case 'g':
	nv_goto(cap, (linenr_t)1);
	break;

    /*
     *	 Two-character operators:
     *	 "gq"	    Format text
     *	 "g~"	    Toggle the case of the text.
     *	 "gu"	    Change text to lower case.
     *	 "gU"	    Change text to upper case.
     *   "g?"	    rot13 encoding
     */
    case 'q':
    case '~':
    case 'u':
    case 'U':
    case '?':
	nv_operator(cap);
	break;

/*
 * "gd": Find first occurence of pattern under the cursor in the
 *	 current function
 * "gD": idem, but in the current file.
 */
    case 'd':
    case 'D':
	nv_gd(oap, cap->nchar);
	break;

#ifdef USE_MOUSE
    /*
     * g<*Mouse> : <C-*mouse>
     */
    case K_MIDDLEMOUSE:
    case K_MIDDLEDRAG:
    case K_MIDDLERELEASE:
    case K_LEFTMOUSE:
    case K_LEFTDRAG:
    case K_LEFTRELEASE:
    case K_RIGHTMOUSE:
    case K_RIGHTDRAG:
    case K_RIGHTRELEASE:
	mod_mask = MOD_MASK_CTRL;
	(void)do_mouse(oap, cap->nchar, BACKWARD, cap->count1, 0);
	break;

    case K_IGNORE:
	break;
#endif

    case Ctrl(']'):		/* :tag or :tselect for current identifier */
    case ']':			/* :tselect for current identifier */
	nv_ident(cap, searchp);
	break;

    /*
     * "gP" and "gp": same as "P" and "p" but leave cursor just after new text
     */
    case 'p':
    case 'P':
	nv_put(cap);
	break;

#ifdef BYTE_OFFSET
    /* "go": goto byte count from start of buffer */
    case 'o':
	goto_byte(cap->count0);
	break;
#endif

    default:
	clearopbeep(oap);
	break;
    }

    return command_busy;
}

/*
 * Handle "o" and "O" commands.
 */
    static int
n_opencmd(cap)
    CMDARG	*cap;
{
    int	    command_busy = FALSE;

    if (!checkclearopq(cap->oap))
    {
#ifdef COMMENTS
	if (has_format_option(FO_OPEN_COMS))
	    fo_do_comments = TRUE;
#endif
	if (u_save((linenr_t)(curwin->w_cursor.lnum -
					       (cap->cmdchar == 'O' ? 1 : 0)),
		   (linenr_t)(curwin->w_cursor.lnum +
					       (cap->cmdchar == 'o' ? 1 : 0))
		       ) == OK
		&& open_line(cap->cmdchar == 'O' ? BACKWARD : FORWARD,
							  TRUE, FALSE, 0))
	{
	    /* This is a new edit command, not a restart.  We don't edit
	     * recursively. */
	    restart_edit = 0;
	    command_busy = edit(cap->cmdchar, TRUE, cap->count1);
	}
#ifdef COMMENTS
	fo_do_comments = FALSE;
#endif
    }
    return command_busy;
}

/*
 * Handle "U" command.
 */
    static void
nv_Undo(cap)
    CMDARG	*cap;
{
    /* In Visual mode and typing "gUU" triggers an operator */
    if (VIsual_active || cap->oap->op_type == OP_UPPER)
    {
	/* translate "gUU" to "gUgU" */
	cap->cmdchar = 'g';
	cap->nchar = 'U';
	nv_operator(cap);
    }
    else if (!checkclearopq(cap->oap))
    {
	u_undoline();
	curwin->w_set_curswant = TRUE;
    }
}

/*
 * Handle an operator command.
 */
    static void
nv_operator(cap)
    CMDARG	*cap;
{
    int	    op_type;

    op_type = get_op_type(cap->cmdchar, cap->nchar);

    if (op_type == cap->oap->op_type)	    /* double operator works on lines */
	nv_lineop(cap);
    else if (!checkclearop(cap->oap))
    {
	cap->oap->start = curwin->w_cursor;
	cap->oap->op_type = op_type;
    }
}

/*
 * Handle linewise operator "dd", "yy", etc.
 */
    static void
nv_lineop(cap)
    CMDARG	*cap;
{
    cap->oap->motion_type = MLINE;
    if (cursor_down(cap->count1 - 1L, cap->oap->op_type == OP_NOP) == FAIL)
	clearopbeep(cap->oap);
    else if (  cap->oap->op_type == OP_DELETE
	    || cap->oap->op_type == OP_LSHIFT
	    || cap->oap->op_type == OP_RSHIFT)
	beginline(BL_SOL | BL_FIX);
    else if (cap->oap->op_type != OP_YANK)	/* 'Y' does not move cursor */
	beginline(BL_WHITE | BL_FIX);
}

/*
 * Handle "|" command.
 */
    static void
nv_pipe(cap)
    CMDARG *cap;
{
    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    beginline(0);
    if (cap->count0 > 0)
    {
	coladvance((colnr_t)(cap->count0 - 1));
	curwin->w_curswant = (colnr_t)(cap->count0 - 1);
    }
    else
	curwin->w_curswant = 0;
    /* keep curswant at the column where we wanted to go, not where
       we ended; differs is line is too short */
    curwin->w_set_curswant = FALSE;
}

/*
 * Handle back-word command "b".
 */
    static void
nv_bck_word(cap, type)
    CMDARG	*cap;
    int		type;
{
    cap->oap->motion_type = MCHAR;
    cap->oap->inclusive = FALSE;
    curwin->w_set_curswant = TRUE;
    if (bck_word(cap->count1, type, FALSE) == FAIL)
	clearopbeep(cap->oap);
}

/*
 * Handle word motion commands "e", "E", "w" and "W".
 */
    static void
nv_wordcmd(cap, type)
    CMDARG	*cap;
    int		type;
{
    int	    n;
    int	    word_end;
    int	    flag = FALSE;

    /*
     * Inclusive has been set for the "E" and "e" command.
     */
    word_end = cap->oap->inclusive;

    /*
     * "cw" and "cW" are a special case.
     */
    if (!word_end && cap->oap->op_type == OP_CHANGE)
    {
	n = gchar_cursor();
	if (n != NUL)			/* not an empty line */
	{
	    if (vim_iswhite(n))
	    {
		/*
		 * Reproduce a funny Vi behaviour: "cw" on a blank only
		 * changes one character, not all blanks until the start of
		 * the next word.  Only do this when the 'w' flag is included
		 * in 'cpoptions'.
		 */
		if (cap->count1 == 1 && vim_strchr(p_cpo, CPO_CW) != NULL)
		{
		    cap->oap->inclusive = TRUE;
		    cap->oap->motion_type = MCHAR;
		    return;
		}
	    }
	    else
	    {
		/*
		 * This is a little strange. To match what the real Vi does,
		 * we effectively map 'cw' to 'ce', and 'cW' to 'cE', provided
		 * that we are not on a space or a TAB.  This seems impolite
		 * at first, but it's really more what we mean when we say
		 * 'cw'.
		 * Another strangeness: When standing on the end of a word
		 * "ce" will change until the end of the next wordt, but "cw"
		 * will change only one character! This is done by setting
		 * flag.
		 */
		cap->oap->inclusive = TRUE;
		word_end = TRUE;
		flag = TRUE;
	    }
	}
    }

    cap->oap->motion_type = MCHAR;
    curwin->w_set_curswant = TRUE;
    if (word_end)
	n = end_word(cap->count1, type, flag, FALSE);
    else
	n = fwd_word(cap->count1, type, cap->oap->op_type != OP_NOP);

    /* Don't leave the cursor on the NUL past a line */
    if (curwin->w_cursor.col && gchar_cursor() == NUL)
    {
	--curwin->w_cursor.col;
	cap->oap->inclusive = TRUE;
    }

    if (n == FAIL && cap->oap->op_type == OP_NOP)
	clearopbeep(cap->oap);
    else
	adjust_for_sel(cap);
}

/*
 * In exclusive Visual mode, may include the last character.
 */
    static void
adjust_for_sel(cap)
    CMDARG	*cap;
{
    if (VIsual_active && cap->oap->inclusive && *p_sel == 'e'
						     && gchar_cursor() != NUL)
    {
	++curwin->w_cursor.col;
	cap->oap->inclusive = FALSE;
    }
}

/*
 * Exclude last character at end of Visual area for 'selection' == "exclusive".
 * Should check VIsual_mode before calling this.
 */
    static void
unadjust_for_sel()
{
    FPOS	*pp;

    if (*p_sel == 'e' && !equal(VIsual, curwin->w_cursor))
    {
	if (lt(VIsual, curwin->w_cursor))
	    pp = &curwin->w_cursor;
	else
	    pp = &VIsual;
	if (pp->col > 0)
	    --pp->col;
	else if (pp->lnum > 1)
	{
	    --pp->lnum;
	    pp->col = STRLEN(ml_get(pp->lnum));
	}
    }
}

/*
 * "G", "gg", CTRL-END, CTRL-HOME.
 */
    static void
nv_goto(cap, lnum)
    CMDARG	*cap;
    linenr_t	lnum;
{
    cap->oap->motion_type = MLINE;
    setpcmark();

    /* When a count is given, use it instead of the default lnum */
    if (cap->count0 != 0)
	lnum = cap->count0;
    if (lnum < 1L)
	lnum = 1L;
    else if (lnum > curbuf->b_ml.ml_line_count)
	lnum = curbuf->b_ml.ml_line_count;
    curwin->w_cursor.lnum = lnum;
    beginline(BL_SOL | BL_FIX);
}

/*
 * SELECT key in Normal or Visual mode: end of Select mode mapping.
 */
    static void
nv_select(cap)
    CMDARG	*cap;
{
    FPOS	*pp;

    if (VIsual_active)
	VIsual_select = TRUE;
    else if (VIsual_reselect)
    {
	cap->nchar = 'v';	    /* fake "gv" command */
	nv_g_cmd(cap, NULL);
	if (*p_sel == 'e' && VIsual_mode == 'v')
	{
	    /* exclusive mode: advance the end one character */
	    if (lt(VIsual, curwin->w_cursor))
		pp = &curwin->w_cursor;
	    else
		pp = &VIsual;
	    if (*ml_get_pos(pp) != NUL)
		++pp->col;
	    else if (pp->lnum < curbuf->b_ml.ml_line_count)
	    {
		++pp->lnum;
		pp->col = 0;
	    }
	    curwin->w_set_curswant = TRUE;
	    update_curbuf(NOT_VALID);
	}
    }
}

/*
 * CTRL-\ in Normal mode.
 */
    static void
nv_normal(cap)
    CMDARG	*cap;
{
    if (safe_vgetc() == Ctrl('N'))
    {
	clearop(cap->oap);
	if (VIsual_active)
	{
	    end_visual_mode();		/* stop Visual */
	    redraw_curbuf_later(NOT_VALID);
	}
    }
    else
	clearopbeep(cap->oap);
}

/*
 * ESC in Normal mode: beep, but don't flush buffers.
 * Don't even beep if we are canceling a command.
 */
    static void
nv_esc(cap, opnum)
    CMDARG	*cap;
    linenr_t	opnum;
{
    if (VIsual_active)
    {
	end_visual_mode();	/* stop Visual */
	check_cursor_col();	/* make sure cursor is not beyond EOL */
	curwin->w_set_curswant = TRUE;
	update_curbuf(NOT_VALID);
    }
    else if (cap->oap->op_type == OP_NOP && opnum == 0
		       && cap->count0 == 0 && cap->oap->regname == 0 && !p_im)
	vim_beep();
    clearop(cap->oap);
    if (p_im && !restart_edit)
	restart_edit = 'a';
}

/*
 * Handle "A", "a", "I", "i" and <Insert> commands.
 * Returns command_busy flag.
 */
    static int
nv_edit(cap)
    CMDARG *cap;
{
    /* <Insert> is equal to "i" */
    if (cap->cmdchar == K_INS || cap->cmdchar == K_KINS)
	cap->cmdchar = 'i';

    /* in Visual mode "A" and "I" are an operator */
    if ((cap->cmdchar == 'A' || cap->cmdchar == 'I')
	    && VIsual_active)
    {
	v_visop(cap);
    }
    /* in Visual mode and after an operator "a" and "i" are for text objects */
    else if ((cap->cmdchar == 'a' || cap->cmdchar == 'i')
	    && (cap->oap->op_type != OP_NOP || VIsual_active))
    {
#ifdef TEXT_OBJECTS
	nv_object(cap);
#else
	clearopbeep(cap->oap);
#endif
    }
    else if (!checkclearopq(cap->oap) && u_save_cursor() == OK)
    {
	switch (cap->cmdchar)
	{
	    case 'A':	/* "A"ppend after the line */
		curwin->w_set_curswant = TRUE;
		curwin->w_cursor.col += STRLEN(ml_get_cursor());
		break;

	    case 'I':	/* "I"nsert before the first non-blank */
		beginline(BL_WHITE);
		break;

	    case 'a':	/* "a"ppend is like "i"nsert on the next character. */
		if (*ml_get_cursor() != NUL)
		    inc_cursor();
		break;
	}
	/* This is a new edit command, not a restart.  We don't edit
	 * recursively. */
	restart_edit = 0;
	return edit(cap->cmdchar, FALSE, cap->count1);
    }
    return FALSE;
}

#ifdef TEXT_OBJECTS
/*
 * "a" or "i" while an operator is pending or in Visual mode: object motion.
 */
    static void
nv_object(cap)
    CMDARG	*cap;
{
    int		flag;
    int		include;
    char_u	*mps_save;

    if (cap->cmdchar == 'i')
	include = FALSE;    /* "ix" = inner object: exclude white space */
    else
	include = TRUE;	    /* "ax" = an object: include white space */

    /* Make sure (), [], {} and <> are in 'matchpairs' */
    mps_save = curbuf->b_p_mps;
    curbuf->b_p_mps = (char_u *)"(:),{:},[:],<:>";

    switch (cap->nchar)
    {
	case 'w': /* "aw" = a word */
		flag = current_word(cap->oap, cap->count1, include, FALSE);
		break;
	case 'W': /* "aW" = a WORD */
		flag = current_word(cap->oap, cap->count1, include, TRUE);
		break;
	case 'b': /* "ab" = a braces block */
	case '(':
	case ')':
		flag = current_block(cap->oap, cap->count1, include, '(', ')');
		break;
	case 'B': /* "aB" = a Brackets block */
	case '{':
	case '}':
		flag = current_block(cap->oap, cap->count1, include, '{', '}');
		break;
	case '[': /* "a[" = a [] block */
	case ']':
		flag = current_block(cap->oap, cap->count1, include, '[', ']');
		break;
	case '<': /* "a<" = a <> block */
	case '>':
		flag = current_block(cap->oap, cap->count1, include, '<', '>');
		break;
	case 'p': /* "ap" = a paragraph */
		flag = current_par(cap->oap, cap->count1, include, 'p');
		break;
	case 's': /* "as" = a sentence */
		flag = current_sent(cap->oap, cap->count1, include);
		break;
#if 0	/* TODO */
	case 'S': /* "aS" = a section */
	case 'f': /* "af" = a filename */
	case 'u': /* "au" = a URL */
#endif
	default:
		flag = FAIL;
		break;
    }

    curbuf->b_p_mps = mps_save;
    if (flag == FAIL)
	clearopbeep(cap->oap);
    adjust_cursor_col();
    curwin->w_set_curswant = TRUE;
}
#endif

/*
 * Handle the "q" key.
 */
    static void
nv_q(cap)
    CMDARG	*cap;
{
    if (cap->oap->op_type == OP_FORMAT)
    {
	/* "gqq" is the same as "gqgq": format line */
	cap->cmdchar = 'g';
	cap->nchar = 'q';
	nv_operator(cap);
    }
    else if (!checkclearop(cap->oap))
    {
	/* (stop) recording into a named register */
	/* command is ignored while executing a register */
	if (!Exec_reg && do_record(cap->nchar) == FAIL)
	    clearopbeep(cap->oap);
    }
}

/*
 * Handle the "@r" command.
 */
    static void
nv_at(cap)
    CMDARG	*cap;
{
    if (checkclearop(cap->oap))
	return;
#ifdef WANT_EVAL
    if (cap->nchar == '=')
    {
	if (get_expr_register() == NUL)
	    return;
    }
#endif
    while (cap->count1-- && !got_int)
    {
	if (do_execreg(cap->nchar, FALSE, FALSE) == FAIL)
	{
	    clearopbeep(cap->oap);
	    break;
	}
	line_breakcheck();
    }
}

/*
 * Handle the CTRL-U and CTRL-D commands.
 */
    static void
nv_halfpage(cap)
    CMDARG	*cap;
{
    if ((cap->cmdchar == Ctrl('U') && curwin->w_cursor.lnum == 1)
	    || (cap->cmdchar == Ctrl('D')
		&& curwin->w_cursor.lnum == curbuf->b_ml.ml_line_count))
	clearopbeep(cap->oap);
    else if (!checkclearop(cap->oap))
	halfpage(cap->cmdchar == Ctrl('D'), cap->count0);
}

/*
 * Handle "J" or "gJ" command.
 */
    static void
nv_join(cap)
    CMDARG *cap;
{
    if (VIsual_active)	/* join the visual lines */
	nv_operator(cap);
    else if (!checkclearop(cap->oap))
    {
	if (cap->count0 <= 1)
	    cap->count0 = 2;	    /* default for join is two lines! */
	if (curwin->w_cursor.lnum + cap->count0 - 1 >
						   curbuf->b_ml.ml_line_count)
	    clearopbeep(cap->oap);  /* beyond last line */
	else
	{
	    prep_redo(cap->oap->regname, cap->count0,
			 NUL, cap->cmdchar, NUL, cap->nchar);
	    do_do_join(cap->count0, cap->nchar == NUL, TRUE);
	}
    }
}

/*
 * "P", "gP", "p" and "gp" commands.
 */
    static void
nv_put(cap)
    CMDARG  *cap;
{
    if (cap->oap->op_type != OP_NOP || VIsual_active)
	clearopbeep(cap->oap);
    else
    {
	prep_redo_cmd(cap);
	do_put(cap->oap->regname,
		(cap->cmdchar == 'P'
		 || (cap->cmdchar == 'g' && cap->nchar == 'P'))
							 ? BACKWARD : FORWARD,
		cap->count1, cap->cmdchar == 'g' ? PUT_CURSEND : 0);
    }
}
