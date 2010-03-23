/*****************************************************************************
*   $Id: strlist.c,v 8.4 1999/09/29 02:20:25 darren Exp $
*
*   Copyright (c) 1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions managing resizable string lists.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>
#include "strlist.h"
#include "main.h"
#include "debug.h"

/*============================================================================
=   Function definitions
============================================================================*/

extern stringList* stringListNew()
{
    stringList* const result = (stringList*)eMalloc(sizeof(stringList));
    result->max   = 0;
    result->count = 0;
    result->list  = NULL;
    return result;
}

extern stringList* stringListNewFromArgv( argv )
    const char* const* const argv;
{
    stringList* const result = stringListNew();
    const char *const *p;
    Assert(argv != NULL);
    for (p = argv  ;  *p != NULL  ;  ++p)
	stringListAdd(result, vStringNewInit(*p));
    return result;
}

extern void stringListAdd( current, string )
    stringList *const current;
    vString *string;
{
    enum { incrementalIncrease = 10 };
    Assert(current != NULL);
    if (current->list == NULL)
    {
	Assert(current->max == 0);
	current->count = 0;
	current->max   = incrementalIncrease;
	current->list  = (vString **)eMalloc((size_t)current->max *
					   sizeof(vString *));
    }
    else if (current->count == current->max)
    {
	current->max += incrementalIncrease;
	current->list = (vString **)eRealloc(current->list,
				    (size_t)current->max * sizeof(vString *));
    }
    current->list[current->count++] = string;
}

extern unsigned int stringListCount( current )
    stringList *const current;
{
    Assert(current != NULL);
    return current->count;
}

extern vString* stringListItem( current, indx )
    stringList *const current;
    const unsigned int indx;
{
    Assert(current != NULL);
    return current->list[indx];
}

extern void stringListClear( current )
    stringList *const current;
{
    unsigned int i;
    Assert(current != NULL);
    for (i = 0  ;  i < current->count  ;  ++i)
    {
	vStringDelete(current->list[i]);
	current->list[i] = NULL;
    }
    current->count = 0;
}

extern void stringListDelete( current )
    stringList* const current;
{
    if (current != NULL)
    {
	if (current->list != NULL)
	{
	    stringListClear(current);
	    eFree(current->list);
	    current->list = NULL;
	}
	current->max   = 0;
	current->count = 0;
	eFree(current);
    }
}

extern boolean stringListHas( current, string)
    stringList *const current;
    const char *const string;
{
    boolean result = FALSE;
    unsigned int i;
    Assert(current != NULL);
    for (i = 0  ;  ! result  &&  i < current->count  ;  ++i)
    {
	if (strcmp(string, vStringValue(current->list[i])) == 0)
	    result = TRUE;
    }
    return result;
}

extern boolean stringListHasInsensitive( current, string)
    stringList *const current;
    const char *const string;
{
    boolean result = FALSE;
    unsigned int i;
    Assert(current != NULL);
    for (i = 0  ;  ! result  &&  i < current->count  ;  ++i)
    {
	if (strequiv(string, vStringValue(current->list[i])))
	    result = TRUE;
    }
    return result;
}

extern void stringListPrint( current )
    stringList *const current;
{
    unsigned int i;
    Assert(current != NULL);
    for (i = 0  ;  i < current->count  ;  ++i)
	printf("%s%s", (i > 0) ? ", " : "", vStringValue(current->list[i]));
}
