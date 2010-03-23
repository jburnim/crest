/* vi:set ts=8 sts=4 sw=4:
 *
 * Handling of regular expressions: vim_regcomp(), vim_regexec(), vim_regsub()
 *
 * NOTICE:
 *
 * This is NOT the original regular expression code as written by Henry
 * Spencer.  This code has been modified specifically for use with the VIM
 * editor, and should not be used apart from compiling VIM.  If you want a
 * good regular expression library, get the original code.  The copyright
 * notice that follows is from the original.
 *
 * END NOTICE
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.
 *
 * Changes have been made by Tony Andrews, Olaf 'Rhialto' Seibert, Robert Webb
 * and Bram Moolenaar.
 * Named character class support added by Walter Briscoe (1998 Jul 01)
 */

#include "vim.h"

#undef DEBUG

#include <stdio.h>
#include "option.h"

/*
 * Get around a problem with #defined char class functions.
 */
#ifdef isalnum
static int myisalnum __ARGS((int c));
static int myisalnum(c) int c; { return isalnum(c); }
# undef isalnum
# define isalnum myisalnum
#endif
#ifdef isalpha
static int myisalpha __ARGS((int c));
static int myisalpha(c) int c; { return isalpha(c); }
# undef isalpha
# define isalpha myisalpha
#endif
#ifdef iscntrl
static int myiscntrl __ARGS((int c));
static int myiscntrl(c) int c; { return iscntrl(c); }
# undef iscntrl
# define iscntrl myiscntrl
#endif
#ifdef isdigit
static int myisdigit __ARGS((int c));
static int myisdigit(c) int c; { return isdigit(c); }
# undef isdigit
# define isdigit myisdigit
#endif
# ifdef isgraph
static int myisgraph __ARGS((int c));
static int myisgraph(c) int c; { return isgraph(c); }
# undef isgraph
# define isgraph myisgraph
#endif
#ifdef islower
static int myislower __ARGS((int c));
static int myislower(c) int c; { return islower(c); }
# undef islower
# define islower myislower
#endif
#ifdef ispunct
static int myispunct __ARGS((int c));
static int myispunct(c) int c; { return ispunct(c); }
# undef ispunct
# define ispunct myispunct
#endif
#ifdef isupper
static int myisupper __ARGS((int c));
static int myisupper(c) int c; { return isupper(c); }
# undef isupper
# define isupper myisupper
#endif
#ifdef isxdigit
static int myisxdigit __ARGS((int c));
static int myisxdigit(c) int c; { return isxdigit(c); }
# undef isxdigit
# define isxdigit myisxdigit
#endif

/*
 * The "internal use only" fields in regexp.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * regstart	char that must begin a match; '\0' if none obvious
 * reganch	is the match anchored (at beginning-of-line only)?
 * regmust	string (pointer into program) that match must include, or NULL
 * regmlen	length of regmust string
 *
 * Regstart and reganch permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  Regmust permits fast rejection
 * of lines that cannot possibly match.  The regmust tests are costly enough
 * that vim_regcomp() supplies a regmust only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  Regmlen is
 * supplied because the test in vim_regexec() needs it and vim_regcomp() is
 * computing it anyway.
 */

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH and BRACES_COMPLEX implement concatenation; a "next"
 * pointer with a BRANCH on both ends of it is connecting two alternatives.
 * (Here we have one of the subtle syntax dependencies:	an individual BRANCH
 * (as opposed to a collection of them) is never concatenated with anything
 * because of operator precedence).  The "next" pointer of a BRACES_COMPLEX
 * node points to the node after the stuff to be repeated.  The operand of some
 * types of node is a literal string; for others, it is a node leading into a
 * sub-FSM.  In particular, the operand of a BRANCH node is the first node of
 * the branch.	(NB this is *not* a tree structure: the tail of the branch
 * connects to the thing following the set of BRANCHes.)  The opcodes are:
 */

/* definition	number		   opnd?    meaning */
#define END		0	/* no	End of program. */
#define BOL		1	/* no	Match "" at beginning of line. */
#define EOL		2	/* no	Match "" at end of line. */
#define ANY		3	/* no	Match any one character. */
#define ANYOF		4	/* str	Match any character in this string. */
#define ANYBUT		5	/* str	Match any character not in this
				 *	string. */
#define BRANCH		6	/* node Match this alternative, or the
				 *	next... */
#define BACK		7	/* no	Match "", "next" ptr points backward. */
#define EXACTLY		8	/* str	Match this string. */
#define NOTHING		9	/* no	Match empty string. */
#define STAR		10	/* node Match this (simple) thing 0 or more
				 *	times. */
#define PLUS		11	/* node Match this (simple) thing 1 or more
				 *	times. */
#define BRACE_SIMPLE	12	/* node Match this (simple) thing between m and
				 *	n times (\{m,n\}). */
#define BOW		13	/* no	Match "" after [^a-zA-Z0-9_] */
#define EOW		14	/* no	Match "" at    [^a-zA-Z0-9_] */
#define IDENT		15	/* no	Match identifier char */
#define KWORD		16	/* no	Match keyword char */
#define FNAME		17	/* no	Match file name char */
#define PRINT		18	/* no	Match printable char */
#define SIDENT		19	/* no	Match identifier char but no digit */
#define SWORD		20	/* no	Match word char but no digit */
#define SFNAME		21	/* no	Match file name char but no digit */
#define SPRINT		22	/* no	Match printable char but no digit */
#define BRACE_LIMITS	23	/* 2int	define the min & max for BRACE_SIMPLE
				 *	and BRACE_COMPLEX. */
#define WHITE		24	/* no	Match whitespace char */
#define NWHITE		25	/* no	Match non-whitespace char */
#define DIGIT		26	/* no	Match digit char */
#define NDIGIT		27	/* no	Match non-digit char */
#define HEX		28	/* no   Match hex char */
#define NHEX		29	/* no   Match non-hex char */
#define OCTAL		30	/* no	Match octal char */
#define NOCTAL		31	/* no	Match non-octal char */
#define WORD		32	/* no	Match word char */
#define NWORD		33	/* no	Match non-word char */
#define HEAD		34	/* no	Match head char */
#define NHEAD		35	/* no	Match non-head char */
#define ALPHA		36	/* no	Match alpha char */
#define NALPHA		37	/* no	Match non-alpha char */
#define LOWER		38	/* no	Match lowercase char */
#define NLOWER		39	/* no	Match non-lowercase char */
#define UPPER		40	/* no	Match uppercase char */
#define NUPPER		41	/* no	Match non-uppercase char */
#define MOPEN		60 /* -69  no	Mark this point in input as start of
				 *	\( subexpr. */
#define MCLOSE		70 /* -79  no	Analogous to MOPEN. */
#define BACKREF		80 /* -89  node Match same string again \1-\9 */
#define BRACE_COMPLEX	90 /* -99  node Match nodes between m & n times */

#define Magic(x)    ((x) | ('\\' << 8))

/*
 * Opcode notes:
 *
 * BRANCH	The set of branches constituting a single choice are hooked
 *		together with their "next" pointers, since precedence prevents
 *		anything being concatenated to any individual branch.  The
 *		"next" pointer of the last BRANCH in a choice points to the
 *		thing following the whole choice.  This is also where the
 *		final "next" pointer of each individual branch points; each
 *		branch starts with the operand node of a BRANCH node.
 *
 * BACK		Normal "next" pointers all implicitly point forward; BACK
 *		exists to make loop structures possible.
 *
 * STAR,PLUS	'=', and complex '*' and '+', are implemented as circular
 *		BRANCH structures using BACK.  Simple cases (one character
 *		per match) are implemented with STAR and PLUS for speed
 *		and to minimize recursive plunges.
 *		Note: We would like to use "\?" instead of "\=", but a "\?"
 *		can be part of a pattern to escape the special meaning of '?'
 *		at the end of the pattern in "?pattern?e".
 *
 * BRACE_LIMITS	This is always followed by a BRACE_SIMPLE or BRACE_COMPLEX
 *		node, and defines the min and max limits to be used for that
 *		node.
 *
 * MOPEN,MCLOSE	...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
#define OP(p)		((int)*(p))
#define NEXT(p)		(((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
#define OPERAND(p)	((p) + 3)
#define OPERAND_MIN(p)	(((*((p)+3)&0377)<<8) + (*((p)+4)&0377))
#define OPERAND_MAX(p)	(((*((p)+5)&0377)<<8) + (*((p)+6)&0377))

/*
 * Utility definitions.
 */
#define UCHARAT(p)	((int)*(unsigned char *)(p))

/* Used for an error (down from) vim_regcomp(): give the error message, set
 * rc_did_emsg and return NULL */
#define EMSG_RET_NULL(m) { EMSG(m); rc_did_emsg = TRUE; return NULL; }
#define EMSG_RET_FAIL(m) { EMSG(m); rc_did_emsg = TRUE; return FAIL; }

#define MAX_LIMIT	32767

static int re_ismult __ARGS((int));
static int cstrncmp __ARGS((char_u *s1, char_u *s2, int n));
static char_u *cstrchr __ARGS((char_u *, int));

#ifdef DEBUG
static void	regdump __ARGS((char_u *, vim_regexp *));
static char_u	*regprop __ARGS((char_u *));
#endif

    static int
re_ismult(c)
    int c;
{
    return (c == Magic('*') || c == Magic('+') || c == Magic('=') ||
	    c == Magic('{'));
}

/*
 * Flags to be passed up and down.
 */
#define HASWIDTH	01	/* Known never to match null string. */
#define SIMPLE		02	/* Simple enough to be STAR/PLUS operand. */
#define SPSTART		04	/* Starts with * or +. */
#define WORST		0	/* Worst case. */

/*
 * When regcode is set to this value, code is not emitted and size is computed
 * instead.
 */
#define JUST_CALC_SIZE	((char_u *) -1)

static char_u		*reg_prev_sub;

/*
 * REGEXP_INRANGE contains all characters which are always special in a []
 * range after '\'.
 * REGEXP_ABBR contains all characters which act as abbreviations after '\'.
 * These are:
 *  \r	- New line (CR).
 *  \t	- Tab (TAB).
 *  \e	- Escape (ESC).
 *  \b	- Backspace (Ctrl('H')).
 */
static char_u REGEXP_INRANGE[] = "]^-\\";
static char_u REGEXP_ABBR[] = "rteb";

static int	backslash_trans __ARGS((int c));
static int	my_isblank __ARGS((int c));
static int	my_istab __ARGS((int c));
static int	my_isbspace __ARGS((int c));
static int	my_isreturn __ARGS((int c));
static int	my_isesc __ARGS((int c));
static int	(* skip_class_name __ARGS((char_u **pp)))__ARGS((int));
static char_u * skip_range __ARGS((char_u *p));
static void	init_class_tab __ARGS((void));

    static int
backslash_trans(c)
    int	    c;
{
    switch (c)
    {
	case 'r':   return CR;
	case 't':   return TAB;
	case 'e':   return ESC;
	case 'b':   return BS;
    }
    return c;
}

/*
 * Function version of the macro vim_iswhite().
 */
    static int
my_isblank(c)
    int		c;
{
    return vim_iswhite(c);
}

/*
 * Simplistic functions to recognize a single character.  It's a bit slow...
 */
static int my_istab(c) int c; { return c == TAB; }
static int my_isbspace(c) int c; { return c == BS; }
static int my_isreturn(c) int c; { return c == CR; }
static int my_isesc(c) int c; { return c == ESC; }

/*
 * Check for a character class name.  "pp" is at the '['.
 * If not: NULL is returned; If so, a function of the sort is* is returned and
 * the name is skipped.
 */
#if defined(macintosh) || defined(__BEOS__)
/* the compiler doesn't understand the other one */
    static int (*
skip_class_name(char_u **pp))__ARGS((int))
#else
    static int (*
skip_class_name(pp))__ARGS((int))
    char_u	**pp;
#endif
{
    typedef struct
    {
	size_t	    len;
	int	    (*func)__ARGS((int));
	char_u	    name[sizeof("backspace:]")];
    } namedata_t;

#define t(n, func) { sizeof(n) - 1, func, n }
    static const namedata_t class_names[] =
    {
	t("alnum:]", isalnum),		t("alpha:]", isalpha),
	t("blank:]", my_isblank),	t("cntrl:]", iscntrl),
	t("digit:]", isdigit),		t("graph:]", isgraph),
	t("lower:]", islower),		t("print:]", vim_isprintc),
	t("punct:]", ispunct),		t("space:]", vim_isspace),
	t("upper:]", isupper),		t("xdigit:]", isxdigit),
	t("tab:]",   my_istab),		t("return:]", my_isreturn),
	t("backspace:]", my_isbspace),	t("escape:]", my_isesc)
    };
#undef t

    const namedata_t *np;

    if ((*pp)[1] != ':')
	return NULL;
    for (   np = class_names;
	    np < class_names + sizeof(class_names) / sizeof(*class_names);
	    np++)
	if (STRNCMP(*pp + 2, np->name, np->len) == 0)
	{
	    *pp += np->len + 2;
	    return np->func;
	}
    return NULL;
}

/*
 * Skip over a "[]" range.
 * "p" must point to the character after the '['.
 * The returned pointer is on the matching ']', or the terminating NUL.
 */
    static char_u *
skip_range(p)
    char_u	*p;
{
    int		    cpo_lit;	    /* 'cpoptions' contains 'l' flag */

    cpo_lit = (!reg_syn && vim_strchr(p_cpo, CPO_LITERAL) != NULL);

    if (*p == '^')	/* Complement of range. */
	++p;
    if (*p == ']' || *p == '-')
	++p;
    while (*p != NUL && *p != ']')
    {
	if (*p == '-')
	{
	    ++p;
	    if (*p != ']' && *p != NUL)
		++p;
	}
	else if (*p == '\\'
		&& (vim_strchr(REGEXP_INRANGE, p[1]) != NULL
		    || (!cpo_lit && vim_strchr(REGEXP_ABBR, p[1]) != NULL)))
	    p += 2;
	else if (*p == '[')
	{
	    if (skip_class_name(&p) == NULL)
		++p; /* It was not a class name */
	}
	else
	    ++p;
    }

    return p;
}

/*
 * Specific version of character class functions.
 * Using a table to keep this fast.
 */
static char_u	class_tab[256];

#define	    RI_DIGIT	0x01
#define	    RI_HEX	0x02
#define	    RI_OCTAL	0x04
#define	    RI_WORD	0x08
#define	    RI_HEAD	0x10
#define	    RI_ALPHA	0x20
#define	    RI_LOWER	0x40
#define	    RI_UPPER	0x80

    static void
init_class_tab()
{
    int		i;
    static int	done = FALSE;

    if (done)
	return;

    for (i = 0; i < 256; ++i)
    {
	if (i >= '0' && i <= '7')
	    class_tab[i] = RI_DIGIT + RI_HEX + RI_OCTAL + RI_WORD;
	else if (i >= '8' && i <= '9')
	    class_tab[i] = RI_DIGIT + RI_HEX + RI_WORD;
	else if (i >= 'a' && i <= 'f')
	    class_tab[i] = RI_HEX + RI_WORD + RI_HEAD + RI_ALPHA + RI_LOWER;
	else if (i >= 'g' && i <= 'z')
	    class_tab[i] = RI_WORD + RI_HEAD + RI_ALPHA + RI_LOWER;
	else if (i >= 'A' && i <= 'F')
	    class_tab[i] = RI_HEX + RI_WORD + RI_HEAD + RI_ALPHA + RI_UPPER;
	else if (i >= 'G' && i <= 'Z')
	    class_tab[i] = RI_WORD + RI_HEAD + RI_ALPHA + RI_UPPER;
	else if (i == '_')
	    class_tab[i] = RI_WORD + RI_HEAD;
    }
    done = TRUE;
}

#define ri_digit(c)	(class_tab[c] & RI_DIGIT)
#define ri_hex(c)	(class_tab[c] & RI_HEX)
#define ri_octal(c)	(class_tab[c] & RI_OCTAL)
#define ri_word(c)	(class_tab[c] & RI_WORD)
#define ri_head(c)	(class_tab[c] & RI_HEAD)
#define ri_alpha(c)	(class_tab[c] & RI_ALPHA)
#define ri_lower(c)	(class_tab[c] & RI_LOWER)
#define ri_upper(c)	(class_tab[c] & RI_UPPER)

/*
 * Global work variables for vim_regcomp().
 */

static char_u	*regparse;  /* Input-scan pointer. */
static int	num_complex_braces; /* Complex \{...} count */
static int	regnpar;	/* () count. */
static char_u	*regcode;	/* Code-emit pointer, or JUST_CALC_SIZE */
static long	regsize;	/* Code size. */
static char_u	**regendp;	/* Pointer to endp array */
static int	brace_min[10];	/* Minimums for complex brace repeats */
static int	brace_max[10];	/* Maximums for complex brace repeats */
static int	brace_count[10]; /* Current counts for complex brace repeats */
#if defined(SYNTAX_HL) || defined(PROTO)
static int	had_eol;	/* TRUE when EOL found by vim_regcomp() */
#endif
static int	reg_magic;	/* p_magic passed to vim_regexec() */

/*
 * META contains all characters that may be magic, except '^' and '$'.
 */

static char_u META[] = ".[()|=+*<>iIkKfFpPsSdDxXoOwWhHaAlLuU~123456789{";

/*
 * Forward declarations for vim_regcomp()'s friends.
 */
static void	initchr __ARGS((char_u *));
static int	getchr __ARGS((void));
static int	peekchr __ARGS((void));
#define PeekChr() curchr    /* shortcut only when last action was peekchr() */
static int	curchr;
static void	skipchr __ARGS((void));
static void	ungetchr __ARGS((void));
static char_u	*reg __ARGS((int, int *));
static char_u	*regbranch __ARGS((int *));
static char_u	*regpiece __ARGS((int *));
static char_u	*regatom __ARGS((int *));
static char_u	*regnode __ARGS((int));
static char_u	*regnext __ARGS((char_u *));
static void	regc __ARGS((int));
static void	unregc __ARGS((void));
static void	reginsert __ARGS((int, char_u *));
static void	reginsert_limits __ARGS((int, int, int, char_u *));
static int	read_limits __ARGS((int, int, int *, int *));
static void	regtail __ARGS((char_u *, char_u *));
static void	regoptail __ARGS((char_u *, char_u *));

/*
 * Skip past regular expression.
 * Stop at end of 'p' of where 'dirc' is found ('/', '?', etc).
 * Take care of characters with a backslash in front of it.
 * Skip strings inside [ and ].
 */
    char_u *
skip_regexp(p, dirc, magic)
    char_u  *p;
    int	    dirc;
    int	    magic;
{
    for (; p[0] != NUL; ++p)
    {
	if (p[0] == dirc)	/* found end of regexp */
	    break;
	if ((p[0] == '[' && magic) || (p[0] == '\\' && p[1] == '[' && !magic))
	{
	    p = skip_range(p + 1);
	    if (p[0] == NUL)
		break;
	}
	else if (p[0] == '\\' && p[1] != NUL)
	    ++p;    /* skip next character */
    }
    return p;
}

/*
 * vim_regcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because vim_free() must be able to free it all.)
 *
 * Does not use reg_ic, see vim_regexec() for that.
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */
    vim_regexp *
vim_regcomp(expr, magic)
    char_u	*expr;
    int		magic;
{
    vim_regexp	*r;
    char_u	*scan;
    char_u	*longest;
    int		len;
    int		flags;

    if (expr == NULL)
	EMSG_RET_NULL(e_null);

    reg_magic = magic;
    init_class_tab();

    /* First pass: determine size, legality. */
    initchr((char_u *)expr);
    num_complex_braces = 0;
    regnpar = 1;
    regsize = 0L;
    regcode = JUST_CALC_SIZE;
    regendp = NULL;
#if defined(SYNTAX_HL) || defined(PROTO)
    had_eol = FALSE;
#endif
    regc(MAGIC);
    if (reg(0, &flags) == NULL)
	return NULL;

    /* Small enough for pointer-storage convention? */
#ifdef SMALL_MALLOC		/* 16 bit storage allocation */
    if (regsize >= 65536L - 256L)
	EMSG_RET_NULL(e_toolong);
#endif

    /* Allocate space. */
    r = (vim_regexp *)lalloc(sizeof(vim_regexp) + regsize, TRUE);
    if (r == NULL)
	return NULL;

    /* Second pass: emit code. */
    initchr((char_u *)expr);
    num_complex_braces = 0;
    regnpar = 1;
    regcode = r->program;
    regendp = r->endp;
    regc(MAGIC);
    if (reg(0, &flags) == NULL)
    {
	vim_free(r);
	return NULL;
    }

    /* Dig out information for optimizations. */
    r->regstart = '\0';		/* Worst-case defaults. */
    r->reganch = 0;
    r->regmust = NULL;
    r->regmlen = 0;
    scan = r->program + 1;	/* First BRANCH. */
    if (OP(regnext(scan)) == END)   /* Only one top-level choice. */
    {
	scan = OPERAND(scan);

	/* Starting-point info. */
	if (OP(scan) == BOL)
	{
	    r->reganch++;
	    scan = regnext(scan);
	}
	if (OP(scan) == EXACTLY)
	    r->regstart = *OPERAND(scan);
	else if ((OP(scan) == BOW || OP(scan) == EOW)
		 && OP(regnext(scan)) == EXACTLY)
	    r->regstart = *OPERAND(regnext(scan));

	/*
	 * If there's something expensive in the r.e., find the longest
	 * literal string that must appear and make it the regmust.  Resolve
	 * ties in favor of later strings, since the regstart check works
	 * with the beginning of the r.e. and avoiding duplication
	 * strengthens checking.  Not a strong reason, but sufficient in the
	 * absence of others.
	 */
	/*
	 * When the r.e. starts with BOW, it is faster to look for a regmust
	 * first. Used a lot for "#" and "*" commands. (Added by mool).
	 */
	if (flags & SPSTART || OP(scan) == BOW || OP(scan) == EOW)
	{
	    longest = NULL;
	    len = 0;
	    for (; scan != NULL; scan = regnext(scan))
		if (OP(scan) == EXACTLY && STRLEN(OPERAND(scan)) >= (size_t)len)
		{
		    longest = OPERAND(scan);
		    len = STRLEN(OPERAND(scan));
		}
	    r->regmust = longest;
	    r->regmlen = len;
	}
    }
#ifdef DEBUG
    regdump(expr, r);
#endif
    return r;
}

#if defined(SYNTAX_HL) || defined(PROTO)
/*
 * Check if during the previous call to vim_regcomp the EOL item "$" has been
 * found.  This is messy, but it works fine.
 */
    int
vim_regcomp_had_eol()
{
    return had_eol;
}
#endif

/*
 * reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
    static char_u *
reg(paren, flagp)
    int		    paren;	/* Parenthesized? */
    int		    *flagp;
{
    char_u	*ret;
    char_u	*br;
    char_u	*ender;
    int		parno = 0;
    int		flags;

    *flagp = HASWIDTH;		/* Tentatively. */

    /* Make an MOPEN node, if parenthesized. */
    if (paren)
    {
	if (regnpar >= NSUBEXP)
	    EMSG_RET_NULL(e_toombra);
	parno = regnpar;
	regnpar++;
	ret = regnode(MOPEN + parno);
	if (regendp)
	    regendp[parno] = NULL;  /* haven't seen the close paren yet */
    }
    else
	ret = NULL;

    /* Pick up the branches, linking them together. */
    br = regbranch(&flags);
    if (br == NULL)
	return NULL;
    if (ret != NULL)
	regtail(ret, br);	/* MOPEN -> first. */
    else
	ret = br;
    if (!(flags & HASWIDTH))
	*flagp &= ~HASWIDTH;
    *flagp |= flags & SPSTART;
    while (peekchr() == Magic('|'))
    {
	skipchr();
	br = regbranch(&flags);
	if (br == NULL)
	    return NULL;
	regtail(ret, br);	/* BRANCH -> BRANCH. */
	if (!(flags & HASWIDTH))
	    *flagp &= ~HASWIDTH;
	*flagp |= flags & SPSTART;
    }

    /* Make a closing node, and hook it on the end. */
    ender = regnode((paren) ? MCLOSE + parno : END);
    regtail(ret, ender);

    /* Hook the tails of the branches to the closing node. */
    for (br = ret; br != NULL; br = regnext(br))
	regoptail(br, ender);

    /* Check for proper termination. */
    if (paren && getchr() != Magic(')'))
	EMSG_RET_NULL(e_toombra)
    else if (!paren && peekchr() != '\0')
    {
	if (PeekChr() == Magic(')'))
	    EMSG_RET_NULL(e_toomket)
	else
	    EMSG_RET_NULL(e_trailing)	/* "Can't happen". */
	/* NOTREACHED */
    }
    /*
     * Here we set the flag allowing back references to this set of
     * parentheses.
     */
    if (paren && regendp)
	regendp[parno] = ender;	/* have seen the close paren */
    return ret;
}

/*
 * regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
    static char_u    *
regbranch(flagp)
    int		    *flagp;
{
    char_u	    *ret;
    char_u	    *chain;
    char_u	    *latest;
    int		    flags;

    *flagp = WORST;		/* Tentatively. */

    ret = regnode(BRANCH);
    chain = NULL;
    while (peekchr() != '\0' && PeekChr() != Magic('|') &&
						      PeekChr() != Magic(')'))
    {
	latest = regpiece(&flags);
	if (latest == NULL)
	    return NULL;
	*flagp |= flags & HASWIDTH;
	if (chain == NULL)	/* First piece. */
	    *flagp |= flags & SPSTART;
	else
	    regtail(chain, latest);
	chain = latest;
    }
    if (chain == NULL)		/* Loop ran zero times. */
	(void) regnode(NOTHING);

    return ret;
}

/*
 * regpiece - something followed by possible [*+=]
 *
 * Note that the branching code sequences used for = and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
    static char_u *
regpiece(flagp)
    int		    *flagp;
{
    char_u	    *ret;
    int		    op;
    char_u	    *next;
    int		    flags;
    int		    minval;
    int		    maxval;

    ret = regatom(&flags);
    if (ret == NULL)
	return NULL;

    op = peekchr();
    if (!re_ismult(op))
    {
	*flagp = flags;
	return ret;
    }
    if (!(flags & HASWIDTH) && op != Magic('='))
	EMSG_RET_NULL("*, \\+, or \\{ operand could be empty");
    *flagp = (WORST | SPSTART);		    /* default flags */

    skipchr();
    if (op == Magic('*') && (flags & SIMPLE))
	reginsert(STAR, ret);
    else if (op == Magic('*'))
    {
	/* Emit x* as (x&|), where & means "self". */
	reginsert(BRANCH, ret); /* Either x */
	regoptail(ret, regnode(BACK));	/* and loop */
	regoptail(ret, ret);	/* back */
	regtail(ret, regnode(BRANCH));	/* or */
	regtail(ret, regnode(NOTHING)); /* null. */
    }
    else if (op == Magic('+') && (flags & SIMPLE))
    {
	reginsert(PLUS, ret);
	*flagp = (WORST | HASWIDTH);
    }
    else if (op == Magic('+'))
    {
	/* Emit x+ as x(&|), where & means "self". */
	next = regnode(BRANCH); /* Either */
	regtail(ret, next);
	regtail(regnode(BACK), ret);	/* loop back */
	regtail(next, regnode(BRANCH)); /* or */
	regtail(ret, regnode(NOTHING)); /* null. */
	*flagp = (WORST | HASWIDTH);
    }
    else if (op == Magic('='))
    {
	/* Emit x= as (x|) */
	reginsert(BRANCH, ret); /* Either x */
	regtail(ret, regnode(BRANCH));	/* or */
	next = regnode(NOTHING);/* null. */
	regtail(ret, next);
	regoptail(ret, next);
    }
    else if (op == Magic('{') && (flags & SIMPLE))
    {
	if (!read_limits('{', '}', &minval, &maxval))
	    return NULL;
	reginsert(BRACE_SIMPLE, ret);
	reginsert_limits(BRACE_LIMITS, minval, maxval, ret);
	if (minval > 0)
	    *flagp = (WORST | HASWIDTH);
    }
    else if (op == Magic('{'))
    {
	if (!read_limits('{', '}', &minval, &maxval))
	    return NULL;
	if (num_complex_braces >= 10)
	    EMSG_RET_NULL("Too many complex \\{...}s");
	reginsert(BRACE_COMPLEX + num_complex_braces, ret);
	regoptail(ret, regnode(BACK));
	regoptail(ret, ret);
	reginsert_limits(BRACE_LIMITS, minval, maxval, ret);
	if (minval > 0)
	    *flagp = (WORST | HASWIDTH);
	++num_complex_braces;
    }
    if (re_ismult(peekchr()))
	EMSG_RET_NULL("Nested *, \\=, \\+, or \\{");

    return ret;
}

/*
 * regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.
 */
    static char_u *
regatom(flagp)
    int		   *flagp;
{
    char_u	    *ret;
    int		    flags;
    int		    cpo_lit;	    /* 'cpoptions' contains 'l' flag */

    *flagp = WORST;		/* Tentatively. */
    cpo_lit = (!reg_syn && vim_strchr(p_cpo, CPO_LITERAL) != NULL);

    switch (getchr())
    {
      case Magic('^'):
	ret = regnode(BOL);
	break;
      case Magic('$'):
	ret = regnode(EOL);
#if defined(SYNTAX_HL) || defined(PROTO)
	had_eol = TRUE;
#endif
	break;
      case Magic('<'):
	ret = regnode(BOW);
	break;
      case Magic('>'):
	ret = regnode(EOW);
	break;
      case Magic('.'):
	ret = regnode(ANY);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('i'):
	ret = regnode(IDENT);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('k'):
	ret = regnode(KWORD);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('I'):
	ret = regnode(SIDENT);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('K'):
	ret = regnode(SWORD);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('f'):
	ret = regnode(FNAME);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('F'):
	ret = regnode(SFNAME);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('p'):
	ret = regnode(PRINT);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('P'):
	ret = regnode(SPRINT);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('s'):
	ret = regnode(WHITE);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('S'):
	ret = regnode(NWHITE);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('d'):
	ret = regnode(DIGIT);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('D'):
	ret = regnode(NDIGIT);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('x'):
	ret = regnode(HEX);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('X'):
	ret = regnode(NHEX);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('o'):
	ret = regnode(OCTAL);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('O'):
	ret = regnode(NOCTAL);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('w'):
	ret = regnode(WORD);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('W'):
	ret = regnode(NWORD);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('h'):
	ret = regnode(HEAD);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('H'):
	ret = regnode(NHEAD);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('a'):
	ret = regnode(ALPHA);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('A'):
	ret = regnode(NALPHA);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('l'):
	ret = regnode(LOWER);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('L'):
	ret = regnode(NLOWER);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('u'):
	ret = regnode(UPPER);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('U'):
	ret = regnode(NUPPER);
	*flagp |= HASWIDTH | SIMPLE;
	break;
      case Magic('('):
	ret = reg(1, &flags);
	if (ret == NULL)
	    return NULL;
	*flagp |= flags & (HASWIDTH | SPSTART);
	break;
      case '\0':
      case Magic('|'):
      case Magic(')'):
	EMSG_RET_NULL(e_internal)	    /* Supposed to be caught earlier. */
	/* NOTREACHED */
      case Magic('='):
	EMSG_RET_NULL("\\= follows nothing")
	/* NOTREACHED */
      case Magic('+'):
	EMSG_RET_NULL("\\+ follows nothing")
	/* NOTREACHED */
      case Magic('{'):
	EMSG_RET_NULL("\\{ follows nothing")
	/* NOTREACHED */
      case Magic('*'):
	if (reg_magic)
	    EMSG_RET_NULL("* follows nothing")
	else
	    EMSG_RET_NULL("\\* follows nothing")
	/* break; Not Reached */
      case Magic('~'):		/* previous substitute pattern */
	    if (reg_prev_sub)
	    {
		char_u	    *p;

		ret = regnode(EXACTLY);
		p = reg_prev_sub;
		while (*p)
		{
		    regc(*p++);
		}
		regc('\0');
		if (p - reg_prev_sub)
		{
		    *flagp |= HASWIDTH;
		    if ((p - reg_prev_sub) == 1)
			*flagp |= SIMPLE;
		}
	    }
	    else
		EMSG_RET_NULL(e_nopresub);
	    break;
      case Magic('1'):
      case Magic('2'):
      case Magic('3'):
      case Magic('4'):
      case Magic('5'):
      case Magic('6'):
      case Magic('7'):
      case Magic('8'):
      case Magic('9'):
	    {
	    int		    refnum;

	    ungetchr();
	    refnum = getchr() - Magic('0');
	    /*
	     * Check if the back reference is legal. We use the parentheses
	     * pointers to mark encountered close parentheses, but this
	     * is only available in the second pass. Checking opens is
	     * always possible.
	     * Should also check that we don't refer to something that
	     * is repeated (+*=): what instance of the repetition should
	     * we match? TODO.
	     */
	    if (refnum < regnpar &&
		(regendp == NULL || regendp[refnum] != NULL))
		ret = regnode(BACKREF + refnum);
	    else
		EMSG_RET_NULL("Illegal back reference");
	}
	break;
      case Magic('['):
	{
	    char_u	*p;

	    /*
	     * If there is no matching ']', we assume the '[' is a normal
	     * character. This makes ":help [" work.
	     */
	    p = skip_range(regparse);
	    if (*p == ']')	/* there is a matching ']' */
	    {
		/*
		 * In a character class, different parsing rules apply.
		 * Not even \ is special anymore, nothing is.
		 */
		if (*regparse == '^') {	    /* Complement of range. */
		    ret = regnode(ANYBUT);
		    regparse++;
		}
		else
		    ret = regnode(ANYOF);
		if (*regparse == ']' || *regparse == '-')
		    regc(*regparse++);
		while (*regparse != '\0' && *regparse != ']')
		{
		    if (*regparse == '-')
		    {
			regparse++;
			if (*regparse == ']' || *regparse == '\0')
			    regc('-');
			else
			{
			    int		cclass;
			    int		cclassend;

			    cclass = UCHARAT(regparse - 2) + 1;
			    cclassend = UCHARAT(regparse);
			    if (cclass > cclassend + 1)
				EMSG_RET_NULL(e_invrange);
			    for (; cclass <= cclassend; cclass++)
				regc(cclass);
			    regparse++;
			}
		    }
		    /*
		     * Only "\]", "\^", "\]" and "\\" are special in Vi.  Vim
		     * accepts "\t", "\e", etc., but only when the 'l' flag in
		     * 'cpoptions' is not included.
		     */
		    else if (*regparse == '\\' &&
			    (vim_strchr(REGEXP_INRANGE, regparse[1]) != NULL ||
			     (!cpo_lit &&
			       vim_strchr(REGEXP_ABBR, regparse[1]) != NULL)))
		    {
			regparse++;
			regc(backslash_trans(*regparse++));
		    }
		    else if (*regparse == '[')
		    {
			int (*func)__ARGS((int));
			int cu;

			if ((func = skip_class_name(&regparse)) == NULL)
			    regc(*regparse++);
			else
			    /* Characters assumed to be 8 bits */
			    for (cu = 0; cu <= 255; cu++)
				if ((*func)(cu))
				    regc(cu);
		    }
		    else
			regc(*regparse++);
		}
		regc('\0');
		if (*regparse != ']')
		    EMSG_RET_NULL(e_toomsbra);
		skipchr();	    /* let's be friends with the lexer again */
		*flagp |= HASWIDTH | SIMPLE;
		break;
	    }
	}
	/* FALLTHROUGH */

      default:
	{
	    int		    len;
	    int		    chr;

	    ungetchr();
	    len = 0;
	    ret = regnode(EXACTLY);
	    /*
	     * Always take at least one character, for '[' without matching
	     * ']'.
	     */
	    while ((chr = peekchr()) != '\0' && (chr < Magic(0) || len == 0))
	    {
		regc(chr);
		skipchr();
		len++;
	    }
#ifdef DEBUG
	    if (len == 0)
		 EMSG_RET_NULL("Unexpected magic character; check META.");
#endif
	    /*
	     * If there is a following *, \+ or \= we need the character
	     * in front of it as a single character operand
	     */
	    if (len > 1 && re_ismult(chr))
	    {
		unregc();	    /* Back off of *+= operand */
		ungetchr();	    /* and put it back for next time */
		--len;
	    }
	    regc('\0');
	    *flagp |= HASWIDTH;
	    if (len == 1)
		*flagp |= SIMPLE;
	}
	break;
    }

    return ret;
}

/*
 * regnode - emit a node
 */
    static char_u *		/* Location. */
regnode(op)
    int		op;
{
    char_u  *ret;
    char_u  *ptr;

    ret = regcode;
    if (ret == JUST_CALC_SIZE)
    {
	regsize += 3;
	return ret;
    }
    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';		/* Null "next" pointer. */
    *ptr++ = '\0';
    regcode = ptr;

    return ret;
}

/*
 * regc - emit (if appropriate) a byte of code
 */
    static void
regc(b)
    int		b;
{
    if (regcode != JUST_CALC_SIZE)
	*regcode++ = b;
    else
	regsize++;
}

/*
 * unregc - take back (if appropriate) a byte of code
 */
    static void
unregc()
{
    if (regcode != JUST_CALC_SIZE)
	regcode--;
    else
	regsize--;
}

/*
 * reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
    static void
reginsert(op, opnd)
    int		op;
    char_u     *opnd;
{
    char_u  *src;
    char_u  *dst;
    char_u  *place;

    if (regcode == JUST_CALC_SIZE)
    {
	regsize += 3;
	return;
    }
    src = regcode;
    regcode += 3;
    dst = regcode;
    while (src > opnd)
	*--dst = *--src;

    place = opnd;		/* Op node, where operand used to be. */
    *place++ = op;
    *place++ = '\0';
    *place = '\0';
}

/*
 * reginsert_limits - insert an operator in front of already-emitted operand.
 * The operator has the given limit values as operands.  Also set next pointer.
 *
 * Means relocating the operand.
 */
    static void
reginsert_limits(op, minval, maxval, opnd)
    int		op;
    int		minval;
    int		maxval;
    char_u     *opnd;
{
    char_u  *src;
    char_u  *dst;
    char_u  *place;

    if (regcode == JUST_CALC_SIZE)
    {
	regsize += 7;
	return;
    }
    src = regcode;
    regcode += 7;
    dst = regcode;
    while (src > opnd)
	*--dst = *--src;

    place = opnd;		/* Op node, where operand used to be. */
    *place++ = op;
    *place++ = '\0';
    *place++ = '\0';
    *place++ = (char_u) (((unsigned)minval >> 8) & 0377);
    *place++ = (char_u) (minval & 0377);
    *place++ = (char_u) (((unsigned)maxval >> 8) & 0377);
    *place++ = (char_u) (maxval & 0377);
    regtail(opnd, place);
}

/*
 * regtail - set the next-pointer at the end of a node chain
 */
    static void
regtail(p, val)
    char_u	   *p;
    char_u	   *val;
{
    char_u  *scan;
    char_u  *temp;
    int	    offset;

    if (p == JUST_CALC_SIZE)
	return;

    /* Find last node. */
    scan = p;
    for (;;)
    {
	temp = regnext(scan);
	if (temp == NULL)
	    break;
	scan = temp;
    }

    if (OP(scan) == BACK)
	offset = (int)(scan - val);
    else
	offset = (int)(val - scan);
    *(scan + 1) = (char_u) (((unsigned)offset >> 8) & 0377);
    *(scan + 2) = (char_u) (offset & 0377);
}

/*
 * regoptail - regtail on operand of first argument; nop if operandless
 */
    static void
regoptail(p, val)
    char_u	   *p;
    char_u	   *val;
{
    /* When op is neither BRANCH nor BRACE_COMPLEX0-9, it is "operandless" */
    if (p == NULL || p == JUST_CALC_SIZE ||
	    (OP(p) != BRANCH &&
	     (OP(p) < BRACE_COMPLEX || OP(p) > BRACE_COMPLEX + 9)))
	return;
    regtail(OPERAND(p), val);
}

/*
 * getchr() - get the next character from the pattern. We know about
 * magic and such, so therefore we need a lexical analyzer.
 */

/* static int	    curchr; */
static int	prevchr;
static int	nextchr;    /* used for ungetchr() */
/*
 * Note: prevchr is sometimes -1 when we are not at the start,
 * eg in /[ ^I]^ the pattern was never found even if it existed, because ^ was
 * taken to be magic -- webb
 */
static int	at_start;	/* True when on the first character */
static int	prev_at_start;  /* True when on the second character */

    static void
initchr(str)
    char_u *str;
{
    regparse = str;
    curchr = prevchr = nextchr = -1;
    at_start = TRUE;
    prev_at_start = FALSE;
}

    static int
peekchr()
{
    if (curchr < 0)
    {
	switch (curchr = regparse[0])
	{
	case '.':
    /*	case '+':*/
    /*	case '=':*/
	case '[':
	case '~':
	    if (reg_magic)
		curchr = Magic(curchr);
	    break;
	case '*':
	    /* * is not magic as the very first character, eg "?*ptr" and when
	     * after '^', eg "/^*ptr" */
	    if (reg_magic && !at_start
				 && !(prev_at_start && prevchr == Magic('^')))
		curchr = Magic('*');
	    break;
	case '^':
	    /* '^' is only magic as the very first character and if it's after
	     * either "\(" or "\|" */
	    if (at_start || prevchr == Magic('(') || prevchr == Magic('|'))
	    {
		curchr = Magic('^');
		at_start = TRUE;
		prev_at_start = FALSE;
	    }
	    break;
	case '$':
	    /* '$' is only magic as the very last char and if it's in front of
	     * either "\|" or "\)" */
	    if (regparse[1] == NUL || (regparse[1] == '\\'
			       && (regparse[2] == '|' || regparse[2] == ')')))
		curchr = Magic('$');
	    break;
	case '\\':
	    regparse++;
	    if (regparse[0] == NUL)
	    {
		curchr = '\\';	/* trailing '\' */
		--regparse;	/* there is no extra character to skip */
	    }
	    else if (vim_strchr(META, regparse[0]))
	    {
		/*
		 * META contains everything that may be magic sometimes, except
		 * ^ and $ ("\^" and "\$" are never magic).
		 * We now fetch the next character and toggle its magicness.
		 * Therefore, \ is so meta-magic that it is not in META.
		 */
		curchr = -1;
		prev_at_start = at_start;
		at_start = FALSE;	/* be able to say "/\*ptr" */
		peekchr();
		curchr ^= Magic(0);
	    }
	    else if (vim_strchr(REGEXP_ABBR, regparse[0]))
	    {
		/*
		 * Handle abbreviations, like "\t" for TAB -- webb
		 */
		curchr = backslash_trans(regparse[0]);
	    }
	    else
	    {
		/*
		 * Next character can never be (made) magic?
		 * Then backslashing it won't do anything.
		 */
		curchr = regparse[0];
	    }
	    break;
	}
    }

    return curchr;
}

    static void
skipchr()
{
    regparse++;
    prev_at_start = at_start;
    at_start = FALSE;
    prevchr = curchr;
    curchr = nextchr;	    /* use previously unget char, or -1 */
    nextchr = -1;
}

    static int
getchr()
{
    int chr;

    chr = peekchr();
    skipchr();

    return chr;
}

/*
 * put character back. Works only once!
 */
    static void
ungetchr()
{
    nextchr = curchr;
    curchr = prevchr;
    at_start = prev_at_start;
    prev_at_start = FALSE;
    /*
     * Backup regparse as well; not because we will use what it points at,
     * but because skipchr() will bump it again.
     */
    regparse--;
}

/*
 * read_limits - Read two integers to be taken as a minimum and maximum.
 * If the first character is '-', then the range is reversed.
 * Should end with 'end'.  If minval is missing, zero is default, if maxval is
 * missing, a very big number is the default.
 */
    static int
read_limits(start, end, minval, maxval)
    int	    start;
    int	    end;
    int	    *minval;
    int	    *maxval;
{
    int	    reverse = FALSE;
    char_u  *first_char;

    if (*regparse == '-')
    {
	/* Starts with '-', so reverse the range later */
	regparse++;
	reverse = TRUE;
    }
    first_char = regparse;
    *minval = getdigits(&regparse);
    if (*regparse == ',')	    /* There is a comma */
    {
	if (isdigit(*++regparse))
	    *maxval = getdigits(&regparse);
	else
	    *maxval = MAX_LIMIT;
    }
    else if (isdigit(*first_char))
	*maxval = *minval;	    /* It was \{n} or \{-n} */
    else
	*maxval = MAX_LIMIT;	    /* It was \{} or \{-} */
    if (*regparse == '\\')
	regparse++;	/* Allow either \{...} or \{...\} */
    if (       (*regparse != end && *regparse != NUL)
	    || (*maxval == 0 && *minval == 0))
    {
	sprintf((char *)IObuff, "Syntax error in \\%c...%c", start, end);
	EMSG_RET_FAIL(IObuff);
    }

    /*
     * Reverse the range if there was a '-', or make sure it is in the right
     * order otherwise.
     */
    if ((!reverse && *minval > *maxval) || (reverse && *minval < *maxval))
    {
	int	tmp;

	tmp = *minval;
	*minval = *maxval;
	*maxval = tmp;
    }
    skipchr();		/* let's be friends with the lexer again */
    return OK;
}

/*
 * vim_regexec and friends
 */

/*
 * Global work variables for vim_regexec().
 */
static char_u	 *reginput;		/* String-input pointer. */
static char_u	 *regbol;		/* Beginning of input, for ^ check. */
static char_u	**regstartp;		/* Pointer to startp array. */
static int	  need_clear_subexpr;	/* *regstartp end *regendp still need
					   to be cleared */

static int	regtry __ARGS((vim_regexp *, char_u *));
static void	clear_subexpr __ARGS((void));
static int	regmatch __ARGS((char_u *));
static int	regrepeat __ARGS((char_u *));

#ifdef DEBUG
int		regnarrate = 0;
#endif

/*
 * vim_regexec - match a regexp against a string
 * Uses current value of reg_ic.
 * Return non-zero if there is a match.
 */
    int
vim_regexec(prog, string, at_bol)
    vim_regexp	*prog;
    char_u	*string;
    int		at_bol;
{
    char_u	*s;

    /* Be paranoid... */
    if (prog == NULL || string == NULL)
    {
	emsg(e_null);
	return 0;
    }
    /* Check validity of program. */
    if (UCHARAT(prog->program) != MAGIC)
    {
	emsg(e_re_corr);
	return 0;
    }
    /* If there is a "must appear" string, look for it. */
    if (prog->regmust != NULL)
    {
	s = string;
	while ((s = cstrchr(s, prog->regmust[0])) != NULL)
	{
	    if (cstrncmp(s, prog->regmust, prog->regmlen) == 0)
		break;		/* Found it. */
	    s++;
	}
	if (s == NULL)		/* Not present. */
	    return 0;
    }
    /* Mark beginning of line for ^ . */
    if (at_bol)
	regbol = string;	/* is possible to match bol */
    else
	regbol = NULL;		/* we aren't there, so don't match it */

    /* Simplest case:  anchored match need be tried only once. */
    if (prog->reganch)
    {
	if (prog->regstart != '\0' && prog->regstart != string[0] &&
	    (!reg_ic || TO_LOWER(prog->regstart) != TO_LOWER(string[0])))
	    return 0;
	return regtry(prog, string);
    }

    /* Messy cases:  unanchored match. */
    s = string;
    if (prog->regstart != '\0')
	/* We know what char it must start with. */
	while ((s = cstrchr(s, prog->regstart)) != NULL)
	{
	    if (regtry(prog, s))
		return 1;
	    s++;
	}
    else
	/* We don't -- general case. */
	do
	{
	    if (regtry(prog, s))
		return 1;
	} while (*s++ != '\0');

    /* Failure. */
    return 0;
}

/*
 * regtry - try match at specific point
 */
    static int			/* 0 failure, 1 success */
regtry(prog, string)
    vim_regexp	   *prog;
    char_u	   *string;
{
    reginput = string;
    regstartp = prog->startp;
    regendp = prog->endp;
    need_clear_subexpr = TRUE;

    if (regmatch(prog->program + 1))
    {
	clear_subexpr();
	prog->startp[0] = string;
	prog->endp[0] = reginput;
	return 1;
    }
    else
	return 0;
}

/*
 * Clear the subexpressions, if this wasn't done yet.
 * This construction is used to clear the subexpressions only when they are
 * used (to increase speed).
 */
    static void
clear_subexpr()
{
    if (need_clear_subexpr)
    {
	vim_memset(regstartp, 0, sizeof(char_u *) * NSUBEXP);
	vim_memset(regendp, 0, sizeof(char_u *) * NSUBEXP);
	need_clear_subexpr = FALSE;
    }
}

/*
 * regmatch - main matching routine
 *
 * Conceptually the strategy is simple: Check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
    static int			/* 0 failure, 1 success */
regmatch(prog)
    char_u	   *prog;
{
    char_u	    *scan;	/* Current node. */
    char_u	    *next;	/* Next node. */
    int		    minval = -1;
    int		    maxval = -1;
    static int	    break_count = 0;

    /* Some patterns my cause a long time to match, even though they are not
     * illegal.  E.g., "\([a-z]\+\)\+Q".  Allow breaking them with CTRL-C.
     * But don't check too often, that could be slow. */
    if ((++break_count & 0xfff) == 0)
	ui_breakcheck();

    scan = prog;
#ifdef DEBUG
    if (scan != NULL && regnarrate)
    {
	mch_errmsg(regprop(scan));
	mch_errmsg("(\n");
    }
#endif
    while (scan != NULL)
    {
	if (got_int)
	    return 0;
#ifdef DEBUG
	if (regnarrate)
	{
	    mch_errmsg(regprop(scan));
	    mch_errmsg("...\n");
	}
#endif
	next = regnext(scan);
	switch (OP(scan))
	{
	  case BOL:
	    if (reginput != regbol)
		return 0;
	    break;
	  case EOL:
	    if (*reginput != '\0')
		return 0;
	    break;
	  case BOW:	/* \<word; reginput points to w */
	    if (reginput != regbol && vim_iswordc(reginput[-1]))
		return 0;
	    if (!reginput[0] || !vim_iswordc(reginput[0]))
		return 0;
	    break;
	  case EOW:	/* word\>; reginput points after d */
	    if (reginput == regbol || !vim_iswordc(reginput[-1]))
		return 0;
	    if (reginput[0] && vim_iswordc(reginput[0]))
		return 0;
	    break;
	  case ANY:
	    if (*reginput == '\0')
		return 0;
	    reginput++;
	    break;
	  case IDENT:
	    if (!vim_isIDc(*reginput))
		return 0;
	    reginput++;
	    break;
	  case KWORD:
	    if (!vim_iswordc(*reginput))
		return 0;
	    reginput++;
	    break;
	  case FNAME:
	    if (!vim_isfilec(*reginput))
		return 0;
	    reginput++;
	    break;
	  case PRINT:
	    if (charsize(*reginput) != 1)
		return 0;
	    reginput++;
	    break;
	  case SIDENT:
	    if (isdigit(*reginput) || !vim_isIDc(*reginput))
		return 0;
	    reginput++;
	    break;
	  case SWORD:
	    if (isdigit(*reginput) || !vim_iswordc(*reginput))
		return 0;
	    reginput++;
	    break;
	  case SFNAME:
	    if (isdigit(*reginput) || !vim_isfilec(*reginput))
		return 0;
	    reginput++;
	    break;
	  case SPRINT:
	    if (isdigit(*reginput) || charsize(*reginput) != 1)
		return 0;
	    reginput++;
	    break;
	  case WHITE:
	    if (!vim_iswhite(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NWHITE:
	    if (*reginput == NUL || vim_iswhite(*reginput))
		return 0;
	    reginput++;
	    break;
	  case DIGIT:
	    if (!ri_digit(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NDIGIT:
	    if (*reginput == NUL || ri_digit(*reginput))
		return 0;
	    reginput++;
	    break;
	  case HEX:
	    if (!ri_hex(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NHEX:
	    if (*reginput == NUL || ri_hex(*reginput))
		return 0;
	    reginput++;
	    break;
	  case OCTAL:
	    if (!ri_octal(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NOCTAL:
	    if (*reginput == NUL || ri_octal(*reginput))
		return 0;
	    reginput++;
	    break;
	  case WORD:
	    if (!ri_word(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NWORD:
	    if (*reginput == NUL || ri_word(*reginput))
		return 0;
	    reginput++;
	    break;
	  case HEAD:
	    if (!ri_head(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NHEAD:
	    if (*reginput == NUL || ri_head(*reginput))
		return 0;
	    reginput++;
	    break;
	  case ALPHA:
	    if (!ri_alpha(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NALPHA:
	    if (*reginput == NUL || ri_alpha(*reginput))
		return 0;
	    reginput++;
	    break;
	  case LOWER:
	    if (!ri_lower(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NLOWER:
	    if (*reginput == NUL || ri_lower(*reginput))
		return 0;
	    reginput++;
	    break;
	  case UPPER:
	    if (!ri_upper(*reginput))
		return 0;
	    reginput++;
	    break;
	  case NUPPER:
	    if (*reginput == NUL || ri_upper(*reginput))
		return 0;
	    reginput++;
	    break;
	  case EXACTLY:
	    {
		int	    len;
		char_u	    *opnd;

		opnd = OPERAND(scan);
		/* Inline the first character, for speed. */
		if (*opnd != *reginput
			&& (!reg_ic || TO_LOWER(*opnd) != TO_LOWER(*reginput)))
		    return 0;
		len = STRLEN(opnd);
		if (len > 1 && cstrncmp(opnd, reginput, len) != 0)
		    return 0;
		reginput += len;
	    }
	    break;
	  case ANYOF:
	    if (*reginput == '\0' || cstrchr(OPERAND(scan), *reginput) == NULL)
		return 0;
	    reginput++;
	    break;
	  case ANYBUT:
	    if (*reginput == '\0' || cstrchr(OPERAND(scan), *reginput) != NULL)
		return 0;
	    reginput++;
	    break;
	  case NOTHING:
	    break;
	  case BACK:
	    break;
	  case MOPEN + 1:
	  case MOPEN + 2:
	  case MOPEN + 3:
	  case MOPEN + 4:
	  case MOPEN + 5:
	  case MOPEN + 6:
	  case MOPEN + 7:
	  case MOPEN + 8:
	  case MOPEN + 9:
	    {
		int	    no;
		char_u	    *save;

		clear_subexpr();
		no = OP(scan) - MOPEN;
		save = regstartp[no];
		regstartp[no] = reginput; /* Tentatively */
#ifdef DEBUG
		if (regnarrate)
		    printf("MOPEN  %d pre  @'%s' ('%s' )'%s'\n",
			no, save,
			regstartp[no] ? (char *)regstartp[no] : "NULL",
			regendp[no] ? (char *)regendp[no] : "NULL");
#endif

		if (regmatch(next))
		{
#ifdef DEBUG
		    if (regnarrate)
			printf("MOPEN  %d post @'%s' ('%s' )'%s'\n",
				no, save,
				regstartp[no] ? (char *)regstartp[no] : "NULL",
				regendp[no] ? (char *)regendp[no] : "NULL");
#endif
		    return 1;
		}
		regstartp[no] = save;	    /* We were wrong... */
		return 0;
	    }
	    /* break; Not Reached */
	  case MCLOSE + 1:
	  case MCLOSE + 2:
	  case MCLOSE + 3:
	  case MCLOSE + 4:
	  case MCLOSE + 5:
	  case MCLOSE + 6:
	  case MCLOSE + 7:
	  case MCLOSE + 8:
	  case MCLOSE + 9:
	    {
		int	    no;
		char_u	    *save;

		clear_subexpr();
		no = OP(scan) - MCLOSE;
		save = regendp[no];
		regendp[no] = reginput; /* Tentatively */
#ifdef DEBUG
		if (regnarrate)
		    printf("MCLOSE %d pre  @'%s' ('%s' )'%s'\n",
			no, save,
			regstartp[no] ? (char *)regstartp[no] : "NULL",
			regendp[no] ? (char *)regendp[no] : "NULL");
#endif

		if (regmatch(next))
		{
#ifdef DEBUG
		    if (regnarrate)
			printf("MCLOSE %d post @'%s' ('%s' )'%s'\n",
				no, save,
				regstartp[no] ? (char *)regstartp[no] : "NULL",
				regendp[no] ? (char *)regendp[no] : "NULL");
#endif
		    return 1;
		}
		regendp[no] = save;	/* We were wrong... */
		return 0;
	    }
	    /* break; Not Reached */
	  case BACKREF + 1:
	  case BACKREF + 2:
	  case BACKREF + 3:
	  case BACKREF + 4:
	  case BACKREF + 5:
	  case BACKREF + 6:
	  case BACKREF + 7:
	  case BACKREF + 8:
	  case BACKREF + 9:
	    {
		int	no;
		int	len;

		clear_subexpr();
		no = OP(scan) - BACKREF;
		if (regendp[no] != NULL)
		{
		    len = (int)(regendp[no] - regstartp[no]);
		    if (cstrncmp(regstartp[no], reginput, len) != 0)
			return 0;
		    reginput += len;
		}
		else
		{
		    /*emsg("backref to 0-repeat");*/
		    /*return 0;*/
		}
	    }
	    break;
	  case BRANCH:
	    {
		char_u	    *save;

		if (OP(next) != BRANCH) /* No choice. */
		    next = OPERAND(scan);	/* Avoid recursion. */
		else
		{
		    do
		    {
			save = reginput;
			if (regmatch(OPERAND(scan)))
			    return 1;
			reginput = save;
			scan = regnext(scan);
		    } while (scan != NULL && OP(scan) == BRANCH);
		    return 0;
		    /* NOTREACHED */
		}
	    }
	    break;
	  case BRACE_LIMITS:
	    {
		int	no;

		if (OP(next) == BRACE_SIMPLE)
		{
		    minval = OPERAND_MIN(scan);
		    maxval = OPERAND_MAX(scan);
		}
		else if (OP(next) >= BRACE_COMPLEX &&
			 OP(next) < BRACE_COMPLEX + 10)
		{
		    no = OP(next) - BRACE_COMPLEX;
		    brace_min[no] = OPERAND_MIN(scan);
		    brace_max[no] = OPERAND_MAX(scan);
		    brace_count[no] = 0;
		}
		else
		{
		    emsg(e_internal);	    /* Shouldn't happen */
		    return 0;
		}
	    }
	    break;
	  case BRACE_COMPLEX + 0:
	  case BRACE_COMPLEX + 1:
	  case BRACE_COMPLEX + 2:
	  case BRACE_COMPLEX + 3:
	  case BRACE_COMPLEX + 4:
	  case BRACE_COMPLEX + 5:
	  case BRACE_COMPLEX + 6:
	  case BRACE_COMPLEX + 7:
	  case BRACE_COMPLEX + 8:
	  case BRACE_COMPLEX + 9:
	    {
		int	    no;
		char_u	    *save;

		no = OP(scan) - BRACE_COMPLEX;
		++brace_count[no];

		/* If not matched enough times yet, try one more */
		if (brace_count[no] <= (brace_min[no] <= brace_max[no]
				    ? brace_min[no] : brace_max[no]))
		{
		    save = reginput;
		    if (regmatch(OPERAND(scan)))
			return 1;
		    reginput = save;
		    --brace_count[no];	/* failed, decrement match count */
		    return 0;
		}

		/* If matched enough times, may try matching some more */
		if (brace_min[no] <= brace_max[no])
		{
		    /* Range is the normal way around, use longest match */
		    if (brace_count[no] <= brace_max[no])
		    {
			save = reginput;
			if (regmatch(OPERAND(scan)))
			    return 1;	    /* matched some more times */
			reginput = save;
			--brace_count[no];  /* matched just enough times */
			/* continue with the items after \{} */
		    }
		}
		else
		{
		    /* Range is backwards, use shortest match first */
		    if (brace_count[no] <= brace_min[no])
		    {
			save = reginput;
			if (regmatch(next))
			    return 1;
			reginput = save;
			next = OPERAND(scan);
			/* must try to match one more item */
		    }
		}
	    }
	    break;

	  case BRACE_SIMPLE:
	  case STAR:
	  case PLUS:
	    {
		int	nextch;
		int	nextch_ic;
		int	no;
		char_u	*save;

		/*
		 * Lookahead to avoid useless match attempts when we know
		 * what character comes next.
		 */
		if (OP(next) == EXACTLY)
		{
		    nextch = *OPERAND(next);
		    if (reg_ic)
		    {
			if (isupper(nextch))
			    nextch_ic = TO_LOWER(nextch);
			else
			    nextch_ic = TO_UPPER(nextch);
		    }
		    else
			nextch_ic = nextch;
		}
		else
		{
		    nextch = '\0';
		    nextch_ic = '\0';
		}
		if (OP(scan) != BRACE_SIMPLE)
		{
		    minval = (OP(scan) == STAR) ? 0 : 1;
		    maxval = MAX_LIMIT;
		}

		save = reginput;
		no = regrepeat(OPERAND(scan));
		if (minval <= maxval)
		{
		    /* Range is the normal way around, use longest match */
		    if (no > maxval)
		    {
			no = maxval;
			reginput = save + no;
		    }
		    while (no >= minval)
		    {
			/* If it could work, try it. */
			if (nextch == '\0' || *reginput == nextch
						    || *reginput == nextch_ic)
			    if (regmatch(next))
				return 1;
			/* Couldn't or didn't -- back up. */
			no--;
			reginput = save + no;
		    }
		}
		else
		{
		    /* Range is backwards, use shortest match first */
		    if (no < maxval)
			return 0;
		    if (minval > no)
			minval = no;	/* Actually maximum value */
		    no = maxval;
		    reginput = save + no;
		    while (no <= minval)
		    {
			/* If it could work, try it. */
			if (nextch == '\0' || *reginput == nextch
						    || *reginput == nextch_ic)
			    if (regmatch(next))
				return 1;
			/* Couldn't or didn't -- try longer match. */
			no++;
			reginput = save + no;
		    }
		}
		return 0;
	    }
	    /* break; Not Reached */
	  case END:
	    return 1;	    /* Success! */
	    /* break; Not Reached */
	  default:
	    emsg(e_re_corr);
#ifdef DEBUG
	    printf("Illegal op code %d\n", OP(scan));
#endif
	    return 0;
	    /* break; Not Reached */
	}

	scan = next;
    }

    /*
     * We get here only if there's trouble -- normally "case END" is the
     * terminating point.
     */
    emsg(e_re_corr);
#ifdef DEBUG
    printf("Premature EOL\n");
#endif
    return 0;
}

/*
 * regrepeat - repeatedly match something simple, report how many
 */
    static int
regrepeat(p)
    char_u	*p;
{
    int		count = 0;
    char_u	*scan;
    char_u	*opnd;
    int		mask;

    scan = reginput;
    opnd = OPERAND(p);
    switch (OP(p))
    {
      case ANY:
	count = STRLEN(scan);
	scan += count;
	break;
      case IDENT:
	for (count = 0; vim_isIDc(*scan); ++count)
	    ++scan;
	break;
      case KWORD:
	for (count = 0; vim_iswordc(*scan); ++count)
	    ++scan;
	break;
      case FNAME:
	for (count = 0; vim_isfilec(*scan); ++count)
	    ++scan;
	break;
      case PRINT:
	for (count = 0; charsize(*scan) == 1; ++count)
	    ++scan;
	break;
      case SIDENT:
	for (count = 0; !isdigit(*scan) && vim_isIDc(*scan); ++count)
	    ++scan;
	break;
      case SWORD:
	for (count = 0; !isdigit(*scan) && vim_iswordc(*scan); ++count)
	    ++scan;
	break;
      case SFNAME:
	for (count = 0; !isdigit(*scan) && vim_isfilec(*scan); ++count)
	    ++scan;
	break;
      case SPRINT:
	for (count = 0; !isdigit(*scan) && charsize(*scan) == 1; ++count)
	    ++scan;
	break;
      case WHITE:
	for (count = 0; vim_iswhite(*scan); ++count)
	    ++scan;
	break;
      case NWHITE:
	for (count = 0; *scan != NUL && !vim_iswhite(*scan); ++count)
	    ++scan;
	break;
      case DIGIT:
	mask = RI_DIGIT;
do_class:
	for (count = 0; class_tab[*scan] & mask; ++count)
	    ++scan;
	break;
      case NDIGIT:
	mask = RI_DIGIT;
do_nclass:
	for (count = 0; *scan != NUL && !(class_tab[*scan] & mask); ++count)
	    ++scan;
	break;
      case HEX:
	mask = RI_HEX;
	goto do_class;
      case NHEX:
	mask = RI_HEX;
	goto do_nclass;
      case OCTAL:
	mask = RI_OCTAL;
	goto do_class;
      case NOCTAL:
	mask = RI_OCTAL;
	goto do_nclass;
      case WORD:
	mask = RI_WORD;
	goto do_class;
      case NWORD:
	mask = RI_WORD;
	goto do_nclass;
      case HEAD:
	mask = RI_HEAD;
	goto do_class;
      case NHEAD:
	mask = RI_HEAD;
	goto do_nclass;
      case ALPHA:
	mask = RI_ALPHA;
	goto do_class;
      case NALPHA:
	mask = RI_ALPHA;
	goto do_nclass;
      case LOWER:
	mask = RI_LOWER;
	goto do_class;
      case NLOWER:
	mask = RI_LOWER;
	goto do_nclass;
      case UPPER:
	mask = RI_UPPER;
	goto do_class;
      case NUPPER:
	mask = RI_UPPER;
	goto do_nclass;
      case EXACTLY:
	{
	    int	    cu, cl;

	    if (reg_ic)
	    {
		cu = TO_UPPER(*opnd);
		cl = TO_LOWER(*opnd);
		while (*scan == cu || *scan == cl)
		{
		    count++;
		    scan++;
		}
	    }
	    else
	    {
		cu = *opnd;
		while (*scan == cu)
		{
		    count++;
		    scan++;
		}
	    }
	    break;
	}
      case ANYOF:
	while (*scan != '\0' && cstrchr(opnd, *scan) != NULL)
	{
	    count++;
	    scan++;
	}
	break;
      case ANYBUT:
	while (*scan != '\0' && cstrchr(opnd, *scan) == NULL)
	{
	    count++;
	    scan++;
	}
	break;
      default:			/* Oh dear.  Called inappropriately. */
	emsg(e_re_corr);
#ifdef DEBUG
	printf("Called regrepeat with op code %d\n", OP(p));
#endif
	count = 0;		/* Best compromise. */
	break;
    }
    reginput = scan;

    return count;
}

/*
 * regnext - dig the "next" pointer out of a node
 */
    static char_u *
regnext(p)
    char_u  *p;
{
    int	    offset;

    if (p == JUST_CALC_SIZE)
	return NULL;

    offset = NEXT(p);
    if (offset == 0)
	return NULL;

    if (OP(p) == BACK)
	return p - offset;
    else
	return p + offset;
}

#ifdef DEBUG

/*
 * regdump - dump a regexp onto stdout in vaguely comprehensible form
 */
    static void
regdump(pattern, r)
    char_u	*pattern;
    vim_regexp	*r;
{
    char_u  *s;
    int	    op = EXACTLY;	/* Arbitrary non-END op. */
    char_u  *next;

    printf("\nregcomp(%s):\n", pattern);

    s = r->program + 1;
    while (op != END)		/* While that wasn't END last time... */
    {
	op = OP(s);
	printf("%2d%s", (int)(s - r->program), regprop(s)); /* Where, what. */
	next = regnext(s);
	if (next == NULL)	/* Next ptr. */
	    printf("(0)");
	else
	    printf("(%d)", (int)((s - r->program) + (next - s)));
	if (op == BRACE_LIMITS)
	{
	    /* Two short ints */
	    printf(" minval %d, maxval %d", OPERAND_MIN(s), OPERAND_MAX(s));
	    s += 4;
	}
	s += 3;
	if (op == ANYOF || op == ANYBUT || op == EXACTLY)
	{
	    /* Literal string, where present. */
	    while (*s != '\0')
		printf("%c", *s++);
	    s++;
	}
	printf("\n");
    }

    /* Header fields of interest. */
    if (r->regstart != '\0')
	printf("start `%c' ", r->regstart);
    if (r->reganch)
	printf("anchored ");
    if (r->regmust != NULL)
	printf("must have \"%s\"", r->regmust);
    printf("\n");
}

/*
 * regprop - printable representation of opcode
 */
    static char_u *
regprop(op)
    char_u	   *op;
{
    char_u	    *p;
    static char_u   buf[50];

    (void) strcpy(buf, ":");

    switch (OP(op))
    {
      case BOL:
	p = "BOL";
	break;
      case EOL:
	p = "EOL";
	break;
      case BOW:
	p = "BOW";
	break;
      case EOW:
	p = "EOW";
	break;
      case ANY:
	p = "ANY";
	break;
      case IDENT:
	p = "IDENT";
	break;
      case KWORD:
	p = "KWORD";
	break;
      case FNAME:
	p = "FNAME";
	break;
      case PRINT:
	p = "PRINT";
	break;
      case SIDENT:
	p = "SIDENT";
	break;
      case SWORD:
	p = "SWORD";
	break;
      case SFNAME:
	p = "SFNAME";
	break;
      case SPRINT:
	p = "SPRINT";
	break;
      case WHITE:
	p = "WHITE";
	break;
      case NWHITE:
	p = "NWHITE";
	break;
      case DIGIT:
	p = "DIGIT";
	break;
      case NDIGIT:
	p = "NDIGIT";
	break;
      case HEX:
	p = "HEX";
	break;
      case NHEX:
	p = "NHEX";
	break;
      case OCTAL:
	p = "OCTAL";
	break;
      case NOCTAL:
	p = "NOCTAL";
	break;
      case WORD:
	p = "WORD";
	break;
      case NWORD:
	p = "NWORD";
	break;
      case HEAD:
	p = "HEAD";
	break;
      case NHEAD:
	p = "NHEAD";
	break;
      case ALPHA:
	p = "ALPHA";
	break;
      case NALPHA:
	p = "NALPHA";
	break;
      case LOWER:
	p = "LOWER";
	break;
      case NLOWER:
	p = "NLOWER";
	break;
      case UPPER:
	p = "UPPER";
	break;
      case NUPPER:
	p = "NUPPER";
	break;
      case ANYOF:
	p = "ANYOF";
	break;
      case ANYBUT:
	p = "ANYBUT";
	break;
      case BRANCH:
	p = "BRANCH";
	break;
      case EXACTLY:
	p = "EXACTLY";
	break;
      case NOTHING:
	p = "NOTHING";
	break;
      case BACK:
	p = "BACK";
	break;
      case END:
	p = "END";
	break;
      case MOPEN + 1:
      case MOPEN + 2:
      case MOPEN + 3:
      case MOPEN + 4:
      case MOPEN + 5:
      case MOPEN + 6:
      case MOPEN + 7:
      case MOPEN + 8:
      case MOPEN + 9:
	sprintf(buf + STRLEN(buf), "MOPEN%d", OP(op) - MOPEN);
	p = NULL;
	break;
      case MCLOSE + 1:
      case MCLOSE + 2:
      case MCLOSE + 3:
      case MCLOSE + 4:
      case MCLOSE + 5:
      case MCLOSE + 6:
      case MCLOSE + 7:
      case MCLOSE + 8:
      case MCLOSE + 9:
	sprintf(buf + STRLEN(buf), "MCLOSE%d", OP(op) - MCLOSE);
	p = NULL;
	break;
      case BACKREF + 1:
      case BACKREF + 2:
      case BACKREF + 3:
      case BACKREF + 4:
      case BACKREF + 5:
      case BACKREF + 6:
      case BACKREF + 7:
      case BACKREF + 8:
      case BACKREF + 9:
	sprintf(buf + STRLEN(buf), "BACKREF%d", OP(op) - BACKREF);
	p = NULL;
	break;
      case STAR:
	p = "STAR";
	break;
      case PLUS:
	p = "PLUS";
	break;
      case BRACE_LIMITS:
	p = "BRACE_LIMITS";
	break;
      case BRACE_SIMPLE:
	p = "BRACE_SIMPLE";
	break;
      case BRACE_COMPLEX + 0:
      case BRACE_COMPLEX + 1:
      case BRACE_COMPLEX + 2:
      case BRACE_COMPLEX + 3:
      case BRACE_COMPLEX + 4:
      case BRACE_COMPLEX + 5:
      case BRACE_COMPLEX + 6:
      case BRACE_COMPLEX + 7:
      case BRACE_COMPLEX + 8:
      case BRACE_COMPLEX + 9:
	sprintf(buf + STRLEN(buf), "BRACE_COMPLEX%d", OP(op) - BRACE_COMPLEX);
	p = NULL;
	break;
      default:
	sprintf(buf + STRLEN(buf), "corrupt %d", OP(op));
	p = NULL;
	break;
    }
    if (p != NULL)
	(void) strcat(buf, p);
    return buf;
}
#endif

/*
 * Compare two strings, ignore case if reg_ic set.
 * Return 0 if strings match, non-zero otherwise.
 */
    static int
cstrncmp(s1, s2, n)
    char_u	*s1, *s2;
    int		n;
{
    if (!reg_ic)
	return STRNCMP(s1, s2, n);
    return STRNICMP(s1, s2, n);
}

/*
 * cstrchr: This function is used a lot for simple searches, keep it fast!
 */
    static char_u *
cstrchr(s, c)
    char_u	*s;
    int		c;
{
    char_u	*p;
    int		cc;

    if (!reg_ic)
	return vim_strchr(s, c);

    /* tolower() and toupper() can be slow, comparing twice should be a lot
     * faster (esp. when using MS Visual C++!) */
    if (isupper(c))
	cc = TO_LOWER(c);
    else if (islower(c))
	cc = TO_UPPER(c);
    else
	return vim_strchr(s, c);

    for (p = s; *p; ++p)
	if (*p == c || *p == cc)
	    return p;
    return NULL;
}

/***************************************************************
 *		      regsub stuff			       *
 ***************************************************************/

/* This stuff below really confuses cc on an SGI -- webb */
#ifdef __sgi
# undef __ARGS
# define __ARGS(x)  ()
#endif

/*
 * We should define ftpr as a pointer to a function returning a pointer to
 * a function returning a pointer to a function ...
 * This is impossible, so we declare a pointer to a function returning a
 * pointer to a function returning void. This should work for all compilers.
 */
typedef void (*(*fptr) __ARGS((char_u *, int)))();

static fptr do_upper __ARGS((char_u *, int));
static fptr do_Upper __ARGS((char_u *, int));
static fptr do_lower __ARGS((char_u *, int));
static fptr do_Lower __ARGS((char_u *, int));

    static fptr
do_upper(d, c)
    char_u *d;
    int c;
{
    *d = TO_UPPER(c);

    return (fptr)NULL;
}

    static fptr
do_Upper(d, c)
    char_u *d;
    int c;
{
    *d = TO_UPPER(c);

    return (fptr)do_Upper;
}

    static fptr
do_lower(d, c)
    char_u *d;
    int c;
{
    *d = TO_LOWER(c);

    return (fptr)NULL;
}

    static fptr
do_Lower(d, c)
    char_u *d;
    int c;
{
    *d = TO_LOWER(c);

    return (fptr)do_Lower;
}

/*
 * regtilde(): Replace tildes in the pattern by the old pattern.
 *
 * Short explanation of the tilde: It stands for the previous replacement
 * pattern.  If that previous pattern also contains a ~ we should go back a
 * step further...  But we insert the previous pattern into the current one
 * and remember that.
 * This still does not handle the case where "magic" changes. TODO?
 *
 * The tildes are parsed once before the first call to vim_regsub().
 */
    char_u *
regtilde(source, magic)
    char_u  *source;
    int	    magic;
{
    char_u  *newsub = source;
    char_u  *tmpsub;
    char_u  *p;
    int	    len;
    int	    prevlen;

    for (p = newsub; *p; ++p)
    {
	if ((*p == '~' && magic) || (*p == '\\' && *(p + 1) == '~' && !magic))
	{
	    if (reg_prev_sub != NULL)
	    {
		/* length = len(newsub) - 1 + len(prev_sub) + 1 */
		prevlen = STRLEN(reg_prev_sub);
		tmpsub = alloc((unsigned)(STRLEN(newsub) + prevlen));
		if (tmpsub != NULL)
		{
		    /* copy prefix */
		    len = (int)(p - newsub);	/* not including ~ */
		    mch_memmove(tmpsub, newsub, (size_t)len);
		    /* interpretate tilde */
		    mch_memmove(tmpsub + len, reg_prev_sub, (size_t)prevlen);
		    /* copy postfix */
		    if (!magic)
			++p;			/* back off \ */
		    STRCPY(tmpsub + len + prevlen, p + 1);

		    if (newsub != source)	/* already allocated newsub */
			vim_free(newsub);
		    newsub = tmpsub;
		    p = newsub + len + prevlen;
		}
	    }
	    else if (magic)
		STRCPY(p, p + 1);		/* remove '~' */
	    else
		STRCPY(p, p + 2);		/* remove '\~' */
	    --p;
	}
	else if (*p == '\\' && p[1])		/* skip escaped characters */
	    ++p;
    }

    vim_free(reg_prev_sub);
    if (newsub != source)	/* newsub was allocated, just keep it */
	reg_prev_sub = newsub;
    else			/* no ~ found, need to save newsub  */
	reg_prev_sub = vim_strsave(newsub);
    return newsub;
}

/*
 * Adjust the pointers in "prog" for moving the matches text from "old_ptr" to
 * "new_ptr".  Used when saving a copy of the matched text and want to use
 * vim_regsub().
 */
    void
vim_regnewptr(prog, old_ptr, new_ptr)
    vim_regexp	*prog;
    char_u	*old_ptr;
    char_u	*new_ptr;
{
    int		j;

    for (j = 0; j < NSUBEXP; ++j)
    {
	/* Careful with these pointer computations, it can go wrong for
	 * segmented memory (16 bit MS-DOS). */
	if (prog->startp[j] != NULL)
	    prog->startp[j] = new_ptr + (prog->startp[j] - old_ptr);
	if (prog->endp[j] != NULL)
	    prog->endp[j] = new_ptr + (prog->endp[j] - old_ptr);
    }
}

/*
 * vim_regsub() - perform substitutions after a regexp match
 *
 * If copy is TRUE really copy into dest.
 * If copy is FALSE nothing is copied, this is just to find out the length of
 * the result.
 *
 * Returns the size of the replacement, including terminating NUL.
 */
    int
vim_regsub(prog, source, dest, copy, magic)
    vim_regexp	   *prog;
    char_u	   *source;
    char_u	   *dest;
    int		    copy;
    int		    magic;
{
    char_u	*src;
    char_u	*dst;
    char_u	*s;
    int		c;
    int		no;
    fptr	func = (fptr)NULL;

    if (prog == NULL || source == NULL || dest == NULL)
    {
	emsg(e_null);
	return 0;
    }
    if (UCHARAT(prog->program) != MAGIC)
    {
	emsg(e_re_corr);
	return 0;
    }
    src = source;
    dst = dest;

    while ((c = *src++) != '\0')
    {
	no = -1;
	if (c == '&' && magic)
	    no = 0;
	else if (c == '\\' && *src != NUL)
	{
	    if (*src == '&' && !magic)
	    {
		++src;
		no = 0;
	    }
	    else if ('0' <= *src && *src <= '9')
	    {
		no = *src++ - '0';
	    }
	    else if (vim_strchr((char_u *)"uUlLeE", *src))
	    {
		switch (*src++)
		{
		case 'u':   func = (fptr)do_upper;
			    continue;
		case 'U':   func = (fptr)do_Upper;
			    continue;
		case 'l':   func = (fptr)do_lower;
			    continue;
		case 'L':   func = (fptr)do_Lower;
			    continue;
		case 'e':
		case 'E':   func = (fptr)NULL;
			    continue;
		}
	    }
	}
	if (no < 0)	      /* Ordinary character. */
	{
	    if (c == '\\' && *src != NUL)
	    {
		/* Check for abbreviations -- webb */
		switch (*src)
		{
		    case 'r':	c = CR;		break;
		    case 'n':	c = NL;		break;
		    case 't':	c = TAB;	break;
		    /* Oh no!  \e already has meaning in subst pat :-( */
		    /* case 'e':    c = ESC;	    break; */
		    case 'b':	c = Ctrl('H');	break;
		    default:
			/* Normal character, not abbreviation */
			c = *src;
			break;
		}
		src++;
	    }
	    if (copy)
	    {
		if (func == (fptr)NULL)	    /* just copy */
		    *dst = c;
		else			    /* change case */
		    func = (fptr)(func(dst, c));
			    /* Turbo C complains without the typecast */
	    }
	    dst++;
	}
	else if (prog->startp[no] != NULL && prog->endp[no] != NULL)
	{
	    for (s = prog->startp[no]; s < prog->endp[no]; ++s)
	    {
		if (copy && *s == '\0') /* we hit NUL. */
		{
		    emsg(e_re_damg);
		    goto exit;
		}
		/*
		 * Insert a CTRL-V in front of a CR, otherwise
		 * it will be replaced by a line break.
		 */
		if (*s == CR)
		{
		    if (copy)
		    {
			dst[0] = Ctrl('V');
			dst[1] = CR;
		    }
		    dst += 2;
		}
		else
		{
		    if (copy)
		    {
			if (func == (fptr)NULL)	    /* just copy */
			    *dst = *s;
			else			    /* change case */
			    func = (fptr)(func(dst, *s));
				    /* Turbo C complains without the typecast */
		    }
		    ++dst;
		}
	    }
	}
    }
    if (copy)
	*dst = '\0';

exit:
    return (int)((dst - dest) + 1);
}

#if 0 /* not used */
/*
 * Return TRUE if "c" is a wildcard character.  Depends on 'magic'.
 */
    int
vim_iswildc(c)
    int		c;
{
    return vim_strchr((char_u *)(p_magic ? ".*~[^$\\" : "^$\\"), c) != NULL;
}
#endif
