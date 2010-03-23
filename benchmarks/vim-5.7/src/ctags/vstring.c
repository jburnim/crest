/*****************************************************************************
*   $Id: vstring.c,v 8.2 1999/09/29 02:20:25 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions supporting resizeable strings.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <limits.h>	/* to define INT_MAX */
#include <string.h>

#include "vstring.h"
#include "debug.h"
#include "main.h"

/*============================================================================
=   Data definitions
============================================================================*/
const size_t vStringInitialSize = 32;

/*============================================================================
=   Function prototypes
============================================================================*/
static void vStringResize __ARGS((vString *const string, const size_t newSize));

/*============================================================================
=   Function definitions
============================================================================*/

static void vStringResize( string, newSize )
    vString *const string;
    const size_t newSize;
{
    char *const newBuffer = (char *)eRealloc(string->buffer, newSize);

    string->size = newSize;
    string->buffer = newBuffer;
}

/*----------------------------------------------------------------------------
*-	External interface
----------------------------------------------------------------------------*/

extern boolean vStringAutoResize( string )
    vString *const string;
{
    boolean ok = TRUE;

    if (string->size <= INT_MAX / 2)
    {
	const size_t newSize = string->size * 2;

	vStringResize(string, newSize);
    }
    return ok;
}

extern void vStringClear( string )
    vString *const string;
{
    string->length = 0;
    string->buffer[0] = '\0';
    DebugStatement( clearString(string->buffer, string->size); )
}

extern void vStringDelete( string )
    vString *const string;
{
    if (string != NULL)
    {
	if (string->buffer != NULL)
	    eFree(string->buffer);
	eFree(string);
    }
}

extern vString *vStringNew()
{
    vString *const string = (vString *)eMalloc(sizeof(vString));

    string->length = 0;
    string->size   = vStringInitialSize;
    string->buffer = (char *)eMalloc(string->size);

    vStringClear(string);

    return string;
}

extern void vStringPut( string, c )
    vString *const string;
    const int c;
{
    if (string->length == string->size)		/*  check for buffer overflow */
	vStringAutoResize(string);

    string->buffer[string->length] = c;
    if (c != '\0')
	string->length++;
}

extern void vStringCatS( string, s )
    vString *const string;
    const char *const s;
{
    const char *p = s;

    do
	vStringPut(string, *p);
    while (*p++ != '\0');
}

extern vString *vStringNewInit( s )
    const char *const s;
{
    vString *new = vStringNew();

    vStringCatS(new, s);

    return new;
}

extern void vStringNCatS( string, s, length )
    vString *const string;
    const char *const s;
    const size_t length;
{
    const char *p = s;
    size_t remain = length;

    while (*p != '\0'  &&  remain > 0)
    {
	vStringPut(string, *p);
	--remain;
	++p;
    }
    vStringTerminate(string);
}

/*  Strip trailing spaces from string.
 */
extern void vStringStrip( string )
    vString *const string;
{
    while (string->length > 0  &&  string->buffer[string->length - 1] == ' ')
    {
	string->length--;
	string->buffer[string->length] = '\0';
    }
}

extern void vStringCopyS( string, s )
    vString *const string;
    const char *const s;
{
    vStringClear(string);
    vStringCatS(string, s);
}

extern void vStringNCopyS( string, s, length )
    vString *const string;
    const char *const s;
    const size_t length;
{
    vStringClear(string);
    vStringNCatS(string, s, length);
}

extern void vStringCopyToLower( dest, src )
    vString *const dest;
    vString *const src;
{
    const size_t length = src->length;
    const char *s = src->buffer;
    char *d;
    size_t i;

    if (dest->size < src->size)
	vStringResize(dest, src->size);
    d = dest->buffer;
    for (i = 0  ;  i < length  ;  ++i)
    {
	int c = s[i];

	d[i] = tolower(c);
    }
    d[i] = '\0';
}

extern void vStringSetLength( string )
    vString *const string;
{
    string->length = strlen(string->buffer);
}
