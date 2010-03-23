/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * macros.h: macro definitions for often used code
 */

/*
 * pchar(lp, c) - put character 'c' at position 'lp'
 */
#define pchar(lp, c) (*(ml_get_buf(curbuf, (lp).lnum, TRUE) + (lp).col) = (c))

/*
 * Position comparisons
 */
#define lt(a, b) (((a).lnum != (b).lnum) \
		   ? ((a).lnum < (b).lnum) : ((a).col < (b).col))

#define ltoreq(a, b) (((a).lnum != (b).lnum) \
		   ? ((a).lnum < (b).lnum) : ((a).col <= (b).col))

#define equal(a, b) (((a).lnum == (b).lnum) && ((a).col == (b).col))

/*
 * lineempty() - return TRUE if the line is empty
 */
#define lineempty(p) (*ml_get(p) == NUL)

/*
 * bufempty() - return TRUE if the current buffer is empty
 */
#define bufempty() (curbuf->b_ml.ml_line_count == 1 && *ml_get((linenr_t)1) == NUL)

/*
 * On some systems toupper()/tolower() only work on lower/uppercase characters
 * Careful: Only call TO_UPPER() and TO_LOWER() with a character in the range
 * 0 - 255.  toupper()/tolower() on some systems can't handle others.
 */
#ifdef MSWIN
#  define TO_UPPER(c)	toupper_tab[(c) & 255]
#  define TO_LOWER(c)	tolower_tab[(c) & 255]
#else
# ifdef BROKEN_TOUPPER
#  define TO_UPPER(c)	(islower(c) ? toupper(c) : (c))
#  define TO_LOWER(c)	(isupper(c) ? tolower(c) : (c))
# else
#  define TO_UPPER	toupper
#  define TO_LOWER	tolower
# endif
#endif

/* macro version of chartab(), only works with 0-255 values */
#define CHARSIZE(c)	(chartab[c] & CHAR_MASK);

#ifdef HAVE_LANGMAP
/*
 * Adjust chars in a language according to 'langmap' option.
 * NOTE that there is NO overhead if 'langmap' is not set; but even
 * when set we only have to do 2 ifs and an array lookup.
 * Don't apply 'langmap' if the character comes from the Stuff buffer.
 * The do-while is just to ignore a ';' after the macro.
 */
# define LANGMAP_ADJUST(c, condition) do { \
	if (*p_langmap && (condition) && !KeyStuffed && (c) < 256) \
	    c = langmap_mapchar[c]; \
    } while (0)
#endif

/*
 * vim_isbreak() is used very often if 'linebreak' is set, use a macro to make
 * it work fast.
 */
#define vim_isbreak(c) (breakat_flags[(char_u)(c)])

/*
 * On VMS file names are different and require a translation.
 * On the Mac open() has only two arguments.
 */
#ifdef VMS
# define mch_access(n, p) access(vms_fixfilename(n), (p))
# define mch_fopen(n, p)  fopen(vms_fixfilename(n), (p))
# define mch_fstat(n, p)  fstat(vms_fixfilename(n), (p))
# define mch_lstat(n, p)  lstat(vms_fixfilename(n), (p))
# define mch_stat(n, p)   stat(vms_fixfilename(n), (p))
#else
# define mch_access(n, p) access((n), (p))
# define mch_fopen(n, p)  fopen((n), (p))
# define mch_fstat(n, p)  fstat((n), (p))
# define mch_lstat(n, p)  lstat((n), (p))
# define mch_stat(n, p)   stat((n), (p))
#endif

#ifdef macintosh
# define mch_open(n, m, p) open((n), (m))
#else
# ifdef VMS
#  define mch_open(n, m, p) open(vms_fixfilename(n), (m), (p))
# else
#  define mch_open(n, m, p) open((n), (m), (p))
# endif
#endif

/*
 * Encryption macros.  Mohsin Ahmed, mosh@sasi.com 98-09-24
 * Based on zip/crypt sources.
 */

#ifdef CRYPTV

#ifndef __MINGW32__
# define PWLEN 80
#endif

/* encode byte c, using temp t.  Warning: c must not have side effects. */
# define zencode(c, t)  (t = decrypt_byte(), update_keys(c), t^(c))

/* decode byte c in place */
# define zdecode(c)   update_keys(c ^= decrypt_byte())

#endif
