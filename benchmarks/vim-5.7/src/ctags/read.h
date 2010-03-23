/*****************************************************************************
*   $Id: read.h,v 8.3 2000/03/27 05:23:14 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   External interface to read.c
*****************************************************************************/
#ifndef _READ_H
#define _READ_H

#ifdef FILE_WRITE
# define CONST_FILE
#else
# define CONST_FILE const
#endif

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"
#include <stdio.h>
#include "ctags.h"
#include "vstring.h"

/*============================================================================
=   Macros
============================================================================*/
#define fileLanguage()		(File.language)
#define isLanguage(lang)	(boolean)((lang) == File.language)
#define getInputFileLine()	File.lineNumber
#define getFileName()		vStringValue(File.source.name)
#define getFileLine()		File.source.lineNumber
#define getFilePosition()	File.filePosition
#define isHeaderFile()		File.source.isHeader

/*============================================================================
=   Data declarations
============================================================================*/

/*  Maintains the state of the current source file.
 */
typedef struct sInputFile {
    vString	*name;		/* name of the input file */
    vString	*path;		/* the path of the input file (if any) */
    FILE	*fp;		/* stream used for reading the file */
    unsigned long lineNumber;	/* line number in the input file */
    fpos_t	filePosition;	/* file position of current line */
    int		ungetch;	/* a single character that was ungotten */
    boolean	newLine;	/* will the next character begin a new line? */
    langType	language;	/* language of input file */

    /*  Contains data pertaining to the original source file in which the tag
     *  was defined. This may be different from the input file when #line
     *  directives are processed (i.e. the input file is preprocessor output).
     */
    struct sSource {
	vString	*name;			/* name to report for source file */
	unsigned long lineNumber;	/* line number in the source file */
	boolean	 isHeader;		/* is source file a header file? */
	langType language;		/* language of source file */
    } source;
} inputFile;

/*============================================================================
=   Global variables
============================================================================*/
extern CONST_FILE inputFile File;

/*============================================================================
=   Function prototypes
============================================================================*/
#ifdef NEED_PROTO_FGETPOS
extern int fgetpos  __ARGS((FILE *stream, fpos_t *pos));
extern int fsetpos  __ARGS((FILE *stream, const fpos_t *pos));
#endif

extern void freeSourceFileResources __ARGS((void));
extern char *readLine __ARGS((vString *const vLine, FILE *const fp));
extern void setSourceFileName __ARGS((vString *const fileName));
extern void setSourceFileLine __ARGS((const long unsigned int lineNumber));
extern boolean fileOpen __ARGS((const char *const fileName, const langType language));
extern void fileClose __ARGS((void));
extern int fileGetc __ARGS((void));
extern void fileUngetc __ARGS((int c));
extern char *readSourceLine __ARGS((vString *const vLine, fpos_t location, long *const pSeekValue));

#endif	/* _READ_H */

/* vi:set tabstop=8 shiftwidth=4: */
