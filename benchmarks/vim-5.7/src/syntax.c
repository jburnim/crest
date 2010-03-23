/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * syntax.c: code for syntax highlighting
 */

#include "vim.h"

/*
 * Structure that stores information about a highlight group.
 * The ID of a highlight group is also called group ID.  It is the index in
 * the highlight_ga array PLUS ONE.
 */
struct hl_group
{
    char_u	*sg_name;	/* highlight group name */
    char_u	*sg_name_u;	/* uppercase of sg_name */
/* for normal terminals */
    int		sg_term;	/* "term=" highlighting attributes */
    char_u	*sg_start;	/* terminal string for start highl */
    char_u	*sg_stop;	/* terminal string for stop highl */
    int		sg_term_attr;	/* NextScreen attr for term mode */
/* for color terminals */
    int		sg_cterm;	/* "cterm=" highlighting attr */
    int		sg_cterm_bold;	/* bold attr was set for light color */
    int		sg_cterm_fg;	/* terminal fg color number + 1 */
    int		sg_cterm_bg;	/* terminal bg color number + 1 */
    int		sg_cterm_attr;	/* NextScreen attr for color term mode */
#ifdef USE_GUI
/* for when using the GUI */
    int		sg_gui;		/* "gui=" highlighting attributes */
    GuiColor	sg_gui_fg;	/* GUI foreground color handle + 1 */
    char_u	*sg_gui_fg_name;/* GUI foreground color name */
    GuiColor	sg_gui_bg;	/* GUI background color handle + 1 */
    char_u	*sg_gui_bg_name;/* GUI background color name */
    GuiFont	sg_font;	/* GUI font handle */
    char_u	*sg_font_name;  /* GUI font name */
    int		sg_gui_attr;    /* NextScreen attr for GUI mode */
#endif
    int		sg_link;	/* link to this highlight group ID */
    int		sg_set;		/* combination of SG_* */
};

#define SG_TERM		1	/* term has been set */
#define SG_CTERM	2	/* cterm has been set */
#define SG_GUI		4	/* gui has been set */
#define SG_LINK		8	/* link has been set */

static struct growarray highlight_ga;	    /* highlight groups for
					       'highlight' option */

#define HL_TABLE() ((struct hl_group *)((highlight_ga.ga_data)))

#ifdef CMDLINE_COMPL
static int include_link = FALSE;	/* include "link" for expansion */
#endif

/*
 * The "term", "cterm" and "gui" arguments can be any combination of the
 * following names, separated by commas (but no spaces!).
 */
static char *(hl_name_table[]) =
    {"bold", "standout", "underline", "italic", "reverse", "inverse", "NONE"};
static int hl_attr_table[] =
    {HL_BOLD, HL_STANDOUT, HL_UNDERLINE, HL_ITALIC, HL_INVERSE, HL_INVERSE, 0};

static int get_attr_entry  __ARGS((struct growarray *table, struct attr_entry *aep));
static int syn_namen2id __ARGS((char_u *linep, int len));
static void syn_unadd_group __ARGS((void));
static void set_hl_attr __ARGS((int idx));
static void highlight_list_one __ARGS((int id));
static int highlight_list_arg __ARGS((int id, int didh, int type, int iarg, char_u *sarg, char *name));
static int syn_add_group __ARGS((char_u *name));
static int syn_list_header __ARGS((int did_header, int outlen, int id));
static void highlight_clear __ARGS((int idx));

#ifdef USE_GUI
static void gui_do_one_color __ARGS((int idx));
static int  set_group_colors __ARGS((char_u *name, GuiColor *fgp, GuiColor *bgp));
static GuiColor color_name2handle __ARGS((char_u *name));
static GuiFont font_name2handle __ARGS((char_u *name));
#endif

/*
 * An attribute number is the index in attr_table plus ATTR_OFF.
 */
#define ATTR_OFF (HL_ALL + 1)

#if defined(SYNTAX_HL) || defined(PROTO)

#define SYN_NAMELEN	50		/* maximum length of a syntax name */

/* different types of offsets that are possible */
#define SPO_MS_OFF	0	/* match  start offset */
#define SPO_ME_OFF	1	/* match  end	offset */
#define SPO_HS_OFF	2	/* highl. start offset */
#define SPO_HE_OFF	3	/* highl. end	offset */
#define SPO_RS_OFF	4	/* region start offset */
#define SPO_RE_OFF	5	/* region end	offset */
#define SPO_LC_OFF	6	/* leading context offset */
#define SPO_COUNT	7

static char *(spo_name_tab[SPO_COUNT]) =
	    {"ms=", "me=", "hs=", "he=", "rs=", "re=", "lc="};

/*
 * The patterns that are being searched for are stored in a syn_pattern.
 * A match item consists of one pattern.
 * A start/end item consists of n start patterns and m end patterns.
 * A start/skip/end item consists of n start patterns, one skip pattern and m
 * end patterns.
 * For the latter two, the patterns are always consecutive: start-skip-end.
 *
 * A character offset can be given for the matched text (_m_start and _m_end)
 * and for the actually highlighted text (_h_start and _h_end).
 */
struct syn_pattern
{
    char		 sp_type;	    /* see SPTYPE_ defines below */
    char		 sp_syncing;	    /* this item used for syncing */
    short		 sp_flags;	    /* see HL_ defines below */
    int			 sp_syn_inc_tag;    /* ":syn include" unique tag */
    short		 sp_syn_id;	    /* highlight group ID of item */
    short		 sp_syn_match_id;   /* highlight group ID of pattern */
    char_u		*sp_pattern;	    /* regexp to match, pattern */
    vim_regexp		*sp_prog;	    /* regexp to match, program */
    int			 sp_ic;		    /* ignore-case flag for sp_prog */
    short		 sp_off_flags;	    /* see below */
    int			 sp_offsets[SPO_COUNT];	/* offsets */
    short		*sp_cont_list;	    /* cont. group IDs, if non-zero */
    short		*sp_next_list;	    /* next group IDs, if non-zero */
    int			 sp_sync_idx;	    /* sync item index (syncing only) */
    int			 sp_line_id;	    /* ID of last line where tried */
    int			 sp_startcol;	    /* next match in sp_line_id line */
};

/* The sp_off_flags are computed like this:
 * offset from the start of the matched text: (1 << SPO_XX_OFF)
 * offset from the end	 of the matched text: (1 << (SPO_XX_OFF + SPO_COUNT))
 * When both are present, only one is used.
 */

#define SPTYPE_MATCH	1	/* match keyword with this group ID */
#define SPTYPE_START	2	/* match a regexp, start of item */
#define SPTYPE_END	3	/* match a regexp, end of item */
#define SPTYPE_SKIP	4	/* match a regexp, skip within item */

#define HL_CONTAINED	0x01	/* not used on toplevel */
#define HL_TRANSP	0x02	/* has no highlighting	*/
#define HL_ONELINE	0x04	/* match within one line only */
#define HL_HAS_EOL	0x08	/* end pattern that matches with $ */
#define HL_SYNC_HERE	0x10	/* sync point after this item (syncing only) */
#define HL_SYNC_THERE	0x20	/* sync point at current line (syncing only) */
#define HL_MATCH	0x40	/* use match ID instead of item ID */
#define HL_SKIPNL	0x80	/* nextgroup can skip newlines */
#define HL_SKIPWHITE	0x100	/* nextgroup can skip white space */
#define HL_SKIPEMPTY	0x200	/* nextgroup can skip empty lines */
#define HL_KEEPEND	0x400	/* end match always kept */
#define HL_EXCLUDENL	0x800	/* exclude NL from match */

#define SYN_ITEMS(buf)	((struct syn_pattern *)((buf)->b_syn_patterns.ga_data))

#define NONE_IDX	-2	/* value of sp_sync_idx for "NONE" */

/*
 * Flags for b_syn_sync_flags:
 */
#define SF_CCOMMENT	0x01	/* sync on a C-style comment */
#define SF_MATCH	0x02	/* sync by matching a pattern */

/*
 * Struct used to store states for the start of some lines for a buffer.
 */
struct buf_state
{
    int	    bs_idx;		/* index of pattern */
    int	    bs_flags;		/* flags for pattern */
};

#define SYN_STATE_P(ssp)    ((struct buf_state *)((ssp)->ga_data))

/*
 * Settings for keyword hash table.  It uses a simplistic hash function: add
 * all characters together, modulo KHASH_SIZE.
 */
#define KHASH_SIZE	512
#define KHASH_MASK	(KHASH_SIZE - 1)
#define MAXKEYWLEN	80	    /* maximum length of a keyword */

/*
 * The attributes of the syntax item that has been recognized.
 */
static int current_attr = 0;	    /* attr of current syntax word */
#ifdef WANT_EVAL
static int current_id = 0;	    /* ID of current char for syn_get_id() */
static int current_trans_id = 0;    /* idem, transparancy removed */
#endif

struct syn_cluster
{
    char_u	    *scl_name;	    /* syntax cluster name */
    char_u	    *scl_name_u;    /* uppercase of scl_name */
    short	    *scl_list;	    /* IDs in this syntax cluster */
};

/*
 * Syntax group IDs greater than or equal to this are actually cluster IDs.  I
 * was gonna use SHRT_MAX/2, but apparently not everyone has <limits.h>...
 */
#define CLUSTER_ID_MIN	15000

/*
 * Methods of combining two clusters
 */
#define CLUSTER_REPLACE	    1	/* replace first list with second */
#define CLUSTER_ADD	    2	/* add second list to first */
#define CLUSTER_SUBTRACT    3	/* subtract second list from first */

#define SYN_CLSTR(buf)	((struct syn_cluster *)((buf)->b_syn_clusters.ga_data))

/*
 * Annoying Hack(TM):  ":syn include" needs this pointer to pass to
 * expand_filename().  Most of the other syntax commands don't need it, so
 * instead of passing it to them, we stow it here.
 */
static char_u **syn_cmdlinep;

/*
 * Another Annoying Hack(TM):  To prevent rules from other ":syn include"'d
 * files from from leaking into ALLBUT lists, we assign a unique ID to the
 * rules in each ":syn include"'d file.
 */
static int current_syn_inc_tag = 0;
static int running_syn_inc_tag = 0;

/*
 * To reduce the time spent in keepend(), remember at which level in the state
 * stack the first item with "keepend" is present.  When "-1", there is no
 * "keepend" on the stack.
 */
static int keepend_level = -1;

/*
 * For the current state we need to remember more than just the idx.
 * When si_m_endcol is 0, the items other than si_idx are unknown.
 */
struct state_item
{
    int	    si_idx;		    /* index of syntax pattern */
    int	    si_id;		    /* highlight group ID for keywords */
    int	    si_trans_id;	    /* idem, transparancy removed */
    int	    si_m_lnum;		    /* lnum of the match */
    int	    si_m_startcol;	    /* starting column of the match */
    int	    si_m_endcol;	    /* ending column of the match */
    int	    si_h_startcol;	    /* starting column of the highlighting */
    int	    si_h_endcol;	    /* ending column of the highlighting */
    int	    si_eoe_col;		    /* ending column of end pattern */
    int	    si_end_idx;		    /* group ID for end pattern or zero */
    int	    si_ends;		    /* if match ends after si_m_endcol */
    int	    si_attr;		    /* attributes in this state */
    int	    si_flags;		    /* HL_HAS_EOL flag in this state, and
				       HL_SKIP* for si_next_list */
    short   *si_cont_list;	    /* list of contained groups */
    short   *si_next_list;	    /* nextgroup IDs after this item ends */
};

#define KEYWORD_IDX	-1	    /* value of si_idx for keywords */
#define CONTAINS_ALLBUT	9999	    /* value of id for contains ALLBUT */
#define ID_LIST_ALL	(short *)-1 /* valid of si_cont_list for containing all
				       but contained groups */

/*
 * The next possible match for any pattern is remembered, to avoid having to
 * try for a match in each column.
 * If next_match_idx == -1, not tried (in this line) yet.
 * If next_match_col == MAXCOL, no match found in this line.
 */
static int next_match_col;	    /* column for start of next match */
static int next_match_m_endcol;	    /* column for end of next match */
static int next_match_h_startcol;   /* column for highl. start of next match */
static int next_match_h_endcol;	    /* column for highl. end of next match */
static int next_match_idx;	    /* index of matched item */
static int next_match_flags;	    /* flags for next match */
static int next_match_eos_col;	    /* column for end of start pattern */
static int next_match_eoe_col;	    /* column for end of end pattern */
static int next_match_end_idx;	    /* ID of group for end pattern or zero */

/*
 * A state stack is an array of integers or struct state_item, stored in a
 * struct growarray.  A state stack is invalid if it's itemsize entry is zero.
 */
#define INVALID_STATE(ssp)  ((ssp)->ga_itemsize == 0)
#define VALID_STATE(ssp)    ((ssp)->ga_itemsize != 0)

/*
 * The current state (within the line) of the recognition engine.
 */
static BUF	*syn_buf;		/* current buffer for highlighting */
static linenr_t current_lnum = 0;	/* lnum of current state */
static int	current_state_stored = 0; /* TRUE if stored current state
					   * after setting current_finished */
static colnr_t	current_col = 0;	/* column of current state */
static int	current_finished = 0;	/* current line has been finished */
static struct growarray current_state	/* current stack of state_items */
		= {0, 0, 0, 0, NULL};
static short	*current_next_list = NULL; /* when non-zero, nextgroup list */
static int	current_next_flags = 0; /* flags for current_next_list */
static int	current_line_id = 0;	/* unique number for current line */

#define CUR_STATE(idx)	((struct state_item *)(current_state.ga_data))[idx]

static void syn_sync __ARGS((WIN *wp, linenr_t lnum));
static int syn_match_linecont __ARGS((linenr_t lnum));
static void syn_start_line __ARGS((void));
static void syn_free_all_states __ARGS((BUF *buf));
static void syn_clear_states __ARGS((int start, int end));
static void store_current_state __ARGS((void));
static void invalidate_current_state __ARGS((void));
static void validate_current_state __ARGS((void));
static void copy_state_to_current __ARGS((struct syn_state *from));
static void move_state __ARGS((int from, int to));
static int syn_finish_line __ARGS((int syncing));
static int syn_current_attr __ARGS((int syncing, char_u *line));
static int did_match_already __ARGS((int idx));
static struct state_item *push_next_match __ARGS((struct state_item *cur_si, char_u *line));
static void check_state_ends __ARGS((char_u *line));
static void update_si_attr __ARGS((int idx));
static void check_keepend __ARGS((void));
static void update_si_end __ARGS((struct state_item *sip, char_u *line, int startcol));
static short *copy_id_list __ARGS((short *list));
static int in_id_list __ARGS((short *cont_list, int id, int inclvl, int contained));
static int syn_regexec __ARGS((vim_regexp *prog, char_u *string, int at_bol));
static int push_current __ARGS((int idx));
static void pop_current __ARGS((void));
static char_u *find_endp __ARGS((int idx, char_u *sstart, int at_bol, char_u **hl_endp, int *flagsp, char_u **end_endp, int *end_idx));
static char_u *syn_add_end_off __ARGS((struct syn_pattern *spp, int idx, int extra));
static char_u *syn_add_start_off __ARGS((struct syn_pattern *spp, int idx, int extra));
static int check_keyword_id __ARGS((char_u *line, int startcol, int *endcol, int *flags, short **next_list, struct state_item *cur_si));
static void syn_cmd_case __ARGS((EXARG *eap, int syncing));
static void syntax_sync_clear __ARGS((void));
static void syn_remove_pattern __ARGS((BUF *buf, int idx));
static void syn_clear_pattern __ARGS((BUF *buf, int i));
static void syn_clear_cluster __ARGS((BUF *buf, int i));
static void syn_cmd_clear __ARGS((EXARG *eap, int syncing));
static void syn_clear_one __ARGS((int id, int syncing));
static void syn_cmd_on __ARGS((EXARG *eap, int syncing));
static void syn_cmd_manual __ARGS((EXARG *eap, int syncing));
static void syn_cmd_off __ARGS((EXARG *eap, int syncing));
static void syn_cmd_onoff __ARGS((EXARG *eap, char *name));
static void syn_cmd_list __ARGS((EXARG *eap, int syncing));
static void syn_lines_msg __ARGS((void));
static void syn_list_one __ARGS((int id, int syncing, int link_only));
static void syn_list_cluster __ARGS((int id));
static void put_id_list __ARGS((char_u *name, short *list, int attr));
static void put_pattern __ARGS((char *s, int c, struct syn_pattern *spp, int attr));
static int syn_list_keywords __ARGS((int id, struct keyentry **ktabp, int did_header, int attr));
static void syn_clear_keyword __ARGS((int id, struct keyentry **ktabp));
static void free_keywtab __ARGS((struct keyentry **ktabp));
static void add_keyword __ARGS((char_u *name, int id, int flags, short *next_list));
static char_u *get_group_name __ARGS((char_u *arg, char_u **name_end));
static char_u *get_syn_options __ARGS((char_u *arg, int *flagsp, int *sync_idx,
				    short **cont_list, short **next_list));
static void syn_cmd_include __ARGS((EXARG *eap, int syncing));
static void syn_cmd_keyword __ARGS((EXARG *eap, int syncing));
static void syn_cmd_match __ARGS((EXARG *eap, int syncing));
static void syn_cmd_region __ARGS((EXARG *eap, int syncing));
#ifdef __BORLANDC__
static int _RTLENTRYF syn_compare_stub __ARGS((const void *v1, const void *v2));
#else
static int syn_compare_stub __ARGS((const void *v1, const void *v2));
#endif
static void syn_cmd_cluster __ARGS((EXARG *eap, int syncing));
static int syn_scl_name2id __ARGS((char_u *name));
static int syn_scl_namen2id __ARGS((char_u *linep, int len));
static int syn_check_cluster __ARGS((char_u *pp, int len));
static int syn_add_cluster __ARGS((char_u *name));
static void init_syn_patterns __ARGS((void));
static char_u *get_syn_pattern __ARGS((char_u *arg, struct syn_pattern	*ci));
static void syn_cmd_sync __ARGS((EXARG *eap, int syncing));
static int get_id_list __ARGS((char_u **arg, int keylen, short **list));
static void syn_combine_list __ARGS((short **clstr1, short **clstr2, int list_op));
static void syn_incl_toplevel __ARGS((int id, int *flagsp));

/*
 * Start the syntax recognition for a line.  This function is normally called
 * from the screen updating, once for each consecutive line.
 * The buffer is remembered in syn_buf, because get_syntax_attr() doesn't get
 * it.	Careful: curbuf and curwin are likely to point to another buffer and
 * window.
 */
    void
syntax_start(wp, lnum)
    WIN		*wp;
    linenr_t	lnum;
{
    long	to, from, first;
    long	diff;
    int		idx;

    reg_syn = TRUE;	/* let vim_regexec() know we're using syntax */

    /*
     * After switching buffers, invalidate current_state.
     */
    if (syn_buf != wp->w_buffer)
    {
	invalidate_current_state();
	syn_buf = wp->w_buffer;
    }

    /*
     * Keep syncing info for ten lines above the window.  This is a compromise
     * between computing extra lines (jumping around) and reducing computions
     * (mostly when scrolling up).
     */
#define SYNC_LINES 10

    /*
     * If the screen height has changed, re-allocate b_syn_states[].
     * Use the screen height plus SYNC_LINES, so some lines above and one
     * line below the window can always be stored too.
     */
    if (syn_buf->b_syn_states_len != Rows + SYNC_LINES)
    {
	syn_free_all_states(syn_buf);
	syn_buf->b_syn_states = (struct syn_state *)alloc_clear(
		  (int)((Rows + SYNC_LINES) * sizeof(struct syn_state)));
	if (syn_buf->b_syn_states == NULL)	    /* out of memory */
	{
	    syn_buf->b_syn_states_len = 0;
	    goto theend;
	}
	syn_buf->b_syn_states_len = Rows + SYNC_LINES;
	syn_buf->b_syn_states_lnum = 0;
	syn_buf->b_syn_change_lnum = MAXLNUM;
    }

    /*
     * Remove items from b_syn_states[] that have changes in or before them.
     */
    if (syn_buf->b_syn_change_lnum != MAXLNUM)
    {
	/* if change is before the end of the array, something to clear */
	if (syn_buf->b_syn_change_lnum <
		   syn_buf->b_syn_states_lnum + syn_buf->b_syn_states_len - 1)
	{
	    /* line before the start changed: clear all entries */
	    if (syn_buf->b_syn_change_lnum < syn_buf->b_syn_states_lnum)
		idx = 0;
	    else
		idx = syn_buf->b_syn_change_lnum -
					       syn_buf->b_syn_states_lnum + 1;
	    syn_clear_states(idx, syn_buf->b_syn_states_len);
	}
	if (syn_buf->b_syn_change_lnum <= current_lnum)
	    invalidate_current_state();
	syn_buf->b_syn_change_lnum = MAXLNUM;
    }

    /*
     * If the topline has changed out of range of b_syn_states[], move the
     * items in the array.
     */
    if (wp->w_topline < syn_buf->b_syn_states_lnum)
    {
	/*
	 * Topline is above the array: Move entries down.
	 * (w_topline - SYNC_LINES) is the new first line in * b_syn_states[].
	 */
	to = syn_buf->b_syn_states_len - 1;
	if (wp->w_topline > SYNC_LINES)
	    first = wp->w_topline - SYNC_LINES;
	else
	    first = 0;
	from = to - (syn_buf->b_syn_states_lnum - first);
	while (from >= 0)
	{
	    move_state((int)from, (int)to);
	    --from;
	    --to;
	}
	syn_clear_states(0, (int)(to + 1));
	syn_buf->b_syn_states_lnum = first;
    }
    else if ((diff = (wp->w_topline + wp->w_height) -
		 (syn_buf->b_syn_states_lnum + syn_buf->b_syn_states_len)) > 0)
    {
	/*
	 * The last line in the window is below the array: Move entries up
	 * "diff" positions.
	 */
	to = 0;
	from = to + diff;
	while (from < syn_buf->b_syn_states_len)
	{
	    move_state((int)from, (int)to);
	    ++from;
	    ++to;
	}
	syn_clear_states((int)to, syn_buf->b_syn_states_len);
	syn_buf->b_syn_states_lnum += diff;
    }

    /*
     * If the state of the end of the previous line is useful, store it.
     */
    if (VALID_STATE(&current_state)
	    && current_lnum < lnum
	    && current_lnum >= syn_buf->b_syn_states_lnum
	    && current_lnum <
			syn_buf->b_syn_states_lnum + syn_buf->b_syn_states_len
	    && current_lnum < syn_buf->b_ml.ml_line_count)
    {
	(void)syn_finish_line(FALSE);
	if (!current_state_stored)
	{
	    ++current_lnum;
	    store_current_state();
	}

	/*
	 * If the current_lnum is now the same as "lnum", keep the current
	 * state (this happens very often!).  Otherwise invalidate
	 * current_state and figure it out below.
	 */
	if (current_lnum != lnum)
	    invalidate_current_state();
    }
    else
	invalidate_current_state();


    /*
     * Try to synchronize from a saved state in b_syn_states[].
     * Only do this if lnum is not before and not to far beyond a saved state.
     */
    if (INVALID_STATE(&current_state))
    {
	diff = syn_buf->b_syn_sync_minlines;
	if (diff < Rows * 2)
	    diff = Rows * 2;	    /* parse less then two screenfulls extra */
	if (lnum >= syn_buf->b_syn_states_lnum
		&& lnum <= syn_buf->b_syn_states_lnum +
					     syn_buf->b_syn_states_len + diff)
	{
	    idx = lnum - syn_buf->b_syn_states_lnum;
	    if (idx >= syn_buf->b_syn_states_len)
		idx = syn_buf->b_syn_states_len - 1;
	    for ( ; idx >= 0; --idx)
	    {
		if (VALID_STATE(&syn_buf->b_syn_states[idx].sst_ga))
		{
		    current_lnum = syn_buf->b_syn_states_lnum + idx;
		    copy_state_to_current(&(syn_buf->b_syn_states[idx]));
		    break;
		}
	    }
	}
    }

    /*
     * If "lnum" is before or far beyond a line with a saved state, need to
     * re-synchronize.
     */
    if (INVALID_STATE(&current_state))
	syn_sync(wp, lnum);

    /*
     * Advance from the sync point or saved state until the current line.
     */
    while (current_lnum < lnum)
    {
	syn_start_line();
	(void)syn_finish_line(FALSE);
	++current_lnum;
	store_current_state();

	/* This can take a long time: break when CTRL-C pressed. */
	line_breakcheck();
	if (got_int)
	{
	    current_lnum = lnum;
	    break;
	}
    }

    syn_start_line();

theend:
    reg_syn = FALSE;
}

/*
 * Try to find a synchronisation point for line "lnum".
 *
 * This sets current_lnum and the current state.  One of three methods is
 * used:
 * 1. Search backwards for the end of a C-comment.
 * 2. Search backwards for given sync patterns.
 * 3. Simply start on a given number of lines above "lnum".
 */
    static void
syn_sync(wp, start_lnum)
    WIN		*wp;
    linenr_t	start_lnum;
{
    BUF			*curbuf_save;
    WIN			*curwin_save;
    FPOS		cursor_save;
    int			idx;
    linenr_t		lnum;
    linenr_t		end_lnum;
    linenr_t		break_lnum;
    int			had_sync_point;
    struct state_item	*cur_si;
    struct syn_pattern	*spp;
    char_u		*line;
    int			found_flags = 0;
    int			found_match_idx = 0;
    linenr_t		found_current_lnum = 0;
    int			found_current_col= 0;
    colnr_t		found_m_endcol = 0;

    /*
     * Clear any current state that might be hanging around.
     */
    invalidate_current_state();

    /*
     * Start at least "minlines" back.
     * Also subtract SYNC_LINES, so that b_syn_states[] is filled with valid
     * states.
     * Default starting point for parsing is there.
     */
    start_lnum -= syn_buf->b_syn_sync_minlines + SYNC_LINES;
    if (start_lnum < 1)
	start_lnum = 1;
    current_lnum = start_lnum;

    /*
     * 1. Search backwards for the end of a C-style comment.
     */
    if (syn_buf->b_syn_sync_flags & SF_CCOMMENT)
    {
	/* need to make syn_buf the current buffer for a moment */
	curwin_save = curwin;
	curwin = wp;
	curbuf_save = curbuf;
	curbuf = syn_buf;

	/*
	 * Skip lines that end in a backslash.
	 */
	for ( ; start_lnum > 1; --start_lnum)
	{
	    line = ml_get(start_lnum - 1);
	    if (*line == NUL || *(line + STRLEN(line) - 1) != '\\')
		break;
	}
	current_lnum = start_lnum;

	/* set cursor to start of search */
	cursor_save = wp->w_cursor;
	wp->w_cursor.lnum = start_lnum;
	wp->w_cursor.col = 0;

	/*
	 * If the line is inside a comment, need to find the syntax item that
	 * defines the comment.
	 * Restrict the search for the end of a comment to b_syn_sync_maxlines.
	 */
	if (find_start_comment((int)syn_buf->b_syn_sync_maxlines) != NULL)
	{
	    for (idx = syn_buf->b_syn_patterns.ga_len; --idx >= 0; )
		if (SYN_ITEMS(syn_buf)[idx].sp_syn_id == syn_buf->b_syn_sync_id
			&& SYN_ITEMS(syn_buf)[idx].sp_type == SPTYPE_START)
		{
		    validate_current_state();
		    if (push_current(idx) == OK)
			update_si_attr(current_state.ga_len - 1);
		    break;
		}
	}

	/* restore cursor and buffer */
	wp->w_cursor = cursor_save;
	curwin = curwin_save;
	curbuf = curbuf_save;
    }

    /*
     * 2. Search backwards for given sync patterns.
     */
    else if (syn_buf->b_syn_sync_flags & SF_MATCH)
    {
	if (syn_buf->b_syn_sync_maxlines
				 && start_lnum > syn_buf->b_syn_sync_maxlines)
	    break_lnum = start_lnum - syn_buf->b_syn_sync_maxlines;
	else
	    break_lnum = 0;

	end_lnum = start_lnum;
	lnum = start_lnum;
	while (--lnum > break_lnum)
	{
	    /* This can take a long time: break when CTRL-C pressed. */
	    line_breakcheck();
	    if (got_int)
	    {
		invalidate_current_state();
		current_lnum = start_lnum;
		break;
	    }

	    /*
	     * Check if the previous line has the line-continuation pattern.
	     */
	    if (lnum > 1 && syn_match_linecont(lnum - 1))
		continue;

	    /*
	     * Start with nothing on the state stack
	     */
	    validate_current_state();

	    for (current_lnum = lnum; current_lnum < end_lnum; ++current_lnum)
	    {
		syn_start_line();
		for (;;)
		{
		    had_sync_point = syn_finish_line(TRUE);
		    /*
		     * When a sync point has been found, remember where, and
		     * continue to look for another one, further on in the line.
		     */
		    if (had_sync_point && current_state.ga_len)
		    {
			cur_si = &CUR_STATE(current_state.ga_len - 1);
			spp = &(SYN_ITEMS(syn_buf)[cur_si->si_idx]);
			found_flags = spp->sp_flags;
			found_match_idx = spp->sp_sync_idx;
			found_current_lnum = current_lnum;
			found_current_col = current_col;
			found_m_endcol = cur_si->si_m_endcol;
			/*
			 * Continue after the match (be aware of a zero-length
			 * match).
			 */
			if (found_m_endcol > current_col)
			    current_col = found_m_endcol;
			else
			    ++current_col;

			/* syn_current_attr() will have skipped the check for
			 * an item that ends here, need to do that now. */
			++current_col;
			check_state_ends(
				    ml_get_buf(syn_buf, current_lnum, FALSE));
			--current_col;
		    }
		    else
			break;
		}
	    }

	    /*
	     * If a sync point was encountered, break here.
	     */
	    if (found_flags)
	    {
		/*
		 * Put the item that was specified by the sync point on the
		 * state stack.  If there was no item specified, make the
		 * state stack empty.
		 */
		ga_clear(&current_state);
		if (found_match_idx >= 0 &&
			push_current(found_match_idx) == OK)
		    update_si_attr(current_state.ga_len - 1);

		/*
		 * When using "grouphere", continue from the sync point
		 * match, until the end of the line.  Parsing starts at
		 * the next line.
		 * For "groupthere" the parsing starts at start_lnum.
		 */
		if (found_flags & HL_SYNC_HERE)
		{
		    if (current_state.ga_len)
		    {
			cur_si = &CUR_STATE(current_state.ga_len - 1);
			cur_si->si_h_startcol = found_current_col;
			line = ml_get_buf(syn_buf, current_lnum, FALSE);
			update_si_end(cur_si, line, current_col);
		    }
		    current_col = found_m_endcol;
		    current_lnum = found_current_lnum;
		    (void)syn_finish_line(FALSE);
		    ++current_lnum;
		}
		else
		    current_lnum = start_lnum;

		break;
	    }

	    end_lnum = lnum;
	    invalidate_current_state();
	}

	/* Ran into start of the file or exceeded maximum number of lines */
	if (lnum <= break_lnum)
	{
	    invalidate_current_state();
	    current_lnum = break_lnum + 1;
	}
    }

    validate_current_state();
}

/*
 * Return TRUE if the line-continuation pattern matches in line "lnum".
 */
    static int
syn_match_linecont(lnum)
    linenr_t	    lnum;
{
    if (syn_buf->b_syn_linecont_prog != NULL)
    {
	reg_ic = syn_buf->b_syn_linecont_ic;
	return syn_regexec(syn_buf->b_syn_linecont_prog,
				      ml_get_buf(syn_buf, lnum, FALSE), TRUE);
    }
    return FALSE;
}

/*
 * Set the state for the start of a line.
 */
    static void
syn_start_line()
{
    char_u		*line;
    struct state_item	*cur_si;
    int			i;

    current_finished = FALSE;
    current_col = 0;

    /*
     * Need to update the end of a start/skip/end that continues from the
     * previous line.  And regions that have "keepend", because they may
     * influence contained items.
     * Then check for items ending in column 0.
     */
    if (current_state.ga_len)
    {
	line = ml_get_buf(syn_buf, current_lnum, FALSE);
	for (i = 0; i < current_state.ga_len; ++i)
	{
	    cur_si = &CUR_STATE(i);
	    if ((cur_si->si_flags & HL_KEEPEND)
					     || i == current_state.ga_len - 1)
	    {
		cur_si->si_h_startcol = 0;	/* start highl. in col 0 */
		update_si_end(cur_si, line, 0);
	    }
	}
	check_state_ends(line);
    }
    next_match_idx = -1;
    ++current_line_id;
}

/*
 * Free b_syn_states[] for buffer "buf".
 */
    static void
syn_free_all_states(buf)
    BUF	    *buf;
{
    int	    idx;

    if (buf->b_syn_states != NULL)
    {
	for (idx = 0; idx < buf->b_syn_states_len; ++idx)
	    ga_clear(&(buf->b_syn_states[idx].sst_ga));
	vim_free(buf->b_syn_states);
	buf->b_syn_states = NULL;
	buf->b_syn_states_len = 0;
    }
}

/*
 * clear the entries in b_syn_states[] from "start" to (not including) "end"
 */
    static void
syn_clear_states(start, end)
    int		start, end;
{
    int		idx;
    struct	growarray *sp;

    for (idx = start; idx < end; ++idx)
    {
	sp = &(syn_buf->b_syn_states[idx].sst_ga);
	ga_clear(sp);
	sp->ga_itemsize = 0;
    }
}

/*
 * Try saving the current state in b_syn_states[].
 * The current state must be at the start of the current_lnum line!
 */
    static void
store_current_state()
{
    long		idx;
    int			i;
    struct growarray	*to;

    idx = current_lnum - syn_buf->b_syn_states_lnum;
    if (idx >= 0 && idx < syn_buf->b_syn_states_len)
    {
	to = &(syn_buf->b_syn_states[idx].sst_ga);
	if (to->ga_data != NULL)
	    ga_clear(to);
	else if (to->ga_itemsize == 0)
	{
	    to->ga_itemsize = sizeof(struct buf_state);
	    to->ga_growsize = 3;
	}
	if (current_state.ga_len && ga_grow(to, current_state.ga_len) != FAIL)
	{
	    for (i = 0; i < current_state.ga_len; ++i)
	    {
		SYN_STATE_P(to)[i].bs_idx = CUR_STATE(i).si_idx;
		SYN_STATE_P(to)[i].bs_flags = CUR_STATE(i).si_flags;
	    }
	    to->ga_len = current_state.ga_len;
	    to->ga_room -= to->ga_len;
	}
	syn_buf->b_syn_states[idx].sst_next_list = current_next_list;
	syn_buf->b_syn_states[idx].sst_next_flags = current_next_flags;
    }
    current_state_stored = TRUE;
}

/*
 * Copy a state stack from "from" in b_syn_states[] to current_state;
 */
    static void
copy_state_to_current(from)
    struct syn_state *from;
{
    int	    i;
    struct growarray	*ga = &(from->sst_ga);

    ga_clear(&current_state);
    validate_current_state();
    keepend_level = -1;
    if (ga->ga_len && ga_grow(&current_state, ga->ga_len) != FAIL)
    {
	for (i = 0; i < ga->ga_len; ++i)
	{
	    CUR_STATE(i).si_idx = SYN_STATE_P(ga)[i].bs_idx;
	    CUR_STATE(i).si_flags = SYN_STATE_P(ga)[i].bs_flags;
	    if (keepend_level < 0 && (CUR_STATE(i).si_flags & HL_KEEPEND))
		keepend_level = i;
	    CUR_STATE(i).si_m_endcol = 0;
	    CUR_STATE(i).si_m_startcol = 0;
	    CUR_STATE(i).si_m_lnum = 0;
	    CUR_STATE(i).si_next_list =
		       (SYN_ITEMS(syn_buf)[CUR_STATE(i).si_idx]).sp_next_list;
	    update_si_attr(i);
	}
	current_state.ga_len = ga->ga_len;
	current_state.ga_room -= current_state.ga_len;
    }
    current_next_list = from->sst_next_list;
    current_next_flags = from->sst_next_flags;
}

    static void
invalidate_current_state()
{
    ga_clear(&current_state);
    current_state.ga_itemsize = 0;
    current_next_list = NULL;
    keepend_level = -1;
}

    static void
validate_current_state()
{
    current_state.ga_itemsize = sizeof(struct state_item);
    current_state.ga_growsize = 3;
}

/*
 * Move a state stack from b_syn_states[from] to b_syn_states[to].
 */
    static void
move_state(from, to)
    int	    from, to;
{
    ga_clear(&(syn_buf->b_syn_states[to].sst_ga));
    syn_buf->b_syn_states[to] = syn_buf->b_syn_states[from];
    ga_init(&(syn_buf->b_syn_states[from].sst_ga));
    syn_buf->b_syn_states[from].sst_ga.ga_itemsize = 0;	/* invalid entry */
}

/*
 * Mark like "lnum" and following ones as changed: Need to recompute its
 * highlighting.
 * This must be called whenever something is changed.  ml_delete() and
 * ml_append() take care of this when deleting/appending lines.
 * When changing a single line, and calling update_screenline(), for it, no
 * need to call this (syntax_check_changed() will be used then).
 */
    void
syn_changed(lnum)
    linenr_t	lnum;
{
    if (curbuf->b_syn_change_lnum > lnum)
	curbuf->b_syn_change_lnum = lnum;
}

/*
 * Return TRUE if the syntax at start of lnum changed since last time.
 * This will only be called just after get_syntax_attr for the previous line,
 * to check if the next line needs to be redrawn too.
 */
    int
syntax_check_changed(lnum)
    linenr_t	lnum;
{
    struct growarray	*ssp;
    int			i;
    int			retval = TRUE;
    long		idx;

    reg_syn = TRUE;	/* let vim_regexec() know we're using syntax */

    /*
     * Check the state stack when:
     * - lnum is just below the previously syntaxed line.
     * - lnum is not before the lines with saved states.
     * - lnum is not past the lines with saved states.
     * - lnum is at or before the last changed line.
     */
    idx = lnum - syn_buf->b_syn_states_lnum;
    if (VALID_STATE(&current_state) && lnum == current_lnum + 1 &&
	    idx >= 0 && idx < syn_buf->b_syn_states_len &&
	    lnum < syn_buf->b_syn_change_lnum)
    {
	/*
	 * finish the previous line (needed when not all of the line was drawn)
	 */
	(void)syn_finish_line(FALSE);

	ssp = &(syn_buf->b_syn_states[idx].sst_ga);
	if (VALID_STATE(ssp))	/* entry is valid */
	{
	    /*
	     * Compare the current state with the previously saved state of
	     * the line.
	     */
	    if (ssp->ga_len == current_state.ga_len
		    && syn_buf->b_syn_states[idx].sst_next_list
							 == current_next_list)
	    {
		for (i = current_state.ga_len; --i >= 0; )
		    if (SYN_STATE_P(ssp)[i].bs_idx != CUR_STATE(i).si_idx)
			break;
		/*
		 * If still the same state, return FALSE, syntax didn't change.
		 */
		if (i < 0)
		    retval = FALSE;
	    }
	}

	/*
	 * Store the current state in b_syn_states[] for later use.
	 */
	++current_lnum;
	store_current_state();
    }

    reg_syn = FALSE;

    /* If state has changed, the saved states are invalid now */
    if (retval)
	syn_changed(lnum);

    return retval;
}

/*
 * Finish the current line.
 * This doesn't return any attributes, it only gets the state at the end of
 * the line.  It can start anywhere in the line, as long as the current state
 * is valid.
 */
    static int
syn_finish_line(syncing)
    int	    syncing;		/* called for syncing */
{
    char_u		*line;
    struct state_item	*cur_si;

    if (!current_finished)
    {
	line = ml_get_buf(syn_buf, current_lnum, FALSE);
	while (!current_finished)
	{
	    (void)syn_current_attr(syncing, line);
	    /*
	     * When syncing, and found some item, need to check the item.
	     */
	    if (syncing && current_state.ga_len)
	    {
		/*
		 * Check for match with sync item.
		 */
		cur_si = &CUR_STATE(current_state.ga_len - 1);
		if (SYN_ITEMS(syn_buf)[cur_si->si_idx].sp_flags
					       & (HL_SYNC_HERE|HL_SYNC_THERE))
		    return TRUE;

		/* syn_current_attr() will have skipped the check for an item
		 * that ends here, need to do that now. */
		++current_col;
		check_state_ends(line);
		--current_col;
	    }
	    ++current_col;
	}
    }
    return FALSE;
}

/*
 * Return highlight attributes for next character.
 * This function is alwyas called from the screen updating, for each
 * consecutive character.  And only after syntax_start() has been called for
 * the current line.
 * Note that "col" doesn't start at 0, when win->w_leftcol is non-zero, and
 * doesn't continue until the last col when 'nowrap' is set.
 */
    int
get_syntax_attr(col, line)
    colnr_t	col;
    char_u	*line;
{
    int	    attr = 0;

    /* check for out of memory situation */
    if (syn_buf->b_syn_states_len == 0)
	return 0;

    reg_syn = TRUE;	/* let vim_regexec() know we're using syntax */

    /* Make sure current_state is valid */
    if (INVALID_STATE(&current_state))
	validate_current_state();

    /*
     * Skip from the current column to "col", get the attributes for "col".
     */
    while (current_col <= col)
    {
	attr = syn_current_attr(FALSE, line);
	++current_col;
    }

    reg_syn = FALSE;
    return attr;
}

/*
 * Get syntax attributes for current_lnum, current_col.
 */
    static int
syn_current_attr(syncing, line)
    int		syncing;	    /* When 1: called for syncing */
    char_u	*line;
{
    int			syn_id;
    char_u		*endp;
    char_u		*hl_endp = NULL;
    char_u		*eoep;	    /* end-of-end pattern */
    int			end_idx;    /* group ID for end pattern */
    int			idx;
    struct syn_pattern	*spp;
    struct state_item	*cur_si, *sip;
    int			startcol;
    int			hl_startcol;
    int			eos_col;    /* end-of-start column */
    int			endcol;
    int			flags;
    short		*next_list;
    int			found_match;		    /* found usable match */
    static int		try_next_column = FALSE;    /* must try in next col */

    /*
     * No character, no attributes!  Past end of line?
     * Do try matching with an empty line (could be the start of a region).
     */
    if (*(line + current_col) == NUL && current_col != 0)
    {
	/*
	 * If we found a match after the last column, use it.
	 */
	if (next_match_idx >= 0 && next_match_col >= (int)current_col
						  && next_match_col != MAXCOL)
	    (void)push_next_match(NULL, line);

	current_finished = TRUE;
	current_state_stored = FALSE;
	return 0;
    }

    /* if the next character is NUL, we will finish the line now */
    if (*(line + current_col) == NUL || *(line + current_col + 1) == NUL)
    {
	current_finished = TRUE;
	current_state_stored = FALSE;
    }

    /*
     * When in the previous column there was a match but it could not be used
     * (empty match or already matched in this column) need to try again in
     * the next column.
     */
    if (try_next_column)
    {
	next_match_idx = -1;
	try_next_column = FALSE;
    }

    /*
     * Repeat matching keywords and patterns, to find contained items at the
     * same column.  This stops when there are no extra matches at the current
     * column.
     */
    do
    {
	found_match = FALSE;
	syn_id = 0;

	/*
	 * 1. Check for a current state.
	 *    Only when there is no current state, or if the current state may
	 *    contain other things, we need to check for keywords and patterns.
	 */
	if (current_state.ga_len)
	    cur_si = &CUR_STATE(current_state.ga_len - 1);
	else
	    cur_si = NULL;

	if (cur_si == NULL || cur_si->si_cont_list != NULL)
	{
	    /*
	     * 2. Check for keywords, if on a keyword char after a non-keyword
	     *	  char.  Don't do this when syncing.
	     */
	    if (       !syncing
		    && (syn_buf->b_keywtab != NULL
			|| syn_buf->b_keywtab_ic != NULL)
		    && vim_iswordc_buf(line[current_col], syn_buf)
		    && (current_col == 0
			|| !vim_iswordc_buf(line[current_col - 1], syn_buf)))
	    {
		syn_id = check_keyword_id(line, (int)current_col,
					 &endcol, &flags, &next_list, cur_si);
		if (syn_id)
		{
		    if (push_current(KEYWORD_IDX) == OK)
		    {
			cur_si = &CUR_STATE(current_state.ga_len - 1);
			cur_si->si_m_startcol = current_col;
			cur_si->si_h_startcol = 0;	/* starts right away */
			cur_si->si_m_endcol = endcol;
			cur_si->si_h_endcol = endcol;
			cur_si->si_ends = TRUE;
			cur_si->si_end_idx = 0;
			cur_si->si_flags = flags;
			cur_si->si_id = syn_id;
			cur_si->si_trans_id = syn_id;
			if (flags & HL_TRANSP)
			{
			    if (current_state.ga_len < 2)
			    {
				cur_si->si_attr = 0;
				cur_si->si_trans_id = 0;
			    }
			    else
			    {
				cur_si->si_attr = CUR_STATE(
					current_state.ga_len - 2).si_attr;
				cur_si->si_trans_id = CUR_STATE(
					current_state.ga_len - 2).si_trans_id;
			    }
			}
			else
			    cur_si->si_attr = syn_id2attr(syn_id);
			cur_si->si_cont_list = NULL;
			cur_si->si_next_list = next_list;
			check_keepend();
		    }
		    else
			vim_free(next_list);
		}
	    }

	    /*
	     * 3. Check for patterns (only if not found a keyword).
	     */
	    if (syn_id == 0 && syn_buf->b_syn_patterns.ga_len)
	    {
		/*
		 * If we didn't check for a match yet, or we are past it, check
		 * for any match with a pattern.
		 */
		if (next_match_idx < 0 || next_match_col < (int)current_col)
		{
		    /*
		     * Check all relevant patterns for a match at this
		     * position.
		     */
		    next_match_idx = 0;		/* no match in this line yet */
		    next_match_col = MAXCOL;	/* no match in this line yet */
		    for (idx = syn_buf->b_syn_patterns.ga_len; --idx >= 0; )
		    {
			spp = &(SYN_ITEMS(syn_buf)[idx]);
			if (	   spp->sp_syncing == syncing
				&& (spp->sp_type == SPTYPE_MATCH
				    || spp->sp_type == SPTYPE_START)
				&& ((current_next_list != 0
					&& in_id_list(current_next_list,
						spp->sp_syn_id,
						spp->sp_syn_inc_tag, 0))
				    || (current_next_list == 0
					&& ((cur_si == NULL
					    && !(spp->sp_flags & HL_CONTAINED))
						|| (cur_si != NULL
					 && in_id_list(cur_si->si_cont_list,
						spp->sp_syn_id,
						spp->sp_syn_inc_tag,
					     spp->sp_flags & HL_CONTAINED))))))
			{
			    int lc_col;

			    /* If we already tried matching in this line, and
			     * there isn't a match before next_match_col, skip
			     * this item. */
			    if (spp->sp_line_id == current_line_id
					&& spp->sp_startcol >= next_match_col)
				continue;
			    spp->sp_line_id = current_line_id;

			    lc_col = current_col - spp->sp_offsets[SPO_LC_OFF];
			    if (lc_col < 0)
				lc_col = 0;

			    reg_ic = spp->sp_ic;
			    if (!syn_regexec(spp->sp_prog, line + lc_col,
					     lc_col == 0))
			    {
				spp->sp_startcol = MAXCOL;
				continue;
			    }

			    /*
			     * Compute the first column of the match.
			     */
			    startcol = syn_add_start_off(spp,
						       SPO_MS_OFF, -1) - line;
			    if (startcol < 0)
				startcol = 0;
			    spp->sp_startcol = startcol;

			    /*
			     * If an existing match is better, skip this one.
			     */
			    if (startcol >= next_match_col)
				continue;

			    /*
			     * If we matched this pattern at this position
			     * before, skip it.  Must retry in the next
			     * column, because it may match from there..
			     */
			    if (did_match_already(idx))
			    {
				try_next_column = TRUE;
				continue;
			    }

			    endp = spp->sp_prog->endp[0];

			    /* Compute the highlight start. */
			    hl_startcol = syn_add_start_off(spp,
						       SPO_HS_OFF, -1) - line;

			    /* Compute the region start. */
			    /* Default is to use the end of the match. */
			    if (spp->sp_off_flags & (1 << SPO_RS_OFF))
				eos_col = (spp->sp_prog->startp[0] - line)
					    + spp->sp_offsets[SPO_RS_OFF] - 1;
			    else
				eos_col = (endp - line) - 1
						+ spp->sp_offsets[SPO_RS_OFF];

			    /*
			     * If this is a oneline, the end must be found
			     * in the same line too.
			     */
			    flags = 0;
			    eoep = line;	/* avoid warning */
			    end_idx = 0;
			    if (spp->sp_type == SPTYPE_START
				    && (spp->sp_flags & HL_ONELINE))
				endp = find_endp(idx, endp, endp == line,
					&hl_endp, &flags, &eoep, &end_idx);

			    /*
			     * For a "match" the size must be > 0 after the
			     * end offset needs has been added.  Except when
			     * syncing.
			     */
			    else if (spp->sp_type == SPTYPE_MATCH)
			    {
				hl_endp = syn_add_end_off(spp, SPO_HE_OFF, 0);
				endp = syn_add_end_off(spp, SPO_ME_OFF, 0);
				if (endp + syncing <= line + startcol)
				{
				    /*
				     * If an empty string is matched, may need
				     * to try matching again at next column.
				     */
				    if (spp->sp_prog->startp[0] ==
							spp->sp_prog->endp[0])
					try_next_column = TRUE;
				    continue;
				}
			    }

			    /* keep the best match so far in next_match_* */
			    if (endp != NULL)
			    {
				if (hl_startcol < startcol)
				    hl_startcol = startcol;
				if (hl_endp == NULL || hl_endp > endp)
				    hl_endp = endp;

				next_match_idx = idx;
				next_match_col = startcol;
				next_match_m_endcol = endp - line;
				next_match_h_endcol = hl_endp - line;
				next_match_h_startcol = hl_startcol;
				next_match_flags = flags;
				next_match_eos_col = eos_col;
				next_match_eoe_col = eoep - line;
				next_match_end_idx = end_idx;
			    }
			}
		    }
		}

		/*
		 * If we found a match at the current column, use it.
		 */
		if (next_match_idx >= 0 && next_match_col == (int)current_col)
		{
		    cur_si = push_next_match(cur_si, line);
		    found_match = TRUE;
		}
	    }
	}

	/*
	 * Handle searching for nextgroup match.
	 */
	if (current_next_list != NULL)
	{
	    /*
	     * If a nextgroup was not found, continue looking for one if:
	     * - this is an empty line and the "skipempty" option was given
	     * - we are on white space and the "skipwhite" option was given
	     */
	    if (!found_match
		    && (   ((current_next_flags & HL_SKIPWHITE)
			    && vim_iswhite(line[current_col]))
			|| ((current_next_flags & HL_SKIPEMPTY)
			    && *line == NUL)))
		break;

	    /*
	     * If a nextgroup was found: Use it, and continue looking for
	     * contained matches.
	     * If a nextgroup was not found: Continue looking for a normal
	     * match.
	     */
	    current_next_list = NULL;
	    next_match_idx = -1;
	    found_match = TRUE;
	}

    } while (found_match);

    /*
     * Use attributes from the current state, if within its highlighting.
     * If not, use attributes from the current-but-one state, etc.
     */
    current_attr = 0;
#ifdef WANT_EVAL
    current_id = 0;
    current_trans_id = 0;
#endif
    if (cur_si != NULL)
    {
	for (idx = current_state.ga_len - 1; idx >= 0; --idx)
	{
	    sip = &CUR_STATE(idx);
	    if ((int)current_col >= sip->si_h_startcol
				      && (int)current_col <= sip->si_h_endcol)
	    {
		current_attr = sip->si_attr;
#ifdef WANT_EVAL
		current_id = sip->si_id;
		current_trans_id = sip->si_trans_id;
#endif
		break;
	    }
	}

	/*
	 * Check for end of current state (and the states before it) at the
	 * next column.  Don't do this for syncing, because we would miss a
	 * single character match.
	 */
	if (!syncing)
	{
	    ++current_col;
	    check_state_ends(line);
	    --current_col;
	}
    }

    /* nextgroup ends at end of line, unless "skipnl" or "skipemtpy" present */
    if (current_next_list != NULL
	    && line[current_col + 1] == NUL
	    && !(current_next_flags & (HL_SKIPNL | HL_SKIPEMPTY)))
	current_next_list = NULL;

    return current_attr;
}

/*
 * Check if we already matched pattern "idx" at the current column.
 */
    static int
did_match_already(idx)
    int	    idx;
{
    int		i;

    for (i = current_state.ga_len; --i >= 0; )
    {
	if (CUR_STATE(i).si_m_startcol == (int)current_col
		&& CUR_STATE(i).si_m_lnum == (int)current_lnum
		&& CUR_STATE(i).si_idx == idx)
	    return TRUE;
    }
    return FALSE;
}

/*
 * Push the next match onto the stack.
 */
    static struct state_item *
push_next_match(cur_si, line)
    struct state_item	*cur_si;
    char_u		*line;
{
    struct syn_pattern	*spp;

    spp = &(SYN_ITEMS(syn_buf)[next_match_idx]);

    /*
     * Push the item in current_state stack;
     */
    if (push_current(next_match_idx) == OK)
    {
	/*
	 * If it's a start-skip-end type that crosses lines, figure out how
	 * much it continues in this line.  Otherwise just fill in the length.
	 */
	cur_si = &CUR_STATE(current_state.ga_len - 1);
	cur_si->si_h_startcol = next_match_h_startcol;
	cur_si->si_m_startcol = current_col;
	cur_si->si_m_lnum = current_lnum;
	cur_si->si_flags = spp->sp_flags;
	cur_si->si_next_list = spp->sp_next_list;
	if (spp->sp_type == SPTYPE_START && !(spp->sp_flags & HL_ONELINE))
	{
	    update_si_end(cur_si, line, next_match_m_endcol);
	}
	else
	{
	    cur_si->si_m_endcol = next_match_m_endcol - 1;
	    cur_si->si_h_endcol = next_match_h_endcol - 1;
	    cur_si->si_ends = TRUE;
	    cur_si->si_flags |= next_match_flags;
	    cur_si->si_eoe_col = next_match_eoe_col;
	    cur_si->si_end_idx = next_match_end_idx;
	}
	if (keepend_level < 0 && (cur_si->si_flags & HL_KEEPEND))
	    keepend_level = current_state.ga_len - 1;
	check_keepend();
	update_si_attr(current_state.ga_len - 1);

	/*
	 * If the start pattern has another highlight group, push another item
	 * on the stack for the start pattern.
	 */
	if (	   spp->sp_type == SPTYPE_START
		&& spp->sp_syn_match_id != 0
		&& push_current(next_match_idx) == OK)
	{
	    cur_si = &CUR_STATE(current_state.ga_len - 1);
	    cur_si->si_h_startcol = next_match_h_startcol;
	    cur_si->si_m_startcol = current_col;
	    cur_si->si_m_lnum = current_lnum;
	    cur_si->si_m_endcol = next_match_eos_col;
	    cur_si->si_h_endcol = next_match_eos_col;
	    cur_si->si_ends = TRUE;
	    cur_si->si_end_idx = 0;
	    cur_si->si_flags = HL_MATCH;
	    cur_si->si_next_list = NULL;
	    check_keepend();
	    update_si_attr(current_state.ga_len - 1);
	}
    }

    next_match_idx = -1;	/* try other match next time */

    return cur_si;
}

/*
 * Check for end of current state (and the states before it).
 */
    static void
check_state_ends(line)
    char_u		*line;
{
    struct state_item	*cur_si;

    cur_si = &CUR_STATE(current_state.ga_len - 1);
    for (;;)
    {
	if (cur_si->si_m_endcol < (int)current_col && cur_si->si_ends)
	{
	    /*
	     * If there is an end pattern group ID, highlight the end pattern
	     * now.  No need to pop the current item from the stack.
	     * Only do this if the end pattern continuous beyond the current
	     * position.
	     */
	    if (cur_si->si_end_idx && cur_si->si_eoe_col >= (int)current_col)
	    {
		cur_si->si_idx = cur_si->si_end_idx;
		cur_si->si_end_idx = 0;
		cur_si->si_m_endcol = cur_si->si_eoe_col;
		cur_si->si_h_endcol = cur_si->si_eoe_col;
		cur_si->si_flags |= HL_MATCH;
		update_si_attr(current_state.ga_len - 1);
		break;
	    }
	    else
	    {
		/* handle next_list, unless at end of line and no "skipnl" or
		 * "skipempty" */
		current_next_list = cur_si->si_next_list;
		current_next_flags = cur_si->si_flags;
		if (!(current_next_flags & (HL_SKIPNL | HL_SKIPEMPTY))
			&& line[current_col] == NUL)
		    current_next_list = NULL;
		pop_current();

		if (current_state.ga_len == 0)
		    break;
		cur_si = &CUR_STATE(current_state.ga_len - 1);

		/*
		 * Only for a region the search for the end continues after
		 * the end of the contained item.  If the contained match
		 * included the end-of-line, break here, the region continues.
		 * Don't do this when:
		 * - "keepend" is used for the contained item
		 * - not at the end of the line (could be end="x$"me=e-1).
		 * - "excludenl" is used (HL_HAS_EOL won't be set)
		 */
		if (SYN_ITEMS(syn_buf)[cur_si->si_idx].sp_type == SPTYPE_START
			     && !(cur_si->si_flags & (HL_MATCH | HL_KEEPEND)))
		{
		    update_si_end(cur_si, line, (int)current_col);
		    if ((current_next_flags & HL_HAS_EOL)
			    && keepend_level < 0
			    && line[current_col] == NUL)
			break;
		}
	    }
	}
	else
	    break;
    }
}

/*
 * Update an entry in the current_state stack for a match or region.  This
 * fills in si_attr, si_next_list and si_cont_list.
 */
    static void
update_si_attr(idx)
    int	    idx;
{
    struct state_item	*sip = &CUR_STATE(idx);
    struct syn_pattern	*spp;

    spp = &(SYN_ITEMS(syn_buf)[sip->si_idx]);
    if (sip->si_flags & HL_MATCH)
	sip->si_id = spp->sp_syn_match_id;
    else
	sip->si_id = spp->sp_syn_id;
    sip->si_attr = syn_id2attr(sip->si_id);
    sip->si_trans_id = sip->si_id;
    if (sip->si_flags & HL_MATCH)
	sip->si_cont_list = NULL;
    else
	sip->si_cont_list = spp->sp_cont_list;

    /*
     * For transparent items, take attr from outer item.
     * Also take cont_list, if there is none.
     * Don't do this for the matchgroup of a start or end pattern.
     */
    if ((spp->sp_flags & HL_TRANSP) && !(sip->si_flags & HL_MATCH))
    {
	if (idx == 0)
	{
	    sip->si_attr = 0;
	    sip->si_trans_id = 0;
	    if (sip->si_cont_list == NULL)
		sip->si_cont_list = ID_LIST_ALL;
	}
	else
	{
	    sip->si_attr = CUR_STATE(idx - 1).si_attr;
	    sip->si_trans_id = CUR_STATE(idx - 1).si_trans_id;
	    if (sip->si_cont_list == NULL)
		sip->si_cont_list = CUR_STATE(idx - 1).si_cont_list;
	}
    }
}

/*
 * Check the current stack for patterns with "keepend" flag.  Propagate the
 * match-end to contained items.
 */
    static void
check_keepend()
{
    int			i;
    int			maxend = MAXCOL;
    struct state_item	*sip;

    /*
     * This check can consume a lot of time; only do it from the level where
     * there really is a keepend.
     */
    if (keepend_level < 0)
	return;

    for (i = keepend_level; i < current_state.ga_len; ++i)
    {
	sip = &CUR_STATE(i);
	if (maxend < MAXCOL)
	{
	    if (sip->si_m_endcol > maxend)
		sip->si_m_endcol = maxend;
	    if (sip->si_h_endcol > maxend)
		sip->si_h_endcol = maxend;
	    if (sip->si_eoe_col > maxend)
		sip->si_eoe_col = maxend;
	    sip->si_ends = TRUE;
	}
	if (sip->si_ends && (sip->si_flags & HL_KEEPEND)
						 && maxend > sip->si_m_endcol)
	    maxend = sip->si_m_endcol;
    }
}

/*
 * Update an entry in the current_state stack for a start-skip-end pattern.
 * This finds the end of the current item, if it's in the current line.
 *
 * Return the flags for the matched END.
 */
    static void
update_si_end(sip, line, startcol)
    struct state_item	*sip;
    char_u		*line;
    int			startcol;   /* where to start searching for the end */
{
    char_u		*endp;
    char_u		*hl_endp;
    char_u		*end_endp;
    int			end_idx;

    /*
     * We need to find the end of the match.  It may continue in the next
     * line.
     */
    end_idx = 0;
    endp = find_endp(sip->si_idx, line + startcol,
	       startcol == 0, &hl_endp, &(sip->si_flags), &end_endp, &end_idx);
    if (endp == NULL)
    {
	/* continues on next line */
	if (SYN_ITEMS(syn_buf)[sip->si_idx].sp_flags & HL_ONELINE)
	{
	    sip->si_ends = TRUE;
	    sip->si_m_endcol = STRLEN(line) - 1;
	}
	else
	{
	    sip->si_ends = FALSE;
	    sip->si_m_endcol = MAXCOL;
	}
	sip->si_h_endcol = sip->si_m_endcol;
    }
    else
    {
	/* match within this line */
	sip->si_m_endcol = endp - line - 1;
	sip->si_h_endcol = hl_endp - line - 1;
	sip->si_ends = TRUE;
	sip->si_eoe_col = end_endp - line - 1;
	sip->si_end_idx = end_idx;
    }
    check_keepend();
}

/*
 * Add a new state to the current state stack.
 * Return FAIL if it's not possible (out of memory).
 */
    static int
push_current(idx)
    int	    idx;
{
    if (ga_grow(&current_state, 1) == FAIL)
	return FAIL;
    vim_memset(&CUR_STATE(current_state.ga_len), 0, sizeof(struct state_item));
    CUR_STATE(current_state.ga_len).si_idx = idx;
    ++current_state.ga_len;
    --current_state.ga_room;
    return OK;
}

/*
 * Remove a state from the current_state stack.
 */
    static void
pop_current()
{
    if (current_state.ga_len)
    {
	--current_state.ga_len;
	++current_state.ga_room;
    }
    /* after the end of a pattern, try matching a keyword or pattern */
    next_match_idx = -1;

    /* if first state with "keepend" is popped, reset keepend_level */
    if (keepend_level >= current_state.ga_len)
	keepend_level = -1;
}

/*
 * Find the end of a start/skip/end pattern match.
 */
    static char_u *
find_endp(idx, sstart, at_bol, hl_endp, flagsp, end_endp, end_idx)
    int	    idx;	/* index of the pattern */
    char_u  *sstart;	/* where to start looking for an END match */
    int	    at_bol;	/* if sstart is at begin-of-line */
    char_u  **hl_endp;	/* end column for highlighting */
    int	    *flagsp;	/* flags of matching END */
    char_u  **end_endp;	/* end of end pattern match */
    int	    *end_idx;	/* group ID for end pattern match, or 0 */
{
    char_u		*endp;		    /* end of highlighting */
    struct syn_pattern	*spp, *spp_skip;
    char_u		*p;		    /* end of match */
    int			start_idx;
    int			best_idx;
    char_u		*best_ptr;

    /*
     * Check for being called with a START pattern.
     * Can happen with a match that continues to the next line, because it
     * contained a region.
     */
    spp = &(SYN_ITEMS(syn_buf)[idx]);
    if (spp->sp_type != SPTYPE_START)
    {
	*hl_endp = sstart;
	return sstart;
    }

    /*
     * Find the SKIP or first END pattern after the last START pattern.
     */
    for (;;)
    {
	spp = &(SYN_ITEMS(syn_buf)[idx]);
	if (spp->sp_type != SPTYPE_START)
	    break;
	++idx;
    }

    /*
     *	Lookup the SKIP pattern (if present)
     */
    if (spp->sp_type == SPTYPE_SKIP)
    {
	spp_skip = spp;
	++idx;
    }
    else
	spp_skip = NULL;

    endp = sstart;	    /* start looking for a match at sstart */
    start_idx = idx;	    /* remember the first END pattern. */
    for (;;)
    {
	best_idx = -1;
	best_ptr = NULL;
	for (idx = start_idx; idx < syn_buf->b_syn_patterns.ga_len; ++idx)
	{
	    spp = &(SYN_ITEMS(syn_buf)[idx]);
	    if (spp->sp_type != SPTYPE_END)	/* past last END pattern */
		break;

	    reg_ic = spp->sp_ic;
	    if (syn_regexec(spp->sp_prog, endp, (at_bol && endp == sstart)))
	    {
		if (best_idx == -1 || spp->sp_prog->startp[0] < best_ptr)
		{
		    best_idx = idx;
		    best_ptr = spp->sp_prog->startp[0];
		}
	    }
	}

	/*
	 * If all end patterns have been tried, and there is no match, the
	 * item continues until end-of-line.
	 */
	if (best_idx == -1)
	    break;

	/*
	 * If the skip pattern matches before the end pattern,
	 * continue searching after the skip pattern.
	 */
	if (	   spp_skip != NULL
		&& (reg_ic = spp_skip->sp_ic,
			syn_regexec(spp_skip->sp_prog, endp,
						  (at_bol && endp == sstart)))
		&& spp_skip->sp_prog->startp[0] <= best_ptr)
	{
	    /* Add offset to skip pattern match */
	    p = syn_add_end_off(spp_skip, SPO_ME_OFF, 1);

	    /* take care of an empty match or negative offset */
	    if (p <= endp)
		++endp;
	    else if (p <= spp_skip->sp_prog->endp[0])
		endp = p;
	    else
		/* Be careful not to jump over the NUL at the end-of-line */
		for (endp = spp_skip->sp_prog->endp[0];
					     *endp != NUL && endp < p; ++endp)
		    ;

	    /* if skip pattern includes end-of-line, return here */
	    if (*endp == NUL)
		break;

	    continue;	    /* start with first end pattern again */
	}

	/*
	 * Match from start pattern to end pattern.
	 * Correct for match and highlight offset of end pattern.
	 */
	spp = &(SYN_ITEMS(syn_buf)[best_idx]);
	p = syn_add_end_off(spp, SPO_ME_OFF, 1);
	if (p < sstart)
	    p = sstart;

	endp = syn_add_end_off(spp, SPO_HE_OFF, 1);
	if (endp < sstart)
	    endp = sstart;
	if (endp > p)
	    endp = p;
	*end_endp = endp;

	/*
	 * If the end group is highlighted differently, adjust the pointers.
	 */
	if (spp->sp_syn_match_id != spp->sp_syn_id && spp->sp_syn_match_id != 0)
	{
	    *end_idx = best_idx;
	    if (spp->sp_off_flags & (1 << (SPO_RE_OFF + SPO_COUNT)))
		endp = spp->sp_prog->endp[0] + spp->sp_offsets[SPO_RE_OFF];
	    else
		endp = spp->sp_prog->startp[0] + spp->sp_offsets[SPO_RE_OFF];
	    if (endp < sstart)
		endp = sstart;
	    if (endp > p)
		endp = p;
	    p = endp;
	}
	else
	    *end_idx = 0;
	*hl_endp = endp;
	*flagsp = spp->sp_flags;

	return p;
    }

    return NULL;	/* no match for an END pattern in this line */
}

/*
 * Add offset to matched text for end of match or highlight.
 */
    static char_u *
syn_add_end_off(spp, idx, extra)
    struct syn_pattern	*spp;
    int			idx;
    int			extra;	    /* extra chars for offset to start */
{
    if (spp->sp_off_flags & (1 << idx))
	return spp->sp_prog->startp[0] + spp->sp_offsets[idx] + extra;
    return spp->sp_prog->endp[0] + spp->sp_offsets[idx];
}

/*
 * Add offset to matched text for start of match or highlight.
 */
    static char_u *
syn_add_start_off(spp, idx, extra)
    struct syn_pattern	*spp;
    int			idx;
    int			extra;	    /* extra chars for offset to end */
{
    if (spp->sp_off_flags & (1 << (idx + SPO_COUNT)))
	return spp->sp_prog->endp[0] + spp->sp_offsets[idx] + extra;
    return spp->sp_prog->startp[0] + spp->sp_offsets[idx];
}

/*
 * Check one position in a line for a matching keyword.
 * The caller must check if a keyword can start at startcol.
 * Return it's ID if found, 0 otherwise.
 */
    static int
check_keyword_id(line, startcol, endcol, flags, next_list, cur_si)
    char_u		*line;
    int			startcol;   /* position in line to check for keyword */
    int			*endcol;    /* last character of found keyword */
    int			*flags;	    /* flags of matching keyword */
    short		**next_list; /* next_list of matching keyword */
    struct state_item	*cur_si;    /* item at the top of the stack */
{
    struct keyentry *ktab;
    char_u	    *p;
    int		    round;
    int		    hash;
    int		    len;
    char_u	    keyword[MAXKEYWLEN + 1]; /* assume max. keyword len is 80 */

    /* Find first character after the keyword */
    p = line + startcol;
    for (len = 1; vim_iswordc_buf(p[len], syn_buf); ++len)
	;
    if (len > MAXKEYWLEN)
	return 0;

    /*
     * Must make a copy of the keyword, so we can add a NUL and make it
     * lowercase.
     */
    STRNCPY(keyword, p, len);
    keyword[len] = NUL;

    /*
     * Try twice:
     * 1. matching case
     * 2. ignoring case
     */
    for (round = 1; round <= 2; ++round)
    {
	if ((round == 1 ? syn_buf->b_keywtab : syn_buf->b_keywtab_ic) == NULL)
	    continue;
	p = keyword;
	hash = 0;
	if (round == 1)	/* match case */
	{
	    while (*p)
		hash += *p++;
	    ktab = syn_buf->b_keywtab[hash & KHASH_MASK];
	}
	else /* round == 2, ignore case */
	{
	    while (*p)
	    {
		hash += (*p = TO_LOWER(*p));
		++p;
	    }
	    ktab = syn_buf->b_keywtab_ic[hash & KHASH_MASK];
	}

	/*
	 * Find keywords that match.
	 * When current_next_list is non-zero accept only that group, otherwise:
	 *  Accept a not-contained keyword at toplevel.
	 *  Accept a keyword at other levels only if it is in the contains list.
	 */
	for ( ; ktab != NULL; ktab = ktab->next)
	    if (   STRCMP(keyword, ktab->keyword) == 0
		&& (   (current_next_list != 0
			&& in_id_list(current_next_list, ktab->syn_id,
				      ktab->syn_inc_tag, 0))
		    || (current_next_list == 0
			&& ((cur_si == NULL && !(ktab->flags & HL_CONTAINED))
			    || (cur_si != NULL
				&& in_id_list(cur_si->si_cont_list,
					ktab->syn_id,
					ktab->syn_inc_tag,
					ktab->flags & HL_CONTAINED))))))
	    {
		*endcol = startcol + len - 1;
		*flags = ktab->flags;
		*next_list = ktab->next_list;
		return ktab->syn_id;
	    }
    }
    return 0;
}

/*
 * Handle ":syntax case" command.
 */
/* ARGSUSED */
    static void
syn_cmd_case(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* not used */
{
    char_u	*arg = eap->arg;
    char_u	*next;

    eap->nextcmd = find_nextcmd(arg);
    if (eap->skip)
	return;

    next = skiptowhite(arg);
    if (STRNICMP(arg, "match", 5) == 0 && next - arg == 5)
	curbuf->b_syn_ic = FALSE;
    else if (STRNICMP(arg, "ignore", 6) == 0 && next - arg == 6)
	curbuf->b_syn_ic = TRUE;
    else
	EMSG2("Illegal argument: %s", arg);
}

/*
 * Clear all syntax info for one buffer.
 */
    void
syntax_clear(buf)
    BUF	    *buf;
{
    int i;

    curbuf->b_syn_ic = FALSE;	    /* Use case, by default */

    /* free the keywords */
    free_keywtab(buf->b_keywtab);
    buf->b_keywtab = NULL;
    free_keywtab(buf->b_keywtab_ic);
    buf->b_keywtab_ic = NULL;

    /* free the syntax patterns */
    for (i = buf->b_syn_patterns.ga_len; --i >= 0; )
	syn_clear_pattern(buf, i);
    ga_clear(&buf->b_syn_patterns);

    /* free the syntax clusters */
    for (i = buf->b_syn_clusters.ga_len; --i >= 0; )
	syn_clear_cluster(buf, i);
    ga_clear(&buf->b_syn_clusters);

    buf->b_syn_sync_flags = 0;
    buf->b_syn_sync_minlines = 0;
    buf->b_syn_sync_maxlines = 0;

    vim_free(buf->b_syn_linecont_prog);
    buf->b_syn_linecont_prog = NULL;
    vim_free(buf->b_syn_linecont_pat);
    buf->b_syn_linecont_pat = NULL;

    /* free the stored states */
    syn_free_all_states(buf);
    invalidate_current_state();
}

/*
 * Clear syncing info for one buffer.
 */
    static void
syntax_sync_clear()
{
    int i;

    /* free the syntax patterns */
    for (i = curbuf->b_syn_patterns.ga_len; --i >= 0; )
	if (SYN_ITEMS(curbuf)[i].sp_syncing)
	    syn_remove_pattern(curbuf, i);

    curbuf->b_syn_sync_flags = 0;
    curbuf->b_syn_sync_minlines = 0;
    curbuf->b_syn_sync_maxlines = 0;

    vim_free(curbuf->b_syn_linecont_prog);
    curbuf->b_syn_linecont_prog = NULL;
    vim_free(curbuf->b_syn_linecont_pat);
    curbuf->b_syn_linecont_pat = NULL;
}

/*
 * Remove one pattern from the buffer's pattern list.
 */
    static void
syn_remove_pattern(buf, idx)
    BUF	    *buf;
    int	    idx;
{
    struct syn_pattern	*spp;

    spp = &(SYN_ITEMS(buf)[idx]);
    syn_clear_pattern(buf, idx);
    mch_memmove(spp, spp + 1, sizeof(struct syn_pattern) *
				      (buf->b_syn_patterns.ga_len - idx - 1));
    --buf->b_syn_patterns.ga_len;
    --buf->b_syn_patterns.ga_room;
}

/*
 * Clear and free one syntax pattern.  When clearing all, must be called from
 * last to first!
 */
    static void
syn_clear_pattern(buf, i)
    BUF	    *buf;
    int	    i;
{
    vim_free(SYN_ITEMS(buf)[i].sp_pattern);
    vim_free(SYN_ITEMS(buf)[i].sp_prog);
    /* Only free sp_cont_list and sp_next_list of first start pattern */
    if (i == 0 || SYN_ITEMS(buf)[i - 1].sp_type != SPTYPE_START)
    {
	vim_free(SYN_ITEMS(buf)[i].sp_cont_list);
	vim_free(SYN_ITEMS(buf)[i].sp_next_list);
    }
}

/*
 * Clear and free one syntax cluster.
 */
    static void
syn_clear_cluster(buf, i)
    BUF	    *buf;
    int	    i;
{
    vim_free(SYN_CLSTR(buf)[i].scl_name);
    vim_free(SYN_CLSTR(buf)[i].scl_name_u);
    vim_free(SYN_CLSTR(buf)[i].scl_list);
}

/*
 * Handle ":syntax clear" command.
 */
    static void
syn_cmd_clear(eap, syncing)
    EXARG	*eap;
    int		syncing;
{
    char_u	*arg = eap->arg;
    char_u	*arg_end;
    int		id;

    eap->nextcmd = find_nextcmd(arg);
    if (eap->skip)
	return;

    /*
     * We have to disable this within ":syn include @group filename",
     * because otherwise @group would get deleted.
     */
    if (curbuf->b_syn_topgrp != 0)
	return;

    if (ends_excmd(*arg))
    {
	/*
	 * No argument: Clear all syntax items.
	 */
	if (syncing)
	    syntax_sync_clear();
	else
	    syntax_clear(curbuf);
    }
    else
    {
	/*
	 * Clear the group IDs that are in the argument.
	 */
	while (!ends_excmd(*arg))
	{
	    arg_end = skiptowhite(arg);
	    if (*arg == '@')
	    {
		id = syn_scl_namen2id(arg + 1, (int)(arg_end - arg - 1));
		if (id == 0)
		{
		    EMSG2("No such syntax cluster: %s", arg);
		    break;
		}
		else
		{
		    /*
		     * We can't physically delete a cluster without changing
		     * the IDs of other clusters, so we do the next best thing
		     * and make it empty.
		     */
		    short scl_id = id - CLUSTER_ID_MIN;

		    vim_free(SYN_CLSTR(curbuf)[scl_id].scl_list);
		    SYN_CLSTR(curbuf)[scl_id].scl_list = NULL;
		}
	    }
	    else
	    {
		id = syn_namen2id(arg, (int)(arg_end - arg));
		if (id == 0)
		{
		    EMSG2("No such highlight group name: %s", arg);
		    break;
		}
		else
		    syn_clear_one(id, syncing);
	    }
	    arg = skipwhite(arg_end);
	}
    }
    redraw_curbuf_later(NOT_VALID);
    /* invalidate saved status, it may contain a deleted item */
    syn_changed((linenr_t)0);
}

/*
 * Clear one syntax group for the current buffer.
 */
    static void
syn_clear_one(id, syncing)
    int	    id;
    int	    syncing;
{
    struct syn_pattern	*spp;
    int			idx;

    /* Clear keywords only when not ":syn sync clear group-name" */
    if (!syncing)
    {
	(void)syn_clear_keyword(id, curbuf->b_keywtab);
	(void)syn_clear_keyword(id, curbuf->b_keywtab_ic);
    }

    /* clear the patterns for "id" */
    for (idx = curbuf->b_syn_patterns.ga_len; --idx >= 0; )
    {
	spp = &(SYN_ITEMS(curbuf)[idx]);
	if (spp->sp_syn_id != id || spp->sp_syncing != syncing)
	    continue;
	syn_remove_pattern(curbuf, idx);
    }
}

/*
 * Handle ":syntax on" command.
 */
/* ARGSUSED */
    static void
syn_cmd_on(eap, syncing)
    EXARG	*eap;
    int		syncing;	/* not used */
{
    syn_cmd_onoff(eap, "syntax");
}

/*
 * Handle ":syntax manual" command.
 */
/* ARGSUSED */
    static void
syn_cmd_manual(eap, syncing)
    EXARG	*eap;
    int		syncing;	/* not used */
{
    syn_cmd_onoff(eap, "manual");
}

/*
 * Handle ":syntax off" command.
 */
/* ARGSUSED */
    static void
syn_cmd_off(eap, syncing)
    EXARG	*eap;
    int		syncing;	/* not used */
{
    syn_cmd_onoff(eap, "nosyntax");
}

    static void
syn_cmd_onoff(eap, name)
    EXARG	*eap;
    char	*name;
{
    char_u	buf[100];

    eap->nextcmd = check_nextcmd(eap->arg);
    if (!eap->skip)
    {
	STRCPY(buf, "so ");
	sprintf((char *)buf + 3, SYNTAX_FNAME, name);
	do_cmdline(buf, NULL, NULL, DOCMD_VERBOSE);
    }
}

/*
 * Handle ":syntax [list]" command: list current syntax words.
 */
    static void
syn_cmd_list(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* when TRUE: list syncing items */
{
    char_u	*arg = eap->arg;
    int		id;
    char_u	*arg_end;

    eap->nextcmd = find_nextcmd(arg);
    if (eap->skip)
	return;

    if (!syntax_present(curbuf))
    {
	MSG("No Syntax items defined for this buffer");
	return;
    }

    if (syncing)
    {
	if (curbuf->b_syn_sync_flags & SF_CCOMMENT)
	{
	    MSG_PUTS("syncing on C-style comments");
	    if (curbuf->b_syn_sync_minlines || curbuf->b_syn_sync_maxlines)
		syn_lines_msg();
	    return;
	}
	else if (!(curbuf->b_syn_sync_flags & SF_MATCH))
	{
	    MSG_PUTS("syncing starts ");
	    msg_outnum(curbuf->b_syn_sync_minlines);
	    MSG_PUTS(" lines before top line");
	    return;
	}
	MSG_PUTS_TITLE("\n--- Syntax sync items ---");
	if (curbuf->b_syn_sync_minlines || curbuf->b_syn_sync_maxlines)
	{
	    MSG_PUTS("\nsyncing on items");
	    syn_lines_msg();
	}
    }
    else
	MSG_PUTS_TITLE("\n--- Syntax items ---");
    if (ends_excmd(*arg))
    {
	/*
	 * No argument: List all group IDs and all syntax clusters.
	 */
	for (id = 1; id <= highlight_ga.ga_len && !got_int; ++id)
	    syn_list_one(id, syncing, FALSE);
	for (id = 0; id < curbuf->b_syn_clusters.ga_len && !got_int; ++id)
	    syn_list_cluster(id);
    }
    else
    {
	/*
	 * List the group IDs and syntax clusters that are in the argument.
	 */
	while (!ends_excmd(*arg) && !got_int)
	{
	    arg_end = skiptowhite(arg);
	    if (*arg == '@')
	    {
		id = syn_scl_namen2id(arg + 1, (int)(arg_end - arg - 1));
		if (id == 0)
		    EMSG2("No such syntax cluster: %s", arg);
		else
		    syn_list_cluster(id - CLUSTER_ID_MIN);
	    }
	    else
	    {
		id = syn_namen2id(arg, (int)(arg_end - arg));
		if (id == 0)
		    EMSG2("No such highlight group name: %s", arg);
		else
		    syn_list_one(id, syncing, TRUE);
	    }
	    arg = skipwhite(arg_end);
	}
    }
    eap->nextcmd = check_nextcmd(arg);
}

    static void
syn_lines_msg()
{
    MSG_PUTS("; ");
    if (curbuf->b_syn_sync_minlines)
    {
	MSG_PUTS("minimal ");
	msg_outnum(curbuf->b_syn_sync_minlines);
	if (curbuf->b_syn_sync_maxlines)
	    MSG_PUTS(", ");
    }
    if (curbuf->b_syn_sync_maxlines)
    {
	MSG_PUTS("maximal ");
	msg_outnum(curbuf->b_syn_sync_maxlines);
    }
    MSG_PUTS(" lines before top line");
}

static int  last_matchgroup;

/*
 * List one syntax item, for ":syntax" or "syntax list syntax_name".
 */
    static void
syn_list_one(id, syncing, link_only)
    int	    id;
    int	    syncing;	    /* when TRUE: list syncing items */
    int	    link_only;	    /* when TRUE; list link-only too */
{
    int			attr;
    int			idx;
    int			did_header = FALSE;
    struct syn_pattern	*spp;

    attr = hl_attr(HLF_D);		/* highlight like directories */

    /* list the keywords for "id" */
    if (!syncing)
    {
	did_header = syn_list_keywords(id, curbuf->b_keywtab, FALSE, attr);
	did_header = syn_list_keywords(id, curbuf->b_keywtab_ic,
							    did_header, attr);
    }

    /* list the patterns for "id" */
    for (idx = 0; idx < curbuf->b_syn_patterns.ga_len && !got_int; ++idx)
    {
	spp = &(SYN_ITEMS(curbuf)[idx]);
	if (spp->sp_syn_id != id || spp->sp_syncing != syncing)
	    continue;

	(void)syn_list_header(did_header, 999, id);
	did_header = TRUE;
	last_matchgroup = 0;
	if (spp->sp_type == SPTYPE_MATCH)
	{
	    put_pattern("match", ' ', spp, attr);
	    msg_putchar(' ');
	}
	else if (spp->sp_type == SPTYPE_START)
	{
	    while (SYN_ITEMS(curbuf)[idx].sp_type == SPTYPE_START)
		put_pattern("start", '=', &SYN_ITEMS(curbuf)[idx++], attr);
	    if (SYN_ITEMS(curbuf)[idx].sp_type == SPTYPE_SKIP)
		put_pattern("skip", '=', &SYN_ITEMS(curbuf)[idx++], attr);
	    while (idx < curbuf->b_syn_patterns.ga_len
			      && SYN_ITEMS(curbuf)[idx].sp_type == SPTYPE_END)
		put_pattern("end", '=', &SYN_ITEMS(curbuf)[idx++], attr);
	    --idx;
	    msg_putchar(' ');
	}
	if (spp->sp_flags & HL_CONTAINED)
	{
	    msg_puts_attr((char_u *)"contained", attr);
	    msg_putchar(' ');
	}
	if (spp->sp_flags & HL_ONELINE)
	{
	    msg_puts_attr((char_u *)"oneline", attr);
	    msg_putchar(' ');
	}
	if (spp->sp_flags & HL_KEEPEND)
	{
	    msg_puts_attr((char_u *)"keepend", attr);
	    msg_putchar(' ');
	}
	if (spp->sp_flags & HL_EXCLUDENL)
	{
	    msg_puts_attr((char_u *)"excludenl", attr);
	    msg_putchar(' ');
	}
	if (spp->sp_flags & HL_TRANSP)
	{
	    msg_puts_attr((char_u *)"transparent", attr);
	    msg_putchar(' ');
	}
	if (spp->sp_cont_list != NULL)
	{
	    put_id_list((char_u *)"contains", spp->sp_cont_list, attr);
	}
	if (spp->sp_next_list != NULL)
	{
	    put_id_list((char_u *)"nextgroup", spp->sp_next_list, attr);
	    if (spp->sp_flags & HL_SKIPWHITE)
	    {
		msg_puts_attr((char_u *)"skipwhite", attr);
		msg_putchar(' ');
	    }
	    if (spp->sp_flags & HL_SKIPNL)
	    {
		msg_puts_attr((char_u *)"skipnl", attr);
		msg_putchar(' ');
	    }
	    if (spp->sp_flags & HL_SKIPEMPTY)
	    {
		msg_puts_attr((char_u *)"skipempty", attr);
		msg_putchar(' ');
	    }
	}
	if (spp->sp_flags & (HL_SYNC_HERE|HL_SYNC_THERE))
	{
	    if (spp->sp_flags & HL_SYNC_HERE)
		msg_puts_attr((char_u *)"grouphere", attr);
	    else
		msg_puts_attr((char_u *)"groupthere", attr);
	    msg_putchar(' ');
	    if (spp->sp_sync_idx >= 0)
		msg_outtrans(HL_TABLE()[SYN_ITEMS(curbuf)
				   [spp->sp_sync_idx].sp_syn_id - 1].sg_name);
	    else
		MSG_PUTS("NONE");
	    msg_putchar(' ');
	}
    }

    /* list the link, if there is one */
    if (HL_TABLE()[id - 1].sg_link && (did_header || link_only) && !got_int)
    {
	(void)syn_list_header(did_header, 999, id);
	msg_puts_attr((char_u *)"links to", attr);
	msg_putchar(' ');
	msg_outtrans(HL_TABLE()[HL_TABLE()[id - 1].sg_link - 1].sg_name);
    }
}

/*
 * List one syntax cluster, for ":syntax" or "syntax list syntax_name".
 */
    static void
syn_list_cluster(id)
    int id;
{
    int	    endcol = 15;

    /* slight hack:  roughly duplicate the guts of syn_list_header() */
    msg_putchar('\n');
    msg_outtrans(SYN_CLSTR(curbuf)[id].scl_name);

    if (msg_col >= endcol)	/* output at least one space */
	endcol = msg_col + 1;
    if (Columns <= endcol)	/* avoid hang for tiny window */
	endcol = Columns - 1;

    msg_advance(endcol);
    if (SYN_CLSTR(curbuf)[id].scl_list != NULL)
    {
	put_id_list((char_u *)"cluster", SYN_CLSTR(curbuf)[id].scl_list,
		    hl_attr(HLF_D));
    }
    else
    {
	msg_puts_attr((char_u *)"cluster", hl_attr(HLF_D));
	msg_puts((char_u *)"=NONE");
    }
}

    static void
put_id_list(name, list, attr)
    char_u	*name;
    short	*list;
    int		attr;
{
    short		*p;

    msg_puts_attr(name, attr);
    msg_putchar('=');
    for (p = list; *p; ++p)
    {
	if (*p >= CONTAINS_ALLBUT && *p < CLUSTER_ID_MIN)
	{
	    if (p[1])
		MSG_PUTS("ALLBUT");
	    else
		MSG_PUTS("ALL");
	}
	else if (*p >= CLUSTER_ID_MIN)
	{
	    short scl_id = *p - CLUSTER_ID_MIN;

	    msg_putchar('@');
	    msg_outtrans(SYN_CLSTR(curbuf)[scl_id].scl_name);
	}
	else
	    msg_outtrans(HL_TABLE()[*p - 1].sg_name);
	if (p[1])
	    msg_putchar(',');
    }
    msg_putchar(' ');
}

    static void
put_pattern(s, c, spp, attr)
    char		*s;
    int			c;
    struct syn_pattern	*spp;
    int			attr;
{
    long	n;
    int		mask;
    int		first;
    static char	*sepchars = "/+=-#@\"|'^&";
    int		i;

    /* May have to write "matchgroup=group" */
    if (last_matchgroup != spp->sp_syn_match_id)
    {
	last_matchgroup = spp->sp_syn_match_id;
	msg_puts_attr((char_u *)"matchgroup", attr);
	msg_putchar('=');
	if (last_matchgroup == 0)
	    msg_outtrans((char_u *)"NONE");
	else
	    msg_outtrans(HL_TABLE()[last_matchgroup - 1].sg_name);
	msg_putchar(' ');
    }

    /* output the name of the pattern and an '=' or ' ' */
    msg_puts_attr((char_u *)s, attr);
    msg_putchar(c);

    /* output the pattern, in between a char that is not in the pattern */
    for (i = 0; vim_strchr(spp->sp_pattern, sepchars[i]) != NULL; )
	if (sepchars[++i] == NUL)
	{
	    i = 0;	/* no good char found, just use the first one */
	    break;
	}
    msg_putchar(sepchars[i]);
    msg_outtrans(spp->sp_pattern);
    msg_putchar(sepchars[i]);

    /* output any pattern options */
    first = TRUE;
    for (i = 0; i < SPO_COUNT; ++i)
    {
	mask = (1 << i);
	if (spp->sp_off_flags & (mask + (mask << SPO_COUNT)))
	{
	    if (!first)
		msg_putchar(',');	/* separate with commas */
	    msg_puts((char_u *)spo_name_tab[i]);
	    n = spp->sp_offsets[i];
	    if (i != SPO_LC_OFF)
	    {
		if (spp->sp_off_flags & mask)
		    msg_putchar('s');
		else
		    msg_putchar('e');
		if (n > 0)
		    msg_putchar('+');
	    }
	    if (n || i == SPO_LC_OFF)
		msg_outnum(n);
	    first = FALSE;
	}
    }
    msg_putchar(' ');
}

/*
 * List or clear the keywords for one syntax group.
 * Return TRUE if the header has been printed.
 */
    static int
syn_list_keywords(id, ktabp, did_header, attr)
    int		    id;
    struct keyentry **ktabp;
    int		    did_header;		/* header has already been printed */
    int		    attr;
{
    int		    i;
    int		    outlen;
    struct keyentry *ktab;
    int		    prev_contained = 0;
    short	    *prev_next_list = NULL;
    int		    prev_skipnl = 0;
    int		    prev_skipwhite = 0;
    int		    prev_skipempty = 0;

    if (ktabp == NULL)
	return did_header;

    /*
     * Unfortunately, this list of keywords is not sorted on alphabet but on
     * hash value...
     */
    for (i = 0; i < KHASH_SIZE; ++i)
    {
	for (ktab = ktabp[i]; ktab != NULL && !got_int; ktab = ktab->next)
	{
	    if (ktab->syn_id == id)
	    {
		if (prev_contained != (ktab->flags & HL_CONTAINED)
			|| prev_skipnl != (ktab->flags & HL_SKIPNL)
			|| prev_skipwhite != (ktab->flags & HL_SKIPWHITE)
			|| prev_skipempty != (ktab->flags & HL_SKIPEMPTY)
			|| prev_next_list != ktab->next_list)
		    outlen = 9999;
		else
		    outlen = STRLEN(ktab->keyword);
		/* output "contained" and "nextgroup" on each line */
		if (syn_list_header(did_header, outlen, id))
		{
		    prev_contained = 0;
		    prev_next_list = NULL;
		    prev_skipnl = 0;
		    prev_skipwhite = 0;
		    prev_skipempty = 0;
		}
		did_header = TRUE;
		if (prev_contained != (ktab->flags & HL_CONTAINED))
		{
		    msg_puts_attr((char_u *)"contained", attr);
		    msg_putchar(' ');
		    prev_contained = (ktab->flags & HL_CONTAINED);
		}
		if (ktab->next_list != prev_next_list)
		{
		    put_id_list((char_u *)"nextgroup", ktab->next_list, attr);
		    msg_putchar(' ');
		    prev_next_list = ktab->next_list;
		    if (ktab->flags & HL_SKIPNL)
		    {
			msg_puts_attr((char_u *)"skipnl", attr);
			msg_putchar(' ');
			prev_skipnl = (ktab->flags & HL_SKIPNL);
		    }
		    if (ktab->flags & HL_SKIPWHITE)
		    {
			msg_puts_attr((char_u *)"skipwhite", attr);
			msg_putchar(' ');
			prev_skipwhite = (ktab->flags & HL_SKIPWHITE);
		    }
		    if (ktab->flags & HL_SKIPEMPTY)
		    {
			msg_puts_attr((char_u *)"skipempty", attr);
			msg_putchar(' ');
			prev_skipempty = (ktab->flags & HL_SKIPEMPTY);
		    }
		}
		msg_outtrans(ktab->keyword);
	    }
	}
    }

    return did_header;
}

    static void
syn_clear_keyword(id, ktabp)
    int		    id;
    struct keyentry **ktabp;
{
    int		    i;
    struct keyentry *ktab;
    struct keyentry *ktab_prev;
    struct keyentry *ktab_next;

    if (ktabp == NULL)	    /* no keywords present */
	return;

    for (i = 0; i < KHASH_SIZE; ++i)
    {
	ktab_prev = NULL;
	for (ktab = ktabp[i]; ktab != NULL; )
	{
	    if (ktab->syn_id == id)
	    {
		ktab_next = ktab->next;
		if (ktab_prev == NULL)
		    ktabp[i] = ktab_next;
		else
		    ktab_prev->next = ktab_next;
		vim_free(ktab);
		ktab = ktab_next;
	    }
	    else
	    {
		ktab_prev = ktab;
		ktab = ktab->next;
	    }
	}
    }
}

/*
 * Recursive function to free() a branch of a kwordtab.
 */
    static void
free_keywtab(ktabp)
    struct keyentry **ktabp;
{
    int		    i;
    struct keyentry *ktab;
    struct keyentry *ktab_next;

    if (ktabp != NULL)
    {
	for (i = 0; i < KHASH_SIZE; ++i)
	    for (ktab = ktabp[i]; ktab != NULL; ktab = ktab_next)
	    {
		ktab_next = ktab->next;
		vim_free(ktab->next_list);
		vim_free(ktab);
	    }
	vim_free(ktabp);
    }
}

/*
 * Add a keyword to the list of keywords.
 */
    static void
add_keyword(name, id, flags, next_list)
    char_u	*name;	    /* name of keyword */
    int		id;	    /* group ID for this keyword */
    int		flags;	    /* flags for this keyword */
    short	*next_list; /* nextgroup for this keyword */
{
    struct keyentry	*ktab;
    struct keyentry	***ktabpp;
    char_u		*p;
    int			hash;

    ktab = (struct keyentry *)alloc(
			       (int)(sizeof(struct keyentry) + STRLEN(name)));
    if (ktab == NULL)
	return;
    STRCPY(ktab->keyword, name);
    ktab->syn_id = id;
    ktab->syn_inc_tag = current_syn_inc_tag;
    ktab->flags = flags;
    ktab->next_list = copy_id_list(next_list);

    if (curbuf->b_syn_ic)
    {
	for (p = ktab->keyword; *p; ++p)
	    *p = TO_LOWER(*p);
	ktabpp = &curbuf->b_keywtab_ic;
    }
    else
	ktabpp = &curbuf->b_keywtab;

    if (*ktabpp == NULL)
    {
	*ktabpp = (struct keyentry **)alloc_clear(
			       (int)(sizeof(struct keyentry *) * KHASH_SIZE));
	if (*ktabpp == NULL)
	    return;
    }

    hash = 0;
    for (p = ktab->keyword; *p; ++p)
	hash += *p;
    hash &= KHASH_MASK;

    ktab->next = (*ktabpp)[hash];
    (*ktabpp)[hash] = ktab;
}

/*
 * Get the start and end of the group name argument.
 * Return a pointer to the first argument.
 * Return NULL if the end of the command was found instead of further args.
 */
    static char_u *
get_group_name(arg, name_end)
    char_u	*arg;		/* start of the argument */
    char_u	**name_end;	/* pointer to end of the name */
{
    char_u	*rest;

    *name_end = skiptowhite(arg);
    rest = skipwhite(*name_end);

    /*
     * Check if there are enough arguments.  The first argument may be a
     * pattern, where '|' is allowed, so only check for NUL.
     */
    if (ends_excmd(*arg) || *rest == NUL)
	return NULL;
    return rest;
}

/*
 * Check for syntax command option arguments.
 * This can be called at any place in the list of arguments, and just picks
 * out the arguments that are known.  Can be called several times in a row to
 * collect all options in between other arguments.
 * Return a pointer to the next argument (which isn't an option).
 * Return NULL for any error;
 */
    static char_u *
get_syn_options(arg, flagsp, sync_idx, cont_list, next_list)
    char_u	*arg;		/* next argument */
    int		*flagsp;	/* flags for contained and transpartent */
    int		*sync_idx;	/* syntax item for "grouphere" argument, NULL
				   if not allowed */
    short	**cont_list;	/* group IDs for "contains" argument, NULL if
				   not allowed */
    short	**next_list;	/* group IDs for "nextgroup" argument */
{
    int		flags;
    char_u	*gname_start, *gname;
    int		syn_id;
    int		len;
    int		i;
    int		fidx;
    static struct flag
    {
	char	*name;
	int	len;
	int	val;
    } flagtab[] = { {"contained",   9,	HL_CONTAINED},
		    {"oneline",	    7,	HL_ONELINE},
		    {"keepend",	    7,	HL_KEEPEND},
		    {"excludenl",   9,	HL_EXCLUDENL},
		    {"transparent", 11, HL_TRANSP},
		    {"skipnl",	    6,	HL_SKIPNL},
		    {"skipwhite",   9,	HL_SKIPWHITE},
		    {"skipempty",   9,	HL_SKIPEMPTY},
		    {"grouphere",   9,	HL_SYNC_HERE},
		    {"groupthere",  10,	HL_SYNC_THERE},
		    {"display",	    7,	0},
		    {"fold",	    4,	0},
		};

    if (arg == NULL)		/* already detected error */
	return NULL;

    flags = *flagsp;
    for (;;)
    {
	for (fidx = sizeof(flagtab) / sizeof(struct flag); --fidx >= 0; )
	{
	    len = flagtab[fidx].len;
	    if (STRNICMP(arg, flagtab[fidx].name, len) == 0
		    && (ends_excmd(arg[len]) || vim_iswhite(arg[len])))
	    {
		flags |= flagtab[fidx].val;
		arg = skipwhite(arg + len);

		if (flagtab[fidx].val == HL_SYNC_HERE
			|| flagtab[fidx].val == HL_SYNC_THERE)
		{
		    if (sync_idx == NULL)
		    {
			EMSG("group[t]here not accepted here");
			return NULL;
		    }

		    gname_start = arg;
		    arg = skiptowhite(arg);
		    if (gname_start == arg)
			return NULL;
		    gname = vim_strnsave(gname_start, (int)(arg - gname_start));
		    if (gname == NULL)
			return NULL;
		    if (STRCMP(gname, "NONE") == 0)
			*sync_idx = NONE_IDX;
		    else
		    {
			syn_id = syn_name2id(gname);
			for (i = curbuf->b_syn_patterns.ga_len; --i >= 0; )
			    if (SYN_ITEMS(curbuf)[i].sp_syn_id == syn_id
				&& SYN_ITEMS(curbuf)[i].sp_type == SPTYPE_START)
			    {
				*sync_idx = i;
				break;
			    }
			if (i < 0)
			{
			    EMSG2("Didn't find region item for %s", gname);
			    vim_free(gname);
			    return NULL;
			}
		    }

		    vim_free(gname);
		    arg = skipwhite(arg);
		}
		break;
	    }
	}
	if (fidx >= 0)
	    continue;

	if (STRNICMP(arg, "contains", 8) == 0
		&& (vim_iswhite(arg[8]) || arg[8] == '='))
	{
	    if (cont_list == NULL)
	    {
		EMSG("contains argument not accepted here");
		return NULL;
	    }
	    if (get_id_list(&arg, 8, cont_list) == FAIL)
		return NULL;
	}
	else if (STRNICMP(arg, "nextgroup", 9) == 0
		&& (vim_iswhite(arg[9]) || arg[9] == '='))
	{
	    if (get_id_list(&arg, 9, next_list) == FAIL)
		return NULL;
	}
	else
	    break;
    }

    *flagsp = flags;

    return arg;
}

/*
 * Adjustments to syntax item when declared in a ":syn include"'d file.
 * Set the contained flag, and if the item is not already contained, add it
 * to the specified top-level group, if any.
 */
    static void
syn_incl_toplevel(id, flagsp)
    int		id;
    int		*flagsp;
{
    if ((*flagsp & HL_CONTAINED) || curbuf->b_syn_topgrp == 0)
	return;
    *flagsp |= HL_CONTAINED;
    if (curbuf->b_syn_topgrp >= CLUSTER_ID_MIN)
    {
	/* We have to alloc this, because syn_combine_list() will free it. */
	short	    *grp_list = (short *)alloc((unsigned)(2 * sizeof(short)));
	int	    tlg_id = curbuf->b_syn_topgrp - CLUSTER_ID_MIN;

	if (grp_list != NULL)
	{
	    grp_list[0] = id;
	    grp_list[1] = 0;
	    syn_combine_list(&SYN_CLSTR(curbuf)[tlg_id].scl_list, &grp_list,
			 CLUSTER_ADD);
	}
    }
}

/*
 * Handle ":syntax include [@{group-name}] filename" command.
 */
/* ARGSUSED */
    static void
syn_cmd_include(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* not used */
{
    char_u	*arg = eap->arg;
    int		sgl_id = 1;
    char_u	*group_name_end;
    char_u	*rest;
    char_u	*errormsg = NULL;
    int		prev_toplvl_grp;
    int		prev_syn_inc_tag;

    eap->nextcmd = find_nextcmd(arg);
    if (eap->skip)
	return;

    if (arg[0] == '@')
    {
	++arg;
	rest = get_group_name(arg, &group_name_end);
	if (rest == NULL)
	{
	    EMSG((char_u *)"Filename required");
	    return;
	}
	sgl_id = syn_check_cluster(arg, (int)(group_name_end - arg));
	/* separate_nextcmd() and expand_filename() depend on this */
	eap->arg = rest;
    }

    /*
     * Everything that's left, up to the next command, should be the
     * filename to include.
     */
    eap->argt |= (XFILE | NOSPC);
    separate_nextcmd(eap);
    if (expand_filename(eap, syn_cmdlinep, &errormsg) == FAIL)
    {
	if (errormsg != NULL)
	    EMSG(errormsg);
	return;
    }

    /*
     * Save and restore the existing top-level grouplist id and ":syn
     * include" tag around the actual inclusion.
     */
    prev_syn_inc_tag = current_syn_inc_tag;
    current_syn_inc_tag = ++running_syn_inc_tag;
    prev_toplvl_grp = curbuf->b_syn_topgrp;
    curbuf->b_syn_topgrp = sgl_id;
    if (do_source(eap->arg, FALSE, FALSE) == FAIL)
	emsg2(e_notopen, eap->arg);
    curbuf->b_syn_topgrp = prev_toplvl_grp;
    current_syn_inc_tag = prev_syn_inc_tag;
}

/*
 * Handle ":syntax keyword {group-name} [{option}] keyword .." command.
 */
/* ARGSUSED */
    static void
syn_cmd_keyword(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* not used */
{
    char_u	*arg = eap->arg;
    char_u	*group_name_end;
    int		syn_id;
    char_u	*rest;
    char_u	*keyword_copy;
    char_u	*p;
    char_u	*first_arg;
    int		round;
    int		flags = 0;
    short	*next_list = NULL;

    rest = get_group_name(arg, &group_name_end);

    if (rest != NULL)
    {
	syn_id = syn_check_group(arg, (int)(group_name_end - arg));

	/* allocate a buffer, for removing the backslashes in the keyword */
	keyword_copy = alloc((unsigned)STRLEN(rest) + 1);
	if (keyword_copy != NULL)
	{
	    /*
	     * The options given apply to ALL keywords, so all options must be
	     * found before keywords can be created.
	     * round 1: collect the options.
	     * round 2: create the keywords.
	     */
	    first_arg = rest;
	    for (round = 1; round <= 2; ++round)
	    {
		/*
		 * Isolate each keyword and add an entry for it.
		 */
		for (rest = first_arg; rest != NULL && !ends_excmd(*rest);
						       rest = skipwhite(rest))
		{
		    rest = get_syn_options(rest, &flags, NULL,
							    NULL, &next_list);
		    if (rest == NULL || ends_excmd(*rest))
			break;
		    p = keyword_copy;
		    while (*rest && !vim_iswhite(*rest))
		    {
			if (*rest == '\\' && rest[1] != NUL)
			    ++rest;
			*p++ = *rest++;
		    }
		    *p = NUL;
		    if (round == 2 && !eap->skip)
		    {
			for (p = vim_strchr(keyword_copy, '['); ; ++p)
			{
			    if (p != NULL)
				*p = NUL;
			    add_keyword(keyword_copy, syn_id, flags, next_list);
			    if (p == NULL || p[1] == NUL || p[1] == ']')
				break;
			    p[0] = p[1];
			}
		    }
		}
		if (round == 1)
		    syn_incl_toplevel(syn_id, &flags);
	    }
	    vim_free(keyword_copy);
	}
    }

    if (rest != NULL)
	eap->nextcmd = check_nextcmd(rest);
    else
	EMSG2(e_invarg2, arg);

    vim_free(next_list);
    redraw_curbuf_later(NOT_VALID);
}

/*
 * Handle ":syntax match {name} [{options}] {pattern} [{options}]".
 *
 * Also ":syntax sync match {name} [[grouphere | groupthere] {group-name}] .."
 */
    static void
syn_cmd_match(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* TRUE for ":syntax sync match .. " */
{
    char_u		*arg = eap->arg;
    char_u		*group_name_end;
    char_u		*rest;
    struct syn_pattern	item;		/* the item found in the line */
    int			syn_id;
    int			idx;
    int			flags = 0;
    int			sync_idx = 0;
    short		*cont_list = NULL;
    short		*next_list = NULL;

    /* Isolate the group name, check for validity */
    rest = get_group_name(arg, &group_name_end);

    /* Get options before the pattern */
    rest = get_syn_options(rest, &flags, syncing ? &sync_idx : NULL,
						      &cont_list, &next_list);

    /* get the pattern. */
    init_syn_patterns();
    vim_memset(&item, 0, sizeof(item));
    rest = get_syn_pattern(rest, &item);
    if (vim_regcomp_had_eol() && !(flags & HL_EXCLUDENL))
	flags |= HL_HAS_EOL;

    /* Get options after the pattern */
    rest = get_syn_options(rest, &flags, syncing ? &sync_idx : NULL,
						      &cont_list, &next_list);

    if (rest != NULL)		/* all arguments are valid */
    {
	/*
	 * Check for trailing command and illegal trailing arguments.
	 */
	eap->nextcmd = check_nextcmd(rest);
	if (!ends_excmd(*rest) || eap->skip)
	    rest = NULL;
	else if (ga_grow(&curbuf->b_syn_patterns, 1) != FAIL
		&& (syn_id = syn_check_group(arg,
					   (int)(group_name_end - arg))) != 0)
	{
	    syn_incl_toplevel(syn_id, &flags);
	    /*
	     * Store the pattern in the syn_items list
	     */
	    idx = curbuf->b_syn_patterns.ga_len;
	    SYN_ITEMS(curbuf)[idx] = item;
	    SYN_ITEMS(curbuf)[idx].sp_syncing = syncing;
	    SYN_ITEMS(curbuf)[idx].sp_type = SPTYPE_MATCH;
	    SYN_ITEMS(curbuf)[idx].sp_syn_id = syn_id;
	    SYN_ITEMS(curbuf)[idx].sp_syn_inc_tag = current_syn_inc_tag;
	    SYN_ITEMS(curbuf)[idx].sp_flags = flags;
	    SYN_ITEMS(curbuf)[idx].sp_sync_idx = sync_idx;
	    SYN_ITEMS(curbuf)[idx].sp_cont_list = cont_list;
	    SYN_ITEMS(curbuf)[idx].sp_next_list = next_list;
	    ++curbuf->b_syn_patterns.ga_len;
	    --curbuf->b_syn_patterns.ga_room;

	    /* remember that we found a match for syncing on */
	    if (flags & (HL_SYNC_HERE|HL_SYNC_THERE))
		curbuf->b_syn_sync_flags |= SF_MATCH;

	    redraw_curbuf_later(NOT_VALID);
	    return;	/* don't free the progs and patterns now */
	}
    }

    /*
     * Something failed, free the allocated memory.
     */
    vim_free(item.sp_prog);
    vim_free(item.sp_pattern);
    vim_free(cont_list);
    vim_free(next_list);

    if (rest == NULL)
	EMSG2(e_invarg2, arg);
}

/*
 * Handle ":syntax region {group-name} [matchgroup={group-name}]
 *		start {start} .. [skip {skip}] end {end} .. [{options}]".
 */
    static void
syn_cmd_region(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* TRUE for ":syntax sync region .." */
{
    char_u		*arg = eap->arg;
    char_u		*group_name_end;
    char_u		*rest;			/* next arg, NULL on error */
    char_u		*key_end;
    char_u		*key = NULL;
    char_u		*p;
    int			item;
#define ITEM_START	    0
#define ITEM_SKIP	    1
#define ITEM_END	    2
#define ITEM_MATCHGROUP	    3
    struct pat_ptr
    {
	struct syn_pattern  *pp_synp;		/* pointer to syn_pattern */
	int		    pp_matchgroup_id;	/* matchgroup ID */
	struct pat_ptr	    *pp_next;		/* pointer to next pat_ptr */
    }			*(pat_ptrs[3]);
					    /* patterns found in the line */
    struct pat_ptr	*ppp;
    struct pat_ptr	*ppp_next;
    int			pat_count = 0;	    /* number of syn_patterns found */
    int			syn_id;
    int			matchgroup_id = 0;
    int			not_enough = FALSE;	/* not enough arguments */
    int			illegal = FALSE;	/* illegal arguments */
    int			success = FALSE;
    int			idx;
    int			flags = 0;
    short		*cont_list = NULL;
    short		*next_list = NULL;

    /* Isolate the group name, check for validity */
    rest = get_group_name(arg, &group_name_end);

    pat_ptrs[0] = NULL;
    pat_ptrs[1] = NULL;
    pat_ptrs[2] = NULL;

    init_syn_patterns();

    /*
     * get the options, patterns and matchgroup.
     */
    while (rest != NULL && !ends_excmd(*rest))
    {
	/* Check for option arguments */
	rest = get_syn_options(rest, &flags, NULL, &cont_list, &next_list);
	if (rest == NULL || ends_excmd(*rest))
	    break;

	/* must be a pattern or matchgroup then */
	key_end = rest;
	while (*key_end && !vim_iswhite(*key_end) && *key_end != '=')
	    ++key_end;
	vim_free(key);
	key = vim_strnsave_up(rest, (int)(key_end - rest));
	if (key == NULL)			/* out of memory */
	{
	    rest = NULL;
	    break;
	}
	if (STRCMP(key, "MATCHGROUP") == 0)
	    item = ITEM_MATCHGROUP;
	else if (STRCMP(key, "START") == 0)
	    item = ITEM_START;
	else if (STRCMP(key, "END") == 0)
	    item = ITEM_END;
	else if (STRCMP(key, "SKIP") == 0)
	{
	    if (pat_ptrs[ITEM_SKIP] != NULL)	/* one skip pattern allowed */
	    {
		illegal = TRUE;
		break;
	    }
	    item = ITEM_SKIP;
	}
	else
	    break;
	rest = skipwhite(key_end);
	if (*rest != '=')
	{
	    rest = NULL;
	    EMSG2("Missing '=': %s", arg);
	    break;
	}
	rest = skipwhite(rest + 1);
	if (*rest == NUL)
	{
	    not_enough = TRUE;
	    break;
	}

	if (item == ITEM_MATCHGROUP)
	{
	    p = skiptowhite(rest);
	    if ((p - rest == 4 && STRNCMP(rest, "NONE", 4) == 0) || eap->skip)
		matchgroup_id = 0;
	    else
	    {
		matchgroup_id = syn_check_group(rest, (int)(p - rest));
		if (matchgroup_id == 0)
		{
		    illegal = TRUE;
		    break;
		}
	    }
	    rest = skipwhite(p);
	}
	else
	{
	    /*
	     * Allocate room for a syn_pattern, and link it in the list of
	     * syn_patterns for this item, at the start (because the list is
	     * used from end to start).
	     */
	    ppp = (struct pat_ptr *)alloc((unsigned)sizeof(struct pat_ptr));
	    if (ppp == NULL)
	    {
		rest = NULL;
		break;
	    }
	    ppp->pp_next = pat_ptrs[item];
	    pat_ptrs[item] = ppp;
	    ppp->pp_synp = (struct syn_pattern *)alloc_clear(
					(unsigned)sizeof(struct syn_pattern));
	    if (ppp->pp_synp == NULL)
	    {
		rest = NULL;
		break;
	    }

	    /*
	     * Get the syntax pattern and the following offset(s).
	     */
	    rest = get_syn_pattern(rest, ppp->pp_synp);
	    if (item == ITEM_END && vim_regcomp_had_eol()
						   && !(flags & HL_EXCLUDENL))
		ppp->pp_synp->sp_flags |= HL_HAS_EOL;
	    ppp->pp_matchgroup_id = matchgroup_id;
	    ++pat_count;
	}
    }
    vim_free(key);
    if (illegal || not_enough)
	rest = NULL;

    /*
     * Must have a "start" and "end" pattern.
     */
    if (rest != NULL && (pat_ptrs[ITEM_START] == NULL ||
						  pat_ptrs[ITEM_END] == NULL))
    {
	not_enough = TRUE;
	rest = NULL;
    }

    if (rest != NULL)
    {
	/*
	 * Check for trailing garbage or command.
	 * If OK, add the item.
	 */
	eap->nextcmd = check_nextcmd(rest);
	if (!ends_excmd(*rest) || eap->skip)
	    rest = NULL;
	else if (ga_grow(&(curbuf->b_syn_patterns), pat_count) != FAIL
		&& (syn_id = syn_check_group(arg,
					   (int)(group_name_end - arg))) != 0)
	{
	    syn_incl_toplevel(syn_id, &flags);
	    /*
	     * Store the start/skip/end in the syn_items list
	     */
	    idx = curbuf->b_syn_patterns.ga_len;
	    for (item = ITEM_START; item <= ITEM_END; ++item)
	    {
		for (ppp = pat_ptrs[item]; ppp != NULL; ppp = ppp->pp_next)
		{
		    SYN_ITEMS(curbuf)[idx] = *(ppp->pp_synp);
		    SYN_ITEMS(curbuf)[idx].sp_syncing = syncing;
		    SYN_ITEMS(curbuf)[idx].sp_type =
			    (item == ITEM_START) ? SPTYPE_START :
			    (item == ITEM_SKIP) ? SPTYPE_SKIP : SPTYPE_END;
		    SYN_ITEMS(curbuf)[idx].sp_flags |= flags;
		    SYN_ITEMS(curbuf)[idx].sp_syn_id = syn_id;
		    SYN_ITEMS(curbuf)[idx].sp_syn_inc_tag = current_syn_inc_tag;
		    SYN_ITEMS(curbuf)[idx].sp_syn_match_id =
							ppp->pp_matchgroup_id;
		    if (item == ITEM_START)
		    {
			SYN_ITEMS(curbuf)[idx].sp_cont_list = cont_list;
			SYN_ITEMS(curbuf)[idx].sp_next_list = next_list;
		    }
		    ++curbuf->b_syn_patterns.ga_len;
		    --curbuf->b_syn_patterns.ga_room;
		    ++idx;
		}
	    }

	    redraw_curbuf_later(NOT_VALID);
	    success = TRUE;	    /* don't free the progs and patterns now */
	}
    }

    /*
     * Free the allocated memory.
     */
    for (item = ITEM_START; item <= ITEM_END; ++item)
	for (ppp = pat_ptrs[item]; ppp != NULL; ppp = ppp_next)
	{
	    if (!success)
	    {
		vim_free(ppp->pp_synp->sp_prog);
		vim_free(ppp->pp_synp->sp_pattern);
	    }
	    vim_free(ppp->pp_synp);
	    ppp_next = ppp->pp_next;
	    vim_free(ppp);
	}

    if (!success)
    {
	vim_free(cont_list);
	vim_free(next_list);
	if (not_enough)
	    EMSG2("Not enough arguments: syntax region %s", arg);
	else if (illegal || rest == NULL)
	    EMSG2(e_invarg2, arg);
    }
}

/*
 * A simple syntax group ID comparison function suitable for use in qsort()
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
syn_compare_stub(v1, v2)
    const void	*v1;
    const void	*v2;
{
    const short	*s1 = v1;
    const short	*s2 = v2;

    return (*s1 > *s2 ? 1 : *s1 < *s2 ? -1 : 0);
}

/*
 * Combines lists of syntax clusters.
 * *clstr1 and *clstr2 must both be allocated memory; they will be consumed.
 */
    static void
syn_combine_list(clstr1, clstr2, list_op)
    short	**clstr1;
    short	**clstr2;
    int		list_op;
{
    int		count1 = 0;
    int		count2 = 0;
    short	*g1;
    short	*g2;
    short	*clstr = NULL;
    int		count;
    int		round;

    /*
     * Handle degenerate cases.
     */
    if (*clstr2 == NULL)
	return;
    if (*clstr1 == NULL || list_op == CLUSTER_REPLACE)
    {
	if (list_op == CLUSTER_REPLACE)
	    vim_free(*clstr1);
	if (list_op == CLUSTER_REPLACE || list_op == CLUSTER_ADD)
	    *clstr1 = *clstr2;
	else
	    vim_free(*clstr2);
	return;
    }

    for (g1 = *clstr1; *g1; g1++)
	++count1;
    for (g2 = *clstr2; *g2; g2++)
	++count2;

    /*
     * For speed purposes, sort both lists.
     */
    qsort(*clstr1, (size_t)count1, sizeof(short), syn_compare_stub);
    qsort(*clstr2, (size_t)count2, sizeof(short), syn_compare_stub);

    /*
     * We proceed in two passes; in round 1, we count the elements to place
     * in the new list, and in round 2, we allocate and populate the new
     * list.  For speed, we use a mergesort-like method, adding the smaller
     * of the current elements in each list to the new list.
     */
    for (round = 1; round <= 2; round++)
    {
	g1 = *clstr1;
	g2 = *clstr2;
	count = 0;

	/*
	 * First, loop through the lists until one of them is empty.
	 */
	while (*g1 && *g2)
	{
	    /*
	     * We always want to add from the first list.
	     */
	    if (*g1 < *g2)
	    {
		if (round == 2)
		    clstr[count] = *g1;
		count++;
		g1++;
		continue;
	    }
	    /*
	     * We only want to add from the second list if we're adding the
	     * lists.
	     */
	    if (list_op == CLUSTER_ADD)
	    {
		if (round == 2)
		    clstr[count] = *g2;
		count++;
	    }
	    if (*g1 == *g2)
		g1++;
	    g2++;
	}

	/*
	 * Now add the leftovers from whichever list didn't get finished
	 * first.  As before, we only want to add from the second list if
	 * we're adding the lists.
	 */
	for (; *g1; g1++, count++)
	    if (round == 2)
		clstr[count] = *g1;
	if (list_op == CLUSTER_ADD)
	    for (; *g2; g2++, count++)
		if (round == 2)
		    clstr[count] = *g2;

	if (round == 1)
	{
	    /*
	     * If the group ended up empty, we don't need to allocate any
	     * space for it.
	     */
	    if (count == 0)
	    {
		clstr = NULL;
		break;
	    }
	    clstr = (short *)alloc((unsigned)((count + 1) * sizeof(short)));
	    if (clstr == NULL)
		break;
	    clstr[count] = 0;
	}
    }

    /*
     * Finally, put the new list in place.
     */
    vim_free(*clstr1);
    vim_free(*clstr2);
    *clstr1 = clstr;
}

/*
 * Lookup a syntax cluster name and return it's ID.
 * If it is not found, 0 is returned.
 */
    static int
syn_scl_name2id(name)
    char_u	*name;
{
    int		i;
    char_u	*name_u;

    /* Avoid using stricmp() too much, it's slow on some systems */
    name_u = vim_strsave_up(name);
    if (name_u == NULL)
	return 0;
    for (i = curbuf->b_syn_clusters.ga_len; --i >= 0; )
	if (SYN_CLSTR(curbuf)[i].scl_name_u != NULL
		&& STRCMP(name_u, SYN_CLSTR(curbuf)[i].scl_name_u) == 0)
	    break;
    vim_free(name_u);
    return (i < 0 ? 0 : i + CLUSTER_ID_MIN);
}

/*
 * Like syn_scl_name2id(), but take a pointer + length argument.
 */
    static int
syn_scl_namen2id(linep, len)
    char_u  *linep;
    int	    len;
{
    char_u  *name;
    int	    id = 0;

    name = vim_strnsave(linep, len);
    if (name != NULL)
    {
	id = syn_scl_name2id(name);
	vim_free(name);
    }
    return id;
}

/*
 * Find syntax cluster name in the table and return it's ID.
 * The argument is a pointer to the name and the length of the name.
 * If it doesn't exist yet, a new entry is created.
 * Return 0 for failure.
 */
    static int
syn_check_cluster(pp, len)
    char_u	*pp;
    int		len;
{
    int		id;
    char_u	*name;

    name = vim_strnsave(pp, len);
    if (name == NULL)
	return 0;

    id = syn_scl_name2id(name);
    if (id == 0)			/* doesn't exist yet */
	id = syn_add_cluster(name);
    else
	vim_free(name);
    return id;
}

/*
 * Add new syntax cluster and return it's ID.
 * "name" must be an allocated string, it will be consumed.
 * Return 0 for failure.
 */
    static int
syn_add_cluster(name)
    char_u		*name;
{
    int len;

    /*
     * First call for this growarray: init growing array.
     */
    if (curbuf->b_syn_clusters.ga_data == NULL)
    {
	curbuf->b_syn_clusters.ga_itemsize = sizeof(struct syn_cluster);
	curbuf->b_syn_clusters.ga_growsize = 10;
    }

    /*
     * Make room for at least one other cluster entry.
     */
    if (ga_grow(&curbuf->b_syn_clusters, 1) == FAIL)
    {
	vim_free(name);
	return 0;
    }
    len = curbuf->b_syn_clusters.ga_len;

    vim_memset(&(SYN_CLSTR(curbuf)[len]), 0, sizeof(struct syn_cluster));
    SYN_CLSTR(curbuf)[len].scl_name = name;
    SYN_CLSTR(curbuf)[len].scl_name_u = vim_strsave_up(name);
    SYN_CLSTR(curbuf)[len].scl_list = NULL;
    ++curbuf->b_syn_clusters.ga_len;
    --curbuf->b_syn_clusters.ga_room;

    return len + CLUSTER_ID_MIN;
}

/*
 * Handle ":syntax cluster {cluster-name} [contains={groupname},..]
 *		[add={groupname},..] [remove={groupname},..]".
 */
/* ARGSUSED */
    static void
syn_cmd_cluster(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* not used */
{
    char_u	*arg = eap->arg;
    char_u	*group_name_end;
    char_u	*rest;
    int		scl_id;
    short	*clstr_list;
    int		got_clstr = FALSE;
    int		opt_len;
    int		list_op;

    eap->nextcmd = find_nextcmd(arg);
    if (eap->skip)
	return;

    rest = get_group_name(arg, &group_name_end);

    if (rest != NULL)
    {
	scl_id = syn_check_cluster(arg, (int)(group_name_end - arg))
		    - CLUSTER_ID_MIN;

	for (;;)
	{
	    if (STRNICMP(rest, "add", 3) == 0
		    && (vim_iswhite(rest[3]) || rest[3] == '='))
	    {
		opt_len = 3;
		list_op = CLUSTER_ADD;
	    }
	    else if (STRNICMP(rest, "remove", 6) == 0
		    && (vim_iswhite(rest[6]) || rest[6] == '='))
	    {
		opt_len = 6;
		list_op = CLUSTER_SUBTRACT;
	    }
	    else if (STRNICMP(rest, "contains", 8) == 0
			&& (vim_iswhite(rest[8]) || rest[8] == '='))
	    {
		opt_len = 8;
		list_op = CLUSTER_REPLACE;
	    }
	    else
		break;

	    clstr_list = NULL;
	    if (get_id_list(&rest, opt_len, &clstr_list) == FAIL)
	    {
		EMSG2(e_invarg2, rest);
		break;
	    }
	    syn_combine_list(&SYN_CLSTR(curbuf)[scl_id].scl_list,
			     &clstr_list, list_op);
	    got_clstr = TRUE;
	}

	if (got_clstr)
	    redraw_curbuf_later(NOT_VALID);
    }

    if (!got_clstr)
	EMSG("No cluster specified");
    if (rest == NULL || !ends_excmd(*rest))
	EMSG2(e_invarg2, arg);
}

/*
 * On first call for current buffer: Init growing array.
 */
    static void
init_syn_patterns()
{
    curbuf->b_syn_patterns.ga_itemsize = sizeof(struct syn_pattern);
    curbuf->b_syn_patterns.ga_growsize = 10;
}

/*
 * Get one pattern for a ":syntax match" or ":syntax region" command.
 * Stores the pattern and program in a struct syn_pattern.
 * Returns a pointer to the next argument, or NULL in case of an error.
 */
    static char_u *
get_syn_pattern(arg, ci)
    char_u		*arg;
    struct syn_pattern	*ci;
{
    char_u	*end;
    int		*p;
    int		idx;
    char_u	*cpo_save;

    /* need at least three chars */
    if (arg == NULL || arg[1] == NUL || arg[2] == NUL)
	return NULL;

    end = skip_regexp(arg + 1, *arg, TRUE);
    if (*end != *arg)			    /* end delimiter not found */
    {
	EMSG2("Pattern delimiter not found: %s", arg);
	return NULL;
    }
    /* store the pattern and compiled regexp program */
    if ((ci->sp_pattern = vim_strnsave(arg + 1, (int)(end - arg - 1))) == NULL)
	return NULL;

    /* Make 'cpoptions' empty, to avoid the 'l' flag */
    cpo_save = p_cpo;
    p_cpo = (char_u *)"";
    ci->sp_prog = vim_regcomp(ci->sp_pattern, TRUE);
    p_cpo = cpo_save;

    if (ci->sp_prog == NULL)
	return NULL;
    ci->sp_ic = curbuf->b_syn_ic;

    /*
     * Check for a match, highlight or region offset.
     */
    ++end;
    do
    {
	for (idx = SPO_COUNT; --idx >= 0; )
	    if (STRNCMP(end, spo_name_tab[idx], 3) == 0)
		break;
	if (idx >= 0)
	{
	    p = &(ci->sp_offsets[idx]);
	    if (idx != SPO_LC_OFF)
		switch (end[3])
		{
		    case 's':   break;
		    case 'b':   break;
		    case 'e':   idx += SPO_COUNT; break;
		    default:    idx = -1; break;
		}
	    if (idx >= 0)
	    {
		ci->sp_off_flags |= (1 << idx);
		if (idx == SPO_LC_OFF)	    /* lc=99 */
		{
		    end += 3;
		    *p = getdigits(&end);

		    /* "lc=" offset automatically sets "ms=" offset */
		    if (!(ci->sp_off_flags & (1 << SPO_MS_OFF)))
		    {
			ci->sp_off_flags |= (1 << SPO_MS_OFF);
			ci->sp_offsets[SPO_MS_OFF] = *p;
		    }
		}
		else			    /* yy=x+99 */
		{
		    end += 4;
		    if (*end == '+')
		    {
			++end;
			*p = getdigits(&end);		/* positive offset */
		    }
		    else if (*end == '-')
		    {
			++end;
			*p = -getdigits(&end);		/* negative offset */
		    }
		}
		if (*end != ',')
		    break;
		++end;
	    }
	}
    } while (idx >= 0);

    if (!ends_excmd(*end) && !vim_iswhite(*end))
    {
	EMSG2("Garbage after pattern: %s", arg);
	return NULL;
    }
    return skipwhite(end);
}

/*
 * Handle ":syntax sync .." command.
 */
/* ARGSUSED */
    static void
syn_cmd_sync(eap, syncing)
    EXARG	*eap;
    int		syncing;	    /* not used */
{
    char_u	*arg_start = eap->arg;
    char_u	*arg_end;
    char_u	*key = NULL;
    char_u	*next_arg;
    int		illegal = FALSE;
    int		finished = FALSE;
    long	n;
    char_u	*cpo_save;

    if (ends_excmd(*arg_start))
    {
	syn_cmd_list(eap, TRUE);
	return;
    }

    while (!ends_excmd(*arg_start))
    {
	arg_end = skiptowhite(arg_start);
	next_arg = skipwhite(arg_end);
	vim_free(key);
	key = vim_strnsave_up(arg_start, (int)(arg_end - arg_start));
	if (STRCMP(key, "CCOMMENT") == 0)
	{
	    if (!eap->skip)
		curbuf->b_syn_sync_flags |= SF_CCOMMENT;
	    if (!ends_excmd(*next_arg))
	    {
		arg_end = skiptowhite(next_arg);
		if (!eap->skip)
		    curbuf->b_syn_sync_id = syn_check_group(next_arg,
						   (int)(arg_end - next_arg));
		next_arg = skipwhite(arg_end);
	    }
	    else if (!eap->skip)
		curbuf->b_syn_sync_id = syn_name2id((char_u *)"Comment");
	}
	else if (  STRNCMP(key, "LINES", 5) == 0
		|| STRNCMP(key, "MINLINES", 8) == 0
		|| STRNCMP(key, "MAXLINES", 8) == 0)
	{
	    if (key[0] == 'L')
		arg_end = key + 6;
	    else
		arg_end = key + 9;
	    if (arg_end[-1] != '=' || !isdigit(*arg_end))
	    {
		illegal = TRUE;
		break;
	    }
	    n = getdigits(&arg_end);
	    if (!eap->skip)
	    {
		if (key[1] == 'A')
		    curbuf->b_syn_sync_maxlines = n;
		else
		    curbuf->b_syn_sync_minlines = n;
	    }
	}
	else if (STRCMP(key, "LINECONT") == 0)
	{
	    if (curbuf->b_syn_linecont_pat != NULL)
	    {
		EMSG("syntax sync: line continuations pattern specified twice");
		finished = TRUE;
		break;
	    }
	    arg_end = skip_regexp(next_arg + 1, *next_arg, TRUE);
	    if (*arg_end != *next_arg)	    /* end delimiter not found */
	    {
		illegal = TRUE;
		break;
	    }

	    if (!eap->skip)
	    {
		/* store the pattern and compiled regexp program */
		if ((curbuf->b_syn_linecont_pat = vim_strnsave(next_arg + 1,
				      (int)(arg_end - next_arg - 1))) == NULL)
		{
		    finished = TRUE;
		    break;
		}
		curbuf->b_syn_linecont_ic = curbuf->b_syn_ic;

		/* Make 'cpoptions' empty, to avoid the 'l' flag */
		cpo_save = p_cpo;
		p_cpo = (char_u *)"";
		curbuf->b_syn_linecont_prog =
				vim_regcomp(curbuf->b_syn_linecont_pat, TRUE);
		p_cpo = cpo_save;

		if (curbuf->b_syn_linecont_prog == NULL)
		{
		    vim_free(curbuf->b_syn_linecont_pat);
		    curbuf->b_syn_linecont_pat = NULL;
		    finished = TRUE;
		    break;
		}
	    }
	    next_arg = skipwhite(arg_end + 1);
	}
	else
	{
	    eap->arg = next_arg;
	    if (STRCMP(key, "MATCH") == 0)
		syn_cmd_match(eap, TRUE);
	    else if (STRCMP(key, "REGION") == 0)
		syn_cmd_region(eap, TRUE);
	    else if (STRCMP(key, "CLEAR") == 0)
		syn_cmd_clear(eap, TRUE);
	    else
		illegal = TRUE;
	    finished = TRUE;
	    break;
	}
	arg_start = next_arg;
    }
    vim_free(key);
    if (illegal)
	EMSG2("Illegal arguments: %s", arg_start);
    else if (!finished)
    {
	eap->nextcmd = check_nextcmd(arg_start);
	redraw_curbuf_later(NOT_VALID);
    }
}

/*
 * Convert a line of highlight group names into a list of group ID numbers.
 * "arg" should point to the "contains" or "nextgroup" keyword.
 * "arg" is advanced to after the last group name.
 * Careful: the argument is modified (NULs added).
 * returns FAIL for some error, OK for success.
 */
    static int
get_id_list(arg, keylen, list)
    char_u	**arg;
    int		keylen;		/* length of keyword */
    short	**list;		/* where to store the resulting list, if not
				   NULL, the list is silently skipped! */
{
    char_u	*p = NULL;
    char_u	*end;
    int		round;
    int		count;
    int		total_count = 0;
    short	*retval = NULL;
    char_u	*name;
    vim_regexp	*prog;
    int		id;
    int		i;
    int		failed = FALSE;

    /*
     * We parse the list twice:
     * round == 1: count the number of items, allocate the array.
     * round == 2: fill the array with the items.
     * In round 1 new groups may be added, causing the number of items to
     * grow when a regexp is used.  In that case round 1 is done once again.
     */
    for (round = 1; round <= 2; ++round)
    {
	/*
	 * skip "contains"
	 */
	p = skipwhite(*arg + keylen);
	if (*p != '=')
	{
	    EMSG2("Missing equal sign: %s", *arg);
	    break;
	}
	p = skipwhite(p + 1);
	if (ends_excmd(*p))
	{
	    EMSG2("Empty argument: %s", *arg);
	    break;
	}

	/*
	 * parse the arguments after "contains"
	 */
	count = 0;
	while (!ends_excmd(*p))
	{
	    for (end = p; *end && !vim_iswhite(*end) && *end != ','; ++end)
		;
	    name = alloc((int)(end - p + 3));	    /* leave room for "^$" */
	    if (name == NULL)
	    {
		failed = TRUE;
		break;
	    }
	    STRNCPY(name + 1, p, end - p);
	    name[end - p + 1] = NUL;
	    if (       STRCMP(name + 1, "ALLBUT") == 0
		    || STRCMP(name + 1, "ALL") == 0)
	    {
		if (TO_UPPER(**arg) != 'C')
		{
		    EMSG2("%s not allowed here", name + 1);
		    failed = TRUE;
		    vim_free(name);
		    break;
		}
		if (count != 0)
		{
		    EMSG2("%s must be first in contains list", name + 1);
		    failed = TRUE;
		    vim_free(name);
		    break;
		}
		id = CONTAINS_ALLBUT + current_syn_inc_tag;
	    }
	    else if (name[1] == '@')
	    {
		id = syn_check_cluster(name + 2, (int)(end - p - 1));
	    }
	    else
	    {
		/*
		 * Handle full group name.
		 */
		if (vim_strpbrk(name + 1, (char_u *)"\\.*^$~[") == NULL)
		    id = syn_check_group(name + 1, (int)(end - p));
		else
		{
		    /*
		     * Handle match of regexp with group names.
		     */
		    *name = '^';
		    STRCAT(name, "$");
		    prog = vim_regcomp(name, TRUE);
		    if (prog == NULL)
		    {
			failed = TRUE;
			vim_free(name);
			break;
		    }

		    reg_ic = TRUE;
		    id = 0;
		    for (i = highlight_ga.ga_len; --i >= 0; )
		    {
			if (vim_regexec(prog, HL_TABLE()[i].sg_name, TRUE))
			{
			    if (round == 2)
			    {
				/* Got more items than expected; can happen
				 * when adding items that match:
				 * "contains=a.*b,axb".
				 * Go back to first round */
				if (count >= total_count)
				{
				    vim_free(retval);
				    round = 1;
				}
				else
				    retval[count] = i + 1;
			    }
			    ++count;
			    id = -1;	    /* remember that we found one */
			}
		    }
		    vim_free(prog);
		}
	    }
	    vim_free(name);
	    if (id == 0)
	    {
		EMSG2("Unknown group name: %s", p);
		failed = TRUE;
		break;
	    }
	    if (id > 0)
	    {
		if (round == 2)
		{
		    /* Got more items than expected, go back to first round */
		    if (count >= total_count)
		    {
			vim_free(retval);
			round = 1;
		    }
		    else
			retval[count] = id;
		}
		++count;
	    }
	    p = skipwhite(end);
	    if (*p != ',')
		break;
	    p = skipwhite(p + 1);	/* skip comma in between arguments */
	}
	if (failed)
	    break;
	if (round == 1)
	{
	    retval = (short *)alloc((unsigned)((count + 1) * sizeof(short)));
	    if (retval == NULL)
		break;
	    retval[count] = 0;	    /* zero means end of the list */
	    total_count = count;
	}
    }

    *arg = p;
    if (failed || retval == NULL)
    {
	vim_free(retval);
	return FAIL;
    }

    if (*list == NULL)
	*list = retval;
    else
	vim_free(retval);	/* list already found, don't overwrite it */

    return OK;
}

/*
 * Make a copy of an ID list.
 */
    static short *
copy_id_list(list)
    short   *list;
{
    int	    len;
    int	    count;
    short   *retval;

    if (list == NULL)
	return NULL;

    for (count = 0; list[count]; ++count)
	;
    len = (count + 1) * sizeof(short);
    retval = (short *)alloc((unsigned)len);
    if (retval != NULL)
	mch_memmove(retval, list, (size_t)len);

    return retval;
}

/*
 * Check if "id" is in the "contains" or "nextgroup" list of pattern "idx".
 */
    static int
in_id_list(list, id, inctag, contained)
    short	*list;		/* id list */
    int		id;		/* group id */
    int		inctag;		/* ":syn include" tag of group id */
    int		contained;	/* group id is contained */
{
    int		retval;
    short	scl_id;
    short	*scl_list;

    /*
     * If list is ID_LIST_ALL, we are in a transparent item that isn't
     * inside anything.  Only allow not-contained groups.
     */
    if (list == ID_LIST_ALL)
	return !contained;

    /*
     * If the first item is "ALLBUT", return TRUE if id is NOT in the contains
     * list.  We also require that id is at the same ":syn include" level
     * as the list.
     */
    if (*list >= CONTAINS_ALLBUT && *list < CLUSTER_ID_MIN)
    {
	if (*list - CONTAINS_ALLBUT != inctag)
	    return FALSE;
	++list;
	retval = FALSE;
    }
    else
	retval = TRUE;

    /*
     * Return "retval" if id is in the contains list.
     */
    for (; *list; ++list)
    {
	if (*list == id)
	    return retval;
	scl_id = *list - CLUSTER_ID_MIN;
	if (scl_id >= 0)
	{
	    scl_list = SYN_CLSTR(syn_buf)[scl_id].scl_list;
	    if (scl_list != NULL && in_id_list(scl_list, id, inctag, contained))
		return retval;
	}
    }
    return !retval;
}

struct subcommand
{
    char    *name;				/* subcommand name */
    void    (*func)__ARGS((EXARG *, int));	/* function to call */
};

static struct subcommand subcommands[] =
{
    {"case",		syn_cmd_case},
    {"clear",		syn_cmd_clear},
    {"cluster",		syn_cmd_cluster},
    {"include",		syn_cmd_include},
    {"keyword",		syn_cmd_keyword},
    {"list",		syn_cmd_list},
    {"manual",		syn_cmd_manual},
    {"match",		syn_cmd_match},
    {"on",		syn_cmd_on},
    {"off",		syn_cmd_off},
    {"region",		syn_cmd_region},
    {"sync",		syn_cmd_sync},
    {"",		syn_cmd_list},
    {NULL, NULL}
};

/*
 * Handle the ":syntax" command.
 * This searches the subcommands[] table for the subcommand name, and calls a
 * syntax_subcommand() function to do the rest.
 */
    void
do_syntax(eap, cmdlinep)
    EXARG	*eap;
    char_u	**cmdlinep;
{
    char_u	*arg = eap->arg;
    char_u	*subcmd_end;
    char_u	*subcmd_name;
    int		i;

    syn_cmdlinep = cmdlinep;

    /* isolate subcommand name */
    for (subcmd_end = arg; isalpha(*subcmd_end); ++subcmd_end)
	;
    subcmd_name = vim_strnsave(arg, (int)(subcmd_end - arg));
    if (subcmd_name != NULL)
    {
	if (eap->skip)		/* skip error messages for all subcommands */
	    ++emsg_off;
	for (i = 0; ; ++i)
	{
	    if (subcommands[i].name == NULL)
	    {
		EMSG2("Invalid :syntax subcommand: %s", subcmd_name);
		break;
	    }
	    if (STRCMP(subcmd_name, (char_u *)subcommands[i].name) == 0)
	    {
		eap->arg = skipwhite(subcmd_end);
		(subcommands[i].func)(eap, FALSE);
		break;
	    }
	}
	vim_free(subcmd_name);
	if (eap->skip)
	    --emsg_off;
    }
}

    int
syntax_present(buf)
    BUF	    *buf;
{
    return (buf->b_syn_patterns.ga_len != 0
	    || buf->b_syn_clusters.ga_len != 0
	    || curbuf->b_keywtab != NULL
	    || curbuf->b_keywtab_ic != NULL);
}

#ifdef CMDLINE_COMPL

static enum
{
    EXP_SUBCMD,	    /* expand ":syn" sub-commands */
    EXP_CASE	    /* expand ":syn case" arguments */
} expand_what;


/*
 * Handle command line completion for :syntax command.
 */
    void
set_context_in_syntax_cmd(arg)
    char_u *arg;
{
    char_u	*p;

    /* Default: expand subcommands */
    expand_context = EXPAND_SYNTAX;
    expand_what = EXP_SUBCMD;
    expand_pattern = arg;
    include_link = FALSE;

    /* (part of) subcommand already typed */
    if (*arg != NUL)
    {
	p = skiptowhite(arg);
	if (*p != NUL)		    /* past first word */
	{
	    expand_pattern = skipwhite(p);
	    if (*skiptowhite(expand_pattern) != NUL)
		expand_context = EXPAND_NOTHING;
	    else if (STRNICMP(arg, "case", p - arg) == 0)
		expand_what = EXP_CASE;
	    else if (  STRNICMP(arg, "keyword", p - arg) == 0
		    || STRNICMP(arg, "region", p - arg) == 0
		    || STRNICMP(arg, "match", p - arg) == 0
		    || STRNICMP(arg, "list", p - arg) == 0)
		expand_context = EXPAND_HIGHLIGHT;
	    else
		expand_context = EXPAND_NOTHING;
	}
    }
}

static char *(case_args[]) = {"match", "ignore", NULL};

/*
 * Function given to ExpandGeneric() to obtain the list syntax names for
 * expansion.
 */
    char_u *
get_syntax_name(idx)
    int	    idx;
{
    if (expand_what == EXP_SUBCMD)
	return (char_u *)subcommands[idx].name;
    return (char_u *)case_args[idx];
}

#endif /* CMDLINE_COMPL */

#ifdef WANT_EVAL
/*
 * Function called for expression evaluation: get syntax ID at file position.
 */
    int
syn_get_id(line, col, trans)
    long	line;
    long	col;
    int		trans;	    /* remove transparancy */
{
    if (curwin->w_buffer != syn_buf
			   || col < (long)current_col || line != current_lnum)
	syntax_start(curwin, line);

    (void)get_syntax_attr((colnr_t)col, ml_get((linenr_t)line));

    return (trans ? current_trans_id : current_id);
}
#endif

/*
 * Call vim_regexec() with the current buffer set to syn_buf.  Makes
 * vim_iswordc() work correctly.
 */
    static int
syn_regexec(prog, string, at_bol)
    vim_regexp	*prog;
    char_u	*string;
    int		at_bol;
{
    int		retval;
    BUF		*save_curbuf;

    save_curbuf = curbuf;
    curbuf = syn_buf;
    retval = vim_regexec(prog, string, at_bol);
    curbuf = save_curbuf;
    return retval;
}

#endif /* SYNTAX_HL */


/**************************************
 *  Highlighting stuff		      *
 **************************************/

/*
 * The default highlight groups.  Used in the 'highlight' and 'guicursor'
 * options default.  Depends on 'background' option.
 * These are compiled in, because they are needed even when no configuration
 * file is used (e.g., for error messages).
 */
static char *(highlight_init_both[]) =
    {
#ifdef USE_GUI
	"Cursor guibg=fg guifg=bg",
#endif
	"ErrorMsg term=standout ctermbg=DarkRed ctermfg=White guibg=Red guifg=White",
	"IncSearch term=reverse cterm=reverse gui=reverse",
	"ModeMsg term=bold cterm=bold gui=bold",
	"NonText term=bold ctermfg=Blue gui=bold guifg=Blue",
	"StatusLine term=reverse,bold cterm=reverse,bold gui=reverse,bold",
	"StatusLineNC term=reverse cterm=reverse gui=reverse",
	"Visual term=reverse cterm=reverse gui=reverse guifg=Grey guibg=fg",
	"VisualNOS term=underline,bold cterm=underline,bold gui=underline,bold",
	NULL
    };

static char *(highlight_init_light[]) =
    {
	"Directory term=bold ctermfg=DarkBlue guifg=Blue",
	"LineNr term=underline ctermfg=Brown guifg=Brown",
	"MoreMsg term=bold ctermfg=DarkGreen gui=bold guifg=SeaGreen",
	"Normal gui=NONE",
	"Question term=standout ctermfg=DarkGreen gui=bold guifg=SeaGreen",
	"Search term=reverse ctermbg=Yellow ctermfg=NONE guibg=Yellow guifg=NONE",
	"SpecialKey term=bold ctermfg=DarkBlue guifg=Blue",
	"Title term=bold ctermfg=DarkMagenta gui=bold guifg=Magenta",
	"WarningMsg term=standout ctermfg=DarkRed guifg=Red",
	"WildMenu term=standout ctermbg=Yellow ctermfg=Black guibg=Yellow guifg=Black",
	NULL
    };

static char *(highlight_init_dark[]) =
    {
	"Directory term=bold ctermfg=LightCyan guifg=Cyan",
	"LineNr term=underline ctermfg=Yellow guifg=Yellow",
	"MoreMsg term=bold ctermfg=LightGreen gui=bold guifg=SeaGreen",
	"Normal gui=NONE",
	"Question term=standout ctermfg=LightGreen gui=bold guifg=Green",
	"Search term=reverse ctermbg=Yellow ctermfg=Black guibg=Yellow guifg=Black",
	"SpecialKey term=bold ctermfg=LightBlue guifg=Cyan",
	"Title term=bold ctermfg=LightMagenta gui=bold guifg=Magenta",
	"WarningMsg term=standout ctermfg=LightRed guifg=Red",
	"WildMenu term=standout ctermbg=Yellow ctermfg=Black guibg=Yellow guifg=Black",
	NULL
    };

    void
init_highlight(both)
    int		both;	    /* include groups where 'bg' doesn't matter */
{
    int		i;
    char	**pp;
    static int	had_both = FALSE;

    if (both)
    {
	had_both = TRUE;
	pp = highlight_init_both;
	for (i = 0; pp[i] != NULL; ++i)
	    do_highlight((char_u *)pp[i], FALSE, TRUE);
    }
    else if (!had_both)
	/* Don't do anything before the call with both == TRUE from main().
	 * Not everything has been setup then, and that call will overrule
	 * everything anyway. */
	return;

    if (TO_LOWER(*p_bg) == 'l')
	pp = highlight_init_light;
    else
	pp = highlight_init_dark;
    for (i = 0; pp[i] != NULL; ++i)
	do_highlight((char_u *)pp[i], FALSE, TRUE);
}

/*
 * Handle the ":highlight .." command.
 */
    void
do_highlight(line, forceit, init)
    char_u	*line;
    int		forceit;
    int		init;	    /* TRUE when called for initializing */
{
    char_u	*name_end;
    char_u	*p;
    char_u	*linep;
    char_u	*key_start;
    char_u	*arg_start;
    char_u	*key = NULL, *arg = NULL;
    long	i;
    int		off;
    int		len;
    int		attr;
    int		id;
    int		idx;
    int		doclear = FALSE;
    int		dolink = FALSE;
    int		error = FALSE;
    int		color;
    int		is_normal_group = FALSE;	/* "Normal" group */
#ifdef USE_GUI_X11
    int		is_menu_group = FALSE;		/* "Menu" group */
    int		is_scrollbar_group = FALSE;	/* "Scrollbar" group */
#endif

    /*
     * If no argument, list current highlighting.
     */
    if (ends_excmd(*line))
    {
	for (i = 1; i <= highlight_ga.ga_len && !got_int; ++i)
	    /* TODO: only call when the group has attributes set */
	    highlight_list_one((int)i);
	return;
    }

    /*
     * Isolate the name.
     */
    name_end = skiptowhite(line);
    linep = skipwhite(name_end);

    if (STRNCMP(line, "clear", name_end - line) == 0)
	doclear = TRUE;
    if (STRNCMP(line, "link", name_end - line) == 0)
	dolink = TRUE;

    /*
     * ":highlight {group-name}": list highlighting for one group.
     */
    if (!doclear && !dolink && ends_excmd(*linep))
    {
	id = syn_namen2id(line, (int)(name_end - line));
	if (id == 0)
	    EMSG2("highlight group not found: %s", line);
	else
	    highlight_list_one(id);
	return;
    }

    /*
     * Handle ":highlight link {from} {to}" command.
     */
    if (dolink)
    {
	char_u	    *from_start = linep;
	char_u	    *from_end;
	char_u	    *to_start;
	char_u	    *to_end;
	int	    from_id;
	int	    to_id;

	from_end = skiptowhite(from_start);
	to_start = skipwhite(from_end);
	to_end	 = skiptowhite(to_start);

	if (ends_excmd(*from_start) || ends_excmd(*to_start))
	{
	    EMSG2("Not enough arguments: \":highlight link %s\"", from_start);
	    return;
	}

	if (!ends_excmd(*skipwhite(to_end)))
	{
	    EMSG2("Too many arguments: \":highlight link %s\"", from_start);
	    return;
	}

	from_id = syn_check_group(from_start, (int)(from_end - from_start));
	if (STRNCMP(to_start, "NONE", 4) == 0)
	    to_id = 0;
	else
	    to_id = syn_check_group(to_start, (int)(to_end - to_start));

	if (from_id > 0 && (!init || HL_TABLE()[from_id - 1].sg_set == 0))
	{
	    /*
	     * Don't allow a link when there already is some highlighting
	     * for the group, unless '!' is used
	     */
	    if (to_id > 0 && !forceit && !init
		    &&	  (HL_TABLE()[from_id - 1].sg_term_attr != 0
			|| HL_TABLE()[from_id - 1].sg_cterm_attr != 0
#ifdef USE_GUI
			|| HL_TABLE()[from_id - 1].sg_gui_attr != 0
#endif
		       ))
	    {
		if (sourcing_name == NULL)
		    EMSG("group has settings, highlight link ignored");
	    }
	    else
	    {
		if (!init)
		    HL_TABLE()[from_id - 1].sg_set |= SG_LINK;
		HL_TABLE()[from_id - 1].sg_link = to_id;
	    }
	}

	redraw_curbuf_later(NOT_VALID);
	return;
    }

    /*
     * Handle ":highlight clear {group}" command.
     */
    if (doclear)
    {
	line = linep;
	if (ends_excmd(*line))
	{
	    EMSG("Cannot clear all highlight groups");
	    return;
	}
	name_end = skiptowhite(line);
	linep = skipwhite(name_end);
    }

    /*
     * Find the group name in the table.  If it does not exist yet, add it.
     */
    id = syn_check_group(line, (int)(name_end - line));
    if (id == 0)			/* failed (out of memory) */
	return;
    idx = id - 1;			/* index is ID minus one */
    if (STRCMP(HL_TABLE()[idx].sg_name_u, "NORMAL") == 0)
	is_normal_group = TRUE;
#ifdef USE_GUI_X11
    else if (STRCMP(HL_TABLE()[idx].sg_name_u, "MENU") == 0)
	is_menu_group = TRUE;
    else if (STRCMP(HL_TABLE()[idx].sg_name_u, "SCROLLBAR") == 0)
	is_scrollbar_group = TRUE;
#endif

    if (doclear)
	highlight_clear(idx);
    else
      while (!ends_excmd(*linep))
      {
	key_start = linep;
	if (*linep == '=')
	{
	    EMSG2("unexpected equal sign: %s", key_start);
	    error = TRUE;
	    break;
	}

	/*
	 * Isolate the key ("term", "ctermfg", "ctermbg", "font", "guifg" or
	 * "guibg").
	 */
	while (*linep && !vim_iswhite(*linep) && *linep != '=')
	    ++linep;
	vim_free(key);
	key = vim_strnsave_up(key_start, (int)(linep - key_start));
	if (key == NULL)
	{
	    error = TRUE;
	    break;
	}
	linep = skipwhite(linep);

	if (STRCMP(key, "NONE") == 0)
	{
	    if (!init || HL_TABLE()[idx].sg_set == 0)
	    {
		if (!init)
		    HL_TABLE()[idx].sg_set |= SG_TERM+SG_CTERM+SG_GUI;
		highlight_clear(idx);
	    }
	    continue;
	}

	/*
	 * Check for the equal sign.
	 */
	if (*linep != '=')
	{
	    EMSG2("missing equal sign: %s", key_start);
	    error = TRUE;
	    break;
	}
	++linep;

	/*
	 * Isolate the argument.
	 */
	linep = skipwhite(linep);
	if (*linep == '\'')		/* guifg='color name' */
	{
	    arg_start = ++linep;
	    linep = vim_strchr(linep, '\'');
	}
	else
	{
	    arg_start = linep;
	    linep = skiptowhite(linep);
	}
	if (linep == arg_start)
	{
	    EMSG2("missing argument: %s", key_start);
	    error = TRUE;
	    break;
	}
	vim_free(arg);
	arg = vim_strnsave(arg_start, (int)(linep - arg_start));
	if (arg == NULL)
	{
	    error = TRUE;
	    break;
	}
	if (*linep == '\'')
	    ++linep;

	/*
	 * Store the argument.
	 */
	if (  STRCMP(key, "TERM") == 0
		|| STRCMP(key, "CTERM") == 0
		|| STRCMP(key, "GUI") == 0)
	{
	    attr = 0;
	    off = 0;
	    while (arg[off] != NUL)
	    {
		for (i = sizeof(hl_attr_table) / sizeof(int); --i >= 0; )
		{
		    len = STRLEN(hl_name_table[i]);
		    if (STRNICMP(arg + off, hl_name_table[i], len) == 0)
		    {
			attr |= hl_attr_table[i];
			off += len;
			break;
		    }
		}
		if (i < 0)
		{
		    EMSG2("Illegal value: %s", arg);
		    error = TRUE;
		    break;
		}
		if (arg[off] == ',')		/* another one follows */
		    ++off;
	    }
	    if (error)
		break;
	    if (*key == 'T')
	    {
		if (!init || !(HL_TABLE()[idx].sg_set & SG_TERM))
		{
		    if (!init)
			HL_TABLE()[idx].sg_set |= SG_TERM;
		    HL_TABLE()[idx].sg_term = attr;
		}
	    }
	    else if (*key == 'C')
	    {
		if (!init || !(HL_TABLE()[idx].sg_set & SG_CTERM))
		{
		    if (!init)
			HL_TABLE()[idx].sg_set |= SG_CTERM;
		    HL_TABLE()[idx].sg_cterm = attr;
		    HL_TABLE()[idx].sg_cterm_bold = FALSE;
		}
	    }
#ifdef USE_GUI
	    else
	    {
		if (!init || !(HL_TABLE()[idx].sg_set & SG_GUI))
		{
		    if (!init)
			HL_TABLE()[idx].sg_set |= SG_GUI;
		    HL_TABLE()[idx].sg_gui = attr;
		}
	    }
#endif
	}
	else if (STRCMP(key, "FONT") == 0)
	{
#ifdef USE_GUI	    /* in non-GUI fonts are simply ignored */
	    gui_mch_free_font(HL_TABLE()[idx].sg_font);
	    HL_TABLE()[idx].sg_font = font_name2handle(arg);
	    vim_free(HL_TABLE()[idx].sg_font_name);
	    HL_TABLE()[idx].sg_font_name = vim_strsave(arg);
	    if (is_normal_group)
		gui_init_font(arg);
#endif
	}
	else if (STRCMP(key, "CTERMFG") == 0 || STRCMP(key, "CTERMBG") == 0)
	{
	  if (!init || !(HL_TABLE()[idx].sg_set & SG_CTERM))
	  {
	    if (!init)
		HL_TABLE()[idx].sg_set |= SG_CTERM;

	    /* When setting the foreground color, and previously the "bold"
	     * flag was set for a light color, reset it now */
	    if (key[5] == 'F' && HL_TABLE()[idx].sg_cterm_bold)
	    {
		HL_TABLE()[idx].sg_cterm &= ~HL_BOLD;
		HL_TABLE()[idx].sg_cterm_bold = FALSE;
	    }

	    if (isdigit(*arg))
		color = atoi((char *)arg);
	    else if (STRICMP(arg, "fg") == 0)
	    {
		if (cterm_normal_fg_color)
		    color = cterm_normal_fg_color - 1;
		else
		{
		    EMSG("FG color unknown");
		    error = TRUE;
		    break;
		}
	    }
	    else if (STRICMP(arg, "bg") == 0)
	    {
		if (cterm_normal_bg_color)
		    color = cterm_normal_bg_color - 1;
		else
		{
		    EMSG("BG color unknown");
		    error = TRUE;
		    break;
		}
	    }
	    else
	    {
		static char *(color_names[26]) = {
			    "Black", "DarkBlue", "DarkGreen", "DarkCyan",
			    "DarkRed", "DarkMagenta", "Brown", "Gray", "Grey",
			    "LightGray", "LightGrey", "DarkGray", "DarkGrey",
			    "Blue", "LightBlue", "Green", "LightGreen",
			    "Cyan", "LightCyan", "Red", "LightRed", "Magenta",
			    "LightMagenta", "Yellow", "White", "NONE"};
		static int color_numbers_16[26] = {0, 1, 2, 3,
						 4, 5, 6, 7, 7,
						 7, 7, 8, 8,
						 9, 9, 10, 10,
						 11, 11, 12, 12, 13,
						 13, 14, 15, -1};
		/* for terminals with less than 16 colors... */
		static int color_numbers_8[26] = {0, 4, 2, 6,
						 1, 5, 3, 7, 7,
						 7, 7, 0+8, 0+8,
						 4+8, 4+8, 2+8, 2+8,
						 6+8, 6+8, 1+8, 1+8, 5+8,
						 5+8, 3+8, 7+8, -1};

		/* reduce calls to STRICMP a bit, it can be slow */
		off = TO_UPPER(*arg);
		for (i = (sizeof(color_names) / sizeof(char *)); --i >= 0; )
		    if (off == color_names[i][0]
				 && STRICMP(arg + 1, color_names[i] + 1) == 0)
			break;
		if (i < 0)
		{
		    EMSG2("Color name or number not recognized: %s", key_start);
		    error = TRUE;
		    break;
		}
		color = color_numbers_16[i];
		if (color >= 0)
		{
		    if (atoi((char *)T_CCO) == 8)
		    {
			color = color_numbers_8[i];
			if (key[5] == 'F')
			{
			    /* set/reset bold attribute to get light foreground
			     * colors (on some terminals, e.g. "linux") */
			    if (color & 8)
			    {
				HL_TABLE()[idx].sg_cterm |= HL_BOLD;
				HL_TABLE()[idx].sg_cterm_bold = TRUE;
			    }
			    else
				HL_TABLE()[idx].sg_cterm &= ~HL_BOLD;
			}
			color &= 7;
		    }
		    else if (atoi((char *)T_CCO) == 16)
		    {
			/*
			 * Guess: if the termcap entry ends in 'm', it is
			 * probably an xterm-like terminal.  Use the changed
			 * order for colors.
			 */
			if (*T_CAF != NUL)
			    p = T_CAF;
			else
			    p = T_CSF;
			if (*p != NUL && *(p + STRLEN(p) - 1) == 'm')
			    color = color_numbers_8[i];
		    }
		}
	    }
	    /* Add one to the argument, to avoid zero */
	    if (key[5] == 'F')
	    {
		HL_TABLE()[idx].sg_cterm_fg = color + 1;
		if (is_normal_group)
		{
		    cterm_normal_fg_color = color + 1;
		    cterm_normal_fg_bold = (HL_TABLE()[idx].sg_cterm & HL_BOLD);
		    must_redraw = CLEAR;
		    term_fg_color(color);
		}
	    }
	    else
	    {
		HL_TABLE()[idx].sg_cterm_bg = color + 1;
		if (is_normal_group)
		{
		    cterm_normal_bg_color = color + 1;
		    must_redraw = CLEAR;
		    term_bg_color(color);
		    if (atoi((char *)T_CCO) < 16)
			i = (color == 0 || color == 4);
		    else
			i = (color < 7 || color == 8);
		    set_option_value((char_u *)"bg", 0L,
				    i ? (char_u *)"dark" : (char_u *)"light");
		}
	    }
	  }
	}
	else if (STRCMP(key, "GUIFG") == 0)
	{
#ifdef USE_GUI	    /* in non-GUI guifg colors are simply ignored */
	  if (!init || !(HL_TABLE()[idx].sg_set & SG_GUI))
	  {
	    if (!init)
		HL_TABLE()[idx].sg_set |= SG_GUI;

	    /* Add one to the argument, to avoid zero */
	    i = color_name2handle(arg) + 1;
	    if (i > 0 || STRCMP(arg, "NONE") == 0 || !gui.in_use)
	    {
		HL_TABLE()[idx].sg_gui_fg = i;
		vim_free(HL_TABLE()[idx].sg_gui_fg_name);
		if (STRCMP(arg, "NONE"))
		    HL_TABLE()[idx].sg_gui_fg_name = vim_strsave(arg);
		else
		    HL_TABLE()[idx].sg_gui_fg_name = NULL;
# ifdef USE_GUI_X11
		if (is_menu_group)
		    gui.menu_fg_pixel = i - 1;
		if (is_scrollbar_group)
		    gui.scroll_fg_pixel = i - 1;
# endif
	    }
	  }
#endif
	}
	else if (STRCMP(key, "GUIBG") == 0)
	{
#ifdef USE_GUI	    /* in non-GUI guibg colors are simply ignored */
	  if (!init || !(HL_TABLE()[idx].sg_set & SG_GUI))
	  {
	    if (!init)
		HL_TABLE()[idx].sg_set |= SG_GUI;

	    /* Add one to the argument, to avoid zero */
	    i = color_name2handle(arg) + 1;
	    if (i > 0 || STRCMP(arg, "NONE") == 0 || !gui.in_use)
	    {
		HL_TABLE()[idx].sg_gui_bg = i;
		vim_free(HL_TABLE()[idx].sg_gui_bg_name);
		if (STRCMP(arg, "NONE"))
		    HL_TABLE()[idx].sg_gui_bg_name = vim_strsave(arg);
		else
		    HL_TABLE()[idx].sg_gui_bg_name = NULL;
# ifdef USE_GUI_X11
		if (is_menu_group)
		    gui.menu_bg_pixel = i - 1;
		if (is_scrollbar_group)
		    gui.scroll_bg_pixel = i - 1;
# endif
	    }
	  }
#endif
	}
	else if (STRCMP(key, "START") == 0 || STRCMP(key, "STOP") == 0)
	{
	    char_u	buf[100];
	    char_u	*tname;

	    if (!init)
		HL_TABLE()[idx].sg_set |= SG_TERM;

	    /*
	     * The "start" and "stop"  arguments can be a literal escape
	     * sequence, or a comma seperated list of terminal codes.
	     */
	    if (STRNCMP(arg, "t_", 2) == 0)
	    {
		off = 0;
		buf[0] = 0;
		while (arg[off] != NUL)
		{
		    /* Isolate one termcap name */
		    for (len = 0; arg[off + len] &&
						 arg[off + len] != ','; ++len)
			;
		    tname = vim_strnsave(arg + off, len);
		    if (tname == NULL)		/* out of memory */
		    {
			error = TRUE;
			break;
		    }
		    /* lookup the escape sequence for the item */
		    p = get_term_code(tname);
		    vim_free(tname);
		    if (p == NULL)	    /* ignore non-existing things */
			p = (char_u *)"";

		    /* Append it to the already found stuff */
		    if ((int)(STRLEN(buf) + STRLEN(p)) >= 99)
		    {
			EMSG2("terminal code too long: %s", arg);
			error = TRUE;
			break;
		    }
		    STRCAT(buf, p);

		    /* Advance to the next item */
		    off += len;
		    if (arg[off] == ',')	    /* another one follows */
			++off;
		}
	    }
	    else
	    {
		/*
		 * Copy characters from arg[] to buf[], translating <> codes.
		 */
		for (p = arg, off = 0; off < 100 && *p; )
		{
		    len = trans_special(&p, buf + off, FALSE);
		    if (len)		    /* recognized special char */
			off += len;
		    else		    /* copy as normal char */
			buf[off++] = *p++;
		}
		buf[off] = NUL;
	    }
	    if (error)
		break;

	    if (STRCMP(buf, "NONE") == 0)	/* resetting the value */
		p = NULL;
	    else
		p = vim_strsave(buf);
	    if (key[2] == 'A')
	    {
		vim_free(HL_TABLE()[idx].sg_start);
		HL_TABLE()[idx].sg_start = p;
	    }
	    else
	    {
		vim_free(HL_TABLE()[idx].sg_stop);
		HL_TABLE()[idx].sg_stop = p;
	    }
	}
	else
	{
	    EMSG2("Illegal argument: %s", key_start);
	    error = TRUE;
	    break;
	}

	/*
	 * When highlighting has been given for a group, don't link it.
	 */
	if (!init || !(HL_TABLE()[idx].sg_set & SG_LINK))
	    HL_TABLE()[idx].sg_link = 0;

	/*
	 * Continue with next argument.
	 */
	linep = skipwhite(linep);
      }

    /*
     * If there is an error, and it's a new entry, remove it from the table.
     */
    if (error && idx == highlight_ga.ga_len)
	syn_unadd_group();
    else
    {
	if (is_normal_group)
	{
	    HL_TABLE()[idx].sg_term_attr = 0;
	    HL_TABLE()[idx].sg_cterm_attr = 0;
#ifdef USE_GUI
	    HL_TABLE()[idx].sg_gui_attr = 0;
	    /*
	     * Need to update all groups, because they might be using "bg"
	     * and/or "fg", which have been changed now.
	     */
	    if (gui.in_use)
		highlight_gui_started();
#endif
	}
#ifdef USE_GUI_X11
# ifdef WANT_MENU
	else if (is_menu_group)
	    gui_mch_new_menu_colors();
# endif
	else if (is_scrollbar_group)
	{
	    if (gui.in_use)
		gui_new_scrollbar_colors();
	}
#endif
	else
	    set_hl_attr(idx);
	redraw_all_later(NOT_VALID);
    }
    vim_free(key);
    vim_free(arg);

    /* Only call highlight_changed() once, after sourcing a syntax file */
    need_highlight_changed = TRUE;
}

/*
 * Clear highlighting for one group.
 */
    static void
highlight_clear(idx)
    int idx;
{
    HL_TABLE()[idx].sg_term = 0;
    vim_free(HL_TABLE()[idx].sg_start);
    HL_TABLE()[idx].sg_start = NULL;
    vim_free(HL_TABLE()[idx].sg_stop);
    HL_TABLE()[idx].sg_stop = NULL;
    HL_TABLE()[idx].sg_term_attr = 0;
    HL_TABLE()[idx].sg_cterm = 0;
    HL_TABLE()[idx].sg_cterm_bold = FALSE;
    HL_TABLE()[idx].sg_cterm_fg = 0;
    HL_TABLE()[idx].sg_cterm_bg = 0;
    HL_TABLE()[idx].sg_cterm_attr = 0;
#ifdef USE_GUI	    /* in non-GUI fonts are simply ignored */
    HL_TABLE()[idx].sg_gui = 0;
    HL_TABLE()[idx].sg_gui_fg = 0;
    vim_free(HL_TABLE()[idx].sg_gui_fg_name);
    HL_TABLE()[idx].sg_gui_fg_name = NULL;
    HL_TABLE()[idx].sg_gui_bg = 0;
    vim_free(HL_TABLE()[idx].sg_gui_bg_name);
    HL_TABLE()[idx].sg_gui_bg_name = NULL;
    gui_mch_free_font(HL_TABLE()[idx].sg_font);
    HL_TABLE()[idx].sg_font = 0;
    vim_free(HL_TABLE()[idx].sg_font_name);
    HL_TABLE()[idx].sg_font_name = NULL;
    HL_TABLE()[idx].sg_gui_attr = 0;
#endif
}

#ifdef USE_GUI
/*
 * Set the normal foreground and background colors according to the "Normal"
 * highlighighting group.  For X11 also set "Menu" and "Scrollbar" colors.
 */
    void
set_normal_colors()
{
    if (set_group_colors((char_u *)"Normal",
					    &gui.norm_pixel, &gui.back_pixel))
    {
	gui_mch_new_colors();
	must_redraw = CLEAR;
    }
#ifdef USE_GUI_X11
    if (set_group_colors((char_u *)"Menu",
				      &gui.menu_fg_pixel, &gui.menu_bg_pixel))
    {
# ifdef WANT_MENU
	gui_mch_new_menu_colors();
# endif
	must_redraw = CLEAR;
    }
    if (set_group_colors((char_u *)"Scrollbar",
				  &gui.scroll_fg_pixel, &gui.scroll_bg_pixel))
    {
	gui_new_scrollbar_colors();
	must_redraw = CLEAR;
    }
#endif
}

/*
 * Set the colors for "Normal", "Menu" or "Scrollbar".
 */
    static int
set_group_colors(name, fgp, bgp)
    char_u	*name;
    GuiColor	*fgp;
    GuiColor	*bgp;
{
    int		idx;

    idx = syn_name2id(name) - 1;
    if (idx >= 0)
    {
	gui_do_one_color(idx);

	if (HL_TABLE()[idx].sg_gui_fg > 0)
	    *fgp = HL_TABLE()[idx].sg_gui_fg - 1;
	else
	    *fgp = gui.def_norm_pixel;
	if (HL_TABLE()[idx].sg_gui_bg > 0)
	    *bgp = HL_TABLE()[idx].sg_gui_bg - 1;
	else
	    *bgp = gui.def_back_pixel;
	return TRUE;
    }
    return FALSE;
}

/*
 * Set font for "Normal" group.  Called by gui_mch_init_font() when a font has
 * actually chosen to be used.
 */
    void
hl_set_font_name(font_name)
    char_u	*font_name;
{
    int	    id;

    id = syn_name2id((char_u *)"Normal");
    if (id > 0)
    {
	vim_free(HL_TABLE()[id - 1].sg_font_name);
	HL_TABLE()[id - 1].sg_font_name = vim_strsave(font_name);
    }
}

/*
 * Set background color for "Normal" group.  Called by gui_set_bg_color()
 * when the color is known.
 */
    void
hl_set_bg_color_name(name)
    char_u  *name;	    /* must have been allocated */
{
    int	    id;

    if (name != NULL)
    {
	id = syn_name2id((char_u *)"Normal");
	if (id > 0)
	{
	    vim_free(HL_TABLE()[id - 1].sg_gui_bg_name);
	    HL_TABLE()[id - 1].sg_gui_bg_name = name;
	}
    }
}

/*
 * Set foreground color for "Normal" group.  Called by gui_set_fg_color()
 * when the color is known.
 */
    void
hl_set_fg_color_name(name)
    char_u  *name;	    /* must have been allocated */
{
    int	    id;

    if (name != NULL)
    {
	id = syn_name2id((char_u *)"Normal");
	if (id > 0)
	{
	    vim_free(HL_TABLE()[id - 1].sg_gui_fg_name);
	    HL_TABLE()[id - 1].sg_gui_fg_name = name;
	}
    }
}

/*
 * Return the handle for a color name.
 * Returns -1 when failed.
 */
    static GuiColor
color_name2handle(name)
    char_u  *name;
{
    if (STRCMP(name, "NONE") == 0)
	return (GuiColor)-1;

    if (STRICMP(name, "fg") == 0 || STRICMP(name, "foreground") == 0)
	return gui.norm_pixel;
    if (STRICMP(name, "bg") == 0 || STRICMP(name, "background") == 0)
	return gui.back_pixel;

    return gui_mch_get_color(name);
}

/*
 * Return the handle for a font name.
 * Returns 0 when failed.
 */
    static GuiFont
font_name2handle(name)
    char_u  *name;
{
    if (STRCMP(name, "NONE") == 0)
	return (GuiFont)0;

    return gui_mch_get_font(name, TRUE);
}
#endif /* USE_GUI */

/*
 * Table with the specifications for an attribute number.
 * Note that this table is used by ALL buffers.  This is required because the
 * GUI can redraw at any time for any buffer.
 */
struct growarray    term_attr_table = {0, 0, 0, 0, NULL};

#define TERM_ATTR_ENTRY(idx) ((struct attr_entry *)term_attr_table.ga_data)[idx]

struct growarray    cterm_attr_table = {0, 0, 0, 0, NULL};

#define CTERM_ATTR_ENTRY(idx) ((struct attr_entry *)cterm_attr_table.ga_data)[idx]

#ifdef USE_GUI
struct growarray    gui_attr_table = {0, 0, 0, 0, NULL};

#define GUI_ATTR_ENTRY(idx) ((struct attr_entry *)gui_attr_table.ga_data)[idx]
#endif

/*
 * Return the attr number for a set of colors and font.
 * Add a new entry to the term_attr_table, cterm_attr_table or gui_attr_table
 * if the combination is new.
 * Return 0 for error (no more room).
 */
    static int
get_attr_entry(table, aep)
    struct growarray	*table;
    struct attr_entry	*aep;
{
    int			i;
    struct attr_entry	*gap;
    static int		recursive = FALSE;

    /*
     * Init the table, in case it wasn't done yet.
     */
    table->ga_itemsize = sizeof(struct attr_entry);
    table->ga_growsize = 7;

    /*
     * Try to find an entry with the same specifications.
     */
    for (i = 0; i < table->ga_len; ++i)
    {
	gap = &(((struct attr_entry *)table->ga_data)[i]);
	if (	   aep->ae_attr == gap->ae_attr
		&& (
#ifdef USE_GUI
		       (table == &gui_attr_table
			&& (aep->ae_u.gui.fg_color == gap->ae_u.gui.fg_color
			    && aep->ae_u.gui.bg_color == gap->ae_u.gui.bg_color
			    && aep->ae_u.gui.font == gap->ae_u.gui.font))
		    ||
#endif
		       (table == &term_attr_table
			&& (aep->ae_u.term.start == NULL) ==
						(gap->ae_u.term.start == NULL)
			&& (aep->ae_u.term.start == NULL
			    || STRCMP(aep->ae_u.term.start,
						   gap->ae_u.term.start) == 0)
			&& (aep->ae_u.term.stop == NULL) ==
						 (gap->ae_u.term.stop == NULL)
			&& (aep->ae_u.term.stop == NULL
			    || STRCMP(aep->ae_u.term.stop,
						   gap->ae_u.term.stop) == 0))
		    || (table == &cterm_attr_table
			&& aep->ae_u.cterm.fg_color == gap->ae_u.cterm.fg_color
			&& aep->ae_u.cterm.bg_color == gap->ae_u.cterm.bg_color)
		     ))

	return i + ATTR_OFF;
    }

    if (table->ga_len + ATTR_OFF == 256)
    {
	/*
	 * Running out of attribute entries!  remove all attributes, and
	 * compute new ones for all groups.
	 * When called recursively, we are really out of numbers.
	 */
	if (recursive)
	{
	    EMSG("Too many different highlighting attributes in use");
	    return 0;
	}
	recursive = TRUE;

#ifdef USE_GUI
	ga_clear(&gui_attr_table);
#endif
	ga_clear(&term_attr_table);
	ga_clear(&cterm_attr_table);
	must_redraw = CLEAR;

	for (i = 0; i < highlight_ga.ga_len; ++i)
	    set_hl_attr(i);

	recursive = FALSE;
    }

    /*
     * This is a new combination of colors and font, add an entry.
     */
    if (ga_grow(table, 1) == FAIL)
	return 0;

    gap = &(((struct attr_entry *)table->ga_data)[table->ga_len]);
    vim_memset(gap, 0, sizeof(struct attr_entry));
    gap->ae_attr = aep->ae_attr;
#ifdef USE_GUI
    if (table == &gui_attr_table)
    {
	gap->ae_u.gui.fg_color = aep->ae_u.gui.fg_color;
	gap->ae_u.gui.bg_color = aep->ae_u.gui.bg_color;
	gap->ae_u.gui.font = aep->ae_u.gui.font;
    }
#endif
    if (table == &term_attr_table)
    {
	if (aep->ae_u.term.start == NULL)
	    gap->ae_u.term.start = NULL;
	else
	    gap->ae_u.term.start = vim_strsave(aep->ae_u.term.start);
	if (aep->ae_u.term.stop == NULL)
	    gap->ae_u.term.stop = NULL;
	else
	    gap->ae_u.term.stop = vim_strsave(aep->ae_u.term.stop);
    }
    else if (table == &cterm_attr_table)
    {
	gap->ae_u.cterm.fg_color = aep->ae_u.cterm.fg_color;
	gap->ae_u.cterm.bg_color = aep->ae_u.cterm.bg_color;
    }
    ++table->ga_len;
    --table->ga_room;
    return (table->ga_len - 1 + ATTR_OFF);
}

#ifdef USE_GUI

    struct attr_entry *
syn_gui_attr2entry(attr)
    int		    attr;
{
    attr -= ATTR_OFF;
    if (attr >= gui_attr_table.ga_len)	    /* did ":syntax clear" */
	return NULL;
    return &(GUI_ATTR_ENTRY(attr));
}

#endif /* USE_GUI */

    struct attr_entry *
syn_term_attr2entry(attr)
    int		    attr;
{
    attr -= ATTR_OFF;
    if (attr >= term_attr_table.ga_len)	    /* did ":syntax clear" */
	return NULL;
    return &(TERM_ATTR_ENTRY(attr));
}

    struct attr_entry *
syn_cterm_attr2entry(attr)
    int		    attr;
{
    attr -= ATTR_OFF;
    if (attr >= cterm_attr_table.ga_len)	/* did ":syntax clear" */
	return NULL;
    return &(CTERM_ATTR_ENTRY(attr));
}

#define LIST_ATTR   1
#define LIST_STRING 2
#define LIST_INT    3

    static void
highlight_list_one(id)
    int		id;
{
    struct hl_group	*sgp;
    int			didh = FALSE;

    sgp = &HL_TABLE()[id - 1];	    /* index is ID minus one */

    didh = highlight_list_arg(id, didh, LIST_ATTR,
				    sgp->sg_term, NULL, "term");
    didh = highlight_list_arg(id, didh, LIST_STRING,
				    0, sgp->sg_start, "start");
    didh = highlight_list_arg(id, didh, LIST_STRING,
				    0, sgp->sg_stop, "stop");

    didh = highlight_list_arg(id, didh, LIST_ATTR,
				    sgp->sg_cterm, NULL, "cterm");
    didh = highlight_list_arg(id, didh, LIST_INT,
				    sgp->sg_cterm_fg, NULL, "ctermfg");
    didh = highlight_list_arg(id, didh, LIST_INT,
				    sgp->sg_cterm_bg, NULL, "ctermbg");

#ifdef USE_GUI
    didh = highlight_list_arg(id, didh, LIST_ATTR,
				    sgp->sg_gui, NULL, "gui");
    didh = highlight_list_arg(id, didh, LIST_STRING,
				    0, sgp->sg_gui_fg_name, "guifg");
    didh = highlight_list_arg(id, didh, LIST_STRING,
				    0, sgp->sg_gui_bg_name, "guibg");
    didh = highlight_list_arg(id, didh, LIST_STRING,
				    0, sgp->sg_font_name, "font");
#endif

    if (sgp->sg_link)
    {
	(void)syn_list_header(didh, 9999, id);
	msg_puts_attr((char_u *)"links to", hl_attr(HLF_D));
	msg_putchar(' ');
	msg_outtrans(HL_TABLE()[HL_TABLE()[id - 1].sg_link - 1].sg_name);
    }
}

    static int
highlight_list_arg(id, didh, type, iarg, sarg, name)
    int		id;
    int		didh;
    int		type;
    int		iarg;
    char_u	*sarg;
    char	*name;
{
    char_u	buf[100];
    char_u	*ts;
    int		i;

    if (type == LIST_STRING ? (sarg != NULL) : (iarg != 0))
    {
	ts = buf;
	if (type == LIST_INT)
	    sprintf((char *)buf, "%d", iarg - 1);
	else if (type == LIST_STRING)
	    ts = sarg;
	else /* type == LIST_ATTR */
	{
	    buf[0] = NUL;
	    for (i = 0; hl_attr_table[i] != 0; ++i)
	    {
		if (iarg & hl_attr_table[i])
		{
		    if (buf[0] != NUL)
			STRCAT(buf, ",");
		    STRCAT(buf, hl_name_table[i]);
		    iarg &= ~hl_attr_table[i];	    /* don't want "inverse" */
		}
	    }
	}

	(void)syn_list_header(didh,
			       (int)(vim_strsize(ts) + STRLEN(name) + 1), id);
	didh = TRUE;

	MSG_PUTS_ATTR(name, hl_attr(HLF_D));
	MSG_PUTS_ATTR("=", hl_attr(HLF_D));
	msg_outtrans(ts);
    }
    return didh;
}

#if (defined(WANT_EVAL) && defined(SYNTAX_HL)) || defined(PROTO)
/*
 * Return "1" if highlight group "id" has attribute "flag".
 * Return NULL otherwise.
 */
    char_u *
highlight_has_attr(id, flag, modec)
    int		id;
    int		flag;
    int		modec;	/* 'g' for GUI, 'c' for cterm, 't' for term */
{
    int		attr;

    if (id <= 0 || id > highlight_ga.ga_len)
	return NULL;

#ifdef USE_GUI
    if (modec == 'g')
	attr = HL_TABLE()[id - 1].sg_gui;
    else
#endif
	 if (modec == 'c')
	attr = HL_TABLE()[id - 1].sg_cterm;
    else
	attr = HL_TABLE()[id - 1].sg_term;

    if (attr & flag)
	return (char_u *)"1";
    return NULL;
}
#endif

#if (defined(SYNTAX_HL) && defined(WANT_EVAL)) || defined(PROTO)
/*
 * Return color name of highlight group "id".
 */
    char_u *
highlight_color(id, what, modec)
    int		id;
    char_u	*what;	/* "fg", "bg", "fg#" or "bg#" */
    int		modec;	/* 'g' for GUI, 'c' for cterm, 't' for term */
{
    static char_u	name[20];
    int			n;
    int			fg;

    if (id <= 0 || id > highlight_ga.ga_len)
	return NULL;

    if (TO_LOWER(what[0]) == 'f')
	fg = TRUE;
    else
	fg = FALSE;
#ifdef USE_GUI
    if (modec == 'g')
    {
	/* return #RRGGBB form (only possible when GUI is running) */
	if (gui.in_use && what[1] && what[2] == '#')
	{
	    GuiColor	color;

	    if (fg)
		color = HL_TABLE()[id - 1].sg_gui_fg;
	    else
		color = HL_TABLE()[id - 1].sg_gui_bg;
	    if (color == 0)
		return NULL;
	    return gui_mch_get_rgb(color - 1);
	}
	if (fg)
	    return (HL_TABLE()[id - 1].sg_gui_fg_name);
	return (HL_TABLE()[id - 1].sg_gui_bg_name);
    }
#endif
    if (modec == 'c')
    {
	if (fg)
	    n = HL_TABLE()[id - 1].sg_cterm_fg - 1;
	else
	    n = HL_TABLE()[id - 1].sg_cterm_bg - 1;
	sprintf((char *)name, "%d", n);
	return name;
    }
    /* term doesn't have color */
    return NULL;
}
#endif

/*
 * Output the syntax list header.
 * Return TRUE when started a new line.
 */
    static int
syn_list_header(did_header, outlen, id)
    int	    did_header;		/* did header already */
    int	    outlen;		/* length of string that comes */
    int	    id;			/* highlight group id */
{
    int	    endcol = 15;
    int	    newline = TRUE;

    if (!did_header)
    {
	msg_putchar('\n');
	msg_outtrans(HL_TABLE()[id - 1].sg_name);
    }
    else if (msg_col + outlen + 1 >= Columns)
	msg_putchar('\n');
    else
    {
	if (msg_col >= endcol)	/* wrap around is like starting a new line */
	    newline = FALSE;
	msg_putchar(' ');
    }

    if (msg_col >= endcol)	/* output at least one space */
	endcol = msg_col + 1;
    if (Columns <= endcol)	/* avoid hang for tiny window */
	endcol = Columns - 1;

    msg_advance(endcol);

    return newline;
}

/*
 * Set the attribute numbers for a highlight group.
 * Called after one of the attributes has changed.
 */
    static void
set_hl_attr(idx)
    int		    idx;	    /* index in array */
{
    struct attr_entry at_en;

    /* The "Normal" group doesn't need an attribute number */
    if (STRCMP(HL_TABLE()[idx].sg_name_u, "NORMAL") == 0)
	return;

#ifdef USE_GUI
    /*
     * For the GUI mode: If there are other than "normal" highlighting
     * attributes, need to allocate an attr number.
     */
    if (HL_TABLE()[idx].sg_gui_fg == 0
	    && HL_TABLE()[idx].sg_gui_bg == 0
	    && HL_TABLE()[idx].sg_font == 0)
    {
	HL_TABLE()[idx].sg_gui_attr = HL_TABLE()[idx].sg_gui;
    }
    else
    {
	at_en.ae_attr = HL_TABLE()[idx].sg_gui;
	at_en.ae_u.gui.fg_color = HL_TABLE()[idx].sg_gui_fg;
	at_en.ae_u.gui.bg_color = HL_TABLE()[idx].sg_gui_bg;
	at_en.ae_u.gui.font = HL_TABLE()[idx].sg_font;
	HL_TABLE()[idx].sg_gui_attr = get_attr_entry(&gui_attr_table, &at_en);
    }
#endif
    /*
     * For the term mode: If there are other than "normal" highlighting
     * attributes, need to allocate an attr number.
     */
    if (HL_TABLE()[idx].sg_start == NULL && HL_TABLE()[idx].sg_stop == NULL)
	HL_TABLE()[idx].sg_term_attr = HL_TABLE()[idx].sg_term;
    else
    {
	at_en.ae_attr = HL_TABLE()[idx].sg_term;
	at_en.ae_u.term.start = HL_TABLE()[idx].sg_start;
	at_en.ae_u.term.stop = HL_TABLE()[idx].sg_stop;
	HL_TABLE()[idx].sg_term_attr = get_attr_entry(&term_attr_table, &at_en);
    }

    /*
     * For the color term mode: If there are other than "normal"
     * highlighting attributes, need to allocate an attr number.
     */
    if (HL_TABLE()[idx].sg_cterm_fg == 0 && HL_TABLE()[idx].sg_cterm_bg == 0)
	HL_TABLE()[idx].sg_cterm_attr = HL_TABLE()[idx].sg_cterm;
    else
    {
	at_en.ae_attr = HL_TABLE()[idx].sg_cterm;
	at_en.ae_u.cterm.fg_color = HL_TABLE()[idx].sg_cterm_fg;
	at_en.ae_u.cterm.bg_color = HL_TABLE()[idx].sg_cterm_bg;
	HL_TABLE()[idx].sg_cterm_attr =
				    get_attr_entry(&cterm_attr_table, &at_en);
    }
}

/*
 * Lookup a highlight group name and return it's ID.
 * If it is not found, 0 is returned.
 */
    int
syn_name2id(name)
    char_u	*name;
{
    int		i;
    char_u	*name_u;

    /* Avoid using stricmp() too much, it's slow on some systems */
    name_u = vim_strsave_up(name);
    if (name_u == NULL)
	return 0;
    for (i = highlight_ga.ga_len; --i >= 0; )
	if (HL_TABLE()[i].sg_name_u != NULL
		&& STRCMP(name_u, HL_TABLE()[i].sg_name_u) == 0)
	    break;
    vim_free(name_u);
    return i + 1;
}

#if defined(WANT_EVAL) || defined(PROTO)
/*
 * Return TRUE if highlight group "name" exists.
 */
    int
highlight_exists(name)
    char_u	*name;
{
    return (syn_name2id(name) > 0);
}
#endif

/*
 * Like syn_name2id(), but take a pointer + length argument.
 */
    static int
syn_namen2id(linep, len)
    char_u  *linep;
    int	    len;
{
    char_u  *name;
    int	    id = 0;

    name = vim_strnsave(linep, len);
    if (name != NULL)
    {
	id = syn_name2id(name);
	vim_free(name);
    }
    return id;
}

/*
 * Find highlight group name in the table and return it's ID.
 * The argument is a pointer to the name and the length of the name.
 * If it doesn't exist yet, a new entry is created.
 * Return 0 for failure.
 */
    int
syn_check_group(pp, len)
    char_u		*pp;
    int			len;
{
    int	    id;
    char_u  *name;

    name = vim_strnsave(pp, len);
    if (name == NULL)
	return 0;

    id = syn_name2id(name);
    if (id == 0)			/* doesn't exist yet */
	id = syn_add_group(name);
    else
	vim_free(name);
    return id;
}

/*
 * Add new highlight group and return it's ID.
 * "name" must be an allocated string, it will be consumed.
 * Return 0 for failure.
 */
    static int
syn_add_group(name)
    char_u		*name;
{
    /*
     * First call for this growarray: init growing array.
     */
    if (highlight_ga.ga_data == NULL)
    {
	highlight_ga.ga_itemsize = sizeof(struct hl_group);
	highlight_ga.ga_growsize = 10;
    }

    /*
     * Make room for at least one other syntax_highlight entry.
     */
    if (ga_grow(&highlight_ga, 1) == FAIL)
    {
	vim_free(name);
	return 0;
    }

    vim_memset(&(HL_TABLE()[highlight_ga.ga_len]), 0, sizeof(struct hl_group));
    HL_TABLE()[highlight_ga.ga_len].sg_name = name;
    HL_TABLE()[highlight_ga.ga_len].sg_name_u = vim_strsave_up(name);
    ++highlight_ga.ga_len;
    --highlight_ga.ga_room;

    return highlight_ga.ga_len;		    /* ID is index plus one */
}

/*
 * When, just after calling syn_add_group(), an error is discovered, this
 * function deletes the new name.
 */
    static void
syn_unadd_group()
{
    --highlight_ga.ga_len;
    ++highlight_ga.ga_room;
    vim_free(HL_TABLE()[highlight_ga.ga_len].sg_name);
    vim_free(HL_TABLE()[highlight_ga.ga_len].sg_name_u);
}

/*
 * Translate a group ID to highlight attributes.
 */
    int
syn_id2attr(hl_id)
    int			hl_id;
{
    int			attr;
    struct hl_group	*sgp;

    hl_id = syn_get_final_id(hl_id);
    sgp = &HL_TABLE()[hl_id - 1];	    /* index is ID minus one */

#ifdef USE_GUI
    /*
     * Only use GUI attr when the GUI is being used.
     */
    if (gui.in_use)
	attr = sgp->sg_gui_attr;
    else
#endif
	if (*T_CCO)
	    attr = sgp->sg_cterm_attr;
	else
	    attr = sgp->sg_term_attr;

    return attr;
}

#ifdef USE_GUI
/*
 * Get the GUI colors and attributes for a group ID.
 * NOTE: the colors will be 0 when not set, the color plus one otherwise.
 */
    int
syn_id2colors(hl_id, fgp, bgp)
    int		hl_id;
    GuiColor	*fgp;
    GuiColor	*bgp;
{
    struct hl_group	*sgp;

    hl_id = syn_get_final_id(hl_id);
    sgp = &HL_TABLE()[hl_id - 1];	    /* index is ID minus one */

    *fgp = sgp->sg_gui_fg;
    *bgp = sgp->sg_gui_bg;
    return sgp->sg_gui;
}
#endif

/*
 * Translate a group ID to the final group ID (following links).
 */
    int
syn_get_final_id(hl_id)
    int			hl_id;
{
    int			count;
    struct hl_group	*sgp;

    if (hl_id > highlight_ga.ga_len || hl_id < 1)
	return 0;			/* Can be called from eval!! */

    /*
     * Follow links until there is no more.
     * Look out for loops!  Break after 100 links.
     */
    for (count = 100; --count >= 0; )
    {
	sgp = &HL_TABLE()[hl_id - 1];	    /* index is ID minus one */
	if (sgp->sg_link == 0 || sgp->sg_link > highlight_ga.ga_len)
	    break;
	hl_id = sgp->sg_link;
    }

    return hl_id;
}

#ifdef USE_GUI
/*
 * Call this function just after the GUI has started.
 * It finds the font and color handles for the highlighting groups.
 */
    void
highlight_gui_started()
{
    int	    idx;

    /* First get the colors from the "Normal" and "Menu" group, if set */
    set_normal_colors();

    for (idx = 0; idx < highlight_ga.ga_len; ++idx)
	gui_do_one_color(idx);

    highlight_changed();
}

    static void
gui_do_one_color(idx)
    int	    idx;
{
    int	    didit = FALSE;

    if (HL_TABLE()[idx].sg_font_name != NULL)
    {
	HL_TABLE()[idx].sg_font =
	    font_name2handle(HL_TABLE()[idx].sg_font_name);
	didit = TRUE;
    }
    if (HL_TABLE()[idx].sg_gui_fg_name != NULL)
    {
	HL_TABLE()[idx].sg_gui_fg =
	    color_name2handle(HL_TABLE()[idx].sg_gui_fg_name) + 1;
	didit = TRUE;
    }
    if (HL_TABLE()[idx].sg_gui_bg_name != NULL)
    {
	HL_TABLE()[idx].sg_gui_bg =
	    color_name2handle(HL_TABLE()[idx].sg_gui_bg_name) + 1;
	didit = TRUE;
    }
    if (didit)	/* need to get a new attr number */
	set_hl_attr(idx);
}

#endif

/*
 * Translate the 'highlight' option into attributes in highlight_attr[] and
 * set up the user highlights User1..9.  If STATUSLINE is in use, a set of
 * corresponding highlights to use on top of HLF_SNC is computed.
 * Called only when the 'highlight' option has been changed and upon first
 * screen redraw after any :highlight command.
 * Return FAIL when an invalid flag is found in 'highlight'.  OK otherwise.
 */
    int
highlight_changed()
{
    int		hlf;
    int		i;
    char_u	*p;
    int		attr;
    char_u	*end;
    int		id;
#ifdef USER_HIGHLIGHT
    char_u      userhl[10];
# ifdef STATUSLINE
    int		id_SNC = -1;
    int		id_S = -1;
    int		hlcnt;
# endif
#endif
    static int	hl_flags[HLF_COUNT] = HL_FLAGS;

    need_highlight_changed = FALSE;

    /*
     * Clear all attributes.
     */
    for (hlf = 0; hlf < (int)HLF_COUNT; ++hlf)
	highlight_attr[hlf] = 0;

    /*
     * First set all attributes to their default value.
     * Then use the attributes from the 'highlight' option.
     */
    for (i = 0; i < 2; ++i)
    {
	if (i)
	    p = p_hl;
	else
	    p = get_highlight_default();
	if (p == NULL)	    /* just in case */
	    continue;

	while (*p)
	{
	    for (hlf = 0; hlf < (int)HLF_COUNT; ++hlf)
		if (hl_flags[hlf] == *p)
		    break;
	    ++p;
	    if (hlf == (int)HLF_COUNT || *p == NUL)
		return FAIL;

	    /*
	     * Allow several hl_flags to be combined, like "bu" for
	     * bold-underlined.
	     */
	    attr = 0;
	    for ( ; *p && *p != ','; ++p)	    /* parse upto comma */
	    {
		if (vim_iswhite(*p))		    /* ignore white space */
		    continue;

		if (attr > HL_ALL)  /* Combination with ':' is not allowed. */
		    return FAIL;

		switch (*p)
		{
		    case 'b':	attr |= HL_BOLD;
				break;
		    case 'i':	attr |= HL_ITALIC;
				break;
		    case '-':
		    case 'n':			    /* no highlighting */
				break;
		    case 'r':	attr |= HL_INVERSE;
				break;
		    case 's':	attr |= HL_STANDOUT;
				break;
		    case 'u':	attr |= HL_UNDERLINE;
				break;
		    case ':':	++p;		    /* highlight group name */
				if (attr || *p == NUL)	 /* no combinations */
				    return FAIL;
				end = vim_strchr(p, ',');
				if (end == NULL)
				    end = p + STRLEN(p);
				id = syn_check_group(p, (int)(end - p));
				if (id == 0)
				    return FAIL;
				attr = syn_id2attr(id);
				p = end - 1;
#if defined(STATUSLINE) && defined(USER_HIGHLIGHT)
				if (hlf == (int)HLF_SNC)
				    id_SNC = syn_get_final_id(id);
				else if (hlf == (int)HLF_S)
				    id_S = syn_get_final_id(id);
#endif
				break;
		    default:	return FAIL;
		}
	    }
	    highlight_attr[hlf] = attr;

	    p = skip_to_option_part(p);	    /* skip comma and spaces */
	}
    }

#ifdef USER_HIGHLIGHT
    /* Setup the user highlights
     *
     * Temporarily  utilize 10 more hl entries.  Have to be in there
     * simultaneously in case of table overflows in get_attr_entry()
     */
# ifdef STATUSLINE
    if (ga_grow(&highlight_ga, 10) == FAIL)
	return FAIL;
    hlcnt = highlight_ga.ga_len;
    if (id_S == 0)
    {		    /* Make sure id_S is always valid to simplify code below */
	memset(&HL_TABLE()[hlcnt + 9], 0, sizeof(struct hl_group));
	HL_TABLE()[hlcnt + 9].sg_term = highlight_attr[HLF_S];
	id_S = hlcnt + 10;
    }
# endif
    for (i = 0; i < 9; i++)
    {
	sprintf((char *)userhl, "User%d", i + 1);
	id = syn_name2id(userhl);
	if (id == 0)
	{
	    highlight_user[i] = 0;
# ifdef STATUSLINE
	    highlight_stlnc[i] = 0;
# endif
	}
	else
	{
# ifdef STATUSLINE
	    struct hl_group *hlt = HL_TABLE();
# endif

	    highlight_user[i] = syn_id2attr(id);
# ifdef STATUSLINE
	    if (id_SNC == 0)
	    {
		memset(&hlt[hlcnt + i], 0, sizeof(struct hl_group));
		hlt[hlcnt + i].sg_term = highlight_attr[HLF_SNC];
		hlt[hlcnt + i].sg_cterm = highlight_attr[HLF_SNC];
#  ifdef USE_GUI
		hlt[hlcnt + i].sg_gui = highlight_attr[HLF_SNC];
#  endif
	    }
	    else
		mch_memmove(&hlt[hlcnt + i],
			    &hlt[id_SNC - 1],
			    sizeof(struct hl_group));
	    hlt[hlcnt + i].sg_link = 0;

	    /* Apply difference between UserX and HLF_S to HLF_SNC */
	    hlt[hlcnt + i].sg_term ^=
		hlt[id - 1].sg_term ^ hlt[id_S - 1].sg_term;
	    if (hlt[id - 1].sg_start != hlt[id_S - 1].sg_start)
		hlt[hlcnt + i].sg_start = hlt[id - 1].sg_start;
	    if (hlt[id - 1].sg_stop != hlt[id_S - 1].sg_stop)
		hlt[hlcnt + i].sg_stop = hlt[id - 1].sg_stop;
	    hlt[hlcnt + i].sg_cterm ^=
		hlt[id - 1].sg_cterm ^ hlt[id_S - 1].sg_cterm;
	    if (hlt[id - 1].sg_cterm_fg != hlt[id_S - 1].sg_cterm_fg)
		hlt[hlcnt + i].sg_cterm_fg = hlt[id - 1].sg_cterm_fg;
	    if (hlt[id - 1].sg_cterm_bg != hlt[id_S - 1].sg_cterm_bg)
		hlt[hlcnt + i].sg_cterm_bg = hlt[id - 1].sg_cterm_bg;
#  ifdef USE_GUI
	    hlt[hlcnt + i].sg_gui ^=
		hlt[id - 1].sg_gui ^ hlt[id_S - 1].sg_gui;
	    if (hlt[id - 1].sg_gui_fg != hlt[id_S - 1].sg_gui_fg)
		hlt[hlcnt + i].sg_gui_fg = hlt[id - 1].sg_gui_fg;
	    if (hlt[id - 1].sg_gui_bg != hlt[id_S - 1].sg_gui_bg)
		hlt[hlcnt + i].sg_gui_bg = hlt[id - 1].sg_gui_bg;
	    if (hlt[id - 1].sg_font != hlt[id_S - 1].sg_font)
		hlt[hlcnt + i].sg_font = hlt[id - 1].sg_font;
#  endif
	    highlight_ga.ga_len = hlcnt + i + 1;
	    set_hl_attr(hlcnt + i);	/* At long last we can apply */
	    highlight_stlnc[i] = syn_id2attr(hlcnt + i + 1);
# endif
	}
    }
# ifdef STATUSLINE
    highlight_ga.ga_len = hlcnt;
# endif

#endif /* USER_HIGHLIGHT */

    return OK;
}

#ifdef CMDLINE_COMPL

static void highlight_list __ARGS((void));
static void highlight_list_two __ARGS((int cnt, int attr));

/*
 * Handle command line completion for :highlight command.
 */
    void
set_context_in_highlight_cmd(arg)
    char_u *arg;
{
    char_u	*p;

    /* Default: expand group names */
    expand_context = EXPAND_HIGHLIGHT;
    expand_pattern = arg;
    include_link = TRUE;

    /* (part of) subcommand already typed */
    if (*arg != NUL)
    {
	p = skiptowhite(arg);
	if (*p != NUL)			/* past group name */
	{
	    include_link = FALSE;
	    if (arg[1] == 'i' && arg[0] == 'N')
		highlight_list();
	    if (STRNCMP("link", arg, p - arg) == 0
		    || STRNCMP("clear", arg, p - arg) == 0)
	    {
		expand_pattern = skipwhite(p);
		p = skiptowhite(expand_pattern);
		if (*p != NUL)			/* past first group name */
		{
		    expand_pattern = skipwhite(p);
		    p = skiptowhite(expand_pattern);
		}
	    }
	    if (*p != NUL)			/* past group name(s) */
		expand_context = EXPAND_NOTHING;
	}
    }
}

/*
 * List highlighting matches in a nice way.
 */
    static void
highlight_list()
{
    int		i;

    for (i = 10; --i >= 0; )
	highlight_list_two(i, hl_attr(HLF_D));
    for (i = 40; --i >= 0; )
	highlight_list_two(55, 0);
}

    static void
highlight_list_two(cnt, attr)
    int	    cnt;
    int	    attr;
{
    msg_puts_attr((char_u *)("NI!  \b" + cnt / 11), attr);
    msg_clr_eos();
    out_flush();
    ui_delay(cnt == 55 ? 40L : (long)cnt * 50L, FALSE);
}

#endif /* CMDLINE_COMPL */

#if defined(CMDLINE_COMPL) || (defined(SYNTAX_HL) && defined(WANT_EVAL)) \
    || defined(PROTO)
/*
 * Function given to ExpandGeneric() to obtain the list of group names.
 * Also used for synIDattr() function.
 */
    char_u *
get_highlight_name(idx)
    int	    idx;
{
    if (idx == highlight_ga.ga_len
#ifdef CMDLINE_COMPL
	    && include_link
#endif
	    )
	return (char_u *)"link";
    if (idx == highlight_ga.ga_len + 1
#ifdef CMDLINE_COMPL
	    && include_link
#endif
	    )
	return (char_u *)"clear";
    if (idx < 0 || idx >= highlight_ga.ga_len)
	return NULL;
    return HL_TABLE()[idx].sg_name;
}
#endif

#ifdef USE_GUI
/*
 * Free all the highlight group fonts.
 * Used when quitting for systems which need it.
 */
    void
free_highlight_fonts()
{
    int	    idx;

    for (idx = 0; idx < highlight_ga.ga_len; ++idx)
    {
	gui_mch_free_font(HL_TABLE()[idx].sg_font);
	HL_TABLE()[idx].sg_font = 0;
    }

    gui_mch_free_font(gui.norm_font);
    gui_mch_free_font(gui.bold_font);
    gui_mch_free_font(gui.ital_font);
    gui_mch_free_font(gui.boldital_font);
}
#endif

/**************************************
 *  End of Highlighting stuff	      *
 **************************************/
