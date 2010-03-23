/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * edit.c: functions for Insert mode
 */

#include "vim.h"

#ifdef INSERT_EXPAND
/*
 * definitions used for CTRL-X submode
 */
#define CTRL_X_WANT_IDENT	0x100

#define CTRL_X_NOT_DEFINED_YET	1
#define CTRL_X_SCROLL		2
#define CTRL_X_WHOLE_LINE	3
#define CTRL_X_FILES		4
#define CTRL_X_TAGS		(5 + CTRL_X_WANT_IDENT)
#define CTRL_X_PATH_PATTERNS	(6 + CTRL_X_WANT_IDENT)
#define CTRL_X_PATH_DEFINES	(7 + CTRL_X_WANT_IDENT)
#define CTRL_X_FINISHED		8
#define CTRL_X_DICTIONARY	(9 + CTRL_X_WANT_IDENT)

#define C_X_SKIP		7 /* length of " Adding" */

#define CHECK_KEYS_TIME		30

static char *ctrl_x_msgs[] =
{
    " Adding Keyword completion (^N/^P)", /* ctrl_x_mode == 0, ^P/^N compl. */
    " ^X mode (^E/^Y/^L/^]/^F/^I/^K/^D/^N/^P)",
    /* Scroll has it's own msgs, in it's place there is the msg for local
     * ctrl_x_mode = 0 (eg continue_status & CONT_LOCAL)  -- Acevedo */
    " Adding Keyword Local completion (^N/^P)",
    " Adding Whole line completion (^L/^N/^P)",
    " Adding File name completion (^F/^N/^P)",
    " Adding Tag completion (^]/^N/^P)",
    " Adding Path pattern completion (^N/^P)",
    " Adding Definition completion (^D/^N/^P)",
    NULL,
    " Adding Dictionary completion (^K/^N/^P)"
};

static char_u e_hitend[] = "Hit end of paragraph";

/*
 * Structure used to store one match for insert completion.
 */
struct Completion
{
    struct Completion	*next;
    struct Completion	*prev;
    char_u		*str;	  /* matched text */
    char_u		*fname;	  /* file containing the match */
    int			original; /* ORIGINAL_TEXT, CONT_S_IPOS or FREE_FNAME */
};

/* the original text when the expansion begun */
#define ORIGINAL_TEXT	(1)
#define FREE_FNAME	(2)

/*
 * All the current matches are stored in a list.
 * "first_match" points to the start of the list.
 * "curr_match" points to the currently selected entry.
 * "shown_match" is different from curr_match during ins_compl_get_exp().
 */
static struct Completion    *first_match = NULL;
static struct Completion    *curr_match = NULL;
static struct Completion    *shown_match = NULL;
static int		    started_completion = FALSE;
static char_u		    *complete_pat = NULL;
static int		    complete_direction = FORWARD;
static int		    shown_direction = FORWARD;
static int		    completion_pending = FALSE;
static FPOS		    initial_pos;
static colnr_t		    complete_col = 0;
static int		    save_sm;
static char_u		    *original_text = NULL;  /* text before completion */
static int		    continue_mode = 0;

static int  ins_compl_add __ARGS((char_u *str, int len, char_u *, int dir, int reuse));
static int  ins_compl_make_cyclic __ARGS((void));
static void ins_compl_dictionaries __ARGS((char_u *dict, char_u *pat, int dir, int flags));
static void ins_compl_free __ARGS((void));
static void ins_compl_clear __ARGS((void));
static int  ins_compl_prep __ARGS((int c));
static BUF  *ins_compl_next_buf __ARGS((BUF *buf, int flag));
static int  ins_compl_get_exp __ARGS((FPOS *ini, int dir));
static void ins_compl_delete __ARGS((void));
static void ins_compl_insert __ARGS((void));
static int  ins_compl_next __ARGS((int allow_get_expansion));
static int  ins_complete __ARGS((int c));
static int  quote_meta __ARGS((char_u *dest, char_u *str, int len));
#endif /* INSERT_EXPAND */

#define BACKSPACE_CHAR		    1
#define BACKSPACE_WORD		    2
#define BACKSPACE_WORD_NOT_SPACE    3
#define BACKSPACE_LINE		    4

static void edit_putchar __ARGS((int c, int highlight));
static void undisplay_dollar __ARGS((void));
static void insert_special __ARGS((int, int, int));
static void redo_literal __ARGS((int c));
static void start_arrow __ARGS((FPOS *end_insert_pos));
static void stop_insert __ARGS((FPOS *end_insert_pos));
static int  echeck_abbr __ARGS((int));
static void replace_push_off __ARGS((int c));
static int  replace_pop __ARGS((void));
static void replace_join __ARGS((int off));
static void replace_pop_ins __ARGS((void));
static void replace_flush __ARGS((void));
static void replace_do_bs __ARGS((void));
static int  ins_reg __ARGS((void));
static int  ins_esc __ARGS((long *count, int need_redraw, int cmdchar));
#ifdef RIGHTLEFT
static void ins_ctrl_ __ARGS((void));
#endif
static void ins_shift __ARGS((int c, int lastc));
static void ins_del __ARGS((void));
static int  ins_bs __ARGS((int c, int mode, int *inserted_space_p));
#ifdef USE_MOUSE
static void ins_mouse __ARGS((int c));
static void ins_mousescroll __ARGS((int up));
#endif
static void ins_left __ARGS((void));
static void ins_home __ARGS((void));
static void ins_end __ARGS((void));
static void ins_s_left __ARGS((void));
static void ins_right __ARGS((void));
static void ins_s_right __ARGS((void));
static void ins_up __ARGS((void));
static void ins_pageup __ARGS((void));
static void ins_down __ARGS((void));
static void ins_pagedown __ARGS((void));
static int  ins_tab __ARGS((void));
static int  ins_eol __ARGS((int c));
#ifdef DIGRAPHS
static int  ins_digraph __ARGS((void));
#endif
static int  ins_copychar __ARGS((linenr_t lnum));
#ifdef SMARTINDENT
static void ins_try_si __ARGS((int c));
#endif
static colnr_t get_nolist_virtcol __ARGS((void));

static colnr_t	Insstart_textlen;	/* length of line when insert started */
static colnr_t	Insstart_blank_vcol;	/* vcol for first inserted blank */

static char_u	*last_insert = NULL;	/* the text of the previous insert */
static int	last_insert_skip; /* nr of chars in front of previous insert */
static int	new_insert_skip;  /* nr of chars in front of current insert */

#ifdef CINDENT
static int	can_cindent;		/* may do cindenting on this line */
#endif

static int	old_indent = 0;		/* for ^^D command in insert mode */

#ifdef RIGHTLEFT
int	    revins_on;		    /* reverse insert mode on */
int	    revins_chars;	    /* how much to skip after edit */
int	    revins_legal;	    /* was the last char 'legal'? */
int	    revins_scol;	    /* start column of revins session */
#endif

#if defined(MULTI_BYTE) && defined(macintosh)
static short	previous_script = smRoman;
#endif

/*
 * edit(): Start inserting text.
 *
 * "cmdchar" can be:
 * 'i'	normal insert command
 * 'a'	normal append command
 * 'R'	replace command
 * 'r'	"r<CR>" command: insert one <CR>.  Note: count can be > 1, for redo,
 *	but still only one <CR> is inserted.  The <Esc> is not used for redo.
 * 'g'	"gI" command.
 * 'V'	"gR" command for Virtual Replace mode.
 * 'v'	"gr" command for single character Virtual Replace mode.
 *
 * This function is not called recursively.  For CTRL-O commands, it returns
 * and lets the caller handle the Normal-mode command.
 *
 * Return TRUE if a CTRL-O command caused the return (insert mode pending).
 */
    int
edit(cmdchar, startln, count)
    int		cmdchar;
    int		startln;	/* if set, insert at start of line */
    long	count;
{
    int		c = 0;
    char_u	*ptr;
    int		lastc;
    colnr_t	mincol;
    static linenr_t o_lnum = 0;
    static int	o_eol = FALSE;
    int		need_redraw = FALSE;
    int		i;
    int		did_backspace = TRUE;	    /* previous char was backspace */
#ifdef CINDENT
    int		line_is_white = FALSE;	    /* line is empty before insert */
#endif
    linenr_t	old_topline = 0;	    /* topline before insertion */
    int		inserted_space = FALSE;     /* just inserted a space */
    int		has_startsel;		    /* may start selection */
    int		replaceState = REPLACE;
    int		did_restart_edit = restart_edit;

    /* sleep before redrawing, needed for "CTRL-O :" that results in an
     * error message */
    check_for_delay(TRUE);

#ifdef INSERT_EXPAND
    ins_compl_clear();	    /* clear stuff for ctrl-x mode */
#endif

#ifdef USE_MOUSE
    /*
     * When doing a paste with the middle mouse button, Insstart is set to
     * where the paste started.
     */
    if (where_paste_started.lnum != 0)
	Insstart = where_paste_started;
    else
#endif
    {
	Insstart = curwin->w_cursor;
	if (startln)
	    Insstart.col = 0;
    }
    Insstart_textlen = linetabsize(ml_get_curline());
    Insstart_blank_vcol = MAXCOL;
    if (!did_ai)
	ai_col = 0;

    if (cmdchar != NUL && !restart_edit)
    {
	ResetRedobuff();
	AppendNumberToRedobuff(count);
	if (cmdchar == 'V' || cmdchar == 'v')
	{
	    /* "gR" or "gr" command */
	    AppendCharToRedobuff('g');
	    AppendCharToRedobuff((cmdchar == 'v') ? 'r' : 'R');
	}
	else
	{
	    AppendCharToRedobuff(cmdchar);
	    if (cmdchar == 'g')		    /* "gI" command */
		AppendCharToRedobuff('I');
	    else if (cmdchar == 'r')	    /* "r<CR>" command */
		count = 1;		    /* insert only one <CR> */
	}
    }

    if (cmdchar == 'R')
    {
#ifdef FKMAP
	if (p_fkmap && p_ri)
	{
	    beep_flush();
	    EMSG(farsi_text_3);	    /* encoded in Farsi */
	    State = INSERT;
	}
	else
#endif
	State = REPLACE;
    }
    else if (cmdchar == 'V' || cmdchar == 'v')
    {
	State = VREPLACE;
	replaceState = VREPLACE;
	orig_line_count = curbuf->b_ml.ml_line_count;
	vr_lines_changed = 1;
	vr_virtcol = MAXCOL;
    }
    else
	State = INSERT;
#if defined(USE_GUI_WIN32) && defined(MULTI_BYTE_IME)
    ImeSetOriginMode();
#endif
#if defined(MULTI_BYTE) && defined(macintosh)
    KeyScript(previous_script);
#endif

    /*
     * Need to recompute the cursor position, it might move when the cursor is
     * on a TAB or special character.
     */
    curs_columns(TRUE);

#ifdef USE_MOUSE
    setmouse();
#endif
#ifdef CMDLINE_INFO
    clear_showcmd();
#endif
#ifdef RIGHTLEFT
    /* there is no reverse replace mode */
    revins_on = (State == INSERT && p_ri);
    if (revins_on)
	undisplay_dollar();
    revins_chars = 0;
    revins_legal = 0;
    revins_scol = -1;
#endif

    /* if 'keymodel' contains "startsel", may start selection on shifted
     * special key */
    has_startsel = (vim_strchr(p_km, 'a') != NULL);

    /*
     * Handle restarting Insert mode.
     * Don't do this for CTRL-O . (repeat an insert): we get here with
     * restart_edit non-zero, and something in the stuff buffer.
     */
    if (restart_edit && stuff_empty())
    {
#ifdef USE_MOUSE
	/*
	 * After a paste we consider text typed to be part of the insert for
	 * the pasted text. You can backspace over the pasted text too.
	 */
	if (where_paste_started.lnum)
	    arrow_used = FALSE;
	else
#endif
	    arrow_used = TRUE;
	restart_edit = 0;

	/*
	 * If the cursor was after the end-of-line before the CTRL-O and it is
	 * now at the end-of-line, put it after the end-of-line (this is not
	 * correct in very rare cases).
	 * Also do this if curswant is greater than the current virtual
	 * column.  Eg after "^O$" or "^O80|".
	 */
	validate_virtcol();
	update_curswant();
	if (	   ((o_eol && curwin->w_cursor.lnum == o_lnum)
		    || curwin->w_curswant > curwin->w_virtcol)
		&& *(ptr = ml_get_curline() + curwin->w_cursor.col) != NUL)
	{
	    if (ptr[1] == NUL)
		++curwin->w_cursor.col;
#ifdef MULTI_BYTE
	    else if (is_dbcs && ptr[2] == NUL && IsLeadByte(ptr[0])
		    && IsTrailByte(ptr - curwin->w_cursor.col, ptr + 1))
		curwin->w_cursor.col += 2;
#endif
	}
    }
    else
    {
	arrow_used = FALSE;
	o_eol = FALSE;
    }
#ifdef USE_MOUSE
    where_paste_started.lnum = 0;
#endif
#ifdef CINDENT
    can_cindent = TRUE;
#endif

    /*
     * If 'showmode' is set, show the current (insert/replace/..) mode.
     * A warning message for changing a readonly file is given here, before
     * actually changing anything.  It's put after the mode, if any.
     */
    i = 0;
    if (p_smd)
	i = showmode();

    if (!p_im && !did_restart_edit)
	change_warning(i + 1);

#ifdef CURSOR_SHAPE
    ui_cursor_shape();		/* may show different cursor shape */
#endif
#ifdef DIGRAPHS
    do_digraph(-1);		/* clear digraphs */
#endif

/*
 * Get the current length of the redo buffer, those characters have to be
 * skipped if we want to get to the inserted characters.
 */
    ptr = get_inserted();
    if (ptr == NULL)
	new_insert_skip = 0;
    else
    {
	new_insert_skip = STRLEN(ptr);
	vim_free(ptr);
    }

    old_indent = 0;

    for (;;)
    {
#ifdef RIGHTLEFT
	if (!revins_legal)
	    revins_scol = -1;	    /* reset on illegal motions */
	else
	    revins_legal = 0;
#endif
	if (arrow_used)	    /* don't repeat insert when arrow key used */
	    count = 0;

	    /* set curwin->w_curswant for next K_DOWN or K_UP */
	if (!arrow_used)
	    curwin->w_set_curswant = TRUE;

	/*
	 * When emsg() was called msg_scroll will have been set.
	 */
	msg_scroll = FALSE;

	/*
	 * If we inserted a character at the last position of the last line in
	 * the window, scroll the window one line up. This avoids an extra
	 * redraw.
	 * This is detected when the cursor column is smaller after inserting
	 * something.
	 * Don't do this when the topline changed already, it has
	 * already been adjusted (by insertchar() calling open_line())).
	 */
	if (need_redraw && curwin->w_p_wrap && !did_backspace &&
					     curwin->w_topline == old_topline)
	{
	    mincol = curwin->w_wcol;
	    validate_cursor_col();

	    if ((int)curwin->w_wcol < (int)mincol - curbuf->b_p_ts
		    && curwin->w_wrow == curwin->w_winpos +
						curwin->w_height - 1 - p_so
		    && curwin->w_cursor.lnum != curwin->w_topline)
	    {
		set_topline(curwin, curwin->w_topline + 1);
		update_topline();
#ifdef SYNTAX_HL
		/* recompute syntax hl., starting with current line */
		syn_changed(curwin->w_cursor.lnum);
#endif
		update_screen(VALID_TO_CURSCHAR);
		need_redraw = FALSE;
	    }
	    else
		update_topline();
	}
	else
	    update_topline();
	did_backspace = FALSE;

	/*
	 * redraw is postponed until here to make 'dollar' option work
	 * correctly.
	 */
	validate_cursor();		/* may set must_redraw */
	if (need_redraw || must_redraw)
	{
	    update_screenline();
	    if (curwin->w_redr_status == TRUE)
		win_redr_status(curwin);	/* display [+] if required */
	    need_redraw = FALSE;
	}
	else if (clear_cmdline || redraw_cmdline)
	    showmode();		    /* clear cmdline, show mode and ruler */

#ifdef SCROLLBIND
	if (curwin->w_p_scb)
	    do_check_scrollbind(TRUE);
#endif

	showruler(FALSE);
	setcursor();
	update_curswant();
	emsg_on_display = FALSE;	/* may remove error message now */
	old_topline = curwin->w_topline;

#ifdef USE_ON_FLY_SCROLL
	dont_scroll = FALSE;		/* allow scrolling here */
#endif
	lastc = c;			/* remember previous char for CTRL-D */
	c = safe_vgetc();

#ifdef RIGHTLEFT
	if (p_hkmap && KeyTyped)
	    c = hkmap(c);		/* Hebrew mode mapping */
#endif
#ifdef FKMAP
	if (p_fkmap && KeyTyped)
	    c = fkmap(c);		/* Farsi mode mapping */
#endif

#ifdef INSERT_EXPAND
	/*
	 * Prepare for or stop ctrl-x mode.
	 */
	need_redraw |= ins_compl_prep(c);
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
		count = 0;
		goto doESCkey;
	    }
	}

#ifdef DIGRAPHS
	c = do_digraph(c);
#endif

	if (c == Ctrl('V') || c == Ctrl('Q'))
	{
	    if (redrawing() && !char_avail())
		edit_putchar('^', TRUE);
	    AppendToRedobuff((char_u *)"\026");	/* CTRL-V */

#ifdef CMDLINE_INFO
	    add_to_showcmd_c(c);
#endif

	    c = get_literal();
#ifdef CMDLINE_INFO
	    clear_showcmd();
#endif
	    insert_special(c, FALSE, TRUE);
	    need_redraw = TRUE;
#ifdef RIGHTLEFT
	    revins_chars++;
	    revins_legal++;
#endif
	    c = Ctrl('V');	/* pretend CTRL-V is last typed character */
	    continue;
	}
#ifdef MULTI_BYTE
# if defined(USE_GUI) && !defined(USE_GUI_MSWIN)
	if (!gui.in_use)
# endif
	if (is_dbcs && IsLeadByte(c))
	{
	    int c2;

	    c2 = safe_vgetc();
	    insert_special(c, FALSE, FALSE);
	    insert_special(c2, FALSE, FALSE);
	    need_redraw = TRUE;
	    continue;
	}
#endif

#ifdef CINDENT
	if (curbuf->b_p_cin
# ifdef INSERT_EXPAND
			    && !ctrl_x_mode
# endif
					       )
	{
	    line_is_white = inindent(0);

	    /*
	     * A key name preceded by a bang means that this
	     * key wasn't destined to be inserted.  Skip ahead
	     * to the re-indenting if we find one.
	     */
	    if (in_cinkeys(c, '!', line_is_white))
		goto force_cindent;

	    /*
	     * A key name preceded by a star means that indenting
	     * has to be done before inserting the key.
	     */
	    if (can_cindent && in_cinkeys(c, '*', line_is_white))
	    {
		stop_arrow();

		/* re-indent the current line */
		fixthisline(get_c_indent);

		/* draw the changes on the screen later */
		need_redraw = TRUE;
	    }
	}
#endif /* CINDENT */

#ifdef RIGHTLEFT
	if (curwin->w_p_rl)
	    switch (c)
	    {
		case K_LEFT:	c = K_RIGHT; break;
		case K_S_LEFT:	c = K_S_RIGHT; break;
		case K_RIGHT:	c = K_LEFT; break;
		case K_S_RIGHT: c = K_S_LEFT; break;
	    }
#endif

	/* if 'keymodel' contains "startsel", may start selection */
	if (has_startsel)
	    switch (c)
	    {
		case K_KHOME:
		case K_XHOME:
		case K_KEND:
		case K_XEND:
		case K_PAGEUP:
		case K_KPAGEUP:
		case K_PAGEDOWN:
		case K_KPAGEDOWN:
#ifdef macintosh
		case K_LEFT:  /* LEFT, RIGHT, UP, DOWN may be required on HP */
		case K_RIGHT:
		case K_UP:
		case K_DOWN:
		case K_END:
		case K_HOME:
#endif
		    if (!(mod_mask & MOD_MASK_SHIFT))
			break;
		    /* FALLTHROUGH */
		case K_S_LEFT:
		case K_S_RIGHT:
		case K_S_UP:
		case K_S_DOWN:
		case K_S_END:
		case K_S_HOME:
		    /* Start selection right away, the cursor can move with
		     * CTRL-O when beyond the end of the line. */
		    start_selection();

		    /* Execute the key in (insert) Select mode, unless it's
		     * shift-left and beyond the end of the line (the CTRL-O
		     * will move the cursor left already). */
		    stuffcharReadbuff(Ctrl('O'));
		    if (c != K_S_LEFT || gchar_cursor() != NUL)
		    {
			if (mod_mask)
			{
			    char_u	    buf[4];

			    buf[0] = K_SPECIAL;
			    buf[1] = KS_MODIFIER;
			    buf[2] = mod_mask;
			    buf[3] = NUL;
			    stuffReadbuff(buf);
			}
			stuffcharReadbuff(c);
		    }
		    continue;
	    }

	/*
	 * The big switch to handle a character in insert mode.
	 */
	switch (c)
	{
	case K_INS:	    /* toggle insert/replace mode */
	case K_KINS:
#ifdef FKMAP
	    if (p_fkmap && p_ri)
	    {
		beep_flush();
		EMSG(farsi_text_3);	/* encoded in Farsi */
		break;
	    }
#endif
	    if (State == replaceState)
		State = INSERT;
	    else
		State = replaceState;
	    AppendCharToRedobuff(K_INS);
	    showmode();
#ifdef CURSOR_SHAPE
	    ui_cursor_shape();		/* may show different cursor shape */
#endif
	    break;

#ifdef INSERT_EXPAND
	case Ctrl('X'):	    /* Enter ctrl-x mode */
	    /* if the next ^X<> won't ADD nothing, then reset continue_status */
	    continue_status = continue_status & CONT_N_ADDS
					 ?  continue_status | CONT_INTRPT : 0;
	    /* We're not sure which ctrl-x mode it will be yet */
	    ctrl_x_mode = CTRL_X_NOT_DEFINED_YET;
	    edit_submode = (char_u *)ctrl_x_msgs[ctrl_x_mode & 15];
	    showmode();
	    break;
#endif

	case K_SELECT:	    /* end of Select mode mapping - ignore */
	    break;

	case Ctrl('Z'):	    /* suspend when 'insertmode' set */
	    if (!p_im)
		goto normalchar;	/* insert CTRL-Z as normal char */
	    stuffReadbuff((char_u *)":st\r");
	    c = Ctrl('O');
	    /*FALLTHROUGH*/

	case Ctrl('O'):	    /* execute one command */
	    if (echeck_abbr(Ctrl('O') + ABBR_OFF))
		break;
	    count = 0;
	    if (State == INSERT)
		restart_edit = 'I';
	    else if (State == VREPLACE)
		restart_edit = 'V';
	    else
		restart_edit = 'R';
	    o_lnum = curwin->w_cursor.lnum;
	    o_eol = (gchar_cursor() == NUL);
	    goto doESCkey;

#ifdef USE_SNIFF
	case K_SNIFF:
	    stuffcharReadbuff(K_SNIFF);
	    goto doESCkey;
#endif

	  /* Hitting the help key in insert mode is like <ESC> <Help> */
	case K_HELP:
	case K_F1:
	case K_XF1:
	    stuffcharReadbuff(K_HELP);
	    if (p_im)
		stuffcharReadbuff('i');
	    goto doESCkey;

	case ESC:	    /* an escape ends input mode */
	    if (echeck_abbr(ESC + ABBR_OFF))
		break;
	    /*FALLTHROUGH*/

	case Ctrl('C'):
#ifdef UNIX
do_intr:
#endif
	    /* when 'insertmode' set, and not halfway a mapping, don't leave
	     * Insert mode */
	    if (goto_im())
	    {
		if (got_int)
		{
		    (void)vgetc();		/* flush all buffers */
		    got_int = FALSE;
		}
		else
		    vim_beep();
		break;
	    }
doESCkey:
	    /*
	     * This is the ONLY return from edit()!
	     */
	    if (ins_esc(&count, need_redraw, cmdchar))
		return (c == Ctrl('O'));
	    continue;

	    /*
	     * Insert the previously inserted text.
	     * For ^@ the trailing ESC will end the insert, unless there
	     * is an error.
	     */
	case K_ZERO:
	case NUL:
	case Ctrl('A'):
	    if (stuff_inserted(NUL, 1L, (c == Ctrl('A'))) == FAIL
						   && c != Ctrl('A') && !p_im)
		goto doESCkey;		/* quit insert mode */
	    inserted_space = FALSE;
	    break;

	/*
	 * insert the contents of a register
	 */
	case Ctrl('R'):
	    need_redraw |= ins_reg();
	    inserted_space = FALSE;
	    break;

#ifdef RIGHTLEFT
	case Ctrl('_'):
	    if (!p_ari)
		goto normalchar;
	    ins_ctrl_();
	    break;
#endif

	/* Make indent one shiftwidth smaller. */
	case Ctrl('D'):
#if defined(INSERT_EXPAND) && defined(FIND_IN_PATH)
	    if (ctrl_x_mode == CTRL_X_PATH_DEFINES)
		goto docomplete;
#endif
	    /* FALLTHROUGH */

	/* Make indent one shiftwidth greater. */
	case Ctrl('T'):
	    ins_shift(c, lastc);
	    need_redraw = TRUE;
	    inserted_space = FALSE;
	    break;

	/* delete character under the cursor */
	case K_DEL:
	case K_KDEL:
	    ins_del();
	    need_redraw = TRUE;
	    break;

	/* delete character before the cursor */
	case K_BS:
	case Ctrl('H'):
	    did_backspace = ins_bs(c, BACKSPACE_CHAR, &inserted_space);
	    need_redraw = TRUE;
	    break;

	/* delete word before the cursor */
	case Ctrl('W'):
	    did_backspace = ins_bs(c, BACKSPACE_WORD, &inserted_space);
	    need_redraw = TRUE;
	    break;

	/* delete all inserted text in current line */
	case Ctrl('U'):
	    did_backspace = ins_bs(c, BACKSPACE_LINE, &inserted_space);
	    need_redraw = TRUE;
	    inserted_space = FALSE;
	    break;

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
	    ins_mouse(c);
	    break;

	case K_IGNORE:
	    break;

	/* Default action for scroll wheel up: scroll up */
	case K_MOUSEDOWN:
	    ins_mousescroll(FALSE);
	    break;

	/* Default action for scroll wheel down: scroll down */
	case K_MOUSEUP:
	    ins_mousescroll(TRUE);
	    break;
#endif

#ifdef USE_GUI
	case K_SCROLLBAR:
	    ins_scroll();
	    break;

	case K_HORIZ_SCROLLBAR:
	    ins_horscroll();
	    break;
#endif

	case K_HOME:
	case K_KHOME:
	case K_XHOME:
	case K_S_HOME:
	    ins_home();
	    break;

	case K_END:
	case K_KEND:
	case K_XEND:
	case K_S_END:
	    ins_end();
	    break;

	case K_LEFT:
	    if (mod_mask & MOD_MASK_CTRL)
		ins_s_left();
	    else
		ins_left();
	    break;

	case K_S_LEFT:
	    ins_s_left();
	    break;

	case K_RIGHT:
	    if (mod_mask & MOD_MASK_CTRL)
		ins_s_right();
	    else
		ins_right();
	    break;

	case K_S_RIGHT:
	    ins_s_right();
	    break;

	case K_UP:
	    ins_up();
	    break;

	case K_S_UP:
	case K_PAGEUP:
	case K_KPAGEUP:
	    ins_pageup();
	    break;

	case K_DOWN:
	    ins_down();
	    break;

	case K_S_DOWN:
	case K_PAGEDOWN:
	case K_KPAGEDOWN:
	    ins_pagedown();
	    break;

	    /* keypad keys: When not mapped they produce a normal char */
	case K_KPLUS:		c = '+'; goto normalchar;
	case K_KMINUS:		c = '-'; goto normalchar;
	case K_KDIVIDE:		c = '/'; goto normalchar;
	case K_KMULTIPLY:	c = '*'; goto normalchar;

	    /* When <S-Tab> isn't mapped, use it like a normal TAB */
	case K_S_TAB:
	    c = TAB;
	    /* FALLTHROUGH */

	/* TAB or Complete patterns along path */
	case TAB:
#if defined(INSERT_EXPAND) && defined(FIND_IN_PATH)
	    if (ctrl_x_mode == CTRL_X_PATH_PATTERNS)
		goto docomplete;
#endif
	    inserted_space = FALSE;
	    if (ins_tab())
		goto normalchar;	/* insert TAB as a normal char */
	    need_redraw = TRUE;
	    break;

	case K_KENTER:
	    c = CR;
	    /* FALLTHROUGH */
	case CR:
	case NL:
	    if (ins_eol(c) && !p_im)
		goto doESCkey;	    /* out of memory */
	    inserted_space = FALSE;
	    break;

#if defined(DIGRAPHS) || defined (INSERT_EXPAND)
	case Ctrl('K'):
# ifdef INSERT_EXPAND
	    if (ctrl_x_mode == CTRL_X_DICTIONARY)
	    {
		if (*p_dict == NUL)
		{
		    ctrl_x_mode = 0;
		    msg_attr((char_u *)"'dictionary' option is empty",
			     hl_attr(HLF_E));
		    vim_beep();
		    setcursor();
		    out_flush();
		    ui_delay(2000L, FALSE);
		    break;
		}
		goto docomplete;
	    }
# endif
# ifdef DIGRAPHS
	    c = ins_digraph();
	    if (c == NUL)
	    {
		need_redraw = TRUE;
		break;
	    }
# endif
	    goto normalchar;
#endif /* DIGRAPHS || INSERT_EXPAND */

#ifdef INSERT_EXPAND
	case Ctrl(']'):		/* Tag name completion after ^X */
	    if (ctrl_x_mode != CTRL_X_TAGS)
		goto normalchar;
	    goto docomplete;

	case Ctrl('F'):		/* File name completion after ^X */
	    if (ctrl_x_mode != CTRL_X_FILES)
		goto normalchar;
	    goto docomplete;
#endif

	case Ctrl('L'):		/* Whole line completion after ^X */
#ifdef INSERT_EXPAND
	    if (ctrl_x_mode != CTRL_X_WHOLE_LINE)
#endif
	    {
		/* CTRL-L with 'insertmode' set: Leave Insert mode */
		if (p_im)
		{
		    if (echeck_abbr(Ctrl('L') + ABBR_OFF))
			break;
		    goto doESCkey;
		}
		goto normalchar;
	    }
#ifdef INSERT_EXPAND
	    /* FALLTHROUGH */

	case Ctrl('P'):		/* Do previous pattern completion */
	case Ctrl('N'):		/* Do next pattern completion */
	    /* if 'complete' is empty then plain ^P is no longer special,
	     * but it is under other ^X modes */
	    if (    *curbuf->b_p_cpt == NUL && !ctrl_x_mode
		    && !(continue_status & CONT_LOCAL))
		goto normalchar;
docomplete:
	    i = ins_complete(c);
	    if (i)
		need_redraw |= i;
	    else
		continue_status = 0;
	    break;
#endif /* INSERT_EXPAND */

	case Ctrl('Y'):		/* copy from previous line or scroll down */
	case Ctrl('E'):		/* copy from next line	   or scroll up */
#ifdef INSERT_EXPAND
	    if (ctrl_x_mode == CTRL_X_SCROLL)
	    {
		if (c == Ctrl('Y'))
		    scrolldown_clamp();
		else
		    scrollup_clamp();
		update_screen(VALID);
	    }
	    else
#endif
	    {
		c = ins_copychar(curwin->w_cursor.lnum
						 + (c == Ctrl('Y') ? -1 : 1));
		if (c != NUL)
		{
		    long	tw_save;

		    /* The character must be taken literally, insert like it
		     * was typed after a CTRL-V, and pretend 'textwidth'
		     * wasn't set.  Digits, 'o' and 'x' are special after a
		     * CTRL-V, don't use it for these. */
		    if (!isalnum(c))
			AppendToRedobuff((char_u *)"\026");	/* CTRL-V */
		    tw_save = curbuf->b_p_tw;
		    curbuf->b_p_tw = -1;
		    insert_special(c, TRUE, FALSE);
		    curbuf->b_p_tw = tw_save;
		    need_redraw = TRUE;
#ifdef RIGHTLEFT
		    revins_chars++;
		    revins_legal++;
#endif
		    c = Ctrl('V');	/* pretend CTRL-V is last character */
		}
	    }
	    break;

	  default:
#ifdef UNIX
	    if (c == intr_char)		/* special interrupt char */
		goto do_intr;
#endif

normalchar:
#ifdef SMARTINDENT
	    /*
	     * Try to perform smart-indenting.
	     */
	    ins_try_si(c);
#endif

	    if (c == ' ')
	    {
		inserted_space = TRUE;
#ifdef CINDENT
		if (inindent(0))
		    can_cindent = FALSE;
#endif
		if (Insstart_blank_vcol == MAXCOL
			&& curwin->w_cursor.lnum == Insstart.lnum)
		    Insstart_blank_vcol = get_nolist_virtcol();
	    }

	    if (vim_iswordc(c) || !echeck_abbr(c))
	    {
		insert_special(c, FALSE, FALSE);
		need_redraw = TRUE;
#ifdef RIGHTLEFT
		revins_legal++;
		revins_chars++;
#endif
	    }
	    break;
	}   /* end of switch (c) */

	/* If the cursor was moved we didn't just insert a space */
	if (arrow_used)
	    inserted_space = FALSE;

#ifdef CINDENT
	if (curbuf->b_p_cin && can_cindent
# ifdef INSERT_EXPAND
					    && !ctrl_x_mode
# endif
							       )
	{
force_cindent:
	    /*
	     * Indent now if a key was typed that is in 'cinkeys'.
	     */
	    if (in_cinkeys(c, ' ', line_is_white))
	    {
		stop_arrow();

		/* re-indent the current line */
		fixthisline(get_c_indent);

		/* draw the changes on the screen later */
		need_redraw = TRUE;
	    }
	}
#endif /* CINDENT */

    }	/* for (;;) */
    /* NOTREACHED */
}

/*
 * Put a character directly onto the screen.  It's not stored in a buffer.
 * Used while handling CTRL-K, CTRL-V, etc. in Insert mode.
 */
    static void
edit_putchar(c, highlight)
    int	    c;
    int	    highlight;
{
    int	    attr;

    if (NextScreen != NULL)
    {
	update_topline();	/* just in case w_topline isn't valid */
	validate_cursor();
	if (highlight)
	    attr = hl_attr(HLF_8);
	else
	    attr = 0;
	screen_putchar(c, curwin->w_winpos + curwin->w_wrow,
#ifdef RIGHTLEFT
		    curwin->w_p_rl ? (int)Columns - 1 - curwin->w_wcol :
#endif
							 curwin->w_wcol, attr);
    }
}

/*
 * Called when p_dollar is set: display a '$' at the end of the changed text
 * Only works when cursor is in the line that changes.
 */
    void
display_dollar(col)
    colnr_t	col;
{
    colnr_t save_col;

    if (!redrawing())
	return;

    cursor_off();
    save_col = curwin->w_cursor.col;
    curwin->w_cursor.col = col;
    curs_columns(FALSE);	    /* recompute w_wrow and w_wcol */
    if (curwin->w_wcol < Columns)
    {
	edit_putchar('$', FALSE);
	dollar_vcol = curwin->w_virtcol;
    }
    curwin->w_cursor.col = save_col;
}

/*
 * Call this function before moving the cursor from the normal insert position
 * in insert mode.
 */
    static void
undisplay_dollar()
{
    if (dollar_vcol)
    {
	dollar_vcol = 0;
	update_screenline();
    }
}

/*
 * Insert an indent (for <Tab> or CTRL-T) or delete an indent (for CTRL-D).
 * Keep the cursor on the same character.
 * type == INDENT_INC	increase indent (for CTRL-T or <Tab>)
 * type == INDENT_DEC	decrease indent (for CTRL-D)
 * type == INDENT_SET	set indent to "amount"
 * if round is TRUE, round the indent to 'shiftwidth' (only with _INC and _Dec).
 */
    void
change_indent(type, amount, round, replaced)
    int	    type;
    int	    amount;
    int	    round;
    int	    replaced;		/* replaced character, put on replace stack */
{
    int		vcol;
    int		last_vcol;
    int		insstart_less;		/* reduction for Insstart.col */
    int		new_cursor_col;
    int		i;
    char_u	*ptr;
    int		save_p_list;
    int		start_col;
    colnr_t	vc;
    colnr_t	orig_col = 0;		/* init for GCC */
    char_u	*new_line, *orig_line = NULL;	/* init for GCC */

    /* VREPLACE mode needs to know what the line was like before changing */
    if (State == VREPLACE)
    {
	orig_line = vim_strsave(ml_get_curline());  /* Deal with NULL below */
	orig_col = curwin->w_cursor.col;
    }

    /* for the following tricks we don't want list mode */
    save_p_list = curwin->w_p_list;
    curwin->w_p_list = FALSE;
    getvcol(curwin, &curwin->w_cursor, NULL, &vc, NULL);
    vcol = vc;

    /*
     * For Replace mode we need to fix the replace stack later, which is only
     * possible when the cursor is in the indent.  Remember the number of
     * characters before the cursor if it's possible.
     */
    start_col = curwin->w_cursor.col;

    /* determine offset from first non-blank */
    new_cursor_col = curwin->w_cursor.col;
    beginline(BL_WHITE);
    new_cursor_col -= curwin->w_cursor.col;

    insstart_less = curwin->w_cursor.col;

    /*
     * If the cursor is in the indent, compute how many screen columns the
     * cursor is to the left of the first non-blank.
     */
    if (new_cursor_col < 0)
	vcol = get_indent() - vcol;

    if (new_cursor_col > 0)	    /* can't fix replace stack */
	start_col = -1;

    /*
     * Set the new indent.  The cursor will be put on the first non-blank.
     */
    if (type == INDENT_SET)
	set_indent(amount, TRUE);
    else
	shift_line(type == INDENT_DEC, round, 1);
    insstart_less -= curwin->w_cursor.col;

    /*
     * Try to put cursor on same character.
     * If the cursor is at or after the first non-blank in the line,
     * compute the cursor column relative to the column of the first
     * non-blank character.
     * If we are not in insert mode, leave the cursor on the first non-blank.
     * If the cursor is before the first non-blank, position it relative
     * to the first non-blank, counted in screen columns.
     */
    if (new_cursor_col >= 0)
    {
	/*
	 * When changing the indent while the cursor is touching it, reset
	 * Insstart_col to 0.
	 */
	if (new_cursor_col == 0)
	    insstart_less = MAXCOL;
	new_cursor_col += curwin->w_cursor.col;
    }
    else if (!(State & INSERT))
	new_cursor_col = curwin->w_cursor.col;
    else
    {
	/*
	 * Compute the screen column where the cursor should be.
	 */
	vcol = get_indent() - vcol;
	curwin->w_virtcol = (vcol < 0) ? 0 : vcol;

	/*
	 * Advance the cursor until we reach the right screen column.
	 */
	vcol = last_vcol = 0;
	new_cursor_col = -1;
	ptr = ml_get_curline();
	while (vcol <= (int)curwin->w_virtcol)
	{
	    last_vcol = vcol;
	    ++new_cursor_col;
	    vcol += lbr_chartabsize(ptr + new_cursor_col, (colnr_t)vcol);
	}
	vcol = last_vcol;

	/*
	 * May need to insert spaces to be able to position the cursor on
	 * the right screen column.
	 */
	if (vcol != (int)curwin->w_virtcol)
	{
	    curwin->w_cursor.col = new_cursor_col;
	    i = (int)curwin->w_virtcol - vcol;
	    ptr = alloc(i + 1);
	    if (ptr != NULL)
	    {
		new_cursor_col += i;
		ptr[i] = NUL;
		while (--i >= 0)
		    ptr[i] = ' ';
		ins_str(ptr);
		vim_free(ptr);
	    }
	}

	/*
	 * When changing the indent while the cursor is in it, reset
	 * Insstart_col to 0.
	 */
	insstart_less = MAXCOL;
    }

    curwin->w_p_list = save_p_list;

    if (new_cursor_col <= 0)
	curwin->w_cursor.col = 0;
    else
	curwin->w_cursor.col = new_cursor_col;
    curwin->w_set_curswant = TRUE;
    changed_cline_bef_curs();

    /*
     * May have to adjust the start of the insert.
     */
    if (State & INSERT)
    {
	if (curwin->w_cursor.lnum == Insstart.lnum && Insstart.col != 0)
	{
	    if ((int)Insstart.col <= insstart_less)
		Insstart.col = 0;
	    else
		Insstart.col -= insstart_less;
	}
	if ((int)ai_col <= insstart_less)
	    ai_col = 0;
	else
	    ai_col -= insstart_less;
    }

    /*
     * For REPLACE mode, may have to fix the replace stack, if it's possible.
     * If the number of characters before the cursor decreased, need to pop a
     * few characters from the replace stack.
     * If the number of characters before the cursor increased, need to push a
     * few NULs onto the replace stack.
     */
    if (State == REPLACE && start_col >= 0)
    {
	while (start_col > (int)curwin->w_cursor.col)
	{
	    replace_join(0);	    /* remove a NUL from the replace stack */
	    --start_col;
	}
	while (start_col < (int)curwin->w_cursor.col || replaced)
	{
	    replace_push(NUL);
	    if (replaced)
	    {
		replace_push(replaced);
		replaced = NUL;
	    }
	    ++start_col;
	}
    }

    /*
     * For VREPLACE mode, we also have to fix the replace stack.  In this case
     * it is always possible because we backspace over the whole line and then
     * put it back again the way we wanted it.
     */
    if (State == VREPLACE)
    {
	/* If orig_line didn't allocate, just return.  At least we did the job,
	 * even if you can't backspace.
	 */
	if (orig_line == NULL)
	    return;

	/* Save new line */
	new_line = vim_strsave(ml_get_curline());
	if (new_line == NULL)
	    return;

	/* We only put back the new line up to the cursor */
	new_line[curwin->w_cursor.col] = NUL;

	/* Put back original line */
	ml_replace(curwin->w_cursor.lnum, orig_line, FALSE);
	curwin->w_cursor.col = orig_col;

	/* Backspace from cursor to start of line */
	backspace_until_column(0);

	/* Insert new stuff into line again */
	vr_virtcol = MAXCOL;
	ptr = new_line;
	while (*ptr != NUL)
	    ins_char(*ptr++);

	vim_free(new_line);
    }
}

/*
 * Truncate the space at the end of a line.  This is to be used only in an
 * insert mode.  It handles fixing the replace stack for REPLACE and VREPLACE
 * modes.
 */
    void
truncate_spaces(line)
    char_u  *line;
{
    int	    i;

    /* find start of trailing white space */
    for (i = STRLEN(line) - 1; i >= 0 && vim_iswhite(line[i]); i--)
    {
	if (State == REPLACE || State == VREPLACE)
	    replace_join(0);	    /* remove a NUL from the replace stack */
    }
    line[i + 1] = NUL;
}

/*
 * Backspace the cursor until the given column.  Handles REPLACE and VREPLACE
 * modes correctly.  May also be used when not in insert mode at all.
 */
    void
backspace_until_column(col)
    int	    col;
{
    while ((int)curwin->w_cursor.col > col)
    {
	curwin->w_cursor.col--;
	if (State == REPLACE || State == VREPLACE)
	    replace_do_bs();
	else
	    (void)del_char(FALSE);
    }
}

#if defined(INSERT_EXPAND) || defined(PROTO)
/*
 * Is the character 'c' a valid key to keep us in the current ctrl-x mode?
 * -- webb
 */
    int
vim_is_ctrl_x_key(c)
    int	    c;
{
    switch (ctrl_x_mode)
    {
	case 0:		    /* Not in any ctrl-x mode */
	    if (c == Ctrl('N') || c == Ctrl('P') || c == Ctrl('X'))
		return TRUE;
	    break;
	case CTRL_X_NOT_DEFINED_YET:
	    if (       c == Ctrl('X') || c == Ctrl('Y') || c == Ctrl('E')
		    || c == Ctrl('L') || c == Ctrl('F') || c == Ctrl(']')
		    || c == Ctrl('I') || c == Ctrl('D') || c == Ctrl('P')
		    || c == Ctrl('N'))
		return TRUE;
	    break;
	case CTRL_X_SCROLL:
	    if (c == Ctrl('Y') || c == Ctrl('E'))
		return TRUE;
	    break;
	case CTRL_X_WHOLE_LINE:
	    if (c == Ctrl('L') || c == Ctrl('P') || c == Ctrl('N'))
		return TRUE;
	    break;
	case CTRL_X_FILES:
	    if (c == Ctrl('F') || c == Ctrl('P') || c == Ctrl('N'))
		return TRUE;
	    break;
	case CTRL_X_DICTIONARY:
	    if (c == Ctrl('K') || c == Ctrl('P') || c == Ctrl('N'))
		return TRUE;
	    break;
	case CTRL_X_TAGS:
	    if (c == Ctrl(']') || c == Ctrl('P') || c == Ctrl('N'))
		return TRUE;
	    break;
#ifdef FIND_IN_PATH
	case CTRL_X_PATH_PATTERNS:
	    if (c == Ctrl('P') || c == Ctrl('N'))
		return TRUE;
	    break;
	case CTRL_X_PATH_DEFINES:
	    if (c == Ctrl('D') || c == Ctrl('P') || c == Ctrl('N'))
		return TRUE;
	    break;
#endif
	default:
	    emsg(e_internal);
	    break;
    }
    return FALSE;
}

/*
 * This is like ins_compl_add(), but if ic and inf are set, then the
 * case of the originally typed text is used, and the case of the completed
 * text is infered, ie this tries to work out what case you probably wanted
 * the rest of the word to be in -- webb
 */
    int
ins_compl_add_infercase(str, len, fname, dir, reuse)
    char_u  *str;
    int	    len;
    char_u  *fname;
    int	    dir;
    int     reuse;
{
    int has_lower = FALSE;
    int was_letter = FALSE;
    int idx;

    if (p_ic && curbuf->b_p_inf && len < IOSIZE)
    {
	/* Infer case of completed part -- webb */
	/* Use IObuff, str would change text in buffer! */
	STRNCPY(IObuff, str, len);
	IObuff[len] = NUL;

	/* Rule 1: Were any chars converted to lower? */
	for (idx = 0; idx < completion_length; ++idx)
	{
	    if (islower(original_text[idx]))
	    {
		has_lower = TRUE;
		if (isupper(IObuff[idx]))
		{
		    /* Rule 1 is satisfied */
		    for (idx = completion_length; idx < len; ++idx)
			IObuff[idx] = TO_LOWER(IObuff[idx]);
		    break;
		}
	    }
	}

	/*
	 * Rule 2: No lower case, 2nd consecutive letter converted to
	 * upper case.
	 */
	if (!has_lower)
	{
	    for (idx = 0; idx < completion_length; ++idx)
	    {
		if (was_letter && isupper(original_text[idx]) &&
		    islower(IObuff[idx]))
		{
		    /* Rule 2 is satisfied */
		    for (idx = completion_length; idx < len; ++idx)
			IObuff[idx] = TO_UPPER(IObuff[idx]);
		    break;
		}
		was_letter = isalpha(original_text[idx]);
	    }
	}

	/* Copy the original case of the part we typed */
	STRNCPY(IObuff, original_text, completion_length);

	return ins_compl_add(IObuff, len, fname, dir, reuse);
    }
    return ins_compl_add(str, len, fname, dir, reuse);
}

/*
 * Add a match to the list of matches.
 * If the given string is already in the list of completions, then return
 * FAIL, otherwise add it to the list and return OK.  If there is an error,
 * maybe because alloc returns NULL, then RET_ERROR is returned -- webb.
 */
    static int
ins_compl_add(str, len, fname, dir, reuse)
    char_u  *str;
    int	    len;
    char_u  *fname;
    int	    dir;
    int     reuse;
{
    struct Completion *match;

    ui_breakcheck();
    if (got_int)
	return RET_ERROR;
    if (len < 0)
	len = STRLEN(str);

    /*
     * If the same match is already present, don't add it.
     */
    if (first_match != NULL)
    {
	match = first_match;
	do
	{
	    if (    !(match->original & ORIGINAL_TEXT)
		    && STRNCMP(match->str, str, (size_t)len) == 0
		    && match->str[len] == NUL)
		return FAIL;
	    match = match->next;
	} while (match != NULL && match != first_match);
    }

    /*
     * Allocate a new match structure.
     * Copy the values to the new match structure.
     */
    match = (struct Completion *)alloc((unsigned)sizeof(struct Completion));
    if (match == NULL)
	return RET_ERROR;
    if (reuse & ORIGINAL_TEXT)
	match->str = original_text;
    else if ((match->str = vim_strnsave(str, len)) == NULL)
    {
	vim_free(match);
	return RET_ERROR;
    }
    /* match-fname is:
     * - curr_match->fname if it is a string equal to fname.
     * - a copy of fname, FREE_FNAME is set to free later THE allocated mem.
     * - NULL otherwise.	--Acevedo */
    if (fname && curr_match && curr_match->fname
	      && STRCMP(fname, curr_match->fname) == 0)
	match->fname = curr_match->fname;
    else if (fname && (match->fname = vim_strsave(fname)) != NULL)
	reuse |= FREE_FNAME;
    else
	match->fname = NULL;
    match->original = reuse;

    /*
     * Link the new match structure in the list of matches.
     */
    if (first_match == NULL)
	match->next = match->prev = NULL;
    else if (dir == FORWARD)
    {
	match->next = curr_match->next;
	match->prev = curr_match;
    }
    else	/* BACKWARD */
    {
	match->next = curr_match;
	match->prev = curr_match->prev;
    }
    if (match->next)
	match->next->prev = match;
    if (match->prev)
	match->prev->next = match;
    else	/* if there's nothing before, it is the first match */
	first_match = match;
    curr_match = match;

    return OK;
}

/* Make the completion list cyclic.
 * Return the number of matches (excluding the original).
 */
    static int
ins_compl_make_cyclic()
{
    struct Completion *match;
    int	    count = 0;

    if (first_match != NULL)
    {
	/*
	 * Find the end of the list.
	 */
	match = first_match;
	/* there's always an entry for the original_text, it doesn't count. */
	while (match->next != NULL && match->next != first_match)
	{
	    match = match->next;
	    ++count;
	}
	match->next = first_match;
	first_match->prev = match;
    }
    return count;
}

#define DICT_FIRST	(1)	/* use just first element in "dict" */
#define DICT_EXACT	(2)	/* "dict" is the exact name of a file */
/*
 * Add any identifiers that match the given pattern to the list of
 * completions.
 */
    static void
ins_compl_dictionaries(dict, pat, dir, flags)
    char_u  *dict;
    char_u  *pat;
    int	     dir;
    int	     flags;
{
    char_u	*ptr;
    char_u	*buf;
    int		at_start;
    FILE	*fp;
    vim_regexp	*prog;
    int		add_r;
    char_u	**files;
    int		count;
    int		i;
    int		save_p_scs;

    buf = alloc(LSIZE);
    /* If 'infercase' is set, don't use 'smartcase' here */
    save_p_scs = p_scs;
    if (curbuf->b_p_inf)
	p_scs = FALSE;
    set_reg_ic(pat);	/* set reg_ic according to p_ic, p_scs and pat */
    prog = vim_regcomp(pat, (int)p_magic);
    while (buf != NULL && prog != NULL && *dict != NUL
		       && !got_int && !completion_interrupted)
    {
	/* copy one dictionary file name into buf */
	if (flags == DICT_EXACT)
	{
	    count = 1;
	    files = &dict;
	}
	else
	{
	    copy_option_part(&dict, buf, LSIZE, ",");
	    if (expand_wildcards(1, &buf, &count, &files,
						     EW_FILE|EW_SILENT) != OK)
		count = 0;
	}

	for (i = 0; i < count && !got_int && !completion_interrupted; i++)
	{
	    fp = mch_fopen((char *)files[i], "r");  /* open dictionary file */
	    if (flags != DICT_EXACT)
	    {
		sprintf((char*)IObuff, "Scanning dictionary: %s",
							    (char *)files[i]);
		msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
	    }

	    if (fp != NULL)
	    {
		/*
		 * Read dictionary file line by line.
		 * Check each line for a match.
		 */
		while (!got_int && !completion_interrupted
			        && !vim_fgets(buf, LSIZE, fp))
		{
		    ptr = buf;
		    at_start = TRUE;
		    while (vim_regexec(prog, ptr, at_start))
		    {
			at_start = FALSE;
			ptr = prog->startp[0];
			while (vim_iswordc(*ptr))
			    ++ptr;
			add_r = ins_compl_add_infercase(prog->startp[0],
				(int)(ptr - prog->startp[0]), files[i], dir, 0);
			if (add_r == OK)
			    /* if dir was BACKWARD then honor it just once */
			    dir = FORWARD;
			else if (add_r == RET_ERROR)
			    break;
		    }
		    line_breakcheck();
		    ins_compl_check_keys();
		}
		fclose(fp);
	    }
	}
	if (flags != DICT_EXACT)
	    FreeWild(count, files);
	if (flags)
	    break;
    }
    p_scs = save_p_scs;
    vim_free(prog);
    vim_free(buf);
}

/*
 * Free the list of completions
 */
    static void
ins_compl_free()
{
    struct Completion *match;

    if (first_match == NULL)
	return;
    curr_match = first_match;
    do
    {
	match = curr_match;
	curr_match = curr_match->next;
	vim_free(match->str);
	/* several entries may use the same fname, free it just once. */
	if (match->original & FREE_FNAME)
	    vim_free(match->fname);
	vim_free(match);
    } while (curr_match != NULL && curr_match != first_match);
    first_match = curr_match = NULL;
}

    static void
ins_compl_clear()
{
    continue_status = 0;
    started_completion = FALSE;
    complete_pat = NULL;
    save_sm = -1;
}

/*
 * Prepare for insert-expand, or stop it.
 */
    static int
ins_compl_prep(c)
    int	    c;
{
    char_u	*ptr;
    char_u	*tmp_ptr;
    int		temp;
    linenr_t	lnum;
    int		need_redraw = FALSE;

    /* Ignore end of Select mode mapping */
    if (c == K_SELECT)
	return FALSE;

    if (ctrl_x_mode == CTRL_X_NOT_DEFINED_YET)
    {
	/*
	 * We have just entered ctrl-x mode and aren't quite sure which
	 * ctrl-x mode it will be yet.	Now we decide -- webb
	 */
	switch (c)
	{
	    case Ctrl('E'):
	    case Ctrl('Y'):
		ctrl_x_mode = CTRL_X_SCROLL;
		if (State == INSERT)
		    edit_submode = (char_u *)" (insert) Scroll (^E/^Y)";
		else
		    edit_submode = (char_u *)" (replace) Scroll (^E/^Y)";
		showmode();
		break;
	    case Ctrl('L'):
		ctrl_x_mode = CTRL_X_WHOLE_LINE;
		break;
	    case Ctrl('F'):
		ctrl_x_mode = CTRL_X_FILES;
		break;
	    case Ctrl('K'):
		ctrl_x_mode = CTRL_X_DICTIONARY;
		break;
	    case Ctrl(']'):
		ctrl_x_mode = CTRL_X_TAGS;
		break;
#ifdef FIND_IN_PATH
	    case Ctrl('I'):
	    case K_S_TAB:
		ctrl_x_mode = CTRL_X_PATH_PATTERNS;
		break;
	    case Ctrl('D'):
		ctrl_x_mode = CTRL_X_PATH_DEFINES;
		break;
#endif
	    case Ctrl('P'):
	    case Ctrl('N'):
		/* ^X^P means LOCAL expansion if nothing interrupted (eg we
		 * just started ^X mode, or there were enough ^X's to cancel
		 * the previous mode, say ^X^F^X^X^P or ^P^X^X^X^P, see below)
		 * do normal expansion when interrupting a different mode (say
		 * ^X^F^X^P or ^P^X^X^P, see below)
		 * nothing changes if interrupting mode 0, (eg, the flag
		 * doesn't change when going to ADDING mode  -- Acevedo */
		if (!(continue_status & CONT_INTRPT))
		    continue_status |= CONT_LOCAL;
		else if (continue_mode)
		    continue_status &=~CONT_LOCAL;
		/* FALLTHROUGH */
	    default:
		/* if we have typed at least 2 ^X's... for modes != 0, we set
		 * continue_status = 0 (eg, as if we had just started ^X mode)
		 * for mode 0, we set continue_mode to an impossible value, in
		 * both cases ^X^X can be used to restart the same mode
		 * (avoiding ADDING mode).   Undocumented feature:
		 * In a mode != 0 ^X^P and ^X^X^P start 'complete' and local
		 * ^P expansions respectively.	In mode 0 an extra ^X is
		 * needed since ^X^P goes to ADDING mode  -- Acevedo */
		if (c == Ctrl('X'))
		{
		    if (continue_mode)
			continue_status = 0;
		    else
			continue_mode = CTRL_X_NOT_DEFINED_YET;
		}
		ctrl_x_mode = 0;
		edit_submode = NULL;
		showmode();
		break;
	}
    }
    else if (ctrl_x_mode)
    {
	/* We we're already in ctrl-x mode, do we stay in it? */
	if (!vim_is_ctrl_x_key(c))
	{
	    if (ctrl_x_mode == CTRL_X_SCROLL)
		ctrl_x_mode = 0;
	    else
		ctrl_x_mode = CTRL_X_FINISHED;
	    edit_submode = NULL;
	}
	showmode();
    }
    if (started_completion || ctrl_x_mode == CTRL_X_FINISHED)
    {
	/* Show error message from attempted keyword completion (probably
	 * 'Pattern not found') until another key is hit, then go back to
	 * showing what mode we are in.
	 */
	showmode();
	if ((ctrl_x_mode == 0 && c != Ctrl('N') && c != Ctrl('P')) ||
					    ctrl_x_mode == CTRL_X_FINISHED)
	{
	    /* Get here when we have finished typing a sequence of ^N and
	     * ^P or other completion characters in CTRL-X mode. Free up
	     * memory that was used, and make sure we can redo the insert
	     * -- webb.
	     */
	    if (curr_match != NULL)
	    {
		/*
		 * If any of the original typed text has been changed,
		 * eg when ignorecase is set, we must add back-spaces to
		 * the redo buffer.  We add as few as necessary to delete
		 * just the part of the original text that has changed
		 * -- webb
		 */
		ptr = curr_match->str;
		tmp_ptr = original_text;
		while (*tmp_ptr && *tmp_ptr == *ptr)
		{
		    ++tmp_ptr;
		    ++ptr;
		}
		for (temp = 0; tmp_ptr[temp]; ++temp)
		    AppendCharToRedobuff(K_BS);
		while (*ptr)
		{
		    /* Put a string of normal characters in the redo buffer */
		    tmp_ptr = ptr;
		    while (*ptr >= ' ' && *ptr < DEL)
			++ptr;
		    /* Don't put '0' or '^' as last character, just in case a
		     * CTRL-D is typed next */
		    if (*ptr == NUL && (ptr[-1] == '0' || ptr[-1] == '^'))
			--ptr;
		    if (ptr > tmp_ptr)
		    {
			temp = *ptr;
			*ptr = NUL;
			AppendToRedobuff(tmp_ptr);
			*ptr = temp;
		    }
		    if (*ptr)
		    {
			/* quote special chars with a CTRL-V */
			AppendCharToRedobuff(Ctrl('V'));
			AppendCharToRedobuff(*ptr);
			/* CTRL-V '0' must be inserted as CTRL-V 048 */
			if (*ptr++ == '0')
			    AppendToRedobuff((char_u *)"48");
		    }
		}
	    }

	    /*
	     * When completing whole lines: fix indent for 'cindent'.
	     * Otherwise, break line if it's too long.
	     */
	    lnum = curwin->w_cursor.lnum;
	    if (continue_mode == CTRL_X_WHOLE_LINE)
	    {
#ifdef CINDENT
		/* re-indent the current line */
		if (can_cindent && curbuf->b_p_cin)
		    fixthisline(get_c_indent);
#endif
	    }
	    else
	    {
		/* put the cursor on the last char, for 'tw' formatting */
		curwin->w_cursor.col--;
		insertchar(NUL, FALSE, -1, FALSE);
		curwin->w_cursor.col++;
	    }
	    if (lnum != curwin->w_cursor.lnum)
	    {
		update_topline();
		update_screen(NOT_VALID);
	    }
	    else
		need_redraw = TRUE;

	    vim_free(complete_pat);
	    complete_pat = NULL;
	    ins_compl_free();
	    started_completion = FALSE;
	    ctrl_x_mode = 0;
	    p_sm = save_sm;
	    if (edit_submode != NULL)
	    {
		edit_submode = NULL;
		showmode();
	    }
	}
    }

    /* reset continue_* if we left expansion-mode, if we stay they'll be
     * (re)set properly in ins_complete */
    if (!vim_is_ctrl_x_key(c))
	continue_status = continue_mode = 0;

    return need_redraw;
}

/*
 * Loops through the list of windows, loaded-buffers or non-loaded-buffers
 * (depending on flag) starting from buf and looking for a non-scanned
 * buffer (other than curbuf).	curbuf is special, if it is called with
 * buf=curbuf then it has to be the first call for a given flag/expansion.
 *
 * Returns the buffer to scan, if any, otherwise returns curbuf -- Acevedo
 */
    static BUF *
ins_compl_next_buf(buf, flag)
    BUF		*buf;
    int		flag;
{
    static WIN	*w;

    if (flag == 'w')		/* just windows */
    {
	if (buf == curbuf)	/* first call for this flag/expansion */
	    w = curwin;
	while ((w = w->w_next ? w->w_next : firstwin) != curwin
		&& w->w_buffer->b_scanned)
	    ;
	buf = w->w_buffer;
    }
    else	/* 'b' (just loaded buffers) or 'u' (just non-loaded buffers) */
	while ((buf = buf->b_next ? buf->b_next : firstbuf) != curbuf
		&& ((buf->b_ml.ml_mfp == NULL) != (flag == 'u')
		    || buf->b_scanned))
	    ;
    return buf;
}

/*
 * Get the next expansion(s) for the text starting at the initial curbuf
 * position "ini" and in the direction dir.
 * Return the total of matches or -1 if still unknown -- Acevedo
 */
    static int
ins_compl_get_exp(ini, dir)
    FPOS	*ini;
    int		dir;
{
    static FPOS		first_match_pos;
    static FPOS		last_match_pos;
    static char_u	*e_cpt = (char_u *)"";	/* curr. entry in 'complete' */
    static int		found_all = FALSE;	/* Found all matches. */
    static BUF		*ins_buf = NULL;

    FPOS		*pos;
    char_u		**matches;
    int			save_p_scs;
    int			save_p_ws;
    int			save_p_ic;
    int			i;
    int			num_matches;
    int			len;
    int			found_new_match;
    int			type = ctrl_x_mode;
    char_u		*ptr;
    char_u		*tmp_ptr;
    char_u		*dict = NULL;
    int			dict_f = 0;
    struct Completion	*old_match;

    if (!started_completion)
    {
	for (ins_buf = firstbuf; ins_buf != NULL; ins_buf = ins_buf->b_next)
	    ins_buf->b_scanned = 0;
	found_all = FALSE;
	ins_buf = curbuf;
	e_cpt = continue_status & CONT_LOCAL ? (char_u *)"." : curbuf->b_p_cpt;
	last_match_pos = first_match_pos = *ini;
    }

    old_match = curr_match;		/* remember the last current match */
    pos = (dir == FORWARD) ? &last_match_pos : &first_match_pos;
    /* For ^N/^P loop over all the flags/windows/buffers in 'complete' */
    for (;;)
    {
	found_new_match = FAIL;
	/* in mode 0 pick a new entry from e_cpt if started_completion is off,
	 * or if found_all says this entry is done  -- Acevedo */
	if (!ctrl_x_mode && (!started_completion || found_all))
	{
	    found_all = FALSE;
	    while (*e_cpt == ',' || *e_cpt == ' ')
		e_cpt++;
	    if (*e_cpt == '.' && !curbuf->b_scanned)
	    {
		ins_buf = curbuf;
		first_match_pos = *ini;
		/* So that ^N can match word immediately after cursor */
		if (ctrl_x_mode == 0)
		    dec(&first_match_pos);
		last_match_pos = first_match_pos;
		type = 0;
	    }
	    else if (vim_strchr((char_u *)"buw", *e_cpt) != NULL
		 && (ins_buf = ins_compl_next_buf(ins_buf, *e_cpt)) != curbuf)
	    {
		if (*e_cpt != 'u')
		{
		    started_completion = TRUE;
		    first_match_pos.col = last_match_pos.col = 0;
		    first_match_pos.lnum = ins_buf->b_ml.ml_line_count + 1;
		    last_match_pos.lnum = 0;
		    type = 0;
		}
		else
		{
		    found_all = TRUE;
		    if (ins_buf->b_fname == NULL)
			continue;
		    type = CTRL_X_DICTIONARY;
		    dict = ins_buf->b_fname;
		    dict_f = DICT_EXACT;
		}
		sprintf((char*)IObuff, "Scanning: %s",
			ins_buf->b_sfname == NULL ? "No File"
						  : (char *)ins_buf->b_sfname);
		msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
	    }
	    else if (*e_cpt == NUL)
		break;
	    else
	    {
		if (*e_cpt == 'k')
		{
		    type = CTRL_X_DICTIONARY;
		    if (*++e_cpt != ',' && *e_cpt != NUL)
		    {
			dict = e_cpt;
			dict_f = DICT_FIRST;
		    }
		}
#ifdef FIND_IN_PATH
		else if (*e_cpt == 'i')
		    type = CTRL_X_PATH_PATTERNS;
		else if (*e_cpt == 'd')
		    type = CTRL_X_PATH_DEFINES;
#endif
		else if (*e_cpt == ']' || *e_cpt == 't')
		{
		    type = CTRL_X_TAGS;
		    sprintf((char*)IObuff, "Scanning tags.");
		    msg_trunc_attr(IObuff, TRUE, hl_attr(HLF_R));
		}
		else
		    type = -1;

		/* in any case e_cpt is advanced to the next entry */
		(void)copy_option_part(&e_cpt, IObuff, IOSIZE, ",");

		found_all = TRUE;
		if (type == -1)
		    continue;
	    }
	}

	switch (type)
	{
	case -1:
	    break;
#ifdef FIND_IN_PATH
	case CTRL_X_PATH_PATTERNS:
	case CTRL_X_PATH_DEFINES:
	    find_pattern_in_path(complete_pat, dir,
				 (int)STRLEN(complete_pat), FALSE, FALSE,
				 (type == CTRL_X_PATH_DEFINES
				  && !(continue_status & CONT_SOL))
				 ? FIND_DEFINE : FIND_ANY, 1L, ACTION_EXPAND,
				 (linenr_t)1, (linenr_t)MAXLNUM);
	    break;
#endif

	case CTRL_X_DICTIONARY:
	    ins_compl_dictionaries(dict ? dict : p_dict, complete_pat, dir,
				  dict ? dict_f : 0);
	    dict = NULL;
	    break;

	case CTRL_X_TAGS:
	    /* set reg_ic according to p_ic, p_scs and pat */
	    /* Need to set p_ic here, find_tags() overwrites reg_ic */
	    set_reg_ic(complete_pat);
	    save_p_ic = p_ic;
	    p_ic = reg_ic;
	    /* Find up to TAG_MANY matches.  Avoids that an enourmous number
	     * of matches is found when complete_pat is empty */
	    if (find_tags(complete_pat, &num_matches, &matches,
		    TAG_REGEXP | TAG_NAMES | TAG_NOIC |
		    TAG_INS_COMP | (ctrl_x_mode ? TAG_VERBOSE : 0),
		    TAG_MANY) == OK && num_matches > 0)
	    {
		int	add_r = OK;
		int	ldir = dir;

		for (i = 0; i < num_matches && add_r != RET_ERROR; i++)
		    if ((add_r = ins_compl_add(matches[i], -1, NULL, ldir, 0))
			    == OK)
			/* if dir was BACKWARD then honor it just once */
			ldir = FORWARD;
		FreeWild(num_matches, matches);
	    }
	    p_ic = save_p_ic;
	    break;

	case CTRL_X_FILES:
	    if (expand_wildcards(1, &complete_pat, &num_matches, &matches,
				  EW_FILE|EW_DIR|EW_ADDSLASH|EW_SILENT) == OK)
	    {
		int	add_r = OK;
		int	ldir = dir;

		/* May change home directory back to "~". */
		tilde_replace(complete_pat, num_matches, matches);
		for (i = 0; i < num_matches && add_r != RET_ERROR; i++)
		    if ((add_r = ins_compl_add(matches[i], -1, NULL, ldir, 0))
			    == OK)
			/* if dir was BACKWARD then honor it just once */
			ldir = FORWARD;
		FreeWild(num_matches, matches);
	    }
	    break;

	default:	/* normal ^P/^N and ^X^L */
	    /*
	     * If 'infercase' is set, don't use 'smartcase' here
	     */
	    save_p_scs = p_scs;
	    if (ins_buf->b_p_inf)
		p_scs = FALSE;
	    /*	buffers other than curbuf are scanned from the beginning or the
	     *	end but never from the middle, thus setting nowrapscan in this
	     *	buffers is a good idea, on the other hand, we always set
	     *	wrapscan for curbuf to avoid missing matches -- Acevedo,Webb */
	    save_p_ws = p_ws;
	    if (ins_buf != curbuf)
		p_ws = FALSE;
	    else if (*e_cpt == '.')
		p_ws = TRUE;
	    for (;;)
	    {
		int	reuse = 0;

		/* ctrl_x_mode == CTRL_X_WHOLE_LINE || word-wise search that has
		 * added a word that was at the beginning of the line */
		if (	ctrl_x_mode == CTRL_X_WHOLE_LINE
			|| (continue_status & CONT_SOL))
		    found_new_match = search_for_exact_line(ins_buf, pos,
							    dir, complete_pat);
		else
		    found_new_match = searchit(ins_buf, pos, dir, complete_pat,
					       1L, SEARCH_KEEP + SEARCH_NFMSG,
					       RE_LAST);
		if (!started_completion)
		{
		    /* set started_completion even on fail */
		    started_completion = TRUE;
		    first_match_pos = *pos;
		    last_match_pos = *pos;
		}
		else if (first_match_pos.lnum == last_match_pos.lnum
			 && first_match_pos.col == last_match_pos.col)
		    found_new_match = FAIL;
		if (!found_new_match)
		{
		    if (ins_buf == curbuf)
			found_all = TRUE;
		    break;
		}

		/* when ADDING, the text before the cursor matches, skip it */
		if (	(continue_status & CONT_ADDING) && ins_buf == curbuf
			&& ini->lnum == pos->lnum
			&& ini->col  == pos->col)
		    continue;
		ptr = ml_get_buf(ins_buf, pos->lnum, FALSE) + pos->col;
		if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
		{
		    if (continue_status & CONT_ADDING)
		    {
			if (pos->lnum >= ins_buf->b_ml.ml_line_count)
			    continue;
			ptr = ml_get_buf(ins_buf, pos->lnum + 1, FALSE);
			if (!p_paste)
			    ptr = skipwhite(ptr);
		    }
		    len = STRLEN(ptr);
		}
		else
		{
		    tmp_ptr = ptr;
		    if (continue_status & CONT_ADDING)
		    {
			tmp_ptr += completion_length;
			if (vim_iswordc(*tmp_ptr))
			    continue;
			while (*tmp_ptr && !vim_iswordc(*tmp_ptr++))
			    ;
		    }
		    while (vim_iswordc(*tmp_ptr))
			tmp_ptr++;
		    len = tmp_ptr - ptr;
		    if ((continue_status & CONT_ADDING)
			&& len == completion_length)
		    {
			if (pos->lnum < ins_buf->b_ml.ml_line_count)
			{
			    /* Try next line, if any. the new word will be
			     * "join" as if the normal command "J" was used.
			     * IOSIZE is always greater than
			     * completion_length, so the next STRNCPY always
			     * works -- Acevedo */
			    STRNCPY(IObuff, ptr, len);
			    ptr = ml_get_buf(ins_buf, pos->lnum + 1, FALSE);
			    tmp_ptr = ptr = skipwhite(ptr);
			    while (*tmp_ptr && !vim_iswordc(*tmp_ptr++))
				;
			    while (vim_iswordc(*tmp_ptr))
				tmp_ptr++;
			    if (tmp_ptr > ptr)
			    {
				if (*ptr != ')' && IObuff[len-1] != TAB)
				{
				    if (IObuff[len-1] != ' ')
					IObuff[len++] = ' ';
				    /* IObuf =~ "\k.* ", thus len >= 2 */
				    if (p_js
					&& (IObuff[len-2] == '.'
					    || (vim_strchr(p_cpo, CPO_JOINSP)
								       == NULL
						&& (IObuff[len-2] == '?'
						    || IObuff[len-2] == '!'))))
					IObuff[len++] = ' ';
				}
				/* copy as much as posible of the new word */
				if (tmp_ptr - ptr >= IOSIZE - len)
				    tmp_ptr = ptr + IOSIZE - len - 1;
				STRNCPY(IObuff + len, ptr, tmp_ptr - ptr);
				len += tmp_ptr - ptr;
				reuse |= CONT_S_IPOS;
			    }
			    IObuff[len] = NUL;
			    ptr = IObuff;
			}
			if (len == completion_length)
			    continue;
		    }
		}
		if (ins_compl_add_infercase(ptr, len, ins_buf == curbuf ?
			NULL : ins_buf->b_sfname, dir, reuse) != FAIL)
		{
		    found_new_match = OK;
		    break;
		}
	    }
	    p_scs = save_p_scs;
	    p_ws = save_p_ws;
	}
	/* check if curr_match has changed, (e.g. other type of expansion
	 * added somenthing) */
	if (curr_match != old_match)
	    found_new_match = OK;
	/* break the loop for specialized modes (use 'complete' just for the
	 * generic ctrl_x_mode == 0) or when we've found a new match */
	if (ctrl_x_mode || found_new_match)
	    break;
	/* Mark a buffer scanned when it has been scanned completely */
	if (type == 0 || type == CTRL_X_PATH_PATTERNS)
	    ins_buf->b_scanned = TRUE;

	started_completion = FALSE;
    }
    started_completion = TRUE;

    if (!ctrl_x_mode && *e_cpt == NUL)	/* Got to end of 'complete' */
	found_new_match = FAIL;

    i = -1;		/* total of matches, unknown */
    if (!found_new_match || (ctrl_x_mode != 0 &&
			     ctrl_x_mode != CTRL_X_WHOLE_LINE))
	i = ins_compl_make_cyclic();

    /* If several matches were added (FORWARD) or the search failed and has
     * just been made cyclic then we have to move curr_match to the next or
     * previous entry (if any) -- Acevedo */
    curr_match = dir == FORWARD ? old_match->next : old_match->prev;
    if (curr_match == NULL)
	curr_match = old_match;
    return i;
}

/* Delete the old text being completed */
    static void
ins_compl_delete()
{
    int	    i;

    /*
     * In insert mode: Delete the typed part.
     * In replace mode: Put the old characters back, if any.
     */
    i = complete_col + (continue_status & CONT_ADDING ? completion_length : 0);
    backspace_until_column(i);
    changed_cline_bef_curs();
}

/* Insert the new text being completed */
    static void
ins_compl_insert()
{
    char_u  *ptr;

    /*
     * Use ins_char() to insert the text, it is a bit slower than
     * ins_str(), but it takes care of replace mode.
     */
    ptr = shown_match->str + curwin->w_cursor.col - complete_col;
    while (*ptr)
	ins_char(*ptr++);

    update_screenline();
}

/*
 * Fill in the next completion in the current direction.  If
 * allow_get_expansion is TRUE, then we may call ins_compl_get_exp() to get
 * more completions.  If it is FALSE, then we just do nothing when there are
 * no more completions in a given direction.  The latter case is used when we
 * are still in the middle of finding completions, to allow browsing through
 * the ones found so far.
 * Return the total number of matches, or -1 if still unknown -- webb.
 *
 * curr_match is currently being used by ins_compl_get_exp(), so we use
 * shown_match here.
 *
 * Note that this function may be called recursively once only.  First with
 * allow_get_expansion TRUE, which calls ins_compl_get_exp(), which in turn
 * calls this function with allow_get_expansion FALSE.
 */
    static int
ins_compl_next(allow_get_expansion)
    int	    allow_get_expansion;
{
    int	    num_matches = -1;
    int	    i;

    if (allow_get_expansion)
    {
	/* Delete old text to be replaced */
	ins_compl_delete();
    }
    completion_pending = FALSE;
    if (shown_direction == FORWARD && shown_match->next != NULL)
	shown_match = shown_match->next;
    else if (shown_direction == BACKWARD && shown_match->prev != NULL)
	shown_match = shown_match->prev;
    else
    {
	completion_pending = TRUE;
	if (allow_get_expansion)
	{
	    num_matches = ins_compl_get_exp(&initial_pos, complete_direction);
	    if (completion_pending)
	    {
		if (complete_direction == shown_direction)
		    shown_match = curr_match;
	    }
	}
	else
	    return -1;
    }

    /* Insert the text of the new completion */
    ins_compl_insert();

    if (!allow_get_expansion)
    {
	/* Delete old text to be replaced, since we're still searching and
	 * don't want to match ourselves!
	 */
	ins_compl_delete();
    }

    /*
     * Show the file name for the match (if any)
     * Truncate the file name to avoid a wait for return.
     */
    if (shown_match->fname != NULL)
    {
	STRCPY(IObuff, "match in file ");
	i = (vim_strsize(shown_match->fname) + 16) - sc_col;
	if (i <= 0)
	    i = 0;
	else
	    STRCAT(IObuff, "<");
	STRCAT(IObuff, shown_match->fname + i);
	msg(IObuff);
	redraw_cmdline = FALSE;	    /* don't overwrite! */
    }

    return num_matches;
}

/*
 * Call this while finding completions, to check whether the user has hit a key
 * that should change the currently displayed completion, or exit completion
 * mode.  Also, when completion_pending is TRUE, show a completion as soon as
 * possible. -- webb
 */
    void
ins_compl_check_keys()
{
    static int	count = 0;

    int	    c;

    /* Don't check when reading keys from a script.  That would break the test
     * scripts */
    if (using_script())
	return;

    /* Only do this at regular intervals */
    if (++count < CHECK_KEYS_TIME)
	return;
    count = 0;

    c = vpeekc();
    /* Trick: when no typeahead found, but there is something in the typeahead
     * buffer, it must be an ESC that is recognized as the start of a key
     * code. */
    if (c == NUL && typelen)
	c = ESC;
    if (c != NUL)
    {
	if (vim_is_ctrl_x_key(c) && c != Ctrl('X'))
	{
	    c = safe_vgetc();	/* Eat the character */
	    if (c == Ctrl('P') || c == Ctrl('L'))
		shown_direction = BACKWARD;
	    else
		shown_direction = FORWARD;
	    (void)ins_compl_next(FALSE);
	}
	else
	    completion_interrupted = TRUE;
    }
    if (completion_pending && !got_int)
	(void)ins_compl_next(FALSE);
}

    static int
ins_complete(c)
    int		    c;
{
    char_u	    *ptr;
    char_u	    *tmp_ptr = NULL;		/* init for gcc */
    int		    temp = 0;
    int		    i;

    if (c == Ctrl('P') || c == Ctrl('L'))
	complete_direction = BACKWARD;
    else
	complete_direction = FORWARD;
    if (!started_completion)
    {
	/* First time we hit ^N or ^P (in a row, I mean) */

	/* Turn off 'sm' so we don't show matches with ^X^L */
	save_sm = p_sm;
	p_sm = FALSE;

	did_ai = FALSE;
#ifdef SMARTINDENT
	did_si = FALSE;
	can_si = FALSE;
	can_si_back = FALSE;
#endif
	stop_arrow();
	ptr = ml_get(curwin->w_cursor.lnum);
	complete_col = curwin->w_cursor.col;

	/* if this same ctrl_x_mode has been interrupted use the text from
	 * initial_pos to the cursor as a pattern to add a new word instead of
	 * expand the one before the cursor, in word-wise if "initial_pos" is
	 * not in the same line as the cursor then fix it (the line has been
	 * split because it was longer than 'tw').  if SOL is set then skip
	 * the previous pattern, a word at the beginning of the line has been
	 * inserted, we'll look for that  -- Acevedo. */
	if ((continue_status & CONT_INTRPT) && continue_mode == ctrl_x_mode)
	{	/* it is a continued search */
	    continue_status &= ~CONT_INTRPT;	/* remove INTRPT */
	    if (ctrl_x_mode == 0 || ctrl_x_mode == CTRL_X_PATH_PATTERNS
					|| ctrl_x_mode == CTRL_X_PATH_DEFINES)
	    {
		if (initial_pos.lnum != curwin->w_cursor.lnum)
		{
		    /* line (probably) wrapped, set initial_pos to the first
		     * non_blank in the line, if it is not a wordchar include
		     * it to get a better pattern, but then we don't want the
		     * "\\<" prefix, check it bellow */
		    tmp_ptr = skipwhite(ptr);
		    initial_pos.col = tmp_ptr - ptr;
		    initial_pos.lnum = curwin->w_cursor.lnum;
		    continue_status &= ~CONT_SOL;    /* clear SOL if present */
		}
		else
		{
		    /* S_IPOS was set when we inserted a word that was at the
		     * beginning of the line, which means that we'll go to SOL
		     * mode but first we need to redefine initial_pos */
		    if (continue_status & CONT_S_IPOS)
		    {
			continue_status |= CONT_SOL;
			initial_pos.col = skipwhite(ptr + completion_length +
						    initial_pos.col) - ptr;
		    }
		    tmp_ptr = ptr + initial_pos.col;
		}
		temp = curwin->w_cursor.col - (tmp_ptr-ptr);
		/* IObuf is used to add a "word from the next line" would we
		 * have enough space?  just being paranoic */
#define	MIN_SPACE 75
		if (temp > (IOSIZE - MIN_SPACE))
		{
		    continue_status &= ~CONT_SOL;
		    temp = (IOSIZE - MIN_SPACE);
		    tmp_ptr = curwin->w_cursor.col - temp + ptr;
		}
		continue_status |= CONT_ADDING | CONT_N_ADDS;
		if (temp < 1)
		    continue_status &= CONT_LOCAL;
	    }
	    else if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
		continue_status = CONT_ADDING | CONT_N_ADDS;
	    else
		continue_status = 0;
	}
	else
	    continue_status &= CONT_LOCAL;

	if (!(continue_status & CONT_ADDING))	/* normal expansion */
	{
	    continue_mode = ctrl_x_mode;
	    if (ctrl_x_mode)	/* Remove LOCAL iff ctrl_x_mode != 0 */
		continue_status = 0;
	    continue_status |= CONT_N_ADDS;
	    initial_pos = curwin->w_cursor;
	    temp = (int)complete_col;
	    tmp_ptr = ptr;
	}

	/* Work out completion pattern and original text -- webb */
	if (ctrl_x_mode == 0 || (ctrl_x_mode & CTRL_X_WANT_IDENT))
	{
	    if (       (continue_status & CONT_SOL)
		    || ctrl_x_mode == CTRL_X_PATH_DEFINES)
	    {
		if (!(continue_status & CONT_ADDING))
		{
		    while (--temp >= 0 && vim_isIDc(ptr[temp]))
			;
		    tmp_ptr += ++temp;
		    temp = complete_col - temp;
		}
		complete_pat = vim_strnsave(tmp_ptr, temp);
		if (complete_pat == NULL)
		    return FALSE;
		if (p_ic)
		    for (i = 0; i < temp; i++)
			complete_pat[i] = TO_LOWER(complete_pat[i]);
	    }
	    else if (continue_status & CONT_ADDING)
	    {
		char_u	    *prefix = (char_u *)"\\<";

		/* we need 3 extra chars, 1 for the NUL and
		 * 2 >= strlen(prefix)	-- Acevedo */
		complete_pat = alloc(quote_meta(NULL, tmp_ptr, temp) + 3);
		if (complete_pat == NULL)
		    return FALSE;
		if (!vim_iswordc(*tmp_ptr) ||
			(tmp_ptr > ptr && vim_iswordc(*(tmp_ptr-1))))
		    prefix = (char_u *)"";
		STRCPY((char *)complete_pat, prefix);
		(void)quote_meta(complete_pat + STRLEN(prefix), tmp_ptr, temp);
	    }
	    else if (--temp < 0 || !vim_iswordc(ptr[temp]))
	    {
		/* Match any word of at least two chars */
		complete_pat = vim_strsave((char_u *)"\\<\\k\\k");
		if (complete_pat == NULL)
		    return FALSE;
		tmp_ptr += complete_col;
		temp = 0;
	    }
	    else
	    {
		while (--temp >= 0 && vim_iswordc(ptr[temp]))
		    ;
		tmp_ptr += ++temp;
		if ((temp = (int)complete_col - temp) == 1)
		{
		    /* Only match word with at least two chars -- webb
		     * there's no need to call quote_meta,
		     * alloc(7) is enough  -- Acevedo
		     */
		    complete_pat = alloc(7);
		    if (complete_pat == NULL)
			return FALSE;
		    STRCPY((char *)complete_pat, "\\<");
		    (void)quote_meta(complete_pat + 2, tmp_ptr, 1);
		    STRCAT((char *)complete_pat, "\\k");
		}
		else
		{
		    complete_pat = alloc(quote_meta(NULL, tmp_ptr, temp) + 3);
		    if (complete_pat == NULL)
			return FALSE;
		    STRCPY((char *)complete_pat, "\\<");
		    (void)quote_meta(complete_pat + 2, tmp_ptr, temp);
		}
	    }
	}
	else if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
	{
	    tmp_ptr = skipwhite(ptr);
	    temp = (int)complete_col - (tmp_ptr - ptr);
	    if (temp < 0)	/* cursor in indent: empty pattern */
		temp = 0;
	    complete_pat = vim_strnsave(tmp_ptr, temp);
	    if (complete_pat == NULL)
		return FALSE;
	    if (p_ic)
		for (i = 0; i < temp; i ++)
		    complete_pat[i] = TO_LOWER(complete_pat[i]);
	}
	else if (ctrl_x_mode == CTRL_X_FILES)
	{
	    while (--temp >= 0 && vim_isfilec(ptr[temp]))
		;
	    tmp_ptr += ++temp;
	    temp = (int)complete_col - temp;
	    complete_pat = addstar(tmp_ptr, temp, EXPAND_FILES);
	    if (complete_pat == NULL)
		return FALSE;
	}
	complete_col = tmp_ptr - ptr;

	if (continue_status & CONT_ADDING)
	{
	    if (continue_status & CONT_LOCAL)
		edit_submode = (char_u *)ctrl_x_msgs[2];
	    else
		edit_submode = (char_u *)ctrl_x_msgs[ctrl_x_mode & 15];
	    if (ctrl_x_mode == CTRL_X_WHOLE_LINE)
	    {
		/* Insert a new line, keep indentation but ignore 'comments' */
#ifdef COMMENTS
		char_u *old = curbuf->b_p_com;

		curbuf->b_p_com = (char_u *)"";
#endif
		initial_pos.lnum = curwin->w_cursor.lnum;
		initial_pos.col = complete_col;
		ins_eol('\r');
#ifdef COMMENTS
		curbuf->b_p_com = old;
#endif
		tmp_ptr = (char_u *)"";
		temp = 0;
		complete_col = curwin->w_cursor.col;
	    }
	}
	else
	{
	    /* msg. without the " Adding" part. */
	    if (continue_status & CONT_LOCAL)
		edit_submode = (char_u *)ctrl_x_msgs[2] + C_X_SKIP;
	    else
		edit_submode = (char_u *)ctrl_x_msgs[ctrl_x_mode & 15]
								   + C_X_SKIP;
	    initial_pos.col = complete_col;
	}

	completion_length = temp;
	/* Always "add completion" for the "original text", it uses
	 * "original_text" not a copy -- Acevedo */
	if ((original_text = vim_strnsave(tmp_ptr, temp)) == NULL
	    || ins_compl_add(original_text, -1, NULL, 0, ORIGINAL_TEXT) != OK)
	{
	    vim_free(complete_pat);
	    complete_pat = NULL;
	    return FALSE;
	}

	/* showmode might reset the internal line pointers, so it must
	 * be called before ptr = ml_get, or when this address is no
	 * longer needed.  -- Acevedo.
	 */
	edit_submode_extra = (char_u *)"-- Searching...";
	edit_submode_highl = HLF_COUNT;
	showmode();
	edit_submode_extra = NULL;
	out_flush();
    }

    shown_match = curr_match;
    shown_direction = complete_direction;
    temp = ins_compl_next(TRUE);
    curr_match = shown_match;
    complete_direction = shown_direction;
    completion_interrupted = FALSE;

    /* eat the ESC to avoid leaving insert mode */
    if (got_int && !global_busy)
    {
	(void)vgetc();
	got_int = FALSE;
    }

    /* we found no match if the list has only the original_text-entry */
    if (first_match == first_match->next)
    {
	edit_submode_extra = (continue_status & CONT_ADDING)
			      && completion_length > 1 ? e_hitend : e_patnotf;
	edit_submode_highl = HLF_E;
	/* remove N_ADDS flag, so next ^X<> won't try to go to ADDING mode,
	 * because we couldn't expand anything at first place, but if we used
	 * ^P, ^N, ^X^I or ^X^D we might want to add-expand a single-char-word
	 * (such as M in M'exico) if not tried already.  -- Acevedo */
	if (	   completion_length > 1
		|| (continue_status & CONT_ADDING)
		|| (ctrl_x_mode != 0
		    && ctrl_x_mode != CTRL_X_PATH_PATTERNS
		    && ctrl_x_mode != CTRL_X_PATH_DEFINES))
	    continue_status &= ~CONT_N_ADDS;
    }

    if (curr_match->original & CONT_S_IPOS)
	continue_status |= CONT_S_IPOS;
    else
	continue_status &= ~CONT_S_IPOS;

    if (edit_submode_extra == NULL)
    {
	if (curr_match->original & ORIGINAL_TEXT)
	{
	    edit_submode_extra = (char_u *)"Back at original";
	    edit_submode_highl = HLF_W;
	}
	else if (continue_status & CONT_S_IPOS)
	{
	    edit_submode_extra = (char_u *)"Word from other line";
	    edit_submode_highl = HLF_COUNT;
	}
	else if (curr_match->next == curr_match->prev)
	{
	    edit_submode_extra = (char_u *)"The only match";
	    edit_submode_highl = HLF_COUNT;
	}
    }

    if (temp > 1)
    {
	if (ctrl_x_mode == 0 || ctrl_x_mode == CTRL_X_WHOLE_LINE)
	    ptr = (char_u *)"All %d matches have now been found";
	else if (ctrl_x_mode == CTRL_X_FILES)
	    ptr = (char_u *)"There are %d matching file names";
	else if (ctrl_x_mode == CTRL_X_TAGS)
	    ptr = (char_u *)"There are %d matching tags";
	else
	    ptr = (char_u *)"There are %d matches";
	sprintf((char *)IObuff, (char *)ptr, temp);
	if (dollar_vcol)
	    curs_columns(FALSE);
	msg_attr(IObuff, hl_attr(HLF_R));
	if (!char_avail())
	{
	    setcursor();
	    out_flush();
	    ui_delay(1500L, FALSE);
	}
    }
    showmode();
    if (edit_submode_extra != NULL)
    {
	if (!p_smd)
	    msg_attr(edit_submode_extra,
		    edit_submode_highl < HLF_COUNT
		    ? hl_attr(edit_submode_highl) : 0);
	if (!char_avail())
	{
	    setcursor();
	    out_flush();
	    ui_delay(1500L, FALSE);
	}
	edit_submode_extra = NULL;
    }

    return TRUE;
}

/*
 * Looks in the first "len" chars. of "src" for search-metachars.
 * If dest is not NULL the chars. are copied there quoting (with
 * a backslash) the metachars, and dest would be NUL terminated.
 * Returns the length (needed) of dest
 */
    static int
quote_meta(dest, src, len)
    char_u	*dest;
    char_u	*src;
    int		len;
{
    int	m;

    for (m = len; --len >= 0; src++)
    {
	switch (*src)
	{
	    case '.':
	    case '*':
	    case '[':
		if (ctrl_x_mode == CTRL_X_DICTIONARY)
		    break;
	    case '~':
		if (!p_magic)	/* quote these only if magic is set */
		    break;
	    case '\\':
		if (ctrl_x_mode == CTRL_X_DICTIONARY)
		    break;
	    case '^':		/* currently it's not needed. */
	    case '$':
		m++;
		if (dest)
		    *dest++ = '\\';
		break;
	}
	if (dest)
	    *dest++ = *src;
    }
    if (dest)
	*dest = NUL;

    return m;
}
#endif /* INSERT_EXPAND */

/*
 * Next character is interpreted literally.
 * A one, two or three digit decimal number is interpreted as its byte value.
 * If one or two digits are entered, the next character is given to vungetc().
 */
    int
get_literal()
{
    int		cc;
    int		nc;
    int		i;
    int		hexmode = 0, octalmode = 0;

    if (got_int)
	return Ctrl('C');

#ifdef USE_GUI
    /*
     * In GUI there is no point inserting the internal code for a special key.
     * It is more useful to insert the string "<KEY>" instead.	This would
     * probably be useful in a text window too, but it would not be
     * vi-compatible (maybe there should be an option for it?) -- webb
     */
    if (gui.in_use)
	++allow_keys;
#endif
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
    ++no_mapping;		/* don't map the next key hits */
    cc = 0;
    i = 0;
    for (;;)
    {
	do
	    nc = safe_vgetc();
	while (nc == K_IGNORE || nc == K_SCROLLBAR || nc == K_HORIZ_SCROLLBAR);
#ifdef CMDLINE_INFO
	if (!(State & CMDLINE)
# ifdef MULTI_BYTE
		&& (is_dbcs && !IsLeadByte(nc))
# endif
		)
	    add_to_showcmd(nc);
#endif
	if (nc == 'x' || nc == 'X')
	    hexmode++;
	else if (nc == 'o' || nc == 'O')
	    octalmode++;
	else
	{
	    if (hexmode)
	    {
		if (!vim_isdigit(nc) && !isxdigit(nc))
		    break;
		nc = TO_LOWER(nc);
		if (nc >= 'a')
		    nc = 10 + nc - 'a';
		else
		    nc = nc - '0';
		cc = cc * 16 + nc ;
	    }
	    else if (octalmode)
	    {
		if (!vim_isdigit(nc) || (nc > '7'))
		    break;
		cc = cc * 8 + nc - '0';
	    }
	    else
	    {
		if (!vim_isdigit(nc))
		    break;
		cc = cc * 10 + nc - '0';
	    }

	    ++i;
	}

	if (cc > 255)
	    cc = 255;		/* limit range to 0-255 */
	nc = 0;

	if (hexmode && (i >= 2))
	    break;
	if (!hexmode && (i >= 3))
	    break;
    }
    if (i == 0)	    /* no number entered */
    {
	if (nc == K_ZERO)   /* NUL is stored as NL */
	{
	    cc = '\n';
	    nc = 0;
	}
	else
	{
	    cc = nc;
	    nc = 0;
	}
    }

    if (cc == 0)	/* NUL is stored as NL */
	cc = '\n';

    --no_mapping;
#ifdef USE_GUI
    if (gui.in_use)
	--allow_keys;
#endif
    if (nc)
	vungetc(nc);
    got_int = FALSE;	    /* CTRL-C typed after CTRL-V is not an interrupt */
    return cc;
}

/*
 * Insert character, taking care of special keys and mod_mask
 */
    static void
insert_special(c, allow_modmask, ctrlv)
    int	    c;
    int	    allow_modmask;
    int	    ctrlv;	    /* c was typed after CTRL-V */
{
    char_u  *p;
    int	    len;

    /*
     * Special function key, translate into "<Key>". Up to the last '>' is
     * inserted with ins_str(), so as not to replace characters in replace
     * mode.
     * Only use mod_mask for special keys, to avoid things like <S-Space>,
     * unless 'allow_modmask' is TRUE.
     */
    if (IS_SPECIAL(c) || (mod_mask && allow_modmask))
    {
	p = get_special_key_name(c, mod_mask);
	len = STRLEN(p);
	c = p[len - 1];
	if (len > 2)
	{
	    p[len - 1] = NUL;
	    ins_str(p);
	    AppendToRedobuff(p);
	    ctrlv = FALSE;
	}
    }
    insertchar(c, FALSE, -1, ctrlv);
}

/*
 * Special characters in this context are those that need processing other
 * than the simple insertion that can be performed here. This includes ESC
 * which terminates the insert, and CR/NL which need special processing to
 * open up a new line. This routine tries to optimize insertions performed by
 * the "redo", "undo" or "put" commands, so it needs to know when it should
 * stop and defer processing to the "normal" mechanism.
 * '0' and '^' are special, because they can be followed by CTRL-D.
 */
#define ISSPECIAL(c)	((c) < ' ' || (c) >= DEL || (c) == '0' || (c) == '^')

    void
insertchar(c, force_formatting, second_indent, ctrlv)
    unsigned	c;
    int		force_formatting;	/* format line regardless of p_fo */
    int		second_indent;		/* indent for second line if >= 0 */
    int		ctrlv;			/* c typed just after CTRL-V */
{
    int		haveto_redraw = FALSE;
    int		textwidth;
#ifdef COMMENTS
    colnr_t	leader_len;
#endif
    int		first_line = TRUE;
    int		fo_ins_blank;
    int		save_char = NUL;
    int		cc;
    char_u	*p;

    stop_arrow();

    textwidth = comp_textwidth(force_formatting);
    fo_ins_blank = has_format_option(FO_INS_BLANK);

    /*
     * Try to break the line in two or more pieces when:
     * - Always do this if we have been called to do formatting only.
     * - Otherwise:
     *	 - Don't do this if inserting a blank
     *	 - Don't do this if an existing character is being replaced, unless
     *	   we're in VREPLACE mode.
     *	 - Do this if the cursor is not on the line where insert started
     *	 or - 'formatoptions' doesn't have 'l' or the line was not too long
     *	       before the insert.
     *	    - 'formatoptions' doesn't have 'b' or a blank was inserted at or
     *	      before 'textwidth'
     */
    if (textwidth
	    && (force_formatting
		|| (!vim_iswhite(c)
		    && !(State == REPLACE && *ml_get_cursor() != NUL)
		    && (curwin->w_cursor.lnum != Insstart.lnum
			|| ((!has_format_option(FO_INS_LONG)
				|| Insstart_textlen <= (colnr_t)textwidth)
			    && (!fo_ins_blank
				|| Insstart_blank_vcol <= (colnr_t)textwidth
			    ))))))
    {
	/*
	 * When 'ai' is off we don't want a space under the cursor to be
	 * deleted.  Replace it with an 'x' temporarily.
	 */
	if (!curbuf->b_p_ai)
	{
	    cc = gchar_cursor();
	    if (vim_iswhite(cc))
	    {
		save_char = cc;
		pchar_cursor('x');
	    }
	}

	/*
	 * Repeat breaking lines, until the current line is not too long.
	 */
	while (!got_int)
	{
	    int		startcol;		/* Cursor column at entry */
	    int		wantcol;		/* column at textwidth border */
	    int		foundcol;		/* column for start of spaces */
	    int		end_foundcol = 0;	/* column for start of word */
	    int		orig_col = 0;
	    colnr_t	len;
	    colnr_t	virtcol;
	    char_u	*saved_text = NULL;

	    virtcol = get_nolist_virtcol();
	    if (virtcol < (colnr_t)textwidth)
		break;

#ifdef COMMENTS
	    if (!force_formatting && has_format_option(FO_WRAP_COMS))
		fo_do_comments = TRUE;

	    /* Don't break until after the comment leader */
	    leader_len = get_leader_len(ml_get_curline(), NULL, FALSE);
#endif
	    if (!force_formatting
#ifdef COMMENTS
		    && leader_len == 0
#endif
		    && !has_format_option(FO_WRAP))

	    {
		textwidth = 0;
		break;
	    }
	    if ((startcol = curwin->w_cursor.col) == 0)
		break;
					/* find column of textwidth border */
	    coladvance((colnr_t)textwidth);
	    wantcol = curwin->w_cursor.col;

	    curwin->w_cursor.col = startcol - 1;
	    foundcol = 0;
	    /*
	     * Find position to break at.
	     * Stop at start of line.
	     * Stop at first entered white when 'formatoptions' has 'v'
	     */
	    while (curwin->w_cursor.col > 0
		    && ((!fo_ins_blank && !has_format_option(FO_INS_VI))
			|| curwin->w_cursor.lnum != Insstart.lnum
			|| curwin->w_cursor.col >= Insstart.col))
	    {
		cc = gchar_cursor();
		if (vim_iswhite(cc))
		{
		    /* remember position of blank just before text */
		    end_foundcol = curwin->w_cursor.col;
		    while (curwin->w_cursor.col > 0 && vim_iswhite(cc))
		    {
			--curwin->w_cursor.col;
			cc = gchar_cursor();
		    }
		    if (curwin->w_cursor.col == 0 && vim_iswhite(cc))
			break;		/* only spaces in front of text */
#ifdef COMMENTS
		    /* Don't break until after the comment leader */
		    if (curwin->w_cursor.col < leader_len)
			break;
#endif
		    foundcol = curwin->w_cursor.col + 1;
		    if (curwin->w_cursor.col < (colnr_t)wantcol)
			break;
		}
		--curwin->w_cursor.col;
	    }

	    if (foundcol == 0)		/* no spaces, cannot break line */
	    {
		curwin->w_cursor.col = startcol;
		break;
	    }

	    /*
	     * Offset between cursor position and line break is used by replace
	     * stack functions.  VREPLACE does not use this, and backspaces
	     * over the text instead.
	     */
	    if (State == VREPLACE)
		orig_col = startcol;	/* Will start backspacing from here */
	    else
		replace_offset = startcol - end_foundcol - 1;

	    /*
	     * adjust startcol for spaces that will be deleted and
	     * characters that will remain on top line
	     */
	    curwin->w_cursor.col = foundcol;
	    while (cc = gchar_cursor(), vim_iswhite(cc))
	    {
		++curwin->w_cursor.col;
		--startcol;
	    }
	    startcol -= foundcol;
	    if (startcol < 0)
		startcol = 0;

	    if (State == VREPLACE)
	    {
		/*
		 * In VREPLACE mode, we will backspace over the text to be
		 * wrapped, so save a copy now to put on the next line.
		 */
		saved_text = vim_strsave(ml_get_cursor());
		curwin->w_cursor.col = orig_col;
		if (saved_text == NULL)
		    break;	/* Can't do it, out of memory */
		saved_text[startcol] = NUL;

		/* Backspace over characters that will move to the next line */
		backspace_until_column(foundcol);
	    }
	    else
	    {
		/* put cursor after pos. to break line */
		curwin->w_cursor.col = foundcol;
	    }

	    /*
	     * Split the line just before the margin.
	     * Only insert/delete lines, but don't really redraw the window.
	     */
	    open_line(FORWARD, (redrawing() && !force_formatting) ? -1 : 0,
							    TRUE, old_indent);
	    old_indent = 0;

	    replace_offset = 0;
	    if (second_indent >= 0 && first_line)
	    {
		if (State == VREPLACE)
		    change_indent(INDENT_SET, second_indent, FALSE, NUL);
		else
		    set_indent(second_indent, TRUE);
	    }
	    first_line = FALSE;

	    if (State == VREPLACE)
	    {
		/*
		 * In VREPLACE mode we have backspaced over the text to be
		 * moved, now we re-insert it into the new line.
		 */
		for (p = saved_text; *p != NUL; p++)
		    ins_char(*p);
		vim_free(saved_text);
	    }
	    else
	    {
		/*
		 * Check if cursor is not past the NUL off the line, cindent
		 * may have added or removed indent.
		 */
		curwin->w_cursor.col += startcol;
		len = STRLEN(ml_get_curline());
		if (curwin->w_cursor.col > len)
		    curwin->w_cursor.col = len;
	    }

	    haveto_redraw = TRUE;
#ifdef CINDENT
	    can_cindent = TRUE;
#endif
	    /* moved the cursor, don't autoindent or cindent now */
	    did_ai = FALSE;
#ifdef SMARTINDENT
	    did_si = FALSE;
	    can_si = FALSE;
	    can_si_back = FALSE;
#endif
	    line_breakcheck();
	}

	if (save_char)			/* put back space after cursor */
	    pchar_cursor(save_char);

	if (c == NUL)			/* formatting only */
	    return;
#ifdef COMMENTS
	fo_do_comments = FALSE;
#endif
	if (haveto_redraw)
	{
	    update_topline();
	    update_screen(NOT_VALID);
	}
    }
    if (c == NUL)	    /* only formatting was wanted */
	return;

#ifdef COMMENTS
    /* Check whether this character should end a comment. */
    if (did_ai && (int)c == end_comment_pending)
    {
	char_u  *line;
	char_u	lead_end[COM_MAX_LEN];	    /* end-comment string */
	int	middle_len, end_len;
	int	i;
	int	old_fo_do_comments = fo_do_comments;

	/*
	 * Need to remove existing (middle) comment leader and insert end
	 * comment leader.  First, check what comment leader we can find.
	 */
	fo_do_comments = TRUE;
	i = get_leader_len(line = ml_get_curline(), &p, FALSE);
	if (i > 0 && vim_strchr(p, COM_MIDDLE) != NULL)	/* Just checking */
	{
	    /* Skip middle-comment string */
	    while (*p && p[-1] != ':')	/* find end of middle flags */
		++p;
	    middle_len = copy_option_part(&p, lead_end, COM_MAX_LEN, ",");
	    /* Don't count trailing white space for middle_len */
	    while (middle_len > 0 && vim_iswhite(lead_end[middle_len - 1]))
		--middle_len;

	    /* Find the end-comment string */
	    while (*p && p[-1] != ':')	/* find end of end flags */
		++p;
	    end_len = copy_option_part(&p, lead_end, COM_MAX_LEN, ",");

	    /* Skip white space before the cursor */
	    i = curwin->w_cursor.col;
	    while (--i >= 0 && vim_iswhite(line[i]))
		;
	    i++;

	    /* Skip to before the middle leader */
	    i -= middle_len;

	    /* Check some expected things before we go on */
	    if (i >= 0 && lead_end[end_len - 1] == end_comment_pending)
	    {
		/* Backspace over all the stuff we want to replace */
		backspace_until_column(i);

		/*
		 * Insert the end-comment string, except for the last
		 * character, which will get inserted as normal later.
		 */
		for (i = 0; i < end_len - 1; i++)
		    ins_char(lead_end[i]);
	    }
	}
	fo_do_comments = old_fo_do_comments;
    }
    end_comment_pending = NUL;
#endif

    did_ai = FALSE;
#ifdef SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif

    /*
     * If there's any pending input, grab up to INPUT_BUFLEN at once.
     * This speeds up normal text input considerably.
     * Don't do this when 'cindent' is set, because we might need to re-indent
     * at a ':', or any other character.
     */
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = FALSE;		/* allow scrolling here */
#endif
#define INPUT_BUFLEN 100
    if (       !ISSPECIAL(c)
	    && vpeekc() != NUL
	    && State != REPLACE
	    && State != VREPLACE
#ifdef CINDENT
	    && !curbuf->b_p_cin
#endif
#ifdef RIGHTLEFT
	    && !p_ri
#endif
	       )
    {
	char_u		buf[INPUT_BUFLEN + 1];
	int		i;
	colnr_t		virtcol = 0;

	buf[0] = c;
	i = 1;
	if (textwidth)
	    virtcol = get_nolist_virtcol();
	while (	   (c = vpeekc()) != NUL
		&& !ISSPECIAL(c)
		&& i < INPUT_BUFLEN
		&& (textwidth == 0
		    || (virtcol += charsize(buf[i - 1])) < (colnr_t)textwidth)
		&& !(!no_abbr && !vim_iswordc(c) && vim_iswordc(buf[i - 1])))
	{
#ifdef RIGHTLEFT
	    c = vgetc();
	    if (p_hkmap && KeyTyped)
		c = hkmap(c);		    /* Hebrew mode mapping */
#ifdef FKMAP
	    if (p_fkmap && KeyTyped)
		c = fkmap(c);		    /* Farsi mode mapping */
#endif
	    buf[i++] = c;
#else
	    buf[i++] = vgetc();
#endif
	}

#ifdef DIGRAPHS
	do_digraph(-1);			/* clear digraphs */
	do_digraph(buf[i-1]);		/* may be the start of a digraph */
#endif
	buf[i] = NUL;
	ins_str(buf);
	if (ctrlv)
	{
	    redo_literal(*buf);
	    i = 1;
	}
	else
	    i = 0;
	if (buf[i] != NUL)
	    AppendToRedobuff(buf + i);
    }
    else
    {
	ins_char(c);
	if (ctrlv)
	    redo_literal(c);
	else
	    AppendCharToRedobuff(c);
    }
}

/*
 * Find out textwidth to be used for formatting:
 *	if 'textwidth' option is set, use it
 *	else if 'wrapmargin' option is set, use Columns - 'wrapmargin'
 *	if invalid value, use 0.
 *	Set default to window width (maximum 79) for "Q" command.
 */
    int
comp_textwidth(ff)
    int		ff;	/* force formatting (for "Q" command) */
{
    int		textwidth;

    textwidth = curbuf->b_p_tw;
    if (textwidth == 0 && curbuf->b_p_wm)
	textwidth = Columns - curbuf->b_p_wm;
    if (textwidth < 0)
	textwidth = 0;
    if (ff && textwidth == 0)
    {
	textwidth = Columns - 1;
	if (textwidth > 79)
	    textwidth = 79;
    }
    return textwidth;
}

/*
 * Put a character in the redo buffer, for when just after a CTRL-V.
 */
    static void
redo_literal(c)
    int	    c;
{
    char_u	buf[10];

    /* Only digits need special treatment.  Translate them into a string of
     * three digits. */
    if (vim_isdigit(c))
    {
	sprintf((char *)buf, "%03d", c);
	AppendToRedobuff(buf);
    }
    else
	AppendCharToRedobuff(c);
}

/*
 * start_arrow() is called when an arrow key is used in insert mode.
 * It resembles hitting the <ESC> key.
 */
    static void
start_arrow(end_insert_pos)
    FPOS    *end_insert_pos;
{
    if (!arrow_used)	    /* something has been inserted */
    {
	AppendToRedobuff(ESC_STR);
	arrow_used = TRUE;	/* this means we stopped the current insert */
	stop_insert(end_insert_pos);
    }
}

/*
 * stop_arrow() is called before a change is made in insert mode.
 * If an arrow key has been used, start a new insertion.
 */
    void
stop_arrow()
{
    if (arrow_used)
    {
	(void)u_save_cursor();		/* errors are ignored! */
	Insstart = curwin->w_cursor;	/* new insertion starts here */
	Insstart_textlen = linetabsize(ml_get_curline());
	ai_col = 0;
	if (State == VREPLACE)
	{
	    orig_line_count = curbuf->b_ml.ml_line_count;
	    vr_lines_changed = 1;
	    vr_virtcol = MAXCOL;
	}
	ResetRedobuff();
	AppendToRedobuff((char_u *)"1i");   /* pretend we start an insertion */
	arrow_used = FALSE;
    }
}

/*
 * do a few things to stop inserting
 */
    static void
stop_insert(end_insert_pos)
    FPOS    *end_insert_pos;	/* where insert ended */
{
    int	    cc;

    stop_redo_ins();
    replace_flush();		/* abandon replace stack */

    /*
     * save the inserted text for later redo with ^@
     */
    vim_free(last_insert);
    last_insert = get_inserted();
    last_insert_skip = new_insert_skip;

    /*
     * If we just did an auto-indent, remove the white space from the end of
     * the line, and put the cursor back.
     */
    if (did_ai && !arrow_used)
    {
	if (gchar_cursor() == NUL && curwin->w_cursor.col > 0)
	    --curwin->w_cursor.col;
	while (cc = gchar_cursor(), vim_iswhite(cc))
	    (void)del_char(TRUE);
	if (cc != NUL)
	    ++curwin->w_cursor.col;	/* put cursor back on the NUL */
	/* the deletion is only seen in list mode, when 'hls' is set, and when
	 * trailing spaces are syntax-highlighted. */
	if (curwin->w_p_list
#ifdef EXTRA_SEARCH
		|| (p_hls && !no_hlsearch)
#endif
#ifdef SYNTAX_HL
		|| syntax_present(curbuf)
#endif
	   )
	    update_screenline();
    }
    did_ai = FALSE;
#ifdef SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif

    /* set '[ and '] to the inserted text */
    curbuf->b_op_start = Insstart;
    curbuf->b_op_end = *end_insert_pos;
}

/*
 * Set the last inserted text to a single character.
 * Used for the replace command.
 */
    void
set_last_insert(c)
    int	    c;
{
    vim_free(last_insert);
    last_insert = alloc(4);
    if (last_insert != NULL)
    {
	last_insert[0] = Ctrl('V');
	last_insert[1] = c;
	last_insert[2] = ESC;
	last_insert[3] = NUL;
	    /* Use the CTRL-V only when not entering a digit */
	last_insert_skip = isdigit(c) ? 1 : 0;
    }
}

/*
 * move cursor to start of line
 * if flags & BL_WHITE	move to first non-white
 * if flags & BL_SOL	move to first non-white if startofline is set,
 *			    otherwise keep "curswant" column
 * if flags & BL_FIX	don't leave the cursor on a NUL.
 */
    void
beginline(flags)
    int		flags;
{
    if ((flags & BL_SOL) && !p_sol)
	coladvance(curwin->w_curswant);
    else
    {
	curwin->w_cursor.col = 0;
	if (flags & (BL_WHITE | BL_SOL))
	{
	    char_u  *ptr;

	    for (ptr = ml_get_curline(); vim_iswhite(*ptr)
			       && !((flags & BL_FIX) && ptr[1] == NUL); ++ptr)
		++curwin->w_cursor.col;
	}
	curwin->w_set_curswant = TRUE;
    }
}

/*
 * oneright oneleft cursor_down cursor_up
 *
 * Move one char {right,left,down,up}.
 * Return OK when successful, FAIL when we hit a line of file boundary.
 */

    int
oneright()
{
    char_u *ptr;

    ptr = ml_get_cursor();
    if (*ptr++ == NUL || *ptr == NUL)
	return FAIL;

#ifdef MULTI_BYTE
    if (is_dbcs)
    {
	/* If the character under the cursor is a multi-byte character, move
	 * two bytes right. */
	if (IsTrailByte(ml_get(curwin->w_cursor.lnum), ptr))
	{
	    if (*(ptr + 1) == NUL)
		return FAIL;
	    ++curwin->w_cursor.col;
	}
    }
#endif
    curwin->w_set_curswant = TRUE;
    ++curwin->w_cursor.col;
    return OK;
}

    int
oneleft()
{
    if (curwin->w_cursor.col == 0)
	return FAIL;
    curwin->w_set_curswant = TRUE;
    --curwin->w_cursor.col;
#ifdef MULTI_BYTE
    /* if the character on the left of the current cursor is a multi-byte
     * character, move two characters left */
    if (is_dbcs)
	AdjustCursorForMultiByteChar();
#endif
    return OK;
}

    int
cursor_up(n, upd_topline)
    long	n;
    int		upd_topline;	    /* When TRUE: update topline */
{
    if (n != 0)
    {
	if (curwin->w_cursor.lnum <= 1)
	    return FAIL;
	if (n >= curwin->w_cursor.lnum)
	    curwin->w_cursor.lnum = 1;
	else
	    curwin->w_cursor.lnum -= n;
    }

    /* try to advance to the column we want to be at */
    coladvance(curwin->w_curswant);

    if (upd_topline)
	update_topline();	/* make sure curwin->w_topline is valid */

    return OK;
}

    int
cursor_down(n, upd_topline)
    long    n;
    int	    upd_topline;	    /* When TRUE: update topline */
{
    if (n != 0)
    {
	if (curwin->w_cursor.lnum >= curbuf->b_ml.ml_line_count)
	    return FAIL;
	curwin->w_cursor.lnum += n;
	if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
	    curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    }

    /* try to advance to the column we want to be at */
    coladvance(curwin->w_curswant);

    if (upd_topline)
	update_topline();	/* make sure curwin->w_topline is valid */

    return OK;
}

/*
 * Stuff the last inserted text in the read buffer.
 * Last_insert actually is a copy of the redo buffer, so we
 * first have to remove the command.
 */
    int
stuff_inserted(c, count, no_esc)
    int	    c;		/* Command character to be inserted */
    long    count;	/* Repeat this many times */
    int	    no_esc;	/* Don't add an ESC at the end */
{
    char_u	*esc_ptr;
    char_u	*ptr;
    char_u	*last_ptr;
    char_u	last = NUL;

    ptr = get_last_insert();
    if (ptr == NULL)
    {
	EMSG(e_noinstext);
	return FAIL;
    }

    /* may want to stuff the command character, to start Insert mode */
    if (c)
	stuffcharReadbuff(c);
    if ((esc_ptr = (char_u *)vim_strrchr(ptr, ESC)) != NULL)
	*esc_ptr = NUL;	    /* remove the ESC */

    /* when the last char is either "0" or "^" it will be quoted if no ESC
     * comes after it OR if it will inserted more than once and "ptr"
     * starts with ^D.	-- Acevedo
     */
    last_ptr = (esc_ptr ? esc_ptr : ptr + STRLEN(ptr)) - 1;
    if (last_ptr >= ptr && (*last_ptr == '0' || *last_ptr == '^')
	    && (no_esc || (*ptr == Ctrl('D') && count > 1)))
    {
	last = *last_ptr;
	*last_ptr = NUL;
    }

    do
    {
	stuffReadbuff(ptr);
	/* a trailing "0" is inserted as "<C-V>048", "^" as "<C-V>^" */
	if (last)
	    stuffReadbuff((char_u *)(last == '0' ? "\026048" : "\026^"));
    }
    while (--count > 0);

    if (last)
	*last_ptr = last;

    if (esc_ptr != NULL)
	*esc_ptr = ESC;	    /* put the ESC back */

    /* may want to stuff a trailing ESC, to get out of Insert mode */
    if (!no_esc)
	stuffcharReadbuff(ESC);

    return OK;
}

    char_u *
get_last_insert()
{
    if (last_insert == NULL)
	return NULL;
    return last_insert + last_insert_skip;
}

/*
 * Get last inserted string, and remove trailing <Esc>.
 * Returns pointer to allocated memory (must be freed) or NULL.
 */
    char_u *
get_last_insert_save()
{
    char_u	*s;
    int		len;

    if (last_insert == NULL)
	return NULL;
    s = vim_strsave(last_insert + last_insert_skip);
    if (s != NULL)
    {
	len = STRLEN(s);
	if (len > 0 && s[len - 1] == ESC)	/* remove trailing ESC */
	    s[len - 1] = NUL;
    }
    return s;
}

/*
 * Check the word in front of the cursor for an abbreviation.
 * Called when the non-id character "c" has been entered.
 * When an abbreviation is recognized it is removed from the text and
 * the replacement string is inserted in typebuf[], followed by "c".
 */
    static int
echeck_abbr(c)
    int c;
{
    /* Don't check for abbreviation in paste mode, when disabled and just
     * after moving around with cursor keys. */
    if (p_paste || no_abbr || arrow_used)
	return FALSE;

    return check_abbr(c, ml_get_curline(), curwin->w_cursor.col,
		curwin->w_cursor.lnum == Insstart.lnum ? Insstart.col : 0);
}

/*
 * replace-stack functions
 *
 * When replacing characters, the replaced characters are remembered for each
 * new character.  This is used to re-insert the old text when backspacing.
 *
 * There is a NUL headed list of characters for each character that is
 * currently in the file after the insertion point.  When BS is used, one NUL
 * headed list is put back for the deleted character.
 *
 * For a newline, there are two NUL headed lists.  One contains the characters
 * that the NL replaced.  The extra one stores the characters after the cursor
 * that were deleted (always white space).
 *
 * Replace_offset is normally 0, in which case replace_push will add a new
 * character at the end of the stack.  If replace_offset is not 0, that many
 * characters will be left on the stack above the newly inserted character.
 */

char_u	*replace_stack = NULL;
long	replace_stack_nr = 0;	    /* next entry in replace stack */
long	replace_stack_len = 0;	    /* max. number of entries */

    void
replace_push(c)
    int	    c;	    /* character that is replaced (NUL is none) */
{
    char_u  *p;

    if (replace_stack_nr < replace_offset)	/* nothing to do */
	return;
    if (replace_stack_len <= replace_stack_nr)
    {
	replace_stack_len += 50;
	p = lalloc(sizeof(char_u) * replace_stack_len, TRUE);
	if (p == NULL)	    /* out of memory */
	{
	    replace_stack_len -= 50;
	    return;
	}
	if (replace_stack != NULL)
	{
	    mch_memmove(p, replace_stack,
				 (size_t)(replace_stack_nr * sizeof(char_u)));
	    vim_free(replace_stack);
	}
	replace_stack = p;
    }
    p = replace_stack + replace_stack_nr - replace_offset;
    if (replace_offset)
	mch_memmove(p + 1, p, (size_t)(replace_offset * sizeof(char_u)));
    *p = c;
    ++replace_stack_nr;
}

/*
 * call replace_push(c) with replace_offset set to the first NUL.
 */
    static void
replace_push_off(c)
    int	    c;
{
    char_u	*p;

    p = replace_stack + replace_stack_nr;
    for (replace_offset = 1; replace_offset < replace_stack_nr;
							     ++replace_offset)
	if (*--p == NUL)
	    break;
    replace_push(c);
    replace_offset = 0;
}

/*
 * Pop one item from the replace stack.
 * return -1 if stack empty
 * return replaced character or NUL otherwise
 */
    static int
replace_pop()
{
    vr_virtcol = MAXCOL;
    if (replace_stack_nr == 0)
	return -1;
    return (int)replace_stack[--replace_stack_nr];
}

/*
 * Join the top two items on the replace stack.  This removes to "off"'th NUL
 * encountered.
 */
    static void
replace_join(off)
    int	    off;	/* offset for which NUL to remove */
{
    int	    i;

    for (i = replace_stack_nr; --i >= 0; )
	if (replace_stack[i] == NUL && off-- <= 0)
	{
	    --replace_stack_nr;
	    mch_memmove(replace_stack + i, replace_stack + i + 1,
					      (size_t)(replace_stack_nr - i));
	    return;
	}
}

/*
 * Pop characters from the replace stack until a NUL is found, and insert them
 * before the cursor.  Can only be used in REPLACE or VREPLACE mode.
 */
    static void
replace_pop_ins()
{
    int	    cc;
    int	    oldState = State;

    State = NORMAL;			/* don't want REPLACE here */
    while ((cc = replace_pop()) > 0)
    {
	ins_char(cc);
	dec_cursor();
    }
    State = oldState;
}

/*
 * make the replace stack empty
 * (called when exiting replace mode)
 */
    static void
replace_flush()
{
    vim_free(replace_stack);
    replace_stack = NULL;
    replace_stack_len = 0;
    replace_stack_nr = 0;
}

/*
 * Handle doing a BS for one character.
 * cc < 0: replace stack empty, just move cursor
 * cc == 0: character was inserted, delete it
 * cc > 0: character was replace, put original back
 */
    static void
replace_do_bs()
{
    int	    cc;

    cc = replace_pop();
    if (cc > 0)
    {
	pchar_cursor(cc);
	replace_pop_ins();
    }
    else if (cc == 0)
	(void)del_char(FALSE);
}

/*
 * Returns the virtcol of the text that's been replaced so far on this line.
 */
    int
get_replace_stack_virtcol()
{
    int		col = curwin->w_cursor.col;
    colnr_t	vcol = 0;
    int		i;
    FPOS	pos;

    /* First, find the character that was pushed at the start of the line */
    i = replace_stack_nr;
    while (i >= 0 && col--)
	while (--i >= 0 && replace_stack[i] != NUL)
	    ;

    /* If col is not less than zero, then we ran out of replace stack.  This
     * means that replacing must have started earlier on this line.  Initialise
     * vcol with the width up to the starting col.
     */
    if (col >= 0)
    {
	pos.lnum = curwin->w_cursor.lnum;
	pos.col = col + 1;
	getvcol(curwin, &pos, NULL, &vcol, NULL);
	i++;
    }

    /* Now add up virtcol from slot i onwards */
    for (; i < replace_stack_nr; i++)
    {
	if (replace_stack[i] != NUL)	/* Skip NULs */
	    vcol += chartabsize(replace_stack[i], vcol);
    }
    return (int)vcol;
}

#if defined(LISPINDENT) || defined(CINDENT) || defined(PROTO)
/*
 * Re-indent the current line, based on the current contents of it and the
 * surrounding lines. Fixing the cursor position seems really easy -- I'm very
 * confused what all the part that handles Control-T is doing that I'm not.
 * "get_the_indent" should be get_c_indent or get_lisp_indent.
 */

    void
fixthisline(get_the_indent)
    int (*get_the_indent) __ARGS((void));
{
    change_indent(INDENT_SET, get_the_indent(), FALSE, 0);
    if (linewhite(curwin->w_cursor.lnum))
	did_ai = TRUE;	    /* delete the indent if the line stays empty */
}
#endif /* defined(LISPINDENT) || defined(CINDENT) */

#ifdef CINDENT
/*
 * return TRUE if 'cinkeys' contains the key "keytyped",
 * when == '*':	    Only if key is preceded with '*'	(indent before insert)
 * when == '!':	    Only if key is prededed with '!'	(don't insert)
 * when == ' ':	    Only if key is not preceded with '*'(indent afterwards)
 *
 * If line_is_empty is TRUE accept keys with '0' before them.
 */
    int
in_cinkeys(keytyped, when, line_is_empty)
    int		keytyped;
    int		when;
    int		line_is_empty;
{
    char_u  *look;
    int	    try_match;
    char_u  *p;
    int	    i;

    for (look = curbuf->b_p_cink; *look; )
    {
	/*
	 * Find out if we want to try a match with this key, depending on
	 * 'when' and a '*' or '!' before the key.
	 */
	switch (when)
	{
	    case '*': try_match = (*look == '*'); break;
	    case '!': try_match = (*look == '!'); break;
	     default: try_match = (*look != '*'); break;
	}
	if (*look == '*' || *look == '!')
	    ++look;

	/*
	 * If there is a '0', only accept a match if the line is empty.
	 */
	if (*look == '0')
	{
	    if (!line_is_empty)
		try_match = FALSE;
	    ++look;
	}

	/*
	 * does it look like a control character?
	 */
	if (*look == '^' && look[1] >= '@' && look[1] <= '_')
	{
	    if (try_match && keytyped == Ctrl(look[1]))
		return TRUE;
	    look += 2;
	}
	/*
	 * 'o' means "o" command, open forward.
	 * 'O' means "O" command, open backward.
	 */
	else if (*look == 'o')
	{
	    if (try_match && keytyped == KEY_OPEN_FORW)
		return TRUE;
	    ++look;
	}
	else if (*look == 'O')
	{
	    if (try_match && keytyped == KEY_OPEN_BACK)
		return TRUE;
	    ++look;
	}

	/*
	 * 'e' means to check for "else" at start of line and just before the
	 * cursor.
	 */
	else if (*look == 'e')
	{
	    if (try_match && keytyped == 'e' && curwin->w_cursor.col >= 4)
	    {
		p = ml_get_curline();
		if (skipwhite(p) == p + curwin->w_cursor.col - 4 &&
			STRNCMP(p + curwin->w_cursor.col - 4, "else", 4) == 0)
		    return TRUE;
	    }
	    ++look;
	}

	/*
	 * ':' only causes an indent if it is at the end of a label or case
	 * statement, or when it was before typing the ':' (to fix
	 * class::method for C++).
	 */
	else if (*look == ':')
	{
	    if (try_match && keytyped == ':')
	    {
		p = ml_get_curline();
		if (cin_iscase(p) || cin_isscopedecl(p) || cin_islabel(30))
		    return TRUE;
		if (curwin->w_cursor.col > 2
			&& p[curwin->w_cursor.col - 1] == ':'
			&& p[curwin->w_cursor.col - 2] == ':')
		{
		    p[curwin->w_cursor.col - 1] = ' ';
		    i = (cin_iscase(p) || cin_isscopedecl(p)
							  || cin_islabel(30));
		    p = ml_get_curline();
		    p[curwin->w_cursor.col - 1] = ':';
		    if (i)
			return TRUE;
		}
	    }
	    ++look;
	}


	/*
	 * Is it a key in <>, maybe?
	 */
	else if (*look == '<')
	{
	    if (try_match)
	    {
		/*
		 * make up some named keys <o>, <O>, <e>, <0>, <>>, <<>, <*>
		 * and <!> so that people can re-indent on o, O, e, 0, <, >, *
		 * and ! keys if they really really want to.
		 */
		if (vim_strchr((char_u *)"<>!*oOe0", look[1]) != NULL &&
							  keytyped == look[1])
		    return TRUE;

		if (keytyped == get_special_key_code(look + 1))
		    return TRUE;
	    }
	    while (*look && *look != '>')
		look++;
	    while (*look == '>')
		look++;
	}

	/*
	 * ok, it's a boring generic character.
	 */
	else
	{
	    if (try_match && *look == keytyped)
		return TRUE;
	    ++look;
	}

	/*
	 * Skip over ", ".
	 */
	look = skip_to_option_part(look);
    }
    return FALSE;
}
#endif /* CINDENT */

#if defined(RIGHTLEFT) || defined(PROTO)
/*
 * Map Hebrew keyboard when in hkmap mode.
 */
    int
hkmap(c)
    int c;
{
    if (p_hkmapp)   /* phonetic mapping, by Ilya Dogolazky */
    {
	enum {hALEF=0, BET, GIMEL, DALET, HEI, VAV, ZAIN, HET, TET, IUD,
	    KAFsofit, hKAF, LAMED, MEMsofit, MEM, NUNsofit, NUN, SAMEH, AIN,
	    PEIsofit, PEI, ZADIsofit, ZADI, KOF, RESH, hSHIN, TAV};
	static char_u map[26] =
	    {(char_u)hALEF/*a*/, (char_u)BET  /*b*/, (char_u)hKAF    /*c*/,
	     (char_u)DALET/*d*/, (char_u)-1   /*e*/, (char_u)PEIsofit/*f*/,
	     (char_u)GIMEL/*g*/, (char_u)HEI  /*h*/, (char_u)IUD     /*i*/,
	     (char_u)HET  /*j*/, (char_u)KOF  /*k*/, (char_u)LAMED   /*l*/,
	     (char_u)MEM  /*m*/, (char_u)NUN  /*n*/, (char_u)SAMEH   /*o*/,
	     (char_u)PEI  /*p*/, (char_u)-1   /*q*/, (char_u)RESH    /*r*/,
	     (char_u)ZAIN /*s*/, (char_u)TAV  /*t*/, (char_u)TET     /*u*/,
	     (char_u)VAV  /*v*/, (char_u)hSHIN/*w*/, (char_u)-1      /*x*/,
	     (char_u)AIN  /*y*/, (char_u)ZADI /*z*/};

	if (c == 'N' || c == 'M' || c == 'P' || c == 'C' || c == 'Z')
	    return (int)(map[TO_LOWER(c) - 'a'] - 1 + p_aleph);/* '-1'='sofit' */
	else if (c == 'x')
	    return 'X';
	else if (c == 'q')
	    return '\''; /* {geresh}={'} */
	else if (c == 246)
	    return ' ';  /* \"o --> ' ' for a german keyboard */
	else if (c == 228)
	    return ' ';  /* \"a --> ' '      -- / --	       */
	else if (c == 252)
	    return ' ';  /* \"u --> ' '      -- / --	       */
	else if ('a' <= c && c <= 'z')
	    return (int)(map[c - 'a'] + p_aleph);
	else
	    return c;
    }
    else
    {
	switch (c)
	{
	    case '`':	return ';';
	    case '/':	return '.';
	    case '\'':	return ',';
	    case 'q':	return '/';
	    case 'w':	return '\'';

			/* Hebrew letters - set offset from 'a' */
	    case ',':	c = '{'; break;
	    case '.':	c = 'v'; break;
	    case ';':	c = 't'; break;
	    default: {
			 static char str[] = "zqbcxlsjphmkwonu ydafe rig";

			 if (c < 'a' || c > 'z')
			     return c;
			 c = str[c - 'a'];
			 break;
		     }
	}

	return (int)(c - 'a' + p_aleph);
    }
}
#endif

    static int
ins_reg()
{
    int	    need_redraw = FALSE;
    int	    regname;
    int	    literally = 0;

    /*
     * If we are going to wait for a character, show a '"'.
     */
    if (redrawing() && !char_avail())
    {
	edit_putchar('"', TRUE);
#ifdef CMDLINE_INFO
	add_to_showcmd_c(Ctrl('R'));
#endif
    }

#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif

    /*
     * Don't map the register name. This also prevents the mode message to be
     * deleted when ESC is hit.
     */
    ++no_mapping;
    regname = safe_vgetc();
    if (regname == Ctrl('R') || regname == Ctrl('O') || regname == Ctrl('P'))
    {
	/* Get a third key for literal register insertion */
	literally = regname;
#ifdef CMDLINE_INFO
	add_to_showcmd_c(literally);
#endif
	regname = safe_vgetc();
    }
    --no_mapping;
#ifdef HAVE_LANGMAP
    LANGMAP_ADJUST(regname, TRUE);
#endif

#ifdef WANT_EVAL
    /*
     * Don't call u_sync while getting the expression,
     * evaluating it or giving an error message for it!
     */
    ++no_u_sync;
    if (regname == '=')
	regname = get_expr_register();
    if (regname == NUL)
	need_redraw = TRUE;	/* remove the '"' */
    else
    {
#endif
	if (literally == Ctrl('O') || literally == Ctrl('P'))
	{
	    /* Append the command to the redo buffer. */
	    AppendCharToRedobuff(Ctrl('R'));
	    AppendCharToRedobuff(literally);
	    AppendCharToRedobuff(regname);

	    do_put(regname, BACKWARD, 1L,
		 (literally == Ctrl('P') ? PUT_FIXINDENT : 0) | PUT_CURSEND);
	}
	else if (insert_reg(regname, literally) == FAIL)
	{
	    vim_beep();
	    need_redraw = TRUE;	/* remove the '"' */
	}
#ifdef WANT_EVAL
    }
    --no_u_sync;
#endif
#ifdef CMDLINE_INFO
    clear_showcmd();
#endif

    /* If the inserted register is empty, we need to remove the '"' */
    if (stuff_empty())
	need_redraw = TRUE;

    return need_redraw;
}

/*
 * Handle ESC in insert mode.
 * Returns TRUE when leaving insert mode, FALSE when going to repeat the
 * insert.
 */
    static int
ins_esc(count, need_redraw, cmdchar)
    long	*count;
    int		need_redraw;
    int		cmdchar;
{
    int		 temp;
    static int	 disabled_redraw = FALSE;

#if defined(MULTI_BYTE_IME) && defined(USE_GUI_WIN32)
    ImeSetEnglishMode();
#endif
#if defined(HANGUL_INPUT)
# if defined(ESC_CHG_TO_ENG_MODE)
    hangul_input_state_set(0);
# endif
    if (composing_hangul)
    {
	push_raw_key(composing_hangul_buffer, 2);
	composing_hangul = 0;
    }
#endif
#if defined(MULTI_BYTE) && defined(macintosh)
    previous_script = GetScriptManagerVariable(smKeyScript);
    KeyScript(smKeyRoman); /* or smKeySysScript */
#endif

    temp = curwin->w_cursor.col;
    if (disabled_redraw)
    {
	--RedrawingDisabled;
	disabled_redraw = FALSE;
    }
    if (!arrow_used)
    {
	/*
	 * Don't append the ESC for "r<CR>".
	 */
	if (cmdchar != 'r' && cmdchar != 'v')
	    AppendToRedobuff(ESC_STR);

	/*
	 * Repeating insert may take a long time.  Check for
	 * interrupt now and then.
	 */
	if (*count)
	{
	    line_breakcheck();
	    if (got_int)
		*count = 0;
	}

	if (--*count > 0)	/* repeat what was typed */
	{
	    (void)start_redo_ins();
	    ++RedrawingDisabled;
	    disabled_redraw = TRUE;
	    return FALSE;	/* repeat the insert */
	}
	stop_insert(&curwin->w_cursor);
	if (dollar_vcol)
	{
	    dollar_vcol = 0;
	    /* may have to redraw status line if this was the
	     * first change, show "[+]" */
	    if (curwin->w_redr_status == TRUE)
		redraw_later(NOT_VALID);
	    else
		need_redraw = TRUE;
	}
    }
    if (need_redraw)
	update_screenline();

    /* When an autoindent was removed, curswant stays after the
     * indent */
    if (!restart_edit && (colnr_t)temp == curwin->w_cursor.col)
	curwin->w_set_curswant = TRUE;

    /*
     * The cursor should end up on the last inserted character.
     */
    if (       curwin->w_cursor.col != 0
	    && (!restart_edit
		|| (gchar_cursor() == NUL && !VIsual_active))
#ifdef RIGHTLEFT
	    && !revins_on
#endif
				      )
	--curwin->w_cursor.col;

    /* need to position cursor again (e.g. when on a TAB ) */
    changed_cline_bef_curs();

    State = NORMAL;
#ifdef USE_MOUSE
    setmouse();
#endif
#ifdef CURSOR_SHAPE
    ui_cursor_shape();		/* may show different cursor shape */
#endif

    /*
     * When recording or for CTRL-O, need to display the new mode.
     * Otherwise remove the mode message.
     */
    if (Recording || restart_edit)
	showmode();
    else if (p_smd)
	MSG("");

    return TRUE;	    /* exit Insert mode */
}

#ifdef RIGHTLEFT
/*
 * Toggle language: khmap and revins_on.
 * Move to end of reverse inserted text.
 */
    static void
ins_ctrl_()
{
    if (revins_on && revins_chars && revins_scol >= 0)
    {
	while (gchar_cursor() != NUL && revins_chars--)
	    ++curwin->w_cursor.col;
    }
    p_ri = !p_ri;
    revins_on = (State == INSERT && p_ri);
    if (revins_on)
    {
	revins_scol = curwin->w_cursor.col;
	revins_legal++;
	revins_chars = 0;
	undisplay_dollar();
    }
    else
	revins_scol = -1;
#ifdef FKMAP
    if (p_altkeymap)
    {
	/*
	 * to be consistent also for redo command, using '.'
	 * set arrow_used to true and stop it - causing to redo
	 * characters entered in one mode (normal/reverse insert).
	 */
	arrow_used = TRUE;
	stop_arrow();
	p_fkmap = curwin->w_p_rl ^ p_ri;
	if (p_fkmap && p_ri)
	    State = INSERT;
    }
    else
#endif
	p_hkmap = curwin->w_p_rl ^ p_ri;    /* be consistent! */
    showmode();
}
#endif

/*
 * If the cursor is on an indent, ^T/^D insert/delete one
 * shiftwidth.	Otherwise ^T/^D behave like a "<<" or ">>".
 * Always round the indent to 'shiftwith', this is compatible
 * with vi.  But vi only supports ^T and ^D after an
 * autoindent, we support it everywhere.
 */
    static void
ins_shift(c, lastc)
    int	    c;
    int	    lastc;
{
    stop_arrow();
    AppendCharToRedobuff(c);

    /*
     * 0^D and ^^D: remove all indent.
     */
    if ((lastc == '0' || lastc == '^') && curwin->w_cursor.col)
    {
	--curwin->w_cursor.col;
	(void)del_char(FALSE);		/* delete the '^' or '0' */
	/* In Replace mode, restore the characters that '^' or '0' replaced. */
	if (State == REPLACE || State == VREPLACE)
	    replace_pop_ins();
	if (lastc == '^')
	    old_indent = get_indent();	/* remember curr. indent */
	change_indent(INDENT_SET, 0, TRUE, 0);
    }
    else
	change_indent(c == Ctrl('D') ? INDENT_DEC : INDENT_INC, 0, TRUE, 0);

    did_ai = FALSE;
#ifdef SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
#ifdef CINDENT
    can_cindent = FALSE;	/* no cindenting after ^D or ^T */
#endif
}

    static void
ins_del()
{
    int	    temp;

    stop_arrow();
    if (gchar_cursor() == NUL)	/* delete newline */
    {
	temp = curwin->w_cursor.col;
	if (!can_bs(BS_EOL)		/* only if "eol" included */
		|| u_save((linenr_t)(curwin->w_cursor.lnum - 1),
		    (linenr_t)(curwin->w_cursor.lnum + 2)) == FAIL
		|| do_join(FALSE, TRUE) == FAIL)
	{
	    vim_beep();
	}
	else
	{
	    curwin->w_cursor.col = temp;
	    redraw_later(VALID_TO_CURSCHAR);
	}
    }
    else if (del_char(FALSE) == FAIL)/* delete char under cursor */
	vim_beep();
    did_ai = FALSE;
#ifdef SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
    AppendCharToRedobuff(K_DEL);
}

/*
 * Handle Backspace, delete-word and delete-line in Insert mode.
 * Return TRUE when backspace was actually used.
 */
    static int
ins_bs(c, mode, inserted_space_p)
    int		c;
    int		mode;
    int		*inserted_space_p;
{
    linenr_t	lnum;
    int		cc;
    int		temp = 0;	    /* init for GCC */
    colnr_t	mincol;
    int		did_backspace = FALSE;
    int		in_indent;
    int		oldState;

    /*
     * can't delete anything in an empty file
     * can't backup past first character in buffer
     * can't backup past starting point unless 'backspace' > 1
     * can backup to a previous line if 'backspace' == 0
     */
    if (       bufempty()
	    || (
#ifdef RIGHTLEFT
		!revins_on &&
#endif
		((curwin->w_cursor.lnum == 1 && curwin->w_cursor.col <= 0)
		    || (!can_bs(BS_START)
			&& (arrow_used
			    || (curwin->w_cursor.lnum == Insstart.lnum
				&& curwin->w_cursor.col <= Insstart.col)))
		    || (!can_bs(BS_INDENT) && !arrow_used && ai_col > 0
					 && curwin->w_cursor.col <= ai_col)
		    || (!can_bs(BS_EOL) && curwin->w_cursor.col == 0))))
    {
	vim_beep();
	return FALSE;
    }

    stop_arrow();
    in_indent = inindent(0);
#ifdef CINDENT
    if (in_indent)
	can_cindent = FALSE;
#endif
#ifdef COMMENTS
    end_comment_pending = NUL;	/* After BS, don't auto-end comment */
#endif
#ifdef RIGHTLEFT
    if (revins_on)	    /* put cursor after last inserted char */
	inc_cursor();
#endif

    /*
     * delete newline!
     */
    if (curwin->w_cursor.col <= 0)
    {
	lnum = Insstart.lnum;
	if (curwin->w_cursor.lnum == Insstart.lnum
#ifdef RIGHTLEFT
			|| revins_on
#endif
				    )
	{
	    if (u_save((linenr_t)(curwin->w_cursor.lnum - 2),
			       (linenr_t)(curwin->w_cursor.lnum + 1)) == FAIL)
		return FALSE;
	    --Insstart.lnum;
	    Insstart.col = MAXCOL;
	}
	/*
	 * In replace mode:
	 * cc < 0: NL was inserted, delete it
	 * cc >= 0: NL was replaced, put original characters back
	 */
	cc = -1;
	if (State == REPLACE || State == VREPLACE)
	    cc = replace_pop();	    /* returns -1 if NL was inserted */
	/*
	 * In replace mode, in the line we started replacing, we only move the
	 * cursor.
	 */
	if ((State == REPLACE || State == VREPLACE)
			&& curwin->w_cursor.lnum <= lnum)
	{
	    dec_cursor();
	}
	else
	{
	    if (State != VREPLACE
		|| curwin->w_cursor.lnum > orig_line_count)
	    {
		temp = gchar_cursor();	/* remember current char */
		--curwin->w_cursor.lnum;
		(void)do_join(FALSE, TRUE);
		redraw_later(VALID_TO_CURSCHAR);
		if (temp == NUL && gchar_cursor() != NUL)
		    ++curwin->w_cursor.col;
	    }
	    else
		dec_cursor();

	    /*
	     * In REPLACE mode we have to put back the text that was replace
	     * by the NL. On the replace stack is first a NUL-terminated
	     * sequence of characters that were deleted and then the
	     * characters that NL replaced.
	     */
	    if (State == REPLACE || State == VREPLACE)
	    {
		/*
		 * Do the next ins_char() in NORMAL state, to
		 * prevent ins_char() from replacing characters and
		 * avoiding showmatch().
		 */
		oldState = State;
		State = NORMAL;
		/*
		 * restore characters (blanks) deleted after cursor
		 */
		while (cc > 0)
		{
		    temp = curwin->w_cursor.col;
		    ins_char(cc);
		    curwin->w_cursor.col = temp;
		    cc = replace_pop();
		}
		/* restore the characters that NL replaced */
		replace_pop_ins();
		State = oldState;
	    }
	}
	did_ai = FALSE;
    }
    else
    {
	/*
	 * Delete character(s) before the cursor.
	 */
#ifdef RIGHTLEFT
	if (revins_on)		/* put cursor on last inserted char */
	    dec_cursor();
#endif
	mincol = 0;
						/* keep indent */
	if (mode == BACKSPACE_LINE && curbuf->b_p_ai
#ifdef RIGHTLEFT
		&& !revins_on
#endif
			    )
	{
	    temp = curwin->w_cursor.col;
	    beginline(BL_WHITE);
	    if (curwin->w_cursor.col < (colnr_t)temp)
		mincol = curwin->w_cursor.col;
	    curwin->w_cursor.col = temp;
	}

	/*
	 * Handle deleting one 'shiftwidth' or 'softtabstop'.
	 */
	if (	   mode == BACKSPACE_CHAR
		&& ((p_sta && in_indent)
		    || (curbuf->b_p_sts
			&& (*(ml_get_cursor() - 1) == TAB
			    || (*(ml_get_cursor() - 1) == ' '
				&& (!*inserted_space_p
				    || arrow_used))))))
	{
	    int		ts;
	    colnr_t	vcol;
	    int		want_vcol;
	    int		extra = 0;

	    *inserted_space_p = FALSE;
	    if (p_sta)
		ts = curbuf->b_p_sw;
	    else
		ts = curbuf->b_p_sts;
	    /* compute the virtual column where we want to be */
	    getvcol(curwin, &curwin->w_cursor, &vcol, NULL, NULL);
	    want_vcol = ((vcol - 1) / ts) * ts;
	    /* delete characters until we are at or before want_vcol */
	    while ((int)vcol > want_vcol
		    && (cc = *(ml_get_cursor() - 1), vim_iswhite(cc)))
	    {
		dec_cursor();
		/* TODO: calling getvcol() each time is slow */
		getvcol(curwin, &curwin->w_cursor, &vcol, NULL, NULL);
		if (State == REPLACE || State == VREPLACE)
		{
		    /* Don't delete characters before the insert point when in
		     * Replace mode */
		    if (curwin->w_cursor.lnum != Insstart.lnum
			|| curwin->w_cursor.col >= Insstart.col)
		    {
#if 0	/* what was this for?  It causes problems when sw != ts. */
			if (State == REPLACE && (int)vcol < want_vcol)
			{
			    (void)del_char(FALSE);
			    extra = 2;	/* don't pop too much */
			}
			else
#endif
			    replace_do_bs();
		    }
		}
		else
		    (void)del_char(FALSE);
	    }

	    /* insert extra spaces until we are at want_vcol */
	    while ((int)vcol < want_vcol)
	    {
		/* Remember the first char we inserted */
		if (curwin->w_cursor.lnum == Insstart.lnum
				   && curwin->w_cursor.col < Insstart.col)
		    Insstart.col = curwin->w_cursor.col;

		if (State == VREPLACE)
		    ins_char(' ');
		else
		{
		    ins_str((char_u *)" ");
		    if (State == REPLACE && extra <= 1)
		    {
			if (extra)
			    replace_push_off(NUL);
			else
			    replace_push(NUL);
		    }
		    if (extra == 2)
			extra = 1;
		}
		vcol++;
	    }
	}

	/*
	 * Delete upto starting point, start of line or previous word.
	 */
	else do
	{
#ifdef RIGHTLEFT
	    if (!revins_on) /* put cursor on char to be deleted */
#endif
		dec_cursor();

	    /* start of word? */
	    if (mode == BACKSPACE_WORD && !vim_isspace(gchar_cursor()))
	    {
		mode = BACKSPACE_WORD_NOT_SPACE;
		temp = vim_iswordc(gchar_cursor());
	    }
	    /* end of word? */
	    else if (mode == BACKSPACE_WORD_NOT_SPACE
		    && (vim_isspace(cc = gchar_cursor())
			    || vim_iswordc(cc) != temp))
	    {
#ifdef RIGHTLEFT
		if (!revins_on)
#endif
		    inc_cursor();
#ifdef RIGHTLEFT
		else if (State == REPLACE || State == VREPLACE)
		    dec_cursor();
#endif
		break;
	    }
	    if (State == REPLACE || State == VREPLACE)
		replace_do_bs();
	    else  /* State != REPLACE && State != VREPLACE */
	    {
		(void)del_char(FALSE);
#ifdef RIGHTLEFT
		if (revins_chars)
		{
		    revins_chars--;
		    revins_legal++;
		}
		if (revins_on && gchar_cursor() == NUL)
		    break;
#endif
	    }
	    /* Just a single backspace?: */
	    if (mode == BACKSPACE_CHAR)
		break;
	} while (
#ifdef RIGHTLEFT
		revins_on ||
#endif
		(curwin->w_cursor.col > mincol
		 && (curwin->w_cursor.lnum != Insstart.lnum
		     || curwin->w_cursor.col != Insstart.col)));
	did_backspace = TRUE;
    }
#ifdef SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
    if (curwin->w_cursor.col <= 1)
	did_ai = FALSE;
    /*
     * It's a little strange to put backspaces into the redo
     * buffer, but it makes auto-indent a lot easier to deal
     * with.
     */
    AppendCharToRedobuff(c);

    /* If deleted before the insertion point, adjust it */
    if (curwin->w_cursor.lnum == Insstart.lnum
				       && curwin->w_cursor.col < Insstart.col)
	Insstart.col = curwin->w_cursor.col;

    return did_backspace;
}

#ifdef USE_MOUSE
    static void
ins_mouse(c)
    int	    c;
{
    FPOS	tpos;

# ifdef USE_GUI
    /* When GUI is active, also move/paste when 'mouse' is empty */
    if (!gui.in_use)
# endif
	if (!mouse_has(MOUSE_INSERT))
	    return;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (do_mouse(NULL, c, BACKWARD, 1L, 0))
    {
	start_arrow(&tpos);
# ifdef CINDENT
	can_cindent = TRUE;
# endif
    }

    /* redraw status lines (in case another window became active) */
    redraw_statuslines();
}

    static void
ins_mousescroll(up)
    int		up;
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (mod_mask & MOD_MASK_SHIFT)
	scroll_redraw(up, (long)(curwin->w_botline - curwin->w_topline));
    else
	scroll_redraw(up, 3L);
    if (!equal(curwin->w_cursor, tpos))
    {
	start_arrow(&tpos);
# ifdef CINDENT
	can_cindent = TRUE;
# endif
    }
}
#endif

#ifdef USE_GUI
    void
ins_scroll()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (gui_do_scroll())
    {
	start_arrow(&tpos);
# ifdef CINDENT
	can_cindent = TRUE;
# endif
    }
}

    void
ins_horscroll()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (gui_do_horiz_scroll())
    {
	start_arrow(&tpos);
# ifdef CINDENT
	can_cindent = TRUE;
# endif
    }
}
#endif

    static void
ins_left()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (oneleft() == OK)
    {
	start_arrow(&tpos);
#ifdef RIGHTLEFT
	/* If exit reversed string, position is fixed */
	if (revins_scol != -1 && (int)curwin->w_cursor.col >= revins_scol)
	    revins_legal++;
	revins_chars++;
#endif
    }

    /*
     * if 'whichwrap' set for cursor in insert mode may go to
     * previous line
     */
    else if (vim_strchr(p_ww, '[') != NULL && curwin->w_cursor.lnum > 1)
    {
	start_arrow(&tpos);
	--(curwin->w_cursor.lnum);
	coladvance(MAXCOL);
	curwin->w_set_curswant = TRUE;	/* so we stay at the end */
    }
    else
	vim_beep();
}

    static void
ins_home()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if ((mod_mask & MOD_MASK_CTRL))
	curwin->w_cursor.lnum = 1;
    curwin->w_cursor.col = 0;
    curwin->w_curswant = 0;
    start_arrow(&tpos);
}

    static void
ins_end()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if ((mod_mask & MOD_MASK_CTRL))
	curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    coladvance(MAXCOL);
    curwin->w_curswant = MAXCOL;
    start_arrow(&tpos);
}

    static void
ins_s_left()
{
    undisplay_dollar();
    if (curwin->w_cursor.lnum > 1 || curwin->w_cursor.col > 0)
    {
	start_arrow(&curwin->w_cursor);
	(void)bck_word(1L, 0, FALSE);
	curwin->w_set_curswant = TRUE;
    }
    else
	vim_beep();
}

    static void
ins_right()
{
    undisplay_dollar();
    if (gchar_cursor() != NUL)
    {
	start_arrow(&curwin->w_cursor);
#ifdef MULTI_BYTE
	if (is_dbcs)
	{
	    char_u *p = ml_get_pos(&curwin->w_cursor);

	    if (IsLeadByte(p[0]) && p[1] != NUL)
		++curwin->w_cursor.col;
	}
#endif
	curwin->w_set_curswant = TRUE;
	++curwin->w_cursor.col;
#ifdef RIGHTLEFT
	revins_legal++;
	if (revins_chars)
	    revins_chars--;
#endif
    }
    /* if 'whichwrap' set for cursor in insert mode, may move the
     * cursor to the next line */
    else if (vim_strchr(p_ww, ']') != NULL
	    && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
    {
	start_arrow(&curwin->w_cursor);
	curwin->w_set_curswant = TRUE;
	++curwin->w_cursor.lnum;
	curwin->w_cursor.col = 0;
    }
    else
	vim_beep();
}

    static void
ins_s_right()
{
    undisplay_dollar();
    if (curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count
	    || gchar_cursor() != NUL)
    {
	start_arrow(&curwin->w_cursor);
	(void)fwd_word(1L, 0, 0);
	curwin->w_set_curswant = TRUE;
    }
    else
	vim_beep();
}

    static void
ins_up()
{
    FPOS	tpos;
    linenr_t	old_topline;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    old_topline = curwin->w_topline;
    if (cursor_up(1L, TRUE) == OK)
    {
	if (old_topline != curwin->w_topline)
	    update_screen(VALID);
	start_arrow(&tpos);
#ifdef CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

    static void
ins_pageup()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (onepage(BACKWARD, 1L) == OK)
    {
	start_arrow(&tpos);
#ifdef CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

    static void
ins_down()
{
    FPOS	tpos;
    linenr_t	old_topline = curwin->w_topline;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (cursor_down(1L, TRUE) == OK)
    {
	if (old_topline != curwin->w_topline)
	    update_screen(VALID);
	start_arrow(&tpos);
#ifdef CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

    static void
ins_pagedown()
{
    FPOS	tpos;

    undisplay_dollar();
    tpos = curwin->w_cursor;
    if (onepage(FORWARD, 1L) == OK)
    {
	start_arrow(&tpos);
#ifdef CINDENT
	can_cindent = TRUE;
#endif
    }
    else
	vim_beep();
}

/*
 * Handle TAB in Insert or Replace mode.
 * Return TRUE when the TAB needs to be inserted like a normal character.
 */
    static int
ins_tab()
{
    int		ind;
    int		i;
    int		temp;

    if (Insstart_blank_vcol == MAXCOL && curwin->w_cursor.lnum == Insstart.lnum)
	Insstart_blank_vcol = get_nolist_virtcol();
    if (echeck_abbr(TAB + ABBR_OFF))
	return FALSE;

    ind = inindent(0);
#ifdef CINDENT
    if (ind)
	can_cindent = FALSE;
#endif

    /*
     * When nothing special, insert TAB like a normal character
     */
    if (!curbuf->b_p_et
	    && !(p_sta && ind && curbuf->b_p_ts != curbuf->b_p_sw)
	    && !curbuf->b_p_sts)
	return TRUE;

    stop_arrow();
    did_ai = FALSE;
#ifdef SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif
    AppendToRedobuff((char_u *)"\t");

    if (p_sta && ind)		/* insert tab in indent, use 'shiftwidth' */
	temp = (int)curbuf->b_p_sw;
    else if (curbuf->b_p_sts)	/* use 'softtabstop' when set */
	temp = (int)curbuf->b_p_sts;
    else			/* otherwise use 'tabstop' */
	temp = (int)curbuf->b_p_ts;
    temp -= get_nolist_virtcol() % temp;

    /*
     * Insert the first space with ins_char().	It will delete one char in
     * replace mode.  Insert the rest with ins_str(); it will not delete any
     * chars.  For VREPLACE mode, we use ins_char() for all characters.
     */
    ins_char(' ');
    while (--temp)
    {
	if (State == VREPLACE)
	    ins_char(' ');
	else
	{
	    ins_str((char_u *)" ");
	    if (State == REPLACE)	    /* no char replaced */
		replace_push(NUL);
	}
    }

    /*
     * When 'expandtab' not set: Replace spaces by TABs where possible.
     */
    if (!curbuf->b_p_et && (curbuf->b_p_sts || (p_sta && ind)))
    {
	char_u		*ptr, *saved_line = NULL;	/* init for GCC */
	FPOS		fpos, pos;
	FPOS		*cursor;
	colnr_t		want_vcol, vcol, tab_vcol;
	int		change_col = -1;
	int		ts = curbuf->b_p_ts;

	/*
	 * Get the current line.  For VREPLACE mode, don't make real changes
	 * yet, just work on a copy of the line.
	 */
	if (State == VREPLACE)
	{
	    pos = curwin->w_cursor;
	    cursor = &pos;
	    saved_line = vim_strsave(ml_get_curline());
	    if (saved_line == NULL)
		return FALSE;
	    ptr = saved_line + pos.col;
	}
	else
	{
	    ptr = ml_get_cursor();
	    cursor = &curwin->w_cursor;
	}

	/* Find first white before the cursor */
	fpos = curwin->w_cursor;
	while (fpos.col > 0 && vim_iswhite(ptr[-1]))
	{
	    --fpos.col;
	    --ptr;
	}

	/* In Replace mode, don't change characters before the insert point. */
	if ((State == REPLACE || State == VREPLACE)
		&& fpos.lnum == Insstart.lnum
		&& fpos.col < Insstart.col)
	{
	    ptr += Insstart.col - fpos.col;
	    fpos.col = Insstart.col;
	}

	/* compute virtual column numbers of first white and cursor */
	getvcol(curwin, &fpos, &vcol, NULL, NULL);
	getvcol(curwin, cursor, &want_vcol, NULL, NULL);

	/* use as many TABs as possible */
	tab_vcol = (want_vcol / ts) * ts;
	while (vcol < tab_vcol)
	{
	    if (*ptr != TAB)
	    {
		*ptr = TAB;
		if (change_col < 0)
		{
		    change_col = fpos.col;  /* Column of first change */
		    /* May have to adjust Insstart */
		    if (fpos.lnum == Insstart.lnum && fpos.col < Insstart.col)
			Insstart.col = fpos.col;
		}
	    }
	    ++fpos.col;
	    ++ptr;
	    vcol = ((vcol + ts) / ts) * ts;
	}

	if (change_col >= 0)
	{
	    /* may need to delete a number of the following spaces */
	    i = want_vcol - vcol;
	    ptr += i;
	    fpos.col += i;
	    i = cursor->col - fpos.col;
	    if (i > 0)
	    {
		mch_memmove(ptr, ptr + i, STRLEN(ptr + i) + 1);
		/* correct replace stack. */
		if (State == REPLACE)
		    for (temp = i; --temp >= 0; )
			replace_join(want_vcol - vcol);
	    }
	    cursor->col -= i;

	    /*
	     * In VREPLACE mode, we haven't changed anything yet.  Do it now by
	     * backspacing over the changed spacing and then inserting the new
	     * spacing.
	     */
	    if (State == VREPLACE)
	    {
		/* Backspace from real cursor to change_col */
		backspace_until_column(change_col);

		/* Insert each char in saved_line from changed_col to
		 * ptr-cursor */
		while (change_col < (int)cursor->col)
		    ins_char(saved_line[change_col++]);
	    }
	}

	if (State == VREPLACE)
	    vim_free(saved_line);
    }

    return FALSE;
}

/*
 * Handle CR or NL in insert mode.
 * Return TRUE when out of memory.
 */
    static int
ins_eol(c)
    int		c;
{
    int	    i;

    if (echeck_abbr(c + ABBR_OFF))
	return FALSE;
    stop_arrow();
    /*
     * Strange Vi behaviour: In Replace mode, typing a NL will not delete the
     * character under the cursor.  Only push a NUL on the replace stack,
     * nothing to put back when the NL is deleted.
     */
    if (State == REPLACE)
	replace_push(NUL);

    /*
     * In VREPLACE mode, a NL replaces the rest of the line, and starts
     * replacing the next line, so we push all of the characters left on the
     * line onto the replace stack.  This is not done here though, it is done
     * in open_line().
     */

#ifdef RIGHTLEFT
# ifdef FKMAP
    if (p_altkeymap && p_fkmap)
	fkmap(NL);
# endif
    /* NL in reverse insert will always start in the end of
     * current line. */
    if (revins_on)
	while (gchar_cursor() != NUL)
	    ++curwin->w_cursor.col;
#endif

    AppendToRedobuff(NL_STR);
#ifdef COMMENTS
    if (has_format_option(FO_RET_COMS))
	fo_do_comments = TRUE;
#endif
    i = open_line(FORWARD, TRUE, FALSE, old_indent);
    old_indent = 0;
#ifdef COMMENTS
    fo_do_comments = FALSE;
#endif
#ifdef CINDENT
    can_cindent = TRUE;
#endif

    return (!i);
}

#ifdef DIGRAPHS
/*
 * Handle digraph in insert mode.
 * Returns character still to be inserted, or NUL when nothing remaining to be
 * done.
 */
    static int
ins_digraph()
{
    int	    c;
    int	    cc;

    if (redrawing() && !char_avail())
    {
	edit_putchar('?', TRUE);
#ifdef CMDLINE_INFO
	add_to_showcmd_c(Ctrl('K'));
#endif
    }

#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif

    /* don't map the digraph chars. This also prevents the
     * mode message to be deleted when ESC is hit */
    ++no_mapping;
    ++allow_keys;
    c = safe_vgetc();
    --no_mapping;
    --allow_keys;
    if (IS_SPECIAL(c) || mod_mask)	    /* special key */
    {
#ifdef CMDLINE_INFO
	clear_showcmd();
#endif
	insert_special(c, TRUE, FALSE);
	return NUL;
    }
    if (c != ESC)
    {
	if (redrawing() && !char_avail())
	{
	    if (charsize(c) == 1)
		edit_putchar(c, TRUE);
#ifdef CMDLINE_INFO
	    add_to_showcmd_c(c);
#endif
	}
	++no_mapping;
	++allow_keys;
	cc = safe_vgetc();
	--no_mapping;
	--allow_keys;
	if (cc != ESC)
	{
	    AppendToRedobuff((char_u *)"\026");	/* CTRL-V */
	    c = getdigraph(c, cc, TRUE);
#ifdef CMDLINE_INFO
	    clear_showcmd();
#endif
	    return c;
	}
    }
#ifdef CMDLINE_INFO
    clear_showcmd();
#endif
    return NUL;
}
#endif

/*
 * Handle CTRL-E and CTRL-Y in Insert mode: copy char from other line.
 * Returns the char to be inserted, or NUL if none found.
 */
    static int
ins_copychar(lnum)
    linenr_t	lnum;
{
    int	    c;
    int	    temp;
    char_u  *ptr;

    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
    {
	vim_beep();
	return NUL;
    }

    /* try to advance to the cursor column */
    temp = 0;
    ptr = ml_get(lnum);
    validate_virtcol();
    while ((colnr_t)temp < curwin->w_virtcol && *ptr)
	temp += lbr_chartabsize(ptr++, (colnr_t)temp);

    if ((colnr_t)temp > curwin->w_virtcol)
	--ptr;
    if ((c = *ptr) == NUL)
	vim_beep();
    return c;
}

#ifdef SMARTINDENT
/*
 * Try to do some very smart auto-indenting.
 * Used when inserting a "normal" character.
 */
    static void
ins_try_si(c)
    int	    c;
{
    FPOS	*pos, old_pos;
    char_u	*ptr;
    int		i;
    int		temp;

    /*
     * do some very smart indenting when entering '{' or '}'
     */
    if (((did_si || can_si_back) && c == '{') || (can_si && c == '}'))
    {
	/*
	 * for '}' set indent equal to indent of line containing matching '{'
	 */
	if (c == '}' && (pos = findmatch(NULL, '{')) != NULL)
	{
	    old_pos = curwin->w_cursor;
	    /*
	     * If the matching '{' has a ')' immediately before it (ignoring
	     * white-space), then line up with the start of the line
	     * containing the matching '(' if there is one.  This handles the
	     * case where an "if (..\n..) {" statement continues over multiple
	     * lines -- webb
	     */
	    ptr = ml_get(pos->lnum);
	    i = pos->col;
	    if (i > 0)		/* skip blanks before '{' */
		while (--i > 0 && vim_iswhite(ptr[i]))
		    ;
	    curwin->w_cursor.lnum = pos->lnum;
	    curwin->w_cursor.col = i;
	    if (ptr[i] == ')' && (pos = findmatch(NULL, '(')) != NULL)
		curwin->w_cursor = *pos;
	    i = get_indent();
	    curwin->w_cursor = old_pos;
	    if (State == VREPLACE)
		change_indent(INDENT_SET, i, FALSE, NUL);
	    else
		set_indent(i, TRUE);
	}
	else if (curwin->w_cursor.col > 0)
	{
	    /*
	     * when inserting '{' after "O" reduce indent, but not
	     * more than indent of previous line
	     */
	    temp = TRUE;
	    if (c == '{' && can_si_back && curwin->w_cursor.lnum > 1)
	    {
		old_pos = curwin->w_cursor;
		i = get_indent();
		while (curwin->w_cursor.lnum > 1)
		{
		    ptr = skipwhite(ml_get(--(curwin->w_cursor.lnum)));

		    /* ignore empty lines and lines starting with '#'. */
		    if (*ptr != '#' && *ptr != NUL)
			break;
		}
		if (get_indent() >= i)
		    temp = FALSE;
		curwin->w_cursor = old_pos;
	    }
	    if (temp)
		shift_line(TRUE, FALSE, 1);
	}
    }

    /*
     * set indent of '#' always to 0
     */
    if (curwin->w_cursor.col > 0 && can_si && c == '#')
    {
	/* remember current indent for next line */
	old_indent = get_indent();
	set_indent(0, TRUE);
    }

    /* Adjust ai_col, the char at this position can be deleted. */
    if (ai_col > curwin->w_cursor.col)
	ai_col = curwin->w_cursor.col;
}
#endif

/*
 * Get the value that w_virtcol would have when 'list' is off.
 * Unless 'cpo' contains the 'L' flag.
 */
    static colnr_t
get_nolist_virtcol()
{
    colnr_t	virtcol;

    if (curwin->w_p_list && vim_strchr(p_cpo, CPO_LISTWM) == NULL)
    {
	curwin->w_p_list = FALSE;
	getvcol(curwin, &curwin->w_cursor, NULL, &virtcol, NULL);
	curwin->w_p_list = TRUE;
	return virtcol;
    }
    validate_virtcol();
    return curwin->w_virtcol;
}
