/*****************************************************************************
*   $Id: args.c,v 8.15 2000/03/18 04:27:40 darren Exp $
*
*   Copyright (c) 1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for reading command line arguments.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>
#include "args.h"
#include "main.h"
#include "debug.h"

/*============================================================================
=   Function prototypes
============================================================================*/
static char *nextStringArg __ARGS((const char** const next));
static char* nextStringLine __ARGS((const char** const next));
static char* nextString __ARGS((const Arguments* const current, const char** const next));
static char* nextFileArg __ARGS((FILE* const fp));
static char* nextFileLine __ARGS((FILE* const fp));
static char* nextFileString __ARGS((const Arguments* const current, FILE* const fp));

/*============================================================================
=   Function definitions
============================================================================*/

extern Arguments* argNewFromString( string )
    const char* const string;
{
    Arguments* result = (Arguments*)eMalloc(sizeof(Arguments));
    memset(result, 0, sizeof(Arguments));
    result->type = ARG_STRING;
    result->u.stringArgs.string = string;
    result->u.stringArgs.item = string;
    result->u.stringArgs.next = string;
    result->item = nextString(result, &result->u.stringArgs.next);
    return result;
}

extern Arguments* argNewFromArgv( argv )
    char* const* const argv;
{
    Arguments* result = (Arguments*)eMalloc(sizeof(Arguments));
    memset(result, 0, sizeof(Arguments));
    result->type = ARG_ARGV;
    result->u.argvArgs.argv = argv;
    result->u.argvArgs.item = result->u.argvArgs.argv;
    result->item = *result->u.argvArgs.item;
    return result;
}

extern Arguments* argNewFromFile( fp )
    FILE* const fp;
{
    Arguments* result = (Arguments*)eMalloc(sizeof(Arguments));
    memset(result, 0, sizeof(Arguments));
    result->type = ARG_FILE;
    result->u.fileArgs.fp = fp;
    result->item = nextFileString(result, result->u.fileArgs.fp);
    return result;
}

extern Arguments* argNewFromLineFile( fp )
    FILE* const fp;
{
    Arguments* result = (Arguments*)eMalloc(sizeof(Arguments));
    memset(result, 0, sizeof(Arguments));
    result->type = ARG_FILE;
    result->lineMode = TRUE;
    result->u.fileArgs.fp = fp;
    result->item = nextFileString(result, result->u.fileArgs.fp);
    return result;
}

static char* nextStringArg( next )
    const char** const next;
{
    char* result = NULL;
    const char* start;

    Assert(*next != NULL);
    for (start = *next  ;  isspace((int)*start)  ;  ++start)
	;
    if (*start == '\0')
	*next = start;
    else
    {
	size_t length;
	const char* end;

	for (end = start ;  *end != '\0'  &&  ! isspace((int)*end)  ;  ++end)
	    ;
	length = end - start;
	Assert(length > 0);
	result = (char*)eMalloc((size_t)(length + 1));
	strncpy(result, start, length);
	result[length] = '\0';
	*next = end;
    }
    return result;
}

static char* nextStringLine( next )
    const char** const next;
{
    char* result = NULL;
    size_t length;
    const char* end;

    Assert(*next != NULL);
    for (end = *next ;  *end != '\n'  &&  *end != '\0' ;  ++end)
	;
    length = end - *next;
    if (length > 0)
    {
	result = (char*)eMalloc((size_t)(length + 1));
	strncpy(result, *next, length);
	result[length] = '\0';
    }
    if (*end == '\n')
	++end;
    *next = end;
    return result;
}

static char* nextString( current, next )
    const Arguments* const current;
    const char** const next;
{
    char* result;
    if (current->lineMode)
	result = nextStringLine(next);
    else
	result = nextStringArg(next);
    return result;
}

static char* nextFileArg( fp )
    FILE* const fp;
{
    char* result = NULL;
    Assert(fp != NULL);
    if (! feof(fp))
    {
	vString* vs = vStringNew();
	int c;
	do
	    c = fgetc(fp);
	while (isspace(c));

	if (c != EOF)
	{
	    do
	    {
		vStringPut(vs, c);
		c = fgetc(fp);
	    } while (c != EOF  &&  ! isspace(c));
	    vStringTerminate(vs);
	    Assert(vStringLength(vs) > 0);
	    result = (char*)eMalloc((size_t)(vStringLength(vs) + 1));
	    strcpy(result, vStringValue(vs));
	}
	vStringDelete(vs);
    }
    return result;
}

static char* nextFileLine( fp )
    FILE* const fp;
{
    char* result = NULL;
    if (! feof(fp))
    {
	vString* vs = vStringNew();
	int c;

	Assert(fp != NULL);
	c = fgetc(fp);
	while (c != EOF  &&  c != '\n')
	{
	    vStringPut(vs, c);
	    c = fgetc(fp);
	}
	vStringTerminate(vs);
	if (vStringLength(vs) > 0)
	{
	    result = (char*)eMalloc((size_t)(vStringLength(vs) + 1));
	    strcpy(result, vStringValue(vs));
	}
	vStringDelete(vs);
    }
    return result;
}

static char* nextFileString( current, fp )
    const Arguments* const current;
    FILE* const fp;
{
    char* result;
    if (current->lineMode)
	result = nextFileLine(fp);
    else
	result = nextFileArg(fp);
    return result;
}

extern char* argItem( current )
    const Arguments* const current;
{
    Assert(current != NULL);
    Assert(! argOff(current));
    return current->item;
}

extern boolean argOff( current )
    const Arguments* const current;
{
    Assert(current != NULL);
    return (boolean)(current->item == NULL);
}

extern void argSetWordMode( current )
    Arguments* const current;
{
    Assert(current != NULL);
    current->lineMode = FALSE;
}

extern void argSetLineMode( current )
    Arguments* const current;
{
    Assert(current != NULL);
    current->lineMode = TRUE;
}

extern void argForth( current )
    Arguments* const current;
{
    Assert(current != NULL);
    Assert(! argOff(current));
    switch (current->type)
    {
	case ARG_STRING:
	    if (current->item != NULL)
		eFree(current->item);
	    current->u.stringArgs.item = current->u.stringArgs.next;
	    current->item = nextString(current, &current->u.stringArgs.next);
	    break;
	case ARG_ARGV:
	    ++current->u.argvArgs.item;
	    current->item = *current->u.argvArgs.item;
	    break;
	case ARG_FILE:
	    if (current->item != NULL)
		eFree(current->item);
	    current->item = nextFileString(current, current->u.fileArgs.fp);
	    break;
	default:
	    Assert("Invalid argument type" == NULL);
	    break;
    }
}

extern void argDelete( current )
    Arguments* const current;
{
    Assert(current != NULL);
    if (current->type ==  ARG_STRING  &&  current->item != NULL)
	eFree(current->item);
    memset(current, 0, sizeof(Arguments));
    eFree(current);
}
