/*****************************************************************************
*   $Id: args.h,v 8.9 1999/12/14 05:28:12 darren Exp $
*
*   Copyright (c) 1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   Defines external interface to command line argument reading.
*****************************************************************************/
#ifndef _ARGS_H
#define _ARGS_H

/*============================================================================
=   Include files
============================================================================*/
#include <stdio.h>
#include "general.h"

/*============================================================================
=   Data declarations
============================================================================*/

typedef struct sArgs {
    enum { ARG_NONE, ARG_STRING, ARG_ARGV, ARG_FILE } type;
    union {
	struct sStringArgs {
	    const char* string;
	    const char* next;
	    const char* item;
	} stringArgs;
	struct sArgvArgs {
	    char* const* argv;
	    char* const* item;
	} argvArgs;
	struct sFileArgs {
	    FILE* fp;
	} fileArgs;
    } u;
    char* item;
    boolean lineMode;
} Arguments;

/*============================================================================
=   Function prototypes
============================================================================*/
extern Arguments* argNewFromString __ARGS((const char* const string));
extern Arguments* argNewFromArgv __ARGS((char* const* const argv));
extern Arguments* argNewFromFile __ARGS((FILE* const fp));
extern Arguments* argNewFromLineFile __ARGS((FILE* const fp));
extern char *argItem __ARGS((const Arguments* const current));
extern boolean argOff __ARGS((const Arguments* const current));
extern void argSetWordMode __ARGS((Arguments* const current));
extern void argSetLineMode __ARGS((Arguments* const current));
extern void argForth __ARGS((Arguments* const current));
extern void argDelete __ARGS((Arguments* const current));

#endif	/* _ARGS_H */

/* vi:set tabstop=8 shiftwidth=4: */
