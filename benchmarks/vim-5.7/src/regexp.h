/* vi:set ts=8 sts=4 sw=4:
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * This is NOT the original regular expression code as written by
 * Henry Spencer. This code has been modified specifically for use
 * with the VIM editor, and should not be used apart from compiling
 * VIM. If you want a good regular expression library, get the
 * original code.
 *
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * Definitions etc. for regexp(3) routines.
 */

#ifndef _REGEXP_H
#define _REGEXP_H

#define NSUBEXP  10
typedef struct
{
    char_u	   *startp[NSUBEXP];
    char_u	   *endp[NSUBEXP];
    char_u	    regstart;	/* Internal use only. */
    char_u	    reganch;	/* Internal use only. */
    char_u	   *regmust;	/* Internal use only. */
    int		    regmlen;	/* Internal use only. */
    char_u	    program[1]; /* Unwarranted chumminess with compiler. */
} vim_regexp;

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */

#define MAGIC	0234

#endif	/* _REGEXP_H */
