/*****************************************************************************
*   $Id: debug.c,v 8.1 1999/03/04 04:16:38 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains debugging functions.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <ctype.h>
#ifdef ENABLE_STDARG
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#include "debug.h"
#include "options.h"
#include "read.h"

/*============================================================================
=   Function definitions
============================================================================*/

#ifdef DEBUG

extern void lineBreak() {}	/* provides a line-specified break point */

#ifdef ENABLE_STDARG
extern void debugPrintf( const enum eDebugLevels level,
			 const char *const format, ... )
#else
extern void debugPrintf( va_alist )
    va_dcl
#endif
{
    va_list ap;

#ifdef ENABLE_STDARG
    va_start(ap, format);
#else
    enum eDebugLevels level;
    const char *format;

    va_start(ap);
    level = va_arg(ap, enum eDebugLevels);
    format = va_arg(ap, char *);
#endif

    if (debug(level))
	vprintf(format, ap);
    fflush(stdout);

    va_end(ap);
}

extern void debugPutc( c, level )
    const int c;
    const int level;
{
    if (debug(level)  &&  c != EOF)
    {
	     if (c == STRING_SYMBOL)	printf("\"string\"");
	else if (c == CHAR_SYMBOL)	printf("'c'");
	else				putchar(c);

	fflush(stdout);
    }
}

extern void debugParseNest( increase, level )
    const boolean increase;
    const unsigned int level;
{
    debugPrintf(DEBUG_PARSE, "<*%snesting:%d*>", increase ? "++" : "--", level);
}

extern void debugCppNest( begin, level )
    const boolean begin;
    const unsigned int level;
{
    debugPrintf(DEBUG_CPP, "<*cpp:%s level %d*>", begin ? "begin":"end", level);
}

extern void debugCppIgnore( ignore )
    const boolean ignore;
{
    debugPrintf(DEBUG_CPP, "<*cpp:%s ignore*>", ignore ? "begin":"end");
}

extern void clearString( string, length )
    char *const string;
    const int length;
{
    int i;

    for (i = 0 ; i < length ; ++i)
	string[i] = '\0';
}

extern void debugEntry( tag )
    const tagEntryInfo *const tag;
{
    const char *const scope = tag->isFileScope ? "{fs}" : "";

    if (debug(DEBUG_PARSE))
    {
	unsigned int i;

	printf("<#%s%s:%s", scope, tag->kindName, tag->name);

	/*  Add any other extension flags.
	 */
	for (i = 0  ;  i < tag->otherFields.count  ;  ++i)
	{
	    const char *const label = tag->otherFields.label[i];
	    const char *const value = tag->otherFields.value[i];

	    if (label != NULL)
		printf("[%s:%s]", label, value == NULL ? "" : value);
	}
	printf("#>");
	fflush(stdout);
    }
}

#endif

/* vi:set tabstop=8 shiftwidth=4: */
