/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * This file contains various definitions of structures that are used by Vim
 */

/*
 * There is something wrong in the SAS compiler that makes typedefs not
 * valid in include files.  Has been fixed in version 6.58.
 */
#if defined(SASC) && SASC < 658
typedef long		linenr_t;
typedef unsigned	colnr_t;
typedef unsigned short	short_u;
#endif

/*
 * file position
 */
typedef struct fpos	FPOS;

struct fpos
{
    linenr_t	    lnum;	    /* line number */
    colnr_t	    col;	    /* column number */
};

/*
 * Sorry to put this here, but gui.h needs the FPOS type, and WIN needs gui.h
 * for GuiScrollbar.  There is probably somewhere better it could go -- webb
 */
#ifdef USE_GUI
# include "gui.h"
#else
# ifdef XTERM_CLIP
#  include <X11/Intrinsic.h>
# endif
# define GuiColor int		    /* avoid error in prototypes */
#endif

/*
 * marks: positions in a file
 * (a normal mark is a lnum/col pair, the same as a file position)
 */

#define NMARKS		26	    /* max. # of named marks */
#define JUMPLISTSIZE	50	    /* max. # of marks in jump list */
#define TAGSTACKSIZE	20	    /* max. # of tags in tag stack */

struct filemark
{
    FPOS	    mark;	    /* cursor position */
    int		    fnum;	    /* file number */
};

/*
 * The taggy struct is used to store the information about a :tag command.
 */
struct taggy
{
    char_u	    *tagname;		/* tag name */
    struct filemark fmark;		/* cursor position BEFORE ":tag" */
    int		    cur_match;		/* match number */
};

/*
 * line number list
 */

/*
 * Each window can have a different line number associated with a buffer.
 * The window-pointer/line-number pairs are kept in the line number list.
 * The list of line numbers is kept in most-recently-used order.
 */

typedef struct window	    WIN;
typedef struct winfpos	    WINFPOS;

struct winfpos
{
    WINFPOS	*wl_next;	    /* next entry or NULL for last entry */
    WINFPOS	*wl_prev;	    /* previous entry or NULL for first entry */
    WIN		*wl_win;	    /* pointer to window that did set wl_lnum */
    FPOS	wl_fpos;	    /* last cursor position in the file */
};

/*
 * stuctures used for undo
 */

struct u_entry
{
    struct u_entry  *ue_next;	/* pointer to next entry in list */
    linenr_t	    ue_top;	/* number of line above undo block */
    linenr_t	    ue_bot;	/* number of line below undo block */
    linenr_t	    ue_lcount;	/* linecount when u_save called */
    char_u	    **ue_array;	/* array of lines in undo block */
    long	    ue_size;	/* number of lines in ue_array */
};

struct u_header
{
    struct u_header *uh_next;	/* pointer to next header in list */
    struct u_header *uh_prev;	/* pointer to previous header in list */
    struct u_entry  *uh_entry;	/* pointer to first entry */
    FPOS	     uh_cursor;	/* cursor position before saving */
    int		     uh_flags;	/* see below */
    FPOS	     uh_namedm[NMARKS];	/* marks before undo/after redo */
};

/* values for uh_flags */
#define UH_CHANGED  0x01	/* b_changed flag before undo/after redo */
#define UH_EMPTYBUF 0x02	/* buffer was empty */

/*
 * stuctures used in undo.c
 */
#if SIZEOF_INT > 2
# define ALIGN_LONG	/* longword alignment and use filler byte */
# define ALIGN_SIZE (sizeof(long))
#else
# define ALIGN_SIZE (sizeof(short))
#endif

#define ALIGN_MASK (ALIGN_SIZE - 1)

typedef struct m_info info_t;

/*
 * stucture used to link chunks in one of the free chunk lists.
 */
struct m_info
{
#ifdef ALIGN_LONG
    long_u   m_size;	/* size of the chunk (including m_info) */
#else
    short_u  m_size;	/* size of the chunk (including m_info) */
#endif
    info_t  *m_next;	/* pointer to next free chunk in the list */
};

/*
 * structure used to link blocks in the list of allocated blocks.
 */
struct m_block
{
    struct m_block  *mb_next;	/* pointer to next allocated block */
    info_t	    mb_info;	/* head of free chuck list for this block */
};

/*
 * Structure used for growing arrays.
 * This is used to store information that only grows, is deleted all at
 * once, and needs to be accessed by index.  See ga_clear() and ga_grow().
 */
struct growarray
{
    int	    ga_len;		    /* current number of items used */
    int	    ga_room;		    /* number of unused items at the end */
    int	    ga_itemsize;	    /* sizeof one item */
    int	    ga_growsize;	    /* number of items to grow each time */
    void    *ga_data;		    /* pointer to the first item */
};

/*
 * things used in memfile.c
 */

typedef struct block_hdr    BHDR;
typedef struct memfile	    MEMFILE;
typedef long		    blocknr_t;

/*
 * for each (previously) used block in the memfile there is one block header.
 *
 * The block may be linked in the used list OR in the free list.
 * The used blocks are also kept in hash lists.
 *
 * The used list is a doubly linked list, most recently used block first.
 *	The blocks in the used list have a block of memory allocated.
 *	mf_used_count is the number of pages in the used list.
 * The hash lists are used to quickly find a block in the used list.
 * The free list is a single linked list, not sorted.
 *	The blocks in the free list have no block of memory allocated and
 *	the contents of the block in the file (if any) is irrelevant.
 */

struct block_hdr
{
    BHDR	*bh_next;	    /* next block_hdr in free or used list */
    BHDR	*bh_prev;	    /* previous block_hdr in used list */
    BHDR	*bh_hash_next;	    /* next block_hdr in hash list */
    BHDR	*bh_hash_prev;	    /* previous block_hdr in hash list */
    blocknr_t	bh_bnum;		/* block number */
    char_u	*bh_data;	    /* pointer to memory (for used block) */
    int		bh_page_count;	    /* number of pages in this block */

#define BH_DIRTY    1
#define BH_LOCKED   2
    char	bh_flags;	    /* BH_DIRTY or BH_LOCKED */
};

/*
 * when a block with a negative number is flushed to the file, it gets
 * a positive number. Because the reference to the block is still the negative
 * number, we remember the translation to the new positive number in the
 * double linked trans lists. The structure is the same as the hash lists.
 */
typedef struct nr_trans NR_TRANS;

struct nr_trans
{
    NR_TRANS	*nt_next;	    /* next nr_trans in hash list */
    NR_TRANS	*nt_prev;	    /* previous nr_trans in hash list */
    blocknr_t	nt_old_bnum;		/* old, negative, number */
    blocknr_t	nt_new_bnum;		/* new, positive, number */
};

/*
 * Simplistic hashing scheme to quickly locate the blocks in the used list.
 * 64 blocks are found directly (64 * 4K = 256K, most files are smaller).
 */
#define MEMHASHSIZE	64
#define MEMHASH(nr)	((nr) & (MEMHASHSIZE - 1))

struct memfile
{
    char_u	*mf_fname;	    /* name of the file */
    char_u	*mf_ffname;	    /* idem, full path */
    int		mf_fd;		    /* file descriptor */
    BHDR	*mf_free_first;	    /* first block_hdr in free list */
    BHDR	*mf_used_first;	    /* mru block_hdr in used list */
    BHDR	*mf_used_last;	    /* lru block_hdr in used list */
    unsigned	mf_used_count;	    /* number of pages in used list */
    unsigned	mf_used_count_max;  /* maximum number of pages in memory */
    BHDR	*mf_hash[MEMHASHSIZE];	/* array of hash lists */
    NR_TRANS	*mf_trans[MEMHASHSIZE];	/* array of trans lists */
    blocknr_t	mf_blocknr_max;	    /* highest positive block number + 1*/
    blocknr_t	mf_blocknr_min;	    /* lowest negative block number - 1 */
    blocknr_t	mf_neg_count;	    /* number of negative blocks numbers */
    blocknr_t	mf_infile_count;    /* number of pages in the file */
    unsigned	mf_page_size;	    /* number of bytes in a page */
    int		mf_dirty;	    /* Set to TRUE if there are dirty blocks */
};

/*
 * things used in memline.c
 */
typedef struct info_pointer	IPTR;	    /* block/index pair */

/*
 * When searching for a specific line, we remember what blocks in the tree
 * are the branches leading to that block. This is stored in ml_stack.  Each
 * entry is a pointer to info in a block (may be data block or pointer block)
 */
struct info_pointer
{
    blocknr_t	ip_bnum;	/* block number */
    linenr_t	ip_low;		/* lowest lnum in this block */
    linenr_t	ip_high;	/* highest lnum in this block */
    int		ip_index;	/* index for block with current lnum */
};

#ifdef BYTE_OFFSET
typedef struct ml_chunksize
{
    int     mlcs_numlines;
    int     mlcs_totalsize;
} ML_CHUNKSIZE;

 /* Flags when calling ml_updatechunk() */

#define ML_CHNK_ADDLINE 1
#define ML_CHNK_DELLINE 2
#define ML_CHNK_UPDLINE 3
#endif

typedef struct memline MEMLINE;

/*
 * the memline structure holds all the information about a memline
 */
struct memline
{
    linenr_t	ml_line_count;	/* number of lines in the buffer */

    MEMFILE	*ml_mfp;	/* pointer to associated memfile */

#define ML_EMPTY	1	/* empty buffer */
#define ML_LINE_DIRTY	2	/* cached line was changed and allocated */
#define ML_LOCKED_DIRTY	4	/* ml_locked was changed */
#define ML_LOCKED_POS	8	/* ml_locked needs positive block number */
    int		ml_flags;

    IPTR	*ml_stack;	/* stack of pointer blocks (array of IPTRs) */
    int		ml_stack_top;	/* current top if ml_stack */
    int		ml_stack_size;	/* total number of entries in ml_stack */

    linenr_t	ml_line_lnum;	/* line number of cached line, 0 if not valid */
    char_u	*ml_line_ptr;	/* pointer to cached line */

    BHDR	*ml_locked;	/* block used by last ml_get */
    linenr_t	ml_locked_low;	/* first line in ml_locked */
    linenr_t	ml_locked_high;	/* last line in ml_locked */
    int		ml_locked_lineadd;  /* number of lines inserted in ml_locked */
#ifdef BYTE_OFFSET
    ML_CHUNKSIZE *ml_chunksize;
    int		ml_numchunks;
    int		ml_usedchunks;
#endif
};

#ifdef SYNTAX_HL
/*
 * Each keyword has one keyentry, which is linked in a hash list.
 */
struct keyentry
{
    struct keyentry *next;	/* next keyword in the hash list */
    int		    syn_inc_tag;    /* ":syn include" tag for this match */
    short	    syn_id;	/* syntax ID for this match (if non-zero) */
    short	    *next_list;	/* ID list for next match (if non-zero) */
    short	    flags;	/* see syntax.c */
    char_u	    keyword[1];	/* actually longer */
};

/*
 * syn_state contains syntax state at the start of a line.
 */
struct syn_state
{
    struct growarray	sst_ga;		/* growarray to store syntax state */
    short		*sst_next_list;	/* "nextgroup" list in this state
					   (this is a copy, don't free it! */
    int			sst_next_flags;	/* flags for sst_next_list */
};
#endif /* SYNTAX_HL */

/*
 * Structure shared between syntax.c, screen.c and gui_x11.c.
 */
struct attr_entry
{
    short	    ae_attr;		/* HL_BOLD, etc. */
    union
    {
	struct
	{
	    char_u	    *start;	/* start escape sequence */
	    char_u	    *stop;	/* stop escape sequence */
	} term;
	struct
	{
	    char_u	    fg_color;	/* foreground color number */
	    char_u	    bg_color;	/* background color number */
	} cterm;
# ifdef USE_GUI
	struct
	{
	    GuiColor	    fg_color;	/* foreground color handle */
	    GuiColor	    bg_color;	/* background color handle */
	    GuiFont	    font;	/* font handle */
	} gui;
# endif
    } ae_u;
};

/*
 * buffer: structure that holds information about one file
 *
 * Several windows can share a single Buffer
 * A buffer is unallocated if there is no memfile for it.
 * A buffer is new if the associated file has never been loaded yet.
 */

typedef struct buffer BUF;

struct buffer
{
    MEMLINE	     b_ml;		/* associated memline (also contains
					 * line count) */

    BUF		    *b_next;		/* links in list of buffers */
    BUF		    *b_prev;

    int		     b_changed;		/* 'modified': Set to TRUE if
					 * something in the file has
					 * been changed and not written out.
					 */

    int		     b_nwindows;	/* nr of windows open on this buffer */

    int		     b_flags;		/* various BF_ flags */

    /*
     * b_ffname has the full path of the file.
     * b_sfname is the name as the user typed it.
     * b_fname is the same as b_sfname, unless ":cd" has been done,
     *		then it is the same as b_ffname.
     */
    char_u	    *b_ffname;		/* full path file name */
    char_u	    *b_sfname;		/* short file name */
    char_u	    *b_fname;		/* current file name */

#ifdef UNIX
    int		     b_dev;		/* device number (-1 if not set) */
    ino_t	     b_ino;		/* inode number */
#endif
#ifdef macintosh
    FSSpec	     b_FSSpec;		/* MacOS File Identification */
#endif
#ifdef USE_SNIFF
    int		     b_sniff;		/* file was loaded through Sniff */
#endif

    int		     b_fnum;		/* file number for this file. */
    WINFPOS	    *b_winfpos;		/* list of last used lnum for
					 * each window */

    long	     b_mtime;		/* last change time of original file */
    long	     b_mtime_read;	/* last change time when reading */

    FPOS	     b_namedm[NMARKS];	/* current named marks (mark.c) */

    /* These variables are set when VIsual_active becomes FALSE */
    FPOS	     b_visual_start;	/* start pos of last VIsual */
    FPOS	     b_visual_end;	/* end position of last VIsual */
    int		     b_visual_mode;	/* VIsual_mode of last VIsual */

    FPOS	     b_last_cursor;	/* cursor position when last unloading
					   this buffer */

    /*
     * Character table, only used in charset.c for 'iskeyword'
     */
    char	     b_chartab[256];

    /*
     * start and end of an operator, also used for '[ and ']
     */
    FPOS	     b_op_start;
    FPOS	     b_op_end;

#ifdef VIMINFO
    int		     b_marks_read;	/* Have we read viminfo marks yet? */
#endif

    /*
     * The following only used in undo.c.
     */
    struct u_header *b_u_oldhead;	/* pointer to oldest header */
    struct u_header *b_u_newhead;	/* pointer to newest header */
    struct u_header *b_u_curhead;	/* pointer to current header */
    int		     b_u_numhead;	/* current number of headers */
    int		     b_u_synced;	/* entry lists are synced */

    /*
     * variables for "U" command in undo.c
     */
    char_u	    *b_u_line_ptr;	/* saved line for "U" command */
    linenr_t	     b_u_line_lnum;	/* line number of line in u_line */
    colnr_t	     b_u_line_colnr;	/* optional column number */

    /*
     * The following only used in undo.c
     */
    struct m_block   b_block_head;	/* head of allocated memory block list */
    info_t	    *b_m_search;	/* pointer to chunk before previously
					 * allocated/freed chunk */
    struct m_block  *b_mb_current;	/* block where m_search points in */
#ifdef INSERT_EXPAND
    int		     b_scanned;		/* ^N/^P have scanned this buffer */
#endif

    /*
     * Options "local" to a buffer.
     * They are here because their value depends on the type of file
     * or contents of the file being edited.
     * The "save" options are for when the paste option is set.
     */
    int		     b_p_initialized;	/* set when options initialized */
    int		     b_p_ai, b_p_ro, b_p_lisp;
    int		     b_p_inf;		/* infer case of ^N/^P completions */
#ifdef INSERT_EXPAND
    char_u	    *b_p_cpt;		/* 'complete', types of expansions */
#endif
    int		     b_p_bin, b_p_eol, b_p_et, b_p_ml, b_p_tx, b_p_swf;
#ifndef SHORT_FNAME
    int		     b_p_sn;
#endif

    long	     b_p_sw, b_p_sts, b_p_ts, b_p_tw, b_p_wm;
    char_u	    *b_p_ff, *b_p_fo;
#ifdef COMMENTS
    char_u	    *b_p_com;		/* 'comments' */
#endif
    char_u	    *b_p_isk;
#ifdef MULTI_BYTE
    char_u	    *b_p_fe;
#endif
#ifdef CRYPTV
    char_u	    *b_p_key;
#endif
    char_u	    *b_p_nf;	    /* Number formats */
    char_u	    *b_p_mps;

    /* saved values for when 'bin' is set */
    long	     b_p_wm_nobin, b_p_tw_nobin;
    int		     b_p_ml_nobin, b_p_et_nobin;

    /* saved values for when 'paste' is set */
    int		     b_p_ai_save, b_p_lisp_save;
    long	     b_p_tw_save, b_p_wm_save, b_p_sts_save;

#ifdef SMARTINDENT
    int		     b_p_si, b_p_si_save;
#endif
#ifdef CINDENT
    int		     b_p_cin;	    /* use C progam indent mode */
    int		     b_p_cin_save;  /* b_p_cin saved for paste mode */
    char_u	    *b_p_cino;	    /* C progam indent mode options */
    char_u	    *b_p_cink;	    /* C progam indent mode keys */
#endif
#if defined(CINDENT) || defined(SMARTINDENT)
    char_u	    *b_p_cinw;	    /* words extra indent for 'si' and 'cin' */
#endif
#ifdef SYNTAX_HL
    char_u	    *b_p_syn;	    /* 'syntax' */
#endif
#ifdef AUTOCMD
    char_u	    *b_p_ft;	    /* 'filetype' */
#endif
#ifdef WANT_OSFILETYPE
    char_u	    *b_p_oft;	    /* 'osfiletype' */
#endif
    /* end of buffer options */

    int		     b_start_ffc;   /* first char of 'ff' when edit started */

#ifdef WANT_EVAL
    struct growarray b_vars;	    /* internal variables, local to buffer */
#endif

    /* When a buffer is created, it starts without a swap file.  b_may_swap is
     * then set to indicate that a swap file may be opened later.  It is reset
     * if a swap file could not be opened.
     */
    int		     b_may_swap;
    int		     b_did_warn;    /* Set to 1 if user has been warned on
				     * first change of a read-only file */
    int		     b_help;	    /* buffer for help file */

#ifndef SHORT_FNAME
    int		     b_shortname;   /* this file has an 8.3 file name */
#endif

#ifdef HAVE_PERL_INTERP
    void	    *perl_private;
#endif

#ifdef HAVE_PYTHON
    void	    *python_ref;    /* The Python value referring to
				     * this buffer */
#endif

#ifdef HAVE_TCL
    void	    *tcl_ref;
#endif

#ifdef SYNTAX_HL
    struct keyentry	**b_keywtab;	      /* syntax keywords hash table */
    struct keyentry	**b_keywtab_ic;	      /* idem, ignore case */
    int			b_syn_ic;	      /* ignore case for :syn cmds */
    struct growarray	b_syn_patterns;	      /* table for syntax patterns */
    struct growarray	b_syn_clusters;	      /* table for syntax clusters */
    int			b_syn_sync_flags;     /* flags about how to sync */
    short		b_syn_sync_id;	      /* group to sync on */
    long		b_syn_sync_minlines;  /* minimal sync lines offset */
    long		b_syn_sync_maxlines;  /* maximal sync lines offset */
    char_u		*b_syn_linecont_pat;  /* line continuation pattern */
    vim_regexp		*b_syn_linecont_prog; /* line continuation program */
    int			b_syn_linecont_ic;    /* ignore-case flag for above */
    int			b_syn_topgrp;	      /* for ":syntax include" */
/*
 * b_syn_states[] contains the state stack for a number of consecutive lines,
 * for the start of that line (col == 0).
 * Each position in the stack is an index in b_syn_patterns[].
 * Normally these are the lines currently being displayed.
 * b_syn_states	     is a pointer to an array of struct syn_state.
 * b_syn_states_len  is the number of entries in b_syn_states[].
 * b_syn_states_lnum is the first line that is in b_syn_states[].
 * b_syn_change_lnum is the lowest line number with changes since the last
 *		     update of b_syn_states[] (0 means no changes)
 */
    struct syn_state	*b_syn_states;
    int			b_syn_states_len;
    linenr_t		b_syn_states_lnum;
    linenr_t		b_syn_change_lnum;
#endif /* SYNTAX_HL */
};

/*
 * Structure which contains all information that belongs to a window
 *
 * All row numbers are relative to the start of the window, except w_winpos.
 */
struct window
{
    BUF		*w_buffer;	    /* buffer we are a window into */

    WIN		*w_prev;	    /* link to previous window (above) */
    WIN		*w_next;	    /* link to next window (below) */

    FPOS	w_cursor;	    /* cursor's position in buffer */

    /*
     * w_valid is a bitfield of flags, which indicate if specific values are
     * valid or need to be recomputed.	See screen.c for values.
     */
    int		w_valid;
    FPOS	w_valid_cursor;	    /* last known position of w_cursor, used
				       to adjust w_valid */
    colnr_t	w_valid_leftcol;    /* last known w_leftcol */

    /*
     * w_wrow and w_wcol are the cursor's position in the window.
     * This is related to character positions in the window, not in the file.
     * w_wrow is relative to w_winpos.
     */
    int		w_wrow, w_wcol;	    /* cursor's position in window */

    /*
     * w_cline_height is the number of physical lines taken by the buffer line
     * that the cursor is on.  We use this to avoid extra calls to plines().
     */
    int		w_cline_height;	    /* current size of cursor line */

    int		w_cline_row;	    /* starting row of the cursor line */

    colnr_t	w_virtcol;	    /* column number of the file's actual */
				    /* line, as opposed to the column */
				    /* number we're at on the screen. */
				    /* This makes a difference on lines */
				    /* which span more than one screen */
				    /* line or when w_leftcol is non-zero */

    colnr_t	w_curswant;	    /* The column we'd like to be at. */
				    /* This is used to try to stay in */
				    /* the same column through up/down */
				    /* cursor motions. */

    int		w_set_curswant;	    /* If set, then update w_curswant */
				    /* the next time through cursupdate() */
				    /* to the current virtual column */

    /*
     * the next three are used to update the visual part
     */
    linenr_t	w_old_cursor_lnum;  /* last known end of visual part */
    colnr_t	w_old_cursor_fcol;  /* first column for block visual part */
    colnr_t	w_old_cursor_lcol;  /* last column for block visual part */
    linenr_t	w_old_visual_lnum;  /* last known start of visual part */
    colnr_t	w_old_curswant;	    /* last known value of Curswant */

    linenr_t	w_topline;	    /* number of the line at the top of
				     * the screen */
    linenr_t	w_botline;	    /* number of the line below the bottom
				     * of the screen */
    int		w_empty_rows;	    /* number of ~ rows in window */

    int		w_winpos;	    /* row of topline of window in screen */
    int		w_height;	    /* number of rows in window, excluding
					status/command line */
    int		w_status_height;    /* number of status lines (0 or 1) */

    int		w_redr_status;	    /* if TRUE status line must be redrawn */
    int		w_redr_type;	    /* type of redraw to be performed on win */

    /* remember what is shown in the ruler for this window (if 'ruler' set) */
    FPOS	w_ru_cursor;	    /* cursor position shown in ruler */
    colnr_t	w_ru_virtcol;	    /* virtcol shown in ruler */
    linenr_t	w_ru_topline;	    /* topline shown in ruler */
    char	w_ru_empty;	    /* TRUE if ruler shows 0-1 (empty line) */

    colnr_t	w_leftcol;	    /* starting column of the screen when
				     * 'wrap' is off */
    colnr_t	w_skipcol;	    /* starting column when a single line
				       doesn't fit in the window */

/*
 * The height of the lines currently in the window is remembered
 * to avoid recomputing it every time. The number of entries is Rows.
 */
    int		w_lsize_valid;	    /* nr. of valid LineSizes */
    linenr_t	*w_lsize_lnum;	    /* array of line numbers for w_lsize */
    char_u	*w_lsize;	    /* array of line heights */

    int		w_alt_fnum;	    /* alternate file (for # and CTRL-^) */

    int		w_arg_idx;	    /* current index in argument list (can be
				     * out of range!) */
    int		w_arg_idx_invalid;  /* editing another file then w_arg_idx */

    /*
     * Variables "local" to a window.
     * They are here because they influence the layout of the window or
     * depend on the window layout.
     */
    int		w_p_list,	/* 'list' */
		w_p_nu,		/* 'number' */
#ifdef RIGHTLEFT
		w_p_rl,		/* 'rightleft' */
#ifdef FKMAP
		w_p_pers,	/* for the window dependent Farsi functions */
#endif
#endif
		w_p_wrap;	/* 'wrap' */
#ifdef LINEBREAK
    int		w_p_lbr;	/* 'linebreak' */
#endif
    long	w_p_scroll;	/* 'scroll' */

#ifdef SCROLLBIND
    int		w_p_scb;	/* 'scrollbind' */
    long	w_scbind_pos;
#endif
    int		w_preview;	/* Flag to indicate a preview window */

#ifdef WANT_EVAL
    struct growarray w_vars;	/* internal variables, local to window */
#endif

    /*
     * The w_prev_pcmark field is used to check whether we really did jump to
     * a new line after setting the w_pcmark.  If not, then we revert to
     * using the previous w_pcmark.
     */
    FPOS	w_pcmark;	    /* previous context mark */
    FPOS	w_prev_pcmark;	    /* previous w_pcmark */

    /*
     * the jumplist contains old cursor positions
     */
    struct filemark w_jumplist[JUMPLISTSIZE];
    int		    w_jumplistlen;		/* number of active entries */
    int		    w_jumplistidx;		/* current position */

    /*
     * the tagstack grows from 0 upwards:
     * entry 0: older
     * entry 1: newer
     * entry 2: newest
     */
    struct taggy    w_tagstack[TAGSTACKSIZE];	/* the tag stack */
    int		    w_tagstackidx;		/* idx just below activ entry */
    int		    w_tagstacklen;		/* number of tags on stack */

    /*
     * w_fraction is the fractional row of the cursor within the window, from
     * 0 at the top row to FRACTION_MULT at the last row.
     * w_prev_fraction_row was the actual cursor row when w_fraction was last
     * calculated.
     */
    int		    w_fraction;
    int		    w_prev_fraction_row;

#ifdef USE_GUI
    GuiScrollbar    w_scrollbars[2];		/* Scrollbars for this window */
#endif /* USE_GUI */

#ifdef HAVE_PERL_INTERP
    void	    *perl_private;
#endif

#ifdef HAVE_PYTHON
    void	    *python_ref;    /* The Python value referring to
				     * this window */
#endif

#ifdef HAVE_TCL
    void	    *tcl_ref;
#endif
};

/*
 * Arguments for operators.
 */
typedef struct oparg
{
    int	    op_type;		/* current pending operator type */
    int	    regname;		/* register to use for the operator */
    int	    motion_type;	/* type of the current cursor motion */
    int	    inclusive;		/* TRUE if char motion is inclusive (only
				   valid when motion_type is MCHAR */
    int	    end_adjusted;	/* backuped b_op_end one char (only used by
				   do_format()) */
    FPOS    start;		/* start of the operator */
    FPOS    end;		/* end of the operator */
    long    line_count;		/* number of lines from op_start to op_end
				   (inclusive) */
    int	    empty;		/* op_start and op_end the same (only used by
				   do_change()) */
    int	    is_VIsual;		/* operator on Visual area */
    int	    block_mode;		/* current operator is Visual block mode */
    colnr_t start_vcol;		/* start col for block mode operator */
    colnr_t end_vcol;		/* end col for block mode operator */
} OPARG;

/*
 * Arguments for Normal mode commands.
 */
typedef struct cmdarg
{
    OPARG   *oap;		/* Operator arguments */
    int	    prechar;		/* prefix character (optional, always 'g') */
    int	    cmdchar;		/* command character */
    int	    nchar;		/* next character (optional) */
    int	    extra_char;		/* yet another character (optional) */
    long    count0;		/* count, default 0 */
    long    count1;		/* count, default 1 */
} CMDARG;

#ifdef CURSOR_SHAPE
/*
 * struct to store values from 'guicursor'
 */
#define SHAPE_N		0	/* Normal mode */
#define SHAPE_V		1	/* Visual mode */
#define SHAPE_I		2	/* Insert mode */
#define SHAPE_R		3	/* Replace mode */
#define SHAPE_C		4	/* Command line Normal mode */
#define SHAPE_CI	5	/* Command line Insert mode */
#define SHAPE_CR	6	/* Command line Replace mode */
#define SHAPE_SM	7	/* showing matching paren */
#define SHAPE_O		8	/* Operator-pending mode */
#define SHAPE_VE	9	/* Visual mode with 'seleciton' exclusive */
#define SHAPE_COUNT	10

#define SHAPE_BLOCK	0	/* block cursor */
#define SHAPE_HOR	1	/* horizontal bar cursor */
#define SHAPE_VER	2	/* vertical bar cursor */

struct cursor_entry
{
    int	    shape;		/* one of the SHAPE_ defines */
    int	    percentage;		/* percentage of cell for bar */
    long    blinkwait;		/* blinking, wait time before blinking starts */
    long    blinkon;		/* blinking, on time */
    long    blinkoff;		/* blinking, off time */
    int	    id;			/* highlight group ID */
    char    *name;		/* mode name (fixed) */
};
#endif /* CURSOR_SHAPE */

#ifdef WANT_MENU

/* Indices into VimMenu->strings[] and VimMenu->noremap[] for each mode */
#define MENU_INDEX_INVALID	-1
#define MENU_INDEX_NORMAL	0
#define MENU_INDEX_VISUAL	1
#define MENU_INDEX_OP_PENDING	2
#define MENU_INDEX_INSERT	3
#define MENU_INDEX_CMDLINE	4
#define MENU_INDEX_TIP		5
#define MENU_MODES		6

/* Menu modes */
#define MENU_NORMAL_MODE	(1 << MENU_INDEX_NORMAL)
#define MENU_VISUAL_MODE	(1 << MENU_INDEX_VISUAL)
#define MENU_OP_PENDING_MODE	(1 << MENU_INDEX_OP_PENDING)
#define MENU_INSERT_MODE	(1 << MENU_INDEX_INSERT)
#define MENU_CMDLINE_MODE	(1 << MENU_INDEX_CMDLINE)
#define MENU_TIP_MODE		(1 << MENU_INDEX_TIP)
#define MENU_ALL_MODES		((1 << MENU_INDEX_TIP) - 1)
/*note MENU_INDEX_TIP is not a 'real' mode*/

/* Start a menu name with this to not include it on the main menu bar */
#define MNU_HIDDEN_CHAR		']'

typedef struct VimMenu
{
    int		modes;		    /* Which modes is this menu visible for? */
    char_u	*name;		    /* Name of menu */
    char_u	*dname;		    /* Displayed Name (without '&') */
    int		mnemonic;	    /* mnemonic key (after '&') */
    char_u	*actext;	    /* accelerator text (after TAB) */
    int		priority;	    /* Menu order priority */
#ifdef USE_GUI
    void	(*cb)();	    /* Call-back routine */
#endif
    char_u	*strings[MENU_MODES]; /* Mapped string for each mode */
    int		noremap[MENU_MODES]; /* A noremap flag for each mode */
    struct VimMenu *children;	    /* Children of sub-menu */
    struct VimMenu *next;	    /* Next item in menu */
#ifdef USE_GUI_X11
    Widget	id;		    /* Manage this to enable item */
    Widget	submenu_id;	    /* If this is submenu, add children here */
#endif
#ifdef USE_GUI_GTK
    GtkWidget	*id;		    /* Manage this to enable item */
    GtkWidget	*submenu_id;	    /* If this is submenu, add children here */
    GtkWidget	*tearoff_handle;
    struct VimMenu *parent;	    /* Parent of menu (needed for tearoffs) */
#endif
#ifdef USE_GUI_WIN16
    UINT	id;		    /* Id of menu item */
    HMENU	submenu_id;	    /* If this is submenu, add children here */
    struct VimMenu *parent;	    /* Parent of menu */
#endif
#ifdef USE_GUI_WIN32
    UINT	id;		    /* Id of menu item */
    HMENU	submenu_id;	    /* If this is submenu, add children here */
    HWND	tearoff_handle;	    /* hWnd of tearoff if created */
    struct VimMenu *parent;	    /* Parent of menu (needed for tearoffs) */
#endif
#if USE_GUI_BEOS
    BMenuItem	*id;		    /* Id of menu item */
    BMenu	*submenu_id;	    /* If this is submenu, add children here */
#endif
#ifdef macintosh
    MenuHandle	id;
    short	index;		    /* the item index within the father menu */
    short	menu_id;	    /* the menu id to which this item belong */
    short	submenu_id;	    /* the menu id of the children (could be
				       get throught some tricks) */
    MenuHandle	menu_handle;
    MenuHandle	submenu_handle;
#endif
#if defined(USE_GUI_AMIGA)
				    /* only one of these will ever be set, but
				     * they are used to allow the menu routine
				     * to easily get a hold of the parent menu
				     * pointer which is needed by all items to
				     * form the chain correctly */
    int		    id;		    /* unused by the amiga, but used in the
				     * code kept for compatibility */
    struct Menu	    *menuPtr;
    struct MenuItem *menuItemPtr;
#endif
#ifdef RISCOS
    int		*id;		    /* Not used, but gui.c needs it */
    int		greyed_out;	    /* Flag */
    int		hidden;
#endif
} VimMenu;
#else
typedef int VimMenu;

#endif /* WANT_MENU */
