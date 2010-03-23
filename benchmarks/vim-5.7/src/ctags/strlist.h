/*****************************************************************************
*   $Id: strlist.h,v 8.2 1999/09/17 07:15:31 darren Exp $
*
*   Copyright (c) 1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   Defines external interface to resizable string lists.
*****************************************************************************/
#ifndef _STRLIST_H
#define _STRLIST_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include "vstring.h"

/*============================================================================
=   Data declarations
============================================================================*/
typedef struct sStringList {
    unsigned int max;
    unsigned int count;
    vString **list;
} stringList;

/*============================================================================
=   Function prototypes
============================================================================*/
extern stringList *stringListNew __ARGS((void));
extern void stringListAdd __ARGS((stringList *const current, vString *string));
extern stringList* stringListNewFromArgv __ARGS((const char* const* const list));
extern void stringListClear __ARGS((stringList *const current));
extern unsigned int stringListCount __ARGS((stringList *const current));
extern vString* stringListItem __ARGS((stringList *const current, const unsigned int indx));
extern void stringListDelete __ARGS((stringList *const current));
extern boolean stringListHasInsensitive __ARGS((stringList *const current, const char *const string));
extern boolean stringListHas __ARGS((stringList *const current, const char *const string));
extern void stringListPrint __ARGS((stringList *const current));

#endif	/* _STRLIST_H */

/* vi:set tabstop=8 shiftwidth=4: */
