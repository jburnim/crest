/*****************************************************************************
*   $Id: main.h,v 8.5 2000/04/13 05:38:19 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   External interface to main.c
*****************************************************************************/
#ifndef _MAIN_H
#define _MAIN_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"
#include "vstring.h"
#include <stdio.h>

/*============================================================================
=   Data declarations
============================================================================*/
typedef int errorSelection;

enum eErrorTypes { FATAL = 1, WARNING = 2, PERROR = 4 };

/*============================================================================
=   Function prototypes
============================================================================*/
#ifdef NEED_PROTO_MALLOC
extern void *malloc __ARGS((size_t));
extern void *realloc __ARGS((void *ptr, size_t));
#endif

extern void error __ARGS((const errorSelection selection, const char *const format, ...));
extern FILE *tempFile __ARGS((const char *const mode, char **const pName));
extern boolean strequiv __ARGS((const char *const s1, const char *const s2));
extern void *eMalloc __ARGS((const size_t size));
extern void *eRealloc __ARGS((void *const ptr, const size_t size));
extern void eFree __ARGS((void *const ptr));
extern long unsigned int getFileSize __ARGS((const char *const name));
extern boolean doesFileExist __ARGS((const char *const fileName));
extern void addTotals __ARGS((const unsigned int files, const long unsigned int lines, const long unsigned int bytes));
extern const char *baseFilename __ARGS((const char *const filePath));
extern boolean isAbsolutePath __ARGS((const char *const path));
extern vString *combinePathAndFile __ARGS((const char *const path, const char *const file));
extern boolean isDestinationStdout __ARGS((void));
extern const char *getExecutableName __ARGS((void));
extern int main __ARGS((int argc, char **argv));

#endif	/* _MAIN_H */

/* vi:set tabstop=8 shiftwidth=4: */
