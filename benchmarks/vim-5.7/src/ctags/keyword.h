/*****************************************************************************
*   $Id: keyword.h,v 8.1 1999/03/04 04:16:38 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   External interface to keyword.c
*****************************************************************************/
#ifndef _KEYWORD_H
#define _KEYWORD_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

/*============================================================================
=   Function prototypes
============================================================================*/
extern void addKeyword __ARGS((const char *const string, langType language, int value));
extern int lookupKeyword __ARGS((const char *const string, langType language));
extern void freeKeywordTable __ARGS((void));
#ifdef DEBUG
extern void printKeywordTable __ARGS((void));
#endif

#endif	/* _KEYWORD_H */

/* vi:set tabstop=8 shiftwidth=4: */
