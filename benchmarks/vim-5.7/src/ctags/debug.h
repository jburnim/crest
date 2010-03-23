/*****************************************************************************
*   $Id: debug.h,v 8.2 1999/08/29 04:41:42 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   External interface to debug.c
*****************************************************************************/
#ifndef _DEBUG_H
#define _DEBUG_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#ifdef DEBUG
# include <assert.h>
#endif
#include "entry.h"

/*============================================================================
=   Macros
============================================================================*/

#ifdef DEBUG
# define debug(level)	((Option.debugLevel & (int)(level)) != 0)
# define DebugStatement(x)	x
# define PrintStatus(x)		if (debug(DEBUG_STATUS)) printf x;
# define Assert(c)		assert(c)
#else
# define DebugStatement(x)
# define PrintStatus(x)
# define Assert(c)
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif

/*============================================================================
=   Data declarations
============================================================================*/

/*  Defines the debugging levels.
 */
enum eDebugLevels {
    DEBUG_RAW	 = 1,		/* echo raw (filtered) characters */
    DEBUG_PARSE	 = 2,		/* echo parsing results */
    DEBUG_STATUS = 4,		/* echo file status information */
    DEBUG_OPTION = 8,		/* echo option parsing */
    DEBUG_CPP	 = 16		/* echo characters out of pre-processor */
};

/*============================================================================
=   Function prototypes
============================================================================*/
extern void lineBreak __ARGS((void));
extern void debugPrintf __ARGS((const enum eDebugLevels level, const char *const format, ...));
extern void debugPutc __ARGS((const int c, const int level));
extern void debugParseNest __ARGS((const boolean increase, const unsigned int level));
extern void debugCppNest __ARGS((const boolean begin, const unsigned int level));
extern void debugCppIgnore __ARGS((const boolean ignore));
extern void clearString __ARGS((char *const string, const int length));
extern void debugEntry __ARGS((const tagEntryInfo *const tag));

#endif	/* _DEBUG_H */

/* vi:set tabstop=8 shiftwidth=4: */
