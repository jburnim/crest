/*****************************************************************************
*   $Id: ctags.h,v 8.2 1999/09/16 05:03:29 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module is a global include file.
*****************************************************************************/
#ifndef _CTAGS_H
#define _CTAGS_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <ctype.h>	/* to define isalnum() and isalpha() */

/*============================================================================
=   General defines
============================================================================*/
#ifndef PROGRAM_VERSION
# define PROGRAM_VERSION	"3.5.1"
#endif
#define PROGRAM_NAME	"Exuberant Ctags"
#define PROGRAM_URL	"http://darren.hiebert.com/ctags/"
#define AUTHOR_NAME	"Darren Hiebert"
#define AUTHOR_EMAIL	"darren@hiebert.com"

/*============================================================================
=   Macros
============================================================================*/

/*  Is the character valid as a character of a C identifier?
 */
#define isident(c)	(isalnum(c) || (c) == '_')

/*  Is the character valid as the first character of a C identifier?
 */
#define isident1(c)	(isalpha(c) || (c) == '_' || (c) == '~')

/*============================================================================
=   Data declarations
============================================================================*/

enum eCharacters {
    /*  White space characters.
     */
    SPACE	= ' ',
    NEWLINE	= '\n',
    CRETURN	= '\r',
    FORMFEED	= '\f',
    TAB		= '\t',
    VTAB	= '\v',

    /*  Some hard to read characters.
     */
    DOUBLE_QUOTE  = '"',
    SINGLE_QUOTE  = '\'',
    BACKSLASH	  = '\\',

    STRING_SYMBOL = ('S' + 0x80),
    CHAR_SYMBOL	  = ('C' + 0x80)
};

typedef enum eLangType {
    LANG_IGNORE = -1,		/* ignore file (unknown/unsupported language) */
    LANG_AUTO,			/* automatically determine language */
    LANG_C,
    LANG_CPP,
    LANG_EIFFEL,
    LANG_FORTRAN,
    LANG_JAVA,
    LANG_COUNT			/* count of languages */
} langType;

#endif	/* _CTAGS_H */

/* vi:set tabstop=8 shiftwidth=4: */
